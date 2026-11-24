![License](https://img.shields.io/github/license/mausimus/ShaderGlass?color=red) ![GitHub Stars](https://img.shields.io/github/stars/mausimus/ShaderGlass?color=yellow) ![Downloads](https://img.shields.io/github/downloads/mausimus/ShaderGlass/total) ![Latest Release](https://img.shields.io/github/release-date/mausimus/ShaderGlass?label=latest%20release&color=blue) ![Beta Release](https://img.shields.io/github/release-date-pre/mausimus/ShaderGlass?label=beta%20release&color=orange)

## DISCLAIMER! Modified by your everyday idiot with zero coding knowledge and ability, using AI (Gemini, the free one), double checked and audited across multiple chats for almost a week to find any discrepancies/bugs/errors as best as I could until all chats said that it is good.

## Purpose of this fork

I created this fork because ShaderGlass did not have HDR support, due to which I started using RetroArch (1.22.2, later Nightly) + WindowCast for HDR, and while it looked awesome, WindowCast was... buggy, to say the least, and has been largely abandoned with no update for years, and honestly it was a hassle to set up at first. I then thought, since the source code for both ShaderGlass and RetroArch is available, and since AI have gotten quite decent at coding, why not just do an exact mathematical 1:1 implementation of RetroArch Nightly's HDR inside ShaderGlass. As stated in the disclaimer, I understand AI have memory rot over large chats, so I kept opening fresh chats, uploaded all necessary source files (oevr 40 for RetroArch), and asked it to compare/audit and confirm if math was an exact 1:1 replication. Now ShaderGlass has proper HDR support (literally has RetroArch's HDR). I asked Gemini to make a changelog or PR Description, pasted it below:

## Changes
Implement true 10-bit PQ HDR10 rendering pipeline (1:1 RetroArch D3D11 HDR port)

## Summary
Replaces the placeholder 16-bit scRGB swapchain toggle with a full 10-bit ST.2084 PQ (Rec.2020) HDR10 output pipeline matching RetroArch's D3D11 HDR implementation (`hdr_sm5.hlsl.h` / `d3d11.c` / `dxgi_common.c`).

### Why
Previously, enabling **Use HDR** switched the swapchain format without performing gamma-to-linear decoding, gamut rotation, inverse tone-mapping, or ST.2084 PQ encoding. This caused SDR Slang shaders to look washed out or over-bloomed in linear scRGB space and prevented native PQ HDR shaders (such as *Sony Megatron Color Video Monitor*) from outputting valid HDR10 signals.

## Key Changes
- **HDR10 Swapchain & Display Detection (`ShaderGlass.cpp`)**
  - Queries `IDXGIOutput6::GetDesc1()` to verify active Windows HDR (`DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`) and automatically falls back to SDR if Windows HDR is disabled or unsupported on the target display.
  - Configures the swapchain to `DXGI_FORMAT_R10G10B10A2_UNORM` with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` (`FLIP_DISCARD`) and dynamically pushes `DXGI_HDR_METADATA_HDR10` (Rec.2020 primaries, D65 white point, and display luminance limits) via `IDXGISwapChain4::SetHDRMetaData()`.
- **RetroArch-Compatible HDR Output Pass (`Shaders/HDRShaderDef.h`)**
  - Appends an `hdr_output` pass matching RetroArch's `hdr_sm5.hlsl.h` math:
    - `HDRDecodeGamma` (`pow(abs(value), 2.4)`) with sRGB FBO format normalization (`B8G8R8A8_UNORM_SRGB` -> `B8G8R8A8_UNORM`) on the preceding pass to avoid double linearization.
    - `To2020` gamut rotation supporting all four RetroArch **Colour Boost** modes (`0 = Accurate / Rec.709->2020`, `1 = Expanded 709`, `2 = Wide / DCI-P3`, `3 = Super / Rec. 2020`).
    - `InverseTonemap` paper-white scaling and SMPTE ST.2084 (`LinearToST2084`) PQ encoding.
  - Exposes live shader parameters in **Shader Parameters** (`Brightness` / Paper White nits, `Peak Brightness`, `Colour Boost`, and `Use Display Peak`).
- **Native HDR Shader Passthrough (`ShaderGlass.cpp`)**
  - Detects when the active preset's final pass already outputs `DXGI_FORMAT_R10G10B10A2_UNORM` (e.g., *Sony Megatron*), disables redundant inverse tone-mapping / PQ encoding (`Passthrough = 1`), and forwards `BrightnessNits`, `PeakNits`, `ExpandGamut`, and `HDRMode` uniforms directly to the preset.
- **Menu & Registry Persistence (`ShaderWindow.cpp`, `Resource.h`)**
  - Persists the **Use HDR** option in the registry and enforces Flip presentation mode when HDR is active.

## Testing
- Verified with standard SDR Slang presets (e.g., `crt-guest-advanced-ntsc`) on a Lenovo Legion 5 100% DCI-P3 1600p WQXGA OLED EDO EF10QBC64.C panel (`1015` nits peak), don't use Sony Megatron or MegaBezel so haven't tested those, or any, HDR CRT Shaders.
- Confirmed dynamic peak luminance detection via `IDXGIOutput6` (`Use Display Peak`) and clean fallback when Windows HDR is toggled off.




## ShaderGlass

Overlay for running GPU shaders on top of Windows desktop and Wine.

### Features

* applies shader effects on top of desktop, in a floating window or full-screen
* built-in [RetroArch](https://github.com/libretro/RetroArch) shader library (1200+ shaders!) covering:
  * CRT monitor simulation
  * image upscaling
  * TV, VHS and handheld simulation
  * softening, denoising, blur, sharpen and many more
* works with most emulators, retro platforms and pixel art editors including:
  * [DOSBox](https://www.dosbox.com/), [FS-UAE](https://github.com/FrodeSolheim/fs-uae), [Altirra](http://www.virtualdub.org/altirra.html),
  [ScummVM](https://github.com/scummvm/scummvm), [AGS](https://github.com/adventuregamestudio/ags), [VICE](https://sf.net/projects/vice-emu), [Aseprite](https://www.aseprite.org/) etc.
* excellent companion for pixel art showing shaded and/or aspect-ratio corrected preview
* you can even use it on top of YouTube, Twitch or modern games
* allows capture from a USB source (webcam or capture card)
* saving and loading profiles
* import of external .slangp/.slang shaders
* high customizability with various options, operating modes and shader parameters
* can be captured by OBS (using Game Capture source)

Check out [Online Manual](https://mausimus.github.io/ShaderGlass/MANUAL.html) for details.

<br/>

### Download

Latest __stable__ release (v1.3.2, 1 Nov 2026):
* desktop/window capture under Linux/Wine
* shader search function
* shader library update
* new command line parameters for automation
* minor enhancements

[https://github.com/mausimus/ShaderGlass/releases/download/v1.3.2/ShaderGlass-1.3.2-win-x64.zip](https://github.com/mausimus/ShaderGlass/releases/download/v1.3.2/ShaderGlass-1.3.2-win-x64.zip)

[Betas and older versions available here](https://github.com/mausimus/ShaderGlass/releases)

<br/>

[![Buy me a coffee!](images/ko-fi.png)](https://ko-fi.com/mausimus)

<br/>

### Get it on Steam!

[![ShaderGlass on Steam](images/steam.png)](https://store.steampowered.com/app/3613770/ShaderGlass/)

<br/>

Black Frame Insertion (BFI) and Blur Busters' CRT Beam Simulator have been spun off
into a new app [ShaderBeam](https://github.com/mausimus/ShaderBeam).

<br/>

### Requirements

* __Windows 10, version 2004__ (build 19041) or __Windows 11__
  * will work on version 1903 but in limited capacity (no Desktop Glass mode)
  * Windows 11 allows the __removal of yellow border__ (see [FAQ](FAQ.md#windows-10) for tips on avoiding it on Windows 10)
* DirectX 11-capable GPU
* __Linux__: running under Wine/Proton is supported since 1.3 using ScreenCast+PipeWire for capture (clone mode only)

<br/>

### Demo

Click to view on YouTube

[![ShaderGlass (YouTube)](https://img.youtube.com/vi/gWOcucS9_mg/maxresdefault.jpg)](https://www.youtube.com/watch?v=gWOcucS9_mg)

### Screenshots

ShaderGlass running over multiple applications on Windows 11 desktop.

![screenshot](images/screen7.png)

#### Desktop Glass mode

In this mode a transparent floating window will apply a shader to anything behind it.
Requires Windows 10 2004 - on 1903/1909 you will see only a black window if you switch to this mode.

Wikipedia in Chrome processed by crt-geom shader which applies scanlines and CRT curvature.

![screenshot](images/screen1.png)

#### Window Clone mode

When capture is fixed to a specific window it's easier to tune scaling to match the input
and image can also be recaptured (screenshot/OBS etc.)

##### FS-UAE

Amiga version of [The Secret of Monkey Island (1990)](https://store.steampowered.com/app/32360/The_Secret_of_Monkey_Island_Special_Edition/)
running in FS-UAE with crt-interlaced-halation shader applied.

![screenshot](images/screen4.png)

##### Altirra

[Ninja (1986)](https://www.mobygames.com/game/ninja_) for the Atari XL
running in Altirra with a TV-OUT simulation shader.

![screenshot](images/screen5.png)

##### Adventure Game Studio

[The Crimson Diamond (2024)](https://store.steampowered.com/app/1098770/The_Crimson_Diamond/),
a modern AGS game using HSM MegaBezel STD shader.

![screenshot](images/screen3.png)

##### DOSBox

[Police Quest (1987)](https://store.steampowered.com/app/494740/Police_Quest_Collection/)
with its massive half-EGA pixels, aspect-ratio corrected and post-processed using newpixie-crt shader.

![screenshot](images/screen2.png)

[Rick Dangerous (1989)](https://www.mobygames.com/game/rick-dangerous)
with C64 monitor shader applied.

![screenshot](images/screen6.png)

<br/>

### Instructions & Manual

See [Online Manual](https://mausimus.github.io/ShaderGlass/MANUAL.html) for option explanations and Frequently Asked Questions.

<br/>

### Code

Built using Visual Studio 2022 using ISO C++ 20, Windows SDK 10.0.26100, Windows Capture API and DirectX 11.

ShaderGlass includes a limited implementation of RetroArch shader back-end using DirectX 11.
[ShaderGen](ShaderGen) is a command-line tool for converting Slang shaders 
into .h files which can be precompiled in ShaderGlass. The conversion process relies on:
1. [glslang](https://github.com/KhronosGroup/glslang) for converting Slang/GLSL shaders to SPIR-V
2. [SPIR-V cross-compiler](https://github.com/KhronosGroup/SPIRV-Cross) for converting those to HLSL (DX11 format)
3. [Direct3D Shader Compiler (fxc.exe)](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/) for pre-compiling into bytecode

<br/>

### Notices

* ShaderGlass application is provided under [GNU General Public License v3.0](LICENSE)

* Includes precompiled shaders from [libretro/RetroArch shader repository](https://github.com/libretro/slang-shaders).
Please refer to copyright notes within shader code for detailed copyright and license information about each shader.

* App icon courtesy of Icons-Land

* Big kudos to RetroArch team, emulator developers and the wide retro community!

* Thanks to @lonestarr and @EndlesslyFlowering for PRs and everyone for feedback and testing :thumbsup:
