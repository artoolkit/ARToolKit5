/*
 *  gsub_es2.c
 *  ARToolKit5
 *
 *	Graphics Subroutines (OpenGL ES 2.x) for ARToolKit.
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

// ============================================================================
//    Private includes.
// ============================================================================
#include <AR/gsub_es2.h>

#include <stdio.h>        // fprintf(), stderr
#include <string.h>        // strchr(), strstr(), strlen()
#include <AR/gsub_mtx.h>

// ============================================================================
//    Private types and defines.
// ============================================================================

#define ARGL_DEBUG 1
#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 0 // Edit as required.
#endif

#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

#if !ARGL_DISABLE_DISP_IMAGE

#if !defined(GL_IMG_texture_format_BGRA8888) && !defined(GL_APPLE_texture_format_BGRA8888)
#  define GL_BGRA 0x80E1
#elif !defined(GL_BGRA)
#  define GL_BGRA 0x80E1
#endif
#ifndef GL_APPLE_rgb_422
#  define GL_RGB_422_APPLE 0x8A1F
#  define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#  define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#endif

// Indices of GL ES program uniforms.
enum {
	ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX,
    ARGL_UNIFORM_TEXTURE0,
    ARGL_UNIFORM_TEXTURE1,
	ARGL_UNIFORM_COUNT
};

// Indices of of GL ES program attributes.
enum {
	ARGL_ATTRIBUTE_VERTEX,
	ARGL_ATTRIBUTE_TEXCOORD,
	ARGL_ATTRIBUTE_COUNT
};

struct _ARGL_CONTEXT_SETTINGS {
    ARParam arParam;
    GLuint  texture0; // For interleaved images all planes. For bi-planar images, plane 0.
    GLuint  texture1; // For interleaved images, not used.  For bi-planar images, plane 1.
    GLuint  program;
    GLint   uniforms[ARGL_UNIFORM_COUNT];
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
    int     textureDataReady;
    int     useTextureYCbCrBiPlanar;
    int     requestUpdateProjection;
};
typedef struct _ARGL_CONTEXT_SETTINGS ARGL_CONTEXT_SETTINGS;
#endif // !ARGL_DISABLE_DISP_IMAGE

// ============================================================================
//    Public globals.
// ============================================================================


// ============================================================================
//    Private globals.
// ============================================================================


#pragma mark -
// ============================================================================
//    Private functions.
// ============================================================================

#if !ARGL_DISABLE_DISP_IMAGE

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
        for (j = 1; j <= 20; j++) {    // Do 20 rows.
            y_prev = y;
            ty_prev = ty;
            y = imageSizeY * (float)j / 20.0f;
            ty = y / (float)contextSettings->textureSizeY;
            
            
            for (i = 0; i <= 20; i++) {
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

// Create the shader program required for drawing the background texture.
// Sets program, uniforms.
static char arglSetupProgram(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    GLuint vertShader = 0, fragShader = 0;
    const char vertShaderString[] =
        "attribute vec4 position;\n"
        "attribute vec2 texCoord;\n"
        "uniform mat4 modelViewProjectionMatrix;\n"
        "varying vec2 texCoordVarying;\n"
        "void main()\n"
        "{\n"
            "gl_Position = modelViewProjectionMatrix * position;\n"
            "texCoordVarying = texCoord;\n"
        "}\n";
    const char *fragShaderString;
    const char fragShaderStringRGB[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
            "gl_FragColor = texture2D(texture0, texCoordVarying);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601FullRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"   // Column 0
                                                                    "0.0,   -0.344, 1.772,\n" // Column 1
                                                                    "1.402, -0.714, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).r;\n"
            "colourYCbCr.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601FullRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    const char fragShaderStringYCrCbITURec601FullRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCrCb;\n"
            "const mat3 transformYCrCbITURec601FullRangeToRGB = mat3(1.0,    1.0,   1.0,\n"    // Column 0
                                                                    "1.402, -0.714, 0.0,\n"    // Column 1
                                                                    "0.0,   -0.344, 1.772);\n" // Column 2
            "colourYCrCb.x  = texture2D(texture0, texCoordVarying).r;\n"
            "colourYCrCb.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCrCbITURec601FullRangeToRGB * colourYCrCb, 1.0);\n"
        "}\n";
    const char fragShaderStringYCbCrITURec601VideoRangeBiPlanar[] =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).r - 0.0625;\n"
            "colourYCbCr.yz = texture2D(texture1, texCoordVarying).ra - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601FullRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";
    /*const char fragShaderStringYCbCrITURec601VideoRangeInterleaved[] =
        "#extension GL_APPLE_rgb_422​ : require\n"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying highp vec2 texCoordVarying;\n"
        "uniform sampler2D texture0;\n"
        "void main()\n"
        "{\n"
            "vec3 colourYCbCr;\n"
            "const mat3 transformYCbCrITURec601VideoRangeToRGB = mat3(1.164,  1.164, 1.164,\n" // Column 0
                                                                     "0.0,   -0.391, 2.017,\n" // Column 1
                                                                     "1.596, -0.813, 0.0);\n"  // Column 2
            "colourYCbCr.x  = texture2D(texture0, texCoordVarying).r - 0.0625;\n"
            "colourYCbCr.yz = texture2D(texture0, texCoordVarying).gb - 0.5;\n"
            "gl_FragColor = vec4(transformYCbCrITURec601VideoRangeToRGB * colourYCbCr, 1.0);\n"
        "}\n";*/
    
    if (contextSettings->program) arglGLDestroyShaders(0, 0, contextSettings->program);
    contextSettings->program = glCreateProgram();
    if (!contextSettings->program) {
        ARLOGe("ARGL: Error creating shader program.\n");
		goto bail;
    }
    
    if (!arglGLCompileShaderFromString(&vertShader, GL_VERTEX_SHADER, vertShaderString)) {
        ARLOGe("ARGL: Error compiling vertex shader.\n");
		goto bail1;
    }
    if (contextSettings->format == AR_PIXEL_FORMAT_420f) fragShaderString = fragShaderStringYCbCrITURec601FullRangeBiPlanar;
    else if (contextSettings->format == AR_PIXEL_FORMAT_420v) fragShaderString = fragShaderStringYCbCrITURec601VideoRangeBiPlanar;
    else if (contextSettings->format == AR_PIXEL_FORMAT_NV21) fragShaderString = fragShaderStringYCrCbITURec601FullRangeBiPlanar;
    else fragShaderString = fragShaderStringRGB;
    if (!arglGLCompileShaderFromString(&fragShader, GL_FRAGMENT_SHADER, fragShaderString)) {
        ARLOGe("ARGL: Error compiling fragment shader.\n");
		goto bail1;
    }
	glAttachShader(contextSettings->program, vertShader);
	glAttachShader(contextSettings->program, fragShader);
    
	glBindAttribLocation(contextSettings->program, ARGL_ATTRIBUTE_VERTEX, "position");
	glBindAttribLocation(contextSettings->program, ARGL_ATTRIBUTE_TEXCOORD, "texCoord");
	if (!arglGLLinkProgram(contextSettings->program)) {
        ARLOGe("ARGL: Error linking shader program.\n");
		goto bail1;
	}
	arglGLDestroyShaders(vertShader, fragShader, 0); // After linking, shader objects can be deleted.
    
    // Retrieve linked uniform locations.
	contextSettings->uniforms[ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX] = glGetUniformLocation(contextSettings->program, "modelViewProjectionMatrix");
    contextSettings->uniforms[ARGL_UNIFORM_TEXTURE0] = glGetUniformLocation(contextSettings->program, "texture0");
    if (contextSettings->useTextureYCbCrBiPlanar) {
        contextSettings->uniforms[ARGL_UNIFORM_TEXTURE1] = glGetUniformLocation(contextSettings->program, "texture1");
    }
    
    return (TRUE);
    
bail1:
    arglGLDestroyShaders(vertShader, fragShader, contextSettings->program);
bail:
    return (FALSE);
}

// Set up the texture objects.
static char arglSetupTextureObjects(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    // Delete previous textures, unless this is our first time here.
    if (contextSettings->textureObjectsHaveBeenSetup) {
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(contextSettings->texture0));
        if (contextSettings->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture1));
        }
        contextSettings->textureObjectsHaveBeenSetup = FALSE;
    }

    glGenTextures(1, &(contextSettings->texture0));
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(contextSettings->texture0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (contextSettings->useTextureYCbCrBiPlanar) {
        glGenTextures(1, &(contextSettings->texture1));
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(contextSettings->texture1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    contextSettings->textureObjectsHaveBeenSetup = TRUE;
    return (TRUE);
}

#pragma mark -
// ============================================================================
//    Public functions.
// ============================================================================

#endif // !ARGL_DISABLE_DISP_IMAGE

//
// Convert a camera parameter structure into an OpenGL projection matrix.
//
void arglCameraFrustumf(const ARParam *cparam, const float focalmin, const float focalmax, GLfloat m_projection[16])
{
    float   icpara[3][4];
    float   trans[3][4];
    float   p[3][3], q[4][4];
    float   widthm1, heightm1;
    int     i, j;
    
    widthm1  = (float)(cparam->xsize - 1);
    heightm1 = (float)(cparam->ysize - 1);

    if (arParamDecompMatf(cparam->mat, icpara, trans) < 0) {
        printf("arglCameraFrustum(): arParamDecompMat() indicated parameter error.\n"); // Windows bug: when running multi-threaded, can't write to stderr!
        return;
    }
    for (i = 0; i < 4; i++) {
        icpara[1][i] = heightm1*(icpara[2][i]) - icpara[1][i];
    }
        
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0f * p[0][0] / widthm1);
    q[0][1] = (2.0f * p[0][1] / widthm1);
    q[0][2] = ((2.0f * p[0][2] / widthm1)  - 1.0f);
    q[0][3] = 0.0f;
    
    q[1][0] = 0.0f;
    q[1][1] = (2.0f * p[1][1] / heightm1);
    q[1][2] = ((2.0f * p[1][2] / heightm1) - 1.0f);
    q[1][3] = 0.0f;
    
    q[2][0] = 0.0f;
    q[2][1] = 0.0f;
    q[2][2] = (focalmax + focalmin)/(focalmax - focalmin);
    q[2][3] = -2.0f * focalmax * focalmin / (focalmax - focalmin);
    
    q[3][0] = 0.0f;
    q[3][1] = 0.0f;
    q[3][2] = 1.0f;
    q[3][3] = 0.0f;
    
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

void arglCameraFrustumRHf(const ARParam *cparam, const float focalmin, const float focalmax, GLfloat m_projection[16])
{
    float   icpara[3][4];
    float   trans[3][4];
    float   p[3][3], q[4][4];
    float   widthm1, heightm1;
    int     i, j;
    
    widthm1  = (float)(cparam->xsize - 1);
    heightm1 = (float)(cparam->ysize - 1);
    
    if (arParamDecompMatf(cparam->mat, icpara, trans) < 0) {
        printf("arglCameraFrustum(): arParamDecompMat() indicated parameter error.\n"); // Windows bug: when running multi-threaded, can't write to stderr!
        return;
    }
    for (i = 0; i < 4; i++) {
        icpara[1][i] = heightm1*(icpara[2][i]) - icpara[1][i];
    }
    
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            p[i][j] = icpara[i][j] / icpara[2][2];
        }
    }
    q[0][0] = (2.0f * p[0][0] / widthm1);
    q[0][1] = (2.0f * p[0][1] / widthm1);
    q[0][2] = -((2.0f * p[0][2] / widthm1)  - 1.0f);
    q[0][3] = 0.0f;
    
    q[1][0] = 0.0f;
    q[1][1] = -(2.0f * p[1][1] / heightm1);
    q[1][2] = -((2.0f * p[1][2] / heightm1) - 1.0f);
    q[1][3] = 0.0f;
    
    q[2][0] = 0.0f;
    q[2][1] = 0.0f;
    q[2][2] = (focalmax + focalmin)/(focalmin - focalmax);
    q[2][3] = 2.0f * focalmax * focalmin / (focalmin - focalmax);
    
    q[3][0] = 0.0f;
    q[3][1] = 0.0f;
    q[3][2] = -1.0f;
    q[3][3] = 0.0f;
    
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

// para's type is also equivalent to (float(*)[4]).
void arglCameraViewf(float para[3][4], GLfloat m_modelview[16], const float scale)
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
    m_modelview[3 + 0*4] = 0.0f;
    m_modelview[3 + 1*4] = 0.0f;
    m_modelview[3 + 2*4] = 0.0f;
    m_modelview[3 + 3*4] = 1.0f;
    if (scale != 0.0f) {
        m_modelview[12] *= scale;
        m_modelview[13] *= scale;
        m_modelview[14] *= scale;
    }
}

// para's type is also equivalent to (float(*)[4]).
void arglCameraViewRHf(float para[3][4], GLfloat m_modelview[16], const float scale)
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
    m_modelview[3 + 0*4] = 0.0f;
    m_modelview[3 + 1*4] = 0.0f;
    m_modelview[3 + 2*4] = 0.0f;
    m_modelview[3 + 3*4] = 1.0f;
    if (scale != 0.0f) {
        m_modelview[12] *= scale;
        m_modelview[13] *= scale;
        m_modelview[14] *= scale;
    }
}

#if !ARGL_DISABLE_DISP_IMAGE

ARGL_CONTEXT_SETTINGS_REF arglSetupForCurrentContext(ARParam *cparam, AR_PIXEL_FORMAT pixelFormat)
{
    ARGL_CONTEXT_SETTINGS_REF contextSettings;
    
    contextSettings = (ARGL_CONTEXT_SETTINGS_REF)calloc(1, sizeof(ARGL_CONTEXT_SETTINGS));
    contextSettings->arParam = *cparam; // Copy it.
    contextSettings->zoom = 1.0f;
    contextSettings->requestUpdateProjection = TRUE;
    // Because of calloc used above, these are redundant.
    //contextSettings->rotate90 = contextSettings->flipH = contextSettings->flipV = FALSE;
    //contextSettings->program = 0;
    //contextSettings->disableDistortionCompensation = FALSE;
    //contextSettings->textureGeometryHasBeenSetup = FALSE;
    //contextSettings->textureObjectsHaveBeenSetup = FALSE;
    //contextSettings->textureDataReady = FALSE;
    
    // This sets pixIntFormat, pixFormat, pixType, pixSize, and resets textureDataReady.
    // It also calls arglSetupProgram to setup the shader program.
    arglPixelFormatSet(contextSettings, pixelFormat);
    
    // Set pixel buffer sizes to incoming image size, by default.
    if (!arglPixelBufferSizeSet(contextSettings, cparam->xsize, cparam->ysize)) {
        ARLOGe("ARGL: Error setting pixel buffer size.\n");
        free (contextSettings);
        return (NULL);
    }
    
    return (contextSettings);
}

void arglCleanup(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return; // Sanity check.
    
    if (contextSettings->program) arglGLDestroyShaders(0, 0, contextSettings->program);
    
    if (contextSettings->textureObjectsHaveBeenSetup) {
        if (contextSettings->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture1));
        }
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(contextSettings->texture0));
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
    float projection[16];
    float const ir90[16] = {0.0f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f};
    float left, right, bottom, top;
    int i;
    
    if (!contextSettings) return;
    if (!contextSettings->textureObjectsHaveBeenSetup) return;
    if (!contextSettings->textureGeometryHasBeenSetup) return;
    if (!contextSettings->textureDataReady) return;

    glUseProgram(contextSettings->program);
    
    // Prepare an orthographic projection, set camera position for 2D drawing.
    glStateCacheDisableDepthTest();
    if (contextSettings->requestUpdateProjection) {
        if (contextSettings->rotate90) mtxLoadMatrixf(projection, ir90);
        else mtxLoadIdentityf(projection);
        if (contextSettings->flipV) {
            bottom = (float)contextSettings->arParam.ysize;
            top = 0.0f;
        } else {
            bottom = 0.0f;
            top = (float)contextSettings->arParam.ysize;
        }
        if (contextSettings->flipH) {
            left = (float)contextSettings->arParam.xsize;
            right = 0.0f;
        } else {
            left = 0.0f;
            right = (float)contextSettings->arParam.xsize;
        }
        mtxOrthof(projection, left, right, bottom, top, -1.0f, 1.0f);
        glUniformMatrix4fv(contextSettings->uniforms[ARGL_UNIFORM_MODELVIEW_PROJECTION_MATRIX], 1, GL_FALSE, projection);
        contextSettings->requestUpdateProjection = FALSE;
    }
    
#ifdef ARGL_DEBUG
	if (!arglGLValidateProgram(contextSettings->program)) {
		ARLOGe("arglDispImage(): Error: shader program %d validation failed.\n", contextSettings->program);
		return;
	}
#endif

    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(contextSettings->texture0);
    glUniform1i(contextSettings->uniforms[ARGL_UNIFORM_TEXTURE0], 0);
    if (contextSettings->useTextureYCbCrBiPlanar) {
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(contextSettings->texture1);
        glUniform1i(contextSettings->uniforms[ARGL_UNIFORM_TEXTURE1], 1);
    }
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->v2bo);
	glVertexAttribPointer(ARGL_ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(ARGL_ATTRIBUTE_VERTEX);
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
	glVertexAttribPointer(ARGL_ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ARGL_ATTRIBUTE_TEXCOORD);
    
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

#ifdef ARGL_DEBUG
    // Report any errors we generated.
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ARLOGe("ARGL: GL error 0x%04X\n", (int)err);
    }
#endif // ARGL_DEBUG
    
}

char arglDistortionCompensationSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, char enable)
{
    if (!contextSettings) return (FALSE);
    contextSettings->disableDistortionCompensation = !enable;
    return (arglSetupTextureGeometry(contextSettings));
}

char arglDistortionCompensationGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, char *enable)
{
    if (!contextSettings || !enable) return (FALSE);
    *enable = !contextSettings->disableDistortionCompensation;
    return (TRUE);
}

char arglSetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float zoom)
{
    if (!contextSettings) return (FALSE);
    contextSettings->zoom = zoom;
 
    // Changing the zoom invalidates the geometry, so set it up.
    return (arglSetupTextureGeometry(contextSettings));
}

char arglGetPixelZoom(ARGL_CONTEXT_SETTINGS_REF contextSettings, float *zoom)
{
    if (!contextSettings) return (FALSE);
    *zoom = contextSettings->zoom;
    return (TRUE);
}

char arglPixelFormatSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT format)
{
    if (!contextSettings) return (FALSE);
    switch (format) {
        case AR_PIXEL_FORMAT_420f:
        case AR_PIXEL_FORMAT_420v:
        case AR_PIXEL_FORMAT_NV21:
            contextSettings->pixIntFormat = GL_LUMINANCE;
            contextSettings->pixFormat = GL_LUMINANCE;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 1;
            contextSettings->useTextureYCbCrBiPlanar = TRUE;
            glStateCachePixelStoreUnpackAlignment(1);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_420f or AR_PIXEL_FORMAT_420v or AR_PIXEL_FORMAT_NV21.\n");
            break;
        case AR_PIXEL_FORMAT_RGBA:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 4;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(4);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_RGBA.\n");
            break;
        case AR_PIXEL_FORMAT_RGB:
            contextSettings->pixIntFormat = GL_RGB;
            contextSettings->pixFormat = GL_RGB;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 3;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(1);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_RGB.\n");
            break;
        case AR_PIXEL_FORMAT_BGRA:
            if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_texture_format_BGRA8888") || arglGLCapabilityCheck(0, (unsigned char *)"GL_IMG_texture_format_BGRA8888")) {
                contextSettings->pixIntFormat = GL_RGBA;
                contextSettings->pixFormat = GL_BGRA;
                contextSettings->pixType = GL_UNSIGNED_BYTE;
                contextSettings->pixSize = 4;
                contextSettings->useTextureYCbCrBiPlanar = FALSE;
                glStateCachePixelStoreUnpackAlignment(4);
                ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_BGRA.\n");
            } else {
                ARLOGe("Error: ARGL: set pixel format called with AR_PIXEL_FORMAT_BGRA, but GL_APPLE_texture_format_BGRA8888 or GL_IMG_texture_format_BGRA8888 are not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_MONO:
            contextSettings->pixIntFormat = GL_LUMINANCE;
            contextSettings->pixFormat = GL_LUMINANCE;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 1;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(1);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_MONO.\n");
            break;
        case AR_PIXEL_FORMAT_2vuy:
            if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_rgb_422")) {
                contextSettings->pixIntFormat = GL_RGB;
                contextSettings->pixFormat = GL_RGB_422_APPLE;
                contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
                contextSettings->pixSize = 1;
                contextSettings->useTextureYCbCrBiPlanar = FALSE;
                glStateCachePixelStoreUnpackAlignment(2);
                ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_2vuy.\n");
            } else {
                ARLOGe("Error: ARGL: set pixel format called with AR_PIXEL_FORMAT_2vuy, but GL_APPLE_rgb_422 is not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_yuvs:
            if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_rgb_422")) {
                contextSettings->pixIntFormat = GL_RGB;
                contextSettings->pixFormat = GL_RGB_422_APPLE;
                contextSettings->pixType = GL_UNSIGNED_SHORT_8_8_APPLE;
                contextSettings->pixSize = 1;
                contextSettings->useTextureYCbCrBiPlanar = FALSE;
                glStateCachePixelStoreUnpackAlignment(2);
                ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_yuvs.\n");
            } else {
                ARLOGe("Error: ARGL: set pixel format called with AR_PIXEL_FORMAT_yuvs, but GL_APPLE_rgb_422 is not available.\n");
                return (FALSE);
            }
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            contextSettings->pixIntFormat = GL_RGB;
            contextSettings->pixFormat = GL_RGB;
            contextSettings->pixType = GL_UNSIGNED_SHORT_5_6_5;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_RGB_565.\n");
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_SHORT_5_5_5_1;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_RGBA_5551.\n");
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_SHORT_4_4_4_4;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            ARLOGd("ARGL: set pixel format AR_PIXEL_FORMAT_RGBA_4444.\n");
            break;
        default:
            ARLOGe("Error: ARGL: set pixel format called with unsupported pixel format.\n");
            return (FALSE);
            break;
    }
    contextSettings->format = format;
    contextSettings->textureDataReady = FALSE;
    
    if (!arglSetupProgram(contextSettings)) return (FALSE);
    if (!arglSetupTextureObjects(contextSettings)) return (FALSE);
    
    return (TRUE);
}

char arglPixelFormatGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, AR_PIXEL_FORMAT *format, int *size)
{
    if (!contextSettings) return (FALSE);
    
    if (format) *format = contextSettings->format;
    if (size) *size = contextSettings->pixSize;
    
    return (TRUE);
}

void arglSetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings, char rotate90)
{
    if (!contextSettings) return;
    contextSettings->rotate90 = rotate90;
    contextSettings->requestUpdateProjection = TRUE;
}

char arglGetRotate90(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->rotate90);
}

void arglSetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings, char flipH)
{
    if (!contextSettings) return;
    contextSettings->flipH = flipH;
    contextSettings->requestUpdateProjection = TRUE;
}

char arglGetFlipH(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipH);
}

void arglSetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings, char flipV)
{
    if (!contextSettings) return;
    contextSettings->flipV = flipV;
    contextSettings->requestUpdateProjection = TRUE;
}

char arglGetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return (-1);
    return (contextSettings->flipV);
}

// Sets textureSizeMax, textureSizeX, textureSizeY.
char arglPixelBufferSizeSet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int bufWidth, int bufHeight)
{
    if (!contextSettings) return (FALSE);
    
    // Check texturing capabilities.
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &(contextSettings->textureSizeMax));
    if (bufWidth > contextSettings->textureSizeMax || bufHeight > contextSettings->textureSizeMax) {
        ARLOGe("Error: ARGL: Your OpenGL implementation and/or hardware's texturing capabilities are insufficient.\n");
        return (FALSE); // Too big to handle.
    }
    
    contextSettings->textureSizeX = bufWidth;
    contextSettings->textureSizeY = bufHeight;

    // Changing the size of the data we'll be receiving invalidates the geometry, so set it up.
    return (arglSetupTextureGeometry(contextSettings));
}

char arglPixelBufferSizeGet(ARGL_CONTEXT_SETTINGS_REF contextSettings, int *bufWidth, int *bufHeight)
{
    if (!contextSettings) return (FALSE);
    if (!contextSettings->textureGeometryHasBeenSetup) return (FALSE);
    
    if (bufWidth) *bufWidth = contextSettings->textureSizeX;
    if (bufHeight) *bufHeight = contextSettings->textureSizeY;

    return (TRUE);
}

char arglPixelBufferDataUploadBiPlanar(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    if (!contextSettings) return (FALSE);
    if (!contextSettings->textureObjectsHaveBeenSetup || !contextSettings->textureGeometryHasBeenSetup || !contextSettings->pixSize) return (FALSE);
    
    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(contextSettings->texture0);
    glTexImage2D(GL_TEXTURE_2D, 0, contextSettings->pixIntFormat, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, contextSettings->pixFormat, contextSettings->pixType, bufDataPtr0);
    if (bufDataPtr1 && contextSettings->useTextureYCbCrBiPlanar) {
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheBindTexture2D(contextSettings->texture1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, contextSettings->textureSizeX / 2, contextSettings->textureSizeY / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, bufDataPtr1);
    }
    contextSettings->textureDataReady = TRUE;
    
    return (TRUE);
}

#endif // !ARGL_DISABLE_DISP_IMAGE

#ifndef _WINRT

GLboolean arglGluCheckExtension(const GLubyte* extName, const GLubyte *extString)
{
    const GLubyte *start;
    GLubyte *where, *terminator;
    
    // Extension names should not have spaces.
    where = (GLubyte *)strchr((const char *)extName, ' ');
    if (where || *extName == '\0')
        return GL_FALSE;
    // It takes a bit of care to be fool-proof about parsing the
    //    OpenGL extensions string. Don't be fooled by sub-strings, etc.
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
        j = 13; // Of the form "OpenGL ES-XX 1.1" etc, where XX=CM for common, CL for common lite.
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

/* Create and compile a shader from the provided source(s) */
GLint arglGLCompileShaderFromFile(GLuint *shader, GLenum type, const char *pathname)
{
    FILE *fp;
    size_t len;
    char *s;
    GLint status;
    
    fp = fopen("rb", pathname);
    if (!fp) {
        ARLOGe("Unable to open shader source file '%s' for reading.\n", pathname);
        return (GL_FALSE);
    }
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (!(s = (char *)malloc(len + 1))) {
        ARLOGe("Out of memory!!\n");
        fclose(fp);
        return (GL_FALSE);
    }
    
    if (fread(s, len, 1, fp) < len) {
        ARLOGe("Error reading from shader source file '%s'.\n", pathname);
        free(s);
        fclose(fp);
        return (GL_FALSE);
    }
    s[len] = '\0';
    
    fclose(fp);
    status = arglGLCompileShaderFromString(shader, type, s);
    free(s);
    
    return (status);
}

GLint arglGLCompileShaderFromString(GLuint *shader, GLenum type, const char *s)
{
	GLint status;
	
	if (!shader || !s) return (GL_FALSE);
	
    *shader = glCreateShader(type);				// create shader
    glShaderSource(*shader, 1, &s, NULL);	// set source code in the shader
    glCompileShader(*shader);					// compile shader
	
#if defined(DEBUG)
	GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        ARLOGe("Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
		ARLOGe("Failed to compile shader:\n%s", s);
	}
	
	return status;
}

/* Link a program with all currently attached shaders */
GLint arglGLLinkProgram(GLuint prog)
{
	GLint status;
	
	glLinkProgram(prog);
	
#if defined(DEBUG)
	GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ARLOGe("Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
		ARLOGe("Failed to link program %d.\n", prog);
	
	return status;
}


/* Validate a program (for i.e. inconsistent samplers) */
GLint arglGLValidateProgram(GLuint prog)
{
	GLint logLength, status;
	
	glValidateProgram(prog);
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
		ARLOGe("Failed to validate program %d.\n", prog);
    }
	
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        ARLOGe("Program validate log:\n%s", log);
        free(log);
    }

	return status;
}

/* delete shader resources */
void arglGLDestroyShaders(GLuint vertShader, GLuint fragShader, GLuint prog)
{
	if (vertShader) {
		glDeleteShader(vertShader);
	}
	if (fragShader) {
		glDeleteShader(fragShader);
	}
	if (prog) {
		glDeleteProgram(prog);
	}
}

#endif // !_WINRT
