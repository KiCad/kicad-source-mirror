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
 * @file mdw_types.h
 * @brief Core enumerations and the shared result base for the Managed Design Workspace (MDW).
 *
 * Enum numbering matches workspace_types.proto so conversion is a plain cast.
 */

#ifndef MDW_TYPES_H
#define MDW_TYPES_H

#include <kicommon.h>

#include <wx/string.h>
#include <optional>
#include <vector>

/**
 * Every backend maps its native states onto one of these. PENDING_REVIEW and RESTRICTED block
 * writes, and RELEASED allows writes but warns on save. RESTRICTED is a policy-imposed read-only
 * state, independent of a per-user lock. Numbering matches kiapi.workspace.LifecycleCategory.
 */
enum class MDW_LIFECYCLE_CATEGORY
{
    EDITABLE       = 0,
    PENDING_REVIEW = 1,
    RESTRICTED     = 2,
    RELEASED       = 3
};


/// The lifecycle state of one revision. state_id names a custom state and may be empty.
struct MDW_LIFECYCLE_STATE
{
    MDW_LIFECYCLE_CATEGORY category = MDW_LIFECYCLE_CATEGORY::EDITABLE;
    wxString               state_id;
    bool                   not_for_new_designs = false; ///< Existing designs may keep it, new placements warn
};

/// What kind of document a file is. Numbering matches kiapi.workspace.DocumentKind.
enum class MDW_DOCUMENT_KIND
{
    UNKNOWN        = 0,
    PROJECT        = 1,
    SCHEMATIC      = 2,
    BOARD          = 3,
    SYMBOL_LIBRARY = 4,
    FOOTPRINT      = 5,
    OTHER          = 6     ///< A file with no editor or differ
};

/// What kind of point in an item's history a revision is. Numbering matches kiapi.workspace.RevisionKind.
enum class MDW_REVISION_KIND
{
    UNKNOWN  = 0,
    CHECKIN  = 1,
    RELEASE  = 2,          ///< Approved; an attempt still awaiting approval is RELEASE_PENDING
    BASELINE = 3,
    WORKING  = 4,          ///< Planned or in progress, holding no released data yet
    RELEASE_PENDING  = 5,
    RELEASE_REJECTED = 6   ///< Refused or withdrawn; it never released data
};

/// How a source's items reach the disk. Numbering matches kiapi.workspace.CheckinModel.
enum class MDW_CHECKIN_MODEL
{
    UPLOAD       = 0,      ///< Checkout downloads a copy and Checkin uploads it
    WORKING_COPY = 1       ///< Items are files in a working copy, committed in place by Checkin
};

/// Numbering matches kiapi.workspace.WorkspaceErrorCode
enum class MDW_ERROR_CODE
{
    SUCCESS                   = 0,
    NOT_SUPPORTED             = 1,
    NOT_AUTHENTICATED         = 2,
    AUTH_FAILED               = 3,
    PERMISSION_DENIED         = 4,
    NETWORK                   = 5,
    TIMEOUT                   = 6,
    LOCK_CONFLICT             = 7,
    NOT_LOCK_OWNER            = 8,
    ITEM_NOT_FOUND            = 9,
    REVISION_NOT_FOUND        = 10,
    VERSION_CONFLICT          = 11,
    INVALID_STATE_TRANSITION  = 12,
    NO_CHANGES                = 13,
    CANCELLED                 = 14,
    INVALID_PARAMETER         = 15,
    BACKEND_LOST              = 16,
    PROTOCOL                  = 17,
    INTERNAL                  = 18,
    RATE_LIMITED              = 19,     ///< A quota or rate limit; see MDW_RESULT::retry_after_ms
    BUSY                      = 20,     ///< Another process holds a lock the backend needs; retry
    UNTRUSTED_HOST            = 21,     ///< MDW_RESULT::host_key needs the user's confirmation
    COPY_BEHIND               = 22      ///< The library moved on without touching the item; update the copy, retry
};


/// A translated line for a user saying what went wrong, without what was being attempted
KICOMMON_API wxString MdwErrorMessage( MDW_ERROR_CODE aCode );


/// The library part field holding its requested lifecycle state. It is metadata, so it never
/// takes part in a release digest.
constexpr const char MDW_LIFECYCLE_FIELD[] = "Lifecycle";


/// A server key a backend does not trust yet
struct MDW_HOST_KEY
{
    wxString host;
    unsigned port = 0;
    wxString key_type;
    wxString sha256_fingerprint;

    bool operator==( const MDW_HOST_KEY& aOther ) const = default;
};

/// One field of a credential form
struct MDW_AUTH_FIELD
{
    wxString field_id;
    wxString display_name;
    bool     secret = false;
    bool     required = false;
};

/// The credential form to ask the user to fill in
struct MDW_AUTH_CHALLENGE
{
    wxString                    realm;
    std::vector<MDW_AUTH_FIELD> fields;
    wxString                    username_hint;
};

/// The status every operation result carries; concrete results add their payload alongside
struct MDW_RESULT
{
    bool           success = false;
    MDW_ERROR_CODE error   = MDW_ERROR_CODE::SUCCESS;
    wxString       message;
    wxString       detail; ///< The transport's own words for a details view, never the headline

    unsigned                          retry_after_ms = 0;   ///< RATE_LIMITED, BUSY; 0 when unknown
    std::optional<MDW_AUTH_CHALLENGE> auth_challenge;       ///< NOT_AUTHENTICATED, AUTH_FAILED
    std::optional<MDW_HOST_KEY>       host_key;             ///< UNTRUSTED_HOST

    MDW_RESULT() = default;

    bool IsOk() const { return success; }

    static MDW_RESULT Ok()
    {
        MDW_RESULT r;
        r.success = true;
        return r;
    }

    static MDW_RESULT Fail( MDW_ERROR_CODE aCode, const wxString& aMessage = wxEmptyString )
    {
        MDW_RESULT r;
        r.success = false;
        r.error   = aCode;
        r.message = aMessage;
        return r;
    }
};


/// The backend's own reason when it gave one, else a sentence from the code, the server and the user
KICOMMON_API wxString MdwErrorMessage( const MDW_RESULT& aResult );

/// The headline followed by the transport's own text, for a surface with room for lines
KICOMMON_API wxString MdwErrorText( const MDW_RESULT& aResult );


#endif // MDW_TYPES_H
