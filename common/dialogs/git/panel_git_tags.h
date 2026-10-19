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

#ifndef PANEL_GIT_TAGS_H
#define PANEL_GIT_TAGS_H

#include <dialogs/git/git_transfer.h>
#include <dialogs/git/panel_git_tags_base.h>

class EDA_BASE_FRAME;
class GIT_ADVANCED_DIALOG;
struct KIGIT_TAG_DETAIL;


/**
 * The advanced git window's Tags page: the list beside the selected tag's detail, with the
 * actions that create, check out and delete a tag.
 */
class PANEL_GIT_TAGS : public PANEL_GIT_TAGS_BASE
{
public:
    PANEL_GIT_TAGS( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame );

    /// Fill the list from the repository's tags. The repository is read on a worker.
    void Populate();

private:
    /// Fill the detail pane with aTag, or hide it when the name is empty or gone.
    void showTagDetail( const wxString& aTag );
    void fillTagDetail( const KIGIT_TAG_DETAIL& aDetail );
    void hideTagDetail();

    void onNewTag();
    void onCheckoutTag();
    void onDeleteTag();

    GIT_ADVANCED_DIALOG* m_dialog;
    EDA_BASE_FRAME*      m_frame;

    /// List and detail reads share the session lock without waiting on the UI thread.
    KIGIT_QUERY m_listQuery;
    KIGIT_QUERY m_detailQuery;

    /// Where the user left the sash between the list and its detail pane.
    int m_tagSashPos = 0;

    /// True while the list is being refilled, when its selection events are its own.
    bool m_fillingTags = false;
};

#endif // PANEL_GIT_TAGS_H
