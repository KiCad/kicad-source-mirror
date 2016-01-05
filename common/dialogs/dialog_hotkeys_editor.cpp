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

#include <algorithm>

#include <fctsys.h>
#include <pgm_base.h>
#include <common.h>
#include <confirm.h>

#include <wx/dataview.h>

#include <dialog_hotkeys_editor.h>


class DIALOG_HOTKEY_CLIENT_DATA : public wxClientData
{
    EDA_HOTKEY m_hotkey;
    wxString m_section_tag;

public:
    DIALOG_HOTKEY_CLIENT_DATA( const EDA_HOTKEY& aHotkey, const wxString& aSectionTag )
        : m_hotkey( aHotkey ), m_section_tag( aSectionTag ) {}

    EDA_HOTKEY& GetHotkey() { return m_hotkey; }
    wxString GetSectionTag() const { return m_section_tag; }
};


HOTKEY_LIST_CTRL::HOTKEY_LIST_CTRL( wxWindow *aParent, const HOTKEYS_SECTIONS& aSections ) :
    wxTreeListCtrl( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE ),
    m_sections( aSections )
{
    AppendColumn( _( "Command" ) );
    AppendColumn( _( "Hotkey" ) );

    SetColumnWidth( 1, 100 );

    Bind( wxEVT_CHAR, &HOTKEY_LIST_CTRL::OnChar, this );
    Bind( wxEVT_SIZE, &HOTKEY_LIST_CTRL::OnSize, this );
}


void HOTKEY_LIST_CTRL::OnSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
}


void HOTKEY_LIST_CTRL::DeselectRow( int aRow )
{
    wxASSERT( aRow >= 0 && aRow < m_items.size() );
    Unselect( m_items[aRow] );
}


void HOTKEY_LIST_CTRL::OnChar( wxKeyEvent& aEvent )
{
    DIALOG_HOTKEY_CLIENT_DATA* data = GetSelHKClientData();

    if( data )
    {
        long key = aEvent.GetKeyCode();

        switch( key )
        {
        case WXK_ESCAPE:
            UnselectAll();
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

            if( exists && data->GetHotkey().m_KeyCode != key )
            {
                wxString tag = data->GetSectionTag();
                HOTKEY_SECTION_PAGE* parent = static_cast<HOTKEY_SECTION_PAGE*>( m_parent );
                bool canUpdate = parent->GetDialog()->CanSetKey( key, tag );

                if( canUpdate )
                {
                    data->GetHotkey().m_KeyCode = key;
                }

                // Remove selection
                UnselectAll();
            }
        }
    }
    UpdateFromClientData();
}


DIALOG_HOTKEY_CLIENT_DATA* HOTKEY_LIST_CTRL::GetSelHKClientData()
{
    return GetHKClientData( GetSelection() );
}


DIALOG_HOTKEY_CLIENT_DATA* HOTKEY_LIST_CTRL::GetHKClientData( wxTreeListItem aItem )
{
    if( aItem.IsOk() )
    {
        wxClientData* data = GetItemData( aItem );
        if( !data )
            return NULL;

        DIALOG_HOTKEY_CLIENT_DATA* hkdata = static_cast<DIALOG_HOTKEY_CLIENT_DATA*>( data );
        return hkdata;
    }
    else
    {
        return NULL;
    }
}


void HOTKEY_LIST_CTRL::LoadSection( struct EDA_HOTKEY_CONFIG* aSection )
{
    HOTKEY_LIST list;
    EDA_HOTKEY** info_ptr;

    for( info_ptr = aSection->m_HK_InfoList; *info_ptr; info_ptr++ )
    {
        EDA_HOTKEY info = **info_ptr;
        list.push_back( info );
    }

    m_hotkeys.push_back( list );
}


void HOTKEY_LIST_CTRL::UpdateFromClientData()
{
    for( wxTreeListItem i = GetFirstItem(); i.IsOk(); i = GetNextItem( i ) )
    {
        DIALOG_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( i );
        if( !hkdata )
            continue;

        EDA_HOTKEY& hk = hkdata->GetHotkey();

        wxString name = wxGetTranslation( hk.m_InfoMsg );
        wxString key = KeyNameFromKeyCode( hk.m_KeyCode );

        SetItemText( i, 0, name );
        SetItemText( i, 1, key );
    }
}


bool HOTKEY_LIST_CTRL::TransferDataToControl()
{
    Freeze();
    DeleteAllItems();
    m_items.clear();
    m_hotkeys.clear();

    for( size_t i_list = 0; i_list < m_sections.size(); ++i_list )
    {
        LoadSection( m_sections[i_list].second );
        wxString section_tag = *( m_sections[i_list].second->m_SectionTag );

        HOTKEY_LIST& each_list = m_hotkeys[i_list];
        for( size_t i_hotkey = 0; i_hotkey < each_list.size(); ++i_hotkey )
        {
            EDA_HOTKEY* hotkey_descr = &each_list[i_hotkey];

            wxTreeListItem item = AppendItem( GetRootItem(), wxEmptyString );
            SetItemData( item, new DIALOG_HOTKEY_CLIENT_DATA( hotkey_descr, section_tag ) );
            m_items.push_back( item );
        }
    }

    UpdateFromClientData();
    Thaw();

    return true;
}


bool HOTKEY_LIST_CTRL::TransferDataFromControl()
{
    for( size_t i_sec = 0; i_sec < m_sections.size(); ++i_sec )
    {
        struct EDA_HOTKEY_CONFIG* section = m_sections[i_sec].second;
        for( EDA_HOTKEY** info_ptr = section->m_HK_InfoList; *info_ptr; ++info_ptr )
        {
            EDA_HOTKEY* info = *info_ptr;
            for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
            {
                DIALOG_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( item );
                if( !hkdata )
                    continue;

                EDA_HOTKEY& hk = hkdata->GetHotkey();
                if( hk.m_Idcommand == info->m_Idcommand )
                {
                    info->m_KeyCode = hk.m_KeyCode;
                    break;
                }
            }
        }
    }
    return true;
}


bool HOTKEY_LIST_CTRL::CanSetKey( long aKey, const wxString& aSectionTag,
        EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect )
{
    EDA_HOTKEY* conflictingKey = NULL;
    struct EDA_HOTKEY_CONFIG* conflictingSection = NULL;

    for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
    {
        DIALOG_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( item );
        if( !hkdata )
            continue;

        EDA_HOTKEY& hk = hkdata->GetHotkey();
        wxString tag = hkdata->GetSectionTag();

        if( aSectionTag != g_CommonSectionTag
                && tag != g_CommonSectionTag
                && tag != aSectionTag )
            continue;

        if( aKey == hk.m_KeyCode )
        {
            conflictingKey = &hk;

            // Find the section
            HOTKEYS_SECTIONS::iterator it;
            for( it = m_sections.begin(); it != m_sections.end(); ++it )
            {
                if( *it->second->m_SectionTag == tag )
                {
                    conflictingSection = it->second;
                    break;
                }
            }
        }
    }

    if( aConfKey )
        *aConfKey = conflictingKey;

    if( aConfSect )
        *aConfSect = conflictingSection;

    return conflictingKey == NULL;
}


HOTKEY_SECTION_PAGE::HOTKEY_SECTION_PAGE( HOTKEYS_EDITOR_DIALOG* aDialog,
                                          wxNotebook*     aParent,
                                          const wxString& aTitle,
                                          EDA_HOTKEY_CONFIG* aSection ) :
    wxPanel( aParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER ),
    m_dialog( aDialog )
{
    aParent->AddPage( this, aTitle );

	wxBoxSizer* bMainSizer = new wxBoxSizer( wxVERTICAL );

	SetSizer( bMainSizer );

    HOTKEYS_SECTION section( aTitle, aSection );
    HOTKEYS_SECTIONS sections;
    sections.push_back( section );

	m_hotkeyList = new HOTKEY_LIST_CTRL( this, sections );
	bMainSizer->Add( m_hotkeyList, 1, wxALL|wxEXPAND, 5 );

	Layout();
	bMainSizer->Fit( this );
}


void HOTKEY_SECTION_PAGE::Restore()
{
    m_hotkeyList->TransferDataToControl();

    Update();
}


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


HOTKEYS_EDITOR_DIALOG::HOTKEYS_EDITOR_DIALOG( EDA_BASE_FRAME*    aParent,
                                              EDA_HOTKEY_CONFIG* aHotkeys ) :
    HOTKEYS_EDITOR_DIALOG_BASE( aParent ),
    m_parent( aParent ),
    m_hotkeys( aHotkeys )
{
    EDA_HOTKEY_CONFIG* section;

    for( section = m_hotkeys; section->m_HK_InfoList; section++ )
    {
        m_hotkeySectionPages.push_back( new HOTKEY_SECTION_PAGE( this, m_hotkeySections,
                                                                 wxGetTranslation( *section->m_Title ),
                                                                 section ) );
    }

    m_sdbSizerOK->SetDefault();
    Center();
}


bool HOTKEYS_EDITOR_DIALOG::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;
    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        if( !(*i)->GetHotkeyCtrl()->TransferDataToControl() )
            return false;
    }

    return true;
}


bool HOTKEYS_EDITOR_DIALOG::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;
    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        if( !(*i)->GetHotkeyCtrl()->TransferDataFromControl() )
            return false;
    }

    // save the hotkeys
    m_parent->WriteHotkeyConfig( m_hotkeys );

    return true;
}


void HOTKEYS_EDITOR_DIALOG::ResetClicked( wxCommandEvent& aEvent )
{
    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;

    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        (*i)->Restore();
    }
}


bool HOTKEYS_EDITOR_DIALOG::CanSetKey( long aKey, const wxString& aSectionTag )
{
    std::vector<HOTKEY_SECTION_PAGE*>::iterator i;

    EDA_HOTKEY* conflictingKey = NULL;
    EDA_HOTKEY_CONFIG* conflictingSection = NULL;
    HOTKEY_LIST_CTRL *conflictingCtrl = NULL;

    for( i = m_hotkeySectionPages.begin(); i != m_hotkeySectionPages.end(); ++i )
    {
        HOTKEY_LIST_CTRL *ctrl = (*i)->GetHotkeyCtrl();

        if ( !ctrl->CanSetKey( aKey, aSectionTag, &conflictingKey, &conflictingSection ) )
        {
            conflictingCtrl = ctrl;
            break;
        }
    }

    if( conflictingKey != NULL )
    {
        wxString info = wxGetTranslation( conflictingKey->m_InfoMsg );
        wxString msg = wxString::Format(
            _( "<%s> is already assigned to \"%s\" in section \"%s\". Are you sure you want "
               "to change its assignment?" ),
            KeyNameFromKeyCode( aKey ), GetChars( info ),
            *(conflictingSection->m_Title) );

        wxMessageDialog dlg( this, msg, _( "Confirm change" ), wxYES_NO | wxNO_DEFAULT );

        if( dlg.ShowModal() == wxID_YES )
        {
            conflictingKey->m_KeyCode = 0;
            conflictingCtrl->UpdateFromClientData();
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}
