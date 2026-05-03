#include "io/image_handler.hpp"
#include "processors/agglomerative_processor.hpp"
#include "processors/segmentation_processor.hpp"
#include "processors/threshold_processor.hpp"
#include <cstdlib>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

// ── Globals for UI ────────────────────────────────────────────
int targetClusters = 5;
int useLUV = 0;
Image g_img;

void onAggTrackbarChange(int, void *) {
  if (g_img.mat.empty())
    return;

  std::cout << "Running Agglomerative Clustering... (Target Clusters: " << targetClusters << ", Color Space: " << (useLUV ? "LUV" : "RGB") << ")" << std::endl;

  AgglomerativeProcessor::agglomerativeScratch(g_img, targetClusters, useLUV == 1);
  cv::imshow("Segmented", g_img.get("agglomerative_scratch"));

  std::string pyCmd = "python3 ../scripts/draw_dendrogram.py linkage.csv dendrogram.png";
  int ret = std::system(pyCmd.c_str());
  if (ret == 0) {
    cv::Mat dendro = cv::imread("dendrogram.png");
    if (!dendro.empty()) {
      cv::imshow("Dendrogram Tree", dendro);
    }
  } else {
    std::cerr << "Failed to draw dendrogram. Ensure python3, scipy, and matplotlib are installed." << std::endl;
  }
}

int main() {
  // ── Load image ────────────────────────────────────────────
  try {
    g_img = loadImage(std::string(PROJECT_SOURCE_DIR) +
                      "/resources/images/cluster2.png");
  } catch (const std::exception &e) {
    std::cerr << "Error loading cluster.png: " << e.what() << std::endl;
    std::cerr << "Falling back to 1.jpg" << std::endl;
    g_img =
        loadImage(std::string(PROJECT_SOURCE_DIR) + "/resources/images/1.jpg");
  }

  // ── Previous Code (Commented) ──────────────
  /*
  ThresholdProcessor::optimalThreshold(g_img);
  // ...
  cv::imshow("Local", g_img.get("local_binary"));

  SegmentationProcessor::meanShiftOpenCV(g_img, 30, 30);
  // ...
  */

  // ── Display Original ──────────────────────────────────────
  cv::namedWindow("Original", cv::WINDOW_AUTOSIZE);
  cv::imshow("Original", g_img.mat);

  // ── Setup UI Window ───────────────────────────────────────
  cv::namedWindow("Controls", cv::WINDOW_NORMAL);
  cv::resizeWindow("Controls", 400, 150);

  // Agglomerative Trackbars
  cv::createTrackbar("Clusters", "Controls", &targetClusters, 20,
                     onAggTrackbarChange);
  // minimum value for clusters is 1, handled in the processor
  cv::createTrackbar("Color Space (0=RGB, 1=LUV)", "Controls", &useLUV, 1,
                     onAggTrackbarChange);

  // Initial run
  onAggTrackbarChange(0, 0);

  cv::waitKey(0); // wait until any key is pressed
  cv::destroyAllWindows();

  return 0;
}