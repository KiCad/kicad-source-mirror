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

#include "common.h"

#include "bitmaps.h"
#include "protos.h"

#include "id.h"

#include "kicad.h"


// Constructor
/*****************************************************************************/
WinEDA_MainFrame::WinEDA_MainFrame( wxWindow* parent,
                                    const wxString& title,
                                    const wxPoint& pos,
                                    const wxSize& size ) :
    WinEDA_BasicFrame( parent, KICAD_MAIN_FRAME, title, pos, size )
/*****************************************************************************/
{
    wxString  msg;
    wxString  line;
    wxSize    clientsize;
    wxConfig* config = wxGetApp().m_EDA_Config;

    m_FrameName         = wxT( "KicadFrame" );
    m_VToolBar          = NULL;
    m_LeftWin           = NULL;
    m_BottomWin         = NULL;
    m_CommandWin        = NULL;
    m_LeftWin_Width     = 200;
    m_CommandWin_Height = 82;

    GetSettings();

    if( config )
    {
      config->Read( wxT( "LeftWinWidth" ), &m_LeftWin_Width );
      config->Read( wxT( "CommandWinWidth" ), &m_CommandWin_Height );
    }

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line (bottom of the frame
    int dims[3] = { -1, -1, 100 };
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

    m_DialogWin = new wxTextCtrl( m_BottomWin, ID_MAIN_DIALOG, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE |
                                  wxNO_BORDER |
                                  wxTE_READONLY );
    m_DialogWin->SetFont( *g_StdFont );

    // m_CommandWin is the box with buttons which launch eechema, pcbnew ...
    m_CommandWin = new WinEDA_CommandFrame( this,
                                            ID_MAIN_COMMAND,
                                            wxPoint( m_LeftWin_Width,
                                                                 0 ),
                                            wxSize( clientsize.x,
                                                    m_CommandWin_Height ),
                                            wxNO_BORDER | wxSW_3D );

    msg = wxGetCwd();
    line.Printf( _( "Ready\nWorking dir: %s\n" ), msg.GetData() );
    PrintMsg( line );

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->DeclareEvent( wxT( "kicad::LoadProject" ) );
#endif
}


/*****************************************************************************/
WinEDA_MainFrame::~WinEDA_MainFrame()
/*****************************************************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config )
    {
        m_LeftWin_Width     = m_LeftWin->GetSize().x;
        m_CommandWin_Height = m_CommandWin->GetSize().y;
        config->Write( wxT( "LeftWinWidth" ), m_LeftWin_Width );
        config->Write( wxT( "CommandWinWidth" ), m_CommandWin_Height );
    }
}


/*******************************************************/
void WinEDA_MainFrame::PrintMsg( const wxString& text )
/*******************************************************/
/*
 * Put text in the dialog frame
 */
{
    m_DialogWin->SetFont( *g_StdFont );
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
    ;

    wxLayoutAlgorithm layout;
    layout.LayoutFrame( this );
    if( m_CommandWin )
        m_CommandWin->Refresh( TRUE );

    event.Skip();
}


/**********************************************************/
void WinEDA_MainFrame::OnCloseWindow( wxCloseEvent& Event )
/**********************************************************/
{
    int px, py;

    SetLastProject( m_PrjFileName );

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


/**********************************************************/
void WinEDA_MainFrame::OnPaint( wxPaintEvent& event )
/**********************************************************/
{
    event.Skip();
}


/*******************************************/
void WinEDA_MainFrame::ReDraw( wxDC* DC )
/*******************************************/
{
}


/**********************************************************************/
void WinEDA_MainFrame::Process_Special_Functions( wxCommandEvent& event )
/**********************************************************************/
{
    int id = event.GetId();

    switch( id )
    {
    case ID_EXIT:
        Close( TRUE );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_MainFrame::Process_Special_Functions error" ) );
        break;
    }
}


/********************************************************/
void WinEDA_MainFrame::Process_Fct( wxCommandEvent& event )
/*********************************************************/
{
    int      id = event.GetId();
    wxString FullFileName = m_PrjFileName;


    switch( id )
    {
    case ID_TO_PCB:
        ChangeFileNameExt( FullFileName, g_BoardExtBuffer );
        AddDelimiterString( FullFileName );
        ExecuteFile( this, PCBNEW_EXE, FullFileName );
        break;

    case ID_TO_CVPCB:
        ChangeFileNameExt( FullFileName, g_NetlistExtBuffer );
        AddDelimiterString( FullFileName );
        ExecuteFile( this, CVPCB_EXE, FullFileName );
        break;

    case ID_TO_EESCHEMA:
        ChangeFileNameExt( FullFileName, g_SchExtBuffer );
        AddDelimiterString( FullFileName );
        ExecuteFile( this, EESCHEMA_EXE, FullFileName );
        break;

    case ID_TO_GERBVIEW:
        FullFileName = wxGetCwd() + STRING_DIR_SEP;
        AddDelimiterString( FullFileName );
        ExecuteFile( this, GERBVIEW_EXE, FullFileName );
        break;

    case ID_TO_EDITOR:
    {
        wxString editorname = GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, wxEmptyString );
    }
        break;

#ifdef KICAD_PYTHON
    case ID_RUN_PYTHON:
    {
        wxString script = EDA_FileSelector( _( "Execute Python Script:" ),
                                            wxEmptyString,              /* Default path */
                                            wxEmptyString,              /* default filename */
                                            wxT( ".py" ),               /* default ext. */
                                            wxT( "*.py" ),              /* mask for filename filter */
                                            this,
                                            wxFD_OPEN,
                                            FALSE
                                            );
        if( script.IsEmpty() )
            break;
        PyHandler::GetInstance()->RunScript( script );
    }
        break;

#endif
    case ID_BROWSE_AN_SELECT_FILE:
    {
        wxString mask( wxT( "*" ) ), extension;
#ifdef __WINDOWS__
        mask     += wxT( ".*" );
        extension = wxT( ".*" );
#endif
        FullFileName = EDA_FileSelector( _( "Load file:" ),
                                         wxGetCwd(),    /* Default path */
                                         wxEmptyString, /* default filename */
                                         extension,     /* default ext. */
                                         mask,          /* mask for filename filter */
                                         this,
                                         wxFD_OPEN,
                                         TRUE
                                         );
        if( !FullFileName.IsEmpty() )
        {
            AddDelimiterString( FullFileName );
            wxString editorname = GetEditorName();
            if( !editorname.IsEmpty() )
                ExecuteFile( this, editorname, FullFileName );
        }
    }
        break;

        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_MainFrame::Process_Fct Internal Error" ) );
        break;
    }
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



#ifdef KICAD_PYTHON
/*****************************************************************************/
void WinEDA_MainFrame::OnRefreshPy()
/*****************************************************************************/
{
  m_LeftWin->ReCreateTreePrj();
}
#endif
