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
