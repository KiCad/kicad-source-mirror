/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <netlist.h>
#include <netlist_exporter_generic.h>
#include <invoke_sch_dialog.h>
#include <dialog_helpers.h>
#include <dialog_bom_base.h>
#include <html_messagebox.h>
#include <reporter.h>
#include <bom_plugins.h>
#include <make_unique.h>

static constexpr wxChar BOM_TRACE[] = wxT( "BOM_PLUGINS" );

static constexpr wxChar BOM_PLUGINS_KEY[] = wxT( "bom_plugins" );
static constexpr wxChar BOM_PLUGIN_SELECTED_KEY[] =  wxT( "bom_plugin_selected" );

static const char* s_bomHelpInfo =
#include <dialog_bom_help_html.h>
;

#include <dialog_bom_cfg_lexer.h>

using namespace T_BOMCFG_T;

typedef std::vector<BOM_PLUGIN::PTR> BOM_PLUGIN_ARRAY;


/**
 * Holds data and functions pertinent to parsing a S-expression file
 */
class BOM_CFG_PARSER : public DIALOG_BOM_CFG_LEXER
{
    BOM_PLUGIN_ARRAY* m_pluginsList;

public:
    BOM_CFG_PARSER( BOM_PLUGIN_ARRAY* aPlugins, const char* aData, const wxString& aSource );
    void Parse();

private:
    void parsePlugin();
};


BOM_CFG_PARSER::BOM_CFG_PARSER( BOM_PLUGIN_ARRAY* aPlugins, const char* aLine,
                                const wxString& aSource ) :
    DIALOG_BOM_CFG_LEXER( aLine, aSource )
{
    m_pluginsList = aPlugins;
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
            parsePlugin();
            break;

        default:
//            Unexpected( CurText() );
            break;
        }
    }
}


void BOM_CFG_PARSER::parsePlugin()
{
    NeedSYMBOLorNUMBER();
    wxString name = FromUTF8();
    auto plugin = std::make_unique<BOM_PLUGIN>( name );

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
        m_pluginsList->push_back( std::move( plugin ) );
}


// The main dialog frame to run scripts to build bom
class DIALOG_BOM : public DIALOG_BOM_BASE
{
private:
    SCH_EDIT_FRAME*   m_parent;
    BOM_PLUGIN_ARRAY  m_plugins;
    wxConfigBase*     m_config;         // to store the "plugins"
    bool              m_initialized;

public:
    DIALOG_BOM( SCH_EDIT_FRAME* parent );
    ~DIALOG_BOM();

private:
    void OnPluginSelected( wxCommandEvent& event ) override;
    void OnRunPlugin( wxCommandEvent& event ) override;
    void OnHelp( wxCommandEvent& event ) override;
    void OnAddPlugin( wxCommandEvent& event ) override;
    void OnRemovePlugin( wxCommandEvent& event ) override;
    void OnEditPlugin( wxCommandEvent& event ) override;
    void OnCommandLineEdited( wxCommandEvent& event ) override;
    void OnNameEdited( wxCommandEvent& event ) override;
    void OnShowConsoleChanged( wxCommandEvent& event ) override;
    void OnIdle( wxIdleEvent& event ) override;

    void pluginInit();
    void installPluginsList();
    BOM_PLUGIN* addPlugin( const wxString& aPath, const wxString& aName = wxEmptyString );
    bool pluginExists( const wxString& aName );

    BOM_PLUGIN* selectedPlugin()
    {
        int idx = m_lbPlugins->GetSelection();

        if( idx < 0 || idx >= (int)m_plugins.size() )
            return nullptr;

        return m_plugins[idx].get();
    }

    wxString choosePlugin();
};


// Create and show DIALOG_BOM.
int InvokeDialogCreateBOM( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_BOM dlg( aCaller );
    return dlg.ShowModal();
}


DIALOG_BOM::DIALOG_BOM( SCH_EDIT_FRAME* parent ) :
    DIALOG_BOM_BASE( parent )
{
    m_parent = parent;
    m_config = Kiface().KifaceSettings();
    m_initialized = false;

    m_buttonAddPlugin->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_buttonDelPlugin->SetBitmap( KiBitmap( trash_xpm ) );
    m_buttonEdit->SetBitmap( KiBitmap( small_edit_xpm ) );

    installPluginsList();

#ifndef __WINDOWS__
    m_checkBoxShowConsole->Show( false );
#endif

    m_sdbSizer1OK->SetLabel( _( "Generate" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    SetInitialFocus( m_lbPlugins );
    m_sdbSizer1OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}

DIALOG_BOM::~DIALOG_BOM()
{
    // Save the plugin descriptions in config.
    // The config stores only one string, so we save the plugins inside a S-expr:
    // ( plugins
    //    ( plugin "plugin name 1" (cmd "command line 1") )
    //    ( plugin "plugin name 2" (cmd "command line 2") (opts "option1") (opts "option2") )
    //     ....
    // )

    STRING_FORMATTER writer;
    writer.Print( 0, "(plugins" );

    for( auto& plugin : m_plugins )
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

    m_config->Write( BOM_PLUGINS_KEY, list );

    wxString active_plugin_name = m_lbPlugins->GetStringSelection( );
    m_config->Write( BOM_PLUGIN_SELECTED_KEY, active_plugin_name );
}


// Read the initialized plugins in config and fill the list of names
void DIALOG_BOM::installPluginsList()
{
    wxString list, active_plugin_name;
    m_config->Read( BOM_PLUGINS_KEY, &list );
    m_config->Read( BOM_PLUGIN_SELECTED_KEY, &active_plugin_name );

    if( !list.IsEmpty() )
    {
        BOM_CFG_PARSER cfg_parser( &m_plugins, TO_UTF8( list ), wxT( "plugins" ) );

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
        for( unsigned ii = 0; ii < m_plugins.size(); ii++ )
        {
            m_lbPlugins->Append( m_plugins[ii]->GetName() );

            if( active_plugin_name == m_plugins[ii]->GetName() )
                m_lbPlugins->SetSelection( ii );
        }
    }

    if( m_plugins.empty() ) // No plugins found?
    {
        // Load plugins from the default locations
        std::vector<wxString> pluginPaths = {
#if defined(__WXGTK__)
            "/usr/share/kicad/plugins",
            "/usr/local/share/kicad/plugins",
#elif defined(__WXMSW__)
            wxString::Format( "%s\\scripting\\plugins", Pgm().GetExecutablePath() ),
#elif defined(__WXMAC__)
            wxString::Format( "%s/plugins", GetOSXKicadDataDir() ),
#endif
        };

        wxFileName pluginPath;

        for( const auto& path : pluginPaths )
        {
            wxLogDebug( wxString::Format( "Searching directory %s for BOM plugins", path ) );
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
                    wxLogTrace( BOM_TRACE, wxString::Format( "Checking if %s is a BOM plugin", fileName ) );

                    if( BOM_PLUGIN::IsPlugin( fileName ) )
                    {
                        pluginPath.SetFullName( fileName );
                        addPlugin( pluginPath.GetFullPath() );
                    }
                }
                catch( ... ) { /* well, no big deal */ }

                cont = dir.GetNext( &fileName );
            }
        }
    }


    pluginInit();
}


BOM_PLUGIN* DIALOG_BOM::addPlugin( const wxString& aPath, const wxString& aName )
{
    BOM_PLUGIN* ret = nullptr;
    auto plugin = std::make_unique<BOM_PLUGIN>( aPath );

    if( !plugin )
        return nullptr;

    if( !aName.IsEmpty() )
    {
        plugin->SetName( aName );
        m_lbPlugins->Append( aName );
    }
    else
    {
        m_lbPlugins->Append( plugin->GetName() );
    }

    ret = plugin.get();
    m_plugins.push_back( std::move( plugin ) );
    return ret;
}


bool DIALOG_BOM::pluginExists( const wxString& aName )
{
    for( unsigned ii = 0; ii < m_plugins.size(); ii++ )
    {
        if( aName == m_plugins[ii]->GetName() )
            return true;
    }

    return false;
}


void DIALOG_BOM::OnPluginSelected( wxCommandEvent& event )
{
    pluginInit();
}


void DIALOG_BOM::pluginInit()
{
    auto plugin = selectedPlugin();

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
}


void DIALOG_BOM::OnRunPlugin( wxCommandEvent& event )
{
    // Calculate the xml netlist filename
    wxFileName fn = g_RootSheet->GetScreen()->GetFileName();

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

    m_parent->CreateNetlist( -1, fullfilename, 0, &reporter, false );

    m_Messages->SetValue( reportmsg );
}


void DIALOG_BOM::OnRemovePlugin( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_lbPlugins->Delete( ii );
    m_plugins.erase( m_plugins.begin() + ii );

    // Select the next item, if exists
    if( m_lbPlugins->GetCount() )
        m_lbPlugins->SetSelection( std::min( ii, (int) m_lbPlugins->GetCount() - 1 ) );

    pluginInit();
}


void DIALOG_BOM::OnAddPlugin( wxCommandEvent& event )
{
    wxString filename = choosePlugin();

    if( filename.IsEmpty() )
        return;

    // Creates a new plugin entry
    wxFileName fn( filename );
    wxString name = wxGetTextFromUser( _( "Plugin nickname:" ), _( "Add Plugin" ),
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
        auto plugin = addPlugin( fn.GetFullPath(), name );

        if( plugin )
        {
            m_lbPlugins->SetSelection( m_lbPlugins->GetCount() - 1 );
            m_textCtrlCommand->SetValue( plugin->GetCommand() );
            pluginInit();
        }
    }
    catch( const std::runtime_error& e )
    {
        DisplayError( this, e.what() );
    }
}


wxString DIALOG_BOM::choosePlugin()
{
    static wxString lastPath;

    if( lastPath.IsEmpty() )
    {
#ifndef __WXMAC__
        lastPath = Pgm().GetExecutablePath();
#else
        lastPath = GetOSXKicadDataDir() + "/plugins";
#endif
    }

    wxString fullFileName = EDA_FILE_SELECTOR( _( "Plugin files:" ), lastPath, wxEmptyString,
                                               wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                               this, wxFD_OPEN, true );

    return fullFileName;
}


void DIALOG_BOM::OnEditPlugin( wxCommandEvent& event )
{
    auto plugin = selectedPlugin();

    if( !plugin )
        return;

    wxString pluginFile = plugin->GetFile().GetFullPath();

    if( pluginFile.Length() <= 2 )      // if name != ""
    {
        wxMessageBox( _( "Plugin file name not found." ) );
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
    HTML_MESSAGE_BOX help_Dlg( this, _( "Bill of Material Generation Help" ) );
    help_Dlg.SetDialogSizeInDU( 500, 350 );

    wxString msg = FROM_UTF8( s_bomHelpInfo );
    help_Dlg.m_htmlWindow->AppendToPage( msg );
    help_Dlg.ShowModal();
}


void DIALOG_BOM::OnCommandLineEdited( wxCommandEvent& event )
{
    auto plugin = selectedPlugin();

    if( plugin )
        plugin->SetCommand( m_textCtrlCommand->GetValue() );
}


void DIALOG_BOM::OnNameEdited( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_plugins[ii]->SetName( m_textCtrlName->GetValue() );
    m_lbPlugins->SetString( ii, m_plugins[ii]->GetName() );
}


void DIALOG_BOM::OnShowConsoleChanged( wxCommandEvent& event )
{
#ifdef __WINDOWS__
    static constexpr wxChar OPT_SHOW_CONSOLE[] = wxT( "show_console" );

    auto plugin = selectedPlugin();

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
