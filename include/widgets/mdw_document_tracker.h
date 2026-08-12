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

#ifndef MDW_DOCUMENT_TRACKER_H
#define MDW_DOCUMENT_TRACKER_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_revision.h>
#include <mdw/client/mdw_subscription.h>

#include <functional>
#include <memory>

#include <wx/event.h>
#include <wx/string.h>

class MDW_SOURCE;


/**
 * The Design Workspace state of the document a frame shows, read through the client. It finds the
 * document's item and reads its lifecycle and claim again whenever the project's connection changes or
 * a source reports an event, so a new source after a reconnect is picked up. It never polls the backend.
 */
class KICOMMON_API MDW_DOCUMENT_TRACKER : public wxEvtHandler
{
public:
    struct STATE
    {
        bool                   bound = false; ///< The document is a managed item
        MDW_CONNECTION         phase = MDW_CONNECTION::OFFLINE; ///< The project's connection
        bool                   known = false; ///< The backend has answered for this item
        MDW_LIFECYCLE_CATEGORY category = MDW_LIFECYCLE_CATEGORY::EDITABLE;
        MDW_REVISION           revision;         ///< The published revision; its label is empty when none
        bool                   can_lock = false; ///< The backend shares editing claims
        MDW_LOCK_INFO          lock;             ///< As the session's lock tracker holds it
    };

    /// Resolves the local path of the frame's active document, empty when there is none.
    using DOCUMENT_RESOLVER = std::function<wxString()>;

    /// How the tracker reaches the project. Each one left empty asks the client about the tracked project.
    struct HOOKS
    {
        std::function<std::shared_ptr<MDW_SOURCE>()>                     source;
        std::function<MDW_OP( const wxString&, MDW_DONE<MDW_ITEM_REF> )> itemRef;
        std::function<MDW_CONNECTION()>                                  phase;
    };

    /// aChanged receives every new state on the UI thread.
    MDW_DOCUMENT_TRACKER( std::function<void( const STATE& )> aChanged, HOOKS aHooks = HOOKS() );

    /**
     * Follow the document aDocument resolves in the project at aProjectPath. Hosts call this on project
     * open and close and when they replace their document.
     */
    void Track( const wxString& aProjectPath, bool aEnabled, DOCUMENT_RESOLVER aDocument );

    void Refresh();

    /// Refresh only if the resolver now names another document. Local, so cheap enough to call when idle.
    void RefreshIfMoved();

    /// The project's source while tracking is enabled, else null.
    std::shared_ptr<MDW_SOURCE> Source() const;

    /// The item the last state describes, empty until it is known.
    const MDW_ITEM_REF& Item() const { return m_item; }

private:
    void describe( const std::shared_ptr<MDW_SOURCE>& aSource );
    void publish( const STATE& aState );

    std::function<void( const STATE& )> m_changed;
    HOOKS                               m_hooks;
    wxString                            m_projectPath;
    bool                                m_enabled = false;
    DOCUMENT_RESOLVER                   m_document;
    wxString                            m_lastDocument; ///< What m_document gave at the last refresh
    wxString                            m_documentPath; ///< The document m_item belongs to
    MDW_ITEM_REF                        m_item;
    STATE                               m_state;
    MDW_OP                              m_refOp; ///< Each op is replaced by the next refresh, cancelling it
    MDW_OP                              m_infoOp;
    MDW_SUBSCRIPTION                    m_events;
    MDW_SUBSCRIPTION                    m_connections;
};

#endif // MDW_DOCUMENT_TRACKER_H
