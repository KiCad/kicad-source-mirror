/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/stockitem.h>
#include <wx/richmsgdlg.h>

#include <bitmaps.h>
#include <html_messagebox.h>
#include <dialog_exit_base.h>

#include <functional>


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
    void OnSaveAndExit( wxCommandEvent& event ) override { EndModal( wxID_YES ); }
    void OnExitNoSave( wxCommandEvent& event ) override { EndModal( wxID_NO ); }
};


int DisplayExitDialog( wxWindow* parent, const wxString& aMessage )
{
    DIALOG_EXIT dlg( parent, aMessage );

    int ret = dlg.ShowModal();

    // Returns wxID_YES, wxID_NO, or wxID_CANCEL
    return ret;
}


// DisplayError should be deprecated, use DisplayErrorMessage instead
void DisplayError( wxWindow* parent, const wxString& text, int displaytime )
{
    wxMessageDialog* dialog;

    int icon = displaytime > 0 ? wxICON_INFORMATION : wxICON_ERROR;

    dialog = new wxMessageDialog( parent, text, _( "Warning" ),
                                      wxOK | wxCENTRE | wxRESIZE_BORDER | icon );

    dialog->ShowModal();
    dialog->Destroy();
}


void DisplayErrorMessage( wxWindow* aParent, const wxString& aText, const wxString aExtraInfo )
{
    wxRichMessageDialog* dlg;

    dlg = new wxRichMessageDialog( aParent, aText, _( "Error" ),
                                   wxOK | wxCENTRE | wxRESIZE_BORDER | wxICON_ERROR );

    if( !aExtraInfo.IsEmpty() )
    {
        dlg->ShowDetailedText( aExtraInfo );
    }

    dlg->ShowModal();
    dlg->Destroy();
}


void DisplayInfoMessage( wxWindow* aParent, const wxString& aMessage, const wxString aExtraInfo )
{
    wxRichMessageDialog* dlg;

    dlg = new wxRichMessageDialog( aParent, aMessage, _( "Info" ),
                                   wxOK | wxCENTRE | wxRESIZE_BORDER | wxICON_INFORMATION );

    if( !aExtraInfo.IsEmpty() )
    {
        dlg->ShowDetailedText( aExtraInfo );
    }

    dlg->ShowModal();
    dlg->Destroy();
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


int SelectSingleOption( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage, const wxArrayString& aOptions )
{
    int ret = -1;
    wxDialog* dlg = new DIALOG_SHIM( aParent, wxID_ANY, aTitle );

    wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );

    if( !aMessage.IsEmpty() )
        boxSizer->Add( new wxStaticText( dlg, wxID_ANY, aMessage ), 0, wxEXPAND | wxALL, 5  );

    std::vector<wxRadioButton*> radioButtons;
    radioButtons.reserve( aOptions.Count() );

    for( const wxString& option : aOptions )
    {
        radioButtons.emplace_back( new wxRadioButton( dlg, wxID_ANY, _( option ) ) );
        boxSizer->Add( radioButtons.back(), 0, wxEXPAND | wxALL, 5 );
    }

    wxStdDialogButtonSizer* m_sdboxSizer = new wxStdDialogButtonSizer();
    m_sdboxSizer->AddButton( new wxButton( dlg, wxID_OK ) );
    m_sdboxSizer->AddButton( new wxButton( dlg, wxID_CANCEL ) );
    m_sdboxSizer->Realize();
    boxSizer->Add( m_sdboxSizer, 1, wxEXPAND | wxALL, 5 );

    dlg->SetSizer( boxSizer );
    dlg->Layout();
    boxSizer->Fit( dlg );
    boxSizer->SetSizeHints( dlg );
    dlg->Centre( wxBOTH );

    if( dlg->ShowModal() == wxID_OK )
    {
        for( unsigned int i = 0; i < radioButtons.size(); ++i )
        {
            if( radioButtons[i]->GetValue() )
            {
                ret = i;
                break;
            }
        }
    }
    else
    {
        ret = -1;
    }

    dlg->Destroy();

    return ret;
}


class DIALOG_MULTI_OPTIONS : public DIALOG_SHIM
{
public:
    DIALOG_MULTI_OPTIONS( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
            const wxArrayString& aOptions )
        : DIALOG_SHIM( aParent, wxID_ANY, aTitle )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );

        if( !aMessage.IsEmpty() )
            boxSizer->Add( new wxStaticText( this, wxID_ANY, aMessage ), 0, wxEXPAND | wxALL, 5 );

        m_checklist = new wxCheckListBox( this, wxID_ANY );

        for( const wxString& option : aOptions )
            m_checklist->Append( option );

        boxSizer->Add( m_checklist, 0, wxEXPAND | wxALL, 5 );

        wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
        wxButton* selectAll = new wxButton( this, wxID_ANY, _( "Select All" ) );
        btnSizer->Add( selectAll, 0, wxEXPAND | wxALL, 5 );
        wxButton* unselectAll = new wxButton( this, wxID_ANY, _( "Unselect All" ) );
        btnSizer->Add( unselectAll, 0, wxEXPAND | wxALL, 5 );
        boxSizer->Add( btnSizer, 0, wxEXPAND | wxALL, 5 );

        wxStdDialogButtonSizer* m_sdboxSizer = new wxStdDialogButtonSizer();
        m_sdboxSizer->AddButton( new wxButton( this, wxID_OK ) );
        m_sdboxSizer->AddButton( new wxButton( this, wxID_CANCEL ) );
        m_sdboxSizer->Realize();
        boxSizer->Add( m_sdboxSizer, 0, wxEXPAND | wxALL, 5 );

        SetSizer( boxSizer );
        Layout();
        boxSizer->Fit( this );
        boxSizer->SetSizeHints( this );
        Centre( wxBOTH );

        selectAll->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MULTI_OPTIONS::selectAll, this );
        unselectAll->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MULTI_OPTIONS::unselectAll, this );
    }

    std::vector<int> GetSelection() const
    {
        std::vector<int> ret;

        for( unsigned int i = 0; i < m_checklist->GetCount(); ++i )
        {
            if( m_checklist->IsChecked( i ) )
                ret.push_back( i );
        }

        return ret;
    }

    void SetCheckboxes( bool aValue )
    {
        for( unsigned int i = 0; i < m_checklist->GetCount(); ++i )
            m_checklist->Check( i, aValue );
    }

protected:
    wxCheckListBox* m_checklist;

    void selectAll( wxCommandEvent& aEvent )
    {
        SetCheckboxes( true );
    }

    void unselectAll( wxCommandEvent& aEvent )
    {
        SetCheckboxes( false );
    }
};


std::pair<bool, std::vector<int>> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const wxArrayString& aOptions, bool aDefaultState )
{
    std::vector<int> ret;
    bool clickedOk;
    DIALOG_MULTI_OPTIONS dlg( aParent, aTitle, aMessage, aOptions );
    dlg.SetCheckboxes( aDefaultState );

    if( dlg.ShowModal() == wxID_OK )
    {
        ret = dlg.GetSelection();
        clickedOk = true;
    }
    else
    {
        clickedOk = false;
    }

    return std::make_pair( clickedOk, ret );
}


std::pair<bool, std::vector<int>> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const std::vector<std::string>& aOptions, bool aDefaultState )
{
    wxArrayString array;

    for( const auto& option : aOptions )
        array.Add( option );

    return SelectMultipleOptions( aParent, aTitle, aMessage, array, aDefaultState );
}
