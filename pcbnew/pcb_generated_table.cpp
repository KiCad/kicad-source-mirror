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

#include <set>

#include <pcb_generated_table.h>

#include <board.h>
#include <board_tables/generated_table_refresh.h>
#include <font/font.h>
#include <i18n_utility.h>
#include <pcb_tablecell.h>
#include <properties/property_mgr.h>


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


void PCB_GENERATED_TABLE::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage && aImage->Type() == Type(), wxT( "Cannot swap data with invalid table." ) );

    PCB_TABLE::swapData( aImage );

    PCB_GENERATED_TABLE* other = static_cast<PCB_GENERATED_TABLE*>( aImage );

    std::swap( m_columns, other->m_columns );
    std::swap( m_units, other->m_units );
    std::swap( m_precision, other->m_precision );
    std::swap( m_rowKeys, other->m_rowKeys );
}


bool PCB_GENERATED_TABLE::generatedEquals( const PCB_GENERATED_TABLE& aOther ) const
{
    return m_columns == aOther.m_columns && m_units == aOther.m_units && m_precision == aOther.m_precision
           && m_rowKeys == aOther.m_rowKeys && PCB_TABLE::operator==( aOther );
}


void PCB_GENERATED_TABLE::migrateRows( int aRows, int aCols, const std::vector<std::string>& aNewRowKeys )
{
    const int oldCols = GetColCount();

    // With no recorded keys a row has no identity beyond its position, which is what
    // ResizeCells already preserves
    if( oldCols <= 0 || aCols <= 0 || m_cells.empty() || m_rowKeys.empty() )
        return;

    const int oldRows = static_cast<int>( m_cells.size() ) / oldCols;
    const int firstDataRow = 1;

    std::map<std::string, int> oldRowByKey;
    int                        oldFirstData = oldRows;
    int                        oldLastData = -1;

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

    for( int ii = firstDataRow; ii < aRows; ++ii )
    {
        if( aNewRowKeys[ii].empty() )
            continue;

        const auto it = oldRowByKey.find( aNewRowKeys[ii] );

        if( it != oldRowByKey.end() )
            sourceRow[ii] = it->second;
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
    // so without holding the old position a rebuild lands the chart half a metre off-board
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
                    styleNewCell( cell, aRow == 0 );

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
    }
} _PCB_GENERATED_TABLE_DESC;


ENUM_TO_WXANY( GENERATED_TABLE_UNITS )
