/*
 *	VirtualEnvironment2.c
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2010-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb.
 *
 */

#include "VirtualEnvironment2.h"
#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif
#ifdef USE_OPENGL_ES
#  ifdef ANDROID
#    include <GLES/gl.h>
#    include <GLES/glext.h>
#  else
#    include <OpenGLES/ES1/gl.h>
#    include <OpenGLES/ES1/glext.h>
#  endif
#  include "glStateCache.h"
#else
#  if defined(__APPLE__)
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif
#include <string.h>
#include <stdio.h>
#include <AR/arosg.h>

typedef struct _VEObject {
    int modelIndex;
    int markerIndex;
} VEObject;

enum viewPortIndices {
    viewPortIndexLeft = 0,
    viewPortIndexBottom,
    viewPortIndexWidth,
    viewPortIndexHeight
};

struct _VirtualEnvironment2 {
    VEObject *objects;
    int objectCount;
    AROSG *arOSG;
    int viewPort[4];
    ARdouble projection[16];
};

static char *get_buff(char *buf, int n, FILE *fp, int skipblanks)
{
    char *ret;
	size_t l;
    
    do {
        ret = fgets(buf, n, fp);
        if (ret == NULL) return (NULL); // EOF or error.
        
        // Remove NLs and CRs from end of string.
        l = strlen(buf);
        while (l > 0) {
            if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
            l--;
            buf[l] = '\0';
        }
    } while (buf[0] == '#' || (skipblanks && buf[0] == '\0')); // Reject comments and blank lines.
    
    return (ret);
}

VirtualEnvironment2 *VirtualEnvironment2Init(const char *objectListFile)
{
    VirtualEnvironment2 *ve2;
    int numObjects;
    FILE *fp;
    char buf[MAXPATHLEN];
    char objectFullpath[MAXPATHLEN];
    int i;
    ARdouble translation[3], rotation[4], scale[3];
    int lightingFlag, markerIndex;
    
    ARLOGd("Initialising Virtual Environment.\n");

    // Init to NULL.
    ve2 = (VirtualEnvironment2 *)calloc(1, sizeof(VirtualEnvironment2));
    if (!ve2) {
        ARLOGe("Out of memory!\n.");
        return (NULL);
    }
    
    // One-time OSG initialisation.
    ve2->arOSG = arOSGInit();
    if (!ve2->arOSG) {
        ARLOGe("Error: unable to init arOSG library.\n");
        goto bail;
    }
    
    // Locate and open the objects description file.
    if ((fp = fopen(objectListFile, "r")) == NULL) {
        ARLOGe("Error: unable to open object data file '%s'.\n", objectListFile);
        perror(NULL);
        goto bail1;
    }
    
    // First line is number of objects to read.
    numObjects = 0;
    get_buff(buf, MAXPATHLEN, fp, 1);
    if (sscanf(buf, "%d", &numObjects) != 1 ) {
        ARLOGe("Error: unable to read number of objects to load from object data file.\n");
        goto bail2;
    }
    
    // Allocate space for the objects.
    ve2->objects = (VEObject *)calloc(numObjects, sizeof(VEObject));
    if (!ve2->objects) {
        ARLOGe("Out of memory!\n.");
        goto bail2;
    }
    
    ARLOGd("Reading %d objects.\n", numObjects);
    for (i = 0; i < numObjects; i++) {
        
        // Read in all info relating to the object.
        
        // Read model file path (relative to objects description file).
        if (!get_buff(buf, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error: unable to read model file name from object data file.\n");
            goto bail3;
        }
        if (!arUtilGetDirectoryNameFromPath(objectFullpath, objectListFile, sizeof(objectFullpath), 1)) { // Get directory prefix, with path separator.
            goto bail3;
        }
        strncat(objectFullpath, buf, sizeof(objectFullpath) - strlen(objectFullpath) - 1); // Add name of file to open.
        
        // Read translation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &translation[0], &translation[1], &translation[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &translation[0], &translation[1], &translation[2]) != 3)
#endif
        {
            goto bail3;
        }
        // Read rotation.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f %f", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#else
        if (sscanf(buf, "%lf %lf %lf %lf", &rotation[0], &rotation[1], &rotation[2], &rotation[3]) != 4)
#endif
        {
            goto bail3;
        }
        // Read scale.
        get_buff(buf, MAXPATHLEN, fp, 1);
#ifdef ARDOUBLE_IS_FLOAT
        if (sscanf(buf, "%f %f %f", &scale[0], &scale[1], &scale[2]) != 3)
#else
        if (sscanf(buf, "%lf %lf %lf", &scale[0], &scale[1], &scale[2]) != 3)
#endif
        {
            goto bail3;
        }
        
        // Look for optional tokens. A blank line marks end of options.
        lightingFlag = 1; markerIndex = -1;
        
        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "LIGHTING", 8) == 0) {
                if (sscanf(&(buf[8]), " %d", &lightingFlag) != 1) {
                    ARLOGe("Error in object file: LIGHTING token must be followed by an integer >= 0. Discarding.\n");
                }
            } else if (strncmp(buf, "MARKER", 6) == 0) {
                if (sscanf(&(buf[6]), " %d", &markerIndex) != 1) {
                    ARLOGe("Error in object file: MARKER token must be followed by an integer > 0. Discarding.\n");
                } else {
                    markerIndex--; // Marker numbers are zero-indexed, but in the config file they're 1-indexed.
                }
            }
            // Unknown tokens are ignored.
        }
        

        // Now attempt to load objects.
        ARLOGd("Reading object data file %s.\n", objectFullpath);
        ve2->objects[i].modelIndex = arOSGLoadModel2(ve2->arOSG, objectFullpath, translation, rotation, scale);
        if (ve2->objects[i].modelIndex < 0) {
            ARLOGe("Error attempting to read object data file %s.\n", objectFullpath);
            goto bail4;
        }
        
        // Set optional properties.
        arOSGSetModelLighting(ve2->arOSG, ve2->objects[i].modelIndex, lightingFlag);
        
        // If a valid marker index has been specified, save it.
        if (markerIndex >= 0 /*&& markerIndex < markersCount]*/) {
            arOSGSetModelVisibility(ve2->arOSG, ve2->objects[i].modelIndex, FALSE); // Objects tied to markers will not be initially visible.
            ve2->objects[i].markerIndex = markerIndex;
        } else {
            arOSGSetModelVisibility(ve2->arOSG, ve2->objects[i].modelIndex, TRUE); // All other objects will be initially visible.
            ve2->objects[i].markerIndex = -1;
        }
        
        (ve2->objectCount)++;

    }
    
    ARLOGd("Virtual Environment initialised.\n");
    fclose(fp);
    return (ve2);

bail4:
    for (i--; i >= 0; i--) {
        arOSGUnloadModel(ve2->arOSG, i);
    }
bail3:
    free(ve2->objects);
    ve2->objects = NULL;
    ve2->objectCount = 0;
bail2:
    fclose(fp);
bail1:
    arOSGFinal(ve2->arOSG);
bail:
    free(ve2);
#ifdef DEBUG
    ARLOGe("Virtual Environment initialisation failed.\n");
#endif
    return (NULL);
}

void VirtualEnvironment2Final(VirtualEnvironment2 **ve2_p)
{
    int i;
    
    if (!ve2_p || !(*ve2_p)) return;

    if ((*ve2_p)->objects) {
        // Unload all objects.
        for (i = 0; i < (*ve2_p)->objectCount; i++) {
            arOSGUnloadModel((*ve2_p)->arOSG, i);
        }
        free((*ve2_p)->objects);
        (*ve2_p)->objects = NULL;
        (*ve2_p)->objectCount = 0;
    }
    
    // Cleanup all-model state.
    if ((*ve2_p)->arOSG) {
        arOSGFinal((*ve2_p)->arOSG);
        (*ve2_p)->arOSG = NULL;
    }
    
    free(*ve2_p);
    *ve2_p = NULL;
}

void VirtualEnvironment2HandleARMarkerWasUpdated(VirtualEnvironment2 *ve2, int markerIndex, ARPose poseIn)
{
    int i;

    if (!ve2 || !ve2->arOSG) return;

    // Look through all objects for objects which are linked to this marker.
    for (i = 0; i < ve2->objectCount; i++) {
        if (ve2->objects[i].markerIndex == markerIndex) {
            arOSGSetModelPose(ve2->arOSG, ve2->objects[i].modelIndex, poseIn.T);
        }
    }
}

void VirtualEnvironment2HandleARMarkerAppeared(VirtualEnvironment2 *ve2, int markerIndex)
{
    int i;

    if (!ve2 || !ve2->arOSG) return;

    // Look through all objects for objects which are linked to this marker.
    for (i = 0; i < ve2->objectCount; i++) {
        if (ve2->objects[i].markerIndex == markerIndex) {
            arOSGSetModelVisibility(ve2->arOSG, ve2->objects[i].modelIndex, TRUE);
        }
    }
}

void VirtualEnvironment2HandleARMarkerDisappeared(VirtualEnvironment2 *ve2, int markerIndex)
{
    int i;
    
    if (!ve2 || !ve2->arOSG) return;

    // Look through all objects for objects which are linked to this marker.
    for (i = 0; i < ve2->objectCount; i++) {
        if (ve2->objects[i].markerIndex == markerIndex) {
            arOSGSetModelVisibility(ve2->arOSG, ve2->objects[i].modelIndex, FALSE);
        }
    }
}

void VirtualEnvironment2HandleARViewUpdatedCameraLens(VirtualEnvironment2 *ve2, ARdouble *projection_in)
{
	int i;

    if (!ve2) return;

    for (i = 0; i < 16; i++) ve2->projection[i] = projection_in[i];
    // Wait until the OSG viewer is valid to set the projection.
}

void VirtualEnvironment2HandleARViewUpdatedViewport(VirtualEnvironment2 *ve2, int *viewPort_in)
{
    if (!ve2 || !ve2->arOSG) return;
    
    ve2->viewPort[0] = viewPort_in[0]; ve2->viewPort[1] = viewPort_in[1]; ve2->viewPort[2] = viewPort_in[2]; ve2->viewPort[3] = viewPort_in[3];
    arOSGHandleReshape2(ve2->arOSG, viewPort_in[viewPortIndexLeft], viewPort_in[viewPortIndexBottom], viewPort_in[viewPortIndexWidth], viewPort_in[viewPortIndexHeight]);
    // Also, since at this point the OSG viewer is valid, we can set the projection now.
    arOSGSetProjection(ve2->arOSG, ve2->projection);
}

void VirtualEnvironment2HandleARViewDrawPreCamera(VirtualEnvironment2 *ve2)
{
    if (!ve2 || !ve2->arOSG) return;
        
#ifdef USE_OPENGL_ES
    // Set some state to OSG's expected values.
    glStateCacheDisableLighting();
    glStateCacheDisableTex2D();
    glStateCacheDisableBlend();
    glStateCacheEnableClientStateVertexArray();
    glStateCacheEnableClientStateNormalArray();
    glStateCacheEnableClientStateTexCoordArray();
#endif

    // Save the projection and modelview state.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    // Draw the whole scenegraph.
    arOSGDraw(ve2->arOSG);
    
    // OSG modifies the viewport, so restore it.
    // Also restore projection and modelview.
    glViewport(ve2->viewPort[viewPortIndexLeft], ve2->viewPort[viewPortIndexBottom], ve2->viewPort[viewPortIndexWidth], ve2->viewPort[viewPortIndexHeight]);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
#ifdef USE_OPENGL_ES
    // Flush the state cache and ensure depth testing is enabled.
    glStateCacheFlush();
    glStateCacheEnableDepthTest();
#else
    // Ensure depth testing is re-enabled.
    glEnable(GL_DEPTH_TEST);
#endif
}

void VirtualEnvironment2HandleARViewDrawPostCamera(VirtualEnvironment2 *ve2)
{
    // NOP.
}

void VirtualEnvironment2HandleARViewDrawOverlay(VirtualEnvironment2 *ve2)
{
    // NOP.
}
