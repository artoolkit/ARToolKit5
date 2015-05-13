/*
 *	ARMarkerSquare.h
 *  ARToolKit5
 *
 *	Demonstration of ARToolKit NFT with models rendered in OSG,
 *  and marker pose estimates filtered to reduce jitter.
 *
 *  Press '?' while running for help on available key commands.
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
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */

#ifndef __ARMarkerSquare_h__
#define __ARMarkerSquare_h__

#include <AR/ar.h>
#include <AR/arFilterTransMat.h>
#ifndef _MSC_VER
#  include <stdbool.h>
#else
typedef unsigned char bool;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ARdouble v[3];
} ARVec3;

typedef struct {
    ARdouble T[16]; // Position and orientation, column-major order. (position(x,y,z) = {T[12], T[13], T[14]}
} ARPose;

extern const ARPose ARPoseUnity;

typedef struct _ARMarkerSquare {
    // ARMarker protected 
    bool       valid;
    bool       validPrev;
    ARdouble   trans[3][4];
    ARPose     pose;
    ARdouble   marker_width;
    ARdouble   marker_height;
    // ARMarker private
    ARFilterTransMatInfo *ftmi;
    ARdouble   filterCutoffFrequency;
    ARdouble   filterSampleRate;
    // ARMarkerSquare
    int        patt_id; // ARToolKit pattern ID.
    int        patt_type;
    ARdouble   marker_coord[4][2]; // Window coords. of marker corners.
    ARdouble   matchingThreshold;
    char      *patternPathname;
} ARMarkerSquare;

#define    AR_PATTERN_TYPE_TEMPLATE    0
#define    AR_PATTERN_TYPE_MATRIX      1

void newMarkers(const char *markersConfigDataFilePathC, ARPattHandle *arPattHandle, ARMarkerSquare **markersSquare_out, int *markersSquareCount_out, int *patternDetectionMode_out);

void deleteMarkers(ARMarkerSquare **markersSquare_p, int *markersSquareCount_p, ARPattHandle *arPattHandle);

#ifdef __cplusplus
}
#endif
#endif // !__ARMarkerSquare_h__