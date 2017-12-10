REM #! /bin/bash
REM #
REM # Find out where we are and change to ARToolKit5 root.
REM #
REM OURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REM cd "${OURDIR}/../../"
REM echo "Building archive from directory \"$PWD\"."
REM #
REM # Give the archive a sensible name.
REM #


REM VERSION=`sed -En -e 's/.*AR_HEADER_VERSION_STRING[[:space:]]+"([0-9]+\.[0-9]+(\.[0-9]+)*)".*/\1/p' include/AR/config.h.in`
SET VERSION=5.4
REM #
REM # Build the archive.
REM # Exclude: build files and directories, version control info,
REM # ARToolKit settings files which don't carry over.
REM #
SET BOM="share/packaging/ARToolKit5-bin-bom-win32"
SET PACKAGE_NAME="ARToolKit5-bin-${VERSION}-%OS%-%PROCESSOR_ARCHITECTURE%.zip"

SET PACKAGE="../${PACKAGE_NAME}"

REM export PACKAGE_NAME

del /F "$PACKAGE"
zip -r -MM "$PACKAGE" . \
    -i@share/packaging/ARToolKit5-bin-bom-win32 \
    --exclude "*/.git/*" \
    --exclude "*/.DS_Store" \
    --exclude "*/.metadata/*" \
    --exclude "*/gen/*" \
    --exclude "*/objs/*" \
    --exclude "*/.exp/*" \