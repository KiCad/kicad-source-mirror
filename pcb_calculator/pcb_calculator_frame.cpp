/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/msgdlg.h>
#include <wx/treebook.h>
#include <wx/sizer.h>

#include <typeinfo>

#include <bitmaps.h>
#include <bitmap_store.h>
#include <geometry/shape_poly_set.h>
#include <kiface_base.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_control.h>
#include <attenuators/attenuator_classes.h>
#include <pcb_calculator_frame.h>
#include <pcb_calculator_settings.h>

#include <calculator_panels/panel_rf_attenuators.h>
#include <calculator_panels/panel_board_class.h>
#include <calculator_panels/panel_cable_size.h>
#include <calculator_panels/panel_galvanic_corrosion.h>
#include <calculator_panels/panel_color_code.h>
#include <calculator_panels/panel_electrical_spacing.h>
#include <calculator_panels/panel_r_calculator.h>
#include <calculator_panels/panel_fusing_current.h>
#include <calculator_panels/panel_regulator.h>
#include <calculator_panels/panel_track_width.h>
#include <calculator_panels/panel_transline.h>
#include <calculator_panels/panel_via_size.h>
#include <calculator_panels/panel_wavelength.h>
#include <calculator_panels/panel_eseries_display.h>
#include "widgets/wx_menubar.h"


BEGIN_EVENT_TABLE( PCB_CALCULATOR_FRAME, KIWAY_PLAYER )
    EVT_MENU( wxID_CLOSE, PCB_CALCULATOR_FRAME::OnExit )
    EVT_MENU( wxID_EXIT, PCB_CALCULATOR_FRAME::OnExit )
END_EVENT_TABLE()


PCB_CALCULATOR_FRAME::PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    KIWAY_PLAYER( aKiway, aParent, FRAME_CALC, _( "Calculator Tools" ), wxDefaultPosition,
                  wxDefaultSize,
                  wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL,
                  wxT( "calculator_tools" ), unityScale ),
    m_lastNotebookPage( -1 )
{
    m_aboutTitle = _HKI( "KiCad Calculator Tools" );

    SHAPE_POLY_SET dummy;   // A ugly trick to force the linker to include
                            // some methods in code and avoid link errors

    SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_mainSizer = new wxBoxSizer( wxVERTICAL );

    m_treebook = new wxTreebook( this, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    m_mainSizer->Add( m_treebook, 1, wxEXPAND | wxLEFT | wxTOP, 0 );

    SetSizer( m_mainSizer );
    Layout();
    Centre( wxBOTH );

    loadPages();

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_pcbcalculator_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, config(), this );

    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->InitTools();

    ReCreateMenuBar();
    setupUIConditions();

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
                         PCB_CALCULATOR_FRAME::OnPageChanged  ), nullptr, this );
}


PCB_CALCULATOR_FRAME::~PCB_CALCULATOR_FRAME()
{
    m_treebook->Disconnect( wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED, wxTreebookEventHandler(
                            PCB_CALCULATOR_FRAME::OnPageChanged ), nullptr, this );
    // This needed for OSX: avoids further OnDraw processing after this destructor and before
    // the native window is destroyed
    this->Freeze();

    // Clean up the tool framework
    delete m_toolManager;
    delete m_toolDispatcher;
}


void PCB_CALCULATOR_FRAME::OnExit( wxCommandEvent& aEvent )
{
    Close( false );
}


void PCB_CALCULATOR_FRAME::loadPages()
{
    m_treebook->AddPage( nullptr, _( "General system design" ) );

    AddCalculator( new PANEL_REGULATOR( m_treebook ), _( "Regulators" ) );
    AddCalculator( new PANEL_R_CALCULATOR( m_treebook ), _( "Resistor Calculator" ) );

    m_treebook->AddPage( nullptr, _( "Power, current and isolation" ) );

    AddCalculator( new PANEL_ELECTRICAL_SPACING( m_treebook ), _( "Electrical Spacing" ) );
    AddCalculator( new PANEL_VIA_SIZE( m_treebook ), _( "Via Size" ) );
    AddCalculator( new PANEL_TRACK_WIDTH( m_treebook ), _( "Track Width" ) );
    AddCalculator( new PANEL_FUSING_CURRENT( m_treebook ), _( "Fusing Current" ) );
    AddCalculator( new PANEL_CABLE_SIZE( m_treebook ), _( "Cable Size" ) );

    m_treebook->AddPage( nullptr, _( "High Speed" ) );

    AddCalculator( new PANEL_WAVELENGTH( m_treebook ), _( "Wavelength" ) );
    AddCalculator( new PANEL_RF_ATTENUATORS( m_treebook ), _( "RF Attenuators" ) );
    AddCalculator( new PANEL_TRANSLINE( m_treebook ), _( "Transmission Lines") );

    m_treebook->AddPage( nullptr, _( "Memo" ) );

    AddCalculator( new PANEL_ESERIES_DISPLAY( m_treebook ), _( "E-Series" ) );
    AddCalculator( new PANEL_COLOR_CODE( m_treebook ), _( "Color Code" ) );
    AddCalculator( new PANEL_BOARD_CLASS( m_treebook ), _( "Board Classes" ) );
    AddCalculator( new PANEL_GALVANIC_CORROSION( m_treebook ), _( "Galvanic Corrosion" ) );

    LoadSettings( config() );

    if( PANEL_REGULATOR* regPanel = GetCalculator<PANEL_REGULATOR>() )
        regPanel->ReadDataFile();
}


void PCB_CALCULATOR_FRAME::doReCreateMenuBar()
{
    COMMON_CONTROL* tool = m_toolManager->GetTool<COMMON_CONTROL>();
    EDA_BASE_FRAME* base_frame = dynamic_cast<EDA_BASE_FRAME*>( this );

    // base_frame == nullptr should not happen, but it makes Coverity happy
    wxCHECK( base_frame, /* void */ );

    // wxWidgets handles the OSX Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = base_frame->GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, tool );

    fileMenu->AddClose( _( "Calculator Tools" ) );
    fileMenu->AddQuit( _( "Calculator Tools" ) );

    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, tool );

    prefsMenu->Add( ACTIONS::openPreferences );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, tool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    base_frame->AddStandardHelpMenu( menuBar );

    base_frame->SetMenuBar( menuBar );
    delete oldMenuBar;
}


void PCB_CALCULATOR_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    SetTitle( _( "Calculator Tools" ) );

    SaveSettings( config() );
    Freeze();

    int page = m_treebook->GetSelection();
    m_treebook->DeleteAllPages();
    m_panels.clear();

    loadPages();
    Layout();

    m_treebook->SetSelection( page );
    LoadSettings( config() );

    Thaw();
    Refresh();
}


void PCB_CALCULATOR_FRAME::OnPageChanged ( wxTreebookEvent& aEvent )
{
    int page = aEvent.GetSelection();

    // If the selected page is a top level page
    if ( m_treebook->GetPageParent( page ) == wxNOT_FOUND )
    {
        m_treebook->ExpandNode( page );

        // Select the first child
        if( page + 1 < m_treebook->GetPageCount() )
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
        PANEL_RF_ATTENUATORS*     attenPanel       = GetCalculator<PANEL_RF_ATTENUATORS>();
        PANEL_VIA_SIZE*           viaSizePanel     = GetCalculator<PANEL_VIA_SIZE>();
        PANEL_REGULATOR*          regulPanel       = GetCalculator<PANEL_REGULATOR>();

        wxASSERT( translinePanel );
        wxASSERT( attenPanel );
        wxASSERT( viaSizePanel );
        wxASSERT( regulPanel );

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

    for( CALCULATOR_PANEL* panel : m_panels )
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

        for( CALCULATOR_PANEL* panel : m_panels )
            panel->SaveSettings( cfg );
    }

}
