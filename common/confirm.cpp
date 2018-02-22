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

#include <confirm.h>
#include <bitmaps.h>
#include <html_messagebox.h>
#include <dialog_exit_base.h>

#include <functional>
#include <unordered_map>

// Set of dialogs that have been chosen not to be shown again
static std::unordered_map<unsigned long, int> doNotShowAgainDlgs;


KI_DIALOG::KI_DIALOG( wxWindow* aParent, const wxString& aMessage )
    : wxDialog( aParent, wxID_ANY, wxEmptyString ),
    m_btnSizer( nullptr ), m_cbDoNotShow( nullptr ), m_icon( nullptr )
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_sizerMain = new wxBoxSizer( wxVERTICAL );
    m_sizerUpper = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* message = new wxStaticText( this, wxID_ANY, aMessage );
    message->Wrap( -1 );
    //message->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
    m_sizerUpper->Add( message, 1, wxALL | wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );
    m_sizerMain->Add( m_sizerUpper, 1, wxALL | wxEXPAND, 5 );

    Type( KD_NONE );
    Buttons( wxOK );

    SetSizer( m_sizerMain );
    Connect( wxEVT_BUTTON, wxCommandEventHandler( KI_DIALOG::onButtonClick ), NULL, this );
}


KI_DIALOG& KI_DIALOG::Type( KD_TYPE aType )
{
    m_type = aType;

    const std::unordered_map<int, wxString> stdTitle = {
        { KD_NONE, _( "Message" ) }, { KD_INFO, _( "Information" ) }, { KD_QUESTION, _( "Question" ) },
        { KD_WARNING, _( "Warning" ) }, { KD_ERROR, _( "Error" ) }
    };

    const std::unordered_map<int, wxArtID> icons = {
        { KD_INFO, wxART_INFORMATION }, { KD_QUESTION, wxART_QUESTION },
        { KD_WARNING, wxART_WARNING }, { KD_ERROR, wxART_ERROR }
    };

    if( m_icon )
    {
        m_sizerUpper->Remove( 0 );
        m_icon->Destroy();
        m_icon = nullptr;
    }

    if( aType != KD_NONE )
    {
        m_icon = new wxStaticBitmap( this, wxID_ANY,
                wxArtProvider::GetBitmap( icons.at( aType ), wxART_CMN_DIALOG ) );
        m_sizerUpper->Prepend( m_icon, 0, wxALL, 5 );
    }

    if( !m_customTitle )
        SetTitle( stdTitle.at( aType ) );

    return *this;
}


KI_DIALOG& KI_DIALOG::Title( const wxString& aTitle )
{
    m_customTitle = aTitle;
    SetTitle( aTitle );
    return *this;
}


KI_DIALOG& KI_DIALOG::Buttons( long aButtons )
{
    wxASSERT( aButtons );

    if( !aButtons )
        aButtons = wxOK;

    const std::map<long, long> btnTypes = { { wxOK, wxID_OK }, { wxCANCEL, wxID_CANCEL },
        { wxYES, wxID_YES }, { wxNO, wxID_NO }, { wxAPPLY, wxID_APPLY }, { wxCLOSE, wxID_CLOSE },
        { wxHELP, wxID_HELP } };

    if( m_btnSizer )
    {
        m_sizerMain->Remove( m_btnSizer );   // also deletes m_btnSizer

        for( auto btn : m_buttons )
            btn->Destroy();
    }

    m_btnSizer = new wxBoxSizer( wxHORIZONTAL );

    for( auto type : btnTypes )
    {
        if( !( aButtons & type.first ) )
            continue;

        wxButton* btn = new wxButton( this, type.second );
        m_btnSizer->Add( btn, 1, wxALL | wxEXPAND | wxALIGN_RIGHT );
        m_buttons.push_back( btn );
    }

    m_sizerMain->Add( m_btnSizer, 0, wxALL | wxALIGN_RIGHT, 5 );

    return *this;
}


KI_DIALOG& KI_DIALOG::DoNotShowCheckbox()
{
    if( !m_cbDoNotShow )
    {
        m_cbDoNotShow = new wxCheckBox( this, wxID_ANY, _( "Do not show again" ) );
        m_sizerMain->Insert( 1, m_cbDoNotShow, 1, wxALL | wxEXPAND, 5 );
    }

    return *this;
}


bool KI_DIALOG::DoNotShowAgain() const
{
    return doNotShowAgainDlgs.count( hash() ) > 0;
}


void KI_DIALOG::ForceShowAgain()
{
    doNotShowAgainDlgs.erase( hash() );
}


int KI_DIALOG::ShowModal()
{
    // Check if this dialog should be shown to the user
    auto it = doNotShowAgainDlgs.find( hash() );

    if( it != doNotShowAgainDlgs.end() )
        return it->second;

    Layout();
    m_sizerMain->Fit( this );
    int ret = wxDialog::ShowModal();

    // Has the user asked not to show the dialog again
    if( m_cbDoNotShow && m_cbDoNotShow->IsChecked() )
        doNotShowAgainDlgs[hash()] = ret;

    return ret;
}


void KI_DIALOG::onButtonClick( wxCommandEvent& aEvent )
{
    EndModal( aEvent.GetId() );
}


unsigned long KI_DIALOG::hash() const
{
    std::size_t h1 = std::hash<wxString>{}( m_message );
    std::size_t h2 = std::hash<wxString>{}( m_customTitle );
    std::size_t h3 = std::hash<int>{}( m_type );

    return h1 ^ ( h2 << 1 ) ^ ( h3 << 2 );
}


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


void DisplayErrorMessage( wxWindow* aParent, const wxString& aText, const wxString& aExtraInfo )
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


void DisplayInfoMessage( wxWindow* aParent, const wxString& aMessage, const wxString& aExtraInfo )
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
    wxDialog* dlg = new wxDialog( aParent, wxID_ANY, aTitle );

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
    wxButton* btnOk = new wxButton( dlg, wxID_OK );
    m_sdboxSizer->AddButton( btnOk );
    m_sdboxSizer->AddButton( new wxButton( dlg, wxID_CANCEL ) );
    m_sdboxSizer->Realize();
    btnOk->SetDefault();
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


class DIALOG_MULTI_OPTIONS : public wxDialog
{
public:
    DIALOG_MULTI_OPTIONS( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
            const wxArrayString& aOptions )
        : wxDialog( aParent, wxID_ANY, aTitle )
    {
        SetSizeHints( wxDefaultSize, wxDefaultSize );

        wxBoxSizer* boxSizer = new wxBoxSizer( wxVERTICAL );

        if( !aMessage.IsEmpty() )
            boxSizer->Add( new wxStaticText( this, wxID_ANY, aMessage ), 0, wxEXPAND | wxALL, 5 );

        m_checklist = new wxCheckListBox( this, wxID_ANY );

        for( const wxString& option : aOptions )
            m_checklist->Append( option );

        boxSizer->Add( m_checklist, 1, wxEXPAND | wxALL, 5 );

        wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
        wxButton* selectAll = new wxButton( this, wxID_ANY, _( "Select All" ) );
        btnSizer->Add( selectAll, 1, wxEXPAND | wxALL, 5 );
        wxButton* unselectAll = new wxButton( this, wxID_ANY, _( "Unselect All" ) );
        btnSizer->Add( unselectAll, 1, wxEXPAND | wxALL, 5 );
        boxSizer->Add( btnSizer, 0, wxEXPAND | wxALL, 5 );

        wxStdDialogButtonSizer* m_sdboxSizer = new wxStdDialogButtonSizer();
        wxButton* btnOk = new wxButton( this, wxID_OK );
        m_sdboxSizer->AddButton( btnOk );
        m_sdboxSizer->AddButton( new wxButton( this, wxID_CANCEL ) );
        m_sdboxSizer->Realize();
        btnOk->SetDefault();
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
