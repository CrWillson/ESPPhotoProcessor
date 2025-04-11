#pragma once

// Opencv Imports
#include "opencv2.hpp"

#include "params.hpp"

#include <span>
#include <fmt/base.h>

using contour_t = std::vector<cv::Point2i>;

/**
 * @brief Namespace for all functions related to the ESP32's image processing pipeline.
 * These should, where ever possible, match the functions running on the ESP32.
 * 
 */
namespace MicroCV2 {

    /**
     * @brief Convert a single 16-bit RGB565 pixel to 3 8-bit RGB888 pixels
     * 
     * @param pixel - The 16-bit RGB565 pixel
     * @param red - The red value output
     * @param green - The green value output
     * @param blue - The blue value output
     */
    void RGB565toRGB888(const uint16_t pixel, uint16_t& red, uint16_t& green, uint16_t& blue);

    /**
     * @brief Set all pixels outside the specified box to black
     * 
     * @param image - The image to crop
     * @param BOX_TL - The top left corner of the box
     * @param BOX_BR - The bottom right corner of the box
     * @warning This function modifies the image in place
     */
    void cropImage(cv::Mat& image, const cv::Point2i& BOX_TL, const cv::Point2i& BOX_BR);

    /**
     * @brief Return true if a pixel is red enough to be considered a stop line
     * 
     * @param red
     * @param green 
     * @param blue 
     */
    bool isStopLine(const uint16_t red, const uint16_t green, const uint16_t blue);

    /**
     * @brief Return true if a pixel is white enough to be considered a white line
     * 
     * @param red 
     * @param green 
     * @param blue 
     */
    bool isWhiteLine(const uint16_t red, const uint16_t green, const uint16_t blue);

    /**
     * @brief Process a frame for everything related to the stop line.
     * 
     * @param img - Input image
     * @param mask - Output mask of all red pixels
     * @return Whether the stop line was detected or not
     */
    bool processRedImg(const cv::Mat& img, cv::Mat1b& mask);

    /**
     * @warning OBSTACLE AND CAR DETECTION IS CURRENTLY NOT WORKING OR USED (4/8/2025)
     * @brief Process a frame for everything related to detecting obstacles or other cars.
     * 
     * @param img - Input image
     * @param mask - Output mask of all obstacle pixels
     * @return Whether an obstacle was detected or not
     */
    bool processCarImg(const cv::Mat& img, cv::Mat1b& mask);

    /**
     * @brief Process a frame for everything related to the white line.
     * 
     * @param img - Input image
     * @param mask - Output mask of all white pixels
     * @param centerLine - Additional output mask showing other reference lines and points
     * @param dist - The reported distance to the white line
     * @return Whether the white line was detected or not
     */
    bool processWhiteImg(const cv::Mat& img, cv::Mat1b& mask, cv::Mat1b& centerLine, int8_t& dist);

    /**
     * @brief Convert a single channel grayscale mask to a three channel mask of a specified color
     * 
     * @param mask - The mask to colorize
     * @param color - The color to use
     * @return cv::Mat - The colorized mask
     */
    cv::Mat colorizeMask(const cv::Mat1b& mask, const cv::Vec3b& color);

    /**
     * @brief Layer a mask on top of a destination image
     * 
     * @param dest - The destination image
     * @param mask - The mask to layer on top
     * @return true - If the operation was successful
     * @return false - If the operation failed
     */
    bool layerMask(cv::Mat& dest, const cv::Mat& mask);

}