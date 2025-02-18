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

namespace QT5 {
    
// Convert OpenCV Mat to QImage for Qt display
QImage matToQImage(const cv::Mat& mat);
std::vector<QImage> matToQImage(std::span<const cv::Mat> mats);
std::vector<QImage> matToQImage(std::span<const cv::Mat1b> mats);

// Function to create a QLabel with an image
QLabel* createImageLabel(const QImage& image);



}

