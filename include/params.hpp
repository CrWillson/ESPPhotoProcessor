#pragma once

#include <stdint.h>
#include "opencv2.hpp"

// Constants used throughout the code
constexpr uint8_t IMG_ROWS = 96;    // 96 rows in each image
constexpr uint8_t IMG_COLS = 96;    // 96 columns in each image
constexpr uint16_t IMG_SIZE = IMG_ROWS * IMG_COLS * 2;    // 2 bytes per pixel

/**
 * @brief Parameters used for the image processing pipeline
 * 
 */
namespace Params {

constexpr uint16_t BOX_AREA(const uint8_t TL_X, const uint8_t TL_Y, const uint8_t BR_X, const uint8_t BR_Y) {
    if (BR_X >= TL_X && BR_Y >= TL_Y) {
        return (BR_X - TL_X + 1) * (BR_Y - TL_Y + 1);
    }
    return 0;
}

constexpr uint8_t CLAMP_CENTER_POS(const uint8_t IMG_COLS, const uint8_t WHITE_CENTER_POS) {
    if (IMG_COLS > 2*WHITE_CENTER_POS)
        return WHITE_CENTER_POS;
    return IMG_COLS - WHITE_CENTER_POS;
}

// Stop line constants

constexpr uint8_t STOPBOX_TL_X          = 15;          ///< Top Left corner of the stop detection box - X
constexpr uint8_t STOPBOX_TL_Y          = 75;          ///< Top Left corner of the stop detection box - Y
constexpr uint8_t STOPBOX_BR_X          = 40;          ///< Bottom Right corner of the stop detection box - X
constexpr uint8_t STOPBOX_BR_Y          = 85;          ///< Bottom Right corner of the stop detection box - Y

constexpr uint8_t PERCENT_TO_STOP       = 20;          ///< Percentage of the stop detection box that needs to be red to register a stop
constexpr uint8_t STOP_GREEN_TOLERANCE  = 15;          ///< How much more red than green must a pixel be to be "red"
constexpr uint8_t STOP_BLUE_TOLERANCE   = 20;          ///< How much more red than blue must a pixel be to be "blue"

const cv::Point2i STOPBOX_TL(STOPBOX_TL_X,STOPBOX_TL_Y);
const cv::Point2i STOPBOX_BR(STOPBOX_BR_X,STOPBOX_BR_Y);
constexpr uint16_t STOPBOX_AREA = BOX_AREA(STOPBOX_TL_X, STOPBOX_TL_Y, STOPBOX_BR_X, STOPBOX_BR_Y);


// White line constants

constexpr uint8_t WHITE_VERTICAL_CROP   = 50;       ///< Height to remove all white pixels above
constexpr uint8_t WHITE_HORIZONTAL_CROP = 75;       ///< Width to remove all white pixels to the right of
constexpr uint8_t WHITE_RED_THRESH      = 240;      ///< How red does a pixel need to be to be "white"
constexpr uint8_t WHITE_GREEN_THRESH    = 240;      ///< How green does a pixel need to be to be "white"
constexpr uint8_t WHITE_BLUE_THRESH     = 240;      ///< How blue does a pixel need to be to be "white"
constexpr uint16_t WHITE_MIN_SIZE       = 50;       ///< Minimum size of a white blob to be considered as the white line
constexpr uint8_t WHITE_CENTER_POS      = 28;       ///< X-Pos that the robot will attempt to keep the white line at
constexpr uint8_t MAX_WHITE_DIST = CLAMP_CENTER_POS(IMG_COLS, WHITE_CENTER_POS);    // Clamp the distance from center so it can't go off the edge of the image


// Car detect constants

constexpr uint8_t CARBOX_TL_X           = 0;
constexpr uint8_t CARBOX_TL_Y           = 50;
constexpr uint8_t CARBOX_BR_X           = 15;
constexpr uint8_t CARBOX_BR_Y           = 70;

const cv::Point2i CARBOX_TL(CARBOX_TL_X,CARBOX_TL_Y);
const cv::Point2i CARBOX_BR(CARBOX_BR_X,CARBOX_BR_Y);
constexpr uint16_t CARBOX_AREA = BOX_AREA(CARBOX_TL_X, CARBOX_TL_Y, CARBOX_BR_X, CARBOX_BR_Y);

constexpr uint8_t PERCENT_TO_CAR        = 8;
constexpr uint8_t CAR_RED_TOLERANCE     = 50;
constexpr uint8_t CAR_BLUE_TOLERANCE    = 30;

}