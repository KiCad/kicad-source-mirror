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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <set>

#include <pcb_generated_table.h>

#include <base_units.h>
#include <font/font.h>
#include <geometry/approximation.h>
#include <geometry/shape_poly_set.h>
#include <i18n_utility.h>
#include <pcb_tablecell.h>
#include <properties/property_mgr.h>
#include <widgets/msgpanel.h>


namespace
{

GR_TEXT_H_ALIGN_T toHorizJustify( GENERATED_TABLE_ALIGN aAlign )
{
    switch( aAlign )
    {
    case GENERATED_TABLE_ALIGN::LEFT: return GR_TEXT_H_ALIGN_LEFT;
    case GENERATED_TABLE_ALIGN::CENTER: return GR_TEXT_H_ALIGN_CENTER;
    case GENERATED_TABLE_ALIGN::RIGHT: return GR_TEXT_H_ALIGN_RIGHT;
    }

    return GR_TEXT_H_ALIGN_LEFT;
}


void copyCellStyle( PCB_TABLECELL* aCell, const PCB_TABLECELL* aDonor )
{
    aCell->SetAttributes( *aDonor, false );
    aCell->SetIsKnockout( aDonor->IsKnockout() );
    aCell->SetMarginLeft( aDonor->GetMarginLeft() );
    aCell->SetMarginTop( aDonor->GetMarginTop() );
    aCell->SetMarginRight( aDonor->GetMarginRight() );
    aCell->SetMarginBottom( aDonor->GetMarginBottom() );
}


// A subclass short of cells for a column shows it blank rather than reading past the end
const wxString& textAt( const std::vector<wxString>& aCells, int aCol )
{
    static const wxString empty;

    return aCol < static_cast<int>( aCells.size() ) ? aCells[aCol] : empty;
}

} // namespace


PCB_GENERATED_TABLE::PCB_GENERATED_TABLE( BOARD_ITEM* aParent, KICAD_T aType, int aLineWidth ) :
        PCB_TABLE( aParent, aType, aLineWidth ),
        m_units( GENERATED_TABLE_UNITS::MM ),
        m_precision( 3 )
{
    // RefreshGeneratedTables finds tables by type, so an unlisted one would never refresh
    wxASSERT( IsGeneratedTableType( aType ) );
}


PCB_GENERATED_TABLE::PCB_GENERATED_TABLE( const PCB_GENERATED_TABLE& aOther ) :
        PCB_TABLE( aOther ),
        m_columns( aOther.m_columns ),
        m_units( aOther.m_units ),
        m_precision( aOther.m_precision ),
        m_rowKeys( aOther.m_rowKeys ),
        m_hidden( aOther.m_hidden )
{}


void PCB_GENERATED_TABLE::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage && aImage->Type() == Type(), wxT( "Cannot swap data with invalid table." ) );

    PCB_TABLE::swapData( aImage );

    PCB_GENERATED_TABLE* other = static_cast<PCB_GENERATED_TABLE*>( aImage );

    std::swap( m_columns, other->m_columns );
    std::swap( m_units, other->m_units );
    std::swap( m_precision, other->m_precision );
    std::swap( m_rowKeys, other->m_rowKeys );
    std::swap( m_hidden, other->m_hidden );
}


void PCB_GENERATED_TABLE::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == Type(), /* void */ );

    // PCB_TABLE's copy only accepts a plain table. swapData covers every member a subclass adds
    std::unique_ptr<BOARD_ITEM> copy( static_cast<BOARD_ITEM*>( aOther->Clone() ) );
    SwapItemData( copy.get() );
}


void PCB_GENERATED_TABLE::ApplySettings( const PCB_GENERATED_TABLE& aOther )
{
    // Before SetColumns, while both tables still have the same cells to measure from
    Move( aOther.GetPosition() - GetPosition() );

    SetColumns( aOther.AllColumns() );
    SetUnits( aOther.GetUnits() );
    SetPrecision( aOther.GetPrecision() );
    SetLayer( aOther.GetLayer() );
    SetLocked( aOther.IsLocked() );

    SetStrokeExternal( aOther.StrokeExternal() );
    SetStrokeHeaderSeparator( aOther.StrokeHeaderSeparator() );
    SetBorderStroke( aOther.GetBorderStroke() );
    SetStrokeRows( aOther.StrokeRows() );
    SetStrokeColumns( aOther.StrokeColumns() );
    SetSeparatorsStroke( aOther.GetSeparatorsStroke() );

    SetCustomProperties( aOther.GetCustomProperties() );
}


bool PCB_GENERATED_TABLE::generatedEquals( const PCB_GENERATED_TABLE& aOther ) const
{
    return m_columns == aOther.m_columns && m_units == aOther.m_units && m_precision == aOther.m_precision
           && m_rowKeys == aOther.m_rowKeys && m_hidden == aOther.m_hidden
           && PCB_TABLE::operator==( aOther );
}


double PCB_GENERATED_TABLE::Similarity( const BOARD_ITEM& aOther ) const
{
    double similarity = PCB_TABLE::Similarity( aOther );

    if( aOther.Type() != Type() )
        return similarity;

    const PCB_GENERATED_TABLE& other = static_cast<const PCB_GENERATED_TABLE&>( aOther );

    if( m_columns != other.m_columns || m_hidden != other.m_hidden )
        similarity *= 0.9;

    return similarity;
}


std::vector<GENERATED_TABLE_COLUMN> PCB_GENERATED_TABLE::AllColumns() const
{
    std::vector<GENERATED_TABLE_COLUMN> all;
    const size_t                        total = m_columns.size() + m_hidden.size();
    size_t                              shown = 0;
    size_t                              hidden = 0;

    for( size_t pos = 0; pos < total; ++pos )
    {
        const bool hiddenHere = hidden < m_hidden.size()
                                && ( m_hidden[hidden].m_Position == static_cast<int>( pos )
                                     || shown == m_columns.size() );

        if( hiddenHere )
            all.push_back( m_hidden[hidden++].m_Column );
        else
            all.push_back( m_columns[shown++] );
    }

    return all;
}


void PCB_GENERATED_TABLE::RestoreColumns( const std::vector<GENERATED_TABLE_COLUMN>& aColumns )
{
    m_columns.clear();
    m_hidden.clear();

    for( size_t pos = 0; pos < aColumns.size(); ++pos )
    {
        GENERATED_TABLE_COLUMN col = aColumns[pos];

        if( col.m_Visible )
        {
            m_columns.push_back( col );
            continue;
        }

        HIDDEN_COLUMN& entry = m_hidden.emplace_back();
        entry.m_Column = col;
        entry.m_Position = static_cast<int>( pos );
    }
}


std::vector<int> PCB_GENERATED_TABLE::dataRows() const
{
    std::vector<int> rows;

    for( int row = 1; row < GetRowCount(); ++row )
    {
        if( IsDataRow( row ) )
            rows.push_back( row );
    }

    return rows;
}


int PCB_GENERATED_TABLE::GetRowHeightProp() const
{
    const std::vector<int> rows = dataRows();

    return rows.empty() ? 0 : GetRowHeight( rows.front() );
}


void PCB_GENERATED_TABLE::SetRowHeightProp( int aHeight )
{
    for( int row : dataRows() )
        SetRowHeight( row, std::max( aHeight, minRowHeight( row ) ) );

    Normalize();
}


void PCB_GENERATED_TABLE::SetRowHeightToFit( int aRow, int aHeight )
{
    SetRowHeight( aRow, std::max( aHeight, minRowHeight( aRow ) ) );
    Normalize();
}


bool PCB_GENERATED_TABLE::RowHeightsDiffer() const
{
    const int first = GetRowHeightProp();

    return std::ranges::any_of( dataRows(),
                                [&]( int aRow )
                                {
                                    return GetRowHeight( aRow ) != first;
                                } );
}


void PCB_GENERATED_TABLE::setAutosizedRowHeights( const std::vector<int>& aNeeded )
{
    const int rows = GetRowCount();

    // Cells take Autosize's widths first, or the line metrics below follow the old wrap
    Normalize();

    // Normalize() stores a missing row as 0, so 0 is no height rather than the user's
    std::vector<int> held( rows );
    std::vector<int> fit( rows );

    for( int row = 0; row < rows; ++row )
    {
        held[row] = std::max( GetRowHeight( row ), 0 );

        // Glyph outlines differ by character, which splits the Row Height by microns, so data rows
        // size to line metrics unless a glyph reaches past them
        fit[row] = IsDataRow( row ) ? std::max( aNeeded[row], metricRowHeight( row ) ) : aNeeded[row];
    }

    int                maxNeed = 0;
    std::map<int, int> counts;

    for( int row : dataRows() )
    {
        maxNeed = std::max( maxNeed, fit[row] );

        if( held[row] > 0 )
            counts[held[row]]++;
    }

    // A data row without a height of its own matches most of its siblings, larger on a tie. With
    // none held, as in a fresh table, all share the tallest need, or glyph descents split the value
    int common = maxNeed;
    int commonCount = 0;

    for( const auto& [height, count] : counts )
    {
        if( count >= commonCount )
        {
            common = height;
            commonCount = count;
        }
    }

    // Rows default to their text plus margins, but keep any height that still holds the text, down to
    // the text touching its edges. Text that outgrows a height refits it with margins, so one row's
    // font moves no sibling
    for( int row = 0; row < rows; ++row )
    {
        const int height = IsDataRow( row ) && held[row] == 0 ? common : held[row];

        SetRowHeight( row, height > 0 && height >= minRowHeight( row ) ? height : fit[row] );
    }
}


int PCB_GENERATED_TABLE::metricRowHeight( int aRow ) const
{
    int height = 0;

    for( int col = 0; col < GetColCount(); ++col )
    {
        const PCB_TABLECELL* cell = GetCell( aRow, col );

        if( !cell )
            continue;

        const int lines = static_cast<int>( cell->GetShownText( FOR_CANVAS ).Freq( '\n' ) ) + 1;

        // The pen strokes half its width past the glyph geometry on each side
        height = std::max( height, lines * cell->GetInterline( nullptr ) + cell->GetEffectiveTextPenWidth()
                                           + cell->GetMarginTop() + cell->GetMarginBottom() );
    }

    return height;
}


int PCB_GENERATED_TABLE::minRowHeight( int aRow ) const
{
    int height = 0;

    for( int col = 0; col < GetColCount(); ++col )
    {
        const PCB_TABLECELL* cell = GetCell( aRow, col );

        if( !cell )
            continue;

        SHAPE_POLY_SET glyphs;
        cell->TransformTextToPolySet( glyphs, 0, ARC_LOW_DEF, ERROR_INSIDE );

        if( glyphs.IsEmpty() )
            continue;

        const VECTOR2I anchor = cell->GetDrawPos();
        glyphs.Rotate( -cell->GetDrawRotation(), anchor );

        const BOX2I box = glyphs.BBox();
        const int   above = anchor.y - box.GetTop();
        const int   below = box.GetBottom() - anchor.y;

        // Centred text ignores the vertical margins, while top and bottom text sits one margin in
        switch( cell->GetVertJustify() )
        {
        case GR_TEXT_V_ALIGN_TOP:    height = std::max( height, cell->GetMarginTop() + below ); break;
        case GR_TEXT_V_ALIGN_BOTTOM: height = std::max( height, cell->GetMarginBottom() + above ); break;
        default:                     height = std::max( height, 2 * std::max( above, below ) ); break;
        }
    }

    return height;
}


const PCB_TABLECELL* PCB_GENERATED_TABLE::nearestDataCell( int aRow, int aCol, const std::set<KIID>& aStyled ) const
{
    const int rows = GetRowCount();

    for( int dist = 1; dist < rows; ++dist )
    {
        for( int row : { aRow - dist, aRow + dist } )
        {
            if( row < 1 || row >= rows || !IsDataRow( row ) )
                continue;

            if( const PCB_TABLECELL* cell = GetCell( row, aCol ); cell && aStyled.count( cell->m_Uuid ) )
                return cell;
        }
    }

    return nullptr;
}


void PCB_GENERATED_TABLE::styleFromColumn( PCB_TABLECELL* aCell, int aRow, int aCol, const std::set<KIID>& aStyled )
{
    const PCB_TABLECELL* donor = aRow > 0 ? nearestDataCell( aRow, aCol, aStyled ) : nullptr;

    if( !donor )
    {
        styleNewCell( aCell, aRow == 0 );
        return;
    }

    copyCellStyle( aCell, donor );
}


wxString PCB_GENERATED_TABLE::GetFontNameProp() const
{
    const PCB_TABLECELL* first = firstCell();

    return first ? first->GetFontProp() : wxString();
}


void PCB_GENERATED_TABLE::SetFontNameProp( const wxString& aFontName )
{
    forEachCell(
            [&]( PCB_TABLECELL* aCell )
            {
                aCell->SetFontProp( aFontName );
            } );
}


bool PCB_GENERATED_TABLE::FontNamesDiffer() const
{
    const wxString first = GetFontNameProp();

    return anyCell(
            [&]( const PCB_TABLECELL* aCell )
            {
                return aCell->GetFontProp() != first;
            } );
}


int PCB_GENERATED_TABLE::GetFontSizeProp() const
{
    const PCB_TABLECELL* first = firstCell();

    return first ? first->GetFontSize() : 0;
}


void PCB_GENERATED_TABLE::SetFontSizeProp( int aSize )
{
    forEachCell(
            [&]( PCB_TABLECELL* aCell )
            {
                aCell->SetFontSize( aSize );
            } );
}


bool PCB_GENERATED_TABLE::FontSizesDiffer() const
{
    const int first = GetFontSizeProp();

    return anyCell(
            [&]( const PCB_TABLECELL* aCell )
            {
                return !aCell->HasSquareFontSize() || aCell->GetFontSize() != first;
            } );
}


void PCB_GENERATED_TABLE::forEachCell( const std::function<void( PCB_TABLECELL* )>& aFunc )
{
    anyCell(
            [&]( const PCB_TABLECELL* aCell )
            {
                aFunc( const_cast<PCB_TABLECELL*>( aCell ) );
                return false;
            } );
}


bool PCB_GENERATED_TABLE::anyCell( const std::function<bool( const PCB_TABLECELL* )>& aTest ) const
{
    for( const PCB_TABLECELL* cell : m_cells )
    {
        if( aTest( cell ) )
            return true;
    }

    return false;
}


const PCB_TABLECELL* PCB_GENERATED_TABLE::firstCell() const
{
    const PCB_TABLECELL* first = nullptr;

    anyCell(
            [&]( const PCB_TABLECELL* aCell )
            {
                first = aCell;
                return true;
            } );

    return first;
}


void PCB_GENERATED_TABLE::SetColumns( std::vector<GENERATED_TABLE_COLUMN> aColumns )
{
    std::vector<GENERATED_TABLE_COLUMN> visible;

    for( const GENERATED_TABLE_COLUMN& col : aColumns )
    {
        if( col.m_Visible )
            visible.push_back( col );
    }

    const int oldCols = GetColCount();
    const int newCols = static_cast<int>( visible.size() );

    // The cells were laid out for the old columns, so this is the one place that knows which
    // column each belongs to. A rebuild only carries cells across by position
    if( oldCols > 0 && newCols > 0 && oldCols == static_cast<int>( m_columns.size() ) && !m_cells.empty() )
    {
        COLUMN_REMAP remap;
        remap.m_Rows = static_cast<int>( m_cells.size() ) / oldCols;
        remap.m_OldCols = oldCols;
        remap.m_NewCols = newCols;
        remap.m_Cells.assign( static_cast<size_t>( remap.m_Rows ) * newCols, nullptr );
        remap.m_Origin.assign( remap.m_Cells.size(), CELL_ORIGIN::EMPTY );
        remap.m_Carried.assign( m_cells.size(), false );

        moveShownCells( visible, remap );
        placeCells( remap );

        for( size_t ii = 0; ii < m_cells.size(); ++ii )
        {
            if( !remap.m_Carried[ii] )
                delete m_cells[ii];
        }

        m_cells = std::move( remap.m_Cells );
        m_colWidths = std::move( remap.m_ColWidths );
        SetColCount( newCols );

        styleCreatedCells( remap.m_Origin );
    }

    m_columns = std::move( visible );
    m_hidden.clear();

    for( size_t pos = 0; pos < aColumns.size(); ++pos )
    {
        if( aColumns[pos].m_Visible )
            continue;

        HIDDEN_COLUMN& hidden = m_hidden.emplace_back();
        hidden.m_Column = aColumns[pos];
        hidden.m_Position = static_cast<int>( pos );
    }
}


void PCB_GENERATED_TABLE::moveShownCells( const std::vector<GENERATED_TABLE_COLUMN>& aShown,
                                          COLUMN_REMAP&                              aRemap ) const
{
    for( int col = 0; col < aRemap.m_NewCols; ++col )
    {
        int from = -1;

        for( int ii = 0; ii < aRemap.m_OldCols && from < 0; ++ii )
        {
            if( m_columns[ii].m_Id == aShown[col].m_Id )
                from = ii;
        }

        if( from < 0 )
            continue;

        for( int row = 0; row < aRemap.m_Rows; ++row )
        {
            const size_t source = static_cast<size_t>( row ) * aRemap.m_OldCols + from;
            const size_t idx = static_cast<size_t>( row ) * aRemap.m_NewCols + col;

            aRemap.m_Cells[idx] = m_cells[source];
            aRemap.m_Origin[idx] = CELL_ORIGIN::MOVED;
            aRemap.m_Carried[source] = true;
        }

        if( const auto it = m_colWidths.find( from ); it != m_colWidths.end() )
            aRemap.m_ColWidths[col] = it->second;
    }
}


void PCB_GENERATED_TABLE::placeCells( COLUMN_REMAP& aRemap )
{
    struct SLOT
    {
        VECTOR2I  m_Start;
        VECTOR2I  m_End;
        VECTOR2I  m_Position;
        EDA_ANGLE m_Angle;
        bool      m_Mirrored;
    };

    // Taken before any cell moves, since a moved cell takes on its new slot's geometry
    std::vector<SLOT> slots;
    slots.reserve( m_cells.size() );

    for( const PCB_TABLECELL* cell : m_cells )
    {
        slots.push_back(
                { cell->GetStart(), cell->GetEnd(), cell->GetPosition(), cell->GetTextAngle(), cell->IsMirrored() } );
    }

    // A slot past the old last column borrows that column's geometry
    for( int row = 0; row < aRemap.m_Rows; ++row )
    {
        for( int col = 0; col < aRemap.m_NewCols; ++col )
        {
            const size_t    idx = static_cast<size_t>( row ) * aRemap.m_NewCols + col;
            const SLOT&     slot = slots[static_cast<size_t>( row ) * aRemap.m_OldCols
                                     + std::min( col, aRemap.m_OldCols - 1 )];
            PCB_TABLECELL*& cell = aRemap.m_Cells[idx];

            // Moved rather than given new corners, since a rotated cell can be a polygon
            if( aRemap.m_Origin[idx] == CELL_ORIGIN::MOVED )
            {
                cell->Move( slot.m_Position - cell->GetPosition() );
                continue;
            }

            // Styled later, since the rebuild takes a cell that already exists for one it styled
            if( aRemap.m_Origin[idx] == CELL_ORIGIN::EMPTY )
            {
                cell = new PCB_TABLECELL( this );
                aRemap.m_Origin[idx] = CELL_ORIGIN::CREATED;
            }

            cell->SetParent( this );
            cell->SetLayer( GetLayer() );
            cell->SetTextAngle( slot.m_Angle );
            cell->SetMirrored( slot.m_Mirrored );
            cell->SetStart( slot.m_Start );
            cell->SetEnd( slot.m_End );
        }
    }
}


void PCB_GENERATED_TABLE::styleCreatedCells( const std::vector<CELL_ORIGIN>& aOrigin )
{
    const int cols = GetColCount();

    for( size_t idx = 0; idx < m_cells.size(); ++idx )
    {
        if( aOrigin[idx] != CELL_ORIGIN::CREATED )
            continue;

        const int row = static_cast<int>( idx ) / cols;
        const int col = static_cast<int>( idx ) % cols;
        const PCB_TABLECELL* donor = nullptr;

        for( int dist = 1; dist < cols && !donor; ++dist )
        {
            for( int peer : { col - dist, col + dist } )
            {
                if( peer >= 0 && peer < cols
                    && aOrigin[static_cast<size_t>( row ) * cols + peer] == CELL_ORIGIN::MOVED )
                {
                    donor = GetCell( row, peer );
                    break;
                }
            }
        }

        if( donor )
            copyCellStyle( m_cells[idx], donor );
        else
            styleNewCell( m_cells[idx], row == 0 );
    }
}


void PCB_GENERATED_TABLE::migrateRows( int aRows, int aCols, const std::vector<std::string>& aNewRowKeys )
{
    const int oldCols = GetColCount();

    if( oldCols <= 0 || aCols <= 0 || m_cells.empty() )
        return;

    const int oldRows = static_cast<int>( m_cells.size() ) / oldCols;
    const int firstDataRow = 1;

    std::map<std::string, int> oldRowByKey;
    int                        oldFirstData = oldRows;
    int                        oldLastData = -1;

    // A legacy table's data rows have no identity beyond their position. Its trailers still go by
    // role, or a data row appearing would take over the totals
    const bool positionalData = m_rowKeys.empty();

    if( positionalData )
    {
        oldFirstData = firstDataRow;
        oldLastData = firstDataRow - 1;

        for( int row = firstDataRow; row < oldRows; ++row )
        {
            if( IsDataRow( row ) )
                oldLastData = row;
        }
    }
    else
    {
        for( const auto& [oldRow, key] : m_rowKeys )
        {
            if( oldRow < 0 || oldRow >= oldRows )
                continue;

            oldRowByKey[key] = oldRow;
            oldFirstData = std::min( oldFirstData, oldRow );
            oldLastData = std::max( oldLastData, oldRow );
        }

        if( oldRowByKey.empty() )
            return;
    }

    int newDataCount = 0;

    for( const std::string& key : aNewRowKeys )
    {
        if( !key.empty() )
            newDataCount++;
    }

    std::vector<int> sourceRow( aRows, -1 );

    // The title and heading are matched from the end of their run, so the heading stays the
    // heading when a title is added or removed
    for( int ii = 0; ii < firstDataRow; ++ii )
    {
        const int oldIdx = oldFirstData - ( firstDataRow - ii );

        if( oldIdx >= 0 )
            sourceRow[ii] = oldIdx;
    }

    if( positionalData )
    {
        const int count = std::min( newDataCount, oldLastData - oldFirstData + 1 );

        for( int ii = 0; ii < count; ++ii )
            sourceRow[firstDataRow + ii] = oldFirstData + ii;
    }
    else
    {
        for( int ii = firstDataRow; ii < aRows; ++ii )
        {
            if( aNewRowKeys[ii].empty() )
                continue;

            const auto it = oldRowByKey.find( aNewRowKeys[ii] );

            if( it != oldRowByKey.end() )
                sourceRow[ii] = it->second;
        }
    }

    const int trailingNewStart = firstDataRow + newDataCount;

    for( int ii = trailingNewStart; ii < aRows; ++ii )
    {
        const int oldIdx = oldLastData + 1 + ( ii - trailingNewStart );

        if( oldIdx < oldRows )
            sourceRow[ii] = oldIdx;
    }

    std::vector<PCB_TABLECELL*> newCells( static_cast<size_t>( aRows ) * aCols, nullptr );
    std::vector<bool>           carried( m_cells.size(), false );
    std::map<int, int>          newRowHeights;

    for( int ii = 0; ii < aRows; ++ii )
    {
        if( sourceRow[ii] < 0 )
            continue;

        // A column added since the last rebuild has no cell to carry, and one taken away
        // leaves its cells behind to be deleted with the rest of the uncarried ones
        for( int col = 0; col < std::min( oldCols, aCols ); ++col )
        {
            const size_t from = static_cast<size_t>( sourceRow[ii] ) * oldCols + col;

            newCells[static_cast<size_t>( ii ) * aCols + col] = m_cells[from];
            carried[from] = true;
        }

        const auto heightIt = m_rowHeights.find( sourceRow[ii] );

        if( heightIt != m_rowHeights.end() )
            newRowHeights[ii] = heightIt->second;
    }

    for( size_t ii = 0; ii < m_cells.size(); ++ii )
    {
        if( !carried[ii] )
            delete m_cells[ii];
    }

    for( PCB_TABLECELL*& cell : newCells )
    {
        if( !cell )
        {
            cell = new PCB_TABLECELL( this );
            cell->SetLayer( GetLayer() );
        }
    }

    m_cells = std::move( newCells );
    m_rowHeights = std::move( newRowHeights );
}


void PCB_GENERATED_TABLE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    PCB_TABLE::Flip( aCentre, aFlipDirection );

    // Otherwise the next rebuild lays the columns back out in their old order
    std::reverse( m_columns.begin(), m_columns.end() );
    onColumnsReversed();

    // A hidden column keeps its place in the reversed order too
    const int total = static_cast<int>( m_columns.size() + m_hidden.size() );

    for( HIDDEN_COLUMN& hidden : m_hidden )
        hidden.m_Position = total - 1 - hidden.m_Position;

    std::reverse( m_hidden.begin(), m_hidden.end() );

    // Trailer rows are laid out by position rather than by column, so their text goes back to
    // where a rebuild puts it. The cells stay where the flip left them
    const int cols = GetColCount();

    for( int row = 1; row < GetRowCount(); ++row )
    {
        if( IsDataRow( row ) )
            continue;

        for( int col = 0; col < cols / 2; ++col )
        {
            PCB_TABLECELL* left = GetCell( row, col );
            PCB_TABLECELL* right = GetCell( row, cols - 1 - col );
            const wxString text = left->GetText();

            left->SetText( right->GetText() );
            right->SetText( text );
        }
    }
}


INSPECT_RESULT PCB_GENERATED_TABLE::Visit( INSPECTOR aInspector, void* aTestData,
                                           const std::vector<KICAD_T>& aScanTypes )
{
    // A generated table answers to its own type and to PCB_TABLE_T, so reporting per matching
    // type would hand it to the inspector twice and list it twice in the disambiguation menu
    bool wantTable = false;
    bool wantCells = false;

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == Type() || scanType == PCB_TABLE_T )
            wantTable = true;
        else if( scanType == PCB_TABLECELL_T )
            wantCells = true;
    }

    if( wantTable && INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
        return INSPECT_RESULT::QUIT;

    if( wantCells )
    {
        for( PCB_TABLECELL* cell : GetCells() )
        {
            if( INSPECT_RESULT::QUIT == aInspector( cell, aTestData ) )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


void PCB_GENERATED_TABLE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( GetFriendlyName(), wxEmptyString );
    aList.emplace_back( _( "Rows" ), wxString::Format( wxT( "%d" ), GetRowCount() ) );
    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


bool PCB_GENERATED_TABLE::IsStale( const BOARD& aBoard ) const
{
    const std::unique_ptr<GENERATED_TABLE_CONTENT> content = generate( aBoard, nullptr );

    const int cols = static_cast<int>( m_columns.size() );
    const int dataRows = static_cast<int>( content->m_Rows.size() );
    const int rows = 1 + dataRows + static_cast<int>( content->m_Trailer.size() );

    if( GetColCount() != cols || GetRowCount() != rows )
        return true;

    const auto differs =
            [&]( int aRow, int aCol, const wxString& aText )
            {
                const PCB_TABLECELL* cell = GetCell( aRow, aCol );

                return !cell || cell->GetText() != aText;
            };

    for( int col = 0; col < cols; ++col )
    {
        if( differs( 0, col, m_columns[col].m_Heading ) )
            return true;
    }

    for( int ii = 0; ii < dataRows; ++ii )
    {
        const GENERATED_TABLE_ROW& row = content->m_Rows[ii];
        const auto                 keyIt = m_rowKeys.find( 1 + ii );

        if( keyIt == m_rowKeys.end() || keyIt->second != row.m_Key )
            return true;

        for( int col = 0; col < cols; ++col )
        {
            if( differs( 1 + ii, col, textAt( row.m_Cells, col ) ) )
                return true;
        }
    }

    for( size_t ii = 0; ii < content->m_Trailer.size(); ++ii )
    {
        for( int col = 0; col < cols; ++col )
        {
            if( differs( 1 + dataRows + static_cast<int>( ii ), col, textAt( content->m_Trailer[ii], col ) ) )
                return true;
        }
    }

    return false;
}


void PCB_GENERATED_TABLE::RebuildCells( const BOARD& aBoard, GENERATED_TABLE_REFRESH* aRefresh )
{
    std::unique_ptr<GENERATED_TABLE_CONTENT> content = generate( aBoard, aRefresh );

    const int cols = static_cast<int>( m_columns.size() );
    const int dataRows = static_cast<int>( content->m_Rows.size() );

    // The headings are the header row. A table carries no caption of its own
    const int rows = 1 + dataRows + static_cast<int>( content->m_Trailer.size() );

    // A new cell carries a half-INT_MAX rectangle and Normalize() anchors on cell 0's centre,
    // so without holding the old position a rebuild lands the table half a metre off-board
    const VECTOR2I anchor = GetCells().empty() ? VECTOR2I( 0, 0 ) : GetPosition();

    std::vector<std::string> newRowKeys( rows );

    // What a row reports, so a rebuild hands its formatting to the row still reporting the
    // same thing rather than to whatever lands on its index
    for( int ii = 0; ii < dataRows; ++ii )
        newRowKeys[1 + ii] = content->m_Rows[ii].m_Key;

    // By UUID, not pointer. A cell migrateRows deletes can hand its address to a new one
    std::set<KIID> existing;

    for( PCB_TABLECELL* cell : GetCells() )
        existing.insert( cell->m_Uuid );

    migrateRows( rows, cols, newRowKeys );

    onRowsMigrated();

    SetColCount( cols );
    ResizeCells( rows, cols );

    m_rowKeys.clear();

    for( int ii = 0; ii < rows; ++ii )
    {
        if( !newRowKeys[ii].empty() )
            m_rowKeys[ii] = newRowKeys[ii];
    }

    const auto setCell =
            [&]( int aRow, int aCol, const wxString& aText )
            {
                PCB_TABLECELL* cell = GetCell( aRow, aCol );

                if( !cell )
                    return;

                if( !existing.count( cell->m_Uuid ) )
                    styleFromColumn( cell, aRow, aCol, existing );

                cell->SetText( aText );

                // The column's alignment, which was otherwise editable, serialized and
                // ignored
                cell->SetHorizJustify( toHorizJustify( m_columns[aCol].m_Align ) );
            };

    for( int col = 0; col < cols; ++col )
        setCell( 0, col, m_columns[col].m_Heading );

    for( int ii = 0; ii < dataRows; ++ii )
    {
        for( int col = 0; col < cols; ++col )
            setCell( 1 + ii, col, textAt( content->m_Rows[ii].m_Cells, col ) );
    }

    for( size_t ii = 0; ii < content->m_Trailer.size(); ++ii )
    {
        for( int col = 0; col < cols; ++col )
            setCell( 1 + dataRows + static_cast<int>( ii ), col, textAt( content->m_Trailer[ii], col ) );
    }

    // Autosize measures text as wrapped to its cell, so a reused cell narrower than its new text
    // would keep the wrap. Widen those to their resolved, unwrapped text, which Autosize then tightens
    bool prewidened = false;

    for( int col = 0; col < cols; ++col )
    {
        int width = 0;

        for( int row = 0; row < rows; ++row )
        {
            const PCB_TABLECELL* cell = GetCell( row, col );

            if( !cell )
                continue;

            const int      penWidth = cell->GetEffectiveTextPenWidth();
            const VECTOR2I extents = cell->GetDrawFont( nullptr )->StringBoundaryLimits(
                    cell->GetUnwrappedShownText( FOR_CANVAS ), cell->GetTextSize(), penWidth, cell->IsBold(),
                    cell->IsItalic(), cell->GetFontMetrics() );

            // GetShownText breaks at the width less margins, and LinebreakText takes the pen off too
            width = std::max( width, extents.x + penWidth + 2 * ( cell->GetMarginLeft() + cell->GetMarginRight() ) );
        }

        if( width > GetColWidth( col ) )
        {
            SetColWidth( col, width );
            prewidened = true;
        }
    }

    if( prewidened )
        Normalize();

    Autosize();

    // After autosizing or the authored width never shows, and as a minimum so a width
    // saved against a narrower board cannot clip its text
    bool widened = false;

    for( int col = 0; col < cols; ++col )
    {
        if( m_columns[col].m_Width > GetColWidth( col ) )
        {
            SetColWidth( col, m_columns[col].m_Width );
            widened = true;
        }
    }

    // The widths above are only a map until the cells are placed against them
    if( widened )
        Normalize();

    Move( anchor - GetPosition() );

    // Last, so a table is only ever marked fresh once its layout is complete
    onRebuilt( aBoard, *content );
}


static struct PCB_GENERATED_TABLE_DESC
{
    PCB_GENERATED_TABLE_DESC()
    {
        ENUM_MAP<GENERATED_TABLE_UNITS>& unitsEnum = ENUM_MAP<GENERATED_TABLE_UNITS>::Instance();

        if( unitsEnum.Choices().GetCount() == 0 )
        {
            unitsEnum.Map( GENERATED_TABLE_UNITS::MM, _HKI( "Millimeters" ) )
                    .Map( GENERATED_TABLE_UNITS::MILS, _HKI( "Mils" ) )
                    .Map( GENERATED_TABLE_UNITS::INCH, _HKI( "Inches" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_GENERATED_TABLE );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATED_TABLE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATED_TABLE, BOARD_ITEM_CONTAINER> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_GENERATED_TABLE, PCB_TABLE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_GENERATED_TABLE ), TYPE_HASH( PCB_TABLE ) );

        const wxString generatorProps = _( "Table Generator" );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_GENERATED_TABLE, GENERATED_TABLE_UNITS>(
                                     _HKI( "Units" ), &PCB_GENERATED_TABLE::SetUnits, &PCB_GENERATED_TABLE::GetUnits ),
                             generatorProps );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, int>( _HKI( "Decimal Places" ),
                                                                     &PCB_GENERATED_TABLE::SetPrecision,
                                                                     &PCB_GENERATED_TABLE::GetPrecision ),
                             generatorProps );

        // Scoped here rather than removed from PCB_TABLE so colors saved on plain tables stay editable
        propMgr.Mask( TYPE_HASH( PCB_GENERATED_TABLE ), TYPE_HASH( PCB_TABLE ), _HKI( "Border Color" ) );
        propMgr.Mask( TYPE_HASH( PCB_GENERATED_TABLE ), TYPE_HASH( PCB_TABLE ), _HKI( "Separators Color" ) );
        propMgr.Mask( TYPE_HASH( PCB_GENERATED_TABLE ), TYPE_HASH( PCB_TABLE ), _HKI( "Cell Separators" ) );

        // Registered again below so it sits beside Column Separators
        propMgr.Mask( TYPE_HASH( PCB_GENERATED_TABLE ), TYPE_HASH( PCB_TABLE ), _HKI( "Row Separators" ) );

        auto differs =
                []( bool ( PCB_GENERATED_TABLE::*aTest )() const )
                {
                    return [aTest]( INSPECTABLE* aItem ) -> bool
                    {
                        PCB_GENERATED_TABLE* table = dynamic_cast<PCB_GENERATED_TABLE*>( aItem );
                        return table && ( table->*aTest )();
                    };
                };

        const wxString tableProps = _( "Table Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, bool, PCB_TABLE>(
                                     _HKI( "Row Separators" ), &PCB_TABLE::SetStrokeRows, &PCB_TABLE::StrokeRows ),
                             tableProps )
                .SetIsCopyable();

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, bool, PCB_TABLE>( _HKI( "Column Separators" ),
                                                                                 &PCB_TABLE::SetStrokeColumns,
                                                                                 &PCB_TABLE::StrokeColumns ),
                             tableProps )
                .SetIsCopyable();

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, int>(
                                     _HKI( "Row Height" ), &PCB_GENERATED_TABLE::SetRowHeightProp,
                                     &PCB_GENERATED_TABLE::GetRowHeightProp, PROPERTY_DISPLAY::PT_SIZE ),
                             tableProps )
                .SetIsCopyable()
                .SetIndeterminateFunc( differs( &PCB_GENERATED_TABLE::RowHeightsDiffer ) );

        const wxString fontProps = _HKI( "Font" );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, wxString>( _HKI( "Font Name" ),
                                                                          &PCB_GENERATED_TABLE::SetFontNameProp,
                                                                          &PCB_GENERATED_TABLE::GetFontNameProp ),
                             fontProps )
                .SetIsHiddenFromRulesEditor()
                .SetIsCopyable()
                .SetIndeterminateFunc( differs( &PCB_GENERATED_TABLE::FontNamesDiffer ) )
                .SetChoicesFunc( &PCB_TABLECELL::FontChoices );

        propMgr.AddProperty( new PROPERTY<PCB_GENERATED_TABLE, int>(
                                     _HKI( "Font Size" ), &PCB_GENERATED_TABLE::SetFontSizeProp,
                                     &PCB_GENERATED_TABLE::GetFontSizeProp, PROPERTY_DISPLAY::PT_SIZE ),
                             fontProps )
                .SetIsCopyable()
                .SetIndeterminateFunc( differs( &PCB_GENERATED_TABLE::FontSizesDiffer ) );
    }
} _PCB_GENERATED_TABLE_DESC;


ENUM_TO_WXANY( GENERATED_TABLE_UNITS )
