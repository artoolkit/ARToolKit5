/*
 *  kpmHandle.cpp
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

static KpmHandle *kpmCreateHandleCore( ARParamLT *cparamLT, int xsize, int ysize, int poseMode, AR_PIXEL_FORMAT pixFormat );

KpmHandle *kpmCreateHandle( ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat )
{
    return kpmCreateHandleCore( cparamLT, cparamLT->param.xsize, cparamLT->param.ysize, KpmPose6DOF, pixFormat);
}

KpmHandle *kpmCreateHandleHomography( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat )
{
    return kpmCreateHandleCore( NULL, xsize, ysize, KpmPoseHomography, pixFormat );
}

KpmHandle *kpmCreateHandle2( int xsize, int ysize, AR_PIXEL_FORMAT pixFormat )
{
    return kpmCreateHandleCore( NULL, xsize, ysize, KpmPoseHomography, pixFormat );
}

static KpmHandle *kpmCreateHandleCore( ARParamLT *cparamLT, int xsize, int ysize, int poseMode, AR_PIXEL_FORMAT pixFormat )
{
    KpmHandle       *kpmHandle;
    int surfXSize, surfYSize;
    
    if (pixFormat != AR_PIXEL_FORMAT_MONO && pixFormat != AR_PIXEL_FORMAT_420v && pixFormat != AR_PIXEL_FORMAT_420f && pixFormat != AR_PIXEL_FORMAT_NV21) {
        ARLOGw("Performance warning: KPM processing is not using a mono pixel format.\n");
    }

    arMallocClear( kpmHandle, KpmHandle, 1 );

#if ANN2
    kpmHandle->ann2                     = NULL;
#else
    kpmHandle->annInfo                  = NULL;
    kpmHandle->annInfoNum               = 0;
#endif
    
    kpmHandle->cparamLT                = cparamLT;
    kpmHandle->poseMode                = poseMode;
    kpmHandle->xsize                   = xsize;
    kpmHandle->ysize                   = ysize;
    kpmHandle->pixFormat               = pixFormat;
    kpmHandle->procMode                = KpmDefaultProcMode;
    kpmHandle->detectedMaxFeature      = -1;
    kpmHandle->surfThreadNum           = -1;

    kpmHandle->refDataSet.refPoint     = NULL;
    kpmHandle->refDataSet.num          = 0;
    kpmHandle->refDataSet.pageInfo     = NULL;
    kpmHandle->refDataSet.pageNum      = 0;

    kpmHandle->inDataSet.coord         = NULL;
    kpmHandle->inDataSet.num           = 0;

    kpmHandle->preRANSAC.num           = 0;
    kpmHandle->preRANSAC.match         = NULL;
    kpmHandle->aftRANSAC.num           = 0;
    kpmHandle->aftRANSAC.match         = NULL;

    kpmHandle->skipRegion.regionMax    = 0;
    kpmHandle->skipRegion.regionNum    = 0;
    kpmHandle->skipRegion.region       = NULL;
    
    kpmHandle->result                  = NULL;
    kpmHandle->resultNum               = 0;

    switch (kpmHandle->procMode) {
        case KpmProcFullSize:     surfXSize = xsize;     surfYSize = ysize;     break;
        case KpmProcHalfSize:     surfXSize = xsize/2;   surfYSize = ysize/2;   break;
        case KpmProcQuatSize:     surfXSize = xsize/4;   surfYSize = ysize/4;   break;
        case KpmProcOneThirdSize: surfXSize = xsize/3;   surfYSize = ysize/3;   break;
        case KpmProcTwoThirdSize: surfXSize = xsize/3*2; surfYSize = ysize/3*2; break;
        default: ARLOGe("Error: Unknown kpmProcMode %d.\n", kpmHandle->procMode); goto bail; break;
    }
    kpmHandle->surfHandle = surfSubCreateHandle(surfXSize, surfYSize, AR_PIXEL_FORMAT_MONO);
    if (!kpmHandle->surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        goto bail;
    }
    
    surfSubSetMaxPointNum(kpmHandle->surfHandle, kpmHandle->detectedMaxFeature);

    return kpmHandle;
    
bail:
    free(kpmHandle);
    return (NULL);
}


int kpmSetProcMode( KpmHandle *kpmHandle, int  mode )
{
    int    thresh;
    int    maxPointNum;
    int surfXSize, surfYSize;
   
    if( kpmHandle == NULL ) return -1;
    
    if( kpmHandle->procMode == mode ) return 0;
    kpmHandle->procMode = mode;

    surfSubGetThresh( kpmHandle->surfHandle, &thresh );
    surfSubGetMaxPointNum( kpmHandle->surfHandle, &maxPointNum );
    
    surfSubDeleteHandle( &(kpmHandle->surfHandle) );

    switch (kpmHandle->procMode) {
        case KpmProcFullSize:     surfXSize = kpmHandle->xsize;     surfYSize = kpmHandle->ysize;     break;
        case KpmProcHalfSize:     surfXSize = kpmHandle->xsize/2;   surfYSize = kpmHandle->ysize/2;   break;
        case KpmProcQuatSize:     surfXSize = kpmHandle->xsize/4;   surfYSize = kpmHandle->ysize/4;   break;
        case KpmProcOneThirdSize: surfXSize = kpmHandle->xsize/3;   surfYSize = kpmHandle->ysize/3;   break;
        case KpmProcTwoThirdSize: surfXSize = kpmHandle->xsize/3*2; surfYSize = kpmHandle->ysize/3*2; break;
        default: ARLOGe("Error: Unknown kpmProcMode %d.\n", kpmHandle->procMode); goto bail; break;
    }
    kpmHandle->surfHandle = surfSubCreateHandle(surfXSize, surfYSize, AR_PIXEL_FORMAT_MONO);
    if (!kpmHandle->surfHandle) {
        ARLOGe("Error: unable to initialise KPM feature matching.\n");
        goto bail;
    }

    surfSubSetThresh( kpmHandle->surfHandle, thresh );
    surfSubSetMaxPointNum( kpmHandle->surfHandle, maxPointNum );

    return 0;
    
bail:
    return (-1);
}

int kpmGetProcMode( KpmHandle *kpmHandle, int *mode )
{
    if( kpmHandle == NULL ) return -1;
    *mode = kpmHandle->procMode;
    return 0;
}

int kpmSetDetectedFeatureMax( KpmHandle *kpmHandle, int  detectedMaxFeature )
{
    kpmHandle->detectedMaxFeature = detectedMaxFeature;
    surfSubSetMaxPointNum(kpmHandle->surfHandle, kpmHandle->detectedMaxFeature);
    return 0;
}

int kpmGetDetectedFeatureMax( KpmHandle *kpmHandle, int *detectedMaxFeature )
{
    *detectedMaxFeature = kpmHandle->detectedMaxFeature;
    return 0;
}

int kpmSetSurfThreadNum( KpmHandle *kpmHandle, int surfThreadNum )
{
    kpmHandle->surfThreadNum = surfThreadNum;
    surfSubSetThreadNum(kpmHandle->surfHandle, kpmHandle->surfThreadNum);
    return 0;
}



int kpmDeleteHandle( KpmHandle **kpmHandle )
{
    if( *kpmHandle == NULL ) return -1;

#if ANN2
    CAnnMatch2  *ann2 = (CAnnMatch2 *)((*kpmHandle)->ann2);
    delete ann2;
#else
    for(int i = 0; i < (*kpmHandle)->annInfoNum; i++ ) {
        if( (*kpmHandle)->annInfo[i].ann != NULL ) {
            CAnnMatch *ann = (CAnnMatch *)((*kpmHandle)->annInfo[i].ann);
            delete ann;
        }
        if( (*kpmHandle)->annInfo[i].annCoordIndex != NULL ) {
            free((*kpmHandle)->annInfo[i].annCoordIndex);
        }
    }
    free((*kpmHandle)->annInfo);
#endif
    
    surfSubDeleteHandle( &((*kpmHandle)->surfHandle) );

    if( (*kpmHandle)->refDataSet.refPoint != NULL ) {
        free( (*kpmHandle)->refDataSet.refPoint );
    }
    if( (*kpmHandle)->refDataSet.pageInfo != NULL ) {
        free( (*kpmHandle)->refDataSet.pageInfo );
    }
    if( (*kpmHandle)->preRANSAC.match != NULL ) {
        free( (*kpmHandle)->preRANSAC.match );
    }
    if( (*kpmHandle)->aftRANSAC.match != NULL ) {
        free( (*kpmHandle)->aftRANSAC.match );
    }
    if( (*kpmHandle)->skipRegion.region != NULL ) {
        free( (*kpmHandle)->skipRegion.region );
    }
    if( (*kpmHandle)->result != NULL ) {
        free( (*kpmHandle)->result );
    }
    if( (*kpmHandle)->inDataSet.coord != NULL ) {
        free( (*kpmHandle)->inDataSet.coord );
    }

    free( *kpmHandle );
    *kpmHandle = NULL;

    return 0;
}
