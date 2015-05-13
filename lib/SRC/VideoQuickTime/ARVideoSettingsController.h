/*
 *	Copyright (c) 2005-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2005-03-08	PRL		Written.
 *
 */

#import <Cocoa/Cocoa.h>
#import "videoQuickTimePrivateMacOSX.h"

@interface ARVideoSettingsController : NSObject {
	@private
	int					inputIndex;
	SeqGrabComponent	seqGrab; 
	SGChannel			sgchanVideo;
	int					mVersion;
	UserData			mUserData;
}

- (id)initInput:(int)inInputIndex withSeqGrabComponent:(SeqGrabComponent)inSeqGrab withSGChannel:(SGChannel)inSgchanVideo;
- (void)dealloc;
- (IBAction)sgConfigurationDialog:(id)sender withStandardDialog:(int)standardDialog;
- (OSErr)loadUserData:(UserData *)outUserData fromDefaults:(NSUserDefaults *)inDefaults forKey:(NSString *)inKey;
- (OSErr)saveUserData:(UserData)inUserData toDefaults:(NSUserDefaults *)inDefaults withKey:(NSString *)outKey;

@end