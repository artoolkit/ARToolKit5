//
//  ARView.h
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

//
// ARView is a subclass of the standard EAGLView, which provides AR-related
// OpenGL drawing and utility functionality.
//
// Most importantly, it maps the video background into the OpenGL drawing
// context, and provides a skeleton OpenGL drawing loop.
//
// ARView provides a set of drawing "hooks" (via NSNotification)
// to which other classes can connect to be informed of changes to the OpenGL viewport
// and projection and modelview matrices.
// This version also includes hooks to allow other classes  to draw their 3D content
// and 2D overlays at appropriate times in the frame drawing cycle.
//
// A single ARView is managed by a single ARViewController.
// 

#import "EAGLView.h"
#import "ARMarker.h" // ARVec3

@class ARViewController;

@protocol ARViewTouchDelegate<NSObject>
@optional
- (void) handleTouchAtLocation:(CGPoint)location tapCount:(NSUInteger)tapCount;
@end

// Notifications.
extern NSString *const ARViewUpdatedCameraLensNotification;
extern NSString *const ARViewUpdatedCameraPoseNotification;
extern NSString *const ARViewUpdatedViewportNotification;
extern NSString *const ARViewDrawPreCameraNotification;
extern NSString *const ARViewDrawPostCameraNotification;
extern NSString *const ARViewDrawOverlayNotification;
extern NSString *const ARViewTouchNotification;

enum viewPortIndices {
    viewPortIndexLeft = 0,
    viewPortIndexBottom,
    viewPortIndexWidth,
    viewPortIndexHeight
};

typedef enum {
    ARViewContentScaleModeStretch = 0,
    ARViewContentScaleModeFit,
    ARViewContentScaleModeFill,
    ARViewContentScaleModeFit1to1
} ARViewContentScaleMode;

typedef enum {
    ARViewContentAlignModeTopLeft = 0,
    ARViewContentAlignModeTop,
    ARViewContentAlignModeTopRight,
    ARViewContentAlignModeLeft,
    ARViewContentAlignModeCenter,
    ARViewContentAlignModeRight,
    ARViewContentAlignModeBottomLeft,
    ARViewContentAlignModeBottom,
    ARViewContentAlignModeBottomRight,
} ARViewContentAlignMode;

@interface ARView : EAGLView <ARViewTouchDelegate> {
}

- (id) initWithFrame:(CGRect)frame pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained;
@property float *cameraLens;
@property float *cameraPose;
@property(readonly) GLint *viewPort;
- (void) drawView:(id)sender;

// Points to the parent view controller.
@property (nonatomic, assign) IBOutlet ARViewController *arViewController;

// These properties allow variation on the way content is drawn in the GL window.
@property int contentWidth;
@property int contentHeight;
@property BOOL contentRotate90;
@property BOOL contentFlipH;
@property BOOL contentFlipV;
@property ARViewContentScaleMode contentScaleMode; // Defaults to ARViewContentScaleModeFill.
@property ARViewContentAlignMode contentAlignMode; // Defaults to ARViewContentAlignModeCenter.

// Interaction.
@property(nonatomic, assign) id <ARViewTouchDelegate> touchDelegate;
@property(nonatomic, readonly) BOOL rayIsValid;
@property(nonatomic, readonly) ARVec3 rayPoint1;
@property(nonatomic, readonly) ARVec3 rayPoint2;

@end
