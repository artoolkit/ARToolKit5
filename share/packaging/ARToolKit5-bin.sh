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
OS=`uname -s`
ARCH=`uname -m`
if [ "$OS" = "Darwin" ]
then
	OS='OSX'
	ARCH='Universal'
fi
VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' include/AR/config.h.in`
#
# Build the archive.
# Exclude: build files and directories, version control info,
# ARToolKit settings files which don't carry over.
#
if [ "$OS" = "OSX" ]
then
	BOM="share/packaging/ARToolKit5-bin-bom-OSX"
	PACKAGE="../ARToolKit5-bin-${VERSION}-${OS}.tar.gz"
else
	BOM="share/packaging/ARToolKit5-bin-bom"
	PACKAGE="../ARToolKit5-bin-${VERSION}-${OS}-${ARCH}.tar.gz"
fi

tar czvf "$PACKAGE" \
	-T "$BOM" \
	--exclude "*/.svn" \
	--exclude "*.o" \
	--exclude "Makefile" \
	--exclude "build" \
	--exclude "*.mode1*" \
	--exclude "*.pbxuser" \
	--exclude ".DS_Store" \
	--exclude "xcuserdata" \
    --exclude "*.xccheckout" \
    --exclude "*.xcscmblueprint" \

