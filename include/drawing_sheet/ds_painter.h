/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

// For page and paper size, values are in 1/1000 inch

#ifndef DS_PAINTER_H
#define DS_PAINTER_H

#include <gal/color4d.h>
#include <gal/painter.h>
#include <page_info.h>
#include <drawing_sheet/ds_draw_item.h>

// Forward declarations:
class TITLE_BLOCK;

using KIGFX::COLOR4D;

namespace KIGFX
{

/**
 * Store page-layout-specific render settings.
 */
class DS_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class DS_PAINTER;

    DS_RENDER_SETTINGS();

    void LoadColors( const COLOR_SETTINGS* aSettings ) override;

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    inline bool IsBackgroundDark() const override
    {
        auto luma = m_backgroundColor.GetBrightness();
        return luma < 0.5;
    }

    const COLOR4D& GetBackgroundColor() const override { return m_backgroundColor; }
    void SetBackgroundColor( const COLOR4D& aColor ) override { m_backgroundColor = aColor; }

    void SetNormalColor( const COLOR4D& aColor ) { m_normalColor = aColor; }
    void SetSelectedColor( const COLOR4D& aColor ) { m_selectedColor = aColor; }
    void SetBrightenedColor( const COLOR4D& aColor ) { m_brightenedColor = aColor; }
    void SetPageBorderColor( const COLOR4D& aColor ) { m_pageBorderColor = aColor; }

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
    COLOR4D m_pageBorderColor;
};


/**
 * Methods for painting drawing sheet items.
 */
class DS_PAINTER : public PAINTER
{
public:
    DS_PAINTER( GAL* aGal ) :
            PAINTER( aGal )
    { }

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM*, int ) override;

    void DrawBorder( const PAGE_INFO* aPageInfo, int aScaleFactor ) const;

    /// @copydoc PAINTER::GetSettings()
    virtual RENDER_SETTINGS* GetSettings() override { return &m_renderSettings; }

private:
    void draw( const DS_DRAW_ITEM_LINE* aItem, int aLayer ) const;
    void draw( const DS_DRAW_ITEM_RECT* aItem, int aLayer ) const;
    void draw( const DS_DRAW_ITEM_POLYPOLYGONS* aItem, int aLayer ) const;
    void draw( const DS_DRAW_ITEM_TEXT* aItem, int aLayer ) const;
    void draw( const DS_DRAW_ITEM_BITMAP* aItem, int aLayer ) const;
    void draw( const DS_DRAW_ITEM_PAGE* aItem, int aLayer ) const;

private:
    DS_RENDER_SETTINGS m_renderSettings;
};

} // namespace KIGFX


/**
 * Print the border and title block.
 *
 * @param aDC The device context.
 * @param aPageInfo for margins and page size (in mils).
 * @param aSheetName The sheet name, for basic inscriptions.
 * @param aSheetPath The sheetpath (full sheet name), for basic inscriptions.
 * @param aFileName The file name, for basic inscriptions.
 * @param aTitleBlock The sheet title block, for text variable resolution.
 * @param aProperties Optional properties for text variable resolution.
 * @param aSheetCount The number of sheets (for text variable resolution).
 * @param aPageNumber The page number.
 * @param aScalar the scale factor to convert from mils to internal units.
 * @param aSheetLayer The layer from Pcbnew.
 * @param aIsFirstPage True when this is the first page.  This only has meaning for schematics.
 *
 * Parameters used in aPageInfo
 * - the size of the drawing sheet.
 * - the LTmargin The left top margin of the drawing sheet.
 * - the RBmargin The right bottom margin of the drawing sheet.
 */
void PrintDrawingSheet( const RENDER_SETTINGS* aSettings, const PAGE_INFO& aPageInfo,
                        const wxString& aSheetName, const wxString& aSheetPath,
                        const wxString& aFileName, const TITLE_BLOCK& aTitleBlock,
                        const std::map<wxString, wxString>* aProperties, int aSheetCount,
                        const wxString& aPageNumber, double aScalar, const PROJECT* aProject,
                        const wxString& aSheetLayer = wxEmptyString, bool aIsFirstPage = true );

#endif // DS_PAINTER_H
