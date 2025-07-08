/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include "netlist_exporter_cadstar.h"

/* Generate CADSTAR net list. */
static wxString StartLine( wxT( "." ) );

bool NETLIST_EXPORTER_CADSTAR::WriteNetlist( const wxString& aOutFileName,
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

    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString footprint;
    SCH_SYMBOL* symbol;
    wxString title = wxT( "Eeschema " ) + GetBuildVersion();

    ret |= fprintf( f, "%sHEA\n", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "%sTIM %s\n", TO_UTF8( StartLine ), TO_UTF8( GetISO8601CurrentDateTime() ) );
    ret |= fprintf( f, "%sAPP ", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "\"%s\"\n", TO_UTF8( title ) );
    ret |= fprintf( f, ".TYP FULL\n\n" );

    // Create netlist footprints section
    m_referencesAlreadyFound.Clear();

    for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
    {
        // The rtree returns items in a non-deterministic order (platform-dependent)
        // Therefore we need to sort them before outputting to ensure file stability for version
        // control and QA comparisons
        std::vector<EDA_ITEM*> sheetItems;

        for( EDA_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            sheetItems.push_back( item );

        auto pred = []( const EDA_ITEM* item1, const EDA_ITEM* item2 )
        {
            return item1->m_Uuid < item2->m_Uuid;
        };

        std::sort( sheetItems.begin(), sheetItems.end(), pred );

        // Process symbol attributes
        for( EDA_ITEM* item : sheetItems )
        {
            symbol = findNextSymbol( item, sheet );

            if( !symbol )
                continue;

            if( symbol->GetExcludedFromBoard() )
                continue;

            footprint = symbol->GetFootprintFieldText( true, &sheet, false );

            if( footprint.IsEmpty() )
                footprint = "$noname";

            msg = symbol->GetRef( &sheet );
            ret |= fprintf( f, "%s     ", TO_UTF8( StartCmpDesc ) );
            ret |= fprintf( f, "%s", TO_UTF8( msg ) );

            msg = symbol->GetValue( true, &sheet, false );
            msg.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, "     \"%s\"", TO_UTF8( msg ) );
            ret |= fprintf( f, "     \"%s\"", TO_UTF8( footprint ) );
            ret |= fprintf( f, "\n" );
        }
    }

    ret |= fprintf( f, "\n" );

    if( ! writeListOfNets( f ) )
        ret = -1;   // set error

    ret |= fprintf( f, "\n%sEND\n", TO_UTF8( StartLine ) );

    fclose( f );

    return ret >= 0;
}


bool NETLIST_EXPORTER_CADSTAR::writeListOfNets( FILE* f )
{
    int ret       = 0;
    int print_ter = 0;

    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString InitNetDescLine;

    std::vector<std::pair<wxString, std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>>>> all_nets;

    for( const auto& [ key, subgraphs ] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        wxString netName;
        netName.Printf( wxT( "\"%s\"" ), key.Name );

        all_nets.emplace_back( netName, std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>>{} );
        std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>>& sorted_items = all_nets.back().second;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            SCH_SHEET_PATH sheet = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                    sorted_items.emplace_back( static_cast<SCH_PIN*>( item ), sheet );
            }
        }

        // Intra-net ordering: Ref des, then pin name
        std::sort( sorted_items.begin(), sorted_items.end(),
                []( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& a, const std::pair<SCH_PIN*, SCH_SHEET_PATH>& b )
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
                []( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& a, const std::pair<SCH_PIN*, SCH_SHEET_PATH>& b )
                {
                    wxString ref_a = a.first->GetParentSymbol()->GetRef( &a.second );
                    wxString ref_b = b.first->GetParentSymbol()->GetRef( &b.second );

                    return ref_a == ref_b && a.first->GetShownNumber() == b.first->GetShownNumber();
                } ),
                sorted_items.end() );
    }

    // Inter-net ordering by net name
    std::sort( all_nets.begin(), all_nets.end(),
               []( const auto& a, const auto& b )
               {
                   return a.first < b.first;
               } );

    for( const auto& [netName, sorted_items] : all_nets )
    {
        print_ter = 0;

        for( const std::pair<SCH_PIN*, SCH_SHEET_PATH>& pair : sorted_items )
        {
            SCH_PIN*       pin   = pair.first;
            SCH_SHEET_PATH sheet = pair.second;

            wxString refText = pin->GetParentSymbol()->GetRef( &sheet );
            wxString pinText = pin->GetShownNumber();

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
                continue;

            switch( print_ter )
            {
            case 0:
                InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                        InitNetDesc,
                                        refText,
                                        pinText,
                                        netName );
                print_ter++;
                break;

            case 1:
                ret |= fprintf( f, "%s\n", TO_UTF8( InitNetDescLine ) );
                ret |= fprintf( f, "%s       %s   %.4s\n",
                                TO_UTF8( StartNetDesc ),
                                TO_UTF8( refText ),
                                TO_UTF8( pinText ) );
                print_ter++;
                break;

            default:
                ret |= fprintf( f, "            %s   %.4s\n",
                                TO_UTF8( refText ),
                                TO_UTF8( pinText ) );
                break;
            }
        }
    }

    return ret >= 0;
}
