/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/mstream.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/aui/auibar.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <common.h>
#include <bitmaps.h>
#include <pgm_base.h>
#include <eda_base_frame.h>
#include <eda_draw_frame.h>


struct SCALED_BITMAP_ID {
    BITMAP_DEF bitmap;
    int scale;

    bool operator==( SCALED_BITMAP_ID const& other ) const noexcept
    {
        return bitmap == other.bitmap && scale == other.scale;
    }
};


namespace std {
    template<> struct hash<SCALED_BITMAP_ID>
    {
        typedef SCALED_BITMAP_ID argument_type;
        typedef std::size_t result_type;

        result_type operator()( argument_type const& id ) const noexcept
        {
            static const bool sz64 = sizeof( uintptr_t ) == 8;
            static const size_t mask = sz64 ? 0xF000000000000000uLL : 0xF0000000uL;
            static const size_t offset = sz64 ? 60 : 28;

            // The hash only needs to be fast and simple, not necessarily accurate - a collision
            // only makes things slower, not broken. BITMAP_DEF is a pointer, so the most
            // significant several bits are generally going to be the same for all. Just convert
            // it to an integer and stuff the scale factor into those bits.
            return
                ( (uintptr_t)( id.bitmap ) & ~mask ) |
                ( ( (uintptr_t)( id.scale ) & 0xF ) << offset );
        }
    };
}


wxBitmap KiBitmap( BITMAP_DEF aBitmap )
{
    wxMemoryInputStream is( aBitmap->png, aBitmap->byteCount );
    wxImage image( is, wxBITMAP_TYPE_PNG );
    wxBitmap bitmap( image );

    return bitmap;
}


int KiIconScale( wxWindow* aWindow )
{
    const int vert_size = aWindow->ConvertDialogToPixels( wxSize( 0, 8 ) ).y;

    // Autoscale won't exceed unity until the system has quite high resolution,
    // because we don't want the icons to look obviously scaled on a system
    // where it's easy to see it.

    if( vert_size > 34 )        return 8;
    else if( vert_size > 29 )   return 7;
    else if( vert_size > 24 )   return 6;
    else                        return 4;
}


static int get_scale_factor( EDA_BASE_FRAME* aWindow )
{
    int requested_scale;
    Pgm().CommonSettings()->Read( ICON_SCALE_KEY, &requested_scale, 0 );

    if( requested_scale > 0 )
        return requested_scale;
    else
        return KiIconScale( aWindow );
}


wxBitmap KiScaledBitmap( BITMAP_DEF aBitmap, EDA_BASE_FRAME* aWindow )
{
    // Bitmap conversions are cached because they can be slow.
    static std::unordered_map<SCALED_BITMAP_ID, wxBitmap> bitmap_cache;
    static std::mutex bitmap_cache_mutex;
    const int scale = get_scale_factor( aWindow );

    SCALED_BITMAP_ID id = { aBitmap, scale };

    std::lock_guard<std::mutex> guard( bitmap_cache_mutex );
    auto it = bitmap_cache.find( id );

    if( it != bitmap_cache.end() )
    {
        return it->second;
    }
    else
    {
        wxMemoryInputStream is( aBitmap->png, aBitmap->byteCount );
        wxImage image( is, wxBITMAP_TYPE_PNG );

        // Bilinear seems to genuinely look better for these line-drawing icons
        // than bicubic, despite claims in the wx documentation that bicubic is
        // "highest quality". I don't recommend changing this. Bicubic looks
        // blurry and makes me want an eye exam.
        image.Rescale( scale * image.GetWidth() / 4, scale * image.GetHeight() / 4,
                wxIMAGE_QUALITY_BILINEAR );
        return bitmap_cache.emplace( id, wxBitmap( image ) ).first->second;
    }
}


wxBitmap KiScaledBitmap( const wxBitmap& aBitmap, EDA_BASE_FRAME* aWindow )
{
    const int scale = get_scale_factor( aWindow );

    if( scale == 4)
    {
        return wxBitmap( aBitmap );
    }
    else
    {
        wxImage image = aBitmap.ConvertToImage();
        image.Rescale( scale * image.GetWidth() / 4, scale * image.GetHeight() / 4,
            wxIMAGE_QUALITY_BILINEAR );

        return wxBitmap( image );
    }
}


void KiScaledSeparator( wxAuiToolBar* aToolbar, EDA_BASE_FRAME* aWindow )
{
    const int scale = get_scale_factor( aWindow );

    if( scale > 4 )
    {
        aToolbar->AddSpacer( 16 * ( scale - 4 ) / 4 );
    }

    aToolbar->AddSeparator();

    if( scale > 4 )
    {
        aToolbar->AddSpacer( 16 * ( scale - 4 ) / 4 );
    }
}


wxBitmap* KiBitmapNew( BITMAP_DEF aBitmap )
{
    wxMemoryInputStream is( aBitmap->png, aBitmap->byteCount );
    wxImage image( is, wxBITMAP_TYPE_PNG );
    wxBitmap* bitmap = new wxBitmap( image );

    return bitmap;
}


bool SaveCanvasImageToFile( EDA_DRAW_FRAME* aFrame, const wxString& aFileName,
                            wxBitmapType aBitmapType )
{
    wxCHECK( aFrame != nullptr, false );

    bool       retv = true;

    // Make a screen copy of the canvas:
    wxSize image_size = aFrame->GetCanvas()->GetClientSize();

    wxClientDC dc( aFrame->GetCanvas() );
    wxBitmap   bitmap( image_size.x, image_size.y );
    wxMemoryDC memdc;

    memdc.SelectObject( bitmap );
    memdc.Blit( 0, 0, image_size.x, image_size.y, &dc, 0, 0 );
    memdc.SelectObject( wxNullBitmap );

    wxImage image = bitmap.ConvertToImage();

    if( !image.SaveFile( aFileName, aBitmapType ) )
        retv = false;

    image.Destroy();
    return retv;
}


void AddBitmapToMenuItem( wxMenuItem* aMenu, const wxBitmap& aImage )
{
    // Retrieve the global applicaton show icon option:
    bool useImagesInMenus;
    Pgm().CommonSettings()->Read( USE_ICONS_IN_MENUS_KEY, &useImagesInMenus );

    wxItemKind menu_type = aMenu->GetKind();

    if( useImagesInMenus )
    {
        if( menu_type == wxITEM_CHECK || menu_type == wxITEM_RADIO )
        {
    #if defined(  __WINDOWS__ )
            aMenu->SetBitmaps( KiBitmap( checked_ok_xpm ), aImage );
            // A workaround to a strange bug on Windows, wx Widgets 3.0:
            // size of bitmaps is not taken in account for wxITEM_{CHECK,RADIO} menu
            // unless we call SetFont
            aMenu->SetFont( *wxNORMAL_FONT );
    #endif
        }
        else if( menu_type != wxITEM_RADIO )
        {
            aMenu->SetBitmap( aImage );
        }
    }
}


wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                         const wxBitmap& aImage, wxItemKind aType = wxITEM_NORMAL )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, wxEmptyString, aType );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* AddMenuItem( wxMenu* aMenu, int aId, const wxString& aText,
                         const wxString& aHelpText, const wxBitmap& aImage,
                         wxItemKind aType = wxITEM_NORMAL )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, aHelpText, aType );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxBitmap& aImage )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText );
    item->SetSubMenu( aSubMenu );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}


wxMenuItem* AddMenuItem( wxMenu* aMenu, wxMenu* aSubMenu, int aId,
                         const wxString& aText, const wxString& aHelpText,
                         const wxBitmap& aImage )
{
    wxMenuItem* item = new wxMenuItem( aMenu, aId, aText, aHelpText );
    item->SetSubMenu( aSubMenu );
    AddBitmapToMenuItem( item, aImage );

    aMenu->Append( item );

    return item;
}
