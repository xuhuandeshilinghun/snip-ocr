
import os
import json
import struct
from pathlib import Path

try:
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad
except ImportError:
    print("请先安装: pip install pycryptodome")
    exit(1)


AES_KEY = bytes.fromhex("2A7FB13C8E549D16C308F56AE14B287952D9AFC7136F80BD9435EE07CA214F6D")
BIN_MAGIC = 0x4C4F4345  # "LOCE"
BIN_VERSION = 0x0001

LOCALES_DIR = Path(__file__).parent.parent / "resources" / "locales"
OUTPUT_DIRS = [
    Path(__file__).parent.parent / "resources" / "locales",
    Path(__file__).parent.parent / "build" / "Release" / "locales",
    Path(__file__).parent.parent / "build_release_1.0.0" / "locales",
]

LANGS = ["zh_CN", "zh_TW", "en_US", "ja", "ko", "de", "fr", "es", "pt", "ru"]


def compile_lang(lang_id):
    json_path = LOCALES_DIR / f"{lang_id}.json"
    if not json_path.exists():
        print(f"  SKIP: {json_path} not found")
        return False

    
    with open(json_path, "r", encoding="utf-8-sig") as f:
        data = json.load(f)

    
    plain = json.dumps(data, ensure_ascii=False, separators=(',', ':')).encode("utf-8")

    
    iv = os.urandom(16)
    cipher = AES.new(AES_KEY, AES.MODE_CBC, iv)
    ciphertext = cipher.encrypt(pad(plain, AES.block_size))

    
    bin_data = struct.pack("<III", BIN_MAGIC, BIN_VERSION, len(ciphertext)) + ciphertext + iv

    
    for out_dir in OUTPUT_DIRS:
        out_dir.mkdir(parents=True, exist_ok=True)
        out_path = out_dir / f"{lang_id}.bin"
        out_path.write_bytes(bin_data)
        print(f"  -> {out_path} ({len(bin_data)} bytes)")

    return True


if __name__ == "__main__":
    print("=== 编译多语言 .bin 文件 ===")
    for lang in LANGS:
        print(f"\n[{lang}]")
        compile_lang(lang)
    print("\n=== 完成 ===")
