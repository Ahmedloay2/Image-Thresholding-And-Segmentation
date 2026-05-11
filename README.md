# Image Segmentation Suite

A Qt-based desktop application for interactive image segmentation and thresholding, implementing multiple classical computer vision algorithms from scratch alongside OpenCV built-ins.

---

## Features

### Segmentation Algorithms

| Algorithm | Implementation | Notes |
|---|---|---|
| **k-Means++** | From scratch | RGB or RGB+XY spatial feature space |
| **Region Growing** | From scratch | Multi-seed BFS with interactive seed placement |
| **Mean Shift** | From scratch + OpenCV | Optional LUV colour space |
| **Agglomerative Clustering** | From scratch | Average linkage (UPGMA), centroid linkage |

### Thresholding Algorithms

| Algorithm | Description |
|---|---|
| **Optimal Thresholding** | Iterative optimal threshold estimation |
| **Otsu's Method** | Variance-maximising automatic threshold |
| **Spectral Thresholding** | Auto or manual *K* selection |
| **Local (Adaptive) Thresholding** | Window-based with configurable offset *C* |

---

## Architecture

The project follows a layered MVC architecture with SOLID principles applied throughout.

```
┌─────────────────────────────────────────┐
│              Qt UI Layer                │
│  MainWindow · SegmentationWidget        │
│  ClickableLabel (seed placement)        │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│           Controller Layer              │
│  SegmentationController                 │
│  ThresholdController                    │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│           Processor Layer               │
│  KMeansProcessor · RegionGrowingProcessor│
│  SegmentationProcessor (Mean Shift)     │
│  AgglomerativeProcessor                 │
│  ThresholdProcessor                     │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│             Model Layer                 │
│  Image  (cv::Mat + pipeline cache)      │
└─────────────────────────────────────────┘
```

**Key design decisions:**

- `Image` is a plain value type that owns the source `cv::Mat` and a typed string-keyed cache for pipeline results. Processors write named results into this cache; the UI never touches OpenCV directly.
- Controllers are the only layer that call processors. `MainWindow` and widgets deal exclusively in Qt types (`QPixmap`, `QString`).
- All processor namespaces are stateless free functions — no inheritance, easy to test in isolation.

---

## Algorithm Details

### k-Means++ (`KMeansProcessor`)

- **Initialisation:** k-Means++ seeding — O(n·k), significantly reduces poor convergence vs random init.
- **Feature space:** RGB (3-D) or RGB+XY (5-D), where spatial coordinates are normalised and scaled by a configurable `xyWeight`.
- **Parallelism:** Assignment and feature-building steps use OpenMP.
- **Convergence:** Stops when centroid shift < `epsilon` or `maxIter` is reached.
- **Output:** False-colour label map stored as `"kmeans_scratch"`.

### Region Growing (`RegionGrowingProcessor`)

- **Seeds:** Supplied interactively by clicking pixels in the UI; multiple seeds supported.
- **Growth rule:** 4-connectivity BFS; a neighbour is absorbed if its Euclidean RGB distance to the region's *seed colour* is below `threshold`.
- **Multi-source BFS:** All seeds are enqueued simultaneously so regions expand at equal rate, preventing any single region from dominating.
- **Cleanup:** Unlabelled pixels (those that never passed the threshold) are assigned to the nearest labelled pixel via a second O(N) BFS flood-fill pass.
- **Output:** Stored as `"region_growing_scratch"`.

### Mean Shift (`SegmentationProcessor`)

- Two modes: **OpenCV** (`cv::pyrMeanShiftFiltering`) and a **from-scratch** implementation.
- The scratch implementation iterates over unvisited pixels, shifts the mean in joint spatial-colour space until convergence, then merges basins of attraction that are within half the bandwidth of an existing cluster.
- Optional **LUV colour space** conversion for perceptually uniform distance measurements.
- Outputs stored as `"mean_shift_opencv"` and `"mean_shift_scratch"` respectively.

### Agglomerative Clustering (`AgglomerativeProcessor`)

- **Linkage:** Average linkage (UPGMA) — inter-cluster distance is the weighted average of member distances.
- **Scalability:** Input is downsampled to a 40×40 grid before clustering to keep the O(N³) merge loop feasible; full-resolution assignment is done afterwards by nearest-centroid lookup.
- **Feature space:** 5-D (RGB + weighted XY), balancing colour and spatial cohesion.
- Optional **LUV colour space**.
- Output stored as `"agglomerative_scratch"`.

### Thresholding (`ThresholdProcessor`)

- **Optimal Thresholding:** Iterative algorithm that alternates between splitting the histogram at the current threshold and recomputing the threshold as the mean of the two resulting group means. Converges when the threshold stops changing. Operates on the grayscale image.

- **Otsu's Method:** Exhaustively searches all possible thresholds (0–255) and selects the one that maximises the *between-class variance* of the foreground and background pixel distributions. Produces a single global binary result.

- **Spectral Thresholding:** Analyses the histogram to locate *K* dominant peaks and places thresholds in the valleys between them, yielding a multi-level (non-binary) label map. In **auto** mode *K* is estimated from the histogram; in **manual** mode the caller supplies *K* directly. Results stored as `"spectral_auto_labeled"` and `"spectral_manual_labeled"`.

- **Local (Adaptive) Thresholding:** Divides the image into overlapping windows of configurable size and computes a per-window threshold as the local mean minus an offset *C*. This handles uneven illumination that would defeat a single global threshold. Window size must be odd; the controller enforces this automatically. Output stored as `"local_binary"`.

---

## Dependencies

| Dependency | Purpose |
|---|---|
| **Qt 5 / Qt 6** | UI framework |
| **OpenCV** | Image I/O, colour conversion, `pyrMeanShiftFiltering` |
| **OpenMP** | Parallelism in k-Means and feature extraction |
| **C++17** | `std::optional`, `string_view`, structured bindings |

---

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

CMake will locate Qt and OpenCV via their standard `find_package` modules. Ensure `Qt5_DIR` / `Qt6_DIR` and `OpenCV_DIR` are set in your environment if they are installed in non-standard locations.

OpenMP is expected to be available; the k-Means processor uses `#pragma omp parallel for` guarded by `#ifdef _OPENMP`.

---

## Usage

1. **Launch** the application.
2. **Load an image** using the Load button (supports any format OpenCV can read: JPEG, PNG, BMP, TIFF, etc.).
3. **Select an algorithm** from the radio buttons.
4. **Configure parameters** in the panel that appears for the selected algorithm:
   - *k-Means:* set number of clusters *k*, choose RGB or RGB+XY space, adjust spatial weight.
   - *Region Growing:* click seed points directly on the image, adjust the colour threshold slider.
   - *Mean Shift:* set spatial and colour bandwidth; toggle LUV space.
   - *Agglomerative:* set target cluster count; toggle LUV space.
5. **Click Run** to execute segmentation.
6. **Export** the result using the Export button.

For Region Growing, seeds can be cleared and re-placed at any time without reloading the image.

---

## Project Structure

```
├── model/
│   └── image.hpp                   # Core data container + pipeline cache
├── processors/
│   ├── kmeans_processor.hpp/cpp     # k-Means++ from scratch
│   ├── region_growing_processor.hpp/cpp
│   ├── segmentation_processor.hpp/cpp  # Mean Shift (OpenCV + scratch)
│   ├── agglomerative_processor.hpp/cpp
│   └── threshold_processor.hpp/cpp
├── UI/
│   ├── threshold/
│   │   ├── threshold_controller.hpp/cpp
│   │   └── (threshold widget — in mainwindow)
│   └── segmentation/
│       ├── segmentation_controller.hpp/cpp
│       ├── segmentation_widget.hpp/cpp
│       └── clickable_label.hpp
├── io/
│   └── image_handler.cpp           # loadImage() wrapping cv::imread
└── mainwindow.cpp
```
