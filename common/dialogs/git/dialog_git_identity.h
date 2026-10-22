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

#ifndef DIALOG_GIT_IDENTITY_H
#define DIALOG_GIT_IDENTITY_H

#include <dialogs/git/dialog_git_identity_base.h>


/// Ask for the name and email to record commits with.
class DIALOG_GIT_IDENTITY : public DIALOG_GIT_IDENTITY_BASE
{
public:
    DIALOG_GIT_IDENTITY( wxWindow* aParent, const wxString& aTitle, const wxString& aName, const wxString& aEmail );

    wxString GetAuthorName() const { return m_name; }
    wxString GetEmail() const { return m_email; }

private:
    bool TransferDataFromWindow() override;

    wxString m_name;
    wxString m_email;
};

#endif // DIALOG_GIT_IDENTITY_H
