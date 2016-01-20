package sample.ar.artoolkit.org.armarkerdistance;

import android.os.Bundle;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.artoolkit.ar.base.ARActivity;
import org.artoolkit.ar.base.rendering.ARRenderer;

/**
 * Created by Thorsten Bux on 11.01.2016.
 */
public class ArDistanceActivity extends ARActivity{
    private SimpleARRenderer renderer;

    private FrameLayout mainView;
    private TextView textHello;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        textHello = (TextView) this.findViewById(R.id.textView);
        renderer = new SimpleARRenderer(this);

        setContentView(R.layout.activity_ar_distance);
        mainView = (FrameLayout) this.findViewById(R.id.mainLayout);
    }

    @Override
    protected ARRenderer supplyRenderer() {
        return renderer;
    }

    @Override
    public FrameLayout supplyFrameLayout() {
        return mainView;
    }
}
