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
 * @file mdw_baseline.h
 * Design baselines (CG_REVISIONS), which check and release a design with its placed parts pinned.
 */

#ifndef MDW_CLIENT_BASELINE_H
#define MDW_CLIENT_BASELINE_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>

#include <api/mdw/backend_revisions.pb.h>

#include <memory>
#include <optional>
#include <vector>

class MDW_SOURCE;

enum class MDW_SEVERITY
{
    NONE     = 0,
    WARNING  = 1,
    BLOCKING = 2                ///< SEV_ERROR; create refuses the part unless an exception names it
};

/// Why the project source would refuse a release of the design now, whatever its parts
enum class MDW_BASELINE_HOLD
{
    NONE        = 0,
    UNCOMMITTED = 1,            ///< The project has changes no revision holds
    DETACHED    = 2,            ///< The project is on no branch
    NO_REMOTE   = 3,            ///< The project has nowhere to share a release
    LOCKED      = 4             ///< Another user has the project checked out
};

/// What the source owning a placed member knows about it
struct MDW_PART_FACTS
{
    std::vector<kiapi::mdw::PlacedPartStatus> statuses; ///< Every condition that applies
    wxString                                  matched_revision_id;
    wxString                                  matched_revision_label;
    wxString                                  latest_revision_label;
    wxString                                  message;
    MDW_LIFECYCLE_STATE                       matched_state;
};

/**
 * A library member placed in the design, and the part identity its symbol carries, if any.
 * owner_facts holds the owning source's answer when that is not the project's source.
 */
struct MDW_BASELINE_PART
{
    MDW_ITEM_REF                  item;
    wxString                      lib_id;           ///< As placed, for display
    wxString                      content_digest;   ///< Library digest of the member as placed
    std::vector<wxString>         references;
    MDW_ITEM_REF                  part;             ///< From MDW_Part; empty when the symbol has none
    wxString                      part_revision_id;
    std::optional<MDW_PART_FACTS> owner_facts;
};

/// The project source's verdict on the request's parts[part_index]
struct MDW_BASELINE_CHECK
{
    unsigned       part_index = 0;
    MDW_PART_FACTS facts;
    MDW_SEVERITY   severity = MDW_SEVERITY::NONE;
};

/// Why parts[part_index] may be released as placed despite its check
struct MDW_BASELINE_EXCEPTION
{
    unsigned    part_index = 0;
    wxString    note;
    MDW_ECO_REF eco;
};

/// The project source's verdict on a baseline request
struct MDW_BASELINE_VERDICT
{
    std::vector<MDW_BASELINE_CHECK> checks; ///< One per part, in order
    MDW_BASELINE_HOLD               hold = MDW_BASELINE_HOLD::NONE;
    wxString                        holder; ///< Display name of the user holding a LOCKED project
};

/// Read a CheckBaseline reply for aCount request parts, false when it is malformed
KICOMMON_API bool MdwReadBaselineVerdict( const kiapi::mdw::CheckBaselineReply& aReply, size_t aCount,
                                          MDW_BASELINE_VERDICT& aVerdict );

struct MDW_BASELINE_REQUEST
{
    MDW_ITEM_REF                        project;
    wxString                            revision_id;    ///< The project revision to release
    std::vector<MDW_BASELINE_PART>      parts;
    std::vector<MDW_BASELINE_EXCEPTION> exceptions;
    wxString                            comment;
    wxString                            requested_label;
};

class KICOMMON_API MDW_BASELINE
{
public:
    explicit MDW_BASELINE( std::shared_ptr<MDW_SOURCE> aSource );

    /// Asked of the source owning the members; one answer per part, in order
    MDW_OP Facts( const std::vector<MDW_BASELINE_PART>& aParts, MDW_DONE<std::vector<MDW_PART_FACTS>> aDone );

    /// Asked of the project's source
    MDW_OP Check( const MDW_ITEM_REF& aProject, const wxString& aRevisionId,
                  const std::vector<MDW_BASELINE_PART>& aParts, MDW_DONE<MDW_BASELINE_VERDICT> aDone );
    MDW_OP Create( const MDW_BASELINE_REQUEST& aRequest, MDW_DONE<MDW_CHANGE> aDone );

private:
    std::shared_ptr<MDW_SOURCE> m_source;
};

#endif // MDW_CLIENT_BASELINE_H
