# Generates records\state_en.json from records\state_zh.json by swapping the
# user-visible strings for their English originals.
#
# This script is deliberately ASCII-only: Windows PowerShell 5.1 reads a .ps1
# without a BOM as ANSI, which mangles any non-ASCII literal in the file. The
# CJK side of every replacement therefore lives in strings_en.json, read here
# with an explicit UTF-8 decoder.
#
# Replacements run LONGEST KEY FIRST. Several short keys are substrings of the
# longer ones: "法力再生" sits inside the Vampire spell's description
# "法力再生、毒素抗性、半血生命再生。", so replacing it first would corrupt it.
$ErrorActionPreference = 'Stop'

$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$utf8 = [Text.UTF8Encoding]::new($false)

$src = Join-Path $dir 'state_zh.json'
$mapFile = Join-Path $dir 'strings_en.json'
$dst = Join-Path $dir 'state_en.json'

$text = [IO.File]::ReadAllText($src, $utf8)
$map = [IO.File]::ReadAllText($mapFile, $utf8) | ConvertFrom-Json

$pairs = @()
foreach ($p in $map.PSObject.Properties) { $pairs += [pscustomobject]@{ Key = $p.Name; Value = $p.Value } }
$pairs = $pairs | Sort-Object { $_.Key.Length } -Descending

$missing = @()
foreach ($p in $pairs) {
  if ($text.Contains($p.Key)) {
    $text = $text.Replace($p.Key, $p.Value)
  } else {
    $missing += $p.Key
  }
}

if ($missing.Count -gt 0) {
  Write-Error ("source text did not contain " + $missing.Count + " key(s); first: " + $missing[0])
  exit 1
}

# No CJK may survive into the English manifest.
if ($text -match '[\u4e00-\u9fff]') {
  Write-Error 'English manifest still contains CJK characters'
  exit 1
}

[IO.File]::WriteAllText($dst, $text, $utf8)
Write-Host ("wrote " + $dst + " (" + $text.Length + " chars, " + $pairs.Count + " replacements)")
