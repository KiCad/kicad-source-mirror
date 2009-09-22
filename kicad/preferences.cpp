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

#include "kicad.h"

#include <wx/fontdlg.h>


void WinEDA_MainFrame::OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( wxGetApp().m_PdfBrowserIsDefault );
}


void WinEDA_MainFrame::OnSelectDefaultPdfBrowser( wxCommandEvent& event )
{
    wxGetApp().m_PdfBrowserIsDefault = true;
    wxGetApp().WritePdfBrowserInfos();
}


void WinEDA_MainFrame::OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( !wxGetApp().m_PdfBrowserIsDefault );
}


void WinEDA_MainFrame::OnSelectPreferredPdfBrowser( wxCommandEvent& event )
{
    bool select = event.GetId() == ID_SELECT_PREFERED_PDF_BROWSER_NAME;

    if( !wxGetApp().m_PdfBrowser && !select )
    {
        DisplayError( this,
                      _( "You must choose a PDF viewer before using this option." ) );
    }

    wxString wildcard( wxT( "*" ) );

#ifdef __WINDOWS__
    wildcard += wxT( ".exe" );
#endif

    wildcard = _( "Executable files (" ) + wildcard + wxT( ")|" ) + wildcard;

    wxGetApp().ReadPdfBrowserInfos();
    wxFileName fn = wxGetApp().m_PdfBrowser;
    wxFileDialog dlg( this, _( "Select Preferred Pdf Browser" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().m_PdfBrowser = dlg.GetPath();
    wxGetApp().m_PdfBrowserIsDefault = wxGetApp().m_PdfBrowser.IsEmpty();
    wxGetApp().WritePdfBrowserInfos();
}


void WinEDA_MainFrame::OnSelectPreferredEditor( wxCommandEvent& event )
{
    wxFileName fn = wxGetApp().m_EditorName;
    wxString wildcard( wxT( "*" ) );

#ifdef __WINDOWS__
    wildcard += wxT( ".exe" );
#endif

    wildcard = _( "Executable file (" ) + wildcard + wxT( ")|" ) + wildcard;

    wxFileDialog dlg( this, _( "Select Prefered Editor" ), fn.GetPath(),
                      fn.GetFullName(), wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxASSERT( wxGetApp().m_EDA_CommonConfig );

    wxConfig* cfg = wxGetApp().m_EDA_CommonConfig;
    wxGetApp().m_EditorName = dlg.GetPath();
    cfg->Write( wxT( "Editor" ), wxGetApp().m_EditorName );
}


void WinEDA_MainFrame::SetLanguage( wxCommandEvent& event )
{
    wxGetApp().SetLanguageIdentifier( event.GetId() );
    if ( wxGetApp().SetLanguage() )
    {
        wxLogDebug( wxT( "Recreating menu bar due to language change." ) );
        ReCreateMenuBar();
        Refresh();
    }
}
