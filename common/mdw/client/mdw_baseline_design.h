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
 * @file mdw_baseline_design.h
 * Checks a whole design's placed parts. Each owning source answers the facts for its own members,
 * and the project's source applies its policy to all of them.
 */

#ifndef MDW_CLIENT_BASELINE_DESIGN_H
#define MDW_CLIENT_BASELINE_DESIGN_H

#include <kicommon.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_eco.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_review.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class MDW_SOURCE;

/// Finds a configured source by its credential-free locator, or null when none is configured
using MDW_SOURCE_LOOKUP = std::function<std::shared_ptr<MDW_SOURCE>( const wxString& aLocator )>;

enum class MDW_PLACED_KIND
{
    SYMBOL,
    FOOTPRINT
};

/// One placed symbol or footprint
struct MDW_PLACED_INPUT
{
    MDW_PLACED_KIND             kind = MDW_PLACED_KIND::SYMBOL;
    wxString                    reference;
    wxString                    lib_id; ///< Display only
    wxString                    content_digest;
    std::optional<MDW_ITEM_REF> member;    ///< Unset when no library source owns the part
    wxString                    pin_label; ///< The pin of the library copy it came from; empty when unpinned
    MDW_ITEM_REF                part;      ///< The MDW_Part identity; empty item_id when the symbol has none
    wxString                    part_revision_id;
    wxString                    team_source; ///< Without a member, the team source its library nickname names
    wxString                    team_locator; ///< team_source's locator; empty when only a row marker names it
};

/// Placements that share member, digest and part, which every source would judge alike
struct MDW_DESIGN_PART
{
    MDW_PLACED_KIND   kind = MDW_PLACED_KIND::SYMBOL;
    MDW_BASELINE_PART baseline; ///< references holds every placement
    wxString          pin_label;
    wxString          team_source; ///< Without a member, the team source its library nickname names
    wxString          team_locator;
};

struct MDW_DESIGN_CHECK
{
    std::vector<MDW_DESIGN_PART>    parts;  ///< With owner_facts attached, ready for Create
    std::vector<MDW_BASELINE_CHECK> checks; ///< One per part, in order
    MDW_BASELINE_HOLD               hold = MDW_BASELINE_HOLD::NONE;
    wxString                        holder; ///< Display name of the user holding a LOCKED project
};

/**
 * Group placements into baseline parts, keeping the first-seen order. A part from no library source
 * keeps an empty item and is grouped by its library id.
 */
KICOMMON_API std::vector<MDW_DESIGN_PART> MdwGroupPlacedParts( const std::vector<MDW_PLACED_INPUT>& aInputs );

/**
 * The exceptions recorded against aOld's parts, renumbered to the same parts in aNew as MdwGroupPlacedParts
 * groups them. An exception whose part aNew no longer has is dropped.
 */
KICOMMON_API std::vector<MDW_BASELINE_EXCEPTION>
MdwCarryExceptions( const std::vector<MDW_DESIGN_PART>& aOld, const std::vector<MDW_BASELINE_EXCEPTION>& aExceptions,
                    const std::vector<MDW_DESIGN_PART>& aNew );

/**
 * Ask every owning source other than aProject for its members' facts, then aProject's verdict on all
 * parts. A part no library source owns, or whose source does not track releases, goes to aProject as
 * UNKNOWN with the reason. A part without a member whose team_source is set, or whose source is not set up,
 * not connected or fails, goes as UNVERIFIED, which blocks. Facts a part carries from an earlier check are
 * asked again.
 *
 * aLookup defaults to MDW_CLIENT::FindSource.
 */
KICOMMON_API MDW_OP MdwCheckDesign( const std::shared_ptr<MDW_SOURCE>& aProject, const MDW_ITEM_REF& aProjectRef,
                                    const wxString& aRevisionId, std::vector<MDW_DESIGN_PART> aParts,
                                    MDW_DONE<MDW_DESIGN_CHECK> aDone, MDW_SOURCE_LOOKUP aLookup = nullptr );

/// A short status for one part, from its facts, e.g. "Differs from revision 3"
KICOMMON_API wxString MdwPlacedStatusText( const MDW_PART_FACTS& aFacts );

/// A change order and the locator of the source that holds it
struct MDW_SOURCED_ECO
{
    wxString        source;
    kiapi::mdw::Eco eco;
};

/// Open change orders touching a design, matched to the project item and to each part
struct MDW_DESIGN_ECOS
{
    std::vector<MDW_SOURCED_ECO>     ecos;     ///< Each once
    std::vector<size_t>              design;   ///< Indices into ecos of those on the project item
    std::vector<std::vector<size_t>> parts;    ///< Per design part, indices into ecos
    std::vector<wxString>            failures; ///< One line per source that could not answer
};

/**
 * Ask every source in aSources that has ECO for the open change orders on the project item and
 * on each part's member and MDW_Part identity. A source that fails is listed in failures; the op
 * itself fails only when every source does.
 */
KICOMMON_API MDW_OP MdwListDesignEcos( const std::vector<std::shared_ptr<MDW_SOURCE>>& aSources,
                                       const MDW_ITEM_REF& aProjectRef, const std::vector<MDW_DESIGN_PART>& aParts,
                                       MDW_DONE<MDW_DESIGN_ECOS> aDone );

/// What MdwReadReviews read
struct MDW_REVIEW_READ
{
    std::vector<kiapi::mdw::Review> reviews;
    bool                            approval_required = false;
    bool                            partial = false; ///< The page limit stopped a read that allowed it
    bool unattributed = false; ///< The revision filter was refused, so every revision is listed
};

/**
 * Every review of aItem across all pages, filtered to aRevisionId, or of any revision when it is empty. A filter
 * answered NOT_SUPPORTED is retried without it. Pages are merged; approval_required holds if any page says so.
 * aCap says what the page limit does, and a PARTIAL read that it stopped sets partial.
 */
KICOMMON_API MDW_OP MdwReadReviews( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                                    const wxString& aRevisionId, MDW_DONE<MDW_REVIEW_READ> aDone,
                                    MDW_READ_CAP aCap = MDW_READ_CAP::FAIL );

/**
 * Every review of aItem across all pages, of any revision, so MdwShownReview can name an approval
 * of another revision. Pages are merged; approval_required holds if any page says so. Fails when
 * the page cap stopped the read.
 */
KICOMMON_API MDW_OP MdwListDesignReviews( const std::shared_ptr<MDW_SOURCE>& aSource, const MDW_ITEM_REF& aItem,
                                          MDW_DONE<kiapi::mdw::ListReviewsReply> aDone );

/**
 * The review state of the revision being released, as Release Design shows it. Advisory only, since the
 * backend's CreateBaseline enforces its own policy, so a failed read never blocks.
 */
struct MDW_REVIEW_GATE
{
    bool approved = false; ///< An approved review of aItem covers this exact revision
    bool blocks = false;   ///< Approval is required and missing
};

/// True when aReview names aItem; a review without an item names nothing
KICOMMON_API bool MdwReviewIsOf( const kiapi::mdw::Review& aReview, const MDW_ITEM_REF& aItem );

/// The reviews in aList naming aItem, which are the only ones Release Design may show
KICOMMON_API std::vector<kiapi::mdw::Review> MdwReviewsOf( const kiapi::mdw::ListReviewsReply& aList,
                                                           const MDW_ITEM_REF&                 aItem );

/**
 * Only an approved review naming aItem and aRevisionId satisfies the gate.  Which review to show is
 * MdwShownReview's choice among MdwReviewsOf.
 */
KICOMMON_API MDW_REVIEW_GATE MdwReviewGate( const kiapi::mdw::ListReviewsReply& aList, const MDW_ITEM_REF& aItem,
                                            const wxString& aRevisionId );

#endif // MDW_CLIENT_BASELINE_DESIGN_H
