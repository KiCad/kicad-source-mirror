/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <eda_base_frame.h>
#include <panel_hotkeys_editor.h>

PANEL_HOTKEYS_EDITOR::PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow,
                                            EDA_HOTKEY_CONFIG* aHotkeys,
                                            EDA_HOTKEY_CONFIG* aShowHotkeys,
                                            const wxString& aNickname ) :
        PANEL_HOTKEYS_EDITOR_BASE( aWindow ),
        m_frame( aFrame ),
        m_hotkeys( aHotkeys ),
        m_showHotkeys( aShowHotkeys ),
        m_nickname( aNickname ),
        m_hotkeyStore( aShowHotkeys )
{
    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( m_panelHotkeys, m_hotkeyStore );
    m_hotkeyListCtrl->InstallOnPanel( m_panelHotkeys );
}


bool PANEL_HOTKEYS_EDITOR::TransferDataToWindow()
{
    return m_hotkeyListCtrl->TransferDataToControl();
}


bool PANEL_HOTKEYS_EDITOR::TransferDataFromWindow()
{
    if( !m_hotkeyListCtrl->TransferDataFromControl() )
        return false;

    // save the hotkeys
    m_frame->WriteHotkeyConfig( m_hotkeys );

    return true;
}


void PANEL_HOTKEYS_EDITOR::ResetClicked( wxCommandEvent& aEvent )
{
    m_hotkeyListCtrl->ResetAllHotkeys( false );
}

void PANEL_HOTKEYS_EDITOR::DefaultsClicked( wxCommandEvent& aEvent )
{
    m_hotkeyListCtrl->ResetAllHotkeys( true );
}


void PANEL_HOTKEYS_EDITOR::OnExport( wxCommandEvent& aEvent )
{
    m_frame->ExportHotkeyConfigToFile( m_hotkeys, m_nickname );
}


void PANEL_HOTKEYS_EDITOR::OnImport( wxCommandEvent& aEvent )
{
    m_frame->ImportHotkeyConfigFromFile( m_hotkeys, m_nickname );
}
