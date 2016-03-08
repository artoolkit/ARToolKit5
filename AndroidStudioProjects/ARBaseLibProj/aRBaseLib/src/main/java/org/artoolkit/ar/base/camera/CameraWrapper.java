/*
 *  CameraWrapper.java
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

import android.hardware.Camera;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Wraps camera functionality to handle the differences between Android 2.1 and 2.2.
 */
class CameraWrapper {

    /**
     * Android logging tag for this class.
     */
    private static final String TAG = "CameraWrapper";

    private static final String CAMERA_CLASS_NAME = "android.hardware.Camera";

    private Camera camera;

    private Class<?> cameraClass = null;

    private Method setPreviewCallbackMethod = null;
    private Method setPreviewCallbackWithBufferMethod = null;
    private Method addCallbackBufferMethod = null;

    private boolean usingBuffers = false;

    public CameraWrapper(Camera cam) {

        camera = cam;

        try {
            cameraClass = Class.forName(CAMERA_CLASS_NAME);
            Log.i(TAG, "CameraWrapper(): Found class " + CAMERA_CLASS_NAME);

            setPreviewCallbackMethod = cameraClass.getDeclaredMethod("setPreviewCallback", new Class[]{Camera.PreviewCallback.class});
            Log.i(TAG, "CameraWrapper(): Found method setPreviewCallback");

            setPreviewCallbackWithBufferMethod = cameraClass.getDeclaredMethod("setPreviewCallbackWithBuffer", new Class[]{Camera.PreviewCallback.class});
            Log.i(TAG, "CameraWrapper(): Found method setPreviewCallbackWithBuffer");

            addCallbackBufferMethod = cameraClass.getDeclaredMethod("addCallbackBuffer", new Class[]{byte[].class});
            Log.i(TAG, "CameraWrapper(): Found method addCallbackBuffer");

        } catch (NoSuchMethodException nsme) {
            Log.w(TAG, "CameraWrapper(): Could not find method: " + nsme.getMessage());


        } catch (ClassNotFoundException cnfe) {
            Log.w(TAG, "CameraWrapper(): Could not find class " + CAMERA_CLASS_NAME);
        }

    }

    public boolean configureCallback(Camera.PreviewCallback cb, boolean useBuffersIfAvailable, int numBuffersIfAvailable, int bufferSize) {

        boolean success = true;

        if (useBuffersIfAvailable && setPreviewCallbackWithBufferMethod != null && addCallbackBufferMethod != null) {

            success &= setPreviewCallbackWithBuffer(cb);

            for (int i = 0; i < numBuffersIfAvailable; i++) {
                success &= addCallbackBuffer(new byte[bufferSize]);
            }

            usingBuffers = true;

        } else {

            success &= setPreviewCallback(cb);

            usingBuffers = false;

        }

        if (success) {

            if (usingBuffers) {
                Log.i(TAG, "configureCallback(): Configured camera callback using " + numBuffersIfAvailable + " buffers of " + bufferSize + " bytes");
            } else {
                Log.i(TAG, "configureCallback(): Configured camera callback without buffers");
            }

        }


        return success;

    }

    public boolean frameReceived(byte[] data) {

        //Log.d(TAG, "frameReceived");
        if (usingBuffers) {
            return addCallbackBuffer(data);
        } else {
            return true;
        }
    }

    private boolean setPreviewCallback(Camera.PreviewCallback cb) {

        if (setPreviewCallbackMethod == null) return false;

        try {

            setPreviewCallbackMethod.invoke(camera, cb);

        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            return false;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            return false;
        } catch (InvocationTargetException e) {
            e.printStackTrace();
            return false;
        }

        return true;

    }

    private boolean setPreviewCallbackWithBuffer(Camera.PreviewCallback cb) {

        if (setPreviewCallbackMethod == null) return false;

        try {

            setPreviewCallbackWithBufferMethod.invoke(camera, cb);

        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            return false;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            return false;
        } catch (InvocationTargetException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }


    private boolean addCallbackBuffer(byte[] data) {

        if (addCallbackBufferMethod == null) return false;

        try {

            addCallbackBufferMethod.invoke(camera, data);

            //Log.d(TAG, "Returned camera data buffer to pool");

        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            return false;
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            return false;
        } catch (InvocationTargetException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

}