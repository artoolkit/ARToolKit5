//
//  ARView.m
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

#import <QuartzCore/QuartzCore.h>
#import "ARView.h"
#import "ARViewController.h"
#import "glStateCache.h"
#import <AR/gsub_es.h>
#import <AR/gsub_mtx.h>
#import <Eden/EdenMath.h>

NSString *const ARViewUpdatedCameraLensNotification = @"ARViewUpdatedCameraLensNotification";
NSString *const ARViewUpdatedCameraPoseNotification = @"ARViewUpdatedCameraPoseNotification";
NSString *const ARViewUpdatedViewportNotification = @"ARViewUpdatedViewportNotification";
NSString *const ARViewDrawPreCameraNotification = @"ARViewDrawPreCameraNotification";
NSString *const ARViewDrawPostCameraNotification = @"ARViewDrawPostCameraNotification";
NSString *const ARViewDrawOverlayNotification = @"ARViewDrawOverlayNotification";
NSString *const ARViewTouchNotification = @"ARViewTouchNotification";

@interface ARView (ARViewPrivate)
- (void) calculateProjection;
@end

@implementation ARView {
    
    ARViewController *arViewController;
    
    float cameraLens[16];
    float cameraPose[16];
    BOOL cameraPoseValid;
    int contentWidth;
    int contentHeight;
    BOOL contentRotate90;
    BOOL contentFlipH;
    BOOL contentFlipV;
    ARViewContentScaleMode contentScaleMode;
    ARViewContentAlignMode contentAlignMode;

    float projection[16];
    GLint viewPort[4];
    
    // Interaction.
    id <ARViewTouchDelegate> touchDelegate;
    
    BOOL rayIsValid;
    ARVec3 rayPoint1;
    ARVec3 rayPoint2;
}

@synthesize contentWidth, contentHeight, contentAlignMode, contentScaleMode, touchDelegate, arViewController;
@synthesize rayIsValid, rayPoint1, rayPoint2;

- (id) initWithFrame:(CGRect)frame pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained
{
    if ((self = [super initWithFrame:frame renderingAPI:kEAGLRenderingAPIOpenGLES1 pixelFormat:format depthFormat:depth withStencil:stencil preserveBackbuffer:retained])) {
         
        // Init instance variables.
        arViewController = nil;
        
        mtxLoadIdentityf(cameraLens);
        contentRotate90 = contentFlipH = contentFlipV = NO;
        mtxLoadIdentityf(projection);
        
        contentWidth = (int)frame.size.width;
        contentHeight = (int)frame.size.height;
        contentScaleMode = ARViewContentScaleModeFill;
        contentAlignMode = ARViewContentAlignModeCenter;

        cameraPoseValid = NO;
        
        // Init gestures.
        [self setMultipleTouchEnabled:YES];
        [self setTouchDelegate:self];

        // One-time OpenGL setup goes here.
        glStateCacheFlush();
        
        BOOL ok = CHECK_GL_ERROR();
    }
    
    return (self);
    
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    // Calculate viewport.
    int left, bottom, w, h;
    
#ifdef DEBUG
    NSLog(@"[ARView layoutSubviews] backingWidth=%d, backingHeight=%d\n", self.backingWidth, self.backingHeight);
#endif        

    if (self.contentScaleMode == ARViewContentScaleModeStretch) {
        w = self.backingWidth;
        h = self.backingHeight;
    } else {
        int contentWidthFinalOrientation = (contentRotate90 ? contentHeight : contentWidth);
        int contentHeightFinalOrientation = (contentRotate90 ? contentWidth : contentHeight);
        if (self.contentScaleMode == ARViewContentScaleModeFit || self.contentScaleMode == ARViewContentScaleModeFill) {
            float scaleRatioWidth, scaleRatioHeight, scaleRatio;
            scaleRatioWidth = (float)self.backingWidth / (float)contentWidthFinalOrientation;
            scaleRatioHeight = (float)self.backingHeight / (float)contentHeightFinalOrientation;
            if (self.contentScaleMode == ARViewContentScaleModeFill) scaleRatio = MAX(scaleRatioHeight, scaleRatioWidth);
            else scaleRatio = MIN(scaleRatioHeight, scaleRatioWidth);
            w = (int)((float)contentWidthFinalOrientation * scaleRatio);
            h = (int)((float)contentHeightFinalOrientation * scaleRatio);
        } else {
            w = contentWidthFinalOrientation;
            h = contentHeightFinalOrientation;
        }
    }
    
    if (self.contentAlignMode == ARViewContentAlignModeTopLeft
        || self.contentAlignMode == ARViewContentAlignModeLeft
        || self.contentAlignMode == ARViewContentAlignModeBottomLeft) left = 0;
    else if (self.contentAlignMode == ARViewContentAlignModeTopRight
             || self.contentAlignMode == ARViewContentAlignModeRight
             || self.contentAlignMode == ARViewContentAlignModeBottomRight) left = self.backingWidth - w;
    else left = (self.backingWidth - w) / 2;
        
    if (self.contentAlignMode == ARViewContentAlignModeBottomLeft
        || self.contentAlignMode == ARViewContentAlignModeBottom
        || self.contentAlignMode == ARViewContentAlignModeBottomRight) bottom = 0;
    else if (self.contentAlignMode == ARViewContentAlignModeTopLeft
             || self.contentAlignMode == ARViewContentAlignModeTop
             || self.contentAlignMode == ARViewContentAlignModeTopRight) bottom = self.backingHeight - h;
    else bottom = (self.backingHeight - h) / 2;

    glViewport(left, bottom, w, h);
    
    viewPort[viewPortIndexLeft] = left;
    viewPort[viewPortIndexBottom] = bottom;
    viewPort[viewPortIndexWidth] = w;
    viewPort[viewPortIndexHeight] = h;
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedViewportNotification object:self];
#ifdef DEBUG
    NSLog(@"[ARView layoutSubviews] viewport left=%d, bottom=%d, width=%d, height=%d\n", left, bottom, w, h);
#endif
}

- (GLint *)viewPort
{
    return (viewPort);
}

- (void)setCameraLens:(float *)lens
{
    if (lens) {
        mtxLoadMatrixf(cameraLens, lens);
        [self calculateProjection];
    }
}

- (float *)cameraLens
{
    return (projection);
}

- (void) setContentRotate90:(BOOL)contentRotate90_in
{
    contentRotate90 = contentRotate90_in;
    [self calculateProjection];
}

- (BOOL) contentRotate90
{
    return (contentRotate90);
}

- (void) setContentFlipH:(BOOL)contentFlipH_in
{
    contentFlipH = contentFlipH_in;
    [self calculateProjection];
}

- (BOOL) contentFlipH
{
    return (contentFlipH);
}

- (void) setContentFlipV:(BOOL)contentFlipV_in
{
    contentFlipV = contentFlipV_in;
    [self calculateProjection];
}

- (BOOL) contentFlipV
{
    return (contentFlipV);
}

- (void) calculateProjection
{
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    
    if (contentRotate90) mtxLoadMatrixf(projection, ir90);
    else mtxLoadIdentityf(projection);
    if (contentFlipH || contentFlipV) mtxScalef(projection, (contentFlipH ? -1.0f : 1.0f), (contentFlipV ? -1.0f : 1.0f), 1.0f);
    mtxMultMatrixf(projection, cameraLens);
    
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedCameraLensNotification object:self];
}

- (void)setCameraPose:(float *)pose
{
    if (pose) {
        int i;
        for (i = 0; i < 16; i++) cameraPose[i] = pose[i];
        cameraPoseValid = TRUE;
        [[NSNotificationCenter defaultCenter] postNotificationName:ARViewUpdatedCameraPoseNotification object:self];
    } else {
        cameraPoseValid = FALSE;
    }
}

- (float *)cameraPose
{
    if (cameraPoseValid) return (cameraPose);
    else return (NULL);
}

- (void) drawView:(id)sender
{
	float width, height;
	
    [self clearBuffers];

    arglDispImage(arViewController.arglContextSettings);
    
    // Set up 3D mode.
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glStateCacheEnableDepthTest();
    
    // Set any initial per-frame GL state you require here.
    // --->

    // Lighting and geometry that moves with the camera should be added here.
    // (I.e. should be specified before camera pose transform.)
    // --->
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewDrawPreCameraNotification object:self];
    
    if (cameraPoseValid) {
        
        glMultMatrixf(cameraPose);
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
        [[NSNotificationCenter defaultCenter] postNotificationName:ARViewDrawPostCameraNotification object:self];
    }
    
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();
    
    // Set up 2D mode.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (contentRotate90) glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
	width = (float)viewPort[(contentRotate90 ? viewPortIndexHeight : viewPortIndexWidth)];
	height = (float)viewPort[(contentRotate90 ? viewPortIndexWidth : viewPortIndexHeight)];
	glOrthof(0.0f, width, 0.0f, height, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glStateCacheDisableDepthTest();
    
    // Add your own 2D overlays here.
    // --->
    [[NSNotificationCenter defaultCenter] postNotificationName:ARViewDrawOverlayNotification object:self];
   
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();

#ifdef DEBUG
    // Example of 2D drawing. It just draws a white border line.
    const GLfloat square_vertices [4][2] = { {0.5f, 0.5f}, {0.5f, height - 0.5f}, {width - 0.5f, height - 0.5f}, {width - 0.5f, 0.5f} };
    glColor4ub(255, 255, 255, 255);
    glLineWidth(1.0f);
    glStateCacheDisableLighting();
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheDisableTex2D();
    glVertexPointer(2, GL_FLOAT, 0, square_vertices);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glStateCacheDisableClientStateTexCoordArray();
    glStateCacheDisableClientStateNormalArray();
    glDrawArrays(GL_LINE_LOOP, 0, 4);
#endif
    
#ifdef DEBUG
    CHECK_GL_ERROR();
#endif
        
    [self swapBuffers];
}

- (void) dealloc
{
    [super dealloc];
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    CGPoint            location;
    NSUInteger      numTaps;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesBegan].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseBegan) {
            location = [touch locationInView:self];
            numTaps = [touch tapCount];
            if (touchDelegate) {
                if ([touchDelegate respondsToSelector:@selector(handleTouchAtLocation:tapCount:)]) {
                    [touchDelegate handleTouchAtLocation:location tapCount:numTaps];
                }    
            }
        } // phase match.
    } // touches.
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesMoved].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseMoved) {
            // Can do something appropriate for a moving touch here.
         } // phase match.
    } // touches.
}

// Handles the end of a touch event.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesEnded].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseEnded) {
            // Can do something appropriate for end of touch here.
        } // phase match.
    } // touches.
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray*        array = [touches allObjects];
    UITouch*        touch;
    NSUInteger        i;
    
#ifdef DEBUG
    //NSLog(@"[EAGLView touchesCancelled].\n");
#endif
    
    for (i = 0; i < [array count]; ++i) {
        touch = [array objectAtIndex:i];
        if (touch.phase == UITouchPhaseCancelled) {
               // Can do something appropriate for cancellation of a touch (e.g. by a system event) here.
        } // phase match.
    } // touches.
}

- (void)convertPointInViewToRay:(CGPoint)point
{
    
    float m[16], A[16];
    float p[4], q[4];
    
    // Find INVERSE(PROJECTION * MODELVIEW).
    EdenMathMultMatrix(A, projection, cameraPose);
    if (EdenMathInvertMatrix(m, A)) {
        
        // Next, normalise point to viewport range [-1.0, 1.0], and with depth -1.0 (i.e. at near clipping plane).
        p[0] = (point.x - viewPort[viewPortIndexLeft]) * 2.0f / viewPort[viewPortIndexWidth] - 1.0f; // (winx - viewport[0]) * 2 / viewport[2] - 1.0;
        p[1] = (point.y - viewPort[viewPortIndexBottom]) * 2.0f / viewPort[viewPortIndexHeight] - 1.0f; // (winy - viewport[1]) * 2 / viewport[3] - 1.0;
        p[2] = -1.0f; // 2 * winz - 1.0;
        p[3] = 1.0f;
        
        // Calculate the point's world coordinates.
        EdenMathMultMatrixByVector(q, m, p);
        
        if (q[3] != 0.0f) {
            rayPoint1.v[0] = q[0] / q[3];
            rayPoint1.v[1] = q[1] / q[3];
            rayPoint1.v[2] = q[2] / q[3];
            
            // Next, a second point with depth 1.0 (i.e. at far clipping plane).
            p[2] = 1.0f; // 2 * winz - 1.0;
            
            // Calculate the point's world coordinates.
            EdenMathMultMatrixByVector(q, m, p);
            if (q[3] != 0.0f) {
                
                rayPoint2.v[0] = q[0] / q[3];
                rayPoint2.v[1] = q[1] / q[3];
                rayPoint2.v[2] = q[2] / q[3];
                
                rayIsValid = TRUE;
                return;
            }
        }
    }
    rayIsValid = FALSE;
}

- (void) handleTouchAtLocation:(CGPoint)location tapCount:(NSUInteger)tapCount
{
    CGPoint locationFlippedY = CGPointMake(location.x, self.surfaceSize.height - location.y);
    //NSLog(@"Touch at CG location (%.1f,%.1f), surfaceSize.height makes it (%.1f,%.1f) with y flipped.\n", location.x, location.y, locationFlippedY.x, locationFlippedY.y);
    
    [self convertPointInViewToRay:locationFlippedY];
    if (rayIsValid) {
        [[NSNotificationCenter defaultCenter] postNotificationName:ARViewTouchNotification object:self];
    }
}

@end
