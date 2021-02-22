/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef DS_PROXY_VIEW_ITEM_H
#define DS_PROXY_VIEW_ITEM_H

#include <eda_item.h>

class BOARD;
class PAGE_INFO;
class PROJECT;
class TITLE_BLOCK;
class DS_DRAW_ITEM_LINE;
class DS_DRAW_ITEM_RECT;
class DS_DRAW_ITEM_TEXT;
class DS_DRAW_ITEM_BITMAP;
class DS_DRAW_ITEM_LIST;

namespace KIGFX
{
class VIEW;
class GAL;
}

class DS_PROXY_VIEW_ITEM : public EDA_ITEM
{
public:
    DS_PROXY_VIEW_ITEM( int aScaleFactor, const PAGE_INFO* aPageInfo, const PROJECT* aProject,
                        const TITLE_BLOCK* aTitleBlock );

    /**
     * Set the file name displayed in the title block.
     */
    void SetFileName( const std::string& aFileName ) { m_fileName = aFileName; }

    /**
     * Set the sheet name displayed in the title block.
     */
    void SetSheetName( const std::string& aSheetName ) { m_sheetName = aSheetName; }

    /**
     * Changes the page number displayed in the title block.
     */
    void SetPageNumber( const std::string& aPageNumber ) { m_pageNumber = aPageNumber; }

    /**
     * Changes the sheet-count number displayed in the title block.
     */
    void SetSheetCount( int aSheetCount ) { m_sheetCount = aSheetCount; }

    /**
     * Change if this is first page.
     *
     * Title blocks have an option to allow all subsequent pages to not display a title
     * block.  This needs to be set to false when displaying any page but the first page.
     */
    void SetIsFirstPage( bool aIsFirstPage ) { m_isFirstPage = aIsFirstPage; }

    /**
     * Can be used to override which layer ID is used for worksheet item colors
     * @param aLayerId is the color to use (defaults to LAYER_DRAWINGSHEET if this is not called)
     */
    void SetColorLayer( int aLayerId )
    {
        m_colorLayer = aLayerId;
    }

    /**
     * Overrides the layer used to pick the color of the page border (normally LAYER_GRID)
     * @param aLayerId is the layer to use
     */
    void SetPageBorderColorLayer( int aLayerId )
    {
        m_pageBorderColorLayer = aLayerId;
    }

    const PAGE_INFO& GetPageInfo() { return *m_pageInfo; }
    const TITLE_BLOCK& GetTitleBlock() { return *m_titleBlock; }

    /// @copydoc VIEW_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    /// @copydoc VIEW_ITEM::ViewDraw()
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    void ViewGetLayers( int aLayers[], int& aCount ) const override;

#if defined(DEBUG)
    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const override { }
#endif

    /** Get class name
     * @return  string "WORKSHEET_VIEWITEM"
     */
    virtual wxString GetClass() const override
    {
        return wxT( "DS_PROXY_VIEW_ITEM" );
    }

    bool HitTestDrawingSheetItems( KIGFX::VIEW* aView, const wxPoint& aPosition );

protected:
    void buildDrawList( KIGFX::VIEW* aView, DS_DRAW_ITEM_LIST* aDrawList ) const;

    /// the factor between mils (units used in worksheet and internal units)
    /// it is the value IU_PER_MILS used in the caller
    int                m_mils2IUscalefactor;

    std::string        m_fileName;
    std::string        m_sheetName;
    const TITLE_BLOCK* m_titleBlock;
    const PAGE_INFO*   m_pageInfo;
    std::string        m_pageNumber;
    int                m_sheetCount;
    bool               m_isFirstPage;
    const PROJECT*     m_project;

    /// Layer that is used for worksheet color (LAYER_DRAWINGSHEET is always used for visibility)
    int                m_colorLayer;

    /// Layer that is used for page border color
    int                m_pageBorderColorLayer;
};

#endif /* DS_PROXY_VIEW_ITEM_H */
