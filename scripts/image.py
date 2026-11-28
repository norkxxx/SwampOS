from pathlib import Path
from boot_layout import layout
c=layout();b=Path('build/boot.bin').read_bytes();k=Path('build/kernel.bin').read_bytes()
assert len(b)==c['SECTOR_BYTES'] and b[-2:]==b'\x55\xaa'
assert 0<len(k)<=c['PAYLOAD_BYTES']
Path('build/SwampOS.img').write_bytes((b+k.ljust(c['PAYLOAD_BYTES'],b'\0')).ljust(c['IMAGE_BYTES'],b'\0'))
