@echo off
rem *******************************  mk.bat  *******************************
rem            ####### 
rem            ##  ## 
rem            #  ##    ####   #####    #####  ##  ##   ##### 
rem              ##    ##  ##  ##  ##  ##      ##  ##  ##     
rem             ##  #  ######  ##  ##   ####   ##  ##   ####  
rem            ##  ##  ##      ##  ##      ##   #####      ## 
rem           #######   ####   ##  ##  #####       ##  #####  
rem                                            #####
rem           Z-Wave, the wireless language.
rem                                            
rem               Copyright (c) 2001
rem               Zensys A/S
rem               Denmark
rem 
rem               All Rights Reserved
rem 
rem     This source file is subject to the terms and conditions of the
rem     Zensys Software License Agreement which restricts the manner
rem     in which it may be used.
rem
rem ---------------------------------------------------------------------------
rem
rem  Description: Make bat file for building ZW0102 slave application
rem
rem  Author:   Peter Shorty
rem 
rem  Last Changed By:  $Author: jsi $
rem  Revision:         $Revision: 27293 $
rem  Last Changed:     $Date: 2013-10-30 16:31:05 +0100 (on, 30 okt 2013) $
rem 
rem ****************************************************************************

if "%KEILPATH%"==""  goto usage_keil
if "%TOOLSDIR%"=="" goto usage_tools

set oldpath=%path%
set path=%KEILPATH%\bin;%TOOLSDIR%\Python;%path%

set BUILD_NUMBER=52445

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

:exit