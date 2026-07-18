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
 * @file mdw_part.h
 * One item's lifecycle and claim, the operations behind a library part's infobar.
 */

#ifndef MDW_CLIENT_PART_H
#define MDW_CLIENT_PART_H

#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_op.h>

#include <memory>
#include <vector>

class MDW_SOURCE;

struct MDW_TRANSITION
{
    MDW_CHANGE_OUTCOME  outcome = MDW_CHANGE_OUTCOME::UNKNOWN;
    MDW_LIFECYCLE_STATE state;              ///< APPLIED
    MDW_PENDING_CHANGE  pending;            ///< PENDING
};

/**
 * Publish an edited item as its next released revision. base_revision_id is the release the
 * edit started from, empty for a first release; content_digest is the library digest of the
 * content at local_path, and member_digest the digest of all of it when that differs.
 */
struct MDW_RELEASE_REQUEST
{
    wxString base_revision_id;
    wxString local_path;
    wxString content_digest;
    wxString member_digest;
    wxString comment;
    wxString requested_label;               ///< Empty for the backend's next label
};

class KICOMMON_API MDW_PART
{
public:
    MDW_PART( std::shared_ptr<MDW_SOURCE> aSource, const MDW_ITEM_REF& aItem );

    /// The states this item can be in; definitions can differ between items of one source
    MDW_OP Definition( MDW_DONE<kiapi::workspace::LifecycleDefinition> aDone );

    /// An empty aRevisionId means the latest revision
    MDW_OP Transitions( const wxString&                                              aRevisionId,
                        MDW_DONE<std::vector<kiapi::workspace::LifecycleTransition>> aDone );
    MDW_OP Transition( const wxString& aRevisionId, const wxString& aTargetStateId, const wxString& aComment,
                       MDW_DONE<MDW_TRANSITION> aDone );
    MDW_OP Release( const MDW_RELEASE_REQUEST& aRequest, MDW_DONE<MDW_CHANGE> aDone );

    /// Checkout with lock on a released item; the lock is the claim
    MDW_OP Revise( const wxString& aLocalPath, MDW_DONE<MDW_CHECKOUT_RESULT> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
    MDW_ITEM_REF                m_item;
};

#endif // MDW_CLIENT_PART_H
