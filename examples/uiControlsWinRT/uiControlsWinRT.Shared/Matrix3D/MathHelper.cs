#region Header
//
//   Project:           Matrix3DEx - Silverlight Matrix3D extensions
//   Description:       Static math helper methods.
//
//   Changed by:        $Author: unknown $
//   Changed on:        $Date: 2014-09-22 10:06:13 +0200 (Mo, 22 Sep 2014) $
//   Changed in:        $Revision: 73018 $
//   Project:           $URL: https://matrix3dex.svn.codeplex.com/svn/trunk/Source/Matrix3DEx/MathHelper.cs $
//   Id:                $Id: MathHelper.cs 73018 2014-09-22 08:06:13Z unknown $
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
   /// Math helper methods.
   /// </summary>
   public static class MathHelper
   {
      #region Angle conversion

      /// <summary>
      /// Converts a radian into a degreee value.
      /// </summary>
      /// <param name="radians">An angle in rad.</param>
      /// <returns>The angle converted to degress.</returns>
      public static double ToDegrees(double radians)
      {
         return radians * 57.295779513082320876798154814105;
      }

      /// <summary>
      /// Converts a degree into a radian value.
      /// </summary>
      /// <param name="degrees">A angle in deg.</param>
      /// <returns>The angle converted to radians.</returns>
      public static double ToRadians(double degrees)
      {
         return degrees * 0.017453292519943295769236907684886;
      }

      #endregion

      #region Vector algebra

      internal static double VectorLength(double x, double y, double z)
      {
         return Math.Sqrt(x * x + y * y + z * z);
      }

      internal static void VectorNormalize(ref double x, ref double y, ref double z)
      {
         double il = 1 / VectorLength(x, y, z);
         x *= il;
         y *= il;
         z *= il;
      }

      internal static double VectorDot(double v1x, double v1y, double v1z, double v2x, double v2y, double v2z)
      {
         return v1x * v2x + v1y * v2y + v1z * v2z;
      }

      internal static void VectorCross(double v1x, double v1y, double v1z, double v2x, double v2y, double v2z, out double cx, out double cy, out double cz)
      {
         cx = v1y * v2z - v2y * v1z;
         cy = v1z * v2x - v2z * v1x;
         cz = v1x * v2y - v2x * v1y;
      }

      #endregion
   }
}
