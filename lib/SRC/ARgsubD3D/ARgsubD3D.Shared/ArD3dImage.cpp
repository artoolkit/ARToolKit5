/*
*  ArD3dImage.cpp
*  ARToolKit5
*
*  Wrapper for D2D image mainly to render the video frame.
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
*
*  Author(s): Rene Schulte.
*
*/

#include "pch.h"
#include "ArD3dImage.h"

using namespace ARgsubD3D;
using namespace D2D1;
using namespace Microsoft::WRL;
using namespace DirectX;

ArD3dImage::ArD3dImage() { }

bool ArD3dImage::Setup(ArD3dDeviceContext^ context, int imageWidth, int imageHeight, Platform::String^ arPixelFormat, int bytesPerPixel, float dpiX, float dpiY)
{
	if (context == nullptr)
	{
		return false;
	}
	if (_drawBitmap == nullptr || _width != imageWidth || _height != imageHeight || _dpiX != dpiX || _dpiY != dpiY)
	{
		// We only support BGRA as R8G8B8A8_UNORM!
		if (bytesPerPixel != 4 || !wcsstr(arPixelFormat->Data(), L"BGRA"))
		{
			throw Platform::Exception::CreateException(E_INVALIDARG, L"ArD3dImage only supports the AR_PIXEL_FORMAT_BGRA.");
			return false;
		}

		// Create bitmap we want to draw the video frame into
		auto bitmapProperties = BitmapProperties(PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);
		ThrowIfFailed(context->D2dContext->CreateBitmap(SizeU(imageWidth, imageHeight), bitmapProperties, &_drawBitmap));

		_width = imageWidth;
		_height = imageHeight;
		_dpiX = dpiX;
		_dpiY = dpiY;
		_bytesPerPixel = bytesPerPixel;

		return true;
	}
	return false;
}

bool ArD3dImage::Display(ArD3dDeviceContext^ context, unsigned int* image, float targetWidth, float targetHeight)
{
	if (context == nullptr || _drawBitmap == nullptr)
	{
		return false;
	}

	// Update bitmap data
	auto dstRect = D2D1_RECT_U{ 0, 0, _width, _height };
	ThrowIfFailed(_drawBitmap->CopyFromMemory(&dstRect, image, _width * _bytesPerPixel));

	// Draw!
	auto dstRectScr = D2D1_RECT_F{ 0, 0, targetWidth, targetHeight };
	context->D2dContext->DrawBitmap(_drawBitmap.Get(), &dstRectScr);

	return true;
}