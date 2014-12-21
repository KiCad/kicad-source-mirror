/**
 * @file dialog_hotkeys_editor.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 Kicad Developers, see CHANGELOG.TXT for contributors.
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
#include <confirm.h>

#include <dialog_hotkeys_editor.h>


HOTKEY_LIST_CTRL::HOTKEY_LIST_CTRL( wxWindow *aParent, struct EDA_HOTKEY_CONFIG* aSection ) :
    wxListCtrl( aParent, wxID_ANY, wxDefaultPosition,
                wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VIRTUAL )
{
    m_sectionTag = aSection->m_SectionTag;
    m_curEditingRow = -1;

    InsertColumn( 0, _( "Command" ) );
    InsertColumn( 1, _( "Hotkey" ) );

    // Add a dummy hotkey_spec which is a header before each hotkey list
    EDA_HOTKEY** hotkey_descr_list;

    // Add a copy of hotkeys to our list
    for( hotkey_descr_list = aSection->m_HK_InfoList; *hotkey_descr_list; hotkey_descr_list++ )
    {
        EDA_HOTKEY* hotkey_descr = *hotkey_descr_list;
        m_hotkeys.push_back( new EDA_HOTKEY( hotkey_descr ) );
    }

    // Set item count to hotkey size, this gets it to autoload the entries
    SetItemCount( m_hotkeys.size() );

    SetColumnWidth( 0, wxLIST_AUTOSIZE );
    SetColumnWidth( 1, wxLIST_AUTOSIZE );

    Bind( wxEVT_CHAR, &HOTKEY_LIST_CTRL::OnChar, this );
    Bind( wxEVT_LIST_ITEM_SELECTED, &HOTKEY_LIST_CTRL::OnListItemSelected, this );
    Bind( wxEVT_SIZE, &HOTKEY_LIST_CTRL::OnSize, this );
}


void HOTKEY_LIST_CTRL::OnSize( wxSizeEvent& aEvent )
{
    recalculateColumns();
    aEvent.Skip();
}


void HOTKEY_LIST_CTRL::recalculateColumns()
{
    float totalLength = 0;
    float scale = 0;

    // Find max character length of first column
    int maxInfoMsgLength = 0;

    for( int i = 0; i < GetItemCount(); i++ )
    {
        int length = GetItemText( i, 0 ).Length();

        if( length > maxInfoMsgLength )
            maxInfoMsgLength = length;
    }

    // Find max character length of second column
    int maxKeyCodeLength = 0;

    for( int i = 0; i < GetItemCount(); i++ )
    {
        int length = GetItemText( i, 1 ).Length();
        if( length > maxKeyCodeLength )
            maxKeyCodeLength = length;
    }

    // Use the lengths of column texts to create a scale of the max list width
    // to set the column widths
    totalLength = maxInfoMsgLength + maxKeyCodeLength;

    scale = (float) GetClientSize().x / totalLength;

    SetColumnWidth( 0, int( maxInfoMsgLength*scale ) - 2 );
    SetColumnWidth( 1, int( maxKeyCodeLength*scale ) );
}


void HOTKEY_LIST_CTRL::OnListItemSelected( wxListEvent& aEvent )
{
    m_curEditingRow = aEvent.GetIndex();
}


void HOTKEY_LIST_CTRL::DeselectRow( int aRow )
{
    SetItemState( aRow, 0, wxLIST_STATE_SELECTED );
}


wxString HOTKEY_LIST_CTRL::OnGetItemText( long aRow, long aColumn ) const
{
    EDA_HOTKEY* hotkey_descr = m_hotkeys[aRow];

    if( aColumn == 0 )
    {
        return hotkey_descr->m_InfoMsg;
    }
    else
    {
        return KeyNameFromKeyCode( hotkey_descr->m_KeyCode );
    }
}


void HOTKEY_LIST_CTRL::OnChar( wxKeyEvent& aEvent )
{
    if( m_curEditingRow != -1 )
    {
        long key = aEvent.GetKeyCode();

        switch( key )
        {
        case WXK_ESCAPE:
            // Remove selection
            DeselectRow( m_curEditingRow );
            m_curEditingRow = -1;
            break;

        default:
            if( key >= 'a' && key <= 'z' ) // convert to uppercase
                key = key + ('A' - 'a');

            // Remap Ctrl A (=1+GR_KB_CTRL) to Ctrl Z(=26+GR_KB_CTRL)
            // to GR_KB_CTRL+'A' .. GR_KB_CTRL+'Z'
            if( aEvent.ControlDown() && key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
                key += 'A' - 1;

            /* Disallow shift for keys that have two keycodes on them (e.g. number and
             * punctuation keys) leaving only the "letter keys" of A-Z.
             * Then, you can have, e.g. Ctrl-5 and Ctrl-% (GB layout)
             * and Ctrl-( and Ctrl-5 (FR layout).
             * Otherwise, you'd have to have to say Ctrl-Shift-5 on a FR layout
             */
            bool keyIsLetter = key >= 'A' && key <= 'Z';

            if( aEvent.ShiftDown() && ( keyIsLetter || key > 256 ) )
                key |= GR_KB_SHIFT;

            if( aEvent.ControlDown() )
                key |= GR_KB_CTRL;

            if( aEvent.AltDown() )
                key |= GR_KB_ALT;

            // See if this key code is handled in hotkeys names list
            bool exists;
            KeyNameFromKeyCode( key, &exists );

            if( exists && m_hotkeys[m_curEditingRow]->m_KeyCode != key )
            {
                bool canUpdate = ((HOTKEY_SECTION_PAGE *)m_parent)->GetDialog()->CanSetKey( key, m_sectionTag );

                if( canUpdate )
                {
                    m_hotkeys[m_curEditingRow]->m_KeyCode = key;
                    recalculateColumns();
                }

                // Remove selection
                DeselectRow( m_curEditingRow );
                m_curEditingRow = -1;
            }
        }
    }
    RefreshItems(0,m_hotkeys.size()-1);
}


void HOTKEY_LIST_CTRL::RestoreFrom( struct EDA_HOTKEY_CONFIG* aSection )
{
    int row = 0;

    EDA_HOTKEY** info_ptr;

    for( info_ptr = aSection->m_HK_InfoList; *info_ptr; info_ptr++ )
    {
        EDA_HOTKEY* info = *info_ptr;
        m_hotkeys[row++]->m_KeyCode = info->m_KeyCode;
    }

    // Remove selection
    DeselectRow( m_curEditingRow );
    m_curEditingRow = -1;

    RefreshItems( 0, m_hotkeys.size()-1 );
}


HOTKEY_SECTION_PAGE::HOTKEY_SECTION_PAGE( HOTKEYS_EDITOR_DIALOG* aDialog,
                                          wxNotebook*     aParent,
                                          const wxString& aTitle,
                                          EDA_HOTKEY_CONFIG* aSection ) :
    wxPanel( aParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER ),
    m_hotkeySection( aSection ),
    m_dialog( aDialog )
{
    aParent->AddPage( this, aTitle );

	wxBoxSizer* bMainSizer = new wxBoxSizer( wxVERTICAL );

	SetSizer( bMainSizer );
	Layout();
	bMainSizer->Fit( this );

	m_hotkeyList = new HOTKEY_LIST_CTRL( this, aSection );
	bMainSizer->Add( m_hotkeyList, 1, wxALL|wxEXPAND, 5 );
}


void HOTKEY_SECTION_PAGE::Restore()
{
    m_hotkeyList->RestoreFrom( m_hotkeySection );

    Update();
}


void InstallHotkeyFrame( EDA_DRAW_FRAME* aParent, EDA_HOTKEY_CONFIG* aHotkeys )
{
    HOTKEYS_EDITOR_DIALOG dialog( aParent, aHotkeys );

    int diag = dialog.ShowModal();
    if( diag == wxID_OK )
    {
        aParent->ReCreateMenuBar();
        aParent->Refresh();
    }
}


HOTKEYS_EDITOR_DIALOG::HOTKEYS_EDITOR_DIALOG( EDA_DRAW_FRAME*    aParent,
                                              EDA_HOTKEY_CONFIG* aHotkeys ) :
    HOTKEYS_EDITOR_DIALOG_BASE( aParent ),
    m_parent( aParent ),
    m_hotkeys( aHotkeys )
{
    EDA_HOTKEY_CONFIG* section;

    for( section = m_hotkeys; section->m_HK_InfoList; section++ )
    {
        m_hotkeySectionPages.push_back( new HOTKEY_SECTION_PAGE( this, m_hotkeySections,
                                                                 _( *section->m_Title ),
                                                                 section ) );
    }

    m_OKButton->SetDefault();
    Center();
}


void HOTKEYS_EDITOR_DIALOG::OnOKClicked( wxCommandEvent& event )
{
    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;

    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        std::vector<EDA_HOTKEY*>& hotkey_vec = (*i)->GetHotkeys();
        EDA_HOTKEY_CONFIG* section = (*i)->GetHotkeySection();

        EDA_HOTKEY** info_ptr;

        for( info_ptr = section->m_HK_InfoList; *info_ptr; info_ptr++ )
        {
            EDA_HOTKEY* info = *info_ptr;

            /* find the corresponding hotkey */
            std::vector<EDA_HOTKEY*>::iterator j;

            for( j = hotkey_vec.begin(); j != hotkey_vec.end(); ++j )
            {
                if( (*j) && (*j)->m_Idcommand == info->m_Idcommand )
                {
                    info->m_KeyCode = (*j)->m_KeyCode;
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


void HOTKEYS_EDITOR_DIALOG::UndoClicked( wxCommandEvent& aEvent )
{
    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;

    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        (*i)->Restore();
    }
}


bool HOTKEYS_EDITOR_DIALOG::CanSetKey( long aKey, const wxString* sectionTag )
{
    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;

    EDA_HOTKEY* conflictingKey = NULL;
    HOTKEY_SECTION_PAGE* conflictingSection = NULL;

    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        // Any non Common section can only conflict with itself and Common
        if( *sectionTag != g_CommonSectionTag
                 && *((*i)->GetHotkeySection()->m_SectionTag) != g_CommonSectionTag
                 && *((*i)->GetHotkeySection()->m_SectionTag) != *sectionTag )
            continue;

        std::vector<EDA_HOTKEY*>& hotkey_vec = (*i)->GetHotkeys();
        /* find the corresponding hotkey */
        std::vector<EDA_HOTKEY*>::iterator j;

        for( j = hotkey_vec.begin(); j != hotkey_vec.end(); ++j )
        {
            if( aKey == (*j)->m_KeyCode )
            {
                conflictingKey = (*j);
                conflictingSection = (*i);

                break;
            }
        }
    }

    if( conflictingKey != NULL )
    {
        wxString msg = wxString::Format(
            _( "<%s> is already assigned to \"%s\" in section \"%s\". Are you sure you want "
               "to change its assignment?" ),
            KeyNameFromKeyCode( aKey ), conflictingKey->m_InfoMsg,
            *(conflictingSection->GetHotkeySection()->m_Title) );

        wxMessageDialog dlg( this, msg, _( "Confirm change" ), wxYES_NO | wxNO_DEFAULT );

        if( dlg.ShowModal() == wxID_YES )
        {
            conflictingKey->m_KeyCode = 0;
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}
