/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <bom_plugins.h>
#include <confirm.h>
#include <dialog_bom_base.h>
#include <string_utils.h>
#include <eeschema_settings.h>
#include <gestfich.h>
#include <dialogs/html_message_box.h>
#include <i18n_utility.h> // for _HKI definition used in dialog_bom_help_md.h
#include <invoke_sch_dialog.h>
#include <kiface_base.h>
#include <netlist_exporter_xml.h>
#include <pgm_base.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <paths.h>

#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/textdlg.h>

wxString s_bomHelpInfo =
#include <dialog_bom_help_md.h>
;

// BOM "plugins" are not actually plugins. They are external tools
// (scripts or executables) called by this dialog.
typedef std::vector< std::unique_ptr<BOM_GENERATOR_HANDLER> > BOM_GENERATOR_ARRAY;


// The main dialog frame to run scripts to build bom
class DIALOG_BOM : public DIALOG_BOM_BASE
{
private:
    SCH_EDIT_FRAME*     m_parent;
    BOM_GENERATOR_ARRAY m_generators;
    bool                m_initialized;

    HTML_MESSAGE_BOX*   m_helpWindow;

public:
    DIALOG_BOM( SCH_EDIT_FRAME* parent );
    ~DIALOG_BOM();

private:
    void OnGeneratorSelected( wxCommandEvent& event ) override;
    void OnRunGenerator( wxCommandEvent& event ) override;
    void OnHelp( wxCommandEvent& event ) override;
    void OnAddGenerator( wxCommandEvent& event ) override;
    void OnRemoveGenerator( wxCommandEvent& event ) override;
    void OnEditGenerator( wxCommandEvent& event ) override;
    void OnCommandLineEdited( wxCommandEvent& event ) override;
    void OnNameEdited( wxCommandEvent& event ) override;
    void OnShowConsoleChanged( wxCommandEvent& event ) override;
    void OnIdle( wxIdleEvent& event ) override;

    void pluginInit();
    void installGeneratorsList();
    BOM_GENERATOR_HANDLER* addGenerator( const wxString& aPath,
                                         const wxString& aName = wxEmptyString );
    bool pluginExists( const wxString& aName );

    BOM_GENERATOR_HANDLER* selectedGenerator()
    {
        int idx = m_lbGenerators->GetSelection();

        if( idx < 0 || idx >= (int)m_generators.size() )
            return nullptr;

        return m_generators[idx].get();
    }

    wxString chooseGenerator();
};


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
    m_buttonAddGenerator->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_buttonDelGenerator->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_buttonEdit->SetBitmap( KiBitmap( BITMAPS::small_edit ) );

    installGeneratorsList();

#ifndef __WINDOWS__
    m_checkBoxShowConsole->Show( false );
#endif

    m_sdbSizerOK->SetLabel( _( "Generate" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizer->Layout();

    SetInitialFocus( m_lbGenerators );
    m_sdbSizerOK->SetDefault();

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
                wxLogTrace( BOM_TRACE, "BOM plugin %s not found",
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

    if( !plugin )
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

    wxString reportmsg;
    WX_STRING_REPORTER reporter( &reportmsg );
    m_parent->SetNetListerCommand( m_textCtrlCommand->GetValue() );

#ifdef __WINDOWS__
    if( m_checkBoxShowConsole->IsChecked() )
        m_parent->SetExecFlags( wxEXEC_SHOW_CONSOLE );
#endif

    if( m_parent->ReadyToNetlist( _( "Generating BOM requires a fully annotated schematic." ) ) )
        m_parent->WriteNetListFile( -1, fullfilename, GNL_OPT_BOM, &reporter );

    m_Messages->SetValue( reportmsg );

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
        DisplayError( this, e.what() );
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

    AddDelimiterString( pluginFile );
    wxString editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, pluginFile );
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

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, this, _( "Bill of Material Generation Help" ) );
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
