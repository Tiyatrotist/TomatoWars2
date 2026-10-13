$ErrorActionPreference = 'Stop'
Write-Host "== UEMCP kurulumu ==" -ForegroundColor Cyan

# 1) uv kurulu degilse kur
if (-not (Get-Command uv -ErrorAction SilentlyContinue)) {
    Write-Host "uv bulunamadi, winget ile kuruluyor..."
    winget install --id astral-sh.uv -e --accept-source-agreements --accept-package-agreements
    $env:Path += ";$env:USERPROFILE\.local\bin;$env:LOCALAPPDATA\Microsoft\WinGet\Links"
}
uv --version

# 2) Claude Desktop ayarina 'unreal' sunucusunu ekle (once yedek alir)
$cfg = Join-Path $env:APPDATA 'Claude\claude_desktop_config.json'
if (Test-Path $cfg) {
    Copy-Item $cfg "$cfg.bak-pre-unreal" -Force
    $j = Get-Content $cfg -Raw | ConvertFrom-Json
} else {
    New-Item -ItemType Directory -Force (Split-Path $cfg) | Out-Null
    $j = [pscustomobject]@{}
}
if (-not $j.PSObject.Properties['mcpServers']) {
    $j | Add-Member -NotePropertyName mcpServers -NotePropertyValue ([pscustomobject]@{})
}
$j.mcpServers | Add-Member -NotePropertyName unreal -NotePropertyValue ([pscustomobject]@{ command = 'uvx'; args = @('--with','mcp<2','uemcp') }) -Force
$j | ConvertTo-Json -Depth 20 | Set-Content $cfg -Encoding UTF8
Write-Host "Claude Desktop ayari guncellendi: $cfg" -ForegroundColor Green
Write-Host "Simdi Claude Desktop'i tamamen kapatip yeniden ac." -ForegroundColor Yellow
