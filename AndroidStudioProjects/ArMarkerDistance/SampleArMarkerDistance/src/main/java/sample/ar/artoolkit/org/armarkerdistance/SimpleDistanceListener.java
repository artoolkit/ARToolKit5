package sample.ar.artoolkit.org.armarkerdistance;

import org.artoolkit.ar.base.rendering.ARRenderer;

/**
 * Created by Thorsten Bux on 19.01.2016.
 */
public class SimpleDistanceListener implements DistanceListener {

    private ARRenderer renderer;

    public SimpleDistanceListener(ARRenderer renderer) {
        this.renderer = renderer;
    }

    @Override
    public void notify(float distance) {

    }
}
