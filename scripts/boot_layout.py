"""Read and validate the assembly constants used by the legacy boot image."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]

def layout():
    constants={k:int(v,0) for k,v in re.findall(r'^\.equ (BIOS_[A-Z_]+), (0x[0-9a-fA-F]+|[0-9]+)$',(ROOT/'boot/layout.inc').read_text(),re.M)}
    keys=('SECTOR_BYTES','READ_SECTORS','BUFFER_START','BUFFER_END','STACK','MIN_CONVENTIONAL_KIB','KERNEL_DEST','IMAGE_BYTES')
    c={key:constants['BIOS_'+key] for key in keys}
    sector=c['SECTOR_BYTES'];chunk=c['READ_SECTORS']*sector
    first=c['BUFFER_START'];end=c['BUFFER_END'];size=end-first
    assert sector==512 and 0<c['READ_SECTORS']<=127
    assert first>=0x10000 and size>0 and size%chunk==0
    assert end<c['STACK']<c['MIN_CONVENTIONAL_KIB']*1024<=0xa0000
    assert c['KERNEL_DEST']==0x100000 and c['IMAGE_BYTES']>=size+sector
    for address in range(first,end,chunk):
        assert address%16==0 and address//65536==(address+chunk-1)//65536
    c['PAYLOAD_BYTES']=size
    return c
