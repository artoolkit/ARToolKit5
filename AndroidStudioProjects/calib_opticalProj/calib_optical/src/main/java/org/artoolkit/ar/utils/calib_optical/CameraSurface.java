/*
 *  CameraSurface.java
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
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

package org.artoolkit.ar.utils.calib_optical;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.os.Build;
import android.preference.PreferenceManager;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

import java.io.IOException;

public class CameraSurface extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    private static final String TAG = "CameraSurface";
    private Camera camera;

    private boolean mustAskPermissionFirst = false;
    public boolean gettingCameraAccessPermissionsFromUser()
    {
        return mustAskPermissionFirst;
    }

    public void resetGettingCameraAccessPermissionsFromUserState()
    {
        mustAskPermissionFirst = false;
    }

    @SuppressWarnings("deprecation")
    public CameraSurface(Context context) {
        super(context);

        Log.i(TAG, "CameraSurface(): ctor called");
        Activity activityRef = (Activity)context;

        try
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
            {
                if (PackageManager.PERMISSION_GRANTED != ContextCompat.checkSelfPermission(
                                                                           activityRef,
                                                                           Manifest.permission.CAMERA))
                {
                    mustAskPermissionFirst = true;
                    if (activityRef.shouldShowRequestPermissionRationale(Manifest.permission.CAMERA))
                    {
                        // Will drop in here if user denied permissions access camera before.
                        // Or no uses-permission CAMERA element is in the
                        // manifest file. Must explain to the end user why the app wants
                        // permissions to the camera devices.
                        Toast.makeText(activityRef.getApplicationContext(),
                                       "App requires access to camera to be granted",
                                       Toast.LENGTH_SHORT).show();
                    }
                    // Request permission from the user to access the camera.
                    Log.i(TAG, "CameraSurface(): must ask user for camera access permission");
                    activityRef.requestPermissions(new String[]
                                                       {
                                                           Manifest.permission.CAMERA
                                                       },
                                                   calib_optical_Activity.REQUEST_CAMERA_PERMISSION_RESULT);
                    return;
                }
            }
        }
        catch (Exception ex)
        {
            Log.e(TAG, "CameraSurface(): exception caught, " + ex.getMessage());
            return;
        }

        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS); // Deprecated in API level 11. Still required for API levels <= 10.
    }

    // SurfaceHolder.Callback methods

    @SuppressLint("NewApi")
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolderInstance) {
//        Log.i(TAG, "Opening camera.");
//        try {
//            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
//                int cameraIndex = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraIndex", "0"));
//                camera = Camera.open(cameraIndex);
//            } else {
//                camera = Camera.open();
//            }
//        } catch (RuntimeException exception) {
//            Log.e(TAG, "Cannot open camera. It may be in use by another process.");
//        }
//        if (camera != null) {
//            try {
//
//                camera.setPreviewDisplay(holder);
//                //camera.setPreviewCallback(this);
//                camera.setPreviewCallbackWithBuffer(this); // API level 8 (Android 2.2)
//
//            } catch (IOException exception) {
//                Log.e(TAG, "Cannot set camera preview display.");
//                camera.release();
//                camera = null;
//            }
//        }
        int cameraIndex = Integer.parseInt(PreferenceManager.
                                      getDefaultSharedPreferences(getContext()).
                                          getString("pref_cameraIndex", "0"));
        Log.i(TAG, "surfaceCreated(): called, will attempt open camera \"" + cameraIndex +
                       "\", set orientation, set preview surface");
        openCamera(surfaceHolderInstance, cameraIndex);
    }

    private void openCamera(SurfaceHolder surfaceHolderInstance, int cameraIndex) {
        Log.i(TAG, "openCamera(): called");
        try
        {
            camera = Camera.open(cameraIndex);
        }
        catch (RuntimeException ex) {
            Log.e(TAG, "openCamera(): RuntimeException caught, " + ex.getMessage() + ", abnormal exit");
            return;
        }
        //catch (CameraAccessException ex) {
        //      Log.e(TAG, "openCamera(): CameraAccessException caught, " + ex.getMessage() + ", abnormal exit");
        //      return;
        //  }
        catch (Exception ex) {
            Log.e(TAG, "openCamera()): exception caught, " + ex.getMessage() + ", abnormal exit");
            return;
        }

        if (!SetOrientationAndPreview(surfaceHolderInstance, cameraIndex)) {
            Log.e(TAG, "openCamera(): call to SetOrientationAndPreview() failed, openCamera() failed");
        }
        else
            Log.i(TAG, "openCamera(): succeeded");
    }

    private boolean SetOrientationAndPreview(SurfaceHolder surfaceHolderInstance, int cameraIndex)
    {
        Log.i(TAG, "SetOrientationAndPreview(): called");
        boolean success = true;
        try {
            //setCameraDisplayOrientation(cameraIndex, camera);
            camera.setPreviewDisplay(surfaceHolderInstance);
            camera.setPreviewCallbackWithBuffer(this); // API level 8 (Android 2.2)
        }
        catch (IOException ex) {
            Log.e(TAG, "SetOrientationAndPreview(): IOException caught, " + ex.toString());
            success = false;
        }
        catch (Exception ex) {
            Log.e(TAG, "SetOrientationAndPreview(): Exception caught, " + ex.toString());
            success = false;
        }
        if (!success)
        {
            if (null != camera)
            {
                camera.release();
                camera = null;
            }
            Log.e(TAG, "SetOrientationAndPreview(): released camera due to caught exception");
        }
        return success;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

        if (camera != null) {
            Log.i(TAG, "surfaceDestroyed(): closing camera");
            camera.stopPreview();
            camera.setPreviewCallback(null);
            camera.release();
            camera = null;
        }
    }

    @SuppressLint("NewApi") // CameraInfo
    @SuppressWarnings("deprecation") // setPreviewFrameRate
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

        if (camera != null) {

            String camResolution = PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraResolution", getResources().getString(R.string.pref_defaultValue_cameraResolution));
            String[] dims = camResolution.split("x", 2);
            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewSize(Integer.parseInt(dims[0]), Integer.parseInt(dims[1]));
            parameters.setPreviewFrameRate(30);
            camera.setParameters(parameters);

            parameters = camera.getParameters();
            int capWidth = parameters.getPreviewSize().width;
            int capHeight = parameters.getPreviewSize().height;
            int pixelformat = parameters.getPreviewFormat(); // android.graphics.imageformat
            PixelFormat pixelinfo = new PixelFormat();
            PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
            int cameraIndex = 0;
            boolean frontFacing = false;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
                cameraIndex = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraIndex", "0"));
                Camera.getCameraInfo(cameraIndex, cameraInfo);
                if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) frontFacing = true;
            }

            int bufSize = capWidth * capHeight * pixelinfo.bitsPerPixel / 8; // For the default NV21 format, bitsPerPixel = 12.

            for (int i = 0; i < 5; i++) camera.addCallbackBuffer(new byte[bufSize]);

            camera.startPreview();

            calib_optical_Activity.nativeVideoInit(capWidth, capHeight, cameraIndex, frontFacing);
        }
    }

    // Camera.PreviewCallback methods.
    @Override
    public void onPreviewFrame(byte[] data, Camera cam) {

        calib_optical_Activity.nativeVideoFrame(data);

        cam.addCallbackBuffer(data);
    }
} // end: public class CameraSurface extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback
