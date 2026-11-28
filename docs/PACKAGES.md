# SwampPKG v1 — host staging format

`.swp` is a bounded ZIP_STORED container with exactly `manifest.json`, `signature.ed25519` and `payload/<path>` entries. Manifest is canonical ASCII JSON, signed with Ed25519 over `SwampPKG manifest v1\0` followed by the exact bytes. It identifies a package, exact numeric version, exact dependencies, requested permissions and SHA-256/size for each file. No compression, extra members, symlinks, path traversal, duplicate members or scripts. Max archive 16 MiB, 128 files. There is no package repository or automatic trust discovery; choose the trusted public key out of band. Console permission is metadata, not a grant. No key is bundled.

## Host usage (Linux/WSL)

```sh
python3 tools/swp.py keygen publisher-private.pem publisher-public.pem
mkdir -p demo-payload/bin
cp build/sdk-demo.elf demo-payload/bin/demo
python3 tools/swp.py build sdk-demo 1.0.0 demo-payload publisher-private.pem sdk-demo.swp --console
python3 tools/swp.py verify sdk-demo.swp publisher-public.pem
python3 tools/swp.py install ./local-store publisher-public.pem sdk-demo.swp
python3 tools/swp.py list ./local-store publisher-public.pem
python3 tools/swp.py read ./local-store publisher-public.pem sdk-demo bin/demo exported.elf
python3 tools/swp.py remove ./local-store publisher-public.pem sdk-demo
python3 tools/swp.py rollback ./local-store publisher-public.pem
```

Exact dependency: `--depends base=1.0.0`; pass all required packages to one install transaction. The store holds SHA-256-addressed archives, not extracted trees. A file lock serializes changes. State switches atomically via fsync + rename; dependency closure and cycles are checked before commit. The previous active mapping is retained for one-step rollback/redo. Unreferenced archives are retained (no GC). A lower version is rejected during installation; explicit rollback is allowed. No anti-rollback security against someone controlling the host directory, no key rotation/revocation, transparency, repository or SBOM pipeline yet. Store must be under a trusted, single-user host path. Root and object directories must not be symlinks or group/world writable. Not a hardened multi-user service. No guest filesystem writes, script execution or native package integration.

Foreign-format tooling: `python3 tools/compat_inspect.py FILE` reports bounded structural metadata for little-endian ELF64, PE32/PE32+, DEB ar members and RPM header sizes. It does not validate package signatures, inspect compressed payloads, resolve Linux/Windows imports or execute binaries. `native_profile_candidate` is structural only; it is not a trust or safety judgment.
