/*
 *  DetailsActivity.java
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

package org.artoolkit.utils.DeviceDetails;

import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.opengl.GLES10;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.ViewFlipper;
import android.widget.AdapterView.OnItemClickListener;

import org.artoolkit.ar.base.AndroidUtils;
import org.artoolkit.utils.DeviceDetails.R;

public class DetailsActivity extends Activity {
	
	public static final String TAG = "Details";
	
	ListView listView;
	ViewFlipper viewFlipper;
	
	static final String[] listItems = new String[] {
		"Camera", 
		"OpenGL", 
		"Other"
	};
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        
    	
    	super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    
        listView = (ListView)findViewById(R.id.listView);
        listView.setAdapter(new ArrayAdapter<String>(this, R.layout.list_item, listItems));

        
        viewFlipper = (ViewFlipper)findViewById(R.id.flipper);
 
        
        listView.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				viewFlipper.setAnimation(AnimationUtils.loadAnimation(view.getContext(), R.anim.push_left_in));
				viewFlipper.setDisplayedChild(position + 1);
			}
        });

        populateCameraInfoView();     
        populateOpenGLView();
        populateOtherInfoView();
        
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if(viewFlipper.getDisplayedChild() != 0){
            	viewFlipper.setDisplayedChild(0);
               return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    
 
    
    private void populateCameraInfoView() {

    	TextView cameraLabel = (TextView)this.findViewById(R.id.cameraLabel);
        
        StringBuffer sb = new StringBuffer();
        
        // Open the camera
        Camera camera; 
        camera = Camera.open();
        
        // Get necessary information
        Camera.Parameters parameters = camera.getParameters();
        
        sb.append("Supported Preview Rates\n");
        List<Integer> supportedPreviewRates = parameters.getSupportedPreviewFrameRates();
        
        if (supportedPreviewRates == null) {
        	Log.i(TAG, "No supported preview rates");
        } else {
        
	        for (Integer i : supportedPreviewRates) {        	
	        	Log.i(TAG, "Supported preview rate: " + i);
	        	sb.append(i + " fps\n");
	        }
	        
        }
        
        sb.append("\n");
        
        sb.append("Supported Preview Sizes\n");
        List<Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
        for (Size s : supportedPreviewSizes) {
        	Log.i(TAG, "Supported preview size: " + s.width + "x" + s.height);
        	sb.append(s.width + "x" + s.height + "\n");
        }
        
        sb.append("\n");
        
        
        sb.append("Supported Preview Formats\n");
        List<Integer> supportedPreviewFormats = parameters.getSupportedPreviewFormats();
        for (Integer i : supportedPreviewFormats) {
        	Log.i(TAG, "Supported preview format: " + i);
        	sb.append(i + "\n");
        }
        
        
        // Close the camera
        camera.release();
        camera = null;
        
        cameraLabel.setText(sb.toString());
    	
    }
    
    private QueryRenderer renderer;
    
    void populateOpenGLView() {
    	GLSurfaceView glView = (GLSurfaceView)this.findViewById(R.id.glView);
    	renderer = new QueryRenderer(this);
    	glView.setRenderer(renderer);
    }

    private void populateOtherInfoView() {

    	TextView otherLabel = (TextView)this.findViewById(R.id.otherLabel);
        
        StringBuffer sb = new StringBuffer();
        sb.append(AndroidUtils.androidBuildVersion());
        sb.append("\n");
        
        otherLabel.setText(sb.toString());
   }
    
    // 
    final Handler mHandler = new Handler();
    
    public void updateOpenGLView() {
    	
    	mHandler.post(new Runnable() {
            public void run() {
                updateOpenGLViewImpl();
            }
        });
    	
    	
    }
    
    private void updateOpenGLViewImpl() {
    	TextView glLabel = (TextView)this.findViewById(R.id.glLabel);
    	glLabel.setText(renderer.info.toString());
    	
    	// Don't need the GL view anymore
    	GLSurfaceView glView = (GLSurfaceView)this.findViewById(R.id.glView);    
    	glView.setVisibility(GLSurfaceView.GONE);
    	
    }
    
}
    
class QueryRenderer implements GLSurfaceView.Renderer{

	public String info;

	private DetailsActivity activity;
	
	public QueryRenderer(DetailsActivity act) {
		activity = act;
	}
	
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		String renderer = GLES10.glGetString(GL10.GL_RENDERER);    	
    	String version = GLES10.glGetString(GL10.GL_VERSION);
    	String extensions = GLES10.glGetString(GL10.GL_EXTENSIONS); 
	
		int[] maxTextureSize = new int[1];
		gl.glGetIntegerv(GL10.GL_MAX_TEXTURE_SIZE, maxTextureSize, 0);

    	
		StringBuffer sb = new StringBuffer();
		
		sb.append("Renderer: " + renderer + "\n");	
		sb.append("Version: " + version + "\n");
		sb.append("Extensions:\n" + extensions + "\n");
		
		info = sb.toString();
		
		activity.updateOpenGLView();
		
	}

	public void onSurfaceChanged(GL10 gl, int w, int h) {
		gl.glViewport(0, 0, w, h);
	}

	public void onDrawFrame(GL10 gl) {
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
	}
	
}

    
    
