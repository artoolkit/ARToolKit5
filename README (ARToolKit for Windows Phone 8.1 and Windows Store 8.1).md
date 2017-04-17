# Read me for ARToolKit for Windows Phone 8.1 and Windows Store 8.1

## Contents

- About this archive
- Requirements
- Installing
- Getting started
- Training new markers
- Release notes
- libKPM usage
- Next steps


## About this archive

This archive contains the ARToolKit libraries and utilities for Windows Phone 8.1 and Windows Store 8.1, version 5.3.2.

ARToolKit version 5.3.2 is released under the GNU Lesser General Public License version 3, with some additional permissions. Example code is generally released under a more permissive disclaimer; please read the file LICENSE.txt for more information.

If you intend to publish your app on Microsoft's Windows Store or any other commercial marketplace, you must use your own package name, and not the org.artoolkit package name.

>This archive was assembled by:  
    Philip Lamb  
    http://www.artoolkit.org  
    2016-03-23  


## Requirements

**Requirements**  
- Microsoft Visual Studio 2013 Update 3 or later, running on Windows 8.1 or later.  
- These devices are supported: Devices running Windows Phone 8.1, and devices running Windows 8.1 on ARM, x86 and x86-64 CPUs.  
- A valid Microsoft Developer License.  
- A printer to print the pattern PDF images "Hiro pattern", "Kanji pattern", "Sample1 pattern", "Sample2 pattern", and "pinball.jpg".  

ARToolKit is supplied as pre-built binaries for each platform, plus full source code for the SDK libraries and utilities, and documentation. If you wish to view the source for the desktop-only utilities, you will also need to use this Windows Phone 8.1 and Windows Store 8.1 release alongside ARToolKit v5.x for Mac OS X, Windows, or Linux.


## Installing
 
The SDK is supplied as an runnable installer. Double-click the installer to begin installation, and then follow the prompts. After installation, items installed will be available under the Applications list.


## Getting started

Locate and run the Visual Studio 2013 Solution file, found in the folder "vs120-winrt" folder inside the "Visual Studio" folder at the root of the SDK.

Binary, library and include directories are named as follows:

```[platform]-[target]-[arch]```

where [platform] is "WinRT" (for this release), [target] is either "wp8_1" for Windows Phone 8.1 or "w8_1" for Windows Store 8.1, and [arch] is one of "arm" for ARM-based CPUs, "x86" for 32-bit x86 CPUs or emulated 32-bit on 64-bit CPUs (i.e. WOW64), or "x64" for 64-bit x86-64 CPUs.


##Training new markers

The utilities required to train new square and NFT markers are provide in the "bin" directory of the SDK. The utilities are command-line Windows executables which should be run from a command-line environment.

Consult the ARToolKit documentation for more information:  
- [http://www.artoolkit.org/documentation/Creating_and_training_new_ARToolKit_markers](http://www.artoolkit.org/documentation/Creating_and_training_new_ARToolKit_markers)  
- [http://www.artoolkit.org/documentation/ARToolKit_NFT](http://www.artoolkit.org/documentation/ARToolKit_NFT)

Usage:  
- mk\_patt.exe  
- genTexData.exe somejpegfile.jpg  
- dispImageSet.exe somejpegfile.jpg  
- dispFeatureSet.exe somejpegfile.jpg  
- checkResolution.exe Data\camera_para.dat (change camera_para.dat to the correct camera calibration file for your device, camera and resolution).


##Release notes

This release contains ARToolKit Professional v5.3.3 for Windows Phone 8.1 and Windows Store 8.1.

- ARToolKit v5.3.1 introduced a new WinRT component to assist rendering of video backgrounds, libARgsubD3D, and set of native XAML application examples. The examples demonstrate basic square tracking, multimarker tracking, NFT tracking, rendering of UI controls into the AR scene. With these examples, ARToolKit becomes the first full-featured SDK for developing augmented reality apps on Microsoft's newest platform, and we hope that Windows developers are able to create some great apps with the support we've provided. Many thanks to Rene Schulte for his work on these examples, and DAQRI for sponsoring the development work.  
- A C++/CX-based WinRT Component is provided. Component binaries and metadata can be found in the appropriate "bin" subdirectory.  
- ARWrapper is provided both as a native WinRT DLL accessible via P/Invoke and a static library available for direct linking in a XAML C++ app.  
- libARvideo on Windows Phone 8.1 and Windows Store 8.1 uses a new video module, "WinMC" (based on Windows.Media.Capture) by default. The WinMF module (based on Windows Media Foundation) is also present but provides only support for video capture from file, not from a device camera.  

Please see the ChangeLog.txt for details of changes in this and earlier releases.

The major change in ARToolKit v5.3 was a new version of libKPM based on the FREAK detector framework, contributed by DAQRI. See "libKPM usage" below.

ARToolKit v5.2 was the first major release under an open source license in several years, and represented several years of commercial development of ARToolKit by ARToolworks during this time. It was a significant change to previous users of ARToolKit v2.x. Please see [http://www.artoolkit.org/documentation/ARToolKit_feature_comparison](http://www.artoolkit.org/documentation/ARToolKit_feature_comparison) for more information.


##libKPM usage

libKPM, which performs key-point matching for NFT page recognition and initialization now use a FREAK detector framework, contributed by DAQRI. Unlike the previous commercial version of libKPM which used SURF features, FREAK is not encumbered by patents. libKPM now joins the other core ARToolKit libraries under an LGPLv3 license. Additionally the new libKPM no longer has dependencies on OpenCV’s FLANN library, which should simply app builds and app distribution on all supported platforms.

Existing holders of a commercial license to ARToolKit Professional v5.x may use libKPM from ARToolKit v5.2 under the terms of their current license for the remainder of its duration. Please contact us via http://www.artoolkit.org/contact if you are an existing user of ARToolKit Professional with questions.


##Next steps

We have made a forum for discussion of ARToolKit for Windows Phone 8.1 and Windows Store 8.1 development available on our community website.

[http://www.artoolkit.org/community/forums/viewforum.php?f=31](http://www.artoolkit.org/community/forums/viewforum.php?f=31)

You are invited to join the forum and contribute your questions, answers and success stories.

ARToolKit consists of a full ecosystem of SDKs for desktop, web, mobile and in-app plugin augmented reality. Stay up to date with information and releases from artoolkit.org by joining our announcements mailing list.(Click ‘Subscribe’ at the bottom of [http://www.artoolkit.org/](http://www.artoolkit.org/))

Do you have a feature request, bug report, or other code you would like to contribute to ARToolKit? Access the complete source and issue tracker for ARToolKit at [http://github.com/artoolkit/artoolkit5](http://github.com/artoolkit/artoolkit5)


