/*
 *	gsub_es.c
 *  ARToolKit5
 *
 *	Graphics Subroutines (OpenGL ES 1.1) for ARToolKit.
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
 *  Copyright 2008-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 *	Rev		Date		Who		Changes
 *  1.0.0   2008-03-30  PRL     Rewrite of gsub_lite.c from ARToolKit v4.3.2 for OpenGL ES 1.1 capabilities.
 *
 */

// ============================================================================
//    Private includes.
// ============================================================================
#include <AR/gsub_es.h>

#include <stdio.h>         // fprintf(), stderr
#include <string.h>        // strchr(), strstr(), strlen()
#include <stdlib.h>
#ifdef ANDROID
#  define valloc(s) memalign(4096,s)
#endif

#ifdef HAVE_ARM_NEON
#  include <arm_neon.h>
#  ifdef ANDROID
#    include "cpu-features.h"
#  endif
#endif

// ============================================================================
//    Private types and defines.
// ============================================================================

//#define ARGL_DEBUG
#if AR_ENABLE_MINIMIZE_MEMORY_FOOTPRINT
#  define ARGL_SUPPORT_DEBUG_MODE 0
#else
#  define ARGL_SUPPORT_DEBUG_MODE 0 // Edit as required.
#endif

#if !defined(GL_IMG_texture_format_BGRA8888) && !defined(GL_APPLE_texture_format_BGRA8888)
#  define GL_BGRA 0x80E1
#elif !defined(GL_BGRA)
#  define GL_BGRA 0x80E1
#endif

#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

struct _ARGL_CONTEXT_SETTINGS {
    ARParam arParam;
    GLuint  texture0; // For interleaved images all planes. For bi-planar images, plane 0.
    GLuint  texture1; // For interleaved images, not used.  For bi-planar images, part of plane 1.
    GLuint  texture2; // For interleaved images, not used.  For bi-planar images, part of plane 1.
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
    ARUint8 *bufRGBAdd;
    ARUint8 *bufRGBSubtract;
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
    int     useTextureYCbCrBiPlanar;
#ifdef HAVE_ARM_NEON
    int     useTextureYCbCrBiPlanarFastPath;
#endif
};
typedef struct _ARGL_CONTEXT_SETTINGS ARGL_CONTEXT_SETTINGS;

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
    // Delete previous textures, unless this is our first time here.
    if (contextSettings->textureObjectsHaveBeenSetup) {
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(contextSettings->texture0));
        if (contextSettings->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture1));
            glStateCacheActiveTexture(GL_TEXTURE2);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture2));
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
        glGenTextures(1, &(contextSettings->texture2));
        glStateCacheActiveTexture(GL_TEXTURE2);
        glStateCacheBindTexture2D(contextSettings->texture2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    contextSettings->textureObjectsHaveBeenSetup = TRUE;
    return (TRUE);
}

// Convert numPixels pixels of 8-bit CbCr (i.e. 16 bits per pixel) to two sets of RGBA pixels.
// The first set contains the positive RGB factors, the second set contains the negative factors.
// Alpha is always set to 0xFF (i.e. opaque).
// The colour conversion assumes second plane of '420f' full-range ITU R-601 as input.
static void arglCbCrToRGBA(uint8_t * __restrict destRGBAAdd, uint8_t * __restrict destRGBASubtract, uint8_t * __restrict srcCbCr, int numPixels)
{
    int i;
    float Cb, Cr;
    int R, G, B;
    ARUint8 *pCbCr, *pRGBAAdd, *pRGBASubtract;
    
    pCbCr = srcCbCr;
    pRGBAAdd = destRGBAAdd;
    pRGBASubtract = destRGBASubtract;
    
    for (i = 0; i < numPixels; i++) {
        Cb = (float)(*(pCbCr++) - 128);
        Cr = (float)(*(pCbCr++) - 128);
        R = (int)((             1.402f*Cr));
        G = (int)((-0.344f*Cb - 0.714f*Cr));
        B = (int)(( 1.772f*Cb));
        if (R < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -R;
        } else {
            *(pRGBAAdd++) = R;
            *(pRGBASubtract++) = 0;
        }
        if (G < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -G;
        } else {
            *(pRGBAAdd++) = G;
            *(pRGBASubtract++) = 0;
        }
        if (B < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -B;
        } else {
            *(pRGBAAdd++) = B;
            *(pRGBASubtract++) = 0;
        }
        *(pRGBAAdd++) = 255;
        *(pRGBASubtract++) = 255;
    }
}

// Convert numPixels pixels of 8-bit CrCb (i.e. 16 bits per pixel) to two sets of RGBA pixels.
// The first set contains the positive RGB factors, the second set contains the negative factors.
// Alpha is always set to 0xFF (i.e. opaque).
// The colour conversion assumes the second plane of 'NV21' format full-range ITU R-601 as input.
static void arglCrCbToRGBA(uint8_t * __restrict destRGBAAdd, uint8_t * __restrict destRGBASubtract, uint8_t * __restrict srcCrCb, int numPixels)
{
    int i;
    float Cb, Cr;
    int R, G, B;
    ARUint8 *pCrCb, *pRGBAAdd, *pRGBASubtract;
    
    pCrCb = srcCrCb;
    pRGBAAdd = destRGBAAdd;
    pRGBASubtract = destRGBASubtract;
    
    for (i = 0; i < numPixels; i++) {
        Cr = (float)(*(pCrCb++) - 128);
        Cb = (float)(*(pCrCb++) - 128);
        R = (int)((             1.402f*Cr));
        G = (int)((-0.344f*Cb - 0.714f*Cr));
        B = (int)(( 1.772f*Cb));
        if (R < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -R;
        } else {
            *(pRGBAAdd++) = R;
            *(pRGBASubtract++) = 0;
        }
        if (G < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -G;
        } else {
            *(pRGBAAdd++) = G;
            *(pRGBASubtract++) = 0;
        }
        if (B < 0) {
            *(pRGBAAdd++) = 0;
            *(pRGBASubtract++) = -B;
        } else {
            *(pRGBAAdd++) = B;
            *(pRGBASubtract++) = 0;
        }
        *(pRGBAAdd++) = 255;
        *(pRGBASubtract++) = 255;
    }
}

// ARM NEON-optimised version of arglCbCrToRGBA, by Philip Lamb. (Approx. 5x faster).
#ifdef HAVE_ARM_NEON
static void arglCbCrToRGBA_ARM_neon_asm(uint8_t * __restrict destRGBAAdd, uint8_t * __restrict destRGBASubtract, uint8_t * __restrict srcCbCr, int numPixels)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     "    lsr         %3,  %3,  #3     \n" // Divide arg 3 (numPixels) by 8.
                     //Setup factors etc.
					 "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
					 "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
					 "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
					 "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
					 "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
					 "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
					 "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i16    q7,  #0          \n" // Load q7 (d14-d15) with 0s.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
					 "0:						       \n"
                     // Read CbCr, convert to 16-bit.
					 "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 pixels, de-interleaving Cb into d10 (q5[0]), Cr into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cr to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cb to 16 bit in q5 (d10-d11) (overwriting).
                     // Red.
					 "    vmul.s16    q8,  q0,  q6     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmin.s16    q9,  q8,  q7     \n" // Put negative components into q9.
                     "    vneg.s16    q9,  q9          \n" // Make positive.
                     "    vmax.s16    q8,  q8,  q7     \n" // Put positive components into q8 (overwriting).
                     // Green.
					 "    vmul.s16    q10, q1,  q6     \n"
					 "    vmla.s16    q10, q2,  q5     \n" // G is now signed 16 bit in q10 (d20-21).
                     "    vmin.s16    q11, q10, q7     \n" // Put negative components into q11.
                     "    vneg.s16    q11, q11         \n" // Make positive.
                     "    vmax.s16    q10, q10, q7     \n" // Put positive components into q10 (overwriting).
                     // Blue.
					 "    vmul.s16    q12, q3,  q5     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmin.s16    q13, q12, q7     \n" // Put negative components into q13.
                     "    vneg.s16    q13, q13         \n" // Make positive.
                     "    vmax.s16    q12, q12, q7     \n" // Put positive components into q12 (overwriting).
                     // Store 'adds'.
                     "    vshrn.i16   d28, q8,  #7     \n" // Divide by 128.
                     "    vshrn.i16   d29, q10, #7     \n" // Divide by 128.
                     "    vshrn.i16   d30, q12, #7     \n" // Divide by 128.
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Store 'subtracts'.
                     "    vshrn.i16   d28, q9,  #7     \n" // Divide by 128.
                     "    vshrn.i16   d29, q11, #7     \n" // Divide by 128.
                     "    vshrn.i16   d30, q13, #7     \n" // Divide by 128.
                     "    vst4.8      {d28-d31}, [%1]! \n" // Interleave.
                     // 8 pixels done.
					 "    subs        %3, %3, #1       \n" // Decrement iteration count.
					 "    bne         0b               \n" // Repeat unil iteration count is not zero.
					 "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved.
					 :
					 : "r"(destRGBAAdd), "r"(destRGBASubtract), "r"(srcCbCr), "r"(numPixels)
					 : "cc", "r4", "r5", "r6"
					 );
}

static void arglCrCbToRGBA_ARM_neon_asm(uint8_t * __restrict destRGBAAdd, uint8_t * __restrict destRGBASubtract, uint8_t * __restrict srcCrCb, int numPixels)
{
    __asm__ volatile("    vstmdb      sp!, {d8-d15}    \n" // Save any VFP or NEON registers that should be preserved (S16-S31 / Q4-Q7).
                     "    lsr         %3,  %3,  #3     \n" // Divide arg 3 (numPixels) by 8.
                     //Setup factors etc.
					 "    mov         r4,  #179        \n" // R Cr factor =  1.402. 179/128 =  1.398 (179 = 0x00b3)
					 "    mvn         r5,  #90         \n" // G Cr factor = -0.714. -91/128 = -0.711 (-91 = 0xffa5 = NOT 90).
					 "    mvn         r6,  #43         \n" // G Cb factor = -0.344. -44/128 = -0.344 (-44 = 0xffd4 = NOT 43).
                     "    vdup.16     q0,  r4          \n" // Load q0 (d0-d1) with 8 copies of the 16 LSBs of R Cr.
					 "    vdup.16     q1,  r5          \n" // Load q1 (d2-d3) with 8 copies of the 16 LSBs of G Cr.
					 "    vdup.16     q2,  r6          \n" // Load q2 (d4-d5) with 8 copies of the 16 LSBs of G Cb.
					 "    mov         r4,  #227        \n" // B Cb factor =  1.772. 227/128 =  1.774 (227 = 0x00e3).
                     "    mov         r5,  #0x80       \n" // XOR with this value converts unsigned 8-bit val to signed 8-bit val - 128.
					 "    vdup.16     q3,  r4          \n" // Load q3 (d6-d7) with 8 copies of the 16 LSBs of B Cb.
                     "    vdup.8      d8,  r5          \n" // Load d8 (q4[0]) with 8 copies of the 8 LSBs of r5.
                     "    vmov.i16    q7,  #0          \n" // Load q7 (d14-d15) with 0s.
                     "    vmov.i8     d31, #0xFF       \n" // Load d31 (A channel of destRGBA) with FF.
					 "0:						       \n"
                     // Read CrCb, convert to 16-bit.
					 "    vld2.8      {d10-d11}, [%2]! \n" // Load 8 pixels, de-interleaving Cr into d10 (q5[0]), Cb into d11 (q5[1]).
                     "    veor.8      d10, d10, d8     \n" // Subtract 128 from Cr. Result is signed.
                     "    veor.8      d11, d11, d8     \n" // Subtract 128 from Cb. Result is signed.
                     "    vmovl.s8    q6,  d11         \n" // Sign-extend Cb to 16 bit in q6 (d12-d13).
                     "    vmovl.s8    q5,  d10         \n" // Sign-extend Cr to 16 bit in q5 (d10-d11) (overwriting).
                     // Red.
					 "    vmul.s16    q8,  q0,  q5     \n" // R is now signed 16 bit in q8 (d16-d17).
                     "    vmin.s16    q9,  q8,  q7     \n" // Put negative components into q9.
                     "    vneg.s16    q9,  q9          \n" // Make positive.
                     "    vmax.s16    q8,  q8,  q7     \n" // Put positive components into q8 (overwriting).
                     // Green.
					 "    vmul.s16    q10, q1,  q5     \n"
					 "    vmla.s16    q10, q2,  q6     \n" // G is now signed 16 bit in q10 (d20-21).
                     "    vmin.s16    q11, q10, q7     \n" // Put negative components into q11.
                     "    vneg.s16    q11, q11         \n" // Make positive.
                     "    vmax.s16    q10, q10, q7     \n" // Put positive components into q10 (overwriting).
                     // Blue.
					 "    vmul.s16    q12, q3,  q6     \n" // B is now signed 16 bit in q12 (d24-d25).
                     "    vmin.s16    q13, q12, q7     \n" // Put negative components into q13.
                     "    vneg.s16    q13, q13         \n" // Make positive.
                     "    vmax.s16    q12, q12, q7     \n" // Put positive components into q12 (overwriting).
                     // Store 'adds'.
                     "    vshrn.i16   d28, q8,  #7     \n" // Divide by 128.
                     "    vshrn.i16   d29, q10, #7     \n" // Divide by 128.
                     "    vshrn.i16   d30, q12, #7     \n" // Divide by 128.
                     "    vst4.8      {d28-d31}, [%0]! \n" // Interleave.
                     // Store 'subtracts'.
                     "    vshrn.i16   d28, q9,  #7     \n" // Divide by 128.
                     "    vshrn.i16   d29, q11, #7     \n" // Divide by 128.
                     "    vshrn.i16   d30, q13, #7     \n" // Divide by 128.
                     "    vst4.8      {d28-d31}, [%1]! \n" // Interleave.
                     // 8 pixels done.
					 "    subs        %3, %3, #1       \n" // Decrement iteration count.
					 "    bne         0b               \n" // Repeat unil iteration count is not zero.
					 "    vldmia      sp!, {d8-d15}    \n" // Restore any VFP or NEON registers that were saved..
					 :
					 : "r"(destRGBAAdd), "r"(destRGBASubtract), "r"(srcCrCb), "r"(numPixels)
					 : "cc", "r4", "r5", "r6"
					 );
}
#endif // HAVE_ARM_NEON

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

void arglCleanup(ARGL_CONTEXT_SETTINGS_REF contextSettings)
{
    if (!contextSettings) return; // Sanity check.
    
    if (contextSettings->bufRGBAdd) free(contextSettings->bufRGBAdd);
    if (contextSettings->bufRGBSubtract) free(contextSettings->bufRGBSubtract);
    
    if (contextSettings->textureObjectsHaveBeenSetup) {
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheBindTexture2D(0);
        glDeleteTextures(1, &(contextSettings->texture0));
        if (contextSettings->useTextureYCbCrBiPlanar) {
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture1));
            glStateCacheActiveTexture(GL_TEXTURE2);
            glStateCacheBindTexture2D(0);
            glDeleteTextures(1, &(contextSettings->texture2));
        }
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
    float left, right, bottom, top;
    
    if (!contextSettings) return;

    // Prepare an orthographic projection, set camera position for 2D drawing, and save GL state.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    if (contextSettings->rotate90) glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
    
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
    glOrthof(left, right, bottom, top, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();        
    glStateCacheDisableLighting();
    glStateCacheDisableDepthTest();
    
    arglDispImageStateful(contextSettings);

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
    int        i;
    
    if (!contextSettings) return;
    if (!contextSettings->textureObjectsHaveBeenSetup) return;
    if (!contextSettings->textureGeometryHasBeenSetup) return;
    if (!contextSettings->textureDataReady) return;
    
    glStateCacheActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);  
    
    glStateCacheBindTexture2D(contextSettings->texture0);
    glStateCacheTexEnvMode(GL_REPLACE);
    glStateCacheEnableTex2D();

    glStateCacheClientActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
    glTexCoordPointer(2, GL_FLOAT, 0, NULL);
    glStateCacheEnableClientStateTexCoordArray();
    
    if (contextSettings->useTextureYCbCrBiPlanar) {
        
        glStateCacheActiveTexture(GL_TEXTURE1);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        
        glStateCacheBindTexture2D(contextSettings->texture1);
        glStateCacheTexEnvMode(GL_COMBINE);
        glStateCacheTexEnvCombine(GL_ADD);
        glStateCacheTexEnvSrc0(GL_PREVIOUS);
        glStateCacheTexEnvSrc1(GL_TEXTURE);
        glStateCacheEnableTex2D();
        
        glStateCacheClientActiveTexture(GL_TEXTURE1);
        glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);
        glStateCacheEnableClientStateTexCoordArray();

        glStateCacheActiveTexture(GL_TEXTURE2);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        
        glStateCacheBindTexture2D(contextSettings->texture2);
        glStateCacheTexEnvMode(GL_COMBINE);
        glStateCacheTexEnvCombine(GL_SUBTRACT);
        glStateCacheTexEnvSrc0(GL_PREVIOUS);
        glStateCacheTexEnvSrc1(GL_TEXTURE);
        glStateCacheEnableTex2D();
        
        glStateCacheClientActiveTexture(GL_TEXTURE2);
        glBindBuffer(GL_ARRAY_BUFFER, contextSettings->t2bo);
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);
        glStateCacheEnableClientStateTexCoordArray();
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, contextSettings->v2bo);
    glVertexPointer(2, GL_FLOAT, 0, NULL);
    glStateCacheEnableClientStateVertexArray();
    glStateCacheDisableClientStateNormalArray();
    
    if (contextSettings->disableDistortionCompensation) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        for (i = 0; i < 20; i++) {
            glDrawArrays(GL_TRIANGLE_STRIP, i * 42, 42);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (contextSettings->useTextureYCbCrBiPlanar) {
        // Turn multitexturing off before handing back control, as the caller is unlikely to expect it to be on.
        glStateCacheActiveTexture(GL_TEXTURE2);
        glStateCacheTexEnvMode(GL_MODULATE);
        glStateCacheDisableTex2D();
        glStateCacheActiveTexture(GL_TEXTURE1);
        glStateCacheTexEnvMode(GL_MODULATE);
        glStateCacheDisableTex2D();
        glStateCacheActiveTexture(GL_TEXTURE0);
        glStateCacheClientActiveTexture(GL_TEXTURE0);
    }
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
            break;
        case AR_PIXEL_FORMAT_RGBA:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 4;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(4);
            break;
        case AR_PIXEL_FORMAT_RGB:
            contextSettings->pixIntFormat = GL_RGB;
            contextSettings->pixFormat = GL_RGB;
            contextSettings->pixType = GL_UNSIGNED_BYTE;
            contextSettings->pixSize = 3;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(1);
            break;
        case AR_PIXEL_FORMAT_BGRA:
            if (arglGLCapabilityCheck(0, (unsigned char *)"GL_APPLE_texture_format_BGRA8888") || arglGLCapabilityCheck(0, (unsigned char *)"GL_IMG_texture_format_BGRA8888")) {
                contextSettings->pixIntFormat = GL_RGBA;
                contextSettings->pixFormat = GL_BGRA;
                contextSettings->pixType = GL_UNSIGNED_BYTE;
                contextSettings->pixSize = 4;
                contextSettings->useTextureYCbCrBiPlanar = FALSE;
                glStateCachePixelStoreUnpackAlignment(4);
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
            break;
        case AR_PIXEL_FORMAT_RGB_565:
            contextSettings->pixIntFormat = GL_RGB;
            contextSettings->pixFormat = GL_RGB;
            contextSettings->pixType = GL_UNSIGNED_SHORT_5_6_5;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            break;
        case AR_PIXEL_FORMAT_RGBA_5551:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_SHORT_5_5_5_1;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            break;
        case AR_PIXEL_FORMAT_RGBA_4444:
            contextSettings->pixIntFormat = GL_RGBA;
            contextSettings->pixFormat = GL_RGBA;
            contextSettings->pixType = GL_UNSIGNED_SHORT_4_4_4_4;
            contextSettings->pixSize = 2;
            contextSettings->useTextureYCbCrBiPlanar = FALSE;
            glStateCachePixelStoreUnpackAlignment(2);
            break;
        default:
            ARLOGe("Error: ARGL: set pixel format called with unsupported pixel format.\n");
            return (FALSE);
            break;
    }
    contextSettings->format = format;
    ARLOGd("ARGL: set pixel format %s.\n", arUtilGetPixelFormatName(format));
    contextSettings->textureDataReady = FALSE;
    
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
}

char arglGetFlipV(ARGL_CONTEXT_SETTINGS_REF contextSettings)
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
    
    if (arglGLCapabilityCheck(0u, (const unsigned char *)"GL_APPLE_texture_2D_limited_npot")\
        || arglGLCapabilityCheck(0u, (const unsigned char *)"GL_OES_texture_npot")) {
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
    
    // Free bufRGBs, and recreate  at the new size if required.
    if (contextSettings->bufRGBAdd) {
        free(contextSettings->bufRGBAdd);
        contextSettings->bufRGBAdd = NULL;
    }
    if (contextSettings->bufRGBSubtract) {
        free(contextSettings->bufRGBSubtract);
        contextSettings->bufRGBAdd = NULL;
    }
    
    if (contextSettings->useTextureYCbCrBiPlanar) {
        
        int pixels = bufWidth/2 * bufHeight/2;
        contextSettings->bufRGBAdd = valloc(pixels * 4); // RGBA = 4 bytes per pixel.
        if (!contextSettings->bufRGBAdd) return (FALSE);
        contextSettings->bufRGBSubtract = valloc(pixels * 4); // RGBA = 4 bytes per pixel.
        if (!contextSettings->bufRGBSubtract) {
            free(contextSettings->bufRGBAdd);
            return (FALSE);
        }

#ifdef HAVE_ARM_NEON
        contextSettings->useTextureYCbCrBiPlanarFastPath = (pixels % 8 == 0);
#  ifdef ANDROID
        // Not all Android devices with ARMv7 are guaranteed to have NEON, so check.
        uint64_t features = android_getCpuFeatures();
        contextSettings->useTextureYCbCrBiPlanarFastPath = contextSettings->useTextureYCbCrBiPlanarFastPath && (features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON);
#  endif
        ARLOGd("argl %s using ARM NEON acceleration.\n", (contextSettings->useTextureYCbCrBiPlanarFastPath ? "is" : "is NOT"));
#endif
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

char arglPixelBufferDataUploadBiPlanar(ARGL_CONTEXT_SETTINGS_REF contextSettings, ARUint8 *bufDataPtr0, ARUint8 *bufDataPtr1)
{
    int sizeX, sizeY;
    
    if (!contextSettings) return (FALSE);
    if (!contextSettings->textureObjectsHaveBeenSetup || !contextSettings->textureGeometryHasBeenSetup || !contextSettings->pixSize) return (FALSE);

    glStateCacheActiveTexture(GL_TEXTURE0);
    glStateCacheBindTexture2D(contextSettings->texture0);
    
    if (contextSettings->bufSizeIsTextureSize) {
        
        glTexImage2D(GL_TEXTURE_2D, 0, contextSettings->pixIntFormat, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, contextSettings->pixFormat, contextSettings->pixType, bufDataPtr0);
        
        if (bufDataPtr1 && contextSettings->useTextureYCbCrBiPlanar) {
            
            sizeX = contextSettings->textureSizeX / 2;
            sizeY = contextSettings->textureSizeY / 2;
            
#ifdef HAVE_ARM_NEON
            if (contextSettings->useTextureYCbCrBiPlanarFastPath) {
                if (contextSettings->format == AR_PIXEL_FORMAT_NV21) arglCrCbToRGBA_ARM_neon_asm(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
                else arglCbCrToRGBA_ARM_neon_asm(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
            } else {
#endif
                if (contextSettings->format == AR_PIXEL_FORMAT_NV21) arglCrCbToRGBA(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
                else arglCbCrToRGBA(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
#ifdef HAVE_ARM_NEON
            }
#endif

            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(contextSettings->texture1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, contextSettings->bufRGBAdd);
            glStateCacheActiveTexture(GL_TEXTURE2);
            glStateCacheBindTexture2D(contextSettings->texture2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, contextSettings->bufRGBSubtract);
            glStateCacheActiveTexture(GL_TEXTURE0);
        }
        
    } else {
        
        // Request OpenGL allocate memory internally for a power-of-two texture of the appropriate size.
        // Then send the NPOT-data as a subimage.
        glTexImage2D(GL_TEXTURE_2D, 0, contextSettings->pixIntFormat, contextSettings->textureSizeX, contextSettings->textureSizeY, 0, contextSettings->pixFormat, contextSettings->pixType, NULL);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, contextSettings->bufSizeX, contextSettings->bufSizeY, contextSettings->pixFormat, contextSettings->pixType, bufDataPtr0);
        
        if (bufDataPtr1 && contextSettings->useTextureYCbCrBiPlanar) {
            
            sizeX = contextSettings->bufSizeX / 2;
            sizeY = contextSettings->bufSizeY / 2;
            
#ifdef HAVE_ARM_NEON
            if (contextSettings->useTextureYCbCrBiPlanarFastPath) {
                if (contextSettings->format == AR_PIXEL_FORMAT_NV21) arglCrCbToRGBA_ARM_neon_asm(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
                else arglCbCrToRGBA_ARM_neon_asm(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
            } else {
#endif
                if (contextSettings->format == AR_PIXEL_FORMAT_NV21) arglCrCbToRGBA(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
                else arglCbCrToRGBA(contextSettings->bufRGBAdd, contextSettings->bufRGBSubtract, bufDataPtr1, sizeX*sizeY);
#ifdef HAVE_ARM_NEON
            }
#endif
            
            glStateCacheActiveTexture(GL_TEXTURE1);
            glStateCacheBindTexture2D(contextSettings->texture1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, contextSettings->textureSizeX / 2, contextSettings->textureSizeY / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, contextSettings->bufRGBAdd);
            glStateCacheActiveTexture(GL_TEXTURE2);
            glStateCacheBindTexture2D(contextSettings->texture2);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, contextSettings->textureSizeX / 2, contextSettings->textureSizeY / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizeX, sizeY, GL_RGBA, GL_UNSIGNED_BYTE, contextSettings->bufRGBSubtract);
            glStateCacheActiveTexture(GL_TEXTURE0);
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

