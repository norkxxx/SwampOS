<div align="center">
<img src="assets/banner.svg" alt="SwampOS v0.11.0 Integration" width="100%" />

# SwampOS v0.11.0 — Integration

**Security has layers. Connect the layers.**

`EXPERIMENTAL PRE-RELEASE` · `x86_64` · `HOST-TESTED`
</div>

## Highlights
- Five-call native ABI with ephemeral run IDs and explicitly granted read-only data snapshots.
- Kernel session cleanup, stack-only output validation, and default-deny resource access.
- Verified SWP store export into a native ELF staging pipeline; separate C application on the EXT4 fixture.
- Git LFS disk-image rules, Bash and PowerShell setup, image-only checksums and CI integration.
- All previous components, five target profiles, local banners and documentation retained.

## Validation
See [TEST-REPORT.md](TEST-REPORT.md) for actual test results. Guest runtime and remote LFS transport remain unverified. No full process manager, IPC, native package installation, disk authentication, full desktop, or VPN backend claimed.

## Run the development image
```sh
(cd build && sha256sum -c SHA256SUMS)
./install.sh
make clean
make -j4 all
make test
make runtime
```

**SWAMP MATRIX // SWAMP OS**  
`YOUR SYSTEM. YOUR RULES. YOUR SWAMP.`
