[CmdletBinding()]
param(
  [string]$GradleExecutable = ".\gradlew.bat",
  [string]$Serial,
  [string[]]$TestClasses = @(
    "androidx.media3.exoplayer.cppbridge.CppBridgeNativeSmokeTest",
    "androidx.media3.exoplayer.cppbridge.CppBridgeNativePlayerInstrumentationTest"
  ),
  [switch]$SkipBuild,
  [switch]$LaunchDemo
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

$devicesOutput = & adb devices
if ($LASTEXITCODE -ne 0) {
  throw "adb devices failed"
}
if (($devicesOutput | Select-String "device$" | Measure-Object).Count -lt 1) {
  throw "No online adb device was found. Connect a device or start an emulator first."
}

if (-not $SkipBuild) {
  Invoke-CheckedCommand -FilePath $GradleExecutable -Arguments @(
    ":lib-exoplayer-cppbridge:assembleDebugAndroidTest",
    ":demo-cppbridge:installDebug"
  )
}

foreach ($testClass in $TestClasses) {
  Invoke-CheckedCommand -FilePath $GradleExecutable -Arguments @(
    ":lib-exoplayer-cppbridge:connectedDebugAndroidTest",
    "-Pandroid.testInstrumentationRunnerArguments.class=$testClass"
  )
}

if ($LaunchDemo) {
  Invoke-CheckedCommand -FilePath "adb" -Arguments ($adbArgs + @(
      "shell",
      "am",
      "start",
      "-n",
      "androidx.media3.demo.cppbridge/.MainActivity"
    ))
}

Write-Host ""
Write-Host "Validation completed."
