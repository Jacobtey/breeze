#ifndef breezemetrics_h
#define breezemetrics_h

/*
* this file is part of the breeze gtk engine
* Copyright (c) 2010 Hugo Pereira Da Costa <hugo.pereira@free.fr>
* Copyright (c) 2010 Ruslan Kabatsayev <b7.10110111@gmail.com>
*
* This  library is free  software; you can  redistribute it and/or
* modify it  under  the terms  of the  GNU Lesser  General  Public
* License  as published  by the Free  Software  Foundation; either
* version 2 of the License, or( at your option ) any later version.
*
* This library is distributed  in the hope that it will be useful,
* but  WITHOUT ANY WARRANTY; without even  the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License  along  with  this library;  if not,  write to  the Free
* Software Foundation, Inc., 51  Franklin St, Fifth Floor, Boston,
* MA 02110-1301, USA.
*/

namespace Breeze
{

    //! metrics
    /*! these are copied from the old KStyle WidgetProperties */
    enum Metrics
    {

        // frames
        Frame_FrameWidth = 2,

        // line editors
        LineEdit_MarginWidth = 6,

        // combobox
        ComboBox_MarginWidth = 4,
        ComboBox_ButtonWidth = 20,
        ComboBox_BoxTextSpace = 8,

        // groupbox title margin
        GroupBox_TitleMarginWidth = 4,

        // buttons
        Button_MarginWidth = 6,

        // checkboxes and radio buttons
        CheckBox_Size = 22,
        CheckBox_BoxTextSpace = 8,
        RadioButton_Size = 22,

        // scrollbar
        ScrollBar_Extend = 20,
        ScrollBar_Width = 10,
        ScrollBar_SliderWidth = ScrollBar_Width,
        ScrollBar_MinSliderHeight = ScrollBar_Width,
        ScrollBar_NoButtonHeight = 0,
        ScrollBar_SingleButtonHeight = ScrollBar_Extend,
        ScrollBar_DoubleButtonHeight = 2*ScrollBar_Extend,

        // progress bar
        ProgressBar_BusyIndicatorSize = 14,
        ProgressBar_Thickness = 6,
        ProgressBar_BoxTextSpace = 4,

        // sliders
        Slider_Thickness = 6,
        Slider_ControlThickness = 20

    };

}

#endif