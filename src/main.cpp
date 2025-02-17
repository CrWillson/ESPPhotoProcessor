#include <opencv2/opencv.hpp>
#include <fmt/core.h>
#include <QApplication>
#include <QLabel>
#include <QPixmap>
#include <QImage>

cv::Mat load_raw_rgb565(const std::string& filename, int width, int height) {
    // Open file
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        fmt::print("Error: Could not open file {}\n", filename);
        return cv::Mat();
    }

    
    // Check the first few bytes of the file
    uint16_t test_pixel;
    fread(&test_pixel, sizeof(test_pixel), 1, file);
    fmt::print("First pixel: 0x{:04X}\n", test_pixel); // Print as hexadecimal

    // Read raw data into a Mat (CV_8UC2)
    cv::Mat rgb565_image(height, width, CV_8UC2);
    fread(rgb565_image.data, 1, rgb565_image.total() * rgb565_image.elemSize(), file);
    fclose(file);

    return rgb565_image;
}

cv::Mat convert_rgb565_to_rgb888(const cv::Mat& rgb565_image) {
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

            // uint8_t r=0, g=0, b=0;

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


// Convert OpenCV Mat to QImage for Qt display
QImage cvMatToQImage(const cv::Mat& mat) {
    return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
}

int main(int argc, char *argv[]) {
    // QApplication app(argc, argv);

    std::string filename = "../images/IMAGE4.BIN";  // Path to the raw file
    int width = 96;  // Set your image width
    int height = 96; // Set your image height

    // Load and convert RGB565 to RGB888
    cv::Mat rgb565_image = load_raw_rgb565(filename, width, height);
    if (rgb565_image.empty()) {
        fmt::print("Failed to load image.\n");
        return -1;
    }

    cv::Mat rgb888_image = convert_rgb565_to_rgb888(rgb565_image);

    cv::imshow("RGB888 Image", rgb888_image);
    cv::waitKey(0);  // Wait for a key press


    // // Convert to QImage
    // QImage qimg = cvMatToQImage(rgb888_image);

    // // Display in a QLabel window
    // QLabel label;
    // label.setPixmap(QPixmap::fromImage(qimg));
    // label.show();

    // return app.exec();
    return 0;
}
