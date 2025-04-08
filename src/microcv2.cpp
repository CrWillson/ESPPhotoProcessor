#include "microcv2.hpp"
#include "params.hpp"
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>

void MicroCV2::RGB565toRGB888(const uint16_t pixel, uint16_t& red, uint16_t& green, uint16_t& blue)
{
    red = (pixel >> 11) & 0x1F;
    green = (pixel >> 5) & 0x3F;
    blue = pixel & 0x1F;

    red = (red * 255) / 31;
    green = (green * 255) / 63;
    blue = (blue * 255) / 31;
}

void MicroCV2::cropImage(cv::Mat& image, const cv::Point2i& BOX_TL, const cv::Point2i& BOX_BR)
{
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    cv::rectangle(mask, BOX_TL, BOX_BR, cv::Scalar(255), cv::FILLED);
    image.setTo(cv::Scalar(0), ~mask);
}

bool MicroCV2::isStopLine(const uint16_t red, const uint16_t green, const uint16_t blue)
{
    if (red >= green + Params::STOP_GREEN_TOLERANCE && red >= blue + Params::STOP_BLUE_TOLERANCE) { 
        return true;
    }
    return false;
}

bool MicroCV2::isWhiteLine(const uint16_t red, const uint16_t green, const uint16_t blue)
{
    if (red >= Params::WHITE_RED_THRESH && green >= Params::WHITE_GREEN_THRESH && blue >= Params::WHITE_BLUE_THRESH) { 
        return true;
    }
    return false;
}

bool MicroCV2::processRedImg(const cv::Mat& image, cv::Mat1b& mask)
{
    mask = cv::Mat::zeros(image.size(), CV_8UC1);

    uint16_t redCount = 0;
    for (uint8_t y = 0; y < image.rows; ++y) {
        for (uint8_t x = 0; x < image.cols; ++x) {
            cv::Vec2b vecpixel = image.at<cv::Vec2b>(y, x);
            uint16_t pixel = (static_cast<uint16_t>(vecpixel[0]) << 8) | vecpixel[1];

            uint16_t red, green, blue;
            RGB565toRGB888(pixel, red, green, blue);

            if (isStopLine(red, green, blue) && !isWhiteLine(red, green, blue)) {
                if (x >= Params::STOPBOX_TL.x && x <= Params::STOPBOX_BR.x && y >= Params::STOPBOX_TL.y && y <= Params::STOPBOX_BR.y) {
                    redCount++;
                    mask.at<uchar>(y,x) = 255;
                }
            }
        }
    }

    cv::rectangle(mask, Params::STOPBOX_TL, Params::STOPBOX_BR, cv::Scalar(255), 1);

    uint16_t percentRed = (redCount*10000) / Params::STOPBOX_AREA;
    return percentRed >= (Params::PERCENT_TO_STOP*100);
}

bool MicroCV2::processCarImg(const cv::Mat &image, cv::Mat1b &mask)
{
    mask = cv::Mat::zeros(image.size(), CV_8UC1);

    uint16_t carCount = 0;
    for (uint8_t y = 0; y < image.rows; ++y) {
        for (uint8_t x = 0; x < image.cols; ++x) {
            uint16_t pixel = image.at<uint16_t>(y,x);

            uint16_t red, green, blue;
            RGB565toRGB888(pixel, red, green, blue);

            if (green >= red + Params::CAR_RED_TOLERANCE && green >= blue + Params::CAR_BLUE_TOLERANCE) {
                if (x >= Params::CARBOX_TL.x && x <= Params::CARBOX_BR.x && y >= Params::CARBOX_TL.y && y <= Params::CARBOX_BR.y) {
                    carCount++;
                    mask.at<uint8_t>(y,x) = 255;
                }
            }
        }
    }

    cv::rectangle(mask, Params::CARBOX_TL, Params::CARBOX_BR, cv::Scalar(255), 1);

    uint16_t percentCar = (carCount*10000) / Params::CARBOX_AREA;
    return percentCar >= (Params::PERCENT_TO_CAR*100);
}

bool MicroCV2::processWhiteImg(const cv::Mat& image, cv::Mat1b& mask, cv::Mat1b& centerLine, int8_t& dist)
{
    mask = cv::Mat::zeros(image.size(), CV_8UC1);
    centerLine = cv::Mat::zeros(image.size(), CV_8UC1);

    for (uint8_t y = Params::WHITE_VERTICAL_CROP; y < image.rows; ++y) {
        for (uint8_t x = 0; x < Params::WHITE_HORIZONTAL_CROP; ++x) {
            cv::Vec2b vecpixel = image.at<cv::Vec2b>(y, x);
            uint16_t pixel = (static_cast<uint16_t>(vecpixel[0]) << 8) | vecpixel[1];

            uint16_t red, green, blue;
            RGB565toRGB888(pixel, red, green, blue);

            if (isWhiteLine(red, green, blue)) {
                mask.at<uchar>(y,x) = 255;
            }
        }
    }

    // cropImage(mask, {0, WHITE_VERTICAL_CROP}, {WHITE_HORIZONTAL_CROP, 95});

    std::vector<contour_t> contours;
    cv::findContours(mask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    if (contours.size() == 0) {
        return false;
    }

    // Sort the contours to put the largest at index 0
    std::sort(contours.begin(), contours.end(), [](const contour_t& a, const contour_t& b) {
        return cv::contourArea(a) > cv::contourArea(b);
    });
    if (cv::contourArea(contours[0]) < Params::WHITE_MIN_SIZE) return false;

    cv::Point topLeft, topRight, bottomLeft, bottomRight = contours[0][0];
    cv::Point leftTop, leftBottom, rightTop, rightBottom = contours[0][0];

    // Initialize extreme values
    int y_min = contours[0][0].y, y_max = contours[0][0].y;
    int x_min = contours[0][0].x, x_max = contours[0][0].x;

    // First pass to determine min/max x and y
    for (const auto& pt : contours[0]) {
        if (pt.y < y_min) y_min = pt.y;
        if (pt.y > y_max) y_max = pt.y;
        if (pt.x < x_min) x_min = pt.x;
        if (pt.x > x_max) x_max = pt.x;
    }

    // Second pass to find exact extreme points
    for (const auto& pt : contours[0]) {
        // Topmost row (y_min)
        if (pt.y == y_min) {
            if (topLeft == cv::Point() || pt.x < topLeft.x) topLeft = pt;
            if (topRight == cv::Point() || pt.x > topRight.x) topRight = pt;
        }
        // Bottommost row (y_max)
        if (pt.y == y_max) {
            if (bottomLeft == cv::Point() || pt.x < bottomLeft.x) bottomLeft = pt;
            if (bottomRight == cv::Point() || pt.x > bottomRight.x) bottomRight = pt;
        }
        // Leftmost column (x_min)
        if (pt.x == x_min) {
            if (leftTop == cv::Point() || pt.y < leftTop.y) leftTop = pt;
            if (leftBottom == cv::Point() || pt.y > leftBottom.y) leftBottom = pt;
        }
        // Rightmost column (x_max)
        if (pt.x == x_max) {
            if (rightTop == cv::Point() || pt.y < rightTop.y) rightTop = pt;
            if (rightBottom == cv::Point() || pt.y > rightBottom.y) rightBottom = pt;
        }
    }

    cv::circle(centerLine, leftTop, 1, cv::Scalar(255));
    cv::circle(centerLine, topLeft, 1, cv::Scalar(255));
    cv::circle(centerLine, rightTop, 1, cv::Scalar(255));
    cv::circle(centerLine, topRight, 1, cv::Scalar(255));
    cv::circle(centerLine, leftBottom, 1, cv::Scalar(255));
    cv::circle(centerLine, bottomLeft, 1, cv::Scalar(255));
    cv::circle(centerLine, rightBottom, 1, cv::Scalar(255));
    cv::circle(centerLine, bottomRight, 1, cv::Scalar(255));


    // cv::Point top = cv::Point((leftmost_topmost.x + topmost_rightmost.x) / 2, (leftmost_topmost.y + topmost_rightmost.y) / 2);
    // cv::Point bottom = cv::Point((bottommost_leftmost.x + bottommost_rightmost.x) / 2, (bottommost_leftmost.y + bottommost_rightmost.y) / 2);

    cv::Point top = leftTop;
    cv::Point bottom = bottomLeft;

    float slope = (float)(bottom.y - top.y) / (bottom.x - top.x);
    float y_intercept = top.y - slope * top.x;

    int16_t p1_x, p1_y, p2_x, p2_y;         // points for drawing slope line
    p1_y = 0;
    p2_y = mask.rows - 1;
    p1_x = (p1_y - y_intercept) / slope;
    p2_x = (p2_y - y_intercept) / slope;
    cv::line(centerLine, cv::Point(p1_x, p1_y), cv::Point(p2_x, p2_y), cv::Scalar(255), 1);

    cv::Point intersectionPoint;        // Point where the slope line intersects the WHITE_CENTER_POS line
    intersectionPoint.y = Params::WHITE_VERTICAL_CROP;
    intersectionPoint.x = (intersectionPoint.y - y_intercept) / slope;
    cv::circle(centerLine, intersectionPoint, 2, cv::Scalar(255));

    cv::line(centerLine, cv::Point(Params::WHITE_CENTER_POS, 0), cv::Point(Params::WHITE_CENTER_POS, mask.rows - 1), cv::Scalar(255), 1);
    // cv::line(centerLine, cv::Point(0, intersectionPoint.y), cv::Point(mask.cols-1, intersectionPoint.y), cv::Scalar(255), 1);

    dist = intersectionPoint.x - Params::WHITE_CENTER_POS;
    cv::putText(centerLine, std::to_string(dist), cv::Point(0, 10), cv::FONT_HERSHEY_SIMPLEX, 0.25, cv::Scalar(255), 1); 

    cv::line(centerLine, cv::Point(0, Params::WHITE_VERTICAL_CROP), cv::Point(mask.cols - 1, 
             Params::WHITE_VERTICAL_CROP), cv::Scalar(255), 1);
    
    if (dist > Params::MAX_WHITE_DIST) dist = Params::MAX_WHITE_DIST;
    if (dist < -Params::MAX_WHITE_DIST) dist = -Params::MAX_WHITE_DIST;

    return true;
}


cv::Mat MicroCV2::colorizeMask(const cv::Mat1b& mask, const cv::Vec3b& color) {
    cv::Mat3b colorMask(mask.size());
    cv::Vec3b bgrColor = {color[2], color[1], color[0]}; // Swap from RGB to BGR

    for (int row = 0; row < mask.rows; ++row) {
        for (int col = 0; col < mask.cols; ++col) {
            if (mask(row, col) > 0) {
                colorMask(row, col) = bgrColor;
            } else {
                colorMask(row, col) = cv::Vec3b(0, 0, 0);
            }
        }
    }

    return colorMask;
}


bool MicroCV2::layerMask(cv::Mat& dest, const cv::Mat& mask)
{
    if (dest.size != mask.size) {
        fmt::println("Destination and mask size do not match.");
        return false;
    }

    for (int row = 0; row < mask.rows; row++) {
        for (int col = 0; col < mask.cols; col++) {
            cv::Vec3b& basePixel = dest.at<cv::Vec3b>(row, col);
            const cv::Vec3b& maskPixel = mask.at<cv::Vec3b>(row, col);

            // If the mask pixel is not black, overlay it onto the combined image
            if (maskPixel != cv::Vec3b(0, 0, 0)) {
                basePixel = maskPixel;
            }
        }
    }

    return true;
}