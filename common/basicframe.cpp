/******************************************************************/
/* basicframe.cpp - fonctions des classes du type WinEDA_BasicFrame */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

/* wxWidgets about dialog */
#include <wx/aboutdlg.h>
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"
#include <wx/fontdlg.h>

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "online_help.h"
#include "id.h"
#include "confirm.h"
#include "eda_doc.h"
#include "wxstruct.h"


/*******************************************************/
/* Constructeur de WinEDA_BasicFrame: la fenetre generale */
/*******************************************************/

WinEDA_BasicFrame::WinEDA_BasicFrame( wxWindow* father, int idtype,
                                      const wxString& title,
                                      const wxPoint& pos, const wxSize& size,
                                      long style ) :
    wxFrame( father, -1, title, pos, size, style )
{
    wxSize minsize;

    m_Ident  = idtype;
    SetFont( *g_StdFont );
    m_HToolBar       = NULL;
    m_FrameIsActive  = TRUE;
    m_MsgFrameHeight = MSG_PANEL_DEFAULT_HEIGHT;

    minsize.x = 470;
    minsize.y = 350 + m_MsgFrameHeight;
    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    /* Verification des parametres de creation */
    if( (size.x < minsize.x) || (size.y < minsize.y) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );  /* dimx, dimy = dimensions utiles de la
                                                      * zone utilisateur de la fenetre principale */
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;
}


/******************************************/
WinEDA_BasicFrame::~WinEDA_BasicFrame()
/******************************************/
{
    if( wxGetApp().m_HtmlCtrl )
        delete wxGetApp().m_HtmlCtrl;
    wxGetApp().m_HtmlCtrl = NULL;
}


/********************************************/
void WinEDA_BasicFrame::ReCreateMenuBar()
/********************************************/

// Virtual function
{
}


/*********************************************/
void WinEDA_BasicFrame::GetSettings()
/*********************************************/
{
    wxString  text;
    int       Ypos_min;
    wxConfig* config;

    config = wxGetApp().m_EDA_Config;

    if( config )
    {
        text = m_FrameName + wxT( "Pos_x" );
        config->Read( text, &m_FramePos.x );
        text = m_FrameName + wxT( "Pos_y" );
        config->Read( text, &m_FramePos.y );
        text = m_FrameName + wxT( "Size_x" );
        config->Read( text, &m_FrameSize.x, 600 );
        text = m_FrameName + wxT( "Size_y" );
        config->Read( text, &m_FrameSize.y, 400 );
    }

    // Ensure Window title bar is visible
#ifdef __WXMAC__

    // for macOSX, the window must be below system (macOSX) toolbar
    Ypos_min = GetMBarHeight();
#else
    Ypos_min = 0;
#endif
    if( m_FramePos.y < Ypos_min )
        m_FramePos.y = Ypos_min;
}


/*****************************************/
void WinEDA_BasicFrame::SaveSettings()
/*****************************************/
{
    wxString text;
    wxConfig* config;

    config = wxGetApp().m_EDA_Config;

    if( ( config == NULL ) || IsIconized() )
        return;

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    text = m_FrameName + wxT( "Pos_x" );
    config->Write( text, (long) m_FramePos.x );
    text = m_FrameName + wxT( "Pos_y" );
    config->Write( text, (long) m_FramePos.y );
    text = m_FrameName + wxT( "Size_x" );
    config->Write( text, (long) m_FrameSize.x );
    text = m_FrameName + wxT( "Size_y" );
    config->Write( text, (long) m_FrameSize.y );
}


/******************************************************/
void WinEDA_BasicFrame::PrintMsg( const wxString& text )
/******************************************************/
{
    SetStatusText( text );
}


/*************************************************************************/
void WinEDA_BasicFrame::DisplayActivity( int PerCent, const wxString& Text )
/*************************************************************************/

/* Display a bargraph (0 to 50 point length) for a PerCent value from 0 to 100
 */
{
    wxString Line;

    Line = Text;

    PerCent  = (PerCent < 0) ? 0 : PerCent;
    PerCent  = (PerCent > 100) ? 100 : PerCent;
    PerCent /= 2;   // Bargraph is 0 .. 50 points from 0% to 100%
    if( PerCent )
        Line.Pad( PerCent, '*' );

    SetStatusText( Line );
}


/*******************************************************************/
void WinEDA_BasicFrame::SetLastProject( const wxString& FullFileName )
/*******************************************************************/

/* Met a jour la liste des anciens projets
 */
{
    wxGetApp().m_fileHistory.AddFileToHistory( FullFileName );
    ReCreateMenuBar();
}


/**
 * Fetch the file name from the file history list.
 */
wxString WinEDA_BasicFrame::GetFileFromHistory( int cmdId,
                                                const wxString& type )
{
    wxString fn, msg;
    size_t   i;
    int      baseId = wxGetApp().m_fileHistory.GetBaseId();

    wxASSERT( cmdId >= baseId
              && cmdId < baseId + ( int )wxGetApp().m_fileHistory.GetCount() );

    i = ( size_t )( cmdId - baseId );

    if( i < wxGetApp().m_fileHistory.GetCount() )
    {
        fn = wxGetApp().m_fileHistory.GetHistoryFile( i );
        if( !wxFileName::FileExists( fn ) )
        {
            msg = type + _( " file <" ) + fn + _( "> was not found." );
            DisplayError( this, msg );
            wxGetApp().m_fileHistory.RemoveFileFromHistory( i );
            fn = wxEmptyString;
            ReCreateMenuBar();
        }
    }

    return fn;
}


/**************************************************************/
void WinEDA_BasicFrame::GetKicadHelp( wxCommandEvent& event )
/**************************************************************/
{
    wxString msg;
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    if( wxGetApp().m_HtmlCtrl == NULL )
    {
        wxGetApp().InitOnLineHelp();
    }


    if( wxGetApp().m_HtmlCtrl )
    {
        wxGetApp().m_HtmlCtrl->DisplayContents();
        wxGetApp().m_HtmlCtrl->Display( wxGetApp().m_HelpFileName );
    }
    else
    {
        msg.Printf( _( "Help file %s not found" ), wxGetApp().m_HelpFileName.GetData() );
        DisplayError( this, msg );
    }
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    // wxString fullfilename = FindKicadHelpPath() + wxGetApp().m_HelpFileName;
    // if ( wxFileExists(fullfilename) )
    //     GetAssociatedDocument( this, wxEmptyString, fullfilename );
    // else    // Try to find file in English format:
    // {
    //     fullfilename = FindKicadHelpPath() + wxT("../en/") + wxGetApp().m_HelpFileName;;
    //     GetAssociatedDocument( this, wxEmptyString, fullfilename );
    // }

    wxString helpFile = wxGetApp().GetHelpFile();
    if( !helpFile )
    {
        msg.Printf( _( "Help file %s could not be found." ),
                    wxGetApp().m_HelpFileName.c_str() );
        DisplayError( this, msg );
    }
    else
        GetAssociatedDocument( this, wxEmptyString, helpFile );

#else
    #error Help files format not defined
#endif
}

/***********************************************************************/
void WinEDA_BasicFrame::GetKicadAbout( wxCommandEvent& WXUNUSED(event) )
/***********************************************************************/
{
    wxAboutDialogInfo info;
    InitKiCadAbout(info);
    wxAboutBox(info);
}


/********************************************************************/
void WinEDA_BasicFrame::ProcessFontPreferences( int id )
/********************************************************************/
{
    wxFont font;

    switch( id )
    {
    case ID_PREFERENCES_FONT:
        break;

    case ID_PREFERENCES_FONT_STATUS:
        font = wxGetFontFromUser( this, *g_StdFont );
        if( font.Ok() )
        {
            int pointsize = font.GetPointSize();
            *g_StdFont = font;
            SetFont( *g_StdFont );
            if( GetStatusBar() )
                GetStatusBar()->SetFont( *g_StdFont );
            g_StdFontPointSize = pointsize;
        }
        break;

    case ID_PREFERENCES_FONT_DIALOG:
        font = wxGetFontFromUser( this, *g_DialogFont );
        if( font.Ok() )
        {
            int pointsize = font.GetPointSize();
            *g_DialogFont = font;
            SetFont( *g_DialogFont );
            g_DialogFontPointSize = pointsize;
            g_FixedFontPointSize  = pointsize;
            g_FixedFont->SetPointSize( g_FixedFontPointSize );
        }
        break;

    default:
        DisplayError( this, wxT( "WinEDA_BasicFrame::ProcessFontPreferences Internal Error" ) );
        break;
    }
}
