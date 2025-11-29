# Quest SDK Features for Moonlight Enhancement

This document outlines Meta Quest SDK features available through OpenXR that could enhance the Moonlight streaming experience on Quest devices.

## Available SDK Features

### 1. Fixed Foveated Rendering (FFR) ⭐ HIGH PRIORITY

**Extension:** `XR_FB_foveation`

**What it is:**
- Renders the center of the frame at full resolution while reducing resolution at the edges
- Exploits human peripheral vision limitations
- Significant GPU performance savings (20-30% improvement)

**Benefits for Moonlight:**
- Reduced GPU load on Quest device during video decode/display
- Better battery life
- Smoother streaming experience
- Particularly beneficial for high-resolution streams (4K+)

**Implementation:**
```c
// Available in openxr_input.c
XrSwapchainCreateInfoFoveationFB foveationInfo = {
    .type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB,
    .flags = XR_SWAPCHAIN_CREATE_FOVEATION_SCALED_BIN_BIT_FB
};
```

**Difficulty:** Medium (SDK provides the implementation)

**References:**
- [Fixed Foveated Rendering Documentation](https://developers.meta.com/horizon/documentation/unity/os-fixed-foveated-rendering/)
- [Using Fixed Foveated Rendering](https://developers.meta.com/horizon/documentation/unity/unity-fixed-foveated-rendering/)

---

### 2. Meta Quest Super Resolution ⭐ HIGH PRIORITY

**Extension:** `XR_FB_composition_layer_settings` (quality sharpening option)

**What it is:**
- Single-pass spatial upscaling and sharpening
- Edge- and contrast-aware filtering
- Preserves details in foveal region while minimizing halos

**Benefits for Moonlight:**
- Sharper image quality from compressed video stream
- Can stream at lower resolution and upscale on device
- Reduces bandwidth requirements
- Improves perceived quality of compressed video

**Implementation:**
Available through composition layer settings. For native Android:
```c
// Enable super resolution via compositor layer settings
XrCompositionLayerSettingsFB layerSettings = {
    .type = XR_TYPE_COMPOSITION_LAYER_SETTINGS_FB,
    .layerFlags = XR_COMPOSITION_LAYER_SETTINGS_AUTO_LAYER_FILTER_BIT_FB
};
```

**Difficulty:** Medium

**References:**
- [Improve App Image Quality with Meta Quest Super Resolution](https://developers.meta.com/horizon/blog/vr-image-quality-meta-quest-super-resolution/)
- [Better VR Graphics With Meta Quest Super Resolution](https://www.beyondgames.biz/39595/better-vr-graphics-with-meta-quest-super-resolution/)

---

### 3. Application SpaceWarp (ASW) ⭐ MEDIUM PRIORITY

**Extension:** `XR_FB_space_warp`

**What it is:**
- Motion extrapolation technique
- Synthesizes intermediate frames using motion vectors
- Effectively doubles frame rate (e.g., 36fps → 72fps, 45fps → 90fps)
- ~70% performance improvement

**Benefits for Moonlight:**
- Smoother streaming experience with lower bandwidth
- Could stream at 36fps or 45fps and let ASW interpolate to 72fps/90fps
- Reduces network bandwidth by 50%
- Reduces decode overhead on Quest

**Requirements:**
- OpenXR API (✓ already using)
- Vulkan API (would need to add)
- Provide depth buffer or motion vectors

**Difficulty:** High (requires Vulkan integration, motion vector generation)

**Note:** May be complex for a video streaming app since we don't control scene rendering. Most beneficial for native 3D apps.

**References:**
- [Application SpaceWarp Can Effectively Give Quest Apps 70% More Performance](https://www.uploadvr.com/quest-2-application-spacewarp/)
- [Developer Guide to ASW 2.0](https://developers.meta.com/horizon/blog/developer-guide-to-asw-20/)
- [Application SpaceWarp Developer Guide](https://developers.meta.com/horizon/documentation/unreal/unreal-asw/)

---

### 4. Dynamic Foveation (Auto FFR) ⭐ MEDIUM PRIORITY

**Extension:** Built into `XR_FB_foveation`

**What it is:**
- Automatically adjusts foveation level based on GPU utilization
- Maintains performance during demanding scenes
- Reduces foveation when GPU has headroom

**Benefits for Moonlight:**
- Adaptive quality based on decode complexity
- Better quality during simple scenes
- Maintains smoothness during complex scenes

**Implementation:**
```c
// Enable dynamic foveation
XrFoveationDynamicFB dynamicFoveation = {
    .type = XR_TYPE_FOVEATION_DYNAMIC_FB,
    .dynamicFlags = XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_BIT_FB
};
```

**Difficulty:** Low (built into FFR)

---

### 5. Color Space Management ⭐ LOW PRIORITY

**Extension:** `XR_FB_color_space`

**What it is:**
- Support for different color spaces (sRGB, DCI-P3, Rec.2020)
- Better color accuracy and HDR support

**Benefits for Moonlight:**
- Accurate color reproduction from streaming source
- HDR streaming support (if source supports it)
- Better color matching between PC and Quest display

**Difficulty:** Low-Medium

**References:**
- [Meta OpenXR SDK on GitHub](https://github.com/meta-quest/Meta-OpenXR-SDK)

---

### 6. 4x MSAA (Native Support) ⭐ LOW PRIORITY

**What it is:**
- Multi-Sample Anti-Aliasing built into Quest GPU
- Memoryless textures on Vulkan (no memory overhead)

**Benefits for Moonlight:**
- Smoother edges on UI elements
- Better visual quality for overlays
- Minimal performance cost on Quest 3

**Note:** Only beneficial for UI rendering, not the video stream itself.

**Difficulty:** Medium (requires Vulkan)

**References:**
- [Meta Quest Support | OpenXR Plugin](https://docs.unity3d.com/Packages/com.unity.xr.openxr@1.14/manual/features/metaquest.html)

---

### 7. Environment Depth (Quest 3 Only) ⭐ EXPERIMENTAL

**Extension:** `XR_META_environment_depth`

**What it is:**
- Depth-based occlusion for passthrough mode
- Allows virtual content to interact with real environment

**Benefits for Moonlight:**
- Mixed reality streaming experiences
- Could clip stream display behind real objects
- Interesting for creative use cases

**Note:** Quest 3/3S only, experimental feature

**Difficulty:** High (experimental API)

---

## Implementation Recommendations

### Phase 1: Quick Wins (SDK-Provided Features)
1. **Fixed Foveated Rendering** - Immediate performance boost
2. **Meta Quest Super Resolution** - Better image quality
3. **Dynamic Foveation** - Adaptive performance

**Estimated Effort:** 2-3 days
**Impact:** High

### Phase 2: Medium Complexity
4. **Color Space Management** - Better color accuracy
5. **Vulkan Integration** - Foundation for advanced features

**Estimated Effort:** 1-2 weeks
**Impact:** Medium

### Phase 3: Advanced Features (If Vulkan Integrated)
6. **Application SpaceWarp** - Frame interpolation
7. **4x MSAA** - UI anti-aliasing

**Estimated Effort:** 2-4 weeks
**Impact:** Medium-High

---

## Technical Requirements

### Current Stack
- ✅ OpenXR API (already integrated)
- ✅ Native Android NDK
- ⚠️ OpenGL ES (current renderer)

### Required for Advanced Features
- ❌ Vulkan API (needed for ASW, MSAA optimizations)
- ❌ Vulkan renderer integration

### SDK Downloads
- [Meta OpenXR SDK](https://developers.meta.com/horizon/downloads/package/oculus-openxr-mobile-sdk/)
- [Meta OpenXR SDK on GitHub](https://github.com/meta-quest/Meta-OpenXR-SDK)

---

## Sample Code References

The Meta OpenXR SDK includes 20+ sample projects demonstrating these features:

1. **XrCompositor_NativeActivity** - Demonstrates FFR
2. **XrSpaceWarp** - Demonstrates Application SpaceWarp
3. **XrColorSpacesFB** - Demonstrates color space management

Sample location: `\ovr_sdk_mobile\XrSamples\`

---

## Performance Estimates

Based on Meta's documentation and developer reports:

| Feature | GPU Savings | Quality Impact | Bandwidth Savings |
|---------|------------|----------------|-------------------|
| Fixed Foveated Rendering | 20-30% | Minimal (peripheral) | 0% |
| Super Resolution | 0-10% | +10-15% perceived | 10-20% (lower res stream) |
| Application SpaceWarp | ~70% | Varies | ~50% (half framerate) |
| Dynamic Foveation | 0-20% | Adaptive | 0% |
| Color Space Mgmt | 0% | Better accuracy | 0% |

---

## Comparison to FSR

**AMD FSR (FidelityFX Super Resolution):**
- Not provided by Quest SDK
- Would need custom implementation
- Requires compute shaders
- More complex than Meta's Super Resolution

**Recommendation:** Use Meta Quest Super Resolution instead of FSR
- SDK-provided (no custom implementation)
- Optimized for Quest hardware
- Lower development effort
- Officially supported by Meta

---

## Next Steps

1. **Research Phase:** Review Meta OpenXR SDK samples
2. **Prototype:** Implement FFR in a test branch
3. **Test:** Measure performance impact on Quest 2/3
4. **Integrate:** Add Super Resolution support
5. **Evaluate:** Decide if Vulkan migration is worthwhile for ASW

---

## References

### Official Documentation
- [OpenXR Support for Meta Quest Headsets](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr)
- [Native OpenXR Samples](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr-sample/)
- [OpenXR Mobile SDK](https://developers.meta.com/horizon/documentation/native/android/mobile-intro)

### Feature-Specific Guides
- [Fixed Foveated Rendering](https://developers.meta.com/horizon/documentation/unity/os-fixed-foveated-rendering/)
- [Meta Quest Super Resolution](https://developers.meta.com/horizon/blog/vr-image-quality-meta-quest-super-resolution/)
- [Application SpaceWarp Guide](https://developers.meta.com/horizon/documentation/unreal/unreal-asw/)

### Community Resources
- [OpenXR Toolkit Foveated Rendering](https://mbucchia.github.io/OpenXR-Toolkit/fr.html)
- [GitHub: Meta-OpenXR-SDK](https://github.com/meta-quest/Meta-OpenXR-SDK)

---

Last Updated: 2025-11-29
