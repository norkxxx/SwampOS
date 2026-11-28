#!/usr/bin/env python3
"""Bounded format inspection; never extract, install or execute foreign code."""
import argparse
import json
import struct
from pathlib import Path
LIMIT=16*1024*1024

def need(ok,msg):
    if not ok: raise ValueError(msg)

def inspect(data):
    n=len(data); need(4<=n<=LIMIT,'size')
    if data.startswith(b'\x7fELF'):
        need(n>=64 and data[4:7]==b'\x02\x01\x01','only little endian ELF64')
        kind,machine,version,entry,off,_,flags,eh,phsize,count=struct.unpack_from('<HHIQQQIHHH',data,16)
        need(version==1 and eh==64 and phsize==56 and 0<count<=128 and off>=64 and off<=n and count*56<=n-off,'ELF header')
        loads=[]; interp=False
        for i in range(count):
            t,f,p,v,pa,fs,ms,align=struct.unpack_from('<IIQQQQQQ',data,off+i*56)
            if t==3: interp=True
            if t==1:
                need(p<=n and fs<=n-p and fs<=ms,'ELF segment bounds');loads.append((f,p,v,pa,fs,ms,align))
        native=False
        if len(loads)==count and count in (1,2):
            f,p,v,pa,fs,ms,align=loads[0]
            native=(kind==2 and machine==62 and not flags and data[7:9]==b'\0\0' and f==5 and p>=off+56*count and p%4096==0 and v==pa==0x40000000 and 0<fs==ms<=4096 and align==4096 and v<=entry<v+fs)
            if native and count==2:
                df,dp,dv,dpa,ds,dm,da=loads[1]
                native=(df==6 and dp>=off+112 and dp%4096==0 and dv==dpa==0x40004000 and 0<dm<=4096 and ds<=dm and da==4096 and (not ds or dp>=p+fs or p>=dp+ds))
        return {'format':'ELF64','machine':machine,'entry':entry,'load_segments':len(loads),'interpreter':interp,'native_profile_candidate':native}
    if data.startswith(b'MZ'):
        need(n>=64,'DOS header'); off=struct.unpack_from('<I',data,60)[0]
        need(64<=off<=n-24 and data[off:off+4]==b'PE\0\0','PE signature')
        machine,count,_,_,_,size,flags=struct.unpack_from('<HHIIIHH',data,off+4)
        start=off+24; need(0<count<=96 and size>=2 and start+size<=n and count*40<=n-start-size,'PE headers')
        magic=struct.unpack_from('<H',data,start)[0]; need(magic in (0x10b,0x20b) and size>=(112 if magic==0x20b else 96),'optional header')
        for i in range(count):
            raw,ptr=struct.unpack_from('<II',data,start+size+i*40+16); need(not raw or (ptr<=n and raw<=n-ptr),'PE section bounds')
        return {'format':'PE32+' if magic==0x20b else 'PE32','machine':machine,'sections':count,'dll':bool(flags&0x2000),'native_profile_candidate':False}
    if data.startswith(b'!<arch>\n'):
        p=8; names=[]
        while p<n:
            need(len(names)<32 and n-p>=60 and data[p+58:p+60]==b'`\n','ar member')
            name=data[p:p+16].decode('ascii').strip().removesuffix('/'); size=data[p+48:p+58].strip();need(size.isdigit(),'ar size'); size=int(size); p+=60; need(size<=n-p,'ar bounds')
            need(name not in names and name not in ('','/'),'ar names'); names.append(name)
            if name=='debian-binary': need(data[p:p+size]==b'2.0\n','DEB version')
            p+=size
            if size%2: need(p<n and data[p:p+1]==b'\n','ar padding');p+=1
        need(len(names)>=3 and names[0]=='debian-binary' and names[1] in ('control.tar','control.tar.gz','control.tar.xz','control.tar.zst','control.tar.bz2') and names[2] in ('data.tar','data.tar.gz','data.tar.xz','data.tar.zst','data.tar.bz2'),'DEB members')
        return {'format':'DEB','members':names,'native_profile_candidate':False}
    if data.startswith(b'\xed\xab\xee\xdb'):
        need(n>=96 and data[4]==3,'RPM lead'); p=96; counts=[]
        for idx in range(2):
            need(p+16<=n and data[p:p+4]==b'\x8e\xad\xe8\x01','RPM header')
            count,size=struct.unpack_from('>II',data,p+8);need(count<=65536 and count*16+size<=n-p-16,'RPM bounds');p+=16+count*16+size;counts.append(count)
            if idx==0:p=(p+7)&~7
        return {'format':'RPM','header_entries':counts,'payload_bytes':n-p,'native_profile_candidate':False}
    raise ValueError('unknown format')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('file',type=Path);a=p.parse_args()
    with a.file.open('rb') as f: data=f.read(LIMIT+1)
    print(json.dumps(inspect(data),indent=2))
if __name__=='__main__':
    try: main()
    except (ValueError,OSError,UnicodeError,struct.error) as e: raise SystemExit(f'rejected: {e}')
