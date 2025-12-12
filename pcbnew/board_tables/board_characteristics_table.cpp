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


#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_edit_frame.h>
#include <board_statistics_report.h>


PCB_TABLE* Build_Board_Characteristics_Table( BOARD* aBoard, EDA_UNITS aDisplayUnits )
{
    BOARD_DESIGN_SETTINGS& settings = aBoard->GetDesignSettings();
    BOARD_STACKUP&         stackup  = settings.GetStackupDescriptor();
    UNITS_PROVIDER         units_provider( pcbIUScale, aDisplayUnits );

    stackup.SynchronizeWithBoard( &settings );

    BOARD_STATISTICS_DATA brd_stat_data;
    BOARD_STATISTICS_OPTIONS opts;
    ComputeBoardStatistics( aBoard, opts, brd_stat_data );

    PCB_TABLE* table = new PCB_TABLE( aBoard, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) );
    table->SetColCount( 4 );

    auto addHeaderCell =
            [&]( const wxString& text )
            {
                PCB_TABLECELL* c = new PCB_TABLECELL( table );
                c->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 2.0 ), pcbIUScale.mmToIU( 2.0 ) ) );
                c->SetTextThickness( pcbIUScale.mmToIU( 0.4 ) );
                c->SetText( text );
                c->SetColSpan( table->GetColCount() );
                table->AddCell( c );
            };

    auto addDataCell =
            [&]( const wxString& text )
            {
                PCB_TABLECELL* c = new PCB_TABLECELL( table );
                c->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
                c->SetTextThickness( pcbIUScale.mmToIU( 0.2 ) );
                c->SetText( text );
                table->AddCell( c );
            };

    addHeaderCell( _( "BOARD CHARACTERISTICS" ) );

    for( int col = 1; col < table->GetColCount(); ++col )
    {
        addHeaderCell( wxEmptyString );
        table->GetCell( 0, col )->SetColSpan( 0 );
    }

    addDataCell( _( "Copper layer count: " ) );
    addDataCell( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                      settings.GetCopperLayerCount(), false ) );

    addDataCell( _( "Board thickness: " ) );
    addDataCell( units_provider.MessageTextFromValue(  brd_stat_data.boardThickness, true ) );

    SHAPE_POLY_SET outline;
    aBoard->GetBoardPolygonOutlines( outline, false );
    BOX2I size = outline.BBox();

    addDataCell( _( "Board overall dimensions: " ) );
    addDataCell( wxString::Format( wxT( "%s x %s" ),
                                   units_provider.MessageTextFromValue( size.GetWidth(), true ),
                                   units_provider.MessageTextFromValue( size.GetHeight(), true ) ) );

    addDataCell( wxEmptyString );
    addDataCell( wxEmptyString );

    addDataCell( _( "Min track/spacing: " ) );
    addDataCell( wxString::Format( wxT( "%s / %s" ),
                                   units_provider.MessageTextFromValue( brd_stat_data.minTrackWidth, true ),
                                   units_provider.MessageTextFromValue( brd_stat_data.minClearanceTrackToTrack, true ) ) );

    double min_holeSize = brd_stat_data.minDrillSize;

    addDataCell( _( "Min hole diameter: " ) );
    addDataCell( units_provider.MessageTextFromValue( min_holeSize, true ) );

    addDataCell( _( "Copper finish: " ) );
    addDataCell( stackup.m_FinishType );

    addDataCell( _( "Impedance control: " ) );
    addDataCell( stackup.m_HasDielectricConstrains ? _( "Yes" ) : _( "No" ) );

    addDataCell( _( "Castellated pads: " ) );
    int castellated_pad_count = aBoard->GetPadWithCastellatedAttrCount();
    addDataCell( castellated_pad_count ? _( "Yes" ) : _( "No" ) );

    addDataCell( _( "Press-fit pads: " ) );
    int pressfit_pad_count = aBoard->GetPadWithPressFitAttrCount();
    addDataCell( pressfit_pad_count ? _( "Yes" ) : _( "No" ) );

    addDataCell( _( "Plated board edge: " ) );
    addDataCell( stackup.m_EdgePlating ? _( "Yes" ) : _( "No" ) );

    wxString msg;

    switch( stackup.m_EdgeConnectorConstraints )
    {
    case BS_EDGE_CONNECTOR_NONE:     msg = _( "No" );            break;
    case BS_EDGE_CONNECTOR_IN_USE:   msg = _( "Yes" );           break;
    case BS_EDGE_CONNECTOR_BEVELLED: msg = _( "Yes, Bevelled" ); break;
    }

    addDataCell( _( "Edge card connectors: " ) );
    addDataCell( msg );

    // We are building a table having 4 columns.
    // So we must have a cell count multible of 4, to have fully build row.
    // Othewise the table is really badly drawn.
    std::vector<PCB_TABLECELL*> cells_list = table->GetCells();
    int cell_to_add_cnt = cells_list.size() % table->GetColCount();

    for( int ii = 0; ii < cell_to_add_cnt; ii++ )
        addDataCell( wxEmptyString );

    table->SetStrokeExternal( false );
    table->SetStrokeHeaderSeparator( false );
    table->SetStrokeColumns( false );
    table->SetStrokeRows( false );
    table->Autosize();

    return table;
}
