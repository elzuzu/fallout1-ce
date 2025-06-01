param(
    [string]$SourceDir = (Get-Item ".").FullName,
    [string]$BuildDir,
    [switch]$Release
)

if (-not $BuildDir) {
    $BuildDir = Join-Path $SourceDir 'build'
}

function Ensure-ChocoPackage($name) {
    if (-not (choco list --local-only $name | Select-String "^$name")) {
        Write-Host "Installing $name via Chocolatey" -ForegroundColor Cyan
        choco install $name -y --no-progress
    }
    else {
        Write-Host "$name already installed" -ForegroundColor Green
    }
}

if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Error 'Chocolatey is required. Install it from https://chocolatey.org/install'
    exit 1
}

$required = @(
    'visualstudio2022buildtools',
    'cmake',
    'ninja',
    'git'
)

foreach ($pkg in $required) { Ensure-ChocoPackage $pkg }

$conf = if ($Release) { 'Release' } else { 'Debug' }

if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory $BuildDir | Out-Null }

$vswhere = Join-Path "${Env:ProgramFiles(x86)}" 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -requires Microsoft.Component.MSBuild -property installationPath
if (-not $vsPath) {
    Write-Error 'Visual Studio Build Tools not found.'
    exit 1
}
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'

$cmakeArgs = "-S `"$SourceDir`" -B `"$BuildDir`" -G `"Visual Studio 17 2022`" -A x64 -DCMAKE_BUILD_TYPE=$conf"

cmd /c "`"$vcvars`" && cmake $cmakeArgs && cmake --build `"$BuildDir`" --config $conf --parallel" 
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Build completed in $conf mode." -ForegroundColor Green

