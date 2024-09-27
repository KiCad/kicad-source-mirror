/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <footprint.h>
#include <pcb_table.h>
#include <board.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>


PCB_TABLE::PCB_TABLE( BOARD_ITEM* aParent, int aLineWidth ) :
        BOARD_ITEM_CONTAINER( aParent, PCB_TABLE_T ),
        m_strokeExternal( true ),
        m_strokeHeader( true ),
        m_borderStroke( aLineWidth, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_strokeRows( true ),
        m_strokeColumns( true ),
        m_separatorsStroke( aLineWidth, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_colCount( 0 )
{
}


PCB_TABLE::PCB_TABLE( const PCB_TABLE& aTable ) :
        BOARD_ITEM_CONTAINER( aTable )
{
    m_strokeExternal = aTable.m_strokeExternal;
    m_strokeHeader = aTable.m_strokeHeader;
    m_borderStroke = aTable.m_borderStroke;
    m_strokeRows = aTable.m_strokeRows;
    m_strokeColumns = aTable.m_strokeColumns;
    m_separatorsStroke = aTable.m_separatorsStroke;

    m_orientation = aTable.m_orientation;
    m_colCount = aTable.m_colCount;
    m_colWidths = aTable.m_colWidths;
    m_rowHeights = aTable.m_rowHeights;

    for( PCB_TABLECELL* src : aTable.m_cells )
        AddCell( new PCB_TABLECELL( *src ) );
}


PCB_TABLE::~PCB_TABLE()
{
    // We own our cells; delete them
    for( PCB_TABLECELL* cell : m_cells )
        delete cell;
}


void PCB_TABLE::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage != nullptr && aImage->Type() == PCB_TABLE_T,
                 wxT( "Cannot swap data with invalid table." ) );

    PCB_TABLE* table = static_cast<PCB_TABLE*>( aImage );

    std::swap( m_layer, table->m_layer );
    std::swap( m_isLocked, table->m_isLocked );

    std::swap( m_strokeExternal, table->m_strokeExternal );
    std::swap( m_strokeHeader, table->m_strokeHeader );
    std::swap( m_borderStroke, table->m_borderStroke );
    std::swap( m_strokeRows, table->m_strokeRows );
    std::swap( m_strokeColumns, table->m_strokeColumns );
    std::swap( m_separatorsStroke, table->m_separatorsStroke );

    std::swap( m_orientation, table->m_orientation );
    std::swap( m_colCount, table->m_colCount );
    std::swap( m_colWidths, table->m_colWidths );
    std::swap( m_rowHeights, table->m_rowHeights );

    std::swap( m_cells, table->m_cells );

    for( PCB_TABLECELL* cell : m_cells )
        cell->SetParent( this );

    for( PCB_TABLECELL* cell : table->m_cells )
        cell->SetParent( table );
}


void PCB_TABLE::SetPosition( const VECTOR2I& aPos )
{
    Move( aPos - GetPosition() );
}


VECTOR2I PCB_TABLE::GetPosition() const
{
    return m_cells[0]->GetPosition();
}


VECTOR2I PCB_TABLE::GetEnd() const
{
    VECTOR2I tableSize;

    for( int ii = 0; ii < GetColCount(); ++ii )
        tableSize.x += GetColWidth( ii );

    for( int ii = 0; ii < GetRowCount(); ++ii )
        tableSize.y += GetRowHeight( ii );

    return GetPosition() + tableSize;
}


void PCB_TABLE::Normalize()
{
    VECTOR2I  origin = GetPosition();

    auto setCellStart =
            [&]( PCB_TABLECELL* cell, VECTOR2I pt )
            {
                RotatePoint( pt, origin, m_orientation );

                if( cell->GetPosition() != pt )
                {
                    cell->SetPosition( pt );
                    cell->ClearRenderCache();
                }
            };

    auto setCellEnd =
            [&]( PCB_TABLECELL* cell, VECTOR2I pt )
            {
                RotatePoint( pt, origin, m_orientation );

                if( cell->GetEnd() != pt )
                {
                    cell->SetEnd( pt );
                    cell->ClearRenderCache();
                }
            };

    int y = origin.y;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        int x = origin.x;
        int rowHeight = m_rowHeights[ row ];

        for( int col = 0; col < GetColCount(); ++col )
        {
            int colWidth = m_colWidths[ col ];

            PCB_TABLECELL* cell = GetCell( row, col );
            VECTOR2I       pos( x, y );

            setCellStart( cell, pos );

            VECTOR2I end = pos + VECTOR2I( colWidth, rowHeight );

            if( cell->GetColSpan() > 1 || cell->GetRowSpan() > 1 )
            {
                for( int ii = col + 1; ii < col + cell->GetColSpan(); ++ii )
                    end.x += m_colWidths[ii];

                for( int ii = row + 1; ii < row + cell->GetRowSpan(); ++ii )
                    end.y += m_rowHeights[ii];
            }

            setCellEnd( cell, end );
            x += colWidth;
        }

        y += rowHeight;
    }
}


void PCB_TABLE::Move( const VECTOR2I& aMoveVector )
{
    for( PCB_TABLECELL* cell : m_cells )
        cell->Move( aMoveVector );
}


void PCB_TABLE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    m_orientation = ( m_orientation + aAngle ).Normalized();

    for( PCB_TABLECELL* cell : m_cells )
        cell->Rotate( aRotCentre, aAngle );

    Normalize();
}


void PCB_TABLE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    for( PCB_TABLECELL* cell : m_cells )
        cell->Flip( aCentre, aFlipDirection );

    std::vector<PCB_TABLECELL*> oldCells = m_cells;
    int                         rowOffset = 0;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        for( int col = 0; col < GetColCount(); ++col )
            m_cells[ rowOffset + col ] = oldCells[ rowOffset + GetColCount() - 1 - col ];

        rowOffset += GetColCount();
    }

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
    Normalize();
}


void PCB_TABLE::RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction ) const
{
    for( PCB_TABLECELL* cell : m_cells )
        aFunction( cell );
}


const BOX2I PCB_TABLE::GetBoundingBox() const
{
    // Note: a table with no cells is not allowed
    BOX2I bbox = m_cells[0]->GetBoundingBox();

    bbox.Merge( m_cells[ m_cells.size() - 1 ]->GetBoundingBox() );

    return bbox;
}


std::shared_ptr<SHAPE> PCB_TABLE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    VECTOR2I                        origin = GetPosition();
    VECTOR2I                        end = GetEnd();
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();

    std::vector<VECTOR2I> pts;

    pts.emplace_back( origin );
    pts.emplace_back( end.x, origin.y );
    pts.emplace_back( end );
    pts.emplace_back( origin.x, end.y );

    shape->AddShape( new SHAPE_SIMPLE( pts ) );

    auto addSeg =
            [&shape]( const VECTOR2I& ptA, const VECTOR2I& ptB, int width )
            {
                shape->AddShape( new SHAPE_SEGMENT( ptA, ptB, width ) );
            };

    if( StrokeColumns() && GetSeparatorsStroke().GetWidth() >= 0)
    {
        for( int col = 0; col < GetColCount() - 1; ++col )
        {
            for( int row = 0; row < GetRowCount(); ++row )
            {
                PCB_TABLECELL* cell = GetCell( row, col );
                VECTOR2I       topRight( cell->GetEndX(), cell->GetStartY() );

                if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    addSeg( topRight, cell->GetEnd(), GetSeparatorsStroke().GetWidth() );
            }
        }
    }

    if( StrokeRows() && GetSeparatorsStroke().GetWidth() >= 0 )
    {
        for( int row = 0; row < GetRowCount() - 1; ++row )
        {
            for( int col = 0; col < GetColCount(); ++col )
            {
                PCB_TABLECELL* cell = GetCell( row, col );
                VECTOR2I       botLeft( cell->GetStartX(), cell->GetEndY() );

                if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                    addSeg( botLeft, cell->GetEnd(), GetSeparatorsStroke().GetWidth() );
            }
        }
    }

    if( StrokeExternal() && GetBorderStroke().GetWidth() >= 0 )
    {
        addSeg( pts[0], pts[1], GetBorderStroke().GetWidth() );
        addSeg( pts[1], pts[2], GetBorderStroke().GetWidth() );
        addSeg( pts[2], pts[3], GetBorderStroke().GetWidth() );
        addSeg( pts[3], pts[0], GetBorderStroke().GetWidth() );
    }

    return shape;
}


void PCB_TABLE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                         bool aIgnoreLineWidth ) const
{
    int gap = aClearance;

    if( StrokeColumns() || StrokeRows() )
        gap = std::max( gap, aClearance + GetSeparatorsStroke().GetWidth() / 2 );

    if( StrokeExternal() || StrokeHeader() )
        gap = std::max( gap, aClearance + GetBorderStroke().GetWidth() / 2 );

    for( PCB_TABLECELL* cell : m_cells )
        cell->TransformShapeToPolygon( aBuffer, aLayer, gap, aMaxError, aErrorLoc, false );
}


INSPECT_RESULT PCB_TABLE::Visit( INSPECTOR aInspector, void* aTestData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == PCB_TABLE_T )
        {
            if( INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
                return INSPECT_RESULT::QUIT;
        }

        if( scanType == PCB_TABLECELL_T )
        {
            for( PCB_TABLECELL* cell : m_cells )
            {
                if( INSPECT_RESULT::QUIT == aInspector( cell, (void*) this ) )
                    return INSPECT_RESULT::QUIT;
            }
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


wxString PCB_TABLE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "%d Column Table" ), m_colCount );
}


BITMAPS PCB_TABLE::GetMenuImage() const
{
    return BITMAPS::spreadsheet;    // JEY TODO
}


bool PCB_TABLE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_TABLE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void PCB_TABLE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Table" ), wxString::Format( _( "%d Columns" ), m_colCount ) );
}


int PCB_TABLE::Compare( const PCB_TABLE* aTable, const PCB_TABLE* aOther )
{
    int diff;

    if( ( diff = (int) aTable->GetCells().size() - (int) aOther->GetCells().size() ) != 0 )
        return diff;

    if( ( diff = aTable->GetColCount() - aOther->GetColCount() ) != 0 )
        return diff;

    for( int col = 0; col < aTable->GetColCount(); ++col )
    {
        if( ( diff = aTable->GetColWidth( col ) - aOther->GetColWidth( col ) ) != 0 )
            return diff;
    }

    for( int row = 0; row < aTable->GetRowCount(); ++row )
    {
        if( ( diff = aTable->GetRowHeight( row ) - aOther->GetRowHeight( row ) ) != 0 )
            return diff;
    }

    for( int row = 0; row < aTable->GetRowCount(); ++row )
    {
        for( int col = 0; col < aTable->GetColCount(); ++col )
        {
            PCB_TABLECELL* cell = aTable->GetCell( row, col );
            PCB_TABLECELL* other = aOther->GetCell( row, col );

            if( ( diff = cell->PCB_SHAPE::Compare( other ) ) != 0 )
                return diff;

            if( ( diff = cell->EDA_TEXT::Compare( other ) ) != 0 )
                return diff;
        }
    }

    return 0;
}


bool PCB_TABLE::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( Type() != aBoardItem.Type() )
        return false;

    const PCB_TABLE& other = static_cast<const PCB_TABLE&>( aBoardItem );

    return *this == other;
}


bool PCB_TABLE::operator==( const PCB_TABLE& aOther ) const
{
    if( m_cells.size() != aOther.m_cells.size() )
        return false;

    if( m_strokeExternal != aOther.m_strokeExternal )
        return false;

    if( m_strokeHeader != aOther.m_strokeHeader )
        return false;

    if( m_borderStroke != aOther.m_borderStroke )
        return false;

    if( m_strokeRows != aOther.m_strokeRows )
        return false;

    if( m_strokeColumns != aOther.m_strokeColumns )
        return false;

    if( m_separatorsStroke != aOther.m_separatorsStroke )
        return false;

    if( m_orientation != aOther.m_orientation )
        return false;

    if( m_colWidths != aOther.m_colWidths )
        return false;

    if( m_rowHeights != aOther.m_rowHeights )
        return false;

    for( int ii = 0; ii < (int) m_cells.size(); ++ii )
    {
        if( !( *m_cells[ii] == *aOther.m_cells[ii] ) )
            return false;
    }

    return true;
}


double PCB_TABLE::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_TABLE& other = static_cast<const PCB_TABLE&>( aOther );

    if( m_cells.size() != other.m_cells.size() )
        return 0.1;

    double similarity = 1.0;

    if( m_strokeExternal != other.m_strokeExternal )
        similarity *= 0.9;

    if( m_strokeHeader != other.m_strokeHeader )
        similarity *= 0.9;

    if( m_borderStroke != other.m_borderStroke )
        similarity *= 0.9;

    if( m_strokeRows != other.m_strokeRows )
        similarity *= 0.9;

    if( m_strokeColumns != other.m_strokeColumns )
        similarity *= 0.9;

    if( m_separatorsStroke != other.m_separatorsStroke )
        similarity *= 0.9;

    if( m_orientation != other.m_orientation )
        similarity *= 0.9;

    if( m_colWidths != other.m_colWidths )
        similarity *= 0.9;

    if( m_rowHeights != other.m_rowHeights )
        similarity *= 0.9;

    for( int ii = 0; ii < (int) m_cells.size(); ++ii )
        similarity *= m_cells[ii]->Similarity( *other.m_cells[ii] );

    return similarity;
}


static struct PCB_TABLE_DESC
{
    PCB_TABLE_DESC()
    {
        ENUM_MAP<LINE_STYLE>& plotDashTypeEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( plotDashTypeEnum.Choices().GetCount() == 0 )
        {
            plotDashTypeEnum.Map( LINE_STYLE::DEFAULT, _HKI( "Default" ) )
                            .Map( LINE_STYLE::SOLID, _HKI( "Solid" ) )
                            .Map( LINE_STYLE::DASH, _HKI( "Dashed" ) )
                            .Map( LINE_STYLE::DOT, _HKI( "Dotted" ) )
                            .Map( LINE_STYLE::DASHDOT, _HKI( "Dash-Dot" ) )
                            .Map( LINE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TABLE );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TABLE, BOARD_ITEM_CONTAINER> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TABLE ), TYPE_HASH( BOARD_ITEM_CONTAINER ) );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, int>( _HKI( "Start X" ),
                    &PCB_TABLE::SetPositionX, &PCB_TABLE::GetPositionX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );
        propMgr.AddProperty( new PROPERTY<PCB_TABLE, int>( _HKI( "Start Y" ),
                    &PCB_TABLE::SetPositionY, &PCB_TABLE::GetPositionY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, EDA_ANGLE>( _HKI( "Orientation" ),
                    &PCB_TABLE::SetOrientation, &PCB_TABLE::GetOrientation,
                    PROPERTY_DISPLAY::PT_DEGREE ) );

        const wxString tableProps = _( "Table Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "External Border" ),
                    &PCB_TABLE::SetStrokeExternal, &PCB_TABLE::StrokeExternal ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "Header Border" ),
                    &PCB_TABLE::SetStrokeHeader, &PCB_TABLE::StrokeHeader ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, int>( _HKI( "Border Width" ),
                    &PCB_TABLE::SetBorderWidth, &PCB_TABLE::GetBorderWidth,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TABLE, LINE_STYLE>( _HKI( "Border Style" ),
                    &PCB_TABLE::SetBorderStyle, &PCB_TABLE::GetBorderStyle ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, COLOR4D>( _HKI( "Border Color" ),
                    &PCB_TABLE::SetBorderColor, &PCB_TABLE::GetBorderColor ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "Row Separators" ),
                    &PCB_TABLE::SetStrokeRows, &PCB_TABLE::StrokeRows ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "Cell Separators" ),
                    &PCB_TABLE::SetStrokeColumns, &PCB_TABLE::StrokeColumns ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, int>( _HKI( "Separators Width" ),
                    &PCB_TABLE::SetSeparatorsWidth, &PCB_TABLE::GetSeparatorsWidth,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_TABLE, LINE_STYLE>( _HKI( "Separators Style" ),
                    &PCB_TABLE::SetSeparatorsStyle, &PCB_TABLE::GetSeparatorsStyle ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, COLOR4D>( _HKI( "Separators Color" ),
                    &PCB_TABLE::SetSeparatorsColor, &PCB_TABLE::GetSeparatorsColor ),
                    tableProps );
    }
} _PCB_TABLE_DESC;
