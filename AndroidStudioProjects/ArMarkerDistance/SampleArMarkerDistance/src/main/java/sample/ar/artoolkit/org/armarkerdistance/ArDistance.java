package sample.ar.artoolkit.org.armarkerdistance;

import android.app.Application;

import org.artoolkit.ar.base.assets.AssetHelper;

public class ArDistance extends Application {

    private static Application arDistance;

    @Override
    public void onCreate() {
        super.onCreate();
        arDistance = this;

        this.initializeInstance();
    }

    public static Application getInstance(){
        return arDistance;
    }

    // Here we do one-off initialisation which should apply to all activities
    // in the application.
    protected void initializeInstance() {

        // Unpack assets to cache directory so native library can read them.
        // N.B.: If contents of assets folder changes, be sure to increment the
        // versionCode integer in the AndroidManifest.xml file.
        AssetHelper assetHelper = new AssetHelper(getAssets());
        assetHelper.cacheAssetFolder(getInstance(), "Data");
    }
}
