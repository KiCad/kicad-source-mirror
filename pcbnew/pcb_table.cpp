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

#include <pcb_edit_frame.h>
#include <footprint.h>
#include <pcb_table.h>
#include <board.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/geometry_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <pcb_painter.h>    // for PCB_RENDER_SETTINGS


PCB_TABLE::PCB_TABLE( BOARD_ITEM* aParent, int aLineWidth ) :
        BOARD_ITEM_CONTAINER( aParent, PCB_TABLE_T ),
        m_strokeExternal( true ),
        m_StrokeHeaderSeparator( true ),
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
    m_StrokeHeaderSeparator = aTable.m_StrokeHeaderSeparator;
    m_borderStroke = aTable.m_borderStroke;
    m_strokeRows = aTable.m_strokeRows;
    m_strokeColumns = aTable.m_strokeColumns;
    m_separatorsStroke = aTable.m_separatorsStroke;

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
    wxCHECK_RET( aImage != nullptr && aImage->Type() == PCB_TABLE_T, wxT( "Cannot swap data with invalid table." ) );

    PCB_TABLE* table = static_cast<PCB_TABLE*>( aImage );

    std::swap( m_layer, table->m_layer );
    std::swap( m_isLocked, table->m_isLocked );

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
    if( m_cells.empty() )
        return VECTOR2I( 0, 0 );  // Return origin if table has no cells

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
    int y = GetPosition().y;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        int x = GetPosition().x;
        int rowHeight = m_rowHeights[row];

        for( int col = 0; col < GetColCount(); ++col )
        {
            int colWidth = m_colWidths[col];

            PCB_TABLECELL* cell = GetCell( row, col );

            if( !cell )
                continue;  // Skip if cell doesn't exist (shouldn't happen, but be defensive)

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


void PCB_TABLE::Autosize()
{
    std::vector<std::vector<BOX2I>> extents;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        extents.push_back( std::vector<BOX2I>() );

        for( int col = 0; col < GetColCount(); ++col )
        {
            SHAPE_POLY_SET textPoly;
            GetCell( row, col )->TransformTextToPolySet( textPoly, 0, ARC_LOW_DEF, ERROR_INSIDE );
            extents[row].push_back( textPoly.BBox() );
        }
    }

    for( int col = 0; col < GetColCount(); ++col )
    {
        int colWidth = 0;

        for( int row = 0; row < GetRowCount(); ++row )
        {
            PCB_TABLECELL* cell = GetCell( row, col );
            int            margins = cell->GetMarginLeft() + cell->GetMarginRight();
            colWidth = std::max<int>( colWidth, extents[row][col].GetWidth() + ( margins * 1.5 ) );
        }

        SetColWidth( col, colWidth );
    }

    for( int row = 0; row < GetRowCount(); ++row )
    {
        int rowHeight = 0;

        for( int col = 0; col < GetColCount(); ++col )
        {
            PCB_TABLECELL* cell = GetCell( row, col );
            int            margins = cell->GetMarginLeft() + cell->GetMarginRight();
            rowHeight = std::max( rowHeight, (int) extents[row][col].GetHeight() + margins );
        }

        SetRowHeight( row, rowHeight );
    }

    Normalize();
}


void PCB_TABLE::Move( const VECTOR2I& aMoveVector )
{
    for( PCB_TABLECELL* cell : m_cells )
        cell->Move( aMoveVector );
}


void PCB_TABLE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    if( GetCells().empty() )
        return;

    for( PCB_TABLECELL* cell : m_cells )
        cell->Rotate( aRotCentre, aAngle );

    Normalize();
}


void PCB_TABLE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
    {
        Flip( aCentre, FLIP_DIRECTION::LEFT_RIGHT );
        Rotate( aCentre, EDA_ANGLE( 180, DEGREES_T ) );
        return;
    }

    EDA_ANGLE originalAngle = m_cells[0]->GetTextAngle();

    Rotate( aCentre, -originalAngle );
    Normalize();

    for( PCB_TABLECELL* cell : m_cells )
        cell->Flip( aCentre, aFlipDirection );

    std::vector<PCB_TABLECELL*> oldCells = m_cells;
    int                         rowOffset = 0;

    for( int row = 0; row < GetRowCount(); ++row )
    {
        for( int col = 0; col < GetColCount(); ++col )
            m_cells[rowOffset + col] = oldCells[rowOffset + GetColCount() - 1 - col];

        rowOffset += GetColCount();
    }

    VECTOR2I currentPos = GetPosition();

    int      firstWidth = m_colWidths.begin()->second;
    VECTOR2I translationVector = VECTOR2I( firstWidth, 0 );

    VECTOR2I position = currentPos - translationVector;
    SetPosition( position );

    std::map<int, int> newColWidths;
    for( int col = 0; col < GetColCount(); ++col )
    {
        newColWidths[col] = m_colWidths[GetColCount() - 1 - col];
    }

    m_colWidths = std::move( newColWidths );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
    Normalize();

    Rotate( aCentre, originalAngle );
    Normalize();
}


void PCB_TABLE::RunOnChildren( const std::function<void( BOARD_ITEM* )>& aFunction, RECURSE_MODE aMode ) const
{
    for( PCB_TABLECELL* cell : m_cells )
    {
        aFunction( cell );

        if( aMode == RECURSE_MODE::RECURSE )
            cell->RunOnChildren( aFunction, aMode );
    }
}


const BOX2I PCB_TABLE::GetBoundingBox() const
{
    // Note: a table with no cells is not allowed
    BOX2I bbox = m_cells[0]->GetBoundingBox();

    bbox.Merge( m_cells[m_cells.size() - 1]->GetBoundingBox() );

    return bbox;
}


void PCB_TABLE::DrawBorders( const std::function<void( const VECTOR2I& aPt1, const VECTOR2I& aPt2,
                                                       const STROKE_PARAMS& aStroke )>& aCallback ) const
{
    EDA_ANGLE             drawAngle = GetCell( 0, 0 )->GetDrawRotation();
    std::vector<VECTOR2I> topLeft = GetCell( 0, 0 )->GetCornersInSequence( drawAngle );
    std::vector<VECTOR2I> bottomLeft = GetCell( GetRowCount() - 1, 0 )->GetCornersInSequence( drawAngle );
    std::vector<VECTOR2I> topRight = GetCell( 0, GetColCount() - 1 )->GetCornersInSequence( drawAngle );
    std::vector<VECTOR2I> bottomRight =
            GetCell( GetRowCount() - 1, GetColCount() - 1 )->GetCornersInSequence( drawAngle );
    STROKE_PARAMS stroke;

    for( int col = 0; col < GetColCount() - 1; ++col )
    {
        for( int row = 0; row < GetRowCount(); ++row )
        {
            if( row == 0 && StrokeHeaderSeparator() )
                stroke = GetBorderStroke();
            else if( StrokeColumns() )
                stroke = GetSeparatorsStroke();
            else
                continue;

            PCB_TABLECELL* cell = GetCell( row, col );

            if( cell->GetColSpan() == 0 )
                continue;

            if( col + cell->GetColSpan() == GetColCount() )
                continue;

            std::vector<VECTOR2I> corners = cell->GetCornersInSequence( drawAngle );

            if( corners.size() == 4 )
                aCallback( corners[1], corners[2], stroke );
        }
    }

    for( int row = 0; row < GetRowCount() - 1; ++row )
    {
        if( row == 0 && StrokeHeaderSeparator() )
            stroke = GetBorderStroke();
        else if( StrokeRows() )
            stroke = GetSeparatorsStroke();
        else
            continue;

        for( int col = 0; col < GetColCount(); ++col )
        {
            PCB_TABLECELL* cell = GetCell( row, col );

            if( cell->GetRowSpan() == 0 )
                continue;

            if( row + cell->GetRowSpan() == GetRowCount() )
                continue;

            std::vector<VECTOR2I> corners = cell->GetCornersInSequence( drawAngle );

            if( corners.size() == 4 )
                aCallback( corners[2], corners[3], stroke );
        }
    }

    if( StrokeExternal() && GetBorderStroke().GetWidth() >= 0 )
    {
        aCallback( topLeft[0], topRight[1], GetBorderStroke() );
        aCallback( topRight[1], bottomRight[2], GetBorderStroke() );
        aCallback( bottomRight[2], bottomLeft[3], GetBorderStroke() );
        aCallback( bottomLeft[3], topLeft[0], GetBorderStroke() );
    }
}


std::shared_ptr<SHAPE> PCB_TABLE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    EDA_ANGLE             angle = GetCell( 0, 0 )->GetDrawRotation();
    std::vector<VECTOR2I> topLeft = GetCell( 0, 0 )->GetCornersInSequence( angle );
    std::vector<VECTOR2I> bottomLeft = GetCell( GetRowCount() - 1, 0 )->GetCornersInSequence( angle );
    std::vector<VECTOR2I> topRight = GetCell( 0, GetColCount() - 1 )->GetCornersInSequence( angle );
    std::vector<VECTOR2I> bottomRight = GetCell( GetRowCount() - 1, GetColCount() - 1 )->GetCornersInSequence( angle );

    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();

    std::vector<VECTOR2I> pts;

    pts.emplace_back( topLeft[3] );
    pts.emplace_back( topRight[2] );
    pts.emplace_back( bottomRight[2] );
    pts.emplace_back( bottomLeft[3] );

    shape->AddShape( new SHAPE_SIMPLE( pts ) );

    DrawBorders(
            [&shape]( const VECTOR2I& ptA, const VECTOR2I& ptB, const STROKE_PARAMS& stroke )
            {
                shape->AddShape( new SHAPE_SEGMENT( ptA, ptB, stroke.GetWidth() ) );
            } );

    return shape;
}


void PCB_TABLE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance, int aMaxError,
                                         ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
{
    int gap = aClearance;

    if( StrokeColumns() || StrokeRows() )
        gap = std::max( gap, aClearance + GetSeparatorsStroke().GetWidth() / 2 );

    if( StrokeExternal() || StrokeHeaderSeparator() )
        gap = std::max( gap, aClearance + GetBorderStroke().GetWidth() / 2 );

    for( PCB_TABLECELL* cell : m_cells )
        cell->TransformShapeToPolygon( aBuffer, aLayer, gap, aMaxError, aErrorLoc, false );
}


void PCB_TABLE::TransformGraphicItemsToPolySet( SHAPE_POLY_SET& aBuffer, int aMaxError, ERROR_LOC aErrorLoc,
                                                KIGFX::RENDER_SETTINGS* aRenderSettings ) const
{
    // Convert graphic items (segments and texts) to a set of polygonal shapes
    // aRenderSettings is used to draw lines when line style != LINE_STYLE::SOLID, so
    // if nullptr line style will be ignored
    DrawBorders(
            [&aBuffer, aMaxError, aErrorLoc, aRenderSettings]( const VECTOR2I& ptA, const VECTOR2I& ptB,
                                                               const STROKE_PARAMS& stroke )
            {
                int        lineWidth = stroke.GetWidth();
                LINE_STYLE lineStyle = stroke.GetLineStyle();

                if( lineStyle <= LINE_STYLE::FIRST_TYPE || aRenderSettings == nullptr )
                    TransformOvalToPolygon( aBuffer, ptA, ptB, lineWidth, aMaxError, aErrorLoc );
                else
                {
                    SHAPE_SEGMENT              seg( ptA, ptB );
                    KIGFX::PCB_RENDER_SETTINGS defaultRenderSettings;

                    KIGFX::RENDER_SETTINGS* currSettings = aRenderSettings;

                    if( currSettings == nullptr )
                        currSettings = &defaultRenderSettings;

                    STROKE_PARAMS::Stroke( &seg, lineStyle, lineWidth, currSettings,
                            [&]( VECTOR2I a, VECTOR2I b )
                            {
                                if( a == b )
                                    TransformCircleToPolygon( aBuffer, a, lineWidth / 2, aMaxError, aErrorLoc );
                                else
                                    TransformOvalToPolygon( aBuffer, a + 1, b, lineWidth, aMaxError, aErrorLoc );
                            } );
                }
            } );

    for( PCB_TABLECELL* cell : m_cells )
    {
        cell->TransformTextToPolySet( aBuffer, 0, aMaxError, ERROR_INSIDE );
    }
}


void PCB_TABLE::TransformShapeToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                         KIGFX::RENDER_SETTINGS* aRenderSettings ) const
{
    if( aClearance <= 0 )
        TransformGraphicItemsToPolySet( aBuffer, aMaxError, aErrorLoc, aRenderSettings );
    else
    {
        SHAPE_POLY_SET tmp;
        TransformGraphicItemsToPolySet( tmp, aMaxError, aErrorLoc, aRenderSettings );
        tmp.Inflate( aClearance, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, aMaxError );
        aBuffer.Append( tmp );
    }
}


INSPECT_RESULT PCB_TABLE::Visit( INSPECTOR aInspector, void* aTestData, const std::vector<KICAD_T>& aScanTypes )
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
    return wxString::Format( _( "%d column table" ), m_colCount );
}


BITMAPS PCB_TABLE::GetMenuImage() const
{
    return BITMAPS::table;
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


bool PCB_TABLE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::ShapeHitTest( aPoly, *GetEffectiveShape(), aContained );
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

    if( m_StrokeHeaderSeparator != aOther.m_StrokeHeaderSeparator )
        return false;

    if( m_borderStroke != aOther.m_borderStroke )
        return false;

    if( m_strokeRows != aOther.m_strokeRows )
        return false;

    if( m_strokeColumns != aOther.m_strokeColumns )
        return false;

    if( m_separatorsStroke != aOther.m_separatorsStroke )
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

    if( m_StrokeHeaderSeparator != other.m_StrokeHeaderSeparator )
        similarity *= 0.9;

    if( m_borderStroke != other.m_borderStroke )
        similarity *= 0.9;

    if( m_strokeRows != other.m_strokeRows )
        similarity *= 0.9;

    if( m_strokeColumns != other.m_strokeColumns )
        similarity *= 0.9;

    if( m_separatorsStroke != other.m_separatorsStroke )
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
        ENUM_MAP<LINE_STYLE>& lineStyleEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( lineStyleEnum.Choices().GetCount() == 0 )
        {
            lineStyleEnum.Map( LINE_STYLE::SOLID,      _HKI( "Solid" ) )
                         .Map( LINE_STYLE::DASH,       _HKI( "Dashed" ) )
                         .Map( LINE_STYLE::DOT,        _HKI( "Dotted" ) )
                         .Map( LINE_STYLE::DASHDOT,    _HKI( "Dash-Dot" ) )
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

        const wxString tableProps = _( "Table Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "External Border" ),
                    &PCB_TABLE::SetStrokeExternal, &PCB_TABLE::StrokeExternal ),
                    tableProps );

        propMgr.AddProperty( new PROPERTY<PCB_TABLE, bool>( _HKI( "Header Border" ),
                    &PCB_TABLE::SetStrokeHeaderSeparator, &PCB_TABLE::StrokeHeaderSeparator ),
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
