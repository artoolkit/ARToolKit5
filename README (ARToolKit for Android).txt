Read me for ARToolKit for Android.
==================================


Contents.
---------

About this archive.
Requirements.
Getting started.
Training new markers.
Release notes.
libKPM usage.
Next steps.


About this archive.
------------------- 

This archive contains the ARToolKit libraries, utilities and examples for Android, version 5.2.1.

ARToolKit version 5.2.1 is released under the GNU Lesser General Public License version 3, with some additional permissions. Example code is generally released under a more permissive disclaimer; please read the file LICENSE.txt for more information.

If you intend to publish your app on Google's Play Store or any other commercial marketplace, you must use your own package name, and not the org.artoolkit package name.

ARToolKit is designed to build on Windows, Macintosh OS X, Linux, iOS and Android platforms.

This archive was assembled by:
    Philip Lamb
    http://www.artoolkit.org
    2015-05-29


Requirements.
-------------

Requirements:
 * Android SDK Tools r12 (for Android 2.2/API 8) or later required, r22.6.2 (March 2014) or later recommended.
 * Development of native ARToolKit for Android applications or requires Android NDK Revision 8b or later, revision 9d (March 2014) recommended.
 * Use of the Eclipse IDE is recommended, and Eclipse project files are supplied with the ARToolKit for Android SDK.
 * An Android device, running Android 2.2 or later. Testing is not possible using the Android Virtual Device system.
 * A printer to print the pattern PDF images.

ARToolKit is supplied as pre-built binaries for each platform, plus full source code for the SDK libraries, utilities, and examples, and documentation. If you wish to view the source for the desktop-only utilities, you will also need to use this Android release alongside ARToolKit v5.x for Mac OS X, Windows, or Linux.


Getting started.
----------------

For full instructions on using this SDK, please refer to the online user guide at http://www.artoolkit.org/documentation/ARToolKit_for_Android.


Training new markers.
---------------------

The utilities required to train new square and NFT markers are provide in the "bin" directory of the SDK. The utilities are command-line Windows/OS X executables which should be run from a Terminal environment.

Consult the ARToolKit documentation for more information:
    http://www.artoolkit.org/documentation/Creating_and_training_new_ARToolKit_markers
    http://www.artoolkit.org/documentation/ARToolKit_NFT

Usage (OS X):
    ./mk_patt
	./genTexData somejpegfile.jpg
	./dispImageSet somejpegfile.jpg
	./dispFeatureSet somejpegfile.jpg
	./checkResolution Data/camera_para.dat (change camera_para.dat to the correct camera calibration file for your device, camera and resolution).

Usage (Windows):
    mk_patt.exe
	genTexData.exe somejpegfile.jpg
	dispImageSet.exe somejpegfile.jpg
	dispFeatureSet.exe somejpegfile.jpg
	checkResolution.exe Data/camera_para.dat (change camera_para.dat to the correct camera calibration file for your device, camera and resolution).


Release notes.
--------------
This release contains ARToolKit v5.2.1 for Android.

Please see the ChangeLog.txt for details of changes in this and earlier releases.

ARToolKit v5.2 is the first major release under an open source license in several years, and represents several years of commercial development of ARToolKit by ARToolworks during this time. It will appear to be a significant change to previous users of ARToolKit v2.x. Please see http://www.artoolkit.org/documentation/ARToolKit_feature_comparison for more information.

For users of ARToolKit Professional versions 4.0 through 5.1.7, ARToolKit v5.2 includes a number of changes. Significantly, full source is now provided for the NFT libraries libAR2 and libKPM.

ARToolKit 5.2 for Android supports fetching of camera calibration data from an ARToolworks-provided server. On request, ARToolworks provides developers access to an on-device verson of calib_camera which submits calibration data to ARToolworks' server, making it available to all users of that device. The underlying changes to support this include a native version of libARvideo for Android which provides the functions to fetch camera calibration data (note that libARvideo on Android does not handle frame retrieval from the camera; that still happens on the Java side). Existing native apps which wish to use the functionality should follow the example usage from the ARNative example, and also link against libarvideo, and its additional dependencies libcurl, libssl and libcrypto.

Please see http://www.artoolkit.org/documentation/Using_automatic_online_camera_calibration_retrieval for details on this feature and the code changes required.


libKPM usage.
-------------

libKPM, which performs key-point matching for NFT page recognition and initialization depends on the patented SURF algorithm. Previously, this was used under a commercial license from the owners of the SURF patent. Since this license is no longer appropriate for a free and open-source release, we are actively working on replacing SURF in libKPM. We expect to complete this change by the end of June 2015. In the meantime, users are free to use libKPM including SURF under the terms set out in the KPM header and source files. We recommend against deploying products including libKPM until an updated free and open source version is made available. We apologize for the inconvenience.

Existing holders of a commercial license to ARToolKit Professional v5.x may use libKPM from ARToolKit v5.2 under the terms of their current license for the remainder of its duration. Please contact us via http://www.artoolkit.org/contact if you are an existing user of ARToolKit Professional with questions.


Next steps.
-----------

We have made a forum for discussion of ARToolKit for Android development available on our community website.

http://www.artoolkit.org/community/forums/viewforum.php?f=26


You are invited to join the forum and contribute your questions, answers and success stories.

ARToolKit consists of a full ecosystem of SDKs for desktop, web, mobile and in-app plugin augmented reality. Stay up to date with information and releases from artoolkit.org by joining our announcements mailing list.

http://www.artoolkit.org/community/lists/


Do you have a feature request, bug report, or other code you would like to contribute to ARToolKit? Access the complete source and issue tracker for ARToolKit at http://github.com/artoolkit/artoolkit5

--
EOF
