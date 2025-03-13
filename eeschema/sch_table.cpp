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

#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <plotters/plotter.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <schematic.h>
#include <settings/color_settings.h>
#include <sch_painter.h>
#include <wx/log.h>
#include <sch_table.h>


SCH_TABLE::SCH_TABLE( int aLineWidth ) :
        SCH_ITEM( nullptr, SCH_TABLE_T ),
        m_strokeExternal( true ),
        m_StrokeHeaderSeparator( true ),
        m_borderStroke( aLineWidth, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_strokeRows( true ),
        m_strokeColumns( true ),
        m_separatorsStroke( aLineWidth, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_colCount( 0 )
{
    SetLayer( LAYER_NOTES );
}


SCH_TABLE::SCH_TABLE( const SCH_TABLE& aTable ) :
        SCH_ITEM( aTable )
{
    m_strokeExternal = aTable.m_strokeExternal;
    m_StrokeHeaderSeparator = aTable.m_StrokeHeaderSeparator;
    m_borderStroke = aTable.m_borderStroke;
    m_strokeRows = aTable.m_strokeRows;
    m_strokeColumns = aTable.m_strokeColumns;
    m_separatorsStroke = aTable.m_separatorsStroke;

    m_colCount = aTable.m_colCount;
    m_colWidths = aTable.m_colWidths;
    m_rowHeights = aTable.m_rowHeights;

    for( SCH_TABLECELL* src : aTable.m_cells )
        AddCell( new SCH_TABLECELL( *src ) );
}


SCH_TABLE::~SCH_TABLE()
{
    // We own our cells; delete them
    for( SCH_TABLECELL* cell : m_cells )
        delete cell;
}


void SCH_TABLE::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    wxCHECK_RET( aItem != nullptr && aItem->Type() == SCH_TABLE_T,
                 wxT( "Cannot swap data with invalid table." ) );

    SCH_TABLE* table = static_cast<SCH_TABLE*>( aItem );

    std::swap( m_strokeExternal, table->m_strokeExternal );
    std::swap( m_StrokeHeaderSeparator, table->m_StrokeHeaderSeparator );
    std::swap( m_borderStroke, table->m_borderStroke );
    std::swap( m_strokeRows, table->m_strokeRows );
    std::swap( m_strokeColumns, table->m_strokeColumns );
    std::swap( m_separatorsStroke, table->m_separatorsStroke );

    std::swap( m_colCount, table->m_colCount );
    std::swap( m_colWidths, table->m_colWidths );
    std::swap( m_rowHeights, table->m_rowHeights );

    std::swap( m_cells, table->m_cells );

    for( SCH_TABLECELL* cell : m_cells )
        cell->SetParent( this );

    for( SCH_TABLECELL* cell : table->m_cells )
        cell->SetParent( table );
}


void SCH_TABLE::SetPosition( const VECTOR2I& aPos )
{
    Move( aPos - GetPosition() );
}


VECTOR2I SCH_TABLE::GetPosition() const
{
    return m_cells[0]->GetPosition();
}


VECTOR2I SCH_TABLE::GetCenter() const
{
    BOX2I bbox;

    for( SCH_TABLECELL* cell : m_cells )
    {
        bbox.Merge( cell->GetPosition() );
        bbox.Merge( cell->GetEnd() );
    }

    return bbox.GetCenter();
}


VECTOR2I SCH_TABLE::GetEnd() const
{
    VECTOR2I tableSize;

    for( int ii = 0; ii < GetColCount(); ++ii )
        tableSize.x += GetColWidth( ii );

    for( int ii = 0; ii < GetRowCount(); ++ii )
        tableSize.y += GetRowHeight( ii );

    return GetPosition() + tableSize;
}


void SCH_TABLE::Normalize()
{
    int y = GetPosition().y;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        int x = GetPosition().x;
        int rowHeight = m_rowHeights[ row ];

        for( int col = 0; col < GetColCount(); ++col )
        {
            int colWidth = m_colWidths[ col ];

            SCH_TABLECELL* cell = GetCell( row, col );
            VECTOR2I       pos( x, y );

            RotatePoint( pos, GetPosition(), cell->GetTextAngle() );

            if( cell->GetPosition() != pos )
            {
                cell->SetPosition( pos );
                cell->ClearRenderCache();
            }

            VECTOR2I end = VECTOR2I( x + colWidth, y + rowHeight );

            if( cell->GetColSpan() > 1 || cell->GetRowSpan() > 1 )
            {
                for( int ii = col + 1; ii < col + cell->GetColSpan(); ++ii )
                    end.x += m_colWidths[ii];

                for( int ii = row + 1; ii < row + cell->GetRowSpan(); ++ii )
                    end.y += m_rowHeights[ii];
            }

            RotatePoint( end, GetPosition(), cell->GetTextAngle() );

            if( cell->GetEnd() != end )
            {
                cell->SetEnd( end );
                cell->ClearRenderCache();
            }

            x += colWidth;
        }

        y += rowHeight;
    }
}


void SCH_TABLE::Move( const VECTOR2I& aMoveVector )
{
    for( SCH_TABLECELL* cell : m_cells )
        cell->Move( aMoveVector );
}


void SCH_TABLE::MirrorHorizontally( int aCenter )
{
    // We could mirror all the cells, but it doesn't seem useful....
}


void SCH_TABLE::MirrorVertically( int aCenter )
{
    // We could mirror all the cells, but it doesn't seem useful....
}


void SCH_TABLE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    for( SCH_TABLECELL* cell : m_cells )
        cell->Rotate( aCenter, aRotateCCW );

    Normalize();
}


bool SCH_TABLE::operator<( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    const SCH_TABLE& other = static_cast<const SCH_TABLE&>( aItem );

    if( m_cells.size() != other.m_cells.size() )
        return m_cells.size() < other.m_cells.size();

    if( GetPosition().x != other.GetPosition().x )
        return GetPosition().x < GetPosition().x;

    if( GetPosition().y != GetPosition().y )
        return GetPosition().y < GetPosition().y;

    return m_cells[0] < other.m_cells[0];
}


void SCH_TABLE::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction )
{
    for( SCH_TABLECELL* cell : m_cells )
        aFunction( cell );
}


void SCH_TABLE::PrintBackground( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                                 const VECTOR2I& aOffset, bool aDimmed )
{
    for( SCH_TABLECELL* cell : m_cells )
        cell->PrintBackground( aSettings, aUnit, aBodyStyle, aOffset, aDimmed );
}


void SCH_TABLE::Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                       const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed )
{
    for( SCH_TABLECELL* cell : m_cells )
        cell->Print( aSettings, aUnit, aBodyStyle, aOffset, aForceNoFill, aDimmed );

    wxDC*      DC = aSettings->GetPrintDC();
    VECTOR2I   pos = GetPosition();
    VECTOR2I   end = GetEnd();
    int        lineWidth;
    COLOR4D    color;
    LINE_STYLE lineStyle;

    auto setupStroke =
            [&]( const STROKE_PARAMS& stroke )
            {
                lineWidth = stroke.GetWidth();
                color = stroke.GetColor();
                lineStyle = stroke.GetLineStyle();

                if( lineWidth == 0 )
                    lineWidth = aSettings->GetDefaultPenWidth();

                if( color == COLOR4D::UNSPECIFIED )
                    color = aSettings->GetLayerColor( LAYER_NOTES );

                if( lineStyle == LINE_STYLE::DEFAULT )
                    lineStyle = LINE_STYLE::SOLID;
            };

    auto strokeShape =
            [&]( const SHAPE& shape )
            {
                STROKE_PARAMS::Stroke( &shape, lineStyle, lineWidth, aSettings,
                        [&]( const VECTOR2I& a, const VECTOR2I& b )
                        {
                            GRLine( DC, a.x, a.y, b.x, b.y, lineWidth, color );
                        } );
            };

    auto strokeLine =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    GRLine( DC, ptA.x, ptA.y, ptB.x, ptB.y, lineWidth, color );
                }
                else
                {
                    SHAPE_SEGMENT seg( ptA, ptB );
                    strokeShape( seg );
                }
            };

    auto strokeRect =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    GRRect( DC, ptA, ptB, lineWidth, color );
                }
                else
                {
                    SHAPE_RECT rect( BOX2I( ptA, ptB - ptA ) );
                    strokeShape( rect );
                }
            };

    if( GetSeparatorsStroke().GetWidth() >= 0 )
    {
        setupStroke( GetSeparatorsStroke() );

        if( StrokeColumns() )
        {
            for( int col = 0; col < GetColCount() - 1; ++col )
            {
                for( int row = 0; row < GetRowCount(); ++row )
                {
                    SCH_TABLECELL* cell = GetCell( row, col );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    {
                        strokeLine( VECTOR2I( cell->GetEndX(), cell->GetStartY() ),
                                    VECTOR2I( cell->GetEndX(), cell->GetEndY() ) );
                    }
                }
            }
        }

        if( StrokeRows() )
        {
            for( int row = 0; row < GetRowCount() - 1; ++row )
            {
                for( int col = 0; col < GetColCount(); ++col )
                {
                    SCH_TABLECELL* cell = GetCell( row, 0 );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    {
                        strokeLine( VECTOR2I( cell->GetStartX(), cell->GetEndY() ),
                                    VECTOR2I( cell->GetEndX(), cell->GetEndY() ) );
                    }
                }
            }
        }
    }

    if( GetBorderStroke().GetWidth() >= 0 )
    {
        setupStroke( GetBorderStroke() );

        if( StrokeHeaderSeparator() )
        {
            SCH_TABLECELL* cell = GetCell( 0, 0 );
            strokeLine( VECTOR2I( pos.x, cell->GetEndY() ), VECTOR2I( end.x, cell->GetEndY() ) );
        }

        if( StrokeExternal() )
            strokeRect( pos, end );
    }
}


const BOX2I SCH_TABLE::GetBoundingBox() const
{
    // Note: a table with no cells is not allowed
    BOX2I bbox = m_cells[0]->GetBoundingBox();

    bbox.Merge( m_cells[ m_cells.size() - 1 ]->GetBoundingBox() );

    return bbox;
}


INSPECT_RESULT SCH_TABLE::Visit( INSPECTOR aInspector, void* aTestData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_TABLE_T )
        {
            if( INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == SCH_LOCATE_ANY_T || scanType == SCH_TABLECELL_T )
        {
            for( SCH_TABLECELL* cell : m_cells )
            {
                if( INSPECT_RESULT::QUIT == aInspector( cell, (void*) this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


wxString SCH_TABLE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "%d Column Table" ), m_colCount );
}


BITMAPS SCH_TABLE::GetMenuImage() const
{
    return BITMAPS::spreadsheet;    // JEY TODO
}


std::vector<int> SCH_TABLE::ViewGetLayers() const
{
    return { LAYER_NOTES, LAYER_NOTES_BACKGROUND, LAYER_SELECTION_SHADOWS };
}


bool SCH_TABLE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool SCH_TABLE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_TABLE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                      int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    for( SCH_TABLECELL* cell : m_cells )
        cell->Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );

    if( aBackground )
        return;

    RENDER_SETTINGS* settings = aPlotter->RenderSettings();
    VECTOR2I         pos = GetPosition();
    VECTOR2I         end = GetEnd();
    int              lineWidth;
    COLOR4D          color;
    LINE_STYLE       lineStyle;

    auto setupStroke =
            [&]( const STROKE_PARAMS& stroke )
            {
                lineWidth = stroke.GetWidth();
                color = stroke.GetColor();
                lineStyle = stroke.GetLineStyle();

                if( lineWidth == 0 )
                {
                    if( SCHEMATIC* schematic = Schematic() )
                        lineWidth = schematic->Settings().m_DefaultLineWidth;
                    else
                        lineWidth = schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
                }

                if( lineWidth < settings->GetMinPenWidth() )
                    lineWidth = settings->GetMinPenWidth();

                if( !aPlotter->GetColorMode() || color == COLOR4D::UNSPECIFIED )
                    color = settings->GetLayerColor( m_layer );

                if( lineStyle == LINE_STYLE::DEFAULT )
                    lineStyle = LINE_STYLE::SOLID;

                aPlotter->SetColor( color );
                aPlotter->SetDash( lineWidth, lineStyle );
            };

    if( GetSeparatorsStroke().GetWidth() >= 0 )
    {
        setupStroke( GetSeparatorsStroke() );

        if( StrokeColumns() )
        {
            for( int col = 0; col < GetColCount() - 1; ++col )
            {
                for( int row = 0; row < GetRowCount(); ++row )
                {
                    SCH_TABLECELL* cell = GetCell( row, col );
                    VECTOR2I       topRight( cell->GetEndX(), cell->GetStartY() );

                    if( !cell->GetTextAngle().IsHorizontal() )
                        topRight = VECTOR2I( cell->GetStartX(), cell->GetEndY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    {
                        aPlotter->MoveTo( topRight );
                        aPlotter->FinishTo( VECTOR2I( cell->GetEndX(), cell->GetEndY() ) );
                    }
                }
            }
        }

        if( StrokeRows() )
        {
            for( int row = 0; row < GetRowCount() - 1; ++row )
            {
                for( int col = 0; col < GetColCount(); ++col )
                {
                    SCH_TABLECELL* cell = GetCell( row, col );
                    VECTOR2I       botLeft( cell->GetStartX(), cell->GetEndY() );

                    if( !cell->GetTextAngle().IsHorizontal() )
                        botLeft = VECTOR2I( cell->GetEndX(), cell->GetStartY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    {
                        aPlotter->MoveTo( botLeft );
                        aPlotter->FinishTo( VECTOR2I( cell->GetEndX(), cell->GetEndY() ) );
                    }
                }
            }
        }
    }

    if( GetBorderStroke().GetWidth() >= 0 )
    {
        setupStroke( GetBorderStroke() );
        SCH_TABLECELL* cell = GetCell( 0, 0 );

        if( StrokeHeaderSeparator() )
        {
            if( !cell->GetTextAngle().IsHorizontal() )
            {
                aPlotter->MoveTo( VECTOR2I( cell->GetEndX(), pos.y ) );
                aPlotter->FinishTo( VECTOR2I( cell->GetEndX(), cell->GetEndY() ) );
            }
            else
            {
                aPlotter->MoveTo( VECTOR2I( pos.x, cell->GetEndY() ) );
                aPlotter->FinishTo( VECTOR2I( end.x, cell->GetEndY() ) );
            }
         }

        if( StrokeExternal() )
        {
            RotatePoint( pos, GetPosition(), cell->GetTextAngle() );
            RotatePoint( end, GetPosition(), cell->GetTextAngle() );

            aPlotter->Rect( pos, end, FILL_T::NO_FILL, lineWidth );
        }
    }
}


void SCH_TABLE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Table" ), wxString::Format( _( "%d Columns" ), m_colCount ) );
}


bool SCH_TABLE::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_TABLE& other = static_cast<const SCH_TABLE&>( aOther );

    if( m_cells.size() != other.m_cells.size() )
        return false;

    if( m_colWidths != other.m_colWidths )
        return false;

    if( m_rowHeights != other.m_rowHeights )
        return false;

    for( int ii = 0; ii < (int) m_cells.size(); ++ii )
    {
        if( !( *m_cells[ii] == *other.m_cells[ii] ) )
            return false;
    }

    return true;
}


double SCH_TABLE::Similarity( const SCH_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const SCH_TABLE& other = static_cast<const SCH_TABLE&>( aOther );

    if( m_cells.size() != other.m_cells.size() )
        return 0.1;

    double similarity = 1.0;

    for( int ii = 0; ii < (int) m_cells.size(); ++ii )
        similarity *= m_cells[ii]->Similarity( *other.m_cells[ii] );

    return similarity;
}


static struct SCH_TABLE_DESC
{
    SCH_TABLE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_TABLE );

        propMgr.AddTypeCast( new TYPE_CAST<SCH_TABLE, SCH_ITEM> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_TABLE ), TYPE_HASH( SCH_ITEM ) );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, int>( _HKI( "Start X" ),
                    &SCH_TABLE::SetPositionX, &SCH_TABLE::GetPositionX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );
        propMgr.AddProperty( new PROPERTY<SCH_TABLE, int>( _HKI( "Start Y" ),
                    &SCH_TABLE::SetPositionY, &SCH_TABLE::GetPositionY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );

        const wxString tableProps = _( "Table Properties" );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, bool>( _HKI( "External Border" ),
                    &SCH_TABLE::SetStrokeExternal, &SCH_TABLE::StrokeExternal ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, bool>( _HKI( "Header Border" ),
                    &SCH_TABLE::SetStrokeHeaderSeparator, &SCH_TABLE::StrokeHeaderSeparator ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, int>( _HKI( "Border Width" ),
                    &SCH_TABLE::SetBorderWidth, &SCH_TABLE::GetBorderWidth,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_TABLE, LINE_STYLE>( _HKI( "Border Style" ),
                    &SCH_TABLE::SetBorderStyle, &SCH_TABLE::GetBorderStyle ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, COLOR4D>( _HKI( "Border Color" ),
                    &SCH_TABLE::SetBorderColor, &SCH_TABLE::GetBorderColor ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, bool>( _HKI( "Row Separators" ),
                    &SCH_TABLE::SetStrokeRows, &SCH_TABLE::StrokeRows ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, bool>( _HKI( "Cell Separators" ),
                    &SCH_TABLE::SetStrokeColumns, &SCH_TABLE::StrokeColumns ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, int>( _HKI( "Separators Width" ),
                    &SCH_TABLE::SetSeparatorsWidth, &SCH_TABLE::GetSeparatorsWidth,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_TABLE, LINE_STYLE>( _HKI( "Separators Style" ),
                    &SCH_TABLE::SetSeparatorsStyle, &SCH_TABLE::GetSeparatorsStyle ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<SCH_TABLE, COLOR4D>( _HKI( "Separators Color" ),
                    &SCH_TABLE::SetSeparatorsColor, &SCH_TABLE::GetSeparatorsColor ),
                    tableProps );
    }
} _SCH_TABLE_DESC;
