#!/usr/bin/env python3
"""Report actual Live ISO payload and embedded FAT volume utilization."""
import argparse
import hashlib
import struct
from pathlib import Path

B = 2048
S = 512
M = 1024 * 1024


def u16(buf, off): return struct.unpack_from('<H', buf, off)[0]
def u32(buf, off): return struct.unpack_from('<I', buf, off)[0]


def volume_info(read, base, size, bits, payload_size):
    boot = read(base, S)
    bps = u16(boot, 11)
    spc = boot[13]
    reserved = u16(boot, 14)
    fats = boot[16]
    root_entries = u16(boot, 17)
    fat_sectors = u16(boot, 22) if bits == 16 else u32(boot, 36)
    sectors = u16(boot, 19) or u32(boot, 32)
    root_sectors = (root_entries * 32 + bps - 1) // bps
    data_sectors = sectors - reserved - fats * fat_sectors - root_sectors
    clusters = data_sectors // spc
    cluster_bytes = bps * spc
    allocated = ((payload_size + cluster_bytes - 1) // cluster_bytes) * cluster_bytes
    if sectors * bps != size or bps != S or spc == 0 or clusters <= 0:
        raise ValueError(f'invalid FAT{bits} volume geometry')
    return {
        'size': size, 'clusters': clusters, 'capacity': clusters * cluster_bytes,
        'allocated': allocated, 'percent': allocated * 100.0 / (clusters * cluster_bytes),
    }


def report(iso, efi, kernel_bin, kernel_elf, vault):
    iso = Path(iso); efi = Path(efi)
    with iso.open('rb') as f:
        def read(off, n):
            f.seek(off); data = f.read(n)
            if len(data) != n: raise ValueError('truncated ISO')
            return data
        mbr = read(0, S)
        if mbr[510:512] != b'\x55\xaa' or mbr[450] != 0xef:
            raise ValueError('hybrid MBR/EFI partition entry missing')
        start, count = struct.unpack_from('<II', mbr, 454)
        pvd = read(16 * B, B)
        if pvd[:7] != b'\x01CD001\x01': raise ValueError('ISO primary volume descriptor missing')
        total_bytes = u32(pvd, 80) * B
        if total_bytes != iso.stat().st_size or (start + count) * S != total_bytes:
            raise ValueError('ISO volume descriptor and MBR extent disagree with file size')
        boot_record = read(17 * B, B)
        if boot_record[:7] != b'\x00CD001\x01': raise ValueError('El Torito boot record missing')
        cat_lba = u32(boot_record, 71)
        cat = read(cat_lba * B, B)
        opt_lba = u32(cat, 40)
        opt_size = u16(cat, 38) * S
        if cat[:2] != b'\x01\xef' or cat[32] != 0x88 or opt_size == 0:
            raise ValueError('invalid El Torito EFI boot entry')
        usb_base = start * S
        usb_size = count * S
        payload_size = efi.stat().st_size
        optical = volume_info(read, opt_lba * B, opt_size, 16, payload_size)
        usb = volume_info(read, usb_base, usb_size, 32, payload_size)
    rows = [
        ('Kernel binary', Path(kernel_bin).stat().st_size),
        ('Kernel ELF', Path(kernel_elf).stat().st_size),
        ('EXT4 payload', Path(vault).stat().st_size),
        ('EFI application', payload_size),
        ('Optical FAT16', optical['size']),
        ('USB FAT32', usb['size']),
        ('Hybrid Live ISO', total_bytes),
    ]
    out = [
        'SWAMPOS LIVE IMAGE REPORT',
        '=========================',
        f'ISO: {iso.name}',
        f'SHA256: {hashlib.sha256(iso.read_bytes()).hexdigest()}',
        '',
        'Component sizes:',
    ]
    out.extend(f'  {name:<20} {size:>12,} bytes ({size/M:7.2f} MiB)' for name, size in rows)
    out += [
        '',
        'Embedded volume utilization (EFI file allocation only):',
        f"  Optical FAT16: {optical['allocated']:,} / {optical['capacity']:,} data bytes ({optical['percent']:.2f}%), volume {optical['size']/M:.2f} MiB",
        f"  USB FAT32    : {usb['allocated']:,} / {usb['capacity']:,} data bytes ({usb['percent']:.2f}%), volume {usb['size']/M:.2f} MiB",
        '',
        'Note: reported free space excludes filesystem metadata and directory clusters.',
        'The USB volume follows FAT32 minimum-cluster requirements and grows with payload demand.',
    ]
    return '\n'.join(out) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('iso'); ap.add_argument('efi'); ap.add_argument('kernel_bin'); ap.add_argument('kernel_elf'); ap.add_argument('vault')
    ap.add_argument('-o', '--output')
    a = ap.parse_args()
    text = report(a.iso, a.efi, a.kernel_bin, a.kernel_elf, a.vault)
    if a.output: Path(a.output).write_text(text)
    else: print(text, end='')


if __name__ == '__main__': main()
