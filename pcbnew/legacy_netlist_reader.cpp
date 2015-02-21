/**
 * @file pcbnew/legacy_netlist_reader.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>.
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <richio.h>
#include <kicad_string.h>

#include <pcb_netlist.h>
#include <netlist_reader.h>


void LEGACY_NETLIST_READER::LoadNetlist() throw ( IO_ERROR, PARSE_ERROR, boost::bad_pointer )
{
    int state            = 0;
    bool is_comment      = false;
    COMPONENT* component = NULL;

    while( m_lineReader->ReadLine() )
    {
        char* line = StrPurge( m_lineReader->Line() );

        if( is_comment ) // Comments in progress
        {
            // Test for end of the current comment
            if( ( line = strchr( line, '}' ) ) == NULL )
                continue;

            is_comment = false;
        }

        if( *line == '{' ) // Start Comment or Pcbnew info section
        {
            is_comment = true;

            if( m_loadFootprintFilters && state == 0
              && (strnicmp( line, "{ Allowed footprints", 20 ) == 0) )
            {
                loadFootprintFilters();
                continue;
            }

            if( ( line = strchr( line, '}' ) ) == NULL )
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
            wxASSERT( component != NULL );

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
    throw( PARSE_ERROR, boost::bad_pointer )
{
    char*    text;
    wxString msg;
    wxString timeStamp;         // the full time stamp read from netlist
    wxString footprintName;     // the footprint name read from netlist
    wxString value;             // the component value read from netlist
    wxString reference;         // the component schematic reference designator read from netlist
    wxString name;              // the name of component that was placed in the schematic
    char     line[1024];

    strcpy( line, aText );

    value = wxT( "~" );

    // Sample component line:   /40C08647 $noname R20 4.7K {Lib=R}

    // Read time stamp (first word)
    if( ( text = strtok( line, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse time stamp in component section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    timeStamp = FROM_UTF8( text );

    // Read footprint name (second word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse footprint name in component section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    footprintName = FROM_UTF8( text );

    // The footprint name will have to be looked up in the *.cmp file.
    if( footprintName == wxT( "$noname" ) )
        footprintName = wxEmptyString;

    // Read schematic reference designator (third word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse reference designator in component section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    reference = FROM_UTF8( text );

    // Read schematic value (forth word)
    if( ( text = strtok( NULL, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse value in component section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), aText, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    value = FROM_UTF8( text );

    // Read component name (fifth word) {Lib=C}
    // This is an optional field (a comment), which does not always exists
    if( ( text = strtok( NULL, " ()\t\n" ) ) != NULL )
    {
        name = FROM_UTF8( text ).AfterFirst( wxChar( '=' ) ).BeforeLast( wxChar( '}' ) );
    }

    FPID fpid;

    if( !footprintName.IsEmpty() )
        fpid.SetFootprintName( footprintName );

    COMPONENT* component = new COMPONENT( fpid, reference, value, timeStamp );
    component->SetName( name );
    m_netlist->AddComponent( component );
    return component;
}


void LEGACY_NETLIST_READER::loadNet( char* aText, COMPONENT* aComponent ) throw( PARSE_ERROR )
{
    wxString msg;
    char*    p;
    char     line[256];

    strncpy( line, aText, sizeof( line ) );
    line[ sizeof(line) - 1 ] = '\0';

    if( ( p = strtok( line, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse pin name in component net section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    wxString pinName = FROM_UTF8( p );

    if( ( p = strtok( NULL, " ()\t\n" ) ) == NULL )
    {
        msg = _( "Cannot parse net name in component net section of netlist." );
        THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                           m_lineReader->Length() );
    }

    wxString netName = FROM_UTF8( p );

    if( (char) netName[0] == '?' )       // ? indicates no net connected to pin.
        netName = wxEmptyString;

    aComponent->AddNet( pinName, netName );
}


void LEGACY_NETLIST_READER::loadFootprintFilters() throw( IO_ERROR, PARSE_ERROR )
{
    wxArrayString filters;
    wxString      cmpRef;
    char*         line;
    COMPONENT*    component = NULL;     // Suppress compil warning

    while( ( line = m_lineReader->ReadLine() ) != NULL )
    {
        if( strnicmp( line, "$endlist", 8 ) == 0 )   // end of list for the current component
        {
            wxASSERT( component != NULL );
            component->SetFootprintFilters( filters );
            component = NULL;
            filters.Clear();
            continue;
        }

        if( strnicmp( line, "$endfootprintlist", 4 ) == 0 )
            // End of this section
            return;

        if( strnicmp( line, "$component", 10 ) == 0 ) // New component reference found
        {
            cmpRef = FROM_UTF8( line + 11 );
            cmpRef.Trim( true );
            cmpRef.Trim( false );

            component = m_netlist->GetComponentByReference( cmpRef );

            // Cannot happen if the netlist is valid.
            if( component == NULL )
            {
                wxString msg;
                msg.Printf( _( "Cannot find component \'%s\' in footprint filter section "
                               "of netlist." ), GetChars( cmpRef ) );
                THROW_PARSE_ERROR( msg, m_lineReader->GetSource(), line, m_lineReader->LineNumber(),
                                   m_lineReader->Length() );
            }
        }
        else
        {
            // Add new filter to list
            wxString fp = FROM_UTF8( line + 1 );
            fp.Trim( false );
            fp.Trim( true );
            filters.Add( fp );
        }
    }
}
