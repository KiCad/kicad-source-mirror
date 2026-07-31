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

/**
 * @file mdw_client.h
 * The entry point to workspace sources for UI, CLI and scripting code.
 */

#ifndef MDW_CLIENT_H
#define MDW_CLIENT_H

#include <kicommon.h>
#include <mdw/client/mdw_events.h>
#include <mdw/client/mdw_subscription.h>
#include <mdw/client/mdw_source.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class MDW_FRAMEWORK;
class MDW_SAVE_PROGRESS;
class PROJECT;
class wxEvtHandler;
struct MDW_SAVE_CHECK;

/// Where the open project stands against its design source
struct MDW_PROJECT_STATE
{
    MDW_ITEM_REF item;
    wxString     base_revision_id; ///< The revision the local design was checked out at; empty when unknown
    wxString     base_revision_label;
    bool         locally_modified = false;
};

/**
 * Owns the configured sources and the delivery of every operation's completion.
 *
 * Completions run on the UI thread when an app exists. Headless callers get one client thread
 * instead, and MdwSync() may be called from any other.
 */
class KICOMMON_API MDW_CLIENT
{
public:
    using EVENT_HANDLER = std::function<void( const MDW_EVENT& )>;

    static MDW_CLIENT& Get();

    /// Follow aFramework's sessions. Its owner calls this for each framework it builds
    void Listen( MDW_FRAMEWORK& aFramework );

    /// The open project's design source, or null when the project is not managed
    std::shared_ptr<MDW_SOURCE> ProjectSource() const;

    std::vector<std::shared_ptr<MDW_SOURCE>> LibrarySources() const;

    /// The source whose credential-free locator is aLocator, or null; never a guess
    std::shared_ptr<MDW_SOURCE> FindSource( const wxString& aLocator ) const;

    /**
     * The item at aLocalPath, a file in aProject. Fails ITEM_NOT_FOUND when the backend answers that the
     * file is not an item, which callers read as "not managed". Fails NOT_SUPPORTED when it cannot tell,
     * as for an unmanaged project or an upload backend, which callers read as "unknown". Both are silent.
     * A recorded binding answers at once; a working copy may ask its backend first.
     */
    MDW_OP ItemRef( const PROJECT& aProject, const wxString& aLocalPath, MDW_DONE<MDW_ITEM_REF> aDone );

    /**
     * What a save of aLocalPath, a file in aProject, would run into. Never fails, so a file that is not an
     * item, or whose item cannot be told, settles unmanaged, and a lifecycle that cannot be read settles
     * EDITABLE. aProgress, which may be null, follows the steps so a caller that stops waiting can tell
     * which one stalled. Unbound files and a disabled integration settle without a backend call.
     */
    MDW_OP CheckSave( const PROJECT& aProject, const wxString& aLocalPath, std::shared_ptr<MDW_SAVE_PROGRESS> aProgress,
                      MDW_DONE<MDW_SAVE_CHECK> aDone );

    /// After a successful write of aLocalPath, mark its item modified since check-out; nothing for a non-item
    void NoteSaved( const PROJECT& aProject, const wxString& aLocalPath );

    /// ItemRef of aProject's project file
    MDW_OP ProjectRef( const PROJECT& aProject, MDW_DONE<MDW_ITEM_REF> aDone );

    MDW_OP ProjectState( const PROJECT& aProject, MDW_DONE<MDW_PROJECT_STATE> aDone );

    /// Close the workspace session for aProjectPath, even while integration is off
    void CloseProject( const wxString& aProjectPath );

    /// The binding aProject uses on this computer, which is none while the project is left here
    static MDW_PROJECT_BINDING ProjectBinding( const PROJECT& aProject );

    /**
     * Whether aBinding's backend binds the project's own git remote, so it signs in as the project's git
     * username rather than as the server's default account
     */
    static bool BindsGitRemote( const MDW_PROJECT_BINDING& aBinding );

    /// What the client knows now; never blocks and never opens anything
    MDW_PROJECT_CONNECTION ProjectConnection( const PROJECT& aProject ) const;

    /**
     * Open aProject's session from the binding in its project file, unless one is open or opening.
     * Retries after BACKEND_STOPPED. Does nothing when UNAVAILABLE or UNBOUND.
     */
    void ConnectProject( PROJECT& aProject );

    /**
     * Sign aProject's open session in again in place, with aAnswer's trust and credentials, passing through
     * CONNECTING. Without a session, or with a stopped backend, the project is closed instead, so it opens
     * afresh.
     */
    void ReconnectProject( const PROJECT& aProject, const MDW_CONNECTION_ANSWER& aAnswer = {} );

    /**
     * Call aChanged on the UI thread with the project path whenever a project's connection state
     * changes, while aOwner lives. Same reset guarantee as Subscribe.
     */
    [[nodiscard]] MDW_SUBSCRIPTION SubscribeConnections( wxEvtHandler*                          aOwner,
                                                         std::function<void( const wxString& )> aChanged );

    /**
     * Receive every source's events on the UI thread while aOwner lives. Events prompt a re-read;
     * they carry no state of their own. Reset the subscription on the UI thread; no event queued
     * before the reset reaches the handler after it.
     */
    [[nodiscard]] MDW_SUBSCRIPTION Subscribe( wxEvtHandler* aOwner, EVENT_HANDLER aHandler );

    /// Receive events on the client's delivery thread, in the order they were published
    [[nodiscard]] MDW_SUBSCRIPTION Subscribe( EVENT_HANDLER aHandler );

    /// Publish an event to subscribers. Sources call this; consumers do not.
    void Publish( const MDW_EVENT& aEvent );

private:
    MDW_CLIENT();
    ~MDW_CLIENT();

    struct IMPL;
    std::unique_ptr<IMPL> m_impl;
};

#endif // MDW_CLIENT_H
