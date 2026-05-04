#include "../../include/processors/agglomerative_processor.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <limits>

namespace AgglomerativeProcessor {

    struct ClusterFeature {
        double r, g, b;
        double x, y;
        int size;
    };

    void agglomerativeScratch(Image& img, int targetClusters, bool useLUV) {
        if (img.mat.empty()) return;
        
        cv::Mat src = img.mat.clone();
        if (useLUV) {
            cv::cvtColor(src, src, cv::COLOR_BGR2Luv);
        }
        
        // Downsample to make N^3 feasible (N=1600)
        int N_side = 40; 
        cv::Mat small_img;
        cv::resize(src, small_img, cv::Size(N_side, N_side), 0, 0, cv::INTER_AREA);
        
        int N = small_img.rows * small_img.cols;
        if (targetClusters >= N) targetClusters = N - 1;
        if (targetClusters < 1) targetClusters = 1;
        
        std::vector<ClusterFeature> features(N * 2);
        std::vector<bool> active(N * 2, false);
        
        double spatial_weight = 50.0; // Scale spatial coordinates to balance with color
        
        for (int i = 0; i < N; ++i) {
            int y = i / small_img.cols;
            int x = i % small_img.cols;
            cv::Vec3b color = small_img.at<cv::Vec3b>(y, x);
            
            features[i].r = color[2]; 
            features[i].g = color[1];
            features[i].b = color[0];
            features[i].x = (double(x) / small_img.cols) * spatial_weight;
            features[i].y = (double(y) / small_img.rows) * spatial_weight;
            features[i].size = 1;
            active[i] = true;
        }
        
        auto get_dist = [&](int i, int j) {
            double dr = features[i].r - features[j].r;
            double dg = features[i].g - features[j].g;
            double db = features[i].b - features[j].b;
            double dx = features[i].x - features[j].x;
            double dy = features[i].y - features[j].y;
            return std::sqrt(dr*dr + dg*dg + db*db + dx*dx + dy*dy);
        };
        
        std::vector<std::vector<double>> dist(N * 2, std::vector<double>(N * 2, std::numeric_limits<double>::max()));
        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                double d = get_dist(i, j);
                dist[i][j] = d;
                dist[j][i] = d;
            }
        }
        
        int current_clusters = N;
        int next_cluster_id = N;
        
        std::vector<int> final_labels(N);
        std::vector<int> parent(N * 2);
        for (int i = 0; i < N * 2; ++i) parent[i] = i;
        
        auto find_root = [&](int i, auto& find_root_ref) -> int {
            if (parent[i] == i) return i;
            return parent[i] = find_root_ref(parent[i], find_root_ref);
        };

        // Cache the minimum distance for each row to speed up the O(N^3) bottleneck
        std::vector<int> min_idx(N * 2, -1);
        auto update_min_idx = [&](int i) {
            double m = std::numeric_limits<double>::max();
            int best = -1;
            for (int j = 0; j < next_cluster_id; ++j) {
                if (i != j && active[j]) {
                    if (dist[i][j] < m) {
                        m = dist[i][j];
                        best = j;
                    }
                }
            }
            min_idx[i] = best;
        };
        
        for (int i = 0; i < N; ++i) {
            update_min_idx(i);
        }

        // Standard Agglomerative Loop
        while (current_clusters > 1) {
            double min_d = std::numeric_limits<double>::max();
            int best_i = -1, best_j = -1;
            
            for (int i = 0; i < next_cluster_id; ++i) {
                if (!active[i]) continue;
                int j = min_idx[i];
                if (j != -1 && active[j]) {
                    if (dist[i][j] < min_d) {
                        min_d = dist[i][j];
                        best_i = i;
                        best_j = j;
                    }
                }
            }
            
            if (best_i == -1) break;
            
            int new_cluster = next_cluster_id++;
            active[best_i] = false;
            active[best_j] = false;
            active[new_cluster] = true;
            
            int s_i = features[best_i].size;
            int s_j = features[best_j].size;
            features[new_cluster].size = s_i + s_j;
            features[new_cluster].r = (features[best_i].r * s_i + features[best_j].r * s_j) / (s_i + s_j);
            features[new_cluster].g = (features[best_i].g * s_i + features[best_j].g * s_j) / (s_i + s_j);
            features[new_cluster].b = (features[best_i].b * s_i + features[best_j].b * s_j) / (s_i + s_j);
            features[new_cluster].x = (features[best_i].x * s_i + features[best_j].x * s_j) / (s_i + s_j);
            features[new_cluster].y = (features[best_i].y * s_i + features[best_j].y * s_j) / (s_i + s_j);
            
            parent[best_i] = new_cluster;
            parent[best_j] = new_cluster;
            
            // Average Linkage (UPGMA): Update distances to new cluster
            for (int k = 0; k < new_cluster; ++k) {
                if (active[k]) {
                    double d = (dist[best_i][k] * s_i + dist[best_j][k] * s_j) / (s_i + s_j);
                    dist[new_cluster][k] = d;
                    dist[k][new_cluster] = d;
                    
                    // If k's nearest neighbor was best_i or best_j, or new_cluster is closer, update k's min_idx
                    if (min_idx[k] == best_i || min_idx[k] == best_j || d < dist[k][min_idx[k]]) {
                        update_min_idx(k);
                    }
                }
            }
            update_min_idx(new_cluster);
            
            current_clusters--;
            
            if (current_clusters == targetClusters) {
                for (int i = 0; i < N; ++i) {
                    final_labels[i] = find_root(i, find_root);
                }
            }
        }
        
        if (targetClusters == 1) {
            for (int i = 0; i < N; ++i) {
                final_labels[i] = find_root(i, find_root);
            }
        }
        
        std::vector<int> unique_labels;
        for (int i = 0; i < N; ++i) {
            int l = final_labels[i];
            if (std::find(unique_labels.begin(), unique_labels.end(), l) == unique_labels.end()) {
                unique_labels.push_back(l);
            }
            final_labels[i] = std::distance(unique_labels.begin(), std::find(unique_labels.begin(), unique_labels.end(), l));
        }
        
        // Final centroids
        std::vector<ClusterFeature> final_centroids(unique_labels.size());
        for (size_t i = 0; i < unique_labels.size(); ++i) {
            int root = unique_labels[i];
            final_centroids[i] = features[root];
        }
        
        std::vector<cv::Vec3b> colors(unique_labels.size());
        cv::RNG rng(12345);
        for (size_t i = 0; i < unique_labels.size(); ++i) {
            colors[i] = cv::Vec3b(rng.uniform(50, 255), rng.uniform(50, 255), rng.uniform(50, 255));
        }
        
        // Full resolution assignment
        cv::Mat result(src.size(), CV_8UC3);
        
        for (int y = 0; y < src.rows; ++y) {
            for (int x = 0; x < src.cols; ++x) {
                cv::Vec3b color = src.at<cv::Vec3b>(y, x);
                double r = color[2];
                double g = color[1];
                double b = color[0];
                double fx = (double(x) / src.cols) * spatial_weight;
                double fy = (double(y) / src.rows) * spatial_weight;
                
                int best_c = 0;
                double min_dist = std::numeric_limits<double>::max();
                for (size_t c = 0; c < final_centroids.size(); ++c) {
                    double dr = r - final_centroids[c].r;
                    double dg = g - final_centroids[c].g;
                    double db = b - final_centroids[c].b;
                    double dx = fx - final_centroids[c].x;
                    double dy = fy - final_centroids[c].y;
                    double d = dr*dr + dg*dg + db*db + dx*dx + dy*dy;
                    if (d < min_dist) {
                        min_dist = d;
                        best_c = c;
                    }
                }
                
                result.at<cv::Vec3b>(y, x) = colors[best_c];
            }
        }
        
        img.store("agglomerative_scratch", result);
    }
}
