@echo off


del /Q *.ver >nul 2>nul
rundll32 indent.dll,version
if not exist *.ver call :error "Error getting indent.dll version information"

for /F %%i in (indent.ver) do set PRJ_%%i

del /Q indent_%PRJ_VERSION%.zip >nul 2>nul

PKZIPC -add -lev=9 -path=none indent_%PRJ_VERSION%.zip indent.dll *.ver *.reg *.txt

del /Q *.dll >nul 2>nul
del /Q *.ver >nul 2>nul

exit 0





:error
echo %~1
exit 1
