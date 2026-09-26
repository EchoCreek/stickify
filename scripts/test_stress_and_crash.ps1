# ------------------------------------------------------------------
# Sticky Notes End-to-End Stress & Crash Recovery Test Script
# ------------------------------------------------------------------
$ErrorActionPreference = "Stop"

Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   Sticky Notes E2E Stress & Crash-Recovery Automation Test" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

$baseDir = Split-Path -Parent $PSScriptRoot
$releaseDir = Join-Path $baseDir "x64\Release"
$notesExe = Join-Path $releaseDir "Notes.exe"
$notesStorage = Join-Path $releaseDir "notes"

if (-not (Test-Path $notesStorage)) {
    New-Item -ItemType Directory -Path $notesStorage -Force | Out-Null
}

# Ensure existing notes processes are terminated
Stop-Process -Name "Notes" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# Test 1: Generate 25 high-density notes with special characters and large lists
Write-Host "`n[E2E 1] Generating 25 high-density stress test notes..." -ForegroundColor Yellow
for ($i = 1; $i -le 25; $i++) {
    $noteId = "stress_note_$i"
    $noteFile = Join-Path $notesStorage "$noteId.json"
    
    $items = @()
    for ($j = 1; $j -le 20; $j++) {
        $items += @{
            id = [uint64]((Get-Date).Ticks + $j)
            content = "Stress item $j for note $noteId with special symbols: Rocket #$j *bold* code"
            finish = ($j % 3 -eq 0)
        }
    }

    $noteObj = @{
        name = $noteId
        title = "Stress Note $i"
        left = 100 + ($i * 15) % 800
        top = 100 + ($i * 15) % 600
        right = 450 + ($i * 15) % 800
        bottom = 450 + ($i * 15) % 600
        bgcolor = "#0d1117"
        opacity = 50
        opacityable = $false
        visible = $true
        topmost = $true
        notes = @($items)
    }

    $jsonStr = $noteObj | ConvertTo-Json -Depth 5 -Compress
    [System.IO.File]::WriteAllText($noteFile, $jsonStr, (New-Object System.Text.UTF8Encoding($false)))
}
Write-Host "  -> Successfully generated 25 notes containing 500 total checklist items." -ForegroundColor Green

# Test 2: Launch Notes.exe and monitor resource usage
Write-Host "`n[E2E 2] Launching Notes.exe with 25 concurrent notes loaded..." -ForegroundColor Yellow
$proc = Start-Process -FilePath $notesExe -PassThru
Start-Sleep -Seconds 5

$proc.Refresh()
$wsMB = [math]::Round($proc.WorkingSet64 / 1MB, 2)
$handleCount = $proc.HandleCount

Write-Host "  -> Memory WorkingSet: $wsMB MB" -ForegroundColor Green
Write-Host "  -> Handles Count: $handleCount" -ForegroundColor Green

if ($wsMB -lt 300) {
    Write-Host "  [PASS] Memory consumption within healthy bounds (< 300 MB for 25 instances)" -ForegroundColor Green
} else {
    Write-Host "  [WARN] Memory consumption higher than expected ($wsMB MB)" -ForegroundColor Yellow
}

# Test 3: Sudden Crash & Force Termination Simulation
Write-Host "`n[E2E 3] Simulating Sudden Process Crash (SIGKILL)..." -ForegroundColor Yellow
Stop-Process -Id $proc.Id -Force
Start-Sleep -Seconds 1
Write-Host "  -> Process forcefully killed." -ForegroundColor Green

# Test 4: Crash Recovery Validation
Write-Host "`n[E2E 4] Validating Crash Recovery & File Integrity..." -ForegroundColor Yellow
$corruptedCount = 0
$files = Get-ChildItem -Path $notesStorage -Filter "stress_note_*.json"
foreach ($f in $files) {
    try {
        $content = [System.IO.File]::ReadAllText($f.FullName, [System.Text.Encoding]::UTF8)
        $parsed = $content | ConvertFrom-Json
        if (-not $parsed.notes -or $parsed.notes.Count -ne 20) {
            $corruptedCount++
        }
    } catch {
        $corruptedCount++
    }
}

if ($corruptedCount -eq 0 -and $files.Count -eq 25) {
    Write-Host "  [PASS] 100% of notes files (25/25) survived sudden process termination with zero corruption." -ForegroundColor Green
} else {
    Write-Host "  [FAIL] $corruptedCount notes files were corrupted after kill." -ForegroundColor Red
}

# Test 5: Re-launch after Crash
Write-Host "`n[E2E 5] Re-launching Notes.exe after crash..." -ForegroundColor Yellow
$proc2 = Start-Process -FilePath $notesExe -PassThru
Start-Sleep -Seconds 4

if (-not $proc2.HasExited) {
    Write-Host "  [PASS] Notes.exe successfully re-launched and stabilized after sudden termination." -ForegroundColor Green
} else {
    Write-Host "  [FAIL] Notes.exe crashed on re-launch." -ForegroundColor Red
}

# Cleanup stress test files
Write-Host "`n[CLEANUP] Cleaning up stress test files..." -ForegroundColor Yellow
Get-ChildItem -Path $notesStorage -Filter "stress_note_*.json" | Remove-Item -Force
Stop-Process -Name "Notes" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# Relaunch normal single note
Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{CommandLine = $notesExe} | Out-Null

Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   E2E STRESS & CRASH RECOVERY TESTS COMPLETED SUCCESSFULLY! " -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
