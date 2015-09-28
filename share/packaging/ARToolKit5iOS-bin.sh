#! /bin/bash
#
# Find out where we are and change to ARToolKit5iOS root.
#
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${OURDIR}../../"
echo "Building archive from directory \"$PWD\"."

echo "Do you want to copy the utilities from ARToolKit5 folder? (y or n)"
echo -n "Enter : "
read ANS
if [ "$ANS" = "y" ]
then
COPYUTILS="y"
elif [ "$ANS" = "n" ]
then
COPYUTILS="n"
else
echo "Please enter y or n."
exit 0
fi

if [ "$COPYUTILS" = "y" ]
then
cp -v \
    ../ARToolKit5/bin/checkResolution \
    ../ARToolKit5/bin/check_id \
    ../ARToolKit5/bin/dispFeatureSet \
    ../ARToolKit5/bin/dispImageSet \
    ../ARToolKit5/bin/genMarkerSet \
    ../ARToolKit5/bin/genTexData \
    ../ARToolKit5/bin/mk_patt \
    bin/
fi

#
# Give the archive a sensible name.
#
OS=`uname -s`
ARCH=`uname -m`
VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' include/AR/config.h.in`
#
# Build the archive.
# Exclude: build files and directories, version control info,
# ARToolKit5iOS settings files which don't carry over.
#
tar czvf "../ARToolKit5-bin-${VERSION}-iOS.tar.gz" \
	-T share/packaging/ARToolKit5iOS-bin-bom \
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

