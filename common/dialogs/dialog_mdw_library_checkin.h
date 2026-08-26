/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_MDW_LIBRARY_CHECKIN_H
#define DIALOG_MDW_LIBRARY_CHECKIN_H

#include <dialogs/dialog_mdw_library_checkin_base.h>
#include <dialogs/mdw_library_checkin_rows.h>
#include <mdw/client/mdw_library_sources.h>

#include <functional>
#include <vector>

class KIWAY;
class PROJECT;
enum class LIBRARY_TABLE_TYPE;


/**
 * The team library marker on the library row aNickname, or empty when none owns it or team libraries are off.
 * Reads only the loaded library table, so UI conditions may call it.
 */
wxString MdwTeamLibraryMarker( LIBRARY_TABLE_TYPE aType, const wxString& aNickname );

/// True for a team library row whose source is set up, or a pinned copy so the action can explain its refusal
bool MdwCanCheckInLibrary( LIBRARY_TABLE_TYPE aType, const wxString& aNickname );

/// Explain over aParent that a pinned copy cannot be checked in. True when aNickname is one
bool MdwRefusePinnedCheckIn( wxWindow* aParent, LIBRARY_TABLE_TYPE aType, const wxString& aNickname );

/**
 * List the local changes of the copy behind the library row aNickname and let the user check them in. The
 * library itself is ticked; other libraries of the same copy are listed for the user to add. aKiway lends the
 * symbol differ that tells a format rewrite from an edit.
 */
void ShowTeamLibraryCheckIn( wxWindow* aParent, KIWAY& aKiway, LIBRARY_TABLE_TYPE aType, const wxString& aNickname );

/**
 * ShowTeamLibraryCheckIn() after offering to save the editor's unsaved changes to aNickname, which aModified says
 * exist. aSave writes them and returns false on failure, which abandons the check-in.
 */
void SaveAndCheckInTeamLibrary( wxWindow* aParent, KIWAY& aKiway, LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                bool aModified, const std::function<bool()>& aSave );

/// UpdateLibrarySources() for the source behind the library row aNickname, so a librarian needs no project
void UpdateTeamLibrarySource( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject, LIBRARY_TABLE_TYPE aType,
                              const wxString& aNickname );

/**
 * True when the symbol library row aNickname is an unpinned team library copy with a file older than format
 * aVersion, and its source has not said this user may not check in. Footprints are never offered, since each is a
 * file of its own that its first save alone rewrites. Reads only the files' headers, so UI conditions may call it.
 */
bool MdwCanUpgradeLibraryFormat( const wxString& aNickname, long aVersion );

/**
 * After the user agrees, have aSave write the team symbol library aNickname, given its path, in format aVersion to
 * the output path it is given, then check that in for the team as a format upgrade. aSave runs on the UI thread.
 * True when the library's files may have changed.
 */
bool UpgradeTeamLibraryFormat( wxWindow* aParent, KIWAY& aKiway, const wxString& aNickname, long aVersion,
                               const std::function<bool( const wxString&, const wxString& )>& aSave );


/// A copy's local changes as the check-in dialog lists them
struct MDW_CHECKIN_LISTING
{
    MDW_RESULT                   result;
    std::vector<MDW_CHECKIN_ROW> rows;
};


/**
 * A checklist of a team library copy's local changes, a comment, and after the check-in one result per member.
 * The blocking listing and check-in are handed in and run on a worker, so another surface can open the same dialog.
 */
class DIALOG_MDW_LIBRARY_CHECKIN : public DIALOG_MDW_LIBRARY_CHECKIN_BASE
{
public:
    using LIST = std::function<MDW_CHECKIN_LISTING()>;
    using CHECK_IN = std::function<MDW_LIBRARY_CHECKIN_RESULT( const std::vector<MDW_LOCAL_CHANGE>& aChanges,
                                                               const wxString&                      aComment )>;

    /// Ask the user to sign in to the source on the UI thread; true once they did
    using SIGN_IN = std::function<bool( wxWindow* aParent )>;

    /// Bring the copy up to date with the source, showing whatever that asks of the user
    using GET_UPDATES = std::function<void( wxWindow* aParent )>;

    /// aListing is the first listing, which may already say the source needs a sign-in
    DIALOG_MDW_LIBRARY_CHECKIN( wxWindow* aParent, const wxString& aSourceName, MDW_CHECKIN_LISTING aListing,
                                LIST aList, CHECK_IN aCheckIn, SIGN_IN aSignIn, GET_UPDATES aGetUpdates );

private:
    void showChecklist();
    void fillChanges();
    /// aProblem is a failure after the backend answered, shown under the counts
    void showResults( const MDW_LIBRARY_CHECKIN& aCheckIn, const std::vector<MDW_LOCAL_CHANGE>& aSent,
                      const wxString& aProblem = wxEmptyString );
    void updateCheckInButton();
    void checkIn();
    MDW_RESULT refresh();
    void signIn();

    /// Get the updates the refused members were behind, then check them in once more
    void updateAndRetry();

    /// Get the updates, where a member another edit changed is resolved, then close
    void resolve();

    /// Keep the list's width and fit the height to whether the list is shown
    void fitToState();

    /// Offer Sign In... when aResult says the source has no login, else report it as a failure to read
    bool needsSignIn( const MDW_RESULT& aResult );

    void onToggled( wxDataViewEvent& aEvent );
    void onCheckIn( wxCommandEvent& aEvent );

    std::vector<MDW_CHECKIN_ROW> m_rows;
    LIST                         m_list;
    CHECK_IN                     m_checkIn;
    SIGN_IN                      m_signIn;
    GET_UPDATES                  m_getUpdates;
    bool                         m_retried = false;
    bool                         m_done = false;
    int                          m_listWidth = 0;

    /// What the default button does next
    enum class NEXT
    {
        CHECK_IN,
        REFRESH,
        SIGN_IN,
        UPDATE_AND_RETRY,
        RESOLVE
    };

    NEXT m_next = NEXT::CHECK_IN;
};

#endif // DIALOG_MDW_LIBRARY_CHECKIN_H
