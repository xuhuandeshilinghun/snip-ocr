import os
import struct
import argparse
from pathlib import Path

try:
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad, unpad
except ImportError:
    print("请先安装: pip install pycryptodome")
    raise SystemExit(1)

AES_KEY = bytes.fromhex(
    "4B1FA37C9ED40D26F348E59AC16B3869"
    "62E9BFD7237F90CDA455FE17DA315F7D"
)
BIN_MAGIC = 0x494D4752 
BIN_VERSION = 0x0001

PROJECT_ROOT = Path(__file__).resolve().parent.parent
IMG_DIR = PROJECT_ROOT / "resources" / "img"
OUTPUT_DIRS = [
    PROJECT_ROOT / "resources" / "img",
    PROJECT_ROOT / "build" / "Release" / "img",
    PROJECT_ROOT / "build_release_1.0.2" / "img",
]
SKIP_NAMES = {"img_res.bin"}  


def _is_packable(p: Path) -> bool:
    name = p.name
    if name == "img_res.bin" or name.startswith("img_res.bin"):
        return False
    if name.endswith(".bak") or name.endswith(".tmp") or ".bak_" in name or ".bak." in name:
        return False
    return p.is_file()


def pack_image(img_dir: Path = IMG_DIR, out_path: Path | None = None) -> Path:
    if out_path is None:
        out_path = img_dir / "img_res.bin"

    files = sorted([p for p in img_dir.iterdir() if _is_packable(p)], key=lambda x: x.name)
    if not files:
        raise RuntimeError(f"img 目录下没有可打包的文件: {img_dir}")

    buf = bytearray()
    buf += struct.pack("<III", BIN_MAGIC, BIN_VERSION, len(files))

    total_plain = 0
    for f in files:
        plain = f.read_bytes()
        total_plain += len(plain)
        iv = os.urandom(16)
        cipher = AES.new(AES_KEY, AES.MODE_CBC, iv).encrypt(pad(plain, AES.block_size))
        name_bytes = f.name.encode("utf-8")
        buf += struct.pack("<H", len(name_bytes))
        buf += name_bytes
        buf += struct.pack("<I", len(cipher))
        buf += cipher
        buf += iv
        print(f"  + {f.name:<22} plain={len(plain):>7}B cipher={len(cipher):>7}B")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(bytes(buf))
    print(f"\n打包完成: {out_path}")
    print(f"  条目数: {len(files)}  明文总大小: {total_plain}B  bin 大小: {len(buf)}B")
    return out_path


def verify_bin(bin_path: Path) -> bool:
    data = bin_path.read_bytes()
    if len(data) < 12:
        print("VERIFY FAIL: 文件过小")
        return False
    magic, version, count = struct.unpack_from("<III", data, 0)
    if magic != BIN_MAGIC or version != BIN_VERSION:
        print(f"VERIFY FAIL: magic/version 不匹配 (got {magic:#x}/{version})")
        return False

    off = 12
    img_dir = bin_path.parent
    for i in range(count):
        name_len = struct.unpack_from("<H", data, off)[0]; off += 2
        name = data[off:off + name_len].decode("utf-8"); off += name_len
        cipher_len = struct.unpack_from("<I", data, off)[0]; off += 4
        cipher = data[off:off + cipher_len]; off += cipher_len
        iv = data[off:off + 16]; off += 16

        plain = unpad(AES.new(AES_KEY, AES.MODE_CBC, iv).decrypt(cipher), AES.block_size)
        src = img_dir / name
        ok = src.exists() and src.read_bytes() == plain
        flag = "OK" if ok else "MISMATCH"
        print(f"  [{i}] {name:<22} plain={len(plain):>7}B  [{flag}]")

    print(f"校验: {'PASS' if off == len(data) else 'FAIL (trailing bytes)'}  consumed={off}/{len(data)}")
    return off == len(data)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="打包 img_res.bin 加密图片资源包")
    ap.add_argument("--img-dir", default=str(IMG_DIR), help="源图片目录（默认 resources/img）")
    ap.add_argument("--out", default=None, help="输出路径（默认 <img-dir>/img_res.bin）")
    ap.add_argument("--verify", action="store_true", help="打包后立即读回校验")
    ap.add_argument("--copy", action="store_true", default=True,
                    help="同步复制到 build 输出目录（默认开启）")
    args = ap.parse_args()

    img_dir = Path(args.img_dir)
    out = pack_image(img_dir, Path(args.out) if args.out else None)

    if args.verify:
        print("\n=== 校验 img_res.bin ===")
        verify_bin(out)

    if args.copy:
        print("\n=== 同步到 build 输出目录 ===")
        for d in OUTPUT_DIRS:
            if d == img_dir:
                continue
            d.mkdir(parents=True, exist_ok=True)
            target = d / "img_res.bin"
            target.write_bytes(out.read_bytes())
            print(f"  -> {target} ({target.stat().st_size} bytes)")
    print("\n=== 完成 ===")
