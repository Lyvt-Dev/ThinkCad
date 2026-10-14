function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    $output = & $Program @Arguments 2>&1 | Tee-Object -Variable commandOutput
    $code = $LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if ($code -ne 0) {
        $message = (($commandOutput | Select-Object -Last 35 | ForEach-Object { $_.ToString() }) -join "`n")
        $message = $message.Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
        Write-Host "::error title=$Program failed::$message"
        exit $code
    }
}
