param([string]$Csc = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\csc.exe")
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
& $Csc /nologo /codepage:65001 /platform:x64 /optimize+ /r:System.Web.Extensions.dll /target:winexe "/out:$out\Team Swap Installer.exe" "$root\src\installer\Installer.cs"
if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
Write-Output 'Built installer in build. TeamSwap.patch is intentionally not included in this source repository.'
