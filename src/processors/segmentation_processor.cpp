#include "../../include/processors/segmentation_processor.hpp"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

namespace SegmentationProcessor {

    void meanShiftOpenCV(Image& img, int spatialBandwidth, int colorBandwidth, bool useLUV) {
        if (img.mat.empty()) return;

        cv::Mat src = img.mat.clone();
        if (useLUV) {
            cv::cvtColor(src, src, cv::COLOR_BGR2Luv);
        }

        cv::Mat result;
        int sp = std::max(1, spatialBandwidth);
        int sr = std::max(1, colorBandwidth);
        
        cv::pyrMeanShiftFiltering(src, result, sp, sr);
        
        if (useLUV) {
            cv::cvtColor(result, result, cv::COLOR_Luv2BGR);
        }
        
        img.store("mean_shift_opencv", result);
    }

    struct Cluster {
        double c1, c2, c3; // channels (BGR or Luv)
        double x, y;
    };

    void meanShiftScratch(Image& img, int spatialBandwidth, int colorBandwidth, bool useLUV) {
        if (img.mat.empty()) return;

        cv::Mat src = img.mat.clone();
        if (useLUV) {
            cv::cvtColor(src, src, cv::COLOR_BGR2Luv);
        }
        
        cv::Mat result = src.clone();
        
        int sp = std::max(1, spatialBandwidth);
        int sr = std::max(1, colorBandwidth);
        double sp_sq = sp * sp;
        double sr_sq = sr * sr;
        
        cv::Mat visited = cv::Mat::zeros(src.size(), CV_8UC1);
        int total_unvisited = src.rows * src.cols;
        
        std::vector<Cluster> clusters;
        std::vector<cv::Point> tracked_points;
        tracked_points.reserve(src.rows * src.cols);

        while (total_unvisited > 0) {
            int start_x = -1, start_y = -1;
            for (int i = 0; i < src.rows; ++i) {
                const uchar* v_ptr = visited.ptr<uchar>(i);
                for (int j = 0; j < src.cols; ++j) {
                    if (v_ptr[j] == 0) {
                        start_y = i;
                        start_x = j;
                        break;
                    }
                }
                if (start_y != -1) break;
            }
            
            if (start_x == -1) break; // Should not happen
            
            double mean_x = start_x;
            double mean_y = start_y;
            cv::Vec3b start_color = src.at<cv::Vec3b>(start_y, start_x);
            double mean_c1 = start_color[0];
            double mean_c2 = start_color[1];
            double mean_c3 = start_color[2];
            
            tracked_points.clear();
            
            int iter = 0;
            while (iter++ < 30) { // Limit iterations to avoid infinite loops
                double new_mean_x = 0, new_mean_y = 0;
                double new_mean_c1 = 0, new_mean_c2 = 0, new_mean_c3 = 0;
                int count = 0;
                
                int min_y = std::max(0, (int)mean_y - sp);
                int max_y = std::min(src.rows - 1, (int)mean_y + sp);
                int min_x = std::max(0, (int)mean_x - sp);
                int max_x = std::min(src.cols - 1, (int)mean_x + sp);
                
                for (int y = min_y; y <= max_y; ++y) {
                    for (int x = min_x; x <= max_x; ++x) {
                        double spatial_dist_sq = (x - mean_x)*(x - mean_x) + (y - mean_y)*(y - mean_y);
                        if (spatial_dist_sq <= sp_sq) {
                            cv::Vec3b color = src.at<cv::Vec3b>(y, x);
                            double color_dist_sq = 
                                (color[0] - mean_c1)*(color[0] - mean_c1) +
                                (color[1] - mean_c2)*(color[1] - mean_c2) +
                                (color[2] - mean_c3)*(color[2] - mean_c3);
                            
                            if (color_dist_sq <= sr_sq) {
                                new_mean_x += x;
                                new_mean_y += y;
                                new_mean_c1 += color[0];
                                new_mean_c2 += color[1];
                                new_mean_c3 += color[2];
                                count++;
                                
                                // Accumulate basin of attraction
                                if (visited.at<uchar>(y, x) == 0) {
                                    tracked_points.push_back(cv::Point(x, y));
                                    visited.at<uchar>(y, x) = 2; // Temp visited
                                }
                            }
                        }
                    }
                }
                
                if (count == 0) break;
                
                new_mean_x /= count;
                new_mean_y /= count;
                new_mean_c1 /= count;
                new_mean_c2 /= count;
                new_mean_c3 /= count;
                
                double dist_sq = 
                    (new_mean_x - mean_x)*(new_mean_x - mean_x) +
                    (new_mean_y - mean_y)*(new_mean_y - mean_y) +
                    (new_mean_c1 - mean_c1)*(new_mean_c1 - mean_c1) +
                    (new_mean_c2 - mean_c2)*(new_mean_c2 - mean_c2) +
                    (new_mean_c3 - mean_c3)*(new_mean_c3 - mean_c3);
                
                // Convergence threshold
                if (dist_sq < 1.0 || iter == 30) {
                    bool merged = false;
                    for (auto& c : clusters) {
                        double spatial_c_dist_sq = (c.x - new_mean_x)*(c.x - new_mean_x) + (c.y - new_mean_y)*(c.y - new_mean_y);
                        double color_c_dist_sq = (c.c1 - new_mean_c1)*(c.c1 - new_mean_c1) + (c.c2 - new_mean_c2)*(c.c2 - new_mean_c2) + (c.c3 - new_mean_c3)*(c.c3 - new_mean_c3);
                        
                        if (spatial_c_dist_sq < 0.25 * sp_sq && color_c_dist_sq < 0.25 * sr_sq) {
                            c.x = 0.5 * (c.x + new_mean_x);
                            c.y = 0.5 * (c.y + new_mean_y);
                            c.c1 = 0.5 * (c.c1 + new_mean_c1);
                            c.c2 = 0.5 * (c.c2 + new_mean_c2);
                            c.c3 = 0.5 * (c.c3 + new_mean_c3);
                            
                            for (const auto& pt : tracked_points) {
                                visited.at<uchar>(pt.y, pt.x) = 1;
                                result.at<cv::Vec3b>(pt.y, pt.x) = cv::Vec3b(static_cast<uchar>(c.c1), static_cast<uchar>(c.c2), static_cast<uchar>(c.c3));
                            }
                            total_unvisited -= tracked_points.size();
                            merged = true;
                            break;
                        }
                    }
                    
                    if (!merged) {
                        Cluster new_cluster = {new_mean_c1, new_mean_c2, new_mean_c3, new_mean_x, new_mean_y};
                        clusters.push_back(new_cluster);
                        for (const auto& pt : tracked_points) {
                            visited.at<uchar>(pt.y, pt.x) = 1;
                            result.at<cv::Vec3b>(pt.y, pt.x) = cv::Vec3b(static_cast<uchar>(new_mean_c1), static_cast<uchar>(new_mean_c2), static_cast<uchar>(new_mean_c3));
                        }
                        total_unvisited -= tracked_points.size();
                    }
                    break;
                } else {
                    mean_x = new_mean_x;
                    mean_y = new_mean_y;
                    mean_c1 = new_mean_c1;
                    mean_c2 = new_mean_c2;
                    mean_c3 = new_mean_c3;
                }
            }
        }
        
        if (useLUV) {
            cv::cvtColor(result, result, cv::COLOR_Luv2BGR);
        }
        
        img.store("mean_shift_scratch", result);
    }
}

