/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>.
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

#include <richio.h>
#include <string_utils.h>

#include "pcb_netlist.h"
#include "netlist_reader.h"

void LEGACY_NETLIST_READER::LoadNetlist()
{
    int state            = 0;
    bool is_comment      = false;
    COMPONENT* component = nullptr;

    while( m_lineReader->ReadLine() )
    {
        char* line = StrPurge( m_lineReader->Line() );

        if( is_comment ) // Comments in progress
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == nullptr )
                continue;

            is_comment = false;
        }

        if( *line == '{' ) // Start Comment or Pcbnew info section
        {
            is_comment = true;

            if( m_loadFootprintFilters && state == 0
              && (strncasecmp( line, "{ Allowed footprints", 20 ) == 0) )
            {
                loadFootprintFilters();
                continue;
            }

            if( ( line = strchr( line, '}' ) ) == nullptr )
                continue;
        }

        if( *line == '(' )
            state++;

        if( *line == ')' )
            state--;

        if( state == 2 )
        {
            component = loadComponent( line );
            continue;
        }

        if( state >= 3 ) // Pad descriptions are read here.
        {
            wxASSERT( component != nullptr );

            loadNet( line, component );
            state--;
        }
    }

    if( m_footprintReader )
    {
        m_footprintReader->Load( m_netlist );
    }
}


COMPONENT* LEGACY_NETLIST_READER::loadComponent( char* aText )
{
    char*    text;
    wxString msg;
    wxString footprintName;     // the footprint name read from netlist
    wxString value;             // the component value read from netlist
    wxString reference;         // the component schematic reference designator read from netlist
    wxString name;              // the name of component that was placed in the schematic
    char     line[1024];

    strncpy( line, aText, sizeof(line)-1 );
    line[sizeof(line)-1] = '\0';

    value = wxT( "~" );

    // Sample component line:   /68183921-93a5-49ac-91b0-49d05a0e1647 $noname R20 4.7K {Lib=R}

    // Read time stamp (first word)
    if( ( text = strtok( line, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse time stamp in symbol section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    KIID_PATH path( From_UTF8( text ) );

    // Read footprint name (second word)
    if( ( text = strtok( nullptr, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse footprint name in symbol section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    footprintName = From_UTF8( text );

    // The footprint name will have to be looked up in the *.cmp file.
    if( footprintName == wxT( "$noname" ) )
        footprintName = wxEmptyString;

    // Read schematic reference designator (third word)
    if( ( text = strtok( nullptr, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse reference designator in symbol section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    reference = From_UTF8( text );

    // Read schematic value (forth word)
    if( ( text = strtok( nullptr, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse value in symbol section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    value = From_UTF8( text );

    // Read component name (fifth word) {Lib=C}
    // This is an optional field (a comment), which does not always exists
    if( ( text = strtok( nullptr, " ()\t\n" ) ) != nullptr )
    {
        name = From_UTF8( text ).AfterFirst( wxChar( '=' ) ).BeforeLast( wxChar( '}' ) );
    }

    LIB_ID fpid;

    if( !footprintName.IsEmpty() )
        fpid.SetLibItemName( footprintName );

    COMPONENT* component = new COMPONENT( fpid, reference, value, path, {} );
    component->SetName( name );
    m_netlist->AddComponent( component );
    return component;
}


void LEGACY_NETLIST_READER::loadNet( char* aText, COMPONENT* aComponent )
{
    wxString msg;
    char*    p;
    char     line[256];

    strncpy( line, aText, sizeof( line ) );
    line[ sizeof(line) - 1 ] = '\0';

    if( ( p = strtok( line, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse pin name in symbol net section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    wxString pinName = From_UTF8( p );

    if( ( p = strtok( nullptr, " ()\t\n" ) ) == nullptr )
    {
        msg = _( "Cannot parse net name in symbol net section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    wxString netName = From_UTF8( p );

    if( (char) netName[0] == '?' )       // ? indicates no net connected to pin.
        netName = wxEmptyString;

    aComponent->AddNet( pinName, netName, wxEmptyString, wxEmptyString );
}


void LEGACY_NETLIST_READER::loadFootprintFilters()
{
    wxArrayString filters;
    wxString      cmpRef;
    char*         line;
    COMPONENT*    component = nullptr;     // Suppress compile warning

    while( ( line = m_lineReader->ReadLine() ) != nullptr )
    {
        if( strncasecmp( line, "$endlist", 8 ) == 0 )   // end of list for the current component
        {
            wxASSERT( component != nullptr );
            component->SetFootprintFilters( filters );
            component = nullptr;
            filters.Clear();
            continue;
        }

        if( strncasecmp( line, "$endfootprintlist", 4 ) == 0 )
            // End of this section
            return;

        if( strncasecmp( line, "$component", 10 ) == 0 ) // New component reference found
        {
            cmpRef = From_UTF8( line + 11 );
            cmpRef.Trim( true );
            cmpRef.Trim( false );

            component = m_netlist->GetComponentByReference( cmpRef );

            // Cannot happen if the netlist is valid.
            if( component == nullptr )
            {
                wxString msg;
                msg.Printf( _( "Cannot find symbol %s in footprint filter section of netlist." ),
                            cmpRef );
                THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                                   m_lineReader->Length() );
            }
        }
        else
        {
            // Add new filter to list
            wxString fp = From_UTF8( line + 1 );
            fp.Trim( false );
            fp.Trim( true );
            filters.Add( fp );
        }
    }
}

//  LocalWords:  EDA Charras pcb netlist noname cmp endlist
//  LocalWords:  endfootprintlist
