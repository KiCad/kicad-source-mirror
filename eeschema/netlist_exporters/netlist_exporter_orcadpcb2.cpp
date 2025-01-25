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

#include <algorithm>
#include <vector>

#include <confirm.h>
#include <refdes_utils.h>

#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <string_utils.h>

#include <netlist.h>
#include "netlist_exporter_orcadpcb2.h"


bool NETLIST_EXPORTER_ORCADPCB2::WriteNetlist( const wxString& aOutFileName,
                                               unsigned /* aNetlistOptions */,
                                               REPORTER& aReporter )
{
    FILE* f = nullptr;
    wxString    field;
    wxString    footprint;
    int         ret = 0;        // zero now, OR in the sign bit on error
    wxString    netName;


    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == nullptr )
    {
        wxString msg = wxString::Format( _( "Failed to create file '%s'." ), aOutFileName );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    std::vector< SCH_REFERENCE > cmpList;

    ret |= fprintf( f, "( { %s created  %s }\n",
                        NETLIST_HEAD_STRING, TO_UTF8( GetISO8601CurrentDateTime() ) );

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
            SCH_SYMBOL* symbol = findNextSymbol( item, sheet );

            if( !symbol )
                continue;

            if( symbol->GetExcludedFromBoard() )
                continue;

            std::vector<PIN_INFO> pins = CreatePinList( symbol, sheet, true );

            if( symbol->GetLibSymbolRef()
                  && symbol->GetLibSymbolRef()->GetFPFilters().GetCount() != 0  )
            {
                cmpList.push_back( SCH_REFERENCE( symbol, sheet ) );
            }

            footprint = symbol->GetFootprintFieldText( true, &sheet, false );
            footprint.Replace( wxT( " " ), wxT( "_" ) );

            if( footprint.IsEmpty() )
                footprint = wxT( "$noname" );

            ret |= fprintf( f, " ( %s %s",
                            TO_UTF8( sheet.PathAsString() + symbol->m_Uuid.AsString() ),
                            TO_UTF8( footprint ) );

            field = symbol->GetRef( &sheet );

            ret |= fprintf( f, "  %s", TO_UTF8( field ) );

            field = symbol->GetValue( true, &sheet, false );
            field.Replace( wxT( " " ), wxT( "_" ) );

            ret |= fprintf( f, " %s", TO_UTF8( field ) );

            ret |= fprintf( f, "\n" );

            // Write pin list:
            for( const PIN_INFO& pin : pins )
            {
                if( pin.num.IsEmpty() )     // Erased pin in list
                    continue;

                netName = pin.netName;
                netName.Replace( wxT( " " ), wxT( "_" ) );

                ret |= fprintf( f, "  ( %4.4s %s )\n", TO_UTF8( pin.num ), TO_UTF8( netName ) );
            }

            ret |= fprintf( f, " )\n" );
        }
    }

    ret |= fprintf( f, ")\n*\n" );

    fclose( f );

    return ret >= 0;
}
