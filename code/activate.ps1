# Dot-source this file: . .\activate.ps1
param(
    [string]$IdfPath = (Join-Path $env:USERPROFILE 'esp\v5.5.5\esp-idf'),
    [string]$ToolsPath = (Join-Path $env:USERPROFILE '.espressif-c61')
)

$pythonEnvPath = Join-Path $ToolsPath 'python_env\idf5.5_py3.11_env'
$sdkExportPath = Join-Path $IdfPath 'export.ps1'
if (-not (Test-Path -LiteralPath $sdkExportPath) -or
    -not (Test-Path -LiteralPath (Join-Path $pythonEnvPath 'Scripts\python.exe'))) {
    throw 'ESP-IDF 5.5.5 tools were not found. See code/README.md for the expected installation paths.'
}

$env:IDF_TOOLS_PATH = $ToolsPath
$env:IDF_PYTHON_ENV_PATH = $pythonEnvPath
$env:Path = (Join-Path $pythonEnvPath 'Scripts') + ';' + $env:Path
. $sdkExportPath
