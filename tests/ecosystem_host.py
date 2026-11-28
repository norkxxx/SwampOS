import sys, tempfile, struct, random, zipfile, io, stat, json, subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import swp, compat_inspect as c
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.exceptions import InvalidSignature

def reject(fn):
    try: fn()
    except (ValueError,TypeError,KeyError,InvalidSignature,zipfile.BadZipFile,UnicodeError,struct.error,RecursionError): return
    raise AssertionError('unexpected acceptance')
key=Ed25519PrivateKey.generate(); pub=key.public_key(); other=Ed25519PrivateKey.generate().public_key()
app=Path('build/sdk-demo.elf').read_bytes(); assert c.inspect(app)['native_profile_candidate']; assert c.inspect(Path('build/hello.elf').read_bytes())['native_profile_candidate']
base=swp.build('base','1.0.0',{'share/base.txt':b'base'},key)
a=swp.build('demo','1.0.0',{'bin/demo':app},key,{'base':'1.0.0'},['console']); assert a==swp.build('demo','1.0.0',{'bin/demo':app},key,{'base':'1.0.0'},['console'])
b=swp.build('demo','1.1.0',{'bin/demo':app+b'v2'},key,{'base':'1.0.0'})
assert swp.verify(a,pub)['permissions']==['console'];reject(lambda:swp.verify(a,other))
for n in ('../bad','/abs','a/../b','a\\b','a//b','a/.','a/','a/\x00'):
    reject(lambda:swp.build('demo','1.0.0',{n:b'x'},key))
reject(lambda:swp.build('demo','1.0.0',{'a':b'x','a/b':b'y'},key));reject(lambda:swp.build('demo','1.0.0',{'a':b'x'},key,permissions=['network']))
with tempfile.TemporaryDirectory() as td:
    s=swp.Store(td,pub);reject(lambda:s.install([a]));assert s.list()=={};s.install([a,base]);assert s.read('demo','bin/demo')==app;reject(lambda:s.remove('base'));s.install([b]);assert s.list()['demo']['version']=='1.1.0';reject(lambda:s.install([a]));s.rollback();assert s.list()['demo']['version']=='1.0.0';s.rollback();assert s.list()['demo']['version']=='1.1.0';s.remove('demo');s.remove('base');assert s.list()=={};s.rollback();assert set(s.list())=={'base'}
    c1=swp.build('a','1.0.0',{'x':b'x'},key,{'b':'1.0.0'});c2=swp.build('b','1.0.0',{'x':b'x'},key,{'a':'1.0.0'});reject(lambda:s.install([c1,c2]));assert set(s.list())=={'base'}
    # Verify corruption is detected before subsequent package operations.
    p=Path(td)/'objects'/(swp.digest(base)+'.swp');p.write_bytes(b'bad');reject(s.list)
with zipfile.ZipFile(io.BytesIO(a)) as z: members={i.filename:z.read(i) for i in z.infolist()}
def rewrite(changes,kind=stat.S_IFREG):
    out=io.BytesIO()
    with zipfile.ZipFile(out,'w') as z:
        for n,d in changes.items():
            info=zipfile.ZipInfo(n);info.create_system=3;info.external_attr=(kind|0o644)<<16;z.writestr(info,d)
    return out.getvalue()
for name in members:
    modified=dict(members);d=bytearray(modified[name]);d[-1]^=1;modified[name]=bytes(d);reject(lambda:swp.verify(rewrite(modified),pub))
reject(lambda:swp.verify(rewrite({**members,'payload/extra':b'x'}),pub));reject(lambda:swp.verify(rewrite(members,stat.S_IFLNK),pub))
# Actual PE build and package-format fixtures. Foreign archives are never unpacked.
pe=Path('build/GUILAB.EFI').read_bytes();assert c.inspect(pe)['format']=='PE32+';off=struct.unpack_from('<I',pe,60)[0];assert struct.unpack_from('<II',pe,off+24+112+5*8)[1]>0
# Exact independent EFI payload extraction from GPT/FAT32.
with Path('build/GuiLab-GPT.img').open('rb') as f:
    def read(lba,count=1):f.seek(lba*512);return f.read(count*512)
    bs=read(2048);r=struct.unpack_from('<H',bs,14)[0];fs=struct.unpack_from('<I',bs,36)[0];start=2048+r+2*fs;fat=read(2048+r,fs)
    def cl(i):return read(start+i-2)
    def ent(i,name):
        d=cl(i)
        for off in range(0,512,32):
            e=d[off:off+32]
            if e[:11]==name:return (struct.unpack_from('<H',e,20)[0]<<16)|struct.unpack_from('<H',e,26)[0],struct.unpack_from('<I',e,28)[0]
        raise AssertionError(name)
    i,_=ent(2,b'EFI        ');i,_=ent(i,b'BOOT       ');i,n=ent(i,b'BOOTX64 EFI');out=bytearray();seen=set()
    while i<0xffffff8:
        assert i>=2 and i not in seen;seen.add(i);out.extend(cl(i));i=struct.unpack_from('<I',fat,4*i)[0]&0xfffffff
    assert bytes(out[:n])==pe
assert pe!=Path('build/BOOTLAB.EFI').read_bytes()
def ar(n,d):return n.encode().ljust(16)+b'0'.ljust(12)+b'0'.ljust(6)+b'0'.ljust(6)+b'100644'.ljust(8)+str(len(d)).encode().ljust(10)+b'`\n'+d+(b'\n' if len(d)%2 else b'')
deb=b'!<arch>\n'+ar('debian-binary',b'2.0\n')+ar('control.tar.xz',b'abc')+ar('data.tar.xz',b'abc');assert c.inspect(deb)['format']=='DEB'
rpm=b'\xed\xab\xee\xdb\x03'+bytes(91)+(b'\x8e\xad\xe8\x01'+bytes(12))*2+b'payload';assert c.inspect(rpm)['format']=='RPM'
rng=random.Random(1010);accepted=0
for sample in (app,pe,deb,rpm):
    for i in range(5000):
        m=bytearray(sample);m[rng.randrange(len(m))]^=rng.randrange(1,256)
        try: c.inspect(m);accepted+=1
        except (ValueError,UnicodeError,struct.error): pass
print(f'PASS: signed SWP transactions, rollback, dependency rejection, tamper cases, SDK native ELF, GUI PE/GPT, 20,000 compatibility mutations ({accepted} structurally accepted)')
