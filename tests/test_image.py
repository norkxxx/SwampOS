from pathlib import Path
import subprocess, struct
b=Path('build/SwampOS.img').read_bytes();k=Path('build/kernel.bin').read_bytes();e=Path('build/kernel.elf').read_bytes()
assert len(b)==1048576 and b[510:512]==b'\x55\xaa'
assert 0<len(k)<=32768 and b[512:512+len(k)]==k and not any(b[512+len(k):])
assert e[:6]==b'\x7fELF\x02\x01'
assert struct.unpack_from('<H',e,18)[0]==62 and struct.unpack_from('<Q',e,24)[0]==0x100000
phoff=struct.unpack_from('<Q',e,32)[0];entsize,num=struct.unpack_from('<HH',e,54)
loads=[]
for i in range(num):
    p=struct.unpack_from('<IIQQQQQQ',e,phoff+i*entsize)
    if p[0]==1:
        assert p[1] in (4,5,6) and p[3]%4096==0
        loads.append(p[1])
assert loads==[5,4,6]
assert not subprocess.check_output(['nm','-u','build/kernel.elf']).strip()
for marker in (b'BOOT_OK',b'USER_TEST_OK',b'VM_TEST_OK',b'CORE_TEST_OK'):assert marker in k
print('PASS: image geometry, payload, ELF64 entry, RX/R/RW segments, no unresolved symbols, marker presence (static only)')
