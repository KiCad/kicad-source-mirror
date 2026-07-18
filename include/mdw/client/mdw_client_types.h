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
 * @file mdw_client_types.h
 * Records shared by the workspace facades. Each mirrors a kiapi.workspace message, and enum
 * numbering matches the proto so conversion is a cast.
 */

#ifndef MDW_CLIENT_TYPES_H
#define MDW_CLIENT_TYPES_H

#include <mdw/client/mdw_auth.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_revision.h>
#include <mdw/client/mdw_types.h>

#include <wx/datetime.h>
#include <wx/string.h>

#include <cstdint>
#include <optional>
#include <vector>

enum class MDW_CHANGE_OUTCOME
{
    UNKNOWN = 0,
    APPLIED = 1,
    PENDING = 2         ///< Decided later; see MDW_PENDING_CHANGE
};

enum class MDW_PENDING_STATUS
{
    UNKNOWN   = 0,
    PENDING   = 1,
    APPLIED   = 2,
    REJECTED  = 3,
    WITHDRAWN = 4
};

/// The (workspace ...) binding stored in a project's .kicad_pro file.
struct MDW_PROJECT_BINDING
{
    wxString backend_id;
    wxString workspace_id;
    wxString binding;                                   ///< Opaque backend reconstitution blob

    bool IsBound() const { return !backend_id.IsEmpty(); }

    bool operator==( const MDW_PROJECT_BINDING& aOther ) const = default;
};


/// Where a backend that declares no binding fields takes its binding from
enum class MDW_BINDING_SOURCE
{
    NONE,
    GIT_REMOTE ///< The remote of the project's own git repository
};


/// A project binding field a backend asks the user for
struct MDW_BINDING_FIELD
{
    wxString id;    ///< "workspace_id" or "binding", the MDW_PROJECT_BINDING member it fills
    wxString label;
    wxString hint;  ///< An example or short instruction shown while the field is empty
};

/// Something a backend lists for a binding to name, such as an Odoo product
struct MDW_BINDING_TARGET
{
    wxString display_name;
    wxString code;
    wxString locator; ///< The "key=value" locator item that names it, opaque to the client
};

/**
 * An opaque item id with the text a user sees for it. Ids are unique within their source only;
 * source is the owning source's locator, empty for the source that returned it.
 */
struct MDW_ITEM_REF
{
    wxString item_id;
    wxString path_hint;
    wxString source;
};

/// The revision a checked-out file was taken from, as the backend's last reply named it
struct MDW_ITEM_BASE
{
    wxString revision_id;
    wxString revision_label;

    bool operator==( const MDW_ITEM_BASE& aOther ) const = default;
};


/// A file's item and the revision the working copy holds for it; the base is empty when unknown
struct MDW_LOCATED_ITEM
{
    MDW_ITEM_REF item;
    wxString     base_revision_id;
    wxString     base_revision_label;
};

/**
 * The workspace part a placed symbol stands for, as its MDW_Part field records it. Identity
 * only; the revision's label and state are read from the backend.
 */
struct MDW_PART_IDENTITY
{
    MDW_ITEM_REF part;
    wxString     revision_id;
};


enum class MDW_CHANGE_TYPE
{
    UNKNOWN  = 0,
    ADDED    = 1,
    REMOVED  = 2,
    MODIFIED = 3
};

/// A change order cited outside its own source
struct MDW_ECO_REF
{
    wxString source;
    wxString eco_id;
};

/// A change accepted but decided later. change_id is always set; eco_id only when an ECO carries it.
struct MDW_PENDING_CHANGE
{
    wxString change_id;
    wxString eco_id;
    wxString display_id;
    wxString reason;
    wxString url;
};

/// A backend-minted point in a library's history. token is opaque, stored and shown but never parsed.
struct MDW_SNAPSHOT
{
    wxString              token;
    wxString              label;
    std::vector<wxString> warnings; ///< Why membership at this snapshot is only approximate, for the user
};

/// The outcome of a release, a baseline or one member's check-in
struct MDW_CHANGE
{
    MDW_CHANGE_OUTCOME outcome = MDW_CHANGE_OUTCOME::UNKNOWN;
    MDW_REVISION       revision;            ///< APPLIED
    MDW_PENDING_CHANGE pending;             ///< PENDING
    MDW_SNAPSHOT       snapshot;            ///< The library copy an APPLIED release left, or an empty token
    wxString           base_snapshot;       ///< The token of the copy an APPLIED release built on, set with snapshot
};

struct MDW_PENDING_INFO
{
    MDW_PENDING_STATUS status = MDW_PENDING_STATUS::UNKNOWN;
    MDW_PENDING_CHANGE change;
    MDW_ITEM_REF       item;
    MDW_REVISION       revision;            ///< What an APPLIED change made
    wxString           reason;              ///< Why it was rejected or withdrawn
};

/// A page of rows. An empty next_page_token ends the list.
template <typename T>
struct MDW_PAGED
{
    std::vector<T>          rows;
    wxString                next_page_token;
    std::optional<uint64_t> total_count;
};

/**
 * A library source a project needs, as stored in its project file. Each user sets the source up once and
 * every project naming its backend and locator shares that copy.
 */
struct MDW_LIBRARY_REQUIREMENT
{
    wxString     id;
    wxString     backend_id;
    wxString     locator;
    wxString     name;   ///< The folder name offered when the user sets the source up
    wxString     prefix; ///< The project's symbols name libraries with this prefix
    MDW_SNAPSHOT pin;    ///< An empty token follows the latest snapshot
};

struct MDW_ITEM_LOCK
{
    wxString      item_id;
    MDW_LOCK_INFO lock;
};

/// Where a workspace session stands, or why there is none
enum class MDW_CONNECTION
{
    UNAVAILABLE, ///< Integration is off or there is no framework
    UNBOUND,     ///< The project file names no workspace
    IDLE,        ///< Bound, and no open has been tried since the project loaded or closed
    CONNECTING,
    ONLINE,             ///< The last server check passed
    OFFLINE,            ///< The server cannot be reached; probes back off until it answers
    SIGN_IN_NEEDED,     ///< The server answers but has not accepted the user's credentials
    TRUST_NEEDED,       ///< The server's host key needs the user's confirmation
    BACKEND_RESTARTING, ///< The backend process was lost and the supervisor is bringing it back
    BACKEND_STOPPED     ///< The backend could not be started or kept stopping; only a reopen tries again
};

/// The word the workspace strip and the status pill both show for aPhase
KICOMMON_API wxString MdwConnectionName( MDW_CONNECTION aPhase );

/// One session's connection, which MDW_SESSION writes and MDW_CLIENT marks with the stored login's refusal
struct MDW_CONNECTION_STATUS
{
    MDW_CONNECTION phase = MDW_CONNECTION::UNAVAILABLE;
    MDW_ACCOUNT    account;                   ///< Who the server last accepted
    wxString       realm;                     ///< The server the credential form is for, empty when it needs none
    wxString       username;                  ///< Who the binding signs in as, empty for the server's default
    MDW_RESULT     reason = MDW_RESULT::Ok(); ///< Why the phase is not ONLINE, with any auth form or host key
    wxDateTime     lastOnline;                ///< When a server check last passed; invalid when none has
    bool           storedLoginRefused = false; ///< The server refused the last stored login sent
};

/// What the user gave to resolve a connection, a host key to trust or credentials to sign in with
struct MDW_CONNECTION_ANSWER
{
    std::optional<MDW_HOST_KEY>          trust;
    std::optional<MDW_TYPED_CREDENTIALS> credentials;
    std::optional<MDW_TYPED_CREDENTIALS> stored; ///< The stored login an unlock prompt read, sent as the stored one
};

struct MDW_PROJECT_CONNECTION
{
    MDW_CONNECTION_STATUS status;
    wxString              source_name; ///< The backend's display name, or its id when it has none
    wxString              workspace_id;
};

/// What MDW_CLIENT knows about one configured source
struct MDW_SOURCE_INFO
{
    wxString              locator;      ///< Credential-free; the source's identity
    wxString              workspace_id; ///< What MDW_EVENT names the source by
    wxString              display_name;
    MDW_BACKEND_INFO      backend;
    MDW_CONNECTION_STATUS connection;

    wxString Name() const { return display_name.IsEmpty() ? locator : display_name; }
};

#endif // MDW_CLIENT_TYPES_H
