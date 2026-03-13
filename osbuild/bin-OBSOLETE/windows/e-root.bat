@echo off
if NOT "%E_ROOT%" == "" goto skip1
set E_ROOT="c:\coderoot" 
:skip1

if NOT "%E_BUILD_BIN%" == "" goto skip2
set E_BUILD_BIN=%E_ROOT%\eosal\build\bin\windows
set PATH=%PATH%;%E_BUILD_BIN%
:skip2


