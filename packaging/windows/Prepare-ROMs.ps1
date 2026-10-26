# Prepare-ROMs.ps1 - build a runtime-ready Garou ROM set from your own dumps.
#
# This script extracts the MAME garou and neogeo sets and runs the bundled
# decrypt tool. It does not download anything and does not contain ROM data.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1 -GarouZip C:\path\garou.zip -NeoGeoZip C:\path\neogeo.zip

param(
    [string]$GarouZip = "",
    [string]$NeoGeoZip = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $OutDir) { $OutDir = Join-Path $here "roms" }
$src = Join-Path $here "roms-src"

function Ask-Zip($label, $current) {
    if ($current -and (Test-Path $current)) { return (Resolve-Path $current).Path }
    while ($true) {
        $p = Read-Host "Path to $label"
        $p = $p.Trim('"').Trim()
        if (Test-Path $p) { return (Resolve-Path $p).Path }
        Write-Host "Not found: $p" -ForegroundColor Yellow
    }
}

$GarouZip = Ask-Zip "garou.zip" $GarouZip
$NeoGeoZip = Ask-Zip "neogeo.zip" $NeoGeoZip

if (-not (Test-Path (Join-Path $here "garou_decrypt.exe"))) {
    throw "garou_decrypt.exe is missing from $here"
}

New-Item -ItemType Directory -Force $src | Out-Null
New-Item -ItemType Directory -Force $OutDir | Out-Null

Write-Host "Extracting $GarouZip"
tar -xf $GarouZip -C $src
if ($LASTEXITCODE -ne 0) { throw "Could not extract garou.zip with tar" }
Write-Host "Extracting $NeoGeoZip"
tar -xf $NeoGeoZip -C $src
if ($LASTEXITCODE -ne 0) { throw "Could not extract neogeo.zip with tar" }

Write-Host "Decrypting into $OutDir"
& (Join-Path $here "garou_decrypt.exe") --rom-path $src --out $OutDir
if ($LASTEXITCODE -ne 0) { throw "garou_decrypt failed" }

Write-Host ""
Write-Host "Done. Run:" -ForegroundColor Green
Write-Host "  .\Garou.exe --rom-path `"$OutDir`""
