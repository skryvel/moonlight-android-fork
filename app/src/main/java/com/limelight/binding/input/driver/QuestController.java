package com.limelight.binding.input.driver;

import android.content.Context;

import com.limelight.LimeLog;
import com.limelight.nvstream.input.ControllerPacket;
import com.limelight.nvstream.jni.MoonBridge;

/**
 * Controller driver for Meta Quest 3 controllers using OpenXR.
 * Maps Quest Touch controllers to Xbox-style gamepad layout.
 */
public class QuestController extends AbstractController {
    private static final String TAG = "QuestController";
    private static boolean initialized = false;
    private static QuestController instance = null;

    private Thread inputThread;
    private boolean stopped;
    private Context context;

    // Native method declarations
    private static native boolean nativeInit(Context context);
    private static native boolean nativePoll(float[] floatData, boolean[] boolData);
    private static native void nativeCleanup();
    private static native boolean nativeValidate();

    static {
        System.loadLibrary("moonlight-core");
    }

    /**
     * Initialize OpenXR and check if Quest controllers are available.
     * @param context Android application context
     * @return true if controllers were successfully initialized
     * @throws RuntimeException if required gamepad buttons are not found
     */
    public static synchronized QuestController create(Context context, int deviceId, UsbDriverListener listener) {
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

                        // Report input to Moonlight
                        reportInput();
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
}
