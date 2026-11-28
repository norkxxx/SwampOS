import struct,sys,uuid,zlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import gptdisk as g
p=Path('build/SwampBoot-GPT.img');parts,total=g.layout()
with p.open('rb') as f:
    def read(lba,n=1):f.seek(lba*512);return f.read(n*512)
    assert p.stat().st_size==total*512
    for current,alt,table in ((1,total-1,2),(total-1,1,total-33)):
        h=bytearray(read(current));c=struct.unpack_from('<I',h,16)[0];h[16:20]=b'\0'*4;assert zlib.crc32(h[:92])==c
        t=read(table,32);assert zlib.crc32(t)==struct.unpack_from('<I',h,88)[0]
        for i,(_,typ,uid,start,end) in enumerate(parts):assert struct.unpack_from('<16s16sQQ',t,i*128)==(typ.bytes_le,uid.bytes_le,start,end)
    def extract(start,dir_name,app):
        b=read(start);assert b[510:]==b'\x55\xaa' and b[82:90]==b'FAT32   ';r=struct.unpack_from('<H',b,14)[0];fs=struct.unpack_from('<I',b,36)[0];base=start+r+2*fs
        fat=read(start+r,fs);assert fat==read(start+r+fs,fs)
        def cluster(c):return read(base+c-2)
        def find(c,name):
            d=cluster(c)
            for i in range(0,512,32):
                e=d[i:i+32]
                if e[:11]==name:return (struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0]
            raise AssertionError(name)
        c,_=find(2,b'EFI        ');c,_=find(c,dir_name);c,n=find(c,app);out=b'';seen=set()
        while c<0xffffff8:
            assert c>=2 and c not in seen;seen.add(c);out+=cluster(c);c=struct.unpack_from('<I',fat,4*c)[0]&0xfffffff
        return out[:n]
    for i,di,ap,file in [(0,b'BOOT       ',b'BOOTX64 EFI','BOOTLAB.EFI'),(1,b'SWAMP      ',b'LOGIN   EFI','LOGIN.EFI')]:assert extract(parts[i][3],di,ap)==Path('build',file).read_bytes()
    for _,_,_,a,b in parts[2:]:assert not any(read(a)) and not any(read(b))
for name in ['BOOTX64.EFI','BOOTLAB.EFI','LOGIN.EFI']:
    b=Path('build',name).read_bytes();off=struct.unpack_from('<I',b,0x3c)[0];assert b[off:off+4]==b'PE\0\0' and struct.unpack_from('<H',b,off+4)[0]==0x8664;assert struct.unpack_from('<H',b,off+24+68)[0]==10
    assert struct.unpack_from('<II',b,off+24+112+5*8)[1]>0,'relocation table missing'
print('PASS: independent GPT/CRC + FAT32 extraction + PE32+ EFI subsystem/relocations; root/home/recovery empty')
