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
 * @file mdw_git_releases.h
 * @brief Numbered releases recorded as annotated git tags.
 *
 * Release n of a scope is the tag kicad/<kind>/<scope><n>, whose message is a JSON manifest after a readable summary
 * (MdwGitRecordMessage), so what a release holds can be answered from the repository alone. Part releases are kind
 * "rel" scoped by the member, and design releases are kind "design-rel" with no scope. Creating a tag never moves an
 * existing one.
 *
 * Local tags are the pins. Fetch() mirrors the remote's tags of the kind below
 * refs/kicad-remote/<remote>/<kind>/, adopts releases only the remote has, and leaves every local tag alone, so a
 * remote tag that was moved reads as tampered rather than being taken in.
 */

#ifndef MDW_GIT_RELEASES_H
#define MDW_GIT_RELEASES_H

#include <api/workspace/workspace_types.pb.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class MDW_GIT_REMOTE;
struct git_repository;


struct MDW_GIT_RELEASE
{
    std::string    ref; ///< The full name of the local tag
    int            number = 0;
    std::string    tagOid; ///< The tag object, which pins this release
    std::string    commit;
    nlohmann::json manifest;
    std::string    releasedBy;
    std::int64_t   date = 0;
    bool           published = false; ///< The remote held this tag object when last seen
    bool           tampered = false;  ///< The remote holds another object under this tag's name
    std::string    stateId; ///< A part release's state as its state tags moved it; empty until they are read
};


/// The state a release is in until a state tag moves it, as the git lifecycle definition names it
kiapi::workspace::LifecycleState MdwGitReleasedState();

/// Every state a release can be in, released first
std::vector<kiapi::workspace::CustomState> MdwGitStates();

/// The state git names aStateId, or nothing for one it does not know
std::optional<kiapi::workspace::LifecycleState> MdwGitState( const std::string& aStateId );

/// How every reply names design release aNumber
std::string MdwGitBaselineLabel( int aNumber );

/// Part release aRelease as a revision, which is the commit holding the released member
kiapi::workspace::Revision MdwGitPartRevision( const MDW_GIT_RELEASE& aRelease );


class MDW_GIT_RELEASES
{
public:
    /// Whether aManifest describes release aNumber, beyond naming the tagged commit
    using VALID = std::function<bool( const nlohmann::json& aManifest, int aNumber )>;

    MDW_GIT_RELEASES( git_repository* aRepo, MDW_GIT_REMOTE& aRemote, const std::string& aRemoteName,
                      const std::string& aKind );

    /// The releases under aScope whose manifest aValid accepts, oldest first
    std::vector<MDW_GIT_RELEASE> List( const std::string& aScope, const VALID& aValid ) const;

    /// Mirror the remote's releases of this kind, then Adopt
    kiapi::workspace::WorkspaceError Fetch();

    /// Fetch every kind of aKinds in one transfer, so they are read as the remote held them at one moment
    static kiapi::workspace::WorkspaceError Fetch( const std::vector<MDW_GIT_RELEASES*>& aKinds );

    /// The fetch refspec that mirrors this kind's tags, for a caller fetching other refs in the same transfer
    std::string MirrorRefspec() const;

    /// Create a local tag for each mirrored release that has none
    void Adopt();

    /// Tag aCommit as release aNumber under aScope, local until Publish. aManifest names aCommit as its revision_id,
    /// and aSubject and aBody are what a person reads of the tag
    kiapi::workspace::WorkspaceError Create( const std::string& aScope, int aNumber, const std::string& aCommit,
                                             const std::string& aSubject, const std::string& aBody,
                                             const nlohmann::json& aManifest, const std::string& aTaggerName,
                                             const std::string& aTaggerEmail, MDW_GIT_RELEASE& aOut );

    /// Push aRelease only where the remote has no tag of its name, then mirror it. aTaken says another got there first
    kiapi::workspace::WorkspaceError Publish( MDW_GIT_RELEASE& aRelease, bool& aTaken );

    using CREATE = std::function<kiapi::workspace::WorkspaceError( MDW_GIT_RELEASE& aOut )>;

    /// Publish aRelease, calling aCreate for it first when it is empty. A teammate taking the same number first
    /// costs one fetch and one more aCreate, and a tag the remote never answered for stays for a retry to finish
    kiapi::workspace::WorkspaceError PublishNext( std::optional<MDW_GIT_RELEASE>& aRelease, const CREATE& aCreate );

    /// Remove aRelease's local tag, which the remote never took
    void Drop( const MDW_GIT_RELEASE& aRelease );

    /// One past the highest number any local tag under aScope holds, a manifest's validity aside
    int Next( const std::string& aScope ) const;

private:
    std::string tagRoot() const { return "refs/tags/kicad/" + m_kind + "/"; }

    std::string trackingRoot() const { return "refs/kicad-remote/" + m_remoteName + "/" + m_kind + "/"; }

    git_repository* m_repo;
    MDW_GIT_REMOTE& m_remote;
    std::string     m_remoteName;
    std::string     m_kind;
};

#endif // MDW_GIT_RELEASES_H
