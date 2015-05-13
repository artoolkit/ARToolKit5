/*
 *	Copyright (c) 2005-2012 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2005-03-08	PRL		Written.
 *
 */

#include <AR/video.h>

#ifdef AR_INPUT_QUICKTIME

#import "ARVideoSettingsController.h"

@implementation ARVideoSettingsController

- (id)initInput:(int)inInputIndex withSeqGrabComponent:(SeqGrabComponent)inSeqGrab withSGChannel:(SGChannel)inSgchanVideo
{
	ComponentResult err;
	
	self = [super init];

	// initialize NSApplication, using an entry point that is specific to bundles
    // this is a no-op for Cocoa apps, but is required by Carbon apps
	NSApplicationLoad();
	
	inputIndex = inInputIndex;
	seqGrab = inSeqGrab;
	sgchanVideo = inSgchanVideo;
	
	// Grab the defaults.
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	// Try to load the version key, used to see if we have any saved settings.
	mVersion = [defaults floatForKey:@"version"];
	if (!mVersion) {
		// No previous defaults so define new.
		mVersion = 1;
		
		// Write out the defaults.
		[defaults setInteger:mVersion forKey:@"version"];
		[defaults synchronize];
	}
	
	// Load defaults, first time though mUserData may be 0 which is fine.
	[self loadUserData:&mUserData fromDefaults:defaults forKey:[NSString stringWithFormat:@"sgVideoSettings%03i", inputIndex]];
	if (mUserData) {
		if ((err = SGSetChannelSettings(seqGrab, sgchanVideo, mUserData, 0)) != noErr) {
			ARLOGe("-sgConfigurationDialog: error %ld in SGSetChannelSettings().\n", err);
		}
	}
	
	return self;
	
}

- (void)dealloc
{
	if (mUserData) DisposeUserData(mUserData);
	[super dealloc];
}

// NOTE: There is a know bug in QuickTime 6.4 in which the
// Settings Dialog pops up in random locations...sigh...
- (IBAction) sgConfigurationDialog:(id)sender withStandardDialog:(int)standardDialog
{
	// Set up the settings panel list removing the "Compression" panel.
	ComponentResult			err = -1;

	if (standardDialog) {
		err = SGSettingsDialog(seqGrab, sgchanVideo, 0, 0, seqGrabSettingsPreviewOnly, NULL, 0L);
	} else {
		ComponentDescription	cDesc;
		long					cCount;
		ComponentDescription	cDesc2;
		
		cDesc.componentType = SeqGrabPanelType;
		cDesc.componentSubType = VideoMediaType;
		cDesc.componentManufacturer = cDesc.componentFlags = cDesc.componentFlagsMask = 0L;
		cCount = CountComponents(&cDesc);
		if (cCount == 0) {
			ARLOGe("-sgConfigurationDialog: error in CountComponents().\n");
			goto bail0;
		}
		
		Component *PanelListPtr = (Component *)NewPtr(sizeof(Component) * (cCount + 1));
		if ((err = MemError()) || NULL == PanelListPtr) {
			ARLOGe("-sgConfigurationDialog: error in NewPtr().\n");
			goto bail;
		}
		Component *cPtr = PanelListPtr;
		long PanelCount = 0L;	
		Component c = NULL;
		while ((c = FindNextComponent(c, &cDesc)) != NULL) {
			Handle hName = NewHandleClear(0);
			if ((err = MemError()) || NULL == hName) {
				ARLOGe("-sgConfigurationDialog: error in NewHandle().\n");
				goto bail;
			}
			
			GetComponentInfo(c, &cDesc2, hName, NULL, NULL);
			if (PLstrcmp(*(unsigned char **)hName, "\pCompression") != 0) {
				*cPtr = c;
				cPtr++;
				PanelCount++;
			}
			DisposeHandle(hName);
		}
		
		// Bring up the dialog and if the user didn't cancel
		// save the new channel settings for later.
		err = SGSettingsDialog(seqGrab, sgchanVideo, PanelCount, PanelListPtr, seqGrabSettingsPreviewOnly, NULL, 0L);
bail:
		DisposePtr((Ptr)PanelListPtr);
bail0:
        ;
	}

	if (err == noErr) {
		// Dispose the old settings and get the new channel settings.
		if (mUserData) DisposeUserData(mUserData);
		if ((err = SGGetChannelSettings(seqGrab, sgchanVideo, &mUserData, 0)) != noErr) {
			ARLOGe("-sgConfigurationDialog: error %ld in SGGetChannelSettings().\n", err);
		}
		
		// Grab the defaults.
		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
		
		// Write the defaults.
		[self saveUserData:mUserData toDefaults:defaults withKey:[NSString stringWithFormat:@"sgVideoSettings%03i", inputIndex]];
		[defaults synchronize];
		
	} else if (err != userCanceledErr) {
		ARLOGe("-sgConfigurationDialog: error %ld in SGSettingsDialog().\n", err);
	}
}

// Get the Channel Settings as UserData from the preferences
- (OSErr)loadUserData:(UserData *)outUserData fromDefaults:(NSUserDefaults *)inDefaults forKey:(NSString *)inKey
{
	NSData   *theSettings;
	Handle   theHandle = NULL;
	UserData theUserData = NULL;
	OSErr    err = paramErr;
	
	// read the new setttings from our preferences
	theSettings = [inDefaults objectForKey:inKey];
	
	if (theSettings) {
		err = PtrToHand([theSettings bytes], &theHandle, [theSettings length]);
		
		if (theHandle) {
			err = NewUserDataFromHandle(theHandle, &theUserData);
			if (theUserData) {
				*outUserData = theUserData;
			}
			DisposeHandle(theHandle);
		}
	}
	
	return err;
}

// Save the Channel Settings from UserData in the preferences
- (OSErr)saveUserData:(UserData)inUserData toDefaults:(NSUserDefaults *)inDefaults withKey:(NSString *)outKey
{
	NSData *theSettings;
	Handle hSettings;
	OSErr  err;
	
	if (NULL == inUserData) return paramErr;
	
	hSettings = NewHandle(0);
	err = MemError();
	
	if (noErr == err) {
		err = PutUserDataIntoHandle(inUserData, hSettings); 
		
		if (noErr == err) {
			HLock(hSettings);
			theSettings = [NSData dataWithBytes:(UInt8 *)*hSettings length:GetHandleSize(hSettings)];
			
			// save the new setttings to our preferences
			if (theSettings) {
				[inDefaults setObject:theSettings forKey:outKey];
				[inDefaults synchronize];
			}
		}
		
		DisposeHandle(hSettings);
	}
	
	return err;
}

@end

OSStatus RequestSGSettings(const int inputIndex, SeqGrabComponent seqGrab, SGChannel sgchanVideo, const int showDialog, const int standardDialog)
{
    NSAutoreleasePool *localPool;
	ARVideoSettingsController *localController;
	
	// Sanity check.
	if (inputIndex < 0 || !seqGrab || !sgchanVideo) return (paramErr);

    // Calling Objective-C from C necessitates an autorelease pool.
	localPool = [[NSAutoreleasePool alloc] init];
    
    localController = [[ARVideoSettingsController alloc] initInput:inputIndex withSeqGrabComponent:seqGrab withSGChannel:sgchanVideo];
	if (showDialog) [localController sgConfigurationDialog:NULL withStandardDialog:standardDialog];
	[localController release];
	
    [localPool release];
    return (noErr);
}

#endif // AR_INPUT_QUICKTIME
