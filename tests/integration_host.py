import sys,tempfile,subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import swp,stage_native
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

def reject(fn):
    try: fn()
    except (ValueError,OSError,KeyError): return
    raise AssertionError('unexpected acceptance')
app=Path('build/integration-demo.elf').read_bytes()
key=Ed25519PrivateKey.generate()
with tempfile.TemporaryDirectory() as t:
    root=Path(t); store=swp.Store(root/'store',key.public_key())
    pkg=swp.build('integration','0.11.0',{'bin/demo':app,'share/notes':b'notes','bin/bad':b'not executable','bin/overlord':Path('build/overlord-demo.elf').read_bytes() },key)
    store.install([pkg]); target=root/'demo.elf'
    stage_native.export(store,'integration','bin/demo',target); assert target.read_bytes()==app
    dual=root/'dual.elf'; stage_native.export(store,'integration','bin/overlord',dual); assert dual.read_bytes()==Path('build/overlord-demo.elf').read_bytes()
    reject(lambda:stage_native.export(store,'integration','bin/demo',target))
    reject(lambda:stage_native.export(store,'integration','share/notes',root/'notes'))
    reject(lambda:stage_native.export(store,'integration','bin/bad',root/'bad'))
    reject(lambda:stage_native.export(store,'missing','bin/demo',root/'missing'))
    # The verified export is accepted by the same host EXT4 image builder.
    image=root/'stage.ext4'
    subprocess.run(['build/mkvolume',str(image),'build/hello.elf','build/sdk-demo.elf',str(target),'build/overlord-demo.elf','build/gui-demo.elf','build/resource-scan.elf'],check=True)
    assert image.read_bytes()==Path('build/vault.ext4').read_bytes()
print('PASS: signed SWP -> verified native export -> reproducible EXT4 image, rejection paths')
