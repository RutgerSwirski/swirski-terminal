#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"

cmake \
    -S desktop \
    -B desktop/build \
    -G Ninja \
    --log-level=VERBOSE \
    -DFETCHCONTENT_QUIET=OFF

cmake --build desktop/build

exec ./desktop/build/swirski_os_desktop