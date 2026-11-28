import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import vault, boot_registry as r
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.exceptions import InvalidSignature,InvalidTag
p=b'ephemeral test password';priv,pub=vault.keygen(p);s=Ed25519PrivateKey.generate();v=s.public_key()
d={'generation':4,'entries':[{'id':'swamp','partition':'5a700002-1b4d-4707-9110-5357414d5002','path':'\\EFI\\SWAMP\\LOGIN.EFI','sha256':'a'*64}]}
b=r.create(d,pub,s);assert r.open_registry(b,priv,p,v,4)==d and r.create(d,pub,s)!=b
for i in range(len(b)):
    a=bytearray(b);a[i]^=1
    try:r.open_registry(bytes(a),priv,p,v,4)
    except (ValueError,InvalidSignature,InvalidTag):pass
    else:raise AssertionError(i)
for a,k,pw,f in [(b,v,p,5),(b,Ed25519PrivateKey.generate().public_key(),p,4),(b,v,b'wrong',4),(b[:-1],v,p,4)]:
    try:r.open_registry(a,priv,pw,k,f)
    except (ValueError,InvalidSignature,InvalidTag):pass
    else:raise AssertionError('accepted invalid registry')
for path in ('\\EFI\\..\\X.EFI','/EFI/X.EFI','\\EFI\\X.EFI\\extra'):
    e={'generation':4,'entries':[dict(d['entries'][0],path=path)]}
    try:r.create(e,pub,s)
    except ValueError:pass
    else:raise AssertionError('path accepted')
print(f'PASS: signed/encrypted registry; {len(b)} byte mutations, wrong key/password, truncation, explicit rollback floor, path rejection')
