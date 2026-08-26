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
 * @file mdw_claim_text.h
 * @brief The wording of every editing claim message.
 *
 * A claim is advisory when the backend accepts another user's check-in while it is held, as git does, and
 * enforced when the backend refuses it, as an Odoo check out does. Each surface words a claim through these
 * functions so the two kinds never read alike.
 */

#ifndef MDW_CLAIM_TEXT_H
#define MDW_CLAIM_TEXT_H

#include <kicommon.h>
#include <mdw/client/mdw_lock.h>

#include <optional>
#include <vector>

#include <wx/datetime.h>
#include <wx/string.h>

struct MDW_EVENT;


/// One claim command a menu offers for an item
struct MDW_CLAIM_ACTION
{
    enum class KIND
    {
        START,  ///< Take the claim
        STOP,   ///< Give up our own claim
        RELEASE ///< Remove another user's claim, with a reason the holder is told
    };

    KIND          kind = KIND::START;
    wxString      label;
    bool          enabled = true;
    wxString      confirm; ///< The question asked before a RELEASE
    MDW_LOCK_INFO claim;   ///< The claim a RELEASE names, which the backend refuses to remove once replaced
};


/// Who holds aLock, by name where the backend gives one
KICOMMON_API wxString MdwClaimHolder( const MDW_LOCK_INFO& aLock );

/// Who holds aLock and since when, such as "Being edited by dave since 15/10/26 (30 days)". Empty when known free
KICOMMON_API wxString MdwClaimLabel( const MDW_LOCK_INFO& aLock, const wxDateTime& aNow );

/// Why saving aItem under another user's claim needs a decision
KICOMMON_API wxString MdwClaimSavePrompt( const MDW_LOCK_INFO& aLock, const wxString& aItem );

/**
 * The warning before committing aItem under another user's claim, or aContested such files when there are
 * several. aIHoldOne says the committer holds a claim in the commit too, which alone makes the claim "also".
 */
KICOMMON_API wxString MdwClaimCommitWarning( const MDW_LOCK_INFO& aLock, const wxString& aItem, bool aIHoldOne,
                                             size_t aContested = 1 );

/**
 * The news in an ITEM_UNLOCKED event whose payload says this user's claim was released by someone else, or
 * nothing when the claim was another user's or this user released it. A payload naming no releaser reads
 * only that the claim is gone.
 */
KICOMMON_API std::optional<wxString> MdwClaimNotice( const MDW_EVENT& aEvent );

/// The claim commands aLock allows, in menu order
KICOMMON_API std::vector<MDW_CLAIM_ACTION> MdwClaimActions( const MDW_LOCK_INFO& aLock );

/// The RELEASE command of MdwClaimActions(), which only another user's claim offers
KICOMMON_API std::optional<MDW_CLAIM_ACTION> MdwClaimRelease( const MDW_LOCK_INFO& aLock );

#endif // MDW_CLAIM_TEXT_H
