/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <3d_canvas/eda_3d_canvas.h>
#include <project_pcb.h>
#include <widgets/bitmap_button.h>
#include <3d_viewer/eda_3d_viewer_frame.h>


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
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D ),
        m_pinCount( 0 ),
        m_firstPaintEvent( true )
{
    SetModal( true );

    m_showFpMode = true;
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

    SetBoard( new BOARD() );

    // This board will only be used to hold a footprint for viewing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    build3DCanvas();    // must be called after creating m_chooserPanel
    m_preview3DCanvas->Show( !m_showFpMode );

    // The m_filterByFPFilters can have a long string, if the symbol has a FP filter
    // with many items, so give it its own sizer
    wxBoxSizer* fpFilterSizerByFP = new wxBoxSizer( wxVERTICAL );
    fpFilterSizerByFP->Add( m_filterByFPFilters, 0, wxLEFT | wxTOP | wxEXPAND, 5 );
    bottomSizer->Add( fpFilterSizerByFP, 0, wxLEFT | wxTOP, 5 );

    // buttonsSizer contains the m_filterByPinCount and BITMAP buttons
    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
    buttonsSizer->Add( m_filterByPinCount, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );

    // Add a spacer between the wxCheckBox and bitmap buttons
    buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );

    m_grButton3DView = new BITMAP_BUTTON( bottomPanel, wxID_ANY,
                                wxNullBitmap, wxDefaultPosition,
                                wxDefaultSize/*, wxBU_AUTODRAW|wxBORDER_NONE*/ );
    m_grButton3DView->SetIsRadioButton();
    m_grButton3DView->SetBitmap( KiBitmapBundle( BITMAPS::shape_3d ) );
    m_grButton3DView->Check( !m_showFpMode );
    buttonsSizer->Add( m_grButton3DView, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );

    m_grButtonFpView = new BITMAP_BUTTON( bottomPanel, wxID_ANY,
                                          wxNullBitmap, wxDefaultPosition,
                                          wxDefaultSize/*, wxBU_AUTODRAW|wxBORDER_NONE*/ );
    m_grButtonFpView->SetIsRadioButton();
    m_grButtonFpView->SetBitmap( KiBitmapBundle( BITMAPS::module ) );
    m_grButtonFpView->Check( m_showFpMode );
    buttonsSizer->Add( m_grButtonFpView, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );

    m_show3DViewer = new wxCheckBox( bottomPanel, wxID_ANY, _( "3D Viewer Shown in Separate Window" ) );
    buttonsSizer->Add( 30, 0, 0, 0, 5 );     // Add spacer
    buttonsSizer->Add( m_show3DViewer, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( bottomPanel, wxID_OK );
    wxButton*               cancelButton = new wxButton( bottomPanel, wxID_CANCEL );

    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    // Add a spacer between the bitmap buttons and thesdbSizer
    buttonsSizer->Add( 0, 0, 1, wxEXPAND, 5 );
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

    // Connect Events
    m_grButton3DView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                         wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::on3DviewReq ),
                         NULL, this );

    m_grButtonFpView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                             wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpViewReq ),
                             NULL, this );

    m_show3DViewer->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED ,
                             wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onExternalViewer3DEnable ),
                             NULL, this );

    Connect( FP_SELECTION_EVENT,  // custom event fired by a PANEL_FOOTPRINT_CHOOSER
             wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpChanged ), NULL, this );

    // Needed on Linux to fix the position of widgets in bottomPanel
    PostSizeEvent();
}


FOOTPRINT_CHOOSER_FRAME::~FOOTPRINT_CHOOSER_FRAME()
{
    // Disconnect Events
    m_grButton3DView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::on3DviewReq ),
                                  NULL, this );
    m_grButtonFpView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpViewReq ),
                                NULL, this );

    m_show3DViewer->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED ,
                                wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onExternalViewer3DEnable ),
                                NULL, this );

    Disconnect( FP_SELECTION_EVENT,
                wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpChanged ), NULL, this );

    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_FootprintChooser.use_fp_filters = m_filterByFPFilters->GetValue();
        cfg->m_FootprintChooser.filter_on_pin_count = m_filterByPinCount->GetValue();
    }
}


void FOOTPRINT_CHOOSER_FRAME::onExternalViewer3DEnable( wxCommandEvent& aEvent )
{
    if( aEvent.IsChecked() )
    {
        if( m_grButton3DView->IsChecked() )
            Show3DViewerFrame();        // show external 3D viewer
    }
    else
    {
        // Close the external 3D viewer frame, if it is still enabled
        EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();

        if( viewer3D )
            viewer3D->Close( true );
    }

    updatePanelsVisibility();
}


void FOOTPRINT_CHOOSER_FRAME::Show3DViewerFrame()
{
    bool do_reload_board = true;    // reload board flag

    // At EDA_3D_VIEWER_FRAME creation, the current board is loaded, so disable loading
    // the current board if the 3D frame is not yet created
    if( Get3DViewerFrame() == nullptr )
        do_reload_board = false;

    EDA_3D_VIEWER_FRAME* draw3DFrame = CreateAndShow3D_Frame();

    // A stronger version of Raise() which promotes the window to its parent's level.
    KIPLATFORM::UI::ReparentQuasiModal( draw3DFrame );

    // And load or update the current board (if needed)
    if( do_reload_board )
        Update3DView( true, true );
}


void FOOTPRINT_CHOOSER_FRAME::Update3DView( bool aMarkDirty,
                                            bool aRefresh, const wxString* aTitle )
{
    LIB_ID fpID = m_chooserPanel->GetSelectedLibId();
    wxString footprintName;

    if( fpID.IsValid() )
        footprintName = fpID.Format();

    wxString title = _( "3D Viewer" ) + wxT( " \u2014 " ) + footprintName;
    PCB_BASE_FRAME::Update3DView( aMarkDirty, aRefresh, &title );
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
         *   pinNumber pinName <tab> pinNumber pinName...
         *   fpFilter fpFilter...
         */
        std::map<wxString, wxString> pinNames;
        std::vector<std::string>     strings = split( payload, "\r" );

        if( strings.size() >= 1 && !strings[0].empty() )
        {
            for( const wxString& pin : wxSplit( strings[0], '\t' ) )
                pinNames[ pin.BeforeFirst( ' ' ) ] = pin.AfterFirst( ' ' );

            m_pinCount = pinNames.size();

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

        m_chooserPanel->GetViewerPanel()->SetPinFunctions( pinNames );
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


void FOOTPRINT_CHOOSER_FRAME::onFpChanged( wxCommandEvent& event )
{
    // Ensure a 3D display is activated if a 3D view is needed
    if( !displayFootprintPanel() && !m_preview3DCanvas->IsShown() )
    {
        wxCommandEvent dummy_event;
        on3DviewReq( dummy_event );
    }

    updateViews();
}


void FOOTPRINT_CHOOSER_FRAME::build3DCanvas()
{
    // Create the dummy board used by the 3D canvas
    m_dummyBoard = GetBoard();
    m_dummyBoard->SetProject( &Prj(), true );

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    m_boardAdapter.SetBoard( m_dummyBoard );
    m_boardAdapter.m_IsBoardView = false;
    m_boardAdapter.m_IsPreviewer = true;   // Force display 3D models, regardless the 3D viewer options

    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();
    m_boardAdapter.m_Cfg = cfg;

    // Build the 3D canvas
    m_preview3DCanvas = new EDA_3D_CANVAS( m_chooserPanel->m_RightPanel,
                                        OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                        m_boardAdapter, m_currentCamera,
                                        PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    m_chooserPanel->m_RightPanelSizer->Add( m_preview3DCanvas, 1, wxEXPAND, 5 );
    m_chooserPanel->m_RightPanel->Layout();

    BOARD_DESIGN_SETTINGS& dummy_bds = m_dummyBoard->GetDesignSettings();
    dummy_bds.SetBoardThickness( pcbIUScale.mmToIU( 1.6 ) );
    dummy_bds.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );
    BOARD_STACKUP& dummy_board_stackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    dummy_board_stackup.RemoveAll();
    dummy_board_stackup.BuildDefaultStackupList( &dummy_bds, 2 );
}


void FOOTPRINT_CHOOSER_FRAME::on3DviewReq( wxCommandEvent& event )
{
    m_showFpMode = false;
    m_grButtonFpView->Check( m_showFpMode );
    m_grButton3DView->Check( !m_showFpMode );

    if( m_show3DViewer->IsChecked() )
        Show3DViewerFrame();

    updatePanelsVisibility();
}


void FOOTPRINT_CHOOSER_FRAME::onFpViewReq( wxCommandEvent& event )
{
    // Close 3D viewer frame, if it is still enabled
    EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();

    if( viewer3D )
        viewer3D->Close( true );

    m_showFpMode = true;

    m_grButtonFpView->Check( m_showFpMode );
    m_grButton3DView->Check( !m_showFpMode );

    updatePanelsVisibility();
}


bool FOOTPRINT_CHOOSER_FRAME::displayFootprintPanel()
{
    return Get3DViewerFrame() || m_showFpMode;
}


void FOOTPRINT_CHOOSER_FRAME::updateViews()
{
    EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();
    bool reloadFp = viewer3D || m_preview3DCanvas->IsShown();

    if( reloadFp )
    {
        m_dummyBoard->DeleteAllFootprints();

        if( m_chooserPanel->m_CurrFootprint )
            m_dummyBoard->Add( (FOOTPRINT*)m_chooserPanel->m_CurrFootprint->Clone() );

    }

    if( m_preview3DCanvas->IsShown() )
    {
        m_preview3DCanvas->ReloadRequest();
        m_preview3DCanvas->Request_refresh();
    }

    if( viewer3D )
    {
        Update3DView( true, true );
    }

    m_chooserPanel->m_RightPanel->Layout();
    m_chooserPanel->m_RightPanel->Refresh();
}

void FOOTPRINT_CHOOSER_FRAME::updatePanelsVisibility()
{
    FOOTPRINT_PREVIEW_WIDGET* viewFpPanel = m_chooserPanel->GetViewerPanel();
    viewFpPanel->Show( displayFootprintPanel() );
    m_preview3DCanvas->Show( !displayFootprintPanel() );

    updateViews();
}

