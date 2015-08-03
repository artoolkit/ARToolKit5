//
//  visual_database_opencvfacade.cpp
//  ARToolKit5
//
//  This file is part of ARToolKit.
//
//  ARToolKit is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ARToolKit is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
//
//  As a special exception, the copyright holders of this library give you
//  permission to link this library with independent modules to produce an
//  executable, regardless of the license terms of these independent modules, and to
//  copy and distribute the resulting executable under terms of your choice,
//  provided that you also meet, for each linked independent module, the terms and
//  conditions of the license of that module. An independent module is a module
//  which is neither derived from nor based on this library. If you modify this
//  library, you may extend this exception to your version of the library, but you
//  are not obligated to do so. If you do not wish to do so, delete this exception
//  statement from your version.
//
//  Copyright 2013-2015 Daqri, LLC.
//
//  Author(s): Chris Broaddus
//

#pragma once

#include "visual_database_opencvfacade.h"
#include <../akaze/akaze_features.hpp>
#include <math/homography.h>

#include <unordered_map>

static int fileId = 0;

namespace vision {
    typedef std::unordered_map<int, std::vector<cv::KeyPoint> > keypoint_map_t;
    typedef std::unordered_map<int, cv::Mat > descriptor_map_t;
    typedef std::unordered_map<int, cv::Size > size_map_t;
    typedef std::unordered_map<int, cv::Mat > image_map_t;
    
    static std::vector<cv::KeyPoint> NullKeyPoints;
    static cv::Mat NullDescriptors;
    
    static inline float distance_squared(const cv::Point2f& a, const cv::Point2f& b)
    {
        auto dx = a.x - b.x;
        auto dy = a.y - b.y;
        return dx * dx + dy * dy;
    }
    
    class VisualDatabaseOpencvFacadeImpl{
    public:
        VisualDatabaseOpencvFacadeImpl(){
        }
        ~VisualDatabaseOpencvFacadeImpl(){
        }
        keypoint_map_t mKeyPointMap;
        descriptor_map_t mDescMap;
        size_map_t mSizeMap;
        image_map_t mDbImages;
        
        std::vector<cv::KeyPoint> mQueryKeyPoints;
        cv::Mat mQueryDesc;
        int matchedId;
        matches_t mMatchedInliers;
    };
    
    VisualDatabaseOpencvFacade::VisualDatabaseOpencvFacade(){
        mVisualDbImpl.reset(new VisualDatabaseOpencvFacadeImpl());
    }
    VisualDatabaseOpencvFacade::~VisualDatabaseOpencvFacade(){
        
    }
    
    void VisualDatabaseOpencvFacade::addImage(unsigned char* grayImage, size_t width, size_t height, int image_id) {
        cv::Mat img1 = cv::Mat((int)height,(int)width,CV_8UC1,grayImage);
        
        std::vector<cv::KeyPoint> keypoints1;
        //cv::SiftFeatureDetector detector;
        //detector.detect( img1, keypoints1 );

        AKAZE_FEATURES akaze;
        akaze.detectFeaturePoints(img1, 0, keypoints1);
        mVisualDbImpl->mKeyPointMap[image_id] = keypoints1;
        
        cv::FREAK extractor;
        //cv::SiftDescriptorExtractor extractor;
        cv::Mat descriptors1;
        extractor.compute( img1, keypoints1, descriptors1 );
        mVisualDbImpl->mDescMap[image_id] = descriptors1;

        mVisualDbImpl->mSizeMap[image_id] = cv::Size((int)width,(int)height);
        mVisualDbImpl->mDbImages[image_id] = img1.clone();
    }
    
    bool VisualDatabaseOpencvFacade::query(unsigned char* grayImage, size_t width, size_t height){
        
        cv::Mat img1 = cv::Mat((int)height,(int)width,CV_8UC1,grayImage);
        //cv::SiftFeatureDetector detector;
        //detector.detect( img1, mVisualDbImpl->mQueryKeyPoints );
        
        AKAZE_FEATURES akaze;
        akaze.detectFeaturePoints(img1, 0, mVisualDbImpl->mQueryKeyPoints);
        
        cv::FREAK extractor;
        //cv::SiftDescriptorExtractor extractor;
        extractor.compute( img1, mVisualDbImpl->mQueryKeyPoints, mVisualDbImpl->mQueryDesc );
        
        mVisualDbImpl->matchedId = -1;
        
        typename keypoint_map_t::const_iterator it = mVisualDbImpl->mKeyPointMap.begin();
        for(; it != mVisualDbImpl->mKeyPointMap.end(); it++) {
            
            cv::BFMatcher matcher(cv::NORM_HAMMING, true);
            std::vector<cv::DMatch> matches;
            matcher.match(mVisualDbImpl->mQueryDesc, mVisualDbImpl->mDescMap[it->first], matches);
            
            /*double max_dist = 0; double min_dist = 10000;
            for( int i = 0; i < matches.size(); i++ ){
                double dist = matches[i].distance;
                if( dist < min_dist ) min_dist = dist;
                if( dist > max_dist ) max_dist = dist;
            }*/
            
            std::vector< cv::DMatch > good_matches;
            for( int i = 0; i < matches.size(); i++ ){
                //if( matches[i].distance < 3*min_dist )
                good_matches.push_back( matches[i]);
            }
            
            /*cv::Mat img_matches;
            cv::drawMatches( img1, mVisualDbImpl->mQueryKeyPoints,mVisualDbImpl->mDbImages[it->first], it->second,
                            good_matches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1),
                            std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            std::stringstream s;
            s<<"/Users/nalinsenthamil/daqri/src/vision_apps/artoolkit-daqri/bin/DataSet/pinball_out_"<<fileId<<".png";
            cv::imwrite(s.str(), img_matches);
            fileId++;*/
            
            std::vector<cv::Point2f> queryPts;
            std::vector<cv::Point2f> dbPts;
            for( int i = 0; i < good_matches.size(); i++ ){
                queryPts.push_back( mVisualDbImpl->mQueryKeyPoints[good_matches[i].queryIdx].pt );
                dbPts.push_back( it->second[ good_matches[i].trainIdx ].pt );
            }
            
            if(good_matches.size() < 8 ) continue;
            
            cv::Mat_<float> Hmat = cv::findHomography(dbPts,queryPts,cv::FM_RANSAC);
            float H[9];
            H[0] = Hmat(0,0);
            H[1] = Hmat(0,1);
            H[2] = Hmat(0,2);
            H[3] = Hmat(1,0);
            H[4] = Hmat(1,1);
            H[5] = Hmat(1,2);
            H[6] = Hmat(2,0);
            H[7] = Hmat(2,1);
            H[8] = Hmat(2,2);
            
            std::vector<cv::Point2f> sourcePointsInFrame;
            cv::perspectiveTransform(dbPts, sourcePointsInFrame, Hmat);
            
            int visibleFeatures = 0;
            for (size_t i = 0; i < sourcePointsInFrame.size(); i++)
            {
                if (sourcePointsInFrame[i].x > 0 &&
                    sourcePointsInFrame[i].y > 0 &&
                    sourcePointsInFrame[i].x < img1.cols &&
                    sourcePointsInFrame[i].y < img1.rows)
                {
                    visibleFeatures++;
                }
            }
            
            int correctMatches = 0;
            for (size_t i = 0; i < good_matches.size(); i++)
            {
                const cv::Point2f& expected = sourcePointsInFrame[i];
                const cv::Point2f& actual = queryPts[i];
                
                // Maximum difference is 3 pixels
                if (distance_squared(expected, actual) < 9.0)
                {
                    correctMatches++;
                }
            }
            
            std::cout << "visibleFeatures - " << visibleFeatures << ",correctMatches-" << correctMatches << std::endl;
            /*float threshold2 = 3*3;
            matches_t inliers;
            inliers.reserve(good_matches.size());
            for(int i=0;i<mask.size();i++){
                if(mask[i]){
                    inliers.push_back(match_t(good_matches[i].queryIdx,good_matches[i].trainIdx));
                }
            }
            std::cout << "inliers-" << inliers.size() << std::endl;
            if(inliers.size() >= 8 && inliers.size() > mVisualDbImpl->mMatchedInliers.size()) {
                //CopyVector9(mMatchedGeometry, H);
                mVisualDbImpl->mMatchedInliers.swap(inliers);
                mVisualDbImpl->matchedId = it->first;
            }*/
        }
        
        if(mVisualDbImpl->matchedId == -1)
            return false;
        return true;
    }
    
    bool VisualDatabaseOpencvFacade::erase(int image_id){
        typename keypoint_map_t::iterator it = mVisualDbImpl->mKeyPointMap.find(image_id);
        if(it == mVisualDbImpl->mKeyPointMap.end()) {
            return false;
        }
        mVisualDbImpl->mKeyPointMap.erase(it);
        
        typename descriptor_map_t::iterator it2 = mVisualDbImpl->mDescMap.find(image_id);
        if(it2 == mVisualDbImpl->mDescMap.end()) {
            return false;
        }
        mVisualDbImpl->mDescMap.erase(it2);
        
        typename size_map_t::iterator it3 = mVisualDbImpl->mSizeMap.find(image_id);
        if(it3 == mVisualDbImpl->mSizeMap.end()) {
            return false;
        }
        mVisualDbImpl->mSizeMap.erase(it3);
        return true;
    }
    
    const size_t VisualDatabaseOpencvFacade::databaseCount(){
        return mVisualDbImpl->mKeyPointMap.size();
    }
    
    int VisualDatabaseOpencvFacade::matchedId(){
        return mVisualDbImpl->matchedId;
    }
    
    const float* VisualDatabaseOpencvFacade::matchedGeometry(){
        return 0;
    }
    
    const std::vector<cv::KeyPoint> &VisualDatabaseOpencvFacade::getFeaturePoints(int image_id) const{
        typename keypoint_map_t::const_iterator it = mVisualDbImpl->mKeyPointMap.find(image_id);
        if(it != mVisualDbImpl->mKeyPointMap.end()) {
            return it->second;
        } else {
            return NullKeyPoints;
        }
    }
    
    const cv::Mat &VisualDatabaseOpencvFacade::getDescriptors(int image_id) const{
        typename descriptor_map_t::const_iterator it = mVisualDbImpl->mDescMap.find(image_id);
        if(it != mVisualDbImpl->mDescMap.end()) {
            return it->second;
        } else {
            return NullDescriptors;
        }
    }
    
    const std::vector<cv::KeyPoint> &VisualDatabaseOpencvFacade::getQueryFeaturePoints() const{
        return mVisualDbImpl->mQueryKeyPoints;
    }
    
    const cv::Mat &VisualDatabaseOpencvFacade::getQueryDescriptors() const{
        return mVisualDbImpl->mQueryDesc;
    }
    
    const matches_t& VisualDatabaseOpencvFacade::inliers() const{
        return mVisualDbImpl->mMatchedInliers;
    }
    
    int VisualDatabaseOpencvFacade::getWidth(int image_id) const{
        typename size_map_t::const_iterator it = mVisualDbImpl->mSizeMap.find(image_id);
        if(it != mVisualDbImpl->mSizeMap.end()) {
            return it->second.width;
        } else {
            return -1;
        }
    }
    
    int VisualDatabaseOpencvFacade::getHeight(int image_id) const{
        typename size_map_t::const_iterator it = mVisualDbImpl->mSizeMap.find(image_id);
        if(it != mVisualDbImpl->mSizeMap.end()) {
            return it->second.height;
        } else {
            return -1;
        }
    }
} // vision