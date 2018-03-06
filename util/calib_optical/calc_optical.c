/*
 *  calc_optical.c
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
 *  Copyright 2007-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "calc_optical.h"

#include <math.h>
#include <stdio.h>	// printf()
#include <stdlib.h> // malloc()
#include <AR/ar.h> // paramDispExt()

/*!
@defined DTOR
 @abstract Convert degrees to radians.
 @discussion
 Multiply an angle in degrees by this constant to
 get the value of the angle in radians.
 */
#define DTOR            0.0174532925199432957692369076848861

/*!
@defined RTOD
 @abstract Convert radians to degrees.
 @discussion
 Multiply an angle in radians by this constant to
 get the value of the angle in degrees.
 */
#define RTOD            57.2957795130823208767981548141052

/*!
@defined MIN
	@abstract Determine minimum of two values.
 */
#ifndef MIN
#  define MIN(x,y) (x < y ? x : y)
#endif

/*!
@defined MAX
	@abstract Determine maximum of two values.
 */
#ifndef MAX
#  define MAX(x,y) (x > y ? x : y)
#endif

/*!
@defined EPSILON
 @abstract A smallish number.
 @discussion
 */
#ifdef ARDOUBLE_IS_FLOAT
#  define EPSILON	0.001f
#else
#  define EPSILON	0.001
#endif

/*!
@defined CROSS
 @abstract Vector cross-product in R3.
 @discussion
 6 multiplies + 3 subtracts.
 Vector cross product calculates a vector with direction
 orthogonal to plane formed by the other two vectors, and length
 equal to the area of the parallelogram formed by the other two
 vectors.
 Right hand rule for vector cross products: Point thumb of right
 hand in direction of v1, fingers together in direction of v2,
 then palm faces in the direction of dest.
 */
#define CROSS(dest,v1,v2) {dest[0] = v1[1]*v2[2] - v1[2]*v2[1]; dest[1] = v1[2]*v2[0] - v1[0]*v2[2]; dest[2] = v1[0]*v2[1] - v1[1]*v2[0];}

/*!
@defined LENGTH
 @abstract   (description)
 @discussion (description)
 */
#define LENGTH(v) (sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]))

/*!
@defined DOT
 @abstract Vector dot-product in R3.
 @discussion
 3 multiplies + 2 adds.
 */
#define DOT(v1,v2) ((v1[0]*v2[0]) + (v1[1]*v2[1]) + (v1[2]*v2[2]))

/*!
@defined ADD
 @abstract   (description)
 @discussion (description)
 */
#define ADD(dest,v1,v2) {dest[0] = v1[0] + v2[0]; dest[1] = v1[1] + v2[1]; dest[2] = v1[2] + v2[2];}

/*!
@defined SUB
 @abstract   (description)
 @discussion (description)
 */
#define SUB(dest,v1,v2) {dest[0] = v1[0] - v2[0]; dest[1] = v1[1] - v2[1]; dest[2] = v1[2] - v2[2];}

/*!
@defined AVERAGE
	@abstract   (description)
	@discussion (description)
 */
#define AVERAGE(dest,v1,v2) {dest[0] = (v1[0] + v2[0])*0.5; dest[1] = (v1[1] + v2[1])*0.5; dest[2] = (v1[2] + v2[2])*0.5;}

/*!
@defined COPY
	@abstract   (description)
	@discussion (description)
 */
#define COPY(dest, v) {dest[0] = v[0]; dest[1] = v[1]; dest[2] = v[2];}

#ifdef ARDOUBLE_IS_FLOAT
#  define FABS fabsf
#else
#  define FABS fabs
#endif

ARdouble MathNormalise(ARdouble v[3])
{
    ARdouble l;
    
    l = (ARdouble)LENGTH(v);
	if (l) {
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
	return (l);
}

/*
 
 Calculate the line segment PaPb that is the shortest route between
 two lines P1P2 and P3P4. Calculate also the values of mua and mub where
 Pa = P1 + mua (P2 - P1)
 Pb = P3 + mub (P4 - P3)
 Returns -1 if no solution exists.
 
 http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline3d/
 
 Two lines in 3 dimensions generally don't intersect at a point,
 they may be parallel (no intersections) or they may be coincident
 (infinite intersections) but most often only their projection onto
 a plane intersect.. When they don't exactly intersect at a point
 they can be connected by a line segment, the shortest line segment
 is unique and is often considered to be their intersection in 3D.
 
 The following computes this shortest line segment that joins two
 lines in 3D, it will as a biproduct identify parrallel lines. In
 what follows a line will be defined by two points lying on it, a
 point on line "a" defined by points P1 and P2 has an equation
	Pa = P1 + mua (P2 - P1)
 similarly a point on a second line "b" defined by points P4 and P4
 will be written as
	Pb = P3 + mub (P4 - P3)
 The values of mua and mub range from negative to positive infinity.
 The line segments between P1 P2 and P3 P4 have their corresponding
 mu between 0 and 1.
 
 */
int LineLineIntersect( const ARdouble p1[3], const ARdouble p2[3], const ARdouble p3[3], const ARdouble p4[3],
					   ARdouble pa[3], ARdouble pb[3], ARdouble *mua, ARdouble *mub)
{
	double p13[3], p43[3], p21[3];
	double d1343, d4321, d1321, d4343, d2121;
	double numer, denom;
	
	SUB(p43, p4, p3);
	if (LENGTH(p43) < EPSILON) return(-1); // Points defining the line are too close together.
	SUB(p21, p2, p1);
	if (LENGTH(p21) < EPSILON) return(-1); // Points defining the line are too close together.
	SUB(p13, p1, p3);
	
	d1343 = DOT(p13, p43);
	d4321 = DOT(p43, p21);
	d1321 = DOT(p13, p21);
	d4343 = DOT(p43, p43);
	d2121 = DOT(p21, p21);
	
	denom = d2121 * d4343 - d4321 * d4321;
	if (FABS(denom) < EPSILON) return (-1); // Lines are too close to parallel.
	numer = d1343 * d4321 - d1321 * d4343;
	
	*mua = numer / denom;
	*mub = (d1343 + d4321 * (*mua)) / d4343;
	
	pa[0] = p1[0] + *mua * p21[0];
	pa[1] = p1[1] + *mua * p21[1];
	pa[2] = p1[2] + *mua * p21[2];
	pb[0] = p3[0] + *mub * p43[0];
	pb[1] = p3[1] + *mub * p43[1];
	pb[2] = p3[2] + *mub * p43[2];
	
	return (0);
}

int calc_optical(ARdouble global[][3], ARdouble IdealScreen[][2], const int numPoints,
				 ARdouble *fovy, ARdouble *aspect, ARdouble m[16])
{
	int numLines, numLineIntersections, index, i, j;
	ARdouble pa[3], pb[3], pba[3], mua, mub;
	ARdouble eye[3], eyeError;
	ARdouble *lineIntersections;
	ARdouble p_LL[3], p_LR[3], /*p_C[3],*/ p_UL[3], p_UR[3];
	ARdouble right_topedge[3], right_bottomedge[3], up_leftedge[3], up_rightedge[3], right[3], up[3], normal[3];
	ARdouble fovx;
	
	if (numPoints % 2 != 0) {
		ARLOGe("calc_optical(): Error: matched pairs of points required.\n");
		return (-1);
	}
	numLines = numPoints / 2;
	numLineIntersections = numLines * (numLines - 1) / 2;
	lineIntersections = (ARdouble *)malloc(numLineIntersections * sizeof(ARdouble) * 3);
	
	// For every pair of lines, calculate the point in 3-space which is the minimum distance from
	// both lines in the pair.
	index = 0;
	eyeError = 0.0;
	for (i = 0; i < (numLines - 1); i++) {
		for (j = i + 1; j < numLines; j++) {
			if (LineLineIntersect(global[i*2], global[i*2 + 1],
								  global[j*2], global[j*2 + 1],
								  pa, pb, &mua, &mub) < 0) {
				ARLOGe("calc_optical(): Error: bad line data. (Near and far points too close together, or lines do not converge.)\n");
				goto bail;
			}
			AVERAGE((&(lineIntersections[index * 3])), pa, pb);
			SUB(pba, pb, pa);
			eyeError += LENGTH(pba);
			index++;
		}
	}
    eyeError /= (numLines * (numLines - 1) / 2);

	// Now average all those points to find eyepoint in 3-space.
	eye[0] = eye[1] = eye[2] = 0.0;
	for (i = 0; i < numLineIntersections; i++) {
		eye[0] += lineIntersections[i*3];
		eye[1] += lineIntersections[i*3 + 1];
		eye[2] += lineIntersections[i*3 + 2];
	}
	eye[0] /= numLineIntersections;
	eye[1] /= numLineIntersections;
	eye[2] /= numLineIntersections;
    
    // ARToolKit expresses marker pose in a coordinate system where camera right is +x,
    // camera up is -y and camera forward is +z. We want OpenGL-style coordinates where
    // right is +x, up is -y, and forward is -z. So negate y and z coords to get things
    // in OpenGL-style.
	eye[1] = -eye[1]; eye[2] = -eye[2];
	ARLOG("Expressed relative to camera axes, eye is %.1f mm to the %s, %.1f mm %s, and %.1f mm %s the camera.\n",
          FABS(eye[0]), (eye[0] >= 0.0 ? "right" : "left"),
          FABS(eye[1]), (eye[1] >= 0.0 ? "above" : "below"),
          FABS(eye[2]), (eye[2] >= 0.0 ? "behind" : "in front of"));
	ARLOG("Eyepoint error is +/- %5f.\n", eyeError);

	// Now calculate the axes of the viewer.
	// For now, just use the fixed ordering of the crosshairs
	// to know which corner they refer to.
	// A more general method would be good.
	SUB(p_LL, global[0], global[1]); MathNormalise(p_LL); // => a unit vector from the eye towards lower-left crosshairs.
	SUB(p_LR, global[2], global[3]); MathNormalise(p_LR); // => a unit vector from the eye towards lower-right crosshairs.
	//SUB(p_C,  global[4], global[5]); MathNormalise(p_C);
	SUB(p_UL, global[6], global[7]); MathNormalise(p_UL); // => a unit vector from the eye towards upper-left crosshairs.
	SUB(p_UR, global[8], global[9]); MathNormalise(p_UR); // => a unit vector from the eye towards upper-right crosshairs.
	
	SUB(right_topedge, p_UR, p_UL);
	SUB(right_bottomedge, p_LR, p_LL);
	SUB(up_leftedge, p_UL, p_LL);
	SUB(up_rightedge, p_UR, p_LR);
	AVERAGE(right, right_bottomedge, right_topedge); 
	AVERAGE(up, up_leftedge, up_rightedge);
	MathNormalise(right);
	MathNormalise(up);
	CROSS(normal, right, up);
	CROSS(up, normal, right); // Ensure that the coordinate set is exactly orthogonal.
    // ARToolKit expresses marker pose in a coordinate system where camera right is +x,
    // camera up is -y and camera forward is +z. We want OpenGL-style coordinates where
    // right is +x, up is -y, and forward is -z. So negate y and z coords to get things
    // in OpenGL-style.
    right[1] = -right[1]; right[2] = -right[2];
    up[1] = -up[1]; up[2] = -up[2];
    normal[1] = -normal[1]; normal[2] = -normal[2]; // Note that normal is opposite direction to camera forward.
    
	// Calculate field of view, by working out the angles between the vectors.
	// Remember, the angle between the vectors is only HALF as wide as the
	// field of view (because of where the crosshairs were positioned), so we
	// don't need to divide by two when averaging the upper angle and the lower angle.
	// fov is in radians.
	*fovy = RTOD*(acos(DOT(p_UL, p_LL)) + acos(DOT(p_UR, p_LR)));
	fovx = RTOD*(acos(DOT(p_UL, p_UR)) + acos(DOT(p_LL, p_LR)));
	*aspect = fovx / *fovy;
	
    // This transform expresses the position and orientation of the eye in
    // camera coordinates.
	//m[0] = right[0]; m[4] = up[0]; m[ 8] = normal[0]; m[12] = eye[0];
	//m[1] = right[1]; m[5] = up[1]; m[ 9] = normal[1]; m[13] = eye[1];
	//m[2] = right[2]; m[6] = up[2]; m[10] = normal[2]; m[14] = eye[2];
	//m[3] = 0.0;      m[7] = 0.0;   m[11] = 0.0;       m[15] = 1.0;

	// We want a transformation which when loaded as modelview matrix, starts in eye
	// coordinates, moves to camera position, and orients to camera.
	// This is the inverse of [right up normal eye]
	//                        [0     0  0      1  ].
	// The inverse of an HCT matrix [  R   p] is [  RT  -RT.p]
	//                              [0 0 0 1]    [0 0 0   1  ].
	m[0] = right[0];  m[4] = right[1];  m[ 8] = right[2];  m[12] = -eye[0]*right[0]  - eye[1]*right[1]  - eye[2]*right[2];
	m[1] = up[0];     m[5] = up[1];     m[ 9] = up[2];     m[13] = -eye[0]*up[0]     - eye[1]*up[1]     - eye[2]*up[2];
	m[2] = normal[0]; m[6] = normal[1]; m[10] = normal[2]; m[14] = -eye[0]*normal[0] - eye[1]*normal[1] - eye[2]*normal[2];
	m[3] = 0.0;       m[7] = 0.0;       m[11] = 0.0;       m[15] = 1.0;

	ARLOG("Expressed relative to eye axes, camera is %.1f mm to the %s, %.1f mm %s, and %.1f mm %s the eye.\n",
          FABS(m[12]), (m[12] >= 0.0 ? "right" : "left"),
          FABS(m[13]), (m[13] >= 0.0 ? "above" : "below"),
          FABS(m[14]), (m[14] >= 0.0 ? "behind" : "in front of"));

	arParamDispOptical(*fovy, *aspect, m);
	
	free(lineIntersections);
	return (0);
	
bail:
	free(lineIntersections);
	return (-1);
}
