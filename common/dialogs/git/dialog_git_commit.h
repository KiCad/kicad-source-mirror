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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_GIT_COMMIT_H
#define DIALOG_GIT_COMMIT_H

#include <dialog_shim.h>
#include <git2.h>
#include <vector>

class wxCheckBox;
class wxTextCtrl;
class wxListCtrl;
class wxListEvent;
class wxButton;

class DIALOG_GIT_COMMIT : public DIALOG_SHIM
{
public:
    DIALOG_GIT_COMMIT( wxWindow* parent, git_repository* repo,
                       const wxString&              defaultAuthorName,
                       const wxString&              defaultAuthorEmail,
                       const std::map<wxString, int>& filesToCommit );
    ~DIALOG_GIT_COMMIT() override;

    wxString GetCommitMessage() const;

    /// Pre-fill the commit message.
    void SetCommitMessage( const wxString& aMessage );

    wxString GetAuthorName() const;

    wxString GetAuthorEmail() const;

    std::vector<wxString> GetSelectedFiles() const;

    /// When false, OK is allowed with no files selected (e.g. amending only the message).
    void SetFileSelectionRequired( bool aRequired );

    void OnTextChanged( wxCommandEvent& event );
    void OnItemChecked( wxListEvent& event );
    void OnItemUnchecked( wxListEvent& event );

private:
    /// Enable the OK button only when there is both a message and at least one selected file.
    void updateOkButton();
    void loadFileListColumnWidths();
    void saveFileListColumnWidths();

    wxTextCtrl* m_commitMessageTextCtrl;
    wxTextCtrl* m_authorTextCtrl;
    wxListCtrl* m_listCtrl;
    wxButton* m_okButton;

    bool m_requireFiles = true;

    git_repository* m_repo;
    wxString m_defaultAuthorName;
    wxString m_defaultAuthorEmail;
    std::vector<wxString> m_filesToCommit;
};

#endif // DIALOG_GIT_COMMIT_H