# Renders a phase's picture set into captures\<set>\ with small copies and a contact sheet (TECH_SPEC 8.1).
#   scripts\review.ps1 -Set p1|p2|p3|p5|p6 [-Only a,b] [-Config dev|debug] [-NoBuild] [-Lint]
#   scripts\review.ps1 -Set levels [-From N -Count M | -Levels a,b,c]   (levels from above; default every fifth to 300)
# Each picture is one run of the desktop app (silent, off screen). Screens are also rendered across the aspect matrix
# (TECH_SPEC 5.2) when the set asks for it. Exit 1 if any run failed (GL messages: exit 3; a lint: exit 7).
param(
    [Parameter(Mandatory)][string]$Set,
    [string[]]$Only = @(),
    [ValidateSet('debug', 'dev')][string]$Config = 'debug',
    [switch]$NoBuild,
    [switch]$Lint,
    [int]$From = 0,
    [int]$Count = 30,
    [int[]]$Levels = @()
)
. "$PSScriptRoot\env.ps1"
if (-not $NoBuild) { & "$PSScriptRoot\build-desktop.ps1" -Config $Config }
$exe = Join-Path $GT_ROOT "build\desktop-$Config\glowtide.exe"

if ($Set -eq 'p4') {
    # Sound (P4): offline renders through the headless tool (never a device), then one table of their numbers.
    $headless = Join-Path $GT_ROOT "build\desktop-$Config\glowtide_headless.exe"
    $out = Join-Path $GT_ROOT 'captures\p4'
    New-Item -ItemType Directory -Force $out | Out-Null
    foreach ($scene in 'ladder', 'flood', 'geodes', 'ui', 'win', 'lose', 'music', 'timbres') {
        & $headless --render-audio $scene --out (Join-Path $out "$scene.wav")
        if ($LASTEXITCODE -ne 0) { Write-Host "[review] $scene FAILED"; exit 1 }
    }
    $wavs = 'flood', 'geodes', 'music', 'ladder', 'ui', 'win', 'lose', 'timbres_kalimba', 'timbres_glass', 'timbres_bell' |
        ForEach-Object { Join-Path $out "$_.wav" }
    python -I -B (Join-Path $GT_ROOT 'tools\wav_metrics.py') @wavs --ladder (Join-Path $out 'ladder.wav') --table (Join-Path $out 'metrics.md')
    exit $LASTEXITCODE
}
$out = Join-Path $GT_ROOT "captures\$Set"
New-Item -ItemType Directory -Force $out | Out-Null

# name -> app arguments (frames default 120)
$sets = @{
    p1 = [ordered]@{
        concept = '--picture concept'
        elements = '--picture elements'
        unit_tiers = '--picture unit_tiers'
        density_1200 = '--picture density_1200'
        density_2400 = '--picture density_2400'
        density_4000 = '--picture density_4000 --set capW=4000'
        label_over_flood = '--picture label_over_flood'
        world_1 = '--picture world_1'
        world_2 = '--picture world_2'
        world_3 = '--picture world_3'
        glow_low = '--picture concept --set glowStrength=0.6'
        glow_high = '--picture concept --set glowStrength=1.5'
        no_glow = '--picture concept --set glowStrength=0'
        bench_flood = '--scenario bench_flood --fixed-step --frames 600'
    }
    v1 = [ordered]@{
        geodes = '--picture geodes'
        lanterns_3 = '--picture lanterns_3'
        geode_strip = '--picture geode_strip --strip 8 --strip-step 5 --frames 42'
        pan_strip = '--picture pan_strip --strip 8 --strip-step 2 --frames 70'
    }
    p5 = [ordered]@{
        world_1 = '--picture world_1'
        world_2 = '--picture world_2'
        world_3 = '--picture world_3'
        world_4 = '--picture world_4'
        world_5 = '--picture world_5'
        elements = '--picture elements'
    }
}
$matrix = @('1080x2400', '540x1200', '1080x2640', '1080x1920', '600x1024', '1200x1920', '1080x1440', '1264x2780')
$screens = @('home', 'hud', 'pause', 'win', 'lose', 'settings')
$list = [ordered]@{}
if ($sets.ContainsKey($Set)) { foreach ($k in $sets[$Set].Keys) { $list[$k] = $sets[$Set][$k] } }
if ($Set -eq 'p3') {
    foreach ($s in $screens) { foreach ($r in $matrix) { $list["${s}_$r"] = "--picture $s --res $r" } }
    $list['button_strip'] = '--picture button_strip --strip 3 --strip-step 6'
}
if ($Set -eq 'v1') {
    # The HUD with a full boon tray across the aspect matrix, for the layout lint (VARIETY_PLAN V1 task 7).
    foreach ($r in $matrix) { $list["hud_boons_$r"] = "--picture hud_boons --res $r" }
}
if ($Set -eq 'p6') {
    $list['win_strip'] = '--picture win_strip --strip 8 --strip-step 12'
    $list['boss_win_strip'] = '--picture boss_win_strip --strip 8 --strip-step 12'
    $list['lose_strip'] = '--picture lose_strip --strip 8 --strip-step 8'
    $list['next_strip'] = '--picture next_strip --strip 8 --strip-step 4'
}
if ($Set -eq 'levels') {
    # Level sheets from straight above with their labels: every fifth level (P5), thirty consecutive levels from -From
    # (VARIETY_PLAN 7: sameness the variety report cannot see), or the levels listed.
    $nums = if ($Levels.Count) { $Levels } elseif ($From -gt 0) { $From..($From + $Count - 1) } else { (1..60) | ForEach-Object { $_ * 5 } }
    foreach ($n in $nums) { $list[('level_{0:D3}' -f $n)] = "--picture level_top --level $n --no-hud --frames 4 --res 540x1200" }
}
if ($Set -eq 'p2') {
    $list['gate_strip'] = '--picture gate_strip --strip 8 --strip-step 3'
    $list['clash_strip'] = '--picture clash_strip --strip 8 --strip-step 4'
    $list['core_hit_strip'] = '--picture core_hit_strip --strip 8 --strip-step 3'
}
$Only = @($Only | ForEach-Object { $_ -split "," } | Where-Object { $_ })
if ($Only.Count) { $sel = [ordered]@{}; foreach ($k in $list.Keys) { if ($Only -contains $k) { $sel[$k] = $list[$k] } }; $list = $sel }

$failed = @()
$smalls = @()
foreach ($name in $list.Keys) {
    $a = $list[$name]
    $res = '1080x2400'
    if ($a -match '--res (\d+x\d+)') { $res = $Matches[1]; $a = $a -replace '--res \d+x\d+', '' }
    $frames = 120
    if ($a -match '--frames (\d+)') { $frames = [int]$Matches[1]; $a = $a -replace '--frames \d+', '' }
    $png = Join-Path $out "$name.png"
    $argv = @($a -split '\s+' | Where-Object { $_ }) + @('--silent', '--offscreen', '--res', $res, '--screenshot', $png, '--frames', "$frames")
    if ($Lint) { $argv += '--lint' }
    & $exe @argv | Out-Null
    $code = $LASTEXITCODE
    if ($code -ne 0) { $failed += "$name ($code)"; Write-Host "[review] $name FAILED ($code)"; continue }
    python -I -B (Join-Path $GT_ROOT 'tools\shrink_png.py') $png | Out-Null
    $smalls += ($png -replace '\.png$', '.small.png')
    if ($a -match '--strip') { python -I -B (Join-Path $GT_ROOT 'tools\contact.py') strip (Join-Path $out "${name}_sheet.png") $png | Out-Null }
    Write-Host "[review] $name"
}
if ($smalls.Count -and $Set -eq 'levels' -and ($From -gt 0 -or $Levels.Count)) {
    $sheet = if ($Levels.Count) { 'sheet_levels.png' } else { "sheet_${From}_$($From + $Count - 1).png" }
    python -I -B (Join-Path $GT_ROOT 'tools\contact.py') sheet (Join-Path $out $sheet) @smalls --cols ([math]::Min(10, $smalls.Count)) --height 520
} elseif ($smalls.Count -and $Set -eq 'levels') {
    # Sixty thumbnails: two sheets of thirty, readable at review size.
    python -I -B (Join-Path $GT_ROOT 'tools\contact.py') sheet (Join-Path $out 'contact_1.png') @($smalls | Select-Object -First 30) --cols 10 --height 520
    python -I -B (Join-Path $GT_ROOT 'tools\contact.py') sheet (Join-Path $out 'contact_2.png') @($smalls | Select-Object -Skip 30) --cols 10 --height 520
} elseif ($smalls.Count) { python -I -B (Join-Path $GT_ROOT 'tools\contact.py') sheet (Join-Path $out 'contact.png') @smalls --cols 5 --height 560 }
if ($failed.Count) { Write-Host "[review] $($failed.Count) failed: $($failed -join ', ')"; exit 1 }
exit 0
