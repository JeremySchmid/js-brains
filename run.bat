@echo off

run build.bat

pushd ..\..\..\build\brains
start win32_base.exe
popd