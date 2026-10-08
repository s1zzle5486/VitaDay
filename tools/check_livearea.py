#!/usr/bin/env python3
"""Validate the subset of LiveArea PNG constraints used by this application."""
import struct
import sys
import zlib
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

EXPECTED = {
    "sce_sys/icon0.png": (128, 128),
    "sce_sys/livearea/contents/bg.png": (840, 500),
    "sce_sys/livearea/contents/startup.png": (280, 158),
}

def check_png(data, name, dimensions):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{name}: invalid PNG signature")
    offset, chunks = 8, []
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        body = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from(">I", data, offset + 8 + length)[0]
        if zlib.crc32(kind + body) & 0xffffffff != crc:
            raise ValueError(f"{name}: invalid chunk CRC")
        chunks.append((kind, body))
        offset += length + 12
    if chunks[0][0] != b"IHDR" or chunks[-1][0] != b"IEND":
        raise ValueError(f"{name}: missing PNG header/end")
    width, height, depth, color, compression, filtering, interlace = struct.unpack(">IIBBBBB", chunks[0][1])
    if (width, height) != dimensions or depth != 8 or color != 3:
        raise ValueError(f"{name}: expected {dimensions}, indexed PNG, depth 8; got {(width, height)}, type {color}, depth {depth}")
    palette = next((body for kind, body in chunks if kind == b"PLTE"), b"")
    if len(palette) != 768 or compression or filtering or interlace:
        raise ValueError(f"{name}: expected full 256-entry palette and noninterlaced PNG")
    if name.endswith(("icon0.png", "bg.png")) and any(kind == b"tRNS" for kind, body in chunks):
        raise ValueError(f"{name}: transparency is not allowed")

def check(read):
    for name, dimensions in EXPECTED.items():
        check_png(read(name), name, dimensions)
    root = ET.fromstring(read("sce_sys/livearea/contents/template.xml"))
    for node in root.iter():
        if node.tag in ("image", "startup-image"):
            name = (node.text or "").strip()
            if "/" in name or ".." in name:
                raise ValueError("invalid LiveArea image reference")
            read("sce_sys/livearea/contents/" + name)

if __name__ == "__main__":
    target = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    try:
        if target.suffix == ".vpk":
            with zipfile.ZipFile(target) as archive:
                if archive.testzip():
                    raise ValueError("VPK ZIP checksum mismatch")
                check(archive.read)
        else:
            check(lambda name: (target / name).read_bytes())
    except Exception as error:
        print(f"LiveArea validation failed: {error}", file=sys.stderr)
        sys.exit(1)
    print("LiveArea PNG dimensions, 8-bit indexed palettes, CRCs and XML references: PASS")
