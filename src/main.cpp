#include "microcv2.hpp"
#include "opencv2.hpp"
#include <fmt/core.h>
#include "qt5.hpp"
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <array>
#include <span>

namespace fs = std::filesystem;


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

std::vector<cv::Mat> convert_rgb565_to_rgb888(std::span<const cv::Mat> rgb565_images) {
    std::vector<cv::Mat> rgb888_images;
    rgb888_images.reserve(rgb565_images.size());  // Preallocate memory for efficiency

    // Loop through each RGB565 image in the input span
    for (const auto& rgb565_image : rgb565_images) {
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

        rgb888_images.push_back(std::move(rgb888_image));  // Move the image into the vector
    }

    return rgb888_images;
}

std::vector<std::string> get_filenames_in_dir(const std::string& directory_path) {
    std::vector<std::string> filenames;

    try {
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {  // Check if it's a file (not a directory)
                filenames.push_back(entry.path().string());  // Get just the filename
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }

    return filenames;
}

int main(int argc, char *argv[]) {    
    auto filenames = get_filenames_in_dir("../images/");
    int width = 96;  // Set your image width
    int height = 96; // Set your image height
    
    // Load and convert RGB565 to RGB888
    auto images = load_rgb565_images(filenames, width, height);
    if (images.empty()) {
        fmt::print("Failed to load image.\n");
        return -1;
    }
    
    auto rgb888Images = convert_rgb565_to_rgb888(images);

    std::vector<cv::Mat1b> whiteMasks;
    for (const auto& img : images) {
        cv::Mat1b center;
        cv::Mat1b mask;
        int8_t dist, height;

        MicroCV2::processWhiteImg(img, mask, center, dist, height);

        whiteMasks.push_back(mask);
    }
    
    // Create the main window
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Main Window");
    QGridLayout *layout = new QGridLayout();

    // Convert to QImage
    auto whiteMasksQImgs = QT5::matToQImage(whiteMasks);
    auto rgb888QImgs = QT5::matToQImage(rgb888Images);

    layout->addWidget(QT5::createImageLabel(whiteMasksQImgs[0]), 0, 0);  // Row 0, Col 0
    layout->addWidget(QT5::createImageLabel(rgb888QImgs[0]), 0, 1);  // Row 0, Col 0
    layout->addWidget(QT5::createImageLabel(whiteMasksQImgs[1]), 0, 2);  // Row 0, Col 1
    layout->addWidget(QT5::createImageLabel(rgb888QImgs[1]), 0, 3);  // Row 0, Col 1
    layout->addWidget(QT5::createImageLabel(whiteMasksQImgs[2]), 1, 0);  // Row 1, Col 0
    layout->addWidget(QT5::createImageLabel(rgb888QImgs[2]), 1, 1);  // Row 1, Col 0
    layout->addWidget(QT5::createImageLabel(whiteMasksQImgs[3]), 1, 2);  // Row 1, Col 1
    layout->addWidget(QT5::createImageLabel(rgb888QImgs[3]), 1, 3);  // Row 1, Col 1

    // Set the layout for the window
    window.setLayout(layout);

    // Show the window
    window.show();

    return app.exec();
}
