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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PANEL_GIT_STASHES_H
#define PANEL_GIT_STASHES_H

#include <vector>

#include <dialogs/git/git_transfer.h>
#include <dialogs/git/panel_git_stashes_base.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
struct KIGIT_STASH_DETAIL;


/**
 * The advanced git window's Stashes page: the list beside the selected stash's detail, with
 * the actions that save, apply, pop and drop a stash.
 */
class PANEL_GIT_STASHES : public PANEL_GIT_STASHES_BASE
{
public:
    PANEL_GIT_STASHES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );

    /// Fill the list from the repository's stashes. The repository is read on a worker.
    void Populate();

private:
    /// Fill the detail pane with the stash aId names, or hide it when the id is empty or gone.
    void showStashDetail( const wxString& aId );
    void fillStashDetail( const KIGIT_STASH_DETAIL& aDetail );
    void hideStashDetail();

    /// The stash shown on aRow, by its commit, or empty when the row is gone.
    wxString stashIdForRow( long aRow ) const;

    void restoreSelectedStash( bool aRemoveAfterApply );
    void onStashApply();
    void onStashPop();
    void onStashDrop();
    void onStashSave();

    /// After saving the editors, read the changes before asking for a stash description.
    void checkStashChanges();

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    /// The stash commit behind each row, since a row's index is only valid for the listing it
    /// came from.
    std::vector<wxString> m_stashIds;

    /// List and detail reads share the session lock without waiting on the UI thread.
    KIGIT_QUERY m_listQuery;
    KIGIT_QUERY m_detailQuery;

    /// Reads needed before an action's confirmation, separate from list refreshes.
    KIGIT_QUERY m_actionQuery;

    /// Where the user left the sash between the list and its detail pane.
    int m_stashSashPos = 0;

    /// True while the list is being refilled, when its selection events are its own.
    bool m_fillingStashes = false;
};

#endif // PANEL_GIT_STASHES_H
