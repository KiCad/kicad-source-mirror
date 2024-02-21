/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <cursors/cursor-zoom-in.xpm>
#include <cursors/cursor-zoom-out.xpm>
#include <cursors/cursor_tune.xpm>
#include <cursors/voltage_probe.xpm>
#include <cursors/current_probe.xpm>

// HiDPI cursor files
#include <cursors/cursor-add64.xpm>
#include <cursors/cursor-component64.xpm>
#include <cursors/cursor-eraser64.xpm>
#include <cursors/cursor-label-global64.xpm>
#include <cursors/cursor-label-hier64.xpm>
#include <cursors/cursor-label-net64.xpm>
#include <cursors/cursor-line-bus64.xpm>
#include <cursors/cursor-line-graphic64.xpm>
#include <cursors/cursor-line-wire64.xpm>
#include <cursors/cursor-line-wire-add64.xpm>
#include <cursors/cursor-measure64.xpm>
#include <cursors/cursor-pencil64.xpm>
#include <cursors/cursor-select-lasso64.xpm>
#include <cursors/cursor-select-window64.xpm>
#include <cursors/cursor-subtract64.xpm>
#include <cursors/cursor-text64.xpm>
#include <cursors/cursor-xor64.xpm>
#include <cursors/cursor-zoom-in64.xpm>
#include <cursors/cursor-zoom-out64.xpm>
#include <cursors/cursor_tune64.xpm>
#include <cursors/voltage_probe64.xpm>
#include <cursors/current_probe64.xpm>


// Under MSW, the standard cursor is white on black.  Elsewhere it is black on white
#ifdef __WINDOWS__
#include <cursors/cursor-place.xpm>
#include <cursors/cursor-place64.xpm>
#include <cursors/cursor-select-m.xpm>
#include <cursors/cursor-select-m64.xpm>
#else
#include <cursors/cursor-place-black.xpm>
#include <cursors/cursor-place-black64.xpm>
#include <cursors/cursor-select-m-black.xpm>
#include <cursors/cursor-select-m-black64.xpm>
#endif

#include <wx/bitmap.h>
#include <wx/debug.h>


static const std::vector<CURSOR_STORE::CURSOR_DEF> standard_cursors = {
    {
        KICURSOR::VOLTAGE_PROBE,
        nullptr,
        nullptr,
        voltage_probe_xpm,
        { 32, 32 },
        { 1, 31 },
    },
    {
        KICURSOR::CURRENT_PROBE,
        nullptr,
        nullptr,
        current_probe_xpm,
        { 32, 32 },
        { 4, 27 },
    },
    {
        KICURSOR::TUNE,
        nullptr,
        nullptr,
        cursor_tune_xpm,
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
        cursor_zoom_in_xpm,
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


static const std::vector<CURSOR_STORE::CURSOR_DEF> hidpi_cursors = {
    {
        KICURSOR::VOLTAGE_PROBE,
        nullptr,
        nullptr,
        voltage_probe64_xpm,
        { 64, 64 },
        { 1, 62 },
    },
    {
        KICURSOR::CURRENT_PROBE,
        nullptr,
        nullptr,
        current_probe64_xpm,
        { 64, 64 },
        { 8, 54 },
    },
    {
        KICURSOR::TUNE,
        nullptr,
        nullptr,
        cursor_tune64_xpm,
        { 64, 64 },
        { 2, 60 },
    },
    {
        KICURSOR::PENCIL,
        nullptr,
        nullptr,
        cursor_pencil64_xpm,
        { 64, 64 },
        { 8, 54 },
    },
    {
        KICURSOR::MOVING,
        nullptr,
        nullptr,
#ifdef __WINDOWS__
        cursor_select_m64_xpm,
#else
        cursor_select_m_black64_xpm,
#endif
        { 64, 64 },
        { 2, 2 },
    },
    {
        KICURSOR::REMOVE,
        nullptr,
        nullptr,
        cursor_eraser64_xpm,
        { 64, 64 },
        { 8, 8 },
    },
    {
        KICURSOR::TEXT,
        nullptr,
        nullptr,
        cursor_text64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::MEASURE,
        nullptr,
        nullptr,
        cursor_measure64_xpm,
        { 64, 64 },
        { 8, 8 },
    },
    {
        KICURSOR::ADD,
        nullptr,
        nullptr,
        cursor_add64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::SUBTRACT,
        nullptr,
        nullptr,
        cursor_subtract64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::XOR,
        nullptr,
        nullptr,
        cursor_xor64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::ZOOM_IN,
        nullptr,
        nullptr,
        cursor_zoom_in64_xpm,
        { 64, 64 },
        { 12, 12 },
    },
    {
        KICURSOR::ZOOM_OUT,
        nullptr,
        nullptr,
        cursor_zoom_out64_xpm,
        { 64, 64 },
        { 12, 12 },
    },
    {
        KICURSOR::LABEL_NET,
        nullptr,
        nullptr,
        cursor_label_net64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::LABEL_GLOBAL,
        nullptr,
        nullptr,
        cursor_label_global64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::COMPONENT,
        nullptr,
        nullptr,
        cursor_component64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::SELECT_LASSO,
        nullptr,
        nullptr,
        cursor_select_lasso64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::SELECT_WINDOW,
        nullptr,
        nullptr,
        cursor_select_window64_xpm,
        { 64, 64 },
        { 14, 20 },
    },
    {
        KICURSOR::LINE_BUS,
        nullptr,
        nullptr,
        cursor_line_bus64_xpm,
        { 64, 64 },
        { 10, 52 },
    },
    {
        KICURSOR::LINE_WIRE,
        nullptr,
        nullptr,
        cursor_line_wire64_xpm,
        { 64, 64 },
        { 10, 52 },
    },
    {
        KICURSOR::LINE_WIRE_ADD,
        nullptr,
        nullptr,
        cursor_line_wire_add64_xpm,
        { 64, 64 },
        { 10, 52 },
    },
    {
        KICURSOR::LINE_GRAPHIC,
        nullptr,
        nullptr,
        cursor_line_graphic64_xpm,
        { 64, 64 },
        { 10, 52 },
    },
    {
        KICURSOR::LABEL_HIER,
        nullptr,
        nullptr,
        cursor_label_hier64_xpm,
        { 64, 64 },
        { 14, 14 },
    },
    {
        KICURSOR::PLACE,
        nullptr,
        nullptr,
#ifdef __WINDOWS__
        cursor_place64_xpm,
#else
        cursor_place_black64_xpm,
#endif
        { 64, 64 },
        { 2, 2 },
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
        wxASSERT_MSG( false, wxS( "Unknown platform for cursor construction." ) );
        return wxNullCursor;
#endif
    }

    wxASSERT_MSG( false, wxS( "Unknown to find cursor" ) );
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
    wxStockCursor stock = GetStockCursor( aCursorType );

    if( stock != wxCURSOR_MAX )
    {
        return wxCursor( stock );
    }

    static CURSOR_STORE store( standard_cursors );
    return store.Get( aCursorType );
}


const wxCursor CURSOR_STORE::GetHiDPICursor( KICURSOR aCursorType )
{
    wxStockCursor stock = GetStockCursor( aCursorType );

    if( stock != wxCURSOR_MAX )
    {
        return wxCursor( stock );
    }

    static CURSOR_STORE store( hidpi_cursors );
    return store.Get( aCursorType );
}


wxStockCursor CURSOR_STORE::GetStockCursor( KICURSOR aCursorType )
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
