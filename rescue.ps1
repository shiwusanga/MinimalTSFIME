# 急救卸载脚本 —— 纯注册表清理，不需要原 DLL 存在。
# 必须以管理员身份运行（rescue.bat 会拉起本脚本）。

$ErrorActionPreference = 'Continue'

$clsid   = '{B7901AAF-FED6-4E40-905F-6D1353640ECB}'
$profile = '{791D57B7-DC58-4809-B8BB-A5D8EC8186EA}'
$tip     = "0x0804:$clsid$profile"

# ---- 0. 管理员检查 ----
$isAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host '[X] 请以管理员身份运行 rescue.bat' -ForegroundColor Red
    exit 1
}

Write-Host '=== 极简拼音 (TSF 示例) 急救卸载 ===' -ForegroundColor Cyan
Write-Host "CLSID : $clsid"
Write-Host ''

# ---- 1. 若 DLL 还在，先走一次正规反注册 ----
Write-Host '[1/4] 尝试正规反注册 (regsvr32 /u)'
$dlls = @(
    (Join-Path $PSScriptRoot 'x64\Release\MinimalTSFIME.dll'),
    (Join-Path $PSScriptRoot 'Win32\Release\MinimalTSFIME.dll'),
    (Join-Path $PSScriptRoot 'MinimalTSFIME.dll')
) | Where-Object { Test-Path $_ }

foreach ($d in $dlls) {
    # 32 位 DLL 必须用 32 位 regsvr32；这里两种都试，失败可忽略
    Start-Process -FilePath "$env:windir\System32\regsvr32.exe" `
                  -ArgumentList '/u','/s',"`"$d`"" -Wait -NoNewWindow
    Start-Process -FilePath "$env:windir\SysWOW64\regsvr32.exe" `
                  -ArgumentList '/u','/s',"`"$d`"" -Wait -NoNewWindow
}
Write-Host '    完成（失败不影响后续步骤）'

# ---- 2. 从用户键盘列表里撤掉条目 ----
Write-Host '[2/4] 从键盘列表移除 (InstallLayoutOrTip / ILOT_UNINSTALL)'
try {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class InputDll {
    [DllImport("input.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern bool InstallLayoutOrTip(string psz, uint dwFlags);
}
'@
    $ok = [InputDll]::InstallLayoutOrTip($tip, 0x00000001)   # ILOT_UNINSTALL
    Write-Host "    InstallLayoutOrTip 返回: $ok"
} catch {
    Write-Host "    跳过（$($_.Exception.Message)）" -ForegroundColor Yellow
    Write-Host '    若键盘列表里仍有残留，手动在设置里删掉即可。' -ForegroundColor Yellow
}

# ---- 3. 清理注册表（多视图都试一遍，不存在就跳过）----
Write-Host '[3/4] 清理注册表'
$keys = @(
    "Registry::HKEY_CLASSES_ROOT\CLSID\$clsid",
    "HKLM:\SOFTWARE\Classes\CLSID\$clsid",
    "HKLM:\SOFTWARE\Classes\Wow6432Node\CLSID\$clsid",
    "HKLM:\SOFTWARE\Microsoft\CTF\TIP\$clsid",
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\CTF\TIP\$clsid",
    "HKCU:\Software\Microsoft\CTF\TIP\$clsid"
)
foreach ($k in $keys) {
    if (Test-Path $k) {
        Remove-Item -Path $k -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "    已删除: $k"
    }
}

# ---- 4. 收尾提示 ----
Write-Host '[4/4] 完成' -ForegroundColor Green
Write-Host ''
Write-Host '接下来做两件事：' -ForegroundColor Cyan
Write-Host '  1) 按 Win+空格 切回「微软拼音」，确认还能正常打字'
Write-Host '  2) 注销或重启一次 Windows（让 ctfmon 重新加载输入法列表）'
Write-Host ''
Write-Host '如果之前建过系统还原点，也可以直接回滚到安装前的状态。' -ForegroundColor Gray
Write-Host ''
