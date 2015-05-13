@REM	Batch file to open a Cygwin shell with the correct environment variables
@REM	for building the NDK library in the current directory. 

@echo off
call ..\..\SetNDKBuildVars.bat
if ERRORLEVEL 1 goto end

@REM	Run bash. Set NDK_PROJECT_PATH to the current directory (after first converting
@REM	it from Windows path style to Unix path style. Then execute the build script in
@REM	the project directory.
C:\cygwin\bin\bash --login -c "eval build.bsh"

:end
pause