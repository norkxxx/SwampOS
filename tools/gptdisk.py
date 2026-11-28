#!/usr/bin/env python3
"""Deterministic laboratory GPT + FAT32 image writer. Regular files only; never raw devices."""
import argparse, os, struct, uuid, zlib
from pathlib import Path
S=512; M=1024*1024; N=128; ES=128; TABLE=N*ES//S
ESP=uuid.UUID('c12a7328-f81f-11d2-ba4b-00a0c93ec93b')
DATA=uuid.UUID('ebd0a0a2-b9e5-4433-87c0-68b6b72699c7')
VAULT=uuid.UUID('5a700100-1b4d-4707-9110-5357414d5000')
DISK=uuid.UUID('5a700000-1b4d-4707-9110-5357414d5000')
PARTS=[('SWAMPBOOT',ESP,64),('SWAMPLOGIN',DATA,64),('SWAMPROOT',VAULT,17),('SWAMPHOME',VAULT,17),('SWAMPRECOVERY',DATA,2)]
def part_id(i): return uuid.UUID(f'5a7000{i:02x}-1b4d-4707-9110-5357414d50{i:02x}')
def crc(b): return zlib.crc32(b)&0xffffffff
def layout():
    p=2048; out=[]
    for i,(name,typ,mib) in enumerate(PARTS,1):
        n=mib*M//S;out.append((name,typ,part_id(i),p,p+n-1));p+=n
    return out,p+2048

def _fat32_geometry(sectors):
    """Return (FAT sectors, data clusters) for one-sector FAT32 clusters."""
    reserved=32; fats=2; fs=1
    while fs*128 < sectors-reserved-fats*fs+2: fs+=1
    return fs, sectors-reserved-fats*fs


def fat32_size_for_payload(payload_bytes, reserve_bytes=4*M, alignment_bytes=M):
    """Smallest aligned FAT32 volume holding payload plus reserve and FAT32 minimum clusters."""
    if payload_bytes <= 0 or alignment_bytes < S or alignment_bytes % S:
        raise ValueError('invalid FAT32 sizing request')
    chain=(payload_bytes+S-1)//S
    reserve=(reserve_bytes+S-1)//S
    target=max(65525, chain+2+reserve)
    # Start with enough room for the data clusters and both FATs; then align and re-check.
    sectors=target+32+2*((target+2+127)//128)
    align=alignment_bytes//S
    sectors=((sectors+align-1)//align)*align
    while True:
        fs,clusters=_fat32_geometry(sectors)
        if clusters>=target and fs*128>=clusters+2:
            break
        sectors+=align
    if sectors>0xffffffff:
        raise ValueError('FAT32 volume exceeds MBR sector limit')
    return sectors*S


def fat32(filename, app, directory, basename, volume, hidden, max_app=M, size_bytes=64*M):
    """FAT32 writer; defaults to the legacy 64 MiB volume for GPT system images."""
    if size_bytes <= 0 or size_bytes % S:
        raise ValueError('FAT32 size must be a positive multiple of 512 bytes')
    sectors=size_bytes//S; reserved=32; fats=2; fs=1
    while fs*128 < sectors-reserved-fats*fs+2: fs+=1
    clusters=sectors-reserved-fats*fs
    if sectors>0xffffffff or clusters<65525 or not 0<len(app)<=max_app or (len(app)+S-1)//S+2>clusters:raise ValueError('invalid FAT image payload')
    boot=bytearray(S);boot[:3]=b'\xeb\x58\x90';boot[3:11]=b'SWAMPOS '
    struct.pack_into('<HBHBHHBHHHII',boot,11,S,1,reserved,fats,0,0,0xf8,0,63,255,hidden,sectors)
    struct.pack_into('<IHHIHH',boot,36,fs,0,0,2,1,6)
    boot[64]=0x80;boot[66]=0x29;struct.pack_into('<I',boot,67,0x53570000+hidden//2048);boot[71:82]=volume.encode().ljust(11,b' ');boot[82:90]=b'FAT32   ';boot[510:]=b'\x55\xaa'
    info=bytearray(S);struct.pack_into('<I',info,0,0x41615252);struct.pack_into('<III',info,484,0x61417272,0xffffffff,0xffffffff);struct.pack_into('<I',info,508,0xaa550000)
    fat=bytearray(fs*S);chain=max(1,(len(app)+S-1)//S);last=5+chain-1
    for i,v in enumerate([0x0ffffff8,0x0fffffff,0x0fffffff,0x0fffffff,0x0fffffff]):struct.pack_into('<I',fat,4*i,v)
    for i in range(5,last+1):struct.pack_into('<I',fat,i*4, i+1 if i<last else 0x0fffffff)
    def ent(name,attr,c,size=0):
        e=bytearray(32);e[:11]=name; e[11]=attr;struct.pack_into('<H',e,20,c>>16);struct.pack_into('<HI',e,26,c&65535,size);return e
    def dn(n):return n.encode().ljust(11,b' ')
    root=ent(dn(volume),8,0)+ent(dn('EFI'),16,3)
    efi=ent(dn('.'),16,3)+ent(dn('..'),16,0)+ent(dn(directory),16,4)
    sub=ent(dn('.'),16,4)+ent(dn('..'),16,3)+ent(basename,32,5,len(app))
    start=hidden*S
    def put(offset,b):filename.seek(start+offset*S);filename.write(b)
    put(0,boot);put(1,info);put(6,boot);put(7,info);put(reserved,fat);put(reserved+fs,fat)
    base=reserved+2*fs
    for i,b in enumerate((root,efi,sub,app)):put(base+i,b)

def header(current,alternate,first,last,table_lba,entries_crc):
    h=bytearray(S);struct.pack_into('<8sIIIIQQQQ16sQIII',h,0,b'EFI PART',0x10000,92,0,0,current,alternate,first,last,DISK.bytes_le,table_lba,N,ES,entries_crc);struct.pack_into('<I',h,16,crc(h[:92]));return h

def build(output,manager,login,root=None,home=None):
    parts,total=layout();entries=bytearray(N*ES)
    for i,(name,typ,uid,first,last) in enumerate(parts):struct.pack_into('<16s16sQQQ72s',entries,i*ES,typ.bytes_le,uid.bytes_le,first,last,0,name.encode('utf-16le'))
    # root/home are optional pre-encrypted envelopes only; no plaintext is written by this tool.
    from vault import HEADER, MAGIC, MAX_IMAGE
    for p in (root,home):
        if p is not None:
            if not HEADER.size+16<len(p)<=HEADER.size+MAX_IMAGE+16:raise ValueError('invalid envelope length')
            magic,size,_,_=HEADER.unpack_from(p)
            if magic!=MAGIC or size<=0 or len(p)!=HEADER.size+size+16:raise ValueError('invalid envelope shape')
    mbr=bytearray(S);struct.pack_into('<B3sB3sII',mbr,446,0,b'\x00\x02\x00',0xee,b'\xff\xff\xff',1,min(total-1,0xffffffff));mbr[510:]=b'\x55\xaa'
    # Exclusive regular-file creation; never truncate existing images or devices.
    fd=os.open(output,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    try:
        with os.fdopen(fd,'wb') as f:
            f.truncate(total*S);f.write(mbr);f.write(header(1,total-1,34,total-34,2,crc(entries)));f.write(entries)
            f.seek((total-33)*S);f.write(entries);f.write(header(total-1,1,34,total-34,total-33,crc(entries)))
            fat32(f,manager,'BOOT',b'BOOTX64 EFI','SWAMPBOOT',parts[0][3]);fat32(f,login,'SWAMP',b'LOGIN   EFI','SWAMPLOGIN',parts[1][3])
            for i,b in ((2,root),(3,home)):
                if b is not None:f.seek(parts[i][3]*S);f.write(b)
            f.flush();os.fsync(f.fileno())
    except BaseException:
        os.unlink(output);raise

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output');p.add_argument('manager');p.add_argument('login');p.add_argument('--root-envelope');p.add_argument('--home-envelope');a=p.parse_args()
    from vault import bounded,MAX_IMAGE,HEADER
    def env(x):return bounded(x,MAX_IMAGE+HEADER.size+16) if x else None
    build(a.output,bounded(a.manager,M),bounded(a.login,M),env(a.root_envelope),env(a.home_envelope))
if __name__=='__main__':main()
