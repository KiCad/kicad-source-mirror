/**
 * @file class_gbr_layer_box_selector.cpp
 * @brief a derived class of LAYER_BOX_SELECTOR to handle the layer box selector
 * in GerbView
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean-Pierre Charras <jean-pierre.charras@ujf-grenoble.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <layers_id_colors_and_visibility.h>
#include <gerbview_frame.h>
#include <class_GERBER.h>
#include <class_X2_gerber_attributes.h>

#include <class_gbr_layer_box_selector.h>

void GBR_LAYER_BOX_SELECTOR::Resync()
{
    Freeze();
    Clear();

    for( int layerid = 0; layerid < GERBER_DRAWLAYERS_COUNT; ++layerid )
    {
        wxBitmap    layerbmp( 14, 14 );
        wxString    layername;

        if( !IsLayerEnabled( layerid ) )
            continue;

        // Prepare Bitmap
        SetBitmapLayer( layerbmp, layerid );

        layername = GetLayerName( layerid );

        Append( layername, layerbmp, (void*)(intptr_t) layerid );
    }

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
    wxString name = g_GERBER_List.GetDisplayName( aLayer );

    return name;
}
