/*
*  ArD3dCube.cpp
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

#include "pch.h"
#include "ArD3dCube.h"

using namespace ARgsubD3D;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Concurrency;
using namespace DirectX;
using namespace D2D1;

ArD3dCube::ArD3dCube() :
_isInitialized(false)
{
	Yaw = 0.0f;
	Pitch = 0.0f;
	Roll = 0.0f;
	TranslateX = 0.0f;  
	TranslateY = 0.0f;  
	TranslateZ = 0.5f;  // Place base of cube on marker surface.
	Scale = 40.0f;
}

bool ArD3dCube::Setup(ArD3dDeviceContext^ context)
{
	if (context == nullptr)
	{
		return false;
	}

	if (!_isInitialized)
	{
		// Asynchronously load vertex shader and create input layout.
		auto loadVSTask = ReadDataAsync(L"ARgsubD3D\\SimpleVertexShader.cso");
		auto createVSTask = loadVSTask.then([this, context](const std::vector<byte>& fileData)
		{
			ThrowIfFailed(context->D3dDevice->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &_vertexShader));

			static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			ThrowIfFailed(context->D3dDevice->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &_inputLayout));
		});

		// Asynchronously load vertex shader and create constant buffer.
		auto loadPSTask = ReadDataAsync(L"ARgsubD3D\\SimplePixelShader.cso");
		auto createPSTask = loadPSTask.then([this, context](const std::vector<byte>& fileData)
		{
			ThrowIfFailed(context->D3dDevice->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &_pixelShader));
			CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
			ThrowIfFailed(context->D3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &_constantBuffer));
		});
		auto loadPSTask2 = ReadDataAsync(L"ARgsubD3D\\SolidColorPixelShader.cso");
		auto createPSTask2 = loadPSTask2.then([this, context](const std::vector<byte>& fileData)
		{
			ThrowIfFailed(context->D3dDevice->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &_pixelShaderSolidColor));
			XMStoreFloat4(&_constantBufferDataSolidPs.color, DirectX::Colors::DarkSlateBlue);
			CD3D11_BUFFER_DESC constantBufferDesc(sizeof(SolidColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
			ThrowIfFailed(context->D3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &_constantBufferSolid));
		});

		// Once both shaders are loaded, create the mesh.
		auto createCubeTask = (createPSTask2 && createPSTask && createVSTask).then([this, context]()
		{
			// Load mesh vertices. Each vertex has a position and a color.
			static const VertexPositionColor cubeVertices[] =
			{
				{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
				{ XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
				{ XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
				{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
				{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
				{ XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
				{ XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
				{ XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
			};

			D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
			vertexBufferData.pSysMem = cubeVertices;
			vertexBufferData.SysMemPitch = 0;
			vertexBufferData.SysMemSlicePitch = 0;
			CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
			ThrowIfFailed(context->D3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &_vertexBuffer));

			// Load mesh indices. Each triple of indices represents
			// a triangle to be rendered on the screen.
			// For example, 0,2,1 means that the vertices with indexes
			// 0, 2 and 1 from the vertex buffer compose the 
			// first triangle of this mesh.
			static const unsigned short cubeIndices[] =
			{
				0, 2, 1, // -x
				1, 2, 3,

				4, 5, 6, // +x
				5, 7, 6,

				0, 1, 5, // -y
				0, 5, 4,

				2, 6, 7, // +y
				2, 7, 3,

				0, 4, 6, // -z
				0, 6, 2,

				1, 3, 7, // +z
				1, 7, 5,
			};

			_indexCount = ARRAYSIZE(cubeIndices);

			D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
			indexBufferData.pSysMem = cubeIndices;
			indexBufferData.SysMemPitch = 0;
			indexBufferData.SysMemSlicePitch = 0;
			CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
			ThrowIfFailed(context->D3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &_indexBuffer));
		});

		// Once the cube is loaded, the object is ready to be rendered.
		createCubeTask.then([this]() { _isInitialized = true;	});
	}

	return true;
}

bool ArD3dCube::Display(ArD3dDeviceContext^ context, const Platform::Array<float32>^ arModelViewMatrix16, const Platform::Array<float32>^ arProjectionMatrix16)
{
	if (!_isInitialized || context == nullptr)
	{
		return false;
	}
	auto sf = Scale;
	auto scaleMat = XMMatrixScaling(sf, sf, sf);
	auto rotMat = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);
	auto translateMat = XMMatrixTranslation(TranslateX, TranslateY, TranslateZ);
	auto mvMat = ArRhToD3dRhMatrix(arModelViewMatrix16->Data);
	auto modelView = XMMatrixMultiply(XMMatrixMultiply(rotMat, XMMatrixMultiply(translateMat, scaleMat)), mvMat);
	auto projMat = ArRhToD3dRhMatrix(arProjectionMatrix16->Data);

	// Prepare to pass the modelView and projection matrix to the shader
	XMStoreFloat4x4(&_constantBufferData.modelView, XMMatrixTranspose(modelView));
	XMStoreFloat4x4(&_constantBufferData.projection, XMMatrixTranspose(projMat));
	context->D3dContext->UpdateSubresource(_constantBuffer.Get(), 0, NULL, &_constantBufferData, 0, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->D3dContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);

	// Each index is one 16-bit unsigned integer (short).
	context->D3dContext->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->D3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->D3dContext->IASetInputLayout(_inputLayout.Get());

	// Attach our vertex shader and send the constant buffer to the Graphics device.
	context->D3dContext->VSSetShader(_vertexShader.Get(), nullptr, 0);
	context->D3dContext->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());

	// Attach our pixel shader.
	context->D3dContext->PSSetShader(_pixelShader.Get(), nullptr, 0);

	// Draw the objects.
	context->D3dContext->DrawIndexed(_indexCount, 0, 0);

	// Overlay with white wireframe outlines
	context->D3dContext->UpdateSubresource(_constantBufferSolid.Get(), 0, NULL, &_constantBufferDataSolidPs, 0, 0);
	context->D3dContext->PSSetConstantBuffers(0, 1, _constantBufferSolid.GetAddressOf());
	context->D3dContext->PSSetShader(_pixelShaderSolidColor.Get(), nullptr, 0);
	context->D3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	context->D3dContext->DrawIndexed(_indexCount, 0, 0);

	return true;
}