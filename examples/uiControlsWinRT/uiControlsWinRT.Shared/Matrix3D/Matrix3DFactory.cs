#region Header
//
//   Project:           Matrix3DEx - Silverlight Matrix3D extensions
//   Description:       Matrix3D factory methods for common transformations.
//
//   Changed by:        $Author: unknown $
//   Changed on:        $Date: 2014-09-22 10:06:13 +0200 (Mo, 22 Sep 2014) $
//   Changed in:        $Revision: 73018 $
//   Project:           $URL: https://matrix3dex.svn.codeplex.com/svn/trunk/Source/Matrix3DEx/Matrix3DFactory.cs $
//   Id:                $Id: Matrix3DFactory.cs 73018 2014-09-22 08:06:13Z unknown $
//
//
//   Copyright (c) 2009-2015 Rene Schulte
//
//
#endregion

using System;

#if SILVERLIGHT
namespace System.Windows.Media.Media3D
#else
namespace Windows.UI.Xaml.Media.Media3D
#endif
{
   /// <summary>
   /// Matrix3D factory methods for common transformations.
   /// </summary>
   public static class Matrix3DFactory
   {
      #region World transformations

      /// <summary>
      /// Creates a translation matrix.
      /// </summary>
      /// <param name="x">The offset along the x-axis.</param>
      /// <param name="y">The offset along the y-axis..</param>
      /// <param name="z">The offset along the z-axis.</param>
      /// <returns>A new translation matrix.</returns>
      public static Matrix3D CreateTranslation(double x, double y, double z)
      {
         return new Matrix3D( 1, 0, 0, 0,
                              0, 1, 0, 0,
                              0, 0, 1, 0,
                              x, y, z, 1);
      }

      /// <summary>
      /// Creates a scaling matrix.
      /// </summary>
      /// <param name="x">The scaling factor along the x-axis.</param>
      /// <param name="y">The scaling factor along the y-axis.</param>
      /// <param name="z">The scaling factor along the z-axis.</param>
      /// <returns>A new scaling matrix.</returns>
      public static Matrix3D CreateScale(double x, double y, double z)
      {
         return new Matrix3D( x, 0, 0, 0,
                              0, y, 0, 0,
                              0, 0, z, 0,
                              0, 0, 0, 1);
      }

      /// <summary>
      /// Creates a uniform scaling matrix.
      /// </summary>
      /// <param name="xyz">The scaling factor along the x-, the y- and the z-axis.</param>
      /// <returns>A new scaling matrix.</returns>
      public static Matrix3D CreateScale(double xyz)
      {
         return CreateScale(xyz, xyz, xyz);
      }

      /// <summary>
      /// Creates a matrix that rotates around the x-axis.
      /// </summary>
      /// <param name="radians">The rotation angle in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <returns>A new rotation matrix.</returns>
      public static Matrix3D CreateRotationX(double radians)
      {
         double s = Math.Sin(radians);
         double c = Math.Cos(radians);

         return new Matrix3D( 1,  0, 0, 0,
                              0,  c, s, 0,
                              0, -s, c, 0,
                              0,  0, 0, 1);
      }

      /// <summary>
      /// Creates a matrix that rotates around the y-axis.
      /// </summary>
      /// <param name="radians">The rotation angle in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <returns>A new rotation matrix.</returns>
      public static Matrix3D CreateRotationY(double radians)
      {
         double s = Math.Sin(radians);
         double c = Math.Cos(radians);

         return new Matrix3D( c, 0, -s, 0,
                              0, 1,  0, 0,
                              s, 0,  c, 0,
                              0, 0,  0, 1);
      }

      /// <summary>
      /// Creates a matrix that rotates around the z-axis.
      /// </summary>
      /// <param name="radians">The rotation angle in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <returns>A new rotation matrix.</returns>
      public static Matrix3D CreateRotationZ(double radians)
      {
         double s = Math.Sin(radians);
         double c = Math.Cos(radians);

         return new Matrix3D(  c, s, 0, 0,
                              -s, c, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1);
      }

      /// <summary>
      /// Creates a matrix that rotates around any axis.
      /// </summary>
      /// <param name="radians">The rotation angle in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <param name="axisX">The x-coordinate of the rotation axis.</param>
      /// <param name="axisY">The y-coordinate of the rotation axis.</param>
      /// <param name="axisZ">The z-coordinate of the rotation axis.</param>
      /// <returns>A new rotation matrix.</returns>
      public static Matrix3D CreateRotationAnyAxis(double radians, double axisX, double axisY, double axisZ)
      {
         double s = Math.Sin(-radians);
         double c = Math.Cos(-radians);
         double c1 = 1 - c;

         MathHelper.VectorNormalize(ref axisX, ref axisY, ref axisZ);
         var m = new Matrix3D();

         m.M11 = (axisX * axisX) * c1 + c;
         m.M12 = (axisX * axisY) * c1 - (axisZ * s);
         m.M13 = (axisX * axisZ) * c1 + (axisY * s);
         m.M14 = 0.0f;

         m.M21 = (axisY * axisX) * c1 + (axisZ * s);
         m.M22 = (axisY * axisY) * c1 + c;
         m.M23 = (axisY * axisZ) * c1 - (axisX * s);
         m.M24 = 0.0f;

         m.M31 = (axisZ * axisX) * c1 - (axisY * s);
         m.M32 = (axisZ * axisY) * c1 + (axisX * s);
         m.M33 = (axisZ * axisZ) * c1 + c;
         m.M34 = 0.0f;

         m.OffsetX = 0.0f;
         m.OffsetY = 0.0f;
         m.OffsetZ = 0.0f;
         m.M44     = 1.0f;

         return m;
      }

      #endregion

      #region Camera transformation

      #region Left-handed

      /// <summary>
      /// Creates a left-handed perspective projection matrix.
      /// </summary>
      /// <param name="fieldOfView">The Field Of View in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <param name="aspectRatio">The aspect ratio (width divided by height: w/h).</param>
      /// <param name="near">The z-coordinate of the near view-plane.</param>
      /// <param name="far">The z-coordinate of the far view-plane.</param>
      /// <returns>A new projection matrix.</returns>
      public static Matrix3D CreatePerspectiveFieldOfViewLH(double fieldOfView, double aspectRatio, double near, double far)
      {    
         double h = 1.0 / Math.Tan(fieldOfView * 0.5);
         double w = h / aspectRatio;
         double d = far - near;
         double sz = far / d;
         double z = -near * far / d;

         return new Matrix3D( w, 0,  0, 0,
                              0, h,  0, 0,
                              0, 0, sz, 1,
                              0, 0,  z, 1);
      }

      /// <summary>
      /// Creates a left-handed orthographic projection matrix.
      /// </summary>
      /// <param name="width">The width of the view volume.</param>
      /// <param name="height">the height of the view volume.</param>
      /// <param name="near">The minimum z-coordinate of the view volume.</param>
      /// <param name="far">The maximum z-coordinate of the view volume.</param>
      /// <returns>A new projection matrix.</returns>
      public static Matrix3D CreateOrthographicLH(double width, double height, double near, double far)
      {
         double hw = 2.0 / width;
         double hh = 2.0 / height;
         double id = 1.0 / (far - near);
         double nid = near / (near - far);

         return new Matrix3D(hw,  0,   0, 0,
                              0, hh,   0, 0,
                              0,  0,  id, 0,
                              0,  0, nid, 1);
      }

      /// <summary>
      /// Creates a left-handed look-at matrix (camera).
      /// </summary>
      /// <param name="eyePosX">The x-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosY">The y-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosZ">The z-coordinate of the viewer (camera) position.</param>
      /// <param name="targetX">The x-coordinate of the target (look-at).</param>
      /// <param name="targetY">The y-coordinate of the target (look-at).</param>
      /// <param name="targetZ">The z-coordinate of the target (look-at).</param>
      /// <param name="upX">The x-coordinate of the up vector.</param>
      /// <param name="upY">The y-coordinate of the up vector.</param>
      /// <param name="upZ">The z-coordinate of the up vector.</param>
      /// <returns>A new look-at matrix.</returns>
      public static Matrix3D CreateLookAtLH(double eyePosX, double eyePosY, double eyePosZ, double targetX, double targetY, double targetZ, double upX, double upY, double upZ)
      {
         // Z axis
         double zx = targetX - eyePosX;
         double zy = targetY - eyePosY;
         double zz = targetZ - eyePosZ;
         MathHelper.VectorNormalize(ref zx, ref zy, ref zz);

         // X axis
         double xx, xy, xz;
         MathHelper.VectorCross(upX, upY, upZ, zx, zy, zz, out xx, out xy, out xz);
         MathHelper.VectorNormalize(ref xx, ref xy, ref xz);
               
         // Y axis
         double yx, yy, yz;
         MathHelper.VectorCross(zx, zy, zz, xx, xy, xz, out yx, out yy, out yz);

         // Eye angles
         double ex = -MathHelper.VectorDot(xx, xy, xz, eyePosX, eyePosY, eyePosZ);
         double ey = -MathHelper.VectorDot(yx, yy, yz, eyePosX, eyePosY, eyePosZ);
         double ez = -MathHelper.VectorDot(zx, zy, zz, eyePosX, eyePosY, eyePosZ);

         return new Matrix3D(xx, yx, zx, 0,
                             xy, yy, zy, 0,
                             xz, yz, zz, 0,
                             ex, ey, ez, 1);
      }

      /// <summary>
      /// Creates a left-handed look-at matrix (camera) using the deafult up vector (0, 1, 0).
      /// </summary>
      /// <param name="eyePosX">The x-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosY">The y-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosZ">The z-coordinate of the viewer (camera) position.</param>
      /// <param name="targetX">The x-coordinate of the target (look-at).</param>
      /// <param name="targetY">The y-coordinate of the target (look-at).</param>
      /// <param name="targetZ">The z-coordinate of the target (look-at).</param>
      /// <returns>A new look-at matrix.</returns>
      public static Matrix3D CreateLookAtLH(double eyePosX, double eyePosY, double eyePosZ, double targetX, double targetY, double targetZ)
      {
         return CreateLookAtLH(eyePosX, eyePosY, eyePosZ, targetX, targetY, targetZ, 0, 1, 0);
      }

      #endregion

      #region Right-handed

      /// <summary>
      /// Creates a right-handed perspective projection matrix.
      /// </summary>
      /// <param name="fieldOfView">The Field Of View in radians. See <see cref="MathHelper.ToRadians"/> for a conversion method.</param>
      /// <param name="aspectRatio">The aspect ratio (width divided by height: w/h).</param>
      /// <param name="near">The z-coordinate of the near view-plane.</param>
      /// <param name="far">The z-coordinate of the far view-plane.</param>
      /// <returns>A new projection matrix.</returns>
      public static Matrix3D CreatePerspectiveFieldOfViewRH(double fieldOfView, double aspectRatio, double near, double far)
      {
         double h = 1.0 / Math.Tan(fieldOfView * 0.5);
         double w = h / aspectRatio;
         double d = near - far;
         double sz = far / d;
         double z = near * far / d;

         return new Matrix3D( w, 0,  0,  0,
                              0, h,  0,  0,
                              0, 0, sz, -1,
                              0, 0,  z,  1);
      }

      /// <summary>
      /// Creates a right-handed orthographic projection matrix.
      /// </summary>
      /// <param name="width">The width of the view volume.</param>
      /// <param name="height">the height of the view volume.</param>
      /// <param name="near">The minimum z-coordinate of the view volume.</param>
      /// <param name="far">The maximum z-coordinate of the view volume.</param>
      /// <returns>A new projection matrix.</returns>
      public static Matrix3D CreateOrthographicRH(double width, double height, double near, double far)
      {
         double hw = 2.0 / width;
         double hh = 2.0 / height;
         double id = 1.0 / (near - far);
         double nid = near * id;

         return new Matrix3D(hw,  0,   0, 0,
                              0, hh,   0, 0,
                              0,  0,  id, 0,
                              0,  0, nid, 1);
      }

      /// <summary>
      /// Creates a right-handed look-at matrix (camera).
      /// </summary>
      /// <param name="eyePosX">The x-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosY">The y-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosZ">The z-coordinate of the viewer (camera) position.</param>
      /// <param name="targetX">The x-coordinate of the target (look-at).</param>
      /// <param name="targetY">The y-coordinate of the target (look-at).</param>
      /// <param name="targetZ">The z-coordinate of the target (look-at).</param>
      /// <param name="upX">The x-coordinate of the up vector.</param>
      /// <param name="upY">The y-coordinate of the up vector.</param>
      /// <param name="upZ">The z-coordinate of the up vector.</param>
      /// <returns>A new look-at matrix.</returns>
      public static Matrix3D CreateLookAtRH(double eyePosX, double eyePosY, double eyePosZ, double targetX, double targetY, double targetZ, double upX, double upY, double upZ)
      {
         // Z axis
         double zx = eyePosX - targetX;
         double zy = eyePosY - targetY;
         double zz = eyePosZ - targetZ;
         MathHelper.VectorNormalize(ref zx, ref zy, ref zz);

         // X axis
         double xx, xy, xz;
         MathHelper.VectorCross(upX, upY, upZ, zx, zy, zz, out xx, out xy, out xz);
         MathHelper.VectorNormalize(ref xx, ref xy, ref xz);
               
         // Y axis
         double yx, yy, yz;
         MathHelper.VectorCross(zx, zy, zz, xx, xy, xz, out yx, out yy, out yz);

         // Eye angles
         double ex = -MathHelper.VectorDot(xx, xy, xz, eyePosX, eyePosY, eyePosZ);
         double ey = -MathHelper.VectorDot(yx, yy, yz, eyePosX, eyePosY, eyePosZ);
         double ez = -MathHelper.VectorDot(zx, zy, zz, eyePosX, eyePosY, eyePosZ);

         return new Matrix3D(xx, yx, zx, 0,
                             xy, yy, zy, 0,
                             xz, yz, zz, 0,
                             ex, ey, ez, 1);
      }

      /// <summary>
      /// Creates a right-handed look-at matrix (camera) using the deafult up vector (0, 1, 0).
      /// </summary>
      /// <param name="eyePosX">The x-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosY">The y-coordinate of the viewer (camera) position.</param>
      /// <param name="eyePosZ">The z-coordinate of the viewer (camera) position.</param>
      /// <param name="targetX">The x-coordinate of the target (look-at).</param>
      /// <param name="targetY">The y-coordinate of the target (look-at).</param>
      /// <param name="targetZ">The z-coordinate of the target (look-at).</param>
      /// <returns>A new look-at matrix.</returns>
      public static Matrix3D CreateLookAtRH(double eyePosX, double eyePosY, double eyePosZ, double targetX, double targetY, double targetZ)
      {
         return CreateLookAtRH(eyePosX, eyePosY, eyePosZ, targetX, targetY, targetZ, 0, 1, 0);
      }

      #endregion

      /// <summary>
      /// Creates a combined transformation which could be used for the Projection of an UIElement. 
      /// The arguments are multiplied in that order.
      /// </summary>
      /// <param name="world">The world matrix.</param>
      /// <param name="lookAt">The camera (look-at) matrix.</param>
      /// <param name="projection">The projection matrix.</param>
      /// <param name="viewport">The final viewport transformation.</param>
      /// <returns>A new matrix combined matrix.</returns>
      public static Matrix3D CreateViewportProjection(Matrix3D world, Matrix3D lookAt, Matrix3D projection, Matrix3D viewport)
      {
         return world * lookAt * projection * viewport;
      }

      /// <summary>
      /// Creates a viewport transformation matrix.
      /// </summary>
      /// <param name="width">The width of the viewport in screen space.</param>
      /// <param name="height">The height of the viewport in screen space.</param>
      /// <returns>A new viewport transformation matrix.</returns>
      public static Matrix3D CreateViewportTransformation(double width, double height)
      {
         double wh = width * 0.5;
         double hh = height * 0.5;

         return new Matrix3D(wh,   0, 0, 0,
                              0, -hh, 0, 0,
                              0,   0, 1, 0,
                             wh,  hh, 0, 1);
      }

      #endregion
   }
}
