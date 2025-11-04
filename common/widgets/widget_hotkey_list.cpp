/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cctype>

#include <confirm.h>
#include <widgets/widget_hotkey_list.h>
#include <tool/tool_event.h>
#include <dialog_shim.h>

#include <wx/button.h>
#include <wx/log.h>
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/treelist.h>
#include <wx/wupdlock.h>

/**
 * Menu IDs for the hotkey context menu
 */
enum ID_WHKL_MENU_IDS
{
    ID_EDIT_HOTKEY = 2001,
    ID_EDIT_ALT,
    ID_RESET,
    ID_DEFAULT,
    ID_CLEAR,
    ID_CLEAR_ALT,
};


/**
 * Store the hotkey change data associated with each row.
 *
 * To change a hotkey, edit it via GetCurrentValue() in the row's client data, then call
 * WIDGET_HOTKEY_LIST::UpdateFromClientData().
 */
class WIDGET_HOTKEY_CLIENT_DATA : public wxClientData
{
    HOTKEY&  m_changed_hotkey;

public:
    WIDGET_HOTKEY_CLIENT_DATA( HOTKEY& aChangedHotkey ) :
            m_changed_hotkey( aChangedHotkey )
    {}

    HOTKEY& GetChangedHotkey() { return m_changed_hotkey; }
};


/**
 * Dialog to prompt the user to enter a key.
 */
class HK_PROMPT_DIALOG : public DIALOG_SHIM
{
public:
    HK_PROMPT_DIALOG( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
                      const wxString& aName, const wxString& aCurrentKey ) :
            DIALOG_SHIM( aParent, aId, aTitle, wxDefaultPosition, wxDefaultSize )
    {
        wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
        SetSizer( mainSizer );

        /* Dialog layout:
         *
         * inst_label........................
         * ----------------------------------
         *
         * cmd_label_0      cmd_label_1         \
         *                                      | fgsizer
         * key_label_0      key_label_1         /
         */

        wxStaticText* inst_label = new wxStaticText( this, wxID_ANY, wxEmptyString,
                                                     wxDefaultPosition, wxDefaultSize,
                                                     wxALIGN_CENTRE_HORIZONTAL );

        inst_label->SetLabelText( _( "Press a new hotkey, or press Esc to cancel..." ) );
        mainSizer->Add( inst_label, 0, wxALL, 10 );

        mainSizer->Add( new wxStaticLine( this ), 0, wxALL | wxEXPAND, 2 );

        wxPanel* panelDisplayCurrent = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize );
        mainSizer->Add( panelDisplayCurrent, 0, wxALL | wxEXPAND, 5 );

        wxFlexGridSizer* fgsizer = new wxFlexGridSizer( 2 );
        panelDisplayCurrent->SetSizer( fgsizer );

        wxStaticText* cmd_label_0 = new wxStaticText( panelDisplayCurrent, wxID_ANY, _( "Command:" ) );
        fgsizer->Add( cmd_label_0, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* cmd_label_1 = new wxStaticText( panelDisplayCurrent, wxID_ANY, wxEmptyString );
        cmd_label_1->SetFont( cmd_label_1->GetFont().Bold() );
        cmd_label_1->SetLabel( aName );
        fgsizer->Add( cmd_label_1, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* key_label_0 = new wxStaticText( panelDisplayCurrent, wxID_ANY, _( "Current key:" ) );
        fgsizer->Add( key_label_0, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        wxStaticText* key_label_1 = new wxStaticText( panelDisplayCurrent, wxID_ANY, wxEmptyString );
        key_label_1->SetFont( key_label_1->GetFont().Bold() );
        key_label_1->SetLabel( aCurrentKey );
        fgsizer->Add( key_label_1, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 5 );

        fgsizer->AddStretchSpacer();

        wxButton* resetButton = new wxButton( this, wxID_ANY, _( "Clear assigned hotkey" ) );

        mainSizer->Add( resetButton, 0, wxALL | wxALIGN_CENTRE_HORIZONTAL, 5 );

        Layout();
        mainSizer->Fit( this );
        Center();

        SetMinClientSize( GetClientSize() );

        // Binding both EVT_CHAR and EVT_CHAR_HOOK ensures that all key events, including
        // specials like Tab and Return, are received, particularly on MSW.
        panelDisplayCurrent->Bind( wxEVT_CHAR, &HK_PROMPT_DIALOG::OnChar, this );
        panelDisplayCurrent->Bind( wxEVT_CHAR_HOOK, &HK_PROMPT_DIALOG::OnCharHook, this );
        panelDisplayCurrent->Bind( wxEVT_KEY_UP, &HK_PROMPT_DIALOG::OnKeyUp, this );

        resetButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &HK_PROMPT_DIALOG::onResetButton, this );

        SetInitialFocus( panelDisplayCurrent );
    }

    static std::optional<long> PromptForKey( wxWindow* aParent, const wxString& aName,
                                             const wxString& aCurrentKey )
    {
        HK_PROMPT_DIALOG dialog( aParent, wxID_ANY, _( "Set Hotkey" ), aName, aCurrentKey );

        if( dialog.ShowModal() == wxID_OK )
        {
            if( dialog.m_resetkey )
            {
                return std::make_optional( 0 );
            }
            else
            {
                long key = WIDGET_HOTKEY_LIST::MapKeypressToKeycode( dialog.m_event );

                if( key )
                    return std::make_optional( key );
                else    // The ESC key was used to close the dialog
                    return std::nullopt;
            }
        }
        else
        {
            return std::nullopt;
        }
    }

protected:
    void OnCharHook( wxKeyEvent& aEvent ) override
    {
        // On certain platforms, EVT_CHAR_HOOK is the only handler that receives certain
        // "special" keys. However, it doesn't always receive "normal" keys correctly. For
        // example, with a US keyboard, it sees ? as shift+/.
        //
        // Untangling these incorrect keys would be too much trouble, so we bind both events,
        // and simply skip the EVT_CHAR_HOOK if it receives a "normal" key.

        const enum wxKeyCode skipped_keys[] =
        {
            WXK_NONE, WXK_SHIFT, WXK_ALT, WXK_CONTROL, WXK_CAPITAL, WXK_NUMLOCK, WXK_SCROLL,
            WXK_RAW_CONTROL
        };

        int key = aEvent.GetKeyCode();

        for( wxKeyCode skipped_key : skipped_keys )
        {
            if( key == skipped_key )
                return;
        }

        if( key <= 255 && isprint( key ) && !isspace( key ) )
        {
            // Let EVT_CHAR handle this one
            aEvent.DoAllowNextEvent();

#ifdef __WXMSW__
            // On Windows, looks like a OnChar event is not generated when
            // using the Alt key modifier. So ensure m_event is up to date
            // to handle the right key code when releasing the key and therefore
            // closing the dialog
            m_event = aEvent;
#endif
            aEvent.Skip();
        }
        else
        {
            OnChar( aEvent );
        }
    }

    void OnChar( wxKeyEvent& aEvent )
    {
        m_event = aEvent;
    }

    void OnKeyUp( wxKeyEvent& aEvent )
    {
        // If dialog opened using Enter key, prevent closing when releasing Enter.
        if( m_event.GetEventType() != wxEVT_NULL )
        {
            /// This needs to occur in KeyUp, so that we don't pass the event back to pcbnew
            wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        }
    }

    void onResetButton( wxCommandEvent& aEvent )
    {
        m_resetkey = true;
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
    }

private:
    bool       m_resetkey = false;
    wxKeyEvent m_event;
};


/**
 * Manage logic for filtering hotkeys based on user input.
 */
class HOTKEY_FILTER
{
public:
    HOTKEY_FILTER( const wxString& aFilterStr )
    {
        m_normalised_filter_str = aFilterStr.Upper();
        m_valid = m_normalised_filter_str.size() > 0;
    }

    /**
     * Check if the filter matches the given hotkey
     *
     * @return true on match (or if filter is disabled)
     */
    bool FilterMatches( const HOTKEY& aHotkey ) const
    {
        if( !m_valid )
            return true;

        // Match in the (translated) filter string
        const auto normedInfo = wxGetTranslation( aHotkey.m_Actions[0]->GetFriendlyName() ).Upper();

        if( normedInfo.Contains( m_normalised_filter_str ) )
            return true;

        const wxString keyName = KeyNameFromKeyCode( aHotkey.m_EditKeycode );

        if( keyName.Upper().Contains( m_normalised_filter_str ) )
            return true;

        return false;
    }

private:
    bool     m_valid;
    wxString m_normalised_filter_str;
};


WIDGET_HOTKEY_CLIENT_DATA* WIDGET_HOTKEY_LIST::getHKClientData( wxTreeListItem aItem )
{
    if( aItem.IsOk() )
    {
        wxClientData* data = GetItemData( aItem );

        if( data )
            return static_cast<WIDGET_HOTKEY_CLIENT_DATA*>( data );
    }

    return nullptr;
}


void WIDGET_HOTKEY_LIST::updateFromClientData()
{
    for( wxTreeListItem i = GetFirstItem(); i.IsOk(); i = GetNextItem( i ) )
    {
        WIDGET_HOTKEY_CLIENT_DATA* hkdata = getHKClientData( i );

        if( hkdata )
        {
            const HOTKEY& changed_hk = hkdata->GetChangedHotkey();
            wxString      label = changed_hk.m_Actions[ 0 ]->GetFriendlyName();
            wxString      key_text = KeyNameFromKeyCode( changed_hk.m_EditKeycode );
            wxString      alt_text = KeyNameFromKeyCode( changed_hk.m_EditKeycodeAlt );
            wxString      description = changed_hk.m_Actions[ 0 ]->GetDescription();

            if( label.IsEmpty() )
                label = changed_hk.m_Actions[ 0 ]->GetName();

            label.Replace( wxT( "..." ), wxEmptyString );
            label.Replace( wxT( "…" ), wxEmptyString );

            // mark unsaved changes
            if( changed_hk.m_EditKeycode != changed_hk.m_Actions[ 0 ]->GetHotKey() )
                label += wxS( " *" );

            description.Replace( wxS( "\n" ), wxS( " " ) );
            description.Replace( wxS( "\r" ), wxS( " " ) );

            SetItemText( i, 0, label );
            SetItemText( i, 1, key_text );
            SetItemText( i, 2, alt_text );
            SetItemText( i, 3, description );
        }
    }
}


void WIDGET_HOTKEY_LIST::changeHotkey( HOTKEY& aHotkey, long aKey, bool alternate )
{
    // See if this key code is handled in hotkeys names list
    bool exists;
    KeyNameFromKeyCode( aKey, &exists );

    if( exists && aHotkey.m_EditKeycode != aKey )
    {
        if( aKey == 0 || resolveKeyConflicts( aHotkey.m_Actions[ 0 ], aKey ) )
        {
            if( alternate )
                aHotkey.m_EditKeycodeAlt = aKey;
            else
                aHotkey.m_EditKeycode = aKey;
        }
    }
}


void WIDGET_HOTKEY_LIST::editItem( wxTreeListItem aItem, int aEditId )
{
    WIDGET_HOTKEY_CLIENT_DATA* hkdata = getHKClientData( aItem );

    if( !hkdata )
        return;

    wxString    name = GetItemText( aItem, 0 );
    wxString    current_key = aEditId == ID_EDIT_HOTKEY ? GetItemText( aItem, 1 )
                                                        : GetItemText( aItem, 2 );

    std::optional<long> key = HK_PROMPT_DIALOG::PromptForKey( this, name, current_key );

    // An empty optional means don't change the key
    if( key.has_value() )
    {
        auto it = m_reservedHotkeys.find( key.value() );

        if( it != m_reservedHotkeys.end() )
        {
            wxString msg = wxString::Format( _( "'%s' is a reserved hotkey in KiCad and cannot "
                                                "be assigned." ),
                                             it->second );

            DisplayErrorMessage( this, msg );
            return;
        }

        changeHotkey( hkdata->GetChangedHotkey(), key.value(), aEditId == ID_EDIT_ALT );
        updateFromClientData();
    }
}


void WIDGET_HOTKEY_LIST::resetItem( wxTreeListItem aItem, int aResetId )
{
    WIDGET_HOTKEY_CLIENT_DATA* hkdata = getHKClientData( aItem );

    if( !hkdata )
        return;

    HOTKEY& changed_hk = hkdata->GetChangedHotkey();

    if( aResetId == ID_RESET )
    {
        changeHotkey( changed_hk, changed_hk.m_Actions[0]->GetHotKey(), false );
        changeHotkey( changed_hk, changed_hk.m_Actions[0]->GetHotKey(), true );
    }
    else if( aResetId == ID_CLEAR )
    {
        changeHotkey( changed_hk, 0, false );
    }
    else if( aResetId == ID_CLEAR_ALT )
    {
        changeHotkey( changed_hk, 0, true );
    }
    else if( aResetId == ID_DEFAULT )
    {
        changeHotkey( changed_hk, changed_hk.m_Actions[0]->GetDefaultHotKey(), false );
        changeHotkey( changed_hk, changed_hk.m_Actions[0]->GetDefaultHotKeyAlt(), true );
    }

    updateFromClientData();
}


void WIDGET_HOTKEY_LIST::onActivated( wxTreeListEvent& aEvent )
{
    editItem( aEvent.GetItem(), ID_EDIT_HOTKEY );
}


void WIDGET_HOTKEY_LIST::onContextMenu( wxTreeListEvent& aEvent )
{
    // Save the active event for use in OnMenu
    m_context_menu_item = aEvent.GetItem();

    wxMenu menu;

    WIDGET_HOTKEY_CLIENT_DATA* hkdata = getHKClientData( m_context_menu_item );

    // Some actions only apply if the row is hotkey data
    if( hkdata )
    {
        menu.Append( ID_EDIT_HOTKEY, _( "Edit..." ) );
        menu.Append( ID_EDIT_ALT, _( "Edit Alternate..." ) );
        menu.Append( ID_RESET, _( "Undo Changes" ) );
        menu.Append( ID_CLEAR, _( "Clear Assigned Hotkey" ) );
        menu.Append( ID_CLEAR_ALT, _( "Clear Assigned Alternate" ) );
        menu.Append( ID_DEFAULT, _( "Restore Defaults" ) );
        menu.Append( wxID_SEPARATOR );

        PopupMenu( &menu );
    }
}


void WIDGET_HOTKEY_LIST::onMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_EDIT_HOTKEY:
    case ID_EDIT_ALT:
        editItem( m_context_menu_item, aEvent.GetId() );
        break;

    case ID_RESET:
    case ID_CLEAR:
    case ID_CLEAR_ALT:
    case ID_DEFAULT:
        resetItem( m_context_menu_item, aEvent.GetId() );
        break;

    default:
        wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
    }
}


bool WIDGET_HOTKEY_LIST::resolveKeyConflicts( TOOL_ACTION* aAction, long aKey )
{
    HOTKEY* conflictingHotKey = nullptr;

    m_hk_store.CheckKeyConflicts( aAction, aKey, &conflictingHotKey );

    if( !conflictingHotKey )
        return true;

    TOOL_ACTION* conflictingAction = conflictingHotKey->m_Actions[ 0 ];
    wxString msg = wxString::Format( _( "'%s' is already assigned to '%s' in section '%s'. "
                                        "Are you sure you want to change its assignment?" ),
                                     KeyNameFromKeyCode( aKey ),
                                     conflictingAction->GetFriendlyName(),
                                     HOTKEY_STORE::GetSectionName( conflictingAction ) );

    wxMessageDialog dlg( GetParent(), msg, _( "Confirm change" ), wxYES_NO | wxNO_DEFAULT );

    if( dlg.ShowModal() == wxID_YES )
    {
        // Reset the other hotkey
        conflictingHotKey->m_EditKeycode = 0;
        updateFromClientData();
        return true;
    }

    return false;
}


WIDGET_HOTKEY_LIST::WIDGET_HOTKEY_LIST( wxWindow* aParent, HOTKEY_STORE& aHotkeyStore ) :
        wxTreeListCtrl( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE ),
        m_hk_store( aHotkeyStore )
{
    wxString command_header = _( "Command (double-click to edit)" );

    AppendColumn( command_header, 450, wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    AppendColumn( _( "Hotkey" ), 120, wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    AppendColumn( _( "Alternate" ), 120, wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );
    AppendColumn( _( "Description" ), 900, wxALIGN_LEFT, wxCOL_RESIZABLE | wxCOL_SORTABLE );


#if defined( __WXGTK__ )// && !wxCHECK_VERSION( 3, 1, 0 )
    // Automatic column widths are broken in wxGTK 3.0.x; set min widths to ensure visibility
    // They are also broken in wxGTK 3.1.4

    wxDataViewCtrl* dv = GetDataView();

    wxString longKey = wxT( "Ctrl+Alt+Shift+X" );
    int      pad     = 20;

    dv->GetColumn( 0 )->SetMinWidth( aParent->GetTextExtent( command_header ).x * 2 + pad );
    dv->GetColumn( 1 )->SetMinWidth( aParent->GetTextExtent( longKey ).x + pad );
    dv->GetColumn( 2 )->SetMinWidth( aParent->GetTextExtent( longKey ).x + pad );
    dv->GetColumn( 3 )->SetMinWidth( aParent->GetTextExtent( command_header ).x * 5 + pad );

    CallAfter( [this]()
               {
                   GetDataView()->Update();
               } );
#endif

    std::vector<wxString> reserved_keys =
            {
                wxS( "Ctrl+Tab" ),
                wxS( "Ctrl+Shift+Tab" )
            };

    for( const wxString& key : reserved_keys )
    {
        long code = KeyCodeFromKeyName( key );

        if( code )
            m_reservedHotkeys[code] = key;
        else
            wxLogWarning( wxS( "Unknown reserved keycode %s\n" ), key );
    }

    GetDataView()->SetIndent( 10 );

    // The event only apply if the widget is in editable mode
    Bind( wxEVT_TREELIST_ITEM_ACTIVATED, &WIDGET_HOTKEY_LIST::onActivated, this );
    Bind( wxEVT_TREELIST_ITEM_CONTEXT_MENU, &WIDGET_HOTKEY_LIST::onContextMenu, this );
    Bind( wxEVT_MENU, &WIDGET_HOTKEY_LIST::onMenu, this );
}


void WIDGET_HOTKEY_LIST::ApplyFilterString( const wxString& aFilterStr )
{
    updateShownItems( aFilterStr );
}


void WIDGET_HOTKEY_LIST::ResetAllHotkeys( bool aResetToDefault )
{
    wxWindowUpdateLocker updateLock( this );

    // Reset all the hotkeys, not just the ones shown
    // Should not need to check conflicts, as the state we're about
    // to set to a should be consistent
    if( aResetToDefault )
        m_hk_store.ResetAllHotkeysToDefault();
    else
        m_hk_store.ResetAllHotkeysToOriginal();

    updateFromClientData();
    updateColumnWidths();
}


bool WIDGET_HOTKEY_LIST::TransferDataToControl()
{
    updateShownItems( "" );
    updateColumnWidths();

    return true;
}


void WIDGET_HOTKEY_LIST::updateColumnWidths()
{
    wxDataViewColumn* col = GetDataView()->GetColumn( 0 );
    col->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    col->SetWidth( col->GetWidth() );

    col = GetDataView()->GetColumn( 1 );
    col->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    col->SetWidth( col->GetWidth() );

    col = GetDataView()->GetColumn( 2 );
    col->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    col->SetWidth( col->GetWidth() );

    col = GetDataView()->GetColumn( 3 );
    col->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    col->SetWidth( col->GetWidth() );
}


void WIDGET_HOTKEY_LIST::updateShownItems( const wxString& aFilterStr )
{
    wxWindowUpdateLocker updateLock( this );

    DeleteAllItems();

    HOTKEY_FILTER filter( aFilterStr );

    for( HOTKEY_SECTION& section: m_hk_store.GetSections() )
    {
        // Create parent tree item
        wxTreeListItem parent = AppendItem( GetRootItem(), section.m_SectionName );

        for( HOTKEY& hotkey: section.m_HotKeys )
        {
            if( filter.FilterMatches( hotkey ) )
            {
                wxTreeListItem item = AppendItem( parent, wxEmptyString );
                SetItemData( item, new WIDGET_HOTKEY_CLIENT_DATA( hotkey ) );
            }
        }

        Expand( parent );
    }

    updateFromClientData();
}


bool WIDGET_HOTKEY_LIST::TransferDataFromControl()
{
    m_hk_store.SaveAllHotkeys();
    return true;
}


long WIDGET_HOTKEY_LIST::MapKeypressToKeycode( const wxKeyEvent& aEvent )
{
    long key = aEvent.GetKeyCode();
    bool is_tab = aEvent.IsKeyInCategory( WXK_CATEGORY_TAB );

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
        if( !is_tab && aEvent.ControlDown() && key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
            key += 'A' - 1;

        /* Disallow shift for keys that have two keycodes on them (e.g. number and
         * punctuation keys) leaving only the "letter keys" of A-Z, tab and space
         * Then, you can have, e.g. Ctrl-5 and Ctrl-% (GB layout)
         * and Ctrl-( and Ctrl-5 (FR layout).
         * Otherwise, you'd have to have to say Ctrl-Shift-5 on a FR layout
         */
        bool keyIsLetter = key >= 'A' && key <= 'Z';

        int mods = aEvent.GetModifiers();

        if( ( mods & wxMOD_SHIFT ) && ( keyIsLetter || key > 256 || key == 9 || key == 32 ) )
            key |= MD_SHIFT;

        // the flag wxMOD_ALTGR is defined in wxWidgets as wxMOD_CONTROL|wxMOD_ALT
        // So AltGr key cannot used as modifier key because it is the same as Alt key + Ctrl key.
    #if CAN_USE_ALTGR_KEY
        if( wxmods & wxMOD_ALTGR )
            mods |= MD_ALTGR;
        else
    #endif
        {
            if( mods & wxMOD_CONTROL )
                key |= MD_CTRL;

            if( mods & wxMOD_ALT )
                key |= MD_ALT;
        }

#ifdef wxMOD_META
        if( mods & wxMOD_META )
            key |= MD_META;
#endif

#ifdef wxMOD_WIN
        if( mods & wxMOD_WIN )
            key |= MD_SUPER;
#endif

        return key;
    }
}
