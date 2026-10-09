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
 * @file mdw_source.h
 * One configured workspace source, either a design workspace or a library source.
 */

#ifndef MDW_CLIENT_SOURCE_H
#define MDW_CLIENT_SOURCE_H

#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_op.h>

#include <functional>
#include <memory>
#include <vector>

class MDW_BASELINE;
class MDW_BOM;
class MDW_ECO;
class MDW_LIBRARY;
class MDW_PART;
class MDW_PART_CATALOG;
class MDW_REVIEW;
class MDW_SEARCH;
class MDW_BACKEND;
class MDW_SESSION;

/**
 * An item that differs under a local root from what the backend last wrote there. In a library
 * copy it is one member; under the binding's local root it is one file and library_id and name are
 * empty. A provisional id is valid only for a library CheckIn on the same copy until the copy next
 * changes, so list again after every Materialize, Update or check-in and never store one.
 */
struct MDW_LOCAL_CHANGE
{
    MDW_ITEM_REF      member;
    wxString          library_id;
    wxString          name;
    MDW_CHANGE_TYPE   change = MDW_CHANGE_TYPE::UNKNOWN;
    MDW_DOCUMENT_KIND kind = MDW_DOCUMENT_KIND::UNKNOWN;
    wxString          relative_path; ///< The file under the local root
    bool              provisional = false;
    wxString          base_revision_id;   ///< What the copy holds; empty for ADDED
    wxString          base_relative_path; ///< The file the source holds it in; empty when unknown or ADDED

    /// The format version the source holds the member's file in at the copy's snapshot, read at base_relative_path
    /// or else relative_path. Empty when it holds no such file or cannot tell.
    wxString base_format;

    /// Whether the member may be checked in. DENIED disables its row with the reason; UNKNOWN leaves it to the call
    kiapi::workspace::Permission checkin;

    /// Moved unchanged from base_relative_path to relative_path, which checks in and keeps the member's release
    bool moved = false;

    /// The digest of the text listed for a library file that was only reformatted. Such a row has no member and
    /// checks in the file as it is on disk.
    wxString format_digest;

    /// The digest of the text listed for relative_path when it was re-saved, its other changes only reformatting
    /// symbols or its format newer than the source holds, and how many rows edit that file. Checking in all of them
    /// checks in the file as it is on disk, so the source holds the re-save too and the copy no longer differs from it.
    wxString file_digest;
    size_t   file_edits = 0;
};

/**
 * A source answers the mandatory requests and owns its group facades. Every operation checks
 * the capability it needs and completes NOT_SUPPORTED, without a round trip, when it is absent.
 *
 * It is bound to the session that serves it, which is looked up on every use. A source can so be handed
 * out before its session opens and stays valid across backend restarts, and one that outlives its project
 * reports itself empty rather than keeping a closed backend alive.
 */
class KICOMMON_API MDW_SOURCE : public std::enable_shared_from_this<MDW_SOURCE>
{
public:
    using RESOLVER = std::function<std::shared_ptr<MDW_SESSION>()>;

    explicit MDW_SOURCE( std::weak_ptr<MDW_SESSION> aSession );

    /// aResolve answers the session now serving aLocator, or null while none is open
    MDW_SOURCE( RESOLVER aResolve, const wxString& aLocator );

    ~MDW_SOURCE();

    MDW_SOURCE_INFO  Info() const;
    MDW_CAPABILITIES Capabilities() const;
    bool             Has( MDW_CAPABILITY_GROUP aGroup ) const;

    /// An empty aRevisionId describes the latest revision, for every check-in model
    MDW_OP Item( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, MDW_DONE<MDW_ITEM_INFO> aDone );
    MDW_OP Items( const wxString& aFolderId, const wxString& aFilter, const MDW_PAGE& aPage,
                  MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone );

    MDW_OP RootFolders( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone );

    /// The child folders of aParentFolderId, which may be empty since a backend may give a root folder that id
    MDW_OP ChildFolders( const wxString& aParentFolderId, const MDW_PAGE& aPage,
                         MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone );

    MDW_OP Checkout( const MDW_CHECKOUT_REQUEST& aRequest, MDW_DONE<MDW_CHECKOUT_RESULT> aDone );

    /// Make a local file a new item, whose first revision the result's item names. A backend that cannot add
    /// completes NOT_SUPPORTED
    MDW_OP AddItem( const MDW_ADD_ITEM_REQUEST& aRequest, MDW_DONE<MDW_CHECKOUT_RESULT> aDone );
    MDW_OP Checkin( const MDW_CHECKIN_REQUEST& aRequest, MDW_DONE<MDW_REVISION> aDone );
    MDW_OP CancelCheckout( const MDW_ITEM_REF& aItem, const wxString& aLocalPath, MDW_DONE<MDW_NONE> aDone );
    MDW_OP PendingChange( const wxString& aChangeId, MDW_DONE<MDW_PENDING_INFO> aDone );

    /**
     * The items at aRelativePaths under the binding's local root, one per path in order, with the
     * revision the working copy holds; an empty item id means the file is not an item. May fail
     * NOT_SUPPORTED on an upload backend.
     */
    MDW_OP Locate( const std::vector<wxString>& aRelativePaths, MDW_DONE<std::vector<MDW_LOCATED_ITEM>> aDone );

    /**
     * What changed under aLocalRoot, the binding's local root or a library copy. May fail
     * NOT_SUPPORTED on an upload backend.
     */
    MDW_OP LocalChanges( const wxString& aLocalRoot, const MDW_PAGE& aPage,
                         MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone );

    MDW_OP Lock( const MDW_ITEM_REF& aItem, MDW_DONE<MDW_LOCK_INFO> aDone );

    /**
     * With aForce, remove another user's lock; aReason tells them why. aExpected is the lock the user confirmed
     * removing, which the backend refuses BUSY once another claim holds the item. Ignored without aForce.
     */
    MDW_OP Unlock( const MDW_ITEM_REF& aItem, bool aForce, MDW_DONE<MDW_LOCK_INFO> aDone,
                   const wxString& aReason = wxEmptyString,
                   const std::optional<MDW_LOCK_INFO>& aExpected = std::nullopt );

    /// The backend's view of aItem's lock, which also refreshes the session's lock tracker
    MDW_OP LockInfo( const MDW_ITEM_REF& aItem, MDW_DONE<MDW_LOCK_INFO> aDone );

    /**
     * The lock on aItem as the session's lock tracker holds it, without asking the backend. The tracker is
     * filled when the session comes online and kept by events, and outlives the backend.
     */
    MDW_LOCK_INFO Claim( const MDW_ITEM_REF& aItem ) const;

    /// An empty aFolderId lists every lock in the workspace
    MDW_OP Locks( const wxString& aFolderId, const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_ITEM_LOCK>> aDone );

    /// Newest first
    MDW_OP History( const MDW_ITEM_REF& aItem, const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_REVISION>> aDone );

    /// An empty aRevisionId downloads the latest; aDone gets the revision written
    MDW_OP Download( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, const wxString& aLocalPath,
                     MDW_DONE<MDW_REVISION> aDone );

    /// An empty aItem asks for workspace-level permissions
    MDW_OP Permissions( const MDW_ITEM_REF& aItem, const wxString& aRevisionId,
                        MDW_DONE<std::vector<kiapi::workspace::Permission>> aDone );

    MDW_PART         Part( const MDW_ITEM_REF& aItem );
    MDW_PART_CATALOG Parts();
    MDW_LIBRARY      Libraries();
    MDW_BASELINE     Baselines();
    MDW_SEARCH       Search();
    MDW_BOM          Bom();
    MDW_ECO          Ecos();
    MDW_REVIEW       Reviews();

    /// The session serving this source, or null while none is open. Client implementations only
    std::shared_ptr<MDW_SESSION> Session() const { return m_resolve ? m_resolve() : nullptr; }

    /// The running backend process, or null while none runs. Client implementations only
    std::shared_ptr<MDW_BACKEND> Backend() const;

private:
    RESOLVER m_resolve;
    wxString m_locator; ///< Known before the session opens
};

#endif // MDW_CLIENT_SOURCE_H
