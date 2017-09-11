/* 
 *  MainPage.xaml.cs
 *  ARToolKit5
 *
 *  The MainPage for the simpleLiteWinRT sample app.
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
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.System;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Navigation;
using ArWinRtSamples;
using ARgsubD3D;
using ARToolKitComponent;

namespace simpleLiteWinRT
{
    public sealed partial class MainPage
    {
        private const int CurrentTemplateSize = 16;
        private const int CurrentTemplateCountMax = 25;
        private const float NearPlane = 40.0f;
        private const float FarPlane = 1000.0f;
        private const int DesiredVideoWidth = 640;
        private const int DesiredVideoHeight = 480;
        private ARWrapper _arWrapper;
        private ArD3dCube _cube;
        private ArD3dImage _videoImage;

        private int _markerId;
        private bool _isRunning;
        private bool _wasStarted;
        private uint[] _bufferAr;
        private float[] _projectionMatrix;
        private float[] _markerModelViewMatrix;
        private CancellationTokenSource _processingTaskCancellationTokenSource;

        // Key handling
        private bool _shouldRotateCube;
        private ArWrapperViewModel _arWrapperViewModel;

        public MainPage()
        {
            InitializeComponent();
            NavigationCacheMode = NavigationCacheMode.Required;
#if !DEBUG
            DbgTxt.Visibility = Visibility.Collapsed;
#endif
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            await Setup();
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            Cleanup();
        }

        private async Task Setup()
        {
            // Initialize ARToolKitComponent wrapper
            _arWrapper = new ARWrapper();

            // Setup the logging delegate to log the message from ARToolKit and init ARToolKit
            _arWrapper.arwRegisterLogCallback(Helper.Log);
            if (!_arWrapper.arwInitialiseARWithOptions(CurrentTemplateSize, CurrentTemplateCountMax))
            {
                throw new InvalidOperationException("Failed to initialize ARToolKit.");
            }
            // Log the ARToolKit version
            string artVersion;
            if (!_arWrapper.arwGetARToolKitVersion(out artVersion))
            {
                throw new InvalidOperationException("Failed to get ARToolKit version.");
            }
            Helper.Log("ARToolkit version {0}", artVersion);

            // Init ViewModel for UI data binding. Use Adaptive image binarization mode as default
            _arWrapperViewModel = Resources["WrapperVm"] as ArWrapperViewModel;
            if (_arWrapperViewModel != null)
            {
                _arWrapperViewModel.ArWrapper = _arWrapper;
                _arWrapperViewModel.SelectedThresholdMode = _arWrapperViewModel.ThresholdModes.FirstOrDefault(t => t.Mode == ArThresholdMode.Manual);
                _arWrapperViewModel.ManualThresholdValue = 120;
            }

            // Load marker
            await SetupMarker();

            // Setup rendering members which store the projection, model-view matrix and the video frame buffer
            _projectionMatrix = new float[16];
            _markerModelViewMatrix = new float[16];
            _bufferAr = new uint[DesiredVideoWidth * DesiredVideoHeight];

            // Init the D3D device context wrapper and the used instances from ArgsubD3D
            ArD3dPanel.Context = new ArD3dDeviceContext();
            _cube = new ArD3dCube();
            _videoImage = new ArD3dImage();

            // Init video with calibration data and start the processing loop
            await Task.Run(async () =>
            {
                await StartCapturing();
                StartProcessing();
            });

            // Hook up windows key event handler
            Window.Current.CoreWindow.KeyUp += CoreWindowOnKeyUp;
        }

        private void Cleanup()
        {
            // Detach windows key event handler
            Window.Current.CoreWindow.KeyUp -= CoreWindowOnKeyUp;

            // Cancel the processing loop
            if (_processingTaskCancellationTokenSource != null)
            {
                _processingTaskCancellationTokenSource.Cancel();
            }

            // Stop ARToolKit and shutdown
            if (_arWrapper != null)
            {
                _isRunning = false;
                if (!_arWrapper.arwStopRunning())
                {
                    throw new InvalidOperationException("Failed to stop ARToolkit from running.");
                }
                if (!_arWrapper.arwShutdownAR())
                {
                    throw new InvalidOperationException("Failed to shutdown ARToolkit.");
                }
                _arWrapper = null;
            }
            _wasStarted = false;
        }

        private async Task SetupMarker()
        {
            // Load marker data which is deployed with the app in its app-local storage
            var markerData = await PathIO.ReadTextAsync("ms-appx:///data/hiro.patt");
            var cfg = string.Format("single_buffer;{0};buffer={1}", 80, markerData);

            // Add marker to ARToolKit and make sure it succeeded. 
            // The returned marker id is stored for reference.
            _markerId = _arWrapper.arwAddMarker(cfg);
            if (_markerId < 0)
            {
                throw new InvalidOperationException("Failed to load AR marker.");
            }
        }

        private async Task StartCapturing()
        {
            // Load video configuration which is deployed with the app in its app-local storage
            var cameraCalibrationData = await PathIO.ReadBufferAsync("ms-appx:///data/camera_para.dat");
            var desiredVideoCameraPosition = await Helper.GetDefaultVideoDeviceLocation();
            var videoConfiguration0 = string.Format("-module=WinMC -width={0} -height={1} -format=BGRA -position={2}", DesiredVideoWidth, DesiredVideoHeight, desiredVideoCameraPosition);

            // Start capturing by passing the the video config and the Near and Far plane for the projection
            _isRunning = _arWrapper.arwStartRunningB(videoConfiguration0, cameraCalibrationData.ToArray(), NearPlane, FarPlane);
            if (!_isRunning)
            {
                throw new InvalidOperationException("Failed to start ARToolKit.");
            }
        }

        private void StartProcessing()
        {
            // Start AR processing loop in separate task/thread to keep the UI thread more responsive
            // Pass a Task CancellationTokenSource in order to stop processing when desired
            _processingTaskCancellationTokenSource = new CancellationTokenSource();
            var cancellationToken = _processingTaskCancellationTokenSource.Token;
            Task.Run(() =>
            {
                while (!cancellationToken.IsCancellationRequested && _arWrapper != null && _isRunning)
                {
                    Update();
                }
            }, cancellationToken);
        }

        void Update()
        {
            if (_arWrapper == null || !_isRunning) return;

            // Tell ARToolKit to capture a frame from the video camera
            if (!_arWrapper.arwCapture()) return;

            // We need to change the threshold and debug mode here, otherwise a concurrent op can happen and UpdateAr will crash
            if (_arWrapperViewModel != null)
            {
                if (_arWrapper.arwGetVideoThresholdMode() != _arWrapperViewModel.SelectedThresholdMode.Mode)
                {
                    _arWrapper.arwSetVideoThresholdMode(_arWrapperViewModel.SelectedThresholdMode.Mode);
                }
                if (_arWrapper.arwGetVideoDebugMode() != _arWrapperViewModel.UseDebugMode)
                {
                    _arWrapper.arwSetVideoDebugMode(_arWrapperViewModel.UseDebugMode);
                }
            }

            // Run marker detection and update results 
            _arWrapper.arwUpdateAR();

            // Run the initialization which only works after the first ARToolKit arwUpdateAR marker detection was completed
            if (!_wasStarted)
            {
                // Get video camera parameter
                int width, height, pixelSize;
                string pixelFormat;
                if (!_arWrapper.arwGetVideoParams(out width, out height, out pixelSize, out pixelFormat))
                {
                    throw new InvalidOperationException("ARToolkit arwGetVideoParams failed.");
                }
                Helper.Log("Video Params: {0} x {1}. Pixels size: {2} Format: {3}", width, height, pixelSize, pixelFormat);

                // Initialize video frame buffer with the same dimensions like a video frame
                var bufferLen = width * height;
                if (_bufferAr == null || _bufferAr.Length != bufferLen)
                {
                    _bufferAr = new uint[bufferLen];
                }

                // Initialize ArgsubD3D rendering of the D2D video frame bitmap and the cube
                if (!_videoImage.Setup(ArD3dPanel.Context, width, height, pixelFormat, pixelSize, ArD3dPanel.DpiX, ArD3dPanel.DpiY))
                {
                    throw new InvalidOperationException("Could not initialize D2D rendering.");
                }
                _cube.Setup(ArD3dPanel.Context);
                _wasStarted = true;
            }

            // Get projection matrix 
            _arWrapper.arwGetProjectionMatrix(_projectionMatrix);

            // Get marker modelView matrix and other marker properties
            float confidence;
            var isMarkerVisisble = _arWrapper.arwQueryMarkerVisibility(_markerId);
            _arWrapper.arwQueryMarkerTransformation(_markerId, _markerModelViewMatrix);
            confidence = _arWrapper.arwGetMarkerOptionFloat(_markerId, ArMarkerOption.SquareConfidence);
#if DEBUG
            if (isMarkerVisisble)
            {
                Dispatcher.RunAsync(CoreDispatcherPriority.Low, () =>
                {
                    DbgTxt.Text = string.Format("Marker: {0} Confidence: {1:f02}\r\n" +
                                                "{2,7:f02} {3,7:f02} {4,7:f02} {5,7:f02}\r\n" +
                                                "{6,7:f02} {7,7:f02} {8,7:f02} {9,7:f02}\r\n" +
                                                "{10,7:f02} {11,7:f02} {12,7:f02} {13,7:f02}\r\n" +
                                                "{14,7:f02} {15,7:f02} {16,7:f02} {17,7:f02}\r\n",
                        _markerId, confidence,
                        _markerModelViewMatrix[00], _markerModelViewMatrix[04], _markerModelViewMatrix[08], _markerModelViewMatrix[12],
                        _markerModelViewMatrix[01], _markerModelViewMatrix[05], _markerModelViewMatrix[09], _markerModelViewMatrix[13],
                        _markerModelViewMatrix[02], _markerModelViewMatrix[06], _markerModelViewMatrix[10], _markerModelViewMatrix[14],
                        _markerModelViewMatrix[03], _markerModelViewMatrix[07], _markerModelViewMatrix[11], _markerModelViewMatrix[15]);
                });
            }
#endif

            // Copy camera video frame buffer into our own buffer
            if (_arWrapper.arwUpdateTexture32(_bufferAr))
            {
                // Begin rendering with a clear
                ArD3dPanel.BeginDraw(true);

                // Pass the current video frame to the D2D bitmap and render it
                ArD3dPanel.DisplayImage(_videoImage, _bufferAr);

                // Rotate the cube if desired and render it at the marker position using the right modelView and projection matrix
                if (isMarkerVisisble)
                {
                    if (_shouldRotateCube) _cube.Roll += 0.05f;
                    ArD3dPanel.DisplayObject(_cube, _markerModelViewMatrix, _projectionMatrix);
                }

                // Present the rendering result
                ArD3dPanel.EndDraw();
            }
        }

        #region UI Controls

        private void CoreWindowOnKeyUp(CoreWindow sender, KeyEventArgs e)
        {
            switch (e.VirtualKey)
            {
                case VirtualKey.Q:
                    Cleanup();
                    Application.Current.Exit();
                    break;
                case VirtualKey.Space:
                    _shouldRotateCube = !_shouldRotateCube;
                    break;
            }
        }

        private void ArD3dPanel_OnPointerReleased(object sender, PointerRoutedEventArgs e)
        {
            _shouldRotateCube = !_shouldRotateCube;
        }

        private void AppBarButtonIncrementClick(object sender, RoutedEventArgs e)
        {
            _arWrapperViewModel.ManualThresholdValue += 5;
        }

        private void AppBarButtonDecrementClick(object sender, RoutedEventArgs e)
        {
            _arWrapperViewModel.ManualThresholdValue -= 5;
        }

        #endregion
    }
}
