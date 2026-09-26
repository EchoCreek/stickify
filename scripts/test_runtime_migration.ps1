# ------------------------------------------------------------------
# Sticky Notes Runtime Migration & UI Launch Test Script
# ------------------------------------------------------------------
$ErrorActionPreference = "Stop"

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class Win32DialogHelper {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    public const uint WM_COMMAND = 0x0111;
    public const int IDYES = 6;

    public static bool ConfirmNotesDialog(int targetPid) {
        bool handled = false;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid) {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                string title = sb.ToString();
                if (title == "Notes") {
                    SendMessage(hWnd, WM_COMMAND, (IntPtr)IDYES, IntPtr.Zero);
                    handled = true;
                }
            }
            return true;
        }, IntPtr.Zero);
        return handled;
    }
}
"@

Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   Sticky Notes Live Runtime Migration & Verification" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan

$baseDir = Split-Path -Parent $PSScriptRoot
$releaseDir = Join-Path $baseDir "x64\Release"
$notesExe = Join-Path $releaseDir "Notes.exe"
$notesDir = Join-Path $releaseDir "notes"
$dbFile = Join-Path $notesDir "notes.db"
$backupDir = Join-Path $notesDir "json_backup"

# Ensure any existing Notes.exe is closed
Stop-Process -Name "Notes" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

Write-Host "`n[Step 1] Starting Notes.exe to trigger automatic migration..." -ForegroundColor Yellow
$proc = Start-Process -FilePath $notesExe -PassThru

# Monitor for Migration Confirmation Dialog and auto-confirm via Win32 SendMessage
for ($i = 0; $i -lt 25; $i++) {
    Start-Sleep -Milliseconds 200
    [Win32DialogHelper]::ConfirmNotesDialog($proc.Id) | Out-Null
    if (Test-Path $dbFile) {
        Write-Host "  -> Database notes.db detected!" -ForegroundColor Green
        break
    }
}

Start-Sleep -Seconds 3

Write-Host "`n[Step 2] Validating Migration Results on Disk..." -ForegroundColor Yellow

if (Test-Path $dbFile) {
    $dbSize = (Get-Item $dbFile).Length
    Write-Host "  [PASS] notes.db exists with size: $dbSize bytes" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] notes.db was NOT created!" -ForegroundColor Red
}

$remainingJsons = Get-ChildItem -Path $notesDir -Filter "*.json"
if ($remainingJsons.Count -eq 0) {
    Write-Host "  [PASS] Original JSON files cleanly cleaned up from notes/ directory" -ForegroundColor Green
} else {
    Write-Host "  [WARN] Some JSON files remained in notes/ directory: $($remainingJsons.Count)" -ForegroundColor Yellow
}

$backedUpJsons = Get-ChildItem -Path $backupDir -Filter "*.json" -ErrorAction SilentlyContinue
if ($backedUpJsons -and $backedUpJsons.Count -ge 3) {
    Write-Host "  [PASS] Found $($backedUpJsons.Count) backed-up JSON files in notes/json_backup/" -ForegroundColor Green
    foreach ($f in $backedUpJsons) {
        Write-Host "         - $($f.Name)" -ForegroundColor Gray
    }
} else {
    Write-Host "  [WARN] Backup directory contains: $($backedUpJsons.Count) files" -ForegroundColor Yellow
}

Write-Host "`n[Step 3] Checking Notes.exe Process Health..." -ForegroundColor Yellow
$proc.Refresh()
if (-not $proc.HasExited) {
    $wsMB = [math]::Round($proc.WorkingSet64 / 1MB, 2)
    Write-Host "  [PASS] Notes.exe is running stably (Memory: $wsMB MB)" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] Notes.exe exited prematurely!" -ForegroundColor Red
}

Write-Host "`n[Step 4] Clean shutdown of Notes.exe..." -ForegroundColor Yellow
Stop-Process -Id $proc.Id -Force
Start-Sleep -Milliseconds 500
Write-Host "  -> Notes.exe stopped." -ForegroundColor Green

Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "   LIVE RUNTIME MIGRATION VERIFICATION COMPLETE!" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
