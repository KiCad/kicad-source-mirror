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
#include <dialog_shim.h>


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


/**
 * Class HK_PROMPT_DIALOG
 * Dialog to prompt the user to enter a key.
 */
class HK_PROMPT_DIALOG: public DIALOG_SHIM
{
    wxKeyEvent m_event;

public:
    HK_PROMPT_DIALOG( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
            const wxString& aName, const wxString& aCurrentKey )
        :   DIALOG_SHIM( aParent, aId, aTitle, wxDefaultPosition, wxDefaultSize )
    {
        wxPanel* panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize );
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        wxStaticText* inst_label = new wxStaticText( panel, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL );
        inst_label->SetLabelText( _( "Press a new hotkey, or press Esc to reset..." ) );

        {
            wxFont font = inst_label->GetFont();
            inst_label->SetFont( font.Bold() );
        }

        sizer->Add( inst_label, 0, wxALL, 5 );

        sizer->Add( new wxStaticText( panel, wxID_ANY,
                        _( "Command: " ) + aName ),
                    0, wxALL, 5 );

        sizer->Add( new wxStaticText( panel, wxID_ANY,
                        _( "Current key: ") + aCurrentKey ),
                    0, wxALL, 5 );

        // Wrap the sizer in a second to give a larger border around the whole dialog
        wxBoxSizer* outer_sizer = new wxBoxSizer( wxVERTICAL );
        outer_sizer->Add( sizer, 0, wxALL, 10 );
        panel->SetSizer( outer_sizer );

        Layout();
        outer_sizer->Fit( this );
        Center();

        SetMinClientSize( GetClientSize() );

        panel->Bind( wxEVT_CHAR, &HK_PROMPT_DIALOG::OnChar, this );
    }


    void OnChar( wxKeyEvent& aEvent )
    {
        m_event = aEvent;
        EndFlexible( wxID_OK );
    }


    /**
     * End the dialog whether modal or quasimodal
     */
    void EndFlexible( int aRtnCode )
    {
        if( IsQuasiModal() )
            EndQuasiModal( aRtnCode );
        else
            EndModal( aRtnCode );
    }


    static wxKeyEvent PromptForKey( wxWindow* aParent, const wxString& aName,
            const wxString& aCurrentKey )
    {
        HK_PROMPT_DIALOG dialog( aParent, wxID_ANY, _( "Set Hotkey" ), aName, aCurrentKey );
        if( dialog.ShowModal() == wxID_OK )
        {
            return dialog.m_event;
        }
        else
        {
            wxKeyEvent dummy;
            return dummy;
        }
    }
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


void WIDGET_HOTKEY_LIST::OnActivated( wxTreeListEvent& aEvent )
{
    wxTreeListItem item = aEvent.GetItem();
    WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( item );

    if( !hkdata )
    {
        // Activated item was not a hotkey row
        aEvent.Skip();
        return;
    }

    wxString name = GetItemText( item, 0 );
    wxString current_key = GetItemText( item, 1 );

    wxKeyEvent key_event = HK_PROMPT_DIALOG::PromptForKey( GetParent(), name, current_key );

    if( hkdata )
    {
        long key = key_event.GetKeyCode();

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
            if( key_event.ControlDown() && key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
                key += 'A' - 1;

            /* Disallow shift for keys that have two keycodes on them (e.g. number and
             * punctuation keys) leaving only the "letter keys" of A-Z.
             * Then, you can have, e.g. Ctrl-5 and Ctrl-% (GB layout)
             * and Ctrl-( and Ctrl-5 (FR layout).
             * Otherwise, you'd have to have to say Ctrl-Shift-5 on a FR layout
             */
            bool keyIsLetter = key >= 'A' && key <= 'Z';

            if( key_event.ShiftDown() && ( keyIsLetter || key > 256 ) )
                key |= GR_KB_SHIFT;

            if( key_event.ControlDown() )
                key |= GR_KB_CTRL;

            if( key_event.AltDown() )
                key |= GR_KB_ALT;

            // See if this key code is handled in hotkeys names list
            bool exists;
            KeyNameFromKeyCode( key, &exists );

            if( exists && hkdata->GetHotkey().m_KeyCode != key )
            {
                wxString tag = hkdata->GetSectionTag();
                bool canUpdate = ResolveKeyConflicts( key, tag );

                if( canUpdate )
                {
                    hkdata->GetHotkey().m_KeyCode = key;
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

    Bind( wxEVT_TREELIST_ITEM_ACTIVATED, &WIDGET_HOTKEY_LIST::OnActivated, this );
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
