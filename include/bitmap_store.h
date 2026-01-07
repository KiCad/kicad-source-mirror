/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_BITMAP_STORE_H
#define KICAD_BITMAP_STORE_H

#include <memory>
#include <unordered_map>

#include <wx/bmpbndl.h>

#include <bitmaps/bitmap_info.h>
#include <kicommon.h>

class ASSET_ARCHIVE;
class wxImage;


namespace std
{
    template<> struct KICOMMON_API hash<std::pair<BITMAPS, int>>
    {
        size_t operator()( const std::pair<BITMAPS, int>& aPair ) const;
    };
}

/**
 * Helper to retrieve bitmaps while handling icon themes and scaling
 */
class KICOMMON_API BITMAP_STORE
{
public:
    BITMAP_STORE();

    ~BITMAP_STORE() = default;

    /**
     * Retrieves a bitmap from the given bitmap id
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aHeight is the requested height in pixels of the source image, or -1 for any height
     */
    wxBitmap GetBitmap( BITMAPS aBitmapId, int aHeight = -1 );

    /**
     * Constructs and returns a bitmap bundle containing all available sizes of the given ID
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aMinHeight is the minimum height of the bitmaps to include in the bundle.
     * This is important for uses of GetPreferredBitmap and more on wxBitmap bundles because
     * wx assumes the smallest bitmap is the "original" intended size. This is a problem where
     * some icons may be reused between controls at different intended sizes.
     */
    wxBitmapBundle GetBitmapBundle( BITMAPS aBitmapId, int aMinHeight = -1 );

    /**
     * Constructs and returns a bitmap bundle for the given icon ID, with the default
     * bitmap size being aDefHeight. Rescaling is applied if no bitmap of that size
     * is available.
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aDefHeight is the desired height of the default bitmap in the bundle.
     */
    wxBitmapBundle GetBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight );

    /**
     * Constructs and returns a bitmap bundle for the given icon ID, with the bitmaps
     * converted to disabled state according to the current UI theme.
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aMinHeight is the minimum height of the bitmaps to include in the bundle.
     */
    wxBitmapBundle GetDisabledBitmapBundle( BITMAPS aBitmapId, int aMinHeight = -1 );

    /**
     * Constructs and returns a bitmap bundle for the given icon ID, with the bitmaps
     * converted to disabled state according to the current UI theme and the default
     * bitmap size being aDefHeight. Rescaling is applied if no bitmap of that size
     * is available.
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aDefHeight is the desired height of the default bitmap in the bundle.
     */
    wxBitmapBundle GetDisabledBitmapBundleDef( BITMAPS aBitmapId, int aDefHeight );

    /**
     * Retrieves a bitmap from the given bitmap id, scaled to a given factor.
     *
     * This factor is for legacy reasons divided by 4, so a scale factor of 4 will return the
     * original image.
     *
     * @todo this should be improved to take advantage of a number of different resolution PNGs
     * stored in the asset archive, so we take the closest PNG and scale it rather than always
     * starting with a low-resolution version.
     *
     * @param aBitmapId is from the BITMAPS enum in bitmaps_list.h
     * @param aScaleFactor is used to scale the bitmap uniformly
     * @param aHeight is the requested height in pixels of the source image to scale from
     */
    wxBitmap GetBitmapScaled( BITMAPS aBitmapId, int aScaleFactor, int aHeight = -1 );

    /**
     * Notifies the store that the icon theme has been changed by the user, so caches must be
     * invalidated.
     */
    void ThemeChanged();

    bool IsDarkTheme() const { return m_theme == wxT( "dark" ); }

private:

    wxImage getImage( BITMAPS aBitmapId, int aHeight = -1 );

    const wxString& bitmapName( BITMAPS aBitmapId, int aHeight = -1 );

    wxString computeBitmapName( BITMAPS aBitmapId, int aHeight = -1 );

    void buildBitmapInfoCache();

    std::unique_ptr<ASSET_ARCHIVE> m_archive;

    std::unordered_map<std::pair<BITMAPS, int>, wxString> m_bitmapNameCache;

    std::unordered_map<BITMAPS, std::vector<BITMAP_INFO>> m_bitmapInfoCache;

    wxString m_theme;
};

#endif // KICAD_BITMAP_STORE_H
