/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <vector>

#include <gal/cursors.h>
#include <kiplatform/ui.h>

// Cursor files
#include <cursors/cursor-add.xpm>
#include <cursors/cursor-component.xpm>
#include <cursors/cursor-eraser.xpm>
#include <cursors/cursor-label-global.xpm>
#include <cursors/cursor-label-hier.xpm>
#include <cursors/cursor-label-net.xpm>
#include <cursors/cursor-line-bus.xpm>
#include <cursors/cursor-line-graphic.xpm>
#include <cursors/cursor-line-wire.xpm>
#include <cursors/cursor-line-wire-add.xpm>
#include <cursors/cursor-measure.xpm>
#include <cursors/cursor-pencil.xpm>
#include <cursors/cursor-select-lasso.xpm>
#include <cursors/cursor-select-window.xpm>
#include <cursors/cursor-subtract.xpm>
#include <cursors/cursor-text.xpm>
#include <cursors/cursor-xor.xpm>
#include <cursors/cursor-zoom.xpm>
#include <cursors/cursor-zoom-out.xpm>

// Under MSW, the standard cursor is white on black.  Elsewhere it is black on white
#ifdef __WINDOWS__
#include <cursors/cursor-place.xpm>
#include <cursors/cursor-select-m.xpm>
#else
#include <cursors/cursor-place-black.xpm>
#include <cursors/cursor-select-m-black.xpm>
#endif

#include <wx/bitmap.h>
#include <wx/debug.h>


static const unsigned char voltage_probe[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
    0x00, 0x30, 0x06, 0x00, 0x00, 0x18, 0x0c, 0x00, 0x00, 0x0e, 0x08, 0x00, 0x80, 0x07, 0x08, 0x00,
    0xc0, 0x07, 0x18, 0x00, 0xe0, 0x07, 0x30, 0x00, 0xf0, 0x03, 0x60, 0x00, 0xf8, 0x01, 0x00, 0x00,
    0xfc, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0xc0,
    0x0f, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xf0, 0x01, 0x00, 0x00, 0xf0,
    0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const unsigned char current_probe[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00,
    0x00, 0x30, 0x0c, 0x00, 0x00, 0x1c, 0x08, 0x00, 0x00, 0x0f, 0x08, 0x00, 0x80, 0x0f, 0x18, 0x00,
    0xc0, 0x0f, 0x30, 0x80, 0xe1, 0x07, 0x60, 0x80, 0xf1, 0x03, 0x00, 0x80, 0xf9, 0x01, 0x00, 0x80,
    0xfd, 0x00, 0x00, 0x80, 0x7f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0xc0,
    0x0f, 0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0xfe, 0x03, 0x00, 0x00, 0xc6, 0x01, 0x00, 0x00, 0x83,
    0x01, 0x00, 0x00, 0x83, 0x01, 0x00, 0x00, 0x83, 0x01, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0xfc,
    0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00 };

static const unsigned char cursor_tune[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0xc0, 0x0f, 0x00,
    0x00, 0xe0, 0x1f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xfc, 0x07, 0x00,
    0x00, 0xfe, 0x03, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xff, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00,
    0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x1f, 0x00, 0x00, 0xfc, 0x0f, 0x00, 0x00, 0xfc, 0x07, 0x00, 0x00,
    0xfc, 0x03, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x0a,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };

static const unsigned char cursor_tune_mask[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0xc0, 0x0f,
    0x00, 0x00, 0xe0, 0x1f, 0x00, 0x00, 0xf0, 0x1f, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xfc, 0x07,
    0x00, 0x00, 0xfe, 0x03, 0x00, 0x00, 0xff, 0x01, 0x00, 0x80, 0xff, 0x00, 0x00, 0xc0, 0x7f, 0x00,
    0x00, 0xe0, 0x3f, 0x00, 0x00, 0xe0, 0x1f, 0x00, 0x00, 0xfc, 0x0f, 0x00, 0x00, 0xfc, 0x07, 0x00,
    0x00, 0xfc, 0x03, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00,
    0x00, 0x07, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00,
    0x70, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };


static const std::vector<CURSOR_STORE::CURSOR_DEF> standard_cursors = {
    {
        KICURSOR::VOLTAGE_PROBE,
        voltage_probe,
        voltage_probe,
        nullptr,
        { 32, 32 },
        { 1, 31 },
    },
    {
        KICURSOR::CURRENT_PROBE,
        current_probe,
        current_probe,
        nullptr,
        { 32, 32 },
        { 4, 27 },
    },
    {
        KICURSOR::TUNE,
        cursor_tune,
        cursor_tune_mask,
        nullptr,
        { 32, 32 },
        { 1, 30 },
    },
    {
        KICURSOR::PENCIL,
        nullptr,
        nullptr,
        cursor_pencil_xpm,
        { 32, 32 },
        { 4, 27 },
    },
    {
        KICURSOR::MOVING,
        nullptr,
        nullptr,
#ifdef __WINDOWS__
        cursor_select_m_xpm,
#else
        cursor_select_m_black_xpm,
#endif
        { 32, 32 },
        { 1, 1 },
    },
    {
        KICURSOR::REMOVE,
        nullptr,
        nullptr,
        cursor_eraser_xpm,
        { 32, 32 },
        { 4, 4 },
    },
    {
        KICURSOR::TEXT,
        nullptr,
        nullptr,
        cursor_text_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::MEASURE,
        nullptr,
        nullptr,
        cursor_measure_xpm,
        { 32, 32 },
        { 4, 4 },
    },
    {
        KICURSOR::ADD,
        nullptr,
        nullptr,
        cursor_add_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::SUBTRACT,
        nullptr,
        nullptr,
        cursor_subtract_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::XOR,
        nullptr,
        nullptr,
        cursor_xor_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::ZOOM_IN,
        nullptr,
        nullptr,
        cursor_zoom_xpm,
        { 32, 32 },
        { 6, 6 },
    },
    {
        KICURSOR::ZOOM_OUT,
        nullptr,
        nullptr,
        cursor_zoom_out_xpm,
        { 32, 32 },
        { 6, 6 },
    },
    {
        KICURSOR::LABEL_NET,
        nullptr,
        nullptr,
        cursor_label_net_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::LABEL_GLOBAL,
        nullptr,
        nullptr,
        cursor_label_global_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::COMPONENT,
        nullptr,
        nullptr,
        cursor_component_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::SELECT_LASSO,
        nullptr,
        nullptr,
        cursor_select_lasso_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::SELECT_WINDOW,
        nullptr,
        nullptr,
        cursor_select_window_xpm,
        { 32, 32 },
        { 7, 10 },
    },
    {
        KICURSOR::LINE_BUS,
        nullptr,
        nullptr,
        cursor_line_bus_xpm,
        { 32, 32 },
        { 5, 26 },
    },
    {
        KICURSOR::LINE_WIRE,
        nullptr,
        nullptr,
        cursor_line_wire_xpm,
        { 32, 32 },
        { 5, 26 },
    },
    {
        KICURSOR::LINE_WIRE_ADD,
        nullptr,
        nullptr,
        cursor_line_wire_add_xpm,
        { 32, 32 },
        { 5, 26 },
    },
    {
        KICURSOR::LINE_GRAPHIC,
        nullptr,
        nullptr,
        cursor_line_graphic_xpm,
        { 32, 32 },
        { 5, 26 },
    },
    {
        KICURSOR::LABEL_HIER,
        nullptr,
        nullptr,
        cursor_label_hier_xpm,
        { 32, 32 },
        { 7, 7 },
    },
    {
        KICURSOR::PLACE,
        nullptr,
        nullptr,
#ifdef __WINDOWS__
        cursor_place_xpm,
#else
        cursor_place_black_xpm,
#endif
        { 32, 32 },
        { 1, 1 },
    },
};


/**
 * Construct a cursor for the given definition.
 *
 * How to do this depends on the platform, see
 * http://docs.wxwidgets.org/trunk/classwx_cursor.html
 *
 * @param  aDef the cursor definition
 * @return      a newly constructed cursor if the platform is supported,
 *              else wxNullCursor
 */
wxCursor constructCursor( const CURSOR_STORE::CURSOR_DEF& aDef )
{
    if( aDef.m_xpm != nullptr )
    {
        wxImage xpmImage = wxImage( aDef.m_xpm );

        xpmImage.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, aDef.m_hotspot.x );
        xpmImage.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, aDef.m_hotspot.y );

        return wxCursor( xpmImage );
    }
    else if( aDef.m_image_data != nullptr && aDef.m_mask_data != nullptr )
    {
#if defined( __WXMSW__ ) || defined( __WXMAC__ )

        wxBitmap img_bitmap(
                reinterpret_cast<const char*>( aDef.m_image_data ), aDef.m_size.x, aDef.m_size.y );
        wxBitmap msk_bitmap(
                reinterpret_cast<const char*>( aDef.m_mask_data ), aDef.m_size.x, aDef.m_size.y );
        img_bitmap.SetMask( new wxMask( msk_bitmap ) );

        wxImage image( img_bitmap.ConvertToImage() );

#if defined( __WXMSW__ )
        image.SetMaskColour( 255, 255, 255 );
#endif

        image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, aDef.m_hotspot.x );
        image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, aDef.m_hotspot.y );

        return wxCursor{ image };

#elif defined( __WXGTK__ ) || defined( __WXMOTIF__ )

        return wxCursor{
            reinterpret_cast<const char*>( aDef.m_image_data ),
            aDef.m_size.x,
            aDef.m_size.y,
            aDef.m_hotspot.x,
            aDef.m_hotspot.y,
            reinterpret_cast<const char*>( aDef.m_mask_data ),
        };

#else
        wxASSERT_MSG( false, "Unknown platform for cursor construction." );
        return wxNullCursor;
#endif
    }

    wxASSERT_MSG( false, "Unknown to find cursor" );
    return wxNullCursor;
}


CURSOR_STORE::CURSOR_STORE( const std::vector<CURSOR_DEF>& aDefs )
{
    for( const auto& def : aDefs )
    {
        m_store[def.m_id_key] = constructCursor( def );
    }
}


const wxCursor& CURSOR_STORE::Get( KICURSOR aIdKey ) const
{
    const auto find_iter = m_store.find( aIdKey );

    if( find_iter != m_store.end() )
        return find_iter->second;

    wxASSERT_MSG( false, wxString::Format( "Could not find cursor with ID %d",
                                           static_cast<int>( aIdKey ) ) );
    return wxNullCursor;
}


const wxCursor CURSOR_STORE::GetCursor( KICURSOR aCursorType )
{
    wxStockCursor stock =
            GetStockCursor( aCursorType );
    if( stock != wxCURSOR_MAX )
    {
        return wxCursor( stock );
    }

    static CURSOR_STORE store( standard_cursors );
    return store.Get( aCursorType );
}


const wxStockCursor CURSOR_STORE::GetStockCursor( KICURSOR aCursorType )
{
    wxStockCursor stockCursor;
    switch( aCursorType )
    {
    case KICURSOR::MOVING:
        stockCursor = wxCURSOR_SIZING;
        break;
    case KICURSOR::BULLSEYE:
        stockCursor = wxCURSOR_BULLSEYE;
        break;
    case KICURSOR::HAND:
        stockCursor = wxCURSOR_HAND;
        break;
    case KICURSOR::ARROW:
        stockCursor = wxCURSOR_ARROW;
        break;
    default:
        stockCursor = wxCURSOR_MAX;
        break;
    }

    if( !KIPLATFORM::UI::IsStockCursorOk( stockCursor ) )
    {
        stockCursor = wxCURSOR_MAX;
    }

    return stockCursor;
}
