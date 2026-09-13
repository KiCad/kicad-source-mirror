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
 * @file git_lifecycle.h
 * @brief Git's releases and baselines, kept as annotated tags.
 *
 * A library source releases its members as kicad/rel/<member>/<n>, tagging the commit that holds the released
 * text, and moves release n between Released, Not for New Designs and Obsolete with a state tag
 * kicad/state/<member>/<n>/<seq>, the newest of which holds. A project workspace baselines its design as
 * kicad/design-rel/<n> on the newest commit that changed the project, with the placed parts' checks in the
 * manifest, and a baseline never changes state.
 */

#ifndef GIT_LIFECYCLE_H
#define GIT_LIFECYCLE_H

#include <mdw_runtime/mdw_lifecycle.h>

#include <map>
#include <string>
#include <vector>

class MDW_GIT_BACKEND;


class MDW_GIT_LIFECYCLE : public MDW_LIFECYCLE_STORE
{
public:
    explicit MDW_GIT_LIFECYCLE( MDW_GIT_BACKEND& aBackend );

    /// The states Definition names, which a library source advertises as well
    static std::vector<kiapi::workspace::CustomState> States();

    std::string Name() const override { return "Git"; }

    kiapi::workspace::WorkspaceError Definition( const kiapi::mdw::GetLifecycleDefinitionRequest& aRequest,
                                                 kiapi::mdw::GetLifecycleDefinitionReply&         aReply ) override;

    kiapi::workspace::WorkspaceError Transitions( const kiapi::mdw::GetAvailableTransitionsRequest& aRequest,
                                                  kiapi::mdw::GetAvailableTransitionsReply&         aReply ) override;

    kiapi::workspace::WorkspaceError Transition( const kiapi::mdw::TransitionLifecycleRequest& aRequest,
                                                 kiapi::mdw::TransitionLifecycleReply&         aReply ) override;

    kiapi::workspace::WorkspaceError Release( const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                              const std::string&                        aToken,
                                              kiapi::mdw::ReleaseRevisionReply&         aReply ) override;

    kiapi::workspace::WorkspaceError Baseline( const kiapi::mdw::CreateBaselineRequest& aRequest,
                                               kiapi::mdw::CreateBaselineReply&         aReply ) override;

    /// Transition finds the state tag an interrupted attempt left, so recovery replays its answer
    MDW_RECOVERY RecoverTransition( const kiapi::mdw::TransitionLifecycleRequest& aRequest,
                                    kiapi::mdw::TransitionLifecycleReply&         aReply,
                                    kiapi::workspace::WorkspaceError&             aError ) override;

    /// The claim the release will give up, so neither the attempt nor a retry gives up one recorded after it began
    kiapi::workspace::WorkspaceError ReleaseToken( const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                                   std::string&                              aToken ) override;

    /// Release and Baseline find and finish what an interrupted attempt left, so recovery replays their answer
    MDW_RECOVERY RecoverRelease( const kiapi::mdw::ReleaseRevisionRequest& aRequest, const std::string& aToken,
                                 kiapi::mdw::ReleaseRevisionReply& aReply,
                                 kiapi::workspace::WorkspaceError& aError ) override;

    MDW_RECOVERY RecoverBaseline( const kiapi::mdw::CreateBaselineRequest& aRequest,
                                  kiapi::mdw::CreateBaselineReply&         aReply,
                                  kiapi::workspace::WorkspaceError&        aError ) override;

    bool Owns( const kiapi::workspace::ItemRef& aRef ) const override;

    kiapi::workspace::WorkspaceError Releases( const std::set<std::string>&                            aItemIds,
                                               std::map<std::string, std::vector<MDW_RELEASE_RECORD>>& aOut ) override;

    kiapi::workspace::WorkspaceError BaselineSource( const std::string& aItemId, const std::string& aRevisionId,
                                                     bool aCreating, MDW_BASELINE_SOURCE& aOut ) override;

    /// The number of the newest published design release of project item aItemId tagging each commit, read from
    /// the local tags and their mirrors; call with the repository open
    std::map<std::string, int> Baselines( const std::string& aItemId ) const;

    /// The fetch refspec that mirrors the remote's design releases, which tells a published one from a local one
    std::string MirrorRefspec() const;

private:
    /// True for a session whose locator names a library repository, which releases parts but holds no design
    bool librarySource() const;

    /// Release a part, then give up aClaim, the claim it was revised under, when there is one
    kiapi::workspace::WorkspaceError releasePart( const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                                  const std::string& aClaim, kiapi::mdw::ReleaseRevisionReply& aReply );

    MDW_GIT_BACKEND& m_backend;
};

#endif // GIT_LIFECYCLE_H
