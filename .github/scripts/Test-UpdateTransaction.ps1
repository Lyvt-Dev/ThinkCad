$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/Invoke-Checked.ps1"
$root = Join-Path $env:RUNNER_TEMP ('ThinkCad update transaction ' + [Guid]::NewGuid().ToString())
$cache = Join-Path $root 'cache'
$target = Join-Path $root 'ThinkCad'
New-Item -ItemType Directory -Force $cache, $target | Out-Null
Copy-Item out/stage/verifier "$cache/verifier" -Recurse
Copy-Item build/ThinkCadUpdater.exe "$cache/ThinkCadUpdater.exe"
$installer = (Get-ChildItem out/installer/ThinkCad-Setup-*.exe).FullName
$version = (Select-String -Path CMakeLists.txt -Pattern 'project\(ThinkCad VERSION ([0-9.]+)').Matches.Groups[1].Value
[IO.File]::WriteAllText("$target/old-build.marker", 'old installation')
[IO.File]::WriteAllText("$target/local-user-library.txt", 'must survive update')
$parent = Start-Process cmd.exe -ArgumentList '/c exit 0' -PassThru
$args = @($parent.Id, "`"$installer`"", "`"$target`"", $env:GITHUB_SHA, $version, 'en')
$helper = Start-Process "$cache/ThinkCadUpdater.exe" -ArgumentList $args -PassThru
if (-not $helper.WaitForExit(600000)) {
  Stop-Process -Id $helper.Id -Force
  Get-Content "$cache/installation.log" -ErrorAction SilentlyContinue | Write-Host
  throw 'Update helper timed out; transaction must remain recoverable.'
}
Get-Content "$cache/installation.log" | Write-Host
if ($helper.ExitCode -ne 0) { throw "Update helper failed: $($helper.ExitCode)" }
if ((Get-Content "$target/local-user-library.txt" -Raw) -ne 'must survive update') { throw 'Local library changed.' }
if (-not (Test-Path "$target/thinkcad-update-receipt.ini")) { throw 'Installation receipt missing.' }
$backup = @(Get-ChildItem $root -Directory -Filter 'ThinkCad.thinkcad-backup-*')
if ($backup.Count -ne 1 -or -not (Test-Path "$($backup[0].FullName)/old-build.marker")) { throw 'Previous installation backup missing.' }
Get-Process ThinkCad -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1
$uninstaller = Start-Process "$target/unins000.exe" -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART' -Wait -PassThru
if ($uninstaller.ExitCode) { throw 'Transaction test uninstall failed.' }
