/*
 *  QTKitVideo.h
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

#import <QTKit/QTKit.h>
#import <AR/video.h>
#import <pthread.h>

@protocol QTKitVideoGotFrameDelegate<NSObject>
@required
- (void) QTKitVideoGotFrame:(id)sender userData:(void *)data;
@end


@class QTKitVideo;

@interface QTKitVideo : NSObject
{
    QTCaptureSession *captureSession;
    QTCaptureDevice *captureDevice;
    QTCaptureDeviceInput *captureDeviceInput;
    QTCaptureDecompressedVideoOutput *captureVideoDataOutput;
    CVImageBufferRef latestFrame;
    UInt64 latestFrameHostTime;

    BOOL running;
    int preferredDevice;
    NSString *preferredDeviceUID;
    size_t width;
    size_t height;
    OSType pixelFormat;
    size_t bytesPerRow;
    id <QTKitVideoGotFrameDelegate> gotFrameDelegate;
    void *gotFrameDelegateUserData;
    BOOL willSaveNextFrame;
    BOOL pause;
    BOOL showDialogs;
    BOOL acceptMuxedVideo;
    
#ifdef QTKIT_VIDEO_MULTITHREADED
    pthread_mutex_t frameLock_pthread_mutex;
#endif
}

- (id) init;

- (void) startWithRequestedWidth:(size_t)width height:(size_t)height pixelFormat:(OSType)pixelFormat;

// Returns a retained CVImageBufferRef. The caller must call CVBufferRelease when finished with the frame.
- (CVImageBufferRef) frameTimestamp:(UInt64 *)timestampOut;

// Returns a retained CVImageBufferRef. The caller must call CVBufferRelease when finished with the frame.
- (CVImageBufferRef) frameTimestamp:(UInt64 *)timestampOut ifNewerThanTimestamp:(UInt64)timestamp;

- (void) stop;


@property(nonatomic, assign) id <QTKitVideoGotFrameDelegate> gotFrameDelegate; // Called when a new frame is ready. Typically, will be called on secondary thread.
@property(nonatomic) void *gotFrameDelegateUserData; // Passed back to delegate.
@property(nonatomic, readonly) size_t width;
@property(nonatomic, readonly) size_t height;
@property(nonatomic, readonly) OSType pixelFormat;
@property(nonatomic, readonly) size_t bytesPerRow;
@property(nonatomic) int preferredDevice; // Zero-indexed. Should be set prior to -startWithRequestedWidth:height:pixelFormat:.
@property(nonatomic, retain) NSString *preferredDeviceUID; // Overrides preferredDevice. Should be set prior to -startWithRequestedWidth:height:pixelFormat:.
@property(nonatomic, readonly) BOOL running;
@property(nonatomic) BOOL pause;
@property(nonatomic) BOOL showDialogs; // Defaults to TRUE. Set to FALSE to disable display of user dialogs.
@property(nonatomic) BOOL acceptMuxedVideo; // Defaults to TRUE. Set to FALSE to disable use of video from multiplexed video/audio sources (e.g. DV cameras).

@end