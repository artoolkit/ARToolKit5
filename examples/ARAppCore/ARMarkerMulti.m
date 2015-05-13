//
//  ARMarkerMulti.m
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

#import "ARMarkerMulti.h"

#import <AR/ar.h>
#import <AR/arMulti.h>

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <sys/param.h> // MAXPATHLEN

@implementation ARMarkerMulti {
    ARMultiMarkerInfoT *config;
    ARPattHandle *arPattHandle;
    //ARdouble   matchingThreshold;
    BOOL       robust;
}

@synthesize /*matchingThreshold,*/ robust;

- (id) initWithMultiConfigFile:(NSString *)path arPattHandle:(ARPattHandle *)arPattHandle_in;
{
    if ((self = [super init])) {
        
        if (!path || !arPattHandle_in) {
            [self release];
            return (nil);
        }

        marker_width = marker_height = 0;
        arPattHandle = arPattHandle_in;
        robust = TRUE;
        
        char buf[MAXPATHLEN];
        if (![path getFileSystemRepresentation:buf maxLength:(sizeof(buf))]) {
            [self release];
            return (nil);
        }
        if ((config = arMultiReadConfigFile(buf, arPattHandle)) < 0 ) {
            NSLog(@"Unable to load multi config '%@'.\n", path);
            [self release];
            return (nil);
        }
        
        self.name = [path lastPathComponent];
    }
    return (self);
}

- (void) updateWithDetectedMarkers:(ARMarkerInfo *)markerInfo count:(int)markerNum ar3DHandle:(AR3DHandle *)ar3DHandle
{
    ARdouble err;
    
    validPrev = valid;
    if (config && markerInfo && markerNum > 0 && ar3DHandle) {
		if (robust) {
			err = arGetTransMatMultiSquareRobust(ar3DHandle, markerInfo, markerNum, config);
		} else {
			err = arGetTransMatMultiSquare(ar3DHandle, markerInfo, markerNum, config);
		}
        
		// Marker is visible if a match was found.
        if (err >= 0) {
            valid = TRUE;
            for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) trans[j][k] = config->trans[j][k];
        } else valid = FALSE;
        
    } else {
        valid = FALSE;
    }

    [super update];
}

-(void) update
{
    [self updateWithDetectedMarkers:NULL count:0 ar3DHandle:NULL];
}

- (int) multiPatternDetectionMode
{
    if (!config) return (-1);
    return (config->patt_type);
}

- (void) dealloc
{
    if (config) {
        arMultiFreeConfig(config);
        config = NULL;
    }
        
    [super dealloc];
}

@end
