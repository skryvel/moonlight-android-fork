#ifndef OPENXR_INPUT_H
#define OPENXR_INPUT_H

#include <jni.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Controller state structure matching Xbox-style gamepad
typedef struct {
    bool initialized;

    // Button states
    bool button_a;
    bool button_b;
    bool button_x;
    bool button_y;
    bool button_menu;
    bool button_lb;
    bool button_rb;
    bool button_ls_click;
    bool button_rs_click;

    // Analog inputs
    float left_trigger;
    float right_trigger;
    float left_stick_x;
    float left_stick_y;
    float right_stick_x;
    float right_stick_y;
} QuestControllerState;

// Initialize OpenXR and set up controller input
bool openxr_input_init(JNIEnv* env, jobject context);

// Poll controller state
bool openxr_input_poll(QuestControllerState* state);

// Cleanup OpenXR resources
void openxr_input_cleanup();

// Check if required buttons are present (at least 4 buttons, 2 triggers, 2 sticks)
bool openxr_input_validate_controller();

#ifdef __cplusplus
}
#endif

#endif // OPENXR_INPUT_H
