/**********************************/
/* Dialog box for netlist outputs */
/**********************************/

/* Functions relatives to the dialog creating the netlist for pcbnew.
 * The dialo is a notebook with 4 fixed netlist format:
 * PCBNEW ORCADPCB2 CADSTAR and SPICE
 * and up to CUSTOMPANEL_COUNTMAX (see netlist.h) user programmable format
 * calling an external converter with convert an intermediate format to the
 * user specific format.
 * these external converters are refered there as plugins,
 * but there are not really plugins, there are only external binaries
 */

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "netlist.h"
#include "protos.h"

#include "schframe.h"
#include "netlist_control.h"

// ID for configuration:
#define CUSTOM_NETLIST_TITLE   wxT( "CustomNetlistTitle" )
#define CUSTOM_NETLIST_COMMAND wxT( "CustomNetlistCommand" )

/* Loacl variable */

/****************************************************/
wxString ReturnUserNetlistTypeName( bool first_item )
/****************************************************/

/** Function ReturnUserNetlistTypeName
 * to retrieve user netlist type names
 * @param first = true: return first name of the list, false = return next
 * @return a wxString : name of the type netlist or empty string
 * this function must be called first with "first_item" = true
 * and after with "first_item" = false to get all the other existing netlist names
 */
{
    static int index;
    wxString   name, msg;

    if( first_item )
        index = 0;
    else
        index++;

    msg = CUSTOM_NETLIST_TITLE;
    msg << index + 1;

    if( g_EDA_Appl->m_EDA_Config )
        name = g_EDA_Appl->m_EDA_Config->Read( msg );

    return name;
}



BEGIN_EVENT_TABLE( WinEDA_NetlistFrame, wxDialog )
EVT_BUTTON( wxID_CANCEL, WinEDA_NetlistFrame::OnCancelClick )
EVT_BUTTON( ID_CREATE_NETLIST, WinEDA_NetlistFrame::GenNetlist )
EVT_BUTTON( ID_SETUP_PLUGIN, WinEDA_NetlistFrame::SetupPluginData )
EVT_BUTTON( ID_DELETE_PLUGIN, WinEDA_NetlistFrame::DeletePluginPanel )
EVT_BUTTON( ID_VALIDATE_PLUGIN, WinEDA_NetlistFrame::ValidatePluginPanel )
EVT_CHECKBOX( ID_CURRENT_FORMAT_IS_DEFAULT, WinEDA_NetlistFrame::SelectNetlistType )
EVT_BUTTON( ID_RUN_SIMULATOR, WinEDA_NetlistFrame::RunSimulator )
END_EVENT_TABLE()



/*******************************/
/* Functions for these classes */
/*******************************/


/*****************************************************************************/
EDA_NoteBookPage::EDA_NoteBookPage( wxNotebook* parent, const wxString& title,
                                    int id_NetType, int idCheckBox, int idCreateFile ) :
    wxPanel( parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_SUNKEN )
/*****************************************************************************/

/** Contructor to create a setup page for one netlist format.
 * Used in Netlist format Dialog box creation
 * @param parent = wxNotebook * parent
 * @param title = title (name) of the notebook page
 * @param id_NetType = netlist type id
 * @param idCheckBox = event ID attached to the "format is default" check box
 * @param idCreateFile = event ID attached to the "create netlist" button
 */
{
    SetFont( *g_DialogFont );
    m_IdNetType = id_NetType;
    m_CommandStringCtrl = NULL;
    m_TitleStringCtrl   = NULL;
    m_IsCurrentFormat   = NULL;
    m_ButtonCancel = NULL;

    parent->AddPage( this, title, g_NetFormat == m_IdNetType );

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

        if( g_NetFormat == m_IdNetType )
            m_IsCurrentFormat->SetValue( TRUE );
    }

    /* Create the buttons: Create Neltist or browse Plugin and Cancel
     * and a third button for plugins : Remove or Ok button */
    if( idCreateFile )
    {
        wxButton* Button;
        if( idCreateFile == ID_SETUP_PLUGIN )  /* This is the "add plugin" panel */
            Button = new wxButton( this, idCreateFile, _( "&Browse Plugin" ) );
        else
            Button = new wxButton( this, idCreateFile, _( "&Netlist" ) );
        Button->SetForegroundColour( *wxRED );
        m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

        m_ButtonCancel =
            Button = new wxButton( this, wxID_CANCEL, _( "&Cancel" ) );
        Button->SetForegroundColour( *wxBLUE );
        m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

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


/*************************************************************************************/
WinEDA_NetlistFrame::WinEDA_NetlistFrame( WinEDA_SchematicFrame* parent ) :
    wxDialog( parent, -1, _( "Netlist" ), wxDefaultPosition,
              wxDefaultSize, DIALOG_STYLE | MAYBE_RESIZE_BORDER )
/*************************************************************************************/

/* Constructor for the netlist generation dialog box
 */
{
    int ii;

    m_Parent = parent;
    SetFont( *g_DialogFont );

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
    {
        m_PanelNetType[ii] = NULL;
    }

    Centre();

    wxBoxSizer* GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( GeneralBoxSizer );

    m_NoteBook = new wxNotebook( this, ID_NETLIST_NOTEBOOK,
                                 wxDefaultPosition, wxDefaultSize );
    m_NoteBook->SetFont( *g_DialogFont );
    GeneralBoxSizer->Add( m_NoteBook, 0, wxGROW | wxALL, 5 );

    // Add notebook pages:

    // Add Panel FORMAT PCBNEW
    m_PanelNetType[PANELPCBNEW] = new EDA_NoteBookPage( m_NoteBook, wxT(
                                                            "Pcbnew" ), NET_TYPE_PCBNEW,
                                                        ID_CURRENT_FORMAT_IS_DEFAULT,
                                                        ID_CREATE_NETLIST );

    // Add Panel FORMAT ORCADPCB2
    m_PanelNetType[PANELORCADPCB2] = new EDA_NoteBookPage( m_NoteBook, wxT(
                                                               "OrcadPCB2" ), NET_TYPE_ORCADPCB2,
                                                           ID_CURRENT_FORMAT_IS_DEFAULT,
                                                           ID_CREATE_NETLIST );

    // Add Panel FORMAT CADSTAR
    m_PanelNetType[PANELCADSTAR] = new EDA_NoteBookPage( m_NoteBook, wxT(
                                                             "CadStar" ), NET_TYPE_CADSTAR,
                                                         ID_CURRENT_FORMAT_IS_DEFAULT,
                                                         ID_CREATE_NETLIST );

    // Add Panel spice
    InstallPageSpice();

    // Add custom panels:
    InstallCustomPages();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/*************************************************/
void WinEDA_NetlistFrame::InstallPageSpice()
/*************************************************/

/* Create the spice page
 */
{
    wxButton*         Button;
    EDA_NoteBookPage* page;

    page = m_PanelNetType[PANELSPICE] = new EDA_NoteBookPage( m_NoteBook, wxT(
                                                                  "Spice" ), NET_TYPE_SPICE, 0, 0 );

    page->m_IsCurrentFormat = new wxCheckBox( page, ID_CURRENT_FORMAT_IS_DEFAULT,
                                             _( "Default format" ) );
    page->m_IsCurrentFormat->SetValue( g_NetFormat == NET_TYPE_SPICE );
    page->m_LeftBoxSizer->Add( page->m_IsCurrentFormat, 0, wxGROW | wxALL, 5 );

    wxString netlist_opt[2] = { _( "Use Net Names" ), _( "Use Net Numbers" ) };
    m_UseNetNamesInNetlist = new wxRadioBox( page, -1, _( "Netlist Options:" ),
                                             wxDefaultPosition, wxDefaultSize,
                                             2, netlist_opt, 1, wxRA_SPECIFY_COLS );
    if( !g_OptNetListUseNames )
        m_UseNetNamesInNetlist->SetSelection( 1 );
    page->m_LeftBoxSizer->Add( m_UseNetNamesInNetlist, 0, wxGROW | wxALL, 5 );

    page->m_CommandStringCtrl = new WinEDA_EnterText( page,
                                                      _(
                                                          "Simulator command:" ),
                                                      g_SimulatorCommandLine,
                                                      page->m_LowBoxSizer, wxDefaultSize );

    // Add buttons
    Button = new wxButton( page, ID_CREATE_NETLIST, _( "Netlist" ) );
    Button->SetForegroundColour( *wxRED );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( page, ID_RUN_SIMULATOR, _( "&Run Simulator" ) );
    Button->SetForegroundColour( wxColour( 0, 100, 0 ) );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( page, wxID_CANCEL, _( "&Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    page->m_RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
}


/*************************************************/
void WinEDA_NetlistFrame::InstallCustomPages()
/*************************************************/

/* create the pages for custom netlist format selection:
 */
{
    int               ii, CustomCount;
    wxString          title, previoustitle, msg;
    EDA_NoteBookPage* CurrPage;

    CustomCount   = CUSTOMPANEL_COUNTMAX;
    previoustitle = wxT( "dummy_title" );
    for( ii = 0; ii < CustomCount; ii++ )
    {
        title = ReturnUserNetlistTypeName( ii == 0 ? true : false );

        if( title.IsEmpty() && previoustitle.IsEmpty() )
            break; // No more panel to install

        /* Install the panel "Add Plugin" after
         * the last initialised panel */
        previoustitle = title;
        if( title.IsEmpty() )
            CurrPage =
                m_PanelNetType[PANELCUSTOMBASE + ii] =
                    new EDA_NoteBookPage( m_NoteBook, _( "Add Plugin" ),
                                          NET_TYPE_CUSTOM1 + ii,
                                          ID_CURRENT_FORMAT_IS_DEFAULT, ID_SETUP_PLUGIN );
        else  /* Install a plugin panel */
            CurrPage =
                m_PanelNetType[PANELCUSTOMBASE + ii] =
                    new EDA_NoteBookPage( m_NoteBook, title,
                                          NET_TYPE_CUSTOM1 + ii,
                                          ID_CURRENT_FORMAT_IS_DEFAULT, ID_CREATE_NETLIST );

        msg = CUSTOM_NETLIST_COMMAND;
        msg << ii + 1;
        wxString Command = m_Parent->m_Parent->m_EDA_Config->Read( msg );
        CurrPage->m_CommandStringCtrl =
            new WinEDA_EnterText( CurrPage,
                                  _( "Netlist command:" ), Command,
                                  CurrPage->m_LowBoxSizer,
                                  wxDefaultSize );

        CurrPage->m_TitleStringCtrl =
            new WinEDA_EnterText( CurrPage,
                                  _( "Title:" ), title,
                                  CurrPage->m_LowBoxSizer,
                                  wxDefaultSize );
    }
}


/***********************************************************/
void WinEDA_NetlistFrame::SetupPluginData( wxCommandEvent& event )
/***********************************************************/

/* Browse the plugin files and set the m_CommandStringCtrl field
 */
{
    wxString FullFileName, Mask, Path;

    Mask = wxT( "*" );
    Path = g_EDA_Appl->m_BinDir;
    FullFileName = EDA_FileSelector( _( "Plugin files:" ),
                                     Path,          /* Chemin par defaut */
                                     FullFileName,  /* nom fichier par defaut */
                                     wxEmptyString, /* extension par defaut */
                                     Mask,          /* Masque d'affichage */
                                     this,
                                     wxFD_OPEN,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    EDA_NoteBookPage* CurrPage;
    CurrPage = (EDA_NoteBookPage*) m_NoteBook->GetCurrentPage();
    if( CurrPage == NULL )
        return;

    CurrPage->m_CommandStringCtrl->SetValue( FullFileName );

    /* Get a title for this page */
    wxString title = CurrPage->m_TitleStringCtrl->GetValue();
    if( title.IsEmpty() )
        DisplayInfo( this,
                    _(
                        "Do not forget to choose a title for this netlist control page" ) );
}


/*****************************************************************/
void WinEDA_NetlistFrame::SelectNetlistType( wxCommandEvent& event )
/*****************************************************************/
/* Called when the check box "default format" is clicked
*/
{
    int ii;
    EDA_NoteBookPage* CurrPage;

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
        if( m_PanelNetType[ii] )
            m_PanelNetType[ii]->m_IsCurrentFormat->SetValue( FALSE );

    CurrPage = (EDA_NoteBookPage*) m_NoteBook->GetCurrentPage();
    if( CurrPage == NULL )
        return;

    g_NetFormat = CurrPage->m_IdNetType;
    CurrPage->m_IsCurrentFormat->SetValue( TRUE );
}


/***********************************************/
void WinEDA_NetlistFrame::NetlistUpdateOpt()
/***********************************************/
{
    int ii;

    g_SimulatorCommandLine =
        m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue();
    g_NetFormat = NET_TYPE_PCBNEW;

    for( ii = 0; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ii++ )
    {
        if( m_PanelNetType[ii] == NULL )
            break;
        if( m_PanelNetType[ii]->m_IsCurrentFormat->GetValue() == TRUE )
            g_NetFormat = m_PanelNetType[ii]->m_IdNetType;
    }

    g_OptNetListUseNames = TRUE; // Used for pspice, gnucap
    if( m_UseNetNamesInNetlist->GetSelection() == 1 )
        g_OptNetListUseNames = FALSE;
}


/**********************************************************/
void WinEDA_NetlistFrame::GenNetlist( wxCommandEvent& event )
/**********************************************************/

/** Function GenNetlist
 * Create the netlist file:
 * calcualte the filename with the suitable extentions
 * and run the netlist creator
 */
{
    wxString FullFileName, FileExt, Mask;
    wxString msg, Command;
    int      netformat_tmp = g_NetFormat;

    NetlistUpdateOpt();

    EDA_NoteBookPage* CurrPage;

    CurrPage    = (EDA_NoteBookPage*) m_NoteBook->GetCurrentPage();
    g_NetFormat = CurrPage->m_IdNetType;

    /* Calculate the netlist filename */
    FullFileName = g_RootSheet->m_AssociatedScreen->m_FileName;

    switch( g_NetFormat )
    {
    case NET_TYPE_SPICE:
        FileExt = wxT( ".cir" );
        break;

    case NET_TYPE_CADSTAR:
        FileExt = wxT( ".frp" );
        break;

    default:
        FileExt = g_NetExtBuffer;
        break;
    }

    Mask = wxT( "*" ) + FileExt + wxT( "*" );
    ChangeFileNameExt( FullFileName, FileExt );
	FullFileName = FullFileName.AfterLast('/'); 
    FullFileName = EDA_FileSelector( _( "Netlist files:" ),
                                     wxEmptyString, /* Defaut path */
                                     FullFileName,  /* Defaut filename */
                                     FileExt,       /* Defaut extension */
                                     Mask,          /* Mask for filename selection */
                                     this,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    m_Parent->MsgPanel->EraseMsgBox();

    ReAnnotatePowerSymbolsOnly();
    if( CheckAnnotate( m_Parent, 0 ) )
    {
        if( !IsOK( this, _( "Must be Annotated, Continue ?" ) ) )
            return;
    }

    /* Cleanup the entire hierarchy */
	EDA_ScreenList ScreenList;
    for( SCH_SCREEN* screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        bool ModifyWires;
        ModifyWires = screen->SchematicCleanUp( NULL );

        // if wire list has changed, delete the Undo Redo list to avoid
        // pointer problems with deleted data
        if( ModifyWires )
            screen->ClearUndoRedoList();
    }

    m_Parent->BuildNetListBase();
    if( CurrPage->m_CommandStringCtrl )
        g_NetListerCommandLine = CurrPage->m_CommandStringCtrl->GetValue();
    else
        g_NetListerCommandLine.Empty();

    switch( g_NetFormat )
    {
    default:
        WriteNetList( m_Parent, FullFileName, TRUE );
        break;

    case NET_TYPE_CADSTAR:
    case NET_TYPE_ORCADPCB2:
        WriteNetList( m_Parent, FullFileName, FALSE );

    case NET_TYPE_SPICE:
        g_OptNetListUseNames = TRUE; // Used for pspice, gnucap
        if( m_UseNetNamesInNetlist->GetSelection() == 1 )
            g_OptNetListUseNames = FALSE;
        WriteNetList( m_Parent, FullFileName, g_OptNetListUseNames );
        break;
    }

    FreeTabNetList( g_TabObjNet, g_NbrObjNet );
    g_NetFormat = netformat_tmp;

    WriteCurrentNetlistSetup();

    EndModal( NET_OK );
}


/***********************************************************/
void WinEDA_NetlistFrame::OnCancelClick( wxCommandEvent& event )
/***********************************************************/
{
    EndModal( NET_ABORT );
}


/***********************************************************/
void WinEDA_NetlistFrame::RunSimulator( wxCommandEvent& event )
/***********************************************************/
{
    wxString NetlistFullFileName, ExecFile, CommandLine;

    g_SimulatorCommandLine =
        m_PanelNetType[PANELSPICE]->m_CommandStringCtrl->GetValue();
    g_SimulatorCommandLine.Trim( FALSE );
    g_SimulatorCommandLine.Trim( TRUE );
    ExecFile = g_SimulatorCommandLine.BeforeFirst( ' ' );

    CommandLine = g_SimulatorCommandLine.AfterFirst( ' ' );

    /* Calculate the netlist filename */
    NetlistFullFileName = g_RootSheet->m_AssociatedScreen->m_FileName;
    ChangeFileNameExt( NetlistFullFileName, wxT( ".cir" ) );
    AddDelimiterString( NetlistFullFileName );
    CommandLine += wxT( " " ) + NetlistFullFileName;

    ExecuteFile( this, ExecFile, CommandLine );
}


/*********************************************************/
void WinEDA_NetlistFrame::WriteCurrentNetlistSetup( void )
/*********************************************************/

/** Function WriteCurrentNetlistSetup
 * Write the current netlist options setup in the configuration
 */
{
    wxString msg, Command;

    NetlistUpdateOpt();

    // Update the new titles
    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        EDA_NoteBookPage* CurrPage = m_PanelNetType[ii + PANELCUSTOMBASE];
        if( CurrPage == NULL )
            break;
        msg = wxT( "Custom" );
        msg << ii + 1;
        if( CurrPage->m_TitleStringCtrl )
        {
            wxString title = CurrPage->m_TitleStringCtrl->GetValue();
            if( msg != title ) // Title has changed, Update config
            {
                msg = CUSTOM_NETLIST_TITLE;
                msg << ii + 1;
                m_Parent->m_Parent->m_EDA_Config->Write( msg, title );
            }
        }

        if( CurrPage->m_CommandStringCtrl )
        {
            Command = CurrPage->m_CommandStringCtrl->GetValue();
            msg = CUSTOM_NETLIST_COMMAND;
            msg << ii + 1;
            m_Parent->m_Parent->m_EDA_Config->Write( msg, Command );
        }
    }
}


/******************************************************************/
void WinEDA_NetlistFrame::DeletePluginPanel( wxCommandEvent& event )
/******************************************************************/

/** Function DeletePluginPanel
 * Remove a panel relative to a netlist plugin
 */
{
    EDA_NoteBookPage* CurrPage = (EDA_NoteBookPage*) m_NoteBook->GetCurrentPage();

    CurrPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    CurrPage->m_TitleStringCtrl->SetValue( wxEmptyString );
    if( CurrPage->m_IsCurrentFormat->IsChecked() )
    {
        CurrPage->m_IsCurrentFormat->SetValue( FALSE );
        m_PanelNetType[PANELPCBNEW]->m_IsCurrentFormat->SetValue( TRUE );
    }
    WriteCurrentNetlistSetup();
    EndModal( NET_PLUGIN_CHANGE );
}


/******************************************************************/
void WinEDA_NetlistFrame::ValidatePluginPanel( wxCommandEvent& event )
/******************************************************************/

/** Function ValidatePluginPanel
 * Validate the panel info relative to a new netlist plugin
 */
{
    EDA_NoteBookPage* CurrPage = (EDA_NoteBookPage*) m_NoteBook->GetCurrentPage();

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
