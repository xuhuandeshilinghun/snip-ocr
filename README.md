<div align="center">

**Readme Language / Language：**
[English](./README.md)｜[日本語](./i18n/ja.md)｜[한국어](./i18n/ko.md)｜[Русский](./i18n/ru.md)｜[Français](./i18n/fr.md)｜[Deutsch](./i18n/de.md)｜[Español](./i18n/es.md)｜[Português](./i18n/pt.md)｜[简体中文](./i18n/zh.md)｜[繁體中文](./i18n/tw.md)




💖 Support this project: [Ko‑fi](https://ko-fi.com/edenls)

</div>

# SnipOCR
> High‑performance screenshot, desktop sticker and offline OCR utility built on C++17 / Qt6 for Windows platform.

SnipOCR is an open‑source all‑in‑one desktop utility for Windows 10 / 11 x64. It integrates high‑performance screenshot, floating desktop sticker preview, image annotation, multi‑language offline OCR recognition, delayed loop screenshot and auto‑update capabilities.

**UI supported languages: Chinese (Simplified), Chinese (Traditional), English, Japanese, Korean, German, French, Russian; OCR text recognition is available for all above languages.**

All image processing and text recognition are executed **locally on your device. Screenshots will never be uploaded to the cloud, protecting your privacy.**
Suitable for office material extraction, reading & translation assistance, web‑document text grabbing, developer asset capture, teaching‑demo screenshot annotation and many other scenarios.

Developed with native Windows APIs and Qt6. It supports high‑DPI multi‑monitor setups, provides light / dark themes, full internationalized multi‑language UI, and can run in the background from system tray.

---
## Demo
![capture](../docs/images/gif/9.gif)
![capture](./docs/images/gif/1.gif)
![capture](./docs/images/gif/2.gif)
![capture](./docs/images/gif/3.gif)
![capture](./docs/images/gif/4.gif)
![capture](./docs/images/gif/5.gif)
![capture](./docs/images/gif/6.gif)
![capture](./docs/images/gif/7.gif)
![capture](./docs/images/gif/8.gif)

## Features
### 📷 Screenshot Module
- **Global hotkey screenshot**: Implemented via Windows low‑level keyboard hook `WH_KEYBOARD_LL`. Trigger screenshot with one hotkey regardless of active window focus.
- **Free region selection**: Drag mouse to draw selection rectangle with 8 resize control‑points. Arrow‑key fine‑tuning: 1‑pixel per step, `Shift + Arrow‑Key` for 10‑pixel movement for precise pixel selection.
- **Window snap‑to‑edge**: Inspired by Snipaste. Selection edges auto‑snap to window borders with default 8‑pixel threshold for fast full‑window capture.
- **Accurate window & control detection**: Uses `WindowFromPoint` + `RealChildWindowFromPoint` to detect child controls such as buttons, input boxes and toolbars for direct control‑area selection.
- **DWM real window bounds retrieval**: Calls `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` to exclude window drop‑shadows and obtain actual rendered window dimensions.
- **Pixel magnifier**: Magnifier overlay in capture mode, displaying RGB / HEX color values and screen coordinates. Toggle color format for color‑picking and pixel inspection.
- **Selection dimension label**: Rounded blue background `W × H` size label shown near selection. Text color automatically adapts to light / dark theme.
- **Screenshot toolbar**: Bottom toolbar appears after region capture. Supports sticker pinning, OCR recognition, QR‑code recognition, copy image, save as PNG and cancel.

### 🖼️ Desktop Sticker & Annotation
- **Floating desktop stickers**: Pin captured screenshots directly on‑screen at original position, similar to Snipaste. Full DPR device‑pixel‑ratio support to avoid blurriness on high‑resolution displays.
- **Complete annotation toolkit**: Right‑click to open drawing toolbar: select, pen, arrow, rectangle, ellipse, highlight, mosaic pixelation, text note, eraser.
- **Image transformation**: Stickers support scaling from 20%‑500%; 90‑degree clockwise / counter‑clockwise rotation, horizontal & vertical flip. All operations work on original physical image resolution without quality loss.
- **Annotation bake export**: Merge all drawn annotations onto source image and export complete PNG preserving original resolution.
- **Screen color picker panel**: Independent color‑picker implementation, supports sampling any pixel on screen, multi‑language UI compatible.
- **Sticker window shadow toggle**: Enable or disable shadow effect for sticker windows for native Windows 11 rounded‑corner visual style.
- **Sticker group management**: Multiple sticker groups (default + custom groups). Switch groups to hide current‑group stickers and restore another group’s layout.
- **Persistent screenshot history**: Original screenshots saved as PNG inside local `history` folder. Sticker position, scale and annotation states stored in `.sp1` files with XOR+Base64 lightweight obfuscation (not strong encryption, only for accidental‑modification prevention).
- **Auto‑restore stickers on restart**: When application relaunches, all floating stickers restore their last positions, scaling and group assignments.

### 🔍 Offline OCR Text Recognition
- **Parallel multi‑model loading**: Load multiple recognition models simultaneously. Supports mixed Chinese‑English, Korean, Russian Cyrillic and Latin‑script text.
- **Automatic language detection**: Algorithm analyses character proportions in image and auto‑switches corresponding OCR model, no manual language selection required.
- **ONNX Runtime CPU inference**: Runs purely on CPU. **Discrete GPU is not required**, works on regular laptops.
- **Non‑blocking background worker thread**: OCR tasks run inside independent QThread worker so UI stays responsive during recognition.
- **Recognition result dialog**: Editable OCR output window. Supports one‑click copy‑to‑clipboard and saving text to local file.

### ⏱️ Delayed Screenshot
- **Delayed & loop capture**: Set delay from 1‑10 seconds, configure number of loop captures for batch auto‑saving.
- **System toast notification**: Windows native popup when capture sequence completes, showing save path and capture count. Click notification to copy file path.

### 🔄 Auto‑Update System
- **Version checking**: Automatic version check on startup; manual check available inside settings panel.
- **Delta update mechanism**: Generate full‑release packages and differential patch packages based on baseline version. Only changed files are downloaded to reduce download size.
- **Dual‑source routing**: Gitee download source for mainland‑China users, GitHub Releases for overseas users to resolve network‑access differences.
- **Independent updater binary**: `updater.exe` built on raw Win32 API + minizip, no Qt dependencies.
- **Preserve configuration & user data**: Backup user config file `.config_settings.json` during update. Restore preferences after update; hotkeys, themes and sticker‑group settings are retained.
- **Update‑restart workflow**: After update completes, invoke `ShellExecuteExW` to launch new program binary with `--restart` argument to bypass single‑instance mutex for clean restart.

### ⚙️ System Integration
- **System‑tray background service**: Minimize to tray. Tray right‑click menu includes screenshot, OCR, delayed capture, sticker groups, clear history, preferences, restart app and exit.
- **Application internationalization**: Built‑in Simplified‑Chinese `zh_CN` and English `en_US` UI. Switch language at runtime. Qt built‑in dialogs are also localized.
- **Light‑dark theme adaptation**: Follow‑system or manual theme switch. Overlay masks, menu dividers and window shadows adapt accordingly.
- **Single‑instance mutex**: Global Mutex prevents duplicate application launches. Special argument skips mutex check during update‑restart procedure.
- **UTF‑8 Chinese‑path compatibility**: Workaround for Qt native API bug which may return empty values for Windows UTF‑8 paths containing Chinese characters.

---
## Preview
| Capture Region | Sticker Annotation | OCR Result |
| --- | --- | --- |
| ![capture](./docs/images/capture.png) | ![sticker](./docs/images/sticker.png) | ![ocr](./docs/images/ocr.png) |

## Download & Installation
1. Get latest release from [Releases](https://github.com/xuhuandeshilinghun/snip‑ocr/releases): `SnipOCR‑<version>‑win64.zip`
2. Extract archive to any local folder. Portable software, no installer required.
3. Download separate OCR model archive and unpack it into the program’s `model/` sub‑folder.
4. Launch `SnipOCR.exe`.

> **Hard‑system‑requirement: Windows 10 / Windows 11 x64**
> OCR model files are large and are not bundled inside main program archive.
> Avoid extracting to system‑protected directories such as `C:\Windows` or `Program Files` to prevent permission read‑write issues.

## Shortcuts
| Shortcut | Function |
|---|---|
| `F1` | Trigger screenshot mode |
| `Ctrl+Shift+S` | Capture screenshot and run OCR recognition |
| `Esc` | Exit / cancel capture |
| `Mouse Wheel` | Fine‑tune selection by 1 pixel |
| `C` | Copy color value from magnifier picker |

## Tech Stack
- Language: C++17
- GUI Framework: Qt 6.5.3+
- Build System: CMake 3.20+ / MSVC
- OCR Inference: ONNX Runtime 1.18.1 CPU
- Image Processing: OpenCV 4.x
- JSON Parsing: nlohmann/json
- Zip Compress‑Decompress: minizip
- Windows System Interfaces: Win32 API, DWM, low‑level keyboard hook
- Compiler: MSVC2019 / MSVC2022

## Privacy & Security Notes
1. Screenshot processing, sticker rendering and OCR recognition run entirely locally. The software **will never automatically upload screenshots or recognized text to the internet.**
2. Auto‑update function connects to GitHub / Gitee servers solely for version validation and update‑package downloading.
3. Original screenshots stored as plain PNG inside `history` folder. Manually clean‑up history folder if you captured passwords, ID documents or other sensitive material.
4. `.sp1` and `index.dat` use simple XOR+base64 obfuscation **which is NOT secure encryption. Do not store highly‑confidential data inside these files.**
5. You can disable screenshot‑history saving inside application settings.

## Known Limitations
- Only Windows 10 / 11 x64 is supported at present. Linux and macOS are not available.
- OCR is CPU‑bound; recognition speed depends on your CPU performance.
- OCR model files are large and must be manually downloaded and deployed.
- Mixed‑DPI multi‑monitor configurations should be tested in real‑world usage; compatibility issues may appear on rare special‑hardware setups.

## Contributing
Bug reports and feature‑request Issues and Pull Requests are welcome.
1. Fork this repository
2. Create feature branch: `git checkout -b feature/your‑feature‑name`
3. Commit your changes
4. Push branch and submit PR

> Notes for submission: Do NOT commit build artifacts, build‑directory contents or large model files. Prefer git submodules for third‑party libraries.

## Developer: Build, Packaging and Release

> Not for end‑users, for project maintainers only.

### Pre‑release resource preprocessing

You **must** run these resource‑compilation scripts before every release packaging:

```
# Compile multi‑language locale resources
python tools\compile_locales.py
# Pack image resources
python tools\pack_img_res.py
```

### Update package selection logic (auto‑updater rules)

| Installed local version | Package type to download |
| --- | --- |
| V1.0.1 (previous official release) | Delta package (smallest, only several MB) |
| V1.0.0 (baseline version) | Full package (diff computed against baseline 1.0.0) |
| Other older versions | Full package (diff computed against baseline 1.0.0) |
| Not installed yet | **V1.0.0 baseline version must be installed first** |

> ⚠️ `build_release_1.0.0` is the global baseline directory. **Generate it once, keep it permanently, do not delete or modify it**. All subsequent delta updates depend on this baseline.

### make_release.py script examples

```
# 1.【Run once only】Create baseline version 1.0.0
python tools/make_release.py --version 1.0.0 --no-build

# 2.Release 1.0.1 (baseline fixed to 1.0.0, compare against old build_release_1.0.0)
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --base-dir build_release_1.0.0 --no-build

# 3.Release 1.0.2 (baseline fixed to 1.0.0, compare against old build_release_1.0.1)
python tools/make_release.py --version 1.0.2 --make-update --old build_release_1.0.1 --base-dir build_release_1.0.0 --no-build

# 4.Release 1.0.3 (baseline fixed to 1.0.0, compare against old build_release_1.0.2)
python tools/make_release.py --version 1.0.3 --make-update --old build_release_1.0.2 --base-dir build_release_1.0.0 --no-build

# Frequently‑used commands
# Full workflow: compile build + collect release artifacts
python tools/make_release.py --version 1.0.1

# Build compile + generate full package + delta update package
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0

# Binaries already compiled, packaging only, skip build step
python tools/make_release.py --version 1.0.1 --no-build

# Generate update packages with release notes and force‑update flag enabled
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --update-log "1.Optimize UI\n2.Fix known issues" --force-update
```

### Brief parameter reference

- `--version x.x.x`: Set target release version number
- `--no-build`: Skip C++ compilation, perform packaging only (use when binaries are already built)
- `--make-update`: Enable delta‑package generation; outputs both full package and delta patch
- `--old <dir>`: Directory of previous official release, used for file comparison to generate delta patches
- `--base-dir <dir>`: Global baseline directory, always set to `build_release_1.0.0`
- `--update-log`: Release notes for this version; use `\n` for line breaks
- `--force-update`: Enable force‑update flag; client will enforce mandatory upgrade

> Build outputs are placed under `build_release_<version>`. Upload the zip archives inside this folder to GitHub Releases.

## License
SnapOCR is licensed under **PolyForm Noncommercial License 1.0.0**.
> ✅ Personal / non-commercial use is free, you may use, modify and share.
> ⚠️ **Any commercial use (for-profit company internal use, integration into commercial software, selling redistributions) requires a separate commercial license from the author.**
> Contact me for commercial authorization.

## Support Project
SnipOCR is fully open‑source and free for personal use. If you find this tool helpful, you may support ongoing development voluntarily via Ko‑fi:
👉 [https://ko-fi.com/edenls](https://ko-fi.com/edenls)

## Contact & Feedback
‑ Email: senjin@foxmail.com
‑ Prefer to submit bug‑reports and feature‑suggestions through GitHub Issues.
