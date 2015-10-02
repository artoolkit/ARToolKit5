/*
*  Helper.h
*  ARToolKit5
*
*  Various helper functions for dealing with WinRT and D3D / D2D.
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

#pragma once


namespace ARgsubD3D
{
	// Throw a platform exception from an HRESULT
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	// Convert ARToolkit right-handed matrix into a D3D still right-handed XMMATRIX
	inline DirectX::XMMATRIX ArRhToD3dRhMatrix(const float* armatrix)
	{
		return DirectX::XMMATRIX(armatrix);
	}

	// Convert ARToolkit right-handed matrix into a D3D left-handed XMMATRIX
	inline DirectX::XMMATRIX ArRhToD3dLhMatrix(const float* armatrix)
	{
		auto m = armatrix;
		DirectX::XMMATRIX result(
			  m[0],   m[1],  -m[2],  m[3],
			  m[4],   m[5],  -m[6],  m[7],
			 -m[8],  -m[9],  m[10], -m[11],
			 m[12],  m[13], -m[14],  m[15]);
		
		return result;
	}
}