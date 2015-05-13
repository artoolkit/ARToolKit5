/*
 *  color_convert_common.c
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

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

void color_convert_common(unsigned char *pY, unsigned char *pUV, int width, int height, unsigned char *buffer)
{
	const int bytes_per_pixel = 2;

	int nR, nG, nB, nY, nU, nV, i, j, id2, jd2, offset;	
	unsigned char *out = buffer;

	offset = 0;
	
	// YUV 4:2:0
	for (i = 0; i < height; i++) {

		id2 = i >> 1; // Divide by two

	    for (j = 0; j < width; j++) {
		
			jd2 = j >> 1; // Divide by two
		
			nY = *(pY + i * width + j);
			nV = *(pUV + id2 * width + bytes_per_pixel * jd2);
			nU = *(pUV + id2 * width + bytes_per_pixel * jd2 + 1);
	    
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
			
			out[offset++] = (unsigned char)nR;
			out[offset++] = (unsigned char)nG;
			out[offset++] = (unsigned char)nB;
			out[offset++] = (unsigned char)255;
	    }
	}
}  
