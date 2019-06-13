/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

// For page and paper size, values are in 1/1000 inch

#ifndef WS_PAINTER_H
#define WS_PAINTER_H

#include <gal/color4d.h>
#include <painter.h>
#include <page_info.h>

// Forward declarations:
class EDA_RECT;
class TITLE_BLOCK;

using KIGFX::COLOR4D;

namespace KIGFX
{

/**
 * Class WS_RENDER_SETTINGS
 * Stores page-layout-specific render settings.
 */
class WS_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class WS_PAINTER;

    WS_RENDER_SETTINGS();

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual const COLOR4D& GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    inline bool IsBackgroundDark() const override
    {
        auto luma = m_backgroundColor.GetBrightness();
        return luma < 0.5;
    }

    const COLOR4D& GetBackgroundColor() override { return m_backgroundColor; }
    void SetBackgroundColor( const COLOR4D& aColor ) override { m_backgroundColor = aColor; }

    void SetNormalColor( const COLOR4D& aColor ) { m_normalColor = aColor; }
    void SetSelectedColor( const COLOR4D& aColor ) { m_selectedColor = aColor; }
    void SetBrightenedColor( const COLOR4D& aColor ) { m_brightenedColor = aColor; }

    const COLOR4D& GetGridColor() override
    {
        m_gridColor = IsBackgroundDark() ? DARKGRAY : LIGHTGRAY;
        return m_gridColor;
    }

    const COLOR4D& GetCursorColor() override
    {
        m_cursorColor = IsBackgroundDark() ? WHITE : BLACK;
        return m_cursorColor;
    }

private:
    COLOR4D m_normalColor;
    COLOR4D m_selectedColor;
    COLOR4D m_brightenedColor;

    COLOR4D m_gridColor;
    COLOR4D m_cursorColor;
};


/**
 * Class WS_PAINTER
 * Contains methods for drawing worksheet items.
 */
class WS_PAINTER : public PAINTER
{
public:
    WS_PAINTER( GAL* aGal ) :
            PAINTER( aGal )
    { }

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM*, int ) override;

    void DrawBorder( const PAGE_INFO* aPageInfo, int aScaleFactor ) const;

    /// @copydoc PAINTER::ApplySettings()
    virtual void ApplySettings( const RENDER_SETTINGS* aSettings ) override
    {
        m_renderSettings = *static_cast<const WS_RENDER_SETTINGS*>( aSettings );
    }

    /// @copydoc PAINTER::GetSettings()
    virtual RENDER_SETTINGS* GetSettings() override { return &m_renderSettings; }

private:
    void draw( const WS_DRAW_ITEM_LINE* aItem, int aLayer ) const;
    void draw( const WS_DRAW_ITEM_RECT* aItem, int aLayer ) const;
    void draw( const WS_DRAW_ITEM_POLYPOLYGONS* aItem, int aLayer ) const;
    void draw( const WS_DRAW_ITEM_TEXT* aItem, int aLayer ) const;
    void draw( const WS_DRAW_ITEM_BITMAP* aItem, int aLayer ) const;
    void draw( const WS_DRAW_ITEM_PAGE* aItem, int aLayer ) const;

private:
    WS_RENDER_SETTINGS m_renderSettings;
};

} // namespace KIGFX


/**
 * Function PrintPageLayout is a core function to print the page layout with the frame and the
 * basic inscriptions.
 * @param aDC The device context.
 * @param aPageInfo for margins and page size (in mils).
 * @param aFullSheetName The sheetpath (full sheet name), for basic inscriptions.
 * @param aFileName The file name, for basic inscriptions.
 * @param aTitleBlock The sheet title block, for basic inscriptions.
 * @param aSheetCount The number of sheets (for basic inscriptions).
 * @param aSheetNumber The sheet number (for basic inscriptions).
 * @param aPenWidth the pen size The line width for drawing.
 * @param aScalar the scale factor to convert from mils to internal units.
 * @param aColor The color for drawing.
 * @param aSheetLayer The layer from pcbnew.
 *
 * Parameters used in aPageInfo
 * - the size of the page layout.
 * - the LTmargin The left top margin of the page layout.
 * - the RBmargin The right bottom margin of the page layout.
 */
void PrintPageLayout( wxDC* aDC, const PAGE_INFO& aPageInfo, const wxString& aFullSheetName,
                      const wxString& aFileName, const TITLE_BLOCK& aTitleBlock, int aSheetCount,
                      int aSheetNumber, int aPenWidth, double aScalar, COLOR4D aColor,
                      const wxString& aSheetLayer = wxEmptyString );

#endif // WS_PAINTER_H
