#pragma once

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QPixmap>
#include <QImage>

#include <span>
#include <vector>

#include "opencv2.hpp"

/**
 * @brief Namespace for dealing with the QT5 framework
 * 
 */
namespace QT5 {

    /**
     * @brief Convert an opencv matrix to a QImage
     * 
     * @param mat - The opencv matrix
     * @return QImage - The QImage
     */
    QImage matToQImage(const cv::Mat& mat);

    /**
     * @brief Vectorized version of matToQImage that takes a span of CV_8UC3 opencv matrices
     * 
     * @param mats - The span of CV_8UC3 opencv matrices
     * @return std::vector<QImage> - A vector of QImages
     */
    std::vector<QImage> matToQImage(std::span<const cv::Mat> mats);

    /**
     * @brief Vectorized version of matToQImage that takes a span of CV_8UC1 opencv matrices
     * 
     * @param mats - The span of CV_8UC1 opencv matrices
     * @return std::vector<QImage> - A vector of QImages
     */
    std::vector<QImage> matToQImage(std::span<const cv::Mat1b> mats);

    /**
     * @brief Create a QLabel from a QImage
     * 
     * @param mat - The QImage input
     * @return QLabel* - The QLabel
     */
    QLabel* createImageLabel(const QImage& image);

    /**
     * @brief Generate a bunch of windows showing the original image next to the processed image.
     * 
     * @param argc - Taken from main function arguments
     * @param argv - Taken from main function arguments
     * @param originalImages - Span of original images as CV_8UC3 opencv matrices
     * @param processedImages - Span of processed images as CV_8UC3 opencv matrices
     * @param filenames - Span of filenames for each image
     */
    void showImageWindows(int argc, char *argv[], const std::span<cv::Mat>& originalImages, 
        const std::span<cv::Mat>& processedImages, const std::span<std::string>& filenames);


}

