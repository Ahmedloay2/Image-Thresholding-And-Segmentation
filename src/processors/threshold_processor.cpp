#include "processors/threshold_processor.hpp"

#include <opencv2/opencv.hpp>
namespace {
    // ── private utilities ─────────────────────────

    /**
	* @brief Convert a BGR image to grayscale using the luminosity method.
    * 
	* @details The luminosity method computes the grayscale value as a weighted sum of the BGR channels:
    * 
	* @param bgr The input BGR image (CV_8UC3).
    * 
	* @return A grayscale image (CV_8UC1) where each pixel value is computed as:
    */
    cv::Mat toGrayscale(const cv::Mat& bgr) 
    {
		cv::Mat gray(bgr.rows, bgr.cols, CV_8UC1);
		for (int i = 0; i < bgr.rows; i++)
        {
            for(int j=0; j<bgr.cols; j++)
            {
                cv::Vec3b pixel = bgr.at<cv::Vec3b>(i, j);
                uint8_t grayValue = static_cast<uint8_t>(round(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]));
                gray.at<uint8_t>(i, j) = grayValue;
            }
        }
		return gray;
    }

    /**
	* @brief Compute the histogram of a grayscale image.
    * 
	* @details The histogram is an array of 256 integers, where each index corresponds to a pixel intensity value (0-255),
	*   and the value at each index represents the count of pixels in the image that have that intensity.
    * 
	* @param gray The input grayscale image (CV_8UC1).
    * 
	* @return An array of 256 integers representing the histogram of the grayscale image.
    */
    std::array<int, 256> computeHistogram(const cv::Mat& gray) 
    {
        std::array<int, 256> hist = {0};
        for (int i = 0; i < gray.rows; i++) {
            for (int j = 0; j < gray.cols; j++) {
                uint8_t pixelValue = gray.at<uint8_t>(i, j);
                hist[pixelValue]++;
			}
        }
        return hist;
    }

    /**
	* @brief Binarize a grayscale image using a given threshold.
    * 
	* @details This function creates a binary image where each pixel is set to 255 (white) if its intensity is greater than the threshold, and 0 (black) otherwise.
    * 
    * @param gray The input grayscale image (CV_8UC1).
    * 
    * @param t The threshold value.
    * 
    * @return A binary image (CV_8UC1) where each pixel is either 0 or 255.
    */
    cv::Mat binarize(const cv::Mat& gray, uint8_t t) 
    { 
        cv::Mat binary(gray.rows, gray.cols, CV_8UC1);
        for (int i = 0; i < gray.rows; i++)
        {
            for (int j = 0; j < gray.cols; j++)
            {
                uint8_t pixelValue = gray.at<uint8_t>(i, j);
                binary.at<uint8_t>(i, j) = (pixelValue > t) ? 255 : 0;
            }
        }
        return binary;
    }

    // spectral helpers (used by both spectral methods)

    /**
* @brief Smooth the histogram using a moving average filter.
*
* @details This function takes an input histogram (array of 256 integers) and applies a moving average filter to smooth it. The window size determines how many neighboring values are averaged together for each point in the histogram.
*   The smoothed histogram can help in identifying peaks and valleys more clearly,
*   which are important for spectral thresholding methods.
*
* @param hist The input histogram (array of 256 integers).
* @param windowSize The size of the moving average window.
* @return A vector of 256 integers representing the smoothed histogram.
*/
    std::vector<int> smoothHistogram(const std::array<int, 256>& hist, int windowSize) {
        std::vector<int> smoothed(256, 0);
        int halfWindow = windowSize / 2;
        for (int i = 0; i < 256; i++)
        {
            int sum = 0;
            int count = 0;
            for (int j = -halfWindow; j <= halfWindow; j++)
            {
                int idx = i + j;
                if (idx >= 0 && idx < 256)
                {
                    sum += hist[idx];
                    count++;
                }
            }
            smoothed[i] = sum / count;
        }
        return smoothed;
    }

    /**
    * @brief Find peaks in the smoothed histogram.
    *
    * @details This function identifies the indices of the peaks in the smoothed histogram.
    *   A peak is defined as a point that is greater than its immediate neighbors.
    *   The function iterates through the smoothed histogram and checks for this condition to find all the peaks.
    *
    * @param smoothed The input smoothed histogram (vector of 256 integers).
    *
    * @return A vector of integers representing the indices of the peaks in the smoothed histogram.
    */
    std::vector<int> findPeaks(const std::vector<int>& smoothed) {
        std::vector<int> peaks;
        for (int i = 1; i < (int)smoothed.size() - 1; i++)
        {
            if (smoothed[i] > smoothed[i - 1] && smoothed[i] > smoothed[i + 1])
            {
                peaks.push_back(i);
            }
        }
        return peaks;
    }

    /**
    * @brief Find valleys in the smoothed histogram between the identified peaks.
    *
    * @details This function takes the smoothed histogram and the indices of the peaks,
    *   and finds the valleys between those peaks. A valley is defined as a point that is less than its immediate neighbors.
    *   The function iterates through the smoothed histogram between each pair of peaks and identifies the minimum point,
    *   which is considered a valley.
    *
    * @param smoothed The input smoothed histogram (vector of 256 integers).
    * @param peaks The indices of the peaks in the smoothed histogram.
    *
    * @return A vector of integers representing the indices of the valleys in the smoothed histogram, which can be used as potential thresholds for spectral thresholding methods.
    */
    std::vector<int> findValleys(const std::vector<int>& smoothed,
        const std::vector<int>& peaks)
    {
        std::vector<int> valleys;
        for (size_t i = 0; i < peaks.size() - 1; i++)
        {
            int start = peaks[i];
            int end = peaks[i + 1];
            int minVal = smoothed[start];
            int minIdx = start;
            for (int j = start + 1; j < end; j++)
            {
                if (smoothed[j] < minVal)
                {
                    minVal = smoothed[j];
                    minIdx = j;
                }
            }
            valleys.push_back(minIdx);
        }
        return valleys;
    }

    /**
	* @brief Spectral thresholding method with manual K selection.
    * 
	* @details This method performs spectral thresholding by first computing the histogram of the grayscale image,
    *           then smoothing it to identify peaks and valleys. The user specifies the number of classes K,
    *           and the method selects the top K peaks from the smoothed histogram.
    *           The valleys between these peaks are used as thresholds to segment the image into K classes.
    *           The resulting labeled image is stored in the cache with key "spectral_manual_labeled".
    * 
	* @param img The input image. The method will store the labeled result in the cache with key "spectral_manual_labeled".
	* @param K The number of classes to segment the image into. The method will select the top K peaks from the smoothed histogram to determine the thresholds.
    * @
    */
    void spectralThresholdManual(Image& img, int K)
    {
        cv::Mat gray;
        if (!img.has("grayscale"))
        {
            gray = toGrayscale(img.mat);
            img.store("grayscale", gray);
        }
        else {
            gray = img.get("grayscale");
        }

        std::vector<int> smoothed = smoothHistogram(computeHistogram(gray), 11);
        std::vector<int> peaks = findPeaks(smoothed);

        // Sort peaks by histogram value descending, keep top K
        std::sort(peaks.begin(), peaks.end(), [&](int a, int b) {
            return smoothed[a] > smoothed[b];
            });
        if ((int)peaks.size() > K) peaks.resize(K);

        // Re-sort by index ascending to maintain left-to-right order
        std::sort(peaks.begin(), peaks.end());

        std::vector<int> valleys = findValleys(smoothed, peaks);

        if (valleys.empty()) {
            // Fallback: treat whole image as one class → black
            img.store("spectral_manual_labeled", cv::Mat::zeros(gray.rows, gray.cols, CV_8UC1));
            return;
        }
        int numClasses = (int)valleys.size() + 1;

        cv::Mat spectralMat(gray.rows, gray.cols, CV_8UC1);

        for (int i = 0; i < gray.rows; i++)
        {
            for (int j = 0; j < gray.cols; j++)
            {
                uint8_t pixelValue = gray.at<uint8_t>(i, j);
                int classIdx = 0;
                for (int t : valleys)
                {
                    if (pixelValue >= t) classIdx++;
                    else break;
                }
                spectralMat.at<uint8_t>(i, j) =
                    static_cast<uint8_t>(classIdx * (255 / (numClasses - 1)));
            }
        }
        img.store("spectral_manual_labeled", spectralMat);
    }

    /**
	* @brief Spectral thresholding method with automatic K selection.
    * 
	* @details This method performs spectral thresholding by first computing the histogram of the grayscale image, then smoothing it to identify peaks and valleys.
    *   The method automatically determines the number of classes based on the number of peaks in the smoothed histogram. 
    *   The valleys between these peaks are used as thresholds to segment the image into classes.
    *   If there are fewer than 2 peaks, the method falls back to using the mean pixel value as a single threshold for binarization.
    *   The resulting labeled image is stored in the cache with key "spectral_auto_labeled".
    * 
	* @param img The input image. The method will store the labeled result in the cache with key "spectral_auto_labeled".
    * 
    */
    void spectralThresholdAuto(Image& img)
    {
        cv::Mat gray;
        if (!img.has("grayscale"))
        {
            gray = toGrayscale(img.mat);
            img.store("grayscale", gray);
        }
        else {
            gray = img.get("grayscale");
        }

        std::vector<int> smoothed = smoothHistogram(computeHistogram(gray), 11);
        std::vector<int> peaks = findPeaks(smoothed);


        if (peaks.size() < 3)
        {
            // Not enough modes for spectral thresholding (requires 3+)
            // Fallback to mean-based binarization
            cv::Scalar meanVal = cv::mean(gray);
            img.store("spectral_auto_labeled",
                binarize(gray, static_cast<uint8_t>(meanVal[0])));
            return;
        }

        std::vector<int> valleys = findValleys(smoothed, peaks);

        if (valleys.empty()) {
            cv::Scalar meanVal = cv::mean(gray);
            img.store("spectral_auto_labeled", binarize(gray, static_cast<uint8_t>(meanVal[0])));
            return;
        }
        int numClasses = (int)valleys.size() + 1;
        cv::Mat spectralMat(gray.rows, gray.cols, CV_8UC1);

        for (int i = 0; i < gray.rows; i++)
        {
            for (int j = 0; j < gray.cols; j++)
            {
                uint8_t pixelValue = gray.at<uint8_t>(i, j);
                int classIdx = 0;
                for (int t : valleys)
                {
                    if (pixelValue >= t) classIdx++;
                    else break;
                }
                spectralMat.at<uint8_t>(i, j) =
                    static_cast<uint8_t>(classIdx * (255 / (numClasses - 1)));
            }
        }
        img.store("spectral_auto_labeled", spectralMat);
    }


}

namespace ThresholdProcessor {

    /**
	* @brief Iterative optimal thresholding method.
    * 
	* @details This method starts with an initial threshold (the mean pixel value of the grayscale image),
    *   and iteratively updates the threshold by computing the mean of the background and foreground pixels.
    *   The process continues until the threshold converges.
    *   The resulting binary image is stored in the cache with key "optimal_binary".
    * 
	* @param img The input image. The method will store the binary result in the cache with key "optimal_binary".
    * 
	* @note The input image is expected to be in BGR format. The method will convert it to grayscale if not already done.
	* @note The method assumes that the pixel values are in the range [0, 255].
    */
    void optimalThreshold(Image& img)
    {
        cv::Mat gray;
        if (!img.has("grayscale"))
        {
            gray = toGrayscale(img.mat);
            img.store("grayscale", gray);
        }
        else {
            gray = img.get("grayscale");
        }
        cv::Scalar accumlator = cv::sum(gray);
        uint8_t t = static_cast<uint8_t>(round(accumlator[0] / (gray.rows * gray.cols)));
        
        while (true) {
            double background = 0;
            int countBackground = 0;
            double   foreground = 0;
            int countForeground = 0;
            double newT;
            for (int i = 0; i < gray.rows; i++)
            {
                for (int j = 0; j < gray.cols; j++)
                {
                    uint8_t pixelValue = gray.at<uint8_t>(i, j);
                    if (pixelValue < t)
                    {
                        background += pixelValue;
                        countBackground++;
                    }
                    else
                    {
                        foreground += pixelValue;
                        countForeground++;
                    }
                }
            }
            if (countBackground == 0 || countForeground == 0) {
                break;
            }
			double backgroundMean = background / countBackground;
			double foregroundMean = foreground / countForeground;
            newT = static_cast<double>(round((backgroundMean + foregroundMean) / 2));
            if (abs(newT - t) < 0.5) {
                break;
            }
            else
            {
                t = static_cast<uint8_t>(newT);
            }        
        }
		img.store("optimal_binary", binarize(gray, t));

    }

    /**
	* @brief Otsu's method for automatic thresholding.
    * 
	* @details This method computes the optimal threshold by maximizing the between-class variance.
    *   It first computes the histogram of the grayscale image,
    *   then calculates the class probabilities and means for all possible thresholds,
    *   and finally selects the threshold that maximizes the between-class variance.
    *   The resulting binary image is stored in the cache with key "otsu_binary".
    * 
	* @param img The input image. The method will store the binary result in the cache with key "otsu_binary".
    * 
	* @note The input image is expected to be in BGR format. The method will convert it to grayscale if not already done.
	* @note The method assumes that the pixel values are in the range [0, 255].
    */
    void otsuThreshold(Image& img)
    { 
        cv::Mat gray;
        if (!img.has("grayscale"))
        {
            gray = toGrayscale(img.mat);
            img.store("grayscale", gray);
        }
        else {
            gray = img.get("grayscale");
        }
        std::array<int, 256>histogram = computeHistogram(gray);
        std::array<double, 256> normalizedHistogram;
		for (int i = 0; i < 256; i++)
        {
            normalizedHistogram[i] = static_cast<double>(histogram[i]) / (gray.rows * gray.cols);
        }
        std::array<double, 256> classVariance{};
		for (int t = 0; t < 256; t++)
        {
            double backgroundWeight = 0;
            double backgroundMean = 0;
            double foregroundWeight = 0;
            double foregroundMean = 0;
            for (int i = 0; i < t; i++)
            {
                backgroundWeight += normalizedHistogram[i];
                backgroundMean += i * normalizedHistogram[i];
            }
            for (int i = t; i < 256; i++)
            {
                foregroundWeight += normalizedHistogram[i];
                foregroundMean += i * normalizedHistogram[i];
            }
            if (backgroundWeight == 0 || foregroundWeight == 0) {
                continue;
			}
            classVariance[t] = backgroundWeight * foregroundWeight * pow(backgroundMean / backgroundWeight - foregroundMean / foregroundWeight, 2);
        }
        double maxVariance = -1;
        int bestT = 0;
        for (int i = 0; i < 256; i++)
        {
            if (classVariance[i] > maxVariance)
            {
                maxVariance = classVariance[i];
                bestT = i;
            }
        }
        img.store("otsu_binary", binarize(gray, bestT));
    }

    /**
	* @brief Spectral thresholding method with both automatic and manual modes.
    * 
	* @details This method performs spectral thresholding to segment the image into multiple classes based on the histogram peaks and valleys.
    *
	* @param img The input image. The method will store the labeled result in the cache with key "spectral_auto_labeled" for automatic mode or "spectral_manual_labeled" for manual mode.
	* @param autoMode If true, the method will automatically determine the number of classes based on the histogram peaks. If false, it will use the specified number of classes K.
	* @param K The number of classes to segment the image into when autoMode is false. This parameter is ignored when autoMode is true.
    * 
	* @note The input image is expected to be in BGR format. The method will convert it to grayscale if not already done.
    */
    void spectralThreshold(Image& img, bool autoMode, int K)
    {
        if (autoMode)
            spectralThresholdAuto(img);
        else
            spectralThresholdManual(img, K);
        
    }

    /**
	* @brief Local thresholding method using a sliding window approach.
    * 
	* @details This method applies local thresholding to the image by computing a threshold for each pixel
    *       based on the mean intensity of its local neighborhood defined by a sliding window.
    *       The threshold for each pixel is calculated as the mean of the local region minus a constant C.
    *       The resulting binary image is stored in the cache with key "local_binary".
    * 
	* @param img The input image. The method will store the binary result in the cache with key "local_binary".
	* @param windowSize The size of the sliding window (must be an odd integer).
	* @param C A constant value that is subtracted from the local mean to determine the threshold for each pixel.
    * 
    * 
    */
    void localThreshold(Image& img, int windowSize, double C) 
    {
        cv::Mat gray;
        if (!img.has("grayscale"))
        {
            gray = toGrayscale(img.mat);
            img.store("grayscale", gray);
        }
        else {
            gray = img.get("grayscale");
        }
		cv::Mat localBinary(gray.rows, gray.cols, CV_8UC1);

		for (int i = 0; i < gray.rows; i++)
        {
            for(int j=0; j<gray.cols; j++)
            {
                int xStart = std::max(0, j - windowSize / 2);
                int xEnd = std::min(gray.cols - 1, j + windowSize / 2);
                int yStart = std::max(0, i - windowSize / 2);
                int yEnd = std::min(gray.rows - 1, i + windowSize / 2);
                cv::Mat localRegion = gray(cv::Range(yStart, yEnd + 1), cv::Range(xStart, xEnd + 1));
                cv::Scalar meanScalar = cv::mean(localRegion);
                double mean = meanScalar[0];
                double threshold = mean - C;
                localBinary.at<uint8_t>(i, j) = (gray.at<uint8_t>(i, j) > threshold) ? 255 : 0;
            }
        }
        img.store("local_binary", localBinary);
    }
}