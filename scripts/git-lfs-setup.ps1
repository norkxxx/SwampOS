$ErrorActionPreference = 'Stop'
function Git-Checked { & git @args; if ($LASTEXITCODE -ne 0) { throw "Git failed: $args" } }
$root = & git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0) { throw 'Not a Git repository' }
Push-Location $root
try {
    Git-Checked lfs version
    Git-Checked lfs install --local
    foreach ($pattern in @('*.img','*.iso','*.ext4','*.raw','*.qcow2','*.vmdk','*.vdi','*.rom')) { Git-Checked lfs track $pattern }
    Git-Checked check-attr filter -- build/SwampBoot-GPT.img
    Git-Checked lfs status
} finally { Pop-Location }
