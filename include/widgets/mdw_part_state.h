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
 * @file mdw_part_state.h
 * @brief What a library editor shows and allows for a part its workspace manages.
 *
 * The lifecycle state of a part's latest revision, the claim on it, its latest release and the
 * permissions all come from the part's source. Only the digest of the content as edited now is
 * local, to say whether it still matches that release. A part is read-only while its state is
 * anything but editable, unless the user holds its claim. A failed or missing read leaves the part
 * editable, since the source still refuses in the call.
 */

#ifndef MDW_PART_STATE_H
#define MDW_PART_STATE_H

#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_revision.h>

#include <wx/string.h>

#include <optional>
#include <vector>


/// One lifecycle action a part offers, which the infobar and the library tree menus show alike
struct KICOMMON_API MDW_PART_ACTION
{
    enum class ID
    {
        CHECK_IN,
        REVISE,
        CANCEL_REVISION,
        RELEASE,
        RELEASE_CLAIM, ///< Remove another user's claim, telling them why
        TRANSITION,
        HISTORY
    };

    enum class STATE
    {
        ALLOWED,
        DENIED
    };

    explicit MDW_PART_ACTION( ID aId, STATE aState = STATE::ALLOWED, const wxString& aReason = wxEmptyString ) :
            id( aId ),
            state( aState ),
            reason( aReason )
    {
    }

    ID       id;
    STATE    state;
    wxString reason; ///< Why DENIED, from the source, or a warning to show beside an ALLOWED action
    wxString target; ///< The state a TRANSITION moves to
    wxString label;  ///< A TRANSITION's name as the source gives it, or the RELEASE_CLAIM command naming the holder

    bool Allowed() const { return state == STATE::ALLOWED; }
};


struct KICOMMON_API MDW_PART_STATE
{
    bool                              lifecycle = false;    ///< The source advertises LIFECYCLE
    bool                              locking = false;      ///< The source advertises LOCKING
    bool                              known = false;        ///< state was read
    MDW_LIFECYCLE_STATE               state;
    wxString                          stateName;            ///< From the item's lifecycle definition
    std::optional<MDW_REVISION>       release;              ///< The latest release, when read
    wxString                          digest;               ///< The content as edited now
    wxString                          memberDigest;         ///< All of it, when the editor tells it apart
    MDW_LOCK_INFO                     lock;
    std::vector<kiapi::workspace::Permission> permissions;
    std::vector<kiapi::workspace::Permission> sourcePermissions; ///< The source's, which stand where the item has none
    std::optional<MDW_PENDING_CHANGE> pending;              ///< A release awaiting approval
    std::vector<kiapi::workspace::LifecycleTransition> transitions; ///< What the source offers for the latest revision
    bool                              listed = true;        ///< False for a part its library does not list yet

    bool IsRevising() const { return lock.is_locked && lock.locked_by_me; }

    /// Someone else's claim, which this user cannot write past
    bool IsHeldByOther() const { return lock.is_locked && !lock.locked_by_me && !lock.advisory; }

    /// Someone else's claim of either kind, which blocks taking one of our own
    bool IsClaimedByOther() const { return lock.is_locked && !lock.locked_by_me; }

    /// The content is what the latest release stored; false when either digest is unknown
    bool MatchesRelease() const;

    bool IsReadOnly() const;

    /// The item's permission, else the source's, else UNKNOWN, which allows until the call itself refuses. An answer
    /// the item gives wins even when UNKNOWN.
    kiapi::workspace::Permission Permission( const std::string& aName ) const;

    /**
     * What the part offers now, in menu order. An action the part's state rules out is left out, and one the source
     * refuses is DENIED with the source's reason.
     */
    std::vector<MDW_PART_ACTION> Actions() const;

    /// Why an edit to the read-only part aName was not kept, naming whoever holds its claim
    wxString EditRefusal( const wxString& aName ) const;

    /// The state's name, falling back to its category's when the definition does not name it
    wxString StateName() const;

    /// One infobar line joining the name and what is known with middle dots. A long name is shortened in
    /// the middle, so the line is not cut through what follows it.
    wxString Describe( const wxString& aName ) const;

    /// The longest name Describe() leaves whole
    static constexpr size_t MAX_NAME_LENGTH = 32;

    /// aName shortened in the middle to MAX_NAME_LENGTH characters
    static wxString ElideName( const wxString& aName );

    /// A library tree's Lifecycle cell for the listed state aState, the claim aLock and whether the part has
    /// unsaved edits
    static wxString TreeCell( const wxString& aState, const MDW_LOCK_INFO& aLock, bool aModified );
};

#endif // MDW_PART_STATE_H
