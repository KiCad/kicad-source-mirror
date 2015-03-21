/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <schframe.h>
#include <sch_reference_list.h>
#include <class_library.h>
#include <class_netlist_object.h>

#include <netlist.h>
#include "netlist_exporter_orcadpcb2.h"

bool NETLIST_EXPORTER_ORCADPCB2::Write( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    (void)aNetlistOptions;      //unused
    FILE* f = NULL;
    wxString    field;
    wxString    footprint;
    int         ret = 0;        // zero now, OR in the sign bit on error
    wxString    netName;


    if( ( f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to create file '%s'" ),
                    GetChars( aOutFileName ) );
        DisplayError( NULL, msg );
        return false;
    }

    std::vector< SCH_REFERENCE > cmpList;

    ret |= fprintf( f, "( { %s created  %s }\n",
                        NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );

    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // Create netlist module section
    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_ITEM* item = path->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, path );

            if( !comp )
                break;

            item = comp;

            // Get the Component FootprintFilter and put the component in
            // cmpList if filter is present
            LIB_PART* part = m_libs->FindLibPart( comp->GetPartName() );

            if( part )
            {
                if( part->GetFootPrints().GetCount() != 0 )    // Put in list
                {
                    cmpList.push_back( SCH_REFERENCE( comp, part, *path ) );
                }
            }

            if( !comp->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = comp->GetField( FOOTPRINT )->GetText();
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );

            field = comp->GetRef( path );

            ret |= fprintf( f, " ( %s %s",
                            TO_UTF8( comp->GetPath( path ) ),
                            TO_UTF8( footprint ) );

            ret |= fprintf( f, "  %s", TO_UTF8( field ) );

            field = comp->GetField( VALUE )->GetText();
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, " %s", TO_UTF8( field ) );

            ret |= fprintf( f, "\n" );

            // Write pin list:
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];

                if( !pin )
                    continue;

                sprintPinNetName( netName, wxT( "N-%.6d" ), pin );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                netName.Replace( wxT( " " ), wxT( "_" ) );

                ret |= fprintf( f, "  ( %4.4s %s )\n", (char*) &pin->m_PinNum,
                                TO_UTF8( netName ) );
            }

            ret |= fprintf( f, " )\n" );
        }
    }

    ret |= fprintf( f, ")\n*\n" );

    fclose( f );

    m_SortedComponentPinList.clear();
    return ret >= 0;
}
