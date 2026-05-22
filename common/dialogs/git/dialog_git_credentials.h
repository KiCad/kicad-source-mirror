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

#ifndef DIALOG_GIT_CREDENTIALS_H
#define DIALOG_GIT_CREDENTIALS_H

#include <dialogs/git/dialog_git_credentials_base.h>
#include <git/kicad_git_common.h>

class DIALOG_GIT_CREDENTIALS : public DIALOG_GIT_CREDENTIALS_BASE
{
public:
    DIALOG_GIT_CREDENTIALS( wxWindow* aParent, const wxString& aUrl, KIGIT_COMMON::GIT_CONN_TYPE aConnType,
                            const wxString& aDefaultUsername, const wxString& aDefaultSSHKey );

    wxString                    GetUsername() const;
    wxString                    GetPassword() const;
    wxString                    GetSSHKey() const;
    KIGIT_COMMON::GIT_CONN_TYPE GetConnType() const;
    bool                        SaveCredentials() const;

protected:
    void OnConnTypeChanged( wxCommandEvent& aEvent ) override;

private:
    void updateFieldsForConnType();
};

#endif // DIALOG_GIT_CREDENTIALS_H
