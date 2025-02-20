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

QImage matToQImage(const cv::Mat& mat);
std::vector<QImage> matToQImage(std::span<const cv::Mat> mats);
std::vector<QImage> matToQImage(std::span<const cv::Mat1b> mats);

QLabel* createImageLabel(const QImage& image);
void showImageWindows(int argc, char *argv[], const std::span<cv::Mat>& originalImages, const std::span<cv::Mat>& processedImages);


}

