@REM	Environment variables for Cygwin, JDK, Android SDK and NDK
@REM
@REM	Edit this file to suit your development environment!
@REM
@REM 

@REM ----------------------------------------------------------------------------
@REM UNCOMMENT THE FOLLOWING LINE ONCE YOU HAVE CONFIGURED THE PATHS IN THIS FILE
@REM ----------------------------------------------------------------------------
@REM set CONFIGURED="true"

if %CONFIGURED% == "true" goto setup 
echo -------------------------------------------------------------------------
echo Please edit SetNDKBuildVars.bat in the root directory of the SDK.
echo You need to set some environment variables before building any libraries.
echo -------------------------------------------------------------------------
exit /B 1

:setup

@REM Start Edits

@REM EDIT HERE: Set the path to the Java Development Kit
set JAVA_HOME=C:/Program Files (x86)/Java/jdk1.6.0_24

@REM EDIT HERE: Add the Android SDK tools and Android NDK to the system path
set ANDROID_SDK_TOOLS_DIR=C:/Android/android-sdk/tools
set NDK=C:/Android/android-ndk-r9

@REM EDIT HERE: Path to Cygwin's bash
set BASH_PATH=C:\cygwin\bin\bash

@REM End Edits

set PATH=%ANDROID_SDK_TOOLS_DIR%;%NDK%
set IS_UNIX=

exit /B 0