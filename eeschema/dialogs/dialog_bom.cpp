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
#include <wx/ffile.h>
#include <netlist.h>
#include <netlist_exporter_generic.h>
#include <invoke_sch_dialog.h>
#include <dialog_helpers.h>
#include <dialog_bom_base.h>
#include <html_messagebox.h>
#include <reporter.h>

#define BOM_PLUGINS_KEY wxT("bom_plugins")
#define BOM_PLUGIN_SELECTED_KEY wxT("bom_plugin_selected")

const char * s_bomHelpInfo =
#include <dialog_bom_help_html.h>
;

#include <dialog_bom_cfg_lexer.h>

using namespace T_BOMCFG_T;

/**
 * Structure BOM_PLUGIN
 * holds data of the BOM plugin.
 */
struct BOM_PLUGIN
{
    wxString Name;
    wxString Command;
    wxArrayString Options;
};

/**
 * Define wxArray of BOM_PLUGIN.
 */
WX_DECLARE_OBJARRAY( BOM_PLUGIN, BOM_PLUGIN_ARRAY );
#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY( BOM_PLUGIN_ARRAY )

/**
 * Class BOM_CFG_READER_PARSER
 * holds data and functions pertinent to parsing a S-expression file
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
    BOM_PLUGIN plugin;

    NeedSYMBOLorNUMBER();
    plugin.Name = FromUTF8();

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
            plugin.Command = FromUTF8();
            NeedRIGHT();
            break;

        case T_opts:
            NeedSYMBOLorNUMBER();
            plugin.Options.Add( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    if( ! plugin.Name.IsEmpty() )
        m_pluginsList->Add( plugin );
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

    wxString getPluginFileName( const wxString& aCommand );

    /*
     * Display the text found between the keyword @package (compatible with doxygen comments)
     * and the end of comment block (""" in python, --> in xml)
     */
    void displayPluginInfo( FILE * aFile, const wxString& aFilename );

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

    m_buttonAddPlugin->SetBitmap( KiBitmap( plus_xpm ) );
    m_buttonDelPlugin->SetBitmap( KiBitmap( delete_xpm ) );
    m_buttonEdit->SetBitmap( KiBitmap( edit_xpm ) );

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

    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii++ )
    {
        writer.Print( 1, "(plugin %s (cmd %s)",
                      writer.Quotew( m_plugins.Item( ii ).Name ).c_str(),
                      writer.Quotew( m_plugins.Item( ii ).Command ).c_str() );

        for( unsigned jj = 0; jj < m_plugins.Item( ii ).Options.GetCount(); jj++ )
        {
            writer.Print( 1, "(opts %s)",
                          writer.Quotew( m_plugins.Item( ii ).Options.Item( jj ) ).c_str() );
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
    }

    // Populate list box
    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii++ )
    {
        m_lbPlugins->Append( m_plugins.Item( ii ).Name );

        if( active_plugin_name == m_plugins.Item( ii ).Name )
            m_lbPlugins->SetSelection( ii );
    }

    pluginInit();
}


void DIALOG_BOM::OnPluginSelected( wxCommandEvent& event )
{
    pluginInit();
}


void DIALOG_BOM::pluginInit()
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
    {
        m_textCtrlName->SetValue( wxEmptyString );
        m_textCtrlCommand->SetValue( wxEmptyString );
        return;
    }

    m_textCtrlName->SetValue( m_plugins.Item( ii ).Name );
    m_textCtrlCommand->SetValue( m_plugins.Item( ii ).Command );

#ifdef __WINDOWS__
    if( m_plugins.Item( ii ).Options.Index( wxT( "show_console" ) ) == wxNOT_FOUND )
        m_checkBoxShowConsole->SetValue( false );
    else
        m_checkBoxShowConsole->SetValue( true );
#endif

    wxString pluginName = getPluginFileName( m_textCtrlCommand->GetValue() );

    if( pluginName.IsEmpty() )
        return;

    FILE* pluginFile = wxFopen( pluginName, "rt" );

    if( !pluginFile )
    {
        DisplayError( this, wxString::Format( _( "Failed to open file \"%s\"." ), pluginName ) );
        return;
    }

    displayPluginInfo( pluginFile, pluginName );
}


void DIALOG_BOM::displayPluginInfo( FILE * aFile, const wxString& aFilename )
{
    m_Messages->Clear();

    // Display the text found between the keyword @package (compatible with doxygen comments)
    // and the end of comment block (""" in python, --> in xml)

    wxString data;
    wxFFile fdata( aFile );        // dtor will close the file

    if( !fdata.ReadAll( &data ) )
        return;

    wxString header( wxT( "@package" ) );
    wxString endsection;

    wxFileName fn( aFilename );

    if( fn.GetExt().IsSameAs( wxT( "py" ), false ) )
        endsection = wxT( "\"\"\"" );
    else if( fn.GetExt().IsSameAs( wxT( "xsl" ), false ) )
        endsection = wxT( "-->" );
    else
        // If this is not a python or xsl file, then we don't know how to find the info
        return;

    // Extract substring between @package and endsection
    int strstart = data.Find( header );

    if( strstart == wxNOT_FOUND )
        return;

    strstart += header.Length();
    int strend = data.find( endsection, strstart );

    if( strend == wxNOT_FOUND)
        return;

    // Remove empty line if any
    while( data[strstart] < ' ' )
            strstart++;

    m_Messages->SetValue( data.SubString( strstart, strend-1 ) );
    m_Messages->SetSelection( 0, 0 );
}


// run the plugin command line
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

    m_plugins.RemoveAt( ii );

    // Select the next item, if exists
    if( m_lbPlugins->GetCount() )
        m_lbPlugins->SetSelection( std::min( ii, (int) m_lbPlugins->GetCount() - 1 ) );

    pluginInit();
}


void DIALOG_BOM::OnAddPlugin( wxCommandEvent& event )
{
    wxString cmdLine = choosePlugin();
    BOM_PLUGIN newPlugin;

    if( cmdLine.IsEmpty() )
        return;

    // Creates a new plugin entry
    wxFileName fn( getPluginFileName( cmdLine ) );
    wxString name = wxGetTextFromUser( _("Plugin nickname:" ), _("Add Plugin"), fn.GetName(), this );

    if( name.IsEmpty() )
        return;

    // Verify if it does not exists
    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii++ )
    {
        if( name == m_plugins.Item( ii ).Name )
        {
            wxMessageBox( wxString::Format( _("Nickname \"%s\" already in use."), name ) );
            return;
        }
    }

    // Append the new plugin
    newPlugin.Name = name;
    newPlugin.Command = wxEmptyString;
    m_plugins.Add( newPlugin );
    m_lbPlugins->Append( name );
    m_lbPlugins->SetSelection( m_lbPlugins->GetCount() - 1 );
    m_textCtrlCommand->SetValue( cmdLine );

    pluginInit();
}

/*
 * Browse plugin files, and set m_CommandStringCtrl field
 */
wxString DIALOG_BOM::choosePlugin()
{
    wxString mask = wxT( "*" );
#ifndef __WXMAC__
    wxString path = Pgm().GetExecutablePath();
#else
    wxString path = GetOSXKicadDataDir() + wxT( "/plugins" );
#endif

    wxString fullFileName = EDA_FILE_SELECTOR( _( "Plugin files:" ),
                                               path,
                                               wxEmptyString,
                                               wxEmptyString,
                                               mask,
                                               this,
                                               wxFD_OPEN,
                                               true );
    if( fullFileName.IsEmpty() )
        return wxEmptyString;

    // Creates a default command line,
    // suitable to run the external tool xslproc or python
    // The default command line depending on plugin extension, currently
    // "xsl" or "exe" or "py" or "pyw"
    wxString    cmdLine;
    wxFileName  fn( fullFileName );
    wxString    ext = fn.GetExt();

    // Python requires on Windows the path of the script ends by '/' instead of '\'
    // otherwise import does not find modules in the same folder as the python script
    // We cannot change all '\' to '/' in command line because it causes issues with files
    // that are stored on network resources and pointed to using the Windows
    // Universal Naming Convention format. (full filename starting by \\server\)
    //
    // I hope changing the last separator only to '/' will work.
#ifdef __WINDOWS__
    if( ext == wxT("py" ) || ext == wxT("pyw" ) )
    {
        wxString sc_path = fn.GetPathWithSep();
        sc_path.RemoveLast();
        fullFileName = sc_path +'/' + fn.GetFullName();;
    }
#endif

    if( ext == "xsl" )
        cmdLine.Printf( "xsltproc -o \"%%O\" \"%s\" \"%%I\"", GetChars( fullFileName ) );
    else if( ext == "exe" )
        cmdLine.Printf( "\"%s\" < \"%%I\" > \"%%O\"", GetChars( fullFileName ) );
    else if( ext == "py" )
        cmdLine.Printf( "python \"%s\" \"%%I\" \"%%O\"", GetChars( fullFileName ) );
    else if( ext == "pyw" )
#ifdef __WINDOWS__
        cmdLine.Printf( "pythonw \"%s\" \"%%I\" \"%%O\"", GetChars( fullFileName ) );
#else
        cmdLine.Printf( "python \"%s\" \"%%I\" \"%%O\"", GetChars( fullFileName ) );
#endif
    else
        cmdLine.Printf( "\"%s\"", GetChars( fullFileName ) );

    return cmdLine;
}


wxString DIALOG_BOM::getPluginFileName(  const wxString& aCommand )
{
    wxString pluginName;

    // Try to find the plugin name.
    // This is possible if the name ends by .py or .pyw or .xsl or .exe
    int pos = -1;

    if( (pos = aCommand.Find( wxT(".pyw") )) != wxNOT_FOUND )
        pos += 3;
    else if( (pos = aCommand.Find( wxT(".py") )) != wxNOT_FOUND )
        pos += 2;
    else if( (pos = aCommand.Find( wxT(".xsl") )) != wxNOT_FOUND )
        pos += 3;
    else if( (pos = aCommand.Find( wxT(".exe") )) != wxNOT_FOUND )
        pos += 3;

    // the end of plugin name is at position pos.
    if( pos > 0 )
    {
        // Be sure this is the end of the name: the next char is " or space
        int eos = aCommand[pos+1];

        if( eos == ' '|| eos == '\"' )
        {
            // search for the starting point of the name
            int jj = pos-1;
            while( jj >= 0 )
                if( aCommand[jj] != eos )
                    jj--;
                else
                    break;

            // extract the name
            if( jj >= 0 )
            {
                eos = aCommand[jj];

                if( eos == ' '|| eos == '\"' )  // do not include delimiters
                    jj++;

                pluginName = aCommand.SubString( jj, pos );
            }
        }
    }

    // Using a format like %P is possible in  plugin name, so expand it
    wxString prj_dir = Prj().GetProjectPath();

    if( prj_dir.EndsWith( '/' ) || prj_dir.EndsWith( '\\' ) )
        prj_dir.RemoveLast();

    pluginName.Replace( wxT( "%P" ), prj_dir.GetData(), true );

    return pluginName;
}

void DIALOG_BOM::OnEditPlugin( wxCommandEvent& event )
{
    wxString pluginName = getPluginFileName( m_textCtrlCommand->GetValue() );

    if( pluginName.Length() <= 2 )      // if name != ""
    {
        wxMessageBox( _("Plugin file name not found.") );
        return;
    }

    AddDelimiterString( pluginName );
    wxString editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, pluginName );
    else
        wxMessageBox( _( "No text editor selected in KiCad.  Please choose one." ) );
}


void DIALOG_BOM::OnHelp( wxCommandEvent& event )
{
    HTML_MESSAGE_BOX help_Dlg( this, _("Bom Generation Help") );
    help_Dlg.SetDialogSizeInDU( 500, 350 );

    wxString msg = FROM_UTF8(s_bomHelpInfo);
    help_Dlg.m_htmlWindow->AppendToPage( msg );
    help_Dlg.ShowModal();
}


void DIALOG_BOM::OnCommandLineEdited( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_plugins.Item( ii ).Command = m_textCtrlCommand->GetValue();
}


void DIALOG_BOM::OnNameEdited( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_plugins.Item( ii ).Name = m_textCtrlName->GetValue();
    m_lbPlugins->SetString( ii, m_plugins.Item( ii ).Name );
}

void DIALOG_BOM::OnShowConsoleChanged( wxCommandEvent& event )
{
#ifdef __WINDOWS__
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    if( m_checkBoxShowConsole->IsChecked() )
    {
        if( m_plugins.Item( ii ).Options.Index( wxT( "show_console" ) ) == wxNOT_FOUND )
            m_plugins.Item( ii ).Options.Add( wxT( "show_console" ) );
    }
    else
    {
        m_plugins.Item( ii ).Options.Remove( wxT( "show_console" ) );
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
