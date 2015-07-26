/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file confirm.cpp
 * @brief utilities to display some error, warning and info short messges
 */

#include <fctsys.h>
#include <common.h>
#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <wx/stockitem.h>
#include <html_messagebox.h>
#include <dialog_exit_base.h>
#include <bitmaps.h>

class DIALOG_EXIT: public DIALOG_EXIT_BASE
{
public:
    DIALOG_EXIT( wxWindow *aParent, const wxString& aMessage ) :
        DIALOG_EXIT_BASE( aParent )
    {
        m_bitmap->SetBitmap( KiBitmap( dialog_warning_xpm ) );

        if( !aMessage.IsEmpty() )
            m_TextInfo->SetLabel( aMessage );

        GetSizer()->Fit( this );
        GetSizer()->SetSizeHints( this );
    };

private:
    void OnSaveAndExit( wxCommandEvent& event ) { EndModal( wxID_YES ); }
    void OnCancel( wxCommandEvent& event ) { EndModal( wxID_CANCEL ); }
    void OnExitNoSave( wxCommandEvent& event ) { EndModal( wxID_NO ); }
};


int DisplayExitDialog( wxWindow* parent, const wxString& aMessage )
{
    DIALOG_EXIT dlg( parent, aMessage );

    int ret = dlg.ShowModal();
    return ret;
}


void DisplayError( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    if( displaytime > 0 )
        dialog = new wxMessageDialog( parent, text, _( "Warning" ),
                                      wxOK | wxCENTRE | wxICON_INFORMATION
                                      | wxRESIZE_BORDER
                                      );
    else
        dialog = new wxMessageDialog( parent, text, _( "Error" ),
                                      wxOK | wxCENTRE | wxICON_ERROR
                                      | wxRESIZE_BORDER
                                      );

    dialog->ShowModal();
    dialog->Destroy();
}


void DisplayInfoMessage( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    dialog = new wxMessageDialog( parent, text, _( "Info" ),
                                  wxOK | wxCENTRE | wxICON_INFORMATION );

    dialog->ShowModal();
    dialog->Destroy();
}


void DisplayHtmlInfoMessage( wxWindow* parent, const wxString& title,
                             const wxString& text, const wxSize& size )
{
    HTML_MESSAGE_BOX dlg( parent, title, wxDefaultPosition, size );

    dlg.AddHTML_Text( text );
    dlg.ShowModal();
}


bool IsOK( wxWindow* aParent, const wxString& aMessage )
{
    wxMessageDialog dlg( aParent, aMessage, _( "Confirmation" ),
                         wxYES_NO | wxCENTRE | wxICON_QUESTION );

    return dlg.ShowModal() == wxID_YES;
}


class DIALOG_YES_NO_CANCEL : public DIALOG_EXIT
{
public:
    DIALOG_YES_NO_CANCEL( wxWindow        *aParent,
                          const wxString& aPrimaryMessage,
                          const wxString& aSecondaryMessage = wxEmptyString,
                          const wxString& aYesButtonText = wxEmptyString,
                          const wxString& aNoButtonText = wxEmptyString,
                          const wxString& aCancelButtonText = wxEmptyString ) :
        DIALOG_EXIT( aParent, aSecondaryMessage )
    {
        m_TextInfo->SetLabel( aPrimaryMessage );

        if( aSecondaryMessage.IsEmpty() )
            m_staticText2->Hide();

        m_buttonSaveAndExit->SetLabel( aYesButtonText.IsEmpty() ? wxGetStockLabel( wxID_YES ) :
                                       aYesButtonText );
        m_buttonExitNoSave->SetLabel( aNoButtonText.IsEmpty() ? wxGetStockLabel( wxID_NO ) :
                                      aNoButtonText );
        m_buttonCancel->SetLabel( aCancelButtonText.IsEmpty() ? wxGetStockLabel( wxID_CANCEL ) :
                                  aCancelButtonText );
        GetSizer()->Fit( this );
        GetSizer()->SetSizeHints( this );
    };
};


int YesNoCancelDialog( wxWindow*       aParent,
                       const wxString& aPrimaryMessage,
                       const wxString& aSecondaryMessage,
                       const wxString& aYesButtonText,
                       const wxString& aNoButtonText,
                       const wxString& aCancelButtonText )
{
    DIALOG_YES_NO_CANCEL dlg( aParent, aPrimaryMessage, aSecondaryMessage,
                              aYesButtonText, aNoButtonText, aCancelButtonText );

    return dlg.ShowModal();
}
