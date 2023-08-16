@echo off

if "%KEILPATH%"==""  goto usage_keil

set TARGETNAME=%1

set TARGETDIR=build_prj\%TARGETNAME%

set TARGET=%TARGETNAME%

set PROJ_DIR=%CD%
cd %TARGETDIR%
echo .\Rels\%TARGET%.AOF  HEXFILE(%TARGET%.hex) 
%KEILPATH%\bin\OHX51 .\Rels\%TARGET%.AOF H386 MERGE32K HEXFILE(%TARGET%.hex)
copy %TARGET%.hex Rels\ 

echo .\Rels\%TARGETNAME%.AOF  HEXFILE(%TARGETNAME%.hex) 
%KEILPATH%\bin\OHX51 .\Rels\%TARGETNAME%.AOF H386 MERGE32K HEXFILE(%TARGETNAME%.hex)
copy %TARGETNAME%.hex Rels\ 

cd %PROJ_DIR%
goto exit

:usage_keil
@echo Set KEILPATH to point to the location of the Keil Compiler
@echo e.g c:\keil\c51
:exit