# Persistent note on AHCI and NVMe

The Build 38 Live kernel drives one QEMU ICH9 AHCI disk or one QEMU NVMe namespace after ExitBootServices. Use `make live-storage-desktop` from the source root for NVMe, or add `STORAGE_CONTROLLER=ahci`. Linux, GTK-enabled QEMU and OVMF are required for that window. The shell also accepts commands on COM1. For headless regression use `make live-storage-runtime`.

| Shell command | Effect |
|---|---|
| `persist-mount` | Acquire the controller and mount an already initialized test volume |
| `ls /persist` | List the single supported file |
| `write /persist/note TEXT` | Commit a full replacement to the alternate sector |
| `cat /persist/note` | Print the selected version |
| `persist-umount` | Unmount the volume and stop the controller |

Close QEMU and relaunch the same profile to read the committed contents. Each interactive profile keeps its image at `build/storage-demo-ahci.raw` or `build/storage-demo-nvme.raw`. Copy it before `make clean` if needed. The automated suite uses a fresh temporary directory per transport.

## Controller ownership

The activation path requires the Q35 Live desktop opt-in `opt/swamp/disposable-storage=1`, a validated firmware PCI aperture for 00:05.0, one covering Intel VT-d unit, and a matching serial, capacity and on-disk label. NVMe uses namespace 1. AHCI uses port zero and checks that the other implemented ports are empty. The serial and label select a fixture under operator control. They do not authenticate a device.

One legacy VT-d domain maps five pinned pages to the storage requester. The guest uses bounded polling and supervisor-only uncached MMIO. A single command is outstanding. The AHCI path programs a command list, received-FIS area, command table and PRDT. The NVMe path creates admin and I/O queue pairs, tracks completion phase and checks command IDs. The default Live root and the original EXT4 volume stay read-only. Storage and active xHCI currently need separate VM profiles because each requires exclusive ownership of the domain and MMIO window.

## On-disk format

All sectors are 512 bytes and all numbers are little endian. LBA 0 contains `SWAMPOS-DISPOSABLE-V1` followed by zeros. LBAs 1 and 2 hold alternating revisions of one file. Each has an eight-byte magic, a nonzero 64-bit generation, a 16-bit length, a 490-byte printable ASCII field and a CRC-32 over the first 508 bytes. The shell accepts up to 127 characters in a complete command. Each commit writes the other slot, requests a device flush, and reads the complete sector back before acknowledging success. Mount selects the newest valid record. Two invalid slots or an ambiguous generation prevent mounting. CRC detects accidental corruption. Authenticated policy data needs a separate design.

The tested profile uses exactly 8,192 sectors. The driver exposes no arbitrary LBA write command. The headless tests compare every image byte after abrupt guest termination, after damage to the latest record, and after activation refusals. Physical storage, concurrent USB/storage domains, dynamic filenames, allocation and writable EXT4 remain open.
