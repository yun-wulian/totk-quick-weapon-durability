param([string]$Zig = 'zig')
$ErrorActionPreference = 'Stop'
$outputDirectory = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$testPath = Join-Path $outputDirectory 'durability-test.exe'
& $Zig c++ -target x86_64-windows-gnu -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I (Join-Path $PSScriptRoot 'source\program') (Join-Path $PSScriptRoot 'tests\durability.cpp') -o $testPath
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
& $testPath
if ($LASTEXITCODE -ne 0) { throw 'Durability behavior tests failed' }
