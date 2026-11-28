from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import vault,gptdisk as g
pw=b'ephemeral provisioning test';priv,pub=vault.keygen(pw);root=Path('build/vault.ext4').read_bytes();home=b'example home payload';a,b=vault.seal(root,pub),vault.seal(home,pub)
with TemporaryDirectory() as d:
    out=Path(d)/'encrypted.img';manager=Path('build/BOOTLAB.EFI').read_bytes();login=Path('build/LOGIN.EFI').read_bytes()
    try:g.build(out,manager,login,root)
    except ValueError:pass
    else:raise AssertionError('plaintext accepted')
    assert not out.exists();g.build(out,manager,login,a,b)
    with out.open('rb') as f:
        for i,blob,plain in ((2,a,root),(3,b,home)):
            part=g.layout()[0][i];f.seek(part[3]*512);raw=f.read(len(blob));assert raw==blob and raw!=plain and vault.unlock(raw,priv,pw)==plain
    try:g.build(out,manager,login,a,b)
    except FileExistsError:pass
    else:raise AssertionError('overwrote image')
print('PASS: offline GPT provisioning with encrypted ROOT/HOME envelopes, plaintext rejection, no overwrite, authenticated round trip')
