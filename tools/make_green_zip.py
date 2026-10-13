#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
将 build_release_<version> 目录打包为绿色版（解压即用）zip 压缩包。

绿色版结构（zip 内顶层为 SnapOCR-<version>/）：
  SnapOCR-<version>/
  ├── SnapOCR.exe
  ├── updater.exe          (若存在)
  ├── *.dll                (Qt / ONNX / OpenCV / VC 运行时)
  ├── .version.json
  ├── img/                 (img_res.bin)
  ├── locales/             (*.bin)
  └── README.txt           (首次运行说明，自动生成)

用法:
  python tools/make_green_zip.py --version 1.0.0
  python tools/make_green_zip.py --version 1.0.0 --src build_release_1.0.0 --out ./dist

说明:
  - 默认源目录为项目根下的 build_release_<version>
  - 输出 zip 命名: SnapOCR-<version>-win64.zip
  - 自动排除 history/、model/、*.pdb 等非发布文件
"""
import argparse
import json
import os
import sys
import time
import zipfile
from pathlib import Path

README_TEMPLATE = """SnapOCR v{version} - Windows 绿色版
========================================

使用方法:
  1. 解压到任意非系统目录（如 D:\\SnapOCR）
  2. 双击 SnapOCR.exe 运行
  3. 首次使用需下载 OCR 模型包，解压到程序的 model/ 子目录

系统要求:
  - Windows 10 / 11 x64
  - 请勿解压到 C:\\Windows、C:\\Program Files 等受保护目录

快捷:
  F1                截图
  Ctrl+Shift+S      截图 + OCR
  Esc               退出截图

更多信息: https://github.com/xuhuandeshilinghun/snip-ocr
"""

EXCLUDE_DIRS = {"history", "model", "__pycache__"}
EXCLUDE_SUFFIXES = {".pdb", ".ilk", ".exp", ".obj", ".tlog"}


def make_green_zip(src_dir: Path, out_zip: Path, version: str) -> bool:
    if not src_dir.is_dir():
        print(f"[ERROR] 源目录不存在: {src_dir}")
        return False

    top_dir = f"SnapOCR-{version}"

    files_to_pack = []
    for root, dirs, files in os.walk(src_dir):
        # 原地过滤排除目录
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for name in files:
            full = Path(root) / name
            if full.suffix.lower() in EXCLUDE_SUFFIXES:
                continue
            rel = full.relative_to(src_dir)
            files_to_pack.append((full, f"{top_dir}/{rel.as_posix()}"))

    if not files_to_pack:
        print("[ERROR] 没有可打包的文件")
        return False

    out_zip.parent.mkdir(parents=True, exist_ok=True)
    if out_zip.exists():
        out_zip.unlink()

    readme = README_TEMPLATE.format(version=version).encode("utf-8")

    print(f"\n打包绿色版: {out_zip}")
    print(f"  源目录: {src_dir}")
    print(f"  文件数: {len(files_to_pack) + 1}（含 README.txt）")
    print(f"  顶层目录: {top_dir}/")

    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for full, arc in files_to_pack:
            zf.write(full, arc)
        zf.writestr(f"{top_dir}/README.txt", readme)

    size_mb = out_zip.stat().st_size / 1024 / 1024
    print(f"  完成: {out_zip} ({size_mb:.2f} MB)")
    return True


def main():
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent

    ap = argparse.ArgumentParser(description="打包 SnipOCR 绿色版 zip")
    ap.add_argument("--version", required=True, help="版本号（如 1.0.0）")
    ap.add_argument("--src", default="", help="源目录（默认 build_release_<version>）")
    ap.add_argument("--out", default="", help="输出目录（默认项目根）")
    args = ap.parse_args()

    version = args.version.lstrip("vV")
    src_dir = Path(args.src).resolve() if args.src else (project_dir / f"build_release_{version}")
    out_dir = Path(args.out).resolve() if args.out else project_dir
    out_zip = out_dir / f"SnapOCR-{version}-win64.zip"

    print("=" * 60)
    print(f"  SnipOCR 绿色版打包  v{version}")
    print("=" * 60)

    if not make_green_zip(src_dir, out_zip, version):
        sys.exit(1)

    print("\n" + "=" * 60)
    print(f"  ✓ 绿色版打包完成")
    print(f"  输出: {out_zip}")
    print("=" * 60)


if __name__ == "__main__":
    main()
