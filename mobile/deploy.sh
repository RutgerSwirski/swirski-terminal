#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

required_signing_variables=(
    SWIRSKI_UPLOAD_STORE_FILE
    SWIRSKI_UPLOAD_STORE_PASSWORD
    SWIRSKI_UPLOAD_KEY_ALIAS
    SWIRSKI_UPLOAD_KEY_PASSWORD
)

for variable_name in "${required_signing_variables[@]}"; do
    if [[ -z "${!variable_name:-}" ]]; then
        echo "Missing required release signing variable: ${variable_name}"
        exit 1
    fi
done

echo "🚀 Starting Production Release Build..."

echo "🧹 Cleaning previous builds..."
"${script_dir}/android/gradlew" -p "${script_dir}/android" clean

echo "🏗️ Compiling release APK..."
"${script_dir}/android/gradlew" -p "${script_dir}/android" assembleRelease

echo "📲 Pushing APK to your phone..."
apk_path="${script_dir}/android/app/build/outputs/apk/release/app-release.apk"

if [[ ! -f "${apk_path}" ]]; then
    echo "❌ Error: Could not find signed APK at ${apk_path}"
    exit 1
fi

echo "📦 Found APK at: ${apk_path}"
adb install -r "${apk_path}"
echo "✅ App successfully installed!"
