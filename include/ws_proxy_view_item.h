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

#ifndef WS_PROXY_VIEW_ITEM_H
#define WS_PROXY_VIEW_ITEM_H

#include <base_struct.h>

class BOARD;
class PAGE_INFO;
class TITLE_BLOCK;
class WS_DRAW_ITEM_LINE;
class WS_DRAW_ITEM_RECT;
class WS_DRAW_ITEM_POLYGON;
class WS_DRAW_ITEM_TEXT;
class WS_DRAW_ITEM_BITMAP;

namespace KIGFX
{
class VIEW;
class GAL;

class WS_PROXY_VIEW_ITEM : public EDA_ITEM
{
public:
    WS_PROXY_VIEW_ITEM( int aScaleFactor, const PAGE_INFO* aPageInfo,  const TITLE_BLOCK* aTitleBlock );

    /**
     * Function SetFileName()
     * Sets the file name displayed in the title block.
     *
     * @param aFileName is the new file name.
     */
    void SetFileName( const std::string& aFileName )
    {
        m_fileName = aFileName;
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
    }

    /// @copydoc VIEW_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    /// @copydoc VIEW_ITEM::ViewDraw()
    void ViewDraw( int aLayer, VIEW* aView ) const override;

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
        return wxT( "WS_PROXY_VIEW_ITEM" );
    }

protected:
    /// the factor between mils (units used in worksheet and internal units)
    /// it is the value IU_PER_MILS used in the caller
    int m_mils2IUscalefactor;

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
};
}

#endif /* WS_PROXY_VIEW_ITEM_H */
