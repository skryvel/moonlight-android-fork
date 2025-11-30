#!/bin/bash
# Script to download OpenXR headers for native compilation

OPENXR_VERSION="1.1.43"
HEADERS_DIR="app/src/main/jni/moonlight-openxr/include/openxr"
BASE_URL="https://raw.githubusercontent.com/KhronosGroup/OpenXR-SDK/release-${OPENXR_VERSION}/include/openxr"

echo "Downloading OpenXR ${OPENXR_VERSION} headers..."
echo ""

mkdir -p "$HEADERS_DIR"

# Download main headers
HEADERS=(
    "openxr.h"
    "openxr_platform.h"
    "openxr_platform_defines.h"
    "openxr_reflection.h"
)

for header in "${HEADERS[@]}"; do
    echo "Downloading $header..."
    curl -sSL "${BASE_URL}/${header}" -o "${HEADERS_DIR}/${header}"

    if [ $? -eq 0 ] && [ -f "${HEADERS_DIR}/${header}" ]; then
        size=$(stat -f%z "${HEADERS_DIR}/${header}" 2>/dev/null || stat -c%s "${HEADERS_DIR}/${header}" 2>/dev/null)
        echo "  ✓ Downloaded ($size bytes)"
    else
        echo "  ✗ Failed to download"
        exit 1
    fi
done

echo ""
echo "✓ All OpenXR headers downloaded successfully!"
echo ""
echo "Headers installed in: $HEADERS_DIR"
ls -lh "$HEADERS_DIR"
