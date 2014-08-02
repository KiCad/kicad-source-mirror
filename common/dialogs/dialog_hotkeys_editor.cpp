/**
 * @file dialog_hotkeys_editor.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include <algorithm>

#include <fctsys.h>
#include <pgm_base.h>
#include <common.h>

#include <dialog_hotkeys_editor.h>

void InstallHotkeyFrame( EDA_DRAW_FRAME* parent, EDA_HOTKEY_CONFIG* hotkeys )
{
    HOTKEYS_EDITOR_DIALOG dialog( parent, hotkeys );

    int diag = dialog.ShowModal();
    if( diag == wxID_OK )
    {
        parent->ReCreateMenuBar();
        parent->Refresh();
    }
}


HOTKEYS_EDITOR_DIALOG::HOTKEYS_EDITOR_DIALOG( EDA_DRAW_FRAME*    parent,
                                              EDA_HOTKEY_CONFIG* hotkeys ) :
    HOTKEYS_EDITOR_DIALOG_BASE( parent )
{
    m_parent  = parent;
    m_hotkeys = hotkeys;
    m_curEditingRow = -1;

    m_table = new HOTKEY_EDITOR_GRID_TABLE( hotkeys );
    m_hotkeyGrid->SetTable( m_table, true );

    m_hotkeyGrid->AutoSizeColumn( 0 );
    m_hotkeyGrid->EnableDragGridSize( false );

    for( int i = 0; i < m_hotkeyGrid->GetNumberRows(); ++i )
    {
        m_hotkeyGrid->SetReadOnly( i, 0, true );
        m_hotkeyGrid->SetReadOnly( i, 1, true );
    }

    m_OKButton->SetDefault();
    m_hotkeyGrid->SetFocus();
    GetSizer()->SetSizeHints( this );
    Center();
}


void HOTKEYS_EDITOR_DIALOG::OnOKClicked( wxCommandEvent& event )
{
    /* edit the live hotkey table */
    HOTKEY_EDITOR_GRID_TABLE::hotkey_spec_vector& hotkey_vec = m_table->getHotkeys();

    EDA_HOTKEY_CONFIG*      section;

    for( section = m_hotkeys; section->m_HK_InfoList; section++ )
    {
        wxString     sectionTag = *section->m_SectionTag;

        EDA_HOTKEY** info_ptr;

        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            EDA_HOTKEY* info = *info_ptr;

            /* find the corresponding hotkey */
            HOTKEY_EDITOR_GRID_TABLE::hotkey_spec_vector::iterator i;

            for( i = hotkey_vec.begin(); i != hotkey_vec.end(); ++i )
            {
                if( i->first == sectionTag
                    && i->second
                    && i->second->m_Idcommand == info->m_Idcommand )
                {
                    info->m_KeyCode = i->second->m_KeyCode;
                    break;
                }
            }
        }
    }

    /* save the hotkeys */
    m_parent->WriteHotkeyConfig( m_hotkeys );

    EndModal( wxID_OK );
}


void HOTKEYS_EDITOR_DIALOG::CancelClicked( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


/* Reinit the hotkeys to the initial state (remove all pending changes
 */
void HOTKEYS_EDITOR_DIALOG::UndoClicked( wxCommandEvent& event )
{
    m_table->RestoreFrom( m_hotkeys );
    m_curEditingRow = -1;

    for( int i = 0; i < m_hotkeyGrid->GetNumberRows(); ++i )
        SetHotkeyCellState( i, false );

    m_hotkeyGrid->Refresh();
    Update();
}


void HOTKEYS_EDITOR_DIALOG::SetHotkeyCellState( int aRow, bool aHightlight )
{
    if( aHightlight )
    {
        m_hotkeyGrid->SetCellTextColour( aRow, 1, *wxRED );
        wxFont bold_font(m_hotkeyGrid->GetDefaultCellFont() );
        bold_font.SetWeight(wxFONTWEIGHT_BOLD);
        m_hotkeyGrid->SetCellFont( aRow, 1, bold_font );
    }
    else
    {
        m_hotkeyGrid->SetCellTextColour( aRow, 1, m_hotkeyGrid->GetDefaultCellTextColour() );
        m_hotkeyGrid->SetCellFont( aRow, 1, m_hotkeyGrid->GetDefaultCellFont() );
    }
}


void HOTKEYS_EDITOR_DIALOG::OnClickOnCell( wxGridEvent& event )
{
    if( m_curEditingRow != -1 )
        SetHotkeyCellState( m_curEditingRow, false );

    int newRow = event.GetRow();

    if( ( event.GetCol() != 1 ) || ( m_table->IsHeader( newRow ) ) )
    {
        m_curEditingRow = -1;
    }
    else
    {
        m_curEditingRow = newRow;
        SetHotkeyCellState( m_curEditingRow, true );
    }
    m_hotkeyGrid->Refresh();
    Update();
}


/** OnRightClickOnCell
 * If a cell is selected, display a list of keys for selection
 * The list is restricted to keys that cannot be entered:
 * tab, home, return ... because these keys have special functions in dialogs
 */
void HOTKEYS_EDITOR_DIALOG::OnRightClickOnCell( wxGridEvent& event )
{
    // Select the new cell if needed
    OnClickOnCell(event);

    if( m_curEditingRow == -1 )
        return;

    // Do not translate these key names. They are internally used.
    // See hotkeys_basic.cpp
    #define C_COUNT 9
    wxString choices[C_COUNT] =
    {
        wxT("End")
        wxT("Tab"),
        wxT("Ctrl+Tab"),
        wxT("Alt+Tab"),
        wxT("Home"),
        wxT("Space"),
        wxT("Ctrl+Space"),
        wxT("Alt+Space"),
        wxT("Return")
    };

    wxString keyname = wxGetSingleChoice( _( "Special keys only. For others keys, use keyboard" ),
                                          _( "Select a key" ), C_COUNT, choices, this );
    int key = KeyCodeFromKeyName( keyname );

    if( key == 0 )
        return;

    m_table->SetKeyCode( m_curEditingRow, key );
    m_hotkeyGrid->Refresh();
    Update();
}


void HOTKEYS_EDITOR_DIALOG::OnKeyPressed( wxKeyEvent& event )
{
    if( m_curEditingRow != -1 )
    {
        long key = event.GetKeyCode();

        switch( key )
        {
        case WXK_ESCAPE:
            SetHotkeyCellState( m_curEditingRow, false );
            m_curEditingRow = -1;
            break;

        default:
            if( event.ControlDown() )
                key |= GR_KB_CTRL;

            if( event.AltDown() )
                key |= GR_KB_ALT;

            if( event.ShiftDown() && (key > 256) )
                key |= GR_KB_SHIFT;

            // Remap Ctrl A (=1+GR_KB_CTRL) to Ctrl Z(=26+GR_KB_CTRL)
            // to GR_KB_CTRL+'A' .. GR_KB_CTRL+'Z'
            if( (key > GR_KB_CTRL) && (key <= GR_KB_CTRL+26) )
                key += ('A' - 1);

            if( key >= 'a' && key <= 'z' ) // convert to uppercase
                key = key + ('A' - 'a');

#if 0       // For debug only
            wxString msg;
            msg.Printf(wxT("key %X, keycode %X"),event.GetKeyCode(), key);
            wxMessageBox(msg);
#endif
            // See if this key code is handled in hotkeys names list
            bool exists;
            KeyNameFromKeyCode( key, &exists );

            if( !exists )   // not handled, see hotkeys_basic.cpp
            {
                wxMessageBox( _( "Hotkey code not handled" ) );
            }
            else
            {
                m_table->SetKeyCode( m_curEditingRow, key );
            }

            break;
        }
    }

    m_hotkeyGrid->Refresh();
    Update();
}
