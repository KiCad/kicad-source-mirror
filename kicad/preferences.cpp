/*******************/
/* preferences.cpp */
/*******************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"
#include "gestfich.h"

#include "kicad.h"

#include <wx/fontdlg.h>


void KICAD_MANAGER_FRAME::OnUpdateDefaultPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( wxGetApp().m_PdfBrowserIsDefault );
}


void KICAD_MANAGER_FRAME::OnSelectDefaultPdfBrowser( wxCommandEvent& event )
{
    wxGetApp().m_PdfBrowserIsDefault = true;
    wxGetApp().WritePdfBrowserInfos();
}


void KICAD_MANAGER_FRAME::OnUpdatePreferredPdfBrowser( wxUpdateUIEvent& event )
{
    event.Check( !wxGetApp().m_PdfBrowserIsDefault );
}


void KICAD_MANAGER_FRAME::OnSelectPreferredPdfBrowser( wxCommandEvent& event )
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

void KICAD_MANAGER_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}
