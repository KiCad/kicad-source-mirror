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
 * @file mdw_git_checkouts.h
 * @brief Advisory editing claims shared through a git remote.
 *
 * A claim is a ref, refs/kicad/checkouts/<item>, naming a parentless commit whose message records
 * who holds it. Taking a claim creates the remote ref only if it does not exist, and
 * giving one up deletes it only while it still names our commit, so a claim that someone else
 * took over is never removed by its former holder. Nothing stops a plain git push, so the
 * claims coordinate people rather than enforce access.
 *
 * Removing someone else's claim also pushes refs/kicad/released/<item>, a record of who removed which claim and
 * why, so the holder can be told.
 */

#ifndef MDW_GIT_CHECKOUTS_H
#define MDW_GIT_CHECKOUTS_H

#include "mdw_git_remote.h"

#include <git/kigit_lock_retry.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct git_repository;
struct GIT_IDENTITY;


/// Who a claim or its release is recorded for
struct MDW_GIT_HOLDER
{
    std::string owner; ///< The signed-in account as MdwGitOwnerId names it, empty in a record older than owners
    std::string user;  ///< The author's display name
    std::string email;

    /**
     * Whether a record naming aOwner and aEmail is this holder's. A record older than owners names only an email,
     * and so does one written, or read, where no login was known, since the owner then falls back to the email.
     * An email is accepted for one release wherever either side names no account.
     */
    bool Is( const std::string& aOwner, const std::string& aEmail ) const
    {
        if( !aOwner.empty() && aOwner == owner )
            return true;

        const bool recordNamesEmail = aOwner.empty() || aOwner == aEmail;
        const bool weNameEmail = owner.empty() || owner == email;

        return ( recordNamesEmail || weNameEmail ) && !email.empty() && aEmail == email;
    }
};


/// The holder of the account signed in as aUsername to aRemoteUrl, named as aAuthor is
MDW_GIT_HOLDER MdwGitHolder( const GIT_IDENTITY& aAuthor, const std::string& aUsername, const std::string& aRemoteUrl );


struct MDW_GIT_CLAIM
{
    std::string  oid;
    std::string  item;          ///< Empty for a claim pushed by hand under a hashed name
    std::string  owner;
    std::string  user;
    std::string  email;
    std::int64_t since = 0;
    bool         mine = false;

    /// Who holds the claim, as a lock read names them
    std::string OwnerId() const { return owner.empty() ? email : owner; }
};


class MDW_GIT_CHECKOUTS
{
public:
    enum class RESULT
    {
        OK,
        HELD_BY_OTHER,  ///< Someone else holds the claim; Holder() says who
        NOT_HOLDER,     ///< We hold no claim, or ours was taken over
        REPLACED,       ///< A forced release found another claim than the one confirmed; Holder() says whose
        BUSY,           ///< Another git operation holds the repository
        NETWORK,        ///< The remote could not be reached or refused the refs, and nothing changed
        UNKNOWN,        ///< A push failed after it may have reached the remote; our record is kept
        CANCELLED       ///< The caller gave up before anything was pushed
    };

    /// aRepo is the caller's handle. Claims always travel to aRemoteName, so a branch switch between a claim's
    /// fetch and its push cannot split them across two remotes.
    MDW_GIT_CHECKOUTS( git_repository* aRepo, MDW_GIT_REMOTE& aRemote, const std::string& aRemoteName );

    /// Mirror the remote's claims into the tracking namespace, and in the same transfer each of aMirrors' refspecs.
    RESULT Refresh( const std::vector<std::string>& aMirrors = {} );

    /// A poll's Refresh() in three steps, so that its transfer runs beside other calls. The transfer writes no
    /// ref, so a poll shares nothing with another process polling the same repository.
    struct STAGE
    {
        /// Each remote ref prefix, and the tracking prefix it feeds
        std::vector<std::pair<std::string, std::string>> prefixes;

        std::map<std::string, std::string> tracked;   ///< The tracking refs, by name, as Stage() read them
        std::map<std::string, std::string> remote;    ///< What Fetch() found they should name
        std::uint64_t                      moves = 0; ///< The claims' ref writes when Stage() read them
    };

    /// Begin a poll of the claims and aMirrors' refspecs. Call where Refresh() would be called.
    STAGE Stage( const std::vector<std::string>& aMirrors );

    /// Download what aStage needs through aRepo, a handle of the caller's own, and read the remote refs in the same
    /// connection. Writes no ref, so it needs none of the locks the other calls share.
    kiapi::workspace::WorkspaceError Fetch( git_repository* aRepo, STAGE& aStage );

    /**
     * Set the tracking refs to what aStage found and tell the observer what changed. A stage that another call's
     * ref writes overtook is dropped, as that call read the remote later. Call where Refresh() would be called.
     */
    RESULT Apply( const kiapi::workspace::WorkspaceError& aFetched, const STAGE& aStage );

    /// The claim on aItemId as of the last Refresh().
    std::optional<MDW_GIT_CLAIM> Find( const std::string& aItemId );

    /// Every claim as of the last Refresh().
    std::vector<MDW_GIT_CLAIM> All();

    /// Claim aItemId. aCreated gets the claim when this call took it fresh, and stays empty when it was already
    /// ours.
    RESULT Take( const std::string& aItemId, const MDW_GIT_HOLDER& aHolder, std::string* aCreated = nullptr );

    /// Who removed someone else's claim, and why
    struct RELEASE
    {
        std::string user;
        std::string email;
        std::string reason;
        std::string owner;

        /// The claim the user confirmed, by MDW_GIT_CLAIM::OwnerId() and since. Unset releases whoever holds it
        std::optional<std::pair<std::string, std::int64_t>> confirmed;
    };

    /// Give up our claim, or with aForced remove whoever holds it and record aForced beside the removal.
    RESULT Give( const std::string& aItemId, const std::optional<RELEASE>& aForced = std::nullopt );

    /// Give up only the claim aClaimId from LocalClaimId(), leaving any claim taken after it, which another
    /// process sharing this repository may have recorded since.
    RESULT GiveClaim( const std::string& aItemId, const std::string& aClaimId );

    /// A claim a fetch found taken, passed to someone else, or gone
    struct CHANGE
    {
        MDW_GIT_CLAIM          claim;        ///< As it now stands, or as it last stood when it went
        bool                   held = false; ///< False when the claim went
        std::optional<RELEASE> release;      ///< Who removed the claim that went, when they left a record
    };

    /// Told after each Refresh() that changed someone else's claims, so a backend can announce them. A claim
    /// this repository took is left out while it stands, since whoever took it already knows, but not once it
    /// went, since only a release from here would have told its holder.
    void SetChangeObserver( std::function<void( const std::vector<CHANGE>& )> aObserver )
    {
        m_changeObserver = std::move( aObserver );
    }

    /// True when this repository recorded a claim of ours on aItemId, without asking the remote.
    bool Holds( const std::string& aItemId ) { return read( localRef( aItemId ) ).has_value(); }

    /// The commit of the claim this repository recorded on aItemId, or empty. Each claim taken gets a new one.
    std::string LocalClaimId( const std::string& aItemId );

    /// The holder that made the last call fail with HELD_BY_OTHER or NOT_HOLDER, if known.
    const std::optional<MDW_GIT_CLAIM>& Holder() const { return m_holder; }

    const std::string& Error() const { return m_error; }

    /// The error the client acts on when the last transfer needed credentials or a trusted host key, else none
    std::optional<kiapi::workspace::WorkspaceError> CredentialFailure() const;

    /// The ref name for aItemId below refs/kicad/checkouts/, readable and valid for any path.
    static std::string RefComponent( const std::string& aItemId );

private:
    /// The refspecs that mirror the claims and aMirrors into the tracking namespace
    std::vector<std::string> refspecs( const std::vector<std::string>& aMirrors ) const;

    kiapi::workspace::WorkspaceError transfer( git_repository* aRepo, const std::vector<std::string>& aRefspecs );

    /// Every claim the tracking refs hold, by item. Empty without an observer, which nothing would tell
    std::map<std::string, MDW_GIT_CLAIM> snapshot();

    /// Take aFetched as the last transfer and tell the observer how the claims moved since aBefore
    RESULT adopt( const kiapi::workspace::WorkspaceError&     aFetched,
                  const std::map<std::string, MDW_GIT_CLAIM>& aBefore );

    std::string localRef( const std::string& aItemId ) const;
    std::string trackingRef( const std::string& aItemId ) const;

    RESULT take( const std::string& aItemId, const MDW_GIT_HOLDER& aHolder, std::string* aCreated );
    std::string releasedRef( const std::string& aItemId ) const;

    /// Release from aClaimId, or from our recorded claim when null
    RESULT give( const std::string& aItemId, const std::optional<RELEASE>& aForced, const std::string* aClaimId );

    /// The record of who removed aClaim, if the last fetch brought one
    std::optional<RELEASE> releaseOf( const MDW_GIT_CLAIM& aClaim );

    /// Every process sharing this repository changes claims one at a time, since a change rewrites the record
    /// they share. Null, with Error() set, when another process kept it past the budget.
    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> lockClaims();

    RefPushResult push( const std::vector<GIT_REF_UPDATE>& aUpdates );

    std::optional<MDW_GIT_CLAIM> read( const std::string& aRef );
    bool                         setRef( const std::string& aRef, const std::string& aOid );
    void                         dropRef( const std::string& aRef );

    /// Remove aRef only while it names aOid. False when it names something else.
    bool                         dropRefIf( const std::string& aRef, const std::string& aOid );

    git_repository*                  m_repo;
    MDW_GIT_REMOTE&                  m_remote;
    std::string                      m_remoteName;
    std::uint64_t                    m_moves = 0; ///< Ref writes by every call but Apply()
    kiapi::workspace::WorkspaceError m_last; ///< The last transfer's outcome
    std::optional<MDW_GIT_CLAIM>     m_holder;
    std::string                      m_error;

    std::function<void( const std::vector<CHANGE>& )> m_changeObserver;
};

#endif // MDW_GIT_CHECKOUTS_H
