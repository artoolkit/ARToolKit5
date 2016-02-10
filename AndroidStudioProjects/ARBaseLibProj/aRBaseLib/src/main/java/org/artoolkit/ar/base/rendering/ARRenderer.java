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

package org.artoolkit.ar.base.rendering;

import android.opengl.GLES10;
import android.opengl.GLSurfaceView;

import org.artoolkit.ar.base.ARToolKit;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Base renderer which should be subclassed in the main application and provided
 * to the ARActivity using its {@link supplyRenderer} method.
 * <p/>
 * Subclasses should override {@link configureARScene}, which will be called by
 * the Activity when AR initialisation is complete. The Renderer can use this method
 * to add markers to the scene, and perform other scene initialisation.
 * <p/>
 * The {@link draw} method should also be override to perfom actual rendering. This is
 * in preference to directly overriding {@link onDrawFrame}, because ARRenderer will check
 * that the ARToolKit is running before calling draw.
 */
public class ARRenderer implements GLSurfaceView.Renderer {

    /**
     * Allows subclasses to load markers and prepare the scene. This is called after
     * initialisation is complete.
     */
    public boolean configureARScene() {
        return true;
    }

    public void onSurfaceCreated(GL10 unused, EGLConfig config) {

        // Transparent background
        GLES10.glClearColor(0.0f, 0.0f, 0.0f, 0.f);
    }

    public void onSurfaceChanged(GL10 unused, int w, int h) {
        GLES10.glViewport(0, 0, w, h);
    }

    public void onDrawFrame(GL10 gl) {
        if (ARToolKit.getInstance().isRunning()) {
            draw(gl);
        }
    }

    /**
     * Should be overridden in subclasses and used to perform rendering.
     */
    public void draw(GL10 gl) {
        GLES10.glClear(GLES10.GL_COLOR_BUFFER_BIT | GLES10.GL_DEPTH_BUFFER_BIT);
    }

}
