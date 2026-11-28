"""Verify Live ISO volumes grow with the EFI payload instead of fixed-size padding."""
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from mkiso import build, MAX_EFI_BYTES, fat16_size_for_payload
from gptdisk import fat32_size_for_payload, M

source = ROOT / 'build' / 'FARKUAD.EFI'
assert source.is_file()
payload = source.read_bytes()
with tempfile.TemporaryDirectory(prefix='swamp-iso-growth-') as td:
    td = Path(td)
    base_efi = td / 'base.efi'
    larger_efi = td / 'larger.efi'
    base_iso = td / 'base.iso'
    larger_iso = td / 'larger.iso'
    base_efi.write_bytes(payload)
    larger_efi.write_bytes(payload + bytes(3 * M))
    base = build(base_iso, base_efi)
    larger = build(larger_iso, larger_efi)
    assert larger['efi_bytes'] == base['efi_bytes'] + 3 * M
    assert larger['opt_bytes'] > base['opt_bytes']
    assert larger['iso_bytes'] > base['iso_bytes']
    assert larger['usb_bytes'] >= base['usb_bytes']
    assert base_iso.stat().st_size == base['iso_bytes']
    assert larger_iso.stat().st_size == larger['iso_bytes']
    # Exercise FAT16 El Torito boundary and FAT32 sizing threshold.
    assert fat16_size_for_payload(MAX_EFI_BYTES) <= 0xfffc
    try:
        fat16_size_for_payload(MAX_EFI_BYTES + 1)
    except ValueError:
        pass
    else:
        raise AssertionError('FAT16 El Torito sector-count overflow was accepted')
    assert fat32_size_for_payload(29 * M) > fat32_size_for_payload(len(payload))
    try:
        too_large = td / 'too-large.efi'
        too_large.write_bytes(payload + bytes(MAX_EFI_BYTES))
        build(td / 'too-large.iso', too_large)
    except ValueError:
        pass
    else:
        raise AssertionError('oversized El Torito EFI payload was accepted')
print('PASS: Live ISO optical and USB volumes are payload-sized; larger EFI produces a larger ISO')
