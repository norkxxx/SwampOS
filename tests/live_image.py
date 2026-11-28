"""Independent ISO9660/El Torito/MBR/FAT extraction checks."""
from pathlib import Path
import struct as s
from hashlib import sha256

B=2048; S=512; M=1024*1024
p=Path('build/FARKUAD.EFI').read_bytes()
assert Path('build/kernel.bin').read_bytes() in p and Path('build/vault.ext4').read_bytes() in p
pe=s.unpack_from('<I',p,60)[0]
assert p[pe:pe+4]==b'PE\0\0' and s.unpack_from('<H',p,pe+4)[0]==0x8664 and s.unpack_from('<H',p,pe+92)[0]==10
u16=lambda b,i:s.unpack_from('<H',b,i)[0]
u32=lambda b,i:s.unpack_from('<I',b,i)[0]
with Path('build/Farkuad-Live.iso').open('rb') as f:
 def read(off,n):
  f.seek(off); b=f.read(n); assert len(b)==n; return b
 def sector(lba):return read(lba*B,B)
 def both(b,i,sz):
  a=int.from_bytes(b[i:i+sz],'little'); assert a==int.from_bytes(b[i+sz:i+2*sz],'big'); return a
 m=read(0,S); assert m[510:]==b'\x55\xaa' and m[450]==0xef and not any(m[462:510]); start,count=s.unpack_from('<II',m,454)
 assert start%4==0 and count>0
 v=sector(16); assert v[:7]==b'\x01CD001\x01' and both(v,128,2)==B
 total=both(v,80,4)*B; assert total==Path(f.name).stat().st_size and (start+count)*S==total
 br=sector(17); assert br[:7]==b'\0CD001\x01' and br[7:30]==b'EL TORITO SPECIFICATION' and sector(18)[:7]==b'\xffCD001\x01'
 catlba=u32(br,71); cat=sector(catlba); assert cat[:2]==b'\x01\xef' and cat[30:32]==b'\x55\xaa' and sum(s.unpack_from('<16H',cat))%65536==0
 assert cat[32:34]==b'\x88\0'; opt=u32(cat,40); optsize=u16(cat,38)*S; assert len(p)<=optsize<=0xfffc*S and opt*B+optsize<=start*S
 root=both(v,158,4); size=both(v,166,4); assert size==B
 for off,endian in ((140,'little'),(148,'big')):
  table=sector(int.from_bytes(v[off:off+4],endian)); assert table[:2]==b'\x01\0' and int.from_bytes(table[2:6],endian)==root and int.from_bytes(table[6:8],endian)==1
 data=sector(root); entries={}; i=0
 while data[i]:
  n=data[i]; assert n>=34 and i+n<=B; r=data[i:i+n]; name=r[33:33+r[32]]; entries[name]=(both(r,2,4),both(r,10,4)); i+=n
 assert entries[b'BOOT.CAT;1']==(catlba,B) and entries[b'EFIBOOT.IMG;1']==(opt,optsize) and entries[b'USBESP.IMG;1']==(start//4,count*S)
 def extract(base, length, bits):
  def r(off,n):assert 0<=off<=length and 0<=n<=length-off;return read(base+off,n)
  b=r(0,S); assert b[510:]==b'\x55\xaa' and u16(b,11)==S and b[13]==1 and b[16]==2
  reserved=u16(b,14); roots=u16(b,17); fs=u16(b,22) if bits==16 else u32(b,36)
  sectors=u16(b,19) or u32(b,32); assert sectors*S==length
  assert u32(b,28)==(0 if bits==16 else start)
  rootsecs=(roots*32+S-1)//S; first=reserved+2*fs+rootsecs; clusters=sectors-first
  assert (4085<=clusters<65525 if bits==16 else clusters>=65525)
  fat=r(reserved*S,fs*S); assert fat==r((reserved+fs)*S,fs*S)
  def cluster(c):assert 2<=c<clusters+2;return r((first+c-2)*S,S)
  def entry(buf,name):
   for i in range(0,len(buf),32):
    e=buf[i:i+32]
    if e[:11]==name:return ((u16(e,20)<<16 if bits==32 else 0)|u16(e,26),u32(e,28))
   raise AssertionError(name)
  directory=r((reserved+2*fs)*S,rootsecs*S) if bits==16 else cluster(u32(b,44))
  c,_=entry(directory,b'EFI        ');c,_=entry(cluster(c),b'BOOT       ');c,n=entry(cluster(c),b'BOOTX64 EFI')
  out=bytearray();seen=set();eoc=0xfff8 if bits==16 else 0xffffff8
  while c<eoc:
   assert c not in seen;seen.add(c);out.extend(cluster(c));c=(u16(fat,c*2) if bits==16 else u32(fat,c*4)&0xfffffff)
  assert n==len(p) and len(out)==((n+S-1)//S)*S and out[:n]==p
  return sha256(out[:n]).hexdigest()
 a=extract(opt*B,optsize,16);b=extract(start*S,count*S,32);assert a==b
 opt_boot=read(opt*B,S); usb_boot=read(start*S,S)
 assert u16(opt_boot,19)*S==optsize and u32(usb_boot,32)*S==count*S
 assert total > (start*S) and optsize < count*S
 print(f'PASS: dynamic ISO layout: ISO={total:,} bytes, optical FAT16={optsize:,} bytes, USB FAT32={count*S:,} bytes, EFI SHA256={a}')
