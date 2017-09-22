/*
 *  CaptureCameraPreview.java
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

package org.artoolkit.ar.base.camera;

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
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.Toast;
import org.artoolkit.ar.base.FPSCounter;
import org.artoolkit.ar.base.R;
import java.io.IOException;

@SuppressLint("ViewConstructor")
public class CaptureCameraPreview extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = "CameraPreview";

    /**
     * The Camera doing the capturing.
     */
    private Camera camera = null;
    private CameraWrapper cameraWrapper = null;

    /**
     * The camera capture width in pixels.
     */
    private int captureWidth;

    /**
     * The camera capture height in pixels.
     */
    private int captureHeight;

    /**
     * The camera capture rate in frames per second.
     */
    private int captureRate;

    /**
     * Counter to monitor the actual rate at which frames are captured from the camera.
     */
    private FPSCounter fpsCounter = new FPSCounter();

    /**
     * Listener to inform of camera related events: start, frame, and stop.
     */
    private CameraEventListener listener;

    private boolean mustAskPermissionFirst = false;
    public boolean gettingCameraAccessPermissionsFromUser()
    {
        return mustAskPermissionFirst;
    }

    public void resetGettingCameraAccessPermissionsFromUserState()
    {
        mustAskPermissionFirst = false;
    }

    /**
     * Constructor takes a {@link CameraEventListener} which will be called on
     * to handle camera related events.
     *
     * @param cel CameraEventListener to use. Can be null.
     */
    @SuppressWarnings("deprecation")
    public CaptureCameraPreview(Activity activity, CameraEventListener cel) {
        super(activity);

        Log.i(TAG, "CaptureCameraPreview(): ctor called");
        try
        {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
            {
                if (PackageManager.PERMISSION_GRANTED != ContextCompat.checkSelfPermission(
                                                                           activity,
                                                                           Manifest.permission.CAMERA))
                {
                    mustAskPermissionFirst = true;
                    if (activity.shouldShowRequestPermissionRationale(Manifest.permission.CAMERA))
                    {
                        // Will drop in here if user denied permissions access camera before.
                        // Or no uses-permission CAMERA element is in the
                        // manifest file. Must explain to the end user why the app wants
                        // permissions to the camera devices.
                        Toast.makeText(activity.getApplicationContext(),
                                       "App requires access to camera to be granted",
                                       Toast.LENGTH_SHORT).show();
                    }
                    // Request permission from the user to access the camera.
                    Log.i(TAG, "CaptureCameraPreview(): must ask user for camera access permission");
                    activity.requestPermissions(new String[]
                                                    {
                                                        Manifest.permission.CAMERA
                                                    },
                                                REQUEST_CAMERA_PERMISSION_RESULT);
                    return;
                }
            }
        }
        catch (Exception ex)
        {
            Log.e(TAG, "CaptureCameraPreview(): exception caught, " + ex.getMessage());
            return;
        }

        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS); // Deprecated in API level 11. Still required for API levels <= 10.

        setCameraEventListener(cel);
    }

    /**
     * Sets the {@link CameraEventListener} which will be called on to handle camera
     * related events.
     *
     * @param cel CameraEventListener to use. Can be null.
     */
    public void setCameraEventListener(CameraEventListener cel) {
        listener = cel;
    }
    public static final int REQUEST_CAMERA_PERMISSION_RESULT = 0;

    @SuppressLint("NewApi")
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolderInstance) {
        int cameraIndex = Integer.parseInt(PreferenceManager.
                                      getDefaultSharedPreferences(getContext()).
                                          getString("pref_cameraIndex", "0"));
        Log.i(TAG, "surfaceCreated(): called, will attempt to open camera \"" + cameraIndex +
                       "\", set orientation, set preview surface");
        openCamera(surfaceHolderInstance, cameraIndex);
    } // end: public void surfaceCreated(SurfaceHolder holder)

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
            setCameraDisplayOrientation(cameraIndex, camera);
            camera.setPreviewDisplay(surfaceHolderInstance);
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

    private void setCameraDisplayOrientation(int cameraId, android.hardware.Camera camera) {
        android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(cameraId, info);

        WindowManager wMgr = (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);

        int rotation = wMgr.getDefaultDisplay().getRotation();

        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0: degrees = 0; break;
            case Surface.ROTATION_90: degrees = 90; break;
            case Surface.ROTATION_180: degrees = 180; break;
            case Surface.ROTATION_270: degrees = 270; break;
        }

        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360;  // compensate the mirror
        } else {  // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        camera.setDisplayOrientation(result);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolderInstance) {
        // Surface will be destroyed when we return, so stop the preview.
        // Because the CameraDevice object is not a shared resource, it's very
        // important to release it when the activity is paused.
        Log.i(TAG, "surfaceDestroyed(): called");
        if (camera != null) {

            camera.setPreviewCallback(null);
            camera.stopPreview();

            camera.release();
            camera = null;
        }

        if (listener != null) listener.cameraPreviewStopped();
    }

    @SuppressWarnings("deprecation") // setPreviewFrameRate, getPreviewFrameRate
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.i(TAG, "surfaceChanged(): called");
        if (camera == null) {
            // Camera wasn't opened successfully?
            Log.e(TAG, "surfaceChanged(): No camera in surfaceChanged");
            return;
        }

        Log.i(TAG, "surfaceChanged(): Surfaced changed, setting up camera and starting preview");
        String camResolution = PreferenceManager.getDefaultSharedPreferences(
                                                     getContext()).getString("pref_cameraResolution",
                                                                             getResources().getString(
                                                                                                R.string.pref_defaultValue_cameraResolution));
        String[] dims = camResolution.split("x", 2);
        Camera.Parameters parameters = camera.getParameters();
        parameters.setPreviewSize(Integer.parseInt(dims[0]), Integer.parseInt(dims[1]));
        parameters.setPreviewFrameRate(30);
        camera.setParameters(parameters);

        parameters = camera.getParameters();
        captureWidth = parameters.getPreviewSize().width;
        captureHeight = parameters.getPreviewSize().height;
        captureRate = parameters.getPreviewFrameRate();
        int pixelformat = parameters.getPreviewFormat(); // android.graphics.imageformat
        PixelFormat pixelinfo = new PixelFormat();
        PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
        int cameraIndex = 0;
        boolean cameraIsFrontFacing = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
            Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
            cameraIndex = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraIndex", "0"));
            Camera.getCameraInfo(cameraIndex, cameraInfo);
            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT)
                cameraIsFrontFacing = true;
        }

        int bufSize = captureWidth * captureHeight * pixelinfo.bitsPerPixel / 8; // For the default NV21 format, bitsPerPixel = 12.
        Log.i(TAG, "surfaceChanged(): Camera buffers will be " + captureWidth + "x" + captureHeight + "@" + pixelinfo.bitsPerPixel + "bpp, " + bufSize + "bytes.");
        cameraWrapper = new CameraWrapper(camera);
        cameraWrapper.configureCallback(this, true, 10, bufSize); // For the default NV21 format, bitsPerPixel = 12.

        camera.startPreview();

        if (listener != null)
            listener.cameraPreviewStarted(captureWidth, captureHeight, captureRate, cameraIndex, cameraIsFrontFacing);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {

        if (listener != null) listener.cameraPreviewFrame(data, captureWidth*captureHeight + 2 * captureWidth/2*captureHeight/2);

        cameraWrapper.frameReceived(data);

        if (fpsCounter.frame()) {
            Log.i(TAG, "onPreviewFrame(): Camera capture FPS: " + fpsCounter.getFPS());
        }
    }
}
