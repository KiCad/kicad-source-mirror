/*
 * @file confirm.cpp
 * utilities to display some error, warning and info short messges
 */

#include <fctsys.h>
#include <common.h>
#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <html_messagebox.h>


void DisplayError( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    if( displaytime > 0 )
        dialog = new wxMessageDialog( parent, text, _( "Warning" ),
                                      wxOK | wxCENTRE | wxICON_INFORMATION );
    else
        dialog = new wxMessageDialog( parent, text, _( "Error" ),
                                      wxOK | wxCENTRE | wxICON_ERROR );

    dialog->ShowModal();
    dialog->Destroy();
}


void DisplayInfoMessage( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    dialog = new wxMessageDialog( parent, text, _( "Info:" ),
                                  wxOK | wxCENTRE | wxICON_INFORMATION );

    dialog->ShowModal();
    dialog->Destroy();
}


void DisplayHtmlInfoMessage( wxWindow* parent, const wxString& title,
                             const wxString& text, const wxSize& size )
{
    HTML_MESSAGE_BOX *dlg = new HTML_MESSAGE_BOX(parent,title, wxDefaultPosition, size );
    dlg->AddHTML_Text( text );
    dlg->ShowModal();
    dlg->Destroy();
}


bool IsOK( wxWindow* parent, const wxString& text )
{
    int ii;

    ii = wxMessageBox( text, _( "Confirmation" ), wxYES_NO | wxCENTRE | wxICON_HAND, parent );

    if( ii == wxYES )
        return true;

    return false;
}
