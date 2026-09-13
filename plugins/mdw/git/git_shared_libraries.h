/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GIT_SHARED_LIBRARIES_H
#define GIT_SHARED_LIBRARIES_H

#include "mdw_git_releases.h"

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_lifecycle.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

class MDW_BACKEND_SERVER;
class MDW_LIFECYCLE_STORE;
class MDW_GIT_REMOTE;
struct GIT_IDENTITY;
struct git_repository;

/**
 * CG_SHARED_LIBRARIES for a library source, whose Hello names the library repository's URL as its locator.
 * A copy is a clone of that repository at local_root, marked as this source's in its own configuration. Reads
 * before there is a copy answer from a bare cache under aCacheRoot, so they never write local_root.
 *
 * Every .kicad_sym file outside a .kicad_symdir and every .pretty or .kicad_symdir folder is a library. A member
 * is a symbol or a footprint file, with the id "<library path>#<name>" and the last commit that changed its text as
 * its revision. The snapshot token is a commit id. Update only moves the copy's branch forward, carrying local work
 * across as member edits, and a member both sides changed is a conflict for the user to resolve. Releases and their
 * states are tags, as git_lifecycle.h describes.
 */
class MDW_GIT_SHARED_LIBRARIES
{
public:
    MDW_GIT_SHARED_LIBRARIES( MDW_BACKEND_SERVER& aServer, MDW_GIT_REMOTE& aRemote, std::mutex& aRepoMutex,
                              const std::filesystem::path& aCacheRoot );

    /// Register the shared library handlers. Call before serving.
    void Register();

    /// True when aRoot is a copy of the bound locator's repository. Reads through a handle of its own, so it needs
    /// no lock.
    bool IsCopy( const std::string& aRoot ) const;

    /// Put in aTip the tip of the bound copy's branch on the remote when the copy does not hold it, read without
    /// fetching, else empty. A pinned copy takes no updates. Takes the repository mutex only after the remote read,
    /// so a slow remote holds up no request. Answers how the remote read went.
    kiapi::workspace::WorkspaceError UnheldTip( std::string& aTip );

    /// Whether the user holds the claim on a member
    using CLAIM_TEST = std::function<bool( const std::string& aItemId )>;

    /// The test for a claim on a member, which lifts the denial of a released member's check-in
    void SetClaimTest( CLAIM_TEST aHoldsClaim ) { m_holdsClaim = std::move( aHoldsClaim ); }

    /**
     * ListLocalChanges for a copy, a row per member that differs from HEAD. A member with a published release is
     * checked in only by whoever revises it, so its row is denied unless the user holds its claim, and
     * CheckInSharedLibrary refuses it the same way. Call with the repository mutex held.
     */
    kiapi::workspace::WorkspaceError LocalChanges( const kiapi::mdw::ListLocalChangesRequest& aRequest,
                                                   kiapi::mdw::ListLocalChangesReply&         aReply ) const;

    /**
     * The library file holding only member aItemId as it was at commit aRevision of aRepo, or nullopt with
     * aError set. An empty aRevision asks for the latest snapshot and is set to the commit read. A derived symbol
     * answers WE_NOT_SUPPORTED, since it cannot be flattened here.
     */
    static std::optional<std::string> MemberFile( git_repository* aRepo, const std::string& aItemId,
                                                  std::string& aRevision, kiapi::workspace::WorkspaceError& aError );

    /// True when aItemId names a library member rather than a file
    static bool IsMemberId( const std::string& aItemId );

    /// MemberFile from the source's copy, or its cache before there is one. Call with the repository mutex held.
    std::optional<std::string> SourceMemberFile( const std::string& aItemId, std::string& aRevision,
                                                 kiapi::workspace::WorkspaceError& aError ) const;

    /**
     * Release member aRequest.item_id() from the copy at the bound root. Its text from the snapshot is committed
     * onto the branch when it differs and the branch is pushed, then the next release, numbered as aStore names
     * labels, is tagged and pushed. Content matching a release the team already holds answers that release. Call
     * with the repository mutex held.
     */
    kiapi::workspace::WorkspaceError ReleaseMember( const MDW_LIFECYCLE_STORE&                aStore,
                                                    const kiapi::mdw::ReleaseRevisionRequest& aRequest,
                                                    kiapi::mdw::ReleaseRevisionReply&         aReply );

    /**
     * Each of aItemIds' releases, oldest first, after mirroring the team's. A member with no release that the
     * library does not hold is left out. Call with the repository mutex held.
     */
    kiapi::workspace::WorkspaceError MemberReleases( const std::set<std::string>&                         aItemIds,
                                                     std::map<std::string, std::vector<MDW_GIT_RELEASE>>& aOut );

    /**
     * Describe member aItemId as the source's copy holds it at HEAD, or its cache before there is one. It reads
     * released while it is what its newest published release holds. aPath is set to the library file in the copy
     * that holds it, empty without a copy or when that is no regular file reached without links. Call with the
     * repository mutex held.
     */
    kiapi::workspace::WorkspaceError MemberInfo( const std::string& aItemId, kiapi::workspace::ItemInfo& aOut,
                                                 std::string& aPath );

    /// The states the newest release of member aRequest.item_id() can move to. Call with the repository mutex held.
    kiapi::workspace::WorkspaceError MemberTransitions( const kiapi::mdw::GetAvailableTransitionsRequest& aRequest,
                                                        kiapi::mdw::GetAvailableTransitionsReply&         aReply );

    /**
     * Move the newest release of member aRequest.item_id() to the state aRequest names, by tagging its commit with the
     * next state tag and pushing that. A tag an earlier attempt with the same idempotency id left is finished
     * instead. Call with the repository mutex held.
     */
    kiapi::workspace::WorkspaceError TransitionMember( const kiapi::mdw::TransitionLifecycleRequest& aRequest,
                                                       kiapi::mdw::TransitionLifecycleReply&         aReply );

    /// Member aItemId's published releases, newest first. Call with the repository mutex held.
    kiapi::workspace::WorkspaceError MemberHistory( const std::string&                       aItemId,
                                                    std::vector<kiapi::workspace::Revision>& aOut );

    /// aLocator without credentials, a trailing slash or a remote's ".git", safe to show and stable as a key
    static std::string NormalisedLocator( const std::string& aLocator );

    /// Member revisions by member file, or by .kicad_symdir library, and the commit its history starts from
    using FILE_REVISIONS = std::map<std::pair<std::string, std::string>, std::map<std::string, std::string>>;

private:
    std::string locator() const;
    std::string boundRoot() const;

    /// The author of new commits and tags in the copy aRepo, signed in as the remote is now
    GIT_IDENTITY identity( git_repository* aRepo ) const;

    MDW_BACKEND_SERVER&    m_server;
    MDW_GIT_REMOTE&        m_remote;
    std::mutex&            m_repoMutex;
    std::filesystem::path  m_cacheRoot;
    mutable FILE_REVISIONS m_revisions; ///< Guarded by m_repoMutex
    CLAIM_TEST             m_holdsClaim;

    std::function<std::optional<std::string>( const std::string&, std::string&, kiapi::workspace::WorkspaceError& )>
            m_sourceMember;

    std::function<kiapi::workspace::WorkspaceError(
            const MDW_LIFECYCLE_STORE&, const kiapi::mdw::ReleaseRevisionRequest&, kiapi::mdw::ReleaseRevisionReply& )>
            m_releaseMember;

    std::function<kiapi::workspace::WorkspaceError( const std::set<std::string>&,
                                                    std::map<std::string, std::vector<MDW_GIT_RELEASE>>& )>
            m_memberReleases;

    std::function<kiapi::workspace::WorkspaceError( const std::string&, kiapi::workspace::ItemInfo&, std::string& )>
            m_memberInfo;

    std::function<kiapi::workspace::WorkspaceError( const std::string&, std::vector<kiapi::workspace::Revision>& )>
            m_memberHistory;

    std::function<kiapi::workspace::WorkspaceError( const kiapi::mdw::GetAvailableTransitionsRequest&,
                                                    kiapi::mdw::GetAvailableTransitionsReply& )>
            m_memberTransitions;

    std::function<kiapi::workspace::WorkspaceError( const kiapi::mdw::TransitionLifecycleRequest&,
                                                    kiapi::mdw::TransitionLifecycleReply& )>
            m_transitionMember;
};

#endif // GIT_SHARED_LIBRARIES_H
