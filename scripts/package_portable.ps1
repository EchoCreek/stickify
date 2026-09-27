# ------------------------------------------------------------------
# Stickify — Portable (绿化便携免安装版) 自动化打包脚本
# ------------------------------------------------------------------
param(
    [string]$Version = "1.0.1",
    [string]$OutputDir = "output"
)

$ErrorActionPreference = "Stop"

Write-Host "=== 开始构建 Stickify v$Version 绿化便携版压缩包 ===" -ForegroundColor Cyan

# 1. 确保 output 目录存在
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$stageDir = Join-Path $OutputDir "Stickify-$Version-x64-portable"
$zipOutput = Join-Path $OutputDir "Stickify.$Version.x64.portable.zip"

# 2. 清理旧暂存与旧 ZIP
if (Test-Path $stageDir) { Remove-Item -Recurse -Force $stageDir }
if (Test-Path $zipOutput) { Remove-Item -Force $zipOutput }

New-Item -ItemType Directory -Path $stageDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stageDir "themes") | Out-Null

# 3. 复制核心程序二进制与依赖
Copy-Item "x64\Release\Notes.exe" $stageDir
Copy-Item "x64\Release\WebView2Loader.dll" $stageDir
if (Test-Path "LICENSE") { Copy-Item "LICENSE" $stageDir }

# 4. 复制三大前端主题纯净 dist 产物
Copy-Item -Recurse "themes\default\dist" (Join-Path $stageDir "themes\Default")
Copy-Item -Recurse "themes\simple\dist" (Join-Path $stageDir "themes\Simple")
Copy-Item -Recurse "themes\manager\dist" (Join-Path $stageDir "themes\Manager")

# 5. 压缩为 portable zip
Write-Host "正在压缩打包为 ZIP: $zipOutput ..." -ForegroundColor Yellow
Compress-Archive -Path "$stageDir\*" -DestinationPath $zipOutput -CompressionLevel Optimal

# 6. 清理暂存目录
Remove-Item -Recurse -Force $stageDir

# 7. 打印产物信息
$zipItem = Get-Item $zipOutput
$sizeMB = [math]::Round($zipItem.Length / 1MB, 2)
Write-Host "=== 绿化便携版打包成功! ===" -ForegroundColor Green
Write-Host "产物: $($zipItem.FullName) ($sizeMB MB)" -ForegroundColor Green
