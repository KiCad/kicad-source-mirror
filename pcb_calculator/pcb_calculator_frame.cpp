/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 jean-pierre.charras
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/treebook.h>
#include <wx/sizer.h>

#include <typeinfo>

#include <bitmaps.h>
#include <bitmap_store.h>
#include <geometry/shape_poly_set.h>
#include <kiface_base.h>

#include <attenuators/attenuator_classes.h>
#include <pcb_calculator_frame.h>
#include <pcb_calculator_settings.h>

#include <calculator_panels/panel_attenuators.h>
#include <calculator_panels/panel_board_class.h>
#include <calculator_panels/panel_cable_size.h>
#include <calculator_panels/panel_color_code.h>
#include <calculator_panels/panel_electrical_spacing.h>
#include <calculator_panels/panel_eserie.h>
#include <calculator_panels/panel_regulator.h>
#include <calculator_panels/panel_track_width.h>
#include <calculator_panels/panel_transline.h>
#include <calculator_panels/panel_via_size.h>


PCB_CALCULATOR_FRAME::PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    KIWAY_PLAYER( aParent, wxID_ANY,
                 _( "Calculator Tools"  ), // Window title
                 wxDefaultPosition,
                 wxSize( 646,361 ), // Default size
                 wxDEFAULT_FRAME_STYLE | wxRESIZE_BORDER | wxFULL_REPAINT_ON_RESIZE | wxTAB_TRAVERSAL,
                 wxT( "calculator_tools" ) ), // Window name
    m_lastNotebookPage( -1 )
{
    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    SetKiway( this, aKiway );

    SetSizeHints( wxDefaultSize, wxDefaultSize );


    m_mainSizer = new wxBoxSizer( wxVERTICAL );


    m_treebook = new wxTreebook( this, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    m_mainSizer->Add( m_treebook, 1, wxEXPAND | wxLEFT | wxTOP, 0 );


    SetSizer( m_mainSizer );
    Layout();
    Centre( wxBOTH );

    m_treebook->AddPage( nullptr, _( "General system design" ) );

    AddCalculator( new PANEL_REGULATOR( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Regulators" ) );

    m_treebook->AddPage( nullptr, _( "Power, current and isolation" ) );

    AddCalculator( new PANEL_ELECTRICAL_SPACING( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Electrical Spacing" ) );
    AddCalculator( new PANEL_VIA_SIZE( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Via Size" ) );
    AddCalculator( new PANEL_TRACK_WIDTH( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Track Width" ) );
    AddCalculator( new PANEL_CABLE_SIZE( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxTAB_TRAVERSAL ),
                   _( "Cable Size" ) );

    m_treebook->AddPage( nullptr, _( "High speed" ) );


    AddCalculator( new PANEL_ATTENUATORS( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "RF Attenuators" ) );
    AddCalculator( new PANEL_TRANSLINE( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Transmission Lines") );

    m_treebook->AddPage( nullptr, _( "Memo" ) );

    AddCalculator( new PANEL_E_SERIE( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "E-Series" ) );
    AddCalculator( new PANEL_COLOR_CODE( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _( "Color Code" ) );
    AddCalculator( new PANEL_BOARD_CLASS( m_treebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
                   _("Board Classes") );

    LoadSettings( config() );

    if( PANEL_REGULATOR* regPanel = GetCalculator<PANEL_REGULATOR>() )
        regPanel->ReadDataFile();

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    GetSizer()->SetSizeHints( this );

    // Set previous size and position
    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    if( m_framePos == wxDefaultPosition )
        Centre();

    // Expand treebook to show all options:
    for( size_t pageId = 0; pageId < m_treebook->GetPageCount(); pageId++ )
        m_treebook->ExpandNode( pageId );

    // Connect Events
    Bind( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( PCB_CALCULATOR_FRAME::OnClosePcbCalc ), this );
    Bind( wxEVT_UPDATE_UI,    wxUpdateUIEventHandler( PCB_CALCULATOR_FRAME::OnUpdateUI ),  this );

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( PCB_CALCULATOR_FRAME::onThemeChanged ), this );

    m_treebook->Connect( wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED, wxTreebookEventHandler(
                         PCB_CALCULATOR_FRAME::OnPageChanged  ), NULL, this );
}


PCB_CALCULATOR_FRAME::~PCB_CALCULATOR_FRAME()
{
    m_treebook->Disconnect( wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED, wxTreebookEventHandler(
                            PCB_CALCULATOR_FRAME::OnPageChanged ), NULL, this );
    // This needed for OSX: avoids further OnDraw processing after this destructor and before
    // the native window is destroyed
    this->Freeze();
}

void PCB_CALCULATOR_FRAME::OnPageChanged ( wxTreebookEvent& aEvent )
{
    int page = aEvent.GetSelection();

    // If the selected page is a top level page
    if ( m_treebook->GetPageParent( page ) == wxNOT_FOUND )
    {
        m_treebook->ExpandNode( page );
        // Select the first child
        m_treebook->ChangeSelection( page + 1 );
    }
}

void PCB_CALCULATOR_FRAME::AddCalculator( CALCULATOR_PANEL *aPanel, const wxString& panelUIName )
{
    // Update internal structures
    m_panels.push_back( aPanel );
    m_panelTypes[ typeid( *aPanel ).hash_code() ] = aPanel;

    m_treebook->AddSubPage( aPanel, panelUIName );
}


void PCB_CALCULATOR_FRAME::onThemeChanged( wxSysColourChangedEvent& aEvent )
{
    // Force the bitmaps to refresh
    GetBitmapStore()->ThemeChanged();

    // Update the panels
    for( auto& panel : m_panels )
        panel->ThemeChanged();

    aEvent.Skip();
}


void PCB_CALCULATOR_FRAME::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( m_treebook->GetSelection() != m_lastNotebookPage )
    {
        // Kick all the things that wxWidgets can't seem to redraw on its own.
        // This is getting seriously ridiculous....
        PANEL_TRANSLINE*          translinePanel   = GetCalculator<PANEL_TRANSLINE>();
        PANEL_ATTENUATORS*        attenPanel       = GetCalculator<PANEL_ATTENUATORS>();
        PANEL_VIA_SIZE*           viaSizePanel     = GetCalculator<PANEL_VIA_SIZE>();
        PANEL_REGULATOR*          regulPanel       = GetCalculator<PANEL_REGULATOR>();
        PANEL_ELECTRICAL_SPACING* elecSpacingPanel = GetCalculator<PANEL_ELECTRICAL_SPACING>();

        wxASSERT( translinePanel );
        wxASSERT( attenPanel );
        wxASSERT( viaSizePanel );
        wxASSERT( regulPanel );
        wxASSERT( elecSpacingPanel );

        {
            wxCommandEvent event2( wxEVT_RADIOBUTTON );
            event2.SetEventObject( translinePanel->GetTranslineSelector() );
            event2.SetInt( translinePanel->GetCurrTransLineType() );

            translinePanel->GetTranslineSelector()->Command( event2 );
        }

        for( int i = 0; i < attenPanel->m_AttenuatorList.size(); ++i )
        {
            if( attenPanel->m_AttenuatorList[i] == attenPanel->m_CurrAttenuator )
            {
                wxCommandEvent event2( wxEVT_RADIOBUTTON );
                event2.SetEventObject( attenPanel->GetAttenuatorsSelector() );
                event2.SetInt( i );

                attenPanel->GetAttenuatorsSelector()->Command( event2 );
                break;
            }
        }

        attenPanel->UpdateUI();
   	    viaSizePanel->Layout();
  	    regulPanel->Layout();

        // Until it's shown on screen the above won't work; but doing it anyway at least keeps
        // putting new OnUpdateUI events into the queue until it *is* shown on screen.
        if( m_treebook->IsShownOnScreen() )
            m_lastNotebookPage = m_treebook->GetSelection();
    }
}


void PCB_CALCULATOR_FRAME::OnClosePcbCalc( wxCloseEvent& event )
{
    PANEL_REGULATOR* regPanel = GetCalculator<PANEL_REGULATOR>();

    wxASSERT( regPanel );

    if( regPanel->m_RegulatorListChanged )
    {
        wxString msg;
        wxString title = _( "Write Data Failed" );

        if( regPanel->GetDataFilename().IsEmpty() )
        {
            msg = _( "No data filename to save modifications.\n"
                     "Do you want to exit and abandon your changes?" );

            if( wxMessageBox( msg, title, wxYES_NO | wxICON_QUESTION ) == wxNO )
                return;
        }
        else
        {
            if( !regPanel->WriteDataFile() )
            {
                msg.Printf( _( "Unable to write file '%s'\n"
                               "Do you want to exit and abandon your changes?"),
                            regPanel->GetDataFilename() );

                if( wxMessageBox( msg, title, wxYES_NO | wxICON_ERROR ) == wxNO )
                    return;
            }
        }
    }

    event.Skip();
}


void PCB_CALCULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == nullptr )
        return;

    EDA_BASE_FRAME::LoadSettings( aCfg );

    PCB_CALCULATOR_SETTINGS* cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( aCfg );

    m_treebook->ChangeSelection( cfg->m_LastPage );

    for( auto& panel : m_panels )
        panel->LoadSettings( cfg );
}


void PCB_CALCULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    if( aCfg == nullptr )
        return;

    EDA_BASE_FRAME::SaveSettings( aCfg );

    // Save current parameters values in config.
    auto cfg = dynamic_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
    {
        cfg->m_LastPage = m_treebook->GetSelection();

        for( auto& panel : m_panels )
            panel->SaveSettings( cfg );
    }

}
