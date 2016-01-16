/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 Kicad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#include <dialog_hotkeys_editor.h>

void InstallHotkeyFrame( EDA_BASE_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys )
{
    HOTKEYS_EDITOR_DIALOG dialog( aParent, aHotkeys );

    int diag = dialog.ShowModal();

    if( diag == wxID_OK )
    {
        aParent->ReCreateMenuBar();
        aParent->Refresh();
    }
}


HOTKEYS_EDITOR_DIALOG::HOTKEYS_EDITOR_DIALOG( EDA_BASE_FRAME* aParent,
        EDA_HOTKEY_CONFIG* aHotkeys ) :
    HOTKEYS_EDITOR_DIALOG_BASE( aParent ),
    m_hotkeys( aHotkeys )
{
    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( m_panelHotkeys,
            WIDGET_HOTKEY_LIST::GenSections( aHotkeys ) );
    m_hotkeyListCtrl->InstallOnPanel( m_panelHotkeys );

    m_sdbSizerOK->SetDefault();

    Layout();
    Center();
}


bool HOTKEYS_EDITOR_DIALOG::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_hotkeyListCtrl->TransferDataToControl() )
        return false;

    return true;
}


bool HOTKEYS_EDITOR_DIALOG::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_hotkeyListCtrl->TransferDataFromControl() )
        return false;

    // save the hotkeys
    GetParent()->WriteHotkeyConfig( m_hotkeys );

    return true;
}


void HOTKEYS_EDITOR_DIALOG::ResetClicked( wxCommandEvent& aEvent )
{
    m_hotkeyListCtrl->TransferDataToControl();
}
