/*
 *  kpmUtil.cpp
 *  libKPM
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not compile, install, use, or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive,
 *  non-transferable license, under Daqri's copyrights in this original Daqri
 *  software (the "Daqri Software"), to compile, install and execute Daqri Software
 *  exclusively in conjunction with the ARToolKit software development kit version 5.2
 *  ("ARToolKit"). The allowed usage is restricted exclusively to the purposes of
 *  two-dimensional surface identification and camera pose extraction and initialisation,
 *  provided that applications involving automotive manufacture or operation, military,
 *  and mobile mapping are excluded.
 *
 *  You may reproduce and redistribute the Daqri Software in source and binary
 *  forms, provided that you redistribute the Daqri Software in its entirety and
 *  without modifications, and that you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <AR/ar.h>
#include <AR/icp.h>
#include <KPM/kpm.h>
#include <KPM/kpmType.h>
#include <KPM/surfSub.h>

static ARUint8 *genBWImageFull      ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageHalf      ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageOneThird  ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageTwoThird  ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize );
static ARUint8 *genBWImageQuart     ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize );


static int    kpmUtilGetInitPoseHomography( float *sCoord, float *wCoord, int num, float initPose[3][4] );


int kpmUtilGetCorner( ARUint8 *inImage, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int procMode, int maxPointNum, CornerPoints *cornerPoints )
{
    ARUint8          *inImageBW;
    int               xsize2, ysize2;
    int               cornerNum;
    int               i;

    inImageBW = kpmUtilGenBWImage( inImage, pixFormat, xsize, ysize, procMode, &xsize2, &ysize2 );
    if( inImageBW == NULL ) return -1;

    SurfSubHandleT   *surfHandle;
    surfHandle = surfSubCreateHandle(xsize2, ysize2, AR_PIXEL_FORMAT_MONO);
    if (!surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        return -1;
    }
    surfSubSetMaxPointNum( surfHandle, maxPointNum );
    surfSubExtractFeaturePoint( surfHandle, inImageBW, NULL, 0 );

    cornerNum = surfSubGetFeaturePointNum( surfHandle );
    if( procMode == KpmProcFullSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
            cornerPoints->pt[i].x = (int)x;
            cornerPoints->pt[i].y = (int)y;
        }
    }
    else if( procMode == KpmProcTwoThirdSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
            cornerPoints->pt[i].x = (int)(x * 1.5f);
            cornerPoints->pt[i].y = (int)(y * 1.5f);
        }
    }
    else if( procMode == KpmProcHalfSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
            cornerPoints->pt[i].x = (int)(x * 2.0f);
            cornerPoints->pt[i].y = (int)(y * 2.0f);
        }
    }
    else if( procMode == KpmProcOneThirdSize ) {
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
            cornerPoints->pt[i].x = (int)(x * 3.0f);
            cornerPoints->pt[i].y = (int)(y * 3.0f);
        }
    }
    else {      
        for( i = 0; i < cornerNum; i++ ) {
            float  x, y;
            surfSubGetFeaturePosition( surfHandle, i, &x, &y );
            cornerPoints->pt[i].x = (int)(x * 4.0f);
            cornerPoints->pt[i].y = (int)(y * 4.0f);
        }
    }
    cornerPoints->num = cornerNum;

    free( inImageBW );
    surfSubDeleteHandle( &surfHandle );

    return 0;
}


ARUint8 *kpmUtilGenBWImage( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int procMode, int *newXsize, int *newYsize )
{
    if( procMode == KpmProcFullSize ) {
        return genBWImageFull( image, pixFormat, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcTwoThirdSize ) {
        return genBWImageTwoThird( image, pixFormat, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcHalfSize ) {
        return genBWImageHalf( image, pixFormat, xsize, ysize, newXsize, newYsize );
    }
    else if( procMode == KpmProcOneThirdSize ) {
        return genBWImageOneThird( image, pixFormat, xsize, ysize, newXsize, newYsize );
    }
    else {
        return genBWImageQuart( image, pixFormat, xsize, ysize, newXsize, newYsize );
    }
}

int kpmUtilGetPose( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{
    ICPHandleT    *icpHandle;
    ICPDataT       icpData;
    ICP2DCoordT   *sCoord;
    ICP3DCoordT   *wCoord;
    ARdouble       initMatXw2Xc[3][4];
    ARdouble       err;
    int            i;

    if( matchData->num < 4 ) return -1;

    arMalloc( sCoord, ICP2DCoordT, matchData->num );
    arMalloc( wCoord, ICP3DCoordT, matchData->num );

    for( i = 0; i < matchData->num; i++ ) {
        sCoord[i].x = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i].y = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i].x = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.x;
        wCoord[i].y = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.y;
        wCoord[i].z = 0.0;
        //printf("%3d: (%f %f) - (%f %f)\n", i, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
    }

    icpData.num = i;
    icpData.screenCoord = &sCoord[0];
    icpData.worldCoord  = &wCoord[0];

    if( icpGetInitXw2Xc_from_PlanarData( cparamLT->param.mat, sCoord, wCoord, matchData->num, initMatXw2Xc ) < 0 ) {
        //printf("Error!! at icpGetInitXw2Xc_from_PlanarData.\n");
        free( sCoord );
        free( wCoord );
        return -1;
    }
/*
    printf("--- Init pose ---\n");
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  printf(" %8.3f", initMatXw2Xc[j][i]);
        printf("\n"); 
    } 
*/
    if( (icpHandle = icpCreateHandle( cparamLT->param.mat )) == NULL ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
#if 0
    if( icpData.num > 10 ) {
        icpSetInlierProbability( icpHandle, 0.7 );
        if( icpPointRobust( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
    else {
        if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
#else
#  ifdef ARDOUBLE_IS_FLOAT
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
#  else
    ARdouble camPosed[3][4];
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPosed, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) camPose[r][c] = (float)camPosed[r][c];
#  endif
#endif
    icpDeleteHandle( &icpHandle );

/*
    printf("error = %f\n", err);
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  printf(" %8.3f", camPose[j][i]);
        printf("\n"); 
    } 
    if( err > 10.0f ) {
        for( i = 0; i < matchData->num; i++ ) {
            printf("%d\t%f\t%f\t%f\t%f\n", i+1, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
        }
    }
*/


    free( sCoord );
    free( wCoord );

    *error = (float)err;
    if( *error > 10.0f ) return -1;

    return 0;
}


int kpmUtilGetPose2( ARParamLT *cparamLT, KpmMatchResult *matchData, KpmRefDataSet *refDataSet, int *redDataIndex, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{
    ICPHandleT    *icpHandle;
    ICPDataT       icpData;
    ICP2DCoordT   *sCoord;
    ICP3DCoordT   *wCoord;
    ARdouble       initMatXw2Xc[3][4];
    ARdouble       err;
    int            i;
    
    if( matchData->num < 4 ) return -1;
    
    arMalloc( sCoord, ICP2DCoordT, matchData->num );
    arMalloc( wCoord, ICP3DCoordT, matchData->num );
    
    for( i = 0; i < matchData->num; i++ ) {
        sCoord[i].x = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i].y = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i].x = refDataSet->refPoint[redDataIndex[matchData->match[i].refIndex]].coord3D.x;
        wCoord[i].y = refDataSet->refPoint[redDataIndex[matchData->match[i].refIndex]].coord3D.y;
        wCoord[i].z = 0.0;
    }
    
    icpData.num = i;
    icpData.screenCoord = &sCoord[0];
    icpData.worldCoord  = &wCoord[0];
    
    if( icpGetInitXw2Xc_from_PlanarData( cparamLT->param.mat, sCoord, wCoord, matchData->num, initMatXw2Xc ) < 0 ) {
        //ARLOGe("Error!! at icpGetInitXw2Xc_from_PlanarData.\n");
        free( sCoord );
        free( wCoord );
        return -1;
    }
/*
    ARLOG("--- Init pose ---\n");
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  ARLOG(" %8.3f", initMatXw2Xc[j][i]);
        ARLOG("\n"); 
    } 
*/
    if( (icpHandle = icpCreateHandle( cparamLT->param.mat )) == NULL ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
#if 0
    if( icpData.num > 10 ) {
        icpSetInlierProbability( icpHandle, 0.7 );
        if( icpPointRobust( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
    else {
        if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
            ARLOGe("Error!! at icpPoint.\n");
            free( sCoord );
            free( wCoord );
            icpDeleteHandle( &icpHandle );
            return -1;
        }
    }
#else
#  ifdef ARDOUBLE_IS_FLOAT
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPose, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
#  else
    ARdouble camPosed[3][4];
    if( icpPoint( icpHandle, &icpData, initMatXw2Xc, camPosed, &err ) < 0 ) {
        //ARLOGe("Error!! at icpPoint.\n");
        free( sCoord );
        free( wCoord );
        icpDeleteHandle( &icpHandle );
        return -1;
    }
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) camPose[r][c] = (float)camPosed[r][c];
#  endif
#endif
    icpDeleteHandle( &icpHandle );

/*
    ARLOG("error = %f\n", err);
    for( int j = 0; j < 3; j++ ) { 
        for( i = 0; i < 4; i++ )  ARLOG(" %8.3f", camPose[j][i]);
        ARLOG("\n"); 
    } 
    if( err > 10.0 ) {
        for( i = 0; i < matchData->num; i++ ) {
            ARLOG("%d\t%f\t%f\t%f\t%f\n", i+1, sCoord[i].x, sCoord[i].y, wCoord[i].x, wCoord[i].y);
        }
    }
*/


    free( sCoord );
    free( wCoord );

    *error = (float)err;
    if( *error > 10.0f ) return -1;

    return 0;
}

int kpmUtilGetPoseHomography( KpmMatchResult *matchData, KpmRefDataSet *refDataSet, KpmInputDataSet *inputDataSet, float  camPose[3][4], float  *error )
{   
    float   *sCoord;
    float   *wCoord;
    float    initPose[3][4];
    int      num;
    int      i;
    
    if( matchData->num < 4 ) return -1;
    num = matchData->num;
    
    arMalloc( sCoord, float, num*2 );
    arMalloc( wCoord, float, num*2 );
    
    for( i = 0; i < num; i++ ) {
        sCoord[i*2+0] = inputDataSet->coord[matchData->match[i].inIndex].x;
        sCoord[i*2+1] = inputDataSet->coord[matchData->match[i].inIndex].y;
        wCoord[i*2+0] = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.x;
        wCoord[i*2+1] = refDataSet->refPoint[matchData->match[i].refIndex].coord3D.y;
    }
    
    if( kpmUtilGetInitPoseHomography( sCoord, wCoord, num, initPose ) < 0 ) {
        free( sCoord );
        free( wCoord );
        return -1;
    }
    
    for( int j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ )  camPose[j][i] = initPose[j][i];
    }
    
    *error = 0.0;
    float  *p1 = sCoord;
    float  *p2 = wCoord;
    for( i = 0; i < num; i++ ) {
        float  x, y, w;
        x = camPose[0][0] * *p2 + camPose[0][1] * *(p2+1) + camPose[0][3];
        y = camPose[1][0] * *p2 + camPose[1][1] * *(p2+1) + camPose[1][3];
        w = camPose[2][0] * *p2 + camPose[2][1] * *(p2+1) + camPose[2][3];
        if( w == 0.0 ) {
            free( sCoord );
            free( wCoord );
            return -1;
        }
        x /= w;
        y /= w;
        *error += (*p1 - x)*(*p1 - x) + (*(p1+1) - y)*(*(p1+1) - y);
    }
    *error /= num;
    
    free( sCoord );
    free( wCoord );
    if( *error > 10.0 ) return -1;

    return 0;
}

static ARUint8 *genBWImageFull( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage, *p;
    int       xsize2, ysize2;
    int       i, j;

    *newXsize = xsize2 = xsize;
    *newYsize = ysize2 = ysize;
    arMalloc( newImage, ARUint8, xsize*ysize );
    
    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*image + (int)*(image+1) + (int)*(image+2) ) / 3;
                image+=3;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*image + (int)*(image+1) + (int)*(image+2) ) / 3;
                image+=4;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(image+1) + (int)*(image+2) + (int)*(image+3) ) / 3;
                image+=4;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = *(image++);
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = *(image+1);
                image+=2;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        p = newImage;
        for( j = 0; j < ysize2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = *image;
                image+=2;
            }
        }
    }

    return newImage;
}

static ARUint8 *genBWImageHalf( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/2;
    *newYsize = ysize2 = ysize/2;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*3*(j*2+0);
            p2 = image + xsize*3*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+3) + (int)*(p1+4) + (int)*(p1+5)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5) ) / 12;
                p1+=6;
                p2+=6;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*2+0);
            p2 = image + xsize*4*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+4) + (int)*(p1+5) + (int)*(p1+6)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6) ) / 12;
                p1+=8;
                p2+=8;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*2+0);
            p2 = image + xsize*4*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3)
                         + (int)*(p1+5) + (int)*(p1+6) + (int)*(p1+7)
                         + (int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3)
                         + (int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7) ) / 12;
                p1+=8;
                p2+=8;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*(j*2+0);
            p2 = image + xsize*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1)
                         + (int)*(p2+0) + (int)*(p2+1) ) / 4;
                p1+=2;
                p2+=2;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*2+0);
            p2 = image + xsize*2*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+3)
                         + (int)*(p2+1) + (int)*(p2+3) ) / 4;
                p1+=4;
                p2+=4;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*2+0);
            p2 = image + xsize*2*(j*2+1);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+2)
                         + (int)*(p2+0) + (int)*(p2+2) ) / 4;
                p1+=4;
                p2+=4;
            }
        }
    }

    return newImage;
}

static ARUint8 *genBWImageQuart( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2, *p3, *p4;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/4;
    *newYsize = ysize2 = ysize/4;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );
    
    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*3*(j*4+0);
            p2 = image + xsize*3*(j*4+1);
            p3 = image + xsize*3*(j*4+2);
            p4 = image + xsize*3*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+3) + (int)*(p1+4) + (int)*(p1+5)
                         + (int)*(p1+6) + (int)*(p1+7) + (int)*(p1+8)
                         + (int)*(p1+9) + (int)*(p1+10) + (int)*(p1+11)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5)
                         + (int)*(p2+6) + (int)*(p2+7) + (int)*(p2+8)
                         + (int)*(p2+9) + (int)*(p2+10) + (int)*(p2+11)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2)
                         + (int)*(p3+3) + (int)*(p3+4) + (int)*(p3+5)
                         + (int)*(p3+6) + (int)*(p3+7) + (int)*(p3+8)
                         + (int)*(p3+9) + (int)*(p3+10) + (int)*(p3+11)
                         + (int)*(p4+0) + (int)*(p4+1) + (int)*(p4+2)
                         + (int)*(p4+3) + (int)*(p4+4) + (int)*(p4+5)
                         + (int)*(p4+6) + (int)*(p4+7) + (int)*(p4+8)
                         + (int)*(p4+9) + (int)*(p4+10) + (int)*(p4+11)) / 48;
                p1+=12;
                p2+=12;
                p3+=12;
                p4+=12;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*4+0);
            p2 = image + xsize*4*(j*4+1);
            p3 = image + xsize*4*(j*4+2);
            p4 = image + xsize*4*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+4) + (int)*(p1+5) + (int)*(p1+6)
                         + (int)*(p1+8) + (int)*(p1+9) + (int)*(p1+10)
                         + (int)*(p1+12) + (int)*(p1+13) + (int)*(p1+14)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6)
                         + (int)*(p2+8) + (int)*(p2+9) + (int)*(p2+10)
                         + (int)*(p2+12) + (int)*(p2+13) + (int)*(p2+14)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2)
                         + (int)*(p3+4) + (int)*(p3+5) + (int)*(p3+6)
                         + (int)*(p3+8) + (int)*(p3+9) + (int)*(p3+10)
                         + (int)*(p3+12) + (int)*(p3+13) + (int)*(p3+14)
                         + (int)*(p4+0) + (int)*(p4+1) + (int)*(p4+2)
                         + (int)*(p4+4) + (int)*(p4+5) + (int)*(p4+6)
                         + (int)*(p4+8) + (int)*(p4+9) + (int)*(p4+10)
                         + (int)*(p4+12) + (int)*(p4+13) + (int)*(p4+14)) / 48;
                p1+=16;
                p2+=16;
                p3+=16;
                p4+=16;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*4+0);
            p2 = image + xsize*4*(j*4+1);
            p3 = image + xsize*4*(j*4+2);
            p4 = image + xsize*4*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3)
                         + (int)*(p1+5) + (int)*(p1+6) + (int)*(p1+7)
                         + (int)*(p1+9) + (int)*(p1+10) + (int)*(p1+11)
                         + (int)*(p1+13) + (int)*(p1+14) + (int)*(p1+15)
                         + (int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3)
                         + (int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7)
                         + (int)*(p2+9) + (int)*(p2+10) + (int)*(p2+11)
                         + (int)*(p2+13) + (int)*(p2+14) + (int)*(p2+15)
                         + (int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3)
                         + (int)*(p3+5) + (int)*(p3+6) + (int)*(p3+7)
                         + (int)*(p3+9) + (int)*(p3+10) + (int)*(p3+11)
                         + (int)*(p3+13) + (int)*(p3+14) + (int)*(p3+15)
                         + (int)*(p4+1) + (int)*(p4+2) + (int)*(p4+3)
                         + (int)*(p4+5) + (int)*(p4+6) + (int)*(p4+7)
                         + (int)*(p4+9) + (int)*(p4+10) + (int)*(p4+11)
                         + (int)*(p4+13) + (int)*(p4+14) + (int)*(p4+15)) / 48;
                p1+=16;
                p2+=16;
                p3+=16;
                p4+=16;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*(j*4+0);
            p2 = image + xsize*(j*4+1);
            p3 = image + xsize*(j*4+2);
            p4 = image + xsize*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3)
                         + (int)*(p4+0) + (int)*(p4+1) + (int)*(p4+2) + (int)*(p4+3)) / 16;
                p1+=4;
                p2+=4;
                p3+=4;
                p4+=4;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*4+0);
            p2 = image + xsize*2*(j*4+1);
            p3 = image + xsize*2*(j*4+2);
            p4 = image + xsize*2*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+3) + (int)*(p1+5) + (int)*(p1+7)
                         + (int)*(p2+1) + (int)*(p2+3) + (int)*(p2+5) + (int)*(p2+7)
                         + (int)*(p3+1) + (int)*(p3+3) + (int)*(p3+5) + (int)*(p3+7)
                         + (int)*(p4+1) + (int)*(p4+3) + (int)*(p4+5) + (int)*(p4+7)) / 16;
                p1+=8;
                p2+=8;
                p3+=8;
                p4+=8;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*4+0);
            p2 = image + xsize*2*(j*4+1);
            p3 = image + xsize*2*(j*4+2);
            p4 = image + xsize*2*(j*4+3);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+2) + (int)*(p1+4) + (int)*(p1+6)
                         + (int)*(p2+0) + (int)*(p2+2) + (int)*(p2+4) + (int)*(p2+6)
                         + (int)*(p3+0) + (int)*(p3+2) + (int)*(p3+4) + (int)*(p3+6)
                         + (int)*(p4+0) + (int)*(p4+2) + (int)*(p4+4) + (int)*(p4+6)) / 16;
                p1+=8;
                p2+=8;
                p3+=8;
                p4+=8;
            }
        }
    }

    return newImage;
}


static ARUint8 *genBWImageOneThird( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *p, *p1, *p2, *p3;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/3;
    *newYsize = ysize2 = ysize/3;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*3*(j*3+0);
            p2 = image + xsize*3*(j*3+1);
            p3 = image + xsize*3*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+3) + (int)*(p1+4) + (int)*(p1+5)
                         + (int)*(p1+6) + (int)*(p1+7) + (int)*(p1+8)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5)
                         + (int)*(p2+6) + (int)*(p2+7) + (int)*(p2+8)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2)
                         + (int)*(p3+3) + (int)*(p3+4) + (int)*(p3+5)
                         + (int)*(p3+6) + (int)*(p3+7) + (int)*(p3+8) ) / 27;
                p1+=9;
                p2+=9;
                p3+=9;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*3+0);
            p2 = image + xsize*4*(j*3+1);
            p3 = image + xsize*4*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p1+4) + (int)*(p1+5) + (int)*(p1+6)
                         + (int)*(p1+8) + (int)*(p1+9) + (int)*(p1+10)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6)
                         + (int)*(p2+8) + (int)*(p2+9) + (int)*(p2+10)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2)
                         + (int)*(p3+4) + (int)*(p3+5) + (int)*(p3+6)
                         + (int)*(p3+8) + (int)*(p3+9) + (int)*(p3+10) ) / 27;
                p1+=12;
                p2+=12;
                p3+=12;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*4*(j*3+0);
            p2 = image + xsize*4*(j*3+1);
            p3 = image + xsize*4*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3)
                         + (int)*(p1+5) + (int)*(p1+6) + (int)*(p1+7)
                         + (int)*(p1+9) + (int)*(p1+10) + (int)*(p1+11)
                         + (int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3)
                         + (int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7)
                         + (int)*(p2+9) + (int)*(p2+10) + (int)*(p2+11)
                         + (int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3)
                         + (int)*(p3+5) + (int)*(p3+6) + (int)*(p3+7)
                         + (int)*(p3+9) + (int)*(p3+10) + (int)*(p3+11) ) / 27;
                p1+=12;
                p2+=12;
                p3+=12;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*(j*3+0);
            p2 = image + xsize*(j*3+1);
            p3 = image + xsize*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2)
                         + (int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2)
                         + (int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2) ) / 9;
                p1+=3;
                p2+=3;
                p3+=3;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*3+0);
            p2 = image + xsize*2*(j*3+1);
            p3 = image + xsize*2*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+1) + (int)*(p1+3) + (int)*(p1+5)
                         + (int)*(p2+1) + (int)*(p2+3) + (int)*(p2+5)
                         + (int)*(p3+1) + (int)*(p3+3) + (int)*(p3+5) ) / 9;
                p1+=6;
                p2+=6;
                p3+=6;
            }
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        p  = newImage;
        for( j = 0; j < ysize2; j++ ) {
            p1 = image + xsize*2*(j*3+0);
            p2 = image + xsize*2*(j*3+1);
            p3 = image + xsize*2*(j*3+2);
            for( i = 0; i < xsize2; i++ ) {
                *(p++) = ( (int)*(p1+0) + (int)*(p1+2) + (int)*(p1+4)
                         + (int)*(p2+0) + (int)*(p2+2) + (int)*(p2+4)
                         + (int)*(p3+0) + (int)*(p3+2) + (int)*(p3+4) ) / 9;
                p1+=6;
                p2+=6;
                p3+=6;
            }
        }
    }

    return newImage;
}

static ARUint8 *genBWImageTwoThird  ( ARUint8 *image, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, int *newXsize, int *newYsize )
{
    ARUint8  *newImage;
    ARUint8  *q1, *q2, *p1, *p2, *p3;
    int       xsize2, ysize2;
    int       i, j;
    
    *newXsize = xsize2 = xsize/3*2;
    *newYsize = ysize2 = ysize/3*2;
    arMalloc( newImage, ARUint8, xsize2*ysize2 );

    if( pixFormat == AR_PIXEL_FORMAT_RGB || pixFormat == AR_PIXEL_FORMAT_BGR ) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*3*(j*3+0);
            p2 = image + xsize*3*(j*3+1);
            p3 = image + xsize*3*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( ((int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2))
                          + ((int)*(p1+3) + (int)*(p1+4) + (int)*(p1+5))/2
                          + ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/2
                          + ((int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5))/4 ) * 4/27;
                *(q2++) = ( ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/2
                          + ((int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5))/4
                          + ((int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2))
                          + ((int)*(p3+3) + (int)*(p3+4) + (int)*(p3+5))/2 ) * 4/27;
                p1+=3;
                p2+=3;
                p3+=3;
                *(q1++) = ( ((int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2))/2
                          + ((int)*(p1+3) + (int)*(p1+4) + (int)*(p1+5))
                          + ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/4
                          + ((int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5))/2 ) * 4/27;
                *(q2++) = ( ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/4
                          + ((int)*(p2+3) + (int)*(p2+4) + (int)*(p2+5))/2
                          + ((int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2))/2
                          + ((int)*(p3+3) + (int)*(p3+4) + (int)*(p3+5))   ) * 4/27;

                p1+=6;
                p2+=6;
                p3+=6;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_RGBA || pixFormat == AR_PIXEL_FORMAT_BGRA ) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*4*(j*3+0);
            p2 = image + xsize*4*(j*3+1);
            p3 = image + xsize*4*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( ((int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2))
                          + ((int)*(p1+4) + (int)*(p1+5) + (int)*(p1+6))/2
                          + ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/2
                          + ((int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6))/4 ) * 4/27;
                *(q2++) = ( ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/2
                          + ((int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6))/4
                          + ((int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2))
                          + ((int)*(p3+4) + (int)*(p3+5) + (int)*(p3+6))/2 ) * 4/27;
                p1+=4;
                p2+=4;
                p3+=4;
                *(q1++) = ( ((int)*(p1+0) + (int)*(p1+1) + (int)*(p1+2))/2
                          + ((int)*(p1+4) + (int)*(p1+4) + (int)*(p1+4))
                          + ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/4
                          + ((int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6))/2 ) * 4/27;
                *(q2++) = ( ((int)*(p2+0) + (int)*(p2+1) + (int)*(p2+2))/4
                          + ((int)*(p2+4) + (int)*(p2+5) + (int)*(p2+6))/2
                          + ((int)*(p3+0) + (int)*(p3+1) + (int)*(p3+2))/2
                          + ((int)*(p3+4) + (int)*(p3+5) + (int)*(p3+6))   ) * 4/27;

                p1+=8;
                p2+=8;
                p3+=8;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_ABGR || pixFormat == AR_PIXEL_FORMAT_ARGB) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*4*(j*3+0);
            p2 = image + xsize*4*(j*3+1);
            p3 = image + xsize*4*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( ((int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3))
                          + ((int)*(p1+5) + (int)*(p1+6) + (int)*(p1+7))/2
                          + ((int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3))/2
                          + ((int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7))/4 ) * 4/27;
                *(q2++) = ( ((int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3))/2
                          + ((int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7))/4
                          + ((int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3))
                          + ((int)*(p3+5) + (int)*(p3+6) + (int)*(p3+7))/2 ) * 4/27;
                p1+=4;
                p2+=4;
                p3+=4;
                *(q1++) = ( ((int)*(p1+1) + (int)*(p1+2) + (int)*(p1+3))/2
                          + ((int)*(p1+5) + (int)*(p1+6) + (int)*(p1+7))
                          + ((int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3))/4
                          + ((int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7))/2 ) * 4/27;
                *(q2++) = ( ((int)*(p2+1) + (int)*(p2+2) + (int)*(p2+3))/4
                          + ((int)*(p2+5) + (int)*(p2+6) + (int)*(p2+7))/2
                          + ((int)*(p3+1) + (int)*(p3+2) + (int)*(p3+3))/2
                          + ((int)*(p3+5) + (int)*(p3+6) + (int)*(p3+7))   ) * 4/27;

                p1+=8;
                p2+=8;
                p3+=8;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_NV21 ) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*(j*3+0);
            p2 = image + xsize*(j*3+1);
            p3 = image + xsize*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( (int)*(p1+0)   + (int)*(p1+1)/2
                          + (int)*(p2+0)/2 + (int)*(p2+1)/4 ) *4/9;
                *(q2++) = ( (int)*(p2+0)/2 + (int)*(p2+1)/4
                          + (int)*(p3+0)   + (int)*(p3+1)/2 ) *4/9;
                p1++;
                p2++;
                p3++;
                *(q1++) = ( (int)*(p1+0)/2 + (int)*(p1+1)
                          + (int)*(p2+0)/4 + (int)*(p2+1)/2 ) *4/9;
                *(q2++) = ( (int)*(p2+0)/4 + (int)*(p2+1)/2
                          + (int)*(p3+0)/2 + (int)*(p3+1)   ) *4/9;
                p1+=2;
                p2+=2;
                p3+=2;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_2vuy ) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*2*(j*3+0);
            p2 = image + xsize*2*(j*3+1);
            p3 = image + xsize*2*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( (int)*(p1+1)   + (int)*(p1+3)/2
                          + (int)*(p2+1)/2 + (int)*(p2+3)/4 ) *4/9;
                *(q2++) = ( (int)*(p2+1)/2 + (int)*(p2+3)/4
                          + (int)*(p3+1)   + (int)*(p3+3)/2 ) *4/9;
                p1+=2;
                p2+=2;
                p3+=2;
                *(q1++) = ( (int)*(p1+1)/2 + (int)*(p1+3)
                          + (int)*(p2+1)/4 + (int)*(p2+3)/2 ) *4/9;
                *(q2++) = ( (int)*(p2+1)/4 + (int)*(p2+3)/2
                          + (int)*(p3+1)/2 + (int)*(p3+3)   ) *4/9;
                p1+=4;
                p2+=4;
                p3+=4;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }
    else if( pixFormat == AR_PIXEL_FORMAT_yuvs ) {
        q1  = newImage;
        q2  = newImage + xsize2;
        for( j = 0; j < ysize2/2; j++ ) {
            p1 = image + xsize*2*(j*3+0);
            p2 = image + xsize*2*(j*3+1);
            p3 = image + xsize*2*(j*3+2);
            for( i = 0; i < xsize2/2; i++ ) {
                *(q1++) = ( (int)*(p1+0)   + (int)*(p1+2)/2
                          + (int)*(p2+0)/2 + (int)*(p2+2)/4 ) *4/9;
                *(q2++) = ( (int)*(p2+0)/2 + (int)*(p2+2)/4
                          + (int)*(p3+0)   + (int)*(p3+2)/2 ) *4/9;
                p1+=2;
                p2+=2;
                p3+=2;
                *(q1++) = ( (int)*(p1+0)/2 + (int)*(p1+2)
                          + (int)*(p2+0)/4 + (int)*(p2+2)/2 ) *4/9;
                *(q2++) = ( (int)*(p2+0)/4 + (int)*(p2+2)/2
                          + (int)*(p3+0)/2 + (int)*(p3+2)   ) *4/9;
                p1+=4;
                p2+=4;
                p3+=4;
            }
            q1 += xsize2;
            q2 += xsize2;
        }
    }

    return newImage;
}

static int kpmUtilGetInitPoseHomography( float *sCoord, float *wCoord, int num, float initPose[3][4] )
{
    float  *A, *B;
    ARMatf  matA, matB;
    ARMatf *matAt, *matAtA, *matAtB, *matH;
    int     i;
    int     ret = 0;

    arMalloc( A, float, num*8*2 );
    arMalloc( B, float, num*2 );

    for( i = 0; i < num; i++ ) {
        A[i*16+ 0] = wCoord[i*2+0];
        A[i*16+ 1] = wCoord[i*2+1];
        A[i*16+ 2] = 1.0;
        A[i*16+ 3] = 0.0;
        A[i*16+ 4] = 0.0;
        A[i*16+ 5] = 0.0;
        A[i*16+ 6] = -sCoord[i*2+0]*wCoord[i*2+0];
        A[i*16+ 7] = -sCoord[i*2+0]*wCoord[i*2+1];
        A[i*16+ 8] = 0.0;
        A[i*16+ 9] = 0.0;
        A[i*16+10] = 0.0;
        A[i*16+11] = wCoord[i*2+0];
        A[i*16+12] = wCoord[i*2+1];
        A[i*16+13] = 1.0;
        A[i*16+14] = -sCoord[i*2+1]*wCoord[i*2+0];
        A[i*16+15] = -sCoord[i*2+1]*wCoord[i*2+1];
        B[i*2+0]   = sCoord[i*2+0];
        B[i*2+1]   = sCoord[i*2+1];
    }
    
    matA.row = num*2;
    matA.clm = 8;
    matA.m   = A;

    matB.row = num*2;
    matB.clm = 1;
    matB.m   = B;

    matAt = arMatrixAllocTransf( &matA );
    if( matAt == NULL ) {
        ret = -1;
        goto bail;
    }
    matAtA = arMatrixAllocMulf( matAt, &matA );
    if( matAtA == NULL ) {
        ret = -1;
        goto bail1;
    }
    matAtB = arMatrixAllocMulf( matAt, &matB );
    if( matAtB == NULL ) {
        ret = -1;
        goto bail2;
    }
    if( arMatrixSelfInvf(matAtA) < 0 ) {
        ret = -1;
        goto bail3;
    }

    matH = arMatrixAllocMulf( matAtA, matAtB );
    if( matH == NULL ) {
        ret = -1;
        goto bail3;
    }

    initPose[0][0] = matH->m[0];
    initPose[0][1] = matH->m[1];
    initPose[0][2] = 0.0;
    initPose[0][3] = matH->m[2];
    initPose[1][0] = matH->m[3];
    initPose[1][1] = matH->m[4];
    initPose[1][2] = 0.0;
    initPose[1][3] = matH->m[5];
    initPose[2][0] = matH->m[6];
    initPose[2][1] = matH->m[7];
    initPose[2][2] = 0.0;
    initPose[2][3] = 1.0;

    arMatrixFreef( matH );
bail3:
    arMatrixFreef( matAtB );
bail2:
    arMatrixFreef( matAtA );
bail1:
    arMatrixFreef( matAt );
bail:
    free(B);
    free(A);

    return (ret);
}
