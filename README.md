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
The display backlight uses an active-low BC327 stage on GPIO 12; firmware
drives it low at boot and switches it off with the display timeout.

The MAX17048 fuel gauge is read at I2C address `0x36` every five seconds.
For the current direct-GPIO prototype, connect it as follows:

- MAX17048 SDA to ESP32-S3 GPIO 1
- MAX17048 SCL to ESP32-S3 GPIO 2
- MAX17048 VIN to 3.3 V and GND to GND

The two JST-PH battery sockets on the breakout are a pass-through pair: connect
the single-cell LiPo to either socket and the terminal's power/charging path to
the other. Battery voltage is retained in system state as millivolts, charge
percentage is shown in the status bar, and both readings are logged at debug
level. The PCB-v1 schematic instead reserves GPIO 8/9 for its shared I2C bus;
those pins must not be selected in this prototype firmware while the back and
encoder switches still use direct GPIO.

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

Pushes to `main` that change `mobile/` automatically build the Android app and
replace the rolling `mobile-latest` GitHub Release. The app checks that
release's small manifest when it opens and shows **Mobile update available**
when its installed build number is older. The download opens in Android's
normal APK installation flow.

The rolling workflow requires the same permanent signing key for every build so
Android can install each APK over the previous one. Reuse and securely back up
the key that signed any APK already installed on your phone. Only generate a
new key if there is no existing release key:

```bash
keytool -genkeypair -v \
  -keystore swirski-upload.jks \
  -alias swirski-upload \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000
```

Configure these repository secrets before running the workflow, replacing the
example path with the existing key when applicable:

```bash
base64 -w 0 path/to/swirski-upload.jks | gh secret set SWIRSKI_UPLOAD_KEYSTORE_BASE64
gh secret set SWIRSKI_UPLOAD_STORE_PASSWORD
gh secret set SWIRSKI_UPLOAD_KEY_ALIAS
gh secret set SWIRSKI_UPLOAD_KEY_PASSWORD
```

Use the alias entered above (`swirski-upload` in the example). The last three
commands prompt for their values. Never commit the key or passwords. Losing
the key means future APKs cannot update existing installations.

Local standalone builds made by `mobile/deploy.sh` use Android's development
key unless the same `SWIRSKI_UPLOAD_*` environment variables are supplied. If
the app currently installed on a phone uses that development key, uninstall it
once before installing the first `mobile-latest` APK, then re-enable Android's
notification-listener access. Subsequent rolling releases install as updates.

## Protocol

Messages use a versioned JSON envelope. Larger BLE messages are divided into
frames and reassembled by the receiver. See
[`docs/ble-framing.md`](docs/ble-framing.md) for the framing format.

## License

This project is available under the [MIT License](LICENSE).
