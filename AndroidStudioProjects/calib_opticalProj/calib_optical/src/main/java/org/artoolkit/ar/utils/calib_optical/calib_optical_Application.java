/*
 *  calib_optical_Application.java
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


//
// This class provides a subclass of Application to enable app-wide behavior.
// 

package org.artoolkit.ar.utils.calib_optical;

import org.artoolkit.ar.base.assets.AssetHelper;

import android.app.Application;
import android.preference.PreferenceManager;

public class calib_optical_Application extends Application {

	private static Application sInstance;
	 
	// Anywhere in the application where an instance is required, this method
	// can be used to retrieve it.
    public static Application getInstance() {
    	return sInstance;
    }
    
    @Override
    public void onCreate() {
    	super.onCreate(); 
    	sInstance = this;
    	((calib_optical_Application) sInstance).initializeInstance();
    }
    
    // Here we do one-off initialisation which should apply to all activities
	// in the application.
    protected void initializeInstance() {
    	
    	PreferenceManager.setDefaultValues(this, org.artoolkit.ar.utils.calib_optical.R.xml.preferences, false);
    	
		// Unpack assets to cache directory so native library can read them.
    	// N.B.: If contents of assets folder changes, be sure to increment the
    	// versionCode integer in the AndroidManifest.xml file.
		AssetHelper assetHelper = new AssetHelper(getAssets());        
		assetHelper.cacheAssetFolder(getInstance(), "Data");
    }
}
