package sample.ar.artoolkit.org.armarkerdistance;


import android.content.Context;
import android.util.Log;

import org.artoolkit.ar.base.ARToolKit;
import org.artoolkit.ar.base.rendering.Line;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Thorsten Bux on 11.01.2016.
 */
public class SimpleARRenderer extends org.artoolkit.ar.base.rendering.ARRenderer {

    private final static String TAG = "ARRenderer";
    Context context;
    private int markerId1;
    private int markerId2;
    private List<Integer> markerArray = new ArrayList<>();
    private ARToolKit arToolKit;
    private Line line;


    public SimpleARRenderer(Context arDistanceActivity) {
        super();
        this.context = arDistanceActivity;
    }

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

    public void draw(GL10 gl) {

        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadMatrixf(arToolKit.getProjectionMatrix(), 0);

        gl.glEnable(GL10.GL_CULL_FACE);
        gl.glShadeModel(GL10.GL_SMOOTH);
        gl.glEnable(GL10.GL_DEPTH_TEST);
        gl.glFrontFace(GL10.GL_CW);

        gl.glMatrixMode(GL10.GL_MODELVIEW);

        Map<Integer, float[]> transformationMatrixPerVisibleMarker = storeTransformationMatrixPerVisibleMarker();

        workWithVisibleMarkers(gl, transformationMatrixPerVisibleMarker);

    }

    private void workWithVisibleMarkers(GL10 gl, Map<Integer, float[]> transformationArray) {
        //if more than one marker visible
        if (transformationArray.size() > 1) {
            Log.i(TAG, "transformationArray size = " + transformationArray.size());
            for (Map.Entry<Integer, float[]> entry : transformationArray.entrySet()) {

                gl.glLoadMatrixf(entry.getValue(), 0);
                gl.glPushMatrix();

                gl.glPopMatrix();
            }

            float distance = arToolKit.distance(markerId1, markerId2);
            float[] positionMarker2 = arToolKit.retrievePosition(markerId1, markerId2);

            //Draw line from referenceMarker to another marker
            //In relation to the second marker the referenceMarker is on position 0/0/0
            float[] basePosition = {0f, 0f, 0f};
            line = new Line(basePosition, positionMarker2, 3);
            line.draw(gl);
        }


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
