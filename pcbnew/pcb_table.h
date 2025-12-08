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

#ifndef PCB_TABLE_H
#define PCB_TABLE_H


#include <pcb_tablecell.h>
#include <board_item.h>
#include <board_item_container.h>
#include <algorithm>

namespace KIGFX
{
class RENDER_SETTINGS;
};


class PCB_TABLE : public BOARD_ITEM_CONTAINER
{
public:
    PCB_TABLE( BOARD_ITEM* aParent, int aLineWidth );

    PCB_TABLE( const PCB_TABLE& aTable );

    ~PCB_TABLE();

    // If implemented, would need to copy m_cells list.
    PCB_TABLE& operator=( const PCB_TABLE& ) = delete;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_TABLE_T == aItem->Type();
    }

    virtual wxString GetClass() const override
    {
        return wxT( "PCB_TABLE" );
    }

    void SetStrokeExternal( bool aDoStroke ) { m_strokeExternal = aDoStroke; }
    bool StrokeExternal() const              { return m_strokeExternal; }

    void SetStrokeHeaderSeparator( bool aDoStroke ) { m_StrokeHeaderSeparator = aDoStroke; }
    bool StrokeHeaderSeparator() const              { return m_StrokeHeaderSeparator; }

    void SetBorderStroke( const STROKE_PARAMS& aParams ) { m_borderStroke = aParams; }
    const STROKE_PARAMS& GetBorderStroke() const { return m_borderStroke; }

    void SetBorderWidth( int aWidth ) { m_borderStroke.SetWidth( aWidth ); }
    int GetBorderWidth() const        { return m_borderStroke.GetWidth(); }

    void SetBorderStyle( const LINE_STYLE aStyle ) { m_borderStroke.SetLineStyle( aStyle ); }
    LINE_STYLE GetBorderStyle() const
    {
        if( m_borderStroke.GetLineStyle() == LINE_STYLE::DEFAULT )
            return LINE_STYLE::SOLID;
        else
            return m_borderStroke.GetLineStyle();
    }

    void SetBorderColor( const COLOR4D& aColor ) { m_borderStroke.SetColor( aColor ); }
    COLOR4D GetBorderColor() const               { return m_borderStroke.GetColor(); }

    void SetSeparatorsStroke( const STROKE_PARAMS& aParams ) { m_separatorsStroke = aParams; }
    const STROKE_PARAMS& GetSeparatorsStroke() const { return m_separatorsStroke; }

    void SetSeparatorsWidth( int aWidth ) { m_separatorsStroke.SetWidth( aWidth ); }
    int GetSeparatorsWidth() const        { return m_separatorsStroke.GetWidth(); }

    void SetSeparatorsStyle( const LINE_STYLE aStyle ) { m_separatorsStroke.SetLineStyle( aStyle ); }
    LINE_STYLE GetSeparatorsStyle() const
    {
        if( m_separatorsStroke.GetLineStyle() == LINE_STYLE::DEFAULT )
            return LINE_STYLE::SOLID;
        else
            return m_separatorsStroke.GetLineStyle();
    }

    void SetSeparatorsColor( const COLOR4D& aColor ) { m_separatorsStroke.SetColor( aColor ); }
    COLOR4D GetSeparatorsColor() const               { return m_separatorsStroke.GetColor(); }

    void SetStrokeColumns( bool aDoStroke ) { m_strokeColumns = aDoStroke; }
    bool StrokeColumns() const              { return m_strokeColumns; }

    void SetStrokeRows( bool aDoStroke ) { m_strokeRows = aDoStroke; }
    bool StrokeRows() const              { return m_strokeRows; }

    void RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const override;

    void SetPosition( const VECTOR2I& aPos ) override;
    VECTOR2I GetPosition() const override;
    VECTOR2I GetEnd() const;

    // For property manager:
    void SetPositionX( int x ) { SetPosition( VECTOR2I( x, GetPosition().y ) ); }
    void SetPositionY( int y ) { SetPosition( VECTOR2I( GetPosition().x, y ) ); }
    int GetPositionX() const   { return GetPosition().x; }
    int GetPositionY() const   { return GetPosition().y; }

    void SetColCount( int aCount ) { m_colCount = aCount; }
    int GetColCount() const { return m_colCount; }

    int GetRowCount() const
    {
        return m_cells.size() / m_colCount;
    }

    void SetColWidth( int aCol, int aWidth ) { m_colWidths[aCol] = aWidth; }

    int GetColWidth( int aCol ) const
    {
        if( m_colWidths.count( aCol ) )
            return m_colWidths.at( aCol );

        return 0;
    }

    void SetRowHeight( int aRow, int aHeight ) { m_rowHeights[aRow] = aHeight; }

    int GetRowHeight( int aRow ) const
    {
        if( m_rowHeights.count( aRow ) )
            return m_rowHeights.at( aRow );

        return 0;
    }

    PCB_TABLECELL* GetCell( int aRow, int aCol ) const
    {
        int idx = aRow * m_colCount + aCol;

        if( idx < (int) m_cells.size() )
            return m_cells[ idx ];
        else
            return nullptr;
    }

    std::vector<PCB_TABLECELL*> GetCells() const
    {
        return m_cells;
    }

    void AddCell( PCB_TABLECELL* aCell )
    {
        m_cells.push_back( aCell );
        aCell->SetLayer( GetLayer() );
        aCell->SetParent( this );
    }

    void InsertCell( int aIdx, PCB_TABLECELL* aCell )
    {
        m_cells.insert( m_cells.begin() + aIdx, aCell );
        aCell->SetLayer( GetLayer() );
        aCell->SetParent( this );
    }

    void ClearCells()
    {
        for( PCB_TABLECELL* cell : m_cells )
            delete cell;

        m_cells.clear();
    }

    void DeleteMarkedCells()
    {
        std::erase_if( m_cells,
                []( PCB_TABLECELL* cell )
                {
                    if( cell->GetFlags() & STRUCT_DELETED )
                    {
                        delete cell;
                        return true;
                    }
                    return false;
                } );
    }

    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_MODE::INSERT,
              bool aSkipConnectivity = false ) override
    {
        wxFAIL_MSG( wxT( "Use AddCell()/InsertCell() instead." ) );
    }

    void Remove( BOARD_ITEM* aItem, REMOVE_MODE aMode = REMOVE_MODE::NORMAL ) override
    {
        wxFAIL_MSG( wxT( "Use DeleteMarkedCells() instead." ) );
    }

    void Normalize() override;

    void Autosize();

    void Move( const VECTOR2I& aMoveVector ) override;

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    const BOX2I GetBoundingBox() const override;

    void DrawBorders( const std::function<void( const VECTOR2I& aPt1, const VECTOR2I& aPt2,
                                                const STROKE_PARAMS& aStroke )>& aCallback ) const;

    // @copydoc BOARD_ITEM::GetEffectiveShape
    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                                              FLASHING aFlash = FLASHING::DEFAULT ) const override;

    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                  int aMaxError, ERROR_LOC aErrorLoc,
                                  bool aIgnoreLineWidth = false ) const override;

   /**
     * Convert the TABLE shape to a polyset. details will be included.
     *
     * @param aBuffer a buffer to store the polygon.
     * @param aClearance the clearance around the pad.
     * @param aError the maximum deviation from true circle.
     * @param aErrorLoc should the approximation error be placed outside or inside the polygon?
     * @param aRenderSettings used to plot outlines with not solid segments like dashed lines.
     * If null, lines like dashed will be converted as SOLID
     */
    void TransformShapeToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                  int aClearance, int aError, ERROR_LOC aErrorLoc,
                                  KIGFX::RENDER_SETTINGS* aRenderSettings = nullptr ) const override;

    /**
     * Convert graphic items (segments and texts) to a set of polygonal shapes
     */
    void TransformGraphicItemsToPolySet( SHAPE_POLY_SET& aBuffer, int aMaxError, ERROR_LOC aErrorLoc,
                                         KIGFX::RENDER_SETTINGS* aRenderSettings ) const;

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        // Symbols are searchable via the child field and pin item text.
        return false;
    }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    EDA_ITEM* Clone() const override
    {
        return new PCB_TABLE( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_TABLE& aOther ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

    static int Compare( const PCB_TABLE* aTable, const PCB_TABLE* aOther );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

protected:
    bool                        m_strokeExternal;
    bool                        m_StrokeHeaderSeparator;
    STROKE_PARAMS               m_borderStroke;
    bool                        m_strokeRows;
    bool                        m_strokeColumns;
    STROKE_PARAMS               m_separatorsStroke;

    int                         m_colCount;
    std::map<int, int>          m_colWidths;
    std::map<int, int>          m_rowHeights;
    std::vector<PCB_TABLECELL*> m_cells;
};


#endif /* PCB_TABLE_H */
