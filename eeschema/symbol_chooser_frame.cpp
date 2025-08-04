/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <kiplatform/ui.h>
#include <wx/button.h>
#include <sch_base_frame.h>
#include <eeschema_settings.h>
#include <widgets/panel_symbol_chooser.h>
#include <symbol_chooser_frame.h>
#include <algorithm>


static std::vector<PICKED_SYMBOL> s_SymbolHistoryList;
static unsigned                   s_SymbolHistoryMaxCount = 8;

static void AddSymbolToHistory( const PICKED_SYMBOL& aSymbol )
{
    // Remove duplicates
    std::erase_if( s_SymbolHistoryList,
                    [&]( const PICKED_SYMBOL& candidate ) -> bool
                    {
                        return candidate.LibId == aSymbol.LibId
                                && candidate.Unit == aSymbol.Unit
                                && candidate.Convert == aSymbol.Convert;
                    } );

    // Add the new name at the beginning of the history list
    s_SymbolHistoryList.insert( s_SymbolHistoryList.begin(), aSymbol );

    // Remove extra names
    while( s_SymbolHistoryList.size() > s_SymbolHistoryMaxCount )
        s_SymbolHistoryList.resize( s_SymbolHistoryMaxCount );
}


BEGIN_EVENT_TABLE( SYMBOL_CHOOSER_FRAME, SCH_BASE_FRAME )
    // Menu (and/or hotkey) events
    EVT_MENU( wxID_CLOSE, SYMBOL_CHOOSER_FRAME::CloseSymbolChooser )
    EVT_BUTTON( wxID_OK, SYMBOL_CHOOSER_FRAME::OnOK )
    EVT_BUTTON( wxID_CANCEL, SYMBOL_CHOOSER_FRAME::CloseSymbolChooser )
    EVT_PAINT( SYMBOL_CHOOSER_FRAME::OnPaint )
END_EVENT_TABLE()


#define PARENT_STYLE ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                      | wxWANTS_CHARS | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT )
#define MODAL_STYLE ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                      | wxWANTS_CHARS | wxFRAME_NO_TASKBAR )

SYMBOL_CHOOSER_FRAME::SYMBOL_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent, bool& aCancelled ) :
        SCH_BASE_FRAME( aKiway, aParent, FRAME_SYMBOL_CHOOSER, _( "Symbol Chooser" ),
                        wxDefaultPosition, wxDefaultSize, aParent ? PARENT_STYLE : MODAL_STYLE,
                        SYMBOL_CHOOSER_FRAME_NAME )
{
    SetModal( true );

    m_messagePanel->Hide();

    wxBoxSizer* frameSizer = new wxBoxSizer( wxVERTICAL );

    std::vector<PICKED_SYMBOL> dummyAlreadyPlaced;
    m_chooserPanel = new PANEL_SYMBOL_CHOOSER( this, this, nullptr /* no filter */,
                                               s_SymbolHistoryList,
                                               dummyAlreadyPlaced, false, false, aCancelled,
                                               // Accept handler
                                               [this]()
                                               {
                                                   wxCommandEvent dummy;
                                                   OnOK( dummy );
                                               },
                                               // Escape handler
                                               [this]()
                                               {
                                                   DismissModal( false );
                                               } );


    frameSizer->Add( m_chooserPanel, 1, wxEXPAND );

    wxPanel*    bottomPanel = new wxPanel( this );
    wxBoxSizer* bottomSizer = new wxBoxSizer( wxVERTICAL );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( bottomPanel, wxID_OK );
    wxButton*               cancelButton = new wxButton( bottomPanel, wxID_CANCEL );
    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    bottomSizer->Add( sdbSizer, 1, wxEXPAND | wxALL, 5 );

    bottomPanel->SetSizer( bottomSizer );
    frameSizer->Add( bottomPanel, 0, wxEXPAND );

    SetSizer( frameSizer );
    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ), m_chooserPanel->GetItemCount() ) );
    Layout();
    m_chooserPanel->FinishSetup();

    Bind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnChar, m_chooserPanel );
}


SYMBOL_CHOOSER_FRAME::~SYMBOL_CHOOSER_FRAME()
{
    Unbind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnChar, m_chooserPanel );
}


bool SYMBOL_CHOOSER_FRAME::ShowModal( wxString* aSymbol, wxWindow* aParent )
{
    if( aSymbol && !aSymbol->IsEmpty() )
    {
        LIB_ID libid;

        libid.Parse( *aSymbol, true );

        if( libid.IsValid() )
            m_chooserPanel->SetPreselect( libid );
    }

    return KIWAY_PLAYER::ShowModal( aSymbol, aParent );
}


void SYMBOL_CHOOSER_FRAME::doCloseWindow()
{
    m_chooserPanel->ShutdownCanvases();

    // Only dismiss a modal frame once, so that the return values set by
    // the prior DismissModal() are not bashed for ShowModal().
    if( !IsDismissed() )
        DismissModal( false );

    // window to be destroyed by the caller of KIWAY_PLAYER::ShowModal()
}


void SYMBOL_CHOOSER_FRAME::OnPaint( wxPaintEvent& aEvent )
{
    if( m_firstPaintEvent )
    {
        KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );
        KIPLATFORM::UI::ForceFocus( m_chooserPanel->GetFocusTarget() );

        m_firstPaintEvent = false;
    }

    aEvent.Skip();
}


void SYMBOL_CHOOSER_FRAME::OnOK( wxCommandEvent& aEvent )
{
    LIB_ID libId = m_chooserPanel->GetSelectedLibId();

    if( libId.IsValid() )
    {
        PICKED_SYMBOL symbol;
        symbol.LibId = libId;

        AddSymbolToHistory( symbol );
        DismissModal( true, libId.Format() );
    }
    else
    {
        DismissModal( false );
    }
}


WINDOW_SETTINGS* SYMBOL_CHOOSER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg ) )
        return &cfg->m_LibViewPanel.window;

    wxFAIL_MSG( wxT( "SYMBOL_CHOOSER not running with EESCHEMA_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


void SYMBOL_CHOOSER_FRAME::CloseSymbolChooser( wxCommandEvent& event )
{
    Close( false );
}
