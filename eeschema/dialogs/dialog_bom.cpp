/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <schframe.h>

#include <netlist.h>
#include <netlist_exporter_generic.h>
#include <sch_sheet.h>
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
 * Class BOM_CFG_READER_PARSER
 * holds data and functions pertinent to parsing a S-expression file
 * for a WORKSHEET_LAYOUT.
 */
class BOM_CFG_READER_PARSER : public DIALOG_BOM_CFG_LEXER
{
    wxArrayString* m_pluginsList;

public:
    BOM_CFG_READER_PARSER( wxArrayString* aPlugins,
                           const char* aData, const wxString& aSource );
    void Parse() throw( PARSE_ERROR, IO_ERROR );

private:
    void parsePlugin() throw( IO_ERROR, PARSE_ERROR );
};

// PCB_PLOT_PARAMS_PARSER

BOM_CFG_READER_PARSER::BOM_CFG_READER_PARSER(  wxArrayString* aPlugins,
                                              const char* aLine,
                                              const wxString& aSource ) :
    DIALOG_BOM_CFG_LEXER( aLine, aSource )
{
    m_pluginsList = aPlugins;
}


void BOM_CFG_READER_PARSER::Parse() throw( PARSE_ERROR, IO_ERROR )
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

void BOM_CFG_READER_PARSER::parsePlugin() throw( IO_ERROR, PARSE_ERROR )
{
    wxString title, command;

    NeedSYMBOLorNUMBER();
    title = FromUTF8();

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
            command = FromUTF8();
            NeedRIGHT();
            break;

        case T_opts:
            while( ( token = NextTok() ) != T_RIGHT && token != T_EOF );
            break;

        default:
            Unexpected( CurText() );
            break;
        }
    }

    if( ! title.IsEmpty() )
    {
        m_pluginsList->Add( title );
        m_pluginsList->Add( command );
    }
}

// The main dialog frame tu run scripts to build bom
class DIALOG_BOM : public DIALOG_BOM_BASE
{
private:
    SCH_EDIT_FRAME*   m_parent;
    // The list of scripts (or plugins):
    // a script descr uses 2 lines:
    // the first is the title
    // the second is the command line
    wxArrayString     m_plugins;
    wxConfigBase*     m_config;         // to store the "plugins"

public:
    // Constructor and destructor
    DIALOG_BOM( SCH_EDIT_FRAME* parent );
    ~DIALOG_BOM();

private:
    void OnPluginSelected( wxCommandEvent& event );
    void OnRunPlugin( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnHelp( wxCommandEvent& event );
    void OnAddPlugin( wxCommandEvent& event );
    void OnRemovePlugin( wxCommandEvent& event );
    void OnEditPlugin( wxCommandEvent& event );
    void OnCommandLineEdited( wxCommandEvent& event );
    void OnNameEdited( wxCommandEvent& event );

    void pluginInit();
    void installPluginsList();

    /**
     * @return the Plugin filename from a command line
     * @param aCommand = the command line
     */
    wxString getPluginFileName( const wxString& aCommand );

    /**
     * display (when exists) the text found between the keyword "@package"
     * (compatible with doxygen comments)
     * and the end of comment block (""" in python", --> in xml)
     */
    void displayPluginInfo( FILE * aFile, const wxString& aFilename );

    /**
     * Browse plugin files, and set m_CommandStringCtrl field
     * @return a command line ro run the plugin
     */
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
    installPluginsList();

    GetSizer()->Fit( this );
    Centre();
}

DIALOG_BOM::~DIALOG_BOM()
{
    // Save the plugin descriptions in config.
    // the config stores only one string.
    // plugins are saved inside a S expr:
    // ( plugins
    //    ( plugin "plugin name" (cmd "command line") )
    //     ....
    // )

    STRING_FORMATTER writer;
    writer.Print( 0, "(plugins" );

    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii += 2 )
    {
        writer.Print( 1, "(plugin %s (cmd %s))",
                      writer.Quotew( m_plugins[ii] ).c_str(),
                      writer.Quotew( m_plugins[ii+1] ).c_str() );
    }

    writer.Print( 0, ")" );

    wxString list( FROM_UTF8( writer.GetString().c_str() ) );

    m_config->Write( BOM_PLUGINS_KEY, list );

    wxString active_plugin_name = m_lbPlugins->GetStringSelection( );
    m_config->Write( BOM_PLUGIN_SELECTED_KEY, active_plugin_name );

}

/* Read the initialized plugins in config and fill the list
 * of names
 */
void DIALOG_BOM::installPluginsList()
{
    wxString list, active_plugin_name;
    m_config->Read( BOM_PLUGINS_KEY, &list );
    m_config->Read( BOM_PLUGIN_SELECTED_KEY, &active_plugin_name );

    if( !list.IsEmpty() )
    {
        BOM_CFG_READER_PARSER cfg_parser( &m_plugins, TO_UTF8( list ),
                                          wxT( "plugins" ) );
        try
        {
            cfg_parser.Parse();
        }
        catch( const IO_ERROR& ioe )
        {
//            wxLogMessage( ioe.errorText );
        }
    }

    // Populate list box
    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii+=2 )
    {
        m_lbPlugins->Append( m_plugins[ii] );

        if( active_plugin_name == m_plugins[ii] )
            m_lbPlugins->SetSelection( ii/2 );
    }

    pluginInit();
}

void DIALOG_BOM::OnPluginSelected( wxCommandEvent& event )
{
    pluginInit();
}

#include <wx/ffile.h>
void DIALOG_BOM::pluginInit()
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
    {
        m_textCtrlName->SetValue( wxEmptyString );
        m_textCtrlCommand->SetValue( wxEmptyString );
        return;
    }

    m_textCtrlName->SetValue( m_plugins[2 * ii] );
    m_textCtrlCommand->SetValue( m_plugins[(2 * ii)+1] );

    wxString pluginName = getPluginFileName( m_textCtrlCommand->GetValue() );

    if( pluginName.IsEmpty() )
        return;

    FILE* pluginFile = wxFopen( pluginName, "rt" );

    if( pluginFile == NULL )
    {
        wxString msg;
        msg.Printf( _( "Failed to open file '%s'" ), GetChars( pluginName ) );
        DisplayError( this, msg );
        return;
    }

    displayPluginInfo( pluginFile, pluginName );
}


void DIALOG_BOM::displayPluginInfo( FILE * aFile, const wxString& aFilename )
{
    m_Messages->Clear();

    // display (when exists) the text found between the keyword "@package"
    // (compatible with doxygen comments)
    // and the end of comment block (""" in python", --> in xml)

    wxString data;
    wxFFile fdata( aFile );        // dtor will close the file

    if( !fdata.ReadAll( &data ) )
        return;

    wxString header( wxT( "@package" ) );
    wxString endsection( wxT( "-->" ) );        // For xml

    wxFileName fn( aFilename );

    if( fn.GetExt().IsSameAs( wxT("py"), false ) )
        endsection = wxT( "\"\"\"" );
    else if( !fn.GetExt().IsSameAs( wxT("xsl"), false ) )
        // If this is not a python file, we know nothing about file
        // and the info cannot be found
        return;

    // Extract substring between @package and """
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
}

/**
 * Function RunPlugin
 * run the plugin command line
 */
void DIALOG_BOM::OnRunPlugin( wxCommandEvent& event )
{
    wxFileName  fn;

    // Calculate the xml netlist filename
    fn = g_RootSheet->GetScreen()->GetFileName();

    fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );

    fn.ClearExt();
    wxString fullfilename = fn.GetFullPath();
    m_parent->ClearMsgPanel();

    wxString reportmsg;
    WX_STRING_REPORTER reporter( &reportmsg );
    reporter.SetReportAll( true );
    m_parent->SetNetListerCommand( m_textCtrlCommand->GetValue() );
    m_parent->CreateNetlist( -1, fullfilename, 0, &reporter );

    m_Messages->SetValue( reportmsg );
}


void DIALOG_BOM::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


/**
 * Function OnRemovePlugin
 * Remove a plugin from the list
 */
void DIALOG_BOM::OnRemovePlugin( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_lbPlugins->Delete( ii );

    m_plugins.RemoveAt( 2*ii, 2 ); // Remove title and command line

    // Select the next item, if exists
    if( (int)m_lbPlugins->GetCount() >= ii )
        ii =  m_lbPlugins->GetCount() - 1;

    if( ii >= 0 )
        m_lbPlugins->SetSelection( ii );

    pluginInit();
}

/**
 * Function OnAddPlugin
 * Add a new panel for a new netlist plugin
 */
void DIALOG_BOM::OnAddPlugin( wxCommandEvent& event )
{
    wxString cmdLine = choosePlugin();

    if( cmdLine.IsEmpty() )
        return;

    // Creates a new plugin entry
    wxFileName fn( getPluginFileName( cmdLine ) );

    wxString defaultName = fn.GetName();
    wxString name = wxGetTextFromUser( _("Plugin name in plugin list") ,
                                       _("Plugin name"), defaultName );

    if( name.IsEmpty() )
        return;

    // Verify if it does not exists
    for( unsigned ii = 0; ii < m_plugins.GetCount(); ii += 2 )
    {
        if( name == m_plugins[ii] )
        {
            wxMessageBox( _("This name already exists. Abort") );
            return;
        }
    }

    // Eppend the new plugin
    m_plugins.Add( name );
    m_plugins.Add( wxEmptyString );
    m_lbPlugins->SetSelection( m_lbPlugins->GetCount() - 1 );
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

    wxString fullFileName = EDA_FileSelector( _( "Plugin files:" ),
                                     path,
                                     wxEmptyString,
                                     wxEmptyString,
                                     mask,
                                     this,
                                     wxFD_OPEN,
                                     true
                                     );
    if( fullFileName.IsEmpty() )
        return wxEmptyString;

    // Creates a default command line,
    // suitable to run the external tool xslproc or python
    // The default command line depending on plugin extension, currently
    // "xsl" or "exe" or "py"
    wxString    cmdLine;
    wxFileName  fn( fullFileName );
    wxString    ext = fn.GetExt();

    if( ext == wxT("xsl" ) )
        cmdLine.Printf(wxT("xsltproc -o \"%%O\" \"%s\" \"%%I\""), GetChars( fullFileName ) );
    else if( ext == wxT("exe" ) || ext.IsEmpty() )
        cmdLine.Printf(wxT("\"%s\" < \"%%I\" > \"%%O\""), GetChars( fullFileName ) );
    else if( ext == wxT("py" ) || ext.IsEmpty() )
        cmdLine.Printf(wxT("python \"%s\" \"%%I\" \"%%O\""), GetChars( fullFileName ) );
    else
        cmdLine.Printf(wxT("\"%s\""), GetChars( fullFileName ) );

    return cmdLine;
}


wxString DIALOG_BOM::getPluginFileName(  const wxString& aCommand )
{
    wxString pluginName;

    // Try to find the plugin name.
    // This is possible if the name ends by .py or .xsl
    int pos = -1;

    if( (pos = aCommand.Find( wxT(".py") )) != wxNOT_FOUND )
        pos += 2;
    else if( (pos = aCommand.Find( wxT(".xsl") )) != wxNOT_FOUND )
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

    return pluginName;
}

void DIALOG_BOM::OnEditPlugin( wxCommandEvent& event )
{
    wxString    pluginName = getPluginFileName( m_textCtrlCommand->GetValue() );

    if( pluginName.Length() <= 2 )      // if name != ""
    {
        wxMessageBox( _("Plugin file name not found. Cannot edit plugin file") );
        return;
    }

    AddDelimiterString( pluginName );
    wxString    editorname = Pgm().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, pluginName );
    else
        wxMessageBox( _("No text editor selected in KiCad. Please choose it") );
}

void DIALOG_BOM::OnHelp( wxCommandEvent& event )
{
    HTML_MESSAGE_BOX help_Dlg( this, _("Bom Generation Help"),
                               wxDefaultPosition, wxSize( 750,550 ) );

    wxString msg = FROM_UTF8(s_bomHelpInfo);
    help_Dlg.m_htmlWindow->AppendToPage( msg );
    help_Dlg.ShowModal();
}

void DIALOG_BOM::OnCommandLineEdited( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_plugins[(2 * ii)+1] = m_textCtrlCommand->GetValue();
}

void DIALOG_BOM::OnNameEdited( wxCommandEvent& event )
{
    int ii = m_lbPlugins->GetSelection();

    if( ii < 0 )
        return;

    m_plugins[2 * ii] = m_textCtrlName->GetValue();
    m_lbPlugins->SetString( ii, m_plugins[2 * ii] );
}
