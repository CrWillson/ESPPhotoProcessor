#include "qt5.hpp"

std::vector<QImage> QT5::matToQImage(std::span<const cv::Mat1b> mats) {
    std::vector<QImage> qimages;
    qimages.reserve(mats.size());  // Pre-allocate memory for efficiency

    // Convert each cv::Mat1b to QImage and add it to the vector
    for (const auto& mat : mats) {
        // Ensure the input is single-channel and 8-bit
        if (mat.channels() == 1 && mat.depth() == CV_8U) {
            // Create the QImage with the same dimensions as the cv::Mat1b
            qimages.push_back(QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8));
        } else {
            qimages.push_back(QImage());  // Add an empty QImage if the format is unsupported
        }
    }

    return qimages;
}

std::vector<QImage> QT5::matToQImage(std::span<const cv::Mat> mats) {
    std::vector<QImage> qimages;
    qimages.reserve(mats.size());  // Preallocate memory for efficiency

    // Loop through each OpenCV Mat in the input span
    for (const auto& mat : mats) {
        cv::Mat rgbMat;

        // Convert BGR to RGB if the image has 3 channels
        if (mat.type() == CV_8UC3) {
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        } else if (mat.type() == CV_8UC1) {
            // If it's a grayscale image, no need to swap channels
            rgbMat = mat.clone();
        } else {
            // Unsupported image format, return empty vector
            qimages.push_back(QImage());
            continue;
        }

        // Convert the OpenCV Mat to a QImage
        qimages.push_back(QImage(rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step, QImage::Format_RGB888).copy());
    }

    return qimages;
}

QImage QT5::matToQImage(const cv::Mat& mat) {
    cv::Mat rgbMat;
    
    // Convert BGR to RGB if the image has 3 channels
    if (mat.type() == CV_8UC3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
    } else if (mat.type() == CV_8UC1) {
        // If it's a grayscale image, no need to swap channels
        rgbMat = mat.clone();
    } else {
        // Unsupported image format
        return QImage();
    }

    return QImage(rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step, QImage::Format_RGB888).copy();
}


QLabel* QT5::createImageLabel(const QImage& image) {
    QLabel* label = new QLabel();
    label->setPixmap(QPixmap::fromImage(image));
    label->setScaledContents(true);
    return label;
}