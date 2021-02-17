/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <bom_plugins.h>
#include <confirm.h>
#include <dialog_bom_base.h>
#include <dialog_helpers.h>
#include <eeschema_settings.h>
#include <gestfich.h>
#include <dialogs/html_messagebox.h>
#include <i18n_utility.h> // for _HKI definition used in dialog_bom_help_md.h
#include <invoke_sch_dialog.h>
#include <kiface_i.h>
#include <netlist.h>
#include <netlist_exporter_xml.h>
#include <pgm_base.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <paths.h>

#include <dialogs/dialog_bom_cfg_lexer.h>

#include <wx/filedlg.h>
#include <wx/textdlg.h>

static constexpr wxChar BOM_TRACE[] = wxT( "BOM_GENERATORS" );

wxString s_bomHelpInfo =
#include <dialog_bom_help_md.h>
;

using namespace T_BOMCFG_T;     // for the BOM_CFG_PARSER parser and its keywords

// BOM "plugins" are not actually plugins. They are external tools
// (scripts or executables) called by this dialog.
typedef std::vector<BOM_GENERATOR_HANDLER::PTR> BOM_GENERATOR_ARRAY;


/**
 * Holds data and functions pertinent to parsing a S-expression file
 */
class BOM_CFG_PARSER : public DIALOG_BOM_CFG_LEXER
{
    BOM_GENERATOR_ARRAY* m_generatorsList;

public:
    BOM_CFG_PARSER( BOM_GENERATOR_ARRAY* aGenerators, const char* aData, const wxString& aSource );
    void Parse();

private:
    void parseGenerator();
};


BOM_CFG_PARSER::BOM_CFG_PARSER( BOM_GENERATOR_ARRAY* aGenerators, const char* aLine,
                                const wxString& aSource ) :
    DIALOG_BOM_CFG_LEXER( aLine, aSource )
{
    m_generatorsList = aGenerators;
}


void BOM_CFG_PARSER::Parse()
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_plugins )
            continue;

        switch( token )
        {
        case T_plugin:   // Defines a new plugin
            parseGenerator();
            break;

        default:
//            Unexpected( CurText() );
            break;
        }
    }
}


void BOM_CFG_PARSER::parseGenerator()
{
    NeedSYMBOLorNUMBER();
    wxString name = FromUTF8();
    auto plugin = std::make_unique<BOM_GENERATOR_HANDLER>( name );

    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
           break;

        switch( token )
        {
        case T_LEFT:
            break;

        case T_cmd:
            NeedSYMBOLorNUMBER();

            if( plugin )
                plugin->SetCommand( FromUTF8() );

            NeedRIGHT();
            break;

        case T_opts:
            NeedSYMBOLorNUMBER();

            if( plugin )
            {
                wxString option = FromUTF8();

                if( option.StartsWith( "nickname=", &name ) )
                    plugin->SetName( name );
                else
                    plugin->Options().Add( option );
            }

            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    if( plugin )
        m_generatorsList->push_back( std::move( plugin ) );
}


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
    BOM_GENERATOR_HANDLER* addGenerator( const wxString& aPath, const wxString& aName = wxEmptyString );
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
    m_buttonAddGenerator->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_buttonDelGenerator->SetBitmap( KiBitmap( small_trash_xpm ) );
    m_buttonEdit->SetBitmap( KiBitmap( small_edit_xpm ) );

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
}

DIALOG_BOM::~DIALOG_BOM()
{
    if( m_helpWindow )
        m_helpWindow->Destroy();

    // TODO(JE) maybe unpack this into JSON instead of sexpr

    // Save the plugin descriptions in config.
    // The config stores only one string, so we save the plugins inside a S-expr:
    // ( plugins
    //    ( plugin "plugin name 1" (cmd "command line 1") )
    //    ( plugin "plugin name 2" (cmd "command line 2") (opts "option1") (opts "option2") )
    //     ....
    // )

    STRING_FORMATTER writer;
    writer.Print( 0, "(plugins" );

    for( auto& plugin : m_generators )
    {
        writer.Print( 1, "(plugin %s (cmd %s)",
                      writer.Quotew( plugin->GetFile().GetFullPath() ).c_str(),
                      writer.Quotew( plugin->GetCommand() ).c_str() );

        for( unsigned jj = 0; jj < plugin->Options().GetCount(); jj++ )
        {
            writer.Print( 1, "(opts %s)",
                          writer.Quotew( plugin->Options().Item( jj ) ).c_str() );
        }

        if( !plugin->GetName().IsEmpty() )
        {
            wxString option = wxString::Format( "nickname=%s", plugin->GetName() );

            writer.Print( 1, "(opts %s)",
                          writer.Quotew( option ).c_str() );
        }

        writer.Print( 0, ")" );
    }

    writer.Print( 0, ")" );

    wxString list( FROM_UTF8( writer.GetString().c_str() ) );

    auto cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    cfg->m_BomPanel.plugins = list.ToStdString();
    cfg->m_BomPanel.selected_plugin = m_lbGenerators->GetStringSelection().ToStdString();
}


// Read the initialized plugins in config and fill the list of names
void DIALOG_BOM::installGeneratorsList()
{
    auto cfg = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    wxString list               = cfg->m_BomPanel.plugins;
    wxString active_plugin_name = cfg->m_BomPanel.selected_plugin;

    if( !list.IsEmpty() )
    {
        BOM_CFG_PARSER cfg_parser( &m_generators, TO_UTF8( list ), wxT( "plugins" ) );

        try
        {
            cfg_parser.Parse();
        }
        catch( const IO_ERROR& )
        {
//            wxLogMessage( ioe.What() );
        }
        catch( std::runtime_error& e )
        {
            DisplayError( nullptr, e.what() );
        }

        // Populate list box
        for( unsigned ii = 0; ii < m_generators.size(); ii++ )
        {
            if( !m_generators[ii]->GetFile().Exists( wxFILE_EXISTS_REGULAR ) )
                continue;

            m_lbGenerators->Append( m_generators[ii]->GetName() );

            if( active_plugin_name == m_generators[ii]->GetName() )
                m_lbGenerators->SetSelection( ii );
        }
    }

    if( m_generators.empty() ) // No plugins found?
    {
        // Load plugins from the default locations
        std::vector<wxString> pluginPaths = {
#if defined(__WXGTK__)
            "/usr/share/kicad/plugins",
            "/usr/local/share/kicad/plugins",
#elif defined(__WXMSW__)
            wxString::Format( "%s\\scripting\\plugins", Pgm().GetExecutablePath() ),
#elif defined(__WXMAC__)
            wxString::Format( "%s/plugins", PATHS::GetOSXKicadDataDir() ),
#endif
        };

        wxFileName pluginPath;

        for( const auto& path : pluginPaths )
        {
            wxDir dir( path );

            if( !dir.IsOpened() )
                continue;

            pluginPath.AssignDir( dir.GetName() );
            wxString fileName;
            bool cont = dir.GetFirst( &fileName, wxFileSelectorDefaultWildcardStr, wxDIR_FILES );

            while( cont )
            {
                try
                {
                    wxLogTrace( BOM_TRACE,"Checking if %s is a BOM generator", fileName );

                    if( BOM_GENERATOR_HANDLER::IsValidGenerator( fileName ) )
                    {
                        pluginPath.SetFullName( fileName );
                        addGenerator( pluginPath.GetFullPath() );
                    }
                }
                catch( ... ) { /* well, no big deal */ }

                cont = dir.GetNext( &fileName );
            }
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
    auto plugin = selectedGenerator();

    if( !plugin )
    {
        m_textCtrlName->SetValue( wxEmptyString );
        m_textCtrlCommand->SetValue( wxEmptyString );
        m_Messages->SetValue( wxEmptyString );
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

    fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );
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
        wxMessageBox( wxString::Format( _( "Nickname \"%s\" already in use." ), name ) );
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
    {
#ifndef __WXMAC__
        lastPath = Pgm().GetExecutablePath();
#else
        lastPath = PATHS::GetOSXKicadDataDir() + "/plugins";
#endif
    }

    wxString fullFileName = EDA_FILE_SELECTOR( _( "Generator files:" ), lastPath, wxEmptyString,
                                               wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                               this, wxFD_OPEN, true );

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

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, _( "Bill of Material Generation Help" ) );
    m_helpWindow->SetDialogSizeInDU( 500, 350 );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( s_bomHelpInfo ), html_txt );

    m_helpWindow->m_htmlWindow->AppendToPage( html_txt );
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
