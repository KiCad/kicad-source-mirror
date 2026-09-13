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
 * @file git_backend.h
 * @brief kicad-mdw-git, the git workspace served by libgit2 in its own process.
 *
 * Each session gets its own process, which learns the working tree from Hello.workspace. It shares that repository
 * with the Version Control pane and the git CLI through git's own lock files, as two git commands do, and every
 * write is a short compare-and-swap that retries when another writer got there first.
 */

#ifndef GIT_BACKEND_H
#define GIT_BACKEND_H

#include "git_lifecycle.h"
#include "git_shared_libraries.h"
#include "mdw_git_checkouts.h"
#include "mdw_git_push_outcomes.h"
#include "mdw_git_remote.h"
#include "mdw_git_repo.h"

#include <git/libgit_backend.h>
#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_poller.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

class MDW_EVENT_PUBLISHER;

namespace KIGIT
{
class REPO_ACTION_LOCK;
}

/// Installs the application's git backend, which initializes libgit2 with bounded server waits. It must outlive
/// everything holding libgit2 objects, since freeing a pack takes libgit2's locks
class MDW_GIT_LIBGIT2
{
public:
    MDW_GIT_LIBGIT2();
    ~MDW_GIT_LIBGIT2();

    MDW_GIT_LIBGIT2( const MDW_GIT_LIBGIT2& ) = delete;
    MDW_GIT_LIBGIT2& operator=( const MDW_GIT_LIBGIT2& ) = delete;

private:
    std::unique_ptr<LIBGIT_BACKEND> m_backend;
};


class MDW_GIT_BACKEND
{
public:
    static constexpr const char* BACKEND_ID = "org.kicad.mdw.git";

    /// Teammates' claims and library changes that a poll notices go out through aPublisher, when given. The poll
    /// runs regardless, so claim reads stay current
    explicit MDW_GIT_BACKEND( MDW_BACKEND_SERVER& aServer, MDW_EVENT_PUBLISHER* aPublisher = nullptr );

    /// What the backend advertises, with the journal scoped to the repository a Hello names
    static MDW_BACKEND_DESCRIPTOR Descriptor();

    /// Register every handler and the filter that refuses a Hello naming no working tree. Call before serving.
    void Register();

private:
    friend class MDW_GIT_LIFECYCLE;

    /// Open the repository the accepted Hello names, once it exists; call with m_repoMutex held
    bool ensureOpen();

    /// The address this workspace's transfers reach, empty when it has no remote
    std::string remoteUrl();

    /// Connect to this workspace's remote with the kept credentials, or find it when it is a local path
    kiapi::workspace::WorkspaceError checkConnection();

    /// Who the backend acts as, signed in as aUser
    kiapi::workspace::Account account( const std::string& aUser ) const;

    /// Whether this library source may be written, which is pushed whether as a check-in, a release or a state tag
    kiapi::workspace::Permission writePermission( const std::string& aName );

    /// Open aRepo on the bound tree apart from m_repo, so a connection check never waits for a transfer's lock
    bool openBoundTree( MDW_GIT_REPO& aRepo ) const;

    /// The author of new commits in aRepo, signed in as the remote is now
    GIT_IDENTITY identity( const MDW_GIT_REPO& aRepo ) const;

    /// Who aRepo's claims, locks and breaks are recorded for, the signed-in account with the author beside it
    MDW_GIT_HOLDER holder( const MDW_GIT_REPO& aRepo ) const;

    /// aFn wrapped to run with the repository to itself, or to say why there is none yet
    template <typename FN>
    auto withRepo( FN aFn );

    /// aFn wrapped so a request that changed our claim echoes the claim as it ends, after any teammate's change
    /// its fetches announced, so the host applies them in that order; run it inside withRepo
    template <typename FN>
    auto echoes( FN aFn );

    /// Take the action lock of the repository whose working tree aTarget is written into, or none
    /// when aTarget is in no repository. A copy written there must not land inside a branch switch.
    kiapi::workspace::WorkspaceError lockDestination( const std::filesystem::path&              aTarget,
                                                      std::unique_ptr<KIGIT::REPO_ACTION_LOCK>& aLock );

    void registerItemHandlers();
    void registerWorkingCopyHandlers();
    void registerRevisionHandlers();
    void registerAuthHandlers();
    void registerClaimHandlers();
    void registerLifecycleHandlers();
    void registerPermissionHandlers();

    /// True when this workspace is a library source and aItemId names one of its members
    bool isMember( const std::string& aItemId ) const;

    /// Open library member aRequest.item_id() for edits in the copy's library file, claiming it when asked;
    /// run it inside withRepo
    kiapi::workspace::WorkspaceError checkoutMember( const kiapi::mdw::CheckoutRequest& aRequest,
                                                     kiapi::mdw::CheckoutReply&         aReply );

    /// Keep aResult as the claims' outcome when it says something of the remote; call with m_repoMutex held
    kiapi::workspace::WorkspaceError settleClaims( MDW_GIT_CHECKOUTS::RESULT aResult );

    /// The poller's fetch of the remote's claims and design release mirrors, announcing the claims that changed.
    /// Takes m_repoMutex only around local reads, never across the fetch, which runs on a handle of its own
    kiapi::workspace::WorkspaceError pollClaims();

    /// Announce a library branch tip the bound copy lacks, once per tip. Answers how the remote read went
    kiapi::workspace::WorkspaceError announceLibrary();

    /// Hand the poller what a connection check found. A good one reads claims again and resumes polling, and a
    /// refused one holds it, while one that met no server resumes only aNewCredentials. Answers aChecked
    kiapi::workspace::WorkspaceError answered( const kiapi::workspace::WorkspaceError& aChecked,
                                               bool                                    aNewCredentials = false );

    /// What the last poll left of the claims, polling first when none has since the repository opened, or since a
    /// failed one was followed by a good connection check; call with m_repoMutex held and claims set up
    kiapi::workspace::WorkspaceError readClaims();

    /// Claim aItemId for this repository's git identity; call with m_repoMutex held and claims set up
    MDW_GIT_CHECKOUTS::RESULT takeClaim( const std::string& aItemId, std::string* aCreated = nullptr );

    /// The reply for a claim call that did not succeed. Throws when the remote may have changed, leaving the
    /// request to recovery
    kiapi::workspace::WorkspaceError claimFailure( MDW_GIT_CHECKOUTS::RESULT aResult );

    /// Announce claims a fetch found changed, which nobody in this session asked for
    void publishClaimChanges( const std::vector<MDW_GIT_CHECKOUTS::CHANGE>& aChanges );

    /// Announce the claim on aItemId as it now stands, as the echo of request aOperationId
    void echoClaim( const std::string& aItemId, const std::string& aOperationId );

    MDW_BACKEND_SERVER&  m_server;
    MDW_EVENT_PUBLISHER* m_publisher;

    /// One libgit2 handle, so the server's workers take turns with it
    std::mutex   m_repoMutex;
    MDW_GIT_REPO m_repo;
    bool         m_open = false;
    std::string  m_prefix; ///< The bound root relative to the repository root, empty at the top

    /// Credentials from Authenticate, kept in memory only and never written anywhere
    MDW_GIT_REMOTE                     m_remote;
    std::unique_ptr<MDW_GIT_CHECKOUTS> m_claims; ///< Null when the repository has no remote to share claims

    std::optional<kiapi::workspace::WorkspaceError> m_polled;           ///< The last poll's outcome
    std::atomic<bool>                               m_answered = false; ///< A connection check passed since it
    std::string                                     m_announcedTip;     ///< The library branch tip last announced

    MDW_GIT_PUSH_OUTCOMES    m_pushOutcomes;
    MDW_GIT_SHARED_LIBRARIES m_libraries;
    MDW_GIT_LIFECYCLE        m_lifecycle;

    std::unique_ptr<MDW_POLLER> m_poller; ///< Last, so it stops before anything a pass reads goes
};

#endif // GIT_BACKEND_H
