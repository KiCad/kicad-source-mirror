/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_lifecycle.h
 * @brief The lifecycle group and the placed-part checks, served over a backend's own release records.
 *
 * A backend implements MDW_LIFECYCLE_STORE. The runtime registers the requests and their recoveries, judges placed
 * parts and checks baseline requests, so every backend answers those the same way.
 */

#ifndef MDW_LIFECYCLE_H
#define MDW_LIFECYCLE_H

#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/backend_lifecycle.pb.h>
#include <api/mdw/backend_revisions.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

/// One approved release of an item
struct MDW_RELEASE_RECORD
{
    std::string                      revision_id;
    std::string                      label;
    std::string                      digest; ///< The released content's digest
    kiapi::workspace::LifecycleState state;
    bool                             published = true; ///< False until the shared store holds it
};

/// The project revision a baseline releases, as the store found it
struct MDW_BASELINE_SOURCE
{
    std::string revision_id;
    std::string digest;
    std::string next_label; ///< The label the baseline takes

    /// Why the item cannot take a baseline now, such as another user's lock. Only a create fills it
    std::optional<kiapi::workspace::WorkspaceError> held;

    /// Every call fills it, so the release dialog and a create refusal never disagree
    kiapi::mdw::BaselineHold hold = kiapi::mdw::HOLD_NONE;

    std::string holder; ///< Display name of the user holding a HOLD_LOCKED item
};

/// A baseline request that passed every check, ready for the store to record
struct MDW_BASELINE_PLAN
{
    MDW_BASELINE_SOURCE                      source;
    std::vector<kiapi::mdw::PlacedPartCheck> checks; ///< One per part, in request order
};


/**
 * One backend's lifecycles and releases. The runtime calls each method through MDW_LIFECYCLE_HOOKS::use, so an
 * implementation takes only locks of its own.
 */
class MDW_LIFECYCLE_STORE
{
public:
    virtual ~MDW_LIFECYCLE_STORE() = default;

    /// The backend as messages name it, such as "PLM"
    virtual std::string Name() const = 0;

    virtual kiapi::workspace::WorkspaceError Definition( const kiapi::mdw::GetLifecycleDefinitionRequest& aRequest,
                                                         kiapi::mdw::GetLifecycleDefinitionReply&         aReply ) = 0;

    virtual kiapi::workspace::WorkspaceError Transitions( const kiapi::mdw::GetAvailableTransitionsRequest& aRequest,
                                                          kiapi::mdw::GetAvailableTransitionsReply& aReply ) = 0;

    virtual kiapi::workspace::WorkspaceError Transition( const kiapi::mdw::TransitionLifecycleRequest& aRequest,
                                                         kiapi::mdw::TransitionLifecycleReply&         aReply ) = 0;

    /// aToken is what ReleaseToken captured before this attempt
    virtual kiapi::workspace::WorkspaceError Release( const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                                      const std::string&                        aToken,
                                                      kiapi::mdw::ReleaseRevisionReply&         aReply ) = 0;

    /// Record a baseline. MdwPlanBaseline runs the checks every backend shares
    virtual kiapi::workspace::WorkspaceError Baseline( const kiapi::mdw::CreateBaselineRequest& aRequest,
                                                       kiapi::mdw::CreateBaselineReply&         aReply ) = 0;

    /// What aRequest and any retry of it act on, captured before the attempt
    virtual kiapi::workspace::WorkspaceError ReleaseToken( const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                                           std::string&                              aToken ) = 0;

    /// A retry after an interrupted attempt, with MDW_BACKEND_SERVER::RegisterRecovery's contract
    virtual MDW_RECOVERY RecoverTransition( const kiapi::mdw::TransitionLifecycleRequest& aRequest,
                                            kiapi::mdw::TransitionLifecycleReply&         aReply,
                                            kiapi::workspace::WorkspaceError&             aError ) = 0;

    virtual MDW_RECOVERY RecoverRelease( const kiapi::mdw::ReleaseRevisionRequest& aRequest, const std::string& aToken,
                                         kiapi::mdw::ReleaseRevisionReply& aReply,
                                         kiapi::workspace::WorkspaceError& aError ) = 0;

    virtual MDW_RECOVERY RecoverBaseline( const kiapi::mdw::CreateBaselineRequest& aRequest,
                                          kiapi::mdw::CreateBaselineReply&         aReply,
                                          kiapi::workspace::WorkspaceError&        aError ) = 0;

    /// True when this backend holds aRef, so its facts come from Releases
    virtual bool Owns( const kiapi::workspace::ItemRef& aRef ) const = 0;

    /**
     * Each named item's approved releases, oldest first. An item with no revisions at all is left out of aOut;
     * one whose revisions include no release maps to an empty list.
     */
    virtual kiapi::workspace::WorkspaceError
    Releases( const std::set<std::string>& aItemIds, std::map<std::string, std::vector<MDW_RELEASE_RECORD>>& aOut ) = 0;

    /**
     * The revision a baseline of aItemId releases, which must be its latest. aCreating adds the checks only a
     * create needs and fills aOut.held. Every call fills aOut.hold.
     */
    virtual kiapi::workspace::WorkspaceError BaselineSource( const std::string& aItemId, const std::string& aRevisionId,
                                                             bool aCreating, MDW_BASELINE_SOURCE& aOut ) = 0;
};


/// How the runtime reaches a backend's store
struct MDW_LIFECYCLE_HOOKS
{
    using CALL = std::function<kiapi::workspace::WorkspaceError( MDW_LIFECYCLE_STORE& )>;

    /**
     * Call aCall with the store serving the request, holding what that needs, and return its result. An error
     * returned without calling aCall refuses the request.
     */
    std::function<kiapi::workspace::WorkspaceError( const CALL& aCall )> use;
};


/// Register the four lifecycle requests, CreateBaseline, CheckPlacedParts, CheckBaseline and their recoveries
void MdwServeLifecycle( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks );

/// Check aRequest as a create would, writing nothing
kiapi::workspace::WorkspaceError MdwPlanBaseline( MDW_LIFECYCLE_STORE&                     aStore,
                                                  const kiapi::mdw::CreateBaselineRequest& aRequest,
                                                  MDW_BASELINE_PLAN&                       aPlan );

/// aRequest's release note followed by one line per exception, for a store that keeps a baseline's text in one field
std::string MdwBaselineDescription( const kiapi::mdw::CreateBaselineRequest& aRequest );

/// Refuse a requested label other than aNext, for a backend that numbers its releases or baselines (aWhat)
std::optional<kiapi::workspace::WorkspaceError> MdwRequireNextLabel( const MDW_LIFECYCLE_STORE& aStore,
                                                                     const std::string&         aWhat,
                                                                     const std::string&         aRequested,
                                                                     const std::string&         aNext );

#endif // MDW_LIFECYCLE_H
