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

#include <regex>
#include <wx/tokenzr.h>

#include <advanced_config.h>
#include <connection_graph.h>
#include <sch_component.h>
#include <sch_pin.h>
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
static std::regex bus_label_re( "^([^[:space:]]+)(\\[[\\d]+\\.+[\\d]+\\])(~?)$" );

static std::regex bus_group_label_re( "^([^[:space:]]+)?\\{((?:[^[:space:]]+(?:\\[[\\d]+\\.+[\\d]+\\])? ?)+)\\}$" );


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

    for( const auto& member : m_members )
        member->SetDriver( aItem );
}


void SCH_CONNECTION::SetSheet( SCH_SHEET_PATH aSheet )
{
    m_sheet = aSheet;

    for( const auto& member : m_members )
        member->SetSheet( aSheet );
}


bool SCH_CONNECTION::operator!=( const SCH_CONNECTION& aOther ) const
{
    return !( aOther == *this );
}


void SCH_CONNECTION::ConfigureFromLabel( wxString aLabel )
{
    m_members.clear();

    if( IsBusVectorLabel( aLabel ) )
    {
        m_name = aLabel;
        m_type = CONNECTION_BUS;

        std::vector<wxString> members;

        ParseBusVector( aLabel, &m_vector_prefix, members );
        long i = 0;

        for( const auto& vector_member : members )
        {
            auto member = std::make_shared< SCH_CONNECTION >( m_parent, m_sheet );
            member->m_type = CONNECTION_NET;
            member->m_prefix = m_prefix;
            member->m_name = vector_member;
            member->m_vector_index = i++;
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
            wxString prefix = group_name != wxT( "" ) ? ( group_name + wxT( "." ) ) : wxT( "" );

            for( const auto& group_member : members )
            {
                // Handle alias inside bus group member list
                if( auto alias = g_ConnectionGraph->GetBusAlias( group_member ) )
                {
                    for( const auto& alias_member : alias->Members() )
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
    else
    {
        m_name = aLabel;
        m_type = CONNECTION_NET;
    }
}


void SCH_CONNECTION::Reset()
{
    m_type = CONNECTION_NONE;
    m_name.Empty();
    m_prefix.Empty();
    m_suffix .Empty();
    m_driver = nullptr;
    m_members.clear();
    m_dirty = true;
    m_net_code = 0;
    m_bus_code = 0;
    m_subgraph_code = 0;
    m_vector_start = 0;
    m_vector_end = 0;
    m_vector_index = 0;
    m_vector_prefix.Empty();
}


void SCH_CONNECTION::Clone( SCH_CONNECTION& aOther )
{
    m_type = aOther.Type();
    m_driver = aOther.Driver();
    m_sheet = aOther.Sheet();
    m_name = aOther.m_name;
    m_prefix = aOther.Prefix();
    m_suffix = aOther.Suffix();
    m_members = aOther.Members();
    m_net_code = aOther.NetCode();
    m_bus_code = aOther.BusCode();
    m_vector_start = aOther.VectorStart();
    m_vector_end = aOther.VectorEnd();
    m_vector_index = aOther.VectorIndex();
    m_vector_prefix = aOther.VectorPrefix();

    // Note: subgraph code isn't cloned, it should remain with the original object
}


bool SCH_CONNECTION::IsDriver() const
{
    wxASSERT( Parent() );

    switch( Parent()->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
    case SCH_SHEET_T:
    case LIB_PIN_T:
        return true;

    case SCH_PIN_T:
    {
        auto pin = static_cast<SCH_PIN*>( Parent() );

        // Only annotated components should drive nets
        return pin->GetParentComponent()->IsAnnotated( &m_sheet );
    }

    default:
        return false;
    }
}


wxString SCH_CONNECTION::Name( bool aIgnoreSheet ) const
{
    wxString ret = m_prefix + m_name + m_suffix;

    if( m_name.IsEmpty() )
        ret = "<NO NET>";

    if( !Parent() || m_type == CONNECTION_NONE )
        return ret;

    if( !aIgnoreSheet )
    {
        bool prepend_path = true;

        if( m_driver )
        {
            switch( m_driver->Type() )
            {
            case SCH_PIN_T:
                // Pins are either power connections or belong to a uniquely-annotated
                // component, so they don't need a path if they are driving the subgraph
                prepend_path = false;
                break;

            case SCH_GLOBAL_LABEL_T:
                prepend_path = false;
                break;

            default:
                break;
            }
        }

        if( prepend_path )
            ret = m_sheet.PathHumanReadable() + ret;
    }

    return ret;
}


void SCH_CONNECTION::SetPrefix( const wxString& aPrefix )
{
    m_prefix = aPrefix;

    for( const auto& m : Members() )
        m->SetPrefix( aPrefix );
}


void SCH_CONNECTION::SetSuffix( const wxString& aSuffix )
{
    m_suffix = aSuffix;

    for( const auto& m : Members() )
        m->SetSuffix( aSuffix );
}


void SCH_CONNECTION::AppendInfoToMsgPanel( MSG_PANEL_ITEMS& aList ) const
{
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        return;

    wxString msg, group_name;
    std::vector<wxString> group_members;

    aList.push_back( MSG_PANEL_ITEM( _( "Connection Name" ), Name(), BROWN ) );

    if( !IsBus() )
    {
        msg.Printf( "%d", m_net_code );
        aList.push_back( MSG_PANEL_ITEM( _( "Net Code" ), msg, BROWN ) );
    }

    if( auto alias = g_ConnectionGraph->GetBusAlias( m_name ) )
    {
        msg.Printf( _( "Bus Alias %s Members" ), m_name );

        wxString members;

        for( const auto& member : alias->Members() )
            members << member << " ";

        aList.push_back( MSG_PANEL_ITEM( msg, members, RED ) );
    }
    else if( ParseBusGroup( m_name, &group_name, group_members ) )
    {
        for( const auto& group_member : group_members )
        {
            if( auto group_alias = g_ConnectionGraph->GetBusAlias( group_member ) )
            {
                msg.Printf( _( "Bus Alias %s Members" ), group_alias->GetName() );

                wxString members;

                for( const auto& member : group_alias->Members() )
                    members << member << " ";

                aList.push_back( MSG_PANEL_ITEM( msg, members, RED ) );
            }
        }
    }
}


void SCH_CONNECTION::AppendDebugInfoToMsgPanel( MSG_PANEL_ITEMS& aList ) const
{
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        return;

    // These messages are not flagged as translatable, because they are only debug messages
    wxString msg;

    AppendInfoToMsgPanel( aList );

    if( IsBus() )
    {
        msg.Printf( "%d", m_bus_code );
        aList.push_back( MSG_PANEL_ITEM( "Bus Code", msg, BROWN ) );
    }

    msg.Printf( "%d", m_subgraph_code );
    aList.push_back( MSG_PANEL_ITEM( "Subgraph Code", msg, BROWN ) );

    if( auto driver = Driver() )
    {
        msg.Printf( "%s at %p", driver->GetSelectMenuText( MILLIMETRES ), driver );
        aList.push_back( MSG_PANEL_ITEM( "Connection Source", msg, RED ) );
    }
}


bool SCH_CONNECTION::IsBusLabel( const wxString& aLabel )
{
    return IsBusVectorLabel( aLabel ) || IsBusGroupLabel( aLabel );
}


bool SCH_CONNECTION::IsBusVectorLabel( const wxString& aLabel )
{
    try
    {
        return std::regex_match( std::string( aLabel.mb_str() ), bus_label_re );
    }
    catch( ... )
    {
        return false;
    }
}


bool SCH_CONNECTION::IsBusGroupLabel( const wxString& aLabel )
{
    try
    {
        return std::regex_match( std::string( aLabel.mb_str() ), bus_group_label_re );
    }
    catch( ... )
    {
        return false;
    }
}


bool SCH_CONNECTION::ParseBusVector( wxString aBus, wxString* aName,
                                     std::vector<wxString>& aMemberList ) const
{
    auto ss_vector = std::string( aBus.mb_str() );
    std::smatch matches;

    try
    {
        if( !std::regex_match( ss_vector, matches, bus_label_re ) )
            return false;
    }
    catch( ... )
    {
        return false;
    }

    long begin = 0, end = 0;
    *aName = wxString( matches[1] );
    wxString numberString( matches[2] );

    // If we have three match groups, it means there was a tilde at the end of the vector
    bool append_tilde = wxString( matches[3] ).IsSameAs( wxT( "~" ) );

    // numberString will include the brackets, e.g. [5..0] so skip the first one
    size_t i = 1, len = numberString.Len();
    wxString tmp;

    while( i < len && numberString[i] != '.' )
    {
        tmp.Append( numberString[i] );
        i++;
    }

    tmp.ToLong( &begin );

    while( i < len && numberString[i] == '.' )
        i++;

    tmp.Empty();

    while( i < len && numberString[i] != ']' )
    {
        tmp.Append( numberString[i] );
        i++;
    }

    tmp.ToLong( &end );

    if( begin < 0 )
        begin = 0;

    if( end < 0 )
        end = 0;

    if( begin > end )
        std::swap( begin, end );

    for( long idx = begin; idx <= end; ++idx )
    {
        wxString str = *aName;
        str << idx;

        if( append_tilde )
            str << '~';

        aMemberList.emplace_back( str );
    }

    return true;
}


bool SCH_CONNECTION::ParseBusGroup( wxString aGroup, wxString* aName,
                                    std::vector<wxString>& aMemberList ) const
{
    auto ss_group = std::string( aGroup.mb_str() );
    std::smatch matches;

    try
    {
        if( !std::regex_match( ss_group, matches, bus_group_label_re ) )
        {
            return false;
        }
    }
    catch( ... )
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

    for( const auto& m : Members() )
        mine.push_back( m->Name( true ) );

    for( const auto& m : aOther->Members() )
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

    for( const auto& m : aOther->Members() )
        if( m->Name( true ) == me )
            return true;

    return false;
}