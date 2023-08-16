@echo on & setlocal
set CHIP=%1
set TARGETNAME=%2
set TARGETDIR=build_prj\%TARGETNAME%
set TARGET=%TARGETNAME%
set INT_VEC=0x1800
set BOOTLOADER_SIZE=%INT_VEC%

if not "%3" == "" (
set ZWLIBROOT=%3
)

%TOOLSDIR%\fixbootcrc\fixbootcrc.exe 0 %TARGETDIR%\%TARGET%.hex

if exist %TARGETDIR%\%TARGET%.hex (
del %TARGETDIR%\%TARGET%.hex
)
if exist %TARGETDIR%\%TARGET%.ota (
del %TARGETDIR%\%TARGET%.ota
)

if exist %TARGETDIR%\..\bootloader_%CHIP%\bootloader_%CHIP%.hex (
rename %TARGETDIR%\%TARGET%-crc.hex %TARGET%.hex
%TOOLSDIR%\HexTools\srec_cat.exe %TARGETDIR%\..\bootloader_%CHIP%\bootloader_%CHIP%.hex -Intel %TARGETDIR%\%TARGET%.hex -Intel -Output %TARGETDIR%\%TARGET%_BOOTLOADER.hex -Intel -address-length=3 -DO -Line_Length 44
@echo :00000001FF>>%TARGETDIR%\%TARGET%_BOOTLOADER.hex
) else (
rename %TARGETDIR%\%TARGET%-crc.hex %TARGET%.ota
%TOOLSDIR%\HexTools\srec_cat.exe %ZWLIBROOT%\lib\bootloader_%CHIP%\bootloader_%CHIP%.hex -Intel %TARGETDIR%\%TARGET%.ota -Intel -Output %TARGETDIR%\%TARGET%_WITH_BOOTLOADER.hex -Intel -address-length=3 -DO -Line_Length 44
@echo :00000001FF>>%TARGETDIR%\%TARGET%_WITH_BOOTLOADER.hex
if exist %ZWLIBROOT%\lib\otacompress\otacompress.exe (
%TOOLSDIR%\HexTools\srec_cat.exe %TARGETDIR%\%TARGET%.ota -intel -exclude 0x0 %BOOTLOADER_SIZE% -offset -%BOOTLOADER_SIZE% -o %TARGETDIR%\%TARGET%.bin -binary
%ZWLIBROOT%\lib\otacompress\otacompress.exe %TARGETDIR%\%TARGET%.bin %TARGETDIR%\%TARGET%.bin.lz1
%TOOLSDIR%\HexTools\srec_cat.exe  %TARGETDIR%\%TARGET%.bin.lz1 -binary -o %TARGETDIR%\%TARGET%.otz -intel -line-length=43
del %TARGETDIR%\%TARGET%.bin.lz1
del %TARGETDIR%\%TARGET%.bin
del %TARGETDIR%\%TARGET%.ota
)
)
