/**
 * @file class_gbr_layer_box_selector.cpp
 * @brief a derived class of LAYER_BOX_SELECTOR to handle the layer box selector
 * in GerbView
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <common.h>
#include <colors_selection.h>
#include <gerbview_frame.h>
#include <class_gerber_file_image_list.h>

#include <class_gbr_layer_box_selector.h>

void GBR_LAYER_BOX_SELECTOR::Resync()
{
    #define BM_SIZE 14
    Freeze();
    Clear();

    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();

    for( unsigned layerid = 0; layerid < images.ImagesMaxCount(); ++layerid )
    {
        wxBitmap    layerbmp( BM_SIZE, BM_SIZE );
        wxString    layername;

        if( !IsLayerEnabled( layerid ) )
            continue;

        // Prepare Bitmap
        SetBitmapLayer( layerbmp, layerid );

        layername = GetLayerName( layerid );

        Append( layername, layerbmp, (void*)(intptr_t) layerid );
    }

    // Ensure the width of the widget is enough to show the text and the icon
    SetMinSize( wxSize( -1, -1 ) );
    int minwidth = GetBestSize().x + BM_SIZE + 10;
    SetMinSize( wxSize( minwidth, -1 ) );

    Thaw();
}


// Returns a color index from the layer id
EDA_COLOR_T GBR_LAYER_BOX_SELECTOR::GetLayerColor( int aLayer ) const
{
    GERBVIEW_FRAME* frame = (GERBVIEW_FRAME*) GetParent()->GetParent();

    return frame->GetLayerColor( aLayer );
}


// Returns the name of the layer id
wxString GBR_LAYER_BOX_SELECTOR::GetLayerName( int aLayer ) const
{
    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();
    wxString name = images.GetDisplayName( aLayer );

    return name;
}
