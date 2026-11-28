#!/usr/bin/env python3
"""Build a size-aware ISO9660/El Torito + MBR USB hybrid, x64 UEFI only.

The optical El Torito FAT16 image and removable-media FAT32 ESP are sized
from the EFI payload. Output is always a regular file; no physical device is
opened or modified.
"""
import os
import struct as st
import sys
import tempfile
from pathlib import Path
from gptdisk import fat32, fat32_size_for_payload, M, S

B = 2048
OPT = 512                     # optical FAT16 starts at 1 MiB
CAT = 22
ROOT = 21
DATE = bytes([126, 11, 27, 0, 0, 0, 0])
MAX_EFI_BYTES = 30 * M        # El Torito uses a 16-bit count of 512-byte sectors.
FAT16_FS_SECTORS = 256        # 65,536 FAT16 entries, enough for a near-32 MiB image.
FAT16_SLACK_SECTORS = 1024    # 512 KiB plus 1 MiB alignment slack.
ALIGN_SECTORS = B // S        # Keep embedded FAT volumes ISO-sector aligned.


def both16(n): return st.pack('<H', n) + st.pack('>H', n)
def both32(n): return st.pack('<I', n) + st.pack('>I', n)


def record(name, extent, size, flags=0):
    n = len(name)
    r = bytearray(33+n+(n%2 == 0))
    r[0] = len(r)
    r[2:10] = both32(extent)
    r[10:18] = both32(size)
    r[18:25] = DATE
    r[25] = flags
    r[28:32] = both16(1)
    r[32] = n
    r[33:33+n] = name
    return r


def _align_up(value, alignment):
    return ((value + alignment - 1) // alignment) * alignment


def fat16_size_for_payload(payload_bytes):
    """Return aligned FAT16 sectors for the optical El Torito boot image."""
    if payload_bytes <= 0 or payload_bytes > MAX_EFI_BYTES:
        raise ValueError(f'EFI payload must be between 1 byte and {MAX_EFI_BYTES} bytes')
    rootsecs = 512 * 32 // S
    base = 1 + 2 * FAT16_FS_SECTORS + rootsecs
    chain = (payload_bytes + S - 1) // S
    need_clusters = max(4085, chain + 2 + FAT16_SLACK_SECTORS)
    sectors = _align_up(base + need_clusters, ALIGN_SECTORS)
    # The El Torito boot catalog stores the virtual sector count in 16 bits.
    if sectors > 0xfffc:
        sectors = 0xfffc
    clusters = sectors - base
    if not (4085 <= clusters < 65525 and chain + 2 <= clusters and FAT16_FS_SECTORS * S // 2 >= clusters + 2):
        raise ValueError('EFI payload does not fit the El Torito FAT16 image')
    return sectors


def fat16(f, payload, sectors):
    """Write a dynamically sized FAT16 El Torito volume (512-byte clusters)."""
    reserved, fats, fs, roots = 1, 2, FAT16_FS_SECTORS, 512
    rootsecs = roots*32//S
    base = reserved+fats*fs+rootsecs
    clusters = sectors-base
    chain = (len(payload)+S-1)//S
    if not (0xfffc >= sectors >= 4085+base and 4085 <= clusters < 65525 and chain+2 <= clusters and fs*S//2 >= clusters+2):
        raise ValueError('FAT16 capacity exceeded')
    boot = bytearray(S)
    boot[:3] = b'\xeb\x3c\x90'; boot[3:11] = b'SWAMPOS '
    st.pack_into('<HBHBHHBHHHII', boot, 11, S, 1, reserved, fats, roots, sectors, 0xf8, fs, 63, 255, 0, 0)
    boot[36] = 0x80; boot[38] = 0x29
    st.pack_into('<I', boot, 39, 0x53574f50)
    boot[43:54] = b'FARKUAD    '; boot[54:62] = b'FAT16   '; boot[510:] = b'\x55\xaa'
    fat = bytearray(fs*S)
    for i, v in enumerate([0xfff8, 0xffff, 0xffff, 0xffff]): st.pack_into('<H', fat, 2*i, v)
    for c in range(4,4+chain): st.pack_into('<H', fat, 2*c, c+1 if c<3+chain else 0xffff)
    def ent(name, attr, c, size=0):
        e = bytearray(32); e[:11] = name; e[11] = attr
        st.pack_into('<HI', e, 26, c, size); return e
    def dn(s): return s.encode().ljust(11,b' ')
    def put(off, data): f.seek(OPT*B+off*S); f.write(data)
    put(0,boot); put(reserved,fat); put(reserved+fs,fat)
    put(reserved+2*fs,ent(dn('FARKUAD'),8,0)+ent(dn('EFI'),16,2))
    put(base,ent(dn('.'),16,2)+ent(dn('..'),16,0)+ent(dn('BOOT'),16,3))
    put(base+1,ent(dn('.'),16,3)+ent(dn('..'),16,2)+ent(b'BOOTX64 EFI',32,4,len(payload)))
    put(base+2,payload)


def _validate_efi(payload):
    if not 0 < len(payload) <= MAX_EFI_BYTES or payload[:2] != b'MZ':
        raise ValueError(f'invalid EFI size/header (maximum {MAX_EFI_BYTES} bytes)')
    pe = st.unpack_from('<I',payload,60)[0]
    if (pe+94 > len(payload) or payload[pe:pe+4] != b'PE\0\0'
            or st.unpack_from('<H',payload,pe+4)[0] != 0x8664
            or st.unpack_from('<H',payload,pe+24)[0] != 0x20b
            or st.unpack_from('<H',payload,pe+92)[0] != 10):
        raise ValueError('expected x64 EFI application')


def build(dest, efi):
    p = Path(dest)
    if p.is_symlink() or (p.exists() and not p.is_file()): raise ValueError('output must be regular file')
    src = Path(efi)
    if not src.is_file(): raise ValueError('EFI source is not a regular file')
    payload = src.read_bytes()
    _validate_efi(payload)

    opt_sectors = fat16_size_for_payload(len(payload))
    opt_bytes = opt_sectors * S
    usb_bytes = fat32_size_for_payload(len(payload))
    usb_sectors = usb_bytes // S
    if usb_bytes % B: raise ValueError('USB ESP size must be ISO-sector aligned')
    usb_lba = OPT + opt_bytes // B
    total = usb_lba + usb_bytes // B
    if total > 0xffffffff: raise ValueError('ISO9660 volume exceeds 32-bit sector limit')

    mbr=bytearray(S)
    st.pack_into('<B3sB3sII',mbr,446,0,b'\xfe\xff\xff',0xef,b'\xfe\xff\xff',usb_lba*4,usb_sectors)
    mbr[510:]=b'\x55\xaa'
    pvd=bytearray(B); pvd[:7]=b'\x01CD001\x01'
    pvd[8:40]=b'SWAMPOS'.ljust(32,b' '); pvd[40:72]=b'SWAMPOS_FARKUAD'.ljust(32,b' ')
    pvd[80:88]=both32(total); pvd[120:124]=both16(1); pvd[124:128]=both16(1); pvd[128:132]=both16(B)
    pvd[132:140]=both32(10); st.pack_into('<I',pvd,140,19); st.pack_into('>I',pvd,148,20)
    pvd[156:190]=record(b'\0',ROOT,B,2)
    pvd[190:813]=b' '*(813-190)
    pvd[574:702]=b'SWAMP MATRIX'.ljust(128,b' ')
    for off in (813,830,847,864): pvd[off:off+17]=b'2026112700000000\x00' if off in (813,830,864) else b'0000000000000000\x00'
    pvd[881]=1
    br=bytearray(B); br[:7]=b'\x00CD001\x01'; br[7:39]=b'EL TORITO SPECIFICATION'.ljust(32,b'\0'); st.pack_into('<I',br,71,CAT)
    term=b'\xffCD001\x01'.ljust(B,b'\0')
    cat=bytearray(B); cat[0]=1; cat[1]=0xef; cat[4:28]=b'SWAMPOS UEFI'.ljust(24,b' '); cat[30:32]=b'\x55\xaa'
    st.pack_into('<H',cat,28,(-sum(st.unpack_from('<16H',cat)))&0xffff)
    st.pack_into('<BBHBBHI',cat,32,0x88,0,0,0,0,opt_sectors,OPT)
    ptle=st.pack('<BBIHBB',1,0,ROOT,1,0,0); ptbe=st.pack('>BBIHBB',1,0,ROOT,1,0,0)
    root=b''.join((record(b'\0',ROOT,B,2),record(b'\1',ROOT,B,2),record(b'BOOT.CAT;1',CAT,B),record(b'EFIBOOT.IMG;1',OPT,opt_bytes),record(b'USBESP.IMG;1',usb_lba,usb_bytes)))
    fd,tmp=tempfile.mkstemp(prefix=p.name+'.',dir=p.parent)
    try:
        with os.fdopen(fd,'wb') as f:
            f.truncate(total*B); f.write(mbr)
            for lba,data in ((16,pvd),(17,br),(18,term),(19,ptle),(20,ptbe),(ROOT,root),(CAT,cat)):
                f.seek(lba*B); f.write(data)
            fat16(f,payload,opt_sectors)
            fat32(f,payload,'BOOT',b'BOOTX64 EFI','FARKUAD',usb_lba*4,MAX_EFI_BYTES,usb_bytes)
            f.flush(); os.fsync(f.fileno())
        os.replace(tmp,p)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)
    return {
        'efi_bytes': len(payload), 'opt_bytes': opt_bytes, 'usb_bytes': usb_bytes,
        'iso_bytes': total*B, 'opt_lba': OPT, 'usb_lba': usb_lba,
        'usb_sectors': usb_sectors, 'total_iso_sectors': total,
    }


if __name__=='__main__':
    if len(sys.argv)!=3: raise SystemExit('usage: mkiso.py OUTPUT.iso FARKUAD.EFI')
    layout=build(*sys.argv[1:])
    print('Live ISO layout:')
    print(f"  EFI payload : {layout['efi_bytes']:,} bytes")
    print(f"  Optical FAT16: {layout['opt_bytes']:,} bytes")
    print(f"  USB FAT32    : {layout['usb_bytes']:,} bytes")
    print(f"  Hybrid ISO   : {layout['iso_bytes']:,} bytes ({layout['iso_bytes']/M:.2f} MiB)")
