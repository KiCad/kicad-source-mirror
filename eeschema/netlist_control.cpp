/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file netlist_control.cpp
 * @brief Dialog box for creating netlists.
 */

/* Functions relative to the dialog creating the netlist for Pcbnew.
 * The dialog is a notebook with 4 fixed netlist format:
 * Pcbnew ORCADPCB2 CADSTAR and SPICE
 * and up to CUSTOMPANEL_COUNTMAX (see netlist.h) user programmable format
 * calling an external converter with convert an intermediate format to the
 * user specific format.
 * these external converters are referred there as plugins,
 * but there are not really plugins, there are only external binaries
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <gestfich.h>
#include <wxEeschemaStruct.h>

#include <general.h>
#include <netlist.h>
#include <protos.h>
#include <sch_sheet.h>
#include <dialog_helpers.h>
#include <netlist_control.h>
#include <dialogs/annotate_dialog.h>
#include <wildcards_and_files_ext.h>
#include <wildcards_and_files_ext.h>


//Imported function:
int TestDuplicateSheetNames( bool aCreateMarker );

// ID for configuration:
#define CUSTOM_NETLIST_TITLE   wxT( "CustomNetlistTitle" )
#define CUSTOM_NETLIST_COMMAND wxT( "CustomNetlistCommand" )


/**
 * Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first_item = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist names
 */
wxString ReturnUserNetlistTypeName( bool first_item )
{
    static int index;
    wxString   name, msg;

    if( first_item )
        index = 0;
    else
        index++;

    msg = CUSTOM_NETLIST_TITLE;
    msg << index + 1;

    if( wxGetApp().GetSettings() )
        name = wxGetApp().GetSettings()->Read( msg );

    return name;
}


BEGIN_EVENT_TABLE( NETLIST_DIALOG, wxDialog )
    EVT_BUTTON( wxID_CANCEL, NETLIST_DIALOG::OnCancelClick )
    EVT_BUTTON( ID_CREATE_NETLIST, NETLIST_DIALOG::GenNetlist )
    EVT_BUTTON( ID_SETUP_PLUGIN, NETLIST_DIALOG::AddNewPluginPanel )
    EVT_BUTTON( ID_DELETE_PLUGIN, NETLIST_DIALOG::DeletePluginPanel )
    EVT_BUTTON( ID_VALIDATE_PLUGIN, NETLIST_DIALOG::ValidatePluginPanel )
    EVT_CHECKBOX( ID_CURRENT_FORMAT_IS_DEFAULT,
                  NETLIST_DIALOG::SelectNetlistType )
    EVT_CHECKBOX( ID_ADD_SUBCIRCUIT_PREFIX,
                  NETLIST_DIALOG::EnableSubcircuitPrefix )
    EVT_BUTTON( ID_RUN_SIMULATOR, NETLIST_DIALOG::RunSimulator )
END_EVENT_TABLE()


/*******************************/
/* Functions for these classes */
/*******************************/


/* Contructor to create a setup page for one netlist format.
 * Used in Netlist format Dialog box creation
 */
NETLIST_PAGE_DIALOG::NETLIST_PAGE_DIALOG( wxNotebook*     parent,
                                          const wxString& title,
                                          int             id_NetType,
                                          int             idCheckBox,
                                          int             idCreateFile ) :
    wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SUNKEN )
{
    m_IdNetType = id_NetType;
    m_pageNetFmtName = title;
    m_CommandStringCtrl = NULL;
    m_TitleStringCtrl   = NULL;
    m_IsCurrentFormat   = NULL;
    m_AddSubPrefix = NULL;
    m_ButtonCancel = NULL;
    m_NetOption = NULL;
    wxString netfmtName = ((NETLIST_DIALOG*)parent->GetParent())->m_NetFmtName;
    int fmtOption = 0;

    bool selected = m_pageNetFmtName == netfmtName;

    // PCBNEW Format is a special type:
    if( id_NetType == NET_TYPE_PCBNEW )
    {
        if( netfmtName.IsEmpty() )
            selected = true;
        if( netfmtName == wxT("PcbnewAdvanced" ) )
        {
            selected = true;
            fmtOption = 1;
        }
    }


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

    if( idCheckBox )
    {
        wxStaticText* text = new wxStaticText( this, -1, _( "Options:" ) );
        m_LeftBoxSizer->Add( text, 0, wxGROW | wxALL, 5 );

        m_IsCurrentFormat = new wxCheckBox( this, idCheckBox, _( "Default format" ) );
        m_LeftBoxSizer->Add( m_IsCurrentFormat, 0, wxGROW | wxALL, 5 );
        m_IsCurrentFormat->SetValue( selected );
    }

    if( id_NetType == NET_TYPE_PCBNEW )
    {
        wxString netlist_opt[2] = { _( "Pcbnew Format" ), _( "Advanced Format" ) };
        m_NetOption = new wxRadioBox( this, -1, _( "Netlist Options:" ),
                                      wxDefaultPosition, wxDefaultSize,
                                      2, netlist_opt, 1,
                                      wxRA_SPECIFY_COLS );
        m_NetOption->SetSelection( fmtOption );
        m_LeftBoxSizer->Add( m_NetOption, 0, wxGROW | wxALL, 5 );
    }


    /* Create the buttons: Create Netlist or browse Plugin and Cancel
     * and a third button for plugins : Remove or Ok button */
    if( idCreateFile )
    {
        wxButton* Button;

        if( idCreateFile == ID_SETUP_PLUGIN )  /* This is the "add plugin" panel */
            Button = new wxButton( this, idCreateFile, _( "&Browse Plugin" ) );
        else
            Button = new wxButton( this, idCreateFile, _( "&Netlist" ) );

        m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
        Button->SetDefault();
        m_ButtonCancel = new wxButton( this, wxID_CANCEL, _( "&Cancel" ) );
        m_RightBoxSizer->Add( m_ButtonCancel, 0, wxGROW | wxALL, 5 );

        /* Add special buttons to plugin panels:
         * for panel plugins: added the "delete" button
         * for the last panel (add plugin) a Ok button is added
         */
        if( idCreateFile == ID_SETUP_PLUGIN )   /* This is the "add plugin" panel: add Ok button  */
        {
            Button = new wxButton( this, ID_VALIDATE_PLUGIN, _( "&Ok" ) );
            m_RightOptionsBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
        }
        else if( id_NetType >= PANELCUSTOMBASE )    /* This is a plugin panel: add delete button */
        {
            Button = new wxButton( this, ID_DELETE_PLUGIN, _( "&Delete" ) );
            m_RightOptionsBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
        }
    }
}

const wxString NETLIST_PAGE_DIALOG::GetPageNetFmtName()
{
    // PCBNEW Format is a special type:
    if( m_IdNetType == NET_TYPE_PCBNEW )
    {
        if( m_NetOption->GetSelection() )
            return wxT( "PcbnewAdvanced" );
        else
            return wxT( "Pcbnew" );
    }
    return m_pageNetFmtName;
}


NETLIST_DIALOG::NETLIST_DIALOG( SCH_EDIT_FRAME* parent ) :
    wxDialog( parent, -1, _( "Netlist" ), wxDefaultPosition,
              wxDefaultSize, DIALOG_STYLE | MAYBE_RESIZE_BORDER )
{
    int ii;

    m_Parent = parent;
    m_NetFmtName = m_Parent->GetNetListFormatName();

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
    {
        m_PanelNetType[ii] = NULL;
    }

    wxBoxSizer* GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( GeneralBoxSizer );

    m_NoteBook = new wxNotebook( this, ID_NETLIST_NOTEBOOK,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxNB_TOP );   // @todo: tabs on top are being hidden on linux

    GeneralBoxSizer->Add( m_NoteBook, 0, wxGROW | wxALL, 5 );

    // Add notebook pages:

    // Add Panel FORMAT PCBNEW
    m_PanelNetType[PANELPCBNEW] =
        new NETLIST_PAGE_DIALOG( m_NoteBook,
                                 wxT( "Pcbnew" ),
                                 NET_TYPE_PCBNEW,
                                 ID_CURRENT_FORMAT_IS_DEFAULT,
                                 ID_CREATE_NETLIST );

    // Add Panel FORMAT ORCADPCB2
    m_PanelNetType[PANELORCADPCB2] =
        new NETLIST_PAGE_DIALOG( m_NoteBook,
                                 wxT( "OrcadPCB2" ),
                                 NET_TYPE_ORCADPCB2,
                                 ID_CURRENT_FORMAT_IS_DEFAULT,
                                 ID_CREATE_NETLIST );

    // Add Panel FORMAT CADSTAR
    m_PanelNetType[PANELCADSTAR] =
        new NETLIST_PAGE_DIALOG( m_NoteBook,
                                 wxT( "CadStar" ),
                                 NET_TYPE_CADSTAR,
                                 ID_CURRENT_FORMAT_IS_DEFAULT,
                                 ID_CREATE_NETLIST );

    // Add Panel spice
    InstallPageSpice();

    // Add custom panels:
    InstallCustomPages();

//    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    Centre();
}


void NETLIST_DIALOG::InstallPageSpice()
{
    wxButton* Button;
    NETLIST_PAGE_DIALOG* page;
    wxString title = wxT( "Spice" );

    page = m_PanelNetType[PANELSPICE] =
        new NETLIST_PAGE_DIALOG( m_NoteBook,
                                 title,
                                 NET_TYPE_SPICE,
                                 0, 0 );

    page->m_IsCurrentFormat = new wxCheckBox( page, ID_CURRENT_FORMAT_IS_DEFAULT,
                                              _( "Default format" ) );
    page->m_IsCurrentFormat->SetValue( m_NetFmtName == title );
    page->m_LeftBoxSizer->Add( page->m_IsCurrentFormat, 1, wxGROW | wxALL, 5 );

    page->m_AddSubPrefix = new wxCheckBox( page, ID_ADD_SUBCIRCUIT_PREFIX,
                                           _( "Prefix references 'U' and 'IC' with 'X'" ) );
    page->m_AddSubPrefix->SetValue( m_Parent->GetAddReferencePrefix() );
    page->m_LeftBoxSizer->Add( page->m_AddSubPrefix, 0, wxGROW | wxALL, 5 );


    wxString netlist_opt[2] = { _( "Use Net Names" ), _( "Use Net Numbers" ) };
    page->m_NetOption = new wxRadioBox( page, -1, _( "Netlist Options:" ),
                                        wxDefaultPosition, wxDefaultSize,
                                        2, netlist_opt, 1,
                                        wxRA_SPECIFY_COLS );

    if( !g_OptNetListUseNames )
        page->m_NetOption->SetSelection( 1 );

    page->m_LeftBoxSizer->Add( page->m_NetOption, 0, wxGROW | wxALL, 5 );

    page->m_LowBoxSizer->Add( new wxStaticText( page, -1, _( "Simulator command:" ) ), 0,
                              wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    page->m_CommandStringCtrl = new wxTextCtrl( page, -1, m_Parent->GetSimulatorCommand(),
                                                wxDefaultPosition, wxDefaultSize );

    page->m_CommandStringCtrl->SetInsertionPoint( 1 );
    page->m_LowBoxSizer->Add( page->m_CommandStringCtrl,
                              0,
                              wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
                              5 );

    // Add buttons
    Button = new wxButton( page, ID_CREATE_NETLIST, _( "Netlist" ) );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
    Button->SetDefault();

    Button = new wxButton( page, ID_RUN_SIMULATOR, _( "&Run Simulator" ) );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( page, wxID_CANCEL, _( "&Cancel" ) );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
}


/* create the pages for custom netlist format selection:
 */
void NETLIST_DIALOG::InstallCustomPages()
{
    int               ii, CustomCount;
    wxString          title, previoustitle, msg;
    NETLIST_PAGE_DIALOG* CurrPage;

    CustomCount   = CUSTOMPANEL_COUNTMAX;
    previoustitle = wxT( "dummy_title" );

    for( ii = 0; ii < CustomCount; ii++ )
    {
        title = ReturnUserNetlistTypeName( ii == 0 ? true : false );

        if( title.IsEmpty() && previoustitle.IsEmpty() )
            break; // No more panel to install

        /* Install the panel "Add Plugin" after
         * the last initialized panel */

        previoustitle = title;

        if( title.IsEmpty() )
            CurrPage =
                m_PanelNetType[PANELCUSTOMBASE + ii] =
                    new NETLIST_PAGE_DIALOG( m_NoteBook,
                                             _( "Add Plugin" ),
                                             NET_TYPE_CUSTOM1 + ii,
                                             ID_CURRENT_FORMAT_IS_DEFAULT,
                                             ID_SETUP_PLUGIN );
        else  /* Install a plugin panel */
            CurrPage =
                m_PanelNetType[PANELCUSTOMBASE + ii] =
                    new NETLIST_PAGE_DIALOG( m_NoteBook,
                                             title,
                                             NET_TYPE_CUSTOM1 + ii,
                                             ID_CURRENT_FORMAT_IS_DEFAULT,
                                             ID_CREATE_NETLIST );

        msg = CUSTOM_NETLIST_COMMAND;
        msg << ii + 1;
        wxString Command = wxGetApp().GetSettings()->Read( msg );

        CurrPage->m_LowBoxSizer->Add( new wxStaticText( CurrPage,
                                                        -1, _( "Netlist command:" ) ), 0,
                                      wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

        CurrPage->m_CommandStringCtrl = new wxTextCtrl( CurrPage, -1, Command,
                                                        wxDefaultPosition, wxDefaultSize );

        CurrPage->m_CommandStringCtrl->SetInsertionPoint( 1 );
        CurrPage->m_LowBoxSizer->Add( CurrPage->m_CommandStringCtrl,
                                      0,
                                      wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
                                      5 );

        CurrPage->m_LowBoxSizer->Add( new wxStaticText( CurrPage,
                                                        -1, _( "Title:" ) ), 0,
                                      wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

        CurrPage->m_TitleStringCtrl = new wxTextCtrl( CurrPage, -1, title,
                                                      wxDefaultPosition, wxDefaultSize );

        CurrPage->m_TitleStringCtrl->SetInsertionPoint( 1 );
        CurrPage->m_LowBoxSizer->Add( CurrPage->m_TitleStringCtrl,
                                      0,
                                      wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
                                      5 );
    }
}


/*
 * Browse plugin files, add a new panel
 * and set m_CommandStringCtrl field
 */
void NETLIST_DIALOG::AddNewPluginPanel( wxCommandEvent& event )
{
    wxString FullFileName, Mask, Path;

    Mask = wxT( "*" );
    Path = wxGetApp().GetExecutablePath();
    FullFileName = EDA_FileSelector( _( "Plugin files:" ),
                                     Path,
                                     FullFileName,
                                     wxEmptyString,
                                     Mask,
                                     this,
                                     wxFD_OPEN,
                                     true
                                     );
    if( FullFileName.IsEmpty() )
        return;

    NETLIST_PAGE_DIALOG* CurrPage;
    CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( CurrPage == NULL )
        return;

    // Creates a default command line, suitable for external tool xslproc:
    // try to build a default command line depending on plugin extension
    wxString cmdLine;
    wxFileName fn( FullFileName );
    wxString ext = fn.GetExt();

    if( ext == wxT("xsl" ) )
        cmdLine.Printf(wxT("xsltproc -o \"%%O\" \"%s\" \"%%I\""), GetChars(FullFileName) );
    else if( ext == wxT("exe" ) || ext.IsEmpty() )
        cmdLine.Printf(wxT("\"%s\" > \"%%O\" < \"%%I\""), GetChars(FullFileName) );
    else
        cmdLine.Printf(wxT("\"%s\""), GetChars(FullFileName) );

    CurrPage->m_CommandStringCtrl->SetValue( cmdLine );

    /* Get a title for this page */
    wxString title = CurrPage->m_TitleStringCtrl->GetValue();

    if( title.IsEmpty() )
        DisplayInfoMessage( this,
                            _( "Do not forget to choose a title for this netlist control page" ) );
}


/* Called when the check box "default format" is clicked
 */
void NETLIST_DIALOG::SelectNetlistType( wxCommandEvent& event )
{
    int ii;
    NETLIST_PAGE_DIALOG* CurrPage;

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
        if( m_PanelNetType[ii] )
            m_PanelNetType[ii]->m_IsCurrentFormat->SetValue( false );

    CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( CurrPage == NULL )
        return;

    m_Parent->SetNetListFormatName( CurrPage->GetPageNetFmtName() );
    CurrPage->m_IsCurrentFormat->SetValue( true );
}


/* Called when the check box m_AddSubPrefix
 * "default format" is clicked
 * ( Spice format only )
 */
void NETLIST_DIALOG::EnableSubcircuitPrefix( wxCommandEvent& event )
{

    NETLIST_PAGE_DIALOG* CurrPage;

    CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( CurrPage == NULL || CurrPage->m_AddSubPrefix == NULL )
        return;

    m_Parent->SetAddReferencePrefix( CurrPage->m_AddSubPrefix->IsChecked() );
}

void NETLIST_DIALOG::NetlistUpdateOpt()
{
    int ii;

    m_Parent->SetSimulatorCommand( m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue() );
    m_Parent->SetNetListFormatName( wxEmptyString );

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
    {
        if( m_PanelNetType[ii] == NULL )
            break;

        if( m_PanelNetType[ii]->m_IsCurrentFormat->GetValue() == true )
            m_Parent->SetNetListFormatName( m_PanelNetType[ii]->GetPageNetFmtName() );
    }

    g_OptNetListUseNames = true; // Used for pspice, gnucap

    if( m_PanelNetType[PANELSPICE]->m_NetOption->GetSelection() == 1 )
        g_OptNetListUseNames = 	false;
}


/**
 * Function GenNetlist
 * Create the netlist file:
 * calculate the filename with the suitable extensions
 * and run the netlist creator
 */
void NETLIST_DIALOG::GenNetlist( wxCommandEvent& event )
{
    wxFileName  fn;
    wxString    fileWildcard;
    wxString    fileExt;
    wxString    title = _( "Save Netlist File" );

    NetlistUpdateOpt();

    NETLIST_PAGE_DIALOG* CurrPage;
    CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    unsigned netlist_opt = 0;

    /* Calculate the netlist filename */
    fn = g_RootSheet->GetScreen()->GetFileName();

    switch( CurrPage->m_IdNetType )
    {
    case NET_TYPE_SPICE:
        fileExt = wxT( "cir" );
        fileWildcard = _( "SPICE netlist file (.cir)|*.cir" );
        // Set spice netlist options:
        if( g_OptNetListUseNames )
            netlist_opt |= NET_USE_NETNAMES;
        if( CurrPage->m_AddSubPrefix->GetValue() )
            netlist_opt |= NET_USE_X_PREFIX;
        break;

    case NET_TYPE_CADSTAR:
        fileExt = wxT( "frp" );
        fileWildcard = _( "CadStar netlist file (.frp)|*.frp" );
        break;

    case NET_TYPE_PCBNEW:
        if( CurrPage->m_NetOption->GetSelection() != 0 )
            netlist_opt = NET_PCBNEW_USE_NEW_FORMAT;
        fileExt = NetlistFileExtension;
        fileWildcard = NetlistFileWildcard;
        break;

    case NET_TYPE_ORCADPCB2:
        fileExt = NetlistFileExtension;
        fileWildcard = NetlistFileWildcard;
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
        fileExt = wxEmptyString;
        fileWildcard = AllFilesWildcard;
        title.Printf( _( "%s Export" ), CurrPage->m_TitleStringCtrl->GetValue().GetData() );
    }

    fn.SetExt( fileExt );

    wxFileDialog dlg( this, title, fn.GetPath(),
                      fn.GetFullName(), fileWildcard,
                      wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_Parent->ClearMsgPanel();

    if( CurrPage->m_CommandStringCtrl )
        m_Parent->SetNetListerCommand( CurrPage->m_CommandStringCtrl->GetValue() );
    else
        m_Parent->SetNetListerCommand( wxEmptyString );

    m_Parent->CreateNetlist( CurrPage->m_IdNetType, dlg.GetPath(), netlist_opt );

    WriteCurrentNetlistSetup();

    EndModal( NET_OK );
}


/* Function CreateNetlist
 *  > test for some issues (missing or duplicate references and sheet names)
 *  > build netlist info
 *  > create the netlist file
 * param aFormat = netlist format (NET_TYPE_PCBNEW ...)
 * param aFullFileName = full netlist file name
 * param aNetlistOptions = netlist options using OR'ed bits (see WriteNetListFile).
 * return true if success.
 */
bool SCH_EDIT_FRAME::CreateNetlist( int aFormat, const wxString& aFullFileName,
                                    unsigned aNetlistOptions )
{
    SCH_SHEET_LIST sheets;
    sheets.AnnotatePowerSymbols();

    // Performs some controls:
    if( CheckAnnotate( NULL, 0 ) )
    {
        if( !IsOK( NULL, _( "Some items are not annotated\n\
Do you want to annotate schematic?" ) ) )
            return false;

        // Schematic must be annotated: call Annotate dialog:
        wxCommandEvent event;
        OnAnnotate( event );

        if( CheckAnnotate( NULL, 0 ) )
            return false;
    }

    // Test duplicate sheet names:
    if( TestDuplicateSheetNames( false ) > 0 )
    {
        if( !IsOK( NULL, _( "Error: duplicate sheet names. Continue?" ) ) )
            return false;
    }

    /* Cleanup the entire hierarchy */
    SCH_SCREENS screens;
    screens.SchematicCleanUp();

    BuildNetListBase();
    bool success = WriteNetListFile( aFormat, aFullFileName, aNetlistOptions );

    return success;
}


void NETLIST_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( NET_ABORT );
}


void NETLIST_DIALOG::RunSimulator( wxCommandEvent& event )
{
    wxFileName fn;
    wxString   ExecFile, CommandLine;

    wxString tmp = m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue();
    tmp.Trim( false );
    tmp.Trim( true );
    m_Parent->SetSimulatorCommand( tmp );
    ExecFile = tmp.BeforeFirst( ' ' );
    CommandLine = tmp.AfterFirst( ' ' );

    /* Calculate the netlist filename */
    fn = g_RootSheet->GetScreen()->GetFileName();
    fn.SetExt( wxT( "cir" ) );
    CommandLine += wxT( " \"" ) + fn.GetFullPath() + wxT( "\"" );

    NETLIST_PAGE_DIALOG* CurrPage;
    CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();
    g_OptNetListUseNames = CurrPage->m_NetOption->GetSelection() == 0;

    // Set spice netlist options:
    unsigned netlist_opt = 0;

    if( g_OptNetListUseNames )
        netlist_opt |= NET_USE_NETNAMES;
    if( CurrPage->m_AddSubPrefix && CurrPage->m_AddSubPrefix->GetValue() )
        netlist_opt |= NET_USE_X_PREFIX;

    if( ! m_Parent->CreateNetlist( CurrPage->m_IdNetType, fn.GetFullPath(),
                                   netlist_opt ) )
        return;

    ExecuteFile( this, ExecFile, CommandLine );
}


/**
 * Function WriteCurrentNetlistSetup
 * Write the current netlist options setup in the configuration
 */
void NETLIST_DIALOG::WriteCurrentNetlistSetup( void )
{
    wxString  msg, Command;
    wxConfig* config = wxGetApp().GetSettings();

    NetlistUpdateOpt();

    // Update the new titles
    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        NETLIST_PAGE_DIALOG* CurrPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( CurrPage == NULL )
            break;

        msg = wxT( "Custom" );
        msg << ii + 1;

        if( CurrPage->m_TitleStringCtrl )
        {
            wxString title = CurrPage->m_TitleStringCtrl->GetValue();
            CurrPage->SetPageNetFmtName( title );

            if( msg != title ) // Title has changed, Update config
            {
                msg = CUSTOM_NETLIST_TITLE;
                msg << ii + 1;
                config->Write( msg, title );
            }
        }

        if( CurrPage->m_CommandStringCtrl )
        {
            Command = CurrPage->m_CommandStringCtrl->GetValue();
            msg     = CUSTOM_NETLIST_COMMAND;
            msg << ii + 1;
            config->Write( msg, Command );
        }
    }
}


/**
 * Function DeletePluginPanel
 * Remove a panel relative to a netlist plugin
 */
void NETLIST_DIALOG::DeletePluginPanel( wxCommandEvent& event )
{
    NETLIST_PAGE_DIALOG* CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    CurrPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    CurrPage->m_TitleStringCtrl->SetValue( wxEmptyString );

    if( CurrPage->m_IsCurrentFormat->IsChecked() )
    {
        CurrPage->m_IsCurrentFormat->SetValue( false );
        m_PanelNetType[PANELPCBNEW]->m_IsCurrentFormat->SetValue( true );
    }

    WriteCurrentNetlistSetup();
    EndModal( NET_PLUGIN_CHANGE );
}


/**
 * Function ValidatePluginPanel
 * Validate the panel info relative to a new netlist plugin
 */
void NETLIST_DIALOG::ValidatePluginPanel( wxCommandEvent& event )
{
    NETLIST_PAGE_DIALOG* CurrPage = (NETLIST_PAGE_DIALOG*) m_NoteBook->GetCurrentPage();

    if( CurrPage->m_CommandStringCtrl->GetValue() == wxEmptyString )
    {
        DisplayError( this, _( "Error. You must provide a command String" ) );
        return;
    }

    if( CurrPage->m_TitleStringCtrl->GetValue() == wxEmptyString )
    {
        DisplayError( this, _( "Error. You must provide a Title" ) );
        return;
    }

    WriteCurrentNetlistSetup();
    EndModal( NET_PLUGIN_CHANGE );
}
