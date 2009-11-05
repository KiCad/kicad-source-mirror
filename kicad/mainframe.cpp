/***********************************************************/

/* mdiframe.cpp - WinEDA_MainFrame is the kicad main frame */
/***********************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "bitmaps.h"

#include "kicad.h"


static const wxString TreeFrameWidthEntry( wxT( "LeftWinWidth" ) );
static const wxString CommandFrameWidthEntry( wxT( "CommandWinWidth" ) );


WinEDA_MainFrame::WinEDA_MainFrame( wxWindow* parent,
                                    const wxString& title,
                                    const wxPoint& pos,
                                    const wxSize& size ) :
    WinEDA_BasicFrame( parent, KICAD_MAIN_FRAME, title, pos, size )
{
    wxString  msg;
    wxString  line;
    wxSize    clientsize;

    m_FrameName         = wxT( "KicadFrame" );
    m_VToolBar          = NULL;
    m_LeftWin           = NULL;
    m_BottomWin         = NULL;
    m_CommandWin        = NULL;
    m_LeftWin_Width     = 200;
    m_CommandWin_Height = 82;

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
    m_LeftWin = new WinEDA_PrjFrame( this, wxDefaultPosition, wxDefaultSize );
    m_LeftWin->SetDefaultSize( wxSize( m_LeftWin_Width, clientsize.y ) );
    m_LeftWin->SetOrientation( wxLAYOUT_VERTICAL );
    m_LeftWin->SetAlignment( wxLAYOUT_LEFT );
    m_LeftWin->SetSashVisible( wxSASH_RIGHT, TRUE );
    m_LeftWin->SetExtraBorderSize( 2 );

#if !defined(KICAD_AUIMANAGER)
    // Bottom Window: box to display messages
    m_BottomWin = new wxSashLayoutWindow( this, ID_BOTTOM_FRAME,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxNO_BORDER | wxSW_3D );
    m_BottomWin->SetDefaultSize( wxSize( clientsize.x, 150 ) );
    m_BottomWin->SetOrientation( wxLAYOUT_HORIZONTAL );
    m_BottomWin->SetAlignment  ( wxLAYOUT_BOTTOM );
    m_BottomWin->SetSashVisible( wxSASH_TOP, TRUE );
    m_BottomWin->SetSashVisible( wxSASH_LEFT, TRUE );
    m_BottomWin->SetExtraBorderSize( 2 );

    m_DialogWin = new wxTextCtrl( m_BottomWin, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxNO_BORDER | wxTE_READONLY );
#else
    m_DialogWin = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxNO_BORDER | wxTE_READONLY );
#endif
    // m_CommandWin is the box with buttons which launch eechema, pcbnew ...
    m_CommandWin = new WinEDA_CommandFrame( this, ID_MAIN_COMMAND,
                                            wxPoint( m_LeftWin_Width, 0 ),
                                            wxSize( clientsize.x,
                                                    m_CommandWin_Height ),
                                            wxNO_BORDER | wxSW_3D );

    msg = wxGetCwd();
    line.Printf( _( "Ready\nWorking dir: %s\n" ), msg.GetData() );
    PrintMsg( line );

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::LoadProject" ) );
#endif

#if defined(KICAD_AUIMANAGER)
    RecreateBaseHToolbar();

    m_auimgr.SetManagedWindow(this);

    wxAuiPaneInfo horiz;
    horiz.Gripper(false);
    horiz.DockFixed(true);
    horiz.Movable(false);
    horiz.Floatable(false);
    horiz.CloseButton(false);
    horiz.CaptionVisible(false);

    wxAuiPaneInfo vert(horiz);

    vert.TopDockable(false).BottomDockable(false);
    horiz.LeftDockable(false).RightDockable(false);

    m_auimgr.AddPane(m_HToolBar,
        wxAuiPaneInfo(horiz).Name(wxT("m_HToolBar")).Top());

    m_auimgr.AddPane(m_DialogWin,
        wxAuiPaneInfo(horiz).Name(wxT("m_DialogWin")).Center());

    m_auimgr.AddPane(m_CommandWin,
        wxAuiPaneInfo().Name(wxT("m_CommandWin")).CentrePane());

    m_auimgr.AddPane(m_LeftWin,
        wxAuiPaneInfo(horiz).Name(wxT("m_LeftWin")).Left().BestSize(clientsize.x/3,clientsize.y));
    m_auimgr.Update();
#endif
}


/*****************************************************************************/
WinEDA_MainFrame::~WinEDA_MainFrame()
/*****************************************************************************/
{
#if defined(KICAD_AUIMANAGER)
m_auimgr.UnInit();
#endif
}


/*******************************************************/
void WinEDA_MainFrame::PrintMsg( const wxString& text )
/*******************************************************/
/*
 * Put text in the dialog frame
 */
{
    m_DialogWin->AppendText( text );
#ifdef DEBUG
    printf("%s\n", (const char*)text.mb_str() );
#endif
}


/****************************************************/
void WinEDA_MainFrame::OnSashDrag( wxSashEvent& event )
/****************************************************/

/* Resize windows when dragging window borders
 */
{
    int    w, h;
    wxSize newsize;

    if( event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE )
        return;

    GetClientSize( &w, &h );

    switch( event.GetId() )
    {
    case ID_LEFT_FRAME:
    {
        m_LeftWin->SetDefaultSize( wxSize( event.GetDragRect().width, -1 ) );
        break;
    }

    case ID_BOTTOM_FRAME:
    {
        newsize = event.GetDragRect().GetSize();
        m_LeftWin->SetDefaultSize( wxSize( w - newsize.x, -1 ) );
        m_BottomWin->SetDefaultSize( wxSize( -1, newsize.y ) );
        m_CommandWin->SetDefaultSize( wxSize( -1, h - newsize.y ) );
        break;
    }

    case ID_MAIN_COMMAND:
    {
        newsize = event.GetDragRect().GetSize();
        m_LeftWin->SetDefaultSize( wxSize( w - newsize.x, -1 ) );
        m_CommandWin->SetDefaultSize( wxSize( -1, newsize.y ) );
        m_BottomWin->SetDefaultSize( wxSize( -1, h - newsize.y ) );
        break;
    }
    }

    wxLayoutAlgorithm layout;
    layout.LayoutFrame( this );
    event.Skip();
}


/************************************************/
void WinEDA_MainFrame::OnSize( wxSizeEvent& event )
/************************************************/
{
    if( m_CommandWin && m_BottomWin )
    {
        int    w, h, dy;
        wxSize bsize, hsize;
        GetClientSize( &w, &h );
        bsize = m_BottomWin->GetSize();
        hsize = m_CommandWin->GetSize();
        dy    = h - hsize.y;
        if( dy < 50 )
        {
            dy      = 50;
            hsize.y = h - dy;
        }
        m_CommandWin->SetDefaultSize( wxSize( -1, hsize.y ) );
        m_BottomWin->SetDefaultSize( wxSize( -1, dy ) );
    }

    wxLayoutAlgorithm layout;
    layout.LayoutFrame( this );
    if( m_CommandWin )
        m_CommandWin->Refresh( TRUE );
#if defined(KICAD_AUIMANAGER)
   if(m_auimgr.GetManagedWindow())
       m_auimgr.Update();
#endif
    event.Skip();
}


/**********************************************************/
void WinEDA_MainFrame::OnCloseWindow( wxCloseEvent& Event )
/**********************************************************/
{
    int px, py;

    SetLastProject( m_ProjectFileName.GetFullPath() );

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

    m_LeftWin->Show(false);

    Destroy();
}


void WinEDA_MainFrame::OnExit( wxCommandEvent& event )
{
    Close( true );
}


void WinEDA_MainFrame::OnRunPcbNew( wxCommandEvent& event )
{
    wxFileName fn( m_ProjectFileName );
    fn.SetExt( BoardFileExtension );
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
    ExecuteFile( this, GERBVIEW_EXE,
                 fn.GetPath( wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME ) );
}


void WinEDA_MainFrame::OnOpenTextEditor( wxCommandEvent& event )
{
    wxString editorname = wxGetApp().GetEditorName();

    if( !editorname.IsEmpty() )
        ExecuteFile( this, editorname, wxEmptyString );
}


#ifdef KICAD_PYTHON
void WinEDA_MainFrame::OnRunPythonScript( wxCommandEvent& event )
{
    wxFileDialog dlg( this, _( "Execute Python Script" ), wxEmptyString,
                      wxEmptyString, _( "Python script (*.py)|*.py" ),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    PyHandler::GetInstance()->RunScript( dlg.GetPath() );
}
#endif


void WinEDA_MainFrame::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    wxString mask( wxT( "*" ) );
#ifdef __WINDOWS__
    mask += wxT( ".*" );
#endif

    mask = _( "Text file (" ) + mask + wxT( ")|" ) + mask;

    wxFileDialog dlg( this, _( "Load File to Edit" ), wxGetCwd(),
                      wxEmptyString, mask,wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( ! dlg.GetPath().IsEmpty() &&  ! wxGetApp().GetEditorName().IsEmpty() )
        ExecuteFile( this, wxGetApp().GetEditorName(), dlg.GetPath() );
}


/********************************************************/
void WinEDA_MainFrame::OnRefresh( wxCommandEvent& event )
/********************************************************/
{
    m_LeftWin->ReCreateTreePrj();
}



/*********************************/
void WinEDA_MainFrame::ClearMsg()
/*********************************/
{
  m_DialogWin->Clear();
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

    WinEDA_BasicFrame::LoadSettings();
    cfg->Read( TreeFrameWidthEntry, &m_LeftWin_Width );
    cfg->Read( CommandFrameWidthEntry, &m_CommandWin_Height );
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

    WinEDA_BasicFrame::SaveSettings();

    cfg->Write( TreeFrameWidthEntry, m_LeftWin->GetSize().x );
    cfg->Write( CommandFrameWidthEntry, m_CommandWin->GetSize().y );
}


#ifdef KICAD_PYTHON
/*****************************************************************************/
void WinEDA_MainFrame::OnRefreshPy()
/*****************************************************************************/
{
    m_LeftWin->ReCreateTreePrj();
}
#endif
