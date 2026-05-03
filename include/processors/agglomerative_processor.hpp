#pragma once
#include "model/image.hpp"

namespace AgglomerativeProcessor {

    // Standard Agglomerative Clustering (Centroid Linkage)
    void agglomerativeScratch(Image& img, int targetClusters, bool useLUV = false);

}
