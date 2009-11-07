/*********************/
/* File: cvframe.cpp */
/*********************/
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "macros.h"
#include "confirm.h"
#include "eda_doc.h"
#include "gestfich.h"
#include "param_config.h"
#include "bitmaps.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"
#include "dialog_cvpcb_config.h"


#define FRAME_MIN_SIZE_X 450
#define FRAME_MIN_SIZE_Y 300


// option key to close cvpcb after saving files
static const wxString KeepCvpcbOpenEntry( wxT( "KeepCvpcbOpen" ) );
static const wxString FootprintDocFileEntry( wxT( "footprints_doc_file" ) );


BEGIN_EVENT_TABLE( WinEDA_CvpcbFrame, WinEDA_BasicFrame )
    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_CvpcbFrame::LoadNetList )

    // Menu events
    EVT_MENU( ID_LOAD_PROJECT,
              WinEDA_CvpcbFrame::LoadNetList )
    EVT_MENU( ID_SAVE_PROJECT,
              WinEDA_CvpcbFrame::SaveQuitCvpcb )
    EVT_MENU( ID_CVPCB_QUIT,
              WinEDA_CvpcbFrame::OnQuit )
    EVT_MENU( ID_GENERAL_HELP,
              WinEDA_CvpcbFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT,
              WinEDA_CvpcbFrame::GetKicadAbout )
    EVT_MENU( ID_CONFIG_REQ,
              WinEDA_CvpcbFrame::ConfigCvpcb )
    EVT_MENU( ID_CONFIG_SAVE,
              WinEDA_CvpcbFrame::Update_Config )
    EVT_MENU( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
              WinEDA_CvpcbFrame::OnKeepOpenOnSave )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE,
                    ID_LANGUAGE_CHOICE_END,
                    WinEDA_CvpcbFrame::SetLanguage )

    // Toolbar events
    EVT_TOOL( ID_CVPCB_QUIT,
              WinEDA_CvpcbFrame::OnQuit )
    EVT_TOOL( ID_CVPCB_READ_INPUT_NETLIST,
              WinEDA_CvpcbFrame::LoadNetList )
    EVT_TOOL( ID_CVPCB_SAVEQUITCVPCB,
              WinEDA_CvpcbFrame::SaveQuitCvpcb )
    EVT_TOOL( ID_CVPCB_CREATE_CONFIGWINDOW,
              WinEDA_CvpcbFrame::ConfigCvpcb )
    EVT_TOOL( ID_CVPCB_CREATE_SCREENCMP,
              WinEDA_CvpcbFrame::DisplayModule )
    EVT_TOOL( ID_CVPCB_GOTO_FIRSTNA,
              WinEDA_CvpcbFrame::ToFirstNA )
    EVT_TOOL( ID_CVPCB_GOTO_PREVIOUSNA,
              WinEDA_CvpcbFrame::ToPreviousNA )
    EVT_TOOL( ID_CVPCB_DEL_ASSOCIATIONS,
              WinEDA_CvpcbFrame::DelAssociations )
    EVT_TOOL( ID_CVPCB_AUTO_ASSOCIE,
              WinEDA_CvpcbFrame::AssocieModule )
    EVT_TOOL( ID_CVPCB_CREATE_STUFF_FILE,
              WinEDA_CvpcbFrame::WriteStuffList )
    EVT_TOOL( ID_PCB_DISPLAY_FOOTPRINT_DOC,
              WinEDA_CvpcbFrame::DisplayDocFile )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
              WinEDA_CvpcbFrame::OnSelectFilteringFootprint )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
              WinEDA_CvpcbFrame::OnSelectFilteringFootprint )

    // Frame events
    EVT_CHAR( WinEDA_CvpcbFrame::OnChar )
    EVT_CLOSE( WinEDA_CvpcbFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_CvpcbFrame::OnSize )

    // List item events
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_FOOTPRINT_LIST,
                            WinEDA_CvpcbFrame::OnLeftClick )
    EVT_LIST_ITEM_ACTIVATED( ID_CVPCB_FOOTPRINT_LIST,
                             WinEDA_CvpcbFrame::OnLeftDClick )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_COMPONENT_LIST,
                            WinEDA_CvpcbFrame::OnSelectComponent )

    EVT_UPDATE_UI( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                   WinEDA_CvpcbFrame::OnUpdateKeepOpenOnSave )
END_EVENT_TABLE()

WinEDA_CvpcbFrame::WinEDA_CvpcbFrame( const wxString& title,
                                      long            style ) :
    WinEDA_BasicFrame( NULL, CVPCB_FRAME, title, wxDefaultPosition,
                       wxDefaultSize, style )
{
    m_FrameName = wxT( "CvpcbFrame" );

    m_ListCmp = NULL;
    m_FootprintList = NULL;
    DrawFrame      = NULL;
    m_HToolBar     = NULL;
    m_modified     = false;
    m_rightJustify = false;
    m_isEESchemaNetlist     = false;
    m_KeepCvpcbOpen         = false;
    m_undefinedComponentCnt = 0;


    /* Name of the document footprint list
     * usually located in share/modules/footprints_doc
     * this is of the responsibility to users to create this file
     * if they want to have a list of footprints
     */
    m_DocModulesFileName = DEFAULT_FOOTPRINTS_LIST_FILENAME;

    // Give an icon
    #ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_cvpcb ) );
    #else
    SetIcon( wxICON( icon_cvpcb ) );
    #endif

    SetAutoLayout( TRUE );

    LoadSettings();
    if( m_FrameSize.x < FRAME_MIN_SIZE_X )
        m_FrameSize.x = FRAME_MIN_SIZE_X;
    if( m_FrameSize.y < FRAME_MIN_SIZE_Y )
        m_FrameSize.y = FRAME_MIN_SIZE_Y;

    // Set minimal frame width and height
    SetSizeHints( FRAME_MIN_SIZE_X, FRAME_MIN_SIZE_Y, -1, -1, -1, -1 );

    // Framesize and position
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // create the status bar
    static const int dims[3] = { -1, -1, 250 };

    CreateStatusBar( 3 );
    SetStatusWidths( 3, dims );

    ReCreateMenuBar();
    ReCreateHToolbar();

    // Create list of available modules and components of the schematic
    BuildCmpListBox();
    BuildFOOTPRINTS_LISTBOX();

#if !defined(KICAD_AUIMANAGER)
    /* Create size constraints of the component list window display. */
    wxLayoutConstraints* linkpos = new wxLayoutConstraints;
    linkpos->top.SameAs( this, wxTop );
    linkpos->bottom.SameAs( this, wxBottom );
    linkpos->left.SameAs( this, wxLeft );
    linkpos->width.PercentOf( this, wxWidth, 66 );
    if( m_ListCmp )
        m_ListCmp->SetConstraints( linkpos );

    /* Create size constraints for the footprint display window. */
    linkpos = new wxLayoutConstraints;
    linkpos->top.SameAs( m_ListCmp, wxTop );
    linkpos->bottom.SameAs( m_ListCmp, wxBottom );
    linkpos->right.SameAs( this, wxRight );
    linkpos->left.SameAs( m_ListCmp, wxRight );
    if( m_FootprintList )
        m_FootprintList->SetConstraints( linkpos );
#endif

#if defined(KICAD_AUIMANAGER)
    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    horiz.LeftDockable( false ).RightDockable( false );

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top() );

    m_auimgr.AddPane( m_ListCmp,
                      wxAuiPaneInfo(horiz).Name( wxT( "m_ListCmp" ) ).CentrePane() );

    m_auimgr.AddPane( m_FootprintList,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_FootprintList" ) ).
                      Right().BestSize( m_FrameSize.x * 0.36, m_FrameSize.y ) );

    m_auimgr.Update();
#endif
}


WinEDA_CvpcbFrame::~WinEDA_CvpcbFrame()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config )
    {
        int state = m_HToolBar->GetToolState(
            ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST );
        config->Write( wxT( FILTERFOOTPRINTKEY ), state );
    }

#if defined(KICAD_AUIMANAGER)
    m_auimgr.UnInit();
#endif
}


/**
 * Load Cvpcb main frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_CvpcbFrame::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_BasicFrame::LoadSettings();
    cfg->Read( KeepCvpcbOpenEntry, &m_KeepCvpcbOpen, false );
    cfg->Read( FootprintDocFileEntry, &m_DocModulesFileName,
               DEFAULT_FOOTPRINTS_LIST_FILENAME );
}


/**
 * Save Cvpcb frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_CvpcbFrame::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_BasicFrame::SaveSettings();
    cfg->Write( KeepCvpcbOpenEntry, m_KeepCvpcbOpen );
    cfg->Write( FootprintDocFileEntry, m_DocModulesFileName );
}


void WinEDA_CvpcbFrame::OnSize( wxSizeEvent& event )
{
    event.Skip();
}


void WinEDA_CvpcbFrame::OnQuit( wxCommandEvent& event )
{
    Close( TRUE );
}


void WinEDA_CvpcbFrame::OnCloseWindow( wxCloseEvent& Event )
{
    int diag;

    if( m_modified )
    {
        unsigned        ii;
        wxMessageDialog dialog( this,
                                _( "Net and component list modified.\nSave before exit ?" ),
                                _( "Confirmation" ),
                                wxYES_NO | wxCANCEL | wxICON_EXCLAMATION |
                                wxYES_DEFAULT );

        ii = dialog.ShowModal();

        switch( ii )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            diag = SaveNetList( wxEmptyString );
            if( diag > 0 )
                m_modified = false;
            else if( diag == 0 )
            {
                if( !IsOK( this,
                           _( "Problem when saving files, exit anyway ?" ) ) )
                {
                    Event.Veto();
                    return;
                }
            }
            break;
        }
    }

    // Close the help frame
    if( wxGetApp().m_HtmlCtrl )
    {
        if( wxGetApp().m_HtmlCtrl->GetFrame() )  // returns NULL if no help
                                                 // frame active
            wxGetApp().m_HtmlCtrl->GetFrame()->Close( TRUE );
    }

    if( m_NetlistFileName.IsOk() )
    {
        SetLastProject( m_NetlistFileName.GetFullPath() );
    }

    m_modified = false;
    SaveSettings();
    Destroy();
    return;
}


void WinEDA_CvpcbFrame::OnChar( wxKeyEvent& event )
{
    switch( event.GetKeyCode() )
    {
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
        m_ListCmp->SetFocus();
        break;

    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        m_FootprintList->SetFocus();;
        break;

    default:
        event.Skip();
        break;
    }
}


void WinEDA_CvpcbFrame::ToFirstNA( wxCommandEvent& event )
{
    int ii = 0;
    int selection;

    if( m_components.empty() )
        return;

    selection = m_ListCmp->GetSelection();

    if( selection < 0 )
        selection = 0;

    BOOST_FOREACH( COMPONENT & component, m_components ) {
        if( component.m_Module.IsEmpty() && ii > selection )
        {
            m_ListCmp->SetSelection( ii );
            return;
        }

        ii++;
    }

    wxBell();
    m_ListCmp->SetSelection( selection );
}


void WinEDA_CvpcbFrame::ToPreviousNA( wxCommandEvent& event )
{
    int ii;
    int selection;

    if( m_components.empty() )
        return;

    ii = m_ListCmp->GetCount() - 1;
    selection = m_ListCmp->GetSelection();

    if( selection < 0 )
        selection = m_ListCmp->GetCount() - 1;

    BOOST_REVERSE_FOREACH( COMPONENT & component, m_components )
    {
        if( component.m_Module.IsEmpty() && ii < selection )
        {
            m_ListCmp->SetSelection( ii );
            return;
        }
        ii--;
    }

    wxBell();
    m_ListCmp->SetSelection( selection );
}


void WinEDA_CvpcbFrame::SaveQuitCvpcb( wxCommandEvent& event )
{
    if( SaveNetList( wxEmptyString ) > 0 )
    {
        m_modified = false;
        if( !m_KeepCvpcbOpen )
            Close( TRUE );
    }
}


/* Removes all associations already made
 */

void WinEDA_CvpcbFrame::DelAssociations( wxCommandEvent& event )
{
    wxString Line;

    if( IsOK( this, _( "Delete selections" ) ) )
    {
        m_ListCmp->SetSelection( 0 );

        BOOST_FOREACH( COMPONENT & component, m_components )
        {
            component.m_Module.Empty();
            SetNewPkg( wxEmptyString );
        }

        m_ListCmp->SetSelection( 0 );
        m_undefinedComponentCnt = m_components.size();
    }

    Line.Printf( _( "Components: %d (free: %d)" ), m_components.size(),
                 m_components.size() );
    SetStatusText( Line, 1 );
}


/*
 * Called when click on Load Netlist button or by file history menu entries
 * Read a netlist selected by user
 */
void WinEDA_CvpcbFrame::LoadNetList( wxCommandEvent& event )
{
    wxString   oldPath;
    wxFileName newFileName;
    int        id = event.GetId();

    if( id >= wxID_FILE1 && id <= wxID_FILE9 )
    {
        newFileName = GetFileFromHistory( id, _( "Netlist" ) );
    }
    else
    {
        newFileName = wxFileName( wxGetCwd(), _( "unnamed" ), wxT( "net" ) );

        wxFileDialog dlg( this, _( "Open Net List" ), newFileName.GetPath(),
                          newFileName.GetFullName(), NetlistFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        newFileName = dlg.GetPath();
    }

    if( newFileName == m_NetlistFileName )
        return;

    if( m_NetlistFileName.DirExists() )
        oldPath = m_NetlistFileName.GetPath();

    /* Update the library search path list. */
    if( wxGetApp().GetLibraryPathList().Index( oldPath ) != wxNOT_FOUND )
        wxGetApp().GetLibraryPathList().Remove( oldPath );
    wxGetApp().GetLibraryPathList().Insert( newFileName.GetPath(), 0 );
    m_NetlistFileName = newFileName;

    if( ReadNetList() )
    {
        SetLastProject( m_NetlistFileName.GetFullPath() );

        SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                  wxT( " " ) + m_NetlistFileName.GetFullPath() );
    }
    else
    {
        SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() );
    }

    ReCreateMenuBar();
}


void WinEDA_CvpcbFrame::ConfigCvpcb( wxCommandEvent& event )
{
    DIALOG_CVPCB_CONFIG ConfigFrame( this );

    ConfigFrame.ShowModal();
}


void WinEDA_CvpcbFrame::OnKeepOpenOnSave( wxCommandEvent& event )
{
    m_KeepCvpcbOpen = event.IsChecked();
}


void WinEDA_CvpcbFrame::DisplayModule( wxCommandEvent& event )
{
    CreateScreenCmp();
    DrawFrame->AdjustScrollBars();
    DrawFrame->Recadre_Trace( FALSE );
}


void WinEDA_CvpcbFrame::SetLanguage( wxCommandEvent& event )
{
    int id = event.GetId();

    wxGetApp().SetLanguageIdentifier( id );
    wxGetApp().SetLanguage();
    ReCreateMenuBar();
    Refresh();
}


void WinEDA_CvpcbFrame::DisplayDocFile( wxCommandEvent& event )
{
    GetAssociatedDocument( this, m_DocModulesFileName,
                           &wxGetApp().GetLibraryPathList() );
}


void WinEDA_CvpcbFrame::OnLeftClick( wxListEvent& event )
{
    m_FootprintList->OnLeftClick( event );
}


void WinEDA_CvpcbFrame::OnLeftDClick( wxListEvent& event )
{
    m_FootprintList->OnLeftDClick( event );
}


void WinEDA_CvpcbFrame::OnSelectComponent( wxListEvent& event )
{
    int selection;

    if( !m_HToolBar->GetToolState( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST ) )
    {
        m_FootprintList->SetActiveFootprintList( TRUE, TRUE );
        return;
    }

    selection = m_ListCmp->GetSelection();
    if( selection < 0 )
    {
        m_FootprintList->SetActiveFootprintList( TRUE, TRUE );
        return;
    }

    if( &m_components[ selection ] == NULL )
    {
        m_FootprintList->SetActiveFootprintList( TRUE, TRUE );
        return;
    }

    m_FootprintList->SetFootprintFilteredList( &m_components[ selection ],
                                               m_footprints );
}


/* Select full/filtered footprint display on tool click
 */
void WinEDA_CvpcbFrame::OnSelectFilteringFootprint( wxCommandEvent& event )
{
    switch (event.GetId() )
    {
        case ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST:
            m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST, false );
            break;
        case ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST:
            m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST, false );
            break;
        default:
        break;
    }
    wxListEvent l_event;

    OnSelectComponent( l_event );
}


void WinEDA_CvpcbFrame::OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event )
{
    event.Check( m_KeepCvpcbOpen );
}
