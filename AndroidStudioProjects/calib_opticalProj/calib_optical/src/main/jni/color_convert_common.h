/*
 *  color_convert_common.h
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
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2010-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb.
 *
 */

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