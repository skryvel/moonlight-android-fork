#include <jni.h>
#include <android/log.h>
#include "openxr_input.h"

#define LOG_TAG "OpenXR_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/*
 * Class:     com_limelight_binding_input_driver_QuestController
 * Method:    nativeInit
 * Signature: (Landroid/content/Context;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_limelight_binding_input_driver_QuestController_nativeInit(JNIEnv *env, jclass clazz, jobject context) {
    LOGI("Initializing OpenXR input");
    return (jboolean)openxr_input_init(env, context);
}

/*
 * Class:     com_limelight_binding_input_driver_QuestController
 * Method:    nativePoll
 * Signature: ([F[Z)Z
 *
 * Parameters:
 * - floatArray: [leftTrigger, rightTrigger, leftStickX, leftStickY, rightStickX, rightStickY]
 * - boolArray: [A, B, X, Y, Menu, LB, RB, LS_Click, RS_Click]
 */
JNIEXPORT jboolean JNICALL
Java_com_limelight_binding_input_driver_QuestController_nativePoll(JNIEnv *env, jclass clazz,
                                                                     jfloatArray floatArray,
                                                                     jbooleanArray boolArray) {
    QuestControllerState state;

    if (!openxr_input_poll(&state)) {
        return JNI_FALSE;
    }

    // Fill float array (analog inputs)
    jfloat floats[6];
    floats[0] = state.left_trigger;
    floats[1] = state.right_trigger;
    floats[2] = state.left_stick_x;
    floats[3] = state.left_stick_y;
    floats[4] = state.right_stick_x;
    floats[5] = state.right_stick_y;
    (*env)->SetFloatArrayRegion(env, floatArray, 0, 6, floats);

    // Fill boolean array (button states)
    jboolean bools[9];
    bools[0] = state.button_a ? JNI_TRUE : JNI_FALSE;
    bools[1] = state.button_b ? JNI_TRUE : JNI_FALSE;
    bools[2] = state.button_x ? JNI_TRUE : JNI_FALSE;
    bools[3] = state.button_y ? JNI_TRUE : JNI_FALSE;
    bools[4] = state.button_menu ? JNI_TRUE : JNI_FALSE;
    bools[5] = state.button_lb ? JNI_TRUE : JNI_FALSE;
    bools[6] = state.button_rb ? JNI_TRUE : JNI_FALSE;
    bools[7] = state.button_ls_click ? JNI_TRUE : JNI_FALSE;
    bools[8] = state.button_rs_click ? JNI_TRUE : JNI_FALSE;
    (*env)->SetBooleanArrayRegion(env, boolArray, 0, 9, bools);

    return JNI_TRUE;
}

/*
 * Class:     com_limelight_binding_input_driver_QuestController
 * Method:    nativeCleanup
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_limelight_binding_input_driver_QuestController_nativeCleanup(JNIEnv *env, jclass clazz) {
    LOGI("Cleaning up OpenXR input");
    openxr_input_cleanup();
}

/*
 * Class:     com_limelight_binding_input_driver_QuestController
 * Method:    nativeValidate
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_com_limelight_binding_input_driver_QuestController_nativeValidate(JNIEnv *env, jclass clazz) {
    return (jboolean)openxr_input_validate_controller();
}
