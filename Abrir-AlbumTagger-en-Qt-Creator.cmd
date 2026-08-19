@echo off
setlocal
set "QT_CREATOR="
if defined QTCREATOR_PATH if exist "%QTCREATOR_PATH%" set "QT_CREATOR=%QTCREATOR_PATH%"
if not defined QT_CREATOR for /f "delims=" %%I in ('where qtcreator.exe 2^>nul') do if not defined QT_CREATOR set "QT_CREATOR=%%I"
if not defined QT_CREATOR for /d %%D in ("%SystemDrive%\Qt\Tools\QtCreator*") do if exist "%%~fD\bin\qtcreator.exe" set "QT_CREATOR=%%~fD\bin\qtcreator.exe"

if not defined QT_CREATOR (
  echo No se ha encontrado Qt Creator.
  echo Abre Qt Creator manualmente y selecciona:
  echo %~dp0CMakeLists.txt
  echo Tambien puedes definir QTCREATOR_PATH con la ruta de qtcreator.exe.
  pause
  exit /b 1
)

rem El .pro muestra el árbol inmediatamente y delega la compilación en CMake.
start "" "%QT_CREATOR%" "%~dp0AlbumTagger.pro"
endlocal
