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

#include <build_version.h>
#include <confirm.h>

#include <connection_graph.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>

#include "netlist_exporter_pads.h"

bool NETLIST_EXPORTER_PADS::WriteNetlist( const wxString& aOutFileName,
                                             unsigned /* aNetlistOptions */,
                                             REPORTER& aReporter )
{
    int ret = 0;
    FILE* f = nullptr;

    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == nullptr )
    {
        wxString msg = wxString::Format( _( "Failed to create file '%s'." ), aOutFileName );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    wxString msg;
    wxString footprint;
    SCH_SYMBOL* symbol;

    ret |= fputs( "*PADS-PCB*\n", f );
    ret |= fputs( "*PART*\n", f );

    // Create netlist footprints section
    m_referencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            symbol = findNextSymbol( item, &sheetList[ i ] );

            if( !symbol )
                continue;

            if( symbol->GetExcludedFromBoard() )
                continue;

            footprint = symbol->GetFootprintFieldText( true, &sheetList[ i ], false );

            footprint = footprint.Trim( true );
            footprint = footprint.Trim( false );
            footprint.Replace( wxT( " " ), wxT( "_" ) );

            if( footprint.IsEmpty() )
            {
                // fall back to value field
                footprint = symbol->GetValue( true, &sheetList[i], false );
                footprint.Replace( wxT( " " ), wxT( "_" ) );
                footprint = footprint.Trim( true );
                footprint = footprint.Trim( false );
            }

            msg = symbol->GetRef( &sheetList[i] );
            ret |= fprintf( f, "%-16s %s\n", TO_UTF8( msg ), TO_UTF8( footprint ) );
        }
    }

    ret |= fputs( "\n", f );

    if( !writeListOfNets( f ) )
        ret = -1;   // set error

    fclose( f );

    return ret >= 0;
}


bool NETLIST_EXPORTER_PADS::writeListOfNets( FILE* f )
{
    int ret       = 0;

    wxString initialSignalLine;
    wxString netName;

    ret |= fputs( "*NET*\n", f );

    for( const auto& [ key, subgraphs ] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        netName = key.Name;

        std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>> sorted_items;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            SCH_SHEET_PATH sheet = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                    sorted_items.emplace_back( static_cast<SCH_PIN*>( item ), sheet );
            }
        }

        // Netlist ordering: Net name, then ref des, then pin name
        std::sort( sorted_items.begin(), sorted_items.end(),
                []( std::pair<SCH_PIN*, SCH_SHEET_PATH> a, std::pair<SCH_PIN*, SCH_SHEET_PATH> b )
                {
                    wxString ref_a = a.first->GetParentSymbol()->GetRef( &a.second );
                    wxString ref_b = b.first->GetParentSymbol()->GetRef( &b.second );

                    if( ref_a == ref_b )
                        return a.first->GetShownNumber() < b.first->GetShownNumber();

                    return ref_a < ref_b;
                } );

        // Some duplicates can exist, for example on multi-unit parts with duplicated
        // pins across units.  If the user connects the pins on each unit, they will
        // appear on separate subgraphs.  Remove those here:
        sorted_items.erase( std::unique( sorted_items.begin(), sorted_items.end(),
                []( std::pair<SCH_PIN*, SCH_SHEET_PATH> a, std::pair<SCH_PIN*, SCH_SHEET_PATH> b )
                {
                    wxString ref_a = a.first->GetParentSymbol()->GetRef( &a.second );
                    wxString ref_b = b.first->GetParentSymbol()->GetRef( &b.second );

                    return ref_a == ref_b && a.first->GetShownNumber() == b.first->GetShownNumber();
                } ),
                sorted_items.end() );

        std::vector<wxString> netConns;

        for( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& pair : sorted_items )
        {
            SCH_PIN*       pin   = pair.first;
            SCH_SHEET_PATH sheet = pair.second;

            wxString refText = pin->GetParentSymbol()->GetRef( &sheet );
            wxString pinText = pin->GetShownNumber();

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
                continue;

            netConns.push_back(
                    wxString::Format( "%s.%.4s", refText, pinText ) );
        }

        // format it such that there are 6 net connections per line
        // which seems to be the standard everyone follows
        if( netConns .size() > 1 )
        {
            ret |= fprintf( f, "*SIGNAL* %s\n", TO_UTF8(netName) );
            int cnt = 0;
            for( wxString& netConn : netConns )
            {
                ret |= fputs( TO_UTF8( netConn ), f );
                if( cnt != 0 && cnt % 6 == 0 )
                {
                    ret |= fputc( '\n', f );
                }
                else
                {
                    ret |= fputc( ' ', f );
                }

                cnt++;
            }

            ret |= fputc( '\n', f );
        }
    }

    ret |= fprintf( f, "*END*\n" );

    return ret >= 0;
}
