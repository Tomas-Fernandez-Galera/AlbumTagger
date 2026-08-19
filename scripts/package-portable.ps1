param(
    [string]$Configuration = "Release",
    [string]$QtRoot = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDirectory = Join-Path $projectRoot "out\build\release"
$sourceExecutable = Join-Path $buildDirectory "AlbumTagger.exe"
$distRoot = Join-Path $projectRoot "dist"
$cmakeProject = Get-Content -LiteralPath (Join-Path $projectRoot "CMakeLists.txt") -Raw
if ($cmakeProject -notmatch 'project\(AlbumTagger VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw "No se pudo leer la versión desde CMakeLists.txt."
}
$version = $Matches[1]
$packageName = "AlbumTagger-$version-Windows-x64"
$portableDirectory = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"
if ($QtRoot) {
    $deployTool = Join-Path $QtRoot "bin\windeployqt.exe"
} else {
    $deployCommand = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
    if ($deployCommand) { $deployTool = $deployCommand.Source }
}

if (-not (Test-Path -LiteralPath $sourceExecutable -PathType Leaf)) {
    throw "No se encuentra $sourceExecutable. Compila primero la configuración Release."
}
if (-not $deployTool -or -not (Test-Path -LiteralPath $deployTool -PathType Leaf)) {
    throw "No se encuentra windeployqt. Añade el bin de Qt al PATH o usa -QtRoot 'ruta\a\Qt\mingw_64'."
}

New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
$resolvedDist = (Resolve-Path -LiteralPath $distRoot).Path
if (-not $portableDirectory.StartsWith($resolvedDist, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "La carpeta portable calculada está fuera de dist."
}
if (Test-Path -LiteralPath $portableDirectory) {
    Remove-Item -LiteralPath $portableDirectory -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

New-Item -ItemType Directory -Path $portableDirectory -Force | Out-Null
Copy-Item -LiteralPath $sourceExecutable -Destination $portableDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "assets\windows\albumtagger-v2.ico") -Destination $portableDirectory

& $deployTool --release --compiler-runtime --no-translations `
    --dir $portableDirectory (Join-Path $portableDirectory "AlbumTagger.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt terminó con el código $LASTEXITCODE."
}

$licensesDirectory = Join-Path $portableDirectory "licenses"
New-Item -ItemType Directory -Path $licensesDirectory -Force | Out-Null
$qtBase = Split-Path (Split-Path (Split-Path (Split-Path $deployTool -Parent) -Parent) -Parent) -Parent
$qtLicense = Join-Path $qtBase "Licenses\LICENSE"
if (Test-Path -LiteralPath $qtLicense -PathType Leaf) {
    Copy-Item -LiteralPath $qtLicense -Destination (Join-Path $licensesDirectory "Qt-LICENSE.txt")
} else {
    throw "No se encontró la licencia de Qt en $qtLicense. No se creará un paquete incompleto."
}
Copy-Item -LiteralPath (Join-Path $buildDirectory "_deps\taglib-src\COPYING.MPL") -Destination (Join-Path $licensesDirectory "TagLib-MPL-2.0.txt")
Copy-Item -LiteralPath (Join-Path $projectRoot "assets\OPENMOJI-LICENSE.txt") -Destination (Join-Path $licensesDirectory "OpenMoji-CC-BY-SA-4.0.txt")
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md") -Destination $portableDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "LICENSE") -Destination $portableDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "COPYRIGHT.md") -Destination $portableDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot "TRADEMARKS.md") -Destination $portableDirectory

Compress-Archive -LiteralPath $portableDirectory -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "Paquete portable creado:"
Write-Host $portableDirectory
Write-Host $zipPath
