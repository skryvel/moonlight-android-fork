#!/bin/bash
# Script to verify Quest OpenXR library is included in the APK

APK_PATH="app/build/outputs/apk/nonRootQuest/debug/app-nonRootQuest-debug.apk"

if [ ! -f "$APK_PATH" ]; then
    echo "❌ Quest APK not found at: $APK_PATH"
    echo "Build it first with: ./gradlew assembleNonRootQuestDebug"
    exit 1
fi

echo "Checking for libmoonlight-openxr.so in APK..."
echo ""

# List all .so files in the APK
unzip -l "$APK_PATH" | grep "\.so$"

echo ""
echo "Looking specifically for libmoonlight-openxr.so..."
if unzip -l "$APK_PATH" | grep -q "libmoonlight-openxr.so"; then
    echo "✅ Found libmoonlight-openxr.so - Quest build is correct!"
    echo ""
    echo "Architectures:"
    unzip -l "$APK_PATH" | grep "libmoonlight-openxr.so"
else
    echo "❌ libmoonlight-openxr.so NOT FOUND - This is not a Quest build!"
    echo ""
    echo "Make sure you built with: ./gradlew assembleNonRootQuestDebug"
fi
