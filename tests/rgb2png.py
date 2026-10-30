"""Converts the host preview's raw 1920x1080 RGB dumps to PNG (stdlib only)."""
import struct
import sys
import zlib

W, H = 1920, 1080


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


for src in sys.argv[1:]:
    raw = open(src, "rb").read()
    assert len(raw) == W * H * 3, src
    rows = b"".join(b"\0" + raw[y * W * 3:(y + 1) * W * 3] for y in range(H))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b"")
    dst = src[:-4] + ".png"
    open(dst, "wb").write(png)
    print("wrote", dst)
