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


#include <widgets/mdw_document_tracker.h>

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_source.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/settings_manager.h>


namespace
{

PROJECT* loadedProject( const wxString& aProjectPath )
{
    PGM_BASE* pgm = PgmOrNull();

    return pgm && !aProjectPath.IsEmpty() ? pgm->GetSettingsManager().GetProject( aProjectPath ) : nullptr;
}

} // namespace


MDW_DOCUMENT_TRACKER::MDW_DOCUMENT_TRACKER( std::function<void( const STATE& )> aChanged, HOOKS aHooks ) :
        m_changed( std::move( aChanged ) ),
        m_hooks( std::move( aHooks ) )
{
    if( !m_hooks.source )
    {
        m_hooks.source = [this]()
        {
            return loadedProject( m_projectPath ) ? MDW_CLIENT::Get().ProjectSource() : nullptr;
        };
    }

    if( !m_hooks.itemRef )
    {
        m_hooks.itemRef = [this]( const wxString& aPath, MDW_DONE<MDW_ITEM_REF> aDone )
        {
            PROJECT* project = loadedProject( m_projectPath );

            return project ? MDW_CLIENT::Get().ItemRef( *project, aPath, std::move( aDone ) ) : MDW_OP();
        };
    }

    if( !m_hooks.phase )
    {
        m_hooks.phase = [this]()
        {
            PROJECT* project = loadedProject( m_projectPath );

            return project ? MDW_CLIENT::Get().ProjectConnection( *project ).status.phase : MDW_CONNECTION::OFFLINE;
        };
    }
}


void MDW_DOCUMENT_TRACKER::Track( const wxString& aProjectPath, bool aEnabled, DOCUMENT_RESOLVER aDocument )
{
    m_projectPath = aProjectPath;
    m_enabled = aEnabled;
    m_document = std::move( aDocument );

    if( !m_enabled )
    {
        m_events.Reset();
        m_connections.Reset();
    }
    else if( !m_events.IsActive() )
    {
        // A reconnect binds a new source, so every event asks again rather than reading through the old one
        m_events = MDW_CLIENT::Get().Subscribe( this,
                                                [this]( const MDW_EVENT& )
                                                {
                                                    Refresh();
                                                } );

        m_connections = MDW_CLIENT::Get().SubscribeConnections( this,
                                                                [this]( const wxString& aChangedPath )
                                                                {
                                                                    if( aChangedPath == m_projectPath )
                                                                        Refresh();
                                                                } );
    }

    Refresh();
}


std::shared_ptr<MDW_SOURCE> MDW_DOCUMENT_TRACKER::Source() const
{
    return m_enabled ? m_hooks.source() : nullptr;
}


void MDW_DOCUMENT_TRACKER::RefreshIfMoved()
{
    if( m_enabled && m_document && m_document() != m_lastDocument )
        Refresh();
}


void MDW_DOCUMENT_TRACKER::Refresh()
{
    std::shared_ptr<MDW_SOURCE> source = Source();

    m_lastDocument = m_document ? m_document() : wxString();

    wxString path = source ? m_lastDocument : wxString();

    // Any reply still in flight describes an older question
    m_refOp = MDW_OP();
    m_infoOp = MDW_OP();

    if( path.IsEmpty() )
    {
        m_documentPath.clear();
        m_item = MDW_ITEM_REF();
        publish( STATE() );
        return;
    }

    // Until the new document's item is known, no action may reach the previous one
    if( path != m_documentPath )
    {
        STATE pending;
        pending.bound = true;
        pending.phase = m_hooks.phase();

        m_documentPath = path;
        m_item = MDW_ITEM_REF();
        publish( pending );
    }

    m_refOp = m_hooks.itemRef( path,
                               [this, source]( const MDW_RESULT& aResult, const MDW_ITEM_REF& aRef )
                               {
                                   // Not an item, such as a file git does not track yet, or a source that cannot tell
                                   if( aResult.error == MDW_ERROR_CODE::ITEM_NOT_FOUND
                                       || aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED )
                                   {
                                       m_item = MDW_ITEM_REF();
                                       publish( STATE() );
                                       return;
                                   }

                                   MDW_CONNECTION phase = m_hooks.phase();
                                   bool           online = aResult.IsOk() && phase == MDW_CONNECTION::ONLINE;
                                   STATE          state;

                                   // Keep showing what is known about the same item while it is rechecked
                                   if( online && m_state.bound && m_state.known && aRef.item_id == m_item.item_id )
                                       state = m_state;

                                   state.bound = true;
                                   // A failed lookup on a connected session reads as offline, as nothing is known
                                   state.phase =
                                           phase == MDW_CONNECTION::ONLINE && !online ? MDW_CONNECTION::OFFLINE : phase;

                                   m_item = aRef;
                                   publish( state );

                                   if( online )
                                       describe( source );
                               } );
}


void MDW_DOCUMENT_TRACKER::describe( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    m_infoOp = aSource->Item( m_item, wxEmptyString,
                              [this, aSource]( const MDW_RESULT& aResult, const MDW_ITEM_INFO& aInfo )
                              {
                                  STATE state;
                                  state.bound = aResult.error != MDW_ERROR_CODE::ITEM_NOT_FOUND;

                                  if( !aResult.IsOk() )
                                  {
                                      publish( state );
                                      return;
                                  }

                                  state.phase = MDW_CONNECTION::ONLINE;
                                  state.known = true;
                                  state.category = aInfo.state.category;
                                  state.can_lock = aSource->Has( MDW_CAPABILITY_GROUP::LOCKING );

                                  if( state.can_lock )
                                      state.lock = aSource->Claim( m_item );

                                  if( aInfo.state.category == MDW_LIFECYCLE_CATEGORY::RELEASED )
                                  {
                                      state.revision.label = aInfo.revision_label;
                                      state.revision.kind = aInfo.revision_kind;
                                  }

                                  publish( state );
                              } );
}


void MDW_DOCUMENT_TRACKER::publish( const STATE& aState )
{
    m_state = aState;

    if( m_changed )
        m_changed( m_state );
}
