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
rem PL: sytuacja odwrotna - kopiowane s¹ pliki do katalogu budowania aby
rem PL: mozna bylo testowac w srodowisku qt-designera kompilator, dumper lub executor

set VARBINPATH=..\..\qtcreator-build

IF EXIST   %VARBINPATH%\query.txt ECHO query.txt in place. Ok, skiping prepare.
IF NOT EXIST  %VARBINPATH%\query.txt ECHO no query.txt in build dir - copy from here to build dir.
IF NOT EXIST  %VARBINPATH%\query.txt COPY query.txt  %VARBINPATH%\

IF NOT EXIST %VARBINPATH%\compiler.exe GOTO NOCOMPILER

cd %VARBINPATH%
compiler.exe --log
IF ERRORLEVEL 1 GOTO COMPILERFAIL

ECHO Success.
GOTO END

:NOCOMPILER
ECHO Fail, compiler.exe not found.
GOTO END

:NOCOMPILER
ECHO Fail, compiler.exe not found.
GOTO END

:END
pause
