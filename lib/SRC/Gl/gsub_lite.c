/*
 *  gsub_lite.c
 *  ARToolKit5
 *
 *	Graphics Subroutines (Lite) for ARToolKit.
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
 *  Copyright 2003-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

// ============================================================================
//	Private includes.
// ============================================================================

#include <AR/gsub_lite.h>

#include <stdio.h>		// fprintf(), stderr
#include <string.h>		// strchr(), strstr(), strlen()
#ifndef __APPLE__
#  include <GL/glu.h>
#  ifdef _WIN32
#    include "GL/glext.h"
#    include "GL/wglext.h"
#  else
#    ifdef GL_VERSION_1_2
#      include <GL/glext.h>
#    endif
#  endif
#else
#  include <OpenGL/glu.h>
#  include <OpenGL/glext.h>
#endif


// ============================================================================
//	Private types and defines.
// ============================================================================
#ifdef _MSC_VER
#  pragma warning (disable:4068)	// Disable MSVC warnings about unknown pragmas.
#endif

#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 1 // Edit as required.
#endif

// Make sure that required OpenGL constant definitions are available at compile-time.
// N.B. These should not be used unless the renderer indicates (at run-time) that it supports them.

// Define constants for extensions which became core in OpenGL 1.2
#ifndef GL_VERSION_1_2
#  if GL_EXT_bgra
#    define GL_BGR							GL_BGR_EXT
#    define GL_BGRA							GL_BGRA_EXT
#  else
#    define GL_BGR							0x80E0
#    define GL_BGRA							0x80E1
#  endif
#  ifndef GL_APPLE_packed_pixels
#    define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#    define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#    define GL_UNSIGNED_INT_8_8_8_8         0x8035
#    define GL_UNSIGNED_SHORT_5_6_5         0x8363
#    define GL_UNSIGNED_SHORT_5_6_5_REV     0x8364
#    define GL_UNSIGNED_SHORT_4_4_4_4_REV   0x8365
#    define GL_UNSIGNED_SHORT_1_5_5_5_REV   0x8366
#    define GL_UNSIGNED_INT_8_8_8_8_REV     0x8367
#  endif
#  if GL_SGIS_texture_edge_clamp
#    define GL_CLAMP_TO_EDGE				GL_CLAMP_TO_EDGE_SGIS
#  else
#    define GL_CLAMP_TO_EDGE				0x812F
#  endif
#endif

// Define constants for extensions which became core in OpenGL 3.1
#ifndef GL_VERSION_3_1
#  if GL_NV_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_NV
#    define GL_PROXY_TEXTURE_RECTANGLE		GL_PROXY_TEXTURE_RECTANGLE_NV
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_NV
#  elif GL_EXT_texture_rectangle
#    define GL_TEXTURE_RECTANGLE            GL_TEXTURE_RECTANGLE_EXT
#    define GL_PROXY_TEXTURE_RECTANGLE      GL_PROXY_TEXTURE_RECTANGLE_EXT
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT
#  else
#    define GL_TEXTURE_RECTANGLE            0x84F5
#    define GL_PROXY_TEXTURE_RECTANGLE      0x84F7
#    define GL_MAX_RECTANGLE_TEXTURE_SIZE   0x84F8
#  endif
#endif

// Define constants for extensions (not yet core).
#ifndef GL_APPLE_ycbcr_422
#  define GL_YCBCR_422_APPLE				0x85B9
#  define GL_UNSIGNED_SHORT_8_8_APPLE		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE	0x85BB
#endif
#ifndef GL_EXT_abgr
#  define GL_ABGR_EXT						0x8000
#endif
#ifndef GL_MESA_ycbcr_texture
#  define GL_YCBCR_MESA						0x8757
#  define GL_UNSIGNED_SHORT_8_8_MESA		0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_MESA	0x85BB
#endif

// On Windows, all OpenGL v1.5 and later API must be dynamically resolved against the actual driver.
#ifdef _WIN32
PFNGLGENBUFFERSPROC glGenBuffers = NULL; // (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffersARB");
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture = NULL;
#endif

//#define ARGL_DEBUG

struct _ARGL_CONTEXT_SETTINGS {
    ARParam arParam;
    ARHandle *arhandle; // Not used except for debug mode.
    GLuint  texture;
    float   *t2;
    float   *v2;
    GLuint  t2bo;     // Vertex buffer object for t2 data.
    GLuint  v2bo;     // Vertex buffer object for v2 data.
    float   zoom;
    GLint   textureSizeMax;
    GLsizei textureSizeX;
    GLsizei textureSizeY;
    GLenum  pixIntFormat;
    GLenum  pixFormat;
    GLenum  pixType;
    GLenum  pixSize;
    AR_PIXEL_FORMAT format;
    int     disableDistortionCompensation;
    int     textureGeometryHasBeenSetup;
    int     textureObjectsHaveBeenSetup;
    int     rotate90;
    int     flipH;
    int     flipV;
    GLsizei bufSizeX;
    GLsizei bufSizeY;
    int     bufSizeIsTextureSize;
    int     textureDataReady;
    int	    arglTexmapMode;
};
typedef struct _ARGL_CONTEXT_SETTINGS ARGL_CONTEXT_SETTINGS;


// ============================================================================
//	Private globals.
// ============================================================================


#pragma mark -
// ============================================================================
//	Private functions.
// ============================================================================

#if !ARGL_DISABLE_DISP_IMAGE && !EMSCRIPTEN
// Sets texture, t2, v2, textureGeometryHasBeenSetup.
static char arglSetupTextureGeometry(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    float    ty_prev, tx, ty;
    float    y_prev, x, y;
    ARdouble x1, x2, y1, y2;
    float    xx1, xx2, yy1, yy2;
    int      i, j;
    int      vertexCount, t2count, v2count;
    float    imageSizeX, imageSizeY;
    float    zoom;
    
    // Delete previous geometry, unless this is our first time here.
    if (contextSettings->textureGeometryHasBeenSetup) {
        free(contextSettings->t2);
        free(contextSettings->v2);
        glDeleteBuffers(1, &contextSettings->t2bo);
        glDeleteBuffers(1, &contextSettings->v2bo);
        contextSettings->textureGeometryHasBeenSetup = FALSE;
    }
    
    // Set up the geometry for the surface which we will texture upon.
    imageSizeX = (float)contextSettings->arParam.xsize;
    imageSizeY = (float)contextSettings->arParam.ysize;
    zoom = contextSettings->zoom;
    if (contextSettings->disableDistortionCompensation) vertexCount = 4;
    else vertexCount = 840; // 20 rows of 2 x 21 vertices.
    contextSettings->t2 = (float *)malloc(sizeof(float) * 2 * vertexCount);
    contextSettings->v2 = (float *)malloc(sizeof(float) * 2 * vertexCount);
    t2count = v2count = 0;
    if (contextSettings->disableDistortionCompensation) {
        contextSettings->t2[t2count++] = 0.0f; // Top-left.
        contextSettings->t2[t2count++] = 0.0f;
        contextSettings->v2[v2count++] = 0.0f;
        contextSettings->v2[v2count++] = imageSizeY * zoom;
        contextSettings->t2[t2count++] = 0.0f; // Bottom-left.
        contextSettings->t2[t2count++] = imageSizeY/(float)contextSettings->textureSizeY;
        contextSettings->v2[v2count++] = 0.0f;
        contextSettings->v2[v2count++] = 0.0f;
        contextSettings->t2[t2count++] = imageSizeX/(float)contextSettings->textureSizeX; // Top-right.
        contextSettings->t2[t2count++] = 0.0f;
        contextSettings->v2[v2count++] = imageSizeX * zoom;
        contextSettings->v2[v2count++] = imageSizeY * zoom;
        contextSettings->t2[t2count++] = imageSizeX/(float)contextSettings->textureSizeX; // Bottom-right.
        contextSettings->t2[t2count++] = imageSizeY/(float)contextSettings->textureSizeY;
        contextSettings->v2[v2count++] = imageSizeX * zoom;
        contextSettings->v2[v2count++] = 0.0f;
    } else {
        y = 0.0f;
        ty = 0.0f;
        for (j = 1; j <= 20; j++) {    // Do 20 rows of triangle strips.
            y_prev = y;
            ty_prev = ty;
            y = imageSizeY * (float)j / 20.0f;
            ty = y / (float)contextSettings->textureSizeY;
            
            
            for (i = 0; i <= 20; i++) { // 21 columns of triangle strip vertices, 2 vertices per column.
                x = imageSizeX * (float)i / 20.0f;
                tx = x / (float)contextSettings->textureSizeX;
                
                arParamObserv2Ideal(contextSettings->arParam.dist_factor, (ARdouble)x, (ARdouble)y_prev, &x1, &y1, contextSettings->arParam.dist_function_version);
                arParamObserv2Ideal(contextSettings->arParam.dist_factor, (ARdouble)x, (ARdouble)y,      &x2, &y2, contextSettings->arParam.dist_function_version);
                
                xx1 = (float)x1 * zoom;
                yy1 = (imageSizeY - (float)y1) * zoom;
                xx2 = (float)x2 * zoom;
                yy2 = (imageSizeY - (float)y2) * zoom;
                
                contextSettings->t2[t2count++] = tx; // Top.
                contextSettings->t2[t2count++] = ty_prev;
                contextSettings->v2[v2count++] = xx1;
                contextSettings->v2[v2count++] = yy1;
                contextSettings->t2[t2count++] = tx; // Bottom.
                contextSettings->t2[t2count++] = ty;
                contextSettings->v2[v2count++] = xx2;
                contextSettings->v2[v2count++] = yy2;
            } // columns.
        } // rows.
    }
    
    // Now setup VBOs.
    glGenBuffers(1, &contextSettings->t2bo);
    glGenBuffers(1, &contextSettings->v2bo);
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, contextSettings->t2, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->v2bo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * vertexCount, contextSettings->v2, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    contextSettings->textureGeometryHasBeenSetup = TRUE;
    return (TRUE);
}


// Set up the texture objects.
static char arglSetupTextureObjects(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLint textureWrapMode;
    
    // Delete previous textures, unless this is our first time here.
    if (contextSettings->textureObjectsHaveBeenSetup) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(contextSettings->texture));
        contextSettings->textureObjectsHaveBeenSetup = FALSE;
    }
    
    glGenTextures(1, &(contextSettings->texture));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, contextSettings->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Decide whether we can use GL_CLAMP_TO_EDGE.
    if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_SGIS_texture_edge_clamp")) {
        textureWrapMode = GL_CLAMP_TO_EDGE;
    } else {
        textureWrapMode = GL_REPEAT;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapMode);
    
    contextSettings->textureObjectsHaveBeenSetup = TRUE;
    return (TRUE);
}

#endif // !ARGL_DISABLE_DISP_IMAGE

#pragma mark -
// ============================================================================
//	Public functions.
// ============================================================================

//
// Convert a camera parameter structure into an OpenGL projection matrix.
//
void arglCameraFrustum(const ARParam *cparam, const ARdouble focalmin, const ARdouble focalmax, ARdouble m_projection[16])
{
	ARdouble    icpara[3][4];
    ARdouble    trans[3][4];
    ARdouble    p[3][3], q[4][4];
	int         width, height;
    int         i, j;
	
    width  = cparam->xsize;
    height = cparam->ysize;

    if (arParamDecompMat(cparam->mat, icpara, trans) < 0) {
        ARLOGe("arglCameraFrustum(): arParamDecompMat() indicated parameter error.\n");
        return;
    }
	for (i = 0; i < 4; i++) {
        icpara[1][i] = (height - 1)*(icpara[2][i]) - icpara[1][i];
    }
		
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0 * p[0][0] / (width - 1));
    q[0][1] = (2.0 * p[0][1] / (width - 1));
    q[0][2] = ((2.0 * p[0][2] / (width - 1))  - 1.0);
    q[0][3] = 0.0;
	
    q[1][0] = 0.0;
    q[1][1] = (2.0 * p[1][1] / (height - 1));
    q[1][2] = ((2.0 * p[1][2] / (height - 1)) - 1.0);
    q[1][3] = 0.0;
	
    q[2][0] = 0.0;
    q[2][1] = 0.0;
    q[2][2] = (focalmax + focalmin)/(focalmax - focalmin);
    q[2][3] = -2.0 * focalmax * focalmin / (focalmax - focalmin);
	
    q[3][0] = 0.0;
    q[3][1] = 0.0;
    q[3][2] = 1.0;
    q[3][3] = 0.0;
	
    for (i = 0; i < 4; i++) { // Row.
		// First 3 columns of the current row.
        for (j = 0; j < 3; j++) { // Column.
            m_projection[i + j*4] = q[i][0] * trans[0][j] +
									q[i][1] * trans[1][j] +
									q[i][2] * trans[2][j];
        }
		// Fourth column of the current row.
        m_projection[i + 3*4] = q[i][0] * trans[0][3] +
								q[i][1] * trans[1][3] +
								q[i][2] * trans[2][3] +
								q[i][3];
    }	
}

void arglCameraFrustumRH(const ARParam *cparam, const ARdouble focalmin, const ARdouble focalmax, ARdouble m_projection[16])
{
	ARdouble    icpara[3][4];
    ARdouble    trans[3][4];
    ARdouble    p[3][3], q[4][4];
	int         width, height;
    int         i, j;
	
    width  = cparam->xsize;
    height = cparam->ysize;
	
    if (arParamDecompMat(cparam->mat, icpara, trans) < 0) {
        ARLOGe("arglCameraFrustum(): arParamDecompMat() indicated parameter error.\n");
        return;
    }
	for (i = 0; i < 4; i++) {
        icpara[1][i] = (height - 1)*(icpara[2][i]) - icpara[1][i];
    }
	
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0 * p[0][0] / (width - 1));
    q[0][1] = (2.0 * p[0][1] / (width - 1));
    q[0][2] = -((2.0 * p[0][2] / (width - 1))  - 1.0);
    q[0][3] = 0.0;
	
    q[1][0] = 0.0;
    q[1][1] = -(2.0 * p[1][1] / (height - 1));
    q[1][2] = -((2.0 * p[1][2] / (height - 1)) - 1.0);
    q[1][3] = 0.0;
	
    q[2][0] = 0.0;
    q[2][1] = 0.0;
    q[2][2] = (focalmax + focalmin)/(focalmin - focalmax);
    q[2][3] = 2.0 * focalmax * focalmin / (focalmin - focalmax);
	
    q[3][0] = 0.0;
    q[3][1] = 0.0;
    q[3][2] = -1.0;
    q[3][3] = 0.0;
	
    for (i = 0; i < 4; i++) { // Row.
		// First 3 columns of the current row.
        for (j = 0; j < 3; j++) { // Column.
            m_projection[i + j*4] = q[i][0] * trans[0][j] +
									q[i][1] * trans[1][j] +
									q[i][2] * trans[2][j];
        }
		// Fourth column of the current row.
        m_projection[i + 3*4] = q[i][0] * trans[0][3] +
								q[i][1] * trans[1][3] +
								q[i][2] * trans[2][3] +
								q[i][3];
    }	
}

// para's type is also equivalent to (double(*)[4]).
void arglCameraView(const ARdouble para[3][4], ARdouble m_modelview[16], const ARdouble scale)
{
	m_modelview[0 + 0*4] = para[0][0]; // R1C1
	m_modelview[0 + 1*4] = para[0][1]; // R1C2
	m_modelview[0 + 2*4] = para[0][2];
	m_modelview[0 + 3*4] = para[0][3];
	m_modelview[1 + 0*4] = para[1][0]; // R2
	m_modelview[1 + 1*4] = para[1][1];
	m_modelview[1 + 2*4] = para[1][2];
	m_modelview[1 + 3*4] = para[1][3];
	m_modelview[2 + 0*4] = para[2][0]; // R3
	m_modelview[2 + 1*4] = para[2][1];
	m_modelview[2 + 2*4] = para[2][2];
	m_modelview[2 + 3*4] = para[2][3];
	m_modelview[3 + 0*4] = 0.0;
	m_modelview[3 + 1*4] = 0.0;
	m_modelview[3 + 2*4] = 0.0;
	m_modelview[3 + 3*4] = 1.0;
	if (scale != 0.0) {
		m_modelview[12] *= scale;
		m_modelview[13] *= scale;
		m_modelview[14] *= scale;
	}
}

// para's type is also equivalent to (double(*)[4]).
void arglCameraViewRH(const ARdouble para[3][4], ARdouble m_modelview[16], const ARdouble scale)
{
	m_modelview[0 + 0*4] = para[0][0]; // R1C1
	m_modelview[0 + 1*4] = para[0][1]; // R1C2
	m_modelview[0 + 2*4] = para[0][2];
	m_modelview[0 + 3*4] = para[0][3];
	m_modelview[1 + 0*4] = -para[1][0]; // R2
	m_modelview[1 + 1*4] = -para[1][1];
	m_modelview[1 + 2*4] = -para[1][2];
	m_modelview[1 + 3*4] = -para[1][3];
	m_modelview[2 + 0*4] = -para[2][0]; // R3
	m_modelview[2 + 1*4] = -para[2][1];
	m_modelview[2 + 2*4] = -para[2][2];
	m_modelview[2 + 3*4] = -para[2][3];
	m_modelview[3 + 0*4] = 0.0;
	m_modelview[3 + 1*4] = 0.0;
	m_modelview[3 + 2*4] = 0.0;
	m_modelview[3 + 3*4] = 1.0;
	if (scale != 0.0) {
		m_modelview[12] *= scale;
		m_modelview[13] *= scale;
		m_modelview[14] *= scale;
	}
}

#if !ARGL_DISABLE_DISP_IMAGE && !EMSCRIPTEN

ARGL_CONTEXT_SETTINGS_REF arglSetupForCurrentContext(ARParam *cparam, AR_PIXEL_FORMAT pixelFormat)
{
    ARGL_CONTEXT_SETTINGS_REF contextSettings;
    
	// OpenGL 1.5 required.
	if (!arglGLCapabilityCheck(0x0150, NULL)) {
		ARLOGe("Error: OpenGL v1.5 or later is required, but not found. Renderer reported '%s'\n", glGetString(GL_VERSION));
		return (NULL);
	}
#ifdef _WIN32
	if (!glGenBuffers) glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	if (!glDeleteBuffers) glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	if (!glBindBuffer) glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	if (!glBufferData) glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	if (!glActiveTexture) glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	if (!glClientActiveTexture) glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
	if (!glGenBuffers || !glDeleteBuffers || !glBindBuffer || !glBufferData || !glActiveTexture || !glClientActiveTexture) {
		ARLOGe("Error: a required OpenGL function counld not be bound.\n");
		return (NULL);
	}
#endif

    contextSettings = (ARGL_CONTEXT_SETTINGS_REF)calloc(1, sizeof(ARGL_CONTEXT_SETTINGS));
    contextSettings->arParam = *cparam; // Copy it.
    contextSettings->arhandle = NULL;
    contextSettings->zoom = 1.0f;
    // Because of calloc used above, these are redundant.
    //contextSettings->rotate90 = contextSettings->flipH = contextSettings->flipV = FALSE;
    //contextSettings->disableDistortionCompensation = FALSE;
    //contextSettings->textureGeometryHasBeenSetup = FALSE;
    //contextSettings->textureObjectsHaveBeenSetup = FALSE;
    //contextSettings->textureDataReady = FALSE;
    
    // This sets pixIntFormat, pixFormat, pixType, pixSize, and resets textureDataReady.
    arglPixelFormatSet(contextSettings, pixelFormat);
    
    // Set pixel buffer sizes to incoming image size, by default.
    if (!arglPixelBufferSizeSet(contextSettings, cparam->xsize, cparam->ysize)) {
        ARLOGe("ARGL: Error setting pixel buffer size.\n");
        free (contextSettings);
        return (NULL);
    }
    
    return (contextSettings);
}

int arglSetupDebugMode(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARHandle *arHandle)
{
    contextSettings->arhandle = arHandle;
    return (TRUE);
}

void arglCleanup(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return; // Sanity check.
    
    if (contextSettings->textureObjectsHaveBeenSetup) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &(contextSettings->texture));
    }
    
    if (contextSettings->textureGeometryHasBeenSetup) {
        free(contextSettings->t2);
        free(contextSettings->v2);
        glDeleteBuffers(1, &contextSettings->t2bo);
        glDeleteBuffers(1, &contextSettings->v2bo);
    }
    
    free(contextSettings);
}

void arglDispImage(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLdouble left, right, bottom, top;
    GLboolean lightingSave;
    GLboolean depthTestSave;
    
    if (!contextSettings) return;
    
    // Prepare an orthographic projection, set camera position for 2D drawing, and save GL state.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    if (contextSettings->rotate90) glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
    
    if (contextSettings->flipV) {
        bottom = (GLdouble)contextSettings->arParam.ysize;
        top = 0.0;
    } else {
        bottom = 0.0;
        top = (GLdouble)contextSettings->arParam.ysize;
    }
    if (contextSettings->flipH) {
        left = (GLdouble)contextSettings->arParam.xsize;
        right = 0.0;
    } else {
        left = 0.0;
        right = (GLdouble)contextSettings->arParam.xsize;
    }
    glOrtho(left, right, bottom, top, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    lightingSave = glIsEnabled(GL_LIGHTING);			// Save enabled state of lighting.
    if (lightingSave == GL_TRUE) glDisable(GL_LIGHTING);
    depthTestSave = glIsEnabled(GL_DEPTH_TEST);		// Save enabled state of depth test.
    if (depthTestSave == GL_TRUE) glDisable(GL_DEPTH_TEST);
    
    arglDispImageStateful(contextSettings);
    
    if (depthTestSave == GL_TRUE) glEnable(GL_DEPTH_TEST);			// Restore enabled state of depth test.
    if (lightingSave == GL_TRUE) glEnable(GL_LIGHTING);			// Restore enabled state of lighting.
   
    // Restore previous projection & camera position.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
#ifdef ARGL_DEBUG
    // Report any errors we generated.
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ARLOGe("ARGL: GL error 0x%04X\n", (int)err);
    }
#endif // ARGL_DEBUG
    
}

void arglDispImageStateful(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLint texEnvModeSave;
    int        i;
    
    if (!contextSettings) return;
    if (!contextSettings->textureObjectsHaveBeenSetup) return;
    if (!contextSettings->textureGeometryHasBeenSetup) return;
    if (!contextSettings->textureDataReady) return;
    
    glActiveTexture(GL_TEXTURE0);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    
    glBindTexture(GL_TEXTURE_2D, contextSettings->texture);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texEnvModeSave); // Save GL texture environment mode.
    if (texEnvModeSave != GL_REPLACE) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    
    glClientActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
    glTexCoordPointer(2, GL_FLOAT, 0, NULL);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->v2bo);
    glVertexPointer(2, GL_FLOAT, 0, NULL);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDisable(GL_TEXTURE_2D);
    if (texEnvModeSave != GL_REPLACE) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnvModeSave); // Restore GL texture environment mode.
}

int arglDistortionCompensationSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int enable)
{
    if (!contextSettings) return (FALSE);
    contextSettings->disableDistortionCompensation = !enable;
    return (arglSetupTextureGeometry(contextSettings));
}

int arglDistortionCompensationGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *enable)
{
    if (!contextSettings || !enable) return (FALSE);
    *enable = !contextSettings->disableDistortionCompensation;
    return (TRUE);
}

int arglSetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    contextSettings->zoom = zoom;
    
    // Changing the zoom invalidates the geometry, so set it up.
    return (arglSetupTextureGeometry(contextSettings));
}

int arglGetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float *zoom)
{
    if (!contextSettings) return (FALSE);
    *zoom = contextSettings->zoom;
    return (TRUE);
}

int arglPixelFormatSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
	if (!contextSettings) return (FALSE);
	switch (format) {
		case AR_PIXEL_FORMAT_RGBA:
			contextSettings->pixIntFormat = GL_RGBA;
			contextSettings->pixFormat = GL_RGBA;
			contextSettings->pixType = GL_UNSIGNED_BYTE;
			contextSettings->pixSize = 4;
			break;
		case AR_PIXEL_FORMAT_ABGR:	// SGI.
			if (arglGLCapabilityCheck(0, (unsigned char *)"GL_EXT_abgr")) {
				contextSettings->pixIntFormat = GL_RGBA;
				contextSettings->pixFormat = GL_ABGR_EXT;
				contextSettings->pixType = GL_UNSIGNED_BYTE;
				contextSettings->pixSize = 4;
			} else {
				return (FALSE);
			}
			break;
		case AR_PIXEL_FORMAT_BGRA:	// Windows.
			if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_bgra")) {
				contextSettings->pixIntFormat = GL_RGBA;
				contextSettings->pixFormat = GL_BGRA;
				contextSettings->pixType = GL_UNSIGNED_BYTE;
				contextSettings->pixSize = 4;
			} else {
				return (FALSE);
			}
			break;
		case AR_PIXEL_FORMAT_ARGB:	// Mac.
			if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_bgra")
				&& (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_packed_pixels") || arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_APPLE_packed_pixels"))) {
				contextSettings->pixIntFormat = GL_RGBA;
				contextSettings->pixFormat = GL_BGRA;
#ifdef AR_BIG_ENDIAN
				contextSettings->pixType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
				contextSettings->pixType = GL_UNSIGNED_INT_8_8_8_8;
#endif
				contextSettings->pixSize = 4;
			} else {
				return (FALSE);
			}
			break;
		case AR_PIXEL_FORMAT_RGB:
			contextSettings->pixIntFormat = GL_RGB;
			contextSettings->pixFormat = GL_RGB;
			contextSettings->pixType = GL_UNSIGNED_BYTE;
			contextSettings->pixSize = 3;
			break;
		case AR_PIXEL_FORMAT_BGR:
			if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_bgra")) {
				contextSettings->pixIntFormat = GL_RGB;
				contextSettings->pixFormat = GL_BGR;
				contextSettings->pixType = GL_UNSIGNED_BYTE;
				contextSettings->pixSize = 3;
			} else {
				return (FALSE);
			}
			break;
		case AR_PIXEL_FORMAT_MONO:
			contextSettings->pixIntFormat = GL_LUMINANCE;
			contextSettings->pixFormat = GL_LUMINANCE;
			contextSettings->pixType = GL_UNSIGNED_BYTE;
			contextSettings->pixSize = 1;
			break;
		case AR_PIXEL_FORMAT_2vuy:
			if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_ycbcr_422")) {
				contextSettings->pixIntFormat = GL_RGB;
				contextSettings->pixFormat = GL_YCBCR_422_APPLE;
#ifdef AR_BIG_ENDIAN
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#else
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
#endif
			} else if (arglGLCapabilityCheck(0, (unsigned char *)"GL_MESA_ycbcr_texture")) {
				contextSettings->pixIntFormat = GL_YCBCR_MESA;
				contextSettings->pixFormat = GL_YCBCR_MESA;
#ifdef AR_BIG_ENDIAN
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_REV_MESA;
#else
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_MESA;
#endif
			} else {
				return (FALSE);
			}
			contextSettings->pixSize = 2;
			break;
		case AR_PIXEL_FORMAT_yuvs:
			if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_ycbcr_422")) {
				contextSettings->pixIntFormat = GL_RGB;
				contextSettings->pixFormat = GL_YCBCR_422_APPLE;
#ifdef AR_BIG_ENDIAN
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
#else
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#endif
			} else if (arglGLCapabilityCheck(0, (unsigned char *)"GL_MESA_ycbcr_texture")) {
				contextSettings->pixIntFormat = GL_YCBCR_MESA;
				contextSettings->pixFormat = GL_YCBCR_MESA;
#ifdef AR_BIG_ENDIAN
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_MESA;
#else
				contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_REV_MESA;
#endif
			} else {
				return (FALSE);
			}
			contextSettings->pixSize = 2;
			break;
        case AR_PIXEL_FORMAT_RGB_565:
            if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_packed_pixels") || arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_APPLE_packed_pixels")) {
                contextSettings->pixIntFormat = GL_RGB;
                contextSettings->pixFormat = GL_RGB;
                contextSettings->pixType = GL_UNSIGNED_SHORT_5_6_5;
                contextSettings->pixSize = 2;
            } else {
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_packed_pixels") || arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_APPLE_packed_pixels")) {
                contextSettings->pixIntFormat = GL_RGBA;
                contextSettings->pixFormat = GL_RGBA;
                contextSettings->pixType = GL_UNSIGNED_SHORT_5_5_5_1;
                contextSettings->pixSize = 2;
            } else {
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            if (arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_EXT_packed_pixels") || arglGLCapabilityCheck(0x0120, (unsigned char *)"GL_APPLE_packed_pixels")) {
                contextSettings->pixIntFormat = GL_RGBA;
                contextSettings->pixFormat = GL_RGBA;
                contextSettings->pixType = GL_UNSIGNED_SHORT_4_4_4_4;
                contextSettings->pixSize = 2;
            } else {
                return (FALSE);
            }
            break;
        // Do mono-only rendering as a better alternative to doing nothing.
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_NV21:
            contextSettings->pixIntFormat = GL_LUMINANCE;
            contextSettings->pixFormat = GL_LUMINANCE;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 1;
            break;
		default:
			return (FALSE);
			break;
	}
    contextSettings->format = format;
    ARLOGd("ARGL: set pixel format %s.\n", arUtilGetPixelFormatName(format));
    contextSettings->textureDataReady = FALSE;
    
    if (!arglSetupTextureObjects(contextSettings)) return (FALSE);
    
    return (TRUE);
}

int arglPixelFormatGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    
    if (format) *format = contextSettings->format;
    if (size) *size = contextSettings->pixSize;
    
    return (TRUE);
}

void arglSetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings, int rotate90)
{
    if (!contextSettings) return;
    contextSettings->rotate90 = rotate90;
}

int arglGetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->rotate90);
}

void arglSetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipH)
{
    if (!contextSettings) return;
    contextSettings->flipH = flipH;
}

int arglGetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipH);
}

void arglSetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings, int flipV)
{
    if (!contextSettings) return;
    contextSettings->flipV = flipV;
}

int arglGetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipV);
}

char arglPixelBufferSizeSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);
    
    // Check texturing capabilities (sets textureSizeX, textureSizeY, textureSizeMax).
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(contextSettings->textureSizeMax));
    if (bufWidth > contextSettings->textureSizeMax || bufHeight > contextSettings->textureSizeMax) {
        ARLOGe("Error: ARGL: Your OpenGL implementation and/or hardware's texturing capabilities are insufficient.\n");
        return (FALSE);
    }
    
    if (arglGLCapabilityCheck(0x200u, (const unsigned char *)"GL_ARB_texture_non_power_of_two")) {
        contextSettings->textureSizeX = bufWidth;
        contextSettings->textureSizeY = bufHeight;
        contextSettings->bufSizeIsTextureSize = TRUE;
    } else {
        // Work out how big power-of-two textures needs to be.
        contextSettings->textureSizeX = contextSettings->textureSizeY = 1;
        while (contextSettings->textureSizeX < bufWidth) contextSettings->textureSizeX <<= 1;
        while (contextSettings->textureSizeY < bufHeight) contextSettings->textureSizeY <<= 1;
        contextSettings->bufSizeIsTextureSize = FALSE;
        contextSettings->bufSizeX = bufWidth;
        contextSettings->bufSizeY = bufHeight;
    }
    
    // Changing the size of the data we'll be receiving invalidates the geometry, so set it up.
    return (arglSetupTextureGeometry(contextSettings));
}

char arglPixelBufferSizeGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);
    if (!contextSettings->textureGeometryHasBeenSetup) return (FALSE);
    
    if (contextSettings->bufSizeIsTextureSize) {
        if (bufWidth) *bufWidth = contextSettings->textureSizeX;
        if (bufHeight) *bufHeight = contextSettings->textureSizeY;
    } else {
        if (bufWidth) *bufWidth = contextSettings->bufSizeX;
        if (bufHeight) *bufHeight = contextSettings->bufSizeY;
    }
    return (TRUE);
}

int arglPixelBufferDataUpload(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr)
{
    int arDebugMode = AR_DEBUG_DISABLE, arImageProcMode;
    
    if (!contextSettings) return (FALSE);
    if (!contextSettings->textureObjectsHaveBeenSetup || !contextSettings->textureGeometryHasBeenSetup || !contextSettings->pixSize) return (FALSE);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, contextSettings->texture);
    
    glPixelTransferi(GL_UNPACK_ALIGNMENT, (((contextSettings->bufSizeX * contextSettings->pixSize) & 0x3) == 0 ? 4 : 1));
    
    if (contextSettings->arhandle) {
        arGetDebugMode(contextSettings->arhandle, &arDebugMode);
    }
    if (arDebugMode == AR_DEBUG_DISABLE) {
        if (contextSettings->bufSizeIsTextureSize) {
            glTexImage2D(GL_TEXTURE_2D, 0, contextSettings->pixIntFormat, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, contextSettings->pixFormat, contextSettings->pixType, bufDataPtr);
        } else {
            // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
            // Then send the NPOT-data as a subimage.
            glTexImage2D(GL_TEXTURE_2D, 0, contextSettings->pixIntFormat, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, contextSettings->pixFormat, contextSettings->pixType, NULL);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, contextSettings->bufSizeX, contextSettings->bufSizeY, contextSettings->pixFormat, contextSettings->pixType, bufDataPtr);
        }
    } else {
        if (contextSettings->arhandle->labelInfo.bwImage) {
            arGetImageProcMode(contextSettings->arhandle, &arImageProcMode);
            if (arImageProcMode == AR_IMAGE_PROC_FIELD_IMAGE) {
                if (contextSettings->bufSizeIsTextureSize) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, contextSettings->textureSizeX >> 1, contextSettings->textureSizeY >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                } else {
                    // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
                    // Then send the NPOT-data as a subimage.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, contextSettings->textureSizeX >> 1, contextSettings->textureSizeY >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, contextSettings->bufSizeX >> 1, contextSettings->bufSizeY >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                }
            } else {
                if (contextSettings->bufSizeIsTextureSize) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                } else {
                    // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
                    // Then send the NPOT-data as a subimage.
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, contextSettings->bufSizeX, contextSettings->bufSizeY, GL_LUMINANCE, GL_UNSIGNED_BYTE, contextSettings->arhandle->labelInfo.bwImage);
                }
            }
        }
    }
    
    contextSettings->textureDataReady = TRUE;
    
    return (TRUE);
}

#endif // !ARGL_DISABLE_DISP_IMAGE

GLboolean arglGluCheckExtension(const GLubyte* extName, const GLubyte *extString)
{
	const GLubyte *start;
	GLubyte *where, *terminator;
	
	// Extension names should not have spaces.
	where = (GLubyte *)strchr((const char *)extName, ' ');
	if (where || *extName == '\0')
		return GL_FALSE;
	// It takes a bit of care to be fool-proof about parsing the
	//	OpenGL extensions string. Don't be fooled by sub-strings, etc.
	start = extString;
	for (;;) {
		where = (GLubyte *) strstr((const char *)start, (const char *)extName);
		if (!where)
			break;
		terminator = where + strlen((const char *)extName);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return GL_TRUE;
		start = terminator;
	}
	return GL_FALSE;
}

int arglGLCapabilityCheck(const unsigned short minVersion, const unsigned char *extension)
{
    const GLubyte *strVersion;
    const GLubyte *strExtensions;
    short j, shiftVal;
    unsigned short version = 0; // binary-coded decimal gl version (ie. 1.4 is 0x0140).
    
    if (minVersion > 0) {
        strVersion = glGetString(GL_VERSION);
#ifdef EDEN_OPENGLES
        j = 13; // Of the form "OpenGL ES-XX 1.1", where XX=CM for common, CL for common lite.
#else
        j = 0;
#endif
        shiftVal = 8;
        // Construct BCD version.
        while (((strVersion[j] <= '9') && (strVersion[j] >= '0')) || (strVersion[j] == '.')) { // Get only basic version info (until first non-digit or non-.)
            if ((strVersion[j] <= '9') && (strVersion[j] >= '0')) {
                version += (strVersion[j] - '0') << shiftVal;
                shiftVal -= 4;
            }
            j++;
        }
        if (version >= minVersion) return (TRUE);
    }
    
    if (extension) {
        strExtensions = glGetString(GL_EXTENSIONS);
        if (arglGluCheckExtension(extension, strExtensions)) return (TRUE);
    }
    
    return (FALSE);
}
