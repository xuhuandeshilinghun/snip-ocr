#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import io
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
THIRD_PARTY = PROJECT_ROOT / "third_party"

ONNX_VER = "1.18.1"
ONNX_URL = f"https://github.com/microsoft/onnxruntime/releases/download/v{ONNX_VER}/onnxruntime-win-x64-{ONNX_VER}.zip"
ONNX_DIRNAME = f"onnxruntime-win-x64-{ONNX_VER}"

NLOHMANN_VER = "3.11.3"
NLOHMANN_URL = f"https://github.com/nlohmann/json/releases/download/v{NLOHMANN_VER}/json.hpp"

OPENCV_VER = "4.10.0"
OPENCV_URL = f"https://github.com/opencv/opencv/releases/download/{OPENCV_VER}/opencv-{OPENCV_VER}-windows.exe"

# minizip 来自 zlib 仓库 contrib/minizip（取稳定 tag v1.3.1）
MINIZIP_BASE = "https://raw.githubusercontent.com/madler/zlib/v1.3.1/contrib/minizip/"
MINIZIP_FILES = ["ioapi.c", "ioapi.h", "unzip.c", "unzip.h", "crypt.h"]


def _download(url: str, dest: Path) -> None:
    """下载文件，支持 urllib（标准库）回退。"""
    dest.parent.mkdir(parents=True, exist_ok=True)
    print(f"  下载 {url}")
    print(f"    -> {dest}")
    try:
        import requests  # 优先用 requests（支持重定向/进度）
        with requests.get(url, stream=True, timeout=120) as r:
            r.raise_for_status()
            with open(dest, "wb") as f:
                for chunk in r.iter_content(chunk_size=1024 * 256):
                    if chunk:
                        f.write(chunk)
        return
    except ImportError:
        pass
    # 回退到 urllib
    import urllib.request
    urllib.request.urlretrieve(url, dest)


def setup_onnx(force: bool = False) -> None:
    target = THIRD_PARTY / ONNX_DIRNAME
    if (target / "lib" / "onnxruntime.lib").is_file() and not force:
        print(f"[ONNX] 已存在，跳过: {target}")
        return
    print(f"[ONNX] 准备 {ONNX_DIRNAME}")
    archive = THIRD_PARTY / f"{ONNX_DIRNAME}.zip"
    if not archive.is_file() or force:
        _download(ONNX_URL, archive)
    if target.exists():
        shutil.rmtree(target)
    with zipfile.ZipFile(archive) as zf:
        zf.extractall(THIRD_PARTY)
    archive.unlink(missing_ok=True)
    print(f"[ONNX] 完成: {target}")


def setup_nlohmann(force: bool = False) -> None:
    target = THIRD_PARTY / "nlohmann" / "json.hpp"
    if target.is_file() and not force:
        print(f"[nlohmann] 已存在，跳过: {target}")
        return
    print(f"[nlohmann] 准备 json.hpp v{NLOHMANN_VER}")
    _download(NLOHMANN_URL, target)
    print(f"[nlohmann] 完成: {target}")


def setup_minizip(force: bool = False) -> None:
    target_dir = THIRD_PARTY / "minizip"
    target_dir.mkdir(parents=True, exist_ok=True)
    need = [f for f in MINIZIP_FILES if not (target_dir / f).is_file()]
    if not need and not force:
        print(f"[minizip] 已存在，跳过: {target_dir}")
        return
    print(f"[minizip] 下载 {len(need) if not force else len(MINIZIP_FILES)} 个文件")
    for f in MINIZIP_FILES:
        if f in need or force:
            _download(MINIZIP_BASE + f, target_dir / f)
    print(f"[minizip] 完成: {target_dir}")


def _run(cmd, cwd=None):
    print(f"  $ {' '.join(str(c) for c in cmd)}")
    subprocess.run(cmd, cwd=cwd, check=True)


def setup_opencv(force: bool = False, build_zlib: bool = True) -> None:
    opencv_root = THIRD_PARTY / "opencv"
    build_lite = opencv_root / "build_lite"
    install_dir = build_lite / "install"
    sources_3rd = opencv_root / "sources" / "3rdparty"

    # 判断是否已完整
    bin_dir = install_dir / "x64" / "vc16" / "bin"
    world_dll = bin_dir / "opencv_world4100.dll"
    quirc_inc = sources_3rd / "quirc" / "include"
    zlib_src_h = sources_3rd / "zlib" / "zlib.h"

    done = world_dll.is_file() and quirc_inc.is_dir() and zlib_src_h.is_file()
    if done and not force:
        print(f"[OpenCV] 已就绪，跳过: {opencv_root}")
    else:
        print(f"[OpenCV] 准备 {OPENCV_VER}")
        archive = THIRD_PARTY / f"opencv-{OPENCV_VER}-windows.exe"
        if not archive.is_file() or force:
            _download(OPENCV_URL, archive)

        # .exe 是 7z 自解压包，按优先级用 py7zr / 7z.exe 解压
        extract_root = THIRD_PARTY / "_opencv_extract"
        if extract_root.exists():
            shutil.rmtree(extract_root)
        extract_root.mkdir(parents=True)

        extracted = False
        # 优先 py7zr（纯 Python，CI 本地都可用）
        try:
            import py7zr
            print("  使用 py7zr 解压...")
            with py7zr.SevenZipFile(str(archive), mode='r') as z:
                z.extractall(path=str(extract_root))
            extracted = True
        except ImportError:
            print("  [INFO] 未安装 py7zr，尝试 7z.exe")
        except Exception as e:
            print(f"  [WARN] py7zr 解压失败: {e}，尝试 7z.exe")

        # 回退 7z.exe（GitHub Actions windows-latest 自带）
        if not extracted:
            seven_zip = shutil.which("7z") or (
                r"C:\Program Files\7-Zip\7z.exe"
                if Path(r"C:\Program Files\7-Zip\7z.exe").is_file() else ""
            )
            if seven_zip:
                _run([seven_zip, "x", "-y", f"-o{extract_root}", str(archive)])
                extracted = True

        if not extracted:
            raise RuntimeError(
                "无法解压 OpenCV 包。请安装 py7zr (pip install py7zr) 或 7-Zip。"
            )

        # 解压后顶层有 opencv/ 目录
        src_opencv = extract_root / "opencv"
        if not src_opencv.is_dir():
            # 有些解压直接到 extract_root
            candidates = [p for p in extract_root.iterdir() if p.is_dir() and (p / "sources").is_dir()]
            if candidates:
                src_opencv = candidates[0]
            else:
                raise RuntimeError(f"无法在 {extract_root} 找到 opencv 目录")

        if opencv_root.exists():
            shutil.rmtree(opencv_root)
        opencv_root.mkdir(parents=True)

        # sources/3rdparty 整棵保留（quirc + zlib 都在里面）
        src_sources_3rd = src_opencv / "sources" / "3rdparty"
        sources_3rd.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(src_sources_3rd, sources_3rd)

        # build_lite/install/{include, x64/vc16/{bin,lib}}
        install_dir.mkdir(parents=True, exist_ok=True)
        src_build = src_opencv / "build"
        shutil.copytree(src_build / "include", install_dir / "include")
        shutil.copytree(src_build / "x64" / "vc16" / "bin", install_dir / "x64" / "vc16" / "bin")
        shutil.copytree(src_build / "x64" / "vc16" / "lib", install_dir / "x64" / "vc16" / "lib")

        shutil.rmtree(extract_root, ignore_errors=True)
        archive.unlink(missing_ok=True)
        print(f"[OpenCV] 目录重排完成: {opencv_root}")

    # 编译 zlib 静态库（updater 需要）
    if build_zlib:
        setup_zlib(opencv_root, force=force)


def setup_zlib(opencv_root: Path, force: bool = False) -> None:
    """用 CMake 编译 zlib.lib（updater 需要）。

    不直接调 cl.exe（需要 VS 开发者环境），改用 CMake 自动配置 MSVC 环境。
    """
    zlib_src = opencv_root / "sources" / "3rdparty" / "zlib"
    out_dir = opencv_root / "build_lite" / "3rdparty"
    lib_out = out_dir / "lib" / "Release"
    zlib_lib = lib_out / "zlib.lib"
    zconf_out = out_dir / "zlib"
    zconf_h = zconf_out / "zconf.h"

    if zlib_lib.is_file() and zconf_h.is_file() and not force:
        print(f"[zlib] 已编译，跳过: {zlib_lib}")
        return
    if not (zlib_src / "zlib.h").is_file():
        print(f"[zlib] 源码缺失: {zlib_src}，跳过编译")
        return

    cmake = _find_cmake()
    if not cmake:
        print(f"[zlib] 未找到 cmake，跳过编译")
        return

    print(f"[zlib] 编译 zlib.lib (via CMake)")

    # 从 zconf.h.cmakein 生成 zconf.h（OpenCV 的 3rdparty/zlib 用 CMake 模板）
    zconf_cmakein = zlib_src / "zconf.h.cmakein"
    zconf_h_src = zlib_src / "zconf.h"
    if zconf_cmakein.is_file() and not zconf_h_src.is_file():
        text = zconf_cmakein.read_text(encoding="utf-8", errors="ignore")
        text = text.replace("#cmakedefine Z_PREFIX", "/* #undef Z_PREFIX */")
        text = text.replace("#cmakedefine Z_HAVE_UNISTD_H", "/* #undef Z_HAVE_UNISTD_H */")
        zconf_h_src.write_text(text, encoding="utf-8")
        print(f"  生成 zconf.h from zconf.h.cmakein")

    # 写一个最小 CMakeLists.txt 到临时目录
    zlib_build = out_dir / "zlib_build"
    if zlib_build.exists():
        shutil.rmtree(zlib_build)
    zlib_build.mkdir(parents=True)
    (zlib_build / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.16)\n"
        "project(zlib_static C)\n"
        "set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)\n"
        "file(GLOB ZLIB_SRCS \"${CMAKE_CURRENT_SOURCE_DIR}/../../../../sources/3rdparty/zlib/*.c\")\n"
        "add_library(zlib STATIC ${ZLIB_SRCS})\n"
        "target_include_directories(zlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../sources/3rdparty/zlib)\n",
        encoding="utf-8",
    )

    # CMake 配置 + 构建（自动检测 VS 版本）
    gen, arch = _find_vs_generator()
    if not gen:
        print(f"[zlib] 未找到 Visual Studio，跳过编译")
        return
    _run([cmake, "-S", str(zlib_build), "-B", str(zlib_build / "build"),
          "-G", gen, "-A", arch], cwd=str(zlib_build))
    _run([cmake, "--build", str(zlib_build / "build"),
          "--config", "Release", "--target", "zlib"])

    # 找到生成的 zlib.lib（可能在 build/Release/ 或 build/zlib.dir/Release/）
    found_libs = list((zlib_build / "build").rglob("zlib.lib"))
    if not found_libs:
        raise RuntimeError(f"zlib.lib 未在 {zlib_build}/build 下生成")
    lib_out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(found_libs[0], zlib_lib)

    # 复制头文件到 build_lite/3rdparty/zlib（CMakeLists 需要 ZLIB_INCLUDE_DIR2）
    zconf_out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(zlib_src / "zconf.h", zconf_h)
    shutil.copy2(zlib_src / "zlib.h", zconf_out / "zlib.h")

    shutil.rmtree(zlib_build, ignore_errors=True)
    print(f"[zlib] 完成: {zlib_lib}")


def _find_cmake() -> str:
    """查找 cmake.exe：PATH → CMAKE_PATH 环境变量 → VS 内置 CMake。"""
    val = os.environ.get("CMAKE_PATH", "").strip()
    if val and Path(val).is_file():
        return val
    found = shutil.which("cmake")
    if found:
        return found
    for p in [r"C:\Program Files\CMake\bin\cmake.exe",
              r"C:\Program Files (x86)\CMake\bin\cmake.exe"]:
        if Path(p).is_file():
            return p
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if Path(vswhere).is_file():
        try:
            r = subprocess.run(
                [vswhere, "-latest", "-find",
                 r"Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"],
                capture_output=True, text=True, timeout=15,
            )
            for line in r.stdout.splitlines():
                line = line.strip()
                if line and Path(line).is_file():
                    return line
        except Exception:
            pass
    return ""


def _find_vs_generator() -> tuple:
    """返回可用的 CMake VS 生成器名与平台参数。
    优先 VS 2022，回退 VS 2019。找不到返回 (None, None)。"""
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    version_line = ""
    if Path(vswhere).is_file():
        try:
            r = subprocess.run(
                [vswhere, "-latest", "-property", "installationVersion"],
                capture_output=True, text=True, timeout=15,
            )
            version_line = r.stdout.strip()
        except Exception:
            pass
    if version_line.startswith("17"):
        return ("Visual Studio 17 2022", "x64")
    if version_line.startswith("16"):
        return ("Visual Studio 16 2019", "x64")
    # 回退：检查常见路径
    if Path(r"C:\Program Files\Microsoft Visual Studio\2022").exists():
        return ("Visual Studio 17 2022", "x64")
    if Path(r"C:\Program Files (x86)\Microsoft Visual Studio\2019").exists():
        return ("Visual Studio 16 2019", "x64")
    return (None, None)


def _find_msvc_tool(tool: str) -> str:
    """查找 MSVC 工具（cl.exe / lib.exe），优先 PATH，再用 vswhere。"""
    found = shutil.which(tool)
    if found:
        return found
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if Path(vswhere).is_file():
        try:
            r = subprocess.run(
                [vswhere, "-latest", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                 "-find", r"VC\Tools\MSVC\**\bin\Hostx64\x64\\" + tool],
                capture_output=True, text=True, timeout=15,
            )
            lines = [l.strip() for l in r.stdout.splitlines() if l.strip()]
            for line in lines:
                if line and Path(line).is_file():
                    return line
        except Exception:
            pass
    return ""


def main():
    ap = argparse.ArgumentParser(description="准备 SnipOCR 第三方依赖")
    ap.add_argument("--force", action="store_true", help="强制重新下载")
    ap.add_argument("--no-zlib", action="store_true", help="跳过 zlib 编译（不需要 updater 时）")
    args = ap.parse_args()

    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    print("=" * 60)
    print(f"  SnipOCR 第三方依赖准备")
    print(f"  目标目录: {THIRD_PARTY}")
    print("=" * 60)

    setup_onnx(force=args.force)
    setup_nlohmann(force=args.force)
    setup_minizip(force=args.force)
    setup_opencv(force=args.force, build_zlib=not args.no_zlib)

    print("\n" + "=" * 60)
    print("  ✓ 第三方依赖准备完成")
    print("=" * 60)
    print("\nOpenCV 库名提示:")
    print("  本脚本下载的是官方预编译包，使用 opencv_world4100。")
    print("  CMake 配置时需传入: -DOPENCV_LIBS=opencv_world4100")
    print("  本地 lite 构建（opencv_core4100 等）则无需该参数。")


if __name__ == "__main__":
    main()
