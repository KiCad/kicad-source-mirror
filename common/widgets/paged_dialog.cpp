/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/treebook.h>
#include <wx/treectrl.h>
#include <wx/grid.h>
#include <wx/statline.h>

#include <widgets/infobar.h>
#include <widgets/paged_dialog.h>
#include <wx/stc/stc.h>

#include <algorithm>

// Maps from dialogTitle <-> pageTitle for keeping track of last-selected pages.
// This is not a simple page index because some dialogs have dynamic page sets.
std::map<wxString, wxString> g_lastPage;
std::map<wxString, wxString> g_lastParentPage;


PAGED_DIALOG::PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aUseReset,
                            const wxString& aAuxiliaryAction ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_title( aTitle ),
        m_dirty( false ),
        m_errorCtrl( nullptr ),
        m_errorRow( 0 ),
        m_errorCol( 0 ),
        m_auxiliaryButton( nullptr ),
        m_resetButton( nullptr ),
        m_cancelButton( nullptr )
{
    auto mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    m_treebook = new wxTreebook( this, wxID_ANY );
    mainSizer->Add( m_treebook, 1, wxEXPAND|wxLEFT|wxTOP, 10 );

    auto line = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxLI_HORIZONTAL );
    mainSizer->Add( line, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 10 );

    m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    if( aUseReset )
    {
        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_buttonsSizer->Add( m_resetButton, 0, wxRIGHT|wxLEFT, 5 );
    }

    if( !aAuxiliaryAction.IsEmpty() )
    {
        m_auxiliaryButton = new wxButton( this, wxID_ANY, aAuxiliaryAction );
        m_buttonsSizer->Add( m_auxiliaryButton, 0, wxRIGHT|wxLEFT, 5 );
    }

    m_buttonsSizer->AddStretchSpacer();

    auto sdbSizer = new wxStdDialogButtonSizer();
    wxButton* sdbSizerOK = new wxButton( this, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    wxButton* sdbSizerCancel = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    m_buttonsSizer->Add( sdbSizer, 1, 0, 5 );
    mainSizer->Add( m_buttonsSizer, 0, wxALL|wxEXPAND, 5 );

    sdbSizerOK->SetDefault();

    // We normally save the dialog size and position based on its class-name.  This class
    // substitutes the title so that each distinctly-titled dialog can have its own saved
    // size and position.
    m_hash_key = aTitle;

    if( m_auxiliaryButton )
        m_auxiliaryButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                    wxCommandEventHandler( PAGED_DIALOG::OnAuxiliaryAction ),
                                    nullptr, this );

    if( m_resetButton )
        m_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                wxCommandEventHandler( PAGED_DIALOG::OnResetButton ),
                                nullptr, this );

    m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( PAGED_DIALOG::OnPageChange ), NULL, this );
    Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PAGED_DIALOG::OnUpdateUI ), nullptr, this );
}


// Finish initialization after the bookctrl pages have been added.
void PAGED_DIALOG::finishInitialization()
{
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    // For some reason adding page labels to the treeCtrl doesn't invalidate its bestSize
    // cache so we have to do it by hand
    m_treebook->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        m_treebook->ExpandNode( i );
        m_treebook->GetPage( i )->Layout();
    }

    m_treebook->Fit();
    m_treebook->Layout();

    finishDialogSettings();
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
        m_auxiliaryButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                       wxCommandEventHandler( PAGED_DIALOG::OnAuxiliaryAction ),
                                       nullptr, this );

    if( m_resetButton )
        m_resetButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                   wxCommandEventHandler( PAGED_DIALOG::OnResetButton ),
                                   nullptr, this );

    m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                            wxBookCtrlEventHandler( PAGED_DIALOG::OnPageChange ), NULL, this );
    Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PAGED_DIALOG::OnUpdateUI ),
                nullptr, this );
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

    m_treebook->SetSelection( (unsigned) std::max( 0, lastPageIndex ) );

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
            ret = false;
            break;
        }
    }
#endif

    if( !ret && !m_errorMessage.IsEmpty() )
        m_infoBar->ShowMessage( m_errorMessage, wxICON_WARNING );

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
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( m_treebook->GetPage( i ) == aPage )
        {
            m_treebook->SetSelection( i );
            break;
        }
    }

    // Once the page has been changed we want to wait for it to update before displaying
    // the error dialog.  So store the rest of the error info and wait for OnUpdateUI.
    m_errorMessage = aMessage;
    m_errorCtrl = aCtrl;
    m_errorRow = aRow;
    m_errorCol = aCol;
}


void PAGED_DIALOG::AddAuxiliaryAction( const wxString& aTitle, const wxString& aTooltip,
                                       std::function<void( wxCommandEvent& )> aHandler )
{
    // Insert before standard button sizer and flex spacer
    const int idx = m_buttonsSizer->GetItemCount() - 2;

    wxButton* button = new wxButton( this, wxID_ANY, aTitle );
    button->SetToolTip( aTooltip );
    button->Bind( wxEVT_BUTTON, aHandler );
    m_buttonsSizer->Insert( idx, button, 0, wxRIGHT | wxLEFT, 5 );
}


void PAGED_DIALOG::OnValidate( wxCommandEvent& aEvent )
{
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_treebook->GetPage( i );

        // Display first warning
        if( !page->Validate() )
            break;
    }

    if( !m_errorMessage.IsEmpty() )
        m_infoBar->ShowMessage( m_errorMessage, wxICON_WARNING );
}


void PAGED_DIALOG::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Handle an error.  This is delayed to OnUpdateUI so that we can change the focus
    // even when the original validation was triggered from a killFocus event, and so
    // that the corresponding notebook page can be shown in the background when triggered
    // from an OK.
    if( m_errorCtrl )
    {
        // We will re-enter this routine when the error dialog is displayed, so make
        // sure we don't keep putting up more dialogs.
        wxWindow* ctrl = m_errorCtrl;
        m_errorCtrl = nullptr;

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( ctrl ) )
        {
            textCtrl->SetSelection( -1, -1 );
            textCtrl->SetFocus();
            return;
        }

        if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( ctrl ) )
        {
            if( m_errorRow > 0 )
            {
                int pos = scintilla->PositionFromLine( m_errorRow - 1 ) + ( m_errorCol - 1 );
                scintilla->GotoPos( pos );
            }

            scintilla->SetFocus();
            return;
        }

        if( wxGrid* grid = dynamic_cast<wxGrid*>( ctrl ) )
        {
            grid->SetFocus();
            grid->MakeCellVisible( m_errorRow, m_errorCol );
            grid->SetGridCursor( m_errorRow, m_errorCol );

            grid->EnableCellEditControl( true );
            grid->ShowCellEditControl();
            return;
        }
    }

    if( m_treebook->GetCurrentPage()->GetChildren().IsEmpty() )
    {
        unsigned next = m_treebook->GetSelection() + 1;

        if( next < m_treebook->GetPageCount() )
            m_treebook->SetSelection( next );
    }
}


void PAGED_DIALOG::OnPageChange( wxBookCtrlEvent& event )
{
    size_t page = event.GetSelection();

    // Enable the reset button only if the page is resettable
    if( m_resetButton )
    {
        if( auto panel = dynamic_cast<RESETTABLE_PANEL*>( m_treebook->GetPage( page ) ) )
        {
            m_resetButton->SetToolTip( panel->GetResetTooltip() );
            m_resetButton->Enable( true );
        }
        else
        {
            m_resetButton->SetToolTip( wxString() );
            m_resetButton->Enable( false );
        }

    }

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

    Layout();
}


void PAGED_DIALOG::OnResetButton( wxCommandEvent& aEvent )
{
    int sel = m_treebook->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    RESETTABLE_PANEL* panel = dynamic_cast<RESETTABLE_PANEL*>( m_treebook->GetPage( sel ) );

    if( panel )
        panel->ResetPanel();
}
