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
 * @file mdw_part_controller.h
 * @brief The lifecycle line and the Revise and Release actions a library editor shows for the part it has open.
 *
 * Shared by the symbol and footprint editors, which say which part they show and supply the digest of its
 * content as edited now. Every read and action goes through the part's source; completions run on the UI
 * thread while the controller lives, and a newer part cancels what the older one still had in flight.
 */

#ifndef MDW_PART_CONTROLLER_H
#define MDW_PART_CONTROLLER_H

#include <kicommon.h>
#include <lib_id.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_subscription.h>
#include <widgets/mdw_member_index.h>
#include <widgets/mdw_part_state.h>

#include <wx/arrstr.h>
#include <wx/string.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class EDA_BASE_FRAME;
enum class LIBRARY_TABLE_TYPE;
class MDW_PART_CONTROLLER;
class MDW_SOURCE;
class wxEvtHandler;
class wxWindow;


/// A part an editor has open, as its source knows it
struct KICOMMON_API MDW_PART_TARGET
{
    std::shared_ptr<MDW_SOURCE>     source;
    MDW_ITEM_REF                    item;
    wxString                        localPath; ///< The file a release uploads
    wxString                        marker;    ///< The library copy holding localPath
    std::optional<MDW_MEMBER_ENTRY> listed;    ///< The library listing's row; none for a part not checked in yet

    /// What the listing alone says, for a part no editor has open
    MDW_PART_STATE ListedState() const;

    /// What may be done to a part no editor has open, as far as its listing tells
    std::vector<MDW_PART_ACTION> ListedActions() const;
};


/**
 * The part aName of the aType library table row aRow, whose release uploads aLocalPath. Nothing when no source
 * serves the row, or while its library is still being listed. A listing without aName gives a target that is not
 * listed, so the part can be checked in.
 */
KICOMMON_API std::optional<MDW_PART_TARGET> MdwPartTarget( MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow,
                                                           const wxString& aName, const wxString& aLocalPath );

/**
 * The file the part aName of the aType library at aLibraryPath is saved in, as the library editors resolve it.
 * aListed, the listing's row for the part or null, may name a file a folder symbol shares with others.
 */
KICOMMON_API wxString MdwTeamPartFile( MDW_LIBRARY_TYPE aType, const wxString& aLibraryPath, const wxString& aName,
                                       const MDW_LIBRARY_MEMBER* aListed );

/// The part aName of aBinding's library as aMembers lists it, unlisted when aMembers lacks it
KICOMMON_API MDW_PART_TARGET MdwPartTarget( const MDW_TEAM_LIBRARY&                     aBinding,
                                            const std::map<wxString, MDW_MEMBER_ENTRY>& aMembers,
                                            const wxString& aName, const wxString& aLocalPath,
                                            const wxString& aMarker );


/// A library file text with a part placed in it as MDW_PART_EDITOR::CurrentFormat() places it
using MDW_PART_UPGRADE = std::function<std::optional<std::string>(
        const std::string&, const std::vector<std::string>&, const std::string&, const wxString& )>;

/**
 * Put revision aRevisionId of aTarget's listed part back into aTarget.localPath, as its source stores it, leaving the
 * other parts of that file as they are. aMayWrite is asked just before the write, which is CANCELLED when it says no.
 * A part saved again since the call is left as saved, with VERSION_CONFLICT. aUpgrade saves the file and the part
 * whole when their formats differ, as MDW_PART_EDITOR::CurrentFormat() does.
 */
KICOMMON_API MDW_OP MdwRestorePart( const MDW_PART_TARGET& aTarget, const wxString& aRevisionId,
                                    std::function<bool()> aMayWrite, MDW_PART_UPGRADE aUpgrade,
                                    MDW_DONE<MDW_NONE> aDone );


/// The state aName moves to, chosen over aParent among the allowed TRANSITION entries of aActions. Nothing on cancel
KICOMMON_API std::optional<wxString> MdwChooseTransition( wxWindow* aParent, const wxString& aName,
                                                          const std::vector<MDW_PART_ACTION>& aActions );

/**
 * The comment moving aName to aTargetStateId needs, asked over aParent when that state's entry in aTransitions requires
 * one. Empty when none is required, nothing on cancel.
 */
KICOMMON_API std::optional<wxString>
MdwTransitionComment( wxWindow* aParent, const wxString& aName,
                      const std::vector<kiapi::workspace::LifecycleTransition>& aTransitions,
                      const wxString& aTargetStateId );


/// One part a release lists, as the editor holding it describes it
struct KICOMMON_API MDW_RELEASE_ITEM
{
    LIB_ID          id;   ///< The part as its editor's library table names it
    MDW_PART_TARGET target;
    wxString        name;
    wxString        digest;       ///< Of the part as saved, which MDW_PART_EDITOR::SaveForRelease fills
    wxString        memberDigest; ///< Of all of it, when the editor tells the two apart
    wxArrayString   unsaved;      ///< Other parts of its file whose unsaved edits the save takes along unreleased
    bool            modified = false; ///< The editor holds unsaved edits to it, which SaveForRelease saves first
    wxString        footprint;    ///< The footprint a symbol names, as LIB_ID text
    wxString        held;         ///< Why SaveForRelease kept it out of this release
};


/// What the release dialog shows before any part is saved or released
struct KICOMMON_API MDW_RELEASE_PROMPT
{
    std::vector<std::pair<wxString, wxString>> rows;     ///< Each part's name and the revision it becomes
    std::vector<wxString>                      concerns; ///< Per row, why it needs the user's say-so or nothing
    wxArrayString                              notes;   ///< What to know before releasing, saves included
    wxString                                   okLabel; ///< Says Save when the release saves anything first
};

/**
 * What aFacts say of a footprint a symbol names, as the tail of "<symbol> uses the footprint <id>". A footprint
 * absent from aFacts' listing is missing when its file is not there either, and only not checked in when it is.
 */
KICOMMON_API wxString MdwFootprintFactText( const MDW_PART_FACTS& aFacts, bool aExists );

/// The prompt for releasing aItems, which says so when the release saves unsaved edits first
KICOMMON_API MDW_RELEASE_PROMPT MdwReleasePrompt( const std::vector<MDW_RELEASE_ITEM>& aItems );


/// How one part of a release stands. A part SaveForRelease held back never starts, so it is neither stopped nor failed.
struct KICOMMON_API MDW_RELEASE_ROW
{
    enum class OUTCOME
    {
        PENDING,
        RELEASED,
        FAILED
    };

    bool    held = false;
    OUTCOME outcome = OUTCOME::PENDING;
};


struct KICOMMON_API MDW_RELEASE_TALLY
{
    size_t released = 0;
    size_t failed = 0;
    size_t stopped = 0; ///< Parts that never started because the user stopped the release
};

/// Counts the rows. Rows from aStopAt on were not started, so those not held count as stopped.
KICOMMON_API MDW_RELEASE_TALLY MdwReleaseTally( const std::vector<MDW_RELEASE_ROW>& aRows, size_t aStopAt );

/// True when a part that is not held waits behind aRow, so a stop would spare something
KICOMMON_API bool MdwReleaseCanStop( const std::vector<MDW_RELEASE_ROW>& aRows, size_t aRow );

/// How a release that was stopped ended, or nothing when no part was stopped
KICOMMON_API wxString MdwReleaseSummary( const MDW_RELEASE_TALLY& aTally );


/// What a part controller needs from the library editor it serves
class KICOMMON_API MDW_PART_EDITOR
{
public:
    virtual ~MDW_PART_EDITOR() = default;

    /**
     * Save the files of aItems so they hold what the editor shows, settling each against a version its save went
     * over unread. Each item's target and digests are then refilled as saved, or its held reason set. False when
     * nothing may be released, as after a failed or cancelled save, which the editor has reported.
     */
    virtual bool SaveForRelease( std::vector<MDW_RELEASE_ITEM>& aItems ) = 0;

    /**
     * True when ReloadPart( aDiscardEdits ) may load the open part again. Unsaved edits of the part itself only stop
     * it without aDiscardEdits; other unsaved work a reload would drop, an open tool or a dialog always do.
     */
    virtual bool CanReloadPart( bool aDiscardEdits ) = 0;

    /// Load the open part again from its file, dropping its own unsaved edits with aDiscardEdits. False when
    /// CanReloadPart() is
    virtual bool ReloadPart( bool aDiscardEdits ) = 0;

    /**
     * Library file text aDestination with its part aName as aPartFile, a file holding only that part, has it, both
     * saved whole as this editor saves them now. aFolder holds the other files of aDestination's folder, which may
     * hold parents. Asked only when the two are in different formats, since a part reads by its file's header.
     * Nothing when either does not read.
     */
    virtual std::optional<std::string> CurrentFormat( const std::string&              aDestination,
                                                      const std::vector<std::string>& aFolder,
                                                      const std::string& aPartFile, const wxString& aName ) = 0;

    /// Bring the open part's library copy up to date, as the library's Get Latest does
    virtual void GetLatest() = 0;

    /// The controller of the part this editor has open
    virtual MDW_PART_CONTROLLER& TeamPart() = 0;

    /// The frame that reports and parents what the part actions show
    virtual EDA_BASE_FRAME* PartFrame() = 0;

    /// The team part aId names in this editor's library table, or nothing for a part no source serves
    virtual std::optional<MDW_PART_TARGET> ResolvePart( const LIB_ID& aId ) = 0;

    /// The library table this editor's parts live in
    virtual LIBRARY_TABLE_TYPE PartTableType() const = 0;

    /// Show aId unless it already is. False when it could not be opened
    virtual bool OpenPart( const LIB_ID& aId ) = 0;

    /// Bring TeamPart() up to date with the part the editor shows
    virtual void RefreshPart() = 0;

    /// Save library aNickname as the user agrees, then check it in
    virtual void CheckInLibrary( const wxString& aNickname ) = 0;

    /// Fill what only the editor knows of aItem, released together with aReleasing. False leaves it out
    virtual bool DescribeForRelease( MDW_RELEASE_ITEM& aItem, const std::vector<LIB_ID>& aReleasing ) = 0;
};


class KICOMMON_API MDW_PART_CONTROLLER
{
public:
    /// aChanged runs on the UI thread, while aOwner lives, whenever Message() or the actions may have changed
    MDW_PART_CONTROLLER( wxEvtHandler* aOwner, MDW_PART_EDITOR& aEditor, std::function<void()> aChanged );
    ~MDW_PART_CONTROLLER();

    /**
     * Track aTarget, or nothing for a part no source manages. aKey names what the editor has open, so a
     * reply for an earlier part is never applied to this one.
     */
    void SetPart( std::optional<MDW_PART_TARGET> aTarget, const wxString& aName, const wxString& aKey );

    /// True when aTarget's listing alone says the part is released
    static bool IsReleased( const std::optional<MDW_PART_TARGET>& aTarget );

    /// aTarget's listed state when that alone says the part is released, else nothing
    static std::optional<MDW_PART_STATE> ReleasedState( const std::optional<MDW_PART_TARGET>& aTarget );

    /// Tell the user over aParent why part aName, read-only in aReleased, cannot be changed. True when it is read-only
    static bool RefuseReleased( wxWindow* aParent, const std::optional<MDW_PART_STATE>& aReleased,
                                const wxString& aName );

    /**
     * Hear the team libraries' listings, once however often this is called, for as long as the controller lives.
     * aResolve runs when the part could resolve better after a listing, and aTail runs after it on every listing.
     */
    void ListenForListings( const std::function<void()>&                          aResolve,
                            const std::function<void( const MDW_MEMBER_CHANGES& )>& aTail );

    const wxString& Key() const { return m_key; }

    const std::optional<MDW_PART_TARGET>& Target() const { return m_target; }

    void Refresh();

    /// The digest of the content as edited now, which decides whether it still matches the latest release.
    /// aMemberDigest covers all of it where aDigest covers only what a placement can be compared on.
    void SetDigest( const wxString& aDigest, const wxString& aMemberDigest = wxEmptyString );

    /// Drop an outcome the editor's work has overtaken, as when a tool ends without an edit
    void RecheckOutcome();

    bool IsTeamPart() const { return m_target.has_value(); }

    /// True when the part's locator now has another source, as after a reconnect, so the part must be resolved anew
    bool SourceReplaced() const;

    /// True while a new listing could resolve the part better: none yet, not listed, or its source replaced
    bool NeedsResolving() const { return !m_target || !m_target->listed || SourceReplaced(); }

    bool IsReadOnly() const;

    /// The open part's state while IsReadOnly(), else nothing
    std::optional<MDW_PART_STATE> ReadOnlyState() const;

    /// The part's actions, less those its file rules out while a checkout or release has it
    std::vector<MDW_PART_ACTION> Actions() const;

    /// True when Actions() offers aId and allows it
    bool Allows( MDW_PART_ACTION::ID aId ) const;

    /**
     * Actions() for the open part, else MDW_PART_TARGET::ListedActions(). A released part not open offers a state
     * change whose choices are read once it is open.
     */
    std::vector<MDW_PART_ACTION> ActionsOf( const MDW_PART_TARGET& aTarget ) const;

    /// aId as one of aTargets offers it, preferring one that allows it
    std::optional<MDW_PART_ACTION> Offered( const std::vector<MDW_PART_TARGET>& aTargets,
                                            MDW_PART_ACTION::ID aId ) const;

    /// The words a menu or the infobar shows for aAction
    static wxString Label( const MDW_PART_ACTION& aAction );

    /// Why aAction is denied, as the infobar says it
    static wxString Refusal( const MDW_PART_ACTION& aAction );

    const MDW_PART_STATE& State() const { return m_state; }

    /// The infobar text for the part, or empty when there is nothing to say
    wxString Message() const;

    /// True after a release was refused over a newer version, which Get Latest brings in
    bool OffersGetLatest() const { return m_offerGetLatest; }

    /// Bring the library copy up to date, and drop the release refusal once the copy has moved past it
    void GetLatest();

    /// True once after the workspace preference changes, which decides whether any part is a team part
    bool IntegrationToggled();

    /**
     * Tell the user over aParent, once and after the edit in progress, that an edit to the released part was not
     * kept. aRestore runs first and puts the saved part back, returning false when no part is left to name.
     */
    void RefuseEdit( wxWindow* aParent, std::function<bool()> aRestore );

    /**
     * Claim the released part for edits. The checkout may write a newer release than the editor shows, so the
     * editor reloads the part first; until that works the part stays read-only, and Revise retries it.
     */
    void Revise();

    /**
     * Give up the claim on a revised part, so it reads released again. Edits since the release are discarded once the
     * user agrees, by putting the release back and reloading it; until the reload works the claim stays.
     */
    void CancelRevision();

    /// Remove another user's claim on the part, after asking for the reason its holder is told
    void ReleaseClaim();

    /**
     * Move the part's latest revision to the state aTargetStateId, asking for the comment it requires. With no
     * state, offer the part's transitions to choose from once they are read.
     */
    void Transition( const wxString& aTargetStateId );

    /**
     * Show aItems over aParent with the revision each becomes, then release them one after another with the comment
     * the user gives, after the editor saved them. Each from the newest release its library lists once saved.
     * Cancelling or closing the dialog lets the release in flight finish and starts no other. Closing the editor
     * drops the parts still waiting unreported.
     */
    void ReleaseParts( wxWindow* aParent, std::vector<MDW_RELEASE_ITEM> aItems );

    /**
     * Settling rewrote the part's file with the version the user chose. The editor reloads it; until that works
     * the part stays read-only, is not released, and ReloadSettled retries it.
     */
    void Settle();

    /// True while the editor still shows content that settling replaced on disk
    bool SettlePending() const;

    void ReloadSettled();

private:
    /// What this editor knows of a part's file beyond the library listing
    struct PROVENANCE
    {
        bool          reloadPending = false; ///< Checked out, but the editor still shows the old content
        wxString      checkoutPath;          ///< Where the checkout wrote it
        MDW_LOCK_INFO lock;                  ///< The claim the checkout took
        bool          releasing = false;     ///< A release is in flight
        bool          settling = false;      ///< Settled on disk, but the editor still shows the old content
        bool          restoring = false;     ///< Cancel Revision is putting the release back
    };

    using PART_KEY = std::pair<wxString, wxString>;

    PART_KEY          partKey() const;
    PROVENANCE*       provenance();
    const PROVENANCE* provenance() const;

    /// A checkout or release of aKey is still running
    bool busy( const PART_KEY& aKey ) const;

    /// Load the checked-out file into the editor, then hand it the claim
    void finishRevise();

    /// Put the release back over the part's edits and reload it, then give up the claim. aBlocked says why it
    /// could not, when the editor refuses the reload
    void restoreRelease( const wxString& aBlocked );

    /// Give up the claim, which leaves the part's file as it is
    void giveUpClaim();

    /// Show aMessage with part aKey while it is open, otherwise report it to the frame
    void failed( const PART_KEY& aKey, const wxString& aMessage );

    /// The editor's content and whether it has unsaved work, which most outcomes are about
    std::pair<wxString, bool> work() const;

    /**
     * Show aOutcome until the part's work changes, or until another outcome replaces it. An outcome about a block
     * passes aBlocked, which says whether it still holds, so it goes as soon as the block does.
     */
    void setOutcome( const wxString& aOutcome, std::function<bool()> aBlocked = nullptr );

    /// The block that stops ReloadPart( aDiscardEdits ), for an outcome to say it no longer holds
    std::function<bool()> reloadBlocked( bool aDiscardEdits );

    /// True when the outcome describes work that has since been edited or saved, and no op on it still runs
    bool outcomeStale() const;

    /// What happened to one release, and whether it was released
    using RELEASED = std::function<void( const wxString& aOutcome, bool aReleased )>;

    /**
     * Release aItem with aComment from the release its target lists, then run aDone. When aMayUpdate, a copy of the
     * open part found behind is brought up to date and released again.
     */
    void release( const MDW_RELEASE_ITEM& aItem, const wxString& aComment, bool aMayUpdate, RELEASED aDone );

    void transition( const wxString& aTargetStateId, const wxString& aComment );
    void chooseTransition();

    void read();
    void readHistory();

    /// Learn whether the part's library copy holds a change to it that is not checked in
    void readLocalChanges();

    /// The digest of all of the part as edited now, where the editor gives one
    const wxString& editedDigest() const;

    /// True while the part is as it was when its library copy held nothing of it to check in
    bool checkedIn() const;
    void readPending( const wxString& aChangeId );
    void changed();

    /// The snapshot token of the open part's library copy, or empty when none is indexed
    wxString copyToken() const;

    wxEvtHandler*                  m_owner;
    MDW_PART_EDITOR&               m_editor;
    std::function<void()>          m_changed;
    std::optional<MDW_PART_TARGET> m_target;
    wxString                       m_name;
    wxString                       m_key;
    MDW_PART_STATE                 m_state;
    std::map<PART_KEY, PROVENANCE> m_provenance;
    std::map<PART_KEY, MDW_OP>     m_partOps; ///< Per part, so an answer still lands after the editor moves on
    std::shared_ptr<bool>          m_lifetime; ///< Expires with the controller, before the ops it lets finish
    std::vector<kiapi::workspace::CustomState> m_states; ///< The item's lifecycle definition
    wxString                       m_readError;
    wxString                       m_outcome; ///< How the last Revise or Release went
    std::pair<wxString, bool>      m_outcomeWork; ///< work() when m_outcome was set
    std::function<bool()>          m_outcomeBlocked; ///< Whether the block m_outcome describes still holds
    bool                           m_offerGetLatest = false; ///< The last release was refused over a newer version
    wxString                       m_refusedAt;              ///< The copy's snapshot token when that refusal came
    wxString                       m_checkedIn; ///< The digest of the part when its copy held no change to it
    std::vector<MDW_OP>            m_reads;
    MDW_OP                         m_pendingRead;
    MDW_SUBSCRIPTION               m_events;
    std::shared_ptr<MDW_MEMBER_INDEX::LISTENER> m_listing;
    bool                           m_integration;
    bool                           m_refusalQueued = false;
    bool                           m_transitionsRead = false;
    bool                           m_chooseTransition = false; ///< Offer the transitions once they are read
};

#endif // MDW_PART_CONTROLLER_H
