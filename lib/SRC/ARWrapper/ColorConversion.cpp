/*
 *  ColorConversion.cpp
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

#include "ARWrapper/ColorConversion.h"

// ----------------------------------------------------------------------------------------------------
// Color conversion
// ----------------------------------------------------------------------------------------------------

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


/*
   YUV 4:2:0 image with a plane of 8 bit Y samples followed by an interleaved
   U/V plane containing 8 bit 2x2 subsampled chroma samples.
   except the interleave order of U and V is reversed.

                        H V
   Y Sample Period      1 1
   U (Cb) Sample Period 2 2
   V (Cr) Sample Period 2 2
 */

void color_convert_common(unsigned char *pY, unsigned char *pUV, int width, int height, unsigned char *buffer)  {
	
	int nR, nG, nB, nY, nU, nV, i, j, id2, jd2, offset;	

	offset = 0;
	
	// YUV 4:2:0
	for (i = 0; i < height; i++) {

		id2 = i >> 1; // Divide by two

	    for (j = 0; j < width; j++) {
		
			jd2 = j >> 1; // Divide by two
		
			nY = *(pY + i*width + j);
			nV = *(pUV + id2*width + 2*jd2);
			nU = *(pUV + id2*width + 2*jd2 + 1);
	    
			// Yuv Convert
			nY = MAX(nY - 16, 0);
			nU -= 128;
			nV -= 128;		
			
			nB = 1192 * nY + 2066 * nU;
			nG = 1192 * nY - 833 * nV - 400 * nU;
			nR = 1192 * nY + 1634 * nV;
			
			nR = MIN(262143, MAX(0, nR));
			nG = MIN(262143, MAX(0, nG));
			nB = MIN(262143, MAX(0, nB));
			
			nR >>= 10; nR &= 0xff;
			nG >>= 10; nG &= 0xff;
			nB >>= 10; nB &= 0xff;
			
			buffer[offset++] = (unsigned char)nR;
			buffer[offset++] = (unsigned char)nG;
			buffer[offset++] = (unsigned char)nB;
			buffer[offset++] = (unsigned char)255;
	    }
	    
	}
}
