package com.limelight.binding;

import android.content.Context;
import android.os.Build;

import com.limelight.LimeLog;
import com.limelight.preferences.PreferenceConfiguration;

/**
 * Quest-specific optimizations for VR streaming.
 *
 * This class provides device-specific optimizations for Meta Quest devices
 * to improve the VR streaming experience. All optimizations are optional
 * and use standard Android APIs.
 *
 * Design principles:
 * - No Meta SDK dependencies
 * - Runtime detection using Build properties
 * - Graceful degradation on non-Quest devices
 * - Upstream-compatible implementation
 */
public class QuestOptimizations {

    private static final String TAG = "QuestOptimizations";

    /**
     * Recommended streaming settings for Quest devices.
     * These are suggestions based on the Quest model's capabilities.
     */
    public static class StreamingPreset {
        public final int width;
        public final int height;
        public final int fps;
        public final int bitrate;
        public final String codec;
        public final String description;

        public StreamingPreset(int width, int height, int fps, int bitrate, String codec, String description) {
            this.width = width;
            this.height = height;
            this.fps = fps;
            this.bitrate = bitrate;
            this.codec = codec;
            this.description = description;
        }
    }

    /**
     * Get the Quest device generation (1, 2, 3, or Pro).
     * Returns 0 if not a Quest device.
     */
    public static int getQuestGeneration() {
        if (!PreferenceConfiguration.isMetaQuestDevice()) {
            return 0;
        }

        String model = Build.MODEL.toLowerCase();

        // Quest 3
        if (model.contains("quest 3") || model.contains("quest3")) {
            return 3;
        }

        // Quest Pro
        if (model.contains("quest pro") || model.contains("questpro") || model.contains("cambria")) {
            return 100; // Special value for Pro
        }

        // Quest 2
        if (model.contains("quest 2") || model.contains("quest2")) {
            return 2;
        }

        // Quest 1 (original)
        if (model.contains("quest") && !model.contains("2") && !model.contains("3")) {
            return 1;
        }

        return 0; // Unknown Quest device
    }

    /**
     * Get recommended streaming preset for the current Quest device.
     * Returns null if not a Quest device or if unable to determine model.
     */
    public static StreamingPreset getRecommendedPreset() {
        int generation = getQuestGeneration();

        switch (generation) {
            case 3:
                // Quest 3: Best quality with AV1 support
                // Native resolution: 2064x2208 per eye, 120Hz capable
                return new StreamingPreset(
                    2560, 1440, 90,
                    80000, // 80 Mbps for high quality
                    "AV1", // Quest 3 has hardware AV1 decode
                    "Quest 3 Optimized (90Hz, AV1)"
                );

            case 100: // Quest Pro
                // Quest Pro: High quality with 90Hz
                // Native resolution: 1800x1920 per eye, 90Hz capable
                return new StreamingPreset(
                    2560, 1440, 90,
                    75000, // 75 Mbps
                    "HEVC",
                    "Quest Pro Optimized (90Hz, HEVC)"
                );

            case 2:
                // Quest 2: Balanced quality
                // Native resolution: 1832x1920 per eye, 120Hz capable
                return new StreamingPreset(
                    1920, 1080, 90,
                    50000, // 50 Mbps
                    "HEVC",
                    "Quest 2 Optimized (90Hz, HEVC)"
                );

            case 1:
                // Quest 1: Conservative settings
                // Native resolution: 1440x1600 per eye, 72Hz
                return new StreamingPreset(
                    1920, 1080, 60,
                    30000, // 30 Mbps
                    "H264",
                    "Quest 1 Optimized (60Hz, H264)"
                );

            default:
                return null;
        }
    }

    /**
     * Check if the current Quest device supports WiFi 6/6E.
     * Quest 3 has WiFi 6E, Quest 2 has WiFi 6, Quest 1 has WiFi 5.
     */
    public static boolean supportsWiFi6() {
        int generation = getQuestGeneration();
        return generation >= 2;
    }

    /**
     * Check if the current Quest device supports AV1 hardware decode.
     * Only Quest 3 has hardware AV1 support.
     */
    public static boolean supportsAV1() {
        return getQuestGeneration() == 3;
    }

    /**
     * Check if the current Quest device supports 90Hz+ refresh rates.
     * Quest 1 is limited to 72Hz, all others support 90Hz+.
     */
    public static boolean supportsHighRefreshRate() {
        int generation = getQuestGeneration();
        return generation >= 2 || generation == 100;
    }

    /**
     * Check if the current Quest device supports 120Hz refresh rate.
     * Quest 2 and Quest 3 support 120Hz experimental mode.
     */
    public static boolean supports120Hz() {
        int generation = getQuestGeneration();
        return generation == 2 || generation == 3;
    }

    /**
     * Get the maximum recommended bitrate for the current Quest device
     * based on WiFi capabilities.
     */
    public static int getMaxRecommendedBitrate() {
        int generation = getQuestGeneration();

        switch (generation) {
            case 3:
                // WiFi 6E: Can handle very high bitrates
                return 150000; // 150 Mbps
            case 100: // Quest Pro
            case 2:
                // WiFi 6: Good bandwidth
                return 100000; // 100 Mbps
            case 1:
                // WiFi 5: More conservative
                return 50000; // 50 Mbps
            default:
                return 0;
        }
    }

    /**
     * Log Quest-specific information for debugging.
     */
    public static void logQuestInfo(Context context) {
        if (!PreferenceConfiguration.isMetaQuestDevice()) {
            return;
        }

        LimeLog.info("=== Quest Device Information ===");
        LimeLog.info("Manufacturer: " + Build.MANUFACTURER);
        LimeLog.info("Model: " + Build.MODEL);
        LimeLog.info("Device: " + Build.DEVICE);

        int generation = getQuestGeneration();
        String genStr;
        switch (generation) {
            case 3: genStr = "Quest 3"; break;
            case 100: genStr = "Quest Pro"; break;
            case 2: genStr = "Quest 2"; break;
            case 1: genStr = "Quest 1"; break;
            default: genStr = "Unknown Quest"; break;
        }
        LimeLog.info("Detected Generation: " + genStr);

        LimeLog.info("WiFi 6/6E Support: " + supportsWiFi6());
        LimeLog.info("AV1 Hardware Decode: " + supportsAV1());
        LimeLog.info("90Hz+ Support: " + supportsHighRefreshRate());
        LimeLog.info("120Hz Support: " + supports120Hz());
        LimeLog.info("Max Recommended Bitrate: " + getMaxRecommendedBitrate() + " Kbps");

        StreamingPreset preset = getRecommendedPreset();
        if (preset != null) {
            LimeLog.info("Recommended Preset: " + preset.description);
            LimeLog.info("  Resolution: " + preset.width + "x" + preset.height);
            LimeLog.info("  FPS: " + preset.fps);
            LimeLog.info("  Bitrate: " + preset.bitrate + " Kbps");
            LimeLog.info("  Codec: " + preset.codec);
        }

        LimeLog.info("=== End Quest Information ===");
    }

    /**
     * Apply Quest-specific optimizations to a PreferenceConfiguration.
     * This modifies the config to use Quest-optimized settings if appropriate.
     *
     * @param config The configuration to optimize
     * @param applyPreset Whether to apply the recommended streaming preset
     * @return true if optimizations were applied, false otherwise
     */
    public static boolean applyOptimizations(PreferenceConfiguration config, boolean applyPreset) {
        if (!PreferenceConfiguration.isMetaQuestDevice()) {
            return false;
        }

        LimeLog.info("Applying Quest-specific optimizations");

        if (applyPreset) {
            StreamingPreset preset = getRecommendedPreset();
            if (preset != null) {
                LimeLog.info("Applying recommended preset: " + preset.description);
                config.width = preset.width;
                config.height = preset.height;
                config.fps = preset.fps;
                config.bitrate = preset.bitrate;
            }
        }

        // Enable gamepad motion sensors for Quest controllers
        config.gamepadMotionSensors = true;

        // Enable multi-controller support (useful for VR with two controllers)
        config.multiController = true;

        return true;
    }

    /**
     * Check if low-latency mode should be enabled for VR.
     * Similar to NVIDIA Shield's immediate input mode.
     *
     * @return true if low-latency optimizations should be enabled
     */
    public static boolean shouldEnableLowLatencyMode() {
        // Enable for all Quest devices to reduce input lag in VR
        return PreferenceConfiguration.isMetaQuestDevice();
    }
}
