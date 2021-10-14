/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_hotkey_list.h>

#include <panel_hotkeys_editor.h>
#include <widgets/ui_common.h>

#include <wx/sizer.h>
#include <wx/button.h>


DIALOG_LIST_HOTKEYS::DIALOG_LIST_HOTKEYS( EDA_BASE_FRAME* aParent, TOOL_MANAGER* aToolMgr ):
    DIALOG_SHIM( aParent, wxID_ANY, _( "Hotkey List" ) )
{
    const int   margin = KIUI::GetStdMargin();
    wxBoxSizer* main_sizer = new wxBoxSizer( wxVERTICAL );

    m_hk_list = new PANEL_HOTKEYS_EDITOR( aParent, this, true );
    m_hk_list->AddHotKeys( aToolMgr );

    main_sizer->Add( m_hk_list, 1, wxTOP | wxLEFT | wxRIGHT | wxEXPAND, margin );

    wxStdDialogButtonSizer* sdb_sizer = new wxStdDialogButtonSizer;
    sdb_sizer->AddButton( new wxButton( this, wxID_OK ) );
    sdb_sizer->Realize();

    main_sizer->Add( sdb_sizer, 0, wxEXPAND | wxALL, margin );

    SetSizer( main_sizer );

    finishDialogSettings();
}


bool DIALOG_LIST_HOTKEYS::TransferDataToWindow()
{
    return m_hk_list->TransferDataToWindow();
}
