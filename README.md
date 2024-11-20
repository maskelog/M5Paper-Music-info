# M5Paper-Music-info

![Screenshot 1](https://github.com/user-attachments/assets/b8d29e9f-9be1-45a5-89d6-c63535029f24)
![Screenshot 2](https://github.com/user-attachments/assets/734eb89a-14b4-4622-a984-47465fdf760d)

## Description

This project demonstrates how to use A2DP over Bluetooth to fetch music metadata and display it on the M5Paper screen.

## Features

- Connect to Bluetooth devices using A2DP
- Fetch and display music metadata (e.g., title, artist, album)
- Update the display in real-time as the music changes

## Requirements

- M5Paper
- PlatformIO
- Bluetooth-enabled music source

## Installation

1. Clone this repository.
2. Open the project with PlatformIO.
3. Build and upload the project to your M5Paper.

## Usage

1. Power on your M5Paper.
2. Pair it with your Bluetooth-enabled music source.
3. Start playing music on the source device.
4. The M5Paper will display the current music metadata on the screen.

## Libraries Used

- [tanakamasayuki/efont Unicode Font Data](https://github.com/pschatzmann/ESP32-A2DP)
- [pschatzmann/ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP)
- [pschatzmann/arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools.git)
- [moononournation/GFX Library for Arduino@^1.5.0](https://github.com/moononournation/GFX_Library_for_Arduino)
- [lovyan03/LovyanGFX@^1.1.16](https://github.com/lovyan03/LovyanGFX)
- [lvgl/lvgl@8.4.0](https://github.com/lvgl/lvgl)
- [m5stack/M5EPD](https://github.com/m5stack/M5EPD)

## License

This project is licensed under the MIT License.
