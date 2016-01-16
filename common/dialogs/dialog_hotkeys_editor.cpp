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

    Bind( wxEVT_CHAR, &HOTKEY_LIST_CTRL::OnChar, this );
    Bind( wxEVT_SIZE, &HOTKEY_LIST_CTRL::OnSize, this );
}


HOTKEYS_SECTIONS HOTKEY_LIST_CTRL::Sections( EDA_HOTKEY_CONFIG* aHotkeys )
{
    HOTKEYS_SECTIONS sections;
    for( EDA_HOTKEY_CONFIG* section = aHotkeys; section->m_HK_InfoList; ++section )
    {
        HOTKEYS_SECTION sec( wxGetTranslation( *section->m_Title ), section );
        sections.push_back( sec );
    }
    return sections;
}


void HOTKEY_LIST_CTRL::OnSize( wxSizeEvent& aEvent )
{
    // Handle this manually - wxTreeListCtrl screws up the width of the first column
    wxDataViewCtrl* view = GetDataView();

    if( !view )
        return;

    const wxRect rect = GetClientRect();
    view->SetSize( rect );

#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    {
        wxWindow* const view = GetView();
        view->Refresh();
        view->Update();
    }
#endif

    SetColumnWidth( 1, 100 );
    SetColumnWidth( 0, rect.width - 120 );
}


void HOTKEY_LIST_CTRL::DeselectRow( int aRow )
{
    wxASSERT( aRow >= 0 );
    wxASSERT( (size_t)( aRow ) < m_items.size() );
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
                bool canUpdate = ResolveKeyConflicts( key, tag );

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

    HOTKEYS_SECTIONS::iterator sec_it;
    size_t sec_index = 0;
    for( sec_it = m_sections.begin(); sec_it != m_sections.end(); ++sec_it, ++sec_index )
    {
        LoadSection( sec_it->second );
        wxString section_tag = *( sec_it->second->m_SectionTag );

        // Create parent item
        wxTreeListItem parent = AppendItem( GetRootItem(), sec_it->first );

        HOTKEY_LIST& each_list = m_hotkeys[sec_index];
        HOTKEY_LIST::iterator hk_it;
        for( hk_it = each_list.begin(); hk_it != each_list.end(); ++hk_it )
        {
            wxTreeListItem item = AppendItem( parent, wxEmptyString );
            SetItemData( item, new DIALOG_HOTKEY_CLIENT_DATA( &*hk_it, section_tag ) );
            m_items.push_back( item );
        }

        Expand( parent );
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


bool HOTKEY_LIST_CTRL::ResolveKeyConflicts( long aKey, const wxString& aSectionTag )
{
    EDA_HOTKEY* conflictingKey = NULL;
    EDA_HOTKEY_CONFIG* conflictingSection = NULL;

    CheckKeyConflicts( aKey, aSectionTag, &conflictingKey, &conflictingSection );

    if( conflictingKey != NULL )
    {
        wxString info = wxGetTranslation( conflictingKey->m_InfoMsg );
        wxString msg = wxString::Format(
            _( "<%s> is already assigned to \"%s\" in section \"%s\". Are you sure you want "
               "to change its assignment?" ),
            KeyNameFromKeyCode( aKey ), GetChars( info ),
            *(conflictingSection->m_Title) );

        wxMessageDialog dlg( GetParent(), msg, _( "Confirm change" ), wxYES_NO | wxNO_DEFAULT );

        if( dlg.ShowModal() == wxID_YES )
        {
            conflictingKey->m_KeyCode = 0;
            UpdateFromClientData();
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}


bool HOTKEY_LIST_CTRL::CheckKeyConflicts( long aKey, const wxString& aSectionTag,
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
    m_hotkeys( aHotkeys )
{
    m_hotkeyListCtrl = new HOTKEY_LIST_CTRL( this, HOTKEY_LIST_CTRL::Sections( aHotkeys ) );
    m_mainSizer->Insert( 1, m_hotkeyListCtrl, wxSizerFlags( 1 ).Expand().Border( wxALL, 5 ) );
    Layout();

    m_sdbSizerOK->SetDefault();
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

