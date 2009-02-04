/******************************************************************/
/* mdiframe.cpp - fonctions de la classe du type WinEDA_MainFrame */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"

#include "kicad.h"

#include <wx/fontdlg.h>

static bool ChoosePdfBrowser( WinEDA_MainFrame* parent_frame )

/* routine to choose the prefered Pdf browser
 */
{
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".exe" );
#endif

    wxGetApp().ReadPdfBrowserInfos();
    wxString FullFileName = wxGetApp().m_PdfBrowser;
    FullFileName = EDA_FileSelector( _( "Prefered Pdf Browser:" ),
                                     wxPathOnly( FullFileName ),    /* Default path */
                                     FullFileName,                  /* default filename */
                                     wxEmptyString,                 /* default filename extension */
                                     mask,                          /* filter for filename list */
                                     parent_frame,                  /* parent frame */
                                     wxFD_OPEN,                     /* wxFD_SAVE, wxFD_OPEN ..*/
                                     TRUE                           /* true = keep the current path */
                                     );
    if( !FullFileName.IsEmpty() && (wxGetApp().m_PdfBrowser != FullFileName) )
    {
        wxGetApp().m_PdfBrowser = FullFileName;
        wxGetApp().WritePdfBrowserInfos();
        return TRUE;
    }
    return FALSE;
}


/****************************************************************/
void WinEDA_MainFrame::Process_Preferences( wxCommandEvent& event )
/*****************************************************************/
{
    int      id = event.GetId();
    wxString FullFileName;
    wxString mask( wxT( "*" ) );

#ifdef __WINDOWS__
    mask += wxT( ".exe" );
#endif

    switch( id )
    {
    case ID_SELECT_DEFAULT_PDF_BROWSER:
        wxGetApp().m_PdfBrowserIsDefault = TRUE;
        GetMenuBar()->Check( ID_SELECT_DEFAULT_PDF_BROWSER,
                             wxGetApp().m_PdfBrowserIsDefault );
        GetMenuBar()->Check( ID_SELECT_PREFERED_PDF_BROWSER,
                             !wxGetApp().m_PdfBrowserIsDefault );
        wxGetApp().WritePdfBrowserInfos();
        break;

    case ID_SELECT_PREFERED_PDF_BROWSER:
        if( wxGetApp().m_PdfBrowser.IsEmpty() )
        {
            DisplayError( this,
                          _( "You must choose a PDF viewer before use this option" ) );
            ChoosePdfBrowser( this );
        }
        if( wxGetApp().m_PdfBrowser.IsEmpty() )
        {
            wxGetApp().m_PdfBrowserIsDefault = TRUE;
            GetMenuBar()->Check( ID_SELECT_DEFAULT_PDF_BROWSER, TRUE );
            GetMenuBar()->Check( ID_SELECT_PREFERED_PDF_BROWSER, FALSE );
        }
        else
        {
            wxGetApp().m_PdfBrowserIsDefault = FALSE;
            GetMenuBar()->Check( ID_SELECT_DEFAULT_PDF_BROWSER, FALSE );
            GetMenuBar()->Check( ID_SELECT_PREFERED_PDF_BROWSER, TRUE );
        }
        wxGetApp().WritePdfBrowserInfos();
        break;

    case ID_SELECT_PREFERED_PDF_BROWSER_NAME:
        ChoosePdfBrowser( this );
        break;

    case ID_SELECT_PREFERED_EDITOR:
        FullFileName = EDA_FileSelector( _( "Prefered Editor:" ),
                                         wxPathOnly( g_EditorName ),    /* Default path */
                                         g_EditorName,                  /* default filename */
                                         wxEmptyString,                 /* default filename extension */
                                         mask,                          /* filter for filename list */
                                         this,                          /* parent frame */
                                         wxFD_OPEN,                     /* wxFD_SAVE, wxFD_OPEN ..*/
                                         TRUE                           /* true = keep the current path */
                                         );
        if( ( !FullFileName.IsEmpty() ) && wxGetApp().m_EDA_CommonConfig )
        {
            g_EditorName = FullFileName;
            wxGetApp().m_EDA_CommonConfig->Write( wxT( "Editor" ),
                                                  g_EditorName );
        }
        break;

    case ID_PREFERENCES_FONT_INFOSCREEN:
    {
        wxFont font = wxGetFontFromUser( this, *g_StdFont );
        if( font.Ok() )
        {
            int pointsize = font.GetPointSize();
            *g_StdFont = font;
            g_StdFontPointSize    = pointsize;
            g_DialogFontPointSize = pointsize;
            g_FixedFontPointSize  = pointsize;
            m_LeftWin->ReCreateTreePrj();
            m_DialogWin->SetFont( *g_StdFont );
            m_DialogWin->Refresh();
        }
        break;
    }

    default:
        DisplayError( this,
                      wxT( "WinEDA_MainFrame::Process_Preferences Internal Error" ) );
        break;
    }
}


/********************************************************/
void WinEDA_MainFrame::SetLanguage( wxCommandEvent& event )
/********************************************************/
{
    wxGetApp().SetLanguageIdentifier( event.GetId() );
    if ( wxGetApp().SetLanguage() )
    {
        wxLogDebug( wxT( "Recreating menu bar due to language change." ) );
        ReCreateMenuBar();
        Refresh();
    }
}
