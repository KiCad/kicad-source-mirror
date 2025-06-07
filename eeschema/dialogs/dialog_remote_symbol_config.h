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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_REMOTE_SYMBOL_CONFIG_H
#define DIALOG_REMOTE_SYMBOL_CONFIG_H

#include <dialog_shim.h>
#include <eeschema_settings.h>

class wxTextCtrl;
class wxStaticText;
class wxRadioButton;
class wxButton;

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

    void applyRemoteSettings( const EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG& aConfig );
    void updatePrefixHint();

private:
    wxTextCtrl*    m_destinationCtrl;
    wxTextCtrl*    m_prefixCtrl;
    wxStaticText*  m_prefixHint;
    wxRadioButton* m_projectRadio;
    wxRadioButton* m_globalRadio;
    wxButton*      m_resetButton;
    wxButton*      m_browseButton;

    EESCHEMA_SETTINGS* m_settings;
};


#endif // DIALOG_REMOTE_SYMBOL_CONFIG_H
