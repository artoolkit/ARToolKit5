#! /bin/bash
#
# Find out where we are and change to ARToolKit5-Android root.
#
OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${OURDIR}/../../"
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

echo "Do you want to copy the utilities from WINDOWS ARToolKit5 folder? (y or n)"
echo -n "Enter : "
read ANS
if [ "$ANS" = "y" ]
then
COPYUTILSWIN="y"
elif [ "$ANS" = "n" ]
then
COPYUTILSWIN="n"
else
echo "Please enter y or n."
exit 0
fi

if [ "$COPYUTILS" = "y" ]
then
cp -v \
    ../ARToolKit5/bin/check_id \
    ../ARToolKit5/bin/checkResolution \
    ../ARToolKit5/bin/dispFeatureSet \
    ../ARToolKit5/bin/dispImageSet \
    ../ARToolKit5/bin/genMarkerSet \
    ../ARToolKit5/bin/genTexData \
    ../ARToolKit5/bin/mk_patt \
    bin/
fi

if [ "$COPYUTILSWIN" = "y" ]
then
cp -v \
    /Volumes/C/Projects/ARToolKit5/bin/ARvideo.dll \
    /Volumes/C/Projects/ARToolKit5/bin/check_id.exe \
    /Volumes/C/Projects/ARToolKit5/bin/checkResolution.exe \
    /Volumes/C/Projects/ARToolKit5/bin/dispFeatureSet.exe \
    /Volumes/C/Projects/ARToolKit5/bin/dispImageSet.exe \
    /Volumes/C/Projects/ARToolKit5/bin/genMarkerSet.exe \
    /Volumes/C/Projects/ARToolKit5/bin/genTexData.exe \
    /Volumes/C/Projects/ARToolKit5/bin/mk_patt.exe \
    bin/
fi

#
# Give the archive a sensible name.
#
OS=`uname -s`
ARCH=`uname -m`
SED=/tmp/SED.$$
trap "rm -f $SED; exit 0" 0 1 2 3 15
VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' include/AR/config.h`
#
# Build the archive.
# Exclude: build files and directories, version control info,
# ARToolKit settings files which don't carry over.
#
rm -f "../ARToolKit5-bin-${VERSION}-Android.zip"
zip -r -MM "../ARToolKit5-bin-${VERSION}-Android.zip" . \
    -i@share/packaging/ARToolKit5Android-bin-bom \
    --exclude "*/.svn/*" \
    --exclude "*/.DS_Store" \
    --exclude "*/.metadata/*" \
    --exclude "EclipseProjects/*/bin/*" \
    --exclude "EclipseProjects/*/obj/*" \
    --exclude "*/gen/*" \
    --exclude "*/objs/*" \
    --exclude "AndroidStudioProjects/*/*.iml" \
    --exclude "AndroidStudioProjects/.idea/*" \
    --exclude "AndroidStudioProjects/*/.idea/*" \
    --exclude "AndroidStudioProjects/*/build/*" \
    --exclude "AndroidStudioProjects/*/.gradle/*" \

