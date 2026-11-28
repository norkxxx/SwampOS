"""Verify raw Live USB FAT32 image sizing follows the EFI payload."""
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from mklive import build
from gptdisk import M, S

source = ROOT / 'build' / 'FARKUAD.EFI'
payload = source.read_bytes()
with tempfile.TemporaryDirectory(prefix='swamp-mklive-growth-') as td:
    td = Path(td)
    small_efi = td / 'small.efi'
    large_efi = td / 'large.efi'
    small_img = td / 'small.img'
    large_img = td / 'large.img'
    small_efi.write_bytes(payload)
    large_efi.write_bytes(payload + bytes(29 * M - len(payload)))
    small = build(small_img, small_efi)
    large = build(large_img, large_efi)
    assert large['fat32_bytes'] > small['fat32_bytes']
    assert large['image_bytes'] > small['image_bytes']
    for path, layout in ((small_img, small), (large_img, large)):
        data = path.read_bytes()
        assert len(data) == layout['image_bytes']
        assert data[510:512] == b'\x55\xaa'
        start, count = struct.unpack_from('<II', data, 454)
        assert start == 2048 and count * S == layout['fat32_bytes']
        boot = data[start*S:(start+1)*S]
        sectors = struct.unpack_from('<I', boot, 32)[0]
        assert sectors * S == layout['fat32_bytes']
print('PASS: raw Live USB image and FAT32 partition grow with EFI payload')
