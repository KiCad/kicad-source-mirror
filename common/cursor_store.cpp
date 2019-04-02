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

#include <cursor_store.h>

#include <wx/bitmap.h>
#include <wx/debug.h>


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
#if defined( __WXMSW__ ) or defined( __WXMAC__ )

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

#elif defined( __WXGTK__ ) or defined( __WXMOTIF__ )

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


CURSOR_STORE::CURSOR_STORE( const std::vector<CURSOR_DEF>& aDefs )
{
    for( const auto& def : aDefs )
    {
        m_store[def.m_id_key] = constructCursor( def );
    }
}


const wxCursor& CURSOR_STORE::Get( int aIdKey ) const
{
    const auto find_iter = m_store.find( aIdKey );

    if( find_iter != m_store.end() )
    {
        return find_iter->second;
    }

    wxASSERT_MSG( false,
            wxString::Format( "Could not find cursor with ID %d", static_cast<int>( aIdKey ) ) );
    return wxNullCursor;
}