param([Parameter(Mandatory=$true)][string]$ModDirectory)
$ErrorActionPreference = 'Stop'
$sourcePath = Join-Path $PSScriptRoot 'dist\Quick Weapon Durability\exefs\subsdk6'
$loadPath = (Resolve-Path -LiteralPath $ModDirectory).Path
$targetDirectory = Join-Path $loadPath 'Quick Weapon Durability\exefs'
$targetPath = Join-Path $targetDirectory 'subsdk6'
if (-not (Test-Path -LiteralPath $sourcePath)) { throw 'Run Build.ps1 first.' }
$collisions = Get-ChildItem -LiteralPath $loadPath -Filter 'subsdk6' -File -Recurse |
    Where-Object { $_.FullName -ne $targetPath }
if ($collisions) { throw ('Another mod occupies subsdk6: ' + ($collisions.FullName -join ', ')) }
if (Test-Path -LiteralPath $targetPath) {
    $backupPath = Join-Path $PSScriptRoot ('backups\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $backupPath -Force | Out-Null
    Copy-Item -LiteralPath $targetPath -Destination (Join-Path $backupPath 'subsdk6')
}
New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
if ((Get-FileHash -LiteralPath $sourcePath).Hash -ne (Get-FileHash -LiteralPath $targetPath).Hash) {
    throw 'Installed module hash mismatch.'
}
Write-Output "Installed: $targetPath"
