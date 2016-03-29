//
//  EAGLView.h
//  ARToolKit for iOS
//
//  Abstract: The EAGLView class is a UIView subclass that renders OpenGL scene.
//  The caller can request either OpenGL ES 2.0 or 1.1.
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
//  Copyright 2009 Apple Inc. All Rights Reserved.
//
//  Author(s): Philip Lamb
//  Version: 2.0
//

//#import <UIKit/UIKit.h> // Using precompiled header.
#import <OpenGLES/EAGL.h>

#define CHECK_GL_ERROR() ({ GLenum __error = glGetError(); if(__error) printf("OpenGL error 0x%04X in %s\n", __error, __FUNCTION__); (__error ? NO : YES); })

typedef enum {
    kEAGLDepth0 = 0,
    kEAGLDepth16,         // Recommended for normal depth-buffer use.
    kEAGLDepth24,         // Recommended for high-precision depth-buffer use, or will be automatically chosen when a stencil buffer and depth buffer are both requested.
} EAGLDepthFormat;

@class EAGLView;
@protocol EAGLViewTookSnapshotDelegate <NSObject>
- (void) tookSnapshot:(UIImage *)image forView:(EAGLView *)view;
@end

@interface EAGLView : UIView
{
}

- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api; // These also set the current context.
- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format;
- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained;
- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained maxScale:(CGFloat)maxScale NS_DESIGNATED_INITIALIZER;
- (id) initWithCoder:(NSCoder *)aDecoder NS_DESIGNATED_INITIALIZER;

@property(readonly, nonatomic) NSString* pixelFormat; // Either kEAGLColorFormatRGBA8 or kEAGLColorFormatRGB565.
@property(readonly, nonatomic) EAGLDepthFormat depthFormatEAGL;
@property(readonly, nonatomic) BOOL haveStencil;

@property(readonly, nonatomic) EAGLContext *context;
@property(readonly, nonatomic) int backingWidth;  // The pixel dimensions of the CAEAGLLayer.
@property(readonly, nonatomic) int backingHeight; // The pixel dimensions of the CAEAGLLayer.

@property(assign) id <EAGLViewTookSnapshotDelegate> tookSnapshotDelegate;
- (void) takeSnapshot;                     // Request that a snapshot of the current OpenGL ES be generated immediately prior to swapping the buffers. tookSnapshotDelegate will be called when the snapshot is ready.

@property(readonly, nonatomic) CGSize surfaceSize;

- (void) setCurrentContext;
- (BOOL) isCurrentContext;
- (void) clearCurrentContext;

- (void) clearBuffers;                     // Subclass should call this at the start of -drawView:.
- (void) drawView:(id)sender;              // Default implementation does nothing; override in subclass.
- (void) swapBuffers;                      // Subclass should call this at the end of -drawView:.

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;
- (void) startAnimation;
- (void) stopAnimation;

@end

