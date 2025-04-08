# ESP Photo Processor
Written by Caleb Willson

## Summary
This program is intended to be used as a test bed for new parameters and image processing pipelines for the SafeTown Senior Robot. 

Images can be loaded into the program through two means outlined below.

### 1. Integrated Line Following Program Image Saving
> This method will require the virtual file system to be set up on the Pi Pico and for a substantial portion of the flash memory to be allocated to the file system.

The regular line following program for the ESP32 has the ability to send photos to the Pi Pico when requested. To do this, the Pi Pico needs to send a `COMMAND_PACKET` type packet with the `CMD_REQUEST_IMAGE` command. Upon receiving an image request command, the next packet sent from the ESP32 to the Pico will contain an entire image. The Pico will then handle saving that image to its virtual file system.

Once the image is saved on the Pico, the user can navigate to it using the Pico's file explorer menu and select the option to print it over the serial line. This will, as the name implies, dump the entire file's contents onto the serial line. The purpose of `serial_monitor.py` is to read the serial line and automatically save any image files it sees back into a new file on the computer. 

When an image is saved in this way, it will be saved in a compact hex format. This format is human readable and consists of 4 hex characters for each pixel and a newline for every row of pixels. In the current program, these files are loaded from the `/hex_images/` directory.

### 2. SD-Card Image Saving Program
> This method will requre a FAT32 formatted SD card and some means of reading an SD card on a computer.

If the integrated method is too slow, there is also a dedicated program available for saving images from the ESP32. The **ESPCamera** is not capable of line following and only exists to save images to the SD card slot built onto the ESP32. 

When the ESP32 is running the **ESPCamera** program, it will save one image to the SD card every time it boots up. Then, the reset button on the ESP32 can effectively be used as a shutter button, causing the ESP32 to reset and take a new photo.

When an image is saved this way, it will be saved as its raw binary. This format is not human readable but is more space effecient. It does not require the use of the serial port as the files can be transfered directly to the computer via the SD card. In the current program, images of this format are loaded from the `/binary_images/` directory. 

## Dependencies
This program requires both the QT5-base and OpenCV libraries to be installed through vcpkg. It also requires the use of the Visual Studio Community 2022 Release- amd64 compiler. 

