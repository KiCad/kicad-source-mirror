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

#include <pcb_drill_chart.h>

#include <board.h>
#include <board_design_settings.h>
#include <drill/drill_chart_model.h>
#include <drill/drill_enumerator.h>
#include <drill/drill_symbol_assigner.h>
#include <base_units.h>
#include <string_utils.h>
#include <eda_text.h>
#include <i18n_utility.h>
#include <pcb_tablecell.h>
#include <properties/property_mgr.h>
#include <widgets/msgpanel.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>


PCB_DRILL_CHART::PCB_DRILL_CHART( BOARD_ITEM* aParent ) :
        PCB_TABLE( aParent, PCB_DRILL_CHART_T, pcbIUScale.mmToIU( 0.1 ) ),
        m_units( DRILL_CHART_UNITS::MM ),
        m_precision( 3 ),
        m_showTotals( true ),
        m_symbolColumn( -1 ),
        m_builtGeneration( 0 )
{
    ApplyTemplate( DRILL_CHART_TEMPLATE::MakeDefault() );
}


PCB_DRILL_CHART::PCB_DRILL_CHART( const PCB_DRILL_CHART& aOther ) :
        PCB_TABLE( aOther ),
        m_filter( aOther.m_filter ),
        m_columns( aOther.m_columns ),
        m_units( aOther.m_units ),
        m_precision( aOther.m_precision ),
        m_showTotals( aOther.m_showTotals ),
        m_rowShapes( aOther.m_rowShapes ),
        m_rowKeys( aOther.m_rowKeys ),
        m_symbolColumn( aOther.m_symbolColumn ),
        m_builtGeneration( aOther.m_builtGeneration )
{
    // PCB_TABLE's copy ctor propagates m_structType, so the clone reports PCB_DRILL_CHART_T
    // and swapData's guard stays meaningful
}


void PCB_DRILL_CHART::ApplyTemplate( const DRILL_CHART_TEMPLATE& aTemplate )
{
    m_columns = aTemplate.Columns();
    m_units = aTemplate.GetUnits();
    m_precision = aTemplate.GetPrecision();
    m_showTotals = aTemplate.GetShowTotals();
}


void PCB_DRILL_CHART::swapData( BOARD_ITEM* aImage )
{
    wxCHECK_RET( aImage && aImage->Type() == Type(), wxT( "Cannot swap data with invalid chart." ) );

    PCB_TABLE::swapData( aImage );

    PCB_DRILL_CHART* other = static_cast<PCB_DRILL_CHART*>( aImage );

    std::swap( m_filter, other->m_filter );
    std::swap( m_columns, other->m_columns );
    std::swap( m_units, other->m_units );
    std::swap( m_precision, other->m_precision );
    std::swap( m_showTotals, other->m_showTotals );
    std::swap( m_builtGeneration, other->m_builtGeneration );
    std::swap( m_rowShapes, other->m_rowShapes );
    std::swap( m_rowKeys, other->m_rowKeys );
    std::swap( m_symbolColumn, other->m_symbolColumn );
}


std::vector<DRILL_CHART_GROUP> PCB_DRILL_CHART::buildGroups( const BOARD& aBoard ) const
{
    const DRILL_SYMBOL_PROFILE& profile = aBoard.GetDesignSettings().GetDrillSymbolProfile();

    DRILL_CHART_ROW_SPEC spec;
    spec.m_Filter = m_filter;

    // Every span on the board. What shares a row is the profile's grouping to decide
    DRILL_CHART_MODEL model( profile );
    model.Build( aBoard, EnumerateDrillSpans( aBoard ), spec );

    return model.Groups();
}


void PCB_DRILL_CHART::migrateRows( int aRows, int aCols, int aFirstDataRow,
                                   const std::vector<std::string>& aNewRowKeys )
{
    const int oldCols = GetColCount();

    // With no recorded keys a row has no identity beyond its position, which is what
    // ResizeCells already preserves
    if( oldCols <= 0 || aCols <= 0 || m_cells.empty() || m_rowKeys.empty() )
        return;

    const int oldRows = static_cast<int>( m_cells.size() ) / oldCols;

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
    for( int ii = 0; ii < aFirstDataRow; ++ii )
    {
        const int oldIdx = oldFirstData - ( aFirstDataRow - ii );

        if( oldIdx >= 0 )
            sourceRow[ii] = oldIdx;
    }

    for( int ii = aFirstDataRow; ii < aRows; ++ii )
    {
        if( aNewRowKeys[ii].empty() )
            continue;

        const auto it = oldRowByKey.find( aNewRowKeys[ii] );

        if( it != oldRowByKey.end() )
            sourceRow[ii] = it->second;
    }

    const int trailingNewStart = aFirstDataRow + newDataCount;

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


bool PCB_DRILL_CHART::IsDataRow( int aRow ) const
{
    if( !m_rowKeys.empty() )
        return m_rowKeys.count( aRow ) > 0;

    // A chart written before the keys were recorded still has to answer this, and its rows are
    // laid out the way RebuildCells lays them out
    const int firstDataRow = 1;
    const int lastDataRow = GetRowCount() - 1 - ( m_showTotals ? 1 : 0 );

    return aRow >= firstDataRow && aRow <= lastDataRow;
}


INSPECT_RESULT PCB_DRILL_CHART::Visit( INSPECTOR aInspector, void* aTestData,
                                       const std::vector<KICAD_T>& aScanTypes )
{
    // A chart answers to both scan types, so reporting per matching type would hand it to the
    // inspector twice and list it twice in the disambiguation menu
    bool wantChart = false;
    bool wantCells = false;

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == PCB_DRILL_CHART_T || scanType == PCB_TABLE_T )
            wantChart = true;
        else if( scanType == PCB_TABLECELL_T )
            wantCells = true;
    }

    if( wantChart && INSPECT_RESULT::QUIT == aInspector( this, aTestData ) )
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


wxString PCB_DRILL_CHART::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Drill Chart (%d rows)" ), GetRowCount() );
}


void PCB_DRILL_CHART::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Drill Chart" ), wxEmptyString );
    aList.emplace_back( _( "Rows" ), wxString::Format( wxT( "%d" ), GetRowCount() ) );
    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


double PCB_DRILL_CHART::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    return PCB_TABLE::Similarity( aOther );
}


bool PCB_DRILL_CHART::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_DRILL_CHART& other = static_cast<const PCB_DRILL_CHART&>( aOther );

    // Must cover everything swapData swaps. The git merge driver decides a change is a
    // change from this, so an omitted member is a silently dropped edit
    return m_filter == other.m_filter
           && m_columns == other.m_columns && m_units == other.m_units
           && m_precision == other.m_precision
           && m_showTotals == other.m_showTotals
           && m_rowShapes == other.m_rowShapes && m_rowKeys == other.m_rowKeys
           && m_symbolColumn == other.m_symbolColumn && PCB_TABLE::operator==( aOther );
}


namespace
{

const wxString NO_VALUE( wxS( "\u2014" ) );


wxString formatLength( int aValue, DRILL_CHART_UNITS aUnits, int aPrecision )
{
    const double mm = pcbIUScale.IUTomm( aValue );

    switch( aUnits )
    {
    case DRILL_CHART_UNITS::INCH:
        return wxString::Format( wxT( "%.*f\u2033" ), aPrecision + 1, mm / 25.4 );

    case DRILL_CHART_UNITS::MM:
    default:
        return wxString::Format( wxT( "%.*f" ), aPrecision, mm );
    }
}


wxString spanText( const BOARD& aBoard, const DRILL_CHART_GROUP& aGroup )
{
    // The board's own layer names, so a chart matches the stackup the fabricator was given
    return wxString::Format( wxT( "%s / %s" ), aBoard.GetLayerName( aGroup.m_TopLayer ),
                             aBoard.GetLayerName( aGroup.m_BottomLayer ) );
}


wxString operationText( const DRILL_CHART_GROUP& aGroup )
{
    if( aGroup.m_Kind != DRILL_OP_KIND::PRIMARY_DRILL )
        return wxT( "Backdrill" );

    if( aGroup.m_IsSlot )
        return wxT( "Slot" );

    return wxT( "Drill" );
}


wxString protectionText( const DRILL_CHART_GROUP& aGroup )
{
    wxArrayString parts;

    if( aGroup.m_Filled )
        parts.Add( wxT( "filled" ) );

    if( aGroup.m_Capped )
        parts.Add( wxT( "capped" ) );

    if( aGroup.m_TopPlugged || aGroup.m_BottomPlugged )
        parts.Add( wxT( "plugged" ) );

    if( aGroup.m_TopCovered || aGroup.m_BottomCovered )
        parts.Add( wxT( "covered" ) );

    if( aGroup.m_TopTented || aGroup.m_BottomTented )
        parts.Add( wxT( "tented" ) );

    return parts.IsEmpty() ? NO_VALUE : wxJoin( parts, ',' );
}


wxString symbolText( const DRILL_CHART_GROUP& aGroup, DRILL_CHART_UNITS aUnits, int aPrecision )
{
    switch( aGroup.m_Symbol.m_MarkMode )
    {
    case DRILL_MARK_MODE::LETTER:
        return aGroup.m_Symbol.m_Letter;

    case DRILL_MARK_MODE::SIZE_TEXT:
        return formatLength( aGroup.m_Diameter, aUnits, aPrecision );

    case DRILL_MARK_MODE::SHAPE:
    default:
        // The shape itself is drawn by the painter. The cell carries no text
        return wxEmptyString;
    }
}

} // namespace


void PCB_DRILL_CHART::RebuildCells( const BOARD& aBoard, DRILL_SYMBOL_PROFILE* aAssignedProfile )
{
    std::vector<DRILL_CHART_GROUP> groups = buildGroups( aBoard );

    if( aAssignedProfile )
    {
        // The caller's copy, so a cancelled placement leaves nothing behind and a batch
        // rebuild accumulates instead of keeping only the last chart's
        AssignDrillSymbols( groups, *aAssignedProfile );
    }
    else
    {
        // Whole board, not this chart's filtered subset, or a filtered chart prints a
        // different symbol from the map for the same hole
        const std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> resolved =
                ResolveDrillSymbols( aBoard );

        for( DRILL_CHART_GROUP& group : groups )
        {
            const auto it = resolved.find( group.m_SymbolKey );

            if( it != resolved.end() )
                group.m_Symbol = it->second;
        }
    }

    const int cols = static_cast<int>( m_columns.size() );
    const int totalRows = m_showTotals ? 1 : 0;

    // The headings are the header row. A chart carries no caption of its own
    const int rows = 1 + static_cast<int>( groups.size() ) + totalRows;

    // A new cell carries a half-INT_MAX rectangle and Normalize() anchors on cell 0's centre,
    // so without holding the old position a rebuild lands the chart half a metre off-board
    const VECTOR2I anchor = GetCells().empty() ? VECTOR2I( 0, 0 ) : GetPosition();

    std::vector<std::string> newRowKeys( rows );

    // What a row reports, so a rebuild hands its formatting to the row still reporting the
    // same holes rather than to whatever lands on its index
    for( size_t ii = 0; ii < groups.size(); ++ii )
        newRowKeys[1 + static_cast<int>( ii )] = groups[ii].m_Key;

    migrateRows( rows, cols, 1, newRowKeys );

    SetColCount( cols );
    ResizeCells( rows, cols );

    m_rowKeys.clear();

    for( int ii = 0; ii < rows; ++ii )
    {
        if( !newRowKeys[ii].empty() )
            m_rowKeys[ii] = newRowKeys[ii];
    }

    int row = 0;

    auto setCell =
            [&]( int aRow, int aCol, const wxString& aText )
            {
                PCB_TABLECELL* cell = GetCell( aRow, aCol );

                if( !cell )
                    return;

                cell->SetText( aText );

                // The column's alignment, which was otherwise editable, serialized and
                // ignored
                switch( m_columns[aCol].m_Align )
                {
                case DRILL_CHART_ALIGN::LEFT:
                    cell->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                    break;

                case DRILL_CHART_ALIGN::CENTER:
                    cell->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                    break;

                case DRILL_CHART_ALIGN::RIGHT:
                    cell->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    break;
                }
            };

    m_rowShapes.clear();

    for( int col = 0; col < cols; ++col )
        setCell( row, col, m_columns[col].m_Heading );

    row++;

    m_symbolColumn = -1;

    for( int col = 0; col < cols; ++col )
    {
        if( m_columns[col].m_Id == DRILL_CHART_COLUMN_ID::SYMBOL )
            m_symbolColumn = col;
    }

    for( const DRILL_CHART_GROUP& group : groups )
    {
        if( group.m_Symbol.m_MarkMode == DRILL_MARK_MODE::SHAPE )
            m_rowShapes[row] = group.m_Symbol.m_ShapeIndex;

        for( int col = 0; col < cols; ++col )
        {
            wxString text;

            switch( m_columns[col].m_Id )
            {
            case DRILL_CHART_COLUMN_ID::SYMBOL:
                text = symbolText( group, m_units, m_precision );
                break;

            case DRILL_CHART_COLUMN_ID::DRILL_DIAMETER:
                text = formatLength( group.m_Diameter, m_units, m_precision );
                break;

            case DRILL_CHART_COLUMN_ID::SLOT_SIZE:
                text = group.m_IsSlot ? wxString::Format( wxT( "%s x %s" ),
                                                          formatLength( group.m_SizeXY.x, m_units, m_precision ),
                                                          formatLength( group.m_SizeXY.y, m_units, m_precision ) )
                                      : NO_VALUE;
                break;

            case DRILL_CHART_COLUMN_ID::PLATING:
                text = group.m_NotPlated ? wxT( "No" ) : wxT( "Yes" );
                break;

            case DRILL_CHART_COLUMN_ID::OPERATION_COUNT:
                text = wxString::Format( wxT( "%d" ), group.m_OperationCount );
                break;

            case DRILL_CHART_COLUMN_ID::SITE_COUNT:
                text = wxString::Format( wxT( "%d" ), group.m_SiteCount );
                break;

            case DRILL_CHART_COLUMN_ID::LAYER_SPAN:
                text = spanText( aBoard, group );
                break;

            case DRILL_CHART_COLUMN_ID::OPERATION:
                text = operationText( group );
                break;

            case DRILL_CHART_COLUMN_ID::PROTECTION:
                text = protectionText( group );
                break;

            case DRILL_CHART_COLUMN_ID::BACKDRILL_STUB:
                text = group.m_StubLength.has_value()
                               ? formatLength( *group.m_StubLength, m_units, m_precision )
                               : NO_VALUE;
                break;

            case DRILL_CHART_COLUMN_ID::ASPECT_RATIO:
            {
                // Depth over diameter, the number a fabricator uses to judge plating
                // difficulty. Needs a real stackup, so it stays an em dash without one
                const int depth = aBoard.GetStackupOrDefault().GetLayerDistance(
                        group.m_TopLayer, group.m_BottomLayer );

                const int diameter = group.m_Diameter;

                text = depth > 0 && diameter > 0
                               ? wxString::Format( wxT( "%.1f:1" ), (double) depth / diameter )
                               : NO_VALUE;
                break;
            }

            case DRILL_CHART_COLUMN_ID::DESCRIPTION:
                text = group.m_Symbol.m_Description;
                break;
            }

            setCell( row, col, text );
        }

        row++;
    }

    if( totalRows )
    {
        int operations = 0;

        // Sites are counted over distinct coordinates, not summed per group. A backdrilled
        // via contributes several operations at one location and is still one site
        std::set<std::pair<int, int>> sites;

        for( const DRILL_CHART_GROUP& group : groups )
        {
            operations += group.m_OperationCount;

            for( const VECTOR2I& site : group.m_Sites )
                sites.emplace( site.x, site.y );
        }

        setCell( row, 0, wxString::Format( wxT( "%d OPS / %zu SITES" ), operations,
                                           sites.size() ) );

        for( int col = 1; col < cols; ++col )
            setCell( row, col, wxEmptyString );
    }

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

    m_builtGeneration = aBoard.GetDrillModelGeneration();

}


void PCB_DRILL_CHART::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board;
    types::DrillChart chart;

    google::protobuf::Any tableAny;
    PCB_TABLE::Serialize( tableAny );
    tableAny.UnpackTo( chart.mutable_table() );

    types::DrillChartFilter* filter = chart.mutable_filter();
    filter->set_plated( m_filter.m_Plated );
    filter->set_non_plated( m_filter.m_NonPlated );
    filter->set_vias( m_filter.m_Vias );
    filter->set_slots( m_filter.m_Slots );
    filter->set_backdrills( m_filter.m_Backdrills );
    filter->set_castellated( m_filter.m_Castellated );

    for( const DRILL_CHART_COLUMN& col : m_columns )
    {
        types::DrillChartColumn* proto = chart.add_columns();
        proto->set_id( static_cast<types::DrillChartColumnId>( static_cast<int>( col.m_Id ) + 1 ) );
        proto->set_heading( col.m_Heading.ToStdString() );
        proto->set_align( static_cast<types::DrillChartAlign>( static_cast<int>( col.m_Align ) + 1 ) );
        kiapi::common::PackDistance( *proto->mutable_width(), col.m_Width );
    }

    switch( m_units )
    {
    case DRILL_CHART_UNITS::MM:   chart.set_units( kiapi::common::types::U_MM ); break;
    case DRILL_CHART_UNITS::INCH: chart.set_units( kiapi::common::types::U_INCH ); break;
    }

    chart.set_precision( m_precision );
    chart.set_show_totals( m_showTotals );

    chart.set_symbol_column( m_symbolColumn );

    for( const auto& [row, shapeIndex] : m_rowShapes )
        ( *chart.mutable_row_shapes() )[row] = shapeIndex;

    for( const auto& [row, key] : m_rowKeys )
        ( *chart.mutable_row_keys() )[row] = key;

    aContainer.PackFrom( chart );

}


bool PCB_DRILL_CHART::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board;
    types::DrillChart chart;

    if( !aContainer.UnpackTo( &chart ) )
    {
        return false;
    }

    google::protobuf::Any tableAny;
    tableAny.PackFrom( chart.table() );

    if( !PCB_TABLE::Deserialize( tableAny ) )
    {
        return false;
    }

    // The table carries the layer. A chart on a manufacturing layer would be plotted into a
    // fabrication output rather than the documentation
    if( !DrillDocumentationLayers().Contains( GetLayer() ) )
    {
        return false;
    }

    if( chart.has_filter() )
    {
        m_filter.m_Plated = chart.filter().plated();
        m_filter.m_NonPlated = chart.filter().non_plated();
        m_filter.m_Vias = chart.filter().vias();
        m_filter.m_Slots = chart.filter().slots();
        m_filter.m_Backdrills = chart.filter().backdrills();
        m_filter.m_Castellated = chart.filter().castellated();
    }

    m_columns.clear();

    for( const types::DrillChartColumn& proto : chart.columns() )
    {
        if( proto.id() == types::DCC_UNKNOWN )
            continue;

        if( proto.id() > types::DCC_DESCRIPTION )
        {
            return false;
        }

        DRILL_CHART_COLUMN col;
        col.m_Id = static_cast<DRILL_CHART_COLUMN_ID>( static_cast<int>( proto.id() ) - 1 );
        col.m_Heading = wxString::FromUTF8( proto.heading() );

        if( proto.align() < types::DCA_UNKNOWN || proto.align() > types::DCA_RIGHT )
        {
            return false;
        }

        if( proto.align() != types::DCA_UNKNOWN )
            col.m_Align = static_cast<DRILL_CHART_ALIGN>( static_cast<int>( proto.align() ) - 1 );

        kiapi::common::types::Distance width;
        width.set_value_nm( std::clamp<int64_t>( proto.width().value_nm(), 0,
                                               pcbIUScale.IUToNm( DRILL_CHART_MAX_COLUMN_WIDTH ) ) );
        col.m_Width = kiapi::common::UnpackDistance( width );
        m_columns.push_back( col );
    }

    // No columns divides by zero the next time this is rebuilt or autosized. Repeats and
    // implausible widths reach table geometry
    if( !ValidateDrillChartColumns( m_columns ) )
    {
        return false;
    }

    // The shared enum also carries mils, metres and tenths. A chart has no rendering for
    // those, so anything but inches reads back as millimetres rather than as a broken chart
    switch( chart.units() )
    {
    case kiapi::common::types::U_INCH: m_units = DRILL_CHART_UNITS::INCH; break;
    default:                           m_units = DRILL_CHART_UNITS::MM; break;
    }

    m_symbolColumn = chart.symbol_column();

    m_rowShapes.clear();

    for( const auto& [row, shapeIndex] : chart.row_shapes() )
        m_rowShapes[row] = shapeIndex;

    m_rowKeys.clear();

    for( const auto& [row, key] : chart.row_keys() )
        m_rowKeys[row] = key;

    SetPrecision( chart.precision() );
    m_showTotals = chart.show_totals();

    return true;
}


void RefreshDrillCharts( BOARD& aBoard )
{
    const uint64_t generation = aBoard.GetDrillModelGeneration();

    // Assignments are committed only once every chart has rebuilt, so a chart that throws
    // cannot leave half a profile behind
    DRILL_SYMBOL_PROFILE assigned = aBoard.GetDesignSettings().GetDrillSymbolProfile();
    int                  rebuilt = 0;

    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() != PCB_DRILL_CHART_T )
            continue;

        PCB_DRILL_CHART* chart = static_cast<PCB_DRILL_CHART*>( item );

        if( chart->GetBuiltGeneration() == generation )
            continue;

        chart->RebuildCells( aBoard, &assigned );
        rebuilt++;
    }

    if( !rebuilt )
        return;

    aBoard.GetDesignSettings().GetDrillSymbolProfile() = assigned;

}


static struct PCB_DRILL_CHART_DESC
{
    PCB_DRILL_CHART_DESC()
    {
        ENUM_MAP<DRILL_CHART_UNITS>& unitsEnum = ENUM_MAP<DRILL_CHART_UNITS>::Instance();

        if( unitsEnum.Choices().GetCount() == 0 )
        {
            unitsEnum.Map( DRILL_CHART_UNITS::MM, _HKI( "Millimeters" ) )
                    .Map( DRILL_CHART_UNITS::INCH, _HKI( "Inches" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DRILL_CHART );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, BOARD_ITEM_CONTAINER> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, PCB_TABLE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DRILL_CHART ), TYPE_HASH( PCB_TABLE ) );

        const wxString chartProps = _( "Drill Chart Properties" );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DRILL_CHART, DRILL_CHART_UNITS>(
                                     _HKI( "Units" ), &PCB_DRILL_CHART::SetUnits, &PCB_DRILL_CHART::GetUnits ),
                             chartProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_CHART, int>( _HKI( "Decimal Places" ),
                                                                 &PCB_DRILL_CHART::SetPrecision,
                                                                 &PCB_DRILL_CHART::GetPrecision ),
                             chartProps );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_CHART, bool>( _HKI( "Show Totals" ),
                                                                  &PCB_DRILL_CHART::SetShowTotals,
                                                                  &PCB_DRILL_CHART::GetShowTotals ),
                             chartProps );
    }
} _PCB_DRILL_CHART_DESC;


ENUM_TO_WXANY( DRILL_CHART_UNITS )
