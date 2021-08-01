@echo off & setlocal enableextensions enabledelayedexpansion
REM Windows batch script for generating linker input file for second linking of patch targeted for development RAM
REM Parameters:
REM 1. map file from first linking of patchable program targeted for OTP.
REM 2. country (EU, US, ... JP).
REM 3. Application's name (directory name).
REM 4. Patch name (optional, if building a patch for starter project (MyProduct))
REM Example: uv-find-segment-end.bat .\build_prj\DoorBell_ZW040x_EU_starter_devmode\Rels\DoorBell_ZW040x_EU_devmode.map EU MyProduct dev_ctrl

SET MAPFILENAME1=%1
SET COUNTRY=%2
SET APPNAME=%3
if not "%4" == "" (
SET PATCHNAME=_%4
)
REM SET OUTPUTFILENAME=.\build_prj\%APPNAME%_ZW040x_%COUNTRY%_devmode\Rels\%APPNAME%_ZW040x_devmode%PATCHNAME%_patch.lin
echo %MAPFILENAME1:~-10%
if %MAPFILENAME1:~-10% == "_patch.map" (
SET OUTPUTFILENAME=%MAPFILENAME1:.map=.lin%
SET MAPFILENAME2=%MAPFILENAME1%.map
) else (
SET OUTPUTFILENAME=%MAPFILENAME1:.map=_patch.lin%
SET MAPFILENAME2=%MAPFILENAME1:.map=_patch.map%
)
echo PRINT("%MAPFILENAME2%")!>%OUTPUTFILENAME%
echo DISABLEWARNING (9,13,16,25,47)!>>%OUTPUTFILENAME%
echo NOOVERLAY!>>%OUTPUTFILENAME%
echo CLASSES (!>>%OUTPUTFILENAME%
for /F "usebackq tokens=1,4,5 delims=H" %%i IN (`findstr /R /I /C:"I:000020H\.0.* BIT$" %MAPFILENAME1%`) DO (
  SET WORK_VARIABLE=%%j
  SET WORK_VARIABLE2=%%k
  SET WORK_VARIABLE3=!WORK_VARIABLE:~7,1!
  SET WORK_VARIABLE=!WORK_VARIABLE:~8!!WORK_VARIABLE2:~0,2!
)
IF "!WORK_VARIABLE3!" == "0" (
  echo  BIT ^(I:0X002!WORK_VARIABLE!-0X002F.7^),!>>%OUTPUTFILENAME%
) ELSE (
  echo  BIT ^(I:0X003!WORK_VARIABLE!-0X002F.7^),!>>%OUTPUTFILENAME%
)
for /F "usebackq tokens=1,4 delims=H" %%i IN (`findstr /R /I /C:"I:000000H.* IDATA$" %MAPFILENAME1%`) DO (
  SET WORK_VARIABLE=%%j
  SET WORK_VARIABLE=0x!WORK_VARIABLE:~5!
  SET /A "WORK_VARIABLE+=0x80"
  call :DecimalToHex !WORK_VARIABLE! hex_
  SET WORK_VARIABLE=!hex_!
)
echo  IDATA (I:0X!WORK_VARIABLE!-D:0X00FF),!>>%OUTPUTFILENAME%
for /F "usebackq tokens=1,4 delims=H" %%i IN (`findstr /R /I /C:"I:000000H.* DATA$" %MAPFILENAME1%`) DO (
  SET WORK_VARIABLE=%%j
  SET WORK_VARIABLE=0x!WORK_VARIABLE:~5!
  SET /A "WORK_VARIABLE+=0x10"
  call :DecimalToHex !WORK_VARIABLE! hex_
  SET WORK_VARIABLE=!hex_!
)
echo  DATA (D:0X!WORK_VARIABLE!-D:0X007F),!>>%OUTPUTFILENAME%
for /F "usebackq tokens=1,4 delims=H" %%i IN (`findstr /R /I /C:"X:000000H.* XDATA$" %MAPFILENAME1%`) DO (
  SET WORK_VARIABLE=%%j
  SET WORK_VARIABLE=!WORK_VARIABLE:~5!
  SET WORK_VARIABLE2=0x!WORK_VARIABLE!
  SET /A WORK_VARIABLE3=0x!WORK_VARIABLE!+0
  SET /A "WORK_VARIABLE2+=0x1000"

  call :DecimalToHex !WORK_VARIABLE2! hex_
  SET WORK_VARIABLE2=!hex_!

  if !WORK_VARIABLE3! gtr 4095 (
    SET /A "WORK_VARIABLE3+=0xC000"
    call :DecimalToHex !WORK_VARIABLE3! hex_
    SET WORK_VARIABLE3=!hex_!
	
    SET XDATAHEADER=0X!WORK_VARIABLE2!
    SET XDATA_STRING=X:0X!WORK_VARIABLE2!-X:0X4FFF
    SET CODEPATCH_STRING=C:0X!WORK_VARIABLE3!-C:0XFFFF
  ) else (
    SET XDATAHEADER=0X!WORK_VARIABLE!
    SET XDATA_STRING=X:0X!WORK_VARIABLE!-X:0X0FFF,X:0X2000-X:0X4FFF
    SET CODEPATCH_STRING=C:0XD000-C:0XFFFF
)

)
echo  XDATA (!XDATA_STRING!),!>>%OUTPUTFILENAME%
echo  XDATA_PATCH (!XDATA_STRING!),!>>%OUTPUTFILENAME%
echo  XDATA_NON_ZERO_VARS_ZW (X:0X1200-X:0X123F),!>>%OUTPUTFILENAME%
echo  XDATA_NON_ZERO_VARS_APP (X:0X1240-X:0X127F),!>>%OUTPUTFILENAME%
echo  CODE (!CODEPATCH_STRING!),!>>%OUTPUTFILENAME%
echo  CODE_RAMFIXED (C:0XD000-C:0XEFFF),!>>%OUTPUTFILENAME%
echo  CODE_PATCH (!CODEPATCH_STRING!),!>>%OUTPUTFILENAME%
echo  CONST (!CODEPATCH_STRING!),!>>%OUTPUTFILENAME%
echo  CONST_PATCH (!CODEPATCH_STRING!)!>>%OUTPUTFILENAME%
echo )!>>%OUTPUTFILENAME%
echo SEGMENTS (!>>%OUTPUTFILENAME%
echo  ?CO?ZW_PATCHTABLE (^^^^C:0XFF7F), ?XD?ZW_XDATA_TAIL (LAST)!>>%OUTPUTFILENAME%
echo )!>>%OUTPUTFILENAME%
echo ASSIGN (XDATAHEADER (!XDATAHEADER!))!>>%OUTPUTFILENAME%
endlocal & goto :EOF
::
::===============================================================
:DecimalToHex DecimalIn HexOut
::
setlocal enableextensions
set /a dec_=%1
if %dec_% LSS 0 (endlocal & set %2=Error: negative & goto :EOF)
set hex_=
:_hexloop
  set /a dec1_=%dec_%
  set /a dec_=%dec_% / 16
  set /a hexdigit_=%dec1_% - 16*%dec_%
  set /a hexidx_=%hexdigit_%+1
  for /f "tokens=%hexidx_%" %%h in (
    'echo 0 1 2 3 4 5 6 7 8 9 A B C D E F') do set hexdigit_=%%h
  set hex_=%hexdigit_%%hex_%
  if %dec_% GTR 0 goto :_hexloop
endlocal & set %2=%hex_% & goto :EOF
