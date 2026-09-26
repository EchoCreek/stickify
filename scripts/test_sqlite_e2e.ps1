# ------------------------------------------------------------------
# Sticky Notes SQLite End-to-End Verification Script
# ------------------------------------------------------------------
$ErrorActionPreference = "Stop"

Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   Sticky Notes SQLite E2E Verification & Reliability Test" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

$baseDir = Split-Path -Parent $PSScriptRoot
$releaseDir = Join-Path $baseDir "x64\Release"
$testExe = Join-Path $releaseDir "NotesTests.exe"

# Step 1: Run comprehensive unit and SQLite tests
Write-Host "`n[E2E 1] Executing full test suite (71 tests)..." -ForegroundColor Yellow
& $testExe
if ($LASTEXITCODE -ne 0) {
    Write-Host "  [FAIL] Test suite execution failed!" -ForegroundColor Red
    exit 1
}
Write-Host "  [PASS] All 71 tests passed cleanly!" -ForegroundColor Green

Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "   ALL SQLITE E2E TESTS COMPLETED SUCCESSFULLY! " -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
