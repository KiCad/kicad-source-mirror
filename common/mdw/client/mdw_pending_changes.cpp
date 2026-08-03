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

#include "mdw_pending_changes.h"

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_file_lock.h>
#include <mdw/client/mdw_source.h>
#include <kiplatform/io.h>
#include <paths.h>
#include <remote_provider_utils.h>

#include <nlohmann/json.hpp>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <algorithm>


namespace
{

const char* kindName( MDW_PENDING_KIND aKind )
{
    return aKind == MDW_PENDING_KIND::DESIGN_BASELINE ? "design_baseline" : "part_revision";
}


std::string utf8( const wxString& aText )
{
    return aText.utf8_string();
}


bool decided( MDW_PENDING_STATUS aStatus )
{
    return aStatus == MDW_PENDING_STATUS::APPLIED || aStatus == MDW_PENDING_STATUS::REJECTED
           || aStatus == MDW_PENDING_STATUS::WITHDRAWN;
}

} // namespace


MDW_PENDING_CHANGES& MDW_PENDING_CHANGES::Get()
{
    static MDW_PENDING_CHANGES store(
            wxFileName( PATHS::GetUserSettingsPath(), wxS( "mdw_pending_changes.json" ) ).GetFullPath() );
    return store;
}


MDW_PENDING_CHANGES::MDW_PENDING_CHANGES( const wxString& aFile ) :
        m_file( aFile )
{
}


MDW_PENDING_CHANGES::~MDW_PENDING_CHANGES() = default;


std::vector<MDW_PENDING_ENTRY> MDW_PENDING_CHANGES::read() const
{
    std::vector<MDW_PENDING_ENTRY> entries;
    wxFFile                        file;
    wxString                       content;

    if( !wxFileName::FileExists( m_file ) || !file.Open( m_file, wxS( "rb" ) ) || !file.ReadAll( &content ) )
        return entries;

    nlohmann::json root = nlohmann::json::parse( content.utf8_string(), nullptr, false );

    if( !root.is_object() || !root.contains( "changes" ) || !root["changes"].is_array() )
        return entries;

    for( const nlohmann::json& row : root["changes"] )
    {
        if( !row.is_object() || !row.contains( "item" ) || !row["item"].is_object() )
            continue;

        MDW_PENDING_ENTRY entry;
        entry.locator = RemoteProviderJsonString( row, "locator" );
        entry.change.change_id = RemoteProviderJsonString( row, "change_id" );
        entry.change.eco_id = RemoteProviderJsonString( row, "eco_id" );
        entry.change.display_id = RemoteProviderJsonString( row, "display_id" );
        entry.change.reason = RemoteProviderJsonString( row, "reason" );
        entry.change.url = RemoteProviderJsonString( row, "url" );
        entry.item.item_id = RemoteProviderJsonString( row["item"], "item_id" );
        entry.item.path_hint = RemoteProviderJsonString( row["item"], "path_hint" );
        entry.item.source = RemoteProviderJsonString( row["item"], "source" );

        if( row.contains( "status" ) && row["status"].is_number_integer() )
            entry.status = static_cast<MDW_PENDING_STATUS>( row["status"].get<int>() );

        entry.kind = RemoteProviderJsonString( row, "kind" ) == wxS( "design_baseline" )
                             ? MDW_PENDING_KIND::DESIGN_BASELINE
                             : MDW_PENDING_KIND::PART_REVISION;

        // Without these nothing can ask about the change again
        if( !entry.locator.IsEmpty() && !entry.change.change_id.IsEmpty() )
            entries.push_back( entry );
    }

    return entries;
}


void MDW_PENDING_CHANGES::write( const std::vector<MDW_PENDING_ENTRY>& aEntries ) const
{
    nlohmann::json changes = nlohmann::json::array();

    for( const MDW_PENDING_ENTRY& entry : aEntries )
    {
        changes.push_back( { { "locator", utf8( entry.locator ) },
                             { "change_id", utf8( entry.change.change_id ) },
                             { "eco_id", utf8( entry.change.eco_id ) },
                             { "display_id", utf8( entry.change.display_id ) },
                             { "reason", utf8( entry.change.reason ) },
                             { "url", utf8( entry.change.url ) },
                             { "item",
                               { { "item_id", utf8( entry.item.item_id ) },
                                 { "path_hint", utf8( entry.item.path_hint ) },
                                 { "source", utf8( entry.item.source ) } } },
                             { "kind", kindName( entry.kind ) },
                             { "status", static_cast<int>( entry.status ) } } );
    }

    // A failed write leaves the previous file whole, so no change already remembered is lost
    std::string data = nlohmann::json( { { "changes", changes } } ).dump( 2 );
    KIPLATFORM::IO::AtomicWriteFile( m_file, data.data(), data.size() );
}


void MDW_PENDING_CHANGES::update( const std::function<bool( std::vector<MDW_PENDING_ENTRY>& )>& aChange )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    MdwWithFileLock( m_file,
                     [&]()
                     {
                         std::vector<MDW_PENDING_ENTRY> entries = read();

                         if( aChange( entries ) )
                             write( entries );
                     } );
}


void MDW_PENDING_CHANGES::Remember( const MDW_PENDING_ENTRY& aEntry )
{
    MDW_PENDING_ENTRY entry = aEntry;

    // Stamped with its source, so For() matches it by source and id
    if( entry.item.source.IsEmpty() )
        entry.item.source = entry.locator;

    update(
            [&]( std::vector<MDW_PENDING_ENTRY>& aEntries )
            {
                auto known = std::find_if( aEntries.begin(), aEntries.end(),
                                           [&]( const MDW_PENDING_ENTRY& aOld )
                                           {
                                               return aOld.locator == entry.locator
                                                      && aOld.change.change_id == entry.change.change_id;
                                           } );

                // A refresh keeps the change's place, so For() stays oldest first
                if( known != aEntries.end() )
                    *known = entry;
                else
                    aEntries.push_back( entry );

                return true;
            } );
}


void MDW_PENDING_CHANGES::Forget( const wxString& aLocator, const wxString& aChangeId )
{
    update(
            [&]( std::vector<MDW_PENDING_ENTRY>& aEntries )
            {
                return std::erase_if( aEntries,
                                      [&]( const MDW_PENDING_ENTRY& aOld )
                                      {
                                          return aOld.locator == aLocator && aOld.change.change_id == aChangeId;
                                      } )
                       > 0;
            } );
}


void MDW_PENDING_CHANGES::Rehome( const wxString& aOld, const wxString& aNew )
{
    update(
            [&]( std::vector<MDW_PENDING_ENTRY>& aEntries )
            {
                bool moved = false;

                for( MDW_PENDING_ENTRY& entry : aEntries )
                {
                    for( wxString* locator : { &entry.locator, &entry.item.source } )
                    {
                        if( *locator == aOld )
                        {
                            *locator = aNew;
                            moved = true;
                        }
                    }
                }

                return moved;
            } );
}


std::vector<MDW_PENDING_ENTRY> MDW_PENDING_CHANGES::For( const MDW_ITEM_REF& aItem ) const
{
    std::vector<MDW_PENDING_ENTRY> entries = All();

    std::erase_if( entries,
                   [&]( const MDW_PENDING_ENTRY& aEntry )
                   {
                       return aEntry.item.item_id != aItem.item_id || aEntry.item.source != aItem.source;
                   } );

    return entries;
}


std::vector<MDW_PENDING_ENTRY> MDW_PENDING_CHANGES::All() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return read();
}


void MDW_PENDING_CHANGES::Recheck( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    if( !aSource )
        return;

    wxString                    locator = aSource->Info().locator;
    std::lock_guard<std::mutex> lock( m_checkMutex );

    std::erase_if( m_checks,
                   []( const MDW_OP& aOp )
                   {
                       return aOp.IsDone();
                   } );

    for( const MDW_PENDING_ENTRY& entry : All() )
    {
        if( entry.locator != locator )
            continue;

        auto settle = [this, entry]( const MDW_RESULT& aResult, const MDW_PENDING_INFO& aInfo )
        {
            // An unreachable source says nothing about the change, so it stays remembered
            if( !aResult.IsOk() )
                return;

            if( !decided( aInfo.status ) )
            {
                // Another process may have seen it decided and forgotten it meanwhile
                update(
                        [&]( std::vector<MDW_PENDING_ENTRY>& aEntries )
                        {
                            for( MDW_PENDING_ENTRY& known : aEntries )
                            {
                                if( known.locator != entry.locator || known.change.change_id != entry.change.change_id )
                                {
                                    continue;
                                }

                                known.status = aInfo.status;

                                if( aInfo.change.change_id == entry.change.change_id )
                                    known.change = aInfo.change;

                                return true;
                            }

                            return false;
                        } );

                return;
            }

            Forget( entry.locator, entry.change.change_id );

            MDW_EVENT event;
            event.type = MDW_EVENT_TYPE::PENDING_RESOLVED;
            event.item_id = entry.item.item_id;
            event.change_id = entry.change.change_id;
            event.source = entry.locator;
            MDW_CLIENT::Get().Publish( event );
        };

        m_checks.push_back( aSource->PendingChange( entry.change.change_id, settle ) );
    }
}


void MDW_PENDING_CHANGES::Follow()
{
    if( m_events.IsActive() )
        return;

    m_events = MDW_CLIENT::Get().Subscribe(
            [this]( const MDW_EVENT& aEvent )
            {
                if( aEvent.type == MDW_EVENT_TYPE::WORKSPACE_BOUND
                    || aEvent.type == MDW_EVENT_TYPE::WORKSPACE_RECONNECTED )
                {
                    Recheck( MDW_CLIENT::Get().FindSource( aEvent.source ) );
                }
            } );

    if( std::shared_ptr<MDW_SOURCE> project = MDW_CLIENT::Get().ProjectSource() )
        Recheck( project );

    for( const std::shared_ptr<MDW_SOURCE>& source : MDW_CLIENT::Get().LibrarySources() )
        Recheck( source );
}
