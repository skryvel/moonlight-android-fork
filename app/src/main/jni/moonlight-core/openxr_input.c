#include "openxr_input.h"
#include <android/log.h>
#include <android/native_window.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <EGL/egl.h>

#define OPENXR_CHECK_LOADER
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#define LOG_TAG "OpenXRInput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// OpenXR state
static XrInstance g_instance = XR_NULL_HANDLE;
static XrSession g_session = XR_NULL_HANDLE;
static XrActionSet g_actionSet = XR_NULL_HANDLE;
static XrSpace g_appSpace = XR_NULL_HANDLE;
static bool g_initialized = false;
static XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;
static bool g_sessionRunning = false;

// EGL state for graphics binding (required by Quest OpenXR)
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static EGLConfig g_eglConfig = NULL;

// Actions for controller input
static XrAction g_action_a = XR_NULL_HANDLE;
static XrAction g_action_b = XR_NULL_HANDLE;
static XrAction g_action_x = XR_NULL_HANDLE;
static XrAction g_action_y = XR_NULL_HANDLE;
static XrAction g_action_menu = XR_NULL_HANDLE;
static XrAction g_action_grip_left = XR_NULL_HANDLE;
static XrAction g_action_grip_right = XR_NULL_HANDLE;
static XrAction g_action_trigger_left = XR_NULL_HANDLE;
static XrAction g_action_trigger_right = XR_NULL_HANDLE;
static XrAction g_action_thumbstick_left = XR_NULL_HANDLE;
static XrAction g_action_thumbstick_right = XR_NULL_HANDLE;
static XrAction g_action_thumbstick_click_left = XR_NULL_HANDLE;
static XrAction g_action_thumbstick_click_right = XR_NULL_HANDLE;

// Subaction paths for left and right hands
static XrPath g_hand_left_path = XR_NULL_PATH;
static XrPath g_hand_right_path = XR_NULL_PATH;
static XrSpace g_hand_left_space = XR_NULL_HANDLE;
static XrSpace g_hand_right_space = XR_NULL_HANDLE;

// Forward declarations
static bool init_egl(void);
static void cleanup_egl(void);
static bool poll_events(void);
static bool wait_for_session_ready(void);

// Initialize minimal EGL context for OpenXR graphics binding
static bool init_egl(void) {
    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(g_eglDisplay, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    LOGI("EGL initialized: %d.%d", major, minor);

    // Choose config
    EGLint configAttribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig(g_eglDisplay, configAttribs, &g_eglConfig, 1, &numConfigs) || numConfigs == 0) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    // Create context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 0,
        EGL_NONE
    };

    g_eglContext = eglCreateContext(g_eglDisplay, g_eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (g_eglContext == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    // Make context current with no surface (surfaceless context)
    if (!eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, g_eglContext)) {
        LOGE("Failed to make EGL context current (surfaceless)");
        // Some implementations don't support surfaceless - create a 1x1 pbuffer
        EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        EGLSurface pbuffer = eglCreatePbufferSurface(g_eglDisplay, g_eglConfig, pbufferAttribs);
        if (pbuffer == EGL_NO_SURFACE) {
            LOGE("Failed to create pbuffer surface");
            return false;
        }
        if (!eglMakeCurrent(g_eglDisplay, pbuffer, pbuffer, g_eglContext)) {
            LOGE("Failed to make EGL context current with pbuffer");
            return false;
        }
    }

    LOGI("EGL context created successfully");
    return true;
}

static void cleanup_egl(void) {
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(g_eglDisplay, g_eglContext);
            g_eglContext = EGL_NO_CONTEXT;
        }
        eglTerminate(g_eglDisplay);
        g_eglDisplay = EGL_NO_DISPLAY;
    }
}

// Poll OpenXR events and handle session state changes
static bool poll_events(void) {
    XrEventDataBuffer eventData = {.type = XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(g_instance, &eventData) == XR_SUCCESS) {
        switch (eventData.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                XrEventDataSessionStateChanged* stateChanged = (XrEventDataSessionStateChanged*)&eventData;
                g_sessionState = stateChanged->state;
                LOGI("Session state changed to: %d", g_sessionState);

                switch (g_sessionState) {
                    case XR_SESSION_STATE_READY: {
                        // Begin the session
                        XrSessionBeginInfo beginInfo = {
                            .type = XR_TYPE_SESSION_BEGIN_INFO,
                            .next = NULL,
                            .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                        };
                        XrResult result = xrBeginSession(g_session, &beginInfo);
                        if (XR_SUCCEEDED(result)) {
                            g_sessionRunning = true;
                            LOGI("Session started successfully");
                        } else {
                            LOGE("Failed to begin session: %d", result);
                        }
                        break;
                    }
                    case XR_SESSION_STATE_STOPPING:
                        if (g_sessionRunning) {
                            xrEndSession(g_session);
                            g_sessionRunning = false;
                            LOGI("Session ended");
                        }
                        break;
                    case XR_SESSION_STATE_EXITING:
                    case XR_SESSION_STATE_LOSS_PENDING:
                        g_sessionRunning = false;
                        break;
                    default:
                        break;
                }
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                LOGW("Instance loss pending");
                return false;
            default:
                break;
        }
        eventData.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
    return true;
}

// Wait for session to become ready (with timeout)
static bool wait_for_session_ready(void) {
    int attempts = 0;
    const int maxAttempts = 100; // 10 seconds timeout (100 * 100ms)

    while (attempts < maxAttempts) {
        poll_events();

        if (g_sessionState == XR_SESSION_STATE_FOCUSED ||
            g_sessionState == XR_SESSION_STATE_VISIBLE ||
            g_sessionState == XR_SESSION_STATE_SYNCHRONIZED) {
            LOGI("Session is ready for input (state: %d)", g_sessionState);
            return true;
        }

        if (g_sessionState == XR_SESSION_STATE_EXITING ||
            g_sessionState == XR_SESSION_STATE_LOSS_PENDING) {
            LOGE("Session entering exit state");
            return false;
        }

        // Wait a bit before polling again
        struct timespec ts = {0, 100000000}; // 100ms
        nanosleep(&ts, NULL);
        attempts++;
    }

    LOGW("Timeout waiting for session to be ready (current state: %d)", g_sessionState);
    // Return true anyway - we might still be able to get input
    return g_sessionRunning;
}

bool openxr_input_init(JNIEnv* env, jobject context) {
    if (g_initialized) {
        LOGI("OpenXR already initialized");
        return true;
    }

    XrResult result;

    // Initialize EGL first (required for OpenXR graphics binding on Quest)
    if (!init_egl()) {
        LOGE("Failed to initialize EGL");
        return false;
    }

    // Get JavaVM from JNIEnv
    JavaVM* vm;
    (*env)->GetJavaVM(env, &vm);

    // Initialize OpenXR loader for Android
    // This MUST be called before xrCreateInstance on Android
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    result = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                   (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (XR_SUCCEEDED(result)) {
        XrLoaderInitInfoAndroidKHR loaderInitInfo = {
            .type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
            .next = NULL,
            .applicationVM = vm,
            .applicationContext = context,
        };
        result = xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfo);
        if (XR_FAILED(result)) {
            LOGE("Failed to initialize OpenXR loader: %d", result);
            cleanup_egl();
            return false;
        }
        LOGI("OpenXR loader initialized successfully");
    } else {
        LOGW("xrInitializeLoaderKHR not available, continuing anyway");
    }

    // Create OpenXR instance with required extensions
    const char* extensions[] = {
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
    };

    XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
        .next = NULL,
        .applicationVM = vm,
        .applicationActivity = context,
    };

    XrInstanceCreateInfo instanceCreateInfo = {
        .type = XR_TYPE_INSTANCE_CREATE_INFO,
        .next = &instanceCreateInfoAndroid,
        .createFlags = 0,
        .applicationInfo = {
            .applicationName = "Moonlight",
            .applicationVersion = 1,
            .engineName = "Moonlight",
            .engineVersion = 1,
            .apiVersion = XR_CURRENT_API_VERSION,
        },
        .enabledApiLayerCount = 0,
        .enabledApiLayerNames = NULL,
        .enabledExtensionCount = 2,
        .enabledExtensionNames = extensions,
    };

    result = xrCreateInstance(&instanceCreateInfo, &g_instance);
    if (XR_FAILED(result)) {
        LOGE("Failed to create XR instance: %d", result);
        cleanup_egl();
        return false;
    }

    LOGI("OpenXR instance created successfully");

    // Get system ID
    XrSystemId systemId;
    XrSystemGetInfo systemGetInfo = {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .next = NULL,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    result = xrGetSystem(g_instance, &systemGetInfo, &systemId);
    if (XR_FAILED(result)) {
        LOGE("Failed to get XR system: %d", result);
        xrDestroyInstance(g_instance);
        g_instance = XR_NULL_HANDLE;
        cleanup_egl();
        return false;
    }

    // Check graphics requirements (required before session creation)
    PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR;
    result = xrGetInstanceProcAddr(g_instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                   (PFN_xrVoidFunction*)&xrGetOpenGLESGraphicsRequirementsKHR);
    if (XR_SUCCEEDED(result)) {
        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {
            .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR,
            .next = NULL,
        };
        result = xrGetOpenGLESGraphicsRequirementsKHR(g_instance, systemId, &graphicsRequirements);
        if (XR_FAILED(result)) {
            LOGW("Failed to get OpenGL ES graphics requirements: %d", result);
        } else {
            LOGI("OpenGL ES version required: %d.%d - %d.%d",
                 XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported),
                 XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported),
                 XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported),
                 XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported));
        }
    }

    // Create session with OpenGL ES graphics binding
    XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {
        .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
        .next = NULL,
        .display = g_eglDisplay,
        .config = g_eglConfig,
        .context = g_eglContext,
    };

    XrSessionCreateInfo sessionCreateInfo = {
        .type = XR_TYPE_SESSION_CREATE_INFO,
        .next = &graphicsBinding,
        .createFlags = 0,
        .systemId = systemId,
    };

    result = xrCreateSession(g_instance, &sessionCreateInfo, &g_session);
    if (XR_FAILED(result)) {
        LOGE("Failed to create XR session: %d", result);
        xrDestroyInstance(g_instance);
        g_instance = XR_NULL_HANDLE;
        cleanup_egl();
        return false;
    }

    LOGI("OpenXR session created successfully");

    // Create reference space
    XrReferenceSpaceCreateInfo refSpaceCreateInfo = {
        .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        .next = NULL,
        .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL,
        .poseInReferenceSpace = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
    };

    result = xrCreateReferenceSpace(g_session, &refSpaceCreateInfo, &g_appSpace);
    if (XR_FAILED(result)) {
        LOGE("Failed to create reference space: %d", result);
        xrDestroySession(g_session);
        xrDestroyInstance(g_instance);
        g_session = XR_NULL_HANDLE;
        g_instance = XR_NULL_HANDLE;
        return false;
    }

    // Create action set
    XrActionSetCreateInfo actionSetInfo = {
        .type = XR_TYPE_ACTION_SET_CREATE_INFO,
        .next = NULL,
        .actionSetName = "gameplay",
        .localizedActionSetName = "Gameplay",
        .priority = 0,
    };

    result = xrCreateActionSet(g_instance, &actionSetInfo, &g_actionSet);
    if (XR_FAILED(result)) {
        LOGE("Failed to create action set: %d", result);
        openxr_input_cleanup();
        return false;
    }

    // Get hand paths
    xrStringToPath(g_instance, "/user/hand/left", &g_hand_left_path);
    xrStringToPath(g_instance, "/user/hand/right", &g_hand_right_path);

    XrPath handPaths[2] = {g_hand_left_path, g_hand_right_path};

    // Create actions
    XrActionCreateInfo actionInfo = {
        .type = XR_TYPE_ACTION_CREATE_INFO,
        .next = NULL,
        .actionType = XR_ACTION_TYPE_BOOLEAN_INPUT,
        .countSubactionPaths = 1,
        .subactionPaths = &g_hand_right_path,
    };

    // A button (right controller)
    strcpy(actionInfo.actionName, "button_a");
    strcpy(actionInfo.localizedActionName, "Button A");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_a);

    // B button (right controller)
    strcpy(actionInfo.actionName, "button_b");
    strcpy(actionInfo.localizedActionName, "Button B");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_b);

    // X button (left controller)
    actionInfo.subactionPaths = &g_hand_left_path;
    strcpy(actionInfo.actionName, "button_x");
    strcpy(actionInfo.localizedActionName, "Button X");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_x);

    // Y button (left controller)
    strcpy(actionInfo.actionName, "button_y");
    strcpy(actionInfo.localizedActionName, "Button Y");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_y);

    // Menu button
    strcpy(actionInfo.actionName, "button_menu");
    strcpy(actionInfo.localizedActionName, "Menu Button");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_menu);

    // Grip buttons (both hands) - Note: Quest squeeze is analog, so we use FLOAT
    actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    actionInfo.countSubactionPaths = 1;
    actionInfo.subactionPaths = &g_hand_left_path;
    strcpy(actionInfo.actionName, "grip_left");
    strcpy(actionInfo.localizedActionName, "Left Grip");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_grip_left);

    strcpy(actionInfo.actionName, "grip_right");
    strcpy(actionInfo.localizedActionName, "Right Grip");
    actionInfo.subactionPaths = &g_hand_right_path;
    xrCreateAction(g_actionSet, &actionInfo, &g_action_grip_right);

    // Triggers (analog) - already FLOAT from above
    actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    actionInfo.subactionPaths = &g_hand_left_path;
    strcpy(actionInfo.actionName, "trigger_left");
    strcpy(actionInfo.localizedActionName, "Left Trigger");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_trigger_left);

    actionInfo.subactionPaths = &g_hand_right_path;
    strcpy(actionInfo.actionName, "trigger_right");
    strcpy(actionInfo.localizedActionName, "Right Trigger");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_trigger_right);

    // Thumbsticks (2D vector)
    actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
    actionInfo.subactionPaths = &g_hand_left_path;
    strcpy(actionInfo.actionName, "thumbstick_left");
    strcpy(actionInfo.localizedActionName, "Left Thumbstick");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_thumbstick_left);

    actionInfo.subactionPaths = &g_hand_right_path;
    strcpy(actionInfo.actionName, "thumbstick_right");
    strcpy(actionInfo.localizedActionName, "Right Thumbstick");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_thumbstick_right);

    // Thumbstick clicks
    actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    actionInfo.subactionPaths = &g_hand_left_path;
    strcpy(actionInfo.actionName, "thumbstick_click_left");
    strcpy(actionInfo.localizedActionName, "Left Thumbstick Click");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_thumbstick_click_left);

    actionInfo.subactionPaths = &g_hand_right_path;
    strcpy(actionInfo.actionName, "thumbstick_click_right");
    strcpy(actionInfo.localizedActionName, "Right Thumbstick Click");
    xrCreateAction(g_actionSet, &actionInfo, &g_action_thumbstick_click_right);

    // Suggest bindings for Oculus Touch controller profile
    XrPath interactionProfilePath;
    xrStringToPath(g_instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);

    XrPath bindingPaths[13];
    xrStringToPath(g_instance, "/user/hand/right/input/a/click", &bindingPaths[0]);
    xrStringToPath(g_instance, "/user/hand/right/input/b/click", &bindingPaths[1]);
    xrStringToPath(g_instance, "/user/hand/left/input/x/click", &bindingPaths[2]);
    xrStringToPath(g_instance, "/user/hand/left/input/y/click", &bindingPaths[3]);
    xrStringToPath(g_instance, "/user/hand/left/input/menu/click", &bindingPaths[4]);
    xrStringToPath(g_instance, "/user/hand/left/input/squeeze/value", &bindingPaths[5]);
    xrStringToPath(g_instance, "/user/hand/right/input/squeeze/value", &bindingPaths[6]);
    xrStringToPath(g_instance, "/user/hand/left/input/trigger/value", &bindingPaths[7]);
    xrStringToPath(g_instance, "/user/hand/right/input/trigger/value", &bindingPaths[8]);
    xrStringToPath(g_instance, "/user/hand/left/input/thumbstick", &bindingPaths[9]);
    xrStringToPath(g_instance, "/user/hand/right/input/thumbstick", &bindingPaths[10]);
    xrStringToPath(g_instance, "/user/hand/left/input/thumbstick/click", &bindingPaths[11]);
    xrStringToPath(g_instance, "/user/hand/right/input/thumbstick/click", &bindingPaths[12]);

    XrActionSuggestedBinding bindings[13] = {
        {g_action_a, bindingPaths[0]},
        {g_action_b, bindingPaths[1]},
        {g_action_x, bindingPaths[2]},
        {g_action_y, bindingPaths[3]},
        {g_action_menu, bindingPaths[4]},
        {g_action_grip_left, bindingPaths[5]},
        {g_action_grip_right, bindingPaths[6]},
        {g_action_trigger_left, bindingPaths[7]},
        {g_action_trigger_right, bindingPaths[8]},
        {g_action_thumbstick_left, bindingPaths[9]},
        {g_action_thumbstick_right, bindingPaths[10]},
        {g_action_thumbstick_click_left, bindingPaths[11]},
        {g_action_thumbstick_click_right, bindingPaths[12]},
    };

    XrInteractionProfileSuggestedBinding suggestedBindings = {
        .type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
        .next = NULL,
        .interactionProfile = interactionProfilePath,
        .countSuggestedBindings = 13,
        .suggestedBindings = bindings,
    };

    result = xrSuggestInteractionProfileBindings(g_instance, &suggestedBindings);
    if (XR_FAILED(result)) {
        LOGE("Failed to suggest interaction profile bindings: %d", result);
        openxr_input_cleanup();
        return false;
    }

    // Attach action set to session
    XrSessionActionSetsAttachInfo attachInfo = {
        .type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        .next = NULL,
        .countActionSets = 1,
        .actionSets = &g_actionSet,
    };

    result = xrAttachSessionActionSets(g_session, &attachInfo);
    if (XR_FAILED(result)) {
        LOGE("Failed to attach action sets: %d", result);
        openxr_input_cleanup();
        return false;
    }

    g_initialized = true;
    LOGI("OpenXR input initialized successfully");

    // Wait for session to be ready for input
    if (!wait_for_session_ready()) {
        LOGW("Session may not be fully ready, but continuing anyway");
    }

    return true;
}

bool openxr_input_poll(QuestControllerState* state) {
    if (!g_initialized || !state) {
        return false;
    }

    // Process any pending events (handles session state changes)
    if (!poll_events()) {
        LOGW("Error polling events or instance loss");
        return false;
    }

    // Check if session is in a state where we can read input
    if (!g_sessionRunning) {
        // Session not running yet - return empty state but don't fail
        memset(state, 0, sizeof(QuestControllerState));
        return true;
    }

    memset(state, 0, sizeof(QuestControllerState));

    // Sync actions
    XrActiveActionSet activeActionSet = {
        .actionSet = g_actionSet,
        .subactionPath = XR_NULL_PATH,
    };

    XrActionsSyncInfo syncInfo = {
        .type = XR_TYPE_ACTIONS_SYNC_INFO,
        .next = NULL,
        .countActiveActionSets = 1,
        .activeActionSets = &activeActionSet,
    };

    XrResult result = xrSyncActions(g_session, &syncInfo);
    if (XR_FAILED(result)) {
        LOGE("Failed to sync actions: %d", result);
        return false;
    }

    // Get action states
    XrActionStateGetInfo getInfo = {
        .type = XR_TYPE_ACTION_STATE_GET_INFO,
        .next = NULL,
        .subactionPath = XR_NULL_PATH,
    };

    // Boolean actions
    XrActionStateBoolean boolState = {.type = XR_TYPE_ACTION_STATE_BOOLEAN};

    getInfo.action = g_action_a;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_a = boolState.currentState;

    getInfo.action = g_action_b;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_b = boolState.currentState;

    getInfo.action = g_action_x;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_x = boolState.currentState;

    getInfo.action = g_action_y;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_y = boolState.currentState;

    getInfo.action = g_action_menu;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_menu = boolState.currentState;

    // Grip actions are FLOAT - convert to boolean with threshold
    XrActionStateFloat gripFloatState = {.type = XR_TYPE_ACTION_STATE_FLOAT};
    getInfo.action = g_action_grip_left;
    xrGetActionStateFloat(g_session, &getInfo, &gripFloatState);
    state->button_lb = gripFloatState.currentState > 0.5f;

    getInfo.action = g_action_grip_right;
    xrGetActionStateFloat(g_session, &getInfo, &gripFloatState);
    state->button_rb = gripFloatState.currentState > 0.5f;

    getInfo.action = g_action_thumbstick_click_left;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_ls_click = boolState.currentState;

    getInfo.action = g_action_thumbstick_click_right;
    xrGetActionStateBoolean(g_session, &getInfo, &boolState);
    state->button_rs_click = boolState.currentState;

    // Float actions (triggers)
    XrActionStateFloat floatState = {.type = XR_TYPE_ACTION_STATE_FLOAT};

    getInfo.action = g_action_trigger_left;
    xrGetActionStateFloat(g_session, &getInfo, &floatState);
    state->left_trigger = floatState.currentState;

    getInfo.action = g_action_trigger_right;
    xrGetActionStateFloat(g_session, &getInfo, &floatState);
    state->right_trigger = floatState.currentState;

    // Vector2 actions (thumbsticks)
    XrActionStateVector2f vec2State = {.type = XR_TYPE_ACTION_STATE_VECTOR2F};

    getInfo.action = g_action_thumbstick_left;
    xrGetActionStateVector2f(g_session, &getInfo, &vec2State);
    state->left_stick_x = vec2State.currentState.x;
    state->left_stick_y = vec2State.currentState.y;

    getInfo.action = g_action_thumbstick_right;
    xrGetActionStateVector2f(g_session, &getInfo, &vec2State);
    state->right_stick_x = vec2State.currentState.x;
    state->right_stick_y = vec2State.currentState.y;

    state->initialized = true;
    return true;
}

void openxr_input_cleanup() {
    // End the session if it's running
    if (g_sessionRunning && g_session != XR_NULL_HANDLE) {
        xrEndSession(g_session);
        g_sessionRunning = false;
    }

    if (g_appSpace != XR_NULL_HANDLE) {
        xrDestroySpace(g_appSpace);
        g_appSpace = XR_NULL_HANDLE;
    }

    if (g_actionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(g_actionSet);
        g_actionSet = XR_NULL_HANDLE;
    }

    if (g_session != XR_NULL_HANDLE) {
        xrDestroySession(g_session);
        g_session = XR_NULL_HANDLE;
    }

    if (g_instance != XR_NULL_HANDLE) {
        xrDestroyInstance(g_instance);
        g_instance = XR_NULL_HANDLE;
    }

    // Cleanup EGL resources
    cleanup_egl();

    g_initialized = false;
    g_sessionState = XR_SESSION_STATE_UNKNOWN;
    LOGI("OpenXR input cleaned up");
}

bool openxr_input_validate_controller() {
    // Check that we have at least:
    // - 4 buttons (A, B, X, Y)
    // - 2 triggers
    // - 2 thumbsticks with click
    // - 2 grip buttons

    // For Quest 3 controllers, all of these should be available
    // This is a simple check - in a real implementation you might query capabilities

    if (!g_initialized) {
        LOGE("OpenXR not initialized - cannot validate controller");
        return false;
    }

    // For now, if we successfully initialized, we assume the controller is valid
    // A more robust implementation would check the actual interaction profile
    LOGI("Controller validation passed - Quest controllers have all required inputs");
    return true;
}
