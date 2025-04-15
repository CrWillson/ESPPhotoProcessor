#include "microcv2.hpp"
#include "opencv2.hpp"
#include <fmt/core.h>
#include "qt5.hpp"
#include <fstream>
#include <qapplication.h>
#include <string>
#include <type_traits>
#include <vector>
#include <array>
#include <filesystem>
#include <span>

namespace fs = std::filesystem;

/**
 * @brief Convert an RGB888 color to a 16-bit RGB565 color.
 * 
 * @param r - The red value
 * @param g - The green value
 * @param b - the blue value
 * @return constexpr uint16_t - The RGB565 color 
 */
constexpr uint16_t RGB888toRGB565(const uint8_t r, const uint8_t g, const uint8_t b) {
    uint16_t red = (r * 31) / 255;
    uint16_t green = (g * 63) / 255;
    uint16_t blue = (b * 31) / 255;

    return (red << 11) | (green << 5) | blue;
}

/**
 * @brief Convert an RGB565 color to an RGB888 color.
 * 
 * @param pixel - The encoded RGB565 color
 * @return std::array<uint8_t, 3> - Red, green, and blue values
 */
constexpr std::array<uint8_t, 3> RGB565toRGB888(const uint16_t pixel) {
    // Extract individual color components (5-bit Red, 6-bit Green, 5-bit Blue)
    uint8_t r = (pixel >> 11) & 0x1F;  // Extract red (5 bits)
    uint8_t g = (pixel >> 5) & 0x3F;   // Extract green (6 bits)
    uint8_t b = pixel & 0x1F;          // Extract blue (5 bits)

    // Scale the components to 0-255 range
    uint8_t red = (r * 255) / 31;  // Scale red from 5 bits to 8 bits
    uint8_t green = (g * 255) / 63;  // Scale green from 6 bits to 8 bits
    uint8_t blue = (b * 255) / 31;  // Scale blue from 5 bits to 8 bits

    return {red, green, blue};
}

/**
 * @brief Convert an CV_8UC2 opencv matrix of RGB565 to a CV_8UC3 opencv matrix of RGB888
 * 
 * @param rgb565_image - The CV_8UC2 opencv matrix of RGB565
 * @return cv::Mat - The CV_8UC3 opencv matrix of RGB888
 */
cv::Mat convert_rgb565_to_rgb888(const cv::Mat rgb565_image) {
    cv::Mat rgb888_image(rgb565_image.rows, rgb565_image.cols, CV_8UC3);  // RGB888 output image

    // Loop through each pixel in the RGB565 image
    for (int row = 0; row < rgb565_image.rows; ++row) {
        for (int col = 0; col < rgb565_image.cols; ++col) {
            // Get the RGB565 pixel (2 bytes per pixel)
            cv::Vec2b vecpixel = rgb565_image.at<cv::Vec2b>(row, col);
            uint16_t pixel = (static_cast<uint16_t>(vecpixel[0]) << 8) | vecpixel[1];
            auto rgb = RGB565toRGB888(pixel);

            // Set the RGB888 pixel in the output image
            rgb888_image.at<cv::Vec3b>(row, col) = cv::Vec3b(rgb[2], rgb[1], rgb[0]);  // OpenCV uses BGR order
        }
    }

    return rgb888_image;
}

/**
 * @brief Vectorized version of convert_rgb565_to_rgb888. Converts an entire span of RGB565 images to RGB888.
 * 
 * @overload
 * @param rgb565_images - A span of CV_8UC2 opencv matrices of RGB565
 * @return std::vector<cv::Mat> - A vector of CV_8UC3 opencv matrices of RGB888
 */
std::vector<cv::Mat> convert_rgb565_to_rgb888(std::span<const cv::Mat> rgb565_images) {
    std::vector<cv::Mat> rgb888_images;
    rgb888_images.reserve(rgb565_images.size());  // Preallocate memory for efficiency

    // Loop through each RGB565 image in the input span
    for (const auto& rgb565_image : rgb565_images) {
        rgb888_images.push_back(std::move(convert_rgb565_to_rgb888(rgb565_image)));  // Move the image into the vector
    }

    return rgb888_images;
}

/**
 * @brief Load a raw binary image file into an CV_8UC2 opencv matrix
 * 
 * @param filename - The filepath to the binary file
 * @param saveImage - Whether to save the image as a PNG
 * @return cv::Mat - The CV_8UC2 opencv matrix image
 */
cv::Mat load_binary_image(const std::string& filename, bool saveImage = false) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return cv::Mat();
    }

    // Read binary data
    // std::vector<uint8_t> buffer(IMG_SIZE);
    uint8_t buffer[IMG_SIZE];
    file.read(reinterpret_cast<char*>(buffer), IMG_SIZE);
    file.close();

    if (file.gcount() != IMG_SIZE) {
        std::cerr << "Error: Read only " << file.gcount() << " bytes instead of " << IMG_SIZE << std::endl;
        return cv::Mat();
    }

    // Convert buffer into cv::Mat
    cv::Mat image(IMG_ROWS, IMG_COLS, CV_8UC2, buffer);

    if (saveImage) {
        auto rgb888image = convert_rgb565_to_rgb888(image);
        cv::imwrite(filename + std::string(".png"), rgb888image);
    }

    // Make a deep copy to ensure it remains valid after buffer goes out of scope
    return image.clone();
}

/**
 * @brief Vectorized version of load_binary_image. Loads an entire span of binary images into CV_8UC2 opencv matrices.
 * 
 * @param filenames - The filepaths to the binary files
 * @param save_images - Whether to save the images as PNGs
 * @return std::vector<cv::Mat> - A vector of CV_8UC2 opencv matrices
 */
std::vector<cv::Mat> load_binary_images(std::span<const std::string> filenames, bool save_images = false) {
    std::vector<cv::Mat> images;
    images.reserve(filenames.size());  // Preallocate memory for efficiency

    for (const auto& filename : filenames) {
        cv::Mat image = load_binary_image(filename, save_images);
        if (image.empty()) {
            throw std::runtime_error("Failed to load image: " + filename);
        }
        images.push_back(std::move(image));  // Move the matrix into the vector
    }

    return images;
}

/**
 * @brief Load an image file saved in the compact hex format into an CV_8UC2 opencv matrix
 * 
 * @param filename - The filepath to the hex file
 * @param saveImage - Whether to save the image as a PNG
 * @return cv::Mat - The CV_8UC2 opencv matrix image
 */
cv::Mat load_compact_hex_image(const std::string& filename, bool saveImage = false) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return cv::Mat();
    }

    std::vector<uint16_t> hexNumbers;
    hexNumbers.reserve(96*96);

    std::string line;
    while (std::getline(file, line)) {
        for (size_t i = 0; i < line.size(); i += 4) {
            std::string hexStr = line.substr(i, 4);
            uint16_t value = static_cast<uint16_t>(std::stoul(hexStr, nullptr, 16));
            hexNumbers.push_back(value);
        }
    }

    cv::Mat image(IMG_ROWS, IMG_COLS, CV_8UC2);

    for (int i = 0; i < IMG_ROWS; i++) {
        for (int j = 0; j < IMG_COLS; j++) {
            uint16_t value = hexNumbers[i * IMG_COLS + j];
            image.at<cv::Vec2b>(i, j) = cv::Vec2b(static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF));
        }
    }

    if (saveImage) {
        auto rgb888image = convert_rgb565_to_rgb888(image);
        auto newfilename = filename.substr(0, filename.size() - 4) + std::string(".png");

        cv::imwrite(newfilename, rgb888image);
    }

    return image;
}

/**
 * @brief Vectorized version of load_compact_hex_image. Loads an entire span of hex images into CV_8UC2 opencv matrices.
 * 
 * @param filenames - The filepaths to the hex files
 * @param save_images - Whether to save the images as PNGs
 * @return std::vector<cv::Mat> - A vector of CV_8UC2 opencv matrices
 */
std::vector<cv::Mat> load_compact_hex_images(std::span<const std::string> filenames, bool save_images = false) {
    std::vector<cv::Mat> images;
    images.reserve(filenames.size());  // Preallocate memory for efficiency

    for (const auto& filename : filenames) {
        cv::Mat image = load_compact_hex_image(filename, save_images);
        if (image.empty()) {
            throw std::runtime_error("Failed to load image: " + filename);
        }
        images.push_back(std::move(image));  // Move the matrix into the vector
    }

    return images;
}

/**
 * @brief Get all of the filenames in a directory. Filters only files with the specified extensions if given.
 * 
 * @param directory_path - The path to the directory
 * @param extensions - The extensions to filter by. Will include all files if empty.
 * @return std::vector<std::string> - A vector of filepaths 
 */
std::vector<std::string> get_filenames_in_dir(const std::string& directory_path, std::span<std::string> extensions = {}) {
    std::vector<std::string> filenames;

    try {
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                const std::string& filename = entry.path().filename().string();
                if (extensions.empty() || std::any_of(extensions.begin(), extensions.end(), [&](const std::string& ext) {
                    return filename.size() >= ext.size() && filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0;
                })) { 
                    filenames.push_back(entry.path().string()); 
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }

    return filenames;
}

void process_white_presentation_image()
{
    fs::path white_pre_path = "../presentation_images/0_white_preprocess.bin";
    
    // Load the original image
    auto white_img = load_compact_hex_image(white_pre_path.string(), true);

    // Filter the white pixels
    cv::Mat white_processed = cv::Mat::zeros(white_img.size(), CV_8UC1);
    for (uint8_t y = 0; y < white_img.rows; ++y) {
        for (uint8_t x = 0; x < white_img.cols; ++x) {
            cv::Vec2b vecpixel = white_img.at<cv::Vec2b>(y, x);
            uint16_t pixel = (static_cast<uint16_t>(vecpixel[0]) << 8) | vecpixel[1];

            auto [red, green, blue] = RGB565toRGB888(pixel);

            if (MicroCV2::isWhiteLine(red, green, blue)) {
                white_processed.at<uchar>(y,x) = 255;
            }
        }
    }
    cv::imwrite("../presentation_images/1_white_filtered.png", MicroCV2::colorizeMask(white_processed, {255,255,255}));

    // Crop the image
    white_processed = cv::Mat::zeros(white_img.size(), CV_8UC1);
    for (uint8_t y = Params::WHITE_VERTICAL_CROP; y < white_img.rows; ++y) {
        for (uint8_t x = 0; x < Params::WHITE_HORIZONTAL_CROP; ++x) {
            cv::Vec2b vecpixel = white_img.at<cv::Vec2b>(y, x);
            uint16_t pixel = (static_cast<uint16_t>(vecpixel[0]) << 8) | vecpixel[1];

            auto [red, green, blue] = RGB565toRGB888(pixel);

            if (MicroCV2::isWhiteLine(red, green, blue)) {
                white_processed.at<uchar>(y,x) = 255;
            }
        }
    }
    cv::Mat white_decorated = MicroCV2::colorizeMask(white_processed, {255,255,255});
    cv::line(white_decorated, cv::Point(0, Params::WHITE_VERTICAL_CROP), cv::Point(white_decorated.cols - 1, Params::WHITE_VERTICAL_CROP), cv::Scalar(0,255,0), 1);

    cv::imwrite("../presentation_images/2_white_cropped.png", white_decorated);


    // Contour and find slope of the side of the line
    std::vector<contour_t> contours;
    cv::findContours(white_processed, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Sort the contours to put the largest at index 0
    std::sort(contours.begin(), contours.end(), [](const contour_t& a, const contour_t& b) {
            return cv::contourArea(a) > cv::contourArea(b);
    });
    
    cv::Point bottomLeft = contours[0][0];
    cv::Point leftTop = contours[0][0];

    for (const auto& pt : contours[0]) {
        // bottom left point
        if (pt.y > bottomLeft.y || (bottomLeft.y == pt.y && pt.x < bottomLeft.x)) {
            bottomLeft = pt;
        }
        // left top point
        if (pt.x < leftTop.x || (pt.x == leftTop.x && pt.y < leftTop.y)) {
            leftTop = pt;
        }
    }

    cv::circle(white_decorated, leftTop, 3, cv::Scalar(255,0,255));
    cv::circle(white_decorated, bottomLeft, 3, cv::Scalar(255,0,255));

    float slope = (float)(bottomLeft.y - leftTop.y) / (bottomLeft.x - leftTop.x);
    float y_intercept = leftTop.y - slope * leftTop.x;

    int16_t p1_x, p1_y, p2_x, p2_y;         // points for drawing slope line
    p1_y = 0;
    p2_y = white_decorated.rows - 1;
    p1_x = (p1_y - y_intercept) / slope;
    p2_x = (p2_y - y_intercept) / slope;
    cv::line(white_decorated, cv::Point(p1_x, p1_y), cv::Point(p2_x, p2_y), cv::Scalar(0,0,255), 1);

    cv::imwrite("../presentation_images/3_white_slope.png", white_decorated);


    // Find the intersection point
    cv::Point intersectionPoint;        // Point where the slope line intersects the WHITE_CENTER_POS line
    intersectionPoint.y = Params::WHITE_VERTICAL_CROP;
    intersectionPoint.x = (intersectionPoint.y - y_intercept) / slope;

    cv::line(white_decorated, cv::Point(20, 0), cv::Point(20, white_processed.rows - 1), cv::Scalar(255,255,0), 1);
    cv::line(white_decorated, cv::Point(intersectionPoint.x, 0), cv::Point(intersectionPoint.x, white_processed.rows - 1), cv::Scalar(255,0,255), 1);

    cv::imwrite("../presentation_images/4_white_distance.png", white_decorated);
}

int main(int argc, char *argv[]) {
   
    process_white_presentation_image();

    // Gather all of the filenames from the relevant directories       
    std::vector<std::string> extensions = {".bin", ".BIN"};
    auto compacthexfiles = get_filenames_in_dir("../hex_images/", extensions);
    auto binaryFiles = get_filenames_in_dir("../binary_images/", extensions);

    // Combine the filenames into a single vector
    int numFiles = compacthexfiles.size() + binaryFiles.size();
    std::vector<std::string> allFileNames;
    allFileNames.reserve(numFiles);
    allFileNames.insert(allFileNames.end(), compacthexfiles.begin(), compacthexfiles.end());
    allFileNames.insert(allFileNames.end(), binaryFiles.begin(), binaryFiles.end());

    // Load the images
    auto hexImages = load_compact_hex_images(compacthexfiles, true);
    auto binImages = load_binary_images(binaryFiles, true);

    // Combine the images into a single vector
    std::vector<cv::Mat> images;
    images.reserve(numFiles);
    images.insert(images.end(), hexImages.begin(), hexImages.end());
    images.insert(images.end(), binImages.begin(), binImages.end());

    // Convert the images to RGB888 for later use
    auto rgb888Images = convert_rgb565_to_rgb888(images);

    // Reserve space for all the processed images
    std::vector<cv::Mat> combinedMasks;
    combinedMasks.reserve(numFiles);

    // Process the images
    for (const auto& img : images) {
        cv::Mat3b combMat = cv::Mat::zeros(img.size(), CV_8UC3);

        // Process the image for the white line
        cv::Mat1b center;
        cv::Mat1b wmask;
        int8_t dist;

        MicroCV2::processWhiteImg(img, wmask, center, dist);
        auto whitemask = MicroCV2::colorizeMask(wmask, {255,255,255});
        auto centermask = MicroCV2::colorizeMask(center, {0,255,0});

        // Process the image for the stop line
        cv::Mat1b rmask;

        MicroCV2::processRedImg(img, rmask);
        auto redmask = MicroCV2::colorizeMask(rmask, {255,0,0});

        // Layer all the masks into a single processed image
        MicroCV2::layerMask(combMat, whitemask);
        MicroCV2::layerMask(combMat, centermask);
        MicroCV2::layerMask(combMat, redmask);
        combinedMasks.push_back(combMat);

    }

    // Display all the images and their processed versions in windows
    QT5::showImageWindows(argc, argv, rgb888Images, combinedMasks, allFileNames);
    return 0;
}