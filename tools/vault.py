#!/usr/bin/env python3
"""Offline experimental image envelope. NOT LUKS2, block encryption or preboot login."""
import argparse
import getpass
import os
import struct
from pathlib import Path
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

MAGIC = b'SWVAULT1'
MAX_IMAGE = 16 * 1024 * 1024
HEADER = struct.Struct('>8sQ12s512s')
OAEP = padding.OAEP(mgf=padding.MGF1(hashes.SHA256()), algorithm=hashes.SHA256(), label=b'SwampOS-Vault-image-v1')


def keygen(password):
    if len(password) < 12:
        raise ValueError('Passphrase must contain at least 12 bytes')
    key = rsa.generate_private_key(public_exponent=65537, key_size=4096)
    return (key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
                              serialization.BestAvailableEncryption(password)),
            key.public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))


def seal(image, public_pem):
    if not 0 < len(image) <= MAX_IMAGE:
        raise ValueError('Unsupported image size')
    public = serialization.load_pem_public_key(public_pem)
    if not isinstance(public, rsa.RSAPublicKey) or public.key_size != 4096:
        raise ValueError('RSA-4096 key required')
    key = AESGCM.generate_key(bit_length=256)
    nonce = os.urandom(12)
    header = HEADER.pack(MAGIC, len(image), nonce, public.encrypt(key, OAEP))
    return header + AESGCM(key).encrypt(nonce, image, header)


def unlock(blob, private_pem, password):
    if not password or not HEADER.size + 16 < len(blob) <= HEADER.size + MAX_IMAGE + 16:
        raise ValueError('Invalid envelope')
    magic, size, nonce, wrapped = HEADER.unpack_from(blob)
    if magic != MAGIC or not 0 < size <= MAX_IMAGE or len(blob) != HEADER.size + size + 16:
        raise ValueError('Invalid envelope header')
    key = serialization.load_pem_private_key(private_pem, password=password)
    if not isinstance(key, rsa.RSAPrivateKey) or key.key_size != 4096:
        raise ValueError('RSA-4096 key required')
    secret = key.decrypt(wrapped, OAEP)
    if len(secret) != 32:
        raise ValueError('Invalid content key')
    return AESGCM(secret).decrypt(nonce, blob[HEADER.size:], blob[:HEADER.size])


def bounded(path, limit):
    with open(path, 'rb') as f:
        data = f.read(limit + 1)
    if len(data) > limit:
        raise ValueError('Input too large')
    return data


def exclusive(path, data):
    # No overwrite, including symlinks. No output file before authentication.
    fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        with os.fdopen(fd, 'wb') as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
    except BaseException:
        os.unlink(path)
        raise


def unlock_to(blob, private, password, path):
    plaintext = unlock(blob, private, password)
    exclusive(path, plaintext)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    s = p.add_subparsers(dest='cmd', required=True)
    k = s.add_parser('keygen'); k.add_argument('private'); k.add_argument('public')
    e = s.add_parser('seal'); e.add_argument('image'); e.add_argument('public'); e.add_argument('output')
    u = s.add_parser('unlock'); u.add_argument('envelope'); u.add_argument('private'); u.add_argument('output')
    a = p.parse_args()
    try:
        if a.cmd == 'keygen':
            password = getpass.getpass('New key passphrase: ').encode()
            if password != getpass.getpass('Repeat passphrase: ').encode():
                raise ValueError('Passphrases differ')
            if Path(a.private).exists() or Path(a.public).exists() or Path(a.private).resolve() == Path(a.public).resolve():
                raise ValueError('Key paths must be distinct and new')
            private, public = keygen(password)
            exclusive(a.private, private)
            exclusive(a.public, public)
        elif a.cmd == 'seal':
            exclusive(a.output, seal(bounded(a.image, MAX_IMAGE), bounded(a.public, 16384)))
        else:
            password = getpass.getpass('Vault passphrase: ').encode()
            unlock_to(bounded(a.envelope, HEADER.size + MAX_IMAGE + 16), bounded(a.private, 16384), password, a.output)
    except Exception:
        p.exit(1, 'Vault operation failed (authentication, format, I/O or output already exists).\n')


if __name__ == '__main__':
    main()
