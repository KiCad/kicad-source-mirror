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
#include <wx/statline.h>

#include <draw_frame.h>
#include <dialog_shim.h>


/**
 * Minimum width of the hotkey column
 */
static const int HOTKEY_MIN_WIDTH = 100;

/**
 * Extra margin to compensate for vertical scrollbar
 */
static const int HORIZ_MARGIN = 30;


/**
 * Menu IDs for the hotkey context menu
 */
enum ID_WHKL_MENU_IDS
{
    ID_EDIT = 2001,
    ID_RESET,
    ID_RESET_ALL,
};


/**
 * Class WIDGET_HOTKEY_CLIENT_DATA
 * Stores the hotkey and section tag associated with each row. To change a
 * hotkey, edit it in the row's client data, then call WIDGET_HOTKEY_LIST::UpdateFromClientData().
 */
class WIDGET_HOTKEY_CLIENT_DATA : public wxClientData
{
    EDA_HOTKEY  m_hotkey;
    wxString    m_section_tag;

public:
    WIDGET_HOTKEY_CLIENT_DATA( const EDA_HOTKEY& aHotkey, const wxString& aSectionTag )
        :   m_hotkey( aHotkey ), m_section_tag( aSectionTag )
    {}

    EDA_HOTKEY& GetHotkey() { return m_hotkey; }
    const wxString& GetSectionTag() const { return m_section_tag; }
};


/**
 * Class HK_PROMPT_DIALOG
 * Dialog to prompt the user to enter a key.
 */
class HK_PROMPT_DIALOG : public DIALOG_SHIM
{
    wxKeyEvent m_event;

public:
    HK_PROMPT_DIALOG( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
            const wxString& aName, const wxString& aCurrentKey )
        :   DIALOG_SHIM( aParent, aId, aTitle, wxDefaultPosition, wxDefaultSize )
    {
        wxPanel* panel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize );
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        /* Dialog layout:
         *
         * inst_label........................
         * ----------------------------------
         *
         * cmd_label_0      cmd_label_1         \
         *                                      | fgsizer
         * key_label_0      key_label_1         /
         */

        wxStaticText* inst_label = new wxStaticText( panel, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL );

        inst_label->SetLabelText( _( "Press a new hotkey, or press Esc to cancel..." ) );
        sizer->Add( inst_label, 0, wxALL, 5 );

        sizer->Add( new wxStaticLine( panel ), 0, wxALL | wxEXPAND, 2 );

        wxFlexGridSizer* fgsizer = new wxFlexGridSizer( 2 );

        wxStaticText* cmd_label_0 = new wxStaticText( panel, wxID_ANY, _( "Command:" ) );
        fgsizer->Add( cmd_label_0, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* cmd_label_1 = new wxStaticText( panel, wxID_ANY, aName );
        cmd_label_1->SetFont( cmd_label_1->GetFont().Bold().MakeLarger() );
        fgsizer->Add( cmd_label_1, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* key_label_0 = new wxStaticText( panel, wxID_ANY, _( "Current key:" ) );
        fgsizer->Add( key_label_0, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* key_label_1 = new wxStaticText( panel, wxID_ANY, aCurrentKey );
        key_label_1->SetFont( key_label_1->GetFont().Bold().MakeLarger() );
        fgsizer->Add( key_label_1, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );


        sizer->Add( fgsizer, 1, wxEXPAND );

        // Wrap the sizer in a second to give a larger border around the whole dialog
        wxBoxSizer* outer_sizer = new wxBoxSizer( wxVERTICAL );
        outer_sizer->Add( sizer, 0, wxALL | wxEXPAND, 10 );
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


void WIDGET_HOTKEY_LIST::EditItem( wxTreeListItem aItem )
{
    WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( aItem );

    if( !hkdata )
    {
        // Activated item was not a hotkey row
        return;
    }

    wxString    name = GetItemText( aItem, 0 );
    wxString    current_key = GetItemText( aItem, 1 );

    wxKeyEvent key_event = HK_PROMPT_DIALOG::PromptForKey( GetParent(), name, current_key );
    long key = MapKeypressToKeycode( key_event );

    if( hkdata && key )
    {
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
        }

        UpdateFromClientData();
    }
}


void WIDGET_HOTKEY_LIST::ResetItem( wxTreeListItem aItem )
{
    WIDGET_HOTKEY_CLIENT_DATA* hkdata = GetHKClientData( aItem );
    EDA_HOTKEY* hk = &hkdata->GetHotkey();

    for( size_t sec_index = 0; sec_index < m_sections.size(); ++sec_index )
    {
        wxString& section_tag = *( m_sections[sec_index].m_section->m_SectionTag );

        if( section_tag != hkdata->GetSectionTag() )
            continue;

        HOTKEY_LIST& each_list = m_hotkeys[sec_index];
        HOTKEY_LIST::iterator hk_it;

        for( hk_it = each_list.begin(); hk_it != each_list.end(); ++hk_it )
        {
            if( hk_it->m_Idcommand == hk->m_Idcommand )
            {
                hk->m_KeyCode = hk_it->m_KeyCode;
                break;
            }
        }
    }

    UpdateFromClientData();
}


void WIDGET_HOTKEY_LIST::OnActivated( wxTreeListEvent& aEvent )
{
    EditItem( aEvent.GetItem() );
}


void WIDGET_HOTKEY_LIST::OnContextMenu( wxTreeListEvent& aEvent )
{
    // Save the active event for use in OnMenu
    m_context_menu_item = aEvent.GetItem();

    wxMenu menu;

    menu.Append( ID_EDIT, _( "Edit..." ) );
    menu.Append( ID_RESET, _( "Reset" ) );
    menu.Append( wxID_SEPARATOR );
    menu.Append( ID_RESET_ALL, _( "Reset all" ) );

    PopupMenu( &menu );
}


void WIDGET_HOTKEY_LIST::OnMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_EDIT:
        EditItem( m_context_menu_item );
        break;

    case ID_RESET:
        ResetItem( m_context_menu_item );
        break;

    case ID_RESET_ALL:
        TransferDataToControl();
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
    }
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

    // Find the maximum width of the hotkey column
    int hk_column_width = 0;

    for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
    {
        const wxString& text = GetItemText( item, 1 );
        int width = WidthFor( text );

        if( width > hk_column_width )
            hk_column_width = width;
    }

    if( hk_column_width < HOTKEY_MIN_WIDTH )
        hk_column_width = HOTKEY_MIN_WIDTH;

    SetColumnWidth( 1, hk_column_width );
    SetColumnWidth( 0, rect.width - hk_column_width - HORIZ_MARGIN );
}


bool WIDGET_HOTKEY_LIST::CheckKeyConflicts( long aKey, const wxString& aSectionTag,
        EDA_HOTKEY** aConfKey, EDA_HOTKEY_CONFIG** aConfSect )
{
    EDA_HOTKEY* conflicting_key = NULL;
    struct EDA_HOTKEY_CONFIG* conflicting_section = NULL;

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
    EDA_HOTKEY* conflicting_key = NULL;
    EDA_HOTKEY_CONFIG* conflicting_section = NULL;

    CheckKeyConflicts( aKey, aSectionTag, &conflicting_key, &conflicting_section );

    if( conflicting_key != NULL )
    {
        wxString    info    = wxGetTranslation( conflicting_key->m_InfoMsg );
        wxString    msg     = wxString::Format(
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
    Bind( wxEVT_TREELIST_ITEM_CONTEXT_MENU, &WIDGET_HOTKEY_LIST::OnContextMenu, this );
    Bind( wxEVT_MENU, &WIDGET_HOTKEY_LIST::OnMenu, this );
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


void WIDGET_HOTKEY_LIST::InstallOnPanel( wxPanel* aPanel )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->Add( this, 1, wxALL | wxEXPAND, 0 );
    aPanel->SetSizer( sizer );
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


long WIDGET_HOTKEY_LIST::MapKeypressToKeycode( const wxKeyEvent& aEvent )
{
    long key = aEvent.GetKeyCode();

    if( key == WXK_ESCAPE )
    {
        return 0;
    }
    else
    {
        if( key >= 'a' && key <= 'z' )    // convert to uppercase
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

        return key;
    }
}
