/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <confirm.h>
#include <widgets/resettable_panel.h>
#include <wx/button.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/treebook.h>
#include <wx/treectrl.h>

#include <widgets/infobar.h>
#include <widgets/paged_dialog.h>
#include <wx/stc/stc.h>

#include <algorithm>

// Maps from dialogTitle <-> pageTitle for keeping track of last-selected pages.
// This is not a simple page index because some dialogs have dynamic page sets.
std::map<wxString, wxString> g_lastPage;
std::map<wxString, wxString> g_lastParentPage;


PAGED_DIALOG::PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset,
                            const wxString& aAuxiliaryAction ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_auxiliaryButton( nullptr ),
        m_resetButton( nullptr ),
        m_cancelButton( nullptr ),
        m_title( aTitle ),
        m_dirty( false )
{
    auto mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    m_treebook = new wxTreebook( this, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    mainSizer->Add( m_treebook, 1, wxEXPAND|wxLEFT|wxTOP, 10 );

    auto line = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxLI_HORIZONTAL );
    mainSizer->Add( line, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 10 );

    m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    if( aShowReset )
    {
        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_buttonsSizer->Add( m_resetButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
    }

    if( !aAuxiliaryAction.IsEmpty() )
    {
        m_auxiliaryButton = new wxButton( this, wxID_ANY, aAuxiliaryAction );
        m_buttonsSizer->Add( m_auxiliaryButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
    }

    m_buttonsSizer->AddStretchSpacer();

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton* sdbSizerOK = new wxButton( this, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    wxButton* sdbSizerCancel = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    m_buttonsSizer->Add( sdbSizer, 1, 0, 5 );
    mainSizer->Add( m_buttonsSizer, 0, wxALL|wxEXPAND, 5 );

    SetupStandardButtons();

    // We normally save the dialog size and position based on its class-name.  This class
    // substitutes the title so that each distinctly-titled dialog can have its own saved
    // size and position.
    m_hash_key = aTitle;

    if( m_auxiliaryButton )
    {
        m_auxiliaryButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::OnAuxiliaryAction,
                                 this );
    }

    if( m_resetButton )
    {
        m_resetButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::OnResetButton, this );
    }

    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::OnPageChanged, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::OnPageChanging, this );
}


void PAGED_DIALOG::finishInitialization()
{
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    // For some reason adding page labels to the treeCtrl doesn't invalidate its bestSize
    // cache so we have to do it by hand
    m_treebook->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
        m_treebook->GetPage( i )->Layout();

    m_treebook->Layout();
    m_treebook->Fit();

    finishDialogSettings();

    Centre( wxBOTH );
}


void PAGED_DIALOG::SetInitialPage( const wxString& aPage, const wxString& aParentPage )
{
    g_lastPage[ m_title ] = aPage;
    g_lastParentPage[ m_title ] = aParentPage;
}


PAGED_DIALOG::~PAGED_DIALOG()
{
    // Store the current parentPageTitle/pageTitle hierarchy so we can re-select it
    // next time.
    wxString lastPage = wxEmptyString;
    wxString lastParentPage = wxEmptyString;

    int selected = m_treebook->GetSelection();

    if( selected != wxNOT_FOUND )
    {
        lastPage = m_treebook->GetPageText( (unsigned) selected );

        int parent = m_treebook->GetPageParent( (unsigned) selected );

        if( parent != wxNOT_FOUND )
            lastParentPage = m_treebook->GetPageText( (unsigned) parent );
    }

    g_lastPage[ m_title ] = lastPage;
    g_lastParentPage[ m_title ] = lastParentPage;

    if( m_auxiliaryButton )
    {
        m_auxiliaryButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::OnAuxiliaryAction,
                                   this );
    }

    if( m_resetButton )
    {
        m_resetButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::OnResetButton, this );
    }

    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::OnPageChanged, this );
    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::OnPageChanging, this );
}


bool PAGED_DIALOG::TransferDataToWindow()
{
    finishInitialization();

    // Call TransferDataToWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    // On wxWidgets 3.0, TransferDataFromWindow() is not called recursively
    // so we have to call it for each page
#if !wxCHECK_VERSION( 3, 1, 0 )
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_treebook->GetPage( i );

        if( !page->TransferDataToWindow() )
            return false;
    }
#endif

    // Search for a page matching the lastParentPageTitle/lastPageTitle hierarchy
    wxString lastPage = g_lastPage[ m_title ];
    wxString lastParentPage = g_lastParentPage[ m_title ];
    int lastPageIndex = wxNOT_FOUND;

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( m_treebook->GetPageText( i ) == lastPage )
        {
            if( lastParentPage.IsEmpty() )
            {
                lastPageIndex = i;
                break;
            }

            if( m_treebook->GetPageParent( i ) >= 0
                && m_treebook->GetPageText( (unsigned) m_treebook->GetPageParent( i ) ) == lastParentPage )
            {
                lastPageIndex = i;
                break;
            }
        }
    }

    m_treebook->ChangeSelection( (unsigned) std::max( 0, lastPageIndex ) );

    return true;
}


bool PAGED_DIALOG::TransferDataFromWindow()
{
    bool ret = true;

    // Call TransferDataFromWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        ret = false;

    // On wxWidgets 3.0, TransferDataFromWindow() is not called recursively
    // so we have to call it for each page
#if !wxCHECK_VERSION( 3, 1, 0 )
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_treebook->GetPage( i );

        if( !page->TransferDataFromWindow() )
        {
            m_treebook->ChangeSelection( i );
            ret = false;
            break;
        }
    }
#endif

    return ret;
}


void PAGED_DIALOG::SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId,
                             int aRow, int aCol )
{
    SetError( aMessage, FindWindow( aPageName ), FindWindow( aCtrlId ), aRow, aCol );
}


void PAGED_DIALOG::SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl,
                             int aRow, int aCol )
{
    if( aCtrl )
    {
        m_infoBar->ShowMessageFor( aMessage, 10000, wxICON_WARNING );

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
        {
            textCtrl->SetSelection( -1, -1 );
            textCtrl->SetFocus();
            return;
        }

        if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aCtrl ) )
        {
            if( aRow > 0 )
            {
                int pos = scintilla->PositionFromLine( aRow - 1 ) + ( aCol - 1 );
                scintilla->GotoPos( pos );
            }

            scintilla->SetFocus();
            return;
        }

        if( wxGrid* grid = dynamic_cast<wxGrid*>( aCtrl ) )
        {
            grid->SetFocus();
            grid->MakeCellVisible( aRow, aCol );
            grid->SetGridCursor( aRow, aCol );

            grid->EnableCellEditControl( true );
            grid->ShowCellEditControl();
            return;
        }
    }
}


void PAGED_DIALOG::OnPageChanged( wxBookCtrlEvent& event )
{
    int page = event.GetSelection();

    // Use the first sub-page when a tree level node is selected.
    if( m_treebook->GetPageParent( page ) == wxNOT_FOUND )
    {
        unsigned next = page + 1;

        if( next < m_treebook->GetPageCount() )
            m_treebook->ChangeSelection( next );
    }

    // NB: dynamic_cast doesn't work over Kiway.
    wxWindow* panel = m_treebook->GetPage( page );

    wxCHECK( panel, /* void */ );

    // Enable the reset button only if the page is re-settable
    if( m_resetButton )
    {
        if( panel && panel->GetWindowStyle() & wxRESETTABLE )
        {
            m_resetButton->SetLabel( wxString::Format( _( "Reset %s to Defaults" ),
                                                       m_treebook->GetPageText( page ) ) );
            m_resetButton->SetToolTip( panel->GetHelpTextAtPoint( wxPoint( -INT_MAX, INT_MAX ),
                                       wxHelpEvent::Origin_Unknown ) );
            m_resetButton->Enable( true );
        }
        else
        {
            m_resetButton->SetLabel( _( "Reset to Defaults" ) );
            m_resetButton->SetToolTip( wxString() );
            m_resetButton->Enable( false );
        }

        m_resetButton->GetParent()->Layout();
    }

    wxSizeEvent evt( wxDefaultSize );

    wxQueueEvent( m_treebook, evt.Clone() );

    // @todo Test to see if this macOS hack is still necessary now that a psuedo size event is
    //       processed above.

    // Work around an OSX bug where the wxGrid children don't get placed correctly until
    // the first resize event
#ifdef __WXMAC__
    if( page + 1 <= m_macHack.size() && m_macHack[ page ] )
    {
        wxSize pageSize = m_treebook->GetPage( page )->GetSize();
        pageSize.x -= 5;
        pageSize.y += 2;

        m_treebook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#endif
}


void PAGED_DIALOG::OnPageChanging( wxBookCtrlEvent& aEvent )
{
    int currentPage = aEvent.GetOldSelection();

    if( currentPage == wxNOT_FOUND )
        return;

    wxWindow* page = m_treebook->GetPage( currentPage );

    wxCHECK( page, /* void */ );

    // If there is a validation error on the current page, don't allow the page change.
    if( !page->Validate() || !page->TransferDataFromWindow() )
    {
        aEvent.Veto();
        return;
    }
}


void PAGED_DIALOG::OnResetButton( wxCommandEvent& aEvent )
{
    int sel = m_treebook->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    // NB: dynamic_cast doesn't work over Kiway
    wxWindow* panel = m_treebook->GetPage( sel );

    if( panel )
    {
        wxCommandEvent resetCommand( wxEVT_COMMAND_BUTTON_CLICKED, ID_RESET_PANEL );
        panel->ProcessWindowEvent( resetCommand );
    }
}
