/*
 *  ARActivity.java
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

package org.artoolkit.ar.base;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ConfigurationInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.ViewConfiguration;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import org.artoolkit.ar.base.camera.CameraEventListener;
import org.artoolkit.ar.base.camera.CameraPreferencesActivity;
import org.artoolkit.ar.base.camera.CaptureCameraPreview;
import org.artoolkit.ar.base.rendering.ARRenderer;
import org.artoolkit.ar.base.rendering.gles20.ARRendererGLES20;

/**
 * An activity which can be subclassed to create an AR application. ARActivity handles almost all of
 * the required operations to create a simple augmented reality application.
 * <p/>
 * ARActivity automatically creates a camera preview surface and an OpenGL surface view, and
 * arranges these correctly in the user interface.The subclass simply needs to provide a FrameLayout
 * object which will be populated with these UI components, using {@link #supplyFrameLayout() supplyFrameLayout}.
 * <p/>
 * To create a custom AR experience, the subclass should also provide a custom renderer using
 * {@link #supplyRenderer() Renderer}. This allows the subclass to handle OpenGL drawing calls on its own.
 */

public abstract class ARActivity extends /*AppCompat*/Activity implements CameraEventListener {

    /**
     * Android logging tag for this class.
     */
    protected final static String TAG = "ARBaseLib::ARActivity";

    /**
     * Renderer to use. This is provided by the subclass using {@link #supplyRenderer() Renderer()}.
     */
    protected ARRenderer renderer;

    /**
     * Layout that will be filled with the camera preview and GL views. This is provided by the subclass using {@link #supplyFrameLayout() supplyFrameLayout()}.
     */
    protected FrameLayout mainLayout;

    /**
     * Camera preview which will provide video frames.
     */
    private CaptureCameraPreview preview = null;

    /**
     * GL surface to render the virtual objects
     */
    private GLSurfaceView glView;

	/**
     * For any square template (pattern) markers, the number of rows
     * and columns in the template. May not be less than 16 or more than AR_PATT_SIZE1_MAX.
     */
	protected int pattSize = 16;

	/**
     * For any square template (pattern) markers, the maximum number
     * of markers that may be loaded for a single matching pass. Must be > 0.
     */
	protected int pattCountMax = 25;

    private boolean firstUpdate = false;

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    //@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Thread.setDefaultUncaughtExceptionHandler(
            new Thread.UncaughtExceptionHandler()
                {
                    @Override
                    public void uncaughtException (Thread thread, Throwable e)
                    {
                        handleUncaughtException(thread, e);
                    }
                });

        // This needs to be done just only the very first time the application is run,
        // or whenever a new preference is added (e.g. after an application upgrade).
        PreferenceManager.setDefaultValues(this, R.xml.preferences, false);

        // Correctly configures the activity window for running AR in a layer
        // on top of the camera preview. This includes entering
        // fullscreen landscape mode and enabling transparency.
		boolean needActionBar = false;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				if (!ViewConfiguration.get(this).hasPermanentMenuKey()) needActionBar = true;
			} else {
				needActionBar = true;
			}
        }
		if (needActionBar) {
			requestWindowFeature(Window.FEATURE_ACTION_BAR);
		} else {
            requestWindowFeature(Window.FEATURE_NO_TITLE);
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
        getWindow().setFormat(PixelFormat.TRANSLUCENT);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        AndroidUtils.reportDisplayInformation(this);
    }

    private Activity mActivity = null;

    /**
     * Allows subclasses to supply a custom {@link Renderer}.
     *
     * @return The {@link Renderer} to use.
     */
    protected abstract ARRenderer supplyRenderer();

    /**
     * Allows subclasses to supply a {@link FrameLayout} which will be populated
     * with a camera preview and GL surface view.
     *
     * @return The {@link FrameLayout} to use.
     */
    protected abstract FrameLayout supplyFrameLayout();

    @Override
    protected void onStart() {
        super.onStart();

        Log.i(TAG, "onStart(): called");
        mActivity = this;
        if (false == ARToolKit.getInstance().initialiseNativeWithOptions(this.getCacheDir().getAbsolutePath(),
                                                                         pattSize,
                                                                         pattCountMax)) {
            // Use cache directory for Data files.

            new AlertDialog.Builder(this)
                    .setMessage("The native library is not loaded. The application cannot continue.")
                    .setTitle("Error")
                    .setCancelable(true)
                    .setNeutralButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    finish();
                                }
                            })
                    .show();

            return;
        }

        mainLayout = supplyFrameLayout();
        if (mainLayout == null) {
            Log.e(TAG, "onStart(): Error: supplyFrameLayout did not return a layout.");
            return;
        }

        renderer = supplyRenderer();
        if (renderer == null) {
            Log.e(TAG, "onStart(): Error: supplyRenderer did not return a renderer.");
            // No renderer supplied, use default, which does nothing
            renderer = new ARRenderer();
        }
    }

    @SuppressWarnings("deprecation") // FILL_PARENT still required for API level 7 (Android 2.1)
    @Override
    public void onResume() {
        Log.i(TAG, "onResume(): called");
        super.onResume();

        // Create the camera preview
        preview = new CaptureCameraPreview(mActivity, this);
        Log.i(TAG, "onResume(): CaptureCameraPreview constructed");

        if (preview.gettingCameraAccessPermissionsFromUser())
            //No need to go further, must ask user to allow access to the camera first.
            return;

        // Create the GL view
        glView = new GLSurfaceView(this);

        // Check if the system supports OpenGL ES 2.0.
        final ActivityManager activityManager = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs2 = configurationInfo.reqGlEsVersion >= 0x20000;

        if (supportsEs2) {
            Log.i(TAG, "onResume(): OpenGL ES 2.x is supported");

            if (renderer instanceof ARRendererGLES20) {
                // Request an OpenGL ES 2.0 compatible context.
                glView.setEGLContextClientVersion(2);
            } else {
                Log.w(TAG, "onResume(): OpenGL ES 2.x is supported but only a OpenGL 1.x renderer is available." +
                        " \n Use ARRendererGLES20 for ES 2.x support. \n Continuing with OpenGL 1.x.");
                glView.setEGLContextClientVersion(1);
            }
        } else {
            Log.i(TAG, "onResume(): Only OpenGL ES 1.x is supported");
            if (renderer instanceof ARRendererGLES20)
                throw new RuntimeException("Only OpenGL 1.x available but a OpenGL 2.x renderer was provided.");
            // This is where you could create an OpenGL ES 1.x compatible
            // renderer if you wanted to support both ES 1 and ES 2.
            glView.setEGLContextClientVersion(1);
        }

        glView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        glView.getHolder().setFormat(PixelFormat.TRANSLUCENT); // Needs to be a translucent surface so the camera preview shows through.
        glView.setRenderer(renderer);
        glView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY); // Only render when we have a frame (must call requestRender()).
        glView.setZOrderMediaOverlay(true); // Request that GL view's SurfaceView be on top of other SurfaceViews (including CameraPreview's SurfaceView).

        Log.i(TAG, "onResume(): GLSurfaceView created");

        // Add the views to the interface
        mainLayout.addView(preview, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
        mainLayout.addView(glView, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));

        Log.i(TAG, "onResume(): Views added to main layout.");
        if (glView != null) glView.onResume();
    }

    @Override
    protected void onPause() {
        Log.i(TAG, "onPause(): called");
        super.onPause();

        if (glView != null) glView.onPause();

        // System hardware must be released in onPause(), so it's available to
        // any incoming activity. Removing the CameraPreview will do this for the
        // camera. Also do it for the GLSurfaceView, since it serves no purpose
        // with the camera preview gone.
        mainLayout.removeView(glView);
        mainLayout.removeView(preview);
    }

    @Override
    public void onStop() {
        Log.i(TAG, "onStop(): Activity stopping.");
        mActivity = null;
        super.onStop();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.options, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.settings) {
            startActivity(new Intent(this, CameraPreferencesActivity.class));
            return true;
        } else {
            return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Returns the camera preview that is providing the video frames.
     *
     * @return The camera preview that is providing the video frames.
     */
    public CaptureCameraPreview getCameraPreview() {
        return preview;
    }

    /**
     * Returns the GL surface view.
     *
     * @return The GL surface view.
     */
    public GLSurfaceView getGLView() {
        return glView;
    }

    @Override
    public void cameraPreviewStarted(int width, int height, int rate, int cameraIndex, boolean cameraIsFrontFacing) {

        if (ARToolKit.getInstance().startWithPushedVideo(width, height, null, cameraIndex, cameraIsFrontFacing)) {
            // Expects Data to be already in the cache dir. This can be done with the AssetUnpacker.
            Log.i(TAG, "cameraPreviewStarted(): Camera initialised");
        } else {
            // Error
            Log.e(TAG, "cameraPreviewStarted(): Error initialising camera. Cannot continue.");
            finish();
        }

        Toast.makeText(this, "Camera settings: " + width + "x" + height + "@" + rate + "fps", Toast.LENGTH_SHORT).show();
        firstUpdate = true;
    }

    @Override
    public void cameraPreviewFrame(byte[] frame, int frameSize) {
        if (firstUpdate) {
            // ARToolKit has been initialised. The renderer can now add markers, etc...
            if (renderer.configureARScene()) {
                Log.i(TAG, "cameraPreviewFrame(): Scene configured successfully");
            } else {
                // Error
                Log.e(TAG, "cameraPreviewFrame(): Error configuring scene. Cannot continue.");
                finish();
            }
            firstUpdate = false;
        }

        if (ARToolKit.getInstance().convertAndDetect1(frame, frameSize)) {

            // Update the renderer as the frame has changed
            if (glView != null)
                glView.requestRender();
            onFrameProcessed();
        }
    }

    public void onFrameProcessed() {
    }

    @Override
    public void cameraPreviewStopped() {
        ARToolKit.getInstance().stopAndFinal();
    }

    protected void showInfo() {

        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(this);

        dialogBuilder.setMessage("ARToolKit Version: " + NativeInterface.arwGetARToolKitVersion());

        dialogBuilder.setCancelable(false);
        dialogBuilder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });

        AlertDialog alert = dialogBuilder.create();
        alert.setTitle("ARToolKit");
        alert.show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        Log.i(TAG, "onRequestPermissionsResult(): called");
        if (requestCode == CaptureCameraPreview.REQUEST_CAMERA_PERMISSION_RESULT) {
            if (grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(getApplicationContext(),
                    "Application will not run with camera access denied",
                    Toast.LENGTH_LONG).show();
            }
            else if (1 <= permissions.length) {
                Toast.makeText(getApplicationContext(),
                    String.format("Camera access permission \"%s\" allowed", permissions[0]),
                    Toast.LENGTH_SHORT).show();
            }
            CaptureCameraPreview previewHook = getCameraPreview();
            if (null != previewHook) {
                Log.i(TAG, "onRequestPermissionsResult(): reset ask for cam access perm");
                previewHook.resetGettingCameraAccessPermissionsFromUserState();
            }
        }
        else
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    private void handleUncaughtException(Thread thread, Throwable e)
    {
        Log.e(TAG, "handleUncaughtException(): exception type, " + e.toString());
        Log.e(TAG, "handleUncaughtException(): thread, \"" + thread.getName() + "\" exception, \"" + e.getMessage() + "\"");
        e.printStackTrace();
        return;
    }
} // end: public abstract class ARActivity extends /*AppCompat*/Activity implements CameraEventListener