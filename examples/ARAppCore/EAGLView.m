//
//  EAGLView.m
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


#import "EAGLView.h"

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>

@interface EAGLView (EAGLViewPrivate)
- (BOOL) initCommonWithRenderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained maxScale:(CGFloat)maxScale;
- (UIImage *)snapshot;
@end

@implementation EAGLView
{
    NSString *pixelFormat;
    EAGLDepthFormat depthFormatEAGL;
    BOOL haveStencil;
    GLuint depthStencilFormat;
    EAGLContext *context;
    CGSize surfaceSize;
	
    // Tracks attachments for discard optimisation.
    GLenum attachments[3];
    GLsizei numAttachments;

	// The pixel dimensions of the CAEAGLLayer.
	GLint backingWidth;
	GLint backingHeight;
    
	// The OpenGL names for the frameBuffer and renderbuffer used to render to this view.
	GLuint frameBuffer, colorRenderbuffer;
    
    // OpenGL name for the depth buffer or combined and stencil buffer that is attached to frameBuffer, if it exists (0 if it does not exist).
    GLuint depthStencilRenderBuffer;
    BOOL snapshotRequested;
    id <EAGLViewTookSnapshotDelegate> tookSnapshotDelegate;

    BOOL animating;
    NSInteger animationFrameInterval;
    
    // Use of the CADisplayLink class is the preferred method for controlling animation timing.
    // CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
    id displayLink;
}

@synthesize context, pixelFormat, depthFormatEAGL, haveStencil, surfaceSize, tookSnapshotDelegate;
@synthesize animating;
@dynamic animationFrameInterval;

// You must implement this
+ (Class) layerClass
{
	return [CAEAGLLayer class];
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if ((self = [super initWithCoder:aDecoder])) {
        if (![self initCommonWithRenderingAPI:kEAGLRenderingAPIOpenGLES1 pixelFormat:kEAGLColorFormatRGBA8 depthFormat:kEAGLDepth16 withStencil:FALSE preserveBackbuffer:NO maxScale:0.0f]) {
            [self release];
            return nil;
        }
    }
    
    return self;
}

- (id) initWithFrame:(CGRect)frame
{
    return [self initWithFrame:frame renderingAPI:kEAGLRenderingAPIOpenGLES1 pixelFormat:kEAGLColorFormatRGBA8 depthFormat:kEAGLDepth16 withStencil:FALSE preserveBackbuffer:NO maxScale:0.0f];
}

- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api
{
	return [self initWithFrame:frame renderingAPI:api pixelFormat:kEAGLColorFormatRGBA8 depthFormat:kEAGLDepth16 withStencil:FALSE preserveBackbuffer:NO maxScale:0.0f];
}

- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format
{
	return [self initWithFrame:frame renderingAPI:api pixelFormat:format depthFormat:kEAGLDepth16 withStencil:FALSE preserveBackbuffer:NO maxScale:0.0f];
}

- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained
{
    return [self initWithFrame:frame renderingAPI:api pixelFormat:format depthFormat:kEAGLDepth16 withStencil:FALSE preserveBackbuffer:NO maxScale:0.0f];
}

- (id) initWithFrame:(CGRect)frame renderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained maxScale:(CGFloat)maxScale
{
    if ((self = [super initWithFrame:frame])) {
        if (![self initCommonWithRenderingAPI:api pixelFormat:format depthFormat:depth withStencil:stencil preserveBackbuffer:retained maxScale:maxScale]) {
            [self release];
            return nil;
        }
    }
    
    return self;
}

- (BOOL) initCommonWithRenderingAPI:(EAGLRenderingAPI)api pixelFormat:(NSString*)format depthFormat:(EAGLDepthFormat)depth withStencil:(BOOL)stencil preserveBackbuffer:(BOOL)retained maxScale:(CGFloat)maxScale
{
    pixelFormat = format;
    depthFormatEAGL = depth;
    haveStencil = stencil;
    if (stencil) {
        if (depth != kEAGLDepth0) depthStencilFormat = GL_DEPTH24_STENCIL8_OES;
        else depthStencilFormat = GL_STENCIL_INDEX8_OES; // N.B.: GL_STENCIL_INDEX8_OES == GL_STENCIL_INDEX8
    } else {
        switch (depth) {
            case kEAGLDepth0:
                depthStencilFormat = 0;
                break;
            case kEAGLDepth16:
                depthStencilFormat = GL_DEPTH_COMPONENT16_OES; // N.B.: GL_DEPTH_COMPONENT16_OES == GL_DEPTH_COMPONENT16
                break;
            case kEAGLDepth24:
                depthStencilFormat = GL_DEPTH_COMPONENT24_OES;
                break;
        }
    }
    snapshotRequested = FALSE;
    tookSnapshotDelegate = nil;
    surfaceSize = CGSizeZero;
    
    // Set scaling factor for retina displays.
    CGFloat scale = [[UIScreen mainScreen] scale];
    if (scale != 1.0f) {
        self.contentScaleFactor = (maxScale ? MAX(scale, maxScale) : scale);
    }
    
    // Get the layer
    CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
    surfaceSize = eaglLayer.bounds.size;
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:retained], kEAGLDrawablePropertyRetainedBacking,
                                    format, kEAGLDrawablePropertyColorFormat,
                                    nil];
    
    context = [[EAGLContext alloc] initWithAPI:api];
    if (!context || ![EAGLContext setCurrentContext:context]) {
        [self release];
        return FALSE;
    }
    
    // Create default frameBuffer object. The backing will be allocated for the current layer in -resizeFromLayer
    if (api == kEAGLRenderingAPIOpenGLES1) {
        glGenFramebuffersOES(1, &frameBuffer);
        glGenRenderbuffersOES(1, &colorRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
        attachments[0] = GL_COLOR_ATTACHMENT0_OES;
        numAttachments = 1;
        if (depthFormatEAGL != kEAGLDepth0 || haveStencil) {
            glGenRenderbuffersOES(1, &depthStencilRenderBuffer);
            glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthStencilRenderBuffer);
            if (depthFormatEAGL != kEAGLDepth0) {
                glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthStencilRenderBuffer);
                attachments[numAttachments++] = GL_DEPTH_ATTACHMENT_OES;
            }
            if (haveStencil) {
                glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES, GL_RENDERBUFFER_OES, depthStencilRenderBuffer);
                attachments[numAttachments++] = GL_STENCIL_ATTACHMENT_OES;
            }
        }
    } else if (api == kEAGLRenderingAPIOpenGLES2) {
        glGenFramebuffers(1, &frameBuffer);
        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
        attachments[0] = GL_COLOR_ATTACHMENT0;
        numAttachments = 1;
        if (depthFormatEAGL != kEAGLDepth0 || haveStencil) {
            glGenRenderbuffers(1, &depthStencilRenderBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderBuffer);
            if (depthFormatEAGL != kEAGLDepth0) {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderBuffer);
                attachments[numAttachments++] = GL_DEPTH_ATTACHMENT;
            }
            if (haveStencil) {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderBuffer);
                attachments[numAttachments++] = GL_STENCIL_ATTACHMENT;
            }
        }
    }
    
    animating = FALSE;
    animationFrameInterval = 1;
    displayLink = nil;
    
    return TRUE;
}

- (void) layoutSubviews
{
	// Allocate buffer backing(s) based on the current layer size.
    if (context.API == kEAGLRenderingAPIOpenGLES1) {
        
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
        [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer*)self.layer];
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);
        
        if (depthStencilFormat) {
            glBindRenderbufferOES(GL_RENDERBUFFER_OES, depthStencilRenderBuffer);
            glRenderbufferStorageOES(GL_RENDERBUFFER_OES, depthStencilFormat, backingWidth, backingHeight);
        }
        
        if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
            NSLog(@"Failed to make complete frameBuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        }
        
    } else if (context.API == kEAGLRenderingAPIOpenGLES2) {
        
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
        
        if (depthStencilFormat) {
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, depthStencilFormat, backingWidth, backingHeight);
        }
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            NSLog(@"Failed to make complete frameBuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        }
        
    }
    
    //[self drawView:nil];
}

- (int)backingWidth
{
    return (int)backingWidth;
}

- (int)backingHeight
{
    return (int)backingHeight;
}

- (void) setCurrentContext
{
	if (![EAGLContext setCurrentContext:context]) {
		NSLog(@"Failed to set current context %p in %s\n", context, __FUNCTION__);
	}
}

- (BOOL) isCurrentContext
{
	return ([EAGLContext currentContext] == context ? YES : NO);
}

- (void) clearCurrentContext
{
	if (![EAGLContext setCurrentContext:nil])
		NSLog(@"Failed to clear current context in %s\n", __FUNCTION__);
}

- (void) clearBuffers
{
    [self setCurrentContext];
    
 	if (context.API == kEAGLRenderingAPIOpenGLES1) glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
    else if (context.API == kEAGLRenderingAPIOpenGLES2) glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glClear(GL_COLOR_BUFFER_BIT | (depthFormatEAGL != kEAGLDepth0 ? GL_DEPTH_BUFFER_BIT : 0u) | (haveStencil ? GL_STENCIL_BUFFER_BIT : 0u)); // Clear the render buffers for new frame.
}

- (void) drawView:(id)sender
{
	// Override in subclass.
}

- (void) swapBuffers
{
	EAGLContext *oldContext = [EAGLContext currentContext];
	
	if (oldContext != context)
		[EAGLContext setCurrentContext:context];
	
	if (context.API == kEAGLRenderingAPIOpenGLES1) glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    else if (context.API == kEAGLRenderingAPIOpenGLES2) glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	
	if (snapshotRequested && tookSnapshotDelegate) {
		[tookSnapshotDelegate tookSnapshot:[self snapshot] forView:self];
		snapshotRequested = FALSE;
	}
	
	if (![context presentRenderbuffer:GL_RENDERBUFFER_OES]) // N.B.: GL_RENDERBUFFER_OES == GL_RENDERBUFFER.
		NSLog(@"Failed to swap renderbuffer in %s\n", __FUNCTION__);

	glDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, numAttachments, attachments); // N.B.: GL_FRAMEBUFFER_OES == GL_FRAMEBUFFER.
	
	if (oldContext != context)
		[EAGLContext setCurrentContext:oldContext];
}

- (NSInteger) animationFrameInterval
{
	return animationFrameInterval;
}

- (void) setAnimationFrameInterval:(NSInteger)frameInterval
{
	// Frame interval defines how many display frames must pass between each time the
	// display link fires. The display link will only fire 30 times a second when the
	// frame internal is two on a display that refreshes 60 times a second. The default
	// frame interval setting of one will fire 60 times a second when the display refreshes
	// at 60 times a second. A frame interval setting of less than one results in undefined
	// behavior.
	if (frameInterval >= 1) {
        
		animationFrameInterval = frameInterval;
		
		if (animating) {
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void) startAnimation
{
	if (!animating) {
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink setFrameInterval:animationFrameInterval];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating) {
        [displayLink invalidate];
        displayLink = nil;
		animating = FALSE;
	}
}

//
// Subclasses should override to dispose of their GL resources too.
// E.g. an OpenGL ES 2 renderer would want to delete its shader objects.
//
- (void) dealloc
{
	// Tear down GL
    if (context.API == kEAGLRenderingAPIOpenGLES1) {
        if (frameBuffer) {
            glDeleteFramebuffersOES(1, &frameBuffer);
            frameBuffer = 0;
        }
        if (colorRenderbuffer) {
            glDeleteRenderbuffersOES(1, &colorRenderbuffer);
            colorRenderbuffer = 0;
        }
        if (depthStencilRenderBuffer) {
            glDeleteRenderbuffersOES(1, &depthStencilRenderBuffer);
            colorRenderbuffer = 0;
        }
	} else if (context.API == kEAGLRenderingAPIOpenGLES2) {
        if (frameBuffer) {
            glDeleteFramebuffers(1, &frameBuffer);
            frameBuffer = 0;
        }
        if (colorRenderbuffer) {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
            colorRenderbuffer = 0;
        }
        if (depthStencilRenderBuffer) {
            glDeleteRenderbuffers(1, &depthStencilRenderBuffer);
            colorRenderbuffer = 0;
        }
    }
    
	// Tear down context
	if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	
	[context release];
	context = nil;
	
	[super dealloc];
}

- (void) takeSnapshot
{
    snapshotRequested = TRUE;
}

- (UIImage *) snapshot
{
	// Allocate a buffer and read data from the backing CAEAGLLayer.
	NSInteger dataLength = backingWidth * backingHeight * 4;
	GLubyte *data = (GLubyte *)malloc(dataLength * sizeof(GLubyte));
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(0, 0, backingWidth, backingHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	// Create a CGImage with the pixel data.
	// For opaque OpenGL ES content, use kCGImageAlphaNoneSkipLast to ignore the alpha channel. Otherwise, use kCGImageAlphaPremultipliedLast.
	CGDataProviderRef ref = CGDataProviderCreateWithData(NULL, data, dataLength, NULL);
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGImageRef iref = CGImageCreate(backingWidth, backingHeight, 8, 32, backingWidth * 4, colorspace, kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipLast,
									ref, NULL, true, kCGRenderingIntentDefault);
	
	// OpenGL ES measures data in PIXELS.
	// Create a graphics context with the target size measured in POINTS, and using the OpenGL ES view's
    // contentScaleFactor, so that we get a high-resolution snapshot when its value is greater than 1.0.
    CGFloat scale = [self contentScaleFactor];
    NSInteger widthInPoints = backingWidth / scale;
    NSInteger heightInPoints = backingHeight / scale;
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(widthInPoints, heightInPoints), NO, scale);
	
	CGContextRef cgcontext = UIGraphicsGetCurrentContext();
	
	// UIKit coordinate system is upside down to GL/Quartz coordinate system
	// Flip the CGImage by rendering it to the flipped bitmap context
	// The size of the destination area is measured in POINTS
	CGContextSetBlendMode(cgcontext, kCGBlendModeCopy);
	CGContextDrawImage(cgcontext, CGRectMake(0.0, 0.0, widthInPoints, heightInPoints), iref);
	
	// Retrieve the UIImage from the current context
	UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
	
	UIGraphicsEndImageContext();
	
	// Clean up
	free(data);
	CFRelease(ref);
	CFRelease(colorspace);
	CGImageRelease(iref);
	
	return image;
}

@end
