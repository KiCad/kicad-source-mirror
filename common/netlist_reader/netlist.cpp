/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#include <netlist_reader/netlist.h>

#include <richio.h>
#include <string_utils.h>
#include <wx/tokenzr.h>
#include <wx/log.h>


int COMPONENT_NET::Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl )
{
    return aOut->Print( aNestLevel, "(pin_net %s %s)",
                        aOut->Quotew( m_pinName ).c_str(),
                        aOut->Quotew( m_netName ).c_str() );
}


COMPONENT_NET COMPONENT::m_emptyNet;


const COMPONENT_NET& COMPONENT::GetNet( const wxString& aPinName ) const
{
    wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                wxT( "Looking for pin '%s' in component '%s'" ),
                aPinName, m_reference );

    for( const COMPONENT_NET& net : m_nets )
    {
        wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                    wxT( "  Checking net pin name '%s'" ),
                    net.GetPinName() );

        if( net.GetPinName() == aPinName )
        {
            wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                        wxT( "  Found exact match for pin '%s'" ),
                        aPinName );
            return net;
        }

        // Check if this net's pin name is a stacked pin notation that expands to include aPinName
        std::vector<wxString> expandedPins = ExpandStackedPinNotation( net.GetPinName() );
        if( !expandedPins.empty() )
        {
            wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                        wxT( "  Pin name '%s' expanded to %zu pins" ),
                        net.GetPinName(), expandedPins.size() );

            for( const wxString& expandedPin : expandedPins )
            {
                wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                            wxT( "    Checking expanded pin '%s'" ),
                            expandedPin );
                if( expandedPin == aPinName )
                {
                    wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                                wxT( "  Found match for pin '%s' in stacked notation '%s'" ),
                                aPinName, net.GetPinName() );
                    return net;
                }
            }
        }
    }

    wxLogTrace( wxT( "NETLIST_STACKED_PINS" ),
                wxT( "  No net found for pin '%s'" ),
                aPinName );
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

        // Add all fields as a (field) under a (fields) node
        aOut->Print( nl + 1, "(fields" );

        for( std::pair<wxString, wxString> field : m_fields )
            aOut->Print( nl + 2, "\n(field (name %s) %s)", aOut->Quotew( field.first ).c_str(),
                         aOut->Quotew( field.second ).c_str() );

        aOut->Print( 0, ")\n" );

        // Add DNP and Exclude from BOM properties if we have them
        if( m_properties.count( "dnp" ) )
            aOut->Print( nl + 1, "(property (name \"dnp\"))\n" );

        if( m_properties.count( "exclude_from_bom" ) )
            aOut->Print( nl + 1, "(property (name \"exclude_from_bom\"))\n" );
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


void NETLIST::AddGroup( NETLIST_GROUP* aComponent )
{
    m_groups.push_back( aComponent );
}

NETLIST_GROUP* NETLIST::GetGroupByUuid( const KIID& aUuid )
{
    for( NETLIST_GROUP& group : m_groups )
    {
        if( group.uuid == aUuid )
            return &group;
    }

    return nullptr;
}


COMPONENT* NETLIST::GetComponentByReference( const wxString& aReference )
{
    COMPONENT* component = nullptr;

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


COMPONENT* NETLIST::GetComponentByUuid( const KIID& aUuid)
{
    if( aUuid == 0 )
        return nullptr;

    for( COMPONENT& component : m_components )
    {
        for( const KIID& compUuid : component.GetKIIDs() )
        {
            if( aUuid == compUuid )
                return &component;
        }
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
    return StrNumCmp( item1.GetReference(), item2.GetReference(), true ) < 0;
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


void NETLIST::ApplyGroupMembership()
{
    for( NETLIST_GROUP& group : m_groups )
    {
        for( KIID& member : group.members )
        {
            COMPONENT* component = GetComponentByUuid( member );

            if( component )
                component->SetGroup( &group );
        }
    }
}
