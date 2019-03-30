/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/regex.hpp>
#include <wx/tokenzr.h>

#include <connection_graph.h>
#include <sch_pin_connection.h>
#include <sch_screen.h>

#include <sch_connection.h>


/**
 *
 * Buses can be defined in multiple ways. A bus vector consists of a prefix and
 * a numeric range of suffixes:
 *
 *     BUS_NAME[M..N]
 *
 * For example, the bus A[3..0] will contain nets A3, A2, A1, and A0.
 * The BUS_NAME is required.  M and N must be integers but do not need to be in
 * any particular order -- A[0..3] produces the same result.
 *
 * Like net names, bus names cannot contain whitespace.
 *
 * A bus group is just a grouping of signals, separated by spaces, some
 * of which may be bus vectors.  Bus groups can have names, but do not need to.
 *
 *     MEMORY{A[15..0] D[7..0] RW CE OE}
 *
 * In named bus groups, the net names are expanded as <BUS_NAME>.<NET_NAME>
 * In the above example, the nets would be named like MEMORY.A15, MEMORY.D0, etc.
 *
 *     {USB_DP USB_DN}
 *
 * In the above example, the bus is unnamed and so the underlying net names are
 * just USB_DP and USB_DN.
 *
 */
static boost::regex bus_label_re( "^([^[:space:]]+)(\\[[\\d]+\\.+[\\d]+\\])$" );

static boost::regex bus_group_label_re( "^([^[:space:]]+)?\\{((?:[^[:space:]]+(?:\\[[\\d]+\\.+[\\d]+\\])? ?)+)\\}$" );


SCH_CONNECTION::SCH_CONNECTION( SCH_ITEM* aParent, SCH_SHEET_PATH aPath ) :
        m_sheet( aPath ),
        m_parent( aParent )
{
    Reset();
}


bool SCH_CONNECTION::operator==( const SCH_CONNECTION& aOther ) const
{
    // NOTE: Not comparing m_dirty or net/bus/subgraph codes
    if( ( aOther.m_driver == m_driver ) &&
        ( aOther.m_type == m_type ) &&
        ( aOther.m_name == m_name ) &&
        ( aOther.m_sheet == m_sheet ) )
    {
        return true;
    }

    return false;
}


void SCH_CONNECTION::SetDriver( SCH_ITEM* aItem )
{
    m_driver = aItem;

    for( auto member : m_members )
        member->SetDriver( aItem );
}


void SCH_CONNECTION::SetSheet( SCH_SHEET_PATH aSheet )
{
    m_sheet = aSheet;

    for( auto member : m_members )
        member->SetSheet( aSheet );
}


bool SCH_CONNECTION::operator!=( const SCH_CONNECTION& aOther ) const
{
    return !( aOther == *this );
}


void SCH_CONNECTION::ConfigureFromLabel( wxString aLabel )
{
    if( IsBusVectorLabel( aLabel ) )
    {
        m_name = aLabel;
        m_type = CONNECTION_BUS;

        ParseBusVector( aLabel, &m_vector_prefix, &m_vector_start, &m_vector_end );

        for( long i = m_vector_start; i <= m_vector_end; ++i )
        {
            auto member = std::make_shared< SCH_CONNECTION >( m_parent, m_sheet );
            wxString name = m_vector_prefix;
            name << i;
            member->m_type = CONNECTION_NET;
            member->m_name = m_prefix + name;
            member->m_vector_index = i;
            m_members.push_back( member );
        }
    }
    else if( IsBusGroupLabel( aLabel ) )
    {
        m_type = CONNECTION_BUS_GROUP;
        m_name = aLabel;

        std::vector<wxString> members;
        wxString group_name;

        if( ParseBusGroup( aLabel, &group_name, members ) )
        {
            // Named bus groups generate a net prefix, unnamed ones don't
            auto prefix = ( group_name != "" ) ? ( group_name + "." ) : "";

            for( auto group_member : members )
            {
                // Handle alias inside bus group member list
                if( auto alias = g_ConnectionGraph->GetBusAlias( group_member ) )
                {
                    for( auto alias_member : alias->Members() )
                    {
                        auto member = std::make_shared< SCH_CONNECTION >( m_parent, m_sheet );
                        member->SetPrefix( prefix );
                        member->ConfigureFromLabel( alias_member );
                        m_members.push_back( member );
                    }
                }
                else
                {
                    auto member = std::make_shared< SCH_CONNECTION >( m_parent, m_sheet );
                    member->SetPrefix( prefix );
                    member->ConfigureFromLabel( group_member );
                    m_members.push_back( member );
                }
            }
        }
    }
    else if( auto alias = g_ConnectionGraph->GetBusAlias( aLabel ) )
    {
        m_type = CONNECTION_BUS_GROUP;
        m_name = aLabel;

        for( auto alias_member : alias->Members() )
        {
            auto member = std::make_shared< SCH_CONNECTION >( m_parent );
            member->ConfigureFromLabel( alias_member );
            m_members.push_back( member );
        }
    }
    else
    {
        m_name = m_prefix + aLabel;
        m_type = CONNECTION_NET;
    }
}


void SCH_CONNECTION::Reset()
{
    m_type = CONNECTION_NONE;
    m_name = "<NO NET>";
    m_prefix = "";
    m_suffix = "";
    m_driver = nullptr;
    m_members.clear();
    m_dirty = true;
    m_net_code = 0;
    m_bus_code = 0;
    m_subgraph_code = 0;
    m_vector_start = 0;
    m_vector_end = 0;
    m_vector_index = 0;
    m_vector_prefix = "";
}


void SCH_CONNECTION::Clone( SCH_CONNECTION& aOther )
{
    m_type = aOther.Type();
    m_driver = aOther.Driver();
    m_sheet = aOther.Sheet();
    m_name = aOther.Name( true );
    m_prefix = aOther.Prefix();
    // Don't clone suffix, it will be rolled into the name
    //m_suffix = aOther.Suffix();
    m_members = aOther.Members();
    m_net_code = aOther.NetCode();
    m_bus_code = aOther.BusCode();
    //m_subgraph_code = aOther.SubgraphCode();
    m_vector_start = aOther.VectorStart();
    m_vector_end = aOther.VectorEnd();
    m_vector_index = aOther.VectorIndex();
    m_vector_prefix = aOther.VectorPrefix();
}


bool SCH_CONNECTION::IsDriver() const
{
    wxASSERT( Parent() );

    switch( Parent()->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_PIN_CONNECTION_T:
    case SCH_SHEET_PIN_T:
    case SCH_SHEET_T:
    case LIB_PIN_T:
        return true;

    default:
        return false;
    }
}


wxString SCH_CONNECTION::Name( bool aIgnoreSheet ) const
{
    wxString ret = m_name + m_suffix;

    if( !Parent() || m_type == CONNECTION_NONE )
        return ret;

    bool prepend_path = true;

    switch( Parent()->Type() )
    {
    case SCH_PIN_CONNECTION_T:
        // Pins are either power connections or belong to a uniquely-annotated
        // component, so they don't need a path
        prepend_path = false;
        break;

    case SCH_GLOBAL_LABEL_T:
        prepend_path = false;
        break;

    default:
        break;
    }

    if( prepend_path && !aIgnoreSheet )
        ret = m_sheet.PathHumanReadable() + ret;

    return ret;
}


void SCH_CONNECTION::AppendInfoToMsgPanel( MSG_PANEL_ITEMS& aList ) const
{
#ifdef CONNECTIVITY_REAL_TIME

    wxString msg, group_name;
    std::vector<wxString> group_members;

    aList.push_back( MSG_PANEL_ITEM( _( "Connection Name" ), Name(), BROWN ) );

    msg.Printf( "%d", m_net_code );
    aList.push_back( MSG_PANEL_ITEM( _( "Net Code" ), msg, BROWN ) );

    if( auto alias = g_ConnectionGraph->GetBusAlias( m_name ) )
    {
        msg.Printf( _( "Bus Alias %s Members" ), m_name );

        wxString members;

        for( auto member : alias->Members() )
            members << member << " ";

        aList.push_back( MSG_PANEL_ITEM( msg, members, RED ) );
    }
    else if( ParseBusGroup( m_name, &group_name, group_members ) )
    {
        for( auto group_member : group_members )
        {
            if( auto group_alias = g_ConnectionGraph->GetBusAlias( group_member ) )
            {
                msg.Printf( _( "Bus Alias %s Members" ), group_alias->GetName() );

                wxString members;

                for( auto member : group_alias->Members() )
                    members << member << " ";

                aList.push_back( MSG_PANEL_ITEM( msg, members, RED ) );
            }
        }
    }

#endif
}


void SCH_CONNECTION::AppendDebugInfoToMsgPanel( MSG_PANEL_ITEMS& aList ) const
{
#ifdef CONNECTIVITY_REAL_TIME
    wxString msg;

    AppendInfoToMsgPanel( aList );

    msg.Printf( "%d", m_bus_code );
    aList.push_back( MSG_PANEL_ITEM( _( "Bus Code" ), msg, BROWN ) );

    msg.Printf( "%d", m_subgraph_code );
    aList.push_back( MSG_PANEL_ITEM( _( "Subgraph Code" ), msg, BROWN ) );

    if( auto driver = Driver() )
    {
        msg.Printf( "%s at %p", driver->GetSelectMenuText( MILLIMETRES ), driver );
        aList.push_back( MSG_PANEL_ITEM( _( "Connection Source" ), msg, RED ) );
    }

    msg.Printf( "%s at %p", Parent()->GetSelectMenuText( MILLIMETRES ), Parent() );
    aList.push_back( MSG_PANEL_ITEM( _( "Attached To" ), msg, RED ) );
#endif
}


bool SCH_CONNECTION::IsBusLabel( const wxString& aLabel )
{
    return IsBusVectorLabel( aLabel ) || IsBusGroupLabel( aLabel );
}


bool SCH_CONNECTION::IsBusVectorLabel( const wxString& aLabel )
{
    return boost::regex_match( std::string( aLabel.mb_str() ), bus_label_re );
}


bool SCH_CONNECTION::IsBusGroupLabel( const wxString& aLabel )
{
    return boost::regex_match( std::string( aLabel.mb_str() ), bus_group_label_re );
}


void SCH_CONNECTION::ParseBusVector( wxString aVector, wxString* aName,
                                     long* begin, long* end ) const
{
    auto ss_vector = std::string( aVector.mb_str() );
    boost::smatch matches;

    if( !boost::regex_match( ss_vector, matches, bus_label_re ) )
    {
        wxFAIL_MSG( wxT( "<" ) + aVector + wxT( "> is not a valid bus vector." ) );
        return;
    }

    *aName = wxString( matches[1] );
    wxString numberString( matches[2] );

    // numberString will include the brackets, e.g. [5..0] so skip the first one
    size_t i = 1, len = numberString.Len();
    wxString tmp;

    while( i < len && numberString[i] != '.' )
    {
        tmp.Append( numberString[i] );
        i++;
    }

    tmp.ToLong( begin );

    while( i < len && numberString[i] == '.' )
        i++;

    tmp.Empty();

    while( i < len && numberString[i] != ']' )
    {
        tmp.Append( numberString[i] );
        i++;
    }

    tmp.ToLong( end );

    if( *begin < 0 )
        *begin = 0;

    if( *end < 0 )
        *end = 0;

    if( *begin > *end )
        std::swap( *begin, *end );
}


bool SCH_CONNECTION::ParseBusGroup( wxString aGroup, wxString* aName,
                                    std::vector<wxString>& aMemberList ) const
{
    auto ss_group = std::string( aGroup.mb_str() );
    boost::smatch matches;

    if( !boost::regex_match( ss_group, matches, bus_group_label_re ) )
    {
        return false;
    }

    *aName = wxString( matches[1] );

    wxStringTokenizer tokenizer( wxString( matches[2] ), " " );
    while( tokenizer.HasMoreTokens() )
    {
        aMemberList.push_back( tokenizer.GetNextToken() );
    }

    return true;
}


bool SCH_CONNECTION::IsSubsetOf( SCH_CONNECTION* aOther ) const
{
    if( aOther->IsNet() )
        return IsNet() ? ( aOther->Name( true ) == Name( true ) ) : false;

    if( !IsBus() )
        return false;

    std::vector<wxString> mine, theirs;

    for( auto m : Members() )
        mine.push_back( m->Name( true ) );

    for( auto m : aOther->Members() )
        theirs.push_back( m->Name( true ) );

    std::set<wxString> subset;

    std::set_intersection( mine.begin(), mine.end(),
                           theirs.begin(), theirs.end(),
                           std::inserter(subset, subset.begin() ) );

    return ( !subset.empty() );
}


bool SCH_CONNECTION::IsMemberOfBus( SCH_CONNECTION* aOther ) const
{
    if( !aOther->IsBus() )
        return false;

    auto me = Name( true );

    for( auto m : aOther->Members() )
        if( m->Name( true ) == me )
            return true;

    return false;
}
