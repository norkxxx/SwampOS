#!/usr/bin/env python3
"""Create a disposable GPT disk containing a public plaintext demo EXT4 volume.
Never writes a device or existing path. Not a SwampVault/production image.
"""
import os,sys,struct,uuid
from pathlib import Path
from gptdisk import header,crc,S,M,N,ES
if len(sys.argv)!=3:raise SystemExit('usage: mkstoragegpt.py OUTPUT EXT4')
output,volume=map(Path,sys.argv[1:]);data=volume.read_bytes()
if len(data)!=8*M:raise SystemExit('expected 8 MiB demo volume')
first=2048;last=first+len(data)//S-1;total=last+1+2048
entries=bytearray(N*ES);typ=uuid.UUID('0fc63daf-8483-4772-8e79-3d69d8477de4');ident=uuid.UUID('4f47444f-0000-4000-8000-5357414d5008')
struct.pack_into('<16s16sQQQ72s',entries,0,typ.bytes_le,ident.bytes_le,first,last,0,'SWAMP-LAB-DATA'.encode('utf-16le'))
mbr=bytearray(S);struct.pack_into('<B3sB3sII',mbr,446,0,b'\0\x02\0',0xee,b'\xff\xff\xff',1,total-1);mbr[510:]=b'\x55\xaa'
fd=os.open(output,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
try:
 with os.fdopen(fd,'wb') as f:
  f.truncate(total*S);f.write(mbr);f.write(header(1,total-1,34,total-34,2,crc(entries)));f.write(entries)
  f.seek(first*S);f.write(data);f.seek((total-33)*S);f.write(entries);f.write(header(total-1,1,34,total-34,total-33,crc(entries)))
except BaseException:
 output.unlink();raise
