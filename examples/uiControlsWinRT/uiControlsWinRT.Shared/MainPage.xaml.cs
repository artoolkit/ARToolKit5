/* 
 *  MainPage.xaml.cs
 *  ARToolKit5
 *
 *  The MainPage for the uiControlsWinRT sample app.
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
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Media.Media3D;
using Windows.UI.Xaml.Navigation;
using ArWinRtSamples;
using ARToolKitComponent;

namespace uiControlsWinRT
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

        private int _markerId;
        private bool _isRunning;
        private bool _wasStarted;
        private uint[] _bufferAr;
        private byte[] _bufferWb;
        private WriteableBitmap _writeableBitmap;
        private float[] _projectionMatrix;
        private float[] _markerModelViewMatrix;
        private bool _isMarkerVisisble;
        private double rotValue;
        private CancellationTokenSource _processingTaskCancellationTokenSource;

        // Key handling
        private bool _shouldRotate;
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
            // Initialize ARToolKit wrapper
            _arWrapper = new ARWrapper();

            _arWrapper.arwRegisterLogCallback(Helper.Log);
            if (!_arWrapper.arwInitialiseARWithOptions(CurrentTemplateSize, CurrentTemplateCountMax))
            {
                throw new InvalidOperationException("Failed to initialize ARToolKit.");
            }
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

            // Setup rendering members
            _projectionMatrix = new float[16];
            _markerModelViewMatrix = new float[16];
            _bufferAr = new uint[DesiredVideoWidth * DesiredVideoHeight];

            // Init video with calibration data
            // TODO: Run on a background thread since the inner working of the WinMC implementation use Wait() on async op which will fail on the UI thread
            await Task.Run(async () =>
            {
                await StartCapturing();
                StartProcessing();
                //  _isSnapshotRestored = await Helper.RestoreSnapshot(_bufferAr, _markerModelViewMatrix);
            });
            Window.Current.CoreWindow.KeyUp += CoreWindowOnKeyUp;
        }

        private void Cleanup()
        {
            Window.Current.CoreWindow.KeyUp -= CoreWindowOnKeyUp;

            if (_processingTaskCancellationTokenSource != null)
            {
                _processingTaskCancellationTokenSource.Cancel();
            }
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
            // Load marker           
            var markerData = await PathIO.ReadTextAsync("ms-appx:///data/hiro.patt"); // single_buffer;80;buffer=234 221 237...
            var cfg = string.Format("single_buffer;{0};buffer={1}", 80, markerData);
            _markerId = _arWrapper.arwAddMarker(cfg);
            if (_markerId < 0)
            {
                throw new InvalidOperationException("Failed to load AR marker.");
            }
        }

        private async Task StartCapturing()
        {
            // Load video config
            var cameraCalibrationData = await PathIO.ReadBufferAsync("ms-appx:///data/camera_para.dat");
            var desiredVideoCameraPosition = await Helper.GetDefaultVideoDeviceLocation();
            var videoConfiguration0 = string.Format("-module=WinMC -width={0} -height={1} -format=BGRA -position={2}", DesiredVideoWidth, DesiredVideoHeight, desiredVideoCameraPosition);

            // Start capturing
            _isRunning = _arWrapper.arwStartRunningB(videoConfiguration0, cameraCalibrationData.ToArray(), NearPlane, FarPlane);
            if (!_isRunning)
            {
                throw new InvalidOperationException("Failed to start ARToolKit.");
            }
        }

        private void StartProcessing()
        {
            // Start AR processing loop in separate thread to keep the UI thread more responsive
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
            if (_arWrapper != null && _isRunning)
            {
                // Update
                if (_arWrapper.arwCapture())
                {
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

                    // Update
                    _arWrapper.arwUpdateAR();

                    if (!_wasStarted)
                    {
                        // Get video params
                        int width, height, pixelSize;
                        string pixelFormat;
                        if (!_arWrapper.arwGetVideoParams(out width, out height, out pixelSize, out pixelFormat))
                        {
                            throw new InvalidOperationException("ARToolkit arwGetVideoParams failed.");
                        }
                        Helper.Log("Video Params: {0} x {1}. Pixels size: {2} Format: {3}", width, height, pixelSize, pixelFormat);

                        // Initialize buffer
                        var bufferLen = width * height;
                        const int bytesPerPixel = 4;
                        if (_bufferAr == null || _bufferAr.Length != bufferLen)
                        {
                            _bufferAr = new uint[bufferLen];

                        }
                        if (_bufferWb == null || _bufferWb.Length != bufferLen)
                        {
                            _bufferWb = new byte[bufferLen * bytesPerPixel];
                            Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                            {
                                _writeableBitmap = new WriteableBitmap(width, height);
                                PreviewElement.Source = _writeableBitmap;

                            });
                        }
                        _wasStarted = true;
                    }

                    // Get projection matrix 
                    _arWrapper.arwGetProjectionMatrix(_projectionMatrix);

                    // Get marker modelView matrix and other properties
                    _isMarkerVisisble = false;
                    var confidence = -1f;
                    if (!_isMarkerVisisble)
                    {
                        _isMarkerVisisble = _arWrapper.arwQueryMarkerVisibility(_markerId);
                        _arWrapper.arwQueryMarkerTransformation(_markerId, _markerModelViewMatrix);
                        confidence = _arWrapper.arwGetMarkerOptionFloat(_markerId, ArMarkerOption.SquareConfidence);
                    }
#if DEBUG
                    if (_isMarkerVisisble)
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

                    // Update video frame and render
                    if (_writeableBitmap != null && UpdateVideoTexture())
                    {
                        unsafe
                        {
                            fixed (uint* srcPtr = _bufferAr)
                            {
                                var b = 0;
                                var len = _bufferWb.Length / 4;
                                for (var i = 0; i < len; i++, b += 4)
                                {
                                    // On a little-endian system, R occupies the lowest 8 bits, then G, then B, then A the highest 8 bits.
                                    // RGBA -> BGRA
                                    var p = srcPtr[i];
                                    _bufferWb[b + 0] = (byte)((p >> 16) & 0xff); // R
                                    _bufferWb[b + 1] = (byte)((p >> 8) & 0xff); // G
                                    _bufferWb[b + 2] = (byte)((p >> 0) & 0xff); // B 
                                    _bufferWb[b + 3] = (byte)((p >> 24) & 0xff); // A 
                                }
                            }
                        }

                        // Update needs to run on the UI thread
                        Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                        {
                            // Show video buffer
                            using (var stream = _writeableBitmap.PixelBuffer.AsStream())
                            {
                                stream.Write(_bufferWb, 0, _bufferWb.Length);
                            }
                            _writeableBitmap.Invalidate();

                            // Compute XAML tranformation matrix
                            if (_isMarkerVisisble)
                            {
                                // Center at origin of the controls
                                var centerAtOrigin = Matrix3DFactory.CreateTranslation(-ControlPanel.ActualWidth * 0.5, -ControlPanel.ActualHeight * 0.5, 0);
                                // Swap the y-axis and scale down by half
                                var scale = Matrix3DFactory.CreateScale(0.5, -0.5, 0.5);
                                // Rotate around z
                                var rotate = _shouldRotate ? Matrix3DFactory.CreateRotationZ(rotValue += 0.05) : Matrix3D.Identity;
                                // Viewport transformation
                                var viewport = Matrix3DFactory.CreateViewportTransformation(ContentPanel.ActualWidth, ContentPanel.ActualHeight);

                                // Calculate the final transformation matrix by using the marker model view and camera projection matrix 
                                var m = Matrix3DFactory.CreateViewportProjection(
                                    centerAtOrigin * rotate * scale, 
                                    _markerModelViewMatrix.ToMatrix3D(), 
                                    _projectionMatrix.ToMatrix3D(), 
                                    viewport);

                                // Apply the matrix to the UI control
                                ControlPanel.Projection = new Matrix3DProjection { ProjectionMatrix = m };
                                ControlPanel.Visibility = Visibility.Visible;
                            }
                            else
                            {
                                ControlPanel.Visibility = Visibility.Collapsed;
                            }
                        });

                    }
                }
            }
        }

        private bool UpdateVideoTexture()
        {
            return _arWrapper.arwUpdateTexture32(_bufferAr);
        }


        #region UI Controls

        private async void CoreWindowOnKeyUp(CoreWindow sender, KeyEventArgs e)
        {
            switch (e.VirtualKey)
            {
                case VirtualKey.Q:
                    Cleanup();
                    Application.Current.Exit();
                    break;
                case VirtualKey.Space:
                    _shouldRotate = !_shouldRotate;
                    break;
            }
        }

        private void BtnRotate_OnClick(object sender, RoutedEventArgs e)
        {
            _shouldRotate = !_shouldRotate;
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
