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

#ifndef DIALOG_GIT_REMOTE_H
#define DIALOG_GIT_REMOTE_H

#include <dialogs/git/dialog_git_remote_base.h>


/// Ask for a remote's name and URL, for adding one or for editing the URL of one that exists.
class DIALOG_GIT_REMOTE : public DIALOG_GIT_REMOTE_BASE
{
public:
    /// With aName set the remote exists already, so only its URL can be changed.
    DIALOG_GIT_REMOTE( wxWindow* aParent, const wxString& aName = wxEmptyString, const wxString& aUrl = wxEmptyString );

    wxString GetRemoteName() const { return m_name; }
    wxString GetUrl() const { return m_url; }

private:
    bool TransferDataFromWindow() override;

    wxString m_name;
    wxString m_url;
};

#endif // DIALOG_GIT_REMOTE_H
