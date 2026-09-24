# Smart Parcel Locker

A PlatformIO project for an ESP32-based smart parcel locker that uses an ultrasonic sensor to detect parcel presence, a relay-controlled lock, and a web dashboard to monitor the system and control the lock remotely.

## Overview

This project turns an ESP32 into a compact smart locker controller with:

- Wi-Fi connectivity
- Web server dashboard served from the ESP32
- Ultrasonic distance sensing for parcel detection
- Relay-driven lock control
- Camera integration via an IP webcam or similar streaming source
- Manual lock/unlock controls from the browser
- Snapshot capture and recent image history

The firmware exposes a dashboard over HTTP and can be accessed on the ESP32's local IP in a browser.

## Features

- Smart door/locker control with lock and unlock actions
- Parcel detection using an ultrasonic sensor
- Automatic snapshot capture when a parcel is detected
- Live camera preview from an IP camera feed
- Activity log for status changes
- Service worker/PWA-style dashboard for browser use
- PlatformIO project ready for ESP32 development

## Hardware used

This project is designed around:

- ESP32 development board
- Ultrasonic sensor
- Relay module or solenoid control output
- Wi-Fi network
- IP camera (Android IP Webcam or compatible camera endpoint)

### Typical pin mapping

The firmware uses these GPIO pins:

- `RELAY_PIN` -> GPIO 26
- `ULTRASONIC_TRIG_PIN` -> GPIO 33
- `ULTRASONIC_ECHO_PIN` -> GPIO 32

> The project currently assumes the sensor is connected with the same wiring used in the source code. The ultrasonic test sketch in [test/ultra_test.cpp](test/ultra_test.cpp) shows the recommended sensor wiring and test procedure.

## Project structure

- [src/main.cpp](src/main.cpp) — main ESP32 firmware
- [test/ultra_test.cpp](test/ultra_test.cpp) — ultrasonic sensor test sketch
- [data/dashboard.html](data/dashboard.html) — standalone dashboard HTML reference
- [platformio.ini](platformio.ini) — PlatformIO project configuration

## Configuration

Before uploading the firmware, update the Wi-Fi and camera settings in [src/main.cpp](src/main.cpp):

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
#define CAMERA_IP  "http://YOUR_CAMERA_IP:PORT"
```

Make sure:

- The ESP32 is connected to the same Wi-Fi network as the browser used to access the dashboard
- The camera URL points to a valid live-stream or JPEG endpoint
- The relay output is wired correctly to your lock mechanism

## Build and upload

### Prerequisites

- VS Code
- PlatformIO extension
- ESP32 board support installed in PlatformIO

### Build

```bash
pio run
```

### Upload to ESP32

```bash
pio run --target upload
```

### Monitor serial output

```bash
pio device monitor
```

## Running the dashboard

1. Power on the ESP32.
2. Connect your laptop or phone to the same Wi-Fi network.
3. Open the serial monitor and find the assigned ESP32 IP address.
4. Visit the dashboard in a browser:

```text
http://<ESP32_IP>
```

From the page, you can:

- lock or unlock the parcel box
- monitor parcel detection status
- view the live camera feed
- capture a snapshot manually
- see the recent activity log

## Important notes

- The project contains Wi-Fi credentials in the source file for local testing. Update them before deployment or reuse.
- The relay logic is configured for the current hardware behavior used in the firmware; verify the polarity matches your actual lock/solenoid hardware.
- The camera endpoint should be reachable by the ESP32 and should serve a JPEG snapshot if the dashboard is expected to fetch images.
- The file [data/dashboard.html](data/dashboard.html) appears to be a standalone HTML dashboard version and is useful as a reference or for editing the front-end separately.

## License

This project does not include a license file. If you plan to share or distribute it publicly, add an appropriate open source license before doing so.

## Suggested next improvements

- Move Wi-Fi and camera settings to a config file or credentials manager
- Add secure authentication for the web dashboard
- Add notifications for parcel arrival or door events
- Improve sensor calibration and debounce logic
- Add persistent storage for logs and snapshots

## Quick reminder

This is a custom embedded IoT project for a parcel locker system. It is best suited for hobby, prototype, or educational use, and can be extended for real-world deployment with stronger safety, authentication, and reliability improvements.
