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

#include <build_version.h>
#include <confirm.h>

#include <connection_graph.h>
#include <string_utils.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <fmt.h>
#include <system_error>

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

    try
    {
        fmt::print( f, "*PADS-PCB*\n" );
        fmt::print( f, "*PART*\n" );

        // Create netlist footprints section
        m_referencesAlreadyFound.Clear();

        for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                symbol = findNextSymbol( item, sheet );

                if( !symbol )
                    continue;

                if( symbol->GetExcludedFromBoard() )
                    continue;

                footprint = symbol->GetFootprintFieldText( true, &sheet, false );

                footprint = footprint.Trim( true );
                footprint = footprint.Trim( false );
                footprint.Replace( wxT( " " ), wxT( "_" ) );

                if( footprint.IsEmpty() )
                {
                    // fall back to value field
                    footprint = symbol->GetValue( true, &sheet, false );
                    footprint.Replace( wxT( " " ), wxT( "_" ) );
                    footprint = footprint.Trim( true );
                    footprint = footprint.Trim( false );
                }

                msg = symbol->GetRef( &sheet );
                fmt::print( f, "{:<16} {}\n", TO_UTF8( msg ), TO_UTF8( footprint ) );
            }
        }

        fmt::print( f, "\n" );

        if( !writeListOfNets( f ) )
            ret = -1;   // set error

        if( ferror( f ) )
            ret = -1;
    }
    catch( const std::system_error& e )
    {
        aReporter.Report( wxString::Format( _( "I/O error writing netlist: %s" ), e.what() ), RPT_SEVERITY_ERROR );
        ret = -1;
    }
    catch( const fmt::format_error& e )
    {
        aReporter.Report( wxString::Format( _( "Formatting error writing netlist: %s" ), e.what() ), RPT_SEVERITY_ERROR );
        ret = -1;
    }

    fclose( f );

    return ret >= 0;
}


bool NETLIST_EXPORTER_PADS::writeListOfNets( FILE* f )
{
    try
    {
        wxString netName;

        fmt::print( f, "*NET*\n" );

        // Collect all nets and sort them by name to ensure stable ordering
        std::vector<std::pair<wxString, std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>>>> allNets;

        for( const auto& [ key, subgraphs ] : m_schematic->ConnectionGraph()->GetNetMap() )
        {
            netName = key.Name;

            std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>> sortedItems;

            for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
            {
                SCH_SHEET_PATH sheet = subgraph->GetSheet();

                for( SCH_ITEM* item : subgraph->GetItems() )
                {
                    if( item->Type() == SCH_PIN_T )
                        sortedItems.emplace_back( static_cast<SCH_PIN*>( item ), sheet );
                }
            }

            // Netlist ordering: Net name, then ref des, then pin name (intra-net)
            std::sort( sortedItems.begin(), sortedItems.end(),
                    []( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& a, const std::pair<SCH_PIN*, SCH_SHEET_PATH>& b )
                    {
                        wxString ref_a = a.first->GetParentSymbol()->GetRef( &a.second );
                        wxString ref_b = b.first->GetParentSymbol()->GetRef( &b.second );

                        if( ref_a == ref_b )
                            return a.first->GetShownNumber() < b.first->GetShownNumber();

                        return ref_a < ref_b;
                    } );

            // Remove duplicates across subgraphs for multi-unit parts
            sortedItems.erase( std::unique( sortedItems.begin(), sortedItems.end(),
                    []( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& a, const std::pair<SCH_PIN*, SCH_SHEET_PATH>& b )
                    {
                        wxString ref_a = a.first->GetParentSymbol()->GetRef( &a.second );
                        wxString ref_b = b.first->GetParentSymbol()->GetRef( &b.second );

                        return ref_a == ref_b && a.first->GetShownNumber() == b.first->GetShownNumber();
                    } ),
                    sortedItems.end() );

            allNets.emplace_back( netName, std::move( sortedItems ) );
        }

        // Sort nets by name (inter-net ordering) for deterministic output
        std::sort( allNets.begin(), allNets.end(),
                   []( const auto& a, const auto& b )
                   {
                       return a.first < b.first;
                   } );

        for( const auto& [sortedNetName, sorted_items] : allNets )
        {
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

                netConns.push_back( wxString::Format( "%s.%.4s", refText, pinText ) );
            }

            // format it such that there are 6 net connections per line
            // which seems to be the standard everyone follows
            if( netConns .size() > 1 )
            {
                fmt::print( f, "*SIGNAL* {}\n", TO_UTF8(sortedNetName) );
                int cnt = 0;

                for( wxString& netConn : netConns )
                {
                    fmt::print( f, "{}", TO_UTF8( netConn ) );

                    if( cnt != 0 && cnt % 6 == 0 )
                        fmt::print( f, "\n" );
                    else
                        fmt::print( f, " " );

                    cnt++;
                }

                fmt::print( f, "\n" );
            }
        }

        fmt::print( f, "*END*\n" );

        return ferror( f ) == 0;
    }
    catch( const std::system_error& )
    {
        return false;
    }
    catch( const fmt::format_error& )
    {
        return false;
    }
}
