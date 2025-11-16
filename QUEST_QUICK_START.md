# Quest 3 Support - Quick Start Guide

This is a quick reference for Quest 3 support in Moonlight Android. For complete documentation, see [QUEST_SUPPORT.md](QUEST_SUPPORT.md).

## Features

### ✅ What Works Now

**Controller Support**:
- Quest Touch controllers (all generations) detected and working
- **Emulated as Xbox controllers** - games see them as Xbox One controllers
- Full button mapping (A, B, X, Y, triggers, bumpers, D-pad, sticks)
- Motion sensors (gyroscope + accelerometer) enabled
- Controller vibration via standard Android API
- Multi-controller support (up to 16 controllers)

**Device Detection**:
- Automatic Quest device detection (Quest 1/2/3/Pro)
- WiFi capability detection (WiFi 5/6/6E)
- Codec support detection (H264/HEVC/AV1)
- Refresh rate capability detection (60/90/120Hz)

**Optimizations**:
- Recommended streaming presets per Quest model
- Quest-specific logging for debugging
- Xbox controller emulation logging

### Supported Controllers

| Controller | Vendor ID | Product ID | Emulated As |
|-----------|-----------|------------|-------------|
| Quest/Quest 2 Touch (L) | 0x2833 | 0x0186 | Xbox One |
| Quest/Quest 2 Touch (R) | 0x2833 | 0x0187 | Xbox One |
| Quest Pro Touch (L) | 0x2833 | 0x0183 | Xbox One |
| Quest Pro Touch (R) | 0x2833 | 0x0184 | Xbox One |
| Quest 3 Touch Plus (L) | 0x2833 | 0x0510 | Xbox One |
| Quest 3 Touch Plus (R) | 0x2833 | 0x0511 | Xbox One |

## Recommended Settings

### Quest 3
- **Resolution**: 2560x1440
- **FPS**: 90Hz (120Hz experimental)
- **Bitrate**: 80,000 Kbps (80 Mbps)
- **Codec**: AV1 (hardware accelerated)
- **Max Bitrate**: Up to 150 Mbps (WiFi 6E)

### Quest Pro
- **Resolution**: 2560x1440
- **FPS**: 90Hz
- **Bitrate**: 75,000 Kbps (75 Mbps)
- **Codec**: HEVC (H.265)
- **Max Bitrate**: Up to 100 Mbps (WiFi 6)

### Quest 2
- **Resolution**: 1920x1080
- **FPS**: 90Hz (120Hz experimental)
- **Bitrate**: 50,000 Kbps (50 Mbps)
- **Codec**: HEVC (H.265)
- **Max Bitrate**: Up to 100 Mbps (WiFi 6)

### Quest 1
- **Resolution**: 1920x1080
- **FPS**: 60Hz
- **Bitrate**: 30,000 Kbps (30 Mbps)
- **Codec**: H.264
- **Max Bitrate**: Up to 50 Mbps (WiFi 5)

## How It Works

### Xbox Controller Emulation

Quest controllers are **automatically detected and emulated as Xbox One controllers**. This happens through:

1. **Hardware Detection**: Vendor ID 0x2833 (Meta) is recognized
2. **Controller Type Mapping**: Quest controllers map to `k_eControllerType_XBoxOneController`
3. **Protocol Translation**: Controller sends Xbox button codes to PC
4. **Game Compatibility**: PC games see standard Xbox One controller

**What this means**:
- ✅ All Xbox-compatible games work out of the box
- ✅ No special configuration needed
- ✅ Xbox button prompts show correctly in games
- ✅ Controller works exactly like Xbox One controller

### Logging

When a Quest controller connects, you'll see:
```
Creating controller context for device: Meta Quest 3 Touch Plus Controller (Left)
Vendor ID: 10291 (0x2833)
Product ID: 1296 (0x0510)
Controller type: Xbox (0x1)
Meta Quest controller detected - emulating as Xbox controller
```

### Quest Device Detection

When streaming starts on a Quest device, you'll see:
```
=== Quest Device Information ===
Manufacturer: Meta
Model: Quest 3
Detected Generation: Quest 3
WiFi 6/6E Support: true
AV1 Hardware Decode: true
90Hz+ Support: true
120Hz Support: true
Max Recommended Bitrate: 150000 Kbps
Recommended Preset: Quest 3 Optimized (90Hz, AV1)
  Resolution: 2560x1440
  FPS: 90
  Bitrate: 80000 Kbps
  Codec: AV1
=== End Quest Information ===
```

## Using Quest Optimizations in Code

### Check if Running on Quest

```java
if (PreferenceConfiguration.isMetaQuestDevice()) {
    // Quest-specific code
}
```

### Get Recommended Settings

```java
QuestOptimizations.StreamingPreset preset = QuestOptimizations.getRecommendedPreset();
if (preset != null) {
    config.width = preset.width;
    config.height = preset.height;
    config.fps = preset.fps;
    config.bitrate = preset.bitrate;
}
```

### Check Device Capabilities

```java
// Check WiFi generation
boolean hasWiFi6 = QuestOptimizations.supportsWiFi6();

// Check codec support
boolean hasAV1 = QuestOptimizations.supportsAV1();

// Check refresh rate support
boolean supports90Hz = QuestOptimizations.supportsHighRefreshRate();
boolean supports120Hz = QuestOptimizations.supports120Hz();

// Get max recommended bitrate
int maxBitrate = QuestOptimizations.getMaxRecommendedBitrate();
```

## Testing

### Controller Testing
1. Connect Quest controller
2. Check logcat for "Meta Quest controller detected - emulating as Xbox controller"
3. Verify controller type shows as "Xbox (0x1)"
4. Test all buttons in a game
5. Test motion sensors (if enabled in settings)

### Device Detection Testing
1. Start streaming on Quest device
2. Check logcat for Quest Device Information block
3. Verify correct generation detected
4. Verify capabilities match your Quest model

## Troubleshooting

### Controllers Not Detected
- Check vendor ID in logs (should be 10291 or 0x2833)
- Ensure controller is paired to Quest OS
- Try reconnecting the controller

### Not Emulating as Xbox
- Check logs for "Controller type: Xbox (0x1)"
- If showing "Unknown", controller may not be in database
- Report vendor/product ID for addition to controller_list.h

### Motion Sensors Not Working
- Enable "Gamepad motion sensors" in Moonlight settings
- Requires Android 12+ for Quest controllers
- Check logs for sensor manager initialization

### Wrong Streaming Settings
- Check detected Quest generation in logs
- Verify Build.MODEL contains "Quest 3" (or appropriate model)
- Manually adjust settings if auto-detection fails

## Architecture

All Quest support uses **standard Android APIs only**:
- No Meta SDK required
- No XR dependencies
- No OpenXR integration needed
- Works on any Android device (graceful degradation)

This makes the implementation:
- ✅ Upstream compatible
- ✅ Easy to maintain
- ✅ No additional build dependencies
- ✅ Zero breaking changes

## What's Next?

Potential future enhancements:
- Automatic preset application on first launch
- UI for selecting Quest-optimized presets
- VR-specific input modes (head tracking, hand tracking)
- OpenXR integration for advanced VR features

All future features will maintain upstream compatibility and use optional dependencies.

---

For detailed architecture information, see [QUEST_SUPPORT.md](QUEST_SUPPORT.md).
