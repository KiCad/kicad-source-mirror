/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file eeschema/dialogs/dialog_bom.cpp
 * @brief Dialog box for creating bom and other documents from generic netlist.
 */


#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <bom_plugins.h>
#include <confirm.h>
#include <dialog_bom.h>
#include <dialogs/html_message_box.h>
#include <eeschema_settings.h>
#include <gestfich.h>
#include <i18n_utility.h> // for _HKI definition used in dialog_bom_help_md.h
#include <invoke_sch_dialog.h>
#include <kiface_base.h>
#include <netlist.h>
#include <netlist_exporter_xml.h>
#include <paths.h>
#include <pgm_base.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <string_utils.h>

#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

wxString s_bomHelpInfo =
#include <dialog_bom_help_md.h>
;

// Create and show DIALOG_BOM.
int InvokeDialogCreateBOM( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_BOM dlg( aCaller );

    // QuasiModal so syntax help works
    return dlg.ShowQuasiModal();
}


DIALOG_BOM::DIALOG_BOM( SCH_EDIT_FRAME* parent ) :
        DIALOG_BOM_BASE( parent ),
        m_parent( parent ),
        m_initialized( false ),
        m_helpWindow( nullptr )
{
    m_buttonAddGenerator->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_buttonDelGenerator->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_buttonEdit->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    installGeneratorsList();

#ifndef __WINDOWS__
    m_checkBoxShowConsole->Show( false );
#endif

    SetupStandardButtons( { { wxID_OK,     _( "Generate" ) },
                            { wxID_CANCEL, _( "Close" )    } } );

    SetInitialFocus( m_lbGenerators );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    m_buttonReset->Bind( wxEVT_BUTTON,
            [&]( wxCommandEvent& )
            {
                EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

                cfg->m_BomPanel.selected_plugin = wxEmptyString;
                cfg->m_BomPanel.plugins         = cfg->DefaultBomPlugins();

                installGeneratorsList();
            } );
}


DIALOG_BOM::~DIALOG_BOM()
{
    if( m_helpWindow )
        m_helpWindow->Destroy();

    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    cfg->m_BomPanel.plugins.clear();

    for( const std::unique_ptr<BOM_GENERATOR_HANDLER>& plugin : m_generators )
    {
        wxString   name = plugin->GetName();
        wxFileName path( plugin->GetStoredPath() );

        // handle empty nickname by stripping path
        if( name.IsEmpty() )
            name = path.GetName();

        EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS setting( name, path.GetFullPath() );
        setting.command = plugin->GetCommand();

        cfg->m_BomPanel.plugins.emplace_back( setting );
    }

    cfg->m_BomPanel.selected_plugin = m_lbGenerators->GetStringSelection().ToStdString();
}


// Read the initialized plugins in config and fill the list of names
void DIALOG_BOM::installGeneratorsList()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    wxString active_plugin_name = cfg->m_BomPanel.selected_plugin;

    m_generators.clear();

    for( EESCHEMA_SETTINGS::BOM_PLUGIN_SETTINGS& setting : cfg->m_BomPanel.plugins )
    {
        auto plugin = std::make_unique<BOM_GENERATOR_HANDLER>( setting.path );

        plugin->SetName( setting.name );

        if( !setting.command.IsEmpty() )
            plugin->SetCommand( setting.command );

        m_generators.emplace_back( std::move( plugin ) );
    }

    m_lbGenerators->Clear();

    if( !m_generators.empty() )
    {
        for( unsigned ii = 0; ii < m_generators.size(); ii++ )
        {
            wxString name = m_generators[ii]->GetName();

            if( !m_generators[ii]->FindFilePath().Exists( wxFILE_EXISTS_REGULAR ) )
            {
                wxLogTrace( BOM_TRACE, wxS( "BOM plugin %s not found" ),
                            m_generators[ii]->FindFilePath().GetFullName() );
                name.Append( wxT( " " ) + _( "(file missing)" ) );

                if( active_plugin_name == name )
                    active_plugin_name.Clear();
            }

            m_lbGenerators->Append( name );

            if( active_plugin_name == name )
                m_lbGenerators->SetSelection( ii );
        }
    }

    pluginInit();
}


BOM_GENERATOR_HANDLER* DIALOG_BOM::addGenerator( const wxString& aPath, const wxString& aName )
{
    BOM_GENERATOR_HANDLER* ret = nullptr;
    auto plugin = std::make_unique<BOM_GENERATOR_HANDLER>( aPath );

    if( !plugin->IsOk() )
        return nullptr;

    if( !aName.IsEmpty() )
    {
        plugin->SetName( aName );
        m_lbGenerators->Append( aName );
    }
    else
    {
        m_lbGenerators->Append( plugin->GetName() );
    }

    ret = plugin.get();
    m_generators.push_back( std::move( plugin ) );
    return ret;
}


bool DIALOG_BOM::pluginExists( const wxString& aName )
{
    for( unsigned ii = 0; ii < m_generators.size(); ii++ )
    {
        if( aName == m_generators[ii]->GetName() )
            return true;
    }

    return false;
}


void DIALOG_BOM::OnGeneratorSelected( wxCommandEvent& event )
{
    pluginInit();
}


void DIALOG_BOM::pluginInit()
{
    BOM_GENERATOR_HANDLER* plugin = selectedGenerator();

    if( !plugin )
    {
        m_textCtrlName->SetValue( wxEmptyString );
        m_textCtrlCommand->SetValue( wxEmptyString );
        m_Messages->SetValue( wxEmptyString );
        return;
    }

    if( !plugin->FindFilePath().Exists( wxFILE_EXISTS_REGULAR ) )
    {
        m_textCtrlName->SetValue( wxEmptyString );
        m_textCtrlCommand->SetValue( wxEmptyString );

        wxString msg =
                wxString::Format( _( "The selected BOM generator script %s could not be found." ),
                                  plugin->GetFile().GetFullPath() );

        if( !plugin->GetFile().IsAbsolute() )
        {
            msg.Append( wxString::Format( _( "\n\nSearched:\n\t%s\n\t%s" ),
                                          PATHS::GetUserPluginsPath(),
                                          PATHS::GetStockPluginsPath() ) );
        }

        m_Messages->SetValue( msg );
        return;
    }

    m_textCtrlName->SetValue( plugin->GetName() );
    m_textCtrlCommand->SetValue( plugin->GetCommand() );
    m_Messages->SetValue( plugin->GetInfo() );
    m_Messages->SetSelection( 0, 0 );

#ifdef __WINDOWS__
    if( plugin->Options().Index( wxT( "show_console" ) ) == wxNOT_FOUND )
        m_checkBoxShowConsole->SetValue( false );
    else
        m_checkBoxShowConsole->SetValue( true );
#endif

    // A plugin can be not working, so do not left the OK button enabled if
    // the plugin is not ready to use
    m_sdbSizerOK->Enable( plugin->IsOk() );
}


void DIALOG_BOM::OnRunGenerator( wxCommandEvent& event )
{
    // Calculate the xml netlist filename
    wxFileName fn = m_parent->Schematic().GetFileName();

    fn.ClearExt();

    wxString fullfilename = fn.GetFullPath();
    m_parent->ClearMsgPanel();

    WX_STRING_REPORTER reporter;
    m_parent->SetNetListerCommand( m_textCtrlCommand->GetValue() );

#ifdef __WINDOWS__
    if( m_checkBoxShowConsole->IsChecked() )
        m_parent->SetExecFlags( wxEXEC_SHOW_CONSOLE );
#endif

    bool status = false;

    if( m_parent->ReadyToNetlist( _( "Generating BOM requires a fully annotated schematic." ) ) )
        status = m_parent->WriteNetListFile( NET_TYPE_BOM, fullfilename,
                                             GNL_OPT_BOM | GNL_ALL, &reporter );

    if( !status )
        DisplayErrorMessage( this, _( "Failed to create file." ) );

    m_Messages->SetValue( reporter.GetMessages() );

    // Force focus back on the dialog
    SetFocus();
}


void DIALOG_BOM::OnRemoveGenerator( wxCommandEvent& event )
{
    int ii = m_lbGenerators->GetSelection();

    if( ii < 0 )
        return;

    m_lbGenerators->Delete( ii );
    m_generators.erase( m_generators.begin() + ii );

    // Select the next item, if exists
    if( m_lbGenerators->GetCount() )
        m_lbGenerators->SetSelection( std::min( ii, (int) m_lbGenerators->GetCount() - 1 ) );

    pluginInit();
}


void DIALOG_BOM::OnAddGenerator( wxCommandEvent& event )
{
    wxString filename = chooseGenerator();

    if( filename.IsEmpty() )
        return;

    // Creates a new plugin entry
    wxFileName fn( filename );
    wxString name = wxGetTextFromUser( _( "Generator nickname:" ), _( "Add Generator" ),
                                       fn.GetName(), this );

    if( name.IsEmpty() )
        return;

    // Verify if it does not exists
    if( pluginExists( name ) )
    {
        wxMessageBox( wxString::Format( _( "Nickname '%s' already in use." ), name ) );
        return;
    }

    try
    {
        auto plugin = addGenerator( fn.GetFullPath(), name );

        if( plugin )
        {
            m_lbGenerators->SetSelection( m_lbGenerators->GetCount() - 1 );
            m_textCtrlCommand->SetValue( plugin->GetCommand() );
            pluginInit();
        }
    }
    catch( const std::runtime_error& e )
    {
        DisplayErrorMessage( this, e.what() );
    }
}


wxString DIALOG_BOM::chooseGenerator()
{
    static wxString lastPath;

    if( lastPath.IsEmpty() )
        lastPath = PATHS::GetUserPluginsPath();

    wxString fullFileName = wxFileSelector( _( "Generator File" ), lastPath, wxEmptyString,
                                            wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                            wxFD_OPEN, this );

    return fullFileName;
}


void DIALOG_BOM::OnEditGenerator( wxCommandEvent& event )
{
    auto plugin = selectedGenerator();

    if( !plugin )
        return;

    wxString pluginFile = plugin->GetFile().GetFullPath();

    if( pluginFile.Length() <= 2 )      // if name != ""
    {
        wxMessageBox( _( "Generator file name not found." ) );
        return;
    }

    wxString editorname = Pgm().GetTextEditor();

    if( !editorname.IsEmpty() )
        ExecuteFile( editorname, pluginFile );
    else
        wxMessageBox( _( "No text editor selected in KiCad.  Please choose one." ) );
}


void DIALOG_BOM::OnHelp( wxCommandEvent& event )
{
    if( m_helpWindow )
    {
        m_helpWindow->ShowModeless();
        return;
    }

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, _( "Bill of Materials Generation Help" ) );
    m_helpWindow->SetDialogSizeInDU( 500, 350 );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( s_bomHelpInfo ), html_txt );

    m_helpWindow->AddHTML_Text( html_txt );
    m_helpWindow->ShowModeless();
}


void DIALOG_BOM::OnCommandLineEdited( wxCommandEvent& event )
{
    auto generator = selectedGenerator();

    if( generator )
        generator->SetCommand( m_textCtrlCommand->GetValue() );
}


void DIALOG_BOM::OnNameEdited( wxCommandEvent& event )
{
    if( m_textCtrlName->GetValue().IsEmpty() )
        return;

    int ii = m_lbGenerators->GetSelection();

    if( ii < 0 )
        return;

    m_generators[ii]->SetName( m_textCtrlName->GetValue() );
    m_lbGenerators->SetString( ii, m_generators[ii]->GetName() );
}


void DIALOG_BOM::OnShowConsoleChanged( wxCommandEvent& event )
{
#ifdef __WINDOWS__
    static constexpr wxChar OPT_SHOW_CONSOLE[] = wxT( "show_console" );

    auto plugin = selectedGenerator();

    if( !plugin )
        return;

    if( m_checkBoxShowConsole->IsChecked() )
    {
        if( plugin->Options().Index( OPT_SHOW_CONSOLE ) == wxNOT_FOUND )
            plugin->Options().Add( OPT_SHOW_CONSOLE );
    }
    else
    {
        plugin->Options().Remove( OPT_SHOW_CONSOLE );
    }
#endif
}


void DIALOG_BOM::OnIdle( wxIdleEvent& event )
{
    // On some platforms we initialize wxTextCtrls to all-selected, but we don't want that
    // for the messages text box.
    if( !m_initialized )
    {
        m_Messages->SetSelection( 0, 0 );
        m_initialized = true;
    }
}


BOM_GENERATOR_HANDLER* DIALOG_BOM::selectedGenerator()
{
    int idx = m_lbGenerators->GetSelection();

    if( idx < 0 || idx >= (int) m_generators.size() )
        return nullptr;

    return m_generators[idx].get();
}
