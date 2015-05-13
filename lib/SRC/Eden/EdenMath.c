/*
 *  EdenMath.c
 *  The SRMS simulator
 *
 *	Copyright (c) 2001-2013 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *	
 *	Rev		Date		Who		Changes
 *	1.0.0	2001-07-28	PRL		Initial version.
 *
 */

// @@BEGIN_EDEN_LICENSE_HEADER@@
//
//  This file is part of The Eden Library.
//
//  The Eden Library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  The Eden Library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with The Eden Library.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
// @@END_EDEN_LICENSE_HEADER@@

// ============================================================================
//	Private includes
// ============================================================================

#include <Eden/EdenMath.h>
#include <stdio.h>

// ============================================================================
//	Private types and defines
// ============================================================================


// ============================================================================
//	Public functions
// ============================================================================

float EdenMathNormalise(float v[3])
{
    float l;
    
    l = (float)LENGTH(v);
	if (l) {
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
	return (l);
}

double EdenMathNormalised(double v[3])
{
    double l;
    
    l = (double)LENGTH(v);
	if (l) {
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
	return (l);
}

void EdenMathPointsToPlaneHessianNormal(float n[3], float *p, const float p1[3], const float p2[3], const float p3[3])
{
	float l;

	if (!p1 || !p2 || !p3 || !n) return; // Sanity check.
	EdenMathPointsToPlane(n, p, p1, p2, p3);
	l = LENGTH(n);
	if (l) {
		n[0] /= l;
		n[1] /= l;
		n[2] /= l;
		if (p) *p /= l;
	}
}

void EdenMathPointsToPlane(float abc[3], float *d, const float p1[3], const float p2[3], const float p3[3])
{
	if (!p1 || !p2 || !p3) return; // Sanity check.
	
	if (abc) {
		// Equivalent to SUB(pa,p2,p1); SUB(pb,p3,p1); CROSS(abc,pa,pb);
		abc[0] = p1[1]*(p2[2] - p3[2]) + p2[1]*(p3[2] - p1[2]) + p3[1]*(p1[2] - p2[2]);
		abc[1] = p1[2]*(p2[0] - p3[0]) + p2[2]*(p3[0] - p1[0]) + p3[2]*(p1[0] - p2[0]);
		abc[2] = p1[0]*(p2[1] - p3[1]) + p2[0]*(p3[1] - p1[1]) + p3[0]*(p1[1] - p2[1]);
	}
	if (d) *d = -(p1[0]*(p2[1]*p3[2] - p3[1]*p2[2]) + p2[0]*(p3[1]*p1[2] - p1[1]*p3[2]) + p3[0]*(p1[1]*p2[2] - p2[1]*p1[2]));
}

EDEN_BOOL EdenMathIntersectionLinePlane(float intersection[3], float *u_out, const float p1[3], const float p2[3], const float abc[3], const float d)
{
	float p2minusp1[3], u, ud;
	
	if (!abc || !p1 || !p2) return (FALSE); // Sanity check.
	
	SUB(p2minusp1, p2, p1);
	ud = -(DOT(abc,p2minusp1));
	if (!ud) return (FALSE); // Normal to plane is perpendicular to line.
	u = (DOT(abc, p1) + d) / ud;
	intersection[0] = p1[0] + u * p2minusp1[0];
	intersection[1] = p1[1] + u * p2minusp1[1];
	intersection[2] = p1[2] + u * p2minusp1[2];
	if (u_out) *u_out = u;
	return (TRUE);
}

float EdenMathCalcDistanceToPlane(const float x0[3], const float abc[3], const float d)
{
	if (!x0 || !abc) return (0.0f);
	return ((DOT(abc, x0) + d)/LENGTH(abc));
}

void EdenMathIdentityMatrix3by3(float mtx9[9])
{
	mtx9[1] = mtx9[2] = mtx9[3] = mtx9[5] = mtx9[6] = mtx9[7] = 0.0f;
	mtx9[0] = mtx9[4] = mtx9[8] = 1.0f;
}

void EdenMathMultMatrix3by3(float C[9], const float B[9], const float A[9])
{
	#define M(NAME,row,col)  NAME[col*3+row]	// M allows us to use row-major notation.
	// 27 mults + 18 adds
	M(C,0,0) = M(B,0,0)*M(A,0,0) + M(B,0,1)*M(A,1,0) + M(B,0,2)*M(A,2,0);
	M(C,1,0) = M(B,1,0)*M(A,0,0) + M(B,1,1)*M(A,1,0) + M(B,1,2)*M(A,2,0);
	M(C,2,0) = M(B,2,0)*M(A,0,0) + M(B,2,1)*M(A,1,0) + M(B,2,2)*M(A,2,0);
	M(C,0,1) = M(B,0,0)*M(A,0,1) + M(B,0,1)*M(A,1,1) + M(B,0,2)*M(A,2,1);
	M(C,1,1) = M(B,1,0)*M(A,0,1) + M(B,1,1)*M(A,1,1) + M(B,1,2)*M(A,2,1);
	M(C,2,1) = M(B,2,0)*M(A,0,1) + M(B,2,1)*M(A,1,1) + M(B,2,2)*M(A,2,1);
	M(C,0,2) = M(B,0,0)*M(A,0,2) + M(B,0,1)*M(A,1,2) + M(B,0,2)*M(A,2,2);
	M(C,1,2) = M(B,1,0)*M(A,0,2) + M(B,1,1)*M(A,1,2) + M(B,1,2)*M(A,2,2);
	M(C,2,2) = M(B,2,0)*M(A,0,2) + M(B,2,1)*M(A,1,2) + M(B,2,2)*M(A,2,2);
    #undef M
}

//
//  Invert a 3x3 matrix A, placing result in Ainv.
//  A and Ainv are in column-major form, which is standard for OpenGL
//  (wheras the usual mathematical matrix notation is row-major.)
//
EDEN_BOOL EdenMathInvertMatrix3by3(float A[9], float Ainv[9])
{
#define M(NAME,row,col)  NAME[col*3+row]	// M allows us to use row-major notation.
	float	D;
	
	// 30 mults/divides, 12 adds/subtracts.
	// Calculate adjoint matrix, by taking determinants of cofactors.	
	M(Ainv,0,0) =  (M(A,1,1)*M(A,2,2) - M(A,2,1)*M(A,1,2));
	M(Ainv,1,0) = -(M(A,1,0)*M(A,2,2) - M(A,2,0)*M(A,1,2));
	M(Ainv,2,0) =  (M(A,1,0)*M(A,2,1) - M(A,2,0)*M(A,1,1));
	
	M(Ainv,0,1) = -(M(A,0,1)*M(A,2,2) - M(A,2,1)*M(A,0,2));
	M(Ainv,1,1) =  (M(A,0,0)*M(A,2,2) - M(A,2,0)*M(A,0,2));
	M(Ainv,2,1) = -(M(A,0,0)*M(A,2,1) - M(A,2,0)*M(A,0,1));
	
	M(Ainv,0,2) =  (M(A,0,1)*M(A,1,2) - M(A,1,1)*M(A,0,2));
	M(Ainv,1,2) = -(M(A,0,0)*M(A,1,2) - M(A,1,0)*M(A,0,2));
	M(Ainv,2,2) =  (M(A,0,0)*M(A,1,1) - M(A,1,0)*M(A,0,1));
	
	// Determinant, reusing some determinants of cofactors from above.
	D = M(A,0,0)*M(Ainv,0,0) + M(A,0,1)*M(Ainv,1,0) + M(A,0,2)*M(Ainv,2,0);
	if(D == 0.0)		// Not invertible if determinant is equal to zero.
	{
		return (FALSE);
	}
	
	M(Ainv,0,0) /= D;
	M(Ainv,1,0) /= D;
	M(Ainv,2,0) /= D;
	
	M(Ainv,0,1) /= D;
	M(Ainv,1,1) /= D;
	M(Ainv,2,1) /= D;
	
	M(Ainv,0,2) /= D;
	M(Ainv,1,2) /= D;
	M(Ainv,2,2) /= D;
	
	return (TRUE);
#undef M
}

void EdenMathIdentityMatrix(float mtx16[16])
{
	mtx16[1] = mtx16[2] = mtx16[3] = mtx16[4] = mtx16[6] = mtx16[7] = mtx16[8] = mtx16[9] = mtx16[11] = mtx16[12] = mtx16[13] = mtx16[14] = 0.0f;
	mtx16[0] = mtx16[5] = mtx16[10] = mtx16[15] = 1.0f;
}

void EdenMathMultMatrix(float C[16], const float B[16], const float A[16])
{
	#define M(NAME,row,col)  NAME[col*4+row]	// M allows us to use row-major notation.
	// 64 mults + 48 adds
	M(C,0,0) = M(B,0,0)*M(A,0,0) + M(B,0,1)*M(A,1,0) + M(B,0,2)*M(A,2,0) + M(B,0,3)*M(A,3,0);
	M(C,1,0) = M(B,1,0)*M(A,0,0) + M(B,1,1)*M(A,1,0) + M(B,1,2)*M(A,2,0) + M(B,1,3)*M(A,3,0);
	M(C,2,0) = M(B,2,0)*M(A,0,0) + M(B,2,1)*M(A,1,0) + M(B,2,2)*M(A,2,0) + M(B,2,3)*M(A,3,0);
	M(C,3,0) = M(B,3,0)*M(A,0,0) + M(B,3,1)*M(A,1,0) + M(B,3,2)*M(A,2,0) + M(B,3,3)*M(A,3,0);
	
	M(C,0,1) = M(B,0,0)*M(A,0,1) + M(B,0,1)*M(A,1,1) + M(B,0,2)*M(A,2,1) + M(B,0,3)*M(A,3,1);
	M(C,1,1) = M(B,1,0)*M(A,0,1) + M(B,1,1)*M(A,1,1) + M(B,1,2)*M(A,2,1) + M(B,1,3)*M(A,3,1);
	M(C,2,1) = M(B,2,0)*M(A,0,1) + M(B,2,1)*M(A,1,1) + M(B,2,2)*M(A,2,1) + M(B,2,3)*M(A,3,1);
	M(C,3,1) = M(B,3,0)*M(A,0,1) + M(B,3,1)*M(A,1,1) + M(B,3,2)*M(A,2,1) + M(B,3,3)*M(A,3,1);
	
	M(C,0,2) = M(B,0,0)*M(A,0,2) + M(B,0,1)*M(A,1,2) + M(B,0,2)*M(A,2,2) + M(B,0,3)*M(A,3,2);
	M(C,1,2) = M(B,1,0)*M(A,0,2) + M(B,1,1)*M(A,1,2) + M(B,1,2)*M(A,2,2) + M(B,1,3)*M(A,3,2);
	M(C,2,2) = M(B,2,0)*M(A,0,2) + M(B,2,1)*M(A,1,2) + M(B,2,2)*M(A,2,2) + M(B,2,3)*M(A,3,2);
	M(C,3,2) = M(B,3,0)*M(A,0,2) + M(B,3,1)*M(A,1,2) + M(B,3,2)*M(A,2,2) + M(B,3,3)*M(A,3,2);

	M(C,0,3) = M(B,0,0)*M(A,0,3) + M(B,0,1)*M(A,1,3) + M(B,0,2)*M(A,2,3) + M(B,0,3)*M(A,3,3);
	M(C,1,3) = M(B,1,0)*M(A,0,3) + M(B,1,1)*M(A,1,3) + M(B,1,2)*M(A,2,3) + M(B,1,3)*M(A,3,3);
	M(C,2,3) = M(B,2,0)*M(A,0,3) + M(B,2,1)*M(A,1,3) + M(B,2,2)*M(A,2,3) + M(B,2,3)*M(A,3,3);
	M(C,3,3) = M(B,3,0)*M(A,0,3) + M(B,3,1)*M(A,1,3) + M(B,3,2)*M(A,2,3) + M(B,3,3)*M(A,3,3);
	#undef M
}

//
//  Multiplies 4x4 matrix B into 4x4 matrix A, placing result in C.
//  A, B and C are in column-major form, which is standard for OpenGL
//  (wheras the usual mathematical matrix notation is row-major.)
//
void EdenMathMultMatrixd(double C[16], const double B[16], const double A[16])
{
#define M(NAME,row,col)  NAME[col*4+row]	// M allows us to use row-major notation.
	// 64 mults + 48 adds
	M(C,0,0) = M(B,0,0)*M(A,0,0) + M(B,0,1)*M(A,1,0) + M(B,0,2)*M(A,2,0) + M(B,0,3)*M(A,3,0);
	M(C,1,0) = M(B,1,0)*M(A,0,0) + M(B,1,1)*M(A,1,0) + M(B,1,2)*M(A,2,0) + M(B,1,3)*M(A,3,0);
	M(C,2,0) = M(B,2,0)*M(A,0,0) + M(B,2,1)*M(A,1,0) + M(B,2,2)*M(A,2,0) + M(B,2,3)*M(A,3,0);
	M(C,3,0) = M(B,3,0)*M(A,0,0) + M(B,3,1)*M(A,1,0) + M(B,3,2)*M(A,2,0) + M(B,3,3)*M(A,3,0);

	M(C,0,1) = M(B,0,0)*M(A,0,1) + M(B,0,1)*M(A,1,1) + M(B,0,2)*M(A,2,1) + M(B,0,3)*M(A,3,1);
	M(C,1,1) = M(B,1,0)*M(A,0,1) + M(B,1,1)*M(A,1,1) + M(B,1,2)*M(A,2,1) + M(B,1,3)*M(A,3,1);
	M(C,2,1) = M(B,2,0)*M(A,0,1) + M(B,2,1)*M(A,1,1) + M(B,2,2)*M(A,2,1) + M(B,2,3)*M(A,3,1);
	M(C,3,1) = M(B,3,0)*M(A,0,1) + M(B,3,1)*M(A,1,1) + M(B,3,2)*M(A,2,1) + M(B,3,3)*M(A,3,1);

	M(C,0,2) = M(B,0,0)*M(A,0,2) + M(B,0,1)*M(A,1,2) + M(B,0,2)*M(A,2,2) + M(B,0,3)*M(A,3,2);
	M(C,1,2) = M(B,1,0)*M(A,0,2) + M(B,1,1)*M(A,1,2) + M(B,1,2)*M(A,2,2) + M(B,1,3)*M(A,3,2);
	M(C,2,2) = M(B,2,0)*M(A,0,2) + M(B,2,1)*M(A,1,2) + M(B,2,2)*M(A,2,2) + M(B,2,3)*M(A,3,2);
	M(C,3,2) = M(B,3,0)*M(A,0,2) + M(B,3,1)*M(A,1,2) + M(B,3,2)*M(A,2,2) + M(B,3,3)*M(A,3,2);

	M(C,0,3) = M(B,0,0)*M(A,0,3) + M(B,0,1)*M(A,1,3) + M(B,0,2)*M(A,2,3) + M(B,0,3)*M(A,3,3);
	M(C,1,3) = M(B,1,0)*M(A,0,3) + M(B,1,1)*M(A,1,3) + M(B,1,2)*M(A,2,3) + M(B,1,3)*M(A,3,3);
	M(C,2,3) = M(B,2,0)*M(A,0,3) + M(B,2,1)*M(A,1,3) + M(B,2,2)*M(A,2,3) + M(B,2,3)*M(A,3,3);
	M(C,3,3) = M(B,3,0)*M(A,0,3) + M(B,3,1)*M(A,1,3) + M(B,3,2)*M(A,2,3) + M(B,3,3)*M(A,3,3);
#undef M
}

/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
EDEN_BOOL EdenMathInvertMatrix(float out[16], const float m[16])
{
    /* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]
    
    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;
    
    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
    
    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
    r0[4] = 1.0f, r0[5] = r0[6] = r0[7] = 0.0f,
    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
    r1[5] = 1.0f, r1[4] = r1[6] = r1[7] = 0.0f,
    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
    r2[6] = 1.0f, r2[4] = r2[5] = r2[7] = 0.0f,
    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
    r3[7] = 1.0f, r3[4] = r3[5] = r3[6] = 0.0f;
    
    /* choose pivot - or die */
    if (fabsf(r3[0]) > fabsf(r2[0]))
        SWAP_ROWS(r3, r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        SWAP_ROWS(r2, r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        SWAP_ROWS(r1, r0);
    if (0.0f == r0[0])
        return FALSE;
    
    /* eliminate first variable     */
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0f) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0f) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0f) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0f) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    
    /* choose pivot - or die */
    if (fabsf(r3[1]) > fabsf(r2[1]))
        SWAP_ROWS(r3, r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        SWAP_ROWS(r2, r1);
    if (0.0f == r1[1])
        return FALSE;
    
    /* eliminate second variable */
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0.0f != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0.0f != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0.0f != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0.0f != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    
    /* choose pivot - or die */
    if (fabsf(r3[2]) > fabsf(r2[2]))
        SWAP_ROWS(r3, r2);
    if (0.0f == r2[2])
        return FALSE;
    
    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    
    /* last check */
    if (0.0f == r3[3])
        return FALSE;
    
    s = 1.0f / r3[3];		/* now back substitute row 3 */
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;
    
    m2 = r2[3];			/* now back substitute row 2 */
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
    
    m1 = r1[2];			/* now back substitute row 1 */
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
    
    m0 = r0[1];			/* now back substitute row 0 */
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
    
    MAT(out, 0, 0) = r0[4];
    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
    MAT(out, 3, 3) = r3[7];
    
    return TRUE;
    
#undef MAT
#undef SWAP_ROWS
}

/*
 * Compute inverse of 4x4 transformation matrix. Double-precision version.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
EDEN_BOOL EdenMathInvertMatrixd(double out[16], const double m[16])
{
    /* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]
    
    double wtmp[4][8];
    double m0, m1, m2, m3, s;
    double *r0, *r1, *r2, *r3;
    
    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
    
    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
    r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
    r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
    
    /* choose pivot - or die */
    if (fabs(r3[0]) > fabs(r2[0]))
        SWAP_ROWS(r3, r2);
    if (fabs(r2[0]) > fabs(r1[0]))
        SWAP_ROWS(r2, r1);
    if (fabs(r1[0]) > fabs(r0[0]))
        SWAP_ROWS(r1, r0);
    if (0.0 == r0[0])
        return FALSE;
    
    /* eliminate first variable     */
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0f) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    
    /* choose pivot - or die */
    if (fabs(r3[1]) > fabs(r2[1]))
        SWAP_ROWS(r3, r2);
    if (fabs(r2[1]) > fabs(r1[1]))
        SWAP_ROWS(r2, r1);
    if (0.0 == r1[1])
        return FALSE;
    
    /* eliminate second variable */
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0.0 != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0.0 != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0.0 != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0.0 != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    
    /* choose pivot - or die */
    if (fabs(r3[2]) > fabs(r2[2]))
        SWAP_ROWS(r3, r2);
    if (0.0 == r2[2])
        return FALSE;
    
    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    
    /* last check */
    if (0.0 == r3[3])
        return FALSE;
    
    s = 1.0 / r3[3];		/* now back substitute row 3 */
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;
    
    m2 = r2[3];			/* now back substitute row 2 */
    s = 1.0 / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
    
    m1 = r1[2];			/* now back substitute row 1 */
    s = 1.0 / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
    
    m0 = r0[1];			/* now back substitute row 0 */
    s = 1.0 / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
    
    MAT(out, 0, 0) = r0[4];
    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
    MAT(out, 3, 3) = r3[7];
    
    return TRUE;
    
#undef MAT
#undef SWAP_ROWS
}

void EdenMathMultMatrixByVector(float q[4], const float A[16], const float p[4])
{
#define M(NAME,row,col)  NAME[col*4+row]	// M allows us to use row-major notation.
    q[0] = M(A,0,0)*p[0] + M(A,0,1)*p[1] + M(A,0,2)*p[2] + M(A,0,3)*p[3]; 
    q[1] = M(A,1,0)*p[0] + M(A,1,1)*p[1] + M(A,1,2)*p[2] + M(A,1,3)*p[3]; 
    q[2] = M(A,2,0)*p[0] + M(A,2,1)*p[1] + M(A,2,2)*p[2] + M(A,2,3)*p[3]; 
    q[3] = M(A,3,0)*p[0] + M(A,3,1)*p[1] + M(A,3,2)*p[2] + M(A,3,3)*p[3]; 
#undef M
}

void EdenMathMultMatrixByVectord(double q[4], const double A[16], const double p[4])
{
#define M(NAME,row,col)  NAME[col*3+row]	// M allows us to use row-major notation.
    q[0] = M(A,0,0)*p[0] + M(A,0,1)*p[1] + M(A,0,2)*p[2] + M(A,0,3)*p[3]; 
    q[1] = M(A,1,0)*p[0] + M(A,1,1)*p[1] + M(A,1,2)*p[2] + M(A,1,3)*p[3]; 
    q[2] = M(A,2,0)*p[0] + M(A,2,1)*p[1] + M(A,2,2)*p[2] + M(A,2,3)*p[3]; 
    q[3] = M(A,3,0)*p[0] + M(A,3,1)*p[1] + M(A,3,2)*p[2] + M(A,3,3)*p[3]; 
#undef M
}

//
//	Creates a matrix which represents translation by a vector.
//	Input : x, y, z - components of the translation vector.
//	Output: mtx16 - a 4x4 matrix in column major form.
//
void EdenMathTranslationMatrix(float mtx16[16], const float x, const float y, const float z)
{
#define M(row,col)  mtx16[col*4+row]		// M allows us to use row-major notation.
	M(0,0) = M(1,1) = M(2,2) = M(3,3) = 1.0f;
	M(1,0) = M(2,0) = M(3,0) = M(0,1) = M(2,1) = M(3,1) = M(0,2) = M(1,2) = M(3,2) = 0.0f;
	M(0,3) = x;	M(1,3) = y; M(2,3) = z;
#undef M
}

void EdenMathTranslateMatrix(float B[16], const float A[16],
						 const float x, const float y, const float z)
{
	float T[16];

	EdenMathTranslationMatrix(T, x, y, z);
	EdenMathMultMatrix(B, A, T);	// Order is (result, current, transform)
}
//
//	Creates a matrix which represents the general case of a
//	rotation about an arbitrary axis.
//	Input : q - the angle of rotation measured in a right-hand sense, in radians.
//	        x,y,z - the components of the *normalised* non-zero vector representing the axis of rotation.
//	Output: mtx16 - a 4x4 matrix in column major form.
//
void EdenMathRotationMatrix(float mtx16[16], const float q, const float x, const float y, const float z)
{
    #define M(row,col)  mtx16[col*4+row]		// M allows us to use row-major notation.

    float ll, l, x0, y0, z0;
	float C, S, V;
	float xy, yz, xz;
	float Sx, Sy, Sz;
	float Vxy, Vyz, Vxz;
	
    if (q == 0.0f) return;
    
    ll = x*x + y*y + z*z;
    if (ll != 1.0f) {
        l = sqrtf(ll);
        if (!l) return;
        x0 = x / l;
        y0 = y / l;
        z0 = z / l;
    } else {
        x0 = x;
        y0 = y;
        z0 = z;
    }
    
	C = cosf(q);
	S = sinf(q);
	V = 1.0f - C;
	xy = x0*y0;
	yz = y0*z0;
	xz = x0*z0;
	Sx = S*x0;
	Sy = S*y0;
	Sz = S*z0;
	Vxy = V*xy;
	Vyz = V*yz;
	Vxz = V*xz;
	
    // Column 0.
	M(0, 0) = V*x0*x0 + C;
	M(1, 0) = Vxy + Sz;
	M(2, 0) = Vxz - Sy;
	M(3, 0) = 0.0f;

    // Column 1.
	M(0, 1) = Vxy - Sz;
	M(1, 1) = V*y0*y0 + C;
	M(2, 1) = Vyz + Sx;
	M(3, 1) = 0.0f;
    
    // Column 2.
	M(0, 2) = Vxz + Sy;
	M(1, 2) = Vyz - Sx;
	M(2, 2) = V*z0*z0 + C;
	M(3, 2) = 0.0f;
    
    // Column 3.
	M(0, 3) = 0.0f;
	M(1, 3) = 0.0f;
	M(2, 3) = 0.0f;
	M(3, 3) = 1.0f;
    
    #undef M
}

void EdenMathRotateMatrix(float B[16], const float A[16],
					  const float q, const float x, const float y, const float z)
{
	float T[16];

	EdenMathRotationMatrix(T, q, x, y, z);
	EdenMathMultMatrix(B, A, T);	// Order is (result, current, transform)
}

void EdenMathScalingMatrix(float mtx16[16], const float x, const float y, const float z)
{
    mtx16[0] = x; mtx16[1] = mtx16[2] = mtx16[3] = 0.0f; 
    mtx16[4] = 0.0f; mtx16[5] = y; mtx16[6] = mtx16[7] = 0.0f; 
    mtx16[8] = mtx16[9] = 0.0f; mtx16[10] = z; mtx16[11] = 0.0f; 
    mtx16[12] = mtx16[13] = mtx16[14] = 0.0f; mtx16[15] = 1.0f; 
}

void EdenMathScaleMatrix(float B[16], const float A[16], const float x, const float y, const float z)
{
    float T[16];
    
    EdenMathScalingMatrix(T, x, y, z);
	EdenMathMultMatrix(B, A, T);	// Order is (result, current, transform)
}

//
//  Creates a rotation matrix that rotates a vector called
//  "from" into another vector called "to".
//  Input : from[3], to[3] which both must be *normalized* non-zero vectors.
//  Output: mtx[3][3] -- a 3x3 matrix in column-major form.
//  Author: Tomas Moller, 1999
//
void EdenMathRotationMatrixFromTo(const float from[3], const float to[3], float mtx9[9])
{
    #define M(row,col)  mtx9[col*3+row]		// M allows us to use row-major notation.
    #define EPSILON 0.001f

    float v[3];
    float e, h;
    CROSS(v, from, to);
    e = DOT(from, to);
    if (e > 1.0f - EPSILON) {   // "from" almost or equal to "to"-vector?
		// Return identity.
        M(0, 0) = 1.0f; M(0, 1) = 0.0f; M(0, 2) = 0.0f; 
        M(1, 0) = 0.0f; M(1, 1) = 1.0f; M(1, 2) = 0.0f;
        M(2, 0) = 0.0f; M(2, 1) = 0.0f; M(2, 2) = 1.0f;
	} else if (e < -1.0f + EPSILON) { // "from" almost or equal to negated "to"-vector?
        float up[3],left[3];
        float invlen;
        float fxx,fyy,fzz,fxy,fxz,fyz;
        float uxx,uyy,uzz,uxy,uxz,uyz;
        float lxx,lyy,lzz,lxy,lxz,lyz;
        // left=CROSS(from, (1,0,0)).
        left[0] = 0.0f; left[1] = from[2]; left[2] = -from[1];
        if (DOT(left, left) < EPSILON) { // was left=CROSS(from,(1,0,0)) a good choice?
            // here we know that left = CROSS(from,(1,0,0)) will be a good choice.
            left[0] = -from[2]; left[1] = 0.0f; left[2] = from[0];
        }
        // Normalize "left".
        invlen = 1.0f/sqrt(DOT(left,left));
        left[0] *= invlen;
        left[1] *= invlen;
        left[2] *= invlen;
        CROSS(up,left,from);
        // Now we have a coordinate system, i.e., a basis;
        // M=(from, up, left), and we want to rotate to:
        // N=(-from, up, -left). This is done with the matrix:
        // N*M^T where M^T is the transpose of M.
        fxx = -from[0]*from[0]; fyy = -from[1]*from[1]; fzz = -from[2]*from[2];
        fxy = -from[0]*from[1]; fxz = -from[0]*from[2]; fyz = -from[1]*from[2];

        uxx = up[0]*up[0]; uyy = up[1]*up[1]; uzz = up[2]*up[2];
        uxy = up[0]*up[1]; uxz = up[0]*up[2]; uyz = up[1]*up[2];

        lxx =-left[0]*left[0]; lyy = -left[1]*left[1]; lzz = -left[2]*left[2];
        lxy =-left[0]*left[1]; lxz = -left[0]*left[2]; lyz = -left[1]*left[2];
        // Symmetric matrix.
        M(0, 0) = fxx+uxx+lxx; M(0, 1) = fxy+uxy+lxy; M(0, 2) = fxz+uxz+lxz;
        M(1, 0) = M(0, 1);     M(1, 1) = fyy+uyy+lyy; M(1, 2) = fyz+uyz+lyz;
        M(2, 0) = M(0, 2);     M(2, 1) = M(1, 2);     M(2, 2) = fzz+uzz+lzz;
    } else { // The most common case, unless "from"="to", or "from"=-"to".
        #if 0
        // Unoptimized version - a good compiler will optimize this.
        h = (1.0 - e)/DOT(v, v);
        M(0, 0) = e+h*v[0]*v[0];    M(0, 1) = h*v[0]*v[1]-v[2]; M(0,2) = h*v[0]*v[2]+v[1];
        M(1, 0) = h*v[0]*v[1]+v[2]; M(1, 1) = e+h*v[1]*v[1];    M(1,2) = h*v[1]*v[2]-v[0];
        M(2, 0) = h*v[0]*v[2]-v[1]; M(2, 1) = h*v[1]*v[2]+v[0]; M(2,2) = e+h*v[2]*v[2];
        #else
        // ...otherwise use this hand optimized version (9 mults less).
        float hvx,hvz,hvxy,hvxz,hvyz;
        h = (1.0f - e)/DOT(v, v);
        hvx = h*v[0];
        hvz = h*v[2];
        hvxy = hvx*v[1];
        hvxz = hvx*v[2];
        hvyz = hvz*v[1];
        M(0, 0) = e + hvx*v[0];	M(0, 1) = hvxy - v[2];		M(0, 2) = hvxz + v[1];
        M(1, 0) = hvxy + v[2];	M(1, 1) = e + h*v[1]*v[1];	M(1, 2) = hvyz - v[0];
        M(2, 0) = hvxz - v[1];	M(2, 1) = hvyz + v[0];		M(2, 2) = e + hvz*v[2];
        #endif
    }
    #undef M
}

void EdenMathRotatePointAboutAxis(float p2[3], const float p1[3], const float q, const float a[3])
{
	float C, S, V;
	float xy, yz, xz;
	float Sx, Sy, Sz;
	float Vxy, Vyz, Vxz;
	
	if (q) {
		C = cosf(q);
		S = sinf(q);
		V = 1.0f - C;
		xy = a[0] * a[1];
		yz = a[1] * a[2];
		xz = a[0] * a[2];
		Sx = S * a[0];
		Sy = S * a[1];
		Sz = S * a[2];
		Vxy = V * xy;
		Vyz = V * yz;
		Vxz = V * xz;
		
		p2[0] = (p1[0]*(C + V*a[0]*a[0]) + p1[1]*(Vxy - Sz)        + p1[2]*(Vxz + Sy)       ); 
		p2[1] = (p1[0]*(Vxy + Sz)        + p1[1]*(C + V*a[1]*a[1]) + p1[2]*(-Sx + Vyz)      ); 
		p2[2] = (p1[0]*(Vxz - Sy)        + p1[1]*(Sx + Vyz)        + p1[2]*(C + V*a[2]*a[2]));
	} else {
		p2[0] = p1[0];
		p2[1] = p1[1];
		p2[2] = p1[2];
	}
}

void EdenMathRotatePointAboutAxisd(double p2[3], const double p1[3], const double q, const double a[3])
{
	double C, S, V;
	double xy, yz, xz;
	double Sx, Sy, Sz;
	double Vxy, Vyz, Vxz;

	if (q) {
		C = cosf(q);
		S = sinf(q);
		V = 1.0 - C;
		xy = a[0] * a[1];
		yz = a[1] * a[2];
		xz = a[0] * a[2];
		Sx = S * a[0];
		Sy = S * a[1];
		Sz = S * a[2];
		Vxy = V * xy;
		Vyz = V * yz;
		Vxz = V * xz;
		
		p2[0] = (p1[0]*(C + V*a[0]*a[0]) + p1[1]*(Vxy - Sz)        + p1[2]*(Vxz + Sy)       ); 
		p2[1] = (p1[0]*(Vxy + Sz)        + p1[1]*(C + V*a[1]*a[1]) + p1[2]*(-Sx + Vyz)      ); 
		p2[2] = (p1[0]*(Vxz - Sy)        + p1[1]*(Sx + Vyz)        + p1[2]*(C + V*a[2]*a[2]));
	} else {
		p2[0] = p1[0];
		p2[1] = p1[1];
		p2[2] = p1[2];
	}
}

#ifdef __ppc__
#if !defined(__MWERKS__)
// Define __frsqrte() (unless using the Metrowerks compiler, 
// for which this is already available). This causes the frsqrte 
// instruction to be used to calculate a 5 bit estimate of the 
// reciprocal square root of the argument
inline double __frsqrte (double argument)
{
	double result;
	asm ( "frsqrte %0, %1" : /*OUT*/ "=f" ( result ) : /*IN*/ "f" ( argument ) );
	return result;
}
#endif // !__MWERKS

void fsqrt(double *arg)
{
	register double estimate;
	register double halfOfArg;
	int i;
	
	// Calculate a 5 bit starting estimate for the reciprocal sqrt
	estimate = __frsqrte (*arg);
	
	halfOfArg = 0.5 * *arg;
	
	//if you require less precision, you may reduce the number of loop iterations
	for (i = 0; i < 4; i++) {
		estimate = estimate * (1.5 - halfOfArg * estimate * estimate);
	}
	
	*arg = estimate * *arg;
}

void fsqrt3(double *arg1, double *arg2, double *arg3)
{
	register double estimate1, estimate2, estimate3;
	register double halfOfArg1, halfOfArg2, halfOfArg3;
	int i;
	
	//Calculate a 5 bit starting estimate for the reciprocal sqrt of each
	estimate1 = __frsqrte (*arg1);
	estimate2 = __frsqrte (*arg2);
	estimate3 = __frsqrte (*arg3);
	
	halfOfArg1 = 0.5 * *arg1;
	halfOfArg2 = 0.5 * *arg2;
	halfOfArg3 = 0.5 * *arg3;
	
	//if you require less precision, you may reduce the number of loop iterations
	for (i = 0; i < 4; i++) {
		estimate1 = estimate1 * (1.5 - halfOfArg1 * estimate1 * estimate1);
		estimate2 = estimate2 * (1.5 - halfOfArg2 * estimate2 * estimate2);
		estimate3 = estimate3 * (1.5 - halfOfArg3 * estimate3 * estimate3);
	}
		
	*arg1 = estimate1 * *arg1;
	*arg2 = estimate2 * *arg2;
	*arg3 = estimate3 * *arg3;
}

void frsqrt(double *arg)
{
	register double estimate;
	int i;
	
	//Calculate a 5 bit starting estimate for the reciprocal sqrt
	estimate = __frsqrte(*arg);
	
	//if you require less precision, you may reduce the number of loop iterations
	for (i = 0; i < 4; i++) {
		estimate = estimate + 0.5 * estimate * (1.0 - *arg * estimate * estimate);
	}
	
	*arg = estimate;
}

// Caculate three reciprocal square roots simultaneously: (*arg = (*arg)-0.5).
void frsqrt3(double *arg1, double *arg2, double *arg3)
{
	register double estimate1, estimate2, estimate3;
	int i;
	
	//Calculate a 5 bit starting estimate for the reciprocal sqrt of each
	estimate1 = __frsqrte(*arg1); 
	estimate2 = __frsqrte(*arg2);
	estimate3 = __frsqrte(*arg3);
	
	//if you require less precision, you may reduce the number of loop iterations
	for (i = 0; i < 4; i++) {
		estimate1 = estimate1 + 0.5 * estimate1 * (1.0 - *arg1 * estimate1 * estimate1);
		estimate2 = estimate2 + 0.5 * estimate2 * (1.0 - *arg2 * estimate2 * estimate2);
		estimate3 = estimate3 + 0.5 * estimate3 * (1.0 - *arg3 * estimate3 * estimate3);
	}
	
	*arg1 = estimate1;
	*arg2 = estimate2;
	*arg3 = estimate3;
}
#endif // __ppc__
