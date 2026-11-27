from pathlib import Path
b=Path('build/boot.bin').read_bytes(); k=Path('build/kernel.bin').read_bytes()
assert len(b)==512 and b[-2:]==b'\x55\xaa'
assert 0<len(k)<=32768
Path('build/SwampOS.img').write_bytes((b+k.ljust(32768,b'\0')).ljust(1024*1024,b'\0'))
