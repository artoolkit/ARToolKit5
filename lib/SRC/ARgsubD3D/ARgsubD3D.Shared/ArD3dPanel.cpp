/*
*  ArD3dPanel.cpp
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

#include "pch.h"
#include "ArD3dPanel.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace ARgsubD3D;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Concurrency;
using namespace DirectX;
using namespace D2D1;

ArD3dPanel::ArD3dPanel() :
_isInitialized(false),
_alphaMode(DXGI_ALPHA_MODE_UNSPECIFIED), // Default to ignore alpha, which can provide better performance if transparency is not required.
_compositionScaleX(1.0f),
_compositionScaleY(1.0f),
_height(1.0f),
_width(1.0f),
_context(nullptr)
{
	this->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &ArD3dPanel::OnSizeChanged);
	this->CompositionScaleChanged += ref new Windows::Foundation::TypedEventHandler<SwapChainPanel^, Object^>(this, &ArD3dPanel::OnCompositionScaleChanged);
	Application::Current->Suspending += ref new SuspendingEventHandler(this, &ArD3dPanel::OnSuspending);
	Application::Current->Resuming += ref new EventHandler<Object^>(this, &ArD3dPanel::OnResuming);
}

void ArD3dPanel::Context::set(ArD3dDeviceContext^ context)
{
	_context = context;
	CreateDeviceResources();
	CreateSizeDependentResources();
}

void ArD3dPanel::CreateDeviceResources()
{
	if (_context == nullptr)
	{
		return;
	}
	
	_context->D2dContext->CreateSolidColorBrush(ColorF(ColorF::Black), &_strokeBrush);

	_isInitialized = true;
}

void ArD3dPanel::CreateSizeDependentResources()
{
	if (_context == nullptr)
	{
		return;
	}

	// Ensure dependent objects have been released.
	_renderTargetView = nullptr;
	_depthStencilView = nullptr;
	_context->D2dContext->SetTarget(nullptr);
	_d2dTargetBitmap = nullptr;
	_context->D3dContext->OMSetRenderTargets(0, nullptr, nullptr);
	_context->D3dContext->Flush();

	// Set render target size to the rendered size of the panel including the composition scale, 
	// defaulting to the minimum of 1px if no size was specified.
	_renderTargetWidth = _width *_compositionScaleX;
	_renderTargetHeight = _height *_compositionScaleY;

	// If the swap chain already exists, then resize it.
	if (_swapChain != nullptr)
	{
		HRESULT hr = _swapChain->ResizeBuffers(2, static_cast<UINT>(_renderTargetWidth), static_cast<UINT>(_renderTargetHeight), DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();
			return;
		}
		else
		{
			ThrowIfFailed(hr);
		}
	}
	else // Otherwise, create a new one.
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
		swapChainDesc.Width = static_cast<UINT>(_renderTargetWidth);      // Match the size of the panel.
		swapChainDesc.Height = static_cast<UINT>(_renderTargetHeight);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                  // This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;                                 // Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;                                      // Use double buffering to enable flip.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;        // All Windows Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;
		swapChainDesc.AlphaMode = _alphaMode;

		// Get underlying DXGI Device from D3D Device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		ThrowIfFailed(_context->D3dDevice.As(&dxgiDevice));

		// Get adapter.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

		// Get factory.
		ComPtr<IDXGIFactory2> dxgiFactory;
		ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGISwapChain1> swapChain;
		// Create swap chain.
		ThrowIfFailed(dxgiFactory->CreateSwapChainForComposition(_context->D3dDevice.Get(), &swapChainDesc, nullptr, &swapChain));
		swapChain.As(&_swapChain);

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces 
		// latency and ensures that the application will only render after each VSync, minimizing 
		// power consumption.
		ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));

		Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
		{
			//Get backing native interface for SwapChainPanel.
			ComPtr<ISwapChainPanelNative> panelNative;
			ThrowIfFailed(reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&panelNative)));

			// Associate swap chain with SwapChainPanel.  This must be done on the UI thread.
			ThrowIfFailed(panelNative->SetSwapChain(_swapChain.Get()));
		}, CallbackContext::Any));
	}

	// Ensure the physical pixel size of the swap chain takes into account both the XAML SwapChainPanel's logical layout size and 
	// any cumulative composition scale applied due to zooming, render transforms, or the system's current scaling plateau.
	// For example, if a 100x100 SwapChainPanel has a cumulative 2x scale transform applied, we instead create a 200x200 swap chain 
	// to avoid artifacts from scaling it up by 2x, then apply an inverse 1/2x transform to the swap chain to cancel out the 2x transform.
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1.0f / _compositionScaleX;
	inverseScale._22 = 1.0f / _compositionScaleY;

	_swapChain->SetMatrixTransform(&inverseScale);

	// Init D3D
	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

	// Create render target view.
	ThrowIfFailed(_context->D3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &_renderTargetView));

	// Create and set viewport.
	D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, _renderTargetWidth, _renderTargetHeight);
	_context->D3dContext->RSSetViewports(1, &viewport);

	// Create depth/stencil buffer descriptor.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, static_cast<UINT>(_renderTargetWidth), static_cast<UINT>(_renderTargetHeight), 1, 1, D3D11_BIND_DEPTH_STENCIL);

	// Allocate a 2-D surface as the depth/stencil buffer.
	ComPtr<ID3D11Texture2D> depthStencil;
	ThrowIfFailed(_context->D3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil));

	// Create depth/stencil view based on depth/stencil buffer.
	const CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
	ThrowIfFailed(_context->D3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, &_depthStencilView));

	// Init D2D
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
		DIPS_PER_INCH * _compositionScaleX,
		DIPS_PER_INCH * _compositionScaleY);

	// Direct2D needs the DXGI version of the backbuffer surface pointer.
	ComPtr<IDXGISurface> dxgiBackBuffer;
	ThrowIfFailed(_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)));

	// Get a D2D surface from the DXGI back buffer to use as the D2D render target.
	ThrowIfFailed(_context->D2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bitmapProperties, &_d2dTargetBitmap));

	_context->D2dContext->SetDpi(DIPS_PER_INCH * _compositionScaleX, DIPS_PER_INCH * _compositionScaleY);
	_context->D2dContext->SetTarget(_d2dTargetBitmap.Get());
}

void ArD3dPanel::Present()
{
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;

	HRESULT hr = S_OK;

	hr = _swapChain->Present1(1, 0, &parameters);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		OnDeviceLost();
	}
	else
	{
		ThrowIfFailed(hr);
	}
}

void ArD3dPanel::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	critical_section::scoped_lock lock(_criticalSection);
	ComPtr<IDXGIDevice3> dxgiDevice;
	_context->D3dDevice.As(&dxgiDevice);

	// Hints to the driver that the app is entering an idle state and that its memory can be used temporarily for other apps.
	dxgiDevice->Trim();
}

void ArD3dPanel::OnResuming(Object^ sender, Object^ args)
{
}

void ArD3dPanel::OnSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	if (_width != e->NewSize.Width || _height != e->NewSize.Height)
	{
		critical_section::scoped_lock lock(_criticalSection);

		// Store values so they can be accessed from a background thread.
		_width = max(e->NewSize.Width, 1.0f);
		_height = max(e->NewSize.Height, 1.0f);

		// Recreate size-dependent resources when the panel's size changes.
		CreateSizeDependentResources();
	}
}

void ArD3dPanel::OnCompositionScaleChanged(SwapChainPanel ^sender, Object ^args)
{
	if (_compositionScaleX != CompositionScaleX || _compositionScaleY != CompositionScaleY)
	{
		critical_section::scoped_lock lock(_criticalSection);

		// Store values so they can be accessed from a background thread.
		_compositionScaleX = this->CompositionScaleX;
		_compositionScaleY = this->CompositionScaleY;

		// Recreate size-dependent resources when the composition scale changes.
		CreateSizeDependentResources();
	}
}

void ArD3dPanel::OnDeviceLost()
{
	_isInitialized = false;
	_swapChain = nullptr;
	_d2dTargetBitmap = nullptr;

	_context->OnDeviceLost();

	CreateDeviceResources();
	CreateSizeDependentResources();
}

bool ArD3dPanel::BeginDraw(bool shouldClear)
{
	if (!_isInitialized)
	{
		return false;
	}

	if (shouldClear)
	{
		_context->D3dContext->ClearRenderTargetView(_renderTargetView.Get(), DirectX::Colors::Transparent);
		_context->D3dContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	return true;
}

bool ArD3dPanel::DisplayObject(ArD3dIObject^ object, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16)
{
	return DisplayObject(object, arModelViewMatrix16, arProjectionMatrix16, true, true);
}

bool ArD3dPanel::DisplayObject(ArD3dIObject^ object, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16, bool shouldRenderColor, bool shouldRenderDepth)
{
	if (!_isInitialized)
	{
		return false;
	}

	// Store previous render targets
	ID3D11RenderTargetView* prevRenderTargets[1];
	ID3D11DepthStencilView* prevDepthStencil;
	_context->D3dContext->OMGetRenderTargets(1, prevRenderTargets, &prevDepthStencil);
	
	// Set desired render targets
	ID3D11RenderTargetView* renderTargets[1] = { nullptr };
	if (shouldRenderColor)
	{
		renderTargets[0] = _renderTargetView.Get();
	}
	ID3D11DepthStencilView* depthStencil = nullptr;
	if (shouldRenderDepth)
	{
		depthStencil = _depthStencilView.Get();
	}
	_context->D3dContext->OMSetRenderTargets(1, renderTargets, depthStencil);

	// Render
	object->Display(_context, arModelViewMatrix16, arProjectionMatrix16);

	// Restore previous render targets
	_context->D3dContext->OMSetRenderTargets(1, prevRenderTargets, prevDepthStencil);

	return true;
}

bool ArD3dPanel::EndDraw()
{
	if (!_isInitialized)
	{
		return false;
	}

	Present();
	return true;
}

bool ArD3dPanel::DisplayImage(ArD3dImage^ image, const Platform::Array<unsigned int>^ imageData)
{
	if (!_isInitialized)
	{
		return false;
	}

	// Draw!
	auto dstRectScr = D2D1_RECT_F{ 0, 0, _renderTargetWidth, _renderTargetHeight };
	_context->D2dContext->BeginDraw();
	image->Display(_context, imageData->Data, _renderTargetWidth, _renderTargetHeight);
	_context->D2dContext->EndDraw();

	return true;
}
