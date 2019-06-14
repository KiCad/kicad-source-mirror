/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <panel_hotkeys_editor.h>
#include <kiway_player.h>
#include <wx/srchctrl.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <hotkeys_basic.h>
#include <widgets/button_row_panel.h>
#include <widgets/ui_common.h>
#include <tool/tool_manager.h>
#include <tool/tool_action.h>
#include <wx/tokenzr.h>
#include <gestfich.h>

static const wxSize default_dialog_size { 500, 350 };
static const wxSize min_dialog_size { -1, 350 };

/**
 * Helper function to add a filter box to a panel, with some
 * sensible defaults for that purpose.
 *
 * @param  aParent          The panrent widget/panel
 * @param  aDescriptiveText The text to show when the box is empty.
 * @return                  A newly constructed filter box - the caller owns it
 */
static wxSearchCtrl* CreateTextFilterBox( wxWindow* aParent, const wxString& aDescriptiveText )
{
    auto search_widget = new wxSearchCtrl( aParent, wxID_ANY );

    search_widget->ShowSearchButton( false );
    search_widget->ShowCancelButton( true );

    search_widget->SetDescriptiveText( aDescriptiveText);

    return search_widget;
}


PANEL_HOTKEYS_EDITOR::PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow,
                                            bool aReadOnly ) :
        wxPanel( aWindow, wxID_ANY, wxDefaultPosition, default_dialog_size ),
        m_frame( aFrame ),
        m_readOnly( aReadOnly ),
        m_hotkeyStore()
{
    const auto margin = KIUI::GetStdMargin();
    auto mainSizer = new wxBoxSizer( wxVERTICAL );

    // Sub-sizer for setting a wider side margin
    // TODO: Can this be set by the parent widget- doesn't seem to be
    // this panel's responsibility?
    const int side_margins = 10; // seems to be hardcoded in wxFB
    auto bMargins = new wxBoxSizer( wxVERTICAL );

    auto filterSearch = CreateTextFilterBox( this, _( "Type filter text" ) );
    bMargins->Add( filterSearch, 0, wxBOTTOM | wxEXPAND | wxTOP, margin );

    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( this, m_hotkeyStore, m_readOnly );
    bMargins->Add( m_hotkeyListCtrl, 1, wxALL | wxEXPAND, margin );

    if( !m_readOnly )
        installButtons( bMargins );

    mainSizer->Add( bMargins, 1, wxEXPAND | wxRIGHT | wxLEFT, side_margins );

    this->SetSizer( mainSizer );
    this->Layout();

    // Connect Events
    filterSearch->Bind( wxEVT_COMMAND_TEXT_UPDATED, &PANEL_HOTKEYS_EDITOR::OnFilterSearch, this );
}


void PANEL_HOTKEYS_EDITOR::AddHotKeys( TOOL_MANAGER* aToolMgr )
{
    m_toolManagers.push_back( aToolMgr );
}


void PANEL_HOTKEYS_EDITOR::installButtons( wxSizer* aSizer )
{
    const BUTTON_ROW_PANEL::BTN_DEF_LIST l_btn_defs = {
        {
            wxID_RESET,
            _( "Undo All Changes" ),
            _( "Undo all changes made so far in this dialog" ),
            [this]( wxCommandEvent& ){
                m_hotkeyListCtrl->ResetAllHotkeys( false );
            }
        },
        {
            wxID_ANY,
            _( "Restore All to Defaults" ),
            _( "Set all hotkeys to the built-in KiCad defaults" ),
            [this]( wxCommandEvent& ){
                m_hotkeyListCtrl->ResetAllHotkeys( true );
            }
        },
        {
            wxID_ANY,
            _( "Import Hotkeys..." ),
            _( "Import hotkey definitions from an external file, replacing the current values" ),
            [this]( wxCommandEvent& ){
                ImportHotKeys();
            }
        }
    };

    const BUTTON_ROW_PANEL::BTN_DEF_LIST r_btn_defs = {
    };

    auto btnPanel = std::make_unique<BUTTON_ROW_PANEL>( this, l_btn_defs, r_btn_defs );

    aSizer->Add( btnPanel.release(), 0, wxEXPAND | wxTOP, KIUI::GetStdMargin() );
}


bool PANEL_HOTKEYS_EDITOR::TransferDataToWindow()
{
    m_hotkeyStore.Init( m_toolManagers, m_readOnly );
    return m_hotkeyListCtrl->TransferDataToControl();
}


bool PANEL_HOTKEYS_EDITOR::TransferDataFromWindow()
{
    if( !m_hotkeyListCtrl->TransferDataFromControl() )
        return false;

    if( m_readOnly )
        return true;

    // save the hotkeys
    for( TOOL_MANAGER* toolMgr : m_toolManagers )
        WriteHotKeyConfig( toolMgr->GetActions() );

    return true;
}


void PANEL_HOTKEYS_EDITOR::OnFilterSearch( wxCommandEvent& aEvent )
{
    const auto searchStr = aEvent.GetString();
    m_hotkeyListCtrl->ApplyFilterString( searchStr );
}


void PANEL_HOTKEYS_EDITOR::ImportHotKeys()
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;
    wxString filename = EDA_FILE_SELECTOR( _( "Import Hotkeys File:" ), m_frame->GetMruPath(),
                                           wxEmptyString, ext, mask, this, wxFD_OPEN, true );

    if( filename.IsEmpty() )
        return;

    std::map<std::string, int> importedHotKeys;
    ReadHotKeyConfig( filename, importedHotKeys );
    m_frame->SetMruPath( wxFileName( filename ).GetPath() );

    // Overlay the imported hotkeys onto the hotkey store
    for( HOTKEY_SECTION& section: m_hotkeyStore.GetSections() )
    {
        for( HOTKEY& hotkey: section.m_HotKeys )
        {
            if( importedHotKeys.count( hotkey.m_Actions[ 0 ]->GetName() ) )
                hotkey.m_EditKeycode = importedHotKeys[ hotkey.m_Actions[ 0 ]->GetName() ];
        }
    }

    m_hotkeyListCtrl->TransferDataToControl();
}



