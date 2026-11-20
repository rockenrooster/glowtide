# Puts the pinned third-party sources into third_party/ without downloading anything (TECH_SPEC 1):
# SDL is cloned from RAVEFORM's local checkout and checked against the pinned commit; the Khronos headers for the
# desktop build are copied from the NDK. Safe to re-run: existing trees are kept and verified.

. "$PSScriptRoot\env.ps1"

$tp = Join-Path $GT_ROOT 'third_party'
New-Item -ItemType Directory -Force $tp | Out-Null

$sdlTag = 'release-3.4.20'
$sdlCommit = 'f00dc919ca35b9756316e86c4777392de442f9b2'
$sdlSource = 'D:\Stuff\DevFolder\Android_Games\Raveform\third_party\SDL'
$sdl = Join-Path $tp 'SDL'
if (Test-Path (Join-Path $sdl 'CMakeLists.txt')) {
    Write-Host "[deps] SDL already present"
} else {
    if (-not (Test-Path (Join-Path $sdlSource '.git'))) { throw "[deps] $sdlSource is missing: ask the owner before cloning SDL from GitHub" }
    Write-Host "[deps] cloning SDL $sdlTag from $sdlSource"
    $url = 'file:///' + ($sdlSource -replace '\\', '/')
    & $GT_GIT -c advice.detachedHead=false clone --quiet --depth 1 --branch $sdlTag $url $sdl
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[deps] clone failed; copying the tree instead"
        if (Test-Path $sdl) { Remove-Item -Recurse -Force $sdl }
        robocopy $sdlSource $sdl /E /XD .git /NFL /NDL /NJH /NJS /NP | Out-Null
        Set-Content (Join-Path $tp 'SDL.commit') $sdlCommit
    }
}
if (Test-Path (Join-Path $sdl '.git')) {
    $head = (& $GT_GIT -C $sdl rev-parse HEAD).Trim()
    if ($head -ne $sdlCommit) { throw "[deps] SDL is at $head, expected $sdlCommit ($sdlTag): delete third_party\SDL and run again" }
    Write-Host "[deps] SDL verified at $sdlCommit"
} elseif ((Get-Content (Join-Path $tp 'SDL.commit') -ErrorAction SilentlyContinue) -ne $sdlCommit) {
    throw "[deps] third_party\SDL has no .git and no matching SDL.commit"
}

# Khronos headers (desktop only; Android compiles against the NDK sysroot's own copies).
$kh = Join-Path $tp 'khronos'
$sysroot = Join-Path $GT_NDK 'toolchains\llvm\prebuilt\windows-x86_64\sysroot\usr\include'
foreach ($h in @('GLES3\gl3.h', 'GLES3\gl3platform.h', 'KHR\khrplatform.h')) {
    $dst = Join-Path $kh $h
    if (-not (Test-Path $dst)) {
        New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
        Copy-Item (Join-Path $sysroot $h) $dst
        Write-Host "[deps] copied $h"
    }
}
Write-Host "[deps] done"
