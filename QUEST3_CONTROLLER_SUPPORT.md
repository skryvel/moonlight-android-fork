# Meta Quest 3 Controller Support

This document describes the implementation of Meta Quest 3 controller support for Moonlight Android using OpenXR.

## Overview

Meta Quest 3 controllers are now supported as gamepad input devices. The controllers are mapped to an Xbox-style gamepad layout, allowing you to use them for game streaming.

## Implementation

### Architecture

The implementation consists of three main components:

1. **Native OpenXR Integration** (`openxr_input.c/h`)
   - Initializes OpenXR instance and session
   - Creates action sets for controller input
   - Polls controller state (buttons, triggers, thumbsticks)
   - Maps to Oculus Touch controller interaction profile

2. **JNI Bridge** (`openxr_jni.c`)
   - Provides Java Native Interface methods
   - Transfers controller state between native and Java layers

3. **Java Controller Driver** (`QuestController.java`)
   - Extends `AbstractController`
   - Manages controller lifecycle
   - Reports input to Moonlight's input system
   - Validates required buttons are present

### Button Mapping

Quest 3 controllers are mapped to Xbox-style gamepad as follows:

| Quest Controller Input | Xbox Gamepad Equivalent |
|------------------------|-------------------------|
| Left Thumbstick        | Left Analog Stick       |
| Right Thumbstick       | Right Analog Stick      |
| Left Trigger           | LT (Left Trigger)       |
| Right Trigger          | RT (Right Trigger)      |
| Left Grip              | LB (Left Bumper)        |
| Right Grip             | RB (Right Bumper)       |
| A Button (right)       | A                       |
| B Button (right)       | B                       |
| X Button (left)        | X                       |
| Y Button (left)        | Y                       |
| Left Stick Click       | LS (Left Stick Click)   |
| Right Stick Click      | RS (Right Stick Click)  |
| Menu Button (left)     | Start/Menu              |

### Validation

The implementation includes validation to ensure the Quest controllers have all required inputs:
- At least 4 face buttons (A, B, X, Y)
- 2 analog triggers
- 2 analog thumbsticks with click functionality
- 2 grip buttons

If validation fails, a `RuntimeException` is thrown with a descriptive error message.

## Usage

### Initialization

```java
// Initialize Quest controller
Context context = getApplicationContext();
int deviceId = 0; // Controller ID
UsbDriverListener listener = ...; // Your listener implementation

try {
    QuestController controller = QuestController.create(context, deviceId, listener);
    controller.start();
} catch (RuntimeException e) {
    // Handle initialization failure
    Log.e(TAG, "Failed to initialize Quest controller: " + e.getMessage());
}
```

### Cleanup

```java
// When done, cleanup OpenXR resources
QuestController.cleanup();
```

## Dependencies

- **OpenXR Loader**: `org.khronos.openxr:openxr_loader_for_android:1.0.34`
- Android NDK r27
- Minimum SDK: 21

## OpenXR Actions

The implementation uses the following OpenXR actions:

### Boolean Actions
- `button_a` - A button (right controller)
- `button_b` - B button (right controller)
- `button_x` - X button (left controller)
- `button_y` - Y button (left controller)
- `button_menu` - Menu button
- `grip_left` - Left grip button
- `grip_right` - Right grip button
- `thumbstick_click_left` - Left stick click
- `thumbstick_click_right` - Right stick click

### Float Actions
- `trigger_left` - Left trigger (analog)
- `trigger_right` - Right trigger (analog)

### Vector2 Actions
- `thumbstick_left` - Left thumbstick (2D analog)
- `thumbstick_right` - Right thumbstick (2D analog)

## Interaction Profile

The implementation suggests bindings for the Oculus Touch controller profile:
`/interaction_profiles/oculus/touch_controller`

## Limitations

- **Haptics**: Rumble/haptic feedback is not yet implemented
  - Future implementation will use `XR_FB_haptic_amplitude_envelope` extension
- **Trigger Rumble**: Quest controllers don't have trigger motors
- **D-Pad**: Quest controllers don't have a physical D-pad (would require mapping thumbstick to D-pad)

## Future Enhancements

1. **Haptic Feedback**: Implement rumble using OpenXR haptics extensions
2. **Hand Tracking**: Optional hand tracking support via `XR_META_simultaneous_hands_and_controllers`
3. **Controller Battery**: Report battery state via `XR_EXT_hand_tracking`
4. **Advanced Features**: Support for Quest Pro features (face/eye tracking, controller haptics)

## Testing

To test the Quest 3 controller support:

1. Build and install the app on Meta Quest 3
2. Launch Moonlight
3. The QuestController will automatically initialize if Quest controllers are detected
4. If initialization fails, check logcat for error messages:
   ```bash
   adb logcat | grep -E "OpenXR|QuestController"
   ```

## Troubleshooting

### Controller Not Detected
- Ensure you're running on a Meta Quest 3 device
- Check that controllers are paired and powered on
- Verify OpenXR runtime is available

### Initialization Failure
- Check logcat for OpenXR error codes
- Ensure the app has necessary permissions
- Verify the OpenXR loader dependency is correctly included

### Input Not Working
- Confirm the controller validation passes
- Check that action bindings are correctly suggested
- Verify the session is in the FOCUSED state

## References

- [Meta OpenXR SDK](https://github.com/meta-quest/Meta-OpenXR-SDK)
- [OpenXR Specification](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html)
- [Meta Quest Developer Documentation](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr)
- [OpenXR Interaction Profiles](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#semantic-path-interaction-profiles)
