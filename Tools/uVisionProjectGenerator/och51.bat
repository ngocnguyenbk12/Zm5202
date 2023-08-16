@echo off

if "%KEILPATH%"==""  goto usage_keil

set TARGETNAME=%1
set TARGETNEXTNAME=%2

set TARGETDIR=build_prj\%TARGETNAME%

if "%TARGETNEXTNAME%"=="" (
set TARGET=%TARGETNAME%_patch) else (
set TARGET=%TARGETNEXTNAME%)

rem set HEX=$(subst \,\\,$(KEILPATH)\bin\OHX51)

set PROJ_DIR=%CD%
cd %TARGETDIR%
echo .\Rels\%TARGET%.AOF  HEXFILE(%TARGET%.hex) 
rem C:\\KEIL\\C51\\bin\\OHX51 .\Rels\%TARGET%.AOF  HEXFILE(%TARGET%.hex) 
%KEILPATH%\bin\OHX51 .\Rels\%TARGET%.AOF  HEXFILE(%TARGET%.hex)
copy %TARGET%.hex Rels\ 
REM del %TARGET%.hex

echo .\Rels\%TARGETNAME%.AOF  HEXFILE(%TARGETNAME%.hex) 
rem C:\\KEIL\\C51\\bin\\OHX51 .\Rels\%TARGETNAME%.AOF  HEXFILE(%TARGETNAME%.hex) 
%KEILPATH%\bin\OHX51 .\Rels\%TARGETNAME%.AOF  HEXFILE(%TARGETNAME%.hex)
copy %TARGETNAME%.hex Rels\ 
REM del %TARGET%.hex


cd %PROJ_DIR%

goto exit

:usage_keil
@echo Set KEILPATH to point to the location of the Keil Compiler
@echo e.g c:\keil\c51

:exit