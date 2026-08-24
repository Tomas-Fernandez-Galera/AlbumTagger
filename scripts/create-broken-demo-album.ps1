param(
    [string]$Destination = "C:\Proyectos\AlbumTagger-Demo\Broken Signals"
)

$ErrorActionPreference = "Stop"
$sourceCandidates = @(
    (Join-Path $PSScriptRoot "..\out\build\release\_deps\taglib-src\tests\data\lame_cbr.mp3"),
    (Join-Path $PSScriptRoot "..\build-release\_deps\taglib-src\tests\data\lame_cbr.mp3")
)
$sourceMp3 = $sourceCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $sourceMp3) {
    throw "TagLib test MP3 not found. Build AlbumTagger once before running this script."
}

New-Item -ItemType Directory -Path $Destination -Force | Out-Null

function ConvertTo-Synchsafe([int]$value) {
    return [byte[]]@(
        (($value -shr 21) -band 0x7f),
        (($value -shr 14) -band 0x7f),
        (($value -shr 7) -band 0x7f),
        ($value -band 0x7f)
    )
}

function New-TextFrame([string]$id, [string]$value) {
    if ([string]::IsNullOrEmpty($value)) { return [byte[]]@() }
    $text = [Text.Encoding]::UTF8.GetBytes($value)
    $payload = [byte[]]::new($text.Length + 1)
    $payload[0] = 3 # ID3v2 UTF-8 text encoding
    [Array]::Copy($text, 0, $payload, 1, $text.Length)
    $frame = [Collections.Generic.List[byte]]::new()
    $frame.AddRange([Text.Encoding]::ASCII.GetBytes($id))
    $size = [BitConverter]::GetBytes([Net.IPAddress]::HostToNetworkOrder($payload.Length))
    $frame.AddRange($size)
    $frame.AddRange([byte[]]@(0, 0))
    $frame.AddRange($payload)
    return $frame.ToArray()
}

function New-TaggedMp3([hashtable]$track, [byte[]]$audioPayload) {
    $frames = [Collections.Generic.List[byte]]::new()
    foreach ($entry in @(
        @("TIT2", $track.Title), @("TPE1", $track.Artist), @("TALB", $track.Album),
        @("TPE2", $track.AlbumArtist), @("TCON", $track.Genre), @("TYER", $track.Year),
        @("TRCK", $track.Track), @("TCMP", "1")
    )) {
        [byte[]]$frameBytes = New-TextFrame $entry[0] $entry[1]
        if ($frameBytes.Length -gt 0) { $frames.AddRange($frameBytes) }
    }
    $header = [Collections.Generic.List[byte]]::new()
    $header.AddRange([Text.Encoding]::ASCII.GetBytes("ID3"))
    $header.AddRange([byte[]]@(3, 0, 0))
    [byte[]]$synchsafeSize = ConvertTo-Synchsafe $frames.Count
    $header.AddRange($synchsafeSize)
    $result = [Collections.Generic.List[byte]]::new()
    $result.AddRange($header)
    $result.AddRange($frames)
    $result.AddRange($audioPayload)
    [IO.File]::WriteAllBytes((Join-Path $Destination $track.File), $result.ToArray())
}

# Remove any original ID3 header so every file contains only our fictional tags.
$source = [IO.File]::ReadAllBytes($sourceMp3)
$offset = 0
if ($source.Length -ge 10 -and [Text.Encoding]::ASCII.GetString($source, 0, 3) -eq "ID3") {
    $tagSize = (($source[6] -band 0x7f) -shl 21) -bor (($source[7] -band 0x7f) -shl 14) -bor
               (($source[8] -band 0x7f) -shl 7) -bor ($source[9] -band 0x7f)
    $offset = 10 + $tagSize
}
$payloadLength = $source.Length - $offset
if ($payloadLength -ge 128 -and [Text.Encoding]::ASCII.GetString($source, $source.Length - 128, 3) -eq "TAG") {
    $payloadLength -= 128
}
$audioPayload = [byte[]]::new($payloadLength)
[Array]::Copy($source, $offset, $audioPayload, 0, $payloadLength)

# Deliberate defects: missing years/genres/tracks, duplicate track number,
# inconsistent album spelling and inconsistent album-artist values.
$tracks = @(
    @{File="01 - Static Dawn.mp3"; Title="Static Dawn"; Artist="The Test Signals"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Electronic"; Year="2024"; Track="1"},
    @{File="02 - Copper Sky.mp3"; Title="Copper Sky"; Artist="Mira Vale"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre=""; Year="2021"; Track="2"},
    @{File="03 - Empty Calendar.mp3"; Title="Empty Calendar"; Artist="North Circuit"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Ambient"; Year=""; Track="3"},
    @{File="04 - Neon Current.mp3"; Title="Neon Current"; Artist="The Test Signals"; Album="Broken Signal"; AlbumArtist="Various Artists"; Genre="Electronic"; Year="2023"; Track="4"},
    @{File="05 - Paper Satellite.mp3"; Title="Paper Satellite"; Artist="Luna Relay"; Album="Broken Signals"; AlbumArtist=""; Genre="Electronic"; Year="2020"; Track="5"},
    @{File="06 - Missing Step.mp3"; Title="Missing Step"; Artist="Mira Vale"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Electronic"; Year=""; Track=""},
    @{File="07 - Duplicate Pulse.mp3"; Title="Duplicate Pulse"; Artist="North Circuit"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Rock"; Year="2019"; Track="5"},
    @{File="08 - Quiet Machine.mp3"; Title="Quiet Machine"; Artist="Luna Relay"; Album="Broken Signals"; AlbumArtist="Various"; Genre=""; Year="2018"; Track="8"},
    @{File="09 - Glass Frequency.mp3"; Title="Glass Frequency"; Artist="The Test Signals"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Electronic"; Year="2022"; Track="9"},
    @{File="10 - Last Carrier.mp3"; Title="Last Carrier"; Artist="Mira Vale"; Album="Broken Signals"; AlbumArtist="Various Artists"; Genre="Electronic"; Year=""; Track="10"}
)

foreach ($track in $tracks) { New-TaggedMp3 $track $audioPayload }

# Original, programmatically drawn cover; no third-party artwork is used.
Add-Type -AssemblyName System.Drawing
$bitmap = [Drawing.Bitmap]::new(900, 900)
$graphics = [Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.Clear([Drawing.Color]::FromArgb(12, 20, 38))
for ($i = 0; $i -lt 12; $i++) {
    $color = [Drawing.Color]::FromArgb(190, 30 + 15 * $i, 210 - 10 * $i, 245)
    $pen = [Drawing.Pen]::new($color, 5)
    $radius = 65 + 30 * $i
    $graphics.DrawEllipse($pen, 450 - $radius, 410 - $radius, 2 * $radius, 2 * $radius)
    $pen.Dispose()
}
$fontTitle = [Drawing.Font]::new("Segoe UI", 52, [Drawing.FontStyle]::Bold)
$fontSub = [Drawing.Font]::new("Segoe UI", 23)
$white = [Drawing.SolidBrush]::new([Drawing.Color]::White)
$cyan = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(80, 220, 255))
$graphics.DrawString("BROKEN SIGNALS", $fontTitle, $white, 92, 720)
$graphics.DrawString("A FICTIONAL TEST COMPILATION", $fontSub, $cyan, 180, 795)
$bitmap.Save((Join-Path $Destination "folder.png"), [Drawing.Imaging.ImageFormat]::Png)
$cyan.Dispose(); $white.Dispose(); $fontSub.Dispose(); $fontTitle.Dispose()
$graphics.Dispose(); $bitmap.Dispose()

Write-Host "Created $($tracks.Count) deliberately inconsistent MP3 files and folder.png in:"
Write-Host $Destination
