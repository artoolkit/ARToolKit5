/*
*  ArD3dPanel.h
*  ARToolKit5
*
*  Sample of a custom SwapChainPanel used to display the ARToolKit video frame and render 3D objects.
*
*  This file is part of ARToolKit.
*  Partly based on official Microsoft DX samples.
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

#pragma once

#include "ArD3dImage.h"
#include "ArD3dIObject.h"
#include "ArD3dCube.h"
#include "ArD3dDeviceContext.h"

using namespace Microsoft::WRL;

#define DIPS_PER_INCH 96.0f

namespace ARgsubD3D
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class ArD3dPanel sealed : public Windows::UI::Xaml::Controls::SwapChainPanel
	{
	public:
		ArD3dPanel();

		property ArD3dDeviceContext^ Context
		{
			ArD3dDeviceContext^ get() { return _context; }
			void set(ArD3dDeviceContext^);
		}
		property float DpiX	{ float get() { return DIPS_PER_INCH * _compositionScaleX; } }
		property float DpiY { float get() { return DIPS_PER_INCH * _compositionScaleY; } }
		
		bool BeginDraw(bool shouldClear);
		bool DisplayObject(ArD3dIObject^ object, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16);
		bool DisplayObject(ArD3dIObject^ object, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16, bool shouldRenderColor, bool shouldRenderDepth);
		bool EndDraw();
		bool DisplayImage(ArD3dImage^ image, const Platform::Array<unsigned int>^ imageData);

	private:
		void CreateDeviceResources();
		void CreateSizeDependentResources();
		void Present();

		void OnDeviceLost();
		void OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^sender, Platform::Object ^args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);


		ArD3dDeviceContext^													_context;
		ComPtr<IDXGISwapChain2>												_swapChain;
		ComPtr<ID3D11RenderTargetView>										_renderTargetView;
		ComPtr<ID3D11DepthStencilView>										_depthStencilView;

		ComPtr<ID2D1Bitmap1>												_d2dTargetBitmap;
		DXGI_ALPHA_MODE                                                     _alphaMode;

		ComPtr<ID2D1SolidColorBrush>										_strokeBrush;

		bool                                                                _isInitialized;

		Concurrency::critical_section                                       _criticalSection;

		float                                                               _renderTargetHeight;
		float                                                               _renderTargetWidth;

		float                                                               _compositionScaleX;
		float                                                               _compositionScaleY;

		float                                                               _height;
		float                                                               _width;
	};
}
