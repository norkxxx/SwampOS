from pathlib import Path
b=Path('build/SwampOS.img').read_bytes(); k=Path('build/kernel.bin').read_bytes()
assert len(b)==1048576
assert b[510:512]==b'\x55\xaa'
assert b[512:512+len(k)]==k
assert b'BOOT_OK' in k
print('PASS: image size, boot signature, kernel placement, boot marker (static checks only)')
