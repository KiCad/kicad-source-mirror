/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>.
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


#include <refdes_utils.h>
#include "pcb_netlist.h"
#include <footprint.h>


int COMPONENT_NET::Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    return aOut->Print( aNestLevel, "(pin_net %s %s)",
                        aOut->Quotew( m_pinName ).c_str(),
                        aOut->Quotew( m_netName ).c_str() );
}


void COMPONENT::SetFootprint( FOOTPRINT* aFootprint )
{
    m_footprint.reset( aFootprint );
    KIID_PATH path = m_path;

    if( !m_kiids.empty() )
        path.push_back( m_kiids.front() );

    if( aFootprint == NULL )
        return;

    aFootprint->SetReference( m_reference );
    aFootprint->SetValue( m_value );
    aFootprint->SetFPID( m_fpid );
    aFootprint->SetPath( path );
    aFootprint->SetProperties( m_properties );
}


COMPONENT_NET COMPONENT::m_emptyNet;


const COMPONENT_NET& COMPONENT::GetNet( const wxString& aPinName ) const
{
    for( const COMPONENT_NET& net : m_nets )
    {
        if( net.GetPinName() == aPinName )
            return net;
    }

    return m_emptyNet;
}


void COMPONENT::Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    int nl = aNestLevel;

    aOut->Print( nl, "(ref %s ",      aOut->Quotew( m_reference ).c_str() );
    aOut->Print( 0, "(fpid %s)\n",    aOut->Quotew( m_fpid.Format() ).c_str() );

    if( !( aCtl & CTL_OMIT_EXTRA ) )
    {
        aOut->Print( nl+1, "(value %s)\n",    aOut->Quotew( m_value ).c_str() );
        aOut->Print( nl+1, "(name %s)\n",     aOut->Quotew( m_name ).c_str() );
        aOut->Print( nl+1, "(library %s)\n",  aOut->Quotew( m_library ).c_str() );

        wxString path;

        for( const KIID& pathStep : m_path )
            path += '/' + pathStep.AsString();

        if( !( aCtl & CTL_OMIT_FP_UUID ) && !m_kiids.empty() )
            path += '/' + m_kiids.front().AsString();

        aOut->Print( nl+1, "(timestamp %s)\n", aOut->Quotew( path ).c_str() );
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


COMPONENT* NETLIST::GetComponentByPath( const KIID_PATH& aUuidPath )
{
    if( aUuidPath.empty() )
        return nullptr;

    KIID comp_uuid = aUuidPath.back();
    KIID_PATH base = aUuidPath;

    if( !base.empty() )
        base.pop_back();

    for( COMPONENT& component : m_components )
    {
        const std::vector<KIID>& kiids = component.GetKIIDs();

        if( base != component.GetPath() )
            continue;

        if( std::find( kiids.begin(), kiids.end(), comp_uuid ) != kiids.end() )
            return &component;
    }

    return nullptr;
}


/**
 * A helper function used to sort the component list used by loadNewModules.
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
 * Compare two #COMPONENT objects by reference designator.
 */
bool operator < ( const COMPONENT& item1, const COMPONENT& item2 )
{
    return UTIL::RefDesStringCompare( item1.GetReference(), item2.GetReference() ) < 0;
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


