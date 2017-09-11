/*
 *  checkResolution.c
 *  ARToolKit5
 *
 *  Check required resolution of texture data for a range of distances, given supplied camera parameters.
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <AR2/template.h>
#include <AR2/util.h>

static char                *cpara = NULL;
static char                 cparaDefault[] = "../share/checkResolution/Data/camera_para.dat";
static int                  xsize = -1;
static int                  ysize = -1;


static void          usage(char *com);
static void          init(int argc, char *argv[]);


int main(int argc, char *argv[])
{
    ARParam             cparam;
    //ARParamLT          *cparamLT;
    float               trans[3][4];
    float               pos[2];
    float               dpi[2];
    //char                name[1024], ext[1024];
    int                 i, j;
    float               z;

    init(argc, argv);
    
    if (!cpara) cpara = cparaDefault;
    //ar2UtilDivideExt( cpara, name, ext );

	// Load the camera parameters, resize for the window and init.
    //if( arParamLoad(name, ext, 1, &cparam) < 0 )
    if (arParamLoad(cpara, 1, &cparam) < 0) {
		ARLOGe("setupCamera(): Error loading parameter file %s for camera.\n", cpara);
        exit(-1);
    }
    if (xsize != -1 && ysize != -1 && (cparam.xsize != xsize || cparam.ysize != ysize)) {
        ARLOG("*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
        arParamChangeSize(&cparam, xsize, ysize, &cparam);
    }
    ARLOG("*** Camera Parameter ***\n");
    arParamDisp(&cparam);

    //if ((cparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
    //    ARLOGe("setupCamera(): Error: arParamLTCreate.\n");
    //    exit(-1);
    //}

    pos[0] = 0.0;
    pos[1] = 0.0;
    for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) trans[j][i] = ((i == j) ? 1.0 : 0.0);

    for(i = 10; i <= 1000; i*=10 ) {
        for(j = 1; j < 10; j++) {
            z = j*i;
            trans[2][3] = z;
            ar2GetResolution2( &cparam, trans, pos, dpi );
            ARLOG("Distance: %f [mm] --> Resolution = %10.5f, %10.5f [DPI]\n", z, dpi[0], dpi[1]);
        }
    }
    
    return (0);
}

static void usage( char *com )
{
    ARLOG("Usage: %s [options] <camera parameter>\n", com);
    ARLOG("  -width=w: scale the camera parameter to width w.\n");
    ARLOG("  -height=h: scale the camera parameter to height h.\n");
    ARLOG("  --cpara <camera parameter file>\n");
    ARLOG("  -cpara=<camera parameter file>\n");
    ARLOG("  -h -help --help: show this message\n");
    exit(0);
}

static void init(int argc, char *argv[])
{
    int                i;
    int                gotTwoPartOption;
    
    i = 1; // argv[0] is name of app, so start at 1.
    while (i < argc) {
        gotTwoPartOption = FALSE;
        // Look for two-part options first.
        if ((i + 1) < argc) {
            if (strcmp(argv[i], "--cpara") == 0) {
                i++;
                cpara = argv[i];
                gotTwoPartOption = TRUE;
            }
        }
        if (!gotTwoPartOption) {
            // Look for single-part options.
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
                ARLOG("%s version %s\n", argv[0], AR_HEADER_VERSION_STRING);
                exit(0);
            } else if( strncmp(argv[i], "-loglevel=", 10) == 0 ) {
                if (strcmp(&(argv[i][10]), "DEBUG") == 0) arLogLevel = AR_LOG_LEVEL_DEBUG;
                else if (strcmp(&(argv[i][10]), "INFO") == 0) arLogLevel = AR_LOG_LEVEL_INFO;
                else if (strcmp(&(argv[i][10]), "WARN") == 0) arLogLevel = AR_LOG_LEVEL_WARN;
                else if (strcmp(&(argv[i][10]), "ERROR") == 0) arLogLevel = AR_LOG_LEVEL_ERROR;
                else usage(argv[0]);
            } else if( strncmp(argv[i], "-width=", 7) == 0 ) {
                if( sscanf(&(argv[i][7]), "%d", &xsize) != 1 ) usage(argv[0]);
                if( xsize <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-height=", 8) == 0 ) {
                if( sscanf(&(argv[i][8]), "%d", &ysize) != 1 ) usage(argv[0]);
                if( ysize <= 0 ) usage(argv[0]);
            } else if( strncmp(argv[i], "-cpara=", 7) == 0 ) {
                cpara = &(argv[i][7]);
            } else {
                if (!cpara) cpara = argv[i];
                else {
                    ARLOGe("Error: invalid command line argument '%s'.\n", argv[i]);
                    usage(argv[0]);
                }
            }
        }
        i++;
    }
}

