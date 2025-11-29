# Project Modernization Summary

This document summarizes the modernization updates made to bring the Moonlight Android project up to date with recent toolkits and Java 25.

## Changes Made

### Build System Updates

#### Gradle
- **Updated from:** 8.7
- **Updated to:** 8.12
- **File:** `gradle/wrapper/gradle-wrapper.properties`
- **Benefit:** Latest stable Gradle with support for Java 25 and improved build performance

#### Android Gradle Plugin (AGP)
- **Updated from:** 8.5.1
- **Updated to:** 8.8.0
- **File:** `build.gradle`
- **Benefit:** Latest AGP with Java 25 support and modern Android build features

### Language & SDK Updates

#### Java Version
- **Updated from:** Java 11 (VERSION_11)
- **Updated to:** Java 25 (VERSION_25)
- **File:** `app/build.gradle` (compileOptions)
- **Benefit:** Access to latest Java language features and improvements

#### Android SDK
- **compileSdk:** 34 → 35
- **targetSdk:** 34 → 35
- **minSdk:** 21 (unchanged)
- **File:** `app/build.gradle`
- **Benefit:** Support for Android 15 features and APIs

#### NDK
- **Updated from:** 27.0.12077973
- **Updated to:** 28.0.12674087
- **File:** `app/build.gradle`
- **Benefit:** Latest NDK with improved native build performance and bug fixes

### Dependency Updates

All dependencies updated to their latest compatible versions in `app/build.gradle`:

| Dependency | Old Version | New Version |
|------------|------------|-------------|
| BouncyCastle Provider | 1.77 | 1.79 |
| BouncyCastle PKIX | 1.77 | 1.79 |
| JCodec | 0.2.5 | 0.2.5 (unchanged) |
| OkHttp | 4.12.0 | 4.12.0 (unchanged) |
| JmDNS | 3.5.9 | 3.5.9 (unchanged) |
| OpenXR Loader | 1.0.34 | 1.1.43 |

### Gradle Configuration Enhancements

Added modern build optimizations to `gradle.properties`:

```properties
# Increased memory allocation
org.gradle.jvmargs=-Xmx4096m -Dfile.encoding=UTF-8

# Performance optimizations
org.gradle.parallel=true                    # Parallel builds
org.gradle.caching=true                     # Build cache
org.gradle.configuration-cache=true         # Configuration cache

# Android optimizations
android.useAndroidX=true                    # AndroidX libraries
android.nonTransitiveRClass=true            # Non-transitive R classes
kapt.incremental.apt=true                   # Incremental annotation processing
```

## Build Performance Improvements

The modernization includes several optimizations:

1. **Parallel Builds:** Enable concurrent task execution
2. **Build Cache:** Reuse outputs from previous builds
3. **Configuration Cache:** Cache build configuration for faster subsequent builds
4. **Non-transitive R Classes:** Faster compilation by avoiding R class merging
5. **Increased Memory:** 4GB heap for better performance with large projects

## Compatibility

### Requirements
- **Java Development Kit:** JDK 25 or newer
- **Android Studio:** Latest version (Ladybug or newer recommended)
- **Gradle:** Will auto-download 8.12 via wrapper

### Build Variants
All existing build variants remain functional:
- `nonRootDebug` / `nonRootRelease`
- `nonRootQuestDebug` / `nonRootQuestRelease`
- `rootDebug` / `rootRelease`
- `rootQuestDebug` / `rootQuestRelease`

## Testing

After these changes, you should:

1. **Clean build:** `./gradlew clean`
2. **Sync Gradle:** Let IDE sync dependencies
3. **Build:** `./gradlew assembleNonRootDebug` (or your preferred variant)
4. **Test:** Run on emulator/device to verify functionality

## Troubleshooting

### Common Issues

**Issue:** Java version mismatch
- **Solution:** Ensure JDK 25 is installed and set as JAVA_HOME

**Issue:** NDK not found
- **Solution:** Install NDK 28.0.12674087 via Android Studio SDK Manager

**Issue:** Configuration cache errors
- **Solution:** Temporarily disable with `org.gradle.configuration-cache=false` in gradle.properties

**Issue:** Build fails with "Unsupported Java version"
- **Solution:** Update Android Studio to latest version or use command line with JDK 25

## Migration Notes

### For Developers

If you're working on this codebase:
- Ensure your IDE is using JDK 25 for the project
- Update your local Android SDK to include API 35
- Install NDK 28.0.12674087 through SDK Manager
- Clear `.gradle` and `build` folders if encountering cache issues

### For Contributors

When contributing:
- Code can now use Java 25 language features
- Target Android 15 (API 35) APIs when appropriate
- Test on both older (API 21+) and newer Android versions
- Build should work with standard `./gradlew` commands

## Future Considerations

- Monitor for Gradle 9.x release (when stable)
- Consider migrating from ndk-build to CMake for native builds (optional)
- Update remaining dependencies when new versions available
- Watch for Java 26 LTS release (if applicable)

## References

- [Gradle 8.12 Release Notes](https://docs.gradle.org/8.12/release-notes.html)
- [Android Gradle Plugin 8.8 Release Notes](https://developer.android.com/build/releases/gradle-plugin)
- [Java 25 Features](https://openjdk.org/projects/jdk/25/)
- [Android 15 Features](https://developer.android.com/about/versions/15)
- [NDK r28 Changelog](https://developer.android.com/ndk/downloads/revision_history)

---

Last Updated: 2025-11-29
