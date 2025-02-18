#include "microcv2.hpp"

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
    if (red >= green + STOP_GREEN_TOLERANCE && red >= blue + STOP_BLUE_TOLERANCE) { 
        return true;
    }
    return false;
}

bool MicroCV2::isWhiteLine(const uint16_t red, const uint16_t green, const uint16_t blue)
{
    if (red >= WHITE_RED_THRESH && green >= WHITE_GREEN_THRESH && blue >= WHITE_BLUE_THRESH) { 
        return true;
    }
    return false;
}

// bool MicroCV2::maskAllColors(const cv::Mat& image, cv::Mat1b& redMask, uint16_t& rCount, cv::Mat1b& whiteMask, uint16_t& wCount)
// {
//     redMask = cv::Mat::zeros(image.size(), CV_8UC1);
//     whiteMask = cv::Mat::zeros(image.size(), CV_8UC1);

//     for (uint8_t y = 0; y < image.rows; ++y) {
//         for (uint8_t x = 0; x < image.cols; ++x) {
//             uint16_t pixel = (image.at<cv::Vec2b>(y,x)[1] << 8) | image.at<cv::Vec2b>(y,x)[0];

//             uint16_t red, green, blue;
//             RGB565toRGB888(pixel, red, green, blue);

//             if (red >= green + STOP_GREEN_TOLERANCE && red >= blue + STOP_BLUE_TOLERANCE) {
//                 if (x >= STOPBOX_TL.x && x <= STOPBOX_BR.x && y >= STOPBOX_TL.y && y <= STOPBOX_BR.y) {
//                     rCount++;
//                 }
//                 redMask.at<uchar>(y,x) = 255;
//             }
//         }
//     }
// }

bool MicroCV2::processRedImg(const cv::Mat& image, cv::Mat1b& mask)
{
    mask = cv::Mat::zeros(image.size(), CV_8UC1);

    uint16_t redCount = 0;
    for (uint8_t y = 0; y < image.rows; ++y) {
        for (uint8_t x = 0; x < image.cols; ++x) {
            uint16_t pixel = image.at<uint16_t>(y,x);

            uint16_t red, green, blue;
            RGB565toRGB888(pixel, red, green, blue);

            if (isStopLine(red, green, blue) && !isWhiteLine(red, green, blue)) {
                if (x >= STOPBOX_TL.x && x <= STOPBOX_BR.x && y >= STOPBOX_TL.y && y <= STOPBOX_BR.y) {
                    redCount++;
                    mask.at<uchar>(y,x) = 255;
                }
            }
        }
    }

    cv::rectangle(mask, STOPBOX_TL, STOPBOX_BR, cv::Scalar(255), 1);

    uint16_t percentRed = (redCount*10000) / STOPBOX_AREA;
    return percentRed >= (PERCENT_TO_STOP*100);
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

            if (green >= red + CAR_RED_TOLERANCE && green >= blue + CAR_BLUE_TOLERANCE) {
                if (x >= CARBOX_TL.x && x <= CARBOX_BR.x && y >= CARBOX_TL.y && y <= CARBOX_BR.y) {
                    carCount++;
                    mask.at<uint8_t>(y,x) = 255;
                }
            }
        }
    }

    cv::rectangle(mask, CARBOX_TL, CARBOX_BR, cv::Scalar(255), 1);

    uint16_t percentCar = (carCount*10000) / CARBOX_AREA;
    return percentCar >= (PERCENT_TO_CAR*100);
}

bool MicroCV2::processWhiteImg(const cv::Mat& image, cv::Mat1b& mask, cv::Mat1b& centerLine, int8_t& dist, int8_t& height)
{
    mask = cv::Mat::zeros(image.size(), CV_8UC1);
    centerLine = cv::Mat::zeros(image.size(), CV_8UC1);

    for (uint8_t y = WHITE_VERTICAL_CROP; y < image.rows; ++y) {
        for (uint8_t x = 0; x < WHITE_HORIZONTAL_CROP; ++x) {
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

    int maxSize = 0;
    int maxInd = 0;
    for (int i = 0; i < contours.size(); i++) {
        int size = cv::contourArea(contours[i]);
        if (size > maxSize) {
            maxSize = size;
            maxInd = i;
        }
    }
    if (maxSize < WHITE_MIN_SIZE) return false;

    cv::Rect boundingBox = cv::boundingRect(contours[maxInd]);
    dist = boundingBox.x + boundingBox.width / 2;
    height = boundingBox.y + boundingBox.height / 2;

    cv::Point leftmost_topmost = contours[maxInd][0];

    // Iterate through the contour to find the topmost-leftmost point
    for (const auto& point : contours[maxInd]) {
        if (point.x < leftmost_topmost.x || 
            (point.x == leftmost_topmost.x && point.y < leftmost_topmost.y)) {
            leftmost_topmost = point;
        }
    } 
    dist = leftmost_topmost.x;
    height = leftmost_topmost.y;

    cv::line(centerLine, cv::Point(dist, 0), cv::Point(dist, mask.rows - 1), cv::Scalar(255), 1);
    cv::line(centerLine, cv::Point(0, height), cv::Point(mask.cols - 1, height), cv::Scalar(255), 1);
    
    dist -= WHITE_CENTER_POS;

    if (dist > MAX_WHITE_DIST) dist = MAX_WHITE_DIST;
    if (dist < -MAX_WHITE_DIST) dist = -MAX_WHITE_DIST;

    return true;
}