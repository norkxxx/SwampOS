from pathlib import Path
import struct
p=Path('build/KERNELLAB.EFI').read_bytes();k=Path('build/kernel.bin').read_bytes();off=struct.unpack_from('<I',p,0x3c)[0]
assert p[off:off+4]==b'PE\0\0' and struct.unpack_from('<H',p,off+4)[0]==0x8664
assert struct.unpack_from('<H',p,off+24+68)[0]==10 and struct.unpack_from('<II',p,off+24+112+5*8)[1]>0
assert k in p and b'SWAMP KERNEL LAB' not in k
# Independently extract EFI payload from the laboratory ESP, preserving locked image separately.
with Path('build/KernelLab-GPT.img').open('rb') as f:
 def read(lba,n=1):f.seek(lba*512);return f.read(n*512)
 boot=read(2048);r=struct.unpack_from('<H',boot,14)[0];fs=struct.unpack_from('<I',boot,36)[0];base=2048+r+2*fs;fat=read(2048+r,fs)
 def cluster(c):return read(base+c-2)
 def entry(c,name):
  b=cluster(c)
  for i in range(0,512,32):
   e=b[i:i+32]
   if e[:11]==name:return (struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0]
  raise AssertionError(name)
 c,_=entry(2,b'EFI        ');c,_=entry(c,b'BOOT       ');c,n=entry(c,b'BOOTX64 EFI');out=bytearray();seen=set()
 while c<0xffffff8:
  assert c>=2 and c not in seen;seen.add(c);out.extend(cluster(c));c=struct.unpack_from('<I',fat,4*c)[0]&0xfffffff
 assert bytes(out[:n])==p
 assert not any(read(264192)) # interior root/home remain empty by construction (first ROOT LBA)
with Path('build/SwampBoot-GPT.img').open('rb') as a, Path('build/KernelLab-GPT.img').open('rb') as b: assert a.read(512)==b.read(512)
assert Path('build/BOOTLAB.EFI').read_bytes()!=p
print('PASS: independent relocatable KERNELLAB PE, exact kernel embedding and FAT32 extraction')
