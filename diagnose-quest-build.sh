#!/bin/bash
# Debug script to diagnose Quest OpenXR build issues

echo "====== Quest OpenXR Build Diagnosis ======"
echo ""

echo "1. Checking file structure..."
echo ""

MAIN_JNI="app/src/main/jni"
QUEST_JNI="app/src/quest/jni/moonlight-openxr"
OPENXR_SOURCES="app/src/main/jni/moonlight-core"

echo "Main JNI directory:"
ls -la "$MAIN_JNI/Android.mk" 2>/dev/null && echo "  ✓ Main Android.mk exists" || echo "  ✗ Main Android.mk missing"

echo ""
echo "Quest JNI directory:"
ls -la "$QUEST_JNI/Android.mk" 2>/dev/null && echo "  ✓ Quest Android.mk exists" || echo "  ✗ Quest Android.mk missing"

echo ""
echo "OpenXR source files:"
ls -la "$OPENXR_SOURCES/openxr_input.c" 2>/dev/null && echo "  ✓ openxr_input.c exists" || echo "  ✗ openxr_input.c missing"
ls -la "$OPENXR_SOURCES/openxr_jni.c" 2>/dev/null && echo "  ✓ openxr_jni.c exists" || echo "  ✗ openxr_jni.c missing"
ls -la "$OPENXR_SOURCES/openxr_input.h" 2>/dev/null && echo "  ✓ openxr_input.h exists" || echo "  ✗ openxr_input.h missing"

echo ""
echo "2. Checking path from main Android.mk to Quest Android.mk..."
echo ""

MAIN_DIR=$(dirname "$MAIN_JNI/Android.mk")
QUEST_MK_PATH="$MAIN_DIR/../../quest/jni/moonlight-openxr/Android.mk"
echo "Path used in Android.mk: \$(MY_LOCAL_PATH)/../../quest/jni/moonlight-openxr/Android.mk"
echo "Resolved path: $QUEST_MK_PATH"

if [ -f "$QUEST_MK_PATH" ]; then
    echo "  ✓ Path is correct"
else
    echo "  ✗ Path is WRONG - file not found"
    echo ""
    echo "Trying to find the correct relative path..."
    ACTUAL_QUEST_MK=$(find app/src -name "Android.mk" -path "*/quest/jni/moonlight-openxr/*" 2>/dev/null | head -1)
    if [ -n "$ACTUAL_QUEST_MK" ]; then
        echo "  Quest Android.mk actually at: $ACTUAL_QUEST_MK"
    fi
fi

echo ""
echo "3. Checking build.gradle configuration..."
echo ""

if grep -q 'arguments "PRODUCT_FLAVOR=nonRoot", "HAS_OPENXR=1"' app/build.gradle; then
    echo "  ✓ nonRootQuest passes HAS_OPENXR=1"
else
    echo "  ✗ nonRootQuest does NOT pass HAS_OPENXR=1"
fi

if grep -q 'buildConfigField "boolean", "HAS_OPENXR", "true"' app/build.gradle | grep -A5 nonRootQuest; then
    echo "  ✓ nonRootQuest has BuildConfig.HAS_OPENXR = true"
else
    echo "  ✗ nonRootQuest BuildConfig issue"
fi

echo ""
echo "4. Testing if Quest module Android.mk is valid..."
echo ""

if [ -f "$QUEST_JNI/Android.mk" ]; then
    echo "Quest Android.mk content:"
    head -20 "$QUEST_JNI/Android.mk"
else
    echo "  ✗ Cannot test - file not found"
fi

echo ""
echo "====== Diagnosis Complete ======"
echo ""
echo "If all checks pass, try:"
echo "  ./gradlew clean"
echo "  ./gradlew assembleNonRootQuestDebug --info 2>&1 | grep -i openxr"
echo ""
