"""Use a private variable store for each OVMF guest."""
from pathlib import Path
import os
import shutil


def drives(name, *, explicit_secure_boot=False):
    override = os.environ.get("SWAMP_OVMF_DIR")
    directories = [override] if override else ["/usr/share/OVMF", "/usr/share/edk2/ovmf"]
    # Live requires an explicit SecureBoot=0. A firmware build without the
    # variable cannot attest that state. Use a capable CODE and a blank VARS.
    pairs = (("OVMF_CODE_4M.secboot.fd", "OVMF_VARS_4M.fd"),
             ("OVMF_CODE_4M.fd", "OVMF_VARS_4M.fd"),
             ("OVMF_CODE.secboot.fd", "OVMF_VARS.fd"),
             ("OVMF_CODE.fd", "OVMF_VARS.fd"))
    if not explicit_secure_boot:
        pairs = tuple(pair for pair in pairs if ".secboot." not in pair[0])
    for directory in directories:
        for code, template in pairs:
            firmware, variables = Path(directory, code), Path(directory, template)
            if firmware.is_file() and variables.is_file():
                dest = Path("build", name)
                shutil.copyfile(variables, dest)
                return ["-drive", f"if=pflash,unit=0,format=raw,readonly=on,file={firmware}",
                        "-drive", f"if=pflash,unit=1,format=raw,file={dest}"]
    raise SystemExit("NOT RUN: matching OVMF CODE/VARS pair missing")
