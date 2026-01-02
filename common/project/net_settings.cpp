/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <limits>

#include <json_common.h>

#include <project/net_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <base_units.h>
#include <unordered_set>


// const int netSettingsSchemaVersion = 0;
// const int netSettingsSchemaVersion = 1;     // new overbar syntax
// const int netSettingsSchemaVersion = 2;     // exclude buses from netclass members
// const int netSettingsSchemaVersion = 3;     // netclass assignment patterns
// const int netSettingsSchemaVersion = 4;     // netclass ordering
const int netSettingsSchemaVersion = 5; // Tuning profile names


static std::optional<int> getInPcbUnits( const nlohmann::json& aObj, const std::string& aKey,
                               std::optional<int> aDefault = std::optional<int>() )
{
    if( aObj.contains( aKey ) && aObj[aKey].is_number() )
        return pcbIUScale.mmToIU( aObj[aKey].get<double>() );
    else
        return aDefault;
};


static std::optional<int> getInSchUnits( const nlohmann::json& aObj, const std::string& aKey,
                                         std::optional<int> aDefault = std::optional<int>() )
{
    if( aObj.contains( aKey ) && aObj[aKey].is_number() )
        return schIUScale.MilsToIU( aObj[aKey].get<double>() );
    else
        return aDefault;
};


NET_SETTINGS::NET_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "net_settings", netSettingsSchemaVersion, aParent, aPath, false )
{
    m_defaultNetClass = std::make_shared<NETCLASS>( NETCLASS::Default, true );
    m_defaultNetClass->SetDescription( _( "This is the default net class." ) );
    m_defaultNetClass->SetPriority( std::numeric_limits<int>::max() );

    auto saveNetclass =
            []( nlohmann::json& json_array, const std::shared_ptr<NETCLASS>& nc )
            {
                // Note: we're in common/, but we do happen to know which of these
                // fields are used in which units system.
                nlohmann::json nc_json = { { "name", nc->GetName().ToUTF8() },
                                           { "priority", nc->GetPriority() },
                                           { "schematic_color", nc->GetSchematicColor( true ) },
                                           { "pcb_color", nc->GetPcbColor( true ) },
                                           { "tuning_profile", nc->GetTuningProfile() } };

                auto saveInPcbUnits =
                        []( nlohmann::json& json, const std::string& aKey, int aValue )
                        {
                            json.push_back( { aKey, pcbIUScale.IUTomm( aValue ) } );
                        };

                if( nc->HasWireWidth() )
                    nc_json.push_back(
                            { "wire_width", schIUScale.IUToMils( nc->GetWireWidth() ) } );

                if( nc->HasBusWidth() )
                    nc_json.push_back( { "bus_width", schIUScale.IUToMils( nc->GetBusWidth() ) } );

                if( nc->HasLineStyle() )
                    nc_json.push_back( { "line_style", nc->GetLineStyle() } );

                if( nc->HasClearance() )
                    saveInPcbUnits( nc_json, "clearance", nc->GetClearance() );

                if( nc->HasTrackWidth() )
                    saveInPcbUnits( nc_json, "track_width", nc->GetTrackWidth() );

                if( nc->HasViaDiameter() )
                    saveInPcbUnits( nc_json, "via_diameter", nc->GetViaDiameter() );

                if( nc->HasViaDrill() )
                    saveInPcbUnits( nc_json, "via_drill", nc->GetViaDrill() );

                if( nc->HasuViaDiameter() )
                    saveInPcbUnits( nc_json, "microvia_diameter", nc->GetuViaDiameter() );

                if( nc->HasuViaDrill() )
                    saveInPcbUnits( nc_json, "microvia_drill", nc->GetuViaDrill()  );

                if( nc->HasDiffPairWidth() )
                    saveInPcbUnits( nc_json, "diff_pair_width", nc->GetDiffPairWidth() );

                if( nc->HasDiffPairGap() )
                    saveInPcbUnits( nc_json, "diff_pair_gap", nc->GetDiffPairGap() );

                if( nc->HasDiffPairViaGap() )
                    saveInPcbUnits( nc_json, "diff_pair_via_gap", nc->GetDiffPairViaGap() );

                json_array.push_back( nc_json );
            };

    auto readNetClass =
            []( const nlohmann::json& entry )
            {
                wxString name = entry["name"];

                std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( name, false );

                int priority = entry["priority"];
                nc->SetPriority( priority );

                nc->SetTuningProfile( entry["tuning_profile"] );

                if( auto value = getInPcbUnits( entry, "clearance" ) )
                    nc->SetClearance( *value );

                if( auto value = getInPcbUnits( entry, "track_width" ) )
                    nc->SetTrackWidth( *value );

                if( auto value = getInPcbUnits( entry, "via_diameter" ) )
                    nc->SetViaDiameter( *value );

                if( auto value = getInPcbUnits( entry, "via_drill" ) )
                    nc->SetViaDrill( *value );

                if( auto value = getInPcbUnits( entry, "microvia_diameter" ) )
                    nc->SetuViaDiameter( *value );

                if( auto value = getInPcbUnits( entry, "microvia_drill" ) )
                    nc->SetuViaDrill( *value );

                if( auto value = getInPcbUnits( entry, "diff_pair_width" ) )
                    nc->SetDiffPairWidth( *value );

                if( auto value = getInPcbUnits( entry, "diff_pair_gap" ) )
                    nc->SetDiffPairGap( *value );

                if( auto value = getInPcbUnits( entry, "diff_pair_via_gap" ) )
                    nc->SetDiffPairViaGap( *value );

                if( auto value = getInSchUnits( entry, "wire_width" ) )
                    nc->SetWireWidth( *value );

                if( auto value = getInSchUnits( entry, "bus_width" ) )
                    nc->SetBusWidth( *value );

                if( entry.contains( "line_style" ) && entry["line_style"].is_number() )
                    nc->SetLineStyle( entry["line_style"].get<int>() );

                if( entry.contains( "pcb_color" ) && entry["pcb_color"].is_string() )
                    nc->SetPcbColor( entry["pcb_color"].get<KIGFX::COLOR4D>() );

                if( entry.contains( "schematic_color" )
                        && entry["schematic_color"].is_string() )
                {
                    nc->SetSchematicColor( entry["schematic_color"].get<KIGFX::COLOR4D>() );
                }

                return nc;
            };

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "classes",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                if( m_defaultNetClass )
                    saveNetclass( ret, m_defaultNetClass );

                for( const auto& [name, netclass] : m_netClasses )
                    saveNetclass( ret, netclass );

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_netClasses.clear();

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() || !entry.contains( "name" ) )
                        continue;

                    std::shared_ptr<NETCLASS> nc = readNetClass( entry );

                    if( nc->IsDefault() )
                        m_defaultNetClass = nc;
                    else
                        m_netClasses[nc->GetName()] = nc;
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "net_colors",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const auto& [netname, color] : m_netColorAssignments )
                {
                    std::string key( netname.ToUTF8() );
                    ret[ std::move( key ) ] = color;
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                m_netColorAssignments.clear();

                for( const auto& pair : aJson.items() )
                {
                    wxString key( pair.key().c_str(), wxConvUTF8 );
                    m_netColorAssignments[std::move( key )] = pair.value().get<KIGFX::COLOR4D>();
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "netclass_assignments",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const auto& [netname, netclassNames] : m_netClassLabelAssignments )
                {
                    nlohmann::json netclassesJson = nlohmann::json::array();

                    for( const auto& netclass : netclassNames )
                    {
                        std::string netclassStr( netclass.ToUTF8() );
                        netclassesJson.push_back( std::move( netclassStr ) );
                    }

                    std::string key( netname.ToUTF8() );
                    ret[std::move( key )] = netclassesJson;
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                m_netClassLabelAssignments.clear();

                for( const auto& pair : aJson.items() )
                {
                    wxString key( pair.key().c_str(), wxConvUTF8 );

                    for( const auto& netclassName : pair.value() )
                        m_netClassLabelAssignments[key].insert( netclassName.get<wxString>() );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "netclass_patterns",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const auto& [matcher, netclassName] : m_netClassPatternAssignments )
                {
                    nlohmann::json pattern_json = {
                        { "pattern",  matcher->GetPattern().ToUTF8() },
                        { "netclass", netclassName.ToUTF8() }
                    };

                    ret.push_back( std::move( pattern_json ) );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_netClassPatternAssignments.clear();

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() )
                        continue;

                    if( entry.contains( "pattern" ) && entry["pattern"].is_string()
                            && entry.contains( "netclass" ) && entry["netclass"].is_string() )
                    {
                        wxString pattern = entry["pattern"].get<wxString>();
                        wxString netclass = entry["netclass"].get<wxString>();

                        // Expand bus patterns so individual bus member nets can be matched
                        ForEachBusMember( pattern,
                                          [&]( const wxString& memberPattern )
                                          {
                                              addSinglePatternAssignment( memberPattern, netclass );
                                          } );
                    }
                }
            },
            {} ) );

    registerMigration( 0, 1, std::bind( &NET_SETTINGS::migrateSchema0to1, this ) );
    registerMigration( 1, 2, std::bind( &NET_SETTINGS::migrateSchema1to2, this ) );
    registerMigration( 2, 3, std::bind( &NET_SETTINGS::migrateSchema2to3, this ) );
    registerMigration( 3, 4, std::bind( &NET_SETTINGS::migrateSchema3to4, this ) );
    registerMigration( 4, 5, std::bind( &NET_SETTINGS::migrateSchema4to5, this ) );
}


NET_SETTINGS::~NET_SETTINGS()
{
    // Release early before destroying members
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


bool NET_SETTINGS::operator==( const NET_SETTINGS& aOther ) const
{
    if( !std::equal( std::begin( m_netClasses ), std::end( m_netClasses ),
                     std::begin( aOther.m_netClasses ) ) )
        return false;

    if( !std::equal( std::begin( m_netClassPatternAssignments ),
                     std::end( m_netClassPatternAssignments ),
                     std::begin( aOther.m_netClassPatternAssignments ) ) )
        return false;

    if( !std::equal( std::begin( m_netClassLabelAssignments ),
                     std::end( m_netClassLabelAssignments ),
                     std::begin( aOther.m_netClassLabelAssignments ) ) )
        return false;


    if( !std::equal( std::begin( m_netColorAssignments ), std::end( m_netColorAssignments ),
                     std::begin( aOther.m_netColorAssignments ) ) )
        return false;

    return true;
}


bool NET_SETTINGS::migrateSchema0to1()
{
    if( m_internals->contains( "classes" ) && m_internals->At( "classes" ).is_array() )
    {
        for( auto& netClass : m_internals->At( "classes" ).items() )
        {
            if( netClass.value().contains( "nets" ) && netClass.value()["nets"].is_array() )
            {
                nlohmann::json migrated = nlohmann::json::array();

                for( auto& net : netClass.value()["nets"].items() )
                    migrated.push_back( ConvertToNewOverbarNotation( net.value().get<wxString>() ) );

                netClass.value()["nets"] = migrated;
            }
        }
    }

    return true;
}


bool NET_SETTINGS::migrateSchema1to2()
{
    return true;
}


bool NET_SETTINGS::migrateSchema2to3()
{
    if( m_internals->contains( "classes" ) && m_internals->At( "classes" ).is_array() )
    {
        nlohmann::json patterns = nlohmann::json::array();

        for( auto& netClass : m_internals->At( "classes" ).items() )
        {
            if( netClass.value().contains( "name" )
                    && netClass.value().contains( "nets" )
                    && netClass.value()["nets"].is_array() )
            {
                wxString netClassName = netClass.value()["name"].get<wxString>();

                for( auto& net : netClass.value()["nets"].items() )
                {
                    nlohmann::json pattern_json = {
                        { "pattern",  net.value().get<wxString>() },
                        { "netclass", netClassName }
                    };

                    patterns.push_back( pattern_json );
                }
            }
        }

        m_internals->SetFromString( "netclass_patterns", patterns );
    }

    return true;
}


bool NET_SETTINGS::migrateSchema3to4()
{
    // Add priority field to netclasses
    if( m_internals->contains( "classes" ) && m_internals->At( "classes" ).is_array() )
    {
        int priority = 0;

        for( auto& netClass : m_internals->At( "classes" ).items() )
        {
            if( netClass.value()["name"].get<wxString>() == NETCLASS::Default )
                netClass.value()["priority"] = std::numeric_limits<int>::max();
            else
                netClass.value()["priority"] = priority++;
        }
    }

    // Move netclass assignments to a list
    if( m_internals->contains( "netclass_assignments" )
        && m_internals->At( "netclass_assignments" ).is_object() )
    {
        nlohmann::json migrated = {};

        for( const auto& pair : m_internals->At( "netclass_assignments" ).items() )
        {
            nlohmann::json netclassesJson = nlohmann::json::array();

            if( pair.value().get<wxString>() != wxEmptyString )
                netclassesJson.push_back( pair.value() );

            migrated[pair.key()] = netclassesJson;
        }

        m_internals->SetFromString( "netclass_assignments", migrated );
    }

    return true;
}


bool NET_SETTINGS::migrateSchema4to5()
{
    // Add tuning profile name field to netclasses
    if( m_internals->contains( "classes" ) && m_internals->At( "classes" ).is_array() )
    {
        const wxString emptyStr = "";

        for( auto& netClass : m_internals->At( "classes" ).items() )
            netClass.value()["tuning_profile"] = emptyStr.ToUTF8();
    }

    return true;
}


void NET_SETTINGS::SetDefaultNetclass( std::shared_ptr<NETCLASS> netclass )
{
    m_defaultNetClass = netclass;
}


std::shared_ptr<NETCLASS> NET_SETTINGS::GetDefaultNetclass()
{
    return m_defaultNetClass;
}


bool NET_SETTINGS::HasNetclass( const wxString& netclassName ) const
{
    return m_netClasses.find( netclassName ) != m_netClasses.end();
}


void NET_SETTINGS::SetNetclass( const wxString& netclassName, std::shared_ptr<NETCLASS>& netclass )
{
    m_netClasses[netclassName] = netclass;
}


void NET_SETTINGS::SetNetclasses( const std::map<wxString, std::shared_ptr<NETCLASS>>& netclasses )
{
    m_netClasses = netclasses;
    ClearAllCaches();
}


const std::map<wxString, std::shared_ptr<NETCLASS>>& NET_SETTINGS::GetNetclasses() const
{
    return m_netClasses;
}


const std::map<wxString, std::shared_ptr<NETCLASS>>& NET_SETTINGS::GetCompositeNetclasses() const
{
    return m_compositeNetClasses;
}


void NET_SETTINGS::ClearNetclasses()
{
    m_netClasses.clear();
    m_impicitNetClasses.clear();
    ClearAllCaches();
}


const std::map<wxString, std::set<wxString>>& NET_SETTINGS::GetNetclassLabelAssignments() const
{
    return m_netClassLabelAssignments;
}


void NET_SETTINGS::ClearNetclassLabelAssignments()
{
    m_netClassLabelAssignments.clear();
}


void NET_SETTINGS::ClearNetclassLabelAssignment( const wxString& netName )
{
    m_netClassLabelAssignments.erase( netName );
}


void NET_SETTINGS::SetNetclassLabelAssignment( const wxString&           netName,
                                               const std::set<wxString>& netclasses )
{
    m_netClassLabelAssignments[netName] = netclasses;
}


void NET_SETTINGS::AppendNetclassLabelAssignment( const wxString&           netName,
                                                  const std::set<wxString>& netclasses )
{
    m_netClassLabelAssignments[netName].insert( netclasses.begin(), netclasses.end() );
}


bool NET_SETTINGS::HasNetclassLabelAssignment( const wxString& netName ) const
{
    return m_netClassLabelAssignments.find( netName ) != m_netClassLabelAssignments.end();
}


void NET_SETTINGS::SetNetclassPatternAssignment( const wxString& pattern, const wxString& netclass )
{
    // Expand bus patterns (vector buses and bus groups) to individual member patterns.
    // This is necessary because the regex/wildcard matchers interpret brackets and braces
    // as special characters, not as bus notation.
    ForEachBusMember( pattern,
                      [&]( const wxString& memberPattern )
                      {
                          addSinglePatternAssignment( memberPattern, netclass );
                      } );

    ClearAllCaches();
}


void NET_SETTINGS::addSinglePatternAssignment( const wxString& pattern, const wxString& netclass )
{
    // Avoid exact duplicates - these shouldn't cause problems, due to later de-duplication
    // but they are unnecessary.
    for( auto& assignment : m_netClassPatternAssignments )
    {
        if( assignment.first->GetPattern() == pattern && assignment.second == netclass )
            return;
    }

    // No assignment, add a new one
    m_netClassPatternAssignments.push_back(
            { std::make_unique<EDA_COMBINED_MATCHER>( pattern, CTX_NETCLASS ), netclass } );
}


void NET_SETTINGS::SetNetclassPatternAssignments(
        std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>>&& netclassPatterns )
{
    m_netClassPatternAssignments = std::move( netclassPatterns );
    ClearAllCaches();
}


std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>>&
NET_SETTINGS::GetNetclassPatternAssignments()
{
    return m_netClassPatternAssignments;
}


void NET_SETTINGS::ClearNetclassPatternAssignments()
{
    m_netClassPatternAssignments.clear();
}


void NET_SETTINGS::ClearCacheForNet( const wxString& netName )
{
    if( m_effectiveNetclassCache.count( netName ) )
    {
        wxString compositeNetclassName = m_effectiveNetclassCache[netName]->GetName();
        m_compositeNetClasses.erase( compositeNetclassName );
        m_effectiveNetclassCache.erase( netName );
    }
}


void NET_SETTINGS::ClearAllCaches()
{
    m_effectiveNetclassCache.clear();
    m_compositeNetClasses.clear();
}


void NET_SETTINGS::SetNetColorAssignment( const wxString& netName, const KIGFX::COLOR4D& color )
{
    m_netColorAssignments[netName] = color;
}


const std::map<wxString, KIGFX::COLOR4D>& NET_SETTINGS::GetNetColorAssignments() const
{
    return m_netColorAssignments;
}


void NET_SETTINGS::ClearNetColorAssignments()
{
    m_netColorAssignments.clear();
}


bool NET_SETTINGS::HasEffectiveNetClass( const wxString& aNetName ) const
{
    return m_effectiveNetclassCache.count( aNetName ) > 0;
}


std::shared_ptr<NETCLASS> NET_SETTINGS::GetCachedEffectiveNetClass( const wxString& aNetName ) const
{
    return m_effectiveNetclassCache.at( aNetName );
}


std::shared_ptr<NETCLASS> NET_SETTINGS::GetEffectiveNetClass( const wxString& aNetName )
{
    // Lambda to fetch an explicit netclass. Returns a nullptr if not found
    auto getExplicitNetclass =
            [this]( const wxString& netclass ) -> std::shared_ptr<NETCLASS>
            {
                if( netclass == NETCLASS::Default )
                    return m_defaultNetClass;

                auto ii = m_netClasses.find( netclass );

                if( ii == m_netClasses.end() )
                    return {};
                else
                    return ii->second;
            };

    // Lambda to fetch or create an implicit netclass (defined with a label, but not configured)
    // These are needed as while they do not provide any netclass parameters, they do now appear in
    // DRC matching strings as an assigned netclass.
    auto getOrAddImplicitNetcless =
            [this]( const wxString& netclass ) -> std::shared_ptr<NETCLASS>
            {
                auto ii = m_impicitNetClasses.find( netclass );

                if( ii == m_impicitNetClasses.end() )
                {
                    std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( netclass, false );
                    nc->SetPriority( std::numeric_limits<int>::max() - 1 ); // Priority > default netclass
                    m_impicitNetClasses[netclass] = nc;
                    return nc;
                }
                else
                {
                    return ii->second;
                }
            };

    // <no net> is forced to be part of the default netclass.
    if( aNetName.IsEmpty() )
        return m_defaultNetClass;

    // First check if we have a cached resolved netclass
    auto cacheItr = m_effectiveNetclassCache.find( aNetName );

    if( cacheItr != m_effectiveNetclassCache.end() )
        return cacheItr->second;

    // No cache found - build a vector of all netclasses assigned to or matching this net
    std::unordered_set<std::shared_ptr<NETCLASS>> resolvedNetclasses;

    // First find explicit netclass assignments
    auto it = m_netClassLabelAssignments.find( aNetName );

    if( it != m_netClassLabelAssignments.end() && it->second.size() > 0 )
    {
        for( const wxString& netclassName : it->second )
        {
            std::shared_ptr<NETCLASS> netclass = getExplicitNetclass( netclassName );

            if( netclass )
            {
                resolvedNetclasses.insert( std::move( netclass ) );
            }
            else
            {
                resolvedNetclasses.insert( getOrAddImplicitNetcless( netclassName ) );
            }
        }
    }

    // Now find any pattern-matched netclass assignments
    for( const auto& [matcher, netclassName] : m_netClassPatternAssignments )
    {
        if( matcher->StartsWith( aNetName ) )
        {
            std::shared_ptr<NETCLASS> netclass = getExplicitNetclass( netclassName );

            if( netclass )
            {
                resolvedNetclasses.insert( std::move( netclass ) );
            }
            else
            {
                resolvedNetclasses.insert( getOrAddImplicitNetcless( netclassName ) );
            }
        }
    }

    // Handle zero resolved netclasses
    if( resolvedNetclasses.size() == 0 )
    {
        // For bus patterns, check if all members share the same netclass.
        // If they do, the bus inherits that netclass for coloring purposes.
        std::shared_ptr<NETCLASS> sharedNetclass;
        bool                      allSameNetclass = true;
        bool                      isBusPattern = false;

        ForEachBusMember( aNetName,
                          [&]( const wxString& member )
                          {
                              // If ForEachBusMember gives us back the same name, it's not a bus.
                              // Skip to avoid infinite recursion.
                              if( member == aNetName )
                                  return;

                              isBusPattern = true;

                              if( !allSameNetclass )
                                  return;

                              std::shared_ptr<NETCLASS> memberNc = GetEffectiveNetClass( member );

                              if( !sharedNetclass )
                              {
                                  sharedNetclass = memberNc;
                              }
                              else if( memberNc->GetName() != sharedNetclass->GetName() )
                              {
                                  allSameNetclass = false;
                              }
                          } );

        if( isBusPattern && allSameNetclass && sharedNetclass
            && sharedNetclass->GetName() != NETCLASS::Default )
        {
            m_effectiveNetclassCache[aNetName] = sharedNetclass;
            return sharedNetclass;
        }

        m_effectiveNetclassCache[aNetName] = m_defaultNetClass;

        return m_defaultNetClass;
    }

    // Make and cache the effective netclass. Note that makeEffectiveNetclass will add the default
    // netclass to resolvedNetclasses if it is needed to complete the netclass paramters set. It
    // will also sort resolvedNetclasses by priority order.
    std::vector<NETCLASS*> netclassPtrs;

    for( const std::shared_ptr<NETCLASS>& nc : resolvedNetclasses )
        netclassPtrs.push_back( nc.get() );

    wxString name;
    name.Printf( "Effective for net: %s", aNetName );
    std::shared_ptr<NETCLASS> effectiveNetclass = std::make_shared<NETCLASS>( name, false );
    makeEffectiveNetclass( effectiveNetclass, netclassPtrs );

    if( netclassPtrs.size() == 1 )
    {
        // No defaults were added - just return the primary netclass
        m_effectiveNetclassCache[aNetName] = *resolvedNetclasses.begin();
        return *resolvedNetclasses.begin();
    }
    else
    {
        effectiveNetclass->SetConstituentNetclasses( std::move( netclassPtrs ) );

        m_compositeNetClasses[effectiveNetclass->GetName()] = effectiveNetclass;
        m_effectiveNetclassCache[aNetName] = effectiveNetclass;

        return effectiveNetclass;
    }
}


void NET_SETTINGS::RecomputeEffectiveNetclasses()
{
    for( auto& [ncName, nc] : m_compositeNetClasses )
    {
        // Note this needs to be a copy in case we now need to add the default netclass
        std::vector<NETCLASS*> constituents = nc->GetConstituentNetclasses();

        wxASSERT( constituents.size() > 0 );

        // If the last netclass is Default, remove it (it will be re-added if still needed)
        if( ( *constituents.rbegin() )->GetName() == NETCLASS::Default )
        {
            constituents.pop_back();
        }

        // Remake the netclass from original constituents
        nc->ResetParameters();
        makeEffectiveNetclass( nc, constituents );
        nc->SetConstituentNetclasses( std::move( constituents ) );
    }
}


void NET_SETTINGS::makeEffectiveNetclass( std::shared_ptr<NETCLASS>& effectiveNetclass,
                                          std::vector<NETCLASS*>&    constituentNetclasses ) const
{
    // Sort the resolved netclasses by priority (highest first), with same-priority netclasses
    // ordered alphabetically
    std::sort( constituentNetclasses.begin(), constituentNetclasses.end(),
               []( NETCLASS* nc1, NETCLASS* nc2 )
               {
                   int p1 = nc1->GetPriority();
                   int p2 = nc2->GetPriority();

                   if( p1 < p2 )
                       return true;

                   if (p1 == p2)
                       return nc1->GetName().Cmp( nc2->GetName() ) < 0;

                   return false;
               } );

    // Iterate from lowest priority netclass and fill effective netclass parameters
    for( auto itr = constituentNetclasses.rbegin(); itr != constituentNetclasses.rend(); ++itr )
    {
        NETCLASS* nc = *itr;

        if( nc->HasClearance() )
        {
            effectiveNetclass->SetClearance( nc->GetClearance() );
            effectiveNetclass->SetClearanceParent( nc );
        }

        if( nc->HasTrackWidth() )
        {
            effectiveNetclass->SetTrackWidth( nc->GetTrackWidth() );
            effectiveNetclass->SetTrackWidthParent( nc );
        }

        if( nc->HasViaDiameter() )
        {
            effectiveNetclass->SetViaDiameter( nc->GetViaDiameter() );
            effectiveNetclass->SetViaDiameterParent( nc );
        }

        if( nc->HasViaDrill() )
        {
            effectiveNetclass->SetViaDrill( nc->GetViaDrill() );
            effectiveNetclass->SetViaDrillParent( nc );
        }

        if( nc->HasuViaDiameter() )
        {
            effectiveNetclass->SetuViaDiameter( nc->GetuViaDiameter() );
            effectiveNetclass->SetuViaDiameterParent( nc );
        }

        if( nc->HasuViaDrill() )
        {
            effectiveNetclass->SetuViaDrill( nc->GetuViaDrill() );
            effectiveNetclass->SetuViaDrillParent( nc );
        }

        if( nc->HasDiffPairWidth() )
        {
            effectiveNetclass->SetDiffPairWidth( nc->GetDiffPairWidth() );
            effectiveNetclass->SetDiffPairWidthParent( nc );
        }

        if( nc->HasDiffPairGap() )
        {
            effectiveNetclass->SetDiffPairGap( nc->GetDiffPairGap() );
            effectiveNetclass->SetDiffPairGapParent( nc );
        }

        if( nc->HasDiffPairViaGap() )
        {
            effectiveNetclass->SetDiffPairViaGap( nc->GetDiffPairViaGap() );
            effectiveNetclass->SetDiffPairViaGapParent( nc );
        }

        if( nc->HasWireWidth() )
        {
            effectiveNetclass->SetWireWidth( nc->GetWireWidth() );
            effectiveNetclass->SetWireWidthParent( nc );
        }

        if( nc->HasBusWidth() )
        {
            effectiveNetclass->SetBusWidth( nc->GetBusWidth() );
            effectiveNetclass->SetBusWidthParent( nc );
        }

        if( nc->HasLineStyle() )
        {
            effectiveNetclass->SetLineStyle( nc->GetLineStyle() );
            effectiveNetclass->SetLineStyleParent( nc );
        }

        COLOR4D pcbColor = nc->GetPcbColor();

        if( pcbColor != COLOR4D::UNSPECIFIED )
        {
            effectiveNetclass->SetPcbColor( pcbColor );
            effectiveNetclass->SetPcbColorParent( nc );
        }

        COLOR4D schColor = nc->GetSchematicColor();

        if( schColor != COLOR4D::UNSPECIFIED )
        {
            effectiveNetclass->SetSchematicColor( schColor );
            effectiveNetclass->SetSchematicColorParent( nc );
        }

        if( nc->HasTuningProfile() )
        {
            effectiveNetclass->SetTuningProfile( nc->GetTuningProfile() );
            effectiveNetclass->SetTuningProfileParent( nc );
        }
    }

    // Fill in any required defaults
    if( addMissingDefaults( effectiveNetclass.get() ) )
        constituentNetclasses.push_back( m_defaultNetClass.get() );
}


bool NET_SETTINGS::addMissingDefaults( NETCLASS* nc ) const
{
    bool addedDefault = false;

    if( !nc->HasClearance() )
    {
        addedDefault = true;
        nc->SetClearance( m_defaultNetClass->GetClearance() );
        nc->SetClearanceParent( m_defaultNetClass.get() );
    }

    if( !nc->HasTrackWidth() )
    {
        addedDefault = true;
        nc->SetTrackWidth( m_defaultNetClass->GetTrackWidth() );
        nc->SetTrackWidthParent( m_defaultNetClass.get() );
    }

    if( !nc->HasViaDiameter() )
    {
        addedDefault = true;
        nc->SetViaDiameter( m_defaultNetClass->GetViaDiameter() );
        nc->SetViaDiameterParent( m_defaultNetClass.get() );
    }

    if( !nc->HasViaDrill() )
    {
        addedDefault = true;
        nc->SetViaDrill( m_defaultNetClass->GetViaDrill() );
        nc->SetViaDrillParent( m_defaultNetClass.get() );
    }

    if( !nc->HasuViaDiameter() )
    {
        addedDefault = true;
        nc->SetuViaDiameter( m_defaultNetClass->GetuViaDiameter() );
        nc->SetuViaDiameterParent( m_defaultNetClass.get() );
    }

    if( !nc->HasuViaDrill() )
    {
        addedDefault = true;
        nc->SetuViaDrill( m_defaultNetClass->GetuViaDrill() );
        nc->SetuViaDrillParent( m_defaultNetClass.get() );
    }

    if( !nc->HasDiffPairWidth() )
    {
        addedDefault = true;
        nc->SetDiffPairWidth( m_defaultNetClass->GetDiffPairWidth() );
        nc->SetDiffPairWidthParent( m_defaultNetClass.get() );
    }

    if( !nc->HasDiffPairGap() )
    {
        addedDefault = true;
        nc->SetDiffPairGap( m_defaultNetClass->GetDiffPairGap() );
        nc->SetDiffPairGapParent( m_defaultNetClass.get() );
    }

    // Currently this is only on the default netclass, and not editable in the setup panel
    // if( !nc->HasDiffPairViaGap() )
    // {
    //     addedDefault = true;
    //     nc->SetDiffPairViaGap( m_defaultNetClass->GetDiffPairViaGap() );
    //     nc->SetDiffPairViaGapParent( m_defaultNetClass.get() );
    // }

    if( !nc->HasWireWidth() )
    {
        addedDefault = true;
        nc->SetWireWidth( m_defaultNetClass->GetWireWidth() );
        nc->SetWireWidthParent( m_defaultNetClass.get() );
    }

    if( !nc->HasBusWidth() )
    {
        addedDefault = true;
        nc->SetBusWidth( m_defaultNetClass->GetBusWidth() );
        nc->SetBusWidthParent( m_defaultNetClass.get() );
    }

    // The tuning profile can be empty - only fill if a default tuning profile is set
    if( !nc->HasTuningProfile() && m_defaultNetClass->HasTuningProfile() )
    {
        addedDefault = true;
        nc->SetTuningProfile( m_defaultNetClass->GetTuningProfile() );
        nc->SetTuningProfileParent( m_defaultNetClass.get() );
    }

    return addedDefault;
}


std::shared_ptr<NETCLASS> NET_SETTINGS::GetNetClassByName( const wxString& aNetClassName ) const
{
    auto ii = m_netClasses.find( aNetClassName );

    if( ii == m_netClasses.end() )
        return m_defaultNetClass;
    else
        return ii->second;
}


static bool isSuperSubOverbar( wxChar c )
{
    return c == '_' || c == '^' || c == '~';
}


bool NET_SETTINGS::ParseBusVector( const wxString& aBus, wxString* aName,
                                   std::vector<wxString>* aMemberList )
{
    auto isDigit =
            []( wxChar c )
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
            if( i > 0 && isSuperSubOverbar( aBus[i-1] ) )
            {
                braceNesting++;

                if( !prefix.IsEmpty() )
                    prefix.RemoveLast();

                continue;
            }
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
        }
        else if( aBus[i] == '+' || aBus[i] == '-' || aBus[i] == 'P' || aBus[i] == 'N' )
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


bool NET_SETTINGS::ParseBusGroup( const wxString& aGroup, wxString* aName,
                                  std::vector<wxString>* aMemberList )
{
    size_t   groupLen = aGroup.length();
    size_t   i = 0;
    wxString prefix;
    wxString tmp;
    int      braceNesting = 0;

    prefix.reserve( groupLen );

    // Parse prefix
    //
    for( ; i < groupLen; ++i )
    {
        if( aGroup[i] == '{' )
        {
            if( i > 0 && isSuperSubOverbar( aGroup[i-1] ) )
            {
                braceNesting++;

                if( !prefix.IsEmpty() )
                    prefix.RemoveLast();

                continue;
            }
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
            if( i > 0 && isSuperSubOverbar( aGroup[i-1] ) )
            {
                braceNesting++;

                if( !tmp.IsEmpty() )
                    tmp.RemoveLast();

                continue;
            }
            else
                return false;
        }
        else if( aGroup[i] == '}' )
        {
            if( braceNesting )
            {
                braceNesting--;
                continue;
            }
            else
            {
                if( aMemberList && !tmp.IsEmpty() )
                    aMemberList->push_back( EscapeString( tmp, CTX_NETNAME ) );

                return true;
            }
        }

        // Commas aren't strictly legal, but we can be pretty sure what the author had in mind.
        if( aGroup[i] == ' ' || aGroup[i] == ',' )
        {
            if( aMemberList && !tmp.IsEmpty() )
                aMemberList->push_back( EscapeString( tmp, CTX_NETNAME ) );

            tmp.Clear();
            continue;
        }

        tmp += aGroup[i];
    }

    return false;
}


void NET_SETTINGS::ForEachBusMember( const wxString&                              aBusPattern,
                                     const std::function<void( const wxString& )>& aFunction )
{
    std::vector<wxString> members;

    if( ParseBusVector( aBusPattern, nullptr, &members ) )
    {
        // Vector bus: call function for each expanded member
        for( const wxString& member : members )
            aFunction( member );
    }
    else if( ParseBusGroup( aBusPattern, nullptr, &members ) )
    {
        // Bus group: recursively expand each member (which may itself be a vector or group)
        for( const wxString& member : members )
            ForEachBusMember( member, aFunction );
    }
    else
    {
        // Not a bus pattern: call function with the original pattern
        aFunction( aBusPattern );
    }
}
