#!/usr/bin/env python
# -*- coding: utf-8 -*-


import argparse
import json
import time
import os
import shutil
import subprocess
import sys
from pathlib import Path


def find_msbuild() -> str:
    
    
    for env_key in ("MSBUILD_PATH", "Snip_MSBUILD"):
        val = os.environ.get(env_key, "").strip()
        if val and Path(val).is_file():
            return val

    
    vswhere_paths = [
        r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        r"C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe",
    ]
    for vswhere in vswhere_paths:
        if Path(vswhere).is_file():
            try:
                result = subprocess.run(
                    [vswhere, "-latest", "-requires", "Microsoft.Component.MSBuild",
                     "-find", r"MSBuild\**\Bin\MSBuild.exe"],
                    capture_output=True, text=True, timeout=10)
                lines = [l.strip() for l in result.stdout.strip().split("\n") if l.strip()]
                for line in lines:
                    if line and Path(line).is_file():
                        return line
            except Exception:
                pass

    
    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        r"C:\Windows\Microsoft.NET\Framework64\v4.0.30319\MSBuild.exe",
    ]
    for p in candidates:
        if Path(p).is_file():
            return p

    
    found = shutil.which("MSBuild.exe") or shutil.which("msbuild")
    if found:
        return found

    return ""


def find_cmake() -> str:
    
    val = os.environ.get("CMAKE_PATH", "").strip()
    if val and Path(val).is_file():
        return val

    found = shutil.which("cmake")
    if found:
        return found

    
    candidates = [
        r"C:\Program Files\CMake\bin\cmake.exe",
        r"C:\Program Files (x86)\CMake\bin\cmake.exe",
    ]
    for p in candidates:
        if Path(p).is_file():
            return p

    
    vswhere_paths = [
        r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe",
        r"C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe",
    ]
    for vswhere in vswhere_paths:
        if Path(vswhere).is_file():
            try:
                result = subprocess.run(
                    [vswhere, "-latest", "-property", "installationPath"],
                    capture_output=True, text=True, timeout=10)
                vs_path = result.stdout.strip()
                if vs_path:
                    vs_cmake = Path(vs_path) / "Common7" / "IDE" / "CommonExtensions" / "Microsoft" / "CMake" / "CMake" / "bin" / "cmake.exe"
                    if vs_cmake.is_file():
                        return str(vs_cmake)
            except Exception:
                pass

    return ""


def build_project(project_dir: Path) -> bool:
    
    msbuild = find_msbuild()
    if not msbuild:
        print("[ERROR] 找不到 MSBuild.exe")
        print("  请设置环境变量 MSBUILD_PATH 或安装 Visual Studio")
        print("  或运行: set MSBUILD_PATH=C:\\path\\to\\MSBuild.exe")
        return False

    cmake = find_cmake()
    if not cmake:
        print("[ERROR] 找不到 cmake.exe")
        print("  请设置环境变量 CMAKE_PATH 或安装 CMake")
        return False

    build_dir = project_dir / "build"
    build_dir.mkdir(exist_ok=True)

    
    print("\n[1/3] CMake 配置...")
    cmd = [cmake, "-S", str(project_dir), "-B", str(build_dir),
           "-G", "Visual Studio 16 2019", "-A", "x64",
           f"-DCMAKE_BUILD_TYPE=Release"]
    result = subprocess.run(cmd, cwd=str(project_dir))
    if result.returncode != 0:
        print("  CMake configure failed!")
        return False

    
    print("\n[2/3] MSBuild 编译...")
    sln = build_dir / "SnipOCR.sln"
    cmd = [msbuild, str(sln), "/p:Configuration=Release", "/p:Platform=x64",
           "/m", "/verbosity:minimal"]
    result = subprocess.run(cmd, cwd=str(build_dir))
    if result.returncode != 0:
        print("  Build failed!")
        return False

    print("\n[3/3] 构建完成!")
    return True


def collect_files(project_dir: Path, output_dir: Path, version: str) -> bool:
    
    print(f"\n[2/3] 收集发布文件...")

    if output_dir.exists():
        try:
            shutil.rmtree(output_dir)
        except PermissionError:
            print(f"  [ERROR] 无法删除旧的输出目录: {output_dir}")
            print(f"  可能有程序正在运行（如 SnipOCR.exe），请关闭后重试。")
            return False
    output_dir.mkdir(parents=True)

    
    release_dir = project_dir / "build" / "Release"
    if not release_dir.is_dir():
        print(f"  [ERROR] Release 目录不存在: {release_dir}")
        return False

    
    patterns = ["*.exe", "*.dll", "*.qml", "*.json"]
    for pattern in patterns:
        for f in release_dir.glob(pattern):
            shutil.copy2(f, output_dir / f.name)
            print(f"  {f.name}")

    
    skip_dirs = {"history", "model"}  
    for d in release_dir.iterdir():
        if d.is_dir() and d.name not in skip_dirs:
            dst = output_dir / d.name
            if dst.exists():
                shutil.rmtree(dst)
            shutil.copytree(d, dst)
            print(f"  {d.name}/")

    
    res_src = project_dir / "resources"
    if res_src.is_dir():
        res_dst = output_dir / "resources"
        if res_dst.exists():
            shutil.rmtree(res_dst)
        res_dst.mkdir(parents=True)
        for item in res_src.iterdir():
            if item.name == "model":
                
                continue
            elif item.name == "img":
                
                img_dst = res_dst / "img"
                img_dst.mkdir(parents=True)
                for img_file in item.glob("*.bin"):
                    shutil.copy2(img_file, img_dst / img_file.name)
                    print(f"  resources/img/{img_file.name}")
            elif item.name == "locales":
                
                locales_dst = res_dst / "locales"
                locales_dst.mkdir(parents=True)
                for loc_file in item.glob("*.bin"):
                    shutil.copy2(loc_file, locales_dst / loc_file.name)
                    print(f"  resources/locales/{loc_file.name}")
            else:
                
                dst = res_dst / item.name
                if item.is_dir():
                    shutil.copytree(item, dst)
                else:
                    shutil.copy2(item, dst)
                print(f"  resources/{item.name}/")

    
    version_data = {
        "latest_ver": f"V{version}",
        "updated_at": int(time.time()),
        "build_time": subprocess.run(
            ["powershell", "-Command", "Get-Date -Format 'yyyy-MM-dd HH:mm:ss'"],
            capture_output=True, text=True).stdout.strip(),
    }
    (output_dir / ".version.json").write_text(
        json.dumps(version_data, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"  .version.json")

    return True


def make_update_packages(project_dir: Path, new_dir: Path, version: str,
                         old_dir: Path = None, base_dir: Path = None,
                         base_version: str = "1.0.0",
                         update_log: str = "", force_update: bool = False) -> bool:
    
    script_path = project_dir / "tools" / "make_diff_update.py"
    if not script_path.exists():
        print(f"[ERROR] 找不到打包脚本: {script_path}")
        return False

    output_dir = project_dir / "update_output"
    output_dir.mkdir(exist_ok=True)

    cmd = [
        sys.executable, str(script_path),
        "--new", str(new_dir),
        "--version", version,
        "--output", str(output_dir),
    ]

    if old_dir and old_dir.exists():
        cmd.extend(["--old", str(old_dir)])
        cmd.extend(["--base-version", base_version])
    else:
        print("  ⚠ 未指定旧版本目录，仅生成全量包信息")
        cmd.extend(["--old", str(new_dir)])
        cmd.extend(["--base-version", version])

    
    if base_dir and base_dir.exists():
        cmd.extend(["--base-dir", str(base_dir)])
        print(f"  ✓ 基准差分模式：全量包=与 V{base_version} 的差异（非44MB）")
    else:
        print(f"  ℹ 传统模式：全量包=所有文件（44MB）")

    if update_log:
        cmd.extend(["--update-log", update_log])

    if force_update:
        cmd.append("--force-update")

    print(f"\n调用 make_diff_update.py...")
    result = subprocess.run(cmd, cwd=str(project_dir), capture_output=False, text=True)
    return result.returncode == 0


def main():
    parser = argparse.ArgumentParser(
        description="SnipOCR 发布打包工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 构建 1.0.1 版本
  python tools/make_release.py --version 1.0.1

  # 传统模式：构建并生成更新包（全量=44MB）
  python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0

  # 基准差分模式（推荐！全量包不是44MB）
  python tools/make_release.py --version 1.0.2 --make-update \\
      --old build_release_1.0.1 --base-dir build_release_1.0.0

  # 不构建，只从已有的 build/Release 收集文件
  python tools/make_release.py --version 1.0.1 --no-build
        """
    )
    parser.add_argument("--version", required=True, help="版本号（如 1.0.1）")
    parser.add_argument("--no-build", action="store_true", help="跳过构建，直接收集文件")
    parser.add_argument("--make-update", action="store_true", help="生成更新包（全量+差分）")
    parser.add_argument("--old", default="", help="上一版本目录（用于差分对比）")
    parser.add_argument("--base-dir", default="",
                        help="基准版本目录（如 build_release_1.0.0）。"
                             "提供后全量包=与基准的差异，非44MB（推荐！）")
    parser.add_argument("--base-version", default="1.0.0", help="基准版本号（默认 1.0.0）")
    parser.add_argument("--update-log", default="", help="更新日志")
    parser.add_argument("--force-update", action="store_true", help="强制更新")
    parser.add_argument("--output", default="", help="输出目录名（默认 build_release_<version>）")

    args = parser.parse_args()

    
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent

    version = args.version
    if version.startswith('V') or version.startswith('v'):
        version = version[1:]

    output_dir_name = args.output or f"build_release_{version}"
    output_dir = project_dir / output_dir_name

    print("=" * 60)
    print(f"  SnipOCR 发布打包工具")
    print(f"  版本: V{version}")
    print(f"  项目目录: {project_dir}")
    print(f"  输出目录: {output_dir}")
    print("=" * 60)

    
    if not args.no_build:
        if not build_project(project_dir):
            print("\n[ERROR] 构建失败")
            sys.exit(1)
    else:
        print("\n  ⏭ 跳过构建（--no-build）")

    
    if not collect_files(project_dir, output_dir, version):
        print("\n[ERROR] 收集文件失败")
        sys.exit(1)

    
    if args.make_update:
        old_dir = Path(args.old).resolve() if args.old else None
        if old_dir and not old_dir.exists():
            print(f"\n[WARN] 旧版本目录不存在: {old_dir}")
            old_dir = None

        base_dir = Path(args.base_dir).resolve() if args.base_dir else None
        if base_dir and not base_dir.exists():
            print(f"\n[WARN] 基准目录不存在: {base_dir}，回退到传统全量模式")
            base_dir = None

        if not make_update_packages(
            project_dir, output_dir, version,
            old_dir=old_dir,
            base_dir=base_dir,
            base_version=args.base_version,
            update_log=args.update_log,
            force_update=args.force_update,
        ):
            print("\n[ERROR] 生成更新包失败")
            sys.exit(1)

    print("\n" + "=" * 60)
    print(f"  ✓ 打包完成!")
    print(f"  发布目录: {output_dir}")
    if args.make_update:
        print(f"  更新包: {project_dir / 'update_output'}")
    print("=" * 60)


if __name__ == "__main__":
    main()
