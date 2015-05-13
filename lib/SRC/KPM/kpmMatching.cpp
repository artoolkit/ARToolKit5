/*
 *  kpmMatching.cpp
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
#include <AR/ar.h>
#include <KPM/kpm.h>
#include "AnnMatch.h"
#include "AnnMatch2.h"
#include "HomographyEst.h"


#if !ANN2
typedef struct {
    int   pageID;
    int   imageID;
    int   xsize, ysize;
    int   xnum, ynum;
    int   xsize2, ysize2;
    int   xoff, yoff;
} KpmSmallImageInfo;
#endif

int kpmSetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet *refDataSet )
{
#if ANN2
    CAnnMatch2         *ann2;
#else
    CAnnMatch          *ann;
    KpmSmallImageInfo  *smallImageInfo;
    int                 smallImageInfoNum;
    int                 k;
#endif
    FeatureVector       featureVector;
    int                 i, j;
    
    if (!kpmHandle || !refDataSet) {
        ARLOGe("kpmSetRefDataSet(): NULL kpmHandle/refDataSet.\n");
        return -1;
    }
    if (!refDataSet->num) {
        ARLOGe("kpmSetRefDataSet(): refDataSet.\n");
        return -1;
    }
    
    // Copy the refPoints into the kpmHandle's dataset.
    if( kpmHandle->refDataSet.refPoint != NULL ) {
        // Discard any old points first.
        free( kpmHandle->refDataSet.refPoint );
    }
    if( refDataSet->num != 0 ) {
        arMalloc( kpmHandle->refDataSet.refPoint, KpmRefData, refDataSet->num );
        for( i = 0; i < refDataSet->num; i++ ) {
            kpmHandle->refDataSet.refPoint[i] = refDataSet->refPoint[i];
        }
    }
    else {
        kpmHandle->refDataSet.refPoint = NULL;
    }
    kpmHandle->refDataSet.num = refDataSet->num;

    // Copy the pageInfo into the kpmHandle's dataset.
    if( kpmHandle->refDataSet.pageInfo != NULL ) {
        // Discard any old pageInfo (and imageInfo) first.
        for( i = 0; i < kpmHandle->refDataSet.pageNum; i++ ) {
            if( kpmHandle->refDataSet.pageInfo[i].imageInfo != NULL ) {
                free( kpmHandle->refDataSet.pageInfo[i].imageInfo );
            }
        }
        free( kpmHandle->refDataSet.pageInfo );
    }
    if( refDataSet->pageNum != 0 ) {
        arMalloc( kpmHandle->refDataSet.pageInfo, KpmPageInfo, refDataSet->pageNum );
        for( i = 0; i < refDataSet->pageNum; i++ ) {
            kpmHandle->refDataSet.pageInfo[i].pageNo = refDataSet->pageInfo[i].pageNo;
            kpmHandle->refDataSet.pageInfo[i].imageNum = refDataSet->pageInfo[i].imageNum;
            if( refDataSet->pageInfo[i].imageNum != 0 ) {
                arMalloc( kpmHandle->refDataSet.pageInfo[i].imageInfo, KpmImageInfo, refDataSet->pageInfo[i].imageNum );
                for( j = 0; j < refDataSet->pageInfo[i].imageNum; j++ ) {
                    kpmHandle->refDataSet.pageInfo[i].imageInfo[j] = refDataSet->pageInfo[i].imageInfo[j];
                }
            }
            else {
                refDataSet->pageInfo[i].imageInfo = NULL;
            }
        }
    }
    else {
        kpmHandle->refDataSet.pageInfo = NULL;
    }
    kpmHandle->refDataSet.pageNum = refDataSet->pageNum;

    
    if( kpmHandle->result != NULL ) {
        free( kpmHandle->result );
        kpmHandle->result = NULL;
        kpmHandle->resultNum = 0;
    }
    if( refDataSet->pageNum > 0 ) {
        kpmHandle->resultNum = refDataSet->pageNum;
        arMalloc( kpmHandle->result, KpmResult, refDataSet->pageNum );
    }

    // Create feature vectors.
#if ANN2
    if (kpmHandle->ann2) {
        delete (CAnnMatch2 *)(kpmHandle->ann2);
        kpmHandle->ann2 = NULL;
    }
    if (kpmHandle->refDataSet.num != 0) {
        ann2 = new CAnnMatch2();
        kpmHandle->ann2 = (void *)ann2;
        arMalloc( featureVector.sf, SurfFeature, kpmHandle->refDataSet.num );
        for( int l = 0; l < kpmHandle->refDataSet.num; l++ ) {
            featureVector.sf[l] = kpmHandle->refDataSet.refPoint[l].featureVec;
        }
        featureVector.num = kpmHandle->refDataSet.num;
        ann2->Construct(&featureVector);
        free(featureVector.sf);
    }
#else
    if( kpmHandle->annInfo != NULL ) {
        for( i = 0; i < kpmHandle->annInfoNum; i++ ) {
            if( kpmHandle->annInfo[i].ann != NULL ) {
                CAnnMatch *ann = (CAnnMatch *)(kpmHandle->annInfo[i].ann);
                delete ann;
            }
            if( kpmHandle->annInfo[i].annCoordIndex != NULL ) {
                free( kpmHandle->annInfo[i].annCoordIndex );
            }
        }
    }
    free( kpmHandle->annInfo );
    
    //int  regXsize = kpmHandle->xsize*3/2;
    //int  regYsize = kpmHandle->ysize*3/2;
    int  regXsize = 10000;
    int  regYsize = 10000;
    int  overlapX = kpmHandle->xsize/3;
    int  overlapY = kpmHandle->ysize/3;
    for( i = k = 0; i < kpmHandle->refDataSet.pageNum; i++ ) k += kpmHandle->refDataSet.pageInfo[i].imageNum;
    smallImageInfoNum = k;
    arMalloc( smallImageInfo, KpmSmallImageInfo, smallImageInfoNum );
    for( i = k = 0; i < kpmHandle->refDataSet.pageNum; i++ ) {
        for( j = 0; j < kpmHandle->refDataSet.pageInfo[i].imageNum; j++ ) {
            smallImageInfo[k].pageID  = i;
            smallImageInfo[k].imageID = j;
            smallImageInfo[k].xsize =   kpmHandle->refDataSet.pageInfo[i].imageInfo[j].width;
            smallImageInfo[k].ysize =   kpmHandle->refDataSet.pageInfo[i].imageInfo[j].height;
            smallImageInfo[k].xnum =   (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].width -overlapX) / regXsize + 1;
            smallImageInfo[k].ynum =   (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].height-overlapY) / regYsize + 1;
            smallImageInfo[k].xsize2 = (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].width -overlapX) / smallImageInfo[k].xnum + overlapX;
            smallImageInfo[k].ysize2 = (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].height-overlapY) / smallImageInfo[k].ynum + overlapY;
            smallImageInfo[k].xoff =   (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].width -overlapX) / smallImageInfo[i].xnum;
            smallImageInfo[k].yoff =   (kpmHandle->refDataSet.pageInfo[i].imageInfo[j].height-overlapY) / smallImageInfo[i].ynum;
            k++;
        }
    }

    for( i = k = 0; i < smallImageInfoNum; i++ ) {
        k += smallImageInfo[i].xnum * smallImageInfo[i].ynum;
    }
    kpmHandle->annInfoNum = k;
    arMalloc(kpmHandle->annInfo, KpmAnnInfo, k);
    for( i = k = 0; i < smallImageInfoNum; i++ ) {
        for( int jj = 0; jj < smallImageInfo[i].ynum; jj++ ) {
            int sy = smallImageInfo[i].yoff * jj;
            int ey = (jj == smallImageInfo[i].ynum-1)? smallImageInfo[i].ysize-1: sy + smallImageInfo[i].ysize2 - 1;
            for( int ii = 0; ii < smallImageInfo[i].xnum; ii++ ) {
                int sx = smallImageInfo[i].xoff * ii;
                int ex = (ii == smallImageInfo[i].xnum-1)? smallImageInfo[i].xsize-1: sx + smallImageInfo[i].xsize2 - 1;

                ann = new CAnnMatch();
                kpmHandle->annInfo[k].ann = (void *)ann;
                
                kpmHandle->annInfo[k].pageID  = smallImageInfo[i].pageID;
                kpmHandle->annInfo[k].imageID = smallImageInfo[i].imageID;
        
                int dataNum = 0;
                int pageNo  = kpmHandle->refDataSet.pageInfo[smallImageInfo[i].pageID].pageNo;
                int imageNo = kpmHandle->refDataSet.pageInfo[smallImageInfo[i].pageID].imageInfo[smallImageInfo[i].imageID].imageNo;
                for( int l = 0; l < kpmHandle->refDataSet.num; l++ ) {
                    if( kpmHandle->refDataSet.refPoint[l].pageNo     != pageNo  ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].refImageNo != imageNo ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].coord2D.x < sx || kpmHandle->refDataSet.refPoint[l].coord2D.x > ex ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].coord2D.y < sy || kpmHandle->refDataSet.refPoint[l].coord2D.y > ey ) continue;
                    dataNum++;
                }
                arMalloc( kpmHandle->annInfo[k].annCoordIndex, int, dataNum );
                arMalloc( featureVector.sf, SurfFeature, dataNum );

                for( int l = 0, l1 = 0; l < kpmHandle->refDataSet.num; l++ ) {
                    if( kpmHandle->refDataSet.refPoint[l].pageNo     != pageNo  ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].refImageNo != imageNo ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].coord2D.x < sx || kpmHandle->refDataSet.refPoint[l].coord2D.x > ex ) continue;
                    if( kpmHandle->refDataSet.refPoint[l].coord2D.y < sy || kpmHandle->refDataSet.refPoint[l].coord2D.y > ey ) continue;
                    featureVector.sf[l1] = kpmHandle->refDataSet.refPoint[l].featureVec;
                    kpmHandle->annInfo[k].annCoordIndex[l1] = l;
                    l1++;
                }
                featureVector.num = dataNum;
                ann->Construct(&featureVector);
                free(featureVector.sf);
                k++;
            }
        }
    }
    free(smallImageInfo);
#endif

    return 0;
}

int kpmSetRefDataSetFile( KpmHandle *kpmHandle, const char *filename, const char *ext )
{
    KpmRefDataSet   *refDataSet;
    
    if (!kpmHandle || !filename) {
        ARLOGe("kpmSetRefDataSetFile(): NULL kpmHandle/filename.\n");
        return -1;
    }
    
    if( kpmLoadRefDataSet(filename, ext, &refDataSet) < 0 ) return -1;
    if( kpmSetRefDataSet(kpmHandle, refDataSet) < 0 ) {
        kpmDeleteRefDataSet(&refDataSet);
        return -1;
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    return 0;
}

int kpmSetRefDataSetFileOld( KpmHandle *kpmHandle, const char *filename, const char *ext )
{
    KpmRefDataSet   *refDataSet;
    
    if( kpmHandle == NULL )  return -1;
    
    if( kpmLoadRefDataSetOld(filename, ext, &refDataSet) < 0 ) return -1;
    if( kpmSetRefDataSet(kpmHandle, refDataSet) < 0 ) {
        kpmDeleteRefDataSet(&refDataSet);
        return -1;
    }
    kpmDeleteRefDataSet(&refDataSet);
    
    return 0;
}

int kpmSetMatchingSkipPage( KpmHandle *kpmHandle, int skipPages[], int num )
{
    int    i, j;
    
    if( kpmHandle == NULL ) return -1;
    
    for( i = 0; i < num; i++ ) {
        for( j = 0; j < kpmHandle->refDataSet.pageNum; j++ ) {
            if( skipPages[i] == kpmHandle->refDataSet.pageInfo[j].pageNo ) {
                kpmHandle->result[j].skipF = 1;
                break;
            }
        }
        if( j == kpmHandle->refDataSet.pageNum ) {
            ARLOGe("Cannot find the page for skipping.\n");
            return -1;
        }
    }
    
    return 0;
}

int kpmSetMatchingSkipRegion( KpmHandle *kpmHandle, SurfSubRect *skipRegion, int regionNum)
{
    if( kpmHandle->skipRegion.regionMax < regionNum ) {
        if( kpmHandle->skipRegion.region != NULL ) free(kpmHandle->skipRegion.region);
        kpmHandle->skipRegion.regionMax = ((regionNum-1)/10+1) * 10;
        arMalloc(kpmHandle->skipRegion.region, SurfSubSkipRegion, kpmHandle->skipRegion.regionMax);
    }
    kpmHandle->skipRegion.regionNum = regionNum;
    for(int i = 0; i < regionNum; i++ ) {
        kpmHandle->skipRegion.region[i].rect = skipRegion[i];
        for(int j = 0; j < 4; j++) {
            kpmHandle->skipRegion.region[i].param[j][0] = skipRegion[i].vertex[(j+1)%4].y - skipRegion[i].vertex[j].y;
            kpmHandle->skipRegion.region[i].param[j][1] = skipRegion[i].vertex[j].x - skipRegion[i].vertex[(j+1)%4].x;
            kpmHandle->skipRegion.region[i].param[j][2] = skipRegion[i].vertex[(j+1)%4].x * skipRegion[i].vertex[j].y
                                                        - skipRegion[i].vertex[j].x * skipRegion[i].vertex[(j+1)%4].y;
        }
    }
    return 0;
}

int kpmMatching( KpmHandle *kpmHandle, ARUint8 *inImage )
{
    int               xsize, ysize;
    int               xsize2, ysize2;
    int               procMode;
    ARUint8          *inImageBW;
    FeatureVector     featureVector;
    int              *inlierIndex;
    CorspMap          preRANSAC;
    int               inlierNum;
    int               i, j;
    float             h[3][3];
    int               ret;
#if ANN2
    CAnnMatch2       *ann2;
    int              *annMatch2;
    int               knn;
#else
    CAnnMatch        *ann;
    int              *annMatch;
#endif
    
    if (!kpmHandle || !inImage) {
        ARLOGe("kpmMatching(): NULL kpmHandle/inImage.\n");
        return -1;
    }
    
    xsize           = kpmHandle->xsize;
    ysize           = kpmHandle->ysize;
    procMode        = kpmHandle->procMode;
    
    if (procMode == KpmProcFullSize && (kpmHandle->pixFormat == AR_PIXEL_FORMAT_MONO || kpmHandle->pixFormat == AR_PIXEL_FORMAT_420v || kpmHandle->pixFormat == AR_PIXEL_FORMAT_420f || kpmHandle->pixFormat == AR_PIXEL_FORMAT_NV21)) {
        inImageBW = inImage;
    } else {
        inImageBW = kpmUtilGenBWImage( inImage, kpmHandle->pixFormat, xsize, ysize, procMode, &xsize2, &ysize2 );
        if( inImageBW == NULL ) return -1;
    }
   
    surfSubExtractFeaturePoint( kpmHandle->surfHandle, inImageBW, kpmHandle->skipRegion.region, kpmHandle->skipRegion.regionNum );
    kpmHandle->skipRegion.regionNum = 0;
    
    kpmHandle->inDataSet.num = featureVector.num = surfSubGetFeaturePointNum( kpmHandle->surfHandle );
    if( kpmHandle->inDataSet.num != 0 ) {
        if( kpmHandle->inDataSet.coord != NULL ) free(kpmHandle->inDataSet.coord);
        if( kpmHandle->preRANSAC.match != NULL ) free(kpmHandle->preRANSAC.match);
        if( kpmHandle->aftRANSAC.match != NULL ) free(kpmHandle->aftRANSAC.match);
        arMalloc( kpmHandle->inDataSet.coord, KpmCoord2D,     kpmHandle->inDataSet.num );
        arMalloc( kpmHandle->preRANSAC.match, KpmMatchData,   kpmHandle->inDataSet.num );
        arMalloc( kpmHandle->aftRANSAC.match, KpmMatchData,   kpmHandle->inDataSet.num );
        arMalloc( featureVector.sf,           SurfFeature,    kpmHandle->inDataSet.num );
        arMalloc( preRANSAC.mp,               MatchPoint,     kpmHandle->inDataSet.num );
        arMalloc( inlierIndex,                int,            kpmHandle->inDataSet.num );
#if ANN2
        //knn = kpmHandle->refDataSet.pageNum;
        knn = 1;
        arMalloc( annMatch2,                  int,            kpmHandle->inDataSet.num*knn);
#else
        arMalloc( annMatch,                   int,            kpmHandle->inDataSet.num );
#endif

        if( procMode == KpmProcFullSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x, y, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x;
                    kpmHandle->inDataSet.coord[i].y = y;
                }
            }
        }
        else if( procMode == KpmProcTwoThirdSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*1.5f, y*1.5f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*1.5f;
                    kpmHandle->inDataSet.coord[i].y = y*1.5f;
                }
            }
        }
        else if( procMode == KpmProcHalfSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*2.0f, y*2.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*2.0f;
                    kpmHandle->inDataSet.coord[i].y = y*2.0f;
                }
            }
        }
        else if( procMode == KpmProcOneThirdSize ) {
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*3.0f, y*3.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*3.0f;
                    kpmHandle->inDataSet.coord[i].y = y*3.0f;
                }
            }
        }
        else { // procMode == KpmProcQuatSize
            for( i = 0 ; i < kpmHandle->inDataSet.num; i++ ) {
                float  x, y, *desc;
                surfSubGetFeaturePosition( kpmHandle->surfHandle, i, &x, &y );
                desc = surfSubGetFeatureDescPtr( kpmHandle->surfHandle, i );
                for( j = 0; j < SURF_SUB_DIMENSION; j++ ) {
                    featureVector.sf[i].v[j] = desc[j];
                }
                featureVector.sf[i].l = surfSubGetFeatureSign( kpmHandle->surfHandle, i );
                if( kpmHandle->cparamLT != NULL ) {
                    arParamObserv2IdealLTf( &(kpmHandle->cparamLT->paramLTf), x*4.0f, y*4.0f, &(kpmHandle->inDataSet.coord[i].x), &(kpmHandle->inDataSet.coord[i].y) );
                }
                else {
                    kpmHandle->inDataSet.coord[i].x = x*4.0f;
                    kpmHandle->inDataSet.coord[i].y = y*4.0f;
                }
            }
        }

#if ANN2
        ann2 = (CAnnMatch2*)kpmHandle->ann2;
        ann2->Match(&featureVector, knn, annMatch2);
        for(int pageLoop = 0; pageLoop < kpmHandle->resultNum; pageLoop++ ) {
            kpmHandle->preRANSAC.num = 0;
            kpmHandle->aftRANSAC.num = 0;
            
            kpmHandle->result[pageLoop].pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            kpmHandle->result[pageLoop].camPoseF = -1;
            if( kpmHandle->result[pageLoop].skipF ) continue;

            int featureNum = 0;
            int *annMatch2Ptr = annMatch2;
            int pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            for( i = 0; i < kpmHandle->inDataSet.num; i++ ) {
                for( j = 0; j < knn; j++ ) {
                    if( *annMatch2Ptr >= 0 && kpmHandle->refDataSet.refPoint[*annMatch2Ptr].pageNo == pageNo ) {
                        kpmHandle->preRANSAC.match[featureNum].inIndex = i;
                        kpmHandle->preRANSAC.match[featureNum].refIndex = *annMatch2Ptr;
                        preRANSAC.mp[featureNum].x1 = kpmHandle->inDataSet.coord[i].x;
                        preRANSAC.mp[featureNum].y1 = kpmHandle->inDataSet.coord[i].y;
                        preRANSAC.mp[featureNum].x2 = kpmHandle->refDataSet.refPoint[*annMatch2Ptr].coord3D.x;
                        preRANSAC.mp[featureNum].y2 = kpmHandle->refDataSet.refPoint[*annMatch2Ptr].coord3D.y;
                        featureNum++;
                        annMatch2Ptr += knn-j;
                        break;
                    }
                    annMatch2Ptr++;
                }
            }
            //printf("Page[%d] %d\n", pageLoop, featureNum);
            preRANSAC.num = featureNum;
            if( featureNum < 6 ) continue;
            
            if( kpmRansacHomograhyEstimation(&preRANSAC, inlierIndex, &inlierNum, h) < 0 ) {
                inlierNum = 0;
            }
            //printf(" --> page[%d] %d  pre:%3d, aft:%3d\n", pageLoop, kpmHandle->inDataSet.num, preRANSAC.num, inlierNum);
            if( inlierNum < 6 ) continue;
            
            kpmHandle->preRANSAC.num = preRANSAC.num;
            kpmHandle->aftRANSAC.num = inlierNum;
            for( i = 0; i < inlierNum; i++ ) {
                kpmHandle->aftRANSAC.match[i].inIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].inIndex;
                kpmHandle->aftRANSAC.match[i].refIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].refIndex;
            }
            //printf(" ---> %d %d %d\n", kpmHandle->inDataSet.num, kpmHandle->preRANSAC.num, kpmHandle->aftRANSAC.num);
            if( kpmHandle->poseMode == KpmPose6DOF ) {
                //printf("----- Page %d ------\n", pageLoop);
                ret = kpmUtilGetPose(kpmHandle->cparamLT, &(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                     kpmHandle->result[pageLoop].camPose,  &(kpmHandle->result[pageLoop].error) );
                //printf("----- End. ------\n");
            }
            else {
                ret = kpmUtilGetPoseHomography(&(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                         kpmHandle->result[pageLoop].camPose,  &(kpmHandle->result[pageLoop].error) );
            }
            if( ret == 0 ) {
                kpmHandle->result[pageLoop].camPoseF = 0;
                kpmHandle->result[pageLoop].inlierNum = inlierNum;
                ARLOGi("Page[%d]  pre:%3d, aft:%3d, error = %f\n", pageLoop, preRANSAC.num, inlierNum, kpmHandle->result[pageLoop].error);
            }
        }
        free(annMatch2);
#else
        for(int pageLoop = 0; pageLoop < kpmHandle->resultNum; pageLoop++ ) {
            kpmHandle->result[pageLoop].pageNo = kpmHandle->refDataSet.pageInfo[pageLoop].pageNo;
            kpmHandle->result[pageLoop].camPoseF = -1;
            if( kpmHandle->result[pageLoop].skipF ) continue;
            
            kpmHandle->preRANSAC.num = 0;
            kpmHandle->aftRANSAC.num = 0;
            int annBestLoopFeatureNum = -1;
            for( int annLoop = 0; annLoop < kpmHandle->annInfoNum; annLoop++ ) {
                if( kpmHandle->annInfo[annLoop].pageID != pageLoop ) continue;
                ann = (CAnnMatch *)kpmHandle->annInfo[annLoop].ann;
                ann->Match(&featureVector, annMatch);
            
                int featureNum = 0;
                for( i = 0; i < kpmHandle->inDataSet.num; i++ ) {
                    if( annMatch[i] < 0 ) continue;
                    featureNum++;
                }
                if( featureNum < 6 ) continue;

                preRANSAC.num = 0;
                for( i = j = 0; i < kpmHandle->inDataSet.num; i++ ) {
                    if( annMatch[i] < 0 ) continue;
                    kpmHandle->preRANSAC.match[j].inIndex = i;
                    kpmHandle->preRANSAC.match[j].refIndex = kpmHandle->annInfo[annLoop].annCoordIndex[annMatch[i]];
                    preRANSAC.mp[j].x1 = kpmHandle->inDataSet.coord[i].x;
                    preRANSAC.mp[j].y1 = kpmHandle->inDataSet.coord[i].y;
                    preRANSAC.mp[j].x2 = kpmHandle->refDataSet.refPoint[kpmHandle->annInfo[annLoop].annCoordIndex[annMatch[i]]].coord3D.x;
                    preRANSAC.mp[j].y2 = kpmHandle->refDataSet.refPoint[kpmHandle->annInfo[annLoop].annCoordIndex[annMatch[i]]].coord3D.y;
                    j++;
                }
                preRANSAC.num = j;
                if( kpmRansacHomograhyEstimation(&preRANSAC, inlierIndex, &inlierNum, h) < 0 ) {
                    inlierNum = 0;
                }
                //printf("ann[%d] %d  pre:%3d, aft:%3d\n", annLoop, kpmHandle->inDataSet.num, preRANSAC.num, inlierNum);
                if( inlierNum < 6 ) continue;

                kpmHandle->preRANSAC.num = preRANSAC.num;
                kpmHandle->aftRANSAC.num = inlierNum;
                for( i = 0; i < inlierNum; i++ ) {
                    kpmHandle->aftRANSAC.match[i].inIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].inIndex;
                    kpmHandle->aftRANSAC.match[i].refIndex = kpmHandle->preRANSAC.match[inlierIndex[i]].refIndex;
                }
                //printf(" ---> %d %d %d\n", kpmHandle->inDataSet.num, kpmHandle->preRANSAC.num, kpmHandle->aftRANSAC.num);
                float  camPose[3][4], error;
                if( kpmHandle->poseMode == KpmPose6DOF ) {
                    ret = kpmUtilGetPose(kpmHandle->cparamLT, &(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                         camPose,  &error );
                }
                else {
                    ret = kpmUtilGetPoseHomography(&(kpmHandle->aftRANSAC), &(kpmHandle->refDataSet), &(kpmHandle->inDataSet),
                                             camPose,  &error );
                }
                if( ret == 0 ) {
                    if( annBestLoopFeatureNum < 0 || kpmHandle->aftRANSAC.num > annBestLoopFeatureNum ) {
                        annBestLoopFeatureNum = kpmHandle->aftRANSAC.num;
                        for(j=0;j<3;j++) for(i=0;i<4;i++) kpmHandle->result[pageLoop].camPose[j][i] = camPose[j][i];
                        ARLOGi("ann[%d] %d  pre:%3d, aft:%3d\n", annLoop, kpmHandle->inDataSet.num, preRANSAC.num, inlierNum);
                        ARLOGi("page: %d, error = %f\n", kpmHandle->result[pageLoop].pageNo, error);
                        kpmHandle->result[pageLoop].error = error;
                        kpmHandle->result[pageLoop].camPoseF = 0;
                    }
                }
            }
        }
        free(annMatch);
#endif
        free(featureVector.sf);
        free(preRANSAC.mp);
        free(inlierIndex);
    }
    else {
        kpmHandle->preRANSAC.num = 0;
        kpmHandle->aftRANSAC.num = 0;
        for( i = 0; i < kpmHandle->resultNum; i++ ) {
            kpmHandle->result[i].camPoseF = -1;
        }
    }
    
    for( i = 0; i < kpmHandle->resultNum; i++ ) kpmHandle->result[i].skipF = 0;

    if (inImageBW != inImage) free( inImageBW );
    
    return 0;
}
