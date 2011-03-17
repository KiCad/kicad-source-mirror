/***********************************************************/
/* mdiframe.cpp - WinEDA_MainFrame is the kicad main frame */
/***********************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "bitmaps.h"
#include "macros.h"

#include "kicad.h"
#include "tree_project_frame.h"


static const wxString TreeFrameWidthEntry( wxT( "LeftWinWidth" ) );


WinEDA_MainFrame::WinEDA_MainFrame( wxWindow*       parent,
                                    const wxString& title,
                                    const wxPoint&  pos,
                                    const wxSize&   size ) :
    EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME, title, pos, size )
{
    wxString msg;
    wxString line;
    wxSize   clientsize;

    m_FrameName            = wxT( "KicadFrame" );
    m_VToolBar             = NULL;              // No Vertical tooolbar used here
    m_LeftWin              = NULL;              // A shashwindow that contains the project tree
    m_RightWin             = NULL;              /* A shashwindow that contains the buttons
                                                 *  and the window display text
                                                 */
    m_LeftWin_Width        = MAX( 60, GetSize().x/3 );

    LoadSettings();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line (bottom of the frame
    static const int dims[3] = { -1, -1, 100 };

    CreateStatusBar( 3 );
    SetStatusWidths( 3, dims );

    // Give an icon
    #ifdef __WINDOWS__
    SetIcon( wxICON( a_kicad_icon ) );
    #else
    SetIcon( wxICON( kicad_icon ) );
    #endif

    clientsize = GetClientSize();

    // Left window: is the box which display tree project
    m_LeftWin = new TREE_PROJECT_FRAME( this );

    // Bottom Window: box to display messages
    m_RightWin = new RIGHT_KM_FRAME( this );

    msg = wxGetCwd();
    line.Printf( _( "Ready\nWorking dir: %s\n" ), msg.GetData() );
    PrintMsg( line );

    RecreateBaseHToolbar();

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );
    horiz.LeftDockable( false );
    horiz.RightDockable( false );

    if( m_HToolBar )
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Layer( 1 ) );

    if( m_RightWin )
        m_auimgr.AddPane( m_RightWin,
                          wxAuiPaneInfo().Name( wxT( "m_RightWin" ) ).CentrePane().Layer( 1 ) );

    if( m_LeftWin )
        m_auimgr.AddPane( m_LeftWin,
                          wxAuiPaneInfo().Name( wxT( "m_LeftWin" ) ).Floatable( false ).
                          CloseButton( false ).Left().BestSize( m_LeftWin_Width, clientsize.y ).
                          Layer( 1 ).CaptionVisible( false ) );
    m_auimgr.Update();
}


WinEDA_MainFrame::~WinEDA_MainFrame()
{
    m_auimgr.UnInit();
}


/*
 * Put text in the dialog frame
 */
void WinEDA_MainFrame::PrintMsg( const wxString& text )
{
    m_RightWin->m_DialogWin->AppendText( text );
}


/* Resize windows when dragging window borders
 */
void WinEDA_MainFrame::OnSashDrag( wxSashEvent& event )
{
    event.Skip();
}


void WinEDA_MainFrame::OnSize( wxSizeEvent& event )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    event.Skip();
}


void WinEDA_MainFrame::OnCloseWindow( wxCloseEvent& Event )
{
    int px, py;

    UpdateFileHistory( m_ProjectFileName.GetFullPath() );

    if( !IsIconized() )   // save main frame position and size
    {
        GetPosition( &px, &py );
        m_FramePos.x = px;
        m_FramePos.y = py;

        GetSize( &px, &py );
        m_FrameSize.x = px;
        m_FrameSize.y = py;
    }

    Event.SetCanVeto( TRUE );

    SaveSettings();

    // Close the help frame
    if( wxGetApp().m_HtmlCtrl )
    {
        if( wxGetApp().m_HtmlCtrl->GetFrame() )  // returns NULL if no help frame active
            wxGetApp().m_HtmlCtrl->GetFrame()->Close( TRUE );
        wxGetApp().m_HtmlCtrl = NULL;
    }

    m_LeftWin->Show( false );

    Destroy();
}


void WinEDA_MainFrame::OnExit( wxCommandEvent& event )
{
    Close( true );
}

void WinEDA_MainFrame::OnRunBitmapConverter( wxCommandEvent& event )
{
    ExecuteFile( this, BITMAPCONVERTER_EXE, wxEmptyString );
}


void WinEDA_MainFrame::OnRunPcbNew( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( PcbFileExtension );
    ExecuteFile( this, PCBNEW_EXE, QuoteFullPath( fn ) );
}


void WinEDA_MainFrame::OnRunCvpcb( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( NetlistFileExtension );
    ExecuteFile( this, CVPCB_EXE, QuoteFullPath( fn ) );
}


void WinEDA_MainFrame::OnRunEeschema( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( SchematicFileExtension );
    ExecuteFile( this, EESCHEMA_EXE, QuoteFullPath( fn ) );
}


void WinEDA_MainFrame::OnRunGerbview( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );
    wxString path = wxT("\"");
    path += fn.GetPath( wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME ) + wxT("\"");

    ExecuteFile( this, GERBVIEW_EXE, path );
}


void WinEDA_MainFrame::OnOpenTextEditor( wxCommandEvent& event )
{
    wxString editorname = wxGetApp().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, wxEmptyString );
}


void WinEDA_MainFrame::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".*" );
#endif

    mask = _( "Text file (" ) + mask + wxT( ")|" ) + mask;

    wxFileDialog dlg( this, _( "Load File to Edit" ), wxGetCwd(),
                      wxEmptyString, mask, wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = wxT("\"");
    filename += dlg.GetPath() + wxT("\"");
    if( !dlg.GetPath().IsEmpty() &&  !wxGetApp().GetEditorName().IsEmpty() )
        ExecuteFile( this, wxGetApp().GetEditorName(), filename );
}


void WinEDA_MainFrame::OnRefresh( wxCommandEvent& event )
{
    m_LeftWin->ReCreateTreePrj();
}


void WinEDA_MainFrame::ClearMsg()
{
    m_RightWin->m_DialogWin->Clear();
}


/**
 * Load Kicad main frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_MainFrame::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::LoadSettings();
    cfg->Read( TreeFrameWidthEntry, &m_LeftWin_Width );
}


/**
 * Save Kicad main frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_MainFrame::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::SaveSettings();

    cfg->Write( TreeFrameWidthEntry, m_LeftWin->GetSize().x );
}

