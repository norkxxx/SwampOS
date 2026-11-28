from pathlib import Path
import struct
p=Path('build/FARKUAD.EFI').read_bytes();k=Path('build/kernel.bin').read_bytes();d=Path('build/vault.ext4').read_bytes();assert k in p and d in p
pe=struct.unpack_from('<I',p,0x3c)[0];assert p[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',p,pe+4)[0]==0x8664 and struct.unpack_from('<H',p,pe+24+68)[0]==10
with Path('build/Farkuad-Live.img').open('rb') as f:
 def read(lba,n=1):f.seek(lba*512);return f.read(n*512)
 m=read(0);assert m[510:]==b'\x55\xaa' and m[450]==0xef;start,count=struct.unpack_from('<II',m,454);assert start==2048 and count==131072
 b=read(start);r=struct.unpack_from('<H',b,14)[0];fs=struct.unpack_from('<I',b,36)[0];base=start+r+2*fs;fat=read(start+r,fs);assert fat==read(start+r+fs,fs)
 def cluster(c):return read(base+c-2)
 def entry(c,name):
  b=cluster(c)
  for i in range(0,512,32):
   e=b[i:i+32]
   if e[:11]==name:return (struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0]
  raise AssertionError(name)
 c,_=entry(2,b'EFI        ');c,_=entry(c,b'BOOT       ');c,n=entry(c,b'BOOTX64 EFI');out=bytearray();seen=set()
 while c<0xffffff8:
  assert 2<=c<count and c not in seen;seen.add(c);out.extend(cluster(c));c=struct.unpack_from('<I',fat,4*c)[0]&0xfffffff
 assert bytes(out[:n])==p and n==len(p)
assert Path('build/Farkuad-Live.img').stat().st_size==65*1024*1024
print('PASS: MBR type EF / FAT32 removable path / exact PE, kernel and EXT4 extraction (static)')
