/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef WORKSPACE_DETAIL_H
#define WORKSPACE_DETAIL_H

#include <kicommon.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/mdw_claim_text.h>
#include <widgets/mdw_member_index.h>

#include <wx/datetime.h>
#include <wx/string.h>

#include <optional>
#include <vector>


/// The State column for an item; empty without LIFECYCLE and for an editable state the backend does not name
KICOMMON_API wxString WorkspaceStateCell( const MDW_CAPABILITIES& aCaps, const MDW_LIFECYCLE_STATE& aState,
                                          const wxString& aRevisionLabel );

/**
 * The State cell for a team library member as its library lists it, with aCaps those of the library's source. It
 * reads as the library tree's Lifecycle cell does for the member unclaimed and unedited, and is empty without
 * LIFECYCLE.
 */
KICOMMON_API wxString WorkspaceMemberStateCell( const MDW_CAPABILITIES& aCaps, const MDW_MEMBER_ENTRY& aEntry );


/// One label and value pair of the Workspace page's details block
struct WORKSPACE_DETAIL_ROW
{
    wxString label;
    wxString value;
};


/// The details rows for an item, in display order. aModified is unset while the local changes are unknown.
KICOMMON_API std::vector<WORKSPACE_DETAIL_ROW> WorkspaceDetailRows( const MDW_CAPABILITIES& aCaps,
                                                                    const MDW_ITEM_INFO& aItem,
                                                                    const MDW_LOCK_INFO& aLock,
                                                                    std::optional<bool> aModified,
                                                                    const wxString& aFolderPath,
                                                                    const wxDateTime& aNow );

/**
 * The details rows for a team library member as its library lists it, with aCaps those of the library's source and
 * aFacts what the listing says of its newest release. aModified is unset while the local changes are unknown.
 */
KICOMMON_API std::vector<WORKSPACE_DETAIL_ROW> WorkspaceMemberDetailRows( const MDW_CAPABILITIES&              aCaps,
                                                                          const MDW_MEMBER_ENTRY&              aEntry,
                                                                          const std::optional<MDW_PART_FACTS>& aFacts,
                                                                          std::optional<bool> aModified,
                                                                          const wxDateTime&   aNow );

/// The Where used value for a member the design places at aReferences
KICOMMON_API wxString WorkspaceWhereUsedText( const std::vector<wxString>& aReferences );


/// One command the Workspace page offers for the selected item
struct WORKSPACE_ACTION
{
    enum class ID
    {
        OPEN,
        EDITING,
        RELEASE_CLAIM, ///< Remove another user's claim
        CHECK_OUT,
        CHECK_IN,
        CHANGE_STATE,
        SHOW_IN_FOLDER
    };

    ID       id;
    wxString label; ///< EDITING's and RELEASE_CLAIM's from MdwClaimActions, the others fixed
    bool     allowed = true;
    wxString reason; ///< Why not allowed, naming who holds the item or giving the source's refusal

    /// The claim command behind EDITING and RELEASE_CLAIM, since Start and Stop Editing share one id
    MDW_CLAIM_ACTION::KIND claim = MDW_CLAIM_ACTION::KIND::START;
};


/// What decides the actions an item offers
struct WORKSPACE_ACTION_FACTS
{
    bool          locking = false;   ///< Source advertises LOCKING
    bool          inPlace = false;   ///< Files are edited in place and committed, not checked out and in
    bool          member = false;    ///< A team library part, not a design file
    bool          localFile = false; ///< The checkout or library copy exists on disk
    bool          openable = false;  ///< The host can open the file as the project tree does
    MDW_LOCK_INFO lock;
    std::vector<kiapi::workspace::LifecycleTransition> transitions; ///< Empty when unread or not supported
    bool transitionsReading = false; ///< The transitions are being read and none are known yet

    /// The item's permission to transition, PS_UNKNOWN until read or where the source has no permissions
    kiapi::workspace::Permission transition;
};


/// The actions an item offers, in menu order, with those not allowed kept so a menu can say why
KICOMMON_API std::vector<WORKSPACE_ACTION> WorkspaceItemActions( const WORKSPACE_ACTION_FACTS& aFacts );

/// Why aAction is not allowed, after its label, or empty when it is allowed or its label already names the holder
KICOMMON_API wxString WorkspaceActionRefusal( const WORKSPACE_ACTION& aAction );

#endif // WORKSPACE_DETAIL_H
