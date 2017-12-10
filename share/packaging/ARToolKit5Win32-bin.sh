#! /bin/bash
#
# Find out where we are and change to ARToolKit5 root.
#
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${OURDIR}/../../"
echo "Building archive from directory \"$PWD\"."
#
# Give the archive a sensible name.
#
SET os=`systeminfo | findstr /B /C:"OS Name"`
ARCH=`uname -m`

OS='OSX'
ARCH='Universal'

VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' include/AR/config.h.in`
#
# Build the archive.
# Exclude: build files and directories, version control info,
# ARToolKit settings files which don't carry over.
#
BOM="share/packaging/ARToolKit5-bin-bom-win32"
PACKAGE_NAME="ARToolKit5-bin-${VERSION}-${OS}-${ARCH}.zip"

PACKAGE="../${PACKAGE_NAME}"

export PACKAGE_NAME

rm -f "$PACKAGE"
zip -r -MM "$PACKAGE" . \
    -i@share/packaging/ARToolKit5-bin-bom-win32 \
    --exclude "*/.git/*" \
    --exclude "*/.DS_Store" \
    --exclude "*/.metadata/*" \
    --exclude "*/gen/*" \
    --exclude "*/objs/*" \
    --exclude "*/.exp/*" \