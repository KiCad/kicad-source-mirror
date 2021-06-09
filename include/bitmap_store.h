/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps/bitmap_info.h>

class ASSET_ARCHIVE;
class wxImage;


namespace std
{
    template<> struct hash<std::pair<BITMAPS, int>>
    {
        size_t operator()( const std::pair<BITMAPS, int>& aPair ) const;
    };
}

/**
 * Helper to retrieve bitmaps while handling icon themes and scaling
 */
class BITMAP_STORE
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
     * @return true if the new theme is different than what was previously in use
     */
    bool ThemeChanged();

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
