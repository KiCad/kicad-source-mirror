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

#include "dialog_git_auth.h"

DIALOG_GIT_AUTH::DIALOG_GIT_AUTH(wxWindow* parent)
    : DIALOG_SHIM(parent, wxID_ANY, _("Connection"), wxDefaultPosition, wxSize(400, 300))
{
    CreateControls();
    Centre();
}

DIALOG_GIT_AUTH::~DIALOG_GIT_AUTH()
{
}

void DIALOG_GIT_AUTH::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(mainSizer);

    m_NameTextCtrl = new wxTextCtrl(this, wxID_ANY);
    m_UrlTextCtrl = new wxTextCtrl(this, wxID_ANY);
    m_AuthChoice = new wxChoice(this, wxID_ANY);
    m_AuthChoice->Append(_("Basic"));
    m_AuthChoice->Append(_("SSH"));
    m_UserNameTextCtrl = new wxTextCtrl(this, wxID_ANY);
    m_PasswordTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_PublicKeyPicker = new wxFilePickerCtrl(this, wxID_ANY);
    m_PublicKeyPicker->Hide();

    m_TestButton = new wxButton(this, wxID_ANY, _("Test"));
    m_OkButton = new wxButton(this, wxID_OK, _("OK"));
    m_CancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));

    mainSizer->Add(new wxStaticText(this, wxID_ANY, _("Name")), 0, wxALL, 10);
    mainSizer->Add(m_NameTextCtrl, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(new wxStaticText(this, wxID_ANY, _("Url")), 0, wxALL, 10);
    mainSizer->Add(m_UrlTextCtrl, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(new wxStaticText(this, wxID_ANY, _("Authentication")), 0, wxALL, 10);
    mainSizer->Add(m_AuthChoice, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(new wxStaticText(this, wxID_ANY, _("User Name")), 0, wxALL, 10);
    mainSizer->Add(m_UserNameTextCtrl, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(new wxStaticText(this, wxID_ANY, _("Password")), 0, wxALL, 10);
    mainSizer->Add(m_PasswordTextCtrl, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_PublicKeyPicker, 0, wxEXPAND | wxALL, 10);


    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_TestButton, 1, wxALL, 10);
    buttonSizer->Add(m_OkButton, 1, wxALL, 10);
    buttonSizer->Add(m_CancelButton, 1, wxALL, 10);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT);

    mainSizer->Layout();

    // Bind event for authentication method choice change
    m_AuthChoice->Bind(wxEVT_CHOICE, &DIALOG_GIT_AUTH::onAuthChoiceChanged, this);
    m_TestButton->Bind(wxEVT_BUTTON, &DIALOG_GIT_AUTH::onTestClick, this );
}

void DIALOG_GIT_AUTH::onAuthChoiceChanged(wxCommandEvent& event)
{
    if (m_AuthChoice->GetStringSelection() == "SSH")
    {
        m_PasswordTextCtrl->Hide();
        m_PublicKeyPicker->Show();
    }
    else
    {
        m_PasswordTextCtrl->Show();
        m_PublicKeyPicker->Hide();
    }

    Layout();  // Re-arrange the controls
}
