/*
 *  AssetUnpackerActivity.java
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

package org.artoolkit.utils.AssetUnpacker;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import org.artoolkit.ar.base.assets.AssetFileTransfer;
import org.artoolkit.ar.base.assets.AssetHelper;

import java.util.List;

//import java.util.Set;
//import android.os.Environment;

public class AssetUnpackerActivity extends Activity {
    /**
     * Called when the activity is first created.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);
        TextView tv = (TextView) this.findViewById(R.id.infoText);

        AssetHelper assetHelper = new AssetHelper(getAssets());
        List<AssetFileTransfer> transfers = assetHelper.copyAssetFolder("Data", this.getCacheDir().getAbsolutePath());
        //List<AssetFileTransfer> transfers = assetHelper.copyAssetFolder("Data", Environment.getExternalStorageDirectory().getAbsolutePath());

        StringBuilder sb = new StringBuilder();

        for (AssetFileTransfer aft : transfers) {

            sb.append("Asset Path: " + aft.assetFile.getPath() + "\n");
            sb.append("Asset available: " + aft.assetAvailable + " \n");

            if (!aft.assetAvailable) continue;

            sb.append("SD Path: " + aft.targetFile.getPath() + "\n");
            sb.append("File already exists: " + aft.targetFileAlreadyExists + "\n");

            if (aft.targetFileAlreadyExists) {
                //sb.append("Existing file hash: " + aft.targetFileHash + "\n");
                //sb.append("New file hash: " + aft.tempFileHash + "\n");
                sb.append("Existing file CRC: " + aft.targetFileCRC + "\n");
                sb.append("New file CRC: " + aft.tempFileCRC + "\n");
                sb.append("Files match: " + aft.filesMatch + "\n");
            }

            sb.append("Asset copied: " + aft.assetCopied + "\n");

            sb.append("\n");

        }

        tv.setText(sb.toString());
    }
}