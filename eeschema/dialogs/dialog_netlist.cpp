/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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

/* Functions relative to the dialog creating the netlist for Pcbnew.  The dialog is a notebook
 * with 4 fixed netlist formats:
 *   Pcbnew
 *   ORCADPCB2
 *   CADSTAR
 *   SPICE
 * and up to CUSTOMPANEL_COUNTMAX user programmable formats.  These external converters are
 * referred to as plugins, but they are really just external binaries.
 */

#include <pgm_base.h>
#include <kiface_i.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <general.h>
#include <netlist.h>
#include <dialogs/dialog_netlist_base.h>
#include <wildcards_and_files_ext.h>
#include <invoke_sch_dialog.h>
#include <netlist_exporters/netlist_exporter_pspice.h>
#include <eeschema_settings.h>
#include <schematic.h>
#include <paths.h>

#include <eeschema_id.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>



#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins


/* panel (notebook page) identifiers */
enum panel_netlist_index {
    PANELPCBNEW = 0,    /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,     /* Handle Netlist format OracdPcb2 */
    PANELCADSTAR,       /* Handle Netlist format CadStar */
    PANELSPICE,         /* Handle Netlist format Pspice */
    PANELCUSTOMBASE     /* First auxiliary panel (custom netlists).
                         * others use PANELCUSTOMBASE+1, PANELCUSTOMBASE+2.. */
};


/* wxPanels for creating the NoteBook pages for each netlist format: */
class NETLIST_PAGE_DIALOG : public wxPanel
{

public:
    /**
     * Create a setup page for one netlist format.
     *
     * Used in Netlist format dialog box creation.
     *
     * @param parent is the wxNotebook parent.
     * @param title is the title of the notebook page.
     * @param id_NetType is the netlist ID type.
     */
    NETLIST_PAGE_DIALOG( wxNotebook* parent, const wxString& title,
                         NETLIST_TYPE_ID id_NetType );
    ~NETLIST_PAGE_DIALOG() { };

    /**
     * @return the name of the netlist format for this page.
     */
    const wxString GetPageNetFmtName() { return m_pageNetFmtName; }

    NETLIST_TYPE_ID   m_IdNetType;
    // opt to reformat passive component values (e.g. 1M -> 1Meg):
    wxCheckBox*       m_AdjustPassiveValues;
    wxTextCtrl*       m_CommandStringCtrl;
    wxTextCtrl*       m_TitleStringCtrl;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;

private:
    wxString          m_pageNetFmtName;
};


/* Dialog frame for creating netlists */
class NETLIST_DIALOG : public NETLIST_DIALOG_BASE
{
public:
    NETLIST_DIALOG( SCH_EDIT_FRAME* parent );
    ~NETLIST_DIALOG() { };

private:
    friend class NETLIST_PAGE_DIALOG;

    void InstallCustomPages();
    NETLIST_PAGE_DIALOG* AddOneCustomPage( const wxString & aTitle,
                                           const wxString & aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void InstallPageSpice();
    bool TransferDataFromWindow() override;
    void NetlistUpdateOpt();

    // Called when changing the notebook page (and therefore the current netlist format)
    void OnNetlistTypeSelection( wxNotebookEvent& event ) override;

    /**
     * Add a new panel for a new netlist plugin.
     */
    void OnAddGenerator( wxCommandEvent& event ) override;

    /**
     * Remove a panel relative to a netlist plugin.
     */
    void OnDelGenerator( wxCommandEvent& event ) override;

    /**
     * Run the external spice simulator command.
     */
    void OnRunExternSpiceCommand( wxCommandEvent& event );

    /**
     * Enable (if the command line is not empty or disable the button to run spice command.
     */
    void OnRunSpiceButtUI( wxUpdateUIEvent& event );

    /**
     * Write the current netlist options setup in the configuration.
     */
    void WriteCurrentNetlistSetup();

    /**
     * Return the filename extension and the wildcard string for this page or a void name
     * if there is no default name.
     *
     * @param aType is the netlist type ( NET_TYPE_PCBNEW ... ).
     * @param aExt [in] is a holder for the netlist file extension.
     * @param aWildCard [in] is a holder for netlist file dialog wildcard.
     * @return true for known netlist type, false for custom formats.
     */
    bool FilenamePrms( NETLIST_TYPE_ID aType,  wxString* aExt, wxString* aWildCard );

    DECLARE_EVENT_TABLE()

public:
    SCH_EDIT_FRAME*      m_Parent;
    wxString             m_DefaultNetFmtName;
    NETLIST_PAGE_DIALOG* m_PanelNetType[4 + CUSTOMPANEL_COUNTMAX];
};


class NETLIST_DIALOG_ADD_GENERATOR : public NETLIST_DIALOG_ADD_GENERATOR_BASE
{
public:
    NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent );

    const wxString GetGeneratorTitle()  { return m_textCtrlName->GetValue(); }
    const wxString GetGeneratorTCommandLine() { return m_textCtrlCommand->GetValue(); }

    bool TransferDataFromWindow() override;

private:
    /*
     * Browse plugin files, and set m_CommandStringCtrl field
     */
    void OnBrowseGenerators( wxCommandEvent& event ) override;

   NETLIST_DIALOG* m_Parent;
};


/* Event id for notebook page buttons: */
enum id_netlist {
    ID_CREATE_NETLIST = ID_END_EESCHEMA_ID_LIST + 1,
    ID_USE_NETCODE_AS_NETNAME,
    ID_RUN_SIMULATOR
};


BEGIN_EVENT_TABLE( NETLIST_DIALOG, NETLIST_DIALOG_BASE )
    EVT_BUTTON( ID_RUN_SIMULATOR, NETLIST_DIALOG::OnRunExternSpiceCommand )
    EVT_UPDATE_UI( ID_RUN_SIMULATOR, NETLIST_DIALOG::OnRunSpiceButtUI )
END_EVENT_TABLE()


NETLIST_PAGE_DIALOG::NETLIST_PAGE_DIALOG( wxNotebook* parent, const wxString& title,
                                          NETLIST_TYPE_ID id_NetType ) :
        wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_IdNetType           = id_NetType;
    m_pageNetFmtName      = title;
    m_CommandStringCtrl   = nullptr;
    m_TitleStringCtrl     = nullptr;
    m_AdjustPassiveValues = nullptr;

    wxString netfmtName = static_cast<NETLIST_DIALOG*>( parent->GetParent() )->m_DefaultNetFmtName;

    bool selected = m_pageNetFmtName == netfmtName;

    parent->AddPage( this, title, selected );

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* UpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_LowBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( UpperBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( m_LowBoxSizer, 0, wxGROW | wxALL, 5 );

    m_LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    m_RightOptionsBoxSizer = new wxBoxSizer( wxVERTICAL );
    UpperBoxSizer->Add( m_LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    UpperBoxSizer->Add( m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    UpperBoxSizer->Add( m_RightOptionsBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}


NETLIST_DIALOG::NETLIST_DIALOG( SCH_EDIT_FRAME* parent ) :
    NETLIST_DIALOG_BASE( parent )
{
    m_Parent = parent;

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    m_DefaultNetFmtName = settings.m_NetFormatName;

    for( NETLIST_PAGE_DIALOG*& page : m_PanelNetType)
        page = nullptr;

    // Add notebook pages:
    m_PanelNetType[PANELPCBNEW] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "KiCad" ),
                                                           NET_TYPE_PCBNEW );

    m_PanelNetType[PANELORCADPCB2] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "OrcadPCB2" ),
                                                              NET_TYPE_ORCADPCB2 );

    m_PanelNetType[PANELCADSTAR] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "CadStar" ),
                                                            NET_TYPE_CADSTAR );

    InstallPageSpice();
    InstallCustomPages();

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizer2OK->SetLabel( _( "Export Netlist" ) );
    m_sdbSizer2Cancel->SetLabel( _( "Close" ) );
    m_buttonSizer->Layout();

    m_sdbSizer2OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void NETLIST_DIALOG::OnRunExternSpiceCommand( wxCommandEvent& event )
{
    // Run the external spice simulator command
    NetlistUpdateOpt();

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();
    wxString simulatorCommand = settings.m_SpiceCommandString;

    unsigned netlist_opt = 0;

    // Calculate the netlist filename and options
    wxFileName fn = m_Parent->Schematic().GetFileName();
    fn.SetExt( wxT( "cir" ) );

    if( settings.m_SpiceAdjustPassiveValues )
        netlist_opt |= NET_ADJUST_PASSIVE_VALS;

    // Build the command line
    wxString commandLine = simulatorCommand;
    commandLine.Replace( "%I", fn.GetFullPath(), true );

    if( m_Parent->ReadyToNetlist( _( "Simulator requires a fully annotated schematic." ) ) )
    {
        m_Parent->WriteNetListFile( NET_TYPE_SPICE, fn.GetFullPath(), netlist_opt, nullptr );
        wxExecute( commandLine, wxEXEC_ASYNC );
    }
}


void NETLIST_DIALOG::OnRunSpiceButtUI( wxUpdateUIEvent& aEvent )
{
    bool disable = m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->IsEmpty();
    aEvent.Enable( !disable );
}


void NETLIST_DIALOG::InstallPageSpice()
{
    NETLIST_PAGE_DIALOG* page = m_PanelNetType[PANELSPICE] =
                    new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "Spice" ), NET_TYPE_SPICE );

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    page->m_AdjustPassiveValues = new wxCheckBox( page, ID_USE_NETCODE_AS_NETNAME,
                                                  _( "Reformat passive symbol values" ) );
    page->m_AdjustPassiveValues->SetToolTip( _( "Reformat passive symbol values e.g. 1M -> 1Meg" ) );
    page->m_AdjustPassiveValues->SetValue( settings.m_SpiceAdjustPassiveValues );
    page->m_LeftBoxSizer->Add( page->m_AdjustPassiveValues, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );

    wxString simulatorCommand = settings.m_SpiceCommandString;
    wxStaticText* spice_label = new wxStaticText( page, -1, _( "External simulator command:" ) );
    spice_label->SetToolTip( _( "Enter the command line to run spice\n"
                                "Usually <path to spice binary> %I\n"
                                "%I will be replaced by the actual spice netlist name" ) );
    page->m_LowBoxSizer->Add( spice_label, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    page->m_CommandStringCtrl = new wxTextCtrl( page, -1, simulatorCommand,
                                                wxDefaultPosition, wxDefaultSize );

    page->m_CommandStringCtrl->SetInsertionPoint( 1 );
    page->m_LowBoxSizer->Add( page->m_CommandStringCtrl, 0,
                              wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    // Add button to run spice command
    wxButton* button = new wxButton( page, ID_RUN_SIMULATOR,
                                     _( "Create Netlist and Run Simulator Command" ) );
    page->m_LowBoxSizer->Add( button, 0, wxGROW | wxBOTTOM | wxLEFT | wxRIGHT, 5 );
}


void NETLIST_DIALOG::InstallCustomPages()
{
    NETLIST_PAGE_DIALOG* currPage;

    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( size_t i = 0;
             i < CUSTOMPANEL_COUNTMAX && i < cfg->m_NetlistPanel.custom_command_titles.size();
             i++ )
        {
            // pairs of (title, command) are stored
            wxString title = cfg->m_NetlistPanel.custom_command_titles[i];

            if( i >= cfg->m_NetlistPanel.custom_command_paths.size() )
                break; // No more panel to install

            wxString command = cfg->m_NetlistPanel.custom_command_paths[i];

            currPage = AddOneCustomPage( title, command,
                    static_cast<NETLIST_TYPE_ID>( NET_TYPE_CUSTOM1 + i ) );

            m_PanelNetType[PANELCUSTOMBASE + i] = currPage;
        }
    }
}


NETLIST_PAGE_DIALOG* NETLIST_DIALOG::AddOneCustomPage( const wxString & aTitle,
                                                       const wxString & aCommandString,
                                                       NETLIST_TYPE_ID aNetTypeId )
{
    NETLIST_PAGE_DIALOG* currPage = new NETLIST_PAGE_DIALOG( m_NoteBook, aTitle, aNetTypeId );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Title:" ) ), 0,
                                  wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_TitleStringCtrl = new wxTextCtrl( currPage, -1, aTitle,
                                                  wxDefaultPosition, wxDefaultSize );

    currPage->m_TitleStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_TitleStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Netlist command:" ) ), 0,
                                                    wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_CommandStringCtrl = new wxTextCtrl( currPage, -1, aCommandString,
                                                    wxDefaultPosition, wxDefaultSize );

    currPage->m_CommandStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_CommandStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    return currPage;
}


void NETLIST_DIALOG::OnNetlistTypeSelection( wxNotebookEvent& event )
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( currPage == nullptr )
        return;

    m_DefaultNetFmtName = currPage->GetPageNetFmtName();

    m_buttonDelGenerator->Enable( currPage->m_IdNetType >= NET_TYPE_CUSTOM1 );
}


void NETLIST_DIALOG::NetlistUpdateOpt()
{
    bool adjust = m_PanelNetType[ PANELSPICE ]->m_AdjustPassiveValues->IsChecked();
    wxString spice_cmd_string = m_PanelNetType[ PANELSPICE ]->m_CommandStringCtrl->GetValue();

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    settings.m_SpiceAdjustPassiveValues = adjust;
    settings.m_SpiceCommandString       = spice_cmd_string;
    settings.m_NetFormatName            = wxEmptyString;
    settings.m_NetFormatName            = m_DefaultNetFmtName;
}


bool NETLIST_DIALOG::TransferDataFromWindow()
{
    wxFileName  fn;
    wxString    fileWildcard;
    wxString    fileExt;
    wxString    title = _( "Save Netlist File" );

    NetlistUpdateOpt();

    NETLIST_PAGE_DIALOG* currPage;
    currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    unsigned netlist_opt = 0;

    // Calculate the netlist filename
    fn = m_Parent->Schematic().GetFileName();
    FilenamePrms( currPage->m_IdNetType, &fileExt, &fileWildcard );

    // Set some parameters
    switch( currPage->m_IdNetType )
    {
    case NET_TYPE_SPICE:
        // Set spice netlist options:
        if( currPage->m_AdjustPassiveValues->GetValue() )
            netlist_opt |= NET_ADJUST_PASSIVE_VALS;
        break;

    case NET_TYPE_CADSTAR:
        break;

    case NET_TYPE_PCBNEW:
        break;

    case NET_TYPE_ORCADPCB2:
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
    {
        wxString command = currPage->m_CommandStringCtrl->GetValue();
        wxRegEx extRE( wxT( ".*\\.([[:alnum:]][[:alnum:]][[:alnum:]][[:alnum:]]?)\\.xslt?\".*" ) );

        if( extRE.Matches( command ) )
            fileExt = extRE.GetMatch( command, 1 );

        title.Printf( _( "%s Export" ), currPage->m_TitleStringCtrl->GetValue().GetData() );
        break;
    }
    }

    fn.SetExt( fileExt );

    if( fn.GetPath().IsEmpty() )
       fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );

    wxString fullpath = fn.GetFullPath();
    wxString fullname = fn.GetFullName();
    wxString path     = fn.GetPath();

    // full name does not and should not include the path, per wx docs.
    wxFileDialog dlg( this, title, path, fullname, fileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullpath = dlg.GetPath();   // directory + filename

    m_Parent->ClearMsgPanel();

    if( currPage->m_CommandStringCtrl )
        m_Parent->SetNetListerCommand( currPage->m_CommandStringCtrl->GetValue() );
    else
        m_Parent->SetNetListerCommand( wxEmptyString );

    if( m_Parent->ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
        m_Parent->WriteNetListFile( currPage->m_IdNetType, fullpath, netlist_opt, nullptr );

    WriteCurrentNetlistSetup();

    return true;
}


bool NETLIST_DIALOG::FilenamePrms( NETLIST_TYPE_ID aType, wxString * aExt, wxString * aWildCard )
{
    wxString fileExt;
    wxString fileWildcard;
    bool     ret = true;

    switch( aType )
    {
    case NET_TYPE_SPICE:
        fileExt = wxT( "cir" );
        fileWildcard = SpiceNetlistFileWildcard();
        break;

    case NET_TYPE_CADSTAR:
        fileExt = wxT( "frp" );
        fileWildcard = CadstarNetlistFileWildcard();
        break;

    case NET_TYPE_PCBNEW:
    case NET_TYPE_ORCADPCB2:
        fileExt = NetlistFileExtension;
        fileWildcard = NetlistFileWildcard();
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
        fileWildcard = AllFilesWildcard();
        ret = false;
    }

    if( aExt )
        *aExt = fileExt;

    if( aWildCard )
        *aWildCard = fileWildcard;

    return ret;
}


void NETLIST_DIALOG::WriteCurrentNetlistSetup()
{
    wxString  msg;

    NetlistUpdateOpt();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        cfg->m_NetlistPanel.custom_command_titles.clear();
        cfg->m_NetlistPanel.custom_command_paths.clear();

        // Update existing custom pages
        for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
        {
            NETLIST_PAGE_DIALOG* currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

            if( currPage == nullptr )
                break;

            wxString title = currPage->m_TitleStringCtrl->GetValue();

            if( title.IsEmpty() )
                continue;

            cfg->m_NetlistPanel.custom_command_titles.emplace_back( title.ToStdString() );
            cfg->m_NetlistPanel.custom_command_paths.emplace_back(
                    currPage->m_CommandStringCtrl->GetValue().ToStdString() );
        }
    }
}


void NETLIST_DIALOG::OnDelGenerator( wxCommandEvent& event )
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    currPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    currPage->m_TitleStringCtrl->SetValue( wxEmptyString );
    m_DefaultNetFmtName = m_PanelNetType[PANELPCBNEW]->GetPageNetFmtName();

    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


void NETLIST_DIALOG::OnAddGenerator( wxCommandEvent& event )
{
    NETLIST_DIALOG_ADD_GENERATOR dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Creates a new custom plugin page
    wxString title = dlg.GetGeneratorTitle();

    // Verify it does not exists
    int netTypeId = PANELCUSTOMBASE;    // the first not used type id
    NETLIST_PAGE_DIALOG* currPage;

    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        netTypeId = PANELCUSTOMBASE + ii;
        currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == nullptr )
            break;

        if( currPage->GetPageNetFmtName() == title )
        {
            wxMessageBox( _("This plugin already exists.") );
            return;
        }
    }

    wxString cmd = dlg.GetGeneratorTCommandLine();
    currPage = AddOneCustomPage( title,cmd, (NETLIST_TYPE_ID)netTypeId );
    m_PanelNetType[netTypeId] = currPage;
    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


NETLIST_DIALOG_ADD_GENERATOR::NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent ) :
    NETLIST_DIALOG_ADD_GENERATOR_BASE( parent )
{
    m_Parent = parent;
    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
}


bool NETLIST_DIALOG_ADD_GENERATOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlCommand->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator command string" ) );
        return false;
    }

    if( m_textCtrlName->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator title" ) );
        return false;
    }

    return true;
}


void NETLIST_DIALOG_ADD_GENERATOR::OnBrowseGenerators( wxCommandEvent& event )
{
    wxString FullFileName, Path;

#ifndef __WXMAC__
    Path = Pgm().GetExecutablePath();
#else
    Path = PATHS::GetOSXKicadDataDir() + wxT( "/plugins" );
#endif
    FullFileName = EDA_FILE_SELECTOR( _( "Generator files:" ), Path, FullFileName,
                                      wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                      this, wxFD_OPEN, true );
    if( FullFileName.IsEmpty() )
        return;

    // Creates a default command line, suitable for external tool xslproc or python, based on
    // the plugin extension ("xsl" or "exe" or "py")
    wxString cmdLine;
    wxFileName fn( FullFileName );
    wxString ext = fn.GetExt();

    if( ext == wxT( "xsl" ) )
        cmdLine.Printf( wxT( "xsltproc -o \"%%O\" \"%s\" \"%%I\"" ), FullFileName );
    else if( ext == wxT( "exe" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "\"%s\" > \"%%O\" < \"%%I\"" ), FullFileName );
    else if( ext == wxT( "py" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "python \"%s\" \"%%I\" \"%%O\"" ), FullFileName );
    else
        cmdLine.Printf( wxT( "\"%s\"" ), FullFileName );

    m_textCtrlCommand->SetValue( cmdLine );

    /* Get a title for this page */
    wxString title = m_textCtrlName->GetValue();

    if( title.IsEmpty() )
        wxMessageBox( _( "Do not forget to choose a title for this netlist control page" ) );
}


int InvokeDialogNetList( SCH_EDIT_FRAME* aCaller )
{
    NETLIST_DIALOG dlg( aCaller );

    SCHEMATIC_SETTINGS& settings = aCaller->Schematic().Settings();

    wxString curr_default_netformat = settings.m_NetFormatName;

    int ret = dlg.ShowModal();

    // Update the default netlist and store it in prj config if it was explicitly changed.
    settings.m_NetFormatName = dlg.m_DefaultNetFmtName; // can have temporary changed

    if( curr_default_netformat != dlg.m_DefaultNetFmtName )
        aCaller->SaveProjectSettings();

    return ret;
}
