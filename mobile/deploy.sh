#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

node_version_file="${script_dir}/.nvmrc"
nvm_directory="${NVM_DIR:-${HOME}/.nvm}"

if [[ -s "${nvm_directory}/nvm.sh" ]]; then
    export NVM_DIR="${nvm_directory}"
    # shellcheck source=/dev/null
    source "${NVM_DIR}/nvm.sh"

    required_node_version="$(<"${node_version_file}")"

    if ! nvm use --silent "${required_node_version}"; then
        echo "❌ Node ${required_node_version} is required. Install it with: nvm install ${required_node_version}"
        exit 1
    fi
fi

node_version="$(node --version 2>/dev/null || true)"
node_version="${node_version#v}"
node_major="${node_version%%.*}"
node_minor_and_patch="${node_version#*.}"
node_minor="${node_minor_and_patch%%.*}"

if [[ ! "${node_major}" =~ ^[0-9]+$ ]] ||
    [[ ! "${node_minor}" =~ ^[0-9]+$ ]] ||
    ((node_major < 22)) ||
    ((node_major == 22 && node_minor < 11)); then
    echo "❌ Node 22.11.0 or newer is required; found ${node_version:-no Node installation}."
    exit 1
fi

echo "🟢 Using Node ${node_version}"
node_executable="$(command -v node)"
gradle_node_argument="-PswirskiNodeExecutable=${node_executable}"

echo "🚀 Starting Standalone Build..."

echo "🧹 Cleaning previous builds..."
"${script_dir}/android/gradlew" \
    -p "${script_dir}/android" \
    "${gradle_node_argument}" \
    clean

echo "🏗️ Compiling bundled release APK..."
"${script_dir}/android/gradlew" \
    -p "${script_dir}/android" \
    "${gradle_node_argument}" \
    assembleRelease

echo "📲 Pushing APK to your phone..."
apk_path="${script_dir}/android/app/build/outputs/apk/release/app-release.apk"

if [[ ! -f "${apk_path}" ]]; then
    echo "❌ Error: Could not find standalone APK at ${apk_path}"
    exit 1
fi

echo "📦 Found APK at: ${apk_path}"
adb install -r "${apk_path}"
echo "✅ App successfully installed!"
