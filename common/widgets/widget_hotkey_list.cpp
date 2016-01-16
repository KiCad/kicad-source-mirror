/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <widgets/widget_hotkey_list.h>

#include <wx/dataview.h>

#include <draw_frame.h>


/**
 * Class WIDGET_HOTKEY_CLIENT_DATA
 * Stores the hotkey and section tag associated with each row. To change a
 * hotkey, edit it in the row's client data, then call WIDGET_HOTKEY_LIST::UpdateFromClientData().
 */
class WIDGET_HOTKEY_CLIENT_DATA: public wxClientData
{
    EDA_HOTKEY  m_hotkey;
    wxString    m_section_tag;

public:
    WIDGET_HOTKEY_CLIENT_DATA( const EDA_HOTKEY& aHotkey, const wxString& aSectionTag )
        :   m_hotkey( aHotkey ),
            m_section_tag( aSectionTag )
    { }

    EDA_HOTKEY& GetHotkey() { return m_hotkey; }
    const wxString& GetSectionTag() const { return m_section_tag; }
};


WIDGET_HOTKEY_CLIENT_DATA* WIDGET_HOTKEY_LIST::GetHKClientData( wxTreeListItem aItem )
{
    if( aItem.IsOk() )
    {
        wxClientData* data = GetItemData( aItem );
        if( !data )
        {
            return NULL;
        }
        else
        {
            return static_cast<WIDGET_HOTKEY_CLIENT_DATA*>( data );
        }
    }
    else
    {
        return NULL;
    }
}


WIDGET_HOTKEY_CLIENT_DATA* WIDGET_HOTKEY_LIST::GetSelHKClientData()
{
    return GetHKClientData( GetSelection() );
}


void WIDGET_HOTKEY_LIST::UpdateFromClientData()
{
    for( wxTreeListItem i = GetFirstItem(); i.IsOk(); i = GetNextItem( i ) )
    {
        WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( i );

        if( hkdata )
        {
            EDA_HOTKEY& hk = hkdata->GetHotkey();

            SetItemText( i, 0, wxGetTranslation( hk.m_InfoMsg ) );
            SetItemText( i, 1, KeyNameFromKeyCode( hk.m_KeyCode ) );
        }
    }
}


void WIDGET_HOTKEY_LIST::LoadSection( EDA_HOTKEY_CONFIG* aSection )
{
    HOTKEY_LIST list;

    for( EDA_HOTKEY** info_ptr = aSection->m_HK_InfoList; *info_ptr; ++info_ptr )
    {
        list.push_back( **info_ptr );
    }

    m_hotkeys.push_back( list );
}


void WIDGET_HOTKEY_LIST::OnChar( wxKeyEvent& aEvent )
{
    WIDGET_HOTKEY_CLIENT_DATA* data = GetSelHKClientData();

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


void WIDGET_HOTKEY_LIST::OnSize( wxSizeEvent& aEvent )
{
    // Handle this manually - wxTreeListCtrl screws up the width of the first column
    wxDataViewCtrl* view = GetDataView();

    if( !view )
        return;

    wxRect rect = GetClientRect();
    view->SetSize( rect );

#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    {
        wxWindow* view = GetView();
        view->Refresh();
        view->Update();
    }
#endif

    SetColumnWidth( 1, WIDGET_HOTKEY_LIST_HKCOLUMN_WIDTH );
    SetColumnWidth( 0,
            rect.width - WIDGET_HOTKEY_LIST_HKCOLUMN_WIDTH - WIDGET_HOTKEY_LIST_HMARGIN );
}


bool WIDGET_HOTKEY_LIST::CheckKeyConflicts( long aKey, const wxString& aSectionTag,
        EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect )
{
    EDA_HOTKEY*                 conflicting_key = NULL;
    struct EDA_HOTKEY_CONFIG*   conflicting_section = NULL;

    for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
    {
        WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( item );
        if( !hkdata )
            continue;

        EDA_HOTKEY& hk = hkdata->GetHotkey();
        wxString tag = hkdata->GetSectionTag();

        if( aSectionTag != g_CommonSectionTag
                && tag != g_CommonSectionTag
                && tag != aSectionTag )
        {
            // This key and its conflict candidate are in orthogonal sections, so skip.
            continue;
        }

        if( aKey == hk.m_KeyCode )
        {
            conflicting_key = &hk;

            // Find the section
            HOTKEY_SECTIONS::iterator it;
            for( it = m_sections.begin(); it != m_sections.end(); ++it )
            {
                if( *it->m_section->m_SectionTag == tag )
                {
                    conflicting_section = it->m_section;
                    break;
                }
            }
        }
    }

    // Write the outparams
    if( aConfKey )
        *aConfKey = conflicting_key;

    if( aConfSect )
        *aConfSect = conflicting_section;

    return conflicting_key == NULL;
}


bool WIDGET_HOTKEY_LIST::ResolveKeyConflicts( long aKey, const wxString& aSectionTag )
{
    EDA_HOTKEY*         conflicting_key = NULL;
    EDA_HOTKEY_CONFIG*  conflicting_section = NULL;

    CheckKeyConflicts( aKey, aSectionTag, &conflicting_key, &conflicting_section );

    if( conflicting_key != NULL )
    {
        wxString info = wxGetTranslation( conflicting_key->m_InfoMsg );
        wxString msg = wxString::Format(
            _( "<%s> is already assigned to \"%s\" in section \"%s\". Are you sure you want "
               "to change its assignment?" ),
            KeyNameFromKeyCode( aKey ), GetChars( info ),
            *(conflicting_section->m_Title) );

        wxMessageDialog dlg( GetParent(), msg, _( "Confirm change" ), wxYES_NO | wxNO_DEFAULT );

        if( dlg.ShowModal() == wxID_YES )
        {
            conflicting_key->m_KeyCode = 0;
            UpdateFromClientData();
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
}


WIDGET_HOTKEY_LIST::WIDGET_HOTKEY_LIST( wxWindow* aParent, const HOTKEY_SECTIONS& aSections )
        :   wxTreeListCtrl( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE ),
            m_sections( aSections )
{
    AppendColumn( _( "Command" ) );
    AppendColumn( _( "Hotkey" ) );

    Bind( wxEVT_CHAR, &WIDGET_HOTKEY_LIST::OnChar, this );
    Bind( wxEVT_SIZE, &WIDGET_HOTKEY_LIST::OnSize, this );
}


HOTKEY_SECTIONS WIDGET_HOTKEY_LIST::GenSections( EDA_HOTKEY_CONFIG* aHotkeys )
{
    HOTKEY_SECTIONS sections;
    for( EDA_HOTKEY_CONFIG* section = aHotkeys; section->m_HK_InfoList; ++section )
    {
        HOTKEY_SECTION sec;
        sec.m_name = wxGetTranslation( *section->m_Title );
        sec.m_section = section;
        sections.push_back( sec );
    }
    return sections;
}


bool WIDGET_HOTKEY_LIST::TransferDataToControl()
{
    Freeze();
    DeleteAllItems();
    m_hotkeys.clear();

    for( size_t sec_index = 0; sec_index < m_sections.size(); ++sec_index )
    {
        // LoadSection pushes into m_hotkeys
        LoadSection( m_sections[sec_index].m_section );
        wxASSERT( m_hotkeys.size() == sec_index + 1 );

        wxString section_tag = *( m_sections[sec_index].m_section->m_SectionTag );

        // Create parent tree item
        wxTreeListItem parent = AppendItem( GetRootItem(), m_sections[sec_index].m_name );

        HOTKEY_LIST& each_list = m_hotkeys[sec_index];
        HOTKEY_LIST::iterator hk_it;
        for( hk_it = each_list.begin(); hk_it != each_list.end(); ++hk_it )
        {
            wxTreeListItem item = AppendItem( parent, wxEmptyString );
            SetItemData( item, new WIDGET_HOTKEY_CLIENT_DATA( &*hk_it, section_tag ) );
        }

        Expand( parent );
    }

    UpdateFromClientData();
    Thaw();

    return true;
}


bool WIDGET_HOTKEY_LIST::TransferDataFromControl()
{
    for( size_t sec_index = 0; sec_index < m_sections.size(); ++sec_index )
    {
        EDA_HOTKEY_CONFIG* section = m_sections[sec_index].m_section;
        for( EDA_HOTKEY** info_ptr = section->m_HK_InfoList; *info_ptr; ++info_ptr )
        {
            EDA_HOTKEY* info = *info_ptr;
            for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
            {
                WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( item );
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
