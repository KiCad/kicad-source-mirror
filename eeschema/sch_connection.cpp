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

#include <connection_graph.h>
#include <sch_component.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <advanced_config.h>

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

    recacheName();

    for( const auto& member : m_members )
        member->SetDriver( aItem );
}


void SCH_CONNECTION::SetSheet( SCH_SHEET_PATH aSheet )
{
    m_sheet = aSheet;

    recacheName();

    for( const auto& member : m_members )
        member->SetSheet( aSheet );
}


bool SCH_CONNECTION::operator!=( const SCH_CONNECTION& aOther ) const
{
    return !( aOther == *this );
}


void SCH_CONNECTION::ConfigureFromLabel( const wxString& aLabel )
{
    m_members.clear();

    m_name       = aLabel;
    m_local_name = aLabel;

    wxString prefix;
    std::vector<wxString> members;

    wxString unescaped = UnescapeString( aLabel );

    if( ParseBusVector( unescaped, &prefix, &members ) )
    {
        m_type = CONNECTION_TYPE::BUS;
        m_vector_prefix = prefix;

        long i = 0;

        for( const wxString& vector_member : members )
        {
            auto member            = std::make_shared<SCH_CONNECTION>( m_parent, m_sheet );
            member->m_type         = CONNECTION_TYPE::NET;
            member->m_prefix       = m_prefix;
            member->m_local_name   = vector_member;
            member->m_vector_index = i++;
            member->SetName( vector_member );
            m_members.push_back( member );
        }
    }
    else if( ParseBusGroup( unescaped, &prefix, &members ) )
    {
        m_type = CONNECTION_TYPE::BUS_GROUP;

        // Named bus groups generate a net prefix, unnamed ones don't
        if( !prefix.IsEmpty() )
            prefix += wxT( "." );

        for( const wxString& group_member : members )
        {
            // Handle alias inside bus group member list
            if( auto alias = m_graph->GetBusAlias( group_member ) )
            {
                for( const wxString& alias_member : alias->Members() )
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
    else
    {
        m_type = CONNECTION_TYPE::NET;
    }

    recacheName();
}


void SCH_CONNECTION::Reset()
{
    m_type = CONNECTION_TYPE::NONE;
    m_name.Empty();
    m_local_name.Empty();
    m_cached_name.Empty();
    m_cached_name_with_path.Empty();
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
    // Note: m_local_name is not cloned
    m_prefix = aOther.Prefix();
    m_suffix = aOther.Suffix();
    m_members = aOther.Members();
    m_net_code = aOther.NetCode();
    m_bus_code = aOther.BusCode();
    m_vector_start = aOther.VectorStart();
    m_vector_end = aOther.VectorEnd();
    // Note: m_vector_index is not cloned
    m_vector_prefix = aOther.VectorPrefix();

    // Note: subgraph code isn't cloned, it should remain with the original object

    recacheName();
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
        return ( pin->IsPowerConnection()
                 || pin->GetParentComponent()->IsAnnotated( &m_sheet ) );
    }

    default:
        return false;
    }
}


const wxString& SCH_CONNECTION::Name( bool aIgnoreSheet ) const
{
    wxASSERT( !m_cached_name.IsEmpty() );
    return aIgnoreSheet ? m_cached_name : m_cached_name_with_path;
}


void SCH_CONNECTION::recacheName()
{
    m_cached_name = m_name.IsEmpty() ? "<NO NET>" : m_prefix + m_name + m_suffix;

    bool prepend_path = true;

    if( !Parent() || m_type == CONNECTION_TYPE::NONE )
        prepend_path = false;

    if( m_driver )
    {
        switch( m_driver->Type() )
        {
        case SCH_GLOBAL_LABEL_T:
        case SCH_PIN_T:
            // Pins are either power connections or belong to a uniquely-annotated
            // component, so they don't need a path if they are driving the subgraph
            prepend_path = false;
            break;

        default:
            break;
        }
    }

    m_cached_name_with_path =
            prepend_path ? m_sheet.PathHumanReadable() + m_cached_name : m_cached_name;
}


void SCH_CONNECTION::SetPrefix( const wxString& aPrefix )
{
    m_prefix = aPrefix;

    recacheName();

    for( const auto& m : Members() )
        m->SetPrefix( aPrefix );
}


void SCH_CONNECTION::SetSuffix( const wxString& aSuffix )
{
    m_suffix = aSuffix;

    recacheName();

    for( const auto& m : Members() )
        m->SetSuffix( aSuffix );
}


void SCH_CONNECTION::AppendInfoToMsgPanel( MSG_PANEL_ITEMS& aList ) const
{
    if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        return;

    wxString msg, group_name;
    std::vector<wxString> group_members;

    aList.push_back( MSG_PANEL_ITEM( _( "Connection Name" ), UnescapeString( Name() ), BROWN ) );

    // NOTE(JE) Disabling this for now, because net codes are generated in the netlist exporter
    // in order to avoid sort costs.  It may make sense to just tear out net codes from the
    // CONNECTION_GRAPH entirely in the future, as they are mostly only useful for netlist exports.
#if 0
    if( !IsBus() )
    {
        msg.Printf( "%d", m_net_code );
        aList.push_back( MSG_PANEL_ITEM( _( "Net Code" ), msg, BROWN ) );
    }
#endif

    if( auto alias = m_graph->GetBusAlias( m_name ) )
    {
        msg.Printf( _( "Bus Alias %s Members" ), m_name );

        wxString members;

        for( const auto& member : alias->Members() )
            members << member << " ";

        aList.push_back( MSG_PANEL_ITEM( msg, members, RED ) );
    }
    else if( ParseBusGroup( m_name, &group_name, &group_members ) )
    {
        for( const auto& group_member : group_members )
        {
            if( auto group_alias = m_graph->GetBusAlias( group_member ) )
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
        msg.Printf( "%s at %p", driver->GetSelectMenuText( EDA_UNITS::MILLIMETRES ), driver );
        aList.push_back( MSG_PANEL_ITEM( "Connection Source", msg, RED ) );
    }
}


bool SCH_CONNECTION::IsBusLabel( const wxString& aLabel )
{
    return ParseBusVector( aLabel, nullptr, nullptr ) || ParseBusGroup( aLabel, nullptr, nullptr );
}


bool SCH_CONNECTION::MightBeBusLabel( const wxString& aLabel )
{
    // Weak heuristic for performance reasons.  Stronger test will be used for connectivity
    wxString label = UnescapeString( aLabel );

    return label.Contains( wxT( "[" ) ) || label.Contains( wxT( "{" ) );
}


static bool isSuperSub( wxChar c )
{
    return c == '_' || c == '^';
};


bool SCH_CONNECTION::ParseBusVector( const wxString& aBus, wxString* aName,
                                     std::vector<wxString>* aMemberList )
{
    auto isDigit = []( wxChar c )
                   {
                       static   wxString digits( wxT( "0123456789" ) );
                       return digits.Contains( c );
                   };

    size_t   busLen = aBus.length();
    size_t   i = 0;
    wxString prefix;
    wxString suffix;
    wxString tmp;
    long     begin = 0;
    long     end = 0;
    int      braceNesting = 0;

    prefix.reserve( busLen );

    // Parse prefix
    //
    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '{' )
        {
            if( i > 0 && isSuperSub( aBus[i-1] ) )
                braceNesting++;
            else
                return false;
        }
        else if( aBus[i] == '}' )
        {
            braceNesting--;
        }

        if( aBus[i] == ' ' || aBus[i] == ']' )
            return false;

        if( aBus[i] == '[' )
            break;

        prefix += aBus[i];
    }

    // Parse start number
    //
    i++;  // '[' character

    if( i >= busLen )
        return false;

    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '.' && i + 1 < busLen && aBus[i+1] == '.' )
        {
            tmp.ToLong( &begin );
            i += 2;
            break;
        }

        if( !isDigit( aBus[i] ) )
            return false;

        tmp += aBus[i];
    }

    // Parse end number
    //
    tmp = wxEmptyString;

    if( i >= busLen )
        return false;

    for( ; i < busLen; ++i )
    {
        if( aBus[i] == ']' )
        {
            tmp.ToLong( &end );
            ++i;
            break;
        }

        if( !isDigit( aBus[i] ) )
            return false;

        tmp += aBus[i];
    }

    // Parse suffix
    //
    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '}' )
        {
            braceNesting--;
            suffix += aBus[i];
        }
        else if( aBus[i] == '~' )
        {
            suffix += aBus[i];
        }
        else
        {
            return false;
        }
    }

    if( braceNesting != 0 )
        return false;

    if( begin == end )
        return false;
    else if( begin > end )
        std::swap( begin, end );

    if( aName )
        *aName = prefix;

    if( aMemberList )
    {
        for( long idx = begin; idx <= end; ++idx )
        {
            wxString str = prefix;
            str << idx;
            str << suffix;

            aMemberList->emplace_back( str );
        }
    }

    return true;
}


bool SCH_CONNECTION::ParseBusGroup( wxString aGroup, wxString* aName,
                                    std::vector<wxString>* aMemberList )
{
    size_t   groupLen = aGroup.length();
    size_t   i = 0;
    wxString prefix;
    wxString suffix;
    wxString tmp;
    int      braceNesting = 0;

    prefix.reserve( groupLen );

    // Parse prefix
    //
    for( ; i < groupLen; ++i )
    {
        if( aGroup[i] == '{' )
        {
            if( i > 0 && isSuperSub( aGroup[i-1] ) )
                braceNesting++;
            else
                break;
        }
        else if( aGroup[i] == '}' )
        {
            braceNesting--;
        }

        if( aGroup[i] == ' ' || aGroup[i] == '[' || aGroup[i] == ']' )
            return false;

        prefix += aGroup[i];
    }

    if( braceNesting != 0 )
        return false;

    if( aName )
        *aName = prefix;

    // Parse members
    //
    i++;  // '{' character

    if( i >= groupLen )
        return false;

    for( ; i < groupLen; ++i )
    {
        if( aGroup[i] == '{' )
        {
            if( i > 0 && isSuperSub( aGroup[i-1] ) )
                braceNesting++;
            else
                return false;
        }
        else if( aGroup[i] == '}' )
        {
            if( braceNesting )
                braceNesting--;
            else
            {
                if( aMemberList )
                    aMemberList->push_back( tmp );

                return true;
            }
        }

        if( aGroup[i] == ' ' )
        {
            if( aMemberList )
                aMemberList->push_back( tmp );

            tmp.Clear();
            continue;
        }

        tmp += aGroup[i];
    }

    return false;
}


wxString SCH_CONNECTION::PrintBusForUI( const wxString& aGroup )
{
    size_t   groupLen = aGroup.length();
    size_t   i = 0;
    wxString ret;
    int      braceNesting = 0;
    int      tildeNesting = 0;

    // Parse prefix
    //
    for( ; i < groupLen; ++i )
    {
        if( isSuperSub( aGroup[i] ) && i + 1 < groupLen && aGroup[i+1] == '{' )
        {
            braceNesting++;
            i++;
            continue;
        }
        else if( aGroup[i] == '~' )
        {
            if( tildeNesting )
            {
                tildeNesting = 0;
                continue;
            }
            else
            {
                tildeNesting++;
            }
        }
        else if( aGroup[i] == '}' )
        {
            braceNesting--;
            continue;
        }

        ret += aGroup[i];

        if( aGroup[i] == '{' )
            break;
    }

    // Parse members
    //
    i++;  // '{' character

    for( ; i < groupLen; ++i )
    {
        if( isSuperSub( aGroup[i] ) && i + 1 < groupLen && aGroup[i+1] == '{' )
        {
            braceNesting++;
            i++;
            continue;
        }
        else if( aGroup[i] == '~' )
        {
            if( tildeNesting )
            {
                tildeNesting = 0;
                continue;
            }
            else
            {
                tildeNesting++;
            }
        }
        else if( aGroup[i] == '}' )
        {
            braceNesting--;
            continue;
        }

        ret += aGroup[i];

        if( aGroup[i] == '}' )
            break;
    }

    return ret;
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