/*
 *  ARWrapperNativeExample.cpp
 *  ARToolKit for Android
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
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Julian Looser, Philip Lamb
 */

#include <AR/gsub_es.h>
#include <Eden/glm.h>
#include <jni.h>
#include <ARWrapper/ARToolKitWrapperExportedAPI.h>
#include <unistd.h> // chdir()
#include <android/log.h>

// Utility preprocessor directive so only one change needed if Java class name changes
#define JNIFUNCTION_DEMO(sig) Java_org_artoolkit_ar_samples_ARSimpleNative_SimpleNativeRenderer_##sig

extern "C" {
	JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoInitialise(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoShutdown(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoSurfaceCreated(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h));
	JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoDrawFrame(JNIEnv* env, jobject obj));	
};

static int markerID = -1;

JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoInitialise(JNIEnv* env, jobject object)) {
	markerID = arwAddMarker("single;Data/hiro.patt;80");
}

JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoShutdown(JNIEnv* env, jobject object)) {
}

JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoSurfaceCreated(JNIEnv* env, jobject object)) {
	glStateCacheFlush(); // Make sure we don't hold outdated OpenGL state.
}

JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h)) {
	// glViewport(0, 0, w, h) has already been set.
}

static void drawCube(float size, float x, float y, float z)
{
    // Colour cube data.
    int i;
    const GLfloat cube_vertices [8][3] = {
        /* +z */ {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        /* -z */ {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f} };
    const GLubyte cube_vertex_colors [8][4] = {
        {255, 255, 255, 255}, {255, 255, 0, 255}, {0, 255, 0, 255}, {0, 255, 255, 255},
        {255, 0, 255, 255}, {255, 0, 0, 255}, {0, 0, 0, 255}, {0, 0, 255, 255} };
    const GLushort cube_faces [6][4] = { /* ccw-winding */
        /* +z */ {3, 2, 1, 0}, /* -y */ {2, 3, 7, 6}, /* +y */ {0, 1, 5, 4},
        /* -x */ {3, 0, 4, 7}, /* +x */ {1, 2, 6, 5}, /* -z */ {4, 5, 6, 7} };
    
    glPushMatrix(); // Save world coordinate system.
    glTranslatef(x, y, z);
    glScalef(size, size, size);
    glStateCacheDisableLighting();
    glStateCacheDisableTex2D();
    glStateCacheDisableBlend();
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, cube_vertex_colors);
    glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
    glStateCacheEnableClientStateVertexArray();
    glEnableClientState(GL_COLOR_ARRAY);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
    }
    glDisableClientState(GL_COLOR_ARRAY);
    glColor4ub(0, 0, 0, 255);
    for (i = 0; i < 6; i++) {
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, &(cube_faces[i][0]));
    }
    glPopMatrix();    // Restore world coordinate system.
}

JNIEXPORT void JNICALL JNIFUNCTION_DEMO(demoDrawFrame(JNIEnv* env, jobject obj)) {
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    // Set the projection matrix to that provided by ARToolKit.
	float proj[16];
	arwGetProjectionMatrix(proj);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(proj);
	glMatrixMode(GL_MODELVIEW);
	
	glStateCacheEnableDepthTest();
	glStateCacheDisableLighting();	
	glStateCacheDisableTex2D();	

	if (arwQueryMarkerVisibility(markerID)) {

		float trans[16];
		arwQueryMarkerTransformation(markerID, trans);
		
		glLoadMatrixf(trans);		
		drawCube(40.0f, 0.0f, 0.0f, 20.0f);

	}
	
}
