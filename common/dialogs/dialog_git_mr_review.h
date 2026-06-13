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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_GIT_MR_REVIEW_H
#define DIALOG_GIT_MR_REVIEW_H

#include "dialog_git_mr_review_base.h"

#include <git/git_compare_handler.h>

#include <vector>


struct git_repository;


/**
 * Git PR-style branch comparison dialog (Phase 10).
 *
 * The user picks two refs (branches / tags / commit OIDs) and the dialog runs
 * KIGIT::CompareRefs() to produce a changed-file list. Double-clicking a
 * file opens DIALOG_KICAD_DIFF preloaded with both blobs.
 *
 * The repository pointer is borrowed from the caller (project manager) and
 * must outlive the dialog.
 */
class DIALOG_GIT_MR_REVIEW : public DIALOG_GIT_MR_REVIEW_BASE
{
public:
    DIALOG_GIT_MR_REVIEW( wxWindow* aParent, git_repository* aRepo,
                          const std::vector<wxString>& aRefList );
    ~DIALOG_GIT_MR_REVIEW() override = default;

protected:
    void OnClose( wxCloseEvent& aEvent ) override;
    void OnCompareClick( wxCommandEvent& aEvent ) override;
    void OnFileActivated( wxListEvent& aEvent ) override;

private:
    void populateFileList();
    void openFileDiff( long aListIndex );

    git_repository*                m_repo;
    std::vector<KIGIT::CHANGED_FILE> m_changedFiles;
};

#endif // DIALOG_GIT_MR_REVIEW_H
