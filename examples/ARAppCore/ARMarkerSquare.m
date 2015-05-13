//
//  ARMarkerSquare.m
//  ARToolKit for iOS
//
//  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
//  LLC ("Daqri") in consideration of your agreement to the following
//  terms, and your use, installation, modification or redistribution of
//  this Daqri software constitutes acceptance of these terms.  If you do
//  not agree with these terms, please do not use, install, modify or
//  redistribute this Daqri software.
//
//  In consideration of your agreement to abide by the following terms, and
//  subject to these terms, Daqri grants you a personal, non-exclusive
//  license, under Daqri's copyrights in this original Daqri software (the
//  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
//  Software, with or without modifications, in source and/or binary forms;
//  provided that if you redistribute the Daqri Software in its entirety and
//  without modifications, you must retain this notice and the following
//  text and disclaimers in all such redistributions of the Daqri Software.
//  Neither the name, trademarks, service marks or logos of Daqri LLC may
//  be used to endorse or promote products derived from the Daqri Software
//  without specific prior written permission from Daqri.  Except as
//  expressly stated in this notice, no other rights or licenses, express or
//  implied, are granted by Daqri herein, including but not limited to any
//  patent rights that may be infringed by your derivative works or by other
//  works in which the Daqri Software may be incorporated.
//
//  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
//  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
//  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
//  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
//  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
//  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
//  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
//  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
//  Copyright 2015 Daqri LLC. All Rights Reserved.
//  Copyright 2010-2015 ARToolworks, Inc. All rights reserved.
//
//  Author(s): Philip Lamb
//

#define VIEW_SCALEFACTOR        1.0f

#import "ARMarkerSquare.h"

#import <AR/ar.h>

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <sys/param.h> // MAXPATHLEN

@implementation ARMarkerSquare {
    ARPattHandle *arPattHandle;
    int        patt_id; // ARToolKit pattern ID.
    int        patt_type;
    ARdouble   matchingThreshold;
    BOOL       useContPoseEstimation;
}

@synthesize patt_id, patt_type, matchingThreshold, useContPoseEstimation;

- (id) initWithPatternFile:(NSString *)path width:(ARdouble)width arPattHandle:(ARPattHandle *)arPattHandle_in
{
    if ((self = [super init])) {
        
        if (!path || !arPattHandle_in) {
            [self release];
            return (nil);
        }

        marker_width = marker_height = width;
        arPattHandle = arPattHandle_in;
        useContPoseEstimation = FALSE;
        
        char buf[MAXPATHLEN];
        if (![path getFileSystemRepresentation:buf maxLength:(sizeof(buf))]) {
            [self release];
            return (nil);
        }
        if ((patt_id = arPattLoad(arPattHandle, buf)) < 0 ) {
            NSLog(@"Unable to load pattern '%@'.\n", path);
            [self release];
            return (nil);
        }
        patt_type = AR_PATTERN_TYPE_TEMPLATE;
        
        self.name = [path lastPathComponent];
    }
    return (self);
}

- (id) initWithBarcode:(unsigned int)barcodeID width:(ARdouble)width
{
    if ((self = [super init])) {
        
        marker_width = marker_height = width;
        useContPoseEstimation = FALSE;
        
        patt_id = barcodeID;
        patt_type = AR_PATTERN_TYPE_MATRIX;
        
        self.name = [NSString stringWithFormat:@"%u", barcodeID];
    }
    return (self);
}

- (void) updateWithDetectedMarkers:(ARMarkerInfo *)markerInfo count:(int)markerNum ar3DHandle:(AR3DHandle *)ar3DHandle
{
    ARdouble err;
    int j, k;

    validPrev = valid;
    if (markerInfo && markerNum > 0 && ar3DHandle) {
        // Check through the marker_info array for highest confidence
        // visible marker matching our preferred pattern.
        k = -1;
        if (patt_type == AR_PATTERN_TYPE_TEMPLATE) {
            for (j = 0; j < markerNum; j++) {
                if (patt_id == markerInfo[j].idPatt) {
                    if (k == -1) {
                        if (markerInfo[j].cfPatt >= matchingThreshold) k = j; // First marker detected.
                    } else if (markerInfo[j].cfPatt > markerInfo[k].cfPatt) k = j; // Higher confidence marker detected.
                }
            }
            if (k != -1) {
                markerInfo[k].id = markerInfo[k].idPatt;
                markerInfo[k].cf = markerInfo[k].cfPatt;
                markerInfo[k].dir = markerInfo[k].dirPatt;
            }
        } else {
            for (j = 0; j < markerNum; j++) {
                if (patt_id == markerInfo[j].idMatrix) {
                    if (k == -1) {
                        if (markerInfo[j].cfMatrix >= matchingThreshold) k = j; // First marker detected.
                    } else if (markerInfo[j].cfMatrix > markerInfo[k].cfMatrix) k = j; // Higher confidence marker detected.
                }
            }
            if (k != -1) {
                markerInfo[k].id = markerInfo[k].idMatrix;
                markerInfo[k].cf = markerInfo[k].cfMatrix;
                markerInfo[k].dir = markerInfo[k].dirMatrix;
            }
        }
        
        if (k != -1) {
            valid = TRUE;
            // Get the transformation between the marker and the real camera into trans.
            if (validPrev && useContPoseEstimation) {
                err = arGetTransMatSquareCont(ar3DHandle, &markerInfo[k], trans, marker_width, trans);
            } else {
                err = arGetTransMatSquare(ar3DHandle, &markerInfo[k], marker_width, trans);
            }
        } else {
            valid = FALSE;
        }
    } else {
        valid = FALSE;
    }

    [super update];
}

-(void) update
{
    [self updateWithDetectedMarkers:NULL count:0 ar3DHandle:NULL];
}

- (void) dealloc
{
    if (patt_type == AR_PATTERN_TYPE_TEMPLATE && patt_id) arPattFree(arPattHandle, patt_id);
    
    [super dealloc];
}

@end
