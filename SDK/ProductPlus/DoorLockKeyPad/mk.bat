@echo off
rem *******************************  mk.bat  *******************************
rem
rem  Copyright (c) 2001-2011
rem  Sigma Designs, Inc.
rem  All Rights Reserved
rem ---------------------------------------------------------------------------
rem
rem  Description: Make bat file for building generic product apps.
rem
rem  Author:   Peter Shorty
rem
rem  Last Changed By:  $Author: tro $
rem  Revision:         $Revision: 24134 $
rem  Last Changed:     $Date: 2013-01-25 16:35:56 +0200 (??, 25 ??? 2013) $
rem
rem ****************************************************************************

if "%KEILPATH%"==""  goto usage_keil
if "%TOOLSDIR%"=="" goto usage_tools

if not exist %KEILPATH%\bin\c51.exe goto usage_keil
if not exist %KEILPATH%\bin\cx51.exe goto no_ext

set oldpath=%path%
set path=%KEILPATH%\bin;%TOOLSDIR%\Python;%path%

set BUILD_NUMBER=52445

::Build for the normal chip.
%TOOLSDIR%\Make\make %*

set path=%oldpath%
set oldpath=
goto exit

:usage_keil
@echo Set KEILPATH to point to the location of the Keil Compiler
@echo e.g c:\keil\c51
goto exit

:usage_tools
@echo Set TOOLSDIR to point to the location of the Z-Wave tools
@echo e.g c:\projects\zensys\devkit\tools
goto exit

:no_ext
@echo This developers kit requires the Keil PK51 Professional Developer's Kit

:exit