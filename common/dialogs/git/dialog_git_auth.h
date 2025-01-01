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

#ifndef GIT_REPOSITORY_DIALOG_H
#define GIT_REPOSITORY_DIALOG_H

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filepicker.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <dialog_shim.h>

class DIALOG_GIT_AUTH : public DIALOG_SHIM
{
public:
    DIALOG_GIT_AUTH( wxWindow* parent );
    virtual ~DIALOG_GIT_AUTH();

private:
    void CreateControls();
    void onAuthChoiceChanged( wxCommandEvent& event );
    void onTestClick( wxCommandEvent& aEvent );
    void onOKClick( wxCommandEvent& aEvent );
    void onCancelClick( wxCommandEvent& aEvent );

    wxTextCtrl*       m_NameTextCtrl;
    wxTextCtrl*       m_UrlTextCtrl;
    wxChoice*         m_AuthChoice;
    wxTextCtrl*       m_UserNameTextCtrl;
    wxTextCtrl*       m_PasswordTextCtrl;
    wxButton*         m_TestButton;
    wxButton*         m_OkButton;
    wxButton*         m_CancelButton;
    wxFilePickerCtrl* m_PublicKeyPicker;
};

#endif // GIT_REPOSITORY_DIALOG_H
