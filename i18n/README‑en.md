<div align="center">

**Readme Language / Language：**
[English](../README.md)｜[日本語](./i18n/README_ja.md)｜[한국어](./i18n/README_ko.md)｜[Русский](./i18n/README_ru.md)｜[Français](./i18n/README_fr.md)｜[Deutsch](./i18n/README_de.md)｜[简体中文](./i18n/README_zh.md)｜[繁體中文](./i18n/README_zh_TW.md)




💖 Support this project: [Ko‑fi](https://ko-fi.com/edenls)

</div>

# SnapOCR
> High‑performance screenshot, desktop sticker and offline OCR utility built on C++17 / Qt6 for Windows platform.

SnapOCR is an open‑source all‑in‑one desktop utility for Windows 10 / 11 x64. It integrates high‑performance screenshot, floating desktop sticker preview, image annotation, multi‑language offline OCR recognition, delayed loop screenshot and auto‑update capabilities.

**UI supported languages: Chinese (Simplified), Chinese (Traditional), English, Japanese, Korean, German, French, Russian; OCR text recognition is available for all above languages.**

All image processing and text recognition are executed **locally on your device. Screenshots will never be uploaded to the cloud, protecting your privacy.**
Suitable for office material extraction, reading & translation assistance, web‑document text grabbing, developer asset capture, teaching‑demo screenshot annotation and many other scenarios.

Developed with native Windows APIs and Qt6. It supports high‑DPI multi‑monitor setups, provides light / dark themes, full internationalized multi‑language UI, and can run in the background from system tray.

---
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
1. Get latest release from [Releases](https://github.com/xuhuandeshilinghun/snip‑ocr/releases): `SnapOCR‑<version>‑win64.zip`
2. Extract archive to any local folder. Portable software, no installer required.
3. Download separate OCR model archive and unpack it into the program’s `model/` sub‑folder.
4. Launch `SnapOCR.exe`.

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

## License
MIT License, refer to root‑directory `LICENSE` file.

## Support Project
SnapOCR is fully open‑source and free for personal use. If you find this tool helpful, you may support ongoing development voluntarily via Ko‑fi:
👉 [https://ko-fi.com/edenls](https://ko-fi.com/edenls)

## Contact & Feedback
‑ Email: senjin@foxmail.com
‑ Prefer to submit bug‑reports and feature‑suggestions through GitHub Issues.
