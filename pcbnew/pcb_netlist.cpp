/**
 * @file pcb_netlist.cpp
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


#include <macros.h>
#include <kicad_string.h>
#include <reporter.h>

#include <pcb_netlist.h>
#include <class_module.h>


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
    aModule->SetFPID( m_fpid );
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


bool COMPONENT::MatchesFootprintFilters( const wxString& aFootprintName ) const
{
    if( m_footprintFilters.GetCount() == 0 )
        return true;

    // The matching is case insensitive
    wxString name = aFootprintName.Upper();

    for( unsigned ii = 0; ii < m_footprintFilters.GetCount(); ii++ )
    {
        if( name.Matches( m_footprintFilters[ii].Upper() ) )
            return true;
    }

    return false;
}


#if defined(DEBUG)
void COMPONENT::Show( int aNestLevel, REPORTER& aReporter )
{
    NestedSpace( aNestLevel, aReporter );
    aReporter.Report( wxT( "<component>\n" ) );
    NestedSpace( aNestLevel+1, aReporter );
    aReporter.Report( wxString::Format( wxT( "<ref=%s value=%s name=%s library=%s fpid=%s "
                                             "timestamp=%s>\n" ),
                                        GetChars( m_reference ), GetChars( m_value ),
                                        GetChars( m_name ), GetChars( m_library ),
                                        m_fpid.Format().c_str(),
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
 * Function ByFPID
 * is a helper function used to sort the component list used by loadNewModules.
 */
static bool ByFPID( const COMPONENT& ref, const COMPONENT& cmp )
{
    return ref.GetFPID() > cmp.GetFPID();
}


void NETLIST::SortByFPID()
{
    m_components.sort( ByFPID );
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
        if( !m_components[i].GetFPID().empty() )
            return true;
    }

    return false;
}


bool NETLIST::AllFootprintsLinked() const
{
    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        if( m_components[i].GetFPID().empty() )
            return false;
    }

    return true;
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
