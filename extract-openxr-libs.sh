#!/bin/bash
# Script to extract OpenXR loader native libraries from AAR

set -e

OPENXR_VERSION="1.1.43"

echo "Extracting OpenXR loader libraries from AAR..."
echo ""

# Try to find AAR in Gradle cache first
AAR_PATH=$(find ~/.gradle/caches/modules-2/files-2.1/org.khronos.openxr/openxr_loader_for_android/${OPENXR_VERSION} -name "*.aar" 2>/dev/null | head -1)

if [ -z "$AAR_PATH" ]; then
    echo "AAR not in cache, downloading from Maven Central..."
    AAR_URL="https://repo1.maven.org/maven2/org/khronos/openxr/openxr_loader_for_android/${OPENXR_VERSION}/openxr_loader_for_android-${OPENXR_VERSION}.aar"
    AAR_PATH="/tmp/openxr_loader.aar"
    curl -sSL "$AAR_URL" -o "$AAR_PATH"

    if [ $? -ne 0 ] || [ ! -f "$AAR_PATH" ]; then
        echo "❌ Failed to download AAR from Maven Central"
        exit 1
    fi
    echo "✓ Downloaded AAR"
fi

echo "Using AAR: $AAR_PATH"
echo ""

# Create directories for each architecture
DEST_DIR="app/src/main/jni/moonlight-openxr"
for arch in armeabi-v7a arm64-v8a x86 x86_64; do
    mkdir -p "$DEST_DIR/$arch"
done

# Extract the AAR temporarily
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"
unzip -q "$AAR_PATH"

echo "Copying libraries for each architecture..."

# Map Android architecture names to prefab names
declare -A arch_map=(
    ["armeabi-v7a"]="android.armeabi-v7a"
    ["arm64-v8a"]="android.arm64-v8a"
    ["x86"]="android.x86"
    ["x86_64"]="android.x86_64"
)

for arch in armeabi-v7a arm64-v8a x86 x86_64; do
    prefab_arch="${arch_map[$arch]}"
    prefab_path="prefab/modules/openxr_loader/libs/$prefab_arch/libopenxr_loader.so"

    if [ -f "$prefab_path" ]; then
        cp "$prefab_path" "$OLDPWD/$DEST_DIR/$arch/"
        size=$(stat -f%z "$OLDPWD/$DEST_DIR/$arch/libopenxr_loader.so" 2>/dev/null || stat -c%s "$OLDPWD/$DEST_DIR/$arch/libopenxr_loader.so" 2>/dev/null)
        echo "  ✓ $arch ($size bytes)"
    else
        echo "  ✗ $arch (not found at $prefab_path)"
    fi
done

# Cleanup
cd "$OLDPWD"
rm -rf "$TEMP_DIR"

echo ""
echo "✓ OpenXR loader libraries extracted successfully!"
echo ""
echo "Libraries installed in:"
ls -lhR "$DEST_DIR"/*/libopenxr_loader.so
