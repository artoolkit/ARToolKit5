/*
 *  kpmResult.cpp
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


int kpmGetRefDataSet( KpmHandle *kpmHandle, KpmRefDataSet **refDataSet )
{
    if( kpmHandle == NULL ) return -1;
    if( refDataSet == NULL ) return -1;

    *refDataSet = &(kpmHandle->refDataSet);

    return 0;
}

int kpmGetInDataSet( KpmHandle *kpmHandle, KpmInputDataSet **inDataSet )
{
    if( kpmHandle == NULL ) return -1;
    if( inDataSet == NULL ) return -1;

    *inDataSet = &(kpmHandle->inDataSet);
    return 0;
}

int kpmGetMatchingResult( KpmHandle *kpmHandle, KpmMatchResult **preRANSAC, KpmMatchResult **aftRANSAC )
{
    if( kpmHandle == NULL ) return -1;
    if( preRANSAC != NULL ) {
        *preRANSAC = &(kpmHandle->preRANSAC);
    }
    if( aftRANSAC != NULL ) {
        *aftRANSAC = &(kpmHandle->aftRANSAC);
    }

    return 0;
}

int kpmGetPose( KpmHandle *kpmHandle, float pose[3][4], int *pageNo, float *error )
{
    int     i, j;

    if( kpmHandle == NULL ) return -1;
    if( kpmHandle->refDataSet.pageNum == 0 ) return -1;

    for(int pageLoop= 0; pageLoop < kpmHandle->resultNum; pageLoop++) {
        if( kpmHandle->result[pageLoop].camPoseF == 0 ) {
            for(j=0;j<3;j++) for(i=0;i<4;i++) pose[j][i] = kpmHandle->result[pageLoop].camPose[j][i];
            *pageNo = kpmHandle->result[pageLoop].pageNo;
            *error = kpmHandle->result[pageLoop].error;
            return 0;
        }
    }

    return -1;
}

int kpmGetResult( KpmHandle *kpmHandle, KpmResult **result, int *resultNum )
{
    if( kpmHandle == NULL ) return -1;

    *result = kpmHandle->result;
    *resultNum = kpmHandle->resultNum;

    return 0;
}
