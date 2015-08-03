//
// Color conversion from NV21 to RGBA
//
// NV21 format:
// YUV 4:2:0 image with a plane of 8 bit Y samples followed by an interleaved
// V/I plane containing 8 bit 2x2 subsampled chroma samples.
// See http://www.fourcc.org/yuv.php#NV21.
//
//                      H V
// Y Sample Period      1 1
// U (Cb) Sample Period 2 2
// V (Cr) Sample Period 2 2
//

#ifndef __color_convert_common_h__
#define __color_convert_common_h__

#ifdef __cplusplus
extern "C" {
#endif

void color_convert_common(unsigned char *pY, unsigned char *pUV, int width, int height, unsigned char *buffer);

#ifdef __cplusplus
}
#endif
#endif // !__color_convert_common_h__