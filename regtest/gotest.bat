@echo off
rem Copyright 2010-2012 Streams Lab Michal Widera
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
rem this script create enviroment for qt designer
rem run this batch script from console window - from explorer click no picture is presented

set VARBINPATH=..\..\qtcreator-build

ECHO Check %VARBINPATH%

IF NOT EXIST %VARBINPATH% set VARBINPATH=..\..\rc_multiplatform-build

ECHO Check %VARBINPATH%

IF NOT EXIST %VARBINPATH% GOTO SOURCESNOTFOUND

ECHO Path found.

IF NOT EXIST query.txt GOTO NOQUERYTXT GOTO NOQUERYTXT

IF NOT EXIST %VARBINPATH%\xcompiler.exe GOTO NOCOMPILER

IF NOT EXIST %VARBINPATH%\xdumper.exe GOTO NODUMPER

%VARBINPATH%\xcompiler.exe --log
IF ERRORLEVEL 1 GOTO COMPILERFAIL
IF NOT "%1" == "PARTVIEW"  GOTO SKIPPARTVIEW
%VARBINPATH%\xdumper.exe query.qry.lg1 -v -f -s
IF EXIST temp_$$$.png start temp_$$$.png
IF EXIST temp_$$$.jpg start temp_$$$.jpg
pause
%VARBINPATH%\xdumper.exe query.qry.lg2 -v -f -s
IF EXIST temp_$$$.png start temp_$$$.png
IF EXIST temp_$$$.jpg start temp_$$$.jpg
pause
:SKIPPARTVIEW
%VARBINPATH%\xdumper.exe -v -f -s
IF EXIST temp_$$$.png start temp_$$$.png
IF EXIST temp_$$$.jpg start temp_$$$.jpg
ECHO Success.
GOTO END

:NOQUERYTXT
ECHO Fail, xquery.txt not in place.
GOTO END

:NOCOMPILER
ECHO Fail, xcompiler.exe not found.
GOTO END

:NODUMPER
ECHO Fail, xdumper.exe not found.
GOTO END

:COMPILERFAIL
ECHO Compiler fail.
GOTO END

:SOURCESNOTFOUND
ECHO Path with compiled sources not found, compile sources, check scripts, check paths ...
GOTO END

:END
