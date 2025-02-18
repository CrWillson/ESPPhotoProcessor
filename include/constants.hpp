#pragma once

#include <cstdint>

// The number of times I have watched this program recompile over 1000 files has nearly driven me insane.
// ~ Caleb Willson, 9/2024

constexpr uint8_t CAM_PIN_PWDN    = 32;
constexpr uint8_t CAM_PIN_RESET   = -1;
constexpr uint8_t CAM_PIN_XCLK    = 0;
constexpr uint8_t CAM_PIN_SIOD    = 26;
constexpr uint8_t CAM_PIN_SIOC    = 27;

constexpr uint8_t CAM_PIN_D7      = 35;
constexpr uint8_t CAM_PIN_D6      = 34;
constexpr uint8_t CAM_PIN_D5      = 39;
constexpr uint8_t CAM_PIN_D4      = 36;
constexpr uint8_t CAM_PIN_D3      = 21;
constexpr uint8_t CAM_PIN_D2      = 19;
constexpr uint8_t CAM_PIN_D1      = 18;
constexpr uint8_t CAM_PIN_D0      = 5;
constexpr uint8_t CAM_PIN_VSYNC   = 25;
constexpr uint8_t CAM_PIN_HREF    = 23;
constexpr uint8_t CAM_PIN_PCLK    = 22;

// If this is a "1", then send the raw image from the ESP-32 over the serial port. If 0, don't.
#define CALIBRATION_MODE 0

#define PRINT_LINE_THRESH 0
#define PRINT_STOP_THRESH 0
#define PRINT_COMBINED_THRESH 1

// The pin to write to.
#define TX_GPIO GPIO_NUM_1

// The UART device to use for control transmission.
#define UART_NUM UART_NUM_0

// The baudrate of the TX communication.
constexpr uint32_t tx_baud = 500000;

// SD Card file system constants
constexpr const char* MOUNT_POINT = "/sdcard";
constexpr const char* FILE_PREFIX = "IMAGE";
constexpr const char* FILE_EXTEN  = ".BIN";