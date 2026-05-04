#pragma once

/**
 * @file kmeans_processor.hpp
 * @brief From-scratch k-Means++ segmentation on RGB or RGB+XY feature space.
 *
 * SOLID notes
 * ─────────────
 * SRP  – Pure algorithm; no Qt, no UI concerns.
 * OCP  – Space enum can be extended without changing the algorithm core.
 * DIP  – Depends only on the Image abstraction.
 *
 * Algorithm notes
 * ───────────────
 * • Initialisation  : k-Means++ — O(n·k) seeding for well-spread centroids.
 * • Assignment step : parallelised with OpenMP across pixel rows.
 * • Update step     : accumulate per-cluster sums, then divide — no sqrt needed.
 * • Convergence     : centroid shift < epsilon  OR  maxIter reached.
 * • Feature space   : RGB (3-D) or RGB+XY (5-D, spatial coords weighted by xyWeight).
 * • Output          : false-colour label map stored as "kmeans_scratch" in Image cache.
 */

#include "model/image.hpp"
#include <vector>
#include <opencv2/core.hpp>

namespace KMeansProcessor {

    /**
     * @brief Which feature space to use for distance computation.
     * RGB      – cluster purely by colour; spatially disconnected blobs possible.
     * RGB_XY   – colour + weighted pixel position; encourages spatially compact clusters.
     */
    enum class Space { RGB, RGB_XY };

    /**
     * @brief Run k-Means++ segmentation from scratch.
     *
     * @param img       Image container. Result stored as "kmeans_scratch".
     * @param k         Number of clusters (≥ 1).
     * @param space     Feature space — RGB or RGB_XY.
     * @param xyWeight  Spatial weight applied to normalised x/y coords (RGB_XY only).
     *                  0 = ignore position, higher = stronger spatial pull.
     * @param maxIter   Maximum EM iterations before forced convergence.
     * @param epsilon   Centroid-shift convergence threshold (feature-space units).
     */
    void kMeansScratch(Image& img,
                       int    k,
                       Space  space     = Space::RGB,
                       float  xyWeight  = 50.0f,
                       int    maxIter   = 100,
                       float  epsilon   = 0.5f);

} // namespace KMeansProcessor