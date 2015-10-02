/* 
 *  ArWrapperViewModel.cs
 *  ARToolKit5
 *
 *  Common ViewModel for binding the ArWrapper component to UI elements.
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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Windows.UI.Xaml;
using ARToolKitComponent;

namespace ArWinRtSamples
{
    public class ArWrapperViewModel : INotifyPropertyChanged
    {
        private ThresholdMode _selectedThresholdMode;
        private Visibility _manualThresholdVisibility;
        private bool _useDebugMode;
        private ARWrapper _arWrapper;

        public class ThresholdMode
        {
            public ArThresholdMode Mode { get; private set; }
            public string Name { get; private set; }

            public ThresholdMode(ArThresholdMode mode)
            {
                Mode = mode;
                Name = Enum.GetName(mode.GetType(), mode);
            }
        }      
        
        public ObservableCollection<ThresholdMode> ThresholdModes { get; private set; }

        public ThresholdMode SelectedThresholdMode
        {
            get { return _selectedThresholdMode; }
            set
            {
                if (_selectedThresholdMode != value && ArWrapper != null)
                {
                    _selectedThresholdMode = value;
                    // We need to change the threshold mode in the processing thread, not here, otherwise a concurrent op can happen and UpdateAr will crash
                    //ArWrapper.arwSetVideoThresholdMode(value.Mode);
                    OnPropertyChanged();
                    ManualThresholdVisibility = value.Mode == ArThresholdMode.Manual ? Visibility.Visible : Visibility.Collapsed;
                }
            }
        }

        public int ManualThresholdValue
        {
            get { return ArWrapper == null ? -1 : ArWrapper.arwGetVideoThreshold(); }
            set 
            {
                if (ArWrapper != null)
                {
                    if (value > 255) value = 255;
                    else if (value < 0) value = 0;
                    ArWrapper.arwSetVideoThreshold(value);
                    OnPropertyChanged();
                }
            }
        }

        public Visibility ManualThresholdVisibility
        {
            get { return _manualThresholdVisibility; }
            set
            {
                if (_manualThresholdVisibility != value)
                {
                    _manualThresholdVisibility = value;
                    OnPropertyChanged();
                }
            }
        }

        public bool UseDebugMode
        {
            get { return _useDebugMode; }
            set
            {
                if (_useDebugMode != value)
                {
                    // We need to change the debug mode in the processing thread, not here, otherwise a concurrent op can happen and UpdateAr will crash
                    //ArWrapper.arwSetVideoDebugMode(value);
                    _useDebugMode = value;
                    OnPropertyChanged();
                }
            }
        }

        public ARWrapper ArWrapper
        {
            get { return _arWrapper; }
            set
            {
                _arWrapper = value;
                if (_arWrapper != null)
                {
                    UseDebugMode = _arWrapper.arwGetVideoDebugMode();
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public ArWrapperViewModel()
        {
            ThresholdModes = new ObservableCollection<ThresholdMode>();
            foreach (ArThresholdMode mode in Enum.GetValues(typeof(ArThresholdMode)))
            {
                ThresholdModes.Add(new ThresholdMode(mode));
            }
        }

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            var handler = PropertyChanged;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
