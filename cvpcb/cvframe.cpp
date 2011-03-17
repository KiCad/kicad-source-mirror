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
#include "cvpcb_mainframe.h"
#include "cvstruct.h"
#include "dialog_cvpcb_config.h"
#include "class_DisplayFootprintsFrame.h"
#include "cvpcb_id.h"
#include "dialog_load_error.h"


#include "build_version.h"

#define FRAME_MIN_SIZE_X 450
#define FRAME_MIN_SIZE_Y 300


// option key to close cvpcb after saving files
static const wxString KeepCvpcbOpenEntry( wxT( "KeepCvpcbOpen" ) );
static const wxString FootprintDocFileEntry( wxT( "footprints_doc_file" ) );


BEGIN_EVENT_TABLE( CVPCB_MAINFRAME, EDA_BASE_FRAME )
    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, CVPCB_MAINFRAME::LoadNetList )

    // Menu events
    EVT_MENU( ID_LOAD_PROJECT,
              CVPCB_MAINFRAME::LoadNetList )
    EVT_MENU( ID_SAVE_PROJECT,
              CVPCB_MAINFRAME::SaveQuitCvpcb )
    EVT_MENU( wxID_EXIT,
              CVPCB_MAINFRAME::OnQuit )
    EVT_MENU( ID_GENERAL_HELP,
              CVPCB_MAINFRAME::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT,
              CVPCB_MAINFRAME::GetKicadAbout )
    EVT_MENU( ID_CONFIG_REQ,
              CVPCB_MAINFRAME::ConfigCvpcb )
    EVT_MENU( ID_CONFIG_SAVE,
              CVPCB_MAINFRAME::Update_Config )
    EVT_MENU( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
              CVPCB_MAINFRAME::OnKeepOpenOnSave )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE,
                    ID_LANGUAGE_CHOICE_END,
                    CVPCB_MAINFRAME::SetLanguage )

    // Toolbar events
    EVT_TOOL( ID_CVPCB_QUIT,
              CVPCB_MAINFRAME::OnQuit )
    EVT_TOOL( ID_CVPCB_READ_INPUT_NETLIST,
              CVPCB_MAINFRAME::LoadNetList )
    EVT_TOOL( ID_CVPCB_SAVEQUITCVPCB,
              CVPCB_MAINFRAME::SaveQuitCvpcb )
    EVT_TOOL( ID_CVPCB_CREATE_CONFIGWINDOW,
              CVPCB_MAINFRAME::ConfigCvpcb )
    EVT_TOOL( ID_CVPCB_CREATE_SCREENCMP,
              CVPCB_MAINFRAME::DisplayModule )
    EVT_TOOL( ID_CVPCB_GOTO_FIRSTNA,
              CVPCB_MAINFRAME::ToFirstNA )
    EVT_TOOL( ID_CVPCB_GOTO_PREVIOUSNA,
              CVPCB_MAINFRAME::ToPreviousNA )
    EVT_TOOL( ID_CVPCB_DEL_ASSOCIATIONS,
              CVPCB_MAINFRAME::DelAssociations )
    EVT_TOOL( ID_CVPCB_AUTO_ASSOCIE,
              CVPCB_MAINFRAME::AssocieModule )
    EVT_TOOL( ID_CVPCB_CREATE_STUFF_FILE,
              CVPCB_MAINFRAME::WriteStuffList )
    EVT_TOOL( ID_PCB_DISPLAY_FOOTPRINT_DOC,
              CVPCB_MAINFRAME::DisplayDocFile )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
              CVPCB_MAINFRAME::OnSelectFilteringFootprint )
    EVT_TOOL( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
              CVPCB_MAINFRAME::OnSelectFilteringFootprint )

    // Frame events
    EVT_CHAR( CVPCB_MAINFRAME::OnChar )
    EVT_CLOSE( CVPCB_MAINFRAME::OnCloseWindow )
    EVT_SIZE( CVPCB_MAINFRAME::OnSize )

    // List item events
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_FOOTPRINT_LIST,
                            CVPCB_MAINFRAME::OnLeftClick )
    EVT_LIST_ITEM_ACTIVATED( ID_CVPCB_FOOTPRINT_LIST,
                             CVPCB_MAINFRAME::OnLeftDClick )
    EVT_LIST_ITEM_SELECTED( ID_CVPCB_COMPONENT_LIST,
                            CVPCB_MAINFRAME::OnSelectComponent )

    EVT_UPDATE_UI( ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                   CVPCB_MAINFRAME::OnUpdateKeepOpenOnSave )
END_EVENT_TABLE()

CVPCB_MAINFRAME::CVPCB_MAINFRAME( const wxString& title, long style ) :
    EDA_BASE_FRAME( NULL, CVPCB_FRAME, title, wxDefaultPosition, wxDefaultSize, style )
{
    m_FrameName = wxT( "CvpcbFrame" );

    m_ListCmp = NULL;
    m_FootprintList = NULL;
    DrawFrame      = NULL;
    m_HToolBar     = NULL;
    m_modified     = false;
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

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    horiz.LeftDockable( false ).RightDockable( false );

    if( m_HToolBar )
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top() );

    if( m_ListCmp )
        m_auimgr.AddPane( m_ListCmp,
                          wxAuiPaneInfo(horiz).Name( wxT( "m_ListCmp" ) ).CentrePane() );

    if( m_FootprintList )
        m_auimgr.AddPane( m_FootprintList,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_FootprintList" ) ).
                          Right().BestSize( (int) ( m_FrameSize.x * 0.36 ), m_FrameSize.y ) );

    m_auimgr.Update();
}


CVPCB_MAINFRAME::~CVPCB_MAINFRAME()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config )
    {
        int state = m_HToolBar->GetToolState(
            ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST );
        config->Write( wxT( FILTERFOOTPRINTKEY ), state );
    }

    m_auimgr.UnInit();
}


/**
 * Load Cvpcb main frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void CVPCB_MAINFRAME::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::LoadSettings();
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
void CVPCB_MAINFRAME::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::SaveSettings();
    cfg->Write( KeepCvpcbOpenEntry, m_KeepCvpcbOpen );
    cfg->Write( FootprintDocFileEntry, m_DocModulesFileName );
}


void CVPCB_MAINFRAME::OnSize( wxSizeEvent& event )
{
    event.Skip();
}


void CVPCB_MAINFRAME::OnQuit( wxCommandEvent& event )
{
    Close( TRUE );
}


void CVPCB_MAINFRAME::OnCloseWindow( wxCloseEvent& Event )
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
                if( !IsOK( this, _( "Problem when saving files, exit anyway ?" ) ) )
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
        UpdateFileHistory( m_NetlistFileName.GetFullPath() );
    }

    m_modified = false;
    SaveSettings();
    Destroy();
    return;
}


void CVPCB_MAINFRAME::OnChar( wxKeyEvent& event )
{
    switch( event.GetKeyCode() )
    {
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
        m_ListCmp->SetFocus();
        break;

    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        m_FootprintList->SetFocus();
        break;

    default:
        event.Skip();
        break;
    }
}


void CVPCB_MAINFRAME::ToFirstNA( wxCommandEvent& event )
{
    int ii = 0;
    int selection;

    if( m_components.empty() )
        return;

    selection = m_ListCmp->GetSelection();

    if( selection < 0 )
        selection = 0;

    BOOST_FOREACH( COMPONENT & component, m_components )
    {
        if( component.m_Module.IsEmpty() && ii > selection )
        {
            m_ListCmp->SetSelection( ii );
            return;
        }

        ii++;
    }
    m_ListCmp->SetSelection( selection );
}


void CVPCB_MAINFRAME::ToPreviousNA( wxCommandEvent& event )
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
    m_ListCmp->SetSelection( selection );
}


void CVPCB_MAINFRAME::SaveQuitCvpcb( wxCommandEvent& event )
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

void CVPCB_MAINFRAME::DelAssociations( wxCommandEvent& event )
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

    DisplayStatus();
}


/*
 * Called when click on Load Netlist button or by file history menu entries
 * Read a netlist selected by user
 */
void CVPCB_MAINFRAME::LoadNetList( wxCommandEvent& event )
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
        wxFileDialog dlg( this, _( "Open Net List" ), wxGetCwd(),
                          wxEmptyString, NetlistFileWildcard,
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
        UpdateFileHistory( m_NetlistFileName.GetFullPath() );

        SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                  wxT( " " ) + m_NetlistFileName.GetFullPath() );
    }
    else
    {
        SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() );
    }

    ReCreateMenuBar();
}


void CVPCB_MAINFRAME::ConfigCvpcb( wxCommandEvent& event )
{
    DIALOG_CVPCB_CONFIG ConfigFrame( this );

    ConfigFrame.ShowModal();
}


void CVPCB_MAINFRAME::OnKeepOpenOnSave( wxCommandEvent& event )
{
    m_KeepCvpcbOpen = event.IsChecked();
}


void CVPCB_MAINFRAME::DisplayModule( wxCommandEvent& event )
{
    CreateScreenCmp();
    DrawFrame->RedrawScreen( wxPoint( 0, 0 ), false );
}


/** Vitual function SetLanguage
 * called on a language menu selection
 */
void CVPCB_MAINFRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}


void CVPCB_MAINFRAME::DisplayDocFile( wxCommandEvent& event )
{
    GetAssociatedDocument( this, m_DocModulesFileName,
                           &wxGetApp().GetLibraryPathList() );
}


void CVPCB_MAINFRAME::OnLeftClick( wxListEvent& event )
{
    m_FootprintList->OnLeftClick( event );
}


void CVPCB_MAINFRAME::OnLeftDClick( wxListEvent& event )
{
    m_FootprintList->OnLeftDClick( event );
}


void CVPCB_MAINFRAME::OnSelectComponent( wxListEvent& event )
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
    DisplayStatus();
}


/* Select full/filtered footprint display on tool click
 */
void CVPCB_MAINFRAME::OnSelectFilteringFootprint( wxCommandEvent& event )
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


void CVPCB_MAINFRAME::OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event )
{
    event.Check( m_KeepCvpcbOpen );
}


/** DisplayStatus()
 * Displays info to the status line at bottom of the main frame
 */
void CVPCB_MAINFRAME::DisplayStatus()
{
    wxString msg;
    msg.Printf( _( "Components: %d (free: %d)" ),
                 m_components.size(), m_undefinedComponentCnt );
    SetStatusText( msg, 0 );

    SetStatusText( wxEmptyString, 1 );

    if( m_FootprintList )
    {
        if( m_FootprintList->m_UseFootprintFullList )
            msg.Printf( _( "Footprints (All): %d" ),
                       m_FootprintList->m_ActiveFootprintList->GetCount() );
        else
            msg.Printf( _( "Footprints (filtered): %d" ),
                       m_FootprintList->m_ActiveFootprintList->GetCount() );
    }
    else
        msg.Empty();
    SetStatusText( msg, 2 );
}

/*
 * Read the list of libraries (*.mod files) and populates m_footprints
 * ( list of availaible modules in libs ).
 * for each module are stored
 *      the module name
 *      documentation string
 *      associated keywords
 */
bool CVPCB_MAINFRAME::LoadFootprintFiles( )
{
    /* Check if there are footprint libraries in project file */
    if( m_ModuleLibNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB footprint libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    m_footprints.ReadFootprintFiles(m_ModuleLibNames);

    /* Display error messages, if any */
    if( !m_footprints.m_filesNotFound.IsEmpty() || !m_footprints.m_filesInvalid.IsEmpty() )
    {
        DIALOG_LOAD_ERROR dialog(NULL);
        if( !m_footprints.m_filesNotFound.IsEmpty() )
        {
            wxString message = _("Some files could not be found!");
            dialog.MessageSet(message);
            dialog.ListSet(m_footprints.m_filesNotFound);
        }

        /* Display if there are invalid files */
        if( !m_footprints.m_filesInvalid.IsEmpty() )
        {
            dialog.MessageSet( _("Some files are invalid!"));
            dialog.ListSet(m_footprints.m_filesInvalid);
        }
        dialog.ShowModal();
    }

    return true;
}
