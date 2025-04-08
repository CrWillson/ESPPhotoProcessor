#include "microcv2.hpp"
#include "opencv2.hpp"
#include <fmt/core.h>
#include "qt5.hpp"
#include <fstream>
#include <qapplication.h>
#include <string>
#include <filesystem>
#include <type_traits>
#include <vector>
#include <array>
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

            // Extract individual color components (5-bit Red, 6-bit Green, 5-bit Blue)
            uint8_t r = (pixel >> 11) & 0x1F;  // Extract red (5 bits)
            uint8_t g = (pixel >> 5) & 0x3F;   // Extract green (6 bits)
            uint8_t b = pixel & 0x1F;          // Extract blue (5 bits)

            // Scale the components to 0-255 range
            uint8_t r_scaled = (r * 255) / 31;  // Scale red from 5 bits to 8 bits
            uint8_t g_scaled = (g * 255) / 63;  // Scale green from 6 bits to 8 bits
            uint8_t b_scaled = (b * 255) / 31;  // Scale blue from 5 bits to 8 bits

            // Set the RGB888 pixel in the output image
            rgb888_image.at<cv::Vec3b>(row, col) = cv::Vec3b(b_scaled, g_scaled, r_scaled);  // OpenCV uses BGR order
        }
    }

    return rgb888_image;
}

/**
 * @brief Vectorized version of convert_rgb565_to_rgb888. Converts an entire span of RGB565 images to RGB888.
 * 
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
        cv::imwrite(filename + std::string(".png"), rgb888image);
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



int main(int argc, char *argv[]) {
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