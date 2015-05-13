/*
 *  QTKitVideo.m
 *  ARToolKit5
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 *	Rev		Date		Who		Changes
 *	1.0.0	2010-07-01	PRL		Written.
 *
 */

#include <AR/video.h>

#ifdef AR_INPUT_QUICKTIME7

#import "QTKitVideo.h"
#import <Availability.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#ifdef DEBUG
//#define QTKIT_VIDEO_DEBUG
//#define QTKIT_VIDEO_REPORT_FORMAT_CHANGES
#endif

@interface QTKitVideo (QTKitVideoPrivate)
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;
- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)videoFrame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection;
@end

@implementation QTKitVideo

@synthesize gotFrameDelegate, gotFrameDelegateUserData;
@synthesize width, height, pixelFormat, bytesPerRow;
@synthesize preferredDevice, preferredDeviceUID, running, pause, showDialogs, acceptMuxedVideo;

- (id) init
{
	if ((self = [super init])) {
                
        captureSession = nil;
        captureDevice = nil;
        captureDeviceInput = nil;
        captureVideoDataOutput = nil;
        latestFrame = NULL;
        
        running = FALSE;
        width = 0;
        height = 0;
        pixelFormat = 0;
        bytesPerRow = 0;
        gotFrameDelegate = nil;
        gotFrameDelegateUserData = NULL;
        preferredDevice = 0;
        preferredDeviceUID = nil;
        showDialogs = TRUE;
        acceptMuxedVideo = TRUE;
        
#ifdef QTKIT_VIDEO_MULTITHREADED
        // Create a mutex to protect access to the frame.
        // Create a condition variable to signal when a new frame arrives.
        int err;
        if ((err = pthread_mutex_init(&frameLock_pthread_mutex, NULL)) != 0) {
            NSLog(@"QTKitVideo(): Error creating mutex.\n");
            return (nil);
        }
#endif
        
        // QuickTime 7.2 or later is required. Need to check that here.
    }
    return (self);
}

- (void) startWithRequestedWidth:(size_t)w height:(size_t)h pixelFormat:(OSType)pf
{
    NSError *error;
    
    if (running) [self stop]; // Restart functionality.

    captureSession = [[QTCaptureSession alloc] init];
    if (!captureSession) {
        NSLog(@"Unable to initialise video capture session.\n");
        return;
    }

    // Set up notification for capture session runtime errors.
    //  The notification user info dictionary QTCaptureSessionErrorKey entry contains an NSError object that describes the error.
    /*[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleCaptureSessionRuntimeErrorNotification:)
     name:QTCaptureSessionRuntimeErrorNotification
     object:nil];*/
    
    NSArray *videoDevices = [[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo] arrayByAddingObjectsFromArray:[QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeMuxed]];
    if (![videoDevices count]) {
        NSLog(@"Error: No video devices connected.\n");
    } else {
        if (preferredDeviceUID) {
            captureDevice = [QTCaptureDevice deviceWithUniqueID:self.preferredDeviceUID];
            if (!captureDevice) {
                NSLog(@"Error: Device with unique ID %@ is not available. Using default device.\n", self.preferredDeviceUID);
                captureDevice = [videoDevices objectAtIndex:0];
            }
        } else {
            if ([videoDevices count] < (preferredDevice + 1)) {
                NSLog(@"Error: not enough video devices available to satisfy request for device at index %d. Using default device.\n", preferredDevice);
                captureDevice = [videoDevices objectAtIndex:0];
            } else {
                captureDevice = [videoDevices objectAtIndex:preferredDevice];
            }
        }
    }
    if (!captureDevice) {
        NSLog(@"Unable to acquire video capture device.\n");
        [captureSession release];
        captureSession = nil;
        return;
    }
    [captureDevice retain];

#ifdef QTKIT_VIDEO_REPORT_FORMAT_CHANGES
    [captureDevice addObserver:self forKeyPath:@"formatDescriptions" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial context:NULL];
#endif
    
    if (![captureDevice open:&error]) {
        if (showDialogs) [[NSAlert alertWithError:error] runModal];
        NSLog(@"QTKitVideo encountered error opening capture device: %@\n", [error localizedDescription]);
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;        
    }
    
    // Set up the input.
    captureDeviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:captureDevice];
    if (!captureDeviceInput) {
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;
    }
    
    // If a multiplexed device (e.g. a DV cam) disable audio.
    if ([captureDevice hasMediaType:QTMediaTypeMuxed]) {
        NSArray *ownedConnections = [captureDeviceInput connections];
        for (QTCaptureConnection *connection in ownedConnections) {
            if ( [[connection mediaType] isEqualToString:QTMediaTypeSound] ) {
                [connection setEnabled:NO];
            }
        }
    }
    
    if (![captureSession addInput:captureDeviceInput error:&error]) {
        if (showDialogs) [[NSAlert alertWithError:error] runModal];
        NSLog(@"QTKitVideo encountered error adding device input: %@\n", [error localizedDescription]);
        [captureDeviceInput release];
        captureDeviceInput = nil;
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;
    }
    
    // Set up the output.
    captureVideoDataOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
    if (!captureVideoDataOutput) {
        NSLog(@"QTKitVideo encountered error adding video device output.\n");
        [captureDeviceInput release];
        captureDeviceInput = nil;
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;
    }
    [captureVideoDataOutput setDelegate:self];
    [captureVideoDataOutput setAutomaticallyDropsLateVideoFrames:YES];
    NSMutableDictionary *attributes = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithBool:YES], (id)kCVPixelBufferOpenGLCompatibilityKey, // Guarantees that resulting format can be submitted to OpenGL as a texture.
                                       nil];
    if (w) [attributes setObject:[NSNumber numberWithDouble:w] forKey:(id)kCVPixelBufferWidthKey];
    if (h) [attributes setObject:[NSNumber numberWithDouble:h] forKey:(id)kCVPixelBufferHeightKey];
    if (pf) [attributes setObject:[NSNumber numberWithUnsignedInt:pf] forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    [captureVideoDataOutput setPixelBufferAttributes:attributes];
    if (![captureSession addOutput:captureVideoDataOutput error:&error]) {
        if (showDialogs) [[NSAlert alertWithError:error] runModal];
        NSLog(@"QTKitVideo encountered error adding decompressed output: %@\n", [error localizedDescription]);
        [captureDeviceInput release];
        captureDeviceInput = nil;
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;
    }
    
    [captureSession startRunning];
    
    // Wait for a frame to be returned so we can get frame size and pixel format.
    // Effectively a spinlock.
    int timeout = 5000; // 5000 x 1 millisecond (see below).
    while (!latestFrame && (0 < timeout--)) {
        SInt32 result;
        do {
            result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, true);
            //NSLog(@"Device %d runloop returned status %d\n", preferredDevice, (int)result);
        } while (result == kCFRunLoopRunHandledSource);
        if (latestFrame) break;
        usleep(1000); // Wait 1 millisecond.
    };
    if (!latestFrame) {
        NSLog(@"Camera not responding after 5 seconds waiting. Giving up.");
        [captureDeviceInput release];
        captureDeviceInput = nil;
        [captureDevice release];
        captureDevice = nil;
        [captureSession release];
        captureSession = nil;
        return;
    }

    pixelFormat = CVPixelBufferGetPixelFormatType(latestFrame);
    size_t planes = CVPixelBufferGetPlaneCount(latestFrame);
    if (!planes) {
        bytesPerRow = CVPixelBufferGetBytesPerRow(latestFrame);
        width = CVPixelBufferGetWidth(latestFrame);
        height = CVPixelBufferGetHeight(latestFrame);
    } else {
        bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(latestFrame, 0);
        width = CVPixelBufferGetWidthOfPlane(latestFrame, 0);
        height = CVPixelBufferGetHeightOfPlane(latestFrame, 0);
    }

    running = TRUE;
    return;
}

#ifdef QTKIT_VIDEO_REPORT_FORMAT_CHANGES
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqual:@"formatDescriptions"]) {
        
        NSArray *descArray = [change objectForKey:NSKeyValueChangeNewKey]; // An array of QTFormatDescriptions, just like [captureDevice formatDescriptions].
        NSEnumerator *enumerator = [descArray objectEnumerator];
        QTFormatDescription *desc;
        while ((desc = [enumerator nextObject])) {
            if ([desc mediaType] == QTMediaTypeVideo) {
                UInt32 formatType = [desc formatType];
                NSSize size;
                [[desc attributeForKey:QTFormatDescriptionVideoEncodedPixelsSizeAttribute] getValue:&size];
                // More detailed info can be obtained if desired. Uncomment as required.
                // See <QuickTime/ImageCompression.h> header for info on the ImageDescription struct.
                //NSData *data = [desc quickTimeSampleDescription];
                //ImageDescriptionPtr imageDesc = (ImageDescriptionPtr)[data bytes];
#ifdef DEBUG
                // Report video size and compression type.
                ARLOGd("formatDescriptions's video formatType is ");
                if (formatType > 0x28) ARLOGe("%c%c%c%c", (char)((formatType >> 24) & 0xFF),
                                              (char)((formatType >> 16) & 0xFF),
                                              (char)((formatType >>  8) & 0xFF),
                                              (char)((formatType >>  0) & 0xFF));
                else ARLOGd("%u", (int)formatType);
                ARLOGe(", size is %.0fx%.0f.\n", size.width, size.height);
#endif
            }
        }
    }
    // We won't call [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}
#endif // QTKIT_VIDEO_REPORT_FORMAT_CHANGES

- (void)captureOutput:(QTCaptureOutput *)captureOutput didOutputVideoFrame:(CVImageBufferRef)frame withSampleBuffer:(QTSampleBuffer *)sampleBuffer fromConnection:(QTCaptureConnection *)connection
{
    //NSLog(@"(Device %d) captureOutput:didOutputVideoFrame:withSampleBuffer:fromConnection:\n", preferredDevice);
    if (pause) return;
    
    CVImageBufferRef frameToRelease;
    
    CVBufferRetain(frame);
    
    UInt64 hostTime = [[sampleBuffer attributeForKey:QTSampleBufferHostTimeAttribute] unsignedLongLongValue];
     
#ifdef QTKIT_VIDEO_MULTITHREADED
    pthread_mutex_lock(&frameLock_pthread_mutex);
#endif
    frameToRelease = latestFrame;
    latestFrame = frame;
    latestFrameHostTime = hostTime;
#ifdef QTKIT_VIDEO_MULTITHREADED
    pthread_mutex_unlock(&frameLock_pthread_mutex);
#endif
    
    CVBufferRelease(frameToRelease);
    
    if (gotFrameDelegate) [gotFrameDelegate QTKitVideoGotFrame:self userData:gotFrameDelegateUserData];
}

// Returns a retained CVImageBufferRef. The caller must call CVBufferRelease when finished with the frame.
- (CVImageBufferRef) frameTimestamp:(UInt64 *)timestampOut;
{
    if (latestFrame) {
        if (timestampOut) *timestampOut = latestFrameHostTime;
        return (CVBufferRetain(latestFrame));
    } else return (NULL);
}

- (CVImageBufferRef) frameTimestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp;
{
    // In multithreaded mode, this should still be safe to perform without
    // a lock since the producer thread will only ever increase latestFrameHostTime.
    if (latestFrameHostTime <= timestamp) return (NULL);
    return ([self frameTimestamp:timestampOut]);
}

- (void) stop
{
    if (!running) return;
    running = FALSE;
    
    if (latestFrame) {
        CVBufferRelease(latestFrame);
        latestFrame = NULL;
    }
    
    //[[NSNotificationCenter defaultCenter] removeObserver:self];
#ifdef QTKIT_VIDEO_REPORT_FORMAT_CHANGES
    [captureDevice removeObserver:self forKeyPath:@"formatDescriptions"];
#endif
    [captureSession stopRunning];
    
    if ([captureDevice isOpen]) [captureDevice close];
    [captureVideoDataOutput release];
    captureVideoDataOutput = nil;
    [captureDeviceInput release];
    captureDeviceInput = nil;
    [captureDevice release];
    captureDevice = nil;
    [captureSession release];
    captureSession = nil;
}

- (void) dealloc
{
    if (running) [self stop];
    
#ifdef QTKIT_VIDEO_MULTITHREADED
   pthread_mutex_destroy(&frameLock_pthread_mutex);
#endif
    
    [super dealloc];
}

#ifdef QTKIT_VIDEO_DEBUG
- (BOOL) respondsToSelector:(SEL)sel
{
    BOOL r = [super respondsToSelector:sel];
    if (!r)
        NSLog(@"QTKitVideo was just asked for a response to selector \"%@\" (we do%@)\n", NSStringFromSelector(sel), (r ? @"" : @" not"));
    return r;
}
#endif

@end




#endif //  AR_INPUT_QUICKTIME7
