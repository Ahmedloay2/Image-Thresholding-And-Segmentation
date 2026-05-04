#include "processors/kmeans_processor.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

#include <vector>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <numeric>

// ── OpenMP (already required by the project CMake) ───────────────────────────
#ifdef _OPENMP
#  include <omp.h>
#endif

namespace KMeansProcessor {

// ── Internal types ────────────────────────────────────────────────────────────

/**
 * @brief A 5-D feature vector: [r, g, b, x_weighted, y_weighted].
 *        In RGB mode only the first 3 dimensions are used.
 */
struct Feature {
    float v[5] = {0, 0, 0, 0, 0};
};

// ── Private helpers ───────────────────────────────────────────────────────────
namespace {

/**
 * @brief Euclidean distance squared between two feature vectors.
 * @param dims Number of active dimensions (3 for RGB, 5 for RGB_XY).
 */
inline float distSq(const Feature& a, const Feature& b, int dims)
{
    float s = 0.0f;
    for (int d = 0; d < dims; ++d) {
        float diff = a.v[d] - b.v[d];
        s += diff * diff;
    }
    return s;
}

/**
 * @brief Build the flat feature array from the source image.
 *
 * RGB mode  : v = [r, g, b, 0, 0]
 * RGB_XY    : v = [r, g, b, (x/W)*xyWeight, (y/H)*xyWeight]
 *
 * Spatial coords are normalised to [0, xyWeight] so that the magnitude
 * is comparable to colour values (0-255 range).
 */
std::vector<Feature> buildFeatures(const cv::Mat& src,
                                   Space          space,
                                   float          xyWeight)
{
    const int W = src.cols;
    const int H = src.rows;
    const int N = W * H;
    std::vector<Feature> features(N);

    const bool useXY = (space == Space::RGB_XY);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        const cv::Vec3b* row = src.ptr<cv::Vec3b>(y);
        for (int x = 0; x < W; ++x) {
            Feature& f = features[y * W + x];
            f.v[0] = static_cast<float>(row[x][2]); // R
            f.v[1] = static_cast<float>(row[x][1]); // G
            f.v[2] = static_cast<float>(row[x][0]); // B
            if (useXY) {
                f.v[3] = (static_cast<float>(x) / W) * xyWeight;
                f.v[4] = (static_cast<float>(y) / H) * xyWeight;
            }
        }
    }
    return features;
}

/**
 * @brief k-Means++ initialisation.
 *
 * Picks k centroids from the feature array such that each successive
 * centroid is chosen with probability proportional to its squared
 * distance from the nearest already-chosen centroid.
 * This dramatically reduces the chance of poor convergence.
 *
 * Complexity: O(n · k)
 */
std::vector<Feature> kMeansPlusPlus(const std::vector<Feature>& features,
                                    int                          k,
                                    int                          dims,
                                    std::mt19937&                rng)
{
    const int N = static_cast<int>(features.size());
    std::vector<Feature> centroids;
    centroids.reserve(k);

    // 1. Choose first centroid uniformly at random.
    std::uniform_int_distribution<int> uniformDist(0, N - 1);
    centroids.push_back(features[uniformDist(rng)]);

    // 2. For each subsequent centroid, use D² weighting.
    std::vector<float> dists(N, std::numeric_limits<float>::max());

    for (int c = 1; c < k; ++c) {
        // Update minimum distances to the newest centroid.
        const Feature& newest = centroids.back();
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < N; ++i) {
            float d = distSq(features[i], newest, dims);
            if (d < dists[i]) dists[i] = d;
        }

        // Weighted random selection.
        float total = std::accumulate(dists.begin(), dists.end(), 0.0f);
        std::uniform_real_distribution<float> realDist(0.0f, total);
        float pick = realDist(rng);
        float cumul = 0.0f;
        int chosen = 0;
        for (int i = 0; i < N; ++i) {
            cumul += dists[i];
            if (cumul >= pick) { chosen = i; break; }
        }
        centroids.push_back(features[chosen]);
    }
    return centroids;
}

/**
 * @brief Assign each pixel to its nearest centroid.
 * @return true if any assignment changed (used for early exit).
 */
bool assignStep(const std::vector<Feature>& features,
                const std::vector<Feature>& centroids,
                std::vector<int>&           labels,
                int                         dims)
{
    const int N = static_cast<int>(features.size());
    const int K = static_cast<int>(centroids.size());
    bool changed = false;

    #pragma omp parallel for schedule(static) reduction(||:changed)
    for (int i = 0; i < N; ++i) {
        float  best  = std::numeric_limits<float>::max();
        int    bestK = 0;
        for (int c = 0; c < K; ++c) {
            float d = distSq(features[i], centroids[c], dims);
            if (d < best) { best = d; bestK = c; }
        }
        if (labels[i] != bestK) {
            labels[i] = bestK;
            changed = true;
        }
    }
    return changed;
}

/**
 * @brief Recompute centroids as the mean of their assigned pixels.
 * @return Maximum centroid shift (used for epsilon convergence test).
 */
float updateStep(const std::vector<Feature>& features,
                 const std::vector<int>&     labels,
                 std::vector<Feature>&       centroids,
                 int                         dims)
{
    const int N = static_cast<int>(features.size());
    const int K = static_cast<int>(centroids.size());

    // Accumulate sums and counts per cluster.
    std::vector<std::array<double, 5>> sums(K);
    std::vector<int> counts(K, 0);
    for (auto& s : sums) s.fill(0.0);

    for (int i = 0; i < N; ++i) {
        int c = labels[i];
        for (int d = 0; d < dims; ++d)
            sums[c][d] += features[i].v[d];
        counts[c]++;
    }

    float maxShift = 0.0f;
    for (int c = 0; c < K; ++c) {
        if (counts[c] == 0) continue;
        float shift = 0.0f;
        for (int d = 0; d < dims; ++d) {
            float newVal = static_cast<float>(sums[c][d] / counts[c]);
            float diff   = newVal - centroids[c].v[d];
            shift += diff * diff;
            centroids[c].v[d] = newVal;
        }
        maxShift = std::max(maxShift, std::sqrt(shift));
    }
    return maxShift;
}

/**
 * @brief Render the label map to a false-colour BGR image.
 *
 * Each cluster gets a unique, visually distinct colour by sampling
 * evenly-spaced hues in HSV and converting to BGR.
 */
cv::Mat renderLabels(const std::vector<int>& labels,
                     int                     W,
                     int                     H,
                     int                     K)
{
    // Generate K distinct colours.
    std::vector<cv::Vec3b> palette(K);
    for (int c = 0; c < K; ++c) {
        // Spread hues evenly; use full saturation and value for visibility.
        cv::Mat hsv(1, 1, CV_8UC3,
                    cv::Scalar(static_cast<int>((180.0 * c) / K), 220, 210));
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
        palette[c] = bgr.at<cv::Vec3b>(0, 0);
    }

    cv::Mat result(H, W, CV_8UC3);
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        cv::Vec3b* row = result.ptr<cv::Vec3b>(y);
        for (int x = 0; x < W; ++x) {
            row[x] = palette[labels[y * W + x]];
        }
    }
    return result;
}

} // anonymous namespace

// ── Public API ────────────────────────────────────────────────────────────────

void kMeansScratch(Image& img,
                   int    k,
                   Space  space,
                   float  xyWeight,
                   int    maxIter,
                   float  epsilon)
{
    if (img.mat.empty()) return;
    if (k < 1) k = 1;

    const int W    = img.mat.cols;
    const int H    = img.mat.rows;
    const int N    = W * H;
    const int dims = (space == Space::RGB_XY) ? 5 : 3;

    // ── 1. Build feature vectors ──────────────────────────────────────────────
    std::vector<Feature> features = buildFeatures(img.mat, space, xyWeight);

    // ── 2. Initialise centroids with k-Means++ ───────────────────────────────
    std::mt19937 rng(42); // fixed seed for reproducibility
    std::vector<Feature> centroids = kMeansPlusPlus(features, k, dims, rng);

    // ── 3. EM loop ────────────────────────────────────────────────────────────
    std::vector<int> labels(N, 0);

    for (int iter = 0; iter < maxIter; ++iter) {
        bool changed = assignStep(features, centroids, labels, dims);
        if (!changed) break;                       // no pixel moved → converged

        float shift = updateStep(features, labels, centroids, dims);
        if (shift < epsilon) break;                // centroids barely moved
    }

    // ── 4. Render and store ───────────────────────────────────────────────────
    cv::Mat result = renderLabels(labels, W, H, k);
    img.store("kmeans_scratch", result);
}

} // namespace KMeansProcessor