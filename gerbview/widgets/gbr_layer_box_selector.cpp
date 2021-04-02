/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <gerbview_frame.h>
#include <gerber_file_image_list.h>

#include "gbr_layer_box_selector.h"

void GBR_LAYER_BOX_SELECTOR::Resync()
{
    #define BM_SIZE 14
    Freeze();
    Clear();

    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();

    for( unsigned layerid = 0; layerid < images.ImagesMaxCount(); ++layerid )
    {
        if( !isLayerEnabled( layerid ) )
            continue;

        // Prepare Bitmap
        wxBitmap bmp( BM_SIZE, BM_SIZE );
        DrawColorSwatch( bmp, getLayerColor( LAYER_PCB_BACKGROUND ), getLayerColor( layerid ) );

        Append( getLayerName( layerid ), bmp, (void*)(intptr_t) layerid );
    }

    // Ensure the size of the widget is enough to show the text and the icon
    // We have to have a selected item when doing this, because otherwise GTK
    // will just choose a random size that might not fit the actual data
    // (such as in cases where the font size is very large). So we select
    // the first item, get the size of the control and make that the minimum size,
    // then remove the selection (which was the initial state).
    SetSelection( 0 );

    SetMinSize( wxSize( -1, -1 ) );
    wxSize bestSize = GetBestSize();

    bestSize.x = GetBestSize().x + BM_SIZE + 10;
    SetMinSize( bestSize );

    SetSelection( wxNOT_FOUND );
    Thaw();
}


// Returns a color index from the layer id
COLOR4D GBR_LAYER_BOX_SELECTOR::getLayerColor( int aLayer ) const
{
    GERBVIEW_FRAME* frame = (GERBVIEW_FRAME*) GetParent()->GetParent();

    return frame->GetLayerColor( GERBER_DRAW_LAYER( aLayer ) );
}


// Returns the name of the layer id
wxString GBR_LAYER_BOX_SELECTOR::getLayerName( int aLayer ) const
{
    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();
    wxString name = images.GetDisplayName( aLayer );

    return name;
}
