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


int COMPONENT_NET::Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    return aOut->Print( aNestLevel, "(pin_net %s %s)",
            aOut->Quotew( m_pinName ).c_str(),
            aOut->Quotew( m_netName ).c_str() );
}


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


void COMPONENT::Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    int nl = aNestLevel;

    aOut->Print( nl, "(ref %s ",      aOut->Quotew( m_reference ).c_str() );
    aOut->Print( 0, "(fpid %s)\n",    aOut->Quotew( m_fpid.Format() ).c_str() );

    if( ! ( aCtl & CTL_OMIT_EXTRA ) )
    {
        aOut->Print( nl+1, "(value %s)\n",    aOut->Quotew( m_value ).c_str() );
        aOut->Print( nl+1, "(name %s)\n",     aOut->Quotew( m_name ).c_str() );
        aOut->Print( nl+1, "(library %s)\n",  aOut->Quotew( m_library ).c_str() );
        aOut->Print( nl+1, "(timestamp %s)\n", aOut->Quotew( m_timeStamp ).c_str() );
    }

    if( !( aCtl & CTL_OMIT_FILTERS ) && m_footprintFilters.GetCount() )
    {
        aOut->Print( nl+1, "(fp_filters" );

        for( unsigned i = 0;  i < m_footprintFilters.GetCount();  ++i )
            aOut->Print( 0, " %s", aOut->Quotew( m_footprintFilters[i] ).c_str() );

        aOut->Print( 0, ")\n" );
    }

    if( !( aCtl & CTL_OMIT_NETS ) && m_nets.size() )
    {
        int llen = aOut->Print( nl+1, "(nets " );

        for( unsigned i = 0;  i < m_nets.size();  ++i )
        {
            if( llen > 80 )
            {
                aOut->Print( 0, "\n" );
                llen = aOut->Print( nl+1, "  " );
            }

            llen += m_nets[i].Format( aOut, 0, aCtl );
        }

        aOut->Print( 0, ")\n" );
    }

    aOut->Print( nl, ")\n" );    // </ref>
}


void NETLIST::Format( const char* aDocName, OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    int nl = aNestLevel;

    aOut->Print( nl, "(%s\n", aDocName );

    for( unsigned i = 0;  i < m_components.size();  i++ )
    {
        m_components[i].Format( aOut, nl+1, aCtl );
    }

    aOut->Print( nl, ")\n" );
}


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

