# THE SWAMP AWAKENS · Build 38

**SWAMP MATRIX // 0.20.0-dev.38**  
`ITS YOUR SWAMP - ITS YOUR LIFE`

Boot the hybrid UEFI ISO to use the ordinary RAM-backed desktop. Launch the storage profile to mount a disposable disk and preserve `/persist/note` across boots. Both native transports passed cold-boot, torn-record and write-denial checks in QEMU/OVMF. The cumulative ZIP contains the sources, historical logs, ISO and lab images.

## Build 38 · Persistent storage

SwampKernel can preserve a note across guest shutdowns on an emulated AHCI disk or NVMe namespace. Mount the disposable volume explicitly, then use the same shell path with either controller. The Live root stays in RAM.

```sh
make live-storage-desktop                         # Linux/QEMU GTK, NVMe
make live-storage-desktop STORAGE_CONTROLLER=ahci # independent AHCI image
make live-storage-runtime                         # headless persistence and recovery tests
```

In SwampOS, enter `persist-mount`, `write /persist/note hello swamp`, then `cat /persist/note`. Close QEMU, repeat the same launch command, mount again and read the file. Each interactive profile keeps its own 4 MiB image under `build/`. [Storage format and limits](docs/PERSISTENCE.md)

## Commit from the integrated checkout

```sh
git switch -c build/38-persistent-storage
git add -u
git add -- kernel tests docs/PERSISTENCE.md
python3 scripts/check-git-index.py && git commit -m "storage: persist a guest file through AHCI and NVMe"
```
