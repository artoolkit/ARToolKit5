/*
 *  calib_optical_Activity.java
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
 *  Author(s): Philip Lamb
 *
 */

package org.artoolkit.ar.utils.calib_optical;

import org.artoolkit.ar.utils.calib_optical.R;
//import org.artoolkit.ar.base.camera.CameraPreferencesActivity;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.hardware.SensorManager;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.OrientationEventListener;
import android.view.ViewConfiguration;
import android.view.ViewGroup.LayoutParams;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;

// For Epson Moverio BT-200. BT200Ctrl.jar must be in libs/ folder.
import jp.epson.moverio.bt200.DisplayControl;

public class calib_optical_Activity extends Activity {
    
    private static final String TAG = "calib_optical";
	
    // Load the native libraries.
    static {
    	System.loadLibrary("c++_shared");
    	
    	System.loadLibrary("calib_optical_Native");	    	
    }
    
	// Lifecycle functions.
    public static native boolean nativeCreate(Context ctx);
    public static native boolean nativeStart();
    public static native boolean nativeStop();
    public static native boolean nativeDestroy();
    // Camera functions.
    public static native boolean nativeVideoInit(int w, int h, int cameraIndex, boolean cameraIsFrontFacing);
    public static native void nativeVideoFrame(byte[] image);
    // OpenGL functions.
    public static native void nativeSurfaceCreated();
    public static native void nativeSurfaceChanged(int w, int h);
    public static native void nativeDrawFrame();
    // Touch functions.
    public static native void nativeHandleTouchAtLocation(int x, int y);
    public static native boolean nativeHandleBackButton(); // Returns true if handled on native side, false otherwise.
    public static native boolean nativeHandleMenuToggleVideoSeeThrough();
    
    // Other functions.
    public static native void nativeDisplayParametersChanged(int orientation, int w, int h, int dpi, int stereoDisplayMode); // 0 = portrait, 1 = landscape (device rotated 90 degrees ccw), 2 = portrait upside down, 3 = landscape reverse (device rotated 90 degrees cw).
    public static native boolean nativeSetCalibrationParameters(int eyeSelection); // 0 = left, 1 = right (or for stereo, 0 = left then right, 1 = right then left).
    
    private boolean forceLandscape;
	private GLSurfaceView glView;
	private CameraSurface camSurface;
	
	private FrameLayout mainLayout;
	
	private OrientationEventListener orientationListener;
	private int currentOrientation;
	
	// For Epson Moverio BT-200.
	private DisplayControl mDisplayControl = null;
	
	
	/** Called when the activity is first created. */
	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@Override
    public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
        
		forceLandscape = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("pref_forceLandscape", false);
		
		boolean needActionBar = false;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
				if (!ViewConfiguration.get(this).hasPermanentMenuKey() && 
					!(Build.MANUFACTURER.equals("EPSON") && Build.MODEL.equals("embt2"))) {
					needActionBar = true;
				}
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
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        // For Epson Moverio BT-200.
        if (Build.MANUFACTURER.equals("EPSON") && Build.MODEL.equals("embt2")) {
        	mDisplayControl = new DisplayControl(this);
            //private static final int FLAG_SMARTFULLSCREEN = 0x80000000; // For Epson Moverio BT-200.
        	getWindow().addFlags(0x80000000);
        }
        
        if (forceLandscape) setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE); // Force landscape-only.
        
        setContentView(R.layout.main);
        
		calib_optical_Activity.nativeCreate(this);
    }
    
    @Override
    public void onStart() 
    {
   	    super.onStart();
		
   	    // If the user changed preferences, here is the first opportunity to take those changes into account.
   	    boolean forceLandscapeNew = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("pref_forceLandscape", false);
   	    if (forceLandscape != forceLandscapeNew) {
   	    	forceLandscape = forceLandscapeNew;
   	    	setRequestedOrientation(forceLandscape ? ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
   	    }
   	    
   	    // For Epson Moverio BT-200, enable stereo mode. 
   	    if (Build.MANUFACTURER.equals("EPSON") && Build.MODEL.equals("embt2")) {
   	    	boolean stereo = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("pref_stereoDisplay", false);
   	    	//int dimension = (stereo ? DIMENSION_3D : DIMENSION_2D);
   	    	//set2d3d(dimension);
   	    	mDisplayControl.setMode(stereo ? DisplayControl.DISPLAY_MODE_3D : DisplayControl.DISPLAY_MODE_2D, stereo); // Last parameter is 'toast'.
   	    }
   	    
        // Orientation management.
        updateNativeDisplayParameters();
        if (!forceLandscape) {
            orientationListener = new  OrientationEventListener(this, SensorManager.SENSOR_DELAY_NORMAL) {
            	public void onOrientationChanged(int angle) {
            		// angle is 0-359, with 0 when the device is oriented upright, 90 when the device has been rotated clockwise by 90 degrees.
            		// Need to reverse the sense, and add 45 degree offset to match the sense of getWindowManager().getDefaultDisplay().getRotation().
            		if (angle != ORIENTATION_UNKNOWN) {
                		int angle1 = (360 - angle) + 45;
                		if (angle1 >= 360) angle1 -= 360;
                		int orientation = angle1 / 90;
                		if (orientation != currentOrientation) {
                			updateNativeDisplayParameters();
                		}
            		}
            	}        	
            };
        }
        nativeSetCalibrationParameters(Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(this).getString("pref_eyeSelection", "0")));

    	mainLayout = (FrameLayout)this.findViewById(R.id.mainLayout);
        
		calib_optical_Activity.nativeStart();
    }
    
    @SuppressWarnings("deprecation") // FILL_PARENT still required for API level 7 (Android 2.1)
    @Override
    public void onResume() {
    	super.onResume();

   		// In order to ensure that the GL surface covers the camera preview each time onStart
   		// is called, remove and add both back into the FrameLayout.
   		// Removing GLSurfaceView also appears to cause the GL surface to be disposed of.
   		// To work around this, we also recreate GLSurfaceView. This is not a lot of extra
   		// work, since Android has already destroyed the OpenGL context too, requiring us to
   		// recreate that and reload textures etc.
   	
		// Create the camera view.
		camSurface = new CameraSurface(this);

		// Create/recreate the GL view.
	    glView = new ARSurfaceView(this);    		
		//glView.setEGLConfigChooser(8, 8, 8, 8, 16, 0); // Do we actually need a transparent surface? I think not, (default is RGB888 with depth=16) and anyway, Android 2.2 barfs on this.
		glView.setRenderer(new Renderer());
		glView.setZOrderMediaOverlay(true); // Request that GL view's SurfaceView be on top of other SurfaceViews (including CameraPreview's SurfaceView).
        		
        mainLayout.addView(camSurface, new LayoutParams(128, 128));
 		mainLayout.addView(glView, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));

        if (orientationListener != null) orientationListener.enable();

        if (glView != null) glView.onResume();
    }

	@Override
	protected void onPause() {
	    super.onPause();
	    if (glView != null) glView.onPause();
	    
        if (orientationListener != null) orientationListener.disable();

        // System hardware must be release in onPause(), so it's available to
	    // any incoming activity. Removing the CameraPreview will do this for the
	    // camera. Also do it for the GLSurfaceView, since it serves no purpose
	    // with the camera preview gone.
	    mainLayout.removeView(glView);
	    mainLayout.removeView(camSurface);
	}

	@Override 
	public void onStop() {		
		super.onStop();		

		orientationListener = null;
		
		calib_optical_Activity.nativeStop();
	}
	
    @Override
    public void onDestroy() 
    {
   	    super.onDestroy();
   	    
		calib_optical_Activity.nativeDestroy();
    }
  
    private void updateNativeDisplayParameters()
    {
    	boolean forceLandscape = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("pref_forceLandscape", false);
    	Display d = getWindowManager().getDefaultDisplay();
    	currentOrientation = d.getRotation();
    	DisplayMetrics dm = new DisplayMetrics();
    	d.getMetrics(dm);
    	int w = dm.widthPixels;
    	int h = dm.heightPixels;
    	int dpi = dm.densityDpi;
    	boolean stereoDisplay = PreferenceManager.getDefaultSharedPreferences(this).getBoolean("pref_stereoDisplay", false);
    	int stereoDisplayMode = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(this).getString("pref_stereoDisplayMode", "0"));
        nativeDisplayParametersChanged((forceLandscape ? 1 : currentOrientation), w, h, dpi, (stereoDisplay ? stereoDisplayMode : 0));    	
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
    	super.onConfigurationChanged(newConfig);
    	
        //int nativeOrientation;
        //int orientation = newConfig.orientation; // Only portrait or landscape.
    	//if (orientation == Configuration.ORIENTATION_LANSCAPE) nativeOrientation = 0;
        //else /* orientation == Configuration.ORIENTATION_PORTRAIT) */ nativeOrientation = 1;
    	updateNativeDisplayParameters();
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
			startActivity(new Intent(this, OpticalPreferences.class));
			return true;
	    } else if (item.getItemId() == R.id.help) {
	    	Intent browse = new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.artoolworks.com/support/applink/calib_optical-android-help"));
	        startActivity(browse);
			return true;
	    } else if (item.getItemId() == R.id.toggleVideoSeeThrough) {
	    	return nativeHandleMenuToggleVideoSeeThrough();
		} else {
			return super.onOptionsItemSelected(item);
		}
	}
	
	@Override
	public void onBackPressed()
	{
		if (!nativeHandleBackButton()) {
			super.onBackPressed();
		}
	}
	
//	public void finishFromNative()
//	{
//		finish();
//	}
	
}