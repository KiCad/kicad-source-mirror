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
 */

#include <text_var_dependency.h>

#include <mutex>
#include <functional>
#include <utility>


namespace
{
// Title-block field names are defined in common/title_block.cpp and documented
// in the Page Settings dialog. Keeping the list here (rather than pulling in
// the title_block header) avoids a circular dependency — this module lives in
// kicommon and title_block.cpp does too, but the field names are stable ABI.
bool isTitleBlockField( const wxString& aName )
{
    return aName == wxT( "ISSUE_DATE" ) || aName == wxT( "REVISION" )
        || aName == wxT( "TITLE" ) || aName == wxT( "COMPANY" )
        || aName == wxT( "COMMENT1" ) || aName == wxT( "COMMENT2" )
        || aName == wxT( "COMMENT3" ) || aName == wxT( "COMMENT4" )
        || aName == wxT( "COMMENT5" ) || aName == wxT( "COMMENT6" )
        || aName == wxT( "COMMENT7" ) || aName == wxT( "COMMENT8" )
        || aName == wxT( "COMMENT9" ) || aName == wxT( "PROJECTNAME" )
        || aName == wxT( "PAPER" ) || aName == wxT( "KICAD_VERSION" );
}


// Context-sensitive specials resolved by SCHEMATIC/BOARD/SCH_SHEET themselves,
// not by the project. They cannot be driven by a listener — see TRACKER
// routing rules.
bool isSpecial( const wxString& aName )
{
    return aName == wxT( "SHEETNAME" ) || aName == wxT( "SHEETPATH" )
        || aName == wxT( "FILENAME" ) || aName == wxT( "FILEPATH" )
        || aName == wxT( "#" ) || aName == wxT( "##" )
        || aName == wxT( "CURRENT_DATE" ) || aName == wxT( "LAYER" )
        || aName == wxT( "KICAD_VERSION" );
}
}


TEXT_VAR_REF_KEY TEXT_VAR_REF_KEY::FromToken( const wxString& aToken )
{
    TEXT_VAR_REF_KEY key;

    const int colonPos = aToken.Find( ':' );
    const bool hasColon = colonPos != wxNOT_FOUND && colonPos > 0
                          && colonPos < static_cast<int>( aToken.length() ) - 1;

    if( hasColon )
    {
        const wxString left = aToken.Left( colonPos );
        const wxString right = aToken.Mid( colonPos + 1 );

        // `${OP}` with `:port` suffix (SPICE operating points). Not listener-
        // driven; kept categorized so callers can skip registration.
        if( left == wxT( "OP" ) )
        {
            key.kind = KIND::OP;
            key.primary = left;
            key.secondary = right;
        }
        else
        {
            key.kind = KIND::CROSS_REF;
            key.primary = left;
            key.secondary = right;
        }

        return key;
    }

    // Bare `${OP}` (no port) also categorized as OP.
    if( aToken == wxT( "OP" ) )
    {
        key.kind = KIND::OP;
        key.primary = aToken;
        return key;
    }

    if( isTitleBlockField( aToken ) )
    {
        key.kind = KIND::TITLE_BLOCK;
        key.primary = aToken;
        return key;
    }

    if( isSpecial( aToken ) )
    {
        key.kind = KIND::SPECIAL;
        key.primary = aToken;
        return key;
    }

    // Default: LOCAL. The listener adapter may promote to PROJECT_VAR or
    // ENV_VAR at registration time based on whether the token appears in
    // PROJECT::GetTextVars() or ENV_VAR::GetPredefinedEnvVars().
    key.kind = KIND::LOCAL;
    key.primary = aToken;
    return key;
}


std::vector<TEXT_VAR_REF_KEY> FilterTrackable( const std::vector<TEXT_VAR_REF_KEY>& aRefs )
{
    std::vector<TEXT_VAR_REF_KEY> out;
    out.reserve( aRefs.size() );

    for( const TEXT_VAR_REF_KEY& ref : aRefs )
    {
        if( ref.IsTrackable() )
            out.push_back( ref );
    }

    return out;
}


std::size_t TEXT_VAR_REF_KEY_HASH::operator()( const TEXT_VAR_REF_KEY& aKey ) const
{
    std::hash<std::string> hasher;

    auto mix = []( std::size_t& aSeed, std::size_t aValue )
    {
        // boost::hash_combine pattern — good avalanche across field boundaries.
        aSeed ^= aValue + 0x9E3779B97F4A7C15ULL + ( aSeed << 6 ) + ( aSeed >> 2 );
    };

    // std::hash<wxString> is not universally specialized across platforms;
    // route through utf8 std::string for portability.
    std::size_t h = 0;
    mix( h, static_cast<std::size_t>( aKey.kind ) );
    mix( h, hasher( aKey.primary.ToStdString( wxConvUTF8 ) ) );
    mix( h, hasher( aKey.secondary.ToStdString( wxConvUTF8 ) ) );
    return h;
}


void TEXT_VAR_DEPENDENCY_INDEX::Register( EDA_ITEM*                            aItem,
                                          const std::vector<TEXT_VAR_REF_KEY>& aKeys )
{
    if( !aItem )
        return;

    std::unique_lock lock( m_mutex );

    if( auto it = m_itemKeys.find( aItem ); it != m_itemKeys.end() )
    {
        for( const TEXT_VAR_REF_KEY& oldKey : it->second )
        {
            if( auto depIt = m_dependents.find( oldKey ); depIt != m_dependents.end() )
            {
                depIt->second.erase( aItem );

                if( depIt->second.empty() )
                    m_dependents.erase( depIt );
            }
        }

        m_itemKeys.erase( it );
    }

    if( aKeys.empty() )
        return;

    for( const TEXT_VAR_REF_KEY& key : aKeys )
        m_dependents[key].insert( aItem );

    m_itemKeys[aItem] = aKeys;
}


void TEXT_VAR_DEPENDENCY_INDEX::Unregister( EDA_ITEM* aItem )
{
    if( !aItem )
        return;

    std::unique_lock lock( m_mutex );

    auto it = m_itemKeys.find( aItem );

    if( it == m_itemKeys.end() )
        return;

    for( const TEXT_VAR_REF_KEY& key : it->second )
    {
        if( auto depIt = m_dependents.find( key ); depIt != m_dependents.end() )
        {
            depIt->second.erase( aItem );

            if( depIt->second.empty() )
                m_dependents.erase( depIt );
        }
    }

    m_itemKeys.erase( it );
}


void TEXT_VAR_DEPENDENCY_INDEX::ForEachDependent( const TEXT_VAR_REF_KEY&                    aKey,
                                                  const std::function<void( EDA_ITEM* )>&    aFn ) const
{
    // Snapshot under the shared lock, then release before invoking the callback.
    // Callers in Phase 3b listener adapters will re-enter Register/Unregister
    // during the callback (e.g. a dialog refresh that rebuilds its row list);
    // holding the lock across arbitrary code would deadlock on the writer
    // upgrade path.
    std::vector<EDA_ITEM*> snapshot;

    {
        std::shared_lock lock( m_mutex );

        auto it = m_dependents.find( aKey );

        if( it == m_dependents.end() )
            return;

        snapshot.reserve( it->second.size() );
        snapshot.assign( it->second.begin(), it->second.end() );
    }

    for( EDA_ITEM* item : snapshot )
        aFn( item );
}


void TEXT_VAR_DEPENDENCY_INDEX::Clear()
{
    std::unique_lock lock( m_mutex );

    m_dependents.clear();
    m_itemKeys.clear();
}


std::size_t TEXT_VAR_DEPENDENCY_INDEX::DependentCount( const TEXT_VAR_REF_KEY& aKey ) const
{
    std::shared_lock lock( m_mutex );

    auto it = m_dependents.find( aKey );

    return it == m_dependents.end() ? 0u : it->second.size();
}


std::size_t TEXT_VAR_DEPENDENCY_INDEX::ItemCount() const
{
    std::shared_lock lock( m_mutex );
    return m_itemKeys.size();
}


std::vector<TEXT_VAR_REF_KEY> TEXT_VAR_DEPENDENCY_INDEX::GetRegisteredKeys() const
{
    std::shared_lock lock( m_mutex );

    std::vector<TEXT_VAR_REF_KEY> keys;
    keys.reserve( m_dependents.size() );

    for( const auto& [key, deps] : m_dependents )
        keys.push_back( key );

    return keys;
}


TEXT_VAR_TRACKER::TEXT_VAR_TRACKER( SourceKeyExtractor aSourceKeyExtractor ) :
        m_extractSourceKeys( std::move( aSourceKeyExtractor ) )
{
}


void TEXT_VAR_TRACKER::SetSourceKeyExtractor( SourceKeyExtractor aExtractor )
{
    m_extractSourceKeys = std::move( aExtractor );
}


TEXT_VAR_TRACKER::ListenerHandle TEXT_VAR_TRACKER::AddInvalidateListener(
        InvalidateCallback aCallback )
{
    std::unique_lock lock( m_listenersMutex );
    const ListenerHandle handle = m_nextListenerHandle++;
    m_invalidateListeners.emplace( handle, std::move( aCallback ) );
    return handle;
}


void TEXT_VAR_TRACKER::RemoveInvalidateListener( ListenerHandle aHandle )
{
    std::unique_lock lock( m_listenersMutex );
    m_invalidateListeners.erase( aHandle );
}


void TEXT_VAR_TRACKER::RegisterItem( EDA_ITEM*                            aItem,
                                     const std::vector<TEXT_VAR_REF_KEY>& aKeys )
{
    if( aKeys.empty() )
        m_index.Unregister( aItem );
    else
        m_index.Register( aItem, aKeys );
}


void TEXT_VAR_TRACKER::UnregisterItem( EDA_ITEM* aItem )
{
    m_index.Unregister( aItem );
}


void TEXT_VAR_TRACKER::fanOutSourceKeys( EDA_ITEM* aItem )
{
    if( !m_extractSourceKeys )
        return;

    std::vector<TEXT_VAR_REF_KEY> sourceKeys = m_extractSourceKeys( aItem );

    for( const TEXT_VAR_REF_KEY& key : sourceKeys )
        InvalidateKey( key );
}


void TEXT_VAR_TRACKER::HandleItemChanged( EDA_ITEM*                            aItem,
                                          const std::vector<TEXT_VAR_REF_KEY>& aUpdatedKeys )
{
    // Two responsibilities per changed item:
    //   1. Keep its own dependency registration current — text may have been edited.
    //   2. Fan out invalidation to items that depend on it as a cross-ref source.
    // We do (1) before (2) so a self-referencing item sees its own refresh.
    RegisterItem( aItem, aUpdatedKeys );
    fanOutSourceKeys( aItem );
}


void TEXT_VAR_TRACKER::InvalidateByKind(
        std::initializer_list<TEXT_VAR_REF_KEY::KIND> aKinds )
{
    // Snapshot the key set once (copy is unavoidable — InvalidateKey can
    // mutate the index through its listener callbacks, so iterating the
    // live index under a read lock would deadlock on re-entry).
    const std::vector<TEXT_VAR_REF_KEY> all = m_index.GetRegisteredKeys();

    for( const TEXT_VAR_REF_KEY& key : all )
    {
        for( TEXT_VAR_REF_KEY::KIND k : aKinds )
        {
            if( key.kind == k )
            {
                InvalidateKey( key );
                break;
            }
        }
    }
}


void TEXT_VAR_TRACKER::InvalidateProjectScoped()
{
    InvalidateByKind( { TEXT_VAR_REF_KEY::KIND::TITLE_BLOCK,
                        TEXT_VAR_REF_KEY::KIND::SPECIAL,
                        TEXT_VAR_REF_KEY::KIND::PROJECT_VAR,
                        TEXT_VAR_REF_KEY::KIND::ENV_VAR,
                        TEXT_VAR_REF_KEY::KIND::LOCAL } );
}


void TEXT_VAR_TRACKER::InvalidateVariantScoped()
{
    InvalidateByKind( { TEXT_VAR_REF_KEY::KIND::CROSS_REF,
                        TEXT_VAR_REF_KEY::KIND::LOCAL } );
}


void TEXT_VAR_TRACKER::InvalidateKey( const TEXT_VAR_REF_KEY& aKey )
{
    // Snapshot the listener callbacks under a shared lock, then invoke
    // unlocked. Callbacks may add/remove listeners (which would invalidate
    // iterators) or re-enter Invalidate* paths (shared_mutex tolerates
    // re-entrant shared locks on most implementations, but we avoid holding
    // it during the callback anyway so writer starvation is bounded).
    std::vector<InvalidateCallback> snapshot;

    {
        std::shared_lock lock( m_listenersMutex );

        if( m_invalidateListeners.empty() )
            return;

        snapshot.reserve( m_invalidateListeners.size() );

        for( const auto& [handle, cb] : m_invalidateListeners )
            snapshot.push_back( cb );
    }

    m_index.ForEachDependent( aKey,
                              [&]( EDA_ITEM* dependent )
                              {
                                  for( const InvalidateCallback& cb : snapshot )
                                      cb( dependent, aKey );
                              } );
}
