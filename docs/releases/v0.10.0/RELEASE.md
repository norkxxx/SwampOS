<div align="center">
<img src="assets/banner.svg" alt="SwampOS v0.10.0 Ecosystem" width="100%" />

# SwampOS v0.10.0 — Ecosystem

**Security has layers. Build your own swamp.**

`EXPERIMENTAL PRE-RELEASE` · `x86_64` · `HOST-TESTED`
</div>

## Highlights
- SwampPKG signed `.swp` host staging: Ed25519, per-file SHA-256, exact dependencies, atomic activation and snapshot rollback.
- Structural ELF64, PE32/PE32+, DEB and RPM metadata inspection; no foreign execution.
- Native C SDK and separately compiled `/bin/sdk-demo` on the existing public EXT4 volume.
- Kernel audit ring and new `audit` / `audit-test` commands.
- Shared software renderer, static UEFI GOP graphics lab, host-generated preview and an additional guest test gate.

## Evidence
Two clean builds and checksum comparison, complete host regression suite on the first build, repeated ecosystem tests on the second build. See TEST-REPORT.md for exact outcomes. Runtime gates remain separate; no guest success implied.

## Security boundaries
The SWP store is host-only, does not execute scripts or grant permissions. Foreign binaries are not supported for execution. The GUI is a separate unsigned firmware lab, not a desktop or login environment. SwampLogin remains locked, never collects credentials and never releases keys. No production disk encryption, no verified Secure Boot chain or real VPN backend yet. Use a disposable VM and trusted test binaries only.

## Quick start
```sh
(cd build && sha256sum -c SHA256SUMS)
./install.sh
make clean
make -j4 all
make test
make runtime
make gui-lab-runtime
```

The full archive includes source, images, local art, host-rendered preview, documentation, tests, logs and checksums. Earlier features and five target edition profiles remain in the project.

**SWAMP MATRIX // SWAMP OS**  
`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`
