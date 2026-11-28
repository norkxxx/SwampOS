from pathlib import Path
from tempfile import TemporaryDirectory
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import vault
from cryptography.exceptions import InvalidTag

image=Path('build/vault.ext4').read_bytes()
password=b'ephemeral test-only password 2026'
priv,pub=vault.keygen(password)
blob=vault.seal(image,pub)
assert image!=blob and vault.unlock(blob,priv,password)==image
assert vault.seal(image,pub)!=blob

def reject(b,k,p,out):
    try: vault.unlock_to(b,k,p,out)
    except (ValueError,TypeError,InvalidTag): pass
    else: raise AssertionError('unexpected unlock')
    assert not out.exists()

with TemporaryDirectory() as d:
    out=Path(d)/'unlocked.ext4'
    reject(blob,priv,b'wrong',out)
    for pos in (0,8,16,28,vault.HEADER.size,vault.HEADER.size+4096,len(blob)-1):
        bad=bytearray(blob);bad[pos]^=1;reject(bytes(bad),priv,password,out)
    reject(blob[:-1],priv,password,out)
    wrong,_=vault.keygen(password);reject(blob,wrong,password,out)
    vault.unlock_to(blob,priv,password,out)
    assert out.read_bytes()==image and out.stat().st_mode&0o777==0o600
    try: vault.unlock_to(blob,priv,password,out)
    except FileExistsError: pass
    else: raise AssertionError('overwrote output')
    assert out.read_bytes()==image
print('PASS: RSA-4096 OAEP / AES-256-GCM image round trip, wrong password/key, tampering, truncation, no unauthenticated output, no overwrite (host only)')
