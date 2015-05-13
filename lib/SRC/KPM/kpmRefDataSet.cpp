/*
 *  kpmRefDataSet.cpp
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
#include <stdint.h>
#include <AR/ar.h>
#include <KPM/kpm.h>
#include <KPM/kpmType.h>
#include <KPM/surfSub.h>
#include "kpmFopen.h"


int kpmGenRefDataSet ( ARUint8 *refImage, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, float dpi, int procMode, int compMode, int maxFeatureNum,
                       int pageNo, int imageNo, KpmRefDataSet **refDataSetPtr )
{
    ARUint8         *refImageBW;
    KpmRefDataSet   *refDataSet;
    ARUint8         *refImageBW2;
    ARUint8         *p1, *p2;
    int              xsize2, ysize2;
    int              i, j;

    if (!refDataSetPtr || !refImage) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr/refImage.\n");
        return (-1);
    }
    if (!xsize || !ysize || !dpi) {
        ARLOGe("kpmDeleteRefDataSet(): 0 xsize/ysize/dpi.\n");
        return (-1);
    }

    arMalloc( refDataSet, KpmRefDataSet, 1 );
    
    refDataSet->pageNum = 1; // I.e. number of pages = 1.
    arMalloc( refDataSet->pageInfo, KpmPageInfo, 1 );
    refDataSet->pageInfo[0].pageNo = pageNo;
    refDataSet->pageInfo[0].imageNum = 1; // I.e. number of images = 1.
    arMalloc( refDataSet->pageInfo[0].imageInfo, KpmImageInfo, 1 );
    refDataSet->pageInfo[0].imageInfo[0].imageNo = imageNo;
    refImageBW = kpmUtilGenBWImage( refImage, pixFormat, xsize, ysize, procMode, &xsize2, &ysize2 );
    refDataSet->pageInfo[0].imageInfo[0].width   = xsize2;
    refDataSet->pageInfo[0].imageInfo[0].height  = ysize2;

    if( compMode == KpmCompY ) {
        refImageBW2 = refImageBW;
        refImageBW = (ARUint8 *)malloc(sizeof(ARUint8)*xsize2*(ysize2/2));
        if( refImageBW == NULL ) exit(0);

        p1 = refImageBW;
        p2 = refImageBW2;
        for( j = 0; j < ysize2/2; j++ ) {
            for( i = 0; i < xsize2; i++ ) {
                *(p1++) = ( (int)*p2 + (int)*(p2+xsize2) ) / 2;
                p2++;
            }
            p2 += xsize2;
        }
        free( refImageBW2 );
    }

    SurfSubHandleT   *surfHandle;
    surfHandle = surfSubCreateHandle(xsize2, (compMode == KpmCompY)? ysize2/2: ysize2, AR_PIXEL_FORMAT_MONO);
    if (!surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        exit(-1);
    }
    surfSubSetMaxPointNum( surfHandle, maxFeatureNum );
    surfSubExtractFeaturePoint( surfHandle, refImageBW, NULL, 0 );

    refDataSet->num = surfSubGetFeaturePointNum( surfHandle );
    if( refDataSet->num != 0 ) {
        arMalloc( refDataSet->refPoint, KpmRefData, refDataSet->num );
        if( procMode == KpmProcFullSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {       
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x + 0.5f) / dpi * 25.4f;               // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-0.5f) - y) / dpi * 25.4f;       // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcTwoThirdSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {       
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*1.5f + 0.75f) / dpi * 25.4f;         // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-0.75f) - y*1.5f) / dpi * 25.4f; // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcHalfSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*2.0f + 1.0f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-1.0f) - y*2.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else if( procMode == KpmProcOneThirdSize ) {
            for( i = 0 ; i < refDataSet->num ; i++ ) {       
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*3.0f + 1.5f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-1.5f) - y*3.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
        else {
            for( i = 0 ; i < refDataSet->num ; i++ ) {
                float  x, y, *desc;
                desc = surfSubGetFeatureDescPtr( surfHandle, i );
                surfSubGetFeaturePosition( surfHandle, i, &x, &y );
                if( compMode == KpmCompY ) y *= 2.0f;
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    refDataSet->refPoint[i].featureVec.v[j] = desc[j];
                }
                refDataSet->refPoint[i].featureVec.l = surfSubGetFeatureSign( surfHandle, i );
                refDataSet->refPoint[i].coord2D.x = x;
                refDataSet->refPoint[i].coord2D.y = y;
                refDataSet->refPoint[i].coord3D.x = (x*4.0f + 2.0f) / dpi * 25.4f;          // millimetres.
                refDataSet->refPoint[i].coord3D.y = ((ysize-2.0f) - y*4.0f) / dpi * 25.4f;  // millimetres.
                refDataSet->refPoint[i].pageNo = pageNo;
                refDataSet->refPoint[i].refImageNo = imageNo;
            }
        }
    }
    else {
        refDataSet->refPoint = NULL;
    }
    free(refImageBW);
    surfSubDeleteHandle( &surfHandle );
    
    *refDataSetPtr = refDataSet;

    return 0;
}

int kpmAddRefDataSet ( ARUint8 *refImage, AR_PIXEL_FORMAT pixFormat, int xsize, int ysize, float  dpi, int procMode, int compMode, int maxFeatureNum,
                              int pageNo, int imageNo, KpmRefDataSet **refDataSetPtr )
{
    KpmRefDataSet  *refDataSetPtr2;
    int ret;

    ret =  kpmGenRefDataSet(refImage, pixFormat, xsize, ysize, dpi, procMode, compMode, maxFeatureNum, pageNo, imageNo, &refDataSetPtr2);
    if (ret < 0) {
        ARLOGe("Error while adding reference data set: kpmGenRefDataSet() failed.\n");
        return (ret);
    }

    ARLOGi("========= %d ===========\n", refDataSetPtr2->num);

    ret = kpmMergeRefDataSet( refDataSetPtr, &refDataSetPtr2 );
    if (ret < 0) {
        ARLOGe("Error while adding reference data set: kpmMergeRefDataSet() failed.\n");
    }
    return (ret);
}

int kpmMergeRefDataSet ( KpmRefDataSet **refDataSetPtr1, KpmRefDataSet **refDataSetPtr2 )
{
    KpmRefData    *refPoint;
    KpmPageInfo   *pageInfo;
    int            pageNum;
    int            imageNum;
    int            num1, num2, num3;
    int            i, j, k, l;

    if (!refDataSetPtr1 || !refDataSetPtr2) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr1/refDataSetPtr2.\n");
        return (-1);
    }

    if (!*refDataSetPtr1) {
        arMalloc( *refDataSetPtr1, KpmRefDataSet, 1 );
        (*refDataSetPtr1)->num          = 0;
        (*refDataSetPtr1)->refPoint     = NULL;
        (*refDataSetPtr1)->pageNum      = 0;
        (*refDataSetPtr1)->pageInfo     = NULL;
    }
    if (!*refDataSetPtr2) return 0;
    
    // Merge KpmRefData.
    num1 = (*refDataSetPtr1)->num;
    num2 = (*refDataSetPtr2)->num;
    arMalloc( refPoint, KpmRefData, num1+num2 );
    for( i = 0; i < num1; i++ ) {
        refPoint[i] = (*refDataSetPtr1)->refPoint[i];
    }
    for( i = 0; i < num2; i++ ) {
        refPoint[num1+i] = (*refDataSetPtr2)->refPoint[i];
    }
    if( (*refDataSetPtr1)->refPoint != NULL ) free((*refDataSetPtr1)->refPoint);
    (*refDataSetPtr1)->refPoint = refPoint;
    (*refDataSetPtr1)->num      = num1 + num2;
    
    // Allocate pageInfo for the combined sets.
    num1 = (*refDataSetPtr1)->pageNum;
    num2 = (*refDataSetPtr2)->pageNum;
    num3 = 0;
    for( i = 0; i < num2; i++ ) {
        for( j = 0; j < num1; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[i].pageNo == (*refDataSetPtr1)->pageInfo[j].pageNo ) {
                num3++; // count a duplicate.
                break;
            }
        }
    }
    pageNum = num1+num2-num3;
    arMalloc(pageInfo, KpmPageInfo, pageNum);
    
    for( i = 0; i < num1; i++ ) {
        
        pageInfo[i].pageNo = (*refDataSetPtr1)->pageInfo[i].pageNo;
        
        // Count the number of imageInfo records in the combined set for this pageNo.
        imageNum = (*refDataSetPtr1)->pageInfo[i].imageNum;
        for( j = 0; j < num2; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[j].pageNo == (*refDataSetPtr1)->pageInfo[i].pageNo ) {
                imageNum += (*refDataSetPtr2)->pageInfo[j].imageNum;
            }
        }
        arMalloc(pageInfo[i].imageInfo, KpmImageInfo, imageNum);
        
        // Copy the imageInfo records into the new set.
        l = (*refDataSetPtr1)->pageInfo[i].imageNum;
        for( j = 0; j < l; j++ ) {
            pageInfo[i].imageInfo[j] = (*refDataSetPtr1)->pageInfo[i].imageInfo[j];
        }
        for( j = 0; j < num2; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[j].pageNo == (*refDataSetPtr1)->pageInfo[i].pageNo ) {
                for( k = 0; k < (*refDataSetPtr2)->pageInfo[j].imageNum; k++ ) {
                    pageInfo[i].imageInfo[l+k] = (*refDataSetPtr2)->pageInfo[j].imageInfo[k];
                }
                break;
            }
        }
        pageInfo[i].imageNum = imageNum;
    }
    
    k = 0;
    for( i = 0; i < num2; i++ ) {
        for( j = 0; j < num1; j++ ) {
            if( (*refDataSetPtr2)->pageInfo[i].pageNo == (*refDataSetPtr1)->pageInfo[j].pageNo ) {
                k++; // count a duplicate.
                break;
            }
        }
        if( j < num1 ) continue;
        // If we get to here, we have a page from refDataSetPtr2 which doesn't
        // have the same pageNo as any page from refDataSetPtr1.
        pageInfo[num1+i-k].pageNo = (*refDataSetPtr2)->pageInfo[i].pageNo;
        imageNum = (*refDataSetPtr2)->pageInfo[i].imageNum;
        arMalloc(pageInfo[num1+i-k].imageInfo, KpmImageInfo, imageNum);
        for( j = 0; j < imageNum; j++ ) {
            pageInfo[num1+i-k].imageInfo[j] = (*refDataSetPtr2)->pageInfo[i].imageInfo[j];
        }
        pageInfo[num1+i-k].imageNum = imageNum;
    }

    if ((*refDataSetPtr1)->pageInfo) {
        for( i = 0; i < (*refDataSetPtr1)->pageNum; i++ ) {
            free( (*refDataSetPtr1)->pageInfo[i].imageInfo );
        }
        free((*refDataSetPtr1)->pageInfo);
    }
    (*refDataSetPtr1)->pageInfo = pageInfo;
    (*refDataSetPtr1)->pageNum  = pageNum;

    kpmDeleteRefDataSet(refDataSetPtr2);

    return 0;
}

int kpmDeleteRefDataSet( KpmRefDataSet **refDataSetPtr )
{
    if (!refDataSetPtr) {
        ARLOGe("kpmDeleteRefDataSet(): NULL refDataSetPtr.\n");
        return (-1);
    }
    if (!*refDataSetPtr) return 0; // OK to call on already deleted handle.

    if ((*refDataSetPtr)->refPoint) free((*refDataSetPtr)->refPoint);
    
    for(int i = 0; i < (*refDataSetPtr)->pageNum; i++ ) {
        free( (*refDataSetPtr)->pageInfo[i].imageInfo );
    }
    free( (*refDataSetPtr)->pageInfo );
    free( *refDataSetPtr );
    *refDataSetPtr = NULL;

    return 0;
}


int kpmSaveRefDataSet( const char *filename, const char *ext, KpmRefDataSet  *refDataSet )
{
    FILE   *fp;
    char    fmode[] = "wb";
    int     i, j;

    if (!filename || !refDataSet) {
        ARLOGe("kpmSaveRefDataSet(): NULL filename/refDataSet.\n");
        return (-1);
    }

    fp = kpmFopen(filename, ext, fmode);
    if( fp == NULL ) {
        ARLOGe("Error saving KPM data: unable to open file '%s%s%s' for writing.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return -1;
    }

    if( fwrite(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    
    for(i = 0; i < refDataSet->num; i++ ) {
        if( fwrite(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    }

    if( fwrite(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fwrite( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        if( fwrite( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadWrite;
        j = refDataSet->pageInfo[i].imageNum;
        if( fwrite(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadWrite;
    }

    fclose(fp);
    return 0;
    
bailBadWrite:
    ARLOGe("Error saving KPM data: error writing data.\n");
    fclose(fp);
    return -1;
}

int kpmLoadRefDataSet( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr )
{
    KpmRefDataSet  *refDataSet;
    FILE           *fp;
    char            fmode[] = "rb";
    int             i, j;

    if (!filename || !refDataSetPtr) {
        ARLOGe("kpmLoadRefDataSet(): NULL filename/refDataSetPtr.\n");
        return (-1);
    }

    fp = kpmFopen(filename, ext, fmode);
    if (!fp) {
        ARLOGe("Error loading KPM data: unable to open file '%s%s%s' for reading.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return (-1);
    }

    arMallocClear(refDataSet, KpmRefDataSet, 1);
    
    if( fread(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->num <= 0 ) goto bailBadRead;
    arMalloc(refDataSet->refPoint, KpmRefData, refDataSet->num); // each KpmRefData = 68 floats, 3 ints = 284 bytes.
    
    for(i = 0; i < refDataSet->num; i++ ) {
        if( fread(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    }

    if( fread(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;

    if( refDataSet->pageNum <= 0 ) {
        refDataSet->pageInfo = NULL;
        goto bailBadRead;
    }
    arMalloc(refDataSet->pageInfo, KpmPageInfo, refDataSet->pageNum);
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fread( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        j = refDataSet->pageInfo[i].imageNum;
        arMalloc(refDataSet->pageInfo[i].imageInfo, KpmImageInfo, j);
        if( fread(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadRead;
    }

    *refDataSetPtr = refDataSet;

    fclose(fp);
    return 0;

bailBadRead:
    ARLOGe("Error loading KPM data: error reading data.\n");
    if (refDataSet->pageInfo) free(refDataSet->pageInfo);
    if (refDataSet->refPoint) free(refDataSet->refPoint);
    free(refDataSet);
    fclose(fp);
    return (-1);
}

int kpmLoadRefDataSetOld( const char *filename, const char *ext, KpmRefDataSet **refDataSetPtr )
{
    KpmRefDataSet  *refDataSet;
    FILE           *fp;
    char            fmode[] = "rb";
    uint32_t        dummy[2];
    int             i, j;
    
    if (!filename || !refDataSetPtr) {
        ARLOGe("kpmLoadRefDataSetOld(): NULL filename/refDataSetPtr.\n");
        return (-1);
    }
    
    fp = kpmFopen(filename, ext, fmode);
    if( fp == NULL ) {
        ARLOGe("Error loading KPM data: unable to open file '%s%s%s' for reading.\n", filename, (ext ? "." : ""), (ext ? ext : ""));
        return -1;
    }
    
    arMallocClear(refDataSet, KpmRefDataSet, 1);
    
    if( fread(&(refDataSet->num), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->num <= 0 ) goto bailBadRead;
    arMalloc(refDataSet->refPoint, KpmRefData, refDataSet->num);

    if( fread(dummy, sizeof(uint32_t), 2, fp) != 2 ) goto bailBadRead; // Skip old (int)refDataSet->surfThresh and (int)refDataSet->coord3DFlag.

    for(i = 0; i < refDataSet->num; i++ ) {
        if( fread(  &(refDataSet->refPoint[i].coord2D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].coord3D), sizeof(KpmCoord2D), 1, fp) != 1 ) goto bailBadRead;
        //if( fread(  &(refDataSet->refPoint[i].featureVec), sizeof(SurfFeature), 1, fp) != 1 ) {
        //    goto bailBadRead;
        //}
        if( fread( &(refDataSet->refPoint[i].featureVec.v[0]), sizeof(float), SURF_SUB_DIMENSION, fp) != SURF_SUB_DIMENSION ) goto bailBadRead;
        refDataSet->refPoint[i].featureVec.l = 2;
        if( fread(  &(refDataSet->refPoint[i].pageNo),     sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        if( fread(  &(refDataSet->refPoint[i].refImageNo), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        refDataSet->refPoint[i].pageNo     = 1;
        refDataSet->refPoint[i].refImageNo = 1;
    }
    
    if( fread(&(refDataSet->pageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
    if( refDataSet->pageNum <= 0 ) {
        refDataSet->pageInfo = NULL;
        goto bailBadRead;
    }
    arMalloc(refDataSet->pageInfo, KpmPageInfo, refDataSet->pageNum);
    
    for( i = 0; i < refDataSet->pageNum; i++ ) {
        if( fread( &(refDataSet->pageInfo[i].pageNo),   sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        //if( fread( &(refDataSet->pageInfo[i].imageNum), sizeof(int), 1, fp) != 1 ) goto bailBadRead;
        refDataSet->pageInfo[i].imageNum = 1;
        j = refDataSet->pageInfo[i].imageNum;
        arMalloc(refDataSet->pageInfo[i].imageInfo, KpmImageInfo, j);
        //if( fread(  refDataSet->pageInfo[i].imageInfo,  sizeof(KpmImageInfo), j, fp) != j ) goto bailBadRead;
        refDataSet->pageInfo[i].imageInfo->width  = 1000;
        refDataSet->pageInfo[i].imageInfo->height = 1000;
        refDataSet->pageInfo[i].imageInfo->imageNo = 1;
    }
    
    *refDataSetPtr = refDataSet;
    
    fclose(fp);
    return 0;
    
bailBadRead:
    ARLOGe("Error loading KPM data: error reading data.\n");
    if (refDataSet->pageInfo) free(refDataSet->pageInfo);
    if (refDataSet->refPoint) free(refDataSet->refPoint);
    free(refDataSet);
    fclose(fp);
    return (-1);
}

int kpmChangePageNoOfRefDataSet ( KpmRefDataSet *refDataSet, int oldPageNo, int newPageNo )
{
    if (!refDataSet) {
        ARLOGe("kpmChangePageNoOfRefDataSet(): NULL refDataSet.\n");
        return (-1);
    }

    for(int i = 0; i < refDataSet->num; i++ ) {
        if( refDataSet->refPoint[i].pageNo == oldPageNo || (oldPageNo == KpmChangePageNoAllPages && refDataSet->refPoint[i].pageNo >= 0) ) {
            refDataSet->refPoint[i].pageNo = newPageNo;
        }
    }
    
    for(int i = 0; i < refDataSet->pageNum; i++ ) {
        if( refDataSet->pageInfo[i].pageNo == oldPageNo || (oldPageNo == KpmChangePageNoAllPages && refDataSet->pageInfo[i].pageNo >= 0) ) {
            refDataSet->pageInfo[i].pageNo = newPageNo;
        }
    }

    return 0;
}
