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

#ifndef PANEL_GIT_REMOTES_H
#define PANEL_GIT_REMOTES_H

#include <functional>

#include <wx/timer.h>

#include <dialogs/git/panel_git_remotes_base.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
class KIGIT_COMMON;
class wxDateTime;


/**
 * The advanced git window's Remotes page: the list beside the selected remote's detail, with
 * the actions that add, edit and remove a remote.
 */
class PANEL_GIT_REMOTES : public PANEL_GIT_REMOTES_BASE
{
public:
    PANEL_GIT_REMOTES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );

    /// Fill the list from the repository's remotes. The caller holds the git action lock.
    void Populate();

private:
    /// Fill the detail pane with aRemote, or hide it when the name is empty.
    void showRemoteDetail( const wxString& aRemote );
    void fillRemoteDetail( const wxString& aRemote, const wxDateTime& aFetched );
    void hideRemoteDetail();

    /// Run a change to the remotes, then refresh the cached remote so a later push uses it.
    void runRemoteAction( const std::function<bool( KIGIT_COMMON& )>& aMutation, const wxString& aError );

    void onAddRemote();
    void onEditRemote();
    void onRemoveRemote();

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    /// Re-runs the reload once when it had to skip because another git operation held the
    /// action lock.
    wxTimer m_retryTimer;

    /// Where the user left the sash between the list and its detail pane.
    int m_remoteSashPos = 0;

    /// True while the list is being refilled, when its selection events are its own.
    bool m_fillingRemotes = false;
};

#endif // PANEL_GIT_REMOTES_H
