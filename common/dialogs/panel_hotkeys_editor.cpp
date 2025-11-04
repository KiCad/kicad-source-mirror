/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <panel_hotkeys_editor.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <widgets/button_row_panel.h>
#include <widgets/ui_common.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>


/**
 * Helper function to add a filter box to a panel, with some
 * sensible defaults for that purpose.
 *
 * @param  aParent          The parent widget/panel
 * @param  aDescriptiveText The text to show when the box is empty.
 * @return                  A newly constructed filter box - the caller owns it
 */
static wxSearchCtrl* CreateTextFilterBox( wxWindow* aParent, const wxString& aDescriptiveText )
{
    wxSearchCtrl* search_widget = new wxSearchCtrl( aParent, wxID_ANY );

    search_widget->ShowSearchButton( false );
    search_widget->ShowCancelButton( true );

    search_widget->SetDescriptiveText( aDescriptiveText );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    search_widget->SetMinSize( wxSize( -1, aParent->GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    return search_widget;
}


PANEL_HOTKEYS_EDITOR::PANEL_HOTKEYS_EDITOR( EDA_BASE_FRAME* aFrame, wxWindow* aWindow ) :
        RESETTABLE_PANEL( aWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_frame( aFrame ),
        m_hotkeyStore()
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* bMargins = new wxBoxSizer( wxVERTICAL );

    m_filterSearch = CreateTextFilterBox( this, _( "Type filter text" ) );
    bMargins->Add( m_filterSearch, 0, wxEXPAND | wxTOP | wxRIGHT, 5 );

    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( this, m_hotkeyStore );
    bMargins->Add( m_hotkeyListCtrl, 1, wxEXPAND | wxTOP | wxRIGHT, 5 );

    m_bottomSizer = new wxBoxSizer( wxHORIZONTAL );
    installButtons( m_bottomSizer );

    bMargins->Add( m_bottomSizer, 0, wxEXPAND, 5 );
    mainSizer->Add( bMargins, 1, wxEXPAND, 0 );

#ifdef __WXGTK__

    // It appears that this may have been fixed in wxWidgets 3.2.3.
#if wxVERSION_NUMBER < 3203

    // Work around a bug that clips the text vertically in the wxSearchCtrl on GTK
    m_filterSearch->SetMinSize(
            wxSize( m_filterSearch->GetSize().x, int( m_filterSearch->GetSize().y * 1.6 ) ) );
#endif

#endif

    SetSizer( mainSizer );
    Layout();
    mainSizer->Fit( this );

    // Connect Events
    m_filterSearch->Bind( wxEVT_COMMAND_TEXT_UPDATED, &PANEL_HOTKEYS_EDITOR::OnFilterSearch, this );
}


PANEL_HOTKEYS_EDITOR::~PANEL_HOTKEYS_EDITOR()
{
    m_filterSearch->Unbind( wxEVT_COMMAND_TEXT_UPDATED, &PANEL_HOTKEYS_EDITOR::OnFilterSearch,
                            this );
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

    aSizer->Add( btnPanel.release(), 1, wxEXPAND | wxALL, KIUI::GetStdMargin() );
}


bool PANEL_HOTKEYS_EDITOR::TransferDataToWindow()
{
    m_hotkeyStore.Init( m_actions, true );

    if( !m_hotkeyListCtrl->TransferDataToControl() )
        return false;

    // we may have loaded a query from the saved dialog state, so make sure to run it
    if( !m_filterSearch->IsEmpty() )
        m_hotkeyListCtrl->ApplyFilterString( m_filterSearch->GetValue() );

    return true;
}


bool PANEL_HOTKEYS_EDITOR::TransferDataFromWindow()
{
    if( !m_hotkeyListCtrl->TransferDataFromControl() )
        return false;

    WriteHotKeyConfig( m_actions );

    return true;
}


void PANEL_HOTKEYS_EDITOR::OnFilterSearch( wxCommandEvent& aEvent )
{
    const auto searchStr = aEvent.GetString();
    m_hotkeyListCtrl->ApplyFilterString( searchStr );
}


void PANEL_HOTKEYS_EDITOR::ImportHotKeys()
{
    wxString filename = wxFileSelector( _( "Import Hotkeys File:" ), m_frame->GetMruPath(),
                                        wxEmptyString, FILEEXT::HotkeyFileExtension,
                                        FILEEXT::HotkeyFileWildcard(), wxFD_OPEN,
                                        wxGetTopLevelParent( this ) );

    if( filename.IsEmpty() )
        return;

    std::map<std::string, std::pair<int, int>> importedHotKeys;
    ReadHotKeyConfig( filename, importedHotKeys );
    m_frame->SetMruPath( wxFileName( filename ).GetPath() );

    // Overlay the imported hotkeys onto the hotkey store
    for( HOTKEY_SECTION& section: m_hotkeyStore.GetSections() )
    {
        for( HOTKEY& hotkey: section.m_HotKeys )
        {
            if( importedHotKeys.count( hotkey.m_Actions[ 0 ]->GetName() ) )
            {
                hotkey.m_EditKeycode    = importedHotKeys[ hotkey.m_Actions[ 0 ]->GetName() ].first;
                hotkey.m_EditKeycodeAlt = importedHotKeys[ hotkey.m_Actions[ 0 ]->GetName() ].second;
            }
        }
    }

    m_hotkeyListCtrl->TransferDataToControl();
}


void PANEL_HOTKEYS_EDITOR::dumpHotkeys()
{
    wxString filename = wxFileSelector( wxT( "Hotkeys File" ), m_frame->GetMruPath(),
                                        wxEmptyString, FILEEXT::TextFileExtension,
                                        FILEEXT::TextFileWildcard(),
                                        wxFD_SAVE, wxGetTopLevelParent( this ) );

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
            stream << wxT( "| " ) << hk.m_Actions[0]->GetFriendlyName() << endl;

            if( hk.m_EditKeycode > 0 )
            {
                stream << wxT( "  | kbd:[" ) << KeyNameFromKeyCode( hk.m_EditKeycode ) << ']'
                       << endl;
            }
            else
            {
                stream << wxT( "  |" ) << endl;
            }

            stream << wxT( "  | " ) << hk.m_Actions[0]->GetDescription() << endl;
        }

        stream << wxT( "|===" ) << endl << endl;
    }

    stream.Flush();
    fileStream.Close();
}
