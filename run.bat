@echo off

call build.bat

pushd ..\..\..\build\brains
start win32_base.exe
popd