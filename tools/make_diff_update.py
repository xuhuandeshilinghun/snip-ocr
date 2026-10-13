#!/usr/bin/env python3


import argparse
import hashlib
import json
import os
import sys
import zipfile
from datetime import datetime
from pathlib import Path


def calc_sha256(file_path: Path) -> str:
    h = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(8192):
            h.update(chunk)
    return h.hexdigest()


def scan_files(root_dir: Path) -> dict:
    
    result = {}
    for root, _, files in os.walk(root_dir):
        for name in files:
            full = Path(root) / name
            rel = full.relative_to(root_dir).as_posix()
            stat = full.stat()
            result[rel] = (stat.st_size, int(stat.st_mtime))
    return result


def make_zip(zip_path: Path, base_dir: Path, rel_files: list):
    
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for rel in rel_files:
            full = base_dir / rel
            if full.is_file():
                zf.write(full, rel)


def find_diff_files(base_files: dict, new_files: dict) -> tuple:
    
    changed = []
    deleted = []
    new_set = set(new_files.keys())
    base_set = set(base_files.keys())

    for rel in new_set:
        if rel not in base_set:
            changed.append(rel)
        elif new_files[rel] != base_files[rel]:
            changed.append(rel)

    for rel in base_set - new_set:
        deleted.append(rel)

    return changed, deleted


def main():
    parser = argparse.ArgumentParser(
        description="SnipOCR 差分更新包打包工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=,
    )
    parser.add_argument("--old", required=True, help="上一版本目录（用于生成差分包）")
    parser.add_argument("--new", required=True, help="新版本目录")
    parser.add_argument("--version", required=True, help="新版本号（如 1.0.2）")
    parser.add_argument(
        "--base-dir",
        default="",
        help="基准版本目录（如 build_release_1.0.0）。提供后全量包=与基准的差异，非44MB",
    )
    parser.add_argument("--base-version", default="1.0.0", help="基准版本号（默认 1.0.0）")
    parser.add_argument("--output", default="./update_output", help="输出目录")
    parser.add_argument("--update-log", default="", help="更新日志文本")
    parser.add_argument("--changelog-file", default="", help="更新日志文件路径")
    parser.add_argument("--force-update", action="store_true", help="强制更新")
    args = parser.parse_args()

    old_dir = Path(args.old).resolve()
    new_dir = Path(args.new).resolve()
    output_dir = Path(args.output).resolve()
    new_version = args.version
    base_version = args.base_version

    
    base_dir = Path(args.base_dir).resolve() if args.base_dir else None
    use_base_mode = base_dir is not None and base_dir.is_dir()

    if not old_dir.is_dir():
        print(f"[ERROR] 旧版本目录不存在: {old_dir}")
        sys.exit(1)
    if not new_dir.is_dir():
        print(f"[ERROR] 新版本目录不存在: {new_dir}")
        sys.exit(1)
    if base_dir and not base_dir.is_dir():
        print(f"[WARN] 基准目录不存在: {base_dir}，回退到传统全量模式")
        use_base_mode = False

    output_dir.mkdir(parents=True, exist_ok=True)

    
    update_log = args.update_log
    if args.changelog_file:
        cl_path = Path(args.changelog_file)
        if cl_path.exists():
            update_log = cl_path.read_text(encoding="utf-8")

    mode_str = "基准差分" if use_base_mode else "传统全量"
    print("=" * 60)
    print(f"  SnipOCR 差分更新包打包工具 [{mode_str}模式]")
    print(f"  基准版本: V{base_version}")
    print(f"  上一版本: {old_dir.name}")
    print(f"  目标版本: V{new_version}")
    print(f"  新版目录: {new_dir}")
    if use_base_mode:
        print(f"  基准目录: {base_dir}")
    print(f"  输出目录: {output_dir}")
    print("=" * 60)

    
    print("\n[1/6] 扫描新版文件...")
    new_files = scan_files(new_dir)
    print(f"  新版文件数: {len(new_files)}")

    print("\n[2/6] 扫描旧版文件（用于差分包）...")
    old_files = scan_files(old_dir)
    print(f"  旧版文件数: {len(old_files)}")

    base_files = {}
    if use_base_mode:
        print(f"\n[3/6] 扫描基准文件（V{base_version}，用于全量包）...")
        base_files = scan_files(base_dir)
        print(f"  基准文件数: {len(base_files)}")
    else:
        print(f"\n[3/6] 跳过基准扫描（传统全量模式）")

    
    print("\n[4/6] 对比文件差异...")
    diff_changed, diff_deleted = find_diff_files(old_files, new_files)
    for rel in diff_changed:
        if rel not in old_files:
            print(f"  [差分包-新增] {rel}")
        else:
            print(f"  [差分包-修改] {rel}")

    
    if use_base_mode:
        full_changed, full_deleted = find_diff_files(base_files, new_files)
        for rel in full_changed:
            if rel not in base_files:
                print(f"  [全量包-新增] {rel}")
            else:
                print(f"  [全量包-修改] {rel}")
    else:
        full_changed = list(new_files.keys())
        full_deleted = []

    print(f"\n  全量包变化文件数: {len(full_changed)}")
    print(f"  差分包变化文件数: {len(diff_changed)}")
    print(f"  删除文件数: {len(full_deleted)}")

    if not full_changed and not diff_changed:
        print("\n[WARN] 没有变化的文件，无需打包")
        sys.exit(0)

    
    print(f"\n[5/6] 打全量包...")
    full_zip = output_dir / f"SnipOCR_update_full_v{new_version}.zip"
    if full_zip.exists():
        full_zip.unlink()
    print(f"  输出: {full_zip.name}")
    if use_base_mode:
        print(f"  模式: 基准差分（V{base_version} → V{new_version}），只含变化文件")
        make_zip(full_zip, new_dir, full_changed)
    else:
        print(f"  模式: 传统全量，包含所有文件")
        make_zip(full_zip, new_dir, full_changed)
    full_sha256 = calc_sha256(full_zip)
    full_size = full_zip.stat().st_size
    print(f"  SHA256: {full_sha256}")
    print(f"  大小: {full_size} bytes ({full_size/1024/1024:.2f} MB)")
    if use_base_mode:
        total_new_size = sum(s for s, _ in new_files.values())
        saved_pct = (1 - full_size / (total_new_size / 1024 / 1024) / 1024 / 1024) * 100 if total_new_size > 0 else 0
        print(f"  相比传统44MB全量包节省: {100 - full_size*100/(total_new_size):.1f}%")

    
    print(f"\n[6/6] 打差分包 (V{old_dir.name} → V{new_version})...")
    
    old_ver_label = old_dir.name.replace("build_release_", "")
    diff_zip = output_dir / f"SnipOCR_update_diff_v{old_ver_label}_to_v{new_version}.zip"
    if diff_zip.exists():
        diff_zip.unlink()
    print(f"  输出: {diff_zip.name}")
    make_zip(diff_zip, new_dir, diff_changed)
    diff_sha256 = calc_sha256(diff_zip)
    diff_size = diff_zip.stat().st_size
    print(f"  SHA256: {diff_sha256}")
    print(f"  大小: {diff_size} bytes ({diff_size/1024/1024:.2f} MB)")

    
    print(f"\n生成 version.json...")
    full_filename = full_zip.name
    diff_filename = diff_zip.name

    
    gitee_full_url = f"https://gitee.com/edenl/Snip-text-update/releases/download/v{new_version}/{full_filename}"
    github_full_url = f"https://github.com/xuhuandeshilinghun/Snip-text-update/releases/download/v{new_version}/{full_filename}"
    gitee_diff_url = f"https://gitee.com/edenl/Snip-text-update/releases/download/v{new_version}/{diff_filename}"
    github_diff_url = f"https://github.com/xuhuandeshilinghun/Snip-text-update/releases/download/v{new_version}/{diff_filename}"

    version_json = {
        "latest_ver": f"V{new_version}",
        "update_log": update_log,
        "force_update": args.force_update,
        "full_package": {
            "mode": "base_diff" if use_base_mode else "all_files",
            "base_version": base_version if use_base_mode else None,
            "requires_base": use_base_mode,
            "filename": full_filename,
            "sha256": full_sha256,
            "size": full_size,
            "changed_files": len(full_changed),
            "deleted_files": full_deleted,
        },
        "diff_package": {
            "base_version": old_ver_label,
            "filename": diff_filename,
            "sha256": diff_sha256,
            "size": diff_size,
            "changed_files": len(diff_changed),
            "deleted_files": diff_deleted,
        },
        "released_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    }

    version_path = output_dir / "version.json"
    version_path.write_text(
        json.dumps(version_json, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(f"  已生成: {version_path}")

    print("\n" + "=" * 60)
    print(f"  打包完成!")
    print(f"  全量包: {full_zip.name} ({full_size/1024/1024:.2f} MB)")
    print(f"  差分包: {diff_zip.name} ({diff_size/1024/1024:.2f} MB)")
    print(f"  版本文件: version.json")
    print(f"  输出目录: {output_dir}")
    print("=" * 60)
    print("\n上传说明:")
    print(f"  1. 把 {full_zip.name} 和 {diff_zip.name} 上传到 Gitee/GitHub Releases")
    print(f"  2. Gitee Release tag: v{new_version}")
    print(f"  3. GitHub Release tag: v{new_version}")
    print(f"  4. 把 version.json 上传到 Gitee raw / GitHub raw")
    print(f"\n  下载 URL（C++ 端自动构建，无需手填）：")
    print(f"    国内 Gitee:")
    print(f"      全量: {gitee_full_url}")
    print(f"      差分: {gitee_diff_url}")
    print(f"    海外 GitHub:")
    print(f"      全量: {github_full_url}")
    print(f"      差分: {github_diff_url}")
    if use_base_mode:
        print(f"\n  基准差分模式说明:")
        print(f"  - 全量包只含 V{base_version} → V{new_version} 的变化文件（非44MB）")
        print(f"  - 用户必须有 V{base_version}（1.0.0）作为底座")
        print(f"  - 差分包只含 V{old_ver_label} → V{new_version} 的变化文件（最小）")
        print(f"  - 升级逻辑：")
        print(f"    本地==V{old_ver_label} → 下载差分包")
        print(f"    本地!=V{old_ver_label} 但>=V{base_version} → 下载全量包（基准差分）")
        print(f"    本地<V{base_version} → 需先安装 V{base_version} 基准版")


if __name__ == "__main__":
    main()
