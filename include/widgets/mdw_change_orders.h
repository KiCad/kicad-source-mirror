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
 * @file mdw_change_orders.h
 * Wording for a workspace's change orders and reviews. Every decision reads the state category,
 * never the backend's display string.
 */

#ifndef MDW_CHANGE_ORDERS_H
#define MDW_CHANGE_ORDERS_H

#include <kicommon.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_eco.h>
#include <mdw/client/mdw_review.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>


/// The page a Change Orders and Reviews window opens on
enum class MDW_CHANGE_ORDERS_PAGE
{
    CHANGE_ORDERS,
    REVIEWS
};


/// What a Change Orders and Reviews window lists
struct MDW_CHANGE_ORDERS_SUBJECT
{
    wxString                    name;        ///< Shown in the title, such as the part or project name
    std::vector<MDW_ITEM_REF>   eco_items;   ///< Change orders affecting any of these
    std::optional<MDW_ITEM_REF> review_item; ///< Reviews of this item
    wxString                    revision_id; ///< The revision the reviews are about; empty for every one
    MDW_CHANGE_ORDERS_PAGE      page = MDW_CHANGE_ORDERS_PAGE::CHANGE_ORDERS; ///< When the source keeps that page
};


/**
 * The open design as a Change Orders and Reviews subject, named after its project, with the reviews of the
 * revision it is based on, or of every revision when that is unknown.
 */
KICOMMON_API MDW_CHANGE_ORDERS_SUBJECT MdwDesignSubject( const wxString& aProjectName, const MDW_PROJECT_STATE& aState,
                                                         MDW_CHANGE_ORDERS_PAGE aPage );


/// Draft, open, approved and unknown categories are open; the rest are closed
KICOMMON_API bool MdwEcoIsOpen( kiapi::mdw::EcoState aState );

KICOMMON_API std::vector<kiapi::mdw::Eco> MdwOpenEcos( const std::vector<kiapi::mdw::Eco>& aEcos );

/// "Open change orders on this design: ECO-0007, ECO-0009.", or empty when there are none
KICOMMON_API wxString MdwDesignEcoNote( const std::vector<kiapi::mdw::Eco>& aOpenEcos );

/// The display id, or the title for a change order without one; opaque ids are never shown
KICOMMON_API wxString MdwEcoName( const kiapi::mdw::Eco& aEco );

/**
 * The review to show for aRevisionId: an approved one wins, then an open one, then one that
 * asks for changes or rejects. An empty aRevisionId considers every revision. Null when none fits.
 */
KICOMMON_API const kiapi::mdw::Review* MdwPickReview( const std::vector<kiapi::mdw::Review>& aReviews,
                                                      const wxString&                        aRevisionId );

/**
 * The reviews of exactly aRevisionId, in order.  A review stating no revision is dropped.  An empty
 * aRevisionId keeps every review.  A backend may ignore the revision filter it was sent.
 */
KICOMMON_API std::vector<kiapi::mdw::Review> MdwReviewsOfRevision( const std::vector<kiapi::mdw::Review>& aReviews,
                                                                   const wxString&                        aRevisionId );

/**
 * The review MdwReviewStatusLine describes for aRevisionId.  This is MdwPickReview, or when that
 * finds nothing for a named revision, an approval that names no revision and then an approval of
 * another revision.  Null when the line names no review.  The result points into aReviews and is
 * valid only while that vector is alive and unmodified.
 */
KICOMMON_API const kiapi::mdw::Review* MdwShownReview( const std::vector<kiapi::mdw::Review>& aReviews,
                                                       const wxString&                        aRevisionId );

/**
 * One line for the review state of aRevisionId, such as "Review: Waiting for Bob (DR-12).",
 * or the other-revision and no-review sentences when nothing for aRevisionId applies.
 * aSourceName names the backend when it cannot say which revision an approval covers.
 */
KICOMMON_API wxString MdwReviewStatusLine( const std::vector<kiapi::mdw::Review>& aReviews, const wxString& aRevisionId,
                                           const wxString& aSourceName );

/// The reviewer's display name; the user id is opaque and never shown
KICOMMON_API wxString MdwReviewerName( const kiapi::mdw::Reviewer& aReviewer );

/// The details box body; attributes take their names and order from aSchema
KICOMMON_API wxString MdwEcoDetailsHtml( const kiapi::mdw::Eco& aEco, const kiapi::workspace::Schema& aSchema );
KICOMMON_API wxString MdwReviewDetailsHtml( const kiapi::mdw::Review& aReview );

#endif // MDW_CHANGE_ORDERS_H
