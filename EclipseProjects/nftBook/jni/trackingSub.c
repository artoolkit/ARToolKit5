/*
 *	trackingSub.c
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
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
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2010-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Hirokazu Kato, Philip Lamb.
 *
 */

#include "trackingSub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    KpmHandle              *kpmHandle;      // KPM-related data.
    ARUint8                *imagePtr;       // Pointer to image being tracked.
    int                     imageSize;      // Bytes per image.
    float                   trans[3][4];    // Transform containing pose of tracked image.
    int                     page;           // Assigned page number of tracked image.
    int                     flag;           // Tracked successfully.
} TrackingInitHandle;

static void *trackingInitMain( THREAD_HANDLE_T *threadHandle );


int trackingInitQuit( THREAD_HANDLE_T **threadHandle_p )
{
    TrackingInitHandle  *trackingInitHandle;

    if (!threadHandle_p)  {
        ARLOGe("trackingInitQuit(): Error: NULL threadHandle_p.\n");
        return (-1);
    }
    if (!*threadHandle_p) return 0;
    
    threadWaitQuit( *threadHandle_p );
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(*threadHandle_p);
    if (trackingInitHandle) {
        free( trackingInitHandle->imagePtr );
        free( trackingInitHandle );
    }
    threadFree( threadHandle_p );
    return 0;
}

THREAD_HANDLE_T *trackingInitInit( KpmHandle *kpmHandle )
{
    TrackingInitHandle  *trackingInitHandle;
    THREAD_HANDLE_T     *threadHandle;

    if (!kpmHandle) {
        ARLOGe("trackingInitInit(): Error: NULL KpmHandle.\n");
        return (NULL);
    }
    
    trackingInitHandle = (TrackingInitHandle *)malloc(sizeof(TrackingInitHandle));
    if( trackingInitHandle == NULL ) return NULL;
    trackingInitHandle->kpmHandle = kpmHandle;
    trackingInitHandle->imageSize = kpmHandleGetXSize(kpmHandle) * kpmHandleGetYSize(kpmHandle) * arUtilGetPixelSize(kpmHandleGetPixelFormat(kpmHandle));
    trackingInitHandle->imagePtr  = (ARUint8 *)malloc(trackingInitHandle->imageSize);
    trackingInitHandle->flag      = 0;

    threadHandle = threadInit(0, trackingInitHandle, trackingInitMain);
    return threadHandle;
}

int trackingInitStart( THREAD_HANDLE_T *threadHandle, ARUint8 *imagePtr )
{
    TrackingInitHandle     *trackingInitHandle;

    if (!threadHandle || !imagePtr) {
        ARLOGe("trackingInitStart(): Error: NULL threadHandle or imagePtr.\n");
        return (-1);
    }
    
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!trackingInitHandle) {
        ARLOGe("trackingInitStart(): Error: NULL trackingInitHandle.\n");
        return (-1);
    }
    memcpy( trackingInitHandle->imagePtr, imagePtr, trackingInitHandle->imageSize );
    threadStartSignal( threadHandle );

    return 0;
}

int trackingInitGetResult( THREAD_HANDLE_T *threadHandle, float trans[3][4], int *page )
{
    TrackingInitHandle     *trackingInitHandle;
    int  i, j;

    if (!threadHandle || !trans || !page)  {
        ARLOGe("trackingInitGetResult(): Error: NULL threadHandle or trans or page.\n");
        return (-1);
    }
    
    if( threadGetStatus( threadHandle ) == 0 ) return 0;
    threadEndWait( threadHandle );
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!trackingInitHandle) return (-1);
    if( trackingInitHandle->flag ) {
        for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) trans[j][i] = trackingInitHandle->trans[j][i];
        *page = trackingInitHandle->page;
        return 1;
    }

    return -1;
}

static void *trackingInitMain( THREAD_HANDLE_T *threadHandle )
{
    TrackingInitHandle     *trackingInitHandle;
    KpmHandle              *kpmHandle;
    KpmResult              *kpmResult = NULL;
    int                     kpmResultNum;
    ARUint8                *imagePtr;
    float                  err;
    int                    i, j, k;

    if (!threadHandle) {
        ARLOGe("Error starting tracking thread: empty THREAD_HANDLE_T.\n");
        return (NULL);
    }
    trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
    if (!threadHandle) {
        ARLOGe("Error starting tracking thread: empty trackingInitHandle.\n");
        return (NULL);
    }
    kpmHandle          = trackingInitHandle->kpmHandle;
    imagePtr           = trackingInitHandle->imagePtr;
    if (!kpmHandle || !imagePtr) {
        ARLOGe("Error starting tracking thread: empty kpmHandle/imagePtr.\n");
        return (NULL);
    }
    ARLOGi("Start tracking thread.\n");
    
    kpmGetResult( kpmHandle, &kpmResult, &kpmResultNum );

    for(;;) {
        if( threadStartWait(threadHandle) < 0 ) break;

        kpmMatching(kpmHandle, imagePtr);
        trackingInitHandle->flag = 0;
        for( i = 0; i < kpmResultNum; i++ ) {
            if( kpmResult[i].camPoseF != 0 ) continue;
            ARLOGd("kpmGetPose OK.\n");
            if( trackingInitHandle->flag == 0 || err > kpmResult[i].error ) { // Take the first or best result.
                trackingInitHandle->flag = 1;
                trackingInitHandle->page = kpmResult[i].pageNo;
                for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) trackingInitHandle->trans[j][k] = kpmResult[i].camPose[j][k];
                err = kpmResult[i].error;
            }
        }

        threadEndSignal(threadHandle);
    }

    ARLOGi("End tracking thread.\n");
    return (NULL);
}
