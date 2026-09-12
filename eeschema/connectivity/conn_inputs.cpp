/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conn_text.h"
#include "conn_inputs.h"
#include "conn_tasks.h"

#include <sch_screen.h>
#include <schematic.h>
#include <project.h>
#include <sch_sheet_path.h>
#include <git/project_git_utils.h>
#include <text_eval/text_eval_vcs.h>
#include <stdexcept>
#include <wx/thread.h>
#include <algorithm>
#include <cassert>

namespace SCH_CONNECTIVITY
{
namespace
{
    template <typename ROW, typename TABLE, typename KEY_OF>
    void ReplaceRows( const std::vector<ROW>& aPrevious, const std::vector<ROW>& aCurrent, TABLE& aTable,
                      KEY_OF aKeyOf )
    {
        [[maybe_unused]] const auto unordered = []( const ROW& a, const ROW& b )
        {
            return !( a.id < b.id );
        };
        assert( std::adjacent_find( aPrevious.begin(), aPrevious.end(), unordered ) == aPrevious.end() );
        assert( std::adjacent_find( aCurrent.begin(), aCurrent.end(), unordered ) == aCurrent.end() );

        auto previous = aPrevious.begin();

        for( const ROW& row : aCurrent )
        {
            for( ; previous != aPrevious.end() && previous->id < row.id; ++previous )
                aTable.Erase( aKeyOf( previous->id ) );

            const bool matched = previous != aPrevious.end() && previous->id == row.id;

            if( !matched || !( *previous == row ) )
                aTable.Set( aKeyOf( row.id ), row );

            if( matched )
                ++previous;
        }

        for( ; previous != aPrevious.end(); ++previous )
            aTable.Erase( aKeyOf( previous->id ) );
    }
} // namespace

INPUT_STORE::INPUT_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys ) :
        m_keys( aKeys ),
        m_screens( aVersions ),
        m_facts( aVersions ),
        m_instances( aVersions ),
        m_text( aVersions, KEY_LESS{ aKeys } ),
        m_areas( aVersions, KEY_LESS{ aKeys } ),
        m_geometry( aVersions ),
        m_islands( aVersions )
{
}

bool INPUT_STORE::ExternalSourcesChanged( const SCH_SHEET_LIST& aPaths, bool aContextUnchanged ) const
{
    wxASSERT( wxThread::IsMain() );
    INPUT_TEXT_SCOPE frame;

    if( TEXT_EVAL::ENVIRONMENT::Current()->IsCollectingSources() )
        throw std::logic_error( "Cannot compare text sources during source collection" );

    const bool checkReferences = !aContextUnchanged || std::ranges::any_of( aPaths,
            [&]( const SCH_SHEET_PATH& path )
            {
                const SCH_SCREEN* screen = path.LastScreen();

                if( !screen )
                    return false;

                const auto previous = m_screenRevisions.find( screen->ConnectivityId() );
                return previous == m_screenRevisions.end() || previous->second != screen->ConnectivityRevision();
            } );

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        const auto instance = m_keys.FindInstance( path.Path() );

        if( !instance || !path.LastScreen() )
            continue;

        const auto input = m_instanceInputs.find( *instance );

        if( input == m_instanceInputs.end() || input->second.screen != path.LastScreen()->ConnectivityId() )
            continue;

        const auto* sources = Sources( *instance );

        if( !sources )
            continue;

        for( const auto& [name, previous] : sources->environmentVariables )
        {
            wxString value;
            const bool found = wxGetEnv( name, &value );

            if( found != previous.has_value() || ( found && value != *previous ) )
                return true;
        }

        if( sources->randomUsed )
            return true;

        if( sources->time && *sources->time != TEXT_EVAL::ENVIRONMENT::CurrentTime() )
            return true;

        if( checkReferences && !sources->crossReferences.empty() )
        {
            const SCHEMATIC* schematic = path.LastScreen()->Schematic();

            if( !schematic )
                return true;

            std::optional<TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE> vcs;

            if( schematic->IsValid() )
            {
                if( const wxString projectPath = schematic->Project().GetProjectPath(); !projectPath.IsEmpty() )
                    vcs.emplace( projectPath );
            }

            for( const auto& [key, value] : sources->crossReferences )
            {
                wxString text = key.first;
                const bool resolved = schematic->ResolveCrossReference( &text, key.second );

                if( value != TEXT_EVAL::ENVIRONMENT::CROSS_REFERENCE_VALUE{ text, resolved } )
                    return true;
            }
        }

        for( const auto& [file, hash] : sources->gitHashes )
        {
            const auto& current = TEXT_EVAL::ENVIRONMENT::Current()->GitHash( file, [&]
            {
                return KIGIT::PROJECT_GIT_UTILS::GetCurrentHash( file, false );
            } );

            if( current != hash )
                return true;
        }

        for( const auto& [key, value] : sources->vcsValues )
        {
            const auto& current = TEXT_EVAL::ENVIRONMENT::Current()->VcsValue( key, [&]
            {
                return TEXT_EVAL_VCS::ReadSource( key );
            } );

            if( current != value )
                return true;
        }
    }

    return false;
}

const INPUT_STORE::SCREEN_CACHE::ENTRY& INPUT_STORE::Screen( const SCH_SCREEN& aScreen )
{
    wxASSERT( wxThread::IsMain() );
    const SCREEN_ID id = aScreen.ConnectivityId();
    const uint64_t  revision = aScreen.ConnectivityRevision();
    const auto      stamp = m_screenRevisions.find( id );

    if( stamp != m_screenRevisions.end() && stamp->second == revision )
        return *m_screens.Find( id );

    const auto symbolStamp = m_symbolRevisions.find( id );
    const auto* previous = m_screens.Find( id );
    const bool unchanged = previous && symbolStamp != m_symbolRevisions.end()
                           && symbolStamp->second == aScreen.ConnectivitySymbolRevision();
    const auto libraries = unchanged ? previous->value.librarySymbols : nullptr;
    return storeScreen( aScreen, ExtractScreenFacts( aScreen, libraries, unchanged ? &previous->value : nullptr ) );
}

const INPUT_STORE::SCREEN_CACHE::ENTRY& INPUT_STORE::storeScreen( const SCH_SCREEN& aScreen, SCREEN_FACTS aFacts )
{
    const SCREEN_ID     id = aScreen.ConnectivityId();
    const uint64_t      revision = aScreen.ConnectivityRevision();
    const auto*         previous = m_screens.Find( id );
    const SCREEN_FACTS  empty;
    const SCREEN_FACTS& old = previous ? previous->value : empty;
    const auto          keyOf = [&]( const KIID& item )
    {
        return SOURCE_KEY{ id, item };
    };
    try
    {
        ReplaceRows( old.items, aFacts.items, m_facts, keyOf );
        const auto& result = m_screens.Set( id, std::move( aFacts ) );
        m_screenRevisions.insert_or_assign( id, revision );
        m_symbolRevisions.insert_or_assign( id, aScreen.ConnectivitySymbolRevision() );
        return result;
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

const INPUT_STORE::INSTANCE_CACHE::ENTRY& INPUT_STORE::Instance( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath,
                                                                uint64_t aTextEpoch )
{
    wxASSERT( wxThread::IsMain() );

    if( aPath.LastScreen() != &aScreen )
        throw std::invalid_argument( "Connectivity instance does not refer to source screen" );

    const INST_ID        instance = m_keys.InternInstance( aPath.Path() );
    const SCREEN_FACTS&  source = Screen( aScreen ).value;
    const INSTANCE_INPUT input{ aScreen.ConnectivityId(), aScreen.ConnectivityRevision(), aTextEpoch,
                                aPath.GetVirtualPageNumber(), m_symbolRevisions.at( aScreen.ConnectivityId() ) };
    const auto           stamp = m_instanceInputs.find( instance );

    if( stamp != m_instanceInputs.end() && stamp->second == input )
        return *m_instances.Find( instance );

    const auto* previous = m_instances.Find( instance );

    // Wire-only edits keep instance text while the screen's symbol revision is unchanged
    if( previous && stamp != m_instanceInputs.end()
        && stamp->second.symbolRevision == input.symbolRevision && stamp->second.screen == input.screen
        && stamp->second.textEpoch == input.textEpoch && stamp->second.pageOrder == input.pageOrder
        && previous->value.ruleAreas.empty() && source.ruleAreas.empty() )
    {
        stamp->second = input;
        return *previous;
    }

    INSTANCE_FACTS        facts;
    TEXT_EVAL::ENVIRONMENT::SOURCE_VALUES sources;

    {
        INPUT_TEXT_SCOPE inputText;
        TEXT_EVAL::SOURCE_SCOPE sourceScope( *TEXT_EVAL::ENVIRONMENT::Current(), sources );
        facts = ExtractInstanceFacts( source, aScreen, aPath );
    }

    const INSTANCE_FACTS  empty;
    const INSTANCE_FACTS& old = previous ? previous->value : empty;
    const auto            keyOf = [&]( const KIID& item )
    {
        return ITEM_KEY{ item, instance };
    };
    try
    {
        ReplaceRows( old.items, facts.items, m_text, keyOf );
        ReplaceRows( old.ruleAreas, facts.ruleAreas, m_areas, keyOf );
        const auto& result = m_instances.Set( instance, std::move( facts ) );
        m_instanceSources.insert_or_assign( instance, std::move( sources ) );
        m_instanceInputs.insert_or_assign( instance, input );
        return result;
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

const INPUT_STORE::GEOMETRY_CACHE::ENTRY& INPUT_STORE::Geometry( SCREEN_ID aScreen )
{
    wxASSERT( wxThread::IsMain() );
    const auto* source = m_screens.Find( aScreen );

    if( !source )
        throw std::invalid_argument( "Connectivity geometry requires captured screen facts" );

    const auto stamp = m_geometryInputs.find( aScreen );

    if( stamp != m_geometryInputs.end() && stamp->second == source->version )
        return *m_geometry.Find( aScreen );

    SCREEN_GEOMETRY geometry = GeometryOf( source->value );

    try
    {
        const auto& result = m_geometry.Set( aScreen, std::move( geometry ) );
        m_geometryInputs.insert_or_assign( aScreen, source->version );
        return result;
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

const INPUT_STORE::ISLAND_CACHE::ENTRY& INPUT_STORE::Islands( SCREEN_ID aScreen, const UNIT_SIGNATURE& aUnits )
{
    wxASSERT( wxThread::IsMain() );

    const auto&         geometry = Geometry( aScreen );
    const ISLAND_LOOKUP lookup{ aScreen, aUnits };
    const auto          stamp = m_islandInputs.find( lookup );

    if( stamp != m_islandInputs.end() && stamp->second == geometry.version )
        return *m_islands.Find( lookup );

    SCREEN_ISLANDS islands = BuildScreenIslands( geometry.value, aUnits );

    try
    {
        const ISLAND_KEY key{ aScreen, aUnits };
        const auto&      result = m_islands.Set( key, std::move( islands ) );
        m_islandInputs.insert_or_assign( key, geometry.version );
        return result;
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

std::vector<FRAME_INSTANCE> INPUT_STORE::Capture( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch )
{
    wxASSERT( wxThread::IsMain() );

    INPUT_TEXT_SCOPE inputText;
    auto frame = CaptureHierarchy( aPaths, m_keys );
    std::map<INST_ID, const SCH_SHEET_PATH*> paths;

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        if( !path.empty() && path.LastScreen() )
            paths.emplace( m_keys.InternInstance( path.Path() ), &path );
    }

    std::set<SCREEN_ID> screens;
    std::set<INST_ID>   instances;

    for( const FRAME_INSTANCE& item : frame )
    {
        const SCH_SHEET_PATH& path = *paths.at( item.scope.instance );
        Instance( *path.LastScreen(), path, aTextEpoch );
        screens.insert( item.screen );
        instances.insert( item.scope.instance );
    }

    struct ISLAND_JOB
    {
        ISLAND_KEY                   key;
        const GEOMETRY_CACHE::ENTRY* geometry;
    };
    std::vector<ISLAND_JOB> jobs;
    std::set<ISLAND_KEY, ISLAND_LESS> seen;

    for( const FRAME_INSTANCE& item : frame )
    {
        const auto& units = m_instances.Find( item.scope.instance )->value.units;
        ISLAND_KEY key{ item.screen, units };

        if( !seen.insert( key ).second )
            continue;

        const auto& geometry = Geometry( item.screen );
        const auto stamp = m_islandInputs.find( key );

        if( stamp == m_islandInputs.end() || stamp->second != geometry.version )
            jobs.push_back( { std::move( key ), &geometry } );
    }

    std::vector<SCREEN_ISLANDS> results( jobs.size() );

    try
    {
        ParallelFor( jobs.size(), [&]( size_t i )
        {
            results[i] = BuildScreenIslands( jobs[i].geometry->value, jobs[i].key.second );
        } );

        for( size_t i = 0; i < jobs.size(); ++i )
        {
            m_islands.Set( jobs[i].key, std::move( results[i] ) );
            m_islandInputs.insert_or_assign( jobs[i].key, jobs[i].geometry->version );
        }
    }
    catch( ... )
    {
        Clear();
        throw;
    }

    Retain( screens, instances );
    return frame;
}


void INPUT_STORE::Invalidate()
{
    wxASSERT( wxThread::IsMain() );
    m_screenRevisions.clear();
    m_symbolRevisions.clear();
    m_instanceInputs.clear();
    m_instanceSources.clear();
    m_geometry.Clear();
    m_islands.Clear();
    m_geometryInputs.clear();
    m_islandInputs.clear();
}

void INPUT_STORE::Clear()
{
    Invalidate();
    m_screens.Clear();
    m_facts.Clear();
    m_instances.Clear();
    m_text.Clear();
    m_areas.Clear();
}

void INPUT_STORE::Retain( const std::set<SCREEN_ID>& aScreens, const std::set<INST_ID>& aInstances )
{
    wxASSERT( wxThread::IsMain() );
    // Views refer only to retained instances; their unit vectors remain unchanged
    std::set<ISLAND_LOOKUP, ISLAND_LESS> liveIslands;

    for( auto it = m_instances.Entries().begin(); it != m_instances.Entries().end(); )
    {
        const INST_ID id = it->first;
        const auto    input = m_instanceInputs.find( id );

        if( aInstances.contains( id ) && input != m_instanceInputs.end() && aScreens.contains( input->second.screen ) )
        {
            liveIslands.emplace( input->second.screen, it->second->value.units );
            ++it;
            continue;
        }

        for( const auto& row : it->second->value.items )
            m_text.Erase( { row.id, id } );

        for( const auto& row : it->second->value.ruleAreas )
            m_areas.Erase( { row.id, id } );

        ++it;
        m_instances.Erase( id );
        m_instanceInputs.erase( id );
        m_instanceSources.erase( id );
    }

    for( auto it = m_screens.Entries().begin(); it != m_screens.Entries().end(); )
    {
        const SCREEN_ID id = it->first;

        if( aScreens.contains( id ) )
        {
            ++it;
            continue;
        }

        for( const auto& row : it->second->value.items )
            m_facts.Erase( { id, row.id } );

        ++it;
        m_screens.Erase( id );
        m_screenRevisions.erase( id );
        m_symbolRevisions.erase( id );
        m_geometry.Erase( id );
        m_geometryInputs.erase( id );
    }

    for( auto it = m_islands.Entries().begin(); it != m_islands.Entries().end(); )
    {
        if( liveIslands.contains( it->first ) )
        {
            ++it;
            continue;
        }

        const auto removed = it++;
        m_islandInputs.erase( removed->first );
        m_islands.Erase( removed->first );
    }
}
} // namespace SCH_CONNECTIVITY
