#!/bin/sh
# Linux only. Explicit USB target, refuses mounted/non-removable disks.
set -eu
[ "$#" -eq 1 ] || { echo "Usage: sudo $0 /dev/sdX (whole removable USB disk)" >&2; exit 2; }
[ "$(id -u)" -eq 0 ] || { echo 'Run as root, only after identifying the USB drive.' >&2; exit 2; }
cd "$(dirname "$0")/.."
image="$PWD/build/Farkuad-Live.iso"
(cd build && sha256sum -c Farkuad-Live.iso.sha256)
dev=$(realpath -- "$1")
case "$dev" in /dev/sd[a-z]|/dev/sd[a-z][a-z]) ;; *) echo 'Only a whole /dev/sdX USB disk is accepted.' >&2; exit 2;; esac
check_target() {
 [ -b "$dev" ] && [ "$(lsblk -dnro TYPE "$dev")" = disk ] && [ "$(lsblk -dnro TRAN "$dev")" = usb ] && [ "$(lsblk -dnro RM "$dev")" = 1 ] || { echo 'Not a removable USB disk; refusing.' >&2; exit 2; }
 [ -z "$(lsblk -nr -o MOUNTPOINTS "$dev" | tr -d '[:space:]')" ] || { echo 'Mounted filesystem or swap found; unmount manually first.' >&2; exit 2; }
 [ "$(blockdev --getsize64 "$dev")" -ge "$(stat -c %s "$image")" ] || { echo 'USB disk too small.' >&2; exit 2; }
}
check_target
lsblk -o NAME,SIZE,MODEL,SERIAL,TRAN,RM,MOUNTPOINTS "$dev"
printf 'ALL DATA ON %s WILL BE OVERWRITTEN. Type exactly ERASE %s: ' "$dev" "$dev"
IFS= read -r answer
[ "$answer" = "ERASE $dev" ] || { echo 'Cancelled.'; exit 2; }
check_target
dd if="$image" of="$dev" bs=4M conv=fsync status=progress
sync
cmp -n "$(stat -c %s "$image")" "$image" "$dev"
echo 'Image read-back matches. Safely eject the USB drive.'
