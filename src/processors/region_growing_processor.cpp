#include "processors/region_growing_processor.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

#include <vector>
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>

namespace RegionGrowingProcessor {

// ── Private helpers ───────────────────────────────────────────────────────────
namespace {

/**
 * @brief Euclidean distance in RGB space between two BGR pixels.
 *        (OpenCV stores pixels as BGR, so index 2=R, 1=G, 0=B)
 */
inline float colorDist(const cv::Vec3b& a, const cv::Vec3b& b)
{
    float dr = static_cast<float>(a[2]) - static_cast<float>(b[2]);
    float dg = static_cast<float>(a[1]) - static_cast<float>(b[1]);
    float db = static_cast<float>(a[0]) - static_cast<float>(b[0]);
    return std::sqrt(dr * dr + dg * dg + db * db);
}

/**
 * @brief Render the integer label map to a false-colour BGR image.
 *        Label -1 (unlabelled) → black.
 */
cv::Mat renderLabels(const cv::Mat& labelMap, int numRegions)
{
    const int H = labelMap.rows;
    const int W = labelMap.cols;

    // Generate one distinct colour per region using evenly-spread HSV hues.
    std::vector<cv::Vec3b> palette(numRegions);
    for (int i = 0; i < numRegions; ++i) {
        cv::Mat hsv(1, 1, CV_8UC3,
                    cv::Scalar(static_cast<int>((180.0 * i) / numRegions), 220, 210));
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
        palette[i] = bgr.at<cv::Vec3b>(0, 0);
    }

    cv::Mat result(H, W, CV_8UC3, cv::Scalar(0, 0, 0)); // black = unlabelled
    for (int y = 0; y < H; ++y) {
        const int*  lrow = labelMap.ptr<int>(y);
        cv::Vec3b*  rrow = result.ptr<cv::Vec3b>(y);
        for (int x = 0; x < W; ++x) {
            int lbl = lrow[x];
            if (lbl >= 0) rrow[x] = palette[lbl];
        }
    }
    return result;
}

/**
 * @brief Cleanup pass: assign every unlabelled pixel (label == -1) to the
 *        nearest already-labelled pixel, using a second BFS flood from all
 *        labelled pixels simultaneously.
 *
 * This is equivalent to a nearest-labelled-neighbour assignment and runs in O(N).
 */
void fillUnlabelled(cv::Mat& labelMap)
{
    const int H = labelMap.rows;
    const int W = labelMap.cols;

    // Seed the queue with every currently labelled pixel.
    std::queue<cv::Point> q;
    for (int y = 0; y < H; ++y) {
        const int* row = labelMap.ptr<int>(y);
        for (int x = 0; x < W; ++x) {
            if (row[x] >= 0) q.push({x, y});
        }
    }

    const int dx[4] = {1, -1, 0,  0};
    const int dy[4] = {0,  0, 1, -1};

    while (!q.empty()) {
        cv::Point p = q.front(); q.pop();
        int lbl = labelMap.at<int>(p);
        for (int d = 0; d < 4; ++d) {
            int nx = p.x + dx[d];
            int ny = p.y + dy[d];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            if (labelMap.at<int>(ny, nx) == -1) {
                labelMap.at<int>(ny, nx) = lbl;
                q.push({nx, ny});
            }
        }
    }
}

} // anonymous namespace

// ── Public API ────────────────────────────────────────────────────────────────

void regionGrowingScratch(Image&                        img,
                          const std::vector<cv::Point>& seeds,
                          float                         threshold)
{
    if (img.mat.empty() || seeds.empty()) return;

    const int W = img.mat.cols;
    const int H = img.mat.rows;

    // ── 1. Deduplicate seeds and clamp to image bounds ────────────────────────
    std::vector<cv::Point> validSeeds;
    for (const auto& s : seeds) {
        if (s.x < 0 || s.x >= W || s.y < 0 || s.y >= H) continue;
        // Skip if another seed already occupies the same pixel.
        bool dup = false;
        for (const auto& v : validSeeds)
            if (v == s) { dup = true; break; }
        if (!dup) validSeeds.push_back(s);
    }
    if (validSeeds.empty()) return;

    const int numRegions = static_cast<int>(validSeeds.size());

    // ── 2. Initialise label map (−1 = unlabelled) ────────────────────────────
    cv::Mat labelMap(H, W, CV_32SC1, cv::Scalar(-1));

    // ── 3. Record each region's seed colour (used as reference for growth) ────
    std::vector<cv::Vec3b> seedColors(numRegions);
    for (int i = 0; i < numRegions; ++i) {
        seedColors[i] = img.mat.at<cv::Vec3b>(validSeeds[i]);
        labelMap.at<int>(validSeeds[i]) = i;
    }

    // ── 4. BFS from all seeds simultaneously (multi-source) ──────────────────
    //
    // Using a single queue and seeding it with all seed pixels ensures that
    // regions expand at the same "rate". This prevents an early-queued seed
    // from consuming the entire image before later seeds get a chance to grow.
    //
    struct QItem { int x, y, region; };
    std::queue<QItem> bfsQueue;
    for (int i = 0; i < numRegions; ++i)
        bfsQueue.push({validSeeds[i].x, validSeeds[i].y, i});

    const int dx[4] = {1, -1, 0,  0};
    const int dy[4] = {0,  0, 1, -1};

    while (!bfsQueue.empty()) {
        auto [cx, cy, region] = bfsQueue.front();
        bfsQueue.pop();

        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];

            // Bounds check.
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;

            // Already labelled.
            if (labelMap.at<int>(ny, nx) != -1) continue;

            // Colour similarity check against the region's seed colour.
            cv::Vec3b neighborColor = img.mat.at<cv::Vec3b>(ny, nx);
            if (colorDist(neighborColor, seedColors[region]) < threshold) {
                labelMap.at<int>(ny, nx) = region;
                bfsQueue.push({nx, ny, region});
            }
        }
    }

    // ── 5. Assign any remaining unlabelled pixels ─────────────────────────────
    fillUnlabelled(labelMap);

    // ── 6. Render and store ───────────────────────────────────────────────────
    cv::Mat result = renderLabels(labelMap, numRegions);
    img.store("region_growing_scratch", result);
}

} // namespace RegionGrowingProcessor