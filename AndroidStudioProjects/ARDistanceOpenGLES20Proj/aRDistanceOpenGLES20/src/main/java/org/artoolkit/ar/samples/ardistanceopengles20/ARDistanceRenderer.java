/*
 *  ARDistanceRenderer.java
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
 *  Author(s): Philip Lamb
 *
 */


package org.artoolkit.ar.samples.ardistanceopengles20;

import android.opengl.GLES20;
import android.util.Log;

import org.artoolkit.ar.base.ARToolKit;
import org.artoolkit.ar.base.rendering.Line;
import org.artoolkit.ar.base.rendering.gles20.ARDrawableOpenGLES20;
import org.artoolkit.ar.base.rendering.gles20.ARRendererGLES20;
import org.artoolkit.ar.base.rendering.gles20.BaseVertexShader;
import org.artoolkit.ar.base.rendering.gles20.LineGLES20;
import org.artoolkit.ar.base.rendering.gles20.ShaderProgram;
import org.artoolkit.ar.samples.ardistanceopengles20.shader.MarkerDistanceFragmentShader;
import org.artoolkit.ar.samples.ardistanceopengles20.shader.MarkerDistanceShaderProgram;

import java.util.AbstractCollection;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Thorsten Bux on 25.01.2016.
 * <p/>
 * <p/>
 * This is the base rendering class derived from {@link ARRendererGLES20} (or {@link org.artoolkit.ar.base.rendering.ARRenderer} if only OpenGL 1 is used)
 * Basically it consists for three methodes:
 * -  {@link #configureARScene()}: Here you configure the AR scene with the used markers and fetch your ARToolkit instance
 * -  {@link #onSurfaceCreated(GL10, EGLConfig)}: This is the initialization method that is called once Android creates the UI element used for the AR context. All instantiation related to rendering, etc. goes in there
 * -  {@link #draw()}: This method is called from the surrounding framework for each camera frame. Here you kick off the rendering and check for visible markers.
 */
public class ARDistanceRenderer extends ARRendererGLES20 {

    private static final String TAG = "ARDistanceRenderer";
    ARDrawableOpenGLES20 line;
    private ARToolKit arToolKit;
    private int markerId2;
    private int markerId1;
    private AbstractCollection<Integer> markerArray = new ArrayList<>();

    @Override
    /**
     * Here you configure the AR scene with the used markers and fetch your ARToolkit instance
     * This example adds all used markers into an array for further reference.
     */
    public boolean configureARScene() {

        arToolKit = ARToolKit.getInstance();
        markerId2 = arToolKit.addMarker("single;Data/cat.patt;80");
        if (markerId2 < 0) {
            Log.e(TAG, "Unable to load marker 2");
            return false;
        }
        markerId1 = arToolKit.addMarker("single;Data/minion.patt;80");
        if (markerId1 < 0) {
            Log.e(TAG, "Unable to load marker 1");
            return false;
        }

        markerArray.add(markerId1);
        markerArray.add(markerId2);
        arToolKit.setBorderSize(0.1f);
        Log.i(TAG, "Border size: " + arToolKit.getBorderSize());

        return true;
    }

    @Override
    /**
     * This method is called from the surrounding framework for each camera frame. Here you kick off the rendering and check for visible markers.
     */
    public void draw() {
        super.draw();

        GLES20.glEnable(GLES20.GL_CULL_FACE);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glFrontFace(GLES20.GL_CW);

        Map<Integer, float[]> transformationMatrixPerVisibleMarker = storeTransformationMatrixPerVisibleMarker();

        if (transformationMatrixPerVisibleMarker.size() > 1) {

            float[] positionMarker2 = arToolKit.retrievePosition(markerId1, markerId2);

            //Draw line from referenceMarker to another marker
            //In relation to the second marker the referenceMarker is on position 0/0/0
            float[] basePosition = {0f, 0f, 0f, 1f};

            if (positionMarker2 != null) {
                ((Line) line).setStart(basePosition);
                ((Line) line).setEnd(positionMarker2);
                float[] color = {0.38f, 0.757f, 0.761f, 1};
                ((Line) line).setColor(color);

                line.draw(arToolKit.getProjectionMatrix(), transformationMatrixPerVisibleMarker.get(markerId1));
            }
        }
    }

    //Shader calls should be within a GL thread that is onSurfaceChanged(), onSurfaceCreated() or onDrawFrame()
    //As the line instantiates the shader during constructor call we need to do create the line here.
    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        super.onSurfaceCreated(unused, config);

        int lineWidth = 3;
        ShaderProgram shaderProgram = new MarkerDistanceShaderProgram(new BaseVertexShader(), new MarkerDistanceFragmentShader(), lineWidth);

        line = new LineGLES20(lineWidth);
        line.setShaderProgram(shaderProgram);
    }

    private Map<Integer, float[]> storeTransformationMatrixPerVisibleMarker() {
        Map<Integer, float[]> transformationArray = new HashMap<>();

        for (int markerId : markerArray) {
            if (arToolKit.queryMarkerVisible(markerId)) {

                float[] transformation = arToolKit.queryMarkerTransformation(markerId);

                if (transformation != null) {
                    transformationArray.put(markerId, transformation);
                    Log.d(TAG, "Found Marker " + markerId + " with transformation " + Arrays.toString(transformation));
                }
            } else {
                transformationArray.remove(markerId);
            }

        }
        return transformationArray;
    }
}
