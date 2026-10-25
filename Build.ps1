param(
    [string]$Python = 'python',
    [string]$Zig = 'zig',
    [string]$GameExeFs
)
$ErrorActionPreference = 'Stop'
& $Python -B (Join-Path $PSScriptRoot 'tools\build.py') --zig $Zig
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
$verifyArguments = @('-B', (Join-Path $PSScriptRoot 'tools\verify.py'))
if ($GameExeFs) { $verifyArguments += @('--game-exefs', $GameExeFs) }
& $Python @verifyArguments
if ($LASTEXITCODE -ne 0) { throw 'Binary verification failed' }
