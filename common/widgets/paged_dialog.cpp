/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/wx_infobar.h>
#include <widgets/wx_panel.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_treebook.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/treebook.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>

#include <paths.h>

#include <launch_ext.h>

#include <algorithm>

// Maps from dialogTitle <-> pageTitle for keeping track of last-selected pages.
// This is not a simple page index because some dialogs have dynamic page sets.
std::map<wxString, wxString> g_lastPage;
std::map<wxString, wxString> g_lastParentPage;


PAGED_DIALOG::PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset,
                            bool aShowOpenFolder, const wxString& aAuxiliaryAction,
                            const wxSize& aInitialSize ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, aInitialSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_auxiliaryButton( nullptr ),
        m_resetButton( nullptr ),
        m_openPrefsDirButton( nullptr ),
        m_title( aTitle )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    WX_PANEL* treebookPanel = new WX_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxBORDER_NONE | wxTAB_TRAVERSAL );
    treebookPanel->SetBorders( false, false, false, true );
    wxBoxSizer* treebookSizer = new wxBoxSizer( wxVERTICAL );
    treebookPanel->SetSizer( treebookSizer );

    m_treebook = new WX_TREEBOOK( treebookPanel, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    m_treebook->SetFitToCurrentPage( true );

    long treeCtrlFlags = m_treebook->GetTreeCtrl()->GetWindowStyleFlag();
    treeCtrlFlags = ( treeCtrlFlags & ~wxBORDER_MASK ) | wxBORDER_NONE;
    m_treebook->GetTreeCtrl()->SetWindowStyleFlag( treeCtrlFlags );

    treebookSizer->Add( m_treebook, 1, wxEXPAND|wxBOTTOM, 2 );
    mainSizer->Add( treebookPanel, 1, wxEXPAND, 0 );

    m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    if( aShowReset )
    {
        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_buttonsSizer->Add( m_resetButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
    }

    if( aShowOpenFolder )
    {
#ifdef __WXMAC__
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Reveal Preferences in Finder" ) );
#else
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Open Preferences Directory" ) );
#endif
        m_buttonsSizer->Add( m_openPrefsDirButton, 0,
                             wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );
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
        m_auxiliaryButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onAuxiliaryAction, this );

    if( m_resetButton )
        m_resetButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onResetButton, this );

    if( m_openPrefsDirButton )
        m_openPrefsDirButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onOpenPrefsDir, this );

    m_treebook->Bind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onCharHook, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::onPageChanged, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::onPageChanging, this );
}


void PAGED_DIALOG::finishInitialization()
{
    for( size_t i = 1; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    // For some reason adding page labels to the treeCtrl doesn't invalidate its bestSize
    // cache so we have to do it by hand
    m_treebook->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
        m_treebook->GetPage( i )->Layout();

    m_treebook->Layout();
    m_treebook->Fit();

    // Add a bit of width to the treeCtrl for scrollbars
    wxSize ctrlSize = m_treebook->GetTreeCtrl()->GetSize();
    ctrlSize.x += 20;
    m_treebook->GetTreeCtrl()->SetMinSize( ctrlSize );
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
        m_auxiliaryButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onAuxiliaryAction, this );

    if( m_resetButton )
        m_resetButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onResetButton, this );

    if( m_openPrefsDirButton )
        m_openPrefsDirButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onOpenPrefsDir, this );

    m_treebook->Unbind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onCharHook, this );
    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::onPageChanged, this );
    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::onPageChanging, this );
}


bool PAGED_DIALOG::TransferDataToWindow()
{
    finishInitialization();

    // Call TransferDataToWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

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

    lastPageIndex = std::max( 0, lastPageIndex );
    m_treebook->SetSelection( lastPageIndex );
    UpdateResetButton( lastPageIndex );

    return true;
}


bool PAGED_DIALOG::TransferDataFromWindow()
{
    bool ret = true;

    // Call TransferDataFromWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        ret = false;

    return ret;
}


PAGED_DIALOG* PAGED_DIALOG::GetDialog( wxWindow* aParent )
{
    while( aParent )
    {
        if( PAGED_DIALOG* parentDialog = dynamic_cast<PAGED_DIALOG*>( aParent ) )
            return parentDialog;

        aParent = aParent->GetParent();
    }

    return nullptr;
}


void PAGED_DIALOG::SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId,
                             int aRow, int aCol )
{
    SetError( aMessage, FindWindow( aPageName ), FindWindow( aCtrlId ), aRow, aCol );
}


void PAGED_DIALOG::SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl,
                             int aRow, int aCol )
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


void PAGED_DIALOG::UpdateResetButton( int aPage )
{
    wxWindow* panel = m_treebook->ResolvePage( aPage );

    // Enable the reset button only if the page is re-settable
    if( m_resetButton )
    {
        if( panel && ( panel->GetWindowStyle() & wxRESETTABLE ) )
        {
            wxString name = m_treebook->GetPageText( aPage );
            name.Replace( wxT( "&" ), wxT( "&&" ) );

            m_resetButton->SetLabel( wxString::Format( _( "Reset %s to Defaults" ), name ) );
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
}


void PAGED_DIALOG::onCharHook( wxKeyEvent& aEvent )
{
    if( dynamic_cast<wxTextEntry*>( aEvent.GetEventObject() )
            || dynamic_cast<wxStyledTextCtrl*>( aEvent.GetEventObject() )
            || dynamic_cast<wxListView*>( aEvent.GetEventObject() )
            || dynamic_cast<wxGrid*>( FindFocus() ) )
    {
        aEvent.Skip();
        return;
    }

    if( aEvent.GetKeyCode() == WXK_UP )
    {
        int page = m_treebook->GetSelection();

        if( page >= 1 )
        {
            if( m_treebook->GetPage( page - 1 )->GetChildren().IsEmpty() )
                m_treebook->SetSelection( std::max( page - 2, 0 ) );
            else
                m_treebook->SetSelection( page - 1 );
        }

        m_treebook->GetTreeCtrl()->SetFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        int page = m_treebook->GetSelection();

        m_treebook->SetSelection( std::min<int>( page + 1, m_treebook->GetPageCount() - 1 ) );

        m_treebook->GetTreeCtrl()->SetFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else
    {
        aEvent.Skip();
    }
}


void PAGED_DIALOG::onPageChanged( wxBookCtrlEvent& event )
{
    size_t page = event.GetSelection();

    // Use the first sub-page when a tree level node is selected.
    if( m_treebook->GetCurrentPage()->GetChildren().IsEmpty()
            && page + 1 < m_treebook->GetPageCount() )
    {
        m_treebook->ChangeSelection( ++page );
    }

    UpdateResetButton( page );

    // Make sure the dialog size is enough to fit the page content.
    m_treebook->InvalidateBestSize();

    SetMinSize( wxDefaultSize );
    wxSize minSize = GetBestSize();
    minSize.IncTo( FromDIP( wxSize( 600, 500 ) ) );
    minSize.DecTo( FromDIP( wxSize( 1500, 900 ) ) ); // Failsafe
    SetMinSize( minSize );

    wxSize currentSize = GetSize();
    wxSize newSize = currentSize;
    newSize.IncTo( minSize );

    if( newSize != currentSize )
        SetSize( newSize );

#ifdef __WXMAC__
    // Work around an OSX wxWidgets issue where the wxGrid children don't get placed correctly
    // until the first resize event
    if( page < m_macHack.size() && m_macHack[ page ] )
    {
        wxSize pageSize = m_treebook->GetPage( page )->GetSize();
        pageSize.x += 1;
        pageSize.y += 2;

        m_treebook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#else
    wxSizeEvent evt( wxDefaultSize );
    wxQueueEvent( m_treebook, evt.Clone() );
#endif
}


void PAGED_DIALOG::onPageChanging( wxBookCtrlEvent& aEvent )
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


void PAGED_DIALOG::onResetButton( wxCommandEvent& aEvent )
{
    int sel = m_treebook->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    // NB: dynamic_cast doesn't work over Kiway
    wxWindow* panel = m_treebook->ResolvePage( sel );

    if( panel )
    {
        wxCommandEvent resetCommand( wxEVT_COMMAND_BUTTON_CLICKED, ID_RESET_PANEL );
        panel->ProcessWindowEvent( resetCommand );
    }
}

void PAGED_DIALOG::onOpenPrefsDir( wxCommandEvent& aEvent )
{
    wxString dir( PATHS::GetUserSettingsPath() );
    LaunchExternal( dir );
}
