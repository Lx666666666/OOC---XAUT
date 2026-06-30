$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$Compiler = "D:\MinGW-64\mingw64\bin\g++.exe"
$EasyXRoot = "C:\Users\lx\Downloads\easyx4mingw_25.9.10"
$Output = Join-Path $ProjectRoot "Project2.exe"
$Source = Join-Path $ProjectRoot "src\main.cpp"

if (!(Test-Path $Compiler)) {
    throw "Cannot find g++ at $Compiler"
}
if (!(Test-Path $EasyXRoot)) {
    throw "Cannot find EasyX4MinGW at $EasyXRoot"
}
if (Test-Path $Output) {
    Remove-Item -LiteralPath $Output -Force
}

& $Compiler $Source `
    -std=c++17 -O2 -Wall -Wextra -Wno-unknown-pragmas -Wno-deprecated-declarations `
    "-I$EasyXRoot\include" `
    "-L$EasyXRoot\lib64" `
    -leasyxw -lgdi32 -lole32 -lwinmm -lmsimg32 `
    -o $Output

if ($LASTEXITCODE -ne 0) {
    throw "g++ build failed with exit code $LASTEXITCODE"
}

New-Item -ItemType Directory -Force -Path (Join-Path $ProjectRoot "data") | Out-Null
Write-Host "Build OK: $Output"
