//
//  ARMarkerNFT.m
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

#import "ARMarkerNFT.h"

#import "ARViewController.h"

#ifdef WITH_NFT

#import <stdio.h>

@implementation ARMarkerNFT {
    char *pathname;
    int pageNo;
    AR2SurfaceSetT *surfaceSet;
}

@synthesize pageNo, surfaceSet;

- (id) initWithNFTDataSetPathname:(const char *)pathname_in
{
    if (!pathname_in || !pathname_in[0]) return (nil);
    
    if ((self = [super init])) {
        pageNo = -1;
        pathname = strdup(pathname_in);
        
        // Load AR2 data.
        if ((surfaceSet = ar2ReadSurfaceSet(pathname_in, "fset", NULL)) == NULL) {
            NSLog(@"Error reading data from %s.fset", pathname_in);
            [self release];
            return (nil);
        }
        
        // Get width and height in millimetres.
        if (surfaceSet->surface && surfaceSet->surface[0].imageSet && surfaceSet->surface[0].imageSet->scale) {
            AR2ImageT *image = surfaceSet->surface[0].imageSet->scale[0]; // Assume best scale (largest image) is first entry in array scale[index] (index is in range [0, surfaceSet->surface[0].imageSet->num - 1]).
            marker_width = image->xsize * 25.4f / image->dpi;
            marker_height = image->ysize * 25.4f / image->dpi;
        }

    }
    return (self);
}

- (char *)datasetPathname
{
    return (pathname);
}

- (void) updateWithNFTResultsDetectedPage:(int)page_in trackingTrans:(float [3][4])trans_in
{
    int j, k;
    validPrev = valid;
    
    if (pageNo >= 0 && pageNo == page_in && trans_in) {
        valid = TRUE;
        for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) trans[j][k] = (ARdouble)trans_in[j][k];
    }
    else valid = FALSE;

    [super update];
}

- (void) update
{
    [self updateWithNFTResultsDetectedPage:-1 trackingTrans:NULL];
}

- (void) dealloc
{
    if (surfaceSet) ar2FreeSurfaceSet(&surfaceSet);
    if (pathname) free(pathname);
    [super dealloc];
}

-(NSString *) description
{
    return [NSString stringWithFormat:@"ARMarker NFT %p\n  datasetPathname: %s\n  pageNo: %d\n", self, pathname, pageNo];
}
@end

#endif // WITH_NFT
