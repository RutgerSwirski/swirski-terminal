#!/usr/bin/env bash

set -e

cd "$(dirname "$0")/esp32"

source "$HOME/.espressif/v6.0.2/esp-idf/export.sh"

idf.py menuconfig