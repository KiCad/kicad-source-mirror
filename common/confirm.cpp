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


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage,
        const wxString& aCaption, long aStyle )
    : wxRichMessageDialog( aParent, aMessage, aCaption, aStyle | wxCENTRE )
{
    setHash();
}


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage,
        KD_TYPE aType, const wxString& aCaption )
    : wxRichMessageDialog( aParent, aMessage, getCaption( aType, aCaption ), getStyle( aType ) )
{
    setHash();
}


bool KIDIALOG::DoNotShowAgain() const
{
    return doNotShowAgainDlgs.count( m_hash ) > 0;
}


void KIDIALOG::ForceShowAgain()
{
    doNotShowAgainDlgs.erase( m_hash );
}


bool KIDIALOG::Show( bool aShow )
{
    // Check if this dialog should be shown to the user
    auto it = doNotShowAgainDlgs.find( m_hash );

    if( it != doNotShowAgainDlgs.end() )
        return it->second;

    bool ret = wxRichMessageDialog::Show();

    // Has the user asked not to show the dialog again
    if( IsCheckBoxChecked() )
        doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


int KIDIALOG::ShowModal()
{
    // Check if this dialog should be shown to the user
    auto it = doNotShowAgainDlgs.find( m_hash );

    if( it != doNotShowAgainDlgs.end() )
        return it->second;

    int ret = wxRichMessageDialog::ShowModal();

    // Has the user asked not to show the dialog again
    if( IsCheckBoxChecked() )
        doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


void KIDIALOG::setHash()
{
    std::size_t h1 = std::hash<wxString>{}( GetMessage() );
    std::size_t h2 = std::hash<wxString>{}( GetTitle() );
    m_hash = h1 ^ ( h2 << 1 );
}


wxString KIDIALOG::getCaption( KD_TYPE aType, const wxString& aCaption )
{
    if( !aCaption.IsEmpty() )
        return aCaption;

    switch( aType )
    {
        case KD_NONE:       /* fall through */
        case KD_INFO:       return _( "Message" );
        case KD_QUESTION:   return _( "Question" );
        case KD_WARNING:    return _( "Warning" );
        case KD_ERROR:      return _( "Error" );
    }

    return wxEmptyString;
}


long KIDIALOG::getStyle( KD_TYPE aType )
{
    long style = wxOK | wxCENTRE;

    switch( aType )
    {
        case KD_NONE:       break;
        case KD_INFO:       style |= wxICON_INFORMATION; break;
        case KD_QUESTION:   style |= wxICON_QUESTION; break;
        case KD_WARNING:    style |= wxICON_WARNING; break;
        case KD_ERROR:      style |= wxICON_ERROR; break;
    }

    return style;
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
    wxSingleChoiceDialog dlg( aParent, aMessage, aTitle, aOptions );

    if( dlg.ShowModal() != wxID_OK )
        return -1;

    return dlg.GetSelection();
}


class DIALOG_MULTI_OPTIONS : public wxMultiChoiceDialog
{
public:
    DIALOG_MULTI_OPTIONS( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
            const wxArrayString& aOptions )
        : wxMultiChoiceDialog( aParent, aMessage, aTitle, aOptions ),
        m_optionsCount( aOptions.GetCount() )
    {
        wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
        wxButton* selectAll = new wxButton( this, wxID_ANY, _( "Select All" ) );
        btnSizer->Add( selectAll, 1, wxEXPAND | wxALL, 5 );
        wxButton* unselectAll = new wxButton( this, wxID_ANY, _( "Unselect All" ) );
        btnSizer->Add( unselectAll, 1, wxEXPAND | wxALL, 5 );
        auto sizer = GetSizer();
        sizer->Insert( sizer->GetItemCount() - 1, btnSizer, 0, wxEXPAND | wxALL, 0 );

        Layout();
        sizer->Fit( this );
        sizer->SetSizeHints( this );
        Centre( wxBOTH );

        selectAll->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MULTI_OPTIONS::selectAll, this );
        unselectAll->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &DIALOG_MULTI_OPTIONS::unselectAll, this );
    }

    void SetCheckboxes( bool aValue )
    {
        wxArrayInt selIdxs;

        if( aValue )        // select all indices
        {
            for( int i = 0; i < m_optionsCount; ++i )
                selIdxs.Add( i );
        }

        SetSelections( selIdxs );
    }

protected:
    ///> Number of displayed options
    int m_optionsCount;

    void selectAll( wxCommandEvent& aEvent )
    {
        SetCheckboxes( true );
    }

    void unselectAll( wxCommandEvent& aEvent )
    {
        SetCheckboxes( false );
    }
};


std::pair<bool, wxArrayInt> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const wxArrayString& aOptions, bool aDefaultState )
{
    DIALOG_MULTI_OPTIONS dlg( aParent, aTitle, aMessage, aOptions );
    dlg.Layout();
    dlg.SetCheckboxes( aDefaultState );

    wxArrayInt ret;
    bool clickedOk = ( dlg.ShowModal() == wxID_OK );

    if( clickedOk )
        ret = dlg.GetSelections();

    return std::make_pair( clickedOk, ret );
}


std::pair<bool, wxArrayInt> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const std::vector<std::string>& aOptions, bool aDefaultState )
{
    wxArrayString array;

    for( const auto& option : aOptions )
        array.Add( option );

    return SelectMultipleOptions( aParent, aTitle, aMessage, array, aDefaultState );
}
