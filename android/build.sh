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
#  Authors: Julian Looser, Philip Lamb
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

# Set OS-dependent variables.
OS=`uname -s`
ARCH=`uname -m`
if [ "$OS" = "Linux" ]
then
    CPUS=`/usr/bin/nproc`
elif [ "$OS" = "Darwin" ]
then
    CPUS=`/usr/sbin/sysctl -n hw.ncpu`
elif [ "$OS" = "CYGWIN_NT-6.1" ]
then
    CPUS=`/usr/bin/nproc`
else
    CPUS=1
fi

if [ "$1" == "clean" ] ; then
    CPUS=1
fi

#
# Update <AR/config.h> if required.
#
if [ "$1" == "clean" ] ; then
rm -f ../include/AR/config.h
else
if [[ ../include/AR/config.h.in -nt ../include/AR/config.h ]]; then cp -v ../include/AR/config.h.in ../include/AR/config.h; fi
fi

#
# Build core ARToolKit libraries.
#
$NDK/ndk-build -j $CPUS $1

#
# Build ARWrapper and copy output libs to JDK-based targets.
#
$NDK/ndk-build -j $CPUS NDK_APPLICATION_MK=jni/Application-ARWrapper.mk $1

if [ "$1" != "clean" ] ; then
JDK_PROJS=" \
    ARSimple \
    ARSimpleInteraction \
    ARMulti \
"
for i in $JDK_PROJS
do
	if [ ! -d ../EclipseProjects/${i}/libs/ ] ; then
	    mkdir ../EclipseProjects/${i}/libs/
	fi
    cp -Rpv -f libs/* ../EclipseProjects/${i}/libs/
done
fi
