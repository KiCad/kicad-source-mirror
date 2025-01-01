/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef DIALOG_GIT_COMMIT_H
#define DIALOG_GIT_COMMIT_H

#include <dialog_shim.h>
#include <git2.h>
#include <vector>

class wxCheckBox;
class wxTextCtrl;
class wxListCtrl;
class wxButton;

class DIALOG_GIT_COMMIT : public DIALOG_SHIM
{
public:
    DIALOG_GIT_COMMIT( wxWindow* parent, git_repository* repo,
                       const wxString&              defaultAuthorName,
                       const wxString&              defaultAuthorEmail,
                       const std::map<wxString, int>& filesToCommit );

    wxString GetCommitMessage() const;

    wxString GetAuthorName() const;

    wxString GetAuthorEmail() const;

    std::vector<wxString> GetSelectedFiles() const;

    void OnTextChanged( wxCommandEvent& event );

private:
    wxTextCtrl* m_commitMessageTextCtrl;
    wxTextCtrl* m_authorTextCtrl;
    wxListCtrl* m_listCtrl;
    wxButton* m_okButton;

    git_repository* m_repo;
    wxString m_defaultAuthorName;
    wxString m_defaultAuthorEmail;
    std::vector<wxString> m_filesToCommit;
};

#endif // DIALOG_GIT_COMMIT_H