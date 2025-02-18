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

class QT5 {
public: 

static QImage matToQImage(const cv::Mat& mat);
static std::vector<QImage> matToQImage(std::span<const cv::Mat> mats);
static std::vector<QImage> matToQImage(std::span<const cv::Mat1b> mats);

static QLabel* createImageLabel(const QImage& image);


};

