/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/gdicmn.h>
#include <wx/mstream.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/aui/auibar.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <asset_archive.h>
#include <bitmaps.h>
#include <bitmap_store.h>
#include <pgm_base.h>
#include <paths.h>
#include <kiplatform/ui.h>
#include <math/util.h>
#include <settings/common_settings.h>

static std::unique_ptr<BITMAP_STORE> s_BitmapStore;


struct SCALED_BITMAP_ID {
    BITMAPS bitmap;
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
            // only makes things slower, not broken. BITMAPS is a pointer, so the most
            // significant several bits are generally going to be the same for all. Just convert
            // it to an integer and stuff the scale factor into those bits.
            return
                ( (uintptr_t)( id.bitmap ) & ~mask ) |
                ( ( (uintptr_t)( id.scale ) & 0xF ) << offset );
        }
    };
}


static std::unordered_map<SCALED_BITMAP_ID, wxBitmap> s_ScaledBitmapCache;

static std::mutex s_BitmapCacheMutex;


BITMAP_STORE* GetBitmapStore()
{
    if( !s_BitmapStore )
    {
        wxFileName path( PATHS::GetStockDataPath() + wxT( "/resources" ), wxT( "images.zip" ) );
        s_BitmapStore = std::make_unique<BITMAP_STORE>();
    }

    return s_BitmapStore.get();
}


wxBitmap KiBitmap( BITMAPS aBitmap, int aHeightTag )
{
    return GetBitmapStore()->GetBitmap( aBitmap, aHeightTag );
}


wxBitmapBundle KiBitmapBundle( BITMAPS aBitmap, int aMinHeight )
{
    return GetBitmapStore()->GetBitmapBundle( aBitmap, aMinHeight );
}


wxBitmapBundle KiBitmapBundleDef( BITMAPS aBitmap, int aDefHeight )
{
    return GetBitmapStore()->GetBitmapBundleDef( aBitmap, aDefHeight );
}


wxBitmapBundle KiDisabledBitmapBundle( BITMAPS aBitmap, int aMinHeight )
{
    return GetBitmapStore()->GetDisabledBitmapBundle( aBitmap, aMinHeight );
}


KICOMMON_API wxBitmapBundle KiDisabledBitmapBundleDef( BITMAPS aBitmap, int aDefHeight )
{
    return GetBitmapStore()->GetDisabledBitmapBundleDef( aBitmap, aDefHeight );
}


int KiIconScale( wxWindow* aWindow )
{
    // For historical reasons, "4" here means unity (no scaling)

#if defined( __WXMSW__)
    // Basically don't try and scale within KiCad and let wx do its thing
    // with wx introducing bitmap bundles, it will auto scale automatically with dpi
    // the issue is, none of the scaling factors have any tie to system scaling
    // this means wx is actually going to scale again causing even more distorted icons
    return 4;
#else
    const int vert_size = aWindow->ConvertDialogToPixels( wxSize( 0, 8 ) ).y;

    // Autoscale won't exceed unity until the system has quite high resolution,
    // because we don't want the icons to look obviously scaled on a system
    // where it's easy to see it.

    if( vert_size > 34 )        return 8;
    else if( vert_size > 29 )   return 7;
    else if( vert_size > 24 )   return 6;
    else                        return 4;
#endif
}


wxBitmap KiScaledBitmap( BITMAPS aBitmap, wxWindow* aWindow, int aHeight, bool aQuantized )
{
    // Bitmap conversions are cached because they can be slow.
    int scale = KiIconScale( aWindow );

    if( aQuantized )
        scale = KiROUND( (double) scale / 4.0 ) * 4;

    SCALED_BITMAP_ID id = { static_cast<BITMAPS>( aBitmap ), scale };

    std::lock_guard<std::mutex> guard( s_BitmapCacheMutex );
    auto it = s_ScaledBitmapCache.find( id );

    if( it != s_ScaledBitmapCache.end() )
    {
        return it->second;
    }
    else
    {
        wxBitmap bitmap = GetBitmapStore()->GetBitmapScaled( aBitmap, scale, aHeight );
        return s_ScaledBitmapCache.emplace( id, bitmap ).first->second;
    }
}


void ClearScaledBitmapCache()
{
    std::lock_guard<std::mutex> guard( s_BitmapCacheMutex );
    s_ScaledBitmapCache.clear();
}


wxBitmap KiScaledBitmap( const wxBitmap& aBitmap, wxWindow* aWindow )
{
    const int scale = KiIconScale( aWindow );

    if( scale == 4 )
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


wxBitmap* KiBitmapNew( BITMAPS aBitmap )
{
    wxBitmap* bitmap = new wxBitmap( GetBitmapStore()->GetBitmap( aBitmap ) );

    return bitmap;
}
