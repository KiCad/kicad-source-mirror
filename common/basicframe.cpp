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

#include "fctsys.h"
#include <wx/fontdlg.h>
#include "common.h"
#include "online_help.h"
#include "id.h"


/*******************************************************/
/* Constructeur de WinEDA_BasicFrame: la fenetre generale */
/*******************************************************/

WinEDA_BasicFrame::WinEDA_BasicFrame( wxWindow* father, int idtype,
                                      WinEDA_App* parent, const wxString& title,
                                      const wxPoint& pos, const wxSize& size, long style ) :
    wxFrame( father, -1, title, pos, size, style )
{
    wxSize minsize;

    m_Ident  = idtype;
    m_Parent = parent;
    SetFont( *g_StdFont );
    m_MenuBar        = NULL; // menu du haut d'ecran
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
    if( m_Parent->m_HtmlCtrl )
        delete m_Parent->m_HtmlCtrl;
    m_Parent->m_HtmlCtrl = NULL;
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
    wxString text;
    int      Ypos_min;

    if( m_Parent->m_EDA_Config )
    {
        text = m_FrameName + wxT( "Pos_x" );
        m_Parent->m_EDA_Config->Read( text, &m_FramePos.x );
        text = m_FrameName + wxT( "Pos_y" );
        m_Parent->m_EDA_Config->Read( text, &m_FramePos.y );
        text = m_FrameName + wxT( "Size_x" );
        m_Parent->m_EDA_Config->Read( text, &m_FrameSize.x, 600 );
        text = m_FrameName + wxT( "Size_y" );
        m_Parent->m_EDA_Config->Read( text, &m_FrameSize.y, 400 );
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

    if( !m_Parent || !m_Parent->m_EDA_Config )
        return;

    if( !m_Parent->m_EDA_Config || IsIconized() )
        return;

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    text = m_FrameName + wxT( "Pos_x" );
    m_Parent->m_EDA_Config->Write( text, (long) m_FramePos.x );
    text = m_FrameName + wxT( "Pos_y" );
    m_Parent->m_EDA_Config->Write( text, (long) m_FramePos.y );
    text = m_FrameName + wxT( "Size_x" );
    m_Parent->m_EDA_Config->Write( text, (long) m_FrameSize.x );
    text = m_FrameName + wxT( "Size_y" );
    m_Parent->m_EDA_Config->Write( text, (long) m_FrameSize.y );
}


/******************************************************/
void WinEDA_BasicFrame::PrintMsg( const wxString& text )
/******************************************************/
{
    SetStatusText( text );
#ifdef DEBUG
    printf( "%s\n", (const char*) text.mb_str() );
#endif
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
    unsigned ii;

    if( FullFileName.IsEmpty() )
        return;

    //suppression d'une ancienne trace eventuelle du meme fichier
    for( ii = 0; ii < m_Parent->m_LastProject.GetCount(); )
    {
        if( m_Parent->m_LastProject[ii].IsEmpty() )
            break;
#ifdef __WINDOWS__
        if( m_Parent->m_LastProject[ii].CmpNoCase( FullFileName ) == 0 )
#else
        if( m_Parent->m_LastProject[ii] == FullFileName )
#endif
        {
#if ( ( wxMAJOR_VERSION < 2) || ( ( wxMAJOR_VERSION == 2)&& (wxMINOR_VERSION <= 4 )  ) )
            m_Parent->m_LastProject.Remove( ii );
#else
            m_Parent->m_LastProject.RemoveAt( ii );
#endif
        }
        else
            ii++;
    }

    while( m_Parent->m_LastProject.GetCount() >= m_Parent->m_LastProjectMaxCount )
    {
#if ( ( wxMAJOR_VERSION < 2) || ( ( wxMAJOR_VERSION == 2)&& (wxMINOR_VERSION <= 4 )  ) )
        files.Remove( files.GetCount() - 1 );
#else
        m_Parent->m_LastProject.RemoveAt( m_Parent->m_LastProject.GetCount() - 1 );
#endif
    }

    m_Parent->m_LastProject.Insert( FullFileName, 0 );

    ReCreateMenuBar();
}


/**************************************************/
wxString WinEDA_BasicFrame::GetLastProject( int rang )
/**************************************************/
{
    if( rang < 0 )
        rang = 0;
    if( (unsigned) rang >= m_Parent->m_LastProject.GetCount() )
        return wxEmptyString;
    return m_Parent->m_LastProject[rang];
}


/**************************************************************/
void WinEDA_BasicFrame::GetKicadHelp( wxCommandEvent& event )
/**************************************************************/
{
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    if( m_Parent->m_HtmlCtrl == NULL )
    {
        m_Parent->InitOnLineHelp();
    }


    if( m_Parent->m_HtmlCtrl )
    {
        m_Parent->m_HtmlCtrl->DisplayContents();
        m_Parent->m_HtmlCtrl->Display( m_Parent->m_HelpFileName );
    }
    else
    {
        wxString msg;
        msg.Printf( _( "Help file %s not found" ), m_Parent->m_HelpFileName.GetData() );
        DisplayError( this, msg );
    }
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    wxString fullfilename = FindKicadHelpPath() + _T("kicad.pdf");
    if ( wxFileExists(fullfilename) )
        GetAssociatedDocument( this, wxEmptyString, fullfilename );
    else    // Try to find file in English format:
    {
        fullfilename = FindKicadHelpPath() + wxT("../en/") + m_Parent->m_HelpFileName;;
        GetAssociatedDocument( this, wxEmptyString, fullfilename );
    }

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
