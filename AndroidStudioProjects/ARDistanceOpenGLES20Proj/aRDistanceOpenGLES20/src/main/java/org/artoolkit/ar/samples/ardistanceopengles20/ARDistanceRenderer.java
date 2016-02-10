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

import java.util.AbstractCollection;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import org.artoolkit.ar.samples.ardistanceopengles20.shader.MarkerDistanceFragmentShader;
import org.artoolkit.ar.samples.ardistanceopengles20.shader.MarkerDistanceShaderProgram;

/**
 * Created by Thorsten Bux on 25.01.2016.
 */
public class ARDistanceRenderer extends ARRendererGLES20{

    private static final String TAG = "ARDistanceRenderer";
    private ARToolKit arToolKit;
    private int markerId2;
    private int markerId1;
    private AbstractCollection<Integer> markerArray = new ArrayList<>();
    ARDrawableOpenGLES20 line;


    @Override
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
    public void draw() {
        super.draw();

        GLES20.glEnable(GLES20.GL_CULL_FACE);
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glFrontFace(GLES20.GL_CW);

        Map<Integer, float[]> transformationMatrixPerVisibleMarker = storeTransformationMatrixPerVisibleMarker();

        if(transformationMatrixPerVisibleMarker.size() > 1 ){

            float[] positionMarker2 = arToolKit.retrievePosition(markerId1, markerId2);

            //Draw line from referenceMarker to another marker
            //In relation to the second marker the referenceMarker is on position 0/0/0
            float[] basePosition = {0f, 0f, 0f, 1f};

            if(positionMarker2 != null) {
                ((Line) line).setStart(basePosition);
                ((Line) line).setEnd(positionMarker2);
                float[] color = {0.38f, 0.757f, 0.761f,1};
                ((Line) line).setColor(color);

                line.draw(arToolKit.getProjectionMatrix(), transformationMatrixPerVisibleMarker.get(markerId1));
            }
        }
    }

    //Shader calls should be within a GL thread that is onSurfaceChanged(), onSurfaceCreated() or onDrawFrame()
    //As the cube instantiates the shader during constructor call we need to do create the cube here.
    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        super.onSurfaceCreated(unused, config);

        int lineWidth = 3;
        ShaderProgram shaderProgram = new MarkerDistanceShaderProgram(new BaseVertexShader(),new MarkerDistanceFragmentShader(),lineWidth);

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
