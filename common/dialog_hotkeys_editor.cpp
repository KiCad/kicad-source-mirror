#include <wx/tooltip.h>
#include <algorithm>

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "dialog_hotkeys_editor.h"

void InstallHotkeyFrame( WinEDA_DrawFrame*               parent,
                         Ki_HotkeyInfoSectionDescriptor* hotkeys )
{
    HOTKEYS_EDITOR_DIALOG dialog( parent, hotkeys );

    int diag = dialog.ShowModal();
    if( diag == wxID_OK )
    {
        parent->ReCreateMenuBar();
        parent->Refresh();
    }
}


HOTKEYS_EDITOR_DIALOG::HOTKEYS_EDITOR_DIALOG( WinEDA_DrawFrame*               parent,
                                              Ki_HotkeyInfoSectionDescriptor* hotkeys ) :
    HOTKEYS_EDITOR_DIALOG_BASE( parent )
{
    m_parent  = parent;
    m_hotkeys = hotkeys;

    m_table = new HotkeyGridTable( hotkeys );
    m_hotkeyGrid->SetTable( m_table, true );

    m_hotkeyGrid->AutoSizeColumn( 0 );
    m_hotkeyGrid->EnableDragGridSize( false );

    for( int i = 0; i < m_hotkeyGrid->GetNumberRows(); ++i )
    {
        m_hotkeyGrid->SetReadOnly( i, 0, true );
        m_hotkeyGrid->SetReadOnly( i, 1, true );
    }

    SetFocus();
    GetSizer()->SetSizeHints( this );
    Center();
}


void HOTKEYS_EDITOR_DIALOG::OnOKClicked( wxCommandEvent& event )
{
    /* edit the live hotkey table */
    HotkeyGridTable::hotkey_spec_vector& hotkey_vec = m_table->getHotkeys();

    Ki_HotkeyInfoSectionDescriptor*      section;

    for( section = m_hotkeys; section->m_HK_InfoList; section++ )
    {
        wxString        sectionTag = *section->m_SectionTag;

        Ki_HotkeyInfo** info_ptr;
        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            Ki_HotkeyInfo* info = *info_ptr;
            /* find the corresponding hotkey */
            HotkeyGridTable::hotkey_spec_vector::iterator i;
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

    Close( TRUE );
}


void HOTKEYS_EDITOR_DIALOG::CancelClicked( wxCommandEvent& event )
{
    Close( TRUE );
}


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


void HOTKEYS_EDITOR_DIALOG::StartEditing( wxGridEvent& event )
{
    if( m_curEditingRow != -1 )
        SetHotkeyCellState( m_curEditingRow, false );

    int newRow = event.GetRow();
    if( m_curEditingRow == newRow || m_table->isHeader( newRow ) )
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


void HOTKEYS_EDITOR_DIALOG::KeyPressed( wxKeyEvent& event )
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

            if( key >= 'a' && key <= 'z' ) //upcase key
                key = key + ('A' - 'a');

#if 0       // For debug only
            wxString msg;
            msg.Printf(wxT("key %X, keycode %X"),event.GetKeyCode(), key);
            wxMessageBox(msg);
#endif
            // See if this key code is handled in hotkeys list
            bool exists;
            ReturnKeyNameFromKeyCode( key, &exists );
            if( !exists )   // not handled, see hotkeys_basic.cpp
                wxMessageBox( _("Hotkey code not handled" ) );
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
