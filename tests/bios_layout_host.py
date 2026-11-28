"""Static check of the actual DAP in the linked boot sector, plus the transfer geometry."""
import sys,struct,subprocess
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from boot_layout import layout
c=layout();b=Path('build/boot.bin').read_bytes()
assert len(b)==512 and b[510:]==b'\x55\xaa'
expected=struct.pack('<BBHHHQ',16,0,c['READ_SECTORS'],0,c['BUFFER_START']//16,1)
assert b.count(expected)==1 and b.index(expected)%4==0
chunk=c['READ_SECTORS']*512;transfers=c['PAYLOAD_BYTES']//chunk
assert transfers==8 and c['PAYLOAD_BYTES']==256*1024
for i in range(transfers):
    start=c['BUFFER_START']+i*chunk;first_lba=1+i*c['READ_SECTORS']
    assert start%65536 in (0,32768)
    assert first_lba+c['READ_SECTORS']<=1+c['PAYLOAD_BYTES']//512
assert c['BUFFER_END']<c['STACK']
print('PASS: linked BIOS DAP, eight contiguous 32 KiB transfers, 256 KiB payload geometry (static only)')
