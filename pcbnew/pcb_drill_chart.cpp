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
#include <properties/property_mgr.h>
#include <widgets/msgpanel.h>
#include <api/api_enums.h>
#include <api/api_generated_table_utils.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>


DRILL_PROFILE_PENDING::DRILL_PROFILE_PENDING( BOARD& aBoard ) :
        m_Profile( aBoard.GetDesignSettings().GetDrillSymbolProfile() )
{
}


void DRILL_PROFILE_PENDING::Commit( BOARD& aBoard )
{
    aBoard.GetDesignSettings().GetDrillSymbolProfile() = m_Profile;
}


PCB_DRILL_CHART::PCB_DRILL_CHART( BOARD_ITEM* aParent ) :
        PCB_GENERATED_TABLE( aParent, PCB_DRILL_CHART_T, pcbIUScale.mmToIU( 0.1 ) ),
        m_showTotals( true ),
        m_symbolColumn( -1 ),
        m_builtGeneration( 0 )
{
    ApplyTemplate( DRILL_CHART_TEMPLATE::MakeDefault() );
}


PCB_DRILL_CHART::PCB_DRILL_CHART( const PCB_DRILL_CHART& aOther ) :
        PCB_GENERATED_TABLE( aOther ),
        m_filter( aOther.m_filter ),
        m_showTotals( aOther.m_showTotals ),
        m_rowShapes( aOther.m_rowShapes ),
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

    PCB_GENERATED_TABLE::swapData( aImage );

    PCB_DRILL_CHART* other = static_cast<PCB_DRILL_CHART*>( aImage );

    std::swap( m_filter, other->m_filter );
    std::swap( m_showTotals, other->m_showTotals );
    std::swap( m_builtGeneration, other->m_builtGeneration );
    std::swap( m_rowShapes, other->m_rowShapes );
    std::swap( m_symbolColumn, other->m_symbolColumn );
}


DRILL_CHART_COLUMN_ID PCB_DRILL_CHART::columnId( int aCol ) const
{
    return static_cast<DRILL_CHART_COLUMN_ID>( m_columns[aCol].m_Id );
}


std::vector<DRILL_CHART_GROUP> PCB_DRILL_CHART::buildGroups( const BOARD&                aBoard,
                                                             const DRILL_SYMBOL_PROFILE& aProfile ) const
{
    DRILL_CHART_ROW_SPEC spec;
    spec.m_Filter = m_filter;

    // Every span on the board. What shares a row is the profile's grouping to decide
    DRILL_CHART_MODEL model( aProfile );
    model.Build( aBoard, EnumerateDrillSpans( aBoard ), spec );

    return model.Groups();
}


bool PCB_DRILL_CHART::IsStale( const BOARD& aBoard ) const
{
    return m_builtGeneration != aBoard.GetDrillModelGeneration();
}


bool PCB_DRILL_CHART::IsDataRow( int aRow ) const
{
    if( !m_rowKeys.empty() )
        return PCB_GENERATED_TABLE::IsDataRow( aRow );

    // A chart written before the keys were recorded still has to answer this, and its rows are
    // laid out the way RebuildCells lays them out
    const int firstDataRow = 1;
    const int lastDataRow = GetRowCount() - 1 - ( m_showTotals ? 1 : 0 );

    return aRow >= firstDataRow && aRow <= lastDataRow;
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
    return m_filter == other.m_filter && m_showTotals == other.m_showTotals && m_rowShapes == other.m_rowShapes
           && m_symbolColumn == other.m_symbolColumn && generatedEquals( other );
}


namespace
{

const wxString NO_VALUE( wxS( "\u2014" ) );


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


wxString symbolText( const DRILL_CHART_GROUP& aGroup, GENERATED_TABLE_UNITS aUnits, int aPrecision )
{
    switch( aGroup.m_Symbol.m_MarkMode )
    {
    case DRILL_MARK_MODE::LETTER:
        return aGroup.m_Symbol.m_Letter;

    case DRILL_MARK_MODE::SIZE_TEXT:
        return FormatGeneratedTableLength( aGroup.m_Diameter, aUnits, aPrecision );

    case DRILL_MARK_MODE::SHAPE:
    default:
        // The shape itself is drawn by the painter. The cell carries no text
        return wxEmptyString;
    }
}

} // namespace


std::unique_ptr<GENERATED_TABLE_CONTENT> PCB_DRILL_CHART::generate( const BOARD&             aBoard,
                                                                    GENERATED_TABLE_REFRESH* aRefresh ) const
{
    // A refresh can carry grouping the board has not been given yet, as the properties dialog does
    std::vector<DRILL_CHART_GROUP> groups =
            buildGroups( aBoard, aRefresh ? aRefresh->Pending<DRILL_PROFILE_PENDING>().m_Profile
                                          : aBoard.GetDesignSettings().GetDrillSymbolProfile() );

    if( aRefresh )
    {
        // The refresh's copy, so a cancelled placement leaves nothing behind and a batch
        // rebuild accumulates instead of keeping only the last chart's
        AssignDrillSymbols( groups, aRefresh->Pending<DRILL_PROFILE_PENDING>().m_Profile );
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

    auto content = std::make_unique<DRILL_CHART_CONTENT>();
    content->m_Rows.reserve( groups.size() );
    content->m_RowShapes.reserve( groups.size() );

    const int cols = static_cast<int>( m_columns.size() );

    for( int col = 0; col < cols; ++col )
    {
        if( columnId( col ) == DRILL_CHART_COLUMN_ID::SYMBOL )
            content->m_SymbolColumn = col;
    }

    for( const DRILL_CHART_GROUP& group : groups )
    {
        GENERATED_TABLE_ROW& row = content->m_Rows.emplace_back();
        row.m_Key = group.m_Key;
        row.m_Cells.reserve( cols );

        if( group.m_Symbol.m_MarkMode == DRILL_MARK_MODE::SHAPE )
            content->m_RowShapes.emplace_back( group.m_Symbol.m_ShapeIndex );
        else
            content->m_RowShapes.emplace_back();

        for( int col = 0; col < cols; ++col )
        {
            wxString text;

            switch( columnId( col ) )
            {
            case DRILL_CHART_COLUMN_ID::SYMBOL:
                text = symbolText( group, m_units, m_precision );
                break;

            case DRILL_CHART_COLUMN_ID::DRILL_DIAMETER:
                text = FormatGeneratedTableLength( group.m_Diameter, m_units, m_precision );
                break;

            case DRILL_CHART_COLUMN_ID::SLOT_SIZE:
                text = group.m_IsSlot
                               ? wxString::Format( wxT( "%s x %s" ),
                                                   FormatGeneratedTableLength( group.m_SizeXY.x, m_units,
                                                                               m_precision ),
                                                   FormatGeneratedTableLength( group.m_SizeXY.y, m_units,
                                                                               m_precision ) )
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
                               ? FormatGeneratedTableLength( *group.m_StubLength, m_units, m_precision )
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

            row.m_Cells.push_back( std::move( text ) );
        }
    }

    if( m_showTotals )
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

        content->m_Trailer.push_back( { wxString::Format( wxT( "%d OPS / %zu SITES" ), operations, sites.size() ) } );
    }

    return content;
}


void PCB_DRILL_CHART::onRebuilt( const BOARD& aBoard, const GENERATED_TABLE_CONTENT& aContent )
{
    const DRILL_CHART_CONTENT& content = static_cast<const DRILL_CHART_CONTENT&>( aContent );

    m_rowShapes.clear();

    for( size_t ii = 0; ii < content.m_RowShapes.size(); ++ii )
    {
        if( content.m_RowShapes[ii] )
            m_rowShapes[1 + static_cast<int>( ii )] = *content.m_RowShapes[ii];
    }

    m_symbolColumn = content.m_SymbolColumn;
    m_builtGeneration = aBoard.GetDrillModelGeneration();
}


void PCB_DRILL_CHART::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board;
    types::DrillChart chart;

    PackGeneratedTable( *this, chart );

    types::DrillChartFilter* filter = chart.mutable_filter();
    filter->set_plated( m_filter.m_Plated );
    filter->set_non_plated( m_filter.m_NonPlated );
    filter->set_vias( m_filter.m_Vias );
    filter->set_slots( m_filter.m_Slots );
    filter->set_backdrills( m_filter.m_Backdrills );
    filter->set_castellated( m_filter.m_Castellated );

    chart.set_show_totals( m_showTotals );

    chart.set_symbol_column( m_symbolColumn );

    for( const auto& [row, shapeIndex] : m_rowShapes )
        ( *chart.mutable_row_shapes() )[row] = shapeIndex;

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

    if( !UnpackGeneratedTable( chart, *this ) )
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

    m_symbolColumn = chart.symbol_column();

    m_rowShapes.clear();

    for( const auto& [row, shapeIndex] : chart.row_shapes() )
        m_rowShapes[row] = shapeIndex;

    m_showTotals = chart.show_totals();

    return true;
}


static struct PCB_DRILL_CHART_DESC
{
    PCB_DRILL_CHART_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DRILL_CHART );

        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, BOARD_ITEM_CONTAINER> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, PCB_TABLE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DRILL_CHART, PCB_GENERATED_TABLE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DRILL_CHART ), TYPE_HASH( PCB_GENERATED_TABLE ) );

        const wxString chartProps = _( "Drill Chart Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_DRILL_CHART, bool>( _HKI( "Show Totals" ),
                                                                  &PCB_DRILL_CHART::SetShowTotals,
                                                                  &PCB_DRILL_CHART::GetShowTotals ),
                             chartProps );
    }
} _PCB_DRILL_CHART_DESC;
