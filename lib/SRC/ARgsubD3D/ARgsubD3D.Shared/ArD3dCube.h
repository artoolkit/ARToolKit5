/*
*  ArD3dCube.h
*  ARToolKit5
*
*  Colored 3D cube.
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

#include "ShaderConstantsDefinitons.h"
#include "ArD3dIObject.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace ARgsubD3D
{
	public ref class ArD3dCube sealed : ArD3dIObject
	{
	public:
		ArD3dCube();

		property float Pitch;
		property float Yaw;
		property float Roll;
		property float Scale;
		property float TranslateX;
		property float TranslateY;
		property float TranslateZ;

		virtual bool Setup(ArD3dDeviceContext^ context);
		virtual bool Display(ArD3dDeviceContext^ context, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16);
	
	private:
		ComPtr<ID3D11VertexShader>          _vertexShader;
		ComPtr<ID3D11PixelShader>           _pixelShader;
		ComPtr<ID3D11PixelShader>           _pixelShaderSolidColor;
		ComPtr<ID3D11InputLayout>           _inputLayout;
		ComPtr<ID3D11Buffer>                _vertexBuffer;
		ComPtr<ID3D11Buffer>                _indexBuffer;
		ComPtr<ID3D11Buffer>                _constantBuffer;
		ComPtr<ID3D11Buffer>                _constantBufferSolid;

		ModelViewProjectionConstantBuffer	_constantBufferData;
		SolidColorConstantBuffer			_constantBufferDataSolidPs;
		uint32								_indexCount;
		bool								_isInitialized;
	};
}
