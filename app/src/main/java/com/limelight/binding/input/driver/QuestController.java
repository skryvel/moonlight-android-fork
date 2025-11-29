package com.limelight.binding.input.driver;

import android.content.Context;

import com.limelight.BuildConfig;
import com.limelight.LimeLog;
import com.limelight.nvstream.input.ControllerPacket;
import com.limelight.nvstream.jni.MoonBridge;
import com.limelight.preferences.PreferenceConfiguration;

/**
 * Controller driver for Meta Quest 3 controllers using OpenXR.
 * Maps Quest Touch controllers to Xbox-style gamepad layout or mouse/keyboard input.
 *
 * Only available when compiled with HAS_OPENXR build flag (Quest flavor).
 */
public class QuestController extends AbstractController {
    private static final String TAG = "QuestController";
    private static boolean initialized = false;
    private static QuestController instance = null;

    private Thread inputThread;
    private boolean stopped;
    private Context context;
    private boolean gamepadMode;  // true = gamepad mode, false = mouse mode

    // Mouse mode state
    private boolean mouseLeftButtonDown = false;
    private boolean mouseRightButtonDown = false;

    // Native method declarations
    private static native boolean nativeInit(Context context);
    private static native boolean nativePoll(float[] floatData, boolean[] boolData);
    private static native void nativeCleanup();
    private static native boolean nativeValidate();

    static {
        // Only load OpenXR library if built with Quest flavor
        if (BuildConfig.HAS_OPENXR) {
            try {
                System.loadLibrary("moonlight-openxr");
            } catch (UnsatisfiedLinkError e) {
                LimeLog.severe("Failed to load OpenXR library: " + e.getMessage());
            }
        }
    }

    /**
     * Initialize OpenXR and check if Quest controllers are available.
     * @param context Android application context
     * @return true if controllers were successfully initialized
     * @throws RuntimeException if required gamepad buttons are not found or if OpenXR is not available
     */
    public static synchronized QuestController create(Context context, int deviceId, UsbDriverListener listener) {
        // Check if OpenXR support is available at compile time
        if (!BuildConfig.HAS_OPENXR) {
            throw new RuntimeException(
                "Quest controller support not available in this build. " +
                "Please use a Quest-enabled build variant (e.g., nonRootQuestDebug)."
            );
        }

        if (instance != null) {
            LimeLog.info("Quest controller already exists");
            return instance;
        }

        LimeLog.info("Creating Quest controller instance");

        // Initialize OpenXR
        if (!initialized) {
            if (!nativeInit(context)) {
                throw new RuntimeException("Failed to initialize OpenXR for Quest controllers");
            }
            initialized = true;
            LimeLog.info("OpenXR initialized successfully");
        }

        // Validate controller has required buttons
        if (!nativeValidate()) {
            throw new RuntimeException(
                "Quest controller validation failed: Expected at least 4 buttons (A, B, X, Y), " +
                "2 analog triggers, 2 analog sticks with click, and 2 grip buttons. " +
                "Please ensure Quest 3 controllers are properly connected."
            );
        }

        instance = new QuestController(context, deviceId, listener);
        return instance;
    }

    private QuestController(Context context, int deviceId, UsbDriverListener listener) {
        // Use vendor ID 0x2833 (Meta/Oculus) and product ID 0x0186 (Quest 3)
        super(deviceId, listener, 0x2833, 0x0186);

        this.context = context;
        this.type = MoonBridge.LI_CTYPE_XBOX;
        this.capabilities = MoonBridge.LI_CCAP_ANALOG_TRIGGERS;

        // Set supported buttons - Quest controllers have all Xbox-equivalent buttons
        this.supportedButtonFlags =
                ControllerPacket.A_FLAG | ControllerPacket.B_FLAG |
                ControllerPacket.X_FLAG | ControllerPacket.Y_FLAG |
                ControllerPacket.LB_FLAG | ControllerPacket.RB_FLAG |
                ControllerPacket.LS_CLK_FLAG | ControllerPacket.RS_CLK_FLAG |
                ControllerPacket.PLAY_FLAG; // Menu button maps to Play/Start

        LimeLog.info("Quest controller created with device ID: " + deviceId);
    }

    @Override
    public boolean start() {
        if (inputThread != null) {
            LimeLog.warning("Quest controller already started");
            return false;
        }

        stopped = false;

        // Read preference for gamepad vs mouse mode
        PreferenceConfiguration config = PreferenceConfiguration.readPreferences(context);
        gamepadMode = config.questControllerGamepadMode;
        LimeLog.info("Quest controller starting in " + (gamepadMode ? "gamepad" : "mouse") + " mode");

        // Create input polling thread
        inputThread = new Thread(() -> {
            LimeLog.info("Quest controller input thread started");

            // Delay before reporting device to allow time for initialization
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                return;
            }

            // Report device added
            notifyDeviceAdded();

            // Arrays to receive native poll data
            float[] floatData = new float[6]; // [leftTrigger, rightTrigger, leftStickX, leftStickY, rightStickX, rightStickY]
            boolean[] boolData = new boolean[9]; // [A, B, X, Y, Menu, LB, RB, LS_Click, RS_Click]

            while (!Thread.currentThread().isInterrupted() && !stopped) {
                try {
                    // Poll controller state from OpenXR
                    if (nativePoll(floatData, boolData)) {
                        if (gamepadMode) {
                            // Gamepad mode - map to Xbox controller
                            handleGamepadMode(floatData, boolData);
                        } else {
                            // Mouse mode - use controllers as mouse input
                            handleMouseMode(floatData, boolData);
                        }
                    }

                    // Poll at ~60Hz
                    Thread.sleep(16);
                } catch (InterruptedException e) {
                    LimeLog.info("Quest controller input thread interrupted");
                    break;
                } catch (Exception e) {
                    LimeLog.severe("Error polling Quest controller: " + e.getMessage());
                    e.printStackTrace();
                    break;
                }
            }

            LimeLog.info("Quest controller input thread stopped");
        });

        inputThread.setName("Quest-Controller-Input");
        inputThread.start();

        return true;
    }

    @Override
    public void stop() {
        if (stopped) {
            return;
        }

        LimeLog.info("Stopping Quest controller");
        stopped = true;

        if (inputThread != null) {
            inputThread.interrupt();
            try {
                inputThread.join(1000);
            } catch (InterruptedException e) {
                // Ignore
            }
            inputThread = null;
        }

        // Report device removed
        notifyDeviceRemoved();

        // Clear instance
        if (instance == this) {
            instance = null;
        }
    }

    @Override
    public void rumble(short lowFreqMotor, short highFreqMotor) {
        // TODO: Implement haptics via OpenXR XR_FB_haptic_amplitude_envelope extension
        // For now, rumble is not supported
        LimeLog.info("Rumble requested but not yet implemented for Quest controllers");
    }

    @Override
    public void rumbleTriggers(short leftTrigger, short rightTrigger) {
        // Quest controllers don't have trigger motors
    }

    /**
     * Cleanup OpenXR resources when no longer needed.
     * Should be called when the app is shutting down.
     */
    public static synchronized void cleanup() {
        if (initialized) {
            LimeLog.info("Cleaning up Quest controller OpenXR resources");
            nativeCleanup();
            initialized = false;
            instance = null;
        }
    }

    private void setButtonFlag(int flag, boolean pressed) {
        if (pressed) {
            buttonFlags |= flag;
        } else {
            buttonFlags &= ~flag;
        }
    }

    /**
     * Handle input in gamepad mode - maps Quest controllers to Xbox-style gamepad
     */
    private void handleGamepadMode(float[] floatData, boolean[] boolData) {
        // Update button flags
        setButtonFlag(ControllerPacket.A_FLAG, boolData[0]);
        setButtonFlag(ControllerPacket.B_FLAG, boolData[1]);
        setButtonFlag(ControllerPacket.X_FLAG, boolData[2]);
        setButtonFlag(ControllerPacket.Y_FLAG, boolData[3]);
        setButtonFlag(ControllerPacket.PLAY_FLAG, boolData[4]); // Menu -> Play/Start
        setButtonFlag(ControllerPacket.LB_FLAG, boolData[5]);
        setButtonFlag(ControllerPacket.RB_FLAG, boolData[6]);
        setButtonFlag(ControllerPacket.LS_CLK_FLAG, boolData[7]);
        setButtonFlag(ControllerPacket.RS_CLK_FLAG, boolData[8]);

        // Update analog values
        leftTrigger = floatData[0];
        rightTrigger = floatData[1];
        leftStickX = floatData[2];
        leftStickY = floatData[3];
        rightStickX = floatData[4];
        rightStickY = floatData[5];

        // Report input to Moonlight as gamepad
        reportInput();
    }

    /**
     * Handle input in mouse mode - maps Quest controllers to mouse/keyboard input
     *
     * Mouse mode mapping:
     * - Right thumbstick -> Mouse movement
     * - Right trigger -> Left mouse button
     * - Right grip (RB) -> Right mouse button
     * - A button -> Middle mouse button
     * - Left thumbstick Y-axis -> Mouse scroll wheel
     */
    private void handleMouseMode(float[] floatData, boolean[] boolData) {
        // Extract controller state
        // floatData: [leftTrigger, rightTrigger, leftStickX, leftStickY, rightStickX, rightStickY]
        // boolData: [A, B, X, Y, Menu, LB, RB, LS_Click, RS_Click]

        float rightStickX = floatData[4];
        float rightStickY = floatData[5];
        float leftStickY = floatData[3];
        float rightTrigger = floatData[1];
        boolean aButton = boolData[0];
        boolean rightGrip = boolData[6]; // RB

        // Mouse movement from right thumbstick
        // Apply deadzone and scaling
        final float DEADZONE = 0.15f;
        final float MOUSE_SENSITIVITY = 15.0f; // Pixels per frame at full stick deflection

        if (Math.abs(rightStickX) > DEADZONE || Math.abs(rightStickY) > DEADZONE) {
            // Apply deadzone
            float deltaX = Math.abs(rightStickX) > DEADZONE ? rightStickX : 0;
            float deltaY = Math.abs(rightStickY) > DEADZONE ? rightStickY : 0;

            // Scale movement
            short mouseDeltaX = (short)(deltaX * MOUSE_SENSITIVITY);
            short mouseDeltaY = (short)(-deltaY * MOUSE_SENSITIVITY); // Invert Y for natural movement

            // Send mouse move
            MoonBridge.sendMouseMove(mouseDeltaX, mouseDeltaY);
        }

        // Left mouse button (right trigger)
        boolean leftButtonPressed = rightTrigger > 0.5f;
        if (leftButtonPressed != mouseLeftButtonDown) {
            mouseLeftButtonDown = leftButtonPressed;
            MoonBridge.sendMouseButton(
                leftButtonPressed ? (byte)0x07 : (byte)0x08,  // Press/Release
                (byte)0x01  // Left button
            );
        }

        // Right mouse button (right grip/RB)
        if (rightGrip != mouseRightButtonDown) {
            mouseRightButtonDown = rightGrip;
            MoonBridge.sendMouseButton(
                rightGrip ? (byte)0x07 : (byte)0x08,  // Press/Release
                (byte)0x03  // Right button
            );
        }

        // Middle mouse button (A button)
        // Note: Using static to track state across calls
        if (aButton) {
            MoonBridge.sendMouseButton((byte)0x07, (byte)0x02); // Press middle
            MoonBridge.sendMouseButton((byte)0x08, (byte)0x02); // Release middle (immediate)
        }

        // Mouse scroll wheel (left thumbstick Y-axis)
        if (Math.abs(leftStickY) > DEADZONE) {
            short scrollAmount = (short)(leftStickY * 120); // Standard scroll amount
            MoonBridge.sendMouseHighResScroll(scrollAmount);
        }
    }
}
