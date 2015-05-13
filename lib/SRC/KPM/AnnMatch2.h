/*
 *  AnnMatch2.h
 *  libKPM
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not compile, install, use, or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive,
 *  non-transferable license, under Daqri's copyrights in this original Daqri
 *  software (the "Daqri Software"), to compile, install and execute Daqri Software
 *  exclusively in conjunction with the ARToolKit software development kit version 5.2
 *  ("ARToolKit"). The allowed usage is restricted exclusively to the purposes of
 *  two-dimensional surface identification and camera pose extraction and initialisation,
 *  provided that applications involving automotive manufacture or operation, military,
 *  and mobile mapping are excluded.
 *
 *  You may reproduce and redistribute the Daqri Software in source and binary
 *  forms, provided that you redistribute the Daqri Software in its entirety and
 *  without modifications, and that you must retain this notice and the following
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
 *  Copyright 2015 Daqri, LLC. All rights reserved.
 *  Copyright 2006-2015 ARToolworks, Inc. All rights reserved.
 *  Author(s): Hirokazu Kato, Philip Lamb
 *
 */

#ifndef KPM_AnnMatch2_h
#define KPM_AnnMatch2_h

#include <opencv2/core/core.hpp>
#include <opencv2/flann/flann.hpp>
#include <KPM/kpmType.h>

#define   Ann2Thresh       0.2

class CAnnMatch2
{
public:
    CAnnMatch2(void);
    ~CAnnMatch2(void);
    
protected:
    float               annThresh;
    cv::flann::Index*   flann_index1;
    cv::flann::Index*   flann_index2;
    cv::Mat             features1;
    cv::Mat             features2;
    int                *index1;
    int                *index2;
    
public:
    void Construct (FeatureVector* fv);
    void Match     (FeatureVector* fv, int knn, int *result);
};


#endif
