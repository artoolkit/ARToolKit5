/*
*  ArD3dDeviceContext.cpp
*  ARToolKit5
*
*  Wrapper for D3D and D2D device and context so it can be composed via WinRT.
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
#include "ArD3dDeviceContext.h"

using namespace ARgsubD3D;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace DirectX;
using namespace D2D1;

ArD3dDeviceContext::ArD3dDeviceContext()
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

void ArD3dDeviceContext::CreateDeviceIndependentResources()
{
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

// Commented out by default to prevent incompat issues on Windows 10 machines
//#if defined(_DEBUG)
//	// Enable D2D debugging via SDK Layers when in debug mode.
//	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
//#endif

	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), &options, &_d2dFactory));
}

void ArD3dDeviceContext::CreateDeviceResources()
{
	// This flag adds support for surfaces with a different color channel ordering than the API default.
	// It is recommended usage, and is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

// Commented out by default to prevent incompat issues on Windows 10 machines
//#if defined(_DEBUG)
//	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
//	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the DX11 API device object, and get a corresponding context.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ThrowIfFailed(
		D3D11CreateDevice(
		nullptr,                    // Specify null to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlags,              // Optionally set debug and Direct2D compatibility flags.
		featureLevels,              // List of feature levels this app can support.
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&device,                    // Returns the Direct3D device created.
		NULL,                       // Returns feature level of device created.
		&context                    // Returns the device immediate context.
		));

	// Get D3D11.1 device
	ThrowIfFailed(device.As(&_d3dDevice));

	// Get D3D11.1 context
	ThrowIfFailed(context.As(&_d3dContext));

	// Get underlying DXGI device of D3D device
	ComPtr<IDXGIDevice> dxgiDevice;
	ThrowIfFailed(_d3dDevice.As(&dxgiDevice));

	// Get D2D device
	ThrowIfFailed(_d2dFactory->CreateDevice(dxgiDevice.Get(), &_d2dDevice));

	// Get D2D context
	ThrowIfFailed(_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dContext));

	// Set D2D text anti-alias mode to Grayscale to ensure proper rendering of text on intermediate surfaces.
	_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	// Set D2D's unit mode to pixels so that drawing operation units are interpreted in pixels rather than DIPS. 
	_d2dContext->SetUnitMode(D2D1_UNIT_MODE::D2D1_UNIT_MODE_PIXELS);
}

void ArD3dDeviceContext::OnDeviceLost()
{
	_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

	_d2dContext->SetTarget(nullptr);

	_d2dContext = nullptr;
	_d2dDevice = nullptr;

	_d3dContext->Flush();

	CreateDeviceResources();
}