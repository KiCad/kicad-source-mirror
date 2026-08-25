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
 * @file mdw_library_sources.h
 * The library sources a user has set up, the ones a site deploys, and what a project requires of them.
 */

#ifndef MDW_CLIENT_LIBRARY_SOURCES_H
#define MDW_CLIENT_LIBRARY_SOURCES_H

#include <git/git_library_members.h>
#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_source.h>
#include <mdw/client/mdw_types.h>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/string.h>

class KIWAY;
class LIB_ID;
class LIBRARY_MANAGER;
class LIBRARY_TABLE;
class MDW_SESSION;
class PROJECT;

/**
 * One backend source of shared libraries. Its unpinned local copy is <team-libraries>/<name>, and the rows
 * registered for it carry the library table option mdw=<id>; a pinned copy's rows carry mdw=<id>@<pin>.
 */
struct MDW_LIBRARY_SOURCE_CONFIG
{
    wxString id;                ///< Minted when the source is set up
    wxString backend_id;
    wxString locator;           ///< Credential-free; how a project requirement finds the source
    wxString name;              ///< Folder under team-libraries
    wxString prefix;            ///< Prepended to every nickname the source registers
    wxString account;           ///< The username it signs in as, empty for the server's default account
    bool     site = false;      ///< Deployed in the site file, which is never written

    bool operator==( const MDW_LIBRARY_SOURCE_CONFIG& aOther ) const = default;
};


enum class MDW_REQUIREMENT_STATUS
{
    SATISFIED,
    MISSING,                    ///< No configured source has the requirement's backend and locator
    PREFIX_DIFFERS              ///< The configured source registers nicknames the project does not use
};


struct MDW_REQUIREMENT_STATE
{
    MDW_LIBRARY_REQUIREMENT requirement;
    MDW_REQUIREMENT_STATUS  status = MDW_REQUIREMENT_STATUS::MISSING;
    wxString                source_id;      ///< Empty when MISSING
};


/// The team source a part with no member came from
struct MDW_TEAM_SOURCE
{
    wxString name;      ///< Empty for the user's own library
    wxString locator;   ///< Empty when only a row marker names it
};


/// What a project's team libraries need from the user before they resolve, most pressing first
enum class MDW_LIBRARY_NOTICE
{
    NONE,
    DISABLED,       ///< The workspace preference is off
    NOT_SET_UP,     ///< A requirement has no matching source
    NOT_FETCHED     ///< A matching source has no copy yet at the requirement's pin
};


struct MDW_LIBRARY_STATUS
{
    MDW_LIBRARY_NOTICE                 notice = MDW_LIBRARY_NOTICE::NONE;
    std::vector<MDW_REQUIREMENT_STATE> states;      ///< The requirements the notice is about
};


/// The type, nickname, mdw marker and expanded comparable path of each library table row a source registered
using MDW_LIBRARY_REGISTRATIONS = std::set<std::tuple<MDW_LIBRARY_TYPE, wxString, wxString, wxString>>;


/// One library registered from a local copy
struct MDW_COPY_LIBRARY
{
    wxString         library_id;
    wxString         nickname;          ///< As registered, with the source's prefix
    MDW_LIBRARY_TYPE type = MDW_LIBRARY_TYPE::UNKNOWN;
    wxString         relative_path;
};


/// One member of a local copy, as the backend listed it at the copy's snapshot
struct MDW_COPY_MEMBER
{
    wxString member_id;
    wxString library_id;
    wxString name;                      ///< The item name inside the library, as in a LIB_ID
    wxString path_hint;
    wxString checked_in;                ///< The revision a check-in from this copy made, until the copy is synced
};


/**
 * What was last written into one local copy, kept in an index beside it. Every lookup from a library table
 * row to its source and members reads this, so it answers offline and without a round trip.
 */
struct MDW_LIBRARY_COPY
{
    wxString                      source_id;
    wxString                      backend_id;   ///< The source's at the time the copy was written
    wxString                      locator;
    wxString                      local_root;
    MDW_SNAPSHOT                  snapshot;     ///< What the copy holds
    wxString                      generation;   ///< Fresh on every index write, empty in an index from before
    bool                          pinned = false;
    std::vector<MDW_COPY_LIBRARY> libraries;
    std::vector<MDW_COPY_MEMBER>  members;
};


/// A copy a sync found nothing in place of, which only the set-up that ran the sync may roll back
struct MDW_CREATED_COPY
{
    MDW_SNAPSHOT            pin;
    std::optional<wxString> indexed; ///< The generation of the last index the sync wrote, if it wrote one
};


struct MDW_LIBRARY_SYNC_RESULT
{
    MDW_RESULT                       result;
    MDW_LIBRARY_COPY                 copy;       ///< The copy as its index now records it
    std::vector<MDW_MEMBER_CHANGE>   changes;
    std::vector<MDW_UPDATE_CONFLICT> conflicts;  ///< When set, nothing changed locally
    std::optional<MDW_CREATED_COPY>  created;    ///< Set when there was no copy as the sync took its lock

    /// The copy was written whole or updated with changes, even when a later step failed
    bool filesChanged = false;
};


/// An update conflict with its sides written as single-member library files
struct MDW_CONFLICT_SIDES
{
    MDW_UPDATE_CONFLICT conflict;
    wxString            ancestorFile;    ///< Empty when there is no base or the backend cannot write it
    wxString            oursFile;        ///< Empty when hasOurs is false or the member cannot be extracted
    wxString            theirsFile;      ///< Empty when upstream removed it or the backend cannot write it
    bool                hasOurs = false; ///< False when the member is gone from the copy
};


struct MDW_CONFLICT_SIDES_RESULT
{
    MDW_RESULT                      result;
    std::vector<MDW_CONFLICT_SIDES> sides;
};


/// A symbol library's files as a format upgrade read them and as it saved them aside, by path
struct MDW_FORMAT_UPGRADE
{
    wxString                        library; ///< The .kicad_sym file or .kicad_symdir folder
    std::map<wxString, std::string> before;
    std::map<wxString, std::string> after;
};


struct MDW_LIBRARY_CHECKIN_RESULT
{
    MDW_RESULT          result;    ///< Fails only when the batch never ran; each member answers in checkin
    MDW_LIBRARY_CHECKIN checkin;
    MDW_LIBRARY_COPY    copy;      ///< The copy as its index now records it
};


struct MDW_LIBRARY_CHANGES_RESULT
{
    MDW_RESULT                    result;
    std::vector<MDW_LOCAL_CHANGE> changes;      ///< Every change under the copy, each with its checkin permission
};


/// The source and library behind one library table row
struct MDW_LIBRARY_ROW
{
    wxString     source_id;
    wxString     locator;
    wxString     library_id;
    wxString     local_root;
    MDW_SNAPSHOT snapshot;
    bool         pinned = false;
};


/**
 * The user's sources, kept in a writable settings file, followed by the sources of a read-only site file
 * an administrator deploys. A site source wins over a user source with the same id, name or locator. Both
 * files are read again whenever either changes on disk, so every KiCad process sees the same sources.
 */
class KICOMMON_API MDW_LIBRARY_SOURCES
{
public:
    /// An empty aRoot follows DefaultRoot, so copies move when the user changes KICAD_TEAM_LIBRARIES
    MDW_LIBRARY_SOURCES( const wxString& aUserFile, const wxString& aSiteFile, const wxString& aRoot );

    /// The sources of this user, kept in the settings directory and under team-libraries
    static MDW_LIBRARY_SOURCES& Get();

    /// True when the workspace preference is on; without it team libraries behave as ordinary ones
    static bool TeamLibrariesEnabled();

    /// The mdw marker in a table row's options; empty for a row no team library owns
    static wxString MarkerOf( const wxString& aRowOptions );

    /// Where the sources' local copies live, KICAD_TEAM_LIBRARIES when set and else team-libraries in documents
    static wxString DefaultRoot();

    /// A locator with any password or token removed, the only form stored or compared
    static wxString StripCredentials( const wxString& aLocator );

    /// An id that cannot be mistaken for another source's pinned marker
    static bool IsSafeId( const wxString& aId );

    /// A folder name under team-libraries that no other source and no pinned copy can share
    static bool IsSafeName( const wxString& aName );

    /// The last part of aLocator without its .git suffix, or empty when that is not a safe name
    static wxString DefaultName( const wxString& aLocator );

    /// The name a set-up of aRequirement offers, which is the one the project recorded unless that is unsafe
    static wxString NameFor( const MDW_LIBRARY_REQUIREMENT& aRequirement );

    std::vector<MDW_LIBRARY_SOURCE_CONFIG> Sources() const;

    std::optional<MDW_LIBRARY_SOURCE_CONFIG> Find( const wxString& aId ) const;

    /**
     * Add a user source, minting its id when empty and stripping credentials from its locator. Fails when
     * the name is unsafe or taken, or another source already serves the same locator.
     */
    MDW_RESULT Add( MDW_LIBRARY_SOURCE_CONFIG& aSource );

    /// Forget a user source. Its local copy stays on disk, since it may hold work not yet checked in.
    MDW_RESULT Remove( const wxString& aId );

    /// Record who the user source aId signs in as. ITEM_NOT_FOUND for a site source, whose file is never written
    MDW_RESULT SetAccount( const wxString& aId, const wxString& aAccount );

    /// The source's unpinned copy, or the copy of one pinned snapshot beside it
    wxString LocalRoot( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_SNAPSHOT& aPin = {} ) const;

    /**
     * The key of the source's library session. Its trailing separator makes the session's local root the unpinned
     * copy itself, which is where the backend reads members after an update.
     */
    wxString SessionKey( const MDW_LIBRARY_SOURCE_CONFIG& aSource ) const;

    /// Which configured source serves each requirement
    std::vector<MDW_REQUIREMENT_STATE> Match( const std::vector<MDW_LIBRARY_REQUIREMENT>& aRequirements ) const;

    /// Whether two locators name one location, differing at most in credentials, scheme case or trailing slashes
    static bool SameLocator( const wxString& aA, const wxString& aB );

    /**
     * The team source a part with no member came from, from aSources or else from the project's aNeeds; an
     * empty name for the user's own library. aRowMarker is the mdw marker of the nickname's row, nullopt when
     * no row resolves it. A marked row names its source, or the marker itself when neither list knows it. With
     * no row the longest nickname prefix wins. An empty prefix claims nothing, except that of a need no source
     * in aSources satisfies, whose nicknames are unknown here. The first such need claims the nickname.
     */
    static MDW_TEAM_SOURCE TeamSourceOf( const wxString& aNickname, const std::optional<wxString>& aRowMarker,
                                         const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                         const std::vector<MDW_LIBRARY_REQUIREMENT>&   aNeeds );

    /**
     * The value of the mdw table option on the rows of aSource's copy at aPin. A pinned copy's marker, like its
     * folder, comes from the backend and location alone, so a shared project table names it alike everywhere.
     */
    static wxString Marker( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_SNAPSHOT& aPin = {} );

    /// The folder merge backups of aCopy go in, beside the copy's index and never in an open project
    wxString BackupRoot( const MDW_LIBRARY_COPY& aCopy ) const;

    /// aPath absolute and normalized, so a row's expanded URI compares with a copy's library path
    static wxString ComparablePath( const wxString& aPath );

    /// True for the marker of a pinned copy, whose rows belong in the project tables
    static bool IsPinned( const wxString& aMarker );

    /**
     * Replace the index of the copy at aCopy.local_root, which must be one of a source's copies. Each write
     * stamps a fresh generation, returned in aGeneration, so a writer can tell its index from a later one.
     */
    MDW_RESULT WriteCopy( const MDW_LIBRARY_COPY& aCopy, wxString* aGeneration = nullptr );

    /**
     * The index of the copy whose rows carry aMarker; nullopt when the copy was never written. An index another
     * configuration wrote for the same backend, location and prefix serves this source too.
     */
    std::optional<MDW_LIBRARY_COPY> Copy( const wxString& aMarker ) const;

    /// Forget the index of the copy whose rows carry aMarker, leaving its files. Its source must still be set up.
    MDW_RESULT DropCopy( const wxString& aMarker );

    /// True when any copy of aSource, pinned or not, has an index written for it
    bool HasCopy( const MDW_LIBRARY_SOURCE_CONFIG& aSource ) const;

    /// The same index shared rather than copied, for readers that keep no copy of their own
    std::shared_ptr<const MDW_LIBRARY_COPY> SharedCopy( const wxString& aMarker ) const;

    /**
     * The copy and library behind a table row. The row's mdw marker names the copy and aRowPath, the row's fully
     * expanded URI, names the library inside it, so a renamed row still resolves. Nullopt for a row without a
     * marker or one reaching no library of that copy.
     */
    std::optional<MDW_LIBRARY_ROW> ForRowAt( MDW_LIBRARY_TYPE aType, const wxString& aRowOptions,
                                             const wxString& aRowPath ) const;

    /// The member aItemName of the library ForRowAt finds, its source set to the source's locator
    std::optional<MDW_ITEM_REF> MemberForAt( MDW_LIBRARY_TYPE aType, const wxString& aRowOptions,
                                             const wxString& aRowPath, const wxString& aItemName ) const;

    /// The member aLibId names, through the row its nickname resolves to in aTables, project rows over global ones
    std::optional<MDW_ITEM_REF> MemberOf( LIBRARY_MANAGER& aTables, MDW_LIBRARY_TYPE aType,
                                          const LIB_ID& aLibId ) const;

    /**
     * The LIB_ID this computer's library table knows aMember by, found through the member id in the copies of
     * the sources, so the source's nickname prefix applies. Nullopt when no copy holds the member. A member
     * with a source names only that source.
     */
    std::optional<LIB_ID> LocalLibId( MDW_LIBRARY_TYPE aType, const MDW_ITEM_REF& aMember ) const;

    /**
     * What stands between aNeeds and their libraries resolving. A copy counts once its folder exists and every
     * symbol and footprint library it holds has a row among aRegistered pointing at this machine's copy.
     */
    MDW_LIBRARY_STATUS Status( const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds, bool aEnabled,
                               const MDW_LIBRARY_REGISTRATIONS& aRegistered ) const;

    /// True when aSource serves aRequirement with the nicknames the project uses
    static bool Satisfies( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_LIBRARY_REQUIREMENT& aRequirement );

    /// What a project records when it starts using aSource
    static MDW_LIBRARY_REQUIREMENT RequirementFor( const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                                   const MDW_SNAPSHOT&              aPin = {} );

    /**
     * The requirements of the sources whose libraries a design uses, found through the rows aTables holds for
     * the aUsedNicknames. Reads the tables and the source list only, never the network. A pinned row names
     * its pin elsewhere, so it adds none.
     */
    static std::vector<MDW_LIBRARY_REQUIREMENT> RequirementsFor( const std::set<wxString>&  aUsedNicknames,
                                                                 LIBRARY_MANAGER&           aTables,
                                                                 MDW_LIBRARY_TYPE           aType,
                                                                 const MDW_LIBRARY_SOURCES& aSources = Get() );

    /// Append the aNeeded that aRecorded does not name by locator. True when any was appended.
    static bool Record( std::vector<MDW_LIBRARY_REQUIREMENT>&       aRecorded,
                        const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeded );

    /// Set on the GUI thread while a library sync or any of its dialogs is up, kept here so every kiface shares it
    bool& LibrarySyncRunning() { return m_librarySyncRunning; }

private:
    /// A file's identity on disk, so a write by another process within the same second is still seen
    struct STAMP
    {
        bool                            exists = false;
        std::filesystem::file_time_type time;
        std::uintmax_t                  size = 0;

        bool operator==( const STAMP& aOther ) const = default;
    };

    static STAMP stamp( const wxString& aFile );

    /// Re-read both files when either changed since the last read. Holds m_mutex.
    void refresh() const;

    /// Lock the user file against other processes and apply aEdit to its own sources. Holds m_mutex.
    MDW_RESULT editUserFile( const std::function<MDW_RESULT( std::vector<MDW_LIBRARY_SOURCE_CONFIG>& )>& aEdit );

    /// The source and copy folder aMarker names. Holds m_mutex.
    std::optional<std::pair<MDW_LIBRARY_SOURCE_CONFIG, wxString>> resolveMarker( const wxString& aMarker ) const;

    /// A source's name on every machine, from where it is rather than what a user called it
    static wxString SourceKey( const wxString& aBackendId, const wxString& aLocator );

    wxString root() const;
    wxString indexFile( const wxString& aFolder ) const;

    /// A copy with its lookups built once, since every library row and placed part asks again
    struct INDEXED_COPY
    {
        MDW_LIBRARY_COPY                                         copy;
        std::map<std::pair<MDW_LIBRARY_TYPE, wxString>, size_t> libraryByPath;
        std::map<std::pair<wxString, wxString>, size_t>         memberByName;
        std::map<wxString, size_t>                              memberById;
    };

    /// The cached copy aMarker names, shared so a lookup does not copy its members. Holds m_mutex.
    std::shared_ptr<const INDEXED_COPY> cachedCopy( const wxString& aMarker ) const;

    std::shared_ptr<const INDEXED_COPY> copyForRow( const wxString& aRowOptions ) const;

    /// The library of aCopy whose files are at aPath
    static const MDW_COPY_LIBRARY* libraryAt( const INDEXED_COPY& aCopy, MDW_LIBRARY_TYPE aType,
                                              const wxString& aPath );

    wxString m_userFile;
    wxString m_siteFile;
    wxString m_root;

    mutable std::mutex                             m_mutex;
    mutable std::vector<MDW_LIBRARY_SOURCE_CONFIG> m_sources;
    mutable std::optional<STAMP>                   m_userStamp;
    mutable std::optional<STAMP>                   m_siteStamp;

    /// Copy indexes by index path, each with the stamp it was read at
    mutable std::map<wxString, std::pair<STAMP, std::shared_ptr<const INDEXED_COPY>>> m_copies;

    bool m_librarySyncRunning = false;
};


/// A backend a workspace or library source can be set up with
struct MDW_BACKEND_CHOICE
{
    wxString backend_id;
    wxString display_name;

    /// What the backend's manifest says it needs, or nothing when the manifest does not say
    std::optional<std::vector<MDW_BINDING_FIELD>> binding_fields;

    MDW_BINDING_SOURCE binding_source = MDW_BINDING_SOURCE::NONE;
    bool               binding_search = false;
};


/**
 * What the Set Up dialogs need beyond the sources. Credentials go to the platform vault and a password
 * never comes back out to the UI.
 */
class KICOMMON_API MDW_SETUP
{
public:
    /// Every installed backend, or those whose manifest offers aGroup, whether or not the preference is on
    static std::vector<MDW_BACKEND_CHOICE> Backends( std::optional<MDW_CAPABILITY_GROUP> aGroup = std::nullopt );

    /// The server whose credential a binding signs in with, aLocator's or else that of aProjectPath's open session
    static wxString Realm( const wxString& aProjectPath, const wxString& aLocator );

    /// Whether a password is saved for aUsername at aRealm, never prompting; a locked keyring reads as none
    static bool HasCredential( const wxString& aRealm, const wxString& aUsername );

    /// The username a binding signs in to aRealm as, aBindingAccount or else the server's default account
    static wxString Username( const wxString& aRealm, const wxString& aBindingAccount );

    /**
     * Launch the backend for a binding, sign in with aTyped or, when it holds no password, the saved credential,
     * and shut it down again, without touching any open session. aTyped is saved once the server accepts it. A
     * server presenting aTrust is trusted and tried again. A passing test hands its running session to aKeep, when
     * given, for the caller to shut down, instead of stopping it. Gives up as TIMEOUT at its own deadline and as
     * CANCELLED when an MDW_CANCEL_SCOPE on the calling thread asks. aKeyFile is the private key to sign in with,
     * empty for the backend's default. Blocks, so never call it on the UI thread.
     */
    static std::pair<MDW_RESULT, MDW_SOURCE_INFO>
    TestConnection( const wxString& aBackendId, const wxString& aWorkspaceId, const wxString& aLocator,
                    const wxString& aProjectPath, const MDW_TYPED_CREDENTIALS& aTyped,
                    const std::optional<MDW_HOST_KEY>& aTrust = std::nullopt,
                    std::shared_ptr<MDW_SESSION>* aKeep = nullptr, const wxString& aKeyFile = wxEmptyString );

    /**
     * What aSession's backend lists to bind to for aText. NOT_SUPPORTED when it lists none, as every backend
     * that predates the request answers. Blocks, so never call it on the UI thread.
     */
    static std::pair<MDW_RESULT, std::vector<MDW_BINDING_TARGET>>
    FindBindingTargets( const std::shared_ptr<MDW_SESSION>& aSession, const wxString& aText );

    /// aLocator with the item aItem, "key=value", set in its query in place of any item of that key
    static wxString LocatorWithItem( const wxString& aLocator, const wxString& aItem );

    /// aLocator without the items of key aKey in its query, or aLocator itself when it has none
    static wxString LocatorWithoutItem( const wxString& aLocator, const wxString& aKey );

    /// MDW_FRAMEWORK::Reconnect() for the library session of aSource. Blocks, so never call it on the UI thread
    static MDW_RESULT ReconnectLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                              const MDW_CONNECTION_ANSWER&     aAnswer );

    /**
     * Close the session of aProjectPath, so it opens again under the binding now in its project file. Without
     * aWaitForExit its backend shuts down in the background, as MDW_FRAMEWORK::CloseSession() describes.
     */
    static void CloseProjectSession( const wxString& aProjectPath, bool aWaitForExit = true );

    /**
     * Stop using aProject's Design Workspace on this computer. Only the computer's own project settings change, so
     * the binding in the project file, which the team shares, stays. Closes the session. False, with nothing
     * changed, when the project's local settings could not be saved.
     */
    static bool DisconnectHere( PROJECT& aProject );

    /**
     * Remember that aProject signed in to aBinding as aUsername through its own dialog or prompt. A backend on the
     * project's git remote records it as the project's git username; any other binding names no account, so the
     * server's default account follows it. Nothing changes for an empty aUsername.
     */
    static void RememberProjectAccount( PROJECT& aProject, const MDW_PROJECT_BINDING& aBinding,
                                        const wxString& aUsername );

    /**
     * Record aUsername as the account of aSource, which already exists, and close its session so it opens again
     * as that account. A site source names no one, so the server's default account follows it instead.
     */
    static MDW_RESULT RememberSourceAccount( MDW_LIBRARY_SOURCE_CONFIG& aSource, const wxString& aUsername );

    /// Undo DisconnectHere(), so the project opens its session again under the binding in its file
    static bool ReconnectHere( PROJECT& aProject );

    /// Remove in the background what fills of copies left when their backend died before cleaning up
    static void SweepDeadCopies();

    /**
     * Undo a set-up of aSource that left nothing usable by closing its session, deleting aCopies with their
     * index and forgetting the source. A copy whose index anything has written since may hold work not yet checked
     * in, so it stays, and so does one another set-up is still writing. The source stays while any copy of it does.
     */
    static MDW_RESULT DiscardLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG&     aSource,
                                            const std::vector<MDW_CREATED_COPY>& aCopies );

    /**
     * Connect to aSource and bring its copy at aPin up to date, then rewrite its index and announce
     * LIBRARY_CHANGED for each library. An unpinned copy that exists is updated; any other is written whole.
     * aResolutions answer the conflicts an earlier sync returned. Blocks on the backend, so never call it on
     * the UI thread.
     */
    static MDW_LIBRARY_SYNC_RESULT SyncLibrarySource( const MDW_LIBRARY_SOURCE_CONFIG&            aSource,
                                                      const MDW_SNAPSHOT&                         aPin = {},
                                                      const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions = {} );

    /// SyncLibrarySource() of aConfig, one of aSources, for a caller that already holds aSource connected to it
    static MDW_LIBRARY_SYNC_RESULT SyncLibrarySource( MDW_LIBRARY_SOURCES&                        aSources,
                                                      const std::shared_ptr<MDW_SOURCE>&          aSource,
                                                      const MDW_LIBRARY_SOURCE_CONFIG&            aConfig,
                                                      const MDW_SNAPSHOT&                         aPin,
                                                      const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions );

    /**
     * Download the base and theirs of each of aConflicts from aSource into aFolder, and extract ours from the
     * copy at aLocalRoot beside them. Blocks on the backend, so never call it on the UI thread.
     */
    static MDW_CONFLICT_SIDES_RESULT ConflictSides( const MDW_LIBRARY_SOURCE_CONFIG&        aSource,
                                                    const wxString&                         aLocalRoot,
                                                    const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                                                    const wxString&                         aFolder );

    /// True when the ours and theirs files of aSide hold the same text of its member
    static bool SidesMatch( const MDW_CONFLICT_SIDES& aSide );

    /// The text of the member aConflict names in the file at aPath, or nothing when it cannot be read
    static std::optional<std::string> ReadMember( const wxString& aPath, const MDW_UPDATE_CONFLICT& aConflict );

    /// Library file text holding the member a member file holds, saved whole in the current format as
    /// SYM_LIB_DIFFER::UpgradedWithMember() does; given the library text, the texts of the other files of its folder,
    /// the member file text and the member's name
    using UPGRADE = std::function<std::optional<std::string>( const std::string&, const std::vector<std::string>&,
                                                              const std::string&, const wxString& )>;

    /// UPGRADE as the schematic kiface aKiway loads does it, loaded only once a symbol needs it
    static UPGRADE SymbolUpgrade( KIWAY& aKiway );

    /**
     * Library file text aDestination with its symbol aName as aMemberFile, a file holding only that symbol, has it.
     * A symbol reads by its file's header, so it is spliced in as is only where both texts name the same format
     * version. Otherwise aUpgrade saves both whole in the current format, as a first save would, finding parents
     * also in aFolder, the texts of the other files of aDestination's .kicad_symdir folder. Nothing when the symbol
     * cannot be placed.
     */
    static std::optional<std::string> SpliceMember( const std::string&              aDestination,
                                                    const std::vector<std::string>& aFolder,
                                                    const std::string& aMemberFile, const wxString& aName,
                                                    const UPGRADE& aUpgrade );

    /**
     * Replace the member aConflict names in the library file at aPath with aAnswer, a file holding only that member
     * as a conflict prompt answered it, by SpliceMember() for a symbol. False when either lacks the member or aPath
     * cannot be written.
     */
    static bool WriteMember( const wxString& aPath, const MDW_UPDATE_CONFLICT& aConflict, const std::string& aAnswer,
                             const UPGRADE& aUpgrade );

    /**
     * aResolutions, whose merged files each hold only the member aConflicts names for it, as the backend may place
     * them in the copy at aLocalRoot. A symbol answer is spliced as is only into a library file of its own format, and
     * only when it does not extend another. Every answer to any other file goes into that whole file instead, which
     * aUpgrade saves in the current format where formats differ, and each such resolution carries the file with
     * whole_file_of set to the blob id of the file it was built from. Nothing when an answer cannot be placed.
     */
    static std::optional<std::vector<MDW_CONFLICT_RESOLUTION>>
    PlacedAnswers( const wxString& aLocalRoot, const std::vector<MDW_UPDATE_CONFLICT>& aConflicts,
                   const std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions, const UPGRADE& aUpgrade );

    /**
     * Connect to the source of aCopy and check in aChanges, rows LibraryChangesFor listed. The members the
     * reply accepted are recorded in the copy's index and announced as LIBRARY_CHANGED. No file is rewritten,
     * so a member left out or refused keeps its edits. A pinned copy, or a change the source may not check in,
     * is refused before the backend is asked. A row with a format_digest commits its file as it is on disk, and so
     * do the rows with a file_digest when all of that file's file_edits are among aChanges.
     * Blocks on the backend, so never call it on the UI thread.
     */
    static MDW_LIBRARY_CHECKIN_RESULT CheckInLibraryCopy( const MDW_LIBRARY_COPY&              aCopy,
                                                          const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                          const wxString&                      aComment );

    /// CheckInLibraryCopy() through aSource and the index aSources, as a caller that holds both has them
    static MDW_LIBRARY_CHECKIN_RESULT CheckInLibraryCopy( MDW_LIBRARY_SOURCES&                 aSources,
                                                          const std::shared_ptr<MDW_SOURCE>&   aSource,
                                                          const MDW_LIBRARY_COPY&              aCopy,
                                                          const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                          const wxString&                      aComment );

    /**
     * The files of the symbol library at aPath, a .kicad_sym file or a .kicad_symdir folder, whose header names a
     * format version older than aVersion. A file naming none is older, and one that is no symbol library is not.
     */
    static std::vector<wxString> OutdatedSymbolFiles( const wxString& aPath, long aVersion );

    /// True when the source at aLocator last said this user may not check in
    static bool CheckInKnownDenied( const wxString& aLocator );

    /**
     * Read the symbol library at aPath, have aSave write it in the current file format to the path it is given, and
     * answer both, or nothing with aError set. The library's files are left alone. aSave may run editor code, so
     * call this on the UI thread.
     */
    static std::optional<MDW_FORMAT_UPGRADE>
    StageFormatUpgrade( const wxString& aPath, const std::function<bool( const wxString& )>& aSave, wxString& aError );

    /// Ok when aDiffSymbols reads every symbol of aUpgrade the same before and after, parents included
    static MDW_RESULT CheckFormatUpgrade( const MDW_FORMAT_UPGRADE&        aUpgrade,
                                          const KIGIT::LIBRARY_FILES_DIFF& aDiffSymbols );

    /**
     * Save aUpgrade's files of the library in aCopy and check them in with aComment as one format upgrade, which
     * revises no symbol and leaves released ones released. Every file must name format aVersion or later. Holds the
     * copy's lock throughout, and refuses while the library has changes or a file no longer holds what the upgrade
     * read. A file the upgrade saved is put back when nothing was committed, unless it changed since. Nothing is
     * checked in when the files already were current.
     */
    static MDW_LIBRARY_CHECKIN_RESULT CheckInFormatUpgrade( const MDW_LIBRARY_COPY&   aCopy,
                                                            const MDW_FORMAT_UPGRADE& aUpgrade, long aVersion,
                                                            const wxString& aComment );

    /// CheckInFormatUpgrade() through aSource and the index aSources, as a caller that holds both has them
    static MDW_LIBRARY_CHECKIN_RESULT CheckInFormatUpgrade( MDW_LIBRARY_SOURCES&               aSources,
                                                            const std::shared_ptr<MDW_SOURCE>& aSource,
                                                            const MDW_LIBRARY_COPY&            aCopy,
                                                            const MDW_FORMAT_UPGRADE& aUpgrade, long aVersion,
                                                            const wxString& aComment );

    /**
     * Move the unpinned copy of aMarker to aTo, the snapshot a release committed, so its members list as its files
     * now are. Only a copy at aBase, the snapshot the release built on, or at aAskedAt, where the copy stood when
     * the release was asked for, moves. One that a sync is writing is left alone.
     */
    static void AdvanceLibraryCopy( const wxString& aMarker, const wxString& aBase, const wxString& aAskedAt,
                                    const MDW_SNAPSHOT& aTo );

    /**
     * Every local change under aCopy, and those its source may check in. A source with a lifecycle publishes
     * an existing member only through Revise and Release, so only its new members qualify. aDiffSymbols, when
     * set, drops a modified symbol whose text changed but not its meaning, and lists a file with only such
     * symbols as one row with a format_digest. The rows of a file that also has such symbols carry its file_digest.
     * A source not logged in answers NOT_AUTHENTICATED. Blocks, so never call it on the UI thread.
     */
    static MDW_LIBRARY_CHANGES_RESULT LibraryChangesFor( const MDW_LIBRARY_COPY&    aCopy,
                                                         const KIGIT::LIBRARY_DIFF& aDiffSymbols );

    /// LibraryChangesFor() through aSource, as a caller that holds it has it
    static MDW_LIBRARY_CHANGES_RESULT LibraryChangesFor( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                         const MDW_LIBRARY_COPY&            aCopy,
                                                         const KIGIT::LIBRARY_DIFF&         aDiffSymbols );

    /**
     * Make the rows aTable holds for aSources match their unpinned copies, removing the rows of one with no
     * copy. Other sources' rows and rows the user added are never touched, and a nickname of aSources that
     * one of those holds, or one in aTaken, is returned in aConflicts.
     *
     * @return true when the table changed and needs saving.
     */
    static bool SyncLibraryRows( LIBRARY_TABLE& aTable, MDW_LIBRARY_TYPE aType,
                                 const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                 std::vector<wxString>& aConflicts, const std::set<wxString>& aTaken = {},
                                 const MDW_LIBRARY_SOURCES& aStore = MDW_LIBRARY_SOURCES::Get() );

    /// SyncLibraryRows() for a project table and the pinned copies of aNeeds, reporting conflicts of aReportSources
    static bool SyncPinnedRows( LIBRARY_TABLE& aTable, MDW_LIBRARY_TYPE aType,
                                const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds,
                                const std::set<wxString>& aReportSources, std::vector<wxString>& aConflicts,
                                const std::set<wxString>&  aTaken = {},
                                const MDW_LIBRARY_SOURCES& aStore = MDW_LIBRARY_SOURCES::Get() );

    /// MDW_LIBRARY_SOURCES::Status() against the rows of the loaded library tables
    static MDW_LIBRARY_STATUS LibraryStatus( const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds, bool aEnabled );
};


#endif // MDW_CLIENT_LIBRARY_SOURCES_H
