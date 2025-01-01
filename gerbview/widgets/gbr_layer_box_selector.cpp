/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gbr_layer_box_selector.h"

#include <widgets/layer_presentation.h>

#include <gerbview_frame.h>
#include <gerber_file_image_list.h>

#ifdef __WXMSW__
#include <dpi_scaling_common.h>
#endif


/**
 * Gerbview-specific implementation of the LAYER_PRESENTATION interface.
 */
class GBR_LAYER_PRESENTATION : public LAYER_PRESENTATION
{
public:
    GBR_LAYER_PRESENTATION( GERBVIEW_FRAME& aFrame ) : m_frame( aFrame ) {}

    // Returns a color index from the layer id
    COLOR4D getLayerColor( int aLayer ) const override
    {
        return m_frame.GetLayerColor( GERBER_DRAW_LAYER( aLayer ) );
    }

    // Returns the name of the layer id
    wxString getLayerName( int aLayer ) const override
    {
        GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();
        wxString                name = images.GetDisplayName( aLayer );

        return name;
    }

private:
    GERBVIEW_FRAME& m_frame;
};


GBR_LAYER_BOX_SELECTOR::GBR_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                                const wxSize& size, int n,
                                                const wxString choices[] ) :
        LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices ),
        m_layerPresentation( std::make_unique<GBR_LAYER_PRESENTATION>(
                static_cast<GERBVIEW_FRAME&>( *parent->GetParent() ) ) )
{
    m_layerhotkeys = false;
}

void GBR_LAYER_BOX_SELECTOR::Resync()
{
    Freeze();
    Clear();

    const int size = 14;

    GERBER_FILE_IMAGE_LIST& images = GERBER_FILE_IMAGE_LIST::GetImagesList();

    for( unsigned layerid = 0; layerid < images.ImagesMaxCount(); ++layerid )
    {
        if( !isLayerEnabled( layerid ) )
            continue;

        // Don't show unused layers
        if ( images.GetGbrImage( layerid ) == nullptr )
            continue;

        // Prepare Bitmaps
        wxVector<wxBitmap> bitmaps;

        for( int scale = 1; scale <= 3; scale++ )
        {
            wxBitmap bmp( size * scale, size * scale );

            m_layerPresentation->DrawColorSwatch( bmp, layerid );

            bmp.SetScaleFactor( scale );
            bitmaps.push_back( bmp );
        }

        Append( m_layerPresentation->getLayerName( layerid ),
                wxBitmapBundle::FromBitmaps( bitmaps ), (void*) (intptr_t) layerid );
    }

    // Ensure the size of the widget is enough to show the text and the icon
    // We have to have a selected item when doing this, because otherwise GTK
    // will just choose a random size that might not fit the actual data
    // (such as in cases where the font size is very large). So we select
    // the first item, get the size of the control and make that the minimum size,
    // then remove the selection (which was the initial state).
    if( GetCount() )
    {
        SetSelection( 0 );

        SetMinSize( wxSize( -1, -1 ) );
        wxSize bestSize = GetBestSize();

        bestSize.x = GetBestSize().x + size + 10;
        SetMinSize( bestSize );

        SetSelection( wxNOT_FOUND );
    }

    Thaw();
}
