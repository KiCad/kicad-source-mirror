/**
 * @file pcbnew/netlist_reader_common.cpp
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

#include <netlist_reader.h>
#include <class_module.h>

#include <wx/regex.h>


#if defined(DEBUG)
/**
 * Function NestedSpace
 * outputs nested space for pretty indenting.
 * @param aNestLevel The nest count
 * @param aReporter A reference to a #REPORTER object where to output.
 * @return REPORTER& for continuation.
 **/
static REPORTER& NestedSpace( int aNestLevel, REPORTER& aReporter )
{
    for( int i = 0;  i < aNestLevel;  ++i )
        aReporter.Report( wxT( "  " ) );

    return aReporter;
}


void COMPONENT_NET::Show( int aNestLevel, REPORTER& aReporter )
{
    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( wxString::Format( wxT( "<pin_name=%s net_name=%s>\n" ),
                                        GetChars( m_pinName ), GetChars( m_netName ) ) );
}
#endif


void COMPONENT::SetModule( MODULE* aModule )
{
    m_footprint.reset( aModule );

    if( aModule == NULL )
        return;

    aModule->SetReference( m_reference );
    aModule->SetValue( m_value );
    aModule->SetLibRef( m_footprintName );
    aModule->SetPath( m_timeStamp );
}


COMPONENT_NET COMPONENT::m_emptyNet;


const COMPONENT_NET& COMPONENT::GetNet( const wxString& aPinName )
{
    for( unsigned i = 0;  i < m_nets.size();  i++ )
    {
        if( m_nets[i].GetPinName() == aPinName )
            return m_nets[i];
    }

    return m_emptyNet;
}


#if defined(DEBUG)
void COMPONENT::Show( int aNestLevel, REPORTER& aReporter )
{
    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( wxT( "<component>\n" ) );
    NestedSpace( aNestLevel+1, aReporter );
    aReporter.Report( wxString::Format( wxT( "<ref=%s value=%s name=%s library=%s footprint=%s "
                                             "footprint-lib=%s timestamp=%s>\n" ),
                                        GetChars( m_reference ), GetChars( m_value ),
                                        GetChars( m_name ), GetChars( m_library ),
                                        GetChars( m_footprintName ), GetChars( m_footprintLib ),
                                        GetChars( m_timeStamp ) ) );

    if( !m_footprintFilters.IsEmpty() )
    {
        NestedSpace( aNestLevel+1, aReporter );
        aReporter.Report( wxT( "<fp_filters>\n" ) );

        for( unsigned i = 0;  i < m_footprintFilters.GetCount();  i++ )
        {
            NestedSpace( aNestLevel+2, aReporter );
            aReporter.Report( wxString::Format( wxT( "<%s>\n" ),
                                                GetChars( m_footprintFilters[i] ) ) );
        }

        NestedSpace( aNestLevel+1, aReporter );
        aReporter.Report( wxT( "</fp_filters>\n" ) );
    }

    if( !m_nets.empty() )
    {
        NestedSpace( aNestLevel+1, aReporter );
        aReporter.Report( wxT( "<nets>\n" ) );

        for( unsigned i = 0;  i < m_nets.size();  i++ )
            m_nets[i].Show( aNestLevel+3, aReporter );

        NestedSpace( aNestLevel+1, aReporter );
        aReporter.Report( "</nets>\n" );
    }

    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( "</component>\n" );
}
#endif


void NETLIST::AddComponent( COMPONENT* aComponent )
{
    m_components.push_back( aComponent );
}


COMPONENT* NETLIST::GetComponentByReference( const wxString& aReference )
{
    COMPONENT* component = NULL;

    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( m_components[i].GetReference() == aReference )
        {
            component = &m_components[i];
            break;
        }
    }

    return component;
}


COMPONENT* NETLIST::GetComponentByTimeStamp( const wxString& aTimeStamp )
{
    COMPONENT* component = NULL;

    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( m_components[i].GetTimeStamp() == aTimeStamp )
        {
            component = &m_components[i];
            break;
        }
    }

    return component;
}


/**
 * Function ByFootprintName
 * is a helper function used to sort the component list used by loadNewModules.
 */
static bool ByFootprintName( const COMPONENT& ref, const COMPONENT& cmp )
{
    return ref.GetFootprintName().CmpNoCase( cmp.GetFootprintName() ) > 0;
}


void NETLIST::SortByFootprintName()
{
    m_components.sort( ByFootprintName );
}


/**
 * Operator <
 * compares two #COMPONENT objects by reference designator.
 */
bool operator < ( const COMPONENT& item1, const COMPONENT& item2 )
{
    return StrNumCmp( item1.GetReference(), item2.GetReference(), INT_MAX, true ) < 0;
}


void NETLIST::SortByReference()
{
    m_components.sort();
}


bool NETLIST::AnyFootprintsLinked() const
{
    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( !m_components[i].GetFootprintName().IsEmpty() )
            return true;
    }

    return false;
}


bool NETLIST::AllFootprintsLinked() const
{
    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( m_components[i].GetFootprintName().IsEmpty() )
            return false;
    }

    return true;
}


bool NETLIST::AnyFootprintsChanged() const
{
    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( m_components[i].FootprintChanged() )
            return true;
    }

    return false;
}


#if defined( DEBUG )
void NETLIST::Show( int aNestLevel, REPORTER& aReporter )
{
    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( "<netlist>\n" );

    if( !m_components.empty() )
    {
        NestedSpace( aNestLevel+1, aReporter );
        aReporter.Report( "<components>\n" );

        for( unsigned i = 0;  i < m_components.size();  i++ )
        {
            m_components[i].Show( aNestLevel+2, aReporter );
        }

        NestedSpace( aNestLevel+1, aReporter );

        aReporter.Report( "</components>\n" );
    }

    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( "</netlist>\n" );
}
#endif


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

        // the corresponding component could be no more existing in netlist:
        // this is the case when it is just removed from schematic,
        // and still exists in footprint assignment list, before this list is updated
        // This is an usual case during the life of a design
        if( component )
            component->SetFootprintName( footprint );
        else
            ok = false;     // can be used to display a warning in Pcbnew.
    }

    return ok;
}
