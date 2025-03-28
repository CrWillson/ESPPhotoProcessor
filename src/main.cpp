#include "constants.hpp"
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

constexpr uint16_t RGB888toRGB565(const uint8_t r, const uint8_t g, const uint8_t b) {
    uint16_t red = (r * 31) / 255;
    uint16_t green = (g * 63) / 255;
    uint16_t blue = (b * 31) / 255;

    return (red << 11) | (green << 5) | blue;
}

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

std::vector<cv::Mat> convert_rgb565_to_rgb888(std::span<const cv::Mat> rgb565_images) {
    std::vector<cv::Mat> rgb888_images;
    rgb888_images.reserve(rgb565_images.size());  // Preallocate memory for efficiency

    // Loop through each RGB565 image in the input span
    for (const auto& rgb565_image : rgb565_images) {
        rgb888_images.push_back(std::move(convert_rgb565_to_rgb888(rgb565_image)));  // Move the image into the vector
    }

    return rgb888_images;
}

void generateColorBars(cv::Mat& image) {
    uint16_t colors[] = {
        RGB888toRGB565(255, 0, 0),    // Red        - 0xf800
        RGB888toRGB565(0, 255, 0),    // Green      - 0x07e0
        RGB888toRGB565(0, 0, 255),    // Blue       - 0x001f
        RGB888toRGB565(255, 255, 0),  // Yellow     - 0xffe0
        RGB888toRGB565(0, 255, 255),  // Cyan       - 0x07ff
        RGB888toRGB565(255, 0, 255)   // Magenta    - 0xf81f
    };

    int barWidth = IMG_COLS / 6; // Each bar takes 1/6 of the width

    for (int y = 0; y < IMG_ROWS; y++) {
        for (int x = 0; x < IMG_COLS; x++) {
            int barIndex = x / barWidth; // Determine which color bar to use
            uint16_t color = colors[barIndex];

            // Store color as two bytes (little-endian)
            cv::Vec2b& pixel = image.at<cv::Vec2b>(y, x);
            pixel[0] = color & 0xFF;       // Lower byte
            pixel[1] = (color >> 8) & 0xFF; // Upper byte
        }
    }
}

cv::Mat loadBinaryImage(const std::string& filename) {
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

    // Make a deep copy to ensure it remains valid after buffer goes out of scope
    return image.clone();
}

cv::Mat loadHexImage(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return cv::Mat();
    }

    std::vector<uint16_t> hexNumbers;
    hexNumbers.reserve(96*96);
    std::string line;
    bool inImageBlock = false;

    while (std::getline(file, line)) {
        // Check for start and end markers
        if (line == "START IMAGE") {
            inImageBlock = true;
            continue;
        }
        if (line == "END IMAGE") {
            inImageBlock = false;
            break;
        }

        if (inImageBlock) {
            std::istringstream iss(line);
            std::string hexStr;
            while (iss >> hexStr) {  // Read each hex number
                uint16_t value = static_cast<uint16_t>(std::stoul(hexStr, nullptr, 16));
                hexNumbers.push_back(value);
            }
        }
    }

    cv::Mat image(IMG_ROWS, IMG_COLS, CV_8UC2);

    for (int i = 0; i < IMG_ROWS; i++) {
        for (int j = 0; j < IMG_COLS; j++) {
            uint16_t value = hexNumbers[i * IMG_COLS + j];
            image.at<cv::Vec2b>(i, j) = cv::Vec2b(static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF));
        }
    }

    return image;
}

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

std::vector<cv::Mat> load_rgb565_images(std::span<const std::string> filenames, int rows, int cols) {
    std::vector<cv::Mat> images;
    images.reserve(filenames.size());  // Preallocate memory for efficiency

    for (const auto& filename : filenames) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // Create an OpenCV matrix to store the RGB565 image
        cv::Mat image(rows, cols, CV_8UC2);

        // Read the file line by line
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                int rgb565;
                if (!(file >> rgb565)) {
                    throw std::runtime_error("Error reading file: " + filename);
                }

                // Store the 16-bit value as two 8-bit bytes
                image.at<cv::Vec2b>(row, col)[0] = (rgb565 >> 8) & 0xFF;  // High byte
                image.at<cv::Vec2b>(row, col)[1] = rgb565 & 0xFF;         // Low byte
            }
        }

        file.close();
        images.push_back(std::move(image));  // Move the matrix into the vector
    }

    return images;
}

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
    std::vector<std::string> extensions = {".bin", ".BIN"};
    auto compacthexfiles = get_filenames_in_dir("../hex_images/", extensions);
    int numFiles = compacthexfiles.size();

    auto images = load_compact_hex_images(compacthexfiles, true);

    auto rgb888Images = convert_rgb565_to_rgb888(images);

    std::vector<cv::Mat> combinedMasks;
    combinedMasks.reserve(numFiles);

    for (const auto& img : images) {
        cv::Mat3b combMat = cv::Mat::zeros(img.size(), CV_8UC3);

        cv::Mat1b center;
        cv::Mat1b wmask;
        int8_t dist, height;

        MicroCV2::processWhiteImg(img, wmask, center, dist, height);
        auto whitemask = MicroCV2::colorizeMask(wmask, {255,255,255});
        auto centermask = MicroCV2::colorizeMask(center, {0,255,0});

        cv::Mat1b rmask;

        MicroCV2::processRedImg(img, rmask);
        auto redmask = MicroCV2::colorizeMask(rmask, {255,0,0});

        MicroCV2::layerMask(combMat, whitemask);
        MicroCV2::layerMask(combMat, centermask);
        MicroCV2::layerMask(combMat, redmask);
        combinedMasks.push_back(combMat);

    }


    QT5::showImageWindows(argc, argv, rgb888Images, combinedMasks);
    return 0;
}