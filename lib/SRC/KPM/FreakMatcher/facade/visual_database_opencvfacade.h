//
//  visual_database_opencvfacade.h
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

#include <memory>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <matchers/matcher_types.h>
namespace vision {

    class VisualDatabaseOpencvFacadeImpl;
    
    class VisualDatabaseOpencvFacade {
    public:
        
        VisualDatabaseOpencvFacade();
        ~VisualDatabaseOpencvFacade();
        
        void addImage(unsigned char* grayImage, size_t width, size_t height, int image_id);
        
        bool query(unsigned char* grayImage, size_t width, size_t height) ;
        
        
        bool erase(int image_id);
        
        const size_t databaseCount();
        
        int matchedId() ;
        
        const float* matchedGeometry();
        
        const std::vector<cv::KeyPoint>& getFeaturePoints(int image_id) const;
        
        const cv::Mat& getDescriptors(int image_id) const;
        
        int getWidth(int image_id) const;
        int getHeight(int image_id) const;
        
        
        const std::vector<cv::KeyPoint>& getQueryFeaturePoints() const;
        
        const cv::Mat& getQueryDescriptors() const;
        
        const matches_t& inliers() const;
        
    private:
        std::unique_ptr<VisualDatabaseOpencvFacadeImpl> mVisualDbImpl;
    }; // VisualDatabaseFacade
    
} // vision
