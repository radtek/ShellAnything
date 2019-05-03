@echo off

cd /d %~dp0
cd ..\..
set APPVEYOR=True
set APPVEYOR_BUILD_FOLDER=%cd%
set Configuration=Debug
set Platform=x64
REM set Platform=Win32
REM set PlatformToolset=Windows7.1SDK
set PlatformToolset=v100

echo Emulating appveyor's environment.
echo APPVEYOR_BUILD_FOLDER set to '%APPVEYOR_BUILD_FOLDER%'
echo.

:: Return back to original folder
cd /d %~dp0

:: Leave the command prompt open
cmd.exe /k echo AppVeyor ready...
