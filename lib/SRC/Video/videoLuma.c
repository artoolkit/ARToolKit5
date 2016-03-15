/*
 *  videoLuma.c
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
 *  Copyright 2015-2016 Daqri, LLC.
 *
 *  Author(s): Philip Lamb
 *
 */

#include <AR/videoLuma.h>

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

// CCIR 601 recommended values. See http://www.poynton.com/notes/colour_and_gamma/ColorFAQ.html#RTFToC11 .
#define R8_CCIR601 77
#define G8_CCIR601 150
#define B8_CCIR601 29

struct _ARVideoLumaInfo {
    int xsize;
    int ysize;
    int buffSize;
    AR_PIXEL_FORMAT pixFormat;
#ifdef HAVE_ARM_NEON
    int fastPath;
#endif
    ARUint8 *__restrict buff;
};

#ifdef HAVE_ARM_NEON
static void arVideoLumaBGRAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels);
static void arVideoLumaRGBAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels);
static void arVideoLumaABGRtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels);
static void arVideoLumaARGBtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels);
#endif


ARVideoLumaInfo *arVideoLumaInit(int xsize, int ysize, AR_PIXEL_FORMAT pixFormat)
{
    ARVideoLumaInfo *vli;
    
    vli = (ARVideoLumaInfo *)calloc(1, sizeof(ARVideoLumaInfo));
    if (!vli) {
        ARLOGe("Out of memory!!\n");
        return (NULL);
    }
    vli->xsize = xsize;
    vli->ysize = ysize;
    vli->buffSize = xsize*ysize;
    vli->buff = (ARUint8 *)valloc(vli->buffSize);
    if (!vli->buff) {
        ARLOGe("Out of memory!!\n");
        free(vli);
        return (NULL);
    }
    vli->pixFormat = pixFormat;
#ifdef HAVE_ARM_NEON
    vli->fastPath = (xsize * ysize % 8 == 0
                     && (pixFormat == AR_PIXEL_FORMAT_RGBA
                         || pixFormat == AR_PIXEL_FORMAT_BGRA
                         || pixFormat == AR_PIXEL_FORMAT_ABGR
                         ||pixFormat == AR_PIXEL_FORMAT_ARGB
                         )
                     );
#  ifdef ANDROID
    // Not all Android devices with ARMv7 are guaranteed to have NEON, so check.
    uint64_t features = android_getCpuFeatures();
    vli->fastPath = vli->fastPath && (features & ANDROID_CPU_ARM_FEATURE_ARMv7) && (features & ANDROID_CPU_ARM_FEATURE_NEON);
#  endif
    if (vli->fastPath) ARLOGd("arVideoLuma will use ARM NEON acceleration.\n");
#endif

    return (vli);
}

int arVideoLumaFinal(ARVideoLumaInfo **vli_p)
{
    if (!vli_p) return (-1);
    if (!*vli_p) return (0);
    
    free((*vli_p)->buff);
    free(*vli_p);
    *vli_p = NULL;
    
    return (0);
}

ARUint8 *__restrict arVideoLuma(ARVideoLumaInfo *vli, const ARUint8 *__restrict dataPtr)
{
    unsigned int p, q;
    
    AR_PIXEL_FORMAT pixFormat = vli->pixFormat;
#ifdef HAVE_ARM_NEON
    if (vli->fastPath) {
        if (pixFormat == AR_PIXEL_FORMAT_BGRA) {
            arVideoLumaBGRAtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA) {
            arVideoLumaRGBAtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else if (pixFormat == AR_PIXEL_FORMAT_ABGR) {
            arVideoLumaABGRtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        } else /*(pixFormat == AR_PIXEL_FORMAT_ARGB)*/ {
            arVideoLumaARGBtoL_ARM_neon_asm(vli->buff, (unsigned char *__restrict)dataPtr, vli->buffSize);
        }
        return (0);
    }
#endif
    if (pixFormat == AR_PIXEL_FORMAT_MONO || pixFormat == AR_PIXEL_FORMAT_420v || pixFormat == AR_PIXEL_FORMAT_420f || pixFormat == AR_PIXEL_FORMAT_NV21) {
        memcpy(vli->buff, dataPtr, vli->buffSize);
    } else {
        q = 0;
        if (pixFormat == AR_PIXEL_FORMAT_RGBA) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + B8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_BGRA) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + R8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_ARGB) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 1] + G8_CCIR601*dataPtr[q + 2] + B8_CCIR601*dataPtr[q + 3]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_ABGR) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 1] + G8_CCIR601*dataPtr[q + 2] + R8_CCIR601*dataPtr[q + 3]) >> 8;
                q += 4;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGB) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + B8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 3;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_BGR) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (B8_CCIR601*dataPtr[q + 0] + G8_CCIR601*dataPtr[q + 1] + R8_CCIR601*dataPtr[q + 2]) >> 8;
                q += 3;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_yuvs) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = dataPtr[q + 0];
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_2vuy) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = dataPtr[q + 1];
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGB_565) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf8) + 4) + G8_CCIR601*(((dataPtr[q + 0] & 0x07) << 5) + ((dataPtr[q + 1] & 0xe0) >> 3) + 2) + B8_CCIR601*(((dataPtr[q + 1] & 0x1f) << 3) + 4)) >> 8;
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA_5551) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf8) + 4) + G8_CCIR601*(((dataPtr[q + 0] & 0x07) << 5) + ((dataPtr[q + 1] & 0xc0) >> 3) + 2) + B8_CCIR601*(((dataPtr[q + 1] & 0x3e) << 2) + 4)) >> 8;
                q += 2;
            }
        } else if (pixFormat == AR_PIXEL_FORMAT_RGBA_4444) {
            for (p = 0; p < vli->buffSize; p++) {
                vli->buff[p] = (R8_CCIR601*((dataPtr[q + 0] & 0xf0) + 8) + G8_CCIR601*(((dataPtr[q + 0] & 0x0f) << 4) + 8) + B8_CCIR601*((dataPtr[q + 1] & 0xf0) + 8)) >> 8;
                q += 2;
            }
        } else {
            ARLOGe("Error: Unsupported pixel format passed to arVideoLuma().\n");
            return (NULL);
        }
    }
    return (vli->buff);
}

//
// Methods from http://computer-vision-talks.com/2011/02/a-very-fast-bgra-to-grayscale-conversion-on-iphone/
//
#ifdef HAVE_ARM_NEON
#if 0
static void arVideoLumaBGRAtoL_ARM_neon(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    int i;
    uint8x8_t rfac = vdup_n_u8 (R8_CCIR601);
    uint8x8_t gfac = vdup_n_u8 (G8_CCIR601);
    uint8x8_t bfac = vdup_n_u8 (B8_CCIR601);
    int n = numPixels / 8;
    
    // Convert per eight pixels.
    for (i = 0; i < n; i++) {
        uint16x8_t  temp;
        uint8x8x4_t rgb  = vld4_u8 (src);
        uint8x8_t result;
        
        temp = vmull_u8 (rgb.val[0],      bfac);
        temp = vmlal_u8 (temp,rgb.val[1], gfac);
        temp = vmlal_u8 (temp,rgb.val[2], rfac);
        
        result = vshrn_n_u16 (temp, 8);
        vst1_u8 (dest, result);
        src  += 8*4;
        dest += 8;
    }
}
#endif


static void arVideoLumaBGRAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #29          \n" // Blue channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #77          \n" // Red channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // B into d0, G into d1, R into d2, A into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d0, d4       \n"
                     "vmlal.u8    q7, d1, d5       \n"
                     "vmlal.u8    q7, d2, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaRGBAtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #77          \n" // Red channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #29          \n" // Blue channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // R into d0, G into d1, B into d2, A into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d0, d4       \n"
                     "vmlal.u8    q7, d1, d5       \n"
                     "vmlal.u8    q7, d2, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaABGRtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #29          \n" // Blue channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #77          \n" // Red channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // A into d0, B into d1, G into d2, R into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d1, d4       \n"
                     "vmlal.u8    q7, d2, d5       \n"
                     "vmlal.u8    q7, d3, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

static void arVideoLumaARGBtoL_ARM_neon_asm(uint8_t * __restrict dest, uint8_t * __restrict src, int numPixels)
{
    __asm__ volatile("lsr          %2, %2, #3      \n" // Divide arg 2 (numPixels) by 8.
                     "# build the three constants: \n"
                     "mov         r4, #77          \n" // Red channel multiplier.
                     "mov         r5, #150         \n" // Green channel multiplier.
                     "mov         r6, #29          \n" // Blue channel multiplier.
                     "vdup.8      d4, r4           \n"
                     "vdup.8      d5, r5           \n"
                     "vdup.8      d6, r6           \n"
                     "0:						   \n"
                     "# load 8 pixels:             \n"
                     "vld4.8      {d0-d3}, [%1]!   \n" // A into d0, R into d1, G into d2, B into d3.
                     "# do the weight average:     \n"
                     "vmull.u8    q7, d1, d4       \n"
                     "vmlal.u8    q7, d2, d5       \n"
                     "vmlal.u8    q7, d3, d6       \n"
                     "# shift and store:           \n"
                     "vshrn.u16   d7, q7, #8       \n" // Divide q3 by 256 and store in the d7.
                     "vst1.8      {d7}, [%0]!      \n"
                     "subs        %2, %2, #1       \n" // Decrement iteration count.
                     "bne         0b               \n" // Repeat unil iteration count is not zero.
                     :
                     : "r"(dest), "r"(src), "r"(numPixels)
                     : "cc", "r4", "r5", "r6"
                     );
}

#endif // HAVE_ARM_NEON

