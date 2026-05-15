[CmdletBinding()]
param(
  [string]$GradleExecutable = ".\gradlew.bat",
  [string]$Serial,
  [switch]$SkipInstall
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $repoRoot

if (-not [string]::IsNullOrWhiteSpace($Serial)) {
  $env:ANDROID_SERIAL = $Serial
}

function Invoke-CheckedCommand {
  param(
    [Parameter(Mandatory = $true)][string]$FilePath,
    [Parameter(Mandatory = $true)][string[]]$Arguments
  )

  Write-Host ">> $FilePath $($Arguments -join ' ')"
  & $FilePath @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
  }
}

function Get-AdbArgs {
  param([string]$DeviceSerial)
  if ([string]::IsNullOrWhiteSpace($DeviceSerial)) {
    return @()
  }
  return @("-s", $DeviceSerial)
}

$adbArgs = Get-AdbArgs -DeviceSerial $Serial

Invoke-CheckedCommand -FilePath "adb" -Arguments @("start-server")

if (-not $SkipInstall) {
  Invoke-CheckedCommand -FilePath $GradleExecutable -Arguments @(":demo-cppbridge:installDebug")
}

Invoke-CheckedCommand -FilePath "adb" -Arguments ($adbArgs + @(
    "shell",
    "am",
    "start",
    "-n",
    "androidx.media3.demo.cppbridge/.MainActivity"
  ))

Write-Host ""
Write-Host "Demo launched."
