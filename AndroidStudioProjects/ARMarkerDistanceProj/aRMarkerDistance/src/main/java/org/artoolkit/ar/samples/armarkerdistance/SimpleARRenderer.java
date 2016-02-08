/*
 *  ARRenderer.java
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

package org.artoolkit.ar.samples.armarkerdistance;


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

            Log.i(TAG,"Distance: " + distance);

            float[] positionMarker2 = arToolKit.retrievePosition(markerId1, markerId2);

            if(positionMarker2 != null) {
                //Draw line from referenceMarker to another marker
                //In relation to the second marker the referenceMarker is on position 0/0/0
                float[] basePosition = {0f, 0f, 0f};
                line = new Line(basePosition, positionMarker2, 3);
                line.draw(gl);
            }
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
