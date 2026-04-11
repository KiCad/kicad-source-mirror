/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_REMOTE_SYMBOL_CONFIG_H
#define DIALOG_REMOTE_SYMBOL_CONFIG_H

#include <dialog_shim.h>
#include <eeschema_settings.h>

class wxButton;
class wxListBox;
class wxRadioButton;
class wxStaticText;
class wxTextCtrl;

class EESCHEMA_SETTINGS;

class DIALOG_REMOTE_SYMBOL_CONFIG : public DIALOG_SHIM
{
public:
    explicit DIALOG_REMOTE_SYMBOL_CONFIG( wxWindow* aParent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void onBrowseDestination( wxCommandEvent& aEvent );
    void onResetDefaults( wxCommandEvent& aEvent );
    void onPrefixChanged( wxCommandEvent& aEvent );
    void onProviderSelected( wxCommandEvent& aEvent );
    void onAddProvider( wxCommandEvent& aEvent );
    void onRemoveProvider( wxCommandEvent& aEvent );
    void onRefreshProvider( wxCommandEvent& aEvent );
    void onSignOutProvider( wxCommandEvent& aEvent );

    void applyRemoteSettings( const REMOTE_PROVIDER_SETTINGS& aConfig );
    void updatePrefixHint();
    void reloadProviderList();
    void updateProviderEditor();
    void updateProviderButtons();
    void commitProviderEdits();

private:
    wxListBox*     m_providerList;
    wxTextCtrl*    m_providerUrlCtrl;
    wxTextCtrl*    m_providerNameCtrl;
    wxStaticText*  m_providerAccountLabel;
    wxStaticText*  m_providerAuthLabel;
    wxButton*      m_addProviderButton;
    wxButton*      m_removeProviderButton;
    wxButton*      m_refreshProviderButton;
    wxButton*      m_signOutProviderButton;
    wxTextCtrl*    m_destinationCtrl;
    wxTextCtrl*    m_prefixCtrl;
    wxStaticText*  m_prefixHint;
    wxRadioButton* m_projectRadio;
    wxRadioButton* m_globalRadio;
    wxButton*      m_resetButton;
    wxButton*      m_browseButton;

    EESCHEMA_SETTINGS*     m_settings;
    REMOTE_PROVIDER_SETTINGS m_remoteSettings;
};

#endif // DIALOG_REMOTE_SYMBOL_CONFIG_H
