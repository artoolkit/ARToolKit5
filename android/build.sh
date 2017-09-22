#! /bin/bash

#--------------------------------------------------------------------------
#
#  ARToolKit5
#  ARToolKit for Android
#
#  This file is part of ARToolKit.
#
#  ARToolKit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ARToolKit is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
#
#  As a special exception, the copyright holders of this library give you
#  permission to link this library with independent modules to produce an
#  executable, regardless of the license terms of these independent modules, and to
#  copy and distribute the resulting executable under terms of your choice,
#  provided that you also meet, for each linked independent module, the terms and
#  conditions of the license of that module. An independent module is a module
#  which is neither derived from nor based on this library. If you modify this
#  library, you may extend this exception to your version of the library, but you
#  are not obligated to do so. If you do not wish to do so, delete this exception
#  statement from your version.
#
#  Copyright 2015 Daqri, LLC.
#  Copyright 2011-2015 ARToolworks, Inc.
#
#  Authors: Julian Looser, Philip Lamb, with updates from John Wolf
#
#--------------------------------------------------------------------------
# Use this script to build the native libraries in ARToolKit for Android if you have
# changed any of the library source.
# The core libraries are built first, followed by the wrapper.
#--------------------------------------------------------------------------

#
# Find out where we are and change to our directory
#
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${OURDIR}"
echo "Working from directory \"$PWD\"."

function usage {
    echo "Usage: $(basename $0) [--debug] [--verbose] [tests] [examples] [clean]"
    exit 1
}

# -e = exit on errors
set -e

# -x = debug
#set -x

# Parse parameters
while test $# -gt 0
do
    case "$1" in
		tests) BUILD_TESTS=1
		    ;;
		examples) BUILD_EXAMPLES=1
		    ;;
		clean) CLEAN=
		    ;;
        --debug) DEBUG=
            ;;
        --verbose) VERBOSE=
            ;;
        --*) echo "bad option $1"
            usage
            ;;
        *) echo "bad argument $1"
            usage
            ;;
    esac
    shift
done


# Set OS-dependent variables.
OS=`uname -s`
ARCH=`uname -m`
CPUS=
NDK_BUILD_SCRIPT_FILE_EXT=
TAR='/usr/bin/tar'
if [ "$OS" = "Linux" ]
then
    CPUS=`/usr/bin/nproc`
    TAR='/bin/tar'
    # Identify Linux OS. Sets useful variables: ID, ID_LIKE, VERSION, NAME, PRETTY_NAME.
    source /etc/os-release
    # Windows Subsystem for Linux identifies itself as 'Linux'. Additional test required.
    if grep -qE "(Microsoft|WSL)" /proc/version &> /dev/null ; then
        OS='Windows'
        NDK_BUILD_SCRIPT_FILE_EXT=".cmd"
    fi
elif [ "$OS" = "Darwin" ]
then
    CPUS=`/usr/sbin/sysctl -n hw.ncpu`
elif [ "$OS" = "CYGWIN_NT-6.1" ]
then
    # bash on Cygwin.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
    NDK_BUILD_SCRIPT_FILE_EXT=".cmd"
elif [ "$OS" = "MINGW64_NT-10.0" ]
then
    # git-bash on Windows.
    CPUS=`/usr/bin/nproc`
    OS='Windows'
    NDK_BUILD_SCRIPT_FILE_EXT=".cmd"
else
    CPUS=1
fi

if [ -n "${CLEAN+set}" ]; then
    CPUS=1
fi


#
# Update <AR/config.h> if required.
#
if [[ "$1" == "clean" ]]; then
    rm -f ../include/AR/config.h
else
    if [[ ../include/AR/config.h.in -nt ../include/AR/config.h ]]; then
        cp -v ../include/AR/config.h.in ../include/AR/config.h;
    fi
fi

#
# Build the ARToolKit libraries.
#
$NDK/ndk-build$NDK_BUILD_SCRIPT_FILE_EXT -j $CPUS ${DEBUG+NDK_DEBUG=1} ${VERBOSE+V=1} ${CLEAN+clean}
NDK_BLD_RESULT=$?
if [[ ${NDK_BLD_RESULT} != "0" ]]; then
  echo Exiting ndk-build script abnormally terminated.
  exit ${NDK_BLD_RESULT}
fi

#
# Build ARWrapper
#
$NDK/ndk-build$NDK_BUILD_SCRIPT_FILE_EXT -j $CPUS NDK_APPLICATION_MK=jni/Application-ARWrapper.mk ${DEBUG+NDK_DEBUG=1} ${VERBOSE+V=1} ${CLEAN+clean}
NDK_BLD_RESULT=$?
if [[ ${NDK_BLD_RESULT} != "0" ]]; then
  echo Exiting ndk-build script abnormally terminated.
  exit ${NDK_BLD_RESULT}
fi

#
# Copy ARWrapper and dependencies to ./libs folder of ARBaseLib-based examples.
#
ARTK_LibsDir=libs

if ! [ -n "${CLEAN+set}" ]; then
    JDK_PROJS=" \
        ARSimple \
        ARSimpleInteraction \
        ARMarkerDistance \
        ARSimpleOpenGLES20 \
        ARDistanceOpenGLES20 \
        ARMulti \
    "
    for i in $JDK_PROJS
    do
        FirstChar=${i:0:1}
        LCFirstChar=`echo $FirstChar | tr '[:upper:]' '[:lower:]'`
        ModName=$LCFirstChar${i:1}
        cp -Rpvf ${ARTK_LibsDir} ../AndroidStudioProjects/${i}Proj/$ModName/src/main/
    done
fi
