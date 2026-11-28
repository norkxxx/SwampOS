#!/bin/sh
# Build only; does not access physical disks.
set -eu
cd "$(dirname "$0")/.."
for cmd in make gcc as ld objcopy clang lld-link python3 sha256sum; do
 command -v "$cmd" >/dev/null || { echo "Missing $cmd; run ./install.sh on Debian/Ubuntu/WSL2" >&2; exit 1; }
done
make -j"${JOBS:-2}" live
make live-host-test
(cd build && sha256sum Farkuad-Live.iso > Farkuad-Live.iso.sha256 && sha256sum -c Farkuad-Live.iso.sha256)
printf '\nHybrid Live ISO: %s/build/Farkuad-Live.iso\n' "$(pwd)"
cat build/Farkuad-Live.iso.report
printf 'Unsigned x64 UEFI experimental image; use a disposable USB drive.\n'
