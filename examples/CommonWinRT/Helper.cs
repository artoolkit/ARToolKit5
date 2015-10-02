/* 
 *  Helper.cs
 *  ARToolKit5
 *
 *  Common helper methods for the samples.
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

using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Windows.Devices.Enumeration;
using Windows.Storage;

namespace ArWinRtSamples
{
    public static class Helper
    {
        public static void Log(string message)
        {
            Debug.WriteLine(message);
        }

        public static void Log(string message, params object[] args)
        {
            Debug.WriteLine(message, args);
        }

        public static async Task<bool> RestoreSnapshot(uint[] videoImageData, float[] markerMatrix, string fileName = "snapshot.bin", int bytesPerPixel = 4)
        {
            var files = await ApplicationData.Current.LocalFolder.GetFilesAsync();
            var file = files.FirstOrDefault(f => f.Name == fileName);
            if (file != null)
            {
                var bufferByteArray = new byte[(videoImageData.Length + markerMatrix.Length) * bytesPerPixel];
                using (var readStream = await file.OpenStreamForReadAsync())
                {
                    await readStream.ReadAsync(bufferByteArray, 0, bufferByteArray.Length);
                }
                Buffer.BlockCopy(bufferByteArray, 0, videoImageData, 0, videoImageData.Length * bytesPerPixel);
                Buffer.BlockCopy(bufferByteArray, videoImageData.Length * bytesPerPixel, markerMatrix, 0, markerMatrix.Length * bytesPerPixel);
                return true;
            }
            return false;
        }

        public static async Task StoreSnapshot(uint[] videoImageData, float[] markerMat, string fileName = "snapshot.bin", int bytesPerPixel = 4)
        {
            var bufferByteArray = new byte[(videoImageData.Length + markerMat.Length) * bytesPerPixel];
            Buffer.BlockCopy(videoImageData, 0, bufferByteArray, 0, videoImageData.Length * bytesPerPixel);
            Buffer.BlockCopy(markerMat, 0, bufferByteArray, videoImageData.Length * bytesPerPixel, markerMat.Length * bytesPerPixel);
            var file = await ApplicationData.Current.LocalFolder.CreateFileAsync(fileName, CreationCollisionOption.ReplaceExisting);
            using (var writeStream = await file.OpenStreamForWriteAsync())
            {
                await writeStream.WriteAsync(bufferByteArray, 0, bufferByteArray.Length);
            }
        }

        public static async Task<string> GetDefaultVideoDeviceLocation(Panel preferredEnclosureLocation = Panel.Back)
        {
            // Get all video capture device information
            var devices = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);

            // Try to get the preferred location first
            var preferredDevice = devices.FirstOrDefault(x => x.EnclosureLocation != null && x.EnclosureLocation.Panel == preferredEnclosureLocation);
            
            // if the preferred one is not available, try to get the back camera
            if (preferredDevice == null)
            {
                preferredDevice = devices.FirstOrDefault(x => x.EnclosureLocation != null && x.EnclosureLocation.Panel == Panel.Back);
            }
            // if back one is not available, try to get the front camera
            if (preferredDevice == null)
            {
                preferredDevice = devices.FirstOrDefault(x => x.EnclosureLocation != null && x.EnclosureLocation.Panel == Panel.Front);
            }
            // if neither back nor front one is available, just use the first one
            if (preferredDevice == null)
            {
                preferredDevice = devices.FirstOrDefault(x => x.EnclosureLocation != null);
            }

            return preferredDevice == null ? "default" : preferredDevice.EnclosureLocation.Panel.ToString().ToLower();
        }
    }
}