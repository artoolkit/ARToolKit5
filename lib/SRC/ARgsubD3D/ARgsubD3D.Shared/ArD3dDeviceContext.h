/*
*  ArD3dDeviceContext.h
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

#pragma once

using namespace Microsoft::WRL;
using namespace DirectX;

namespace ARgsubD3D
{
	public ref class ArD3dDeviceContext sealed
	{
	public:
		ArD3dDeviceContext();

	internal:
		property ComPtr<ID3D11Device1> D3dDevice
		{
			ComPtr<ID3D11Device1> get() { return _d3dDevice; }
		}
		property ComPtr<ID3D11DeviceContext1> D3dContext
		{
			ComPtr<ID3D11DeviceContext1> get() { return _d3dContext; }
		}
		property ComPtr<ID2D1Factory2> D2dFactory
		{
			ComPtr<ID2D1Factory2> get() { return _d2dFactory; }
		}
		property ComPtr<ID2D1Device> D2dDevice
		{
			ComPtr<ID2D1Device> get() { return _d2dDevice; }
		}
		property ComPtr<ID2D1DeviceContext> D2dContext
		{
			ComPtr<ID2D1DeviceContext> get() { return _d2dContext; }
		}

		void OnDeviceLost();

	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();

		ComPtr<ID3D11Device1>                               _d3dDevice;
		ComPtr<ID3D11DeviceContext1>                        _d3dContext;
		ComPtr<ID2D1Factory2>                               _d2dFactory;
		ComPtr<ID2D1Device>                                 _d2dDevice;
		ComPtr<ID2D1DeviceContext>                          _d2dContext;
	};
}