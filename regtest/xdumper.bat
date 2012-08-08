@echo off
rem Copyright 2011 Streams Lab Michal Widera
rem
rem  RESTRICTED RIGHTS LEGEND
rem  Use, duplication or disclosure by any commercial industry (public or
rem  private), private individual or by any Government Agency without an
rem  expressed written consent of release from Streams Lab Michal Widera
rem  is subject to restriction of the Rights.
rem
rem  Manufacturer is:
rem      Streams Lab Michal Widera
rem      Reymonta str. 56/1
rem      Poland, Zabrze, 41-800
rem      (+48) 606 995 918
rem 
rem this calls compiler from command line
rem
rem http://stackoverflow.com/questions/3215501/batch-remove-file-extension

set VARBINPATH=..\..\qtcreator-build

ECHO Check %VARBINPATH%
IF EXIST %VARBINPATH% GOTO PATH_FOUND
IF NOT EXIST %VARBINPATH% set VARBINPATH=..\..\rc_multiplatform-build
ECHO Check %VARBINPATH%
IF NOT EXIST %VARBINPATH% GOTO SOURCESNOTFOUND
:PATH_FOUND
ECHO Path found.
 
IF NOT EXIST query.txt GOTO NOQUERYTXT GOTO NOQUERYTXT
IF NOT EXIST %VARBINPATH%\%~n0.exe GOTO NOFILEINPLACE

echo.
echo %VARBINPATH%\%~n0 %1 %2 %3 %4 %5 %6 %7 %8 %9
echo.
%VARBINPATH%\%~n0 %1 %2 %3 %4 %5 %6 %7 %8 %9

IF ERRORLEVEL 1 GOTO SCRIPTFAIL

ECHO Success.
GOTO END

:NOFILEINPLACE
ECHO Fail, %~n0.exe not in place.
GOTO END

:SCRIPTFAIL
ECHO %0 fail.
GOTO END

:SOURCESNOTFOUND
ECHO Path with compiled sources not found, compile sources, check scripts, check paths ...
GOTO END

:END
