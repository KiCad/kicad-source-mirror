/***************************************************************/
/* mainframe.cpp - KICAD_MANAGER_FRAME is the kicad main frame */
/***************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"
#include "gestfich.h"
#include "macros.h"

#include "kicad.h"
#include "tree_project_frame.h"


static const wxString TreeFrameWidthEntry( wxT( "LeftWinWidth" ) );


KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow*       parent,
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
    SetIcon( wxICON( icon_kicad ) );
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


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
    m_auimgr.UnInit();
}


void KICAD_MANAGER_FRAME::PrintMsg( const wxString& aText )
{
    m_RightWin->m_DialogWin->AppendText( aText );
}


void KICAD_MANAGER_FRAME::OnSashDrag( wxSashEvent& event )
{
    event.Skip();
}


void KICAD_MANAGER_FRAME::OnSize( wxSizeEvent& event )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    event.Skip();
}


void KICAD_MANAGER_FRAME::OnCloseWindow( wxCloseEvent& Event )
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

    Event.SetCanVeto( true );

    SaveSettings();

    // Close the help frame
    if( wxGetApp().m_HtmlCtrl )
    {
        if( wxGetApp().m_HtmlCtrl->GetFrame() )  // returns NULL if no help frame active
            wxGetApp().m_HtmlCtrl->GetFrame()->Close( true );

        wxGetApp().m_HtmlCtrl = NULL;
    }

    m_LeftWin->Show( false );

    Destroy();
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


void KICAD_MANAGER_FRAME::OnRunBitmapConverter( wxCommandEvent& event )
{
    ExecuteFile( this, BITMAPCONVERTER_EXE, wxEmptyString );
}


void KICAD_MANAGER_FRAME::OnRunPcbCalculator( wxCommandEvent& event )
{
    ExecuteFile( this, PCB_CALCULATOR_EXE, wxEmptyString );
}


void KICAD_MANAGER_FRAME::OnRunPcbNew( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( PcbFileExtension );
    ExecuteFile( this, PCBNEW_EXE, QuoteFullPath( fn ) );
}


void KICAD_MANAGER_FRAME::OnRunCvpcb( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( NetlistFileExtension );
    ExecuteFile( this, CVPCB_EXE, QuoteFullPath( fn ) );
}


void KICAD_MANAGER_FRAME::OnRunEeschema( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );

    fn.SetExt( SchematicFileExtension );
    ExecuteFile( this, EESCHEMA_EXE, QuoteFullPath( fn ) );
}


void KICAD_MANAGER_FRAME::OnRunGerbview( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );
    wxString path = wxT( "\"" );
    path += fn.GetPath( wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME ) + wxT( "\"" );

    ExecuteFile( this, GERBVIEW_EXE, path );
}


void KICAD_MANAGER_FRAME::OnOpenTextEditor( wxCommandEvent& event )
{
    wxString editorname = wxGetApp().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, wxEmptyString );
}


void KICAD_MANAGER_FRAME::OnOpenFileInTextEditor( wxCommandEvent& event )
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

    wxString filename = wxT( "\"" );
    filename += dlg.GetPath() + wxT( "\"" );

    if( !dlg.GetPath().IsEmpty() &&  !wxGetApp().GetEditorName().IsEmpty() )
        ExecuteFile( this, wxGetApp().GetEditorName(), filename );
}


void KICAD_MANAGER_FRAME::OnRefresh( wxCommandEvent& event )
{
    m_LeftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::ClearMsg()
{
    m_RightWin->m_DialogWin->Clear();
}


void KICAD_MANAGER_FRAME::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::LoadSettings();
    cfg->Read( TreeFrameWidthEntry, &m_LeftWin_Width );
}


void KICAD_MANAGER_FRAME::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::SaveSettings();

    cfg->Write( TreeFrameWidthEntry, m_LeftWin->GetSize().x );
}
