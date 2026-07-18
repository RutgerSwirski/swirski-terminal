# Swirski Terminal

Swirski Terminal is a small wearable terminal built around an ESP32-S3,
an ILI9341 display, LVGL, and a rotary encoder. An Android companion app sends
notifications, media state, time, and Wi-Fi configuration over an encrypted
BLE connection.

The shared C++ application can also run as an SDL desktop build for faster UI
development and protocol testing.

## Current Features

- Notification snapshots, live notifications, detail views, and toasts
- Music state, progress, play/pause, previous, and next controls
- BLE pairing, bonding, chunked messages, and automatic reconnection
- Wi-Fi scanning, connection management, signal status, and internet testing
- Phone-synchronized date and time with persistent fallback
- Rotary encoder navigation, virtual keyboard, settings, and screen timeout
- Pong and Blackjack
- Shared LVGL screens and services across ESP32 and desktop targets

## Repository Layout

```text
app/      Shared C++ application, screens, services, protocol, and UI
desktop/  SDL/LVGL desktop simulator and C++ tests
esp32/    ESP-IDF firmware, hardware input, display, Wi-Fi, and BLE transport
mobile/   React Native Android companion app
docs/     Protocol documentation
```

## Desktop Build

Requirements:

- CMake 3.22 or newer
- Ninja
- A C++20 compiler
- SDL2 development files

Build and launch the simulator from the repository root:

```bash
./buildDesktop.sh
```

Run the C++ tests after building:

```bash
ctest --test-dir desktop/build --output-on-failure
```

## ESP32 Build

The firmware currently targets an ESP32-S3 with 4 MB flash, an ILI9341
320x240 SPI display, a rotary encoder, and a back button. Hardware pins are
defined near the top of `esp32/main/main.cpp` and in `esp32/main/inputs/`.

Install ESP-IDF, then build from the `esp32` directory:

```bash
source "$HOME/.espressif/v6.0.2/esp-idf/export.sh"
idf.py build
```

Flash and monitor a connected device:

```bash
idf.py -p /dev/ttyACM0 -b 115200 flash monitor
```

Change the serial port when your device appears under a different path.

Pushes to `main` that change `app/` or `esp32/` automatically build the
firmware and update the `firmware-latest` GitHub Release. The release asset is
published as `swirski_os_esp32.bin` for the terminal's OTA updater. The same
workflow can also be started manually from the GitHub Actions page.

## Android Companion

Requirements:

- Node.js 22.11 or newer
- Android Studio and the Android SDK
- A physical Android device with Bluetooth Low Energy

Install dependencies and start Metro:

```bash
cd mobile
npm install
npm start
```

In another terminal, install the debug app:

```bash
cd mobile
npm run android
```

Android notification-listener access is required for notification sync and
media discovery. The app prompts for the relevant Bluetooth and notification
permissions when needed.

Release APKs are unsigned unless the private signing environment variables in
`mobile/android/app/build.gradle` are configured. Signing keys and passwords
must never be committed to this repository.

## Protocol

Messages use a versioned JSON envelope. Larger BLE messages are divided into
frames and reassembled by the receiver. See
[`docs/ble-framing.md`](docs/ble-framing.md) for the framing format.

## License

This project is available under the [MIT License](LICENSE).
