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
class WS_DRAW_ITEM_TEXT;
class WS_DRAW_ITEM_BITMAP;
class WS_DRAW_ITEM_LIST;

namespace KIGFX
{
class VIEW;
class GAL;

class WS_PROXY_VIEW_ITEM : public EDA_ITEM
{
public:
    WS_PROXY_VIEW_ITEM( int aScaleFactor, const PAGE_INFO* aPageInfo, const PROJECT* aProject,
                        const TITLE_BLOCK* aTitleBlock );

    /**
     * Function SetFileName()
     * Sets the file name displayed in the title block.
     */
    void SetFileName( const std::string& aFileName ) { m_fileName = aFileName; }

    /**
     * Function SetSheetName()
     * Sets the sheet name displayed in the title block.
     */
    void SetSheetName( const std::string& aSheetName ) { m_sheetName = aSheetName; }

    /**
     * Function SetSheetNumber()
     * Changes the sheet number displayed in the title block.
     */
    void SetSheetNumber( int aSheetNumber ) { m_sheetNumber = aSheetNumber; }

    /**
     * Function SetSheetCount()
     * Changes the sheets count number displayed in the title block.
     */
    void SetSheetCount( int aSheetCount ) { m_sheetCount = aSheetCount; }

    /**
     * Can be used to override which layer ID is used for worksheet item colors
     * @param aLayerId is the color to use (will default to LAYER_WORKSHEET if this is not called)
     */
    void SetColorLayer( int aLayerId )
    {
        m_colorLayer = aLayerId;
    }

    const PAGE_INFO& GetPageInfo() { return *m_pageInfo; }
    const TITLE_BLOCK& GetTitleBlock() { return *m_titleBlock; }

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

    bool HitTestWorksheetItems( VIEW* aView, const wxPoint& aPosition );

protected:
    void buildDrawList( VIEW* aView, WS_DRAW_ITEM_LIST* aDrawList ) const;

    /// the factor between mils (units used in worksheet and internal units)
    /// it is the value IU_PER_MILS used in the caller
    int                m_mils2IUscalefactor;

    std::string        m_fileName;
    std::string        m_sheetName;
    const TITLE_BLOCK* m_titleBlock;
    const PAGE_INFO*   m_pageInfo;
    int                m_sheetNumber;
    int                m_sheetCount;
    const PROJECT*     m_project;

    /// Layer that is used for worksheet color (LAYER_WORKSHEET is always used for visibility)
    int                m_colorLayer;
};
}

#endif /* WS_PROXY_VIEW_ITEM_H */
