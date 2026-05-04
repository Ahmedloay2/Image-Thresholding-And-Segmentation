#pragma once

/**
 * @file region_growing_processor.hpp
 * @brief From-scratch Region Growing segmentation.
 *
 * SOLID notes
 * ─────────────
 * SRP  – Pure algorithm; no Qt, no UI concerns.
 * DIP  – Depends only on the Image abstraction.
 *
 * Algorithm notes
 * ───────────────
 * • Seeds       : supplied by the caller (UI passes clicked pixel coords).
 * • Growth rule : 4-connectivity BFS; absorb neighbour if
 *                 Euclidean RGB distance to the region's seed colour < threshold.
 * • Unlabelled  : pixels that never passed the threshold are assigned to the
 *                 nearest labelled neighbour in a cleanup flood-fill pass.
 * • Output      : false-colour label map stored as "region_growing_scratch".
 */

#include "model/image.hpp"
#include <vector>
#include <opencv2/core.hpp>

namespace RegionGrowingProcessor {

    /**
     * @brief Run region growing segmentation from scratch.
     *
     * @param img        Image container. Result stored as "region_growing_scratch".
     * @param seeds      Pixel coordinates of seed points (one per desired region).
     *                   If two seeds are at the same pixel they are deduplicated.
     * @param threshold  Maximum Euclidean RGB distance allowed when absorbing a neighbour.
     *                   Low  → tight regions with possible unlabelled pixels.
     *                   High → large merged regions.
     */
    void regionGrowingScratch(Image&                        img,
                              const std::vector<cv::Point>& seeds,
                              float                         threshold);

} // namespace RegionGrowingProcessor