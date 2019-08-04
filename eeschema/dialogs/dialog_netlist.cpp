/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
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

#include <eeschema_id.h>
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
    NETLIST_TYPE_ID   m_IdNetType;
    wxCheckBox*       m_IsCurrentFormat;
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

public:
    /** Constructor to create a setup page for one netlist format.
     * Used in Netlist format Dialog box creation
     * @param parent = wxNotebook * parent
     * @param title = title (name) of the notebook page
     * @param id_NetType = netlist type id
     */
    NETLIST_PAGE_DIALOG( wxNotebook* parent, const wxString& title,
                         NETLIST_TYPE_ID id_NetType );
    ~NETLIST_PAGE_DIALOG() { };

    /**
     * function GetPageNetFmtName
     * @return the name of the netlist format for this page
     * This is also the page label.
     */
    const wxString GetPageNetFmtName() { return m_pageNetFmtName; }
};


/* Dialog frame for creating netlists */
class NETLIST_DIALOG : public NETLIST_DIALOG_BASE
{
    friend class NETLIST_PAGE_DIALOG;

public:
    SCH_EDIT_FRAME*      m_Parent;
    wxString             m_DefaultNetFmtName;
    NETLIST_PAGE_DIALOG* m_PanelNetType[4 + CUSTOMPANEL_COUNTMAX];

protected:
    bool                 m_asFormatSelected;
    wxConfigBase*        m_config;

public:
    // Constructor and destructor
    NETLIST_DIALOG( SCH_EDIT_FRAME* parent );
    ~NETLIST_DIALOG() { };

private:
    void InstallCustomPages();
    NETLIST_PAGE_DIALOG* AddOneCustomPage( const wxString & aTitle,
                                           const wxString & aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void InstallPageSpice();
    bool TransferDataFromWindow() override;
    void NetlistUpdateOpt();

    // Called when changing the notebook page (and therefore the current netlist format)
    void OnNetlistTypeSelection( wxNotebookEvent& event ) override;

    // called when checking the "Default format" wxCheckBox in the current page of the notbook
    void SelectDefaultNetlistType( wxCommandEvent& event );

    /**
     * Function OnAddGenerator
     * Add a new panel for a new netlist plugin
     */
    void OnAddGenerator( wxCommandEvent& event ) override;

    /**
     * Function OnDelGenerator
     * Remove a panel relative to a netlist plugin
     */
    void OnDelGenerator( wxCommandEvent& event ) override;

    /**
     * Function WriteCurrentNetlistSetup
     * Write the current netlist options setup in the configuration
     */
    void WriteCurrentNetlistSetup();

    /**
     * Function UserNetlistTypeName
     * to retrieve user netlist type names
     * @param first_item = true: return first name of the list, false = return next
     * @return a wxString : name of the type netlist or empty string
     * this function must be called first with "first_item" = true
     * and after with "first_item" = false to get all the other existing netlist names
     */
    const wxString UserNetlistTypeName( bool first_item );

    /**
     * Function FilenamePrms
     * returns the filename extension and the wildcard string for this curr
     * or a void name if there is no default name
     * @param aType = the netlist type ( NET_TYPE_PCBNEW ... )
     * @param aExt = a reference to a wxString to return the default  file ext.
     * @param aWildCard =  reference to a wxString to return the default wildcard.
     * @return true for known netlist type, false for custom formats
     */
    bool FilenamePrms( NETLIST_TYPE_ID aType,
                       wxString * aExt, wxString * aWildCard );

    DECLARE_EVENT_TABLE()
};


class NETLIST_DIALOG_ADD_GENERATOR : public NETLIST_DIALOG_ADD_GENERATOR_BASE
{
private:
   NETLIST_DIALOG* m_Parent;

public:
    NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent );

    const wxString GetGeneratorTitle()  { return m_textCtrlName->GetValue(); }
    const wxString GetGeneratorTCommandLine() { return m_textCtrlCommand->GetValue(); }

private:
    /**
     * Function OnOKClick
     * Validate info relative to a new netlist plugin
     */
    void OnOKClick( wxCommandEvent& event ) override;

    /*
     * Browse plugin files, and set m_CommandStringCtrl field
     */
    void OnBrowseGenerators( wxCommandEvent& event ) override;
};


/* Event id for notebook page buttons: */
enum id_netlist {
    ID_CREATE_NETLIST = ID_END_EESCHEMA_ID_LIST + 1,
    ID_CURRENT_FORMAT_IS_DEFAULT,
    ID_ADD_SUBCIRCUIT_PREFIX,
    ID_USE_NETCODE_AS_NETNAME
};


// keywords for configuration:
#define CUSTOM_NETLIST_TITLE   wxT( "CustomNetlistTitle" )
#define CUSTOM_NETLIST_COMMAND wxT( "CustomNetlistCommand" )
#define NETLIST_USE_DEFAULT_NETNAME wxT( "NetlistUseDefaultNetname" )
#define NETLIST_PSPICE_USE_NETNAME  wxT( "SpiceUseNetNames" )


BEGIN_EVENT_TABLE( NETLIST_DIALOG, NETLIST_DIALOG_BASE )
    EVT_CHECKBOX( ID_CURRENT_FORMAT_IS_DEFAULT, NETLIST_DIALOG::SelectDefaultNetlistType )
END_EVENT_TABLE()



NETLIST_PAGE_DIALOG::NETLIST_PAGE_DIALOG( wxNotebook*     parent,
                                          const wxString& title,
                                          NETLIST_TYPE_ID id_NetType ) :
    wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_IdNetType           = id_NetType;
    m_pageNetFmtName      = title;
    m_CommandStringCtrl   = NULL;
    m_TitleStringCtrl     = NULL;
    m_IsCurrentFormat     = NULL;
    m_AdjustPassiveValues = NULL;

    wxString netfmtName = ((NETLIST_DIALOG*)parent->GetParent())->m_DefaultNetFmtName;

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

    wxStaticText* text = new wxStaticText( this, -1, _( "Options:" ) );
    m_LeftBoxSizer->Add( text, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );

    m_IsCurrentFormat = new wxCheckBox( this, ID_CURRENT_FORMAT_IS_DEFAULT, _( "Default format" ) );
    m_LeftBoxSizer->Add( m_IsCurrentFormat, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );
    m_IsCurrentFormat->SetValue( selected );

    if( selected )
        ((NETLIST_DIALOG*)parent->GetParent())->m_asFormatSelected = true;
}



NETLIST_DIALOG::NETLIST_DIALOG( SCH_EDIT_FRAME* parent ) :
    NETLIST_DIALOG_BASE( parent )
{
    m_Parent = parent;
    m_config = Kiface().KifaceSettings();

    long tmp;
    m_config->Read( NETLIST_USE_DEFAULT_NETNAME, &tmp, 0l );
    m_DefaultNetFmtName = m_Parent->GetNetListFormatName();

    for( NETLIST_PAGE_DIALOG*& page : m_PanelNetType)
        page = NULL;

    // Add notebook pages:
    m_asFormatSelected = false;     // Will be set to true, if a format is selected

    m_PanelNetType[PANELPCBNEW] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "Pcbnew" ),
                                                           NET_TYPE_PCBNEW );

    m_PanelNetType[PANELORCADPCB2] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "OrcadPCB2" ),
                                                              NET_TYPE_ORCADPCB2 );

    m_PanelNetType[PANELCADSTAR] = new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "CadStar" ),
                                                            NET_TYPE_CADSTAR );

    InstallPageSpice();
    InstallCustomPages();

    // Ensure a netlist format is selected:
    if( !m_asFormatSelected )
    {
        m_PanelNetType[PANELPCBNEW]->m_IsCurrentFormat->SetValue( true );
        m_NoteBook->SetSelection( PANELPCBNEW );
        m_DefaultNetFmtName = m_PanelNetType[PANELPCBNEW]->GetPageNetFmtName();
        // call OnNetlistTypeSelection to update some widgets.
        // SetSelection() do nothing if the current page is already PANELPCBNEW
        wxNotebookEvent event;
        OnNetlistTypeSelection( event );
    }

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizer2OK->SetLabel( _( "Export Netlist" ) );
    m_sdbSizer2Cancel->SetLabel( _( "Close" ) );
    m_buttonSizer->Layout();

    m_sdbSizer2OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


const wxString NETLIST_DIALOG::UserNetlistTypeName( bool first_item )
{
    static int index;

    if( first_item )
        index = 0;
    else
        index++;

    wxString msg = CUSTOM_NETLIST_TITLE;
    msg << index + 1;

    return m_config->Read( msg );
}


void NETLIST_DIALOG::InstallPageSpice()
{
    NETLIST_PAGE_DIALOG* page = m_PanelNetType[PANELSPICE] =
                    new NETLIST_PAGE_DIALOG( m_NoteBook, wxT( "Spice" ), NET_TYPE_SPICE );

    page->m_AdjustPassiveValues = new wxCheckBox( page, ID_USE_NETCODE_AS_NETNAME,
                                                  _( "Reformat passive symbol values" ) );
    page->m_AdjustPassiveValues->SetToolTip( _( "Reformat passive symbol values e.g. 1M -> 1Meg" ) );
    page->m_AdjustPassiveValues->SetValue( m_Parent->GetSpiceAjustPassiveValues() );
    page->m_LeftBoxSizer->Add( page->m_AdjustPassiveValues, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );
}


void NETLIST_DIALOG::InstallCustomPages()
{
    int                  ii;
    wxString             title;
    NETLIST_PAGE_DIALOG* currPage;

    for( ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        title = UserNetlistTypeName( ii == 0 ? true : false );

        if( title.IsEmpty() )
            break; // No more panel to install

        // Install a plugin panel
        wxString msg = CUSTOM_NETLIST_COMMAND;
        msg << ii + 1;
        wxString command = m_config->Read( msg );

        currPage = AddOneCustomPage( title, command, (NETLIST_TYPE_ID)(NET_TYPE_CUSTOM1 + ii) );
        m_PanelNetType[PANELCUSTOMBASE + ii] = currPage;
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


void NETLIST_DIALOG::SelectDefaultNetlistType( wxCommandEvent& event )
{
    for( NETLIST_PAGE_DIALOG*& page : m_PanelNetType)
    {
        if( page )
            page->m_IsCurrentFormat->SetValue( false );
    }

    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( currPage == NULL )
        return;

    m_DefaultNetFmtName = currPage->GetPageNetFmtName();
    m_Parent->SetNetListFormatName( m_DefaultNetFmtName );
    currPage->m_IsCurrentFormat->SetValue( true );
}


void NETLIST_DIALOG::OnNetlistTypeSelection( wxNotebookEvent& event )
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( currPage == NULL )
        return;

    m_buttonDelGenerator->Enable( currPage->m_IdNetType >= NET_TYPE_CUSTOM1 );
}


void NETLIST_DIALOG::NetlistUpdateOpt()
{
    bool adjust = m_PanelNetType[ PANELSPICE ]->m_AdjustPassiveValues->IsChecked();

    m_Parent->SetSpiceAdjustPassiveValues( adjust );
    m_Parent->SetNetListFormatName( wxEmptyString );

    for( NETLIST_PAGE_DIALOG*& page : m_PanelNetType)
    {
        if( page == NULL )
            break;

        if( page->m_IsCurrentFormat->GetValue() == true )
            m_Parent->SetNetListFormatName( page->GetPageNetFmtName() );
    }
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
    fn = g_RootSheet->GetScreen()->GetFileName();
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
    }
        break;
    }

    fn.SetExt( fileExt );

    if( fn.GetPath().IsEmpty() )
       fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );

    wxString fullpath = fn.GetFullPath();
    wxString fullname = fn.GetFullName();
    wxString path     = fn.GetPath();

    // fullname does not and should not include the path, per wx docs.
    wxFileDialog dlg( this, title, path, fullname, fileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullpath = dlg.GetPath();   // directory + filename

    m_Parent->ClearMsgPanel();

    if( currPage->m_CommandStringCtrl )
        m_Parent->SetNetListerCommand( currPage->m_CommandStringCtrl->GetValue() );
    else
        m_Parent->SetNetListerCommand( wxEmptyString );

    auto netlist = m_Parent->CreateNetlist( false, false );

    if( netlist == nullptr )
        wxMessageBox( _( "Schematic netlist not available" ) );
    else
        m_Parent->WriteNetListFile( netlist, currPage->m_IdNetType, fullpath, netlist_opt, NULL );

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

    // Update existing custom pages
    int jj = 0;
    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        NETLIST_PAGE_DIALOG* currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == NULL )
            break;

        wxString title = currPage->m_TitleStringCtrl->GetValue();

        if( title.IsEmpty() )
            continue;

        msg = CUSTOM_NETLIST_TITLE;
        msg << jj + 1;
        m_config->Write( msg, title );

        wxString Command = currPage->m_CommandStringCtrl->GetValue();
        msg = CUSTOM_NETLIST_COMMAND;
        msg << jj + 1;
        m_config->Write( msg, Command );
        jj++;
    }

    // Ensure all other pages are void
    for( ; jj < CUSTOMPANEL_COUNTMAX; jj++ )
    {
        msg = CUSTOM_NETLIST_TITLE;
        msg << jj + 1;
        m_config->Write( msg, wxEmptyString );

        msg = CUSTOM_NETLIST_COMMAND;
        msg << jj + 1;
        m_config->Write( msg, wxEmptyString );
    }
}


void NETLIST_DIALOG::OnDelGenerator( wxCommandEvent& event )
{
    NETLIST_PAGE_DIALOG* currPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    currPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    currPage->m_TitleStringCtrl->SetValue( wxEmptyString );

    if( currPage->m_IsCurrentFormat->IsChecked() )
    {
        currPage->m_IsCurrentFormat->SetValue( false );
        m_PanelNetType[PANELPCBNEW]->m_IsCurrentFormat->SetValue( true );
    }

    WriteCurrentNetlistSetup();
    EndModal( NET_PLUGIN_CHANGE );
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

        if( currPage == NULL )
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

    // Close and reopen dialog to rebuild the dialog after changes
    EndModal( NET_PLUGIN_CHANGE );
}


NETLIST_DIALOG_ADD_GENERATOR::NETLIST_DIALOG_ADD_GENERATOR( NETLIST_DIALOG* parent ) :
    NETLIST_DIALOG_ADD_GENERATOR_BASE( parent )
{
    m_Parent = parent;
    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
}


void NETLIST_DIALOG_ADD_GENERATOR::OnOKClick( wxCommandEvent& event )
{
    if( m_textCtrlCommand->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "Error. You must provide a command String" ) );
        return;
    }

    if( m_textCtrlName->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "Error. You must provide a Title" ) );
        return;
    }

    EndModal( wxID_OK );
}


void NETLIST_DIALOG_ADD_GENERATOR::OnBrowseGenerators( wxCommandEvent& event )
{
    wxString FullFileName, Path;

#ifndef __WXMAC__
    Path = Pgm().GetExecutablePath();
#else
    Path = GetOSXKicadDataDir() + wxT( "/plugins" );
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

    if( ext == wxT("xsl" ) )
        cmdLine.Printf(wxT("xsltproc -o \"%%O\" \"%s\" \"%%I\""), GetChars(FullFileName) );
    else if( ext == wxT("exe" ) || ext.IsEmpty() )
        cmdLine.Printf(wxT("\"%s\" > \"%%O\" < \"%%I\""), GetChars(FullFileName) );
    else if( ext == wxT("py" ) || ext.IsEmpty() )
        cmdLine.Printf(wxT("python \"%s\" \"%%I\" \"%%O\""), GetChars(FullFileName) );
    else
        cmdLine.Printf(wxT("\"%s\""), GetChars(FullFileName) );

    m_textCtrlCommand->SetValue( cmdLine );

    /* Get a title for this page */
    wxString title = m_textCtrlName->GetValue();

    if( title.IsEmpty() )
        wxMessageBox( _( "Do not forget to choose a title for this netlist control page" ) );
}


int InvokeDialogNetList( SCH_EDIT_FRAME* aCaller )
{
    NETLIST_DIALOG dlg( aCaller );

    wxString curr_default_netformat = aCaller->GetNetListFormatName();

    int ret = dlg.ShowModal();

    // Update the default netlist and store it in prj config if it was explicitely changed.
    aCaller->SetNetListFormatName( dlg.m_DefaultNetFmtName );   // can have temporary changed

    if( curr_default_netformat != dlg.m_DefaultNetFmtName )
        aCaller->SaveProjectSettings( false );

    return ret;
}
