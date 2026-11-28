#!/usr/bin/env python3
"""Offline signed+encrypted multiboot registry. Not used by firmware yet."""
import argparse, getpass, json, re, struct
from pathlib import Path
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey,Ed25519PublicKey
from vault import seal,unlock,exclusive,bounded
MAGIC=b'SWREG001';HDR=struct.Struct('>8sQI');MAX=65536

def canonical(doc):
    if type(doc) is not dict or set(doc)!= {'generation','entries'}:raise ValueError('registry schema')
    gen=doc['generation'];entries=doc['entries']
    if type(gen) is not int or not 1<=gen<2**63 or type(entries) is not list or not 1<=len(entries)<=32:raise ValueError('registry bounds')
    seen=set()
    for e in entries:
        if type(e) is not dict or set(e)!={'id','partition','path','sha256'}:raise ValueError('entry schema')
        if any(type(v) is not str for v in e.values()):raise ValueError('entry types')
        if not re.fullmatch(r'[a-z0-9_-]{1,32}',e['id']) or e['id'] in seen:raise ValueError('entry ID')
        seen.add(e['id'])
        if not re.fullmatch(r'[0-9a-f]{8}-(?:[0-9a-f]{4}-){3}[0-9a-f]{12}',e['partition']):raise ValueError('partition GUID')
        if not re.fullmatch(r'\\EFI(?:\\[A-Za-z0-9_-]{1,32}){1,5}\.efi',e['path'],re.I):raise ValueError('EFI path')
        if not re.fullmatch(r'[0-9a-f]{64}',e['sha256']):raise ValueError('loader hash')
    return json.dumps(doc,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()

def create(doc,rsa_public,signer):
    body=seal(canonical(doc),rsa_public)
    h=HDR.pack(MAGIC,doc['generation'],len(body))
    return h+body+signer.sign(h+body)

def open_registry(blob,rsa_private,password,verifier,floor):
    if type(floor) is not int or floor<1 or not HDR.size+64<len(blob)<=MAX:raise ValueError('registry size/floor')
    magic,g,n=HDR.unpack_from(blob)
    if magic!=MAGIC or g<floor or len(blob)!=HDR.size+n+64:raise ValueError('registry header or rollback')
    verifier.verify(blob[-64:],blob[:-64])
    raw=unlock(blob[HDR.size:-64],rsa_private,password)
    doc=json.loads(raw)
    if canonical(doc)!=raw or doc['generation']!=g:raise ValueError('noncanonical or generation mismatch')
    return doc

def main():
    p=argparse.ArgumentParser(description=__doc__);s=p.add_subparsers(dest='cmd',required=True)
    k=s.add_parser('keygen');k.add_argument('private');k.add_argument('public')
    c=s.add_parser('seal');c.add_argument('json');c.add_argument('rsa_public');c.add_argument('signing_private');c.add_argument('output')
    v=s.add_parser('verify');v.add_argument('registry');v.add_argument('rsa_private');v.add_argument('signing_public');v.add_argument('--floor',required=True,type=int)
    a=p.parse_args()
    try:
        if a.cmd=='keygen':
            pw=getpass.getpass('Signing passphrase: ').encode()
            if len(pw)<12 or pw!=getpass.getpass('Repeat: ').encode():raise ValueError('password')
            key=Ed25519PrivateKey.generate();exclusive(a.private,key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.BestAvailableEncryption(pw)));exclusive(a.public,key.public_key().public_bytes(serialization.Encoding.PEM,serialization.PublicFormat.SubjectPublicKeyInfo))
        elif a.cmd=='seal':
            key=serialization.load_pem_private_key(bounded(a.signing_private,16384),getpass.getpass('Signing passphrase: ').encode())
            if not isinstance(key,Ed25519PrivateKey):raise ValueError('Ed25519 key required')
            exclusive(a.output,create(json.loads(bounded(a.json,MAX)),bounded(a.rsa_public,16384),key))
        else:
            key=serialization.load_pem_public_key(bounded(a.signing_public,16384))
            if not isinstance(key,Ed25519PublicKey):raise ValueError('Ed25519 key required')
            doc=open_registry(bounded(a.registry,MAX),bounded(a.rsa_private,16384),getpass.getpass('Vault passphrase: ').encode(),key,a.floor);print(json.dumps(doc,indent=2))
    except Exception:p.exit(1,'Registry rejected (format, signature, authentication, rollback or I/O).\n')
if __name__=='__main__':main()
