@setlocal
@echo off
set VERSION=0_99
echo About do do source release of IMAtoolbox ver%VERSION%. Press CTRL-C and Y to abort.
pause
svn copy -m "Source release of version %VERSION%" https://zensys14/svn/anicca/trunk/Tools/IMAtoolbox https://zensys14/svn/anicca/releases/src/Tools/IMAtoolbox/ver%VERSION%
echo About to binary release IMAtoolbox ver%VERSION%. Presss CTRL-C and Y to abort.
pause
svn import -m "Binary release of version %VERSION%" Release/IMAtoolbox.exe https://zensys14/svn/anicca/releases/bin/Tools/IMAtoolbox/ver%VERSION%/IMAtoolbox.exe