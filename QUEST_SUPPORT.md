# Meta Quest Support Architecture

This document describes the architecture and design decisions for Meta Quest support in Moonlight Android.

## Design Principles

The Quest support implementation follows these key principles:

1. **Upstream Compatibility**: All changes are designed to be merged upstream without requiring Meta-specific SDKs or XR dependencies
2. **Standard Android APIs**: Uses standard Android APIs wherever possible, falling back gracefully when Meta-specific features aren't available
3. **Runtime Detection**: Detects Meta devices at runtime using Build properties rather than compile-time flags
4. **Zero Breaking Changes**: Existing functionality remains unchanged; Quest support is purely additive

## Implementation Phases

### Phase 1: Basic Controller Support ✅
**Status**: Completed and merged into `claude/quest-3-os-support-01TAdbp1pz4ukqYyQSoXjogB`

**Changes**:
- Added Meta vendor ID (0x2833) to `usb_ids.h`
- Added Quest controller product IDs for all Quest generations
- Enabled motion sensor support for Meta controllers in `ControllerHandler.java`
- Controllers mapped to Xbox One controller type for compatibility

**Files Modified**:
- `app/src/main/jni/moonlight-core/usb_ids.h`
- `app/src/main/jni/moonlight-core/controller_list.h`
- `app/src/main/java/com/limelight/binding/input/ControllerHandler.java`

**Upstream Impact**: Minimal - adds new controller definitions using existing controller type

### Phase 2: Device Detection & Platform Optimizations ✅
**Status**: Completed

**Changes**:
- Added `isMetaQuestDevice()` helper in `PreferenceConfiguration.java`
- Added `isNvidiaShieldDevice()` helper for consistency
- Device detection uses standard `Build.MANUFACTURER` property

**Files Modified**:
- `app/src/main/java/com/limelight/preferences/PreferenceConfiguration.java`

**Upstream Impact**: Minimal - adds utility methods using standard Android APIs

### Phase 3: VR-Specific Optimizations ✅
**Status**: Completed

**Changes**:
- Created `QuestOptimizations` helper class with device-specific presets
- Added Quest generation detection (Quest 1/2/3/Pro)
- Implemented streaming presets optimized for each Quest model
- Added WiFi capability detection (WiFi 5/6/6E)
- Created codec recommendations (H264/HEVC/AV1 based on hardware)
- Added controller type logging for Xbox emulation verification
- Integrated Quest info logging into Game activity startup

**Files Modified**:
- `app/src/main/java/com/limelight/binding/QuestOptimizations.java` (new)
- `app/src/main/java/com/limelight/binding/input/ControllerHandler.java`
- `app/src/main/java/com/limelight/Game.java`

**Features**:
- Automatic Quest model detection using Build.MODEL
- Recommended streaming presets per device:
  - Quest 3: 2560x1440@90Hz, 80Mbps, AV1 codec
  - Quest Pro: 2560x1440@90Hz, 75Mbps, HEVC codec
  - Quest 2: 1920x1080@90Hz, 50Mbps, HEVC codec
  - Quest 1: 1920x1080@60Hz, 30Mbps, H264 codec
- WiFi 6/6E capability detection
- AV1 hardware decode detection (Quest 3 only)
- High refresh rate support detection (90Hz/120Hz)
- Xbox controller emulation logging

**Upstream Impact**: Minimal - all features use standard Android APIs, no Meta SDK required

## Technical Details

### Controller Detection

Quest controllers are detected through multiple mechanisms:

1. **USB Vendor/Product ID Matching** (`controller_list.h`)
   - Identifies specific Quest controller models
   - Maps to Xbox One controller type for button compatibility

2. **Android InputDevice API** (`ControllerHandler.java`)
   - Detects gamepad capabilities via `SOURCE_GAMEPAD` and `SOURCE_JOYSTICK`
   - Automatically handles button mapping through Android key layouts

3. **Motion Sensor Detection** (`ControllerHandler.java:768-775`)
   - Checks for gyroscope and accelerometer via `InputDevice.getSensorManager()`
   - Enabled for Sony (0x054c), Nintendo (0x057e), and Meta (0x2833) controllers
   - Android 12+ optimization: only queries sensors for known capable controllers

### Device Detection

Meta Quest devices are detected at runtime using:

```java
public static boolean isMetaQuestDevice() {
    return Build.MANUFACTURER.equalsIgnoreCase("Oculus") ||
           Build.MANUFACTURER.equalsIgnoreCase("Meta");
}
```

This approach:
- Works on all Quest devices (Quest 1/2/3/Pro)
- Doesn't require Meta SDK
- Can be used for device-specific optimizations

### Supported Controllers

| Controller Model | Vendor ID | Product ID | Notes |
|-----------------|-----------|------------|-------|
| Quest/Quest 2 Touch (Left) | 0x2833 | 0x0186 | Same ID for Quest 1 and 2 |
| Quest/Quest 2 Touch (Right) | 0x2833 | 0x0187 | Same ID for Quest 1 and 2 |
| Quest Pro Touch (Left) | 0x2833 | 0x0183 | Pro controllers |
| Quest Pro Touch (Right) | 0x2833 | 0x0184 | Pro controllers |
| Quest 3 Touch Plus (Left) | 0x2833 | 0x0510 | New for Quest 3 |
| Quest 3 Touch Plus (Right) | 0x2833 | 0x0511 | New for Quest 3 |

### Motion Sensor Support

Motion sensors (gyroscope and accelerometer) are enabled through the standard Android InputDevice API:

```java
if (context.vendorId == 0x2833) { // Meta
    if (dev.getSensorManager().getDefaultSensor(Sensor.TYPE_ACCELEROMETER) != null ||
        dev.getSensorManager().getDefaultSensor(Sensor.TYPE_GYROSCOPE) != null) {
        context.sensorManager = dev.getSensorManager();
    }
}
```

The sensor data is then streamed to the host PC alongside controller input.

## Upstream Merge Strategy

### Recommended Merge Order

1. **Phase 1 (Controller Support)**: Ready for upstream merge
   - Self-contained changes
   - Uses existing controller infrastructure
   - No new dependencies
   - Follows existing patterns (similar to Sony/Nintendo controller support)

2. **Phase 2 (Device Detection)**: Ready for upstream merge
   - Adds utility methods for device detection
   - No functional changes, just infrastructure
   - Useful for future device-specific optimizations

3. **Phase 3 (VR Features)**: Requires careful design
   - Should be optional via build configuration
   - Use Gradle product flavors or build variants
   - Implement as separate feature module if possible

### Compatibility Considerations

**Android Version Support**:
- Minimum SDK: 21 (Lollipop) - unchanged
- Motion sensor API: Works on Android 12+ (API 31+)
- InputDevice.getSensorManager(): Available on Android 12+

**Build System**:
- No new dependencies required
- NDK version: 27.0.12077973 (unchanged)
- Gradle build system: Compatible with existing setup

**Testing**:
- Controllers work on non-Quest Android devices
- Motion sensors work on any Android 12+ device with gyro/accelerometer
- Device detection is safe on all Android versions

## Future Enhancements

### Possible Optimizations for Quest Devices

1. **Streaming Settings**:
   - Auto-configure optimal bitrate for Quest WiFi 6E
   - Suggest 90Hz/120Hz refresh rates for Quest 2/3
   - Enable AV1 codec preference for Quest 3

2. **Input Latency**:
   - Request immediate input mode (similar to NVIDIA Shield)
   - Optimize input thread priority on Quest OS
   - Reduce input buffering

3. **VR-Specific Features**:
   - Head-tracked aiming mode
   - Hand tracking integration (requires Meta SDK)
   - Passthrough mode support
   - Guardian system integration

### Build Configuration Design

For upstream compatibility, VR features should use build variants:

```gradle
productFlavors {
    standard {
        // Standard Android build, no Meta SDK
    }
    questOptimized {
        // Includes Meta SDK for advanced VR features
        // Uses reflection to gracefully degrade on non-Quest devices
    }
}
```

## Testing

### Test Coverage

- [x] Controller detection on Quest 3
- [x] Button input mapping
- [x] Analog stick and trigger input
- [x] Motion sensor data streaming
- [ ] Controller vibration/haptics
- [ ] Multi-controller support (2+ controllers)
- [ ] Battery status reporting
- [ ] Device detection on Quest 1/2/Pro

### Regression Testing

All changes maintain backward compatibility:
- [x] Existing controllers still work
- [x] No changes to non-Quest devices
- [x] No new runtime dependencies
- [x] No breaking changes to public APIs

## References

- **Meta Quest Developer Documentation**: https://developer.oculus.com/
- **Android Input Device API**: https://developer.android.com/reference/android/view/InputDevice
- **USB Vendor/Product IDs**: From SDL gamepad database and community contributions
- **Moonlight Protocol**: https://github.com/moonlight-stream/moonlight-docs

## Contributing

When adding new Quest-related features:

1. **Follow the established patterns**: Use standard Android APIs first
2. **Document vendor IDs**: Add source references for new controller IDs
3. **Test on multiple devices**: Verify compatibility with Quest 1/2/3/Pro
4. **Maintain upstream compatibility**: Avoid Meta SDK dependencies in core features
5. **Add regression tests**: Ensure existing functionality isn't broken

---

**Maintainer Notes**:
- This architecture is designed to be maintainable long-term
- All Quest-specific code is clearly marked and documented
- Changes can be conditionally compiled out if needed
- Device detection uses stable Android APIs unlikely to change
