/**
 * @file netlist_reader.cpp
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



#include <kicad_string.h>
#include <reporter.h>

#include <pcb_netlist.h>
#include <netlist_reader.h>
#include <class_module.h>

#include <wx/regex.h>


NETLIST_READER::~NETLIST_READER()
{
    if( m_lineReader )
    {
        delete m_lineReader;
        m_lineReader = NULL;
    }

    if( m_footprintReader )
    {
        delete m_footprintReader;
        m_footprintReader = NULL;
    }
}


NETLIST_READER::NETLIST_FILE_T NETLIST_READER::GuessNetlistFileType( LINE_READER* aLineReader )
{
    // Orcad Pcb2 netlist format starts by "( {", followed by an unknown comment,
    // depending on the tool which created the file
    wxRegEx reOrcad( wxT( "(?i)[ ]*\\([ \t]+{+" ), wxRE_ADVANCED );
    wxASSERT( reOrcad.IsValid() );
    // Our legacy netlist format starts by "# EESchema Netlist "
    wxRegEx reLegacy( wxT( "(?i)#[ \t]+EESchema[ \t]+Netlist[ \t]+" ), wxRE_ADVANCED );
    wxASSERT( reLegacy.IsValid() );
    // Our new netlist format starts by "(export (version "
    wxRegEx reKicad( wxT( "[ ]*\\(export[ ]+" ), wxRE_ADVANCED );
    wxASSERT( reKicad.IsValid() );

    wxString line;

    while( aLineReader->ReadLine() )
    {
        line = FROM_UTF8( aLineReader->Line() );

        if( reLegacy.Matches( line ) )
            return LEGACY;
        else if( reKicad.Matches( line ) )
            return KICAD;
        else if( reOrcad.Matches( line ) )
            return ORCAD;
    }

    return UNKNOWN;
}


NETLIST_READER* NETLIST_READER::GetNetlistReader( NETLIST*        aNetlist,
                                                  const wxString& aNetlistFileName,
                                                  const wxString& aCompFootprintFileName )
    throw( IO_ERROR )
{
    wxASSERT( aNetlist != NULL );

    FILE* file = wxFopen( aNetlistFileName, wxT( "rt" ) );

    if( file == NULL )
    {
        wxString msg;
        msg.Printf( _( "Cannot open file %s for reading." ), GetChars( aNetlistFileName ) );
        THROW_IO_ERROR( msg );
    }

    FILE_LINE_READER* reader = new FILE_LINE_READER( file, aNetlistFileName );
    std::auto_ptr< FILE_LINE_READER > r( reader );

    NETLIST_FILE_T type = GuessNetlistFileType( reader );
    reader->Rewind();

    // The component footprint link reader is NULL if no file name was specified.
    CMP_READER* cmpFileReader = NULL;

    if( !aCompFootprintFileName.IsEmpty() )
    {
        cmpFileReader = new CMP_READER( new FILE_LINE_READER( aCompFootprintFileName ) );
    }

    switch( type )
    {
    case LEGACY:
    case ORCAD:
        return new LEGACY_NETLIST_READER( r.release(), aNetlist, cmpFileReader );

    case KICAD:
        return new KICAD_NETLIST_READER( r.release(), aNetlist, cmpFileReader );

    default:    // Unrecognized format:
        break;

    }

    return NULL;
}


bool CMP_READER::Load( NETLIST* aNetlist ) throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( aNetlist != NULL,true, wxT( "No netlist passed to CMP_READER::Load()" ) );

    wxString reference;    // Stores value read from line like Reference = BUS1;
    wxString timestamp;    // Stores value read from line like TimeStamp = /32307DE2/AA450F67;
    wxString footprint;    // Stores value read from line like IdModule  = CP6;
    wxString buffer;
    wxString value;

    bool ok = true;

    while( m_lineReader->ReadLine() )
    {
        buffer = FROM_UTF8( m_lineReader->Line() );

        if( !buffer.StartsWith( wxT( "BeginCmp" ) ) )
            continue;

        // Begin component description.
        reference.Empty();
        footprint.Empty();
        timestamp.Empty();

        while( m_lineReader->ReadLine() )
        {
            buffer = FROM_UTF8( m_lineReader->Line() );

            if( buffer.StartsWith( wxT( "EndCmp" ) ) )
                break;

            // store string value, stored between '=' and ';' delimiters.
            value = buffer.AfterFirst( '=' );
            value = value.BeforeLast( ';' );
            value.Trim( true );
            value.Trim( false );

            if( buffer.StartsWith( wxT( "Reference" ) ) )
            {
                reference = value;
                continue;
            }

            if( buffer.StartsWith( wxT( "IdModule  =" ) ) )
            {
                footprint = value;
                continue;
            }

            if( buffer.StartsWith( wxT( "TimeStamp =" ) ) )
            {
                timestamp = value;
                continue;
            }
        }

        // Find the corresponding item in component list:
        COMPONENT* component = aNetlist->GetComponentByReference( reference );

        // The corresponding component could no longer existing in the netlist.  This
        // can happed when it is removed from schematic and still exists in footprint
        // assignment list.  This is an usual case during the life of a design.
        if( component )
        {
            FPID fpid;

            if( !footprint.IsEmpty() && fpid.Parse( footprint ) >= 0 )
            {
                wxString error;
                error.Printf( _( "invalid PFID in\nfile: <%s>\nline: %d" ),
                              GetChars( m_lineReader->GetSource() ),
                              m_lineReader->LineNumber() );

                THROW_IO_ERROR( error );
            }

            // For checking purpose, store the existing FPID (if any) in the alternate fpid copy
            // if this existing FPID differs from the FPID read from the .cmp file.
            // Cvpcb can ask for user to chose the right FPID.
            // It happens if the FPIT was modified outside CvPcb.
            if( fpid != component->GetFPID() && !component->GetFPID().empty() )
                component->SetAltFPID( component->GetFPID() );

            component->SetFPID( fpid );
        }
        else
        {
            ok = false;     // can be used to display a warning in Pcbnew.
        }
    }

    return ok;
}
