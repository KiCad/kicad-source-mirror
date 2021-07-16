/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kicad_string.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>

#include "netlist_exporter_cadstar.h"

/* Generate CADSTAR net list. */
static wxString StartLine( wxT( "." ) );

bool NETLIST_EXPORTER_CADSTAR::WriteNetlist( const wxString& aOutFileName,
                                             unsigned aNetlistOptions )
{
    (void)aNetlistOptions;      //unused
    int ret = 0;
    FILE* f = nullptr;

    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == nullptr )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'." ), aOutFileName );
        DisplayError( nullptr, msg );
        return false;
    }

    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString footprint;
    SCH_SYMBOL* symbol;
    wxString title = wxT( "Eeschema " ) + GetBuildVersion();

    ret |= fprintf( f, "%sHEA\n", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "%sTIM %s\n", TO_UTF8( StartLine ), TO_UTF8( DateAndTime() ) );
    ret |= fprintf( f, "%sAPP ", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "\"%s\"\n", TO_UTF8( title ) );
    ret |= fprintf( f, ".TYP FULL\n\n" );

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

            if( !symbol->GetField( FOOTPRINT_FIELD )->IsVoid() )
                footprint = symbol->GetField( FOOTPRINT_FIELD )->GetShownText();
            else
                footprint = "$noname";

            msg = symbol->GetRef( &sheetList[i] );
            ret |= fprintf( f, "%s     ", TO_UTF8( StartCmpDesc ) );
            ret |= fprintf( f, "%s", TO_UTF8( msg ) );

            msg = symbol->GetValue( &sheetList[i], true );
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
    wxString netName;

    for( const auto& it : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        auto subgraphs = it.second;

        netName.Printf( wxT( "\"%s\"" ), it.first.first );

        std::vector<std::pair<SCH_PIN*, SCH_SHEET_PATH>> sorted_items;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            SCH_SHEET_PATH sheet = subgraph->m_sheet;

            for( SCH_ITEM* item : subgraph->m_items )
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
