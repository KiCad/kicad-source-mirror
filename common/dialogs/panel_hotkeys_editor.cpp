/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <gestfich.h>
#include <hotkeys_basic.h>
#include <kiway_player.h>
#include <locale_io.h>
#include <panel_hotkeys_editor.h>
#include <tool/tool_manager.h>
#include <widgets/button_row_panel.h>
#include <widgets/ui_common.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

static const wxSize default_dialog_size { 500, 350 };

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
    wxSearchCtrl* search_widget = new wxSearchCtrl( aParent, wxID_ANY );

    search_widget->ShowSearchButton( false );
    search_widget->ShowCancelButton( true );

    search_widget->SetDescriptiveText( aDescriptiveText );

    return search_widget;
}


PANEL_HOTKEYS_EDITOR::PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow,
                                            bool aReadOnly ) :
        RESETTABLE_PANEL( aWindow, wxID_ANY, wxDefaultPosition, default_dialog_size ),
        m_frame( aFrame ),
        m_readOnly( aReadOnly ),
        m_hotkeyStore()
{
    const auto margin = KIUI::GetStdMargin();
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    const int side_margins = margin * 2;
    wxBoxSizer* bMargins = new wxBoxSizer( wxVERTICAL );

    wxSearchCtrl* filterSearch = CreateTextFilterBox( this, _( "Type filter text" ) );
    bMargins->Add( filterSearch, 0, wxALL | wxEXPAND, margin );

    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( this, m_hotkeyStore, m_readOnly );
    bMargins->Add( m_hotkeyListCtrl, 1, wxALL | wxEXPAND, margin );

    if( !m_readOnly )
        installButtons( bMargins );

    mainSizer->Add( bMargins, 1, wxEXPAND | wxRIGHT | wxLEFT, side_margins );

#ifdef __WXGTK__
    // Work around a bug that clips the text vertically in the wxSearchCtrl on GTK
    filterSearch->SetMinSize( wxSize( filterSearch->GetSize().x,
                                      int( filterSearch->GetSize().y * 1.6 ) ) );
#endif

    SetSizer( mainSizer );
    Layout();

    // Connect Events
    filterSearch->Bind( wxEVT_COMMAND_TEXT_UPDATED, &PANEL_HOTKEYS_EDITOR::OnFilterSearch, this );
}


void PANEL_HOTKEYS_EDITOR::AddHotKeys( TOOL_MANAGER* aToolMgr )
{
    m_toolManagers.push_back( aToolMgr );
}


void PANEL_HOTKEYS_EDITOR::ResetPanel()
{
    m_hotkeyListCtrl->ResetAllHotkeys( true );
}


void PANEL_HOTKEYS_EDITOR::installButtons( wxSizer* aSizer )
{
    BUTTON_ROW_PANEL::BTN_DEF_LIST l_btn_defs = {
        {
            wxID_RESET,
            _( "Undo All Changes" ),
            _( "Undo all changes made so far in this dialog" ),
            [this]( wxCommandEvent& )
            {
                m_hotkeyListCtrl->ResetAllHotkeys( false );
            }
        },
        {
            wxID_ANY,
            _( "Import Hotkeys..." ),
            _( "Import hotkey definitions from an external file, replacing the current values" ),
            [this]( wxCommandEvent& )
            {
                ImportHotKeys();
            }
        }
    };


    if( ADVANCED_CFG::GetCfg().m_HotkeysDumper )
    {
        // Add hotkeys dumper (does not need translation, it's a dev tool only)
        l_btn_defs.push_back( {
                wxID_ANY, wxT( "Dump Hotkeys" ), wxEmptyString,
                [this]( wxCommandEvent& )
                {
                    dumpHotkeys();
                }
            } );
    }

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


void PANEL_HOTKEYS_EDITOR::dumpHotkeys()
{
    wxString filename = EDA_FILE_SELECTOR( wxT( "Dump Hotkeys File:" ), m_frame->GetMruPath(),
                                           wxEmptyString, wxT( "txt" ), wxT( "*.txt" ), this,
                                           wxFD_SAVE, true );

    if( filename.IsEmpty() )
        return;

    wxFileName fn( filename );

    wxFFileOutputStream fileStream( fn.GetFullPath(), "w" );
    wxTextOutputStream stream( fileStream );

    if( !fn.IsDirWritable() || ( fn.Exists() && !fn.IsFileWritable() ) )
        return;

    for( HOTKEY_SECTION& section : m_hotkeyStore.GetSections() )
    {
        stream << wxT( "=== " ) << section.m_SectionName << endl << endl;

        stream << wxT( "[width=\"100%\",options=\"header\",cols=\"20%,15%,65%\"]" ) << endl;
        stream << wxT( "|===" ) << endl;
        stream << _( "| Action | Default Hotkey | Description" ) << endl;

        for( HOTKEY& hk : section.m_HotKeys )
        {
            stream << wxT( "| " ) << hk.m_Actions[0]->GetLabel() << endl;

            if( hk.m_EditKeycode > 0 )
                stream << wxT( "  | `" ) << KeyNameFromKeyCode( hk.m_EditKeycode ) << '`' << endl;
            else
                stream << wxT( "  |" ) << endl;

            stream << wxT( "  | " ) << hk.m_Actions[0]->GetDescription( false ) << endl;
        }

        stream << wxT( "|===" ) << endl << endl;
    }

    stream.Flush();
    fileStream.Close();
}
