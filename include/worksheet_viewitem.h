/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

/**
 * @file worksheet_viewitem.h
 * @brief Class that handles properties and drawing of worksheet layout.
 */

#ifndef WORKSHEET_VIEWITEM_H
#define WORKSHEET_VIEWITEM_H

#include <base_struct.h>

class BOARD;
class PAGE_INFO;
class TITLE_BLOCK;
class WS_DRAW_ITEM_LINE;
class WS_DRAW_ITEM_RECT;
class WS_DRAW_ITEM_POLYGON;
class WS_DRAW_ITEM_TEXT;

namespace KIGFX
{
class GAL;

class WORKSHEET_VIEWITEM : public EDA_ITEM
{
public:
    WORKSHEET_VIEWITEM( const PAGE_INFO* aPageInfo, const TITLE_BLOCK* aTitleBlock );

    /**
     * Function SetFileName()
     * Sets the file name displayed in the title block.
     *
     * @param aFileName is the new file name.
     */
    void SetFileName( const std::string& aFileName )
    {
        m_fileName = aFileName;
        ViewUpdate( GEOMETRY );
    }

    /**
     * Function SetSheetName()
     * Sets the sheet name displayed in the title block.
     *
     * @param aSheetName is the new sheet name.
     */
    void SetSheetName( const std::string& aSheetName )
    {
        m_sheetName = aSheetName;
        ViewUpdate( GEOMETRY );
    }

    /**
     * Function SetPageInfo()
     * Changes the PAGE_INFO object used to draw the worksheet.
     *
     * @param aPageInfo is the new PAGE_INFO object.
     */
    void SetPageInfo( const PAGE_INFO* aPageInfo );

    /**
     * Function SetTitleBlock()
     * Changes the TITLE_BLOCK object used to draw the worksheet.
     *
     * @param aTitleBlock is the new TITLE_BLOCK object.
     */
    void SetTitleBlock( const TITLE_BLOCK* aTitleBlock );

    /**
     * Function SetSheetNumber()
     * Changes the sheet number displayed in the title block.
     *
     * @param aSheetNumber is the new sheet number.
     */
    void SetSheetNumber( int aSheetNumber )
    {
        m_sheetNumber = aSheetNumber;
        ViewUpdate( GEOMETRY );

    }

    /**
     * Function SetSheetCount()
     * Changes the sheets count number displayed in the title block.
     *
     * @param aSheetCount is the new sheets count number.
     */
    void SetSheetCount( int aSheetCount )
    {
        m_sheetCount = aSheetCount;
        ViewUpdate( GEOMETRY );
    }

    /// @copydoc VIEW_ITEM::ViewBBox()
    const BOX2I ViewBBox() const;

    /// @copydoc VIEW_ITEM::ViewDraw()
    void ViewDraw( int aLayer, GAL* aGal ) const;

    /// @copydoc VIEW_ITEM::ViewGetLayers()
    void ViewGetLayers( int aLayers[], int& aCount ) const;

    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const
    {
    }

protected:
    /// File name displayed in the title block
    std::string m_fileName;

    /// Sheet name displayed in the title block
    std::string m_sheetName;

    /// Title block that contains properties of the title block displayed in the worksheet.
    const TITLE_BLOCK* m_titleBlock;

    /// Worksheet page information.
    const PAGE_INFO* m_pageInfo;

    /// Sheet number displayed in the title block.
    int m_sheetNumber;

    /// Sheets count number displayed in the title block.
    int m_sheetCount;

    // Functions for drawing items that makes a worksheet
    void draw( const WS_DRAW_ITEM_LINE* aItem, GAL* aGal ) const;
    void draw( const WS_DRAW_ITEM_RECT* aItem, GAL* aGal ) const;
    void draw( const WS_DRAW_ITEM_POLYGON* aItem, GAL* aGal ) const;
    void draw( const WS_DRAW_ITEM_TEXT* aItem, GAL* aGal ) const;

    /// Draws a border that determines the page size.
    void drawBorder( GAL* aGal ) const;
};
}

#endif /* WORKSHEET_VIEWITEM_H */
