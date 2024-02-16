/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_express.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <kiplatform/ui.h>
#include <widgets/panel_footprint_chooser.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <footprint_chooser_frame.h>
#include "wx/display.h"


static wxArrayString s_FootprintHistoryList;
static unsigned      s_FootprintHistoryMaxCount = 8;

static void AddFootprintToHistory( const wxString& aName )
{
    // Remove duplicates
    for( int ii = (int) s_FootprintHistoryList.GetCount() - 1; ii >= 0; --ii )
    {
        if( s_FootprintHistoryList[ ii ] == aName )
            s_FootprintHistoryList.RemoveAt( (size_t) ii );
    }

    // Add the new name at the beginning of the history list
    s_FootprintHistoryList.Insert( aName, 0 );

    // Remove extra names
    while( s_FootprintHistoryList.GetCount() >= s_FootprintHistoryMaxCount )
        s_FootprintHistoryList.RemoveAt( s_FootprintHistoryList.GetCount() - 1 );
}


BEGIN_EVENT_TABLE( FOOTPRINT_CHOOSER_FRAME, PCB_BASE_FRAME )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_CHOOSER_FRAME::closeFootprintChooser )
    EVT_BUTTON( wxID_OK, FOOTPRINT_CHOOSER_FRAME::OnOK )
    EVT_BUTTON( wxID_CANCEL, FOOTPRINT_CHOOSER_FRAME::closeFootprintChooser )
    EVT_PAINT( FOOTPRINT_CHOOSER_FRAME::OnPaint )
END_EVENT_TABLE()


#define MODAL_FRAME ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                      | wxWANTS_CHARS | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP )


FOOTPRINT_CHOOSER_FRAME::FOOTPRINT_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        PCB_BASE_FRAME( aKiway, aParent, FRAME_FOOTPRINT_CHOOSER, _( "Footprint Chooser" ),
                        wxDefaultPosition, wxDefaultSize, MODAL_FRAME,
                        FOOTPRINT_CHOOSER_FRAME_NAME ),
        m_filterByPinCount( nullptr ),
        m_filterByFPFilters( nullptr ),
        m_pinCount( 0 ),
        m_firstPaintEvent( true )
{
    SetModal( true );

    m_messagePanel->Hide();

    wxPanel*    bottomPanel = new wxPanel( this );
    wxBoxSizer* bottomSizer = new wxBoxSizer( wxVERTICAL );

    m_filterByFPFilters = new wxCheckBox( bottomPanel, wxID_ANY, _( "Apply footprint filters" ) );
    m_filterByPinCount = new wxCheckBox( bottomPanel, wxID_ANY, _( "Filter by pin count" ) );

    m_filterByFPFilters->Show( false );
    m_filterByPinCount->Show( false );

    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        m_filterByFPFilters->SetValue( cfg->m_FootprintChooser.use_fp_filters );
        m_filterByPinCount->SetValue( cfg->m_FootprintChooser.filter_on_pin_count );
    }

    wxBoxSizer* frameSizer = new wxBoxSizer( wxVERTICAL );

    m_chooserPanel = new PANEL_FOOTPRINT_CHOOSER( this, this, s_FootprintHistoryList,
            // Filter
            [this]( LIB_TREE_NODE& aNode ) -> bool
            {
                return filterFootprint( aNode );
            },
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

    wxBoxSizer* fpFilterSizer = new wxBoxSizer( wxVERTICAL );
    fpFilterSizer->Add( m_filterByFPFilters, 0, wxTOP | wxEXPAND, 5 );
    bottomSizer->Add( fpFilterSizer, 0, wxEXPAND | wxLEFT, 10 );

    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
    buttonsSizer->Add( m_filterByPinCount, 0, wxLEFT | wxTOP | wxALIGN_TOP, 5 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( bottomPanel, wxID_OK );
    wxButton*               cancelButton = new wxButton( bottomPanel, wxID_CANCEL );

    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    buttonsSizer->Add( sdbSizer, 1, wxALL, 5 );

    bottomSizer->Add( buttonsSizer, 0, wxEXPAND | wxLEFT, 5 );

    bottomPanel->SetSizer( bottomSizer );
    frameSizer->Add( bottomPanel, 0, wxEXPAND );

    SetSizer( frameSizer );

    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ),
                                             m_chooserPanel->GetItemCount() ) );

    Layout();
    m_chooserPanel->FinishSetup();

    m_filterByPinCount->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& evt )
            {
                m_chooserPanel->Regenerate();
            } );

    m_filterByFPFilters->Bind( wxEVT_CHECKBOX,
            [&]( wxCommandEvent& evt )
            {
                m_chooserPanel->Regenerate();
            } );
}


FOOTPRINT_CHOOSER_FRAME::~FOOTPRINT_CHOOSER_FRAME()
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_FootprintChooser.use_fp_filters = m_filterByFPFilters->GetValue();
        cfg->m_FootprintChooser.filter_on_pin_count = m_filterByPinCount->GetValue();
    }
}

bool FOOTPRINT_CHOOSER_FRAME::filterFootprint( LIB_TREE_NODE& aNode )
{
    if( aNode.m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
    {
        // Normally lib nodes get scored by the max of their children's scores.  However, if a
        // lib node *has* no children then the scorer will call the filter on the lib node itself,
        // and we just want to return true if we're not filtering at all.
        return !m_filterByPinCount->GetValue() && !m_filterByFPFilters->GetValue();
    }

    auto patternMatch =
            []( LIB_ID& id, std::vector<std::unique_ptr<EDA_PATTERN_MATCH>>& filters ) -> bool
            {
                // The matching is case insensitive
                wxString name;

                for( const std::unique_ptr<EDA_PATTERN_MATCH>& filter : filters )
                {
                    name.Empty();

                    // If the filter contains a ':' then include the library name in the pattern
                    if( filter->GetPattern().Contains( wxS( ":" ) ) )
                        name = id.GetUniStringLibNickname().Lower() + wxS( ":" );

                    name += id.GetUniStringLibItemName().Lower();

                    if( filter->Find( name ) )
                        return true;
                }

                return false;
            };

    if( m_pinCount > 0 && m_filterByPinCount->GetValue() )
    {
        if( aNode.m_PinCount != m_pinCount )
            return false;
    }

    if( !m_fpFilters.empty() && m_filterByFPFilters->GetValue() )
    {
        if( !patternMatch( aNode.m_LibId, m_fpFilters ) )
            return false;
    }

    return true;
}


void FOOTPRINT_CHOOSER_FRAME::doCloseWindow()
{
    // Only dismiss a modal frame once, so that the return values set by
    // the prior DismissModal() are not bashed for ShowModal().
    if( !IsDismissed() )
        DismissModal( false );

    // window to be destroyed by the caller of KIWAY_PLAYER::ShowModal()
}


WINDOW_SETTINGS* FOOTPRINT_CHOOSER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK_MSG( cfg, nullptr, wxT( "config not existing" ) );

    return &cfg->m_FootprintViewer;
}


COLOR_SETTINGS* FOOTPRINT_CHOOSER_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

    if( settings )
        return Pgm().GetSettingsManager().GetColorSettings( settings->m_ColorTheme );
    else
        return Pgm().GetSettingsManager().GetColorSettings();
}


void FOOTPRINT_CHOOSER_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_SYMBOL_NETLIST:
    {
        m_pinCount = 0;
        m_fpFilters.clear();

        /*
         * Symbol netlist format:
         *   pinCount
         *   fpFilters
         */
        std::vector<std::string> strings = split( payload, "\r" );

        if( strings.size() >= 1 )
        {
            wxString pinCountStr( strings[0] );
            pinCountStr.ToInt( &m_pinCount );

            if( m_pinCount > 0 )
            {
                m_filterByPinCount->SetLabel( m_filterByPinCount->GetLabel()
                                                + wxString::Format( wxS( " (%d)" ), m_pinCount ) );
                m_filterByPinCount->Show( true );
            }
        }

        if( strings.size() >= 2 && !strings[1].empty() )
        {
            for( const wxString& filter : wxSplit( strings[1], ' ' ) )
            {
                m_fpFilters.push_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD_ANCHORED>() );
                m_fpFilters.back()->SetPattern( filter.Lower() );
            }

            m_filterByFPFilters->SetLabel( m_filterByFPFilters->GetLabel()
                                            + wxString::Format( wxS( " (%s)" ), strings[1] ) );
            m_filterByFPFilters->Show( true );
        }

        break;
    }

    default:
        break;
    }
}


bool FOOTPRINT_CHOOSER_FRAME::ShowModal( wxString* aFootprint, wxWindow* aParent )
{
    if( aFootprint && !aFootprint->IsEmpty() )
    {
        LIB_ID fpid;

        fpid.Parse( *aFootprint, true );

        if( fpid.IsValid() )
            m_chooserPanel->SetPreselect( fpid );
    }

    return KIWAY_PLAYER::ShowModal( aFootprint, aParent );
}


static wxRect s_dialogRect( 0, 0, 0, 0 );


void FOOTPRINT_CHOOSER_FRAME::SetPosition( const wxPoint& aNewPosition )
{
    PCB_BASE_FRAME::SetPosition( aNewPosition );

    s_dialogRect.SetPosition( aNewPosition );
}


bool FOOTPRINT_CHOOSER_FRAME::Show( bool show )
{
    bool  ret;

    // Show or hide the window.  If hiding, save current position and size.
    // If showing, use previous position and size.
    if( show )
    {
#ifndef __WINDOWS__
        PCB_BASE_FRAME::Raise();  // Needed on OS X and some other window managers (i.e. Unity)
#endif
        ret = PCB_BASE_FRAME::Show( show );

        // returns a zeroed-out default wxRect if none existed before.
        wxRect savedDialogRect = s_dialogRect;

        if( savedDialogRect.GetSize().x != 0 && savedDialogRect.GetSize().y != 0 )
        {
            SetSize( savedDialogRect.GetPosition().x, savedDialogRect.GetPosition().y,
                     std::max( wxWindow::GetSize().x, savedDialogRect.GetSize().x ),
                     std::max( wxWindow::GetSize().y, savedDialogRect.GetSize().y ),
                     0 );
        }

        // Be sure that the dialog appears in a visible area
        // (the dialog position might have been stored at the time when it was
        // shown on another display)
        if( wxDisplay::GetFromWindow( this ) == wxNOT_FOUND )
            Centre();
    }
    else
    {
        s_dialogRect = wxRect( wxWindow::GetPosition(), wxWindow::GetSize() );
        ret = PCB_BASE_FRAME::Show( show );
    }

    return ret;
}


void FOOTPRINT_CHOOSER_FRAME::OnPaint( wxPaintEvent& aEvent )
{
    if( m_firstPaintEvent )
    {
        KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( this );
        KIPLATFORM::UI::ForceFocus( m_chooserPanel->GetFocusTarget() );

        m_firstPaintEvent = false;
    }

    aEvent.Skip();
}


void FOOTPRINT_CHOOSER_FRAME::OnOK( wxCommandEvent& aEvent )
{
    LIB_ID fpID = m_chooserPanel->GetSelectedLibId();

    if( fpID.IsValid() )
    {
        wxString footprint = fpID.Format();

        AddFootprintToHistory( footprint );
        DismissModal( true, footprint );
    }
    else
    {
        DismissModal( false );
    }
}


void FOOTPRINT_CHOOSER_FRAME::closeFootprintChooser( wxCommandEvent& aEvent )
{
    Close( false );
}


