//
//  VirtualEnvironment.m
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

#import "VirtualEnvironment.h"
#import "ARViewController.h"

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import "glStateCache.h"
#import <string.h>
#import <sys/param.h> // MAXPATHLEN
#import <Eden/EdenMath.h>
#import "VEObject.h"

// --------
//
// A registry of VEObject types.
//

// We use a linked list for the registry, since the registry will be populated
// prior to main() and it's not guaranteed that Objective C array classes e.g.
// NSMutableArray are available prior to main() being called.
typedef struct VEObjectRegistryEntry {
    Class type;
    NSString *extension;
    struct VEObjectRegistryEntry *next;
} VEObjectRegistryEntry_t;

// The head of the linked list.
static VEObjectRegistryEntry_t *registry = NULL;

// This function cleans up the registry at program exit.
static void VEObjectRegistryFinal()
{
    while (registry) {
        VEObjectRegistryEntry_t *tofree = registry;
        registry = tofree->next;
        [tofree->extension release];
        free(tofree);
    }
}

void VEObjectRegistryRegister(const Class type, const NSString *extension)
{
    if (!registry) atexit(VEObjectRegistryFinal); // Register a cleanup handler.
    
    VEObjectRegistryEntry_t *entry = (VEObjectRegistryEntry_t *)malloc(sizeof(VEObjectRegistryEntry_t));
    entry->next = registry;
    registry = entry;
    
    entry->type = type;
    entry->extension = [extension copy];
}

Class VEObjectRegistryGetClassForExtension(const NSString *extension)
{
    VEObjectRegistryEntry_t *entry = registry;
    while (entry) {
        if (entry->extension && [extension isEqualToString:entry->extension]) return (entry->type);
        entry = entry->next;
    }
    return (nil);
}

// --------

static char *get_buff(char *buf, int n, FILE *fp, int skipblanks)
{
    char *ret;
    
    do {
        ret = fgets(buf, n, fp);
        if (ret == NULL) return (NULL); // EOF or error.
        
        // Remove NLs and CRs from end of string.
        size_t l = strlen(buf);
        while (l > 0) {
            if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
            l--;
            buf[l] = '\0';
        }
    } while (buf[0] == '#' || (skipblanks && buf[0] == '\0')); // Reject comments and blank lines.
    
    return (ret);
}

// --------

@implementation VirtualEnvironment {
@private
    ARViewController *arViewController;
    NSMutableArray *objects;
}

@synthesize arViewController;

- (VirtualEnvironment *) initWithARViewController:(ARViewController *)vc
{
    if ((self = [super init])) {
        arViewController = vc;
        
        // Go through the VEObject registry and call +virtualEnvironmentIsBeingCreated
        // on any objects that implement it.
        VEObjectRegistryEntry_t *entry = registry;
        while (entry) {
            if ([entry->type conformsToProtocol:@protocol(VEObjectRegistryEntryIsInterestedInVirtualEnvironmentLifespan)]) {
                [entry->type virtualEnvironmentIsBeingCreated:self];
            }
            entry = entry->next;
        }
        
        objects = [[NSMutableArray alloc] init];
    }
    return (self);
}

- (void)dealloc
{
    [objects makeObjectsPerformSelector:@selector(willBeRemovedFromEnvironment:) withObject:self];
    [objects release];

    // Go through the VEObject registry and call +virtualEnvironmentWillBeDestroyed
    // on any objects that implement it.
    VEObjectRegistryEntry_t *entry = registry;
    while (entry) {
        if ([entry->type conformsToProtocol:@protocol(VEObjectRegistryEntryIsInterestedInVirtualEnvironmentLifespan)]) {
            [entry->type virtualEnvironmentIsBeingDestroyed:self];
        }
        entry = entry->next;
    }
    
    [super dealloc];
}

- (void) addObject:(VEObject *)object
{
    [objects addObject:object];
    [object wasAddedToEnvironment:self];
}

- (void) removeObject:(VEObject *)object
{
    [object willBeRemovedFromEnvironment:self];
    [objects removeObject:object];
}

- (int) addObjectsFromObjectListFile:(NSString *)objectDataFilePath connectToARMarkers:(NSArray *)markers
{
    return ([self addObjectsFromObjectListFile:objectDataFilePath connectToARMarkers:markers autoParentTo:nil]);
}

- (int) addObjectsFromObjectListFile:(NSString *)objectDataFilePath connectToARMarkers:(NSArray *)markers autoParentTo:(VEObject *)autoParent
{
    NSString *objectDataFileFullPath;
    FILE *fp;
    char buf[MAXPATHLEN];
    int i;
    ARdouble translation[3], rotation[4], scale[3];
    int objectsAdded = 0;

    // Locate and open the objects description file.
    if ([objectDataFilePath hasPrefix:@"/"]) {
        objectDataFileFullPath = objectDataFilePath;
    } else {
        objectDataFileFullPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:objectDataFilePath];
    }
    char objectDataFileFullpathC[MAXPATHLEN];
    [objectDataFileFullPath getFileSystemRepresentation:objectDataFileFullpathC maxLength:MAXPATHLEN];
    if ((fp = fopen(objectDataFileFullpathC, "r")) == NULL) {
        NSLog(@"Error: unable to locate object data file %@.\n", objectDataFileFullPath);
        return (objectsAdded);
    }
    
    // First line is number of objects to read.
    int numObjects = 0;
    get_buff(buf, MAXPATHLEN, fp, 1);
    if (sscanf(buf, "%d", &numObjects) != 1 ) {
        NSLog(@"Error: unable to read number of objects to load from object data file.\n");
        fclose(fp);
        return (objectsAdded);
    }
    
#ifdef DEBUG
    NSLog(@"Reading %d objects.\n", numObjects);
#endif
    for (i = 0; i < numObjects; i++) {
        
        // Read in all info relating to the object.
        
        // Read model file path (relative to objects description file).
        char objectFullpathC[MAXPATHLEN];
        if (!get_buff(buf, MAXPATHLEN, fp, 1)) {
            NSLog(@"Error: unable to read model file name from object data file.\n");
            fclose(fp);
            return (objectsAdded);
        }
        if (!arUtilGetDirectoryNameFromPath(objectFullpathC, objectDataFileFullpathC, sizeof(objectFullpathC), 1)) { // Get directory prefix.
            fclose(fp);
            return (objectsAdded);
        } 
        strncat(objectFullpathC, buf, sizeof(objectFullpathC) - strlen(objectFullpathC) - 1); // Add name of file to open.

        // Read translation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &translation[0], &translation[1], &translation[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &translation[0], &translation[1], &translation[2]) != 3)
#endif
        {
            fclose(fp);
            return (objectsAdded);
        }
        // Read rotation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f %f", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#else
        if (sscanf(buf, "%lf %lf %lf %lf", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#endif
        {
            fclose(fp);
            return (objectsAdded);
        }
        // Read scale.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &scale[0], &scale[1], &scale[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &scale[0], &scale[1], &scale[2]) != 3)
#endif
        {
            fclose(fp);
            return (objectsAdded);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        int lightingFlag = 1;
        int autoParentFlag = 0;
        char *bufp;
        char *config = NULL;
        int markerIndex = -1;
        char *markerName = NULL;
        ARMarker *marker = nil;

        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "LIGHTING", 8) == 0) {
                if (sscanf(&(buf[8]), " %d", &lightingFlag) != 1) {
                    NSLog(@"Error in object file: LIGHTING token must be followed by an integer >= 0. Discarding.\n");
                }
            } else if (strncmp(buf, "MARKER_NAME", 11) == 0) {
                bufp = buf + 11;
                while (*bufp == ' ' || *bufp == '\t') bufp++; // Skip whitespace.
                markerName = strdup(bufp);
            } else if (strncmp(buf, "MARKER", 6) == 0) {
                if (sscanf(&(buf[6]), " %d", &markerIndex) != 1) {
                    NSLog(@"Error in object file: MARKER token must be followed by an integer > 0. Discarding.\n");
                } else {
                    markerIndex--; // Marker numbers are zero-indexed, but in the config file they're 1-indexed.
                }
            } else if (strncmp(buf, "CONFIG", 6) == 0) {
                bufp = buf + 6;
                while (*bufp == ' ' || *bufp == '\t') bufp++; // Skip whitespace.
                config = strdup(&buf[7]);
            } else if (strncmp(buf, "AUTOPARENT", 10) == 0) {
                autoParentFlag = 1;
            }
            // Unknown tokens are ignored.
        }

        // Got all options. Now locate the VEObject subclass which should load it.
        NSString *objectFullpath = [NSString stringWithCString:objectFullpathC encoding:NSUTF8StringEncoding];
        NSString *objectPathExt = [[objectFullpath pathExtension] lowercaseString];
        Class type = VEObjectRegistryGetClassForExtension(objectPathExt);
        if (!type) {
            NSLog(@"Error: unsupported model file type (%@). Ignoring.\n", objectPathExt);
            continue;
        }

        // Now attempt to init the object.
#ifdef DEBUG
        NSLog(@"Reading object data file %@.\n", objectFullpath);
#endif
        VEObject *tempObject;
        tempObject = [(VEObject *)[type alloc] initFromFile:objectFullpath translation:translation rotation:rotation scale:scale config:config];
        if (!tempObject) {
            NSLog(@"Error attempting to read object data file (%@).\n", objectFullpath);
            continue;
        }
        
        // Set optional properties.
        tempObject.lit = (lightingFlag ? TRUE : FALSE);
        
        // If a valid marker name has been specified, connect the VEObject to notifications from the referred-to ARMarker.
        if (markers) {
            if (markerName && *markerName) {
                marker = [ARMarker findMarkerWithName:[NSString stringWithUTF8String:markerName] inMarkers:markers];
            } else if (markerIndex >= 0 && markerIndex < [markers count]) {
                marker = [markers objectAtIndex:(NSUInteger)markerIndex];
            }
            if (marker) {
                tempObject.visible = FALSE; // Objects tied to markers will not be initially visible.
                [tempObject startObservingARMarker:marker];
            }
        }
        
        if (autoParentFlag && autoParent) {
            tempObject.visible = FALSE; // Child objects will not be initially visible.
            [autoParent addChild:tempObject];
        }
        [self addObject:tempObject];
        
        [tempObject release];
        if (config) free(config);
        if (markerName) free(markerName);
        objectsAdded++;
        
    } // for (numObjects);
    
    fclose(fp);
    return (objectsAdded);
}

- (void) updateWithSimulationTime:(NSTimeInterval)timeDelta {
}

@end
