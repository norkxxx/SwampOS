#!/usr/bin/env python3
"""Build a size-aware removable x64 UEFI image; never writes a physical device."""
import os
import struct
import sys
import tempfile
from pathlib import Path
from gptdisk import fat32, fat32_size_for_payload, M, S

MAX_EFI_BYTES = 30 * M


def build(dest, efi):
    p = Path(dest)
    if p.exists() and not p.is_file(): raise ValueError('output must be regular file')
    if p.is_symlink(): raise ValueError('refusing symlink output')
    payload = Path(efi).read_bytes()
    if not 0 < len(payload) <= MAX_EFI_BYTES or payload[:2] != b'MZ':
        raise ValueError('invalid EFI size/header')
    size_bytes = fat32_size_for_payload(len(payload))
    sectors = size_bytes // S
    start = 2048
    mbr = bytearray(S)
    struct.pack_into('<B3sB3sII', mbr, 446, 0, b'\x00\x02\x00', 0xef, b'\xff\xff\xff', start, sectors)
    mbr[510:] = b'\x55\xaa'
    fd, tmp = tempfile.mkstemp(prefix=p.name+'.', dir=p.parent)
    try:
        with os.fdopen(fd, 'wb') as f:
            f.truncate((start+sectors)*S)
            f.write(mbr)
            fat32(f, payload, 'BOOT', b'BOOTX64 EFI', 'FARKUAD', start, MAX_EFI_BYTES, size_bytes)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, p)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)
    return {'efi_bytes': len(payload), 'fat32_bytes': size_bytes, 'image_bytes': (start+sectors)*S}


if __name__ == '__main__':
    if len(sys.argv) != 3: raise SystemExit('usage: mklive.py OUTPUT.img FARKUAD.EFI')
    layout = build(*sys.argv[1:])
    print(f"Live USB image: EFI={layout['efi_bytes']:,} bytes, FAT32={layout['fat32_bytes']:,} bytes, total={layout['image_bytes']:,} bytes")
