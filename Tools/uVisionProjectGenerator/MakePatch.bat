@echo off & setlocal enableextensions enabledelayedexpansion
echo Make hex files for Patch System (Development Mode)
REM Usage:
REM MakePatch CHIP STEP TARGET [APPNAME]
REM     CHIP   - chip type: ZW040X
REM     STEP   - setp of the build process: APP, BASE, PATCH
REM     TARGET - target name, for ex. leddimer_devmode. Should be with suffix _patch if STEP = PATCH
REM     APPNAME - patch application name for a starter project built on an empty patchable application.
REM               APPNAME is optional.
REM Description (example):
REM  After the OTP part of leddimer_devmode application for ZW040x has been build,
REM  we will have hex file:
REM   1) leddimer_devmode.hex
REM  MakePatch will use this hex file to fix checksum in it and produce another .hex files. Run:
REM   MakePatch ZW040X BASE leddimer_devmode
REM  After this you will have 2 files:
REM   2) leddimer_devmode_OTP.hex - this is a hex file with OTP part of (1) (leddimer_devmode.hex).
REM   3) leddimer_devmode_RAM.hex - this is a hex file with RAM part of (1) (leddimer_devmode.hex).
REM  After the PATCH part of leddimer_devmode application for ZW040x has been build,
REM  we will have hex file:
REM   4) leddimer_devmode_patch.hex
REM  MakePatch will use this hex file to fix checksum in it and produce another .hex files. Run:
REM   MakePatch ZW040X PATCH leddimer_devmode
REM  After this file (3) will be deleted and you will have additional file:
REM   5) leddimer_devmode_patch_RAM.hex - this is a hex file with RAM part of (4).
REM  You need to program the leddimer_devmode_OTP.hex to the OTP memory of the ZW040x.
REM  And you need to program the leddimer_devmode_patch_RAM.hex to the RAM of the ZW040x.
REM  You can made changes in the patch part of your project, recompile leddimer_devmode_patch.hex
REM   and reupload leddimer_devmode_patch_RAM.hex to the RAM to test the changes.

set CHIP=%1
set STEP=%2

if "%CHIP%" == "M"  (
  %TOOLSDIR%\Python\Python.exe %TOOLSDIR%\uVisionProjectGenerator\__init__.py "%CHIP%" "%STEP%"
  goto exit
) else (
  if "%CHIP%" == "P" (
    %TOOLSDIR%\Python\Python.exe %TOOLSDIR%\uVisionProjectGenerator\__init__.py "%CHIP%" "%STEP%"
    goto exit
  )
)

if "%STEP%" == "PATCH" (
  %TOOLSDIR%\Python\Python.exe %TOOLSDIR%\uVisionProjectGenerator\j.py %3
)

set TARGETNAME=%3
set APPNAME=%4
set TARGETDIR=build_prj\%TARGETNAME%
set TARGET=%TARGETNAME%
set TARGETFIXED=%TARGET%
if not "%STEP%" == "PATCH" goto initPatchStepEnd
if not "%APPNAME%" == "" (
set TARGET=%TARGET%_%APPNAME%
)
set TARGET=%TARGET%_patch
:initPatchStepEnd
set FULLTARGET=%TARGETDIR%\%TARGET%
set FULLOUTPUT=%TARGETDIR%\Rels\%TARGET%

set PATCHTABLECRC=0XFF7E
set FIXPATCHCRC=%TOOLSDIR%\fixpatchcrc\fixpatchcrc.exe
set SREC=%TOOLSDIR%\HexTools\srec_cat.exe

if "%CHIP%" == "ZW010X" goto start
if "%CHIP%" == "ZW020X" goto start
if "%CHIP%" == "ZW030X" goto start
if "%CHIP%" == "ZW040X" goto start
if "%CHIP%" == "ZW050X" goto start
goto error

:start

REM Move all hex files from Rels folder to the target folder, and move map files from List folder to Rels folder.
REM move /Y %TARGETDIR%\Rels\*.hex %TARGETDIR%\ >>nul
if exist %TARGETDIR%\List\*.map   move /Y %TARGETDIR%\List\*.map        %TARGETDIR%\Rels\ >>nul

REM Move all files from target folder to the Rels folder except .hex
if exist %TARGETDIR%\%TARGETNAME% move /Y %TARGETDIR%\%TARGETNAME% %TARGETDIR%\Rels\ >>nul
if exist %TARGETDIR%\*.aof        move /Y %TARGETDIR%\*.aof        %TARGETDIR%\Rels\ >>nul
REM TODO: The following command results in this error "The process cannot access the file because it is being used by another process."
REM TODO: The file is locked by uVision.
REM if exist %TARGETDIR%\*.plg        move /Y %TARGETDIR%\*.plg        %TARGETDIR%\Rels\ >>nul
if exist %TARGETDIR%\*.lnp        move /Y %TARGETDIR%\*.lnp        %TARGETDIR%\Rels\ >>nul
if exist %TARGETDIR%\*.map        move /Y %TARGETDIR%\*.map        %TARGETDIR%\Rels\ >>nul
if exist %TARGETDIR%\*.sbr        move /Y %TARGETDIR%\*.sbr        %TARGETDIR%\Rels\ >>nul


REM if not "%CHIP%" == "ZW040X" goto ZW040XHandlerEnd

if not "%STEP%" == "APP" goto stepAppEnd
move /Y %FULLOUTPUT%.hex %TARGETDIR%\ >>nul
:stepAppEnd

if not "%STEP%" == "BASE" goto stepBaseEnd
move /Y %FULLOUTPUT%.hex %TARGETDIR%\ >>nul
REM Extract OTP contents from hex file from 1. linking for target in development mode
REM %SREC% %FULLTARGET%.hex -Intel -crop 0x0000 0xD000 -Output %FULLTARGET%_OTP.hex -Intel -address-length=2 -DO -Line_Length 44
REM Extract RAM contents from hex file from 1. linking for target in development mode (if there is any)
%SREC% %FULLOUTPUT%.hex -Intel -crop 0xD000 0x10000 -Output %FULLOUTPUT%_RAM.hex -Intel -address-length=2 -DO -Line_Length 44
:stepBaseEnd

if not "%STEP%" == "PATCH" goto stepPatchEnd
@echo Fixing CRC in %FULLTARGET%.hex
%FIXPATCHCRC% %PATCHTABLECRC% %FULLOUTPUT%.hex
del %FULLOUTPUT%.hex
rename %FULLOUTPUT%-crc.hex %TARGET%.hex

REM Extract and concatenate RAM contents from hex files from both 1. and 2. linking for target in development mode
REM %SREC% %TARGETDIR%\Rels\%TARGET%.hex -Intel -crop 0xD000 0x10000 %FULLOUTPUT%.hex -Intel -crop 0xD000 0x10000 -Output %FULLTARGET%_RAM.hex -Intel -address-length=2 -DO -Line_Length 44
%SREC% %TARGETDIR%\%TARGETFIXED%.hex -Intel -crop 0xD000 0x10000 %FULLOUTPUT%.hex -Intel -crop 0xD000 0x10000 -Output %FULLTARGET%_RAM.hex -Intel -address-length=2 -DO -Line_Length 44
REM Remove RAM contents hex file from 1. linking for target in development mode, to make no confusion
REM  if exist %TARGETFIXED%_RAM.hex del %TARGETFIXED%_RAM.hex
if exist %TARGETDIR%\%TARGETFIXED%_patch.hex del %TARGETDIR%\%TARGETFIXED%_patch.hex
:stepPatchEnd

:ZW040XHandlerEnd

goto exit

:error
@echo Chip is not supported

:exit
