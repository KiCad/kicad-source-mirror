/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_express.h>
#include <board.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/splitter.h>
#include <kiplatform/ui.h>
#include <lset.h>
#include <widgets/panel_footprint_chooser.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <footprint_chooser_frame.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <tools/footprint_chooser_selection_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_viewer_tools.h>
#include "settings/cvpcb_settings.h"
#include "wx/display.h"
#include <footprint_preview_panel.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <project_pcb.h>
#include <widgets/bitmap_button.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <tools/pcb_editor_conditions.h>


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


#define PARENT_STYLE ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                       | wxWANTS_CHARS | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT )
#define MODAL_STYLE  ( wxRESIZE_BORDER | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN \
                      | wxWANTS_CHARS | wxFRAME_NO_TASKBAR )


FOOTPRINT_CHOOSER_FRAME::FOOTPRINT_CHOOSER_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        PCB_BASE_FRAME( aKiway, aParent, FRAME_FOOTPRINT_CHOOSER, _( "Footprint Chooser" ),
                        wxDefaultPosition, wxDefaultSize, aParent ? PARENT_STYLE : MODAL_STYLE,
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

    m_messagePanel->Hide();

    m_bottomPanel = new wxPanel( this );
    wxBoxSizer* bottomSizer = new wxBoxSizer( wxVERTICAL );
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

    SetCanvas( m_chooserPanel->GetViewerPanel()->GetPreviewPanel()->GetCanvas() );
    SetBoard( m_chooserPanel->GetViewerPanel()->GetPreviewPanel()->GetBoard() );

    // This board will only be used to hold a footprint for viewing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    build3DCanvas();    // must be called after creating m_chooserPanel
    m_preview3DCanvas->Show( m_show3DMode );

    // buttonsSizer contains the BITMAP buttons
    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    buttonsSizer->Add( 0, 0, 1, 0, 5 );     // Add spacer to right-align buttons


    m_toggleDescription = new BITMAP_BUTTON( m_bottomPanel, wxID_ANY, wxNullBitmap );
    m_toggleDescription->SetIsRadioButton();
    m_toggleDescription->SetBitmap( KiBitmapBundle( BITMAPS::text_visibility_off ) );
    m_toggleDescription->SetToolTip( _( "Show/hide description panel" ) );
    m_toggleDescription->Check( m_showDescription );
    buttonsSizer->Add( m_toggleDescription, 0, wxRIGHT | wxLEFT | wxALIGN_CENTER_VERTICAL, 1 );

    BITMAP_BUTTON* separator = new BITMAP_BUTTON( m_bottomPanel, wxID_ANY, wxNullBitmap );
    separator->SetIsSeparator();
    buttonsSizer->Add( separator, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    m_grButton3DView = new BITMAP_BUTTON( m_bottomPanel, wxID_ANY, wxNullBitmap );
    m_grButton3DView->SetIsRadioButton();
    m_grButton3DView->SetBitmap( KiBitmapBundle( BITMAPS::shape_3d ) );
    m_grButton3DView->SetToolTip( _( "Show/hide 3D view panel" ) );
    m_grButton3DView->Check( m_show3DMode );
    buttonsSizer->Add( m_grButton3DView, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    m_grButtonFpView = new BITMAP_BUTTON( m_bottomPanel, wxID_ANY, wxNullBitmap );
    m_grButtonFpView->SetIsRadioButton();
    m_grButtonFpView->SetBitmap( KiBitmapBundle( BITMAPS::module ) );
    m_grButtonFpView->SetToolTip( _( "Show/hide footprint view panel" ) );
    m_grButtonFpView->Check( m_showFpMode );
    buttonsSizer->Add( m_grButtonFpView, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    separator = new BITMAP_BUTTON( m_bottomPanel, wxID_ANY, wxNullBitmap );
    separator->SetIsSeparator();
    buttonsSizer->Add( separator, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    m_show3DViewer = new wxCheckBox( m_bottomPanel, wxID_ANY, _( "Show 3D viewer in own window" ) );
    buttonsSizer->Add( m_show3DViewer, 0, wxALL | wxALIGN_CENTER_VERTICAL, 3 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( m_bottomPanel, wxID_OK );
    wxButton*               cancelButton = new wxButton( m_bottomPanel, wxID_CANCEL );

    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    buttonsSizer->Add( 20, 0, 0, 0, 5 );     // Add spacer
    buttonsSizer->Add( sdbSizer, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5 );
    bottomSizer->Add( buttonsSizer, 0, wxEXPAND, 5 );

    m_bottomPanel->SetSizer( bottomSizer );
    frameSizer->Add( m_bottomPanel, 0, wxEXPAND );

    SetSizer( frameSizer );

    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ),
                                             m_chooserPanel->GetItemCount() ) );

    Layout();
    m_chooserPanel->FinishSetup();

    if( !m_showDescription )
    {
        m_chooserPanel->GetVerticalSpliter()->SetMinimumPaneSize( 0 );
        m_chooserPanel->GetVerticalSpliter()->GetWindow2()->Hide();
        m_chooserPanel->GetVerticalSpliter()->SetSashInvisible();

        m_toggleDescription->SetBitmap( KiBitmapBundle( BITMAPS::text_visibility ) );
    }

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), GetViewerSettingsBase(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    GetCanvas()->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_TOOLS );    // for std context menus (zoom & grid)
    m_toolManager->RegisterTool( new PCB_PICKER_TOOL ); // for setting grid origin
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );
    m_toolManager->RegisterTool( new FOOTPRINT_CHOOSER_SELECTION_TOOL );

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );
    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetIsDefaultTool( true );

    m_toolManager->InitTools();

    setupUIConditions();
    updatePanelsVisibility();

    // clang-format off
    // Connect Events
    m_toggleDescription->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                                  wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::toggleBottomSplit ),
                                  nullptr, this );

    m_grButton3DView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                               wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::on3DviewReq ),
                               nullptr, this );

    m_grButtonFpView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                               wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpViewReq ),
                               nullptr, this );

    m_show3DViewer->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED ,
                             wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onExternalViewer3DEnable ),
                             nullptr, this );

    Connect( FP_SELECTION_EVENT,  // custom event fired by a PANEL_FOOTPRINT_CHOOSER
             wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpChanged ), nullptr, this );
    // clang-format on

    // Needed on Linux to fix the position of widgets in bottomPanel
    PostSizeEvent();
}


FOOTPRINT_CHOOSER_FRAME::~FOOTPRINT_CHOOSER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // Work around assertion firing when we try to LockCtx on a hidden 3D canvas during dtor
    wxCloseEvent dummy;
    m_preview3DCanvas->Show();
    m_preview3DCanvas->OnCloseWindow( dummy );

    // Ensure view and data used by the preview panel are cleared before deleting other items
    static_cast<FOOTPRINT_PREVIEW_PANEL*>( m_chooserPanel->GetViewerPanel()->GetPreviewPanel() )->ClearViewAndData();

    // Disconnect board, which is owned by FOOTPRINT_PREVIEW_PANEL.
    m_pcb = nullptr;

    // clang-format off
    // Disconnect Events
    m_toggleDescription->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                     wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::toggleBottomSplit ),
                                     nullptr, this );

    m_grButton3DView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::on3DviewReq ),
                                  nullptr, this );
    m_grButtonFpView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpViewReq ),
                                  nullptr, this );

    m_show3DViewer->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED ,
                                wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onExternalViewer3DEnable ),
                                nullptr, this );

    Disconnect( FP_SELECTION_EVENT,
                wxCommandEventHandler( FOOTPRINT_CHOOSER_FRAME::onFpChanged ), nullptr, this );

    // clang-format on

    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        if( m_filterByFPFilters )
            cfg->m_FootprintChooser.use_fp_filters = m_filterByFPFilters->GetValue();

        if( m_filterByPinCount )
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
    KIPLATFORM::UI::ReparentModal( draw3DFrame );

    // And load or update the current board (if needed)
    if( do_reload_board )
        Update3DView( true, true );
}


void FOOTPRINT_CHOOSER_FRAME::Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle )
{
    LIB_ID fpID = m_chooserPanel->GetSelectedLibId();
    wxString footprintName;

    if( fpID.IsValid() )
        footprintName << fpID.Format();

    wxString title = _( "3D Viewer" ) + wxT( " \u2014 " ) + footprintName;
    PCB_BASE_FRAME::Update3DView( aMarkDirty, aRefresh, &title );
}


bool FOOTPRINT_CHOOSER_FRAME::filterByPinCount()
{
    if( m_filterByPinCount )
        return m_filterByPinCount->GetValue();

    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
        return cfg->m_FootprintChooser.filter_on_pin_count;

    return false;
}


bool FOOTPRINT_CHOOSER_FRAME::filterByFPFilters()
{
    if( m_filterByFPFilters )
        return m_filterByFPFilters->GetValue();

    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
        return cfg->m_FootprintChooser.use_fp_filters;

    return false;
}


bool FOOTPRINT_CHOOSER_FRAME::filterFootprint( LIB_TREE_NODE& aNode )
{
    if( aNode.m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
    {
        // Normally lib nodes get scored by the max of their children's scores.  However, if a
        // lib node *has* no children then the scorer will call the filter on the lib node itself,
        // and we just want to return true if we're not filtering at all.
        return !filterByPinCount() && !filterByFPFilters();
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

    if( m_pinCount > 0 && filterByPinCount() )
    {
        if( aNode.m_PinCount != m_pinCount )
            return false;
    }

    if( !m_fpFilters.empty() && filterByFPFilters() )
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
    if( PCBNEW_SETTINGS* pcb_cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg ) )
        return &pcb_cfg->m_FootprintViewer;
    else if( CVPCB_SETTINGS* cvpcb_cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg ) )
        return &cvpcb_cfg->m_FootprintViewer;

    wxFAIL_MSG( wxT( "FOOTPRINT_CHOOSER not running with PCBNEW_SETTINGS or CVPCB_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


COLOR_SETTINGS* FOOTPRINT_CHOOSER_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


static wxRect s_dialogRect( 0, 0, 0, 0 );


void FOOTPRINT_CHOOSER_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_SYMBOL_NETLIST:
    {
        wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "MAIL_SYMBOL_NETLIST received: size=%zu" ), payload.size() );
        wxSizer*  filtersSizer = m_chooserPanel->GetFiltersSizer();
        wxWindow* filtersWindow = filtersSizer->GetContainingWindow();
        wxString  msg;

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
            wxArrayString tokens = wxSplit( strings[0], '\t' );

            wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "First line entries=%u" ), (unsigned) tokens.size() );

            for( const wxString& pin : tokens )
                pinNames[ pin.BeforeFirst( ' ' ) ] = pin.AfterFirst( ' ' );

            m_pinCount = (int) pinNames.size();

            wxString pinList;

            for( const auto& kv : pinNames )
            {
                if( !pinList.IsEmpty() )
                    pinList << wxS( "," );

                pinList << kv.first;
            }

            wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Parsed pins=%d -> [%s]" ), m_pinCount, pinList );
        }

        if( strings.size() >= 2 && !strings[1].empty() )
        {
            for( const wxString& filter : wxSplit( strings[1], ' ' ) )
            {
                m_fpFilters.push_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD_ANCHORED>() );
                m_fpFilters.back()->SetPattern( filter.Lower() );
            }
        }

        if( !m_fpFilters.empty() )
        {
            msg.Printf( _( "Apply footprint filters (%s)" ), strings[1] );

            if( !m_filterByFPFilters )
            {
                m_filterByFPFilters = new wxCheckBox( filtersWindow, wxID_ANY, msg );

                m_filterByFPFilters->Bind( wxEVT_CHECKBOX,
                        [&]( wxCommandEvent& evt )
                        {
                            m_chooserPanel->Regenerate();
                        } );

                if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
                    m_filterByFPFilters->SetValue( cfg->m_FootprintChooser.use_fp_filters );

                m_chooserPanel->GetFiltersSizer()->Add( m_filterByFPFilters, 0, wxEXPAND|wxBOTTOM, 4 );
            }

            m_filterByFPFilters->SetLabel( msg );
        }
        else
        {
            if( m_filterByFPFilters )
                m_filterByFPFilters->Hide();
        }

        if( m_pinCount > 0 )
        {
            msg.Printf( _( "Filter by pin count (%d)" ), m_pinCount );
            wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "Pin-count label: %s" ), msg );

            if( !m_filterByPinCount )
            {
                m_filterByPinCount = new wxCheckBox( filtersWindow, wxID_ANY, msg );

                m_filterByPinCount->Bind( wxEVT_CHECKBOX,
                        [&]( wxCommandEvent& evt )
                        {
                            m_chooserPanel->Regenerate();
                        } );

                if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
                    m_filterByPinCount->SetValue( cfg->m_FootprintChooser.filter_on_pin_count );

                m_chooserPanel->GetFiltersSizer()->Add( m_filterByPinCount, 0, wxEXPAND|wxBOTTOM, 4 );
            }

            m_filterByPinCount->SetLabel( msg );
        }
        else
        {
            if( m_filterByPinCount )
                m_filterByPinCount->Hide();
        }

        m_chooserPanel->GetViewerPanel()->SetPinFunctions( pinNames );
        wxLogTrace( "FOOTPRINT_CHOOSER", wxS( "SetPinFunctions called with %zu entries" ), pinNames.size() );

        // Save the wxFormBuilder size of the dialog...
        if( s_dialogRect.GetSize().x == 0 || s_dialogRect.GetSize().y == 0 )
            s_dialogRect = wxRect( wxWindow::GetPosition(), wxWindow::GetSize() );

        // ... and then give it a kick to get it to layout the new items
        GetSizer()->SetSizeHints( this );
        break;
    }

    default:
        break;
    }
}


BOARD_ITEM_CONTAINER* FOOTPRINT_CHOOSER_FRAME::GetModel() const
{
    return static_cast<FOOTPRINT_PREVIEW_PANEL*>( m_chooserPanel->GetViewerPanel()->GetPreviewPanel() )->GetCurrentFootprint();
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
    updateViews();

    GetToolManager()->RunAction( ACTIONS::measureTool );
}


void FOOTPRINT_CHOOSER_FRAME::build3DCanvas()
{
    // initialize m_boardAdapter used by the 3D canvas
    BOARD* dummyBoard = GetBoard();
    m_boardAdapter.SetBoard( dummyBoard );
    m_boardAdapter.m_IsBoardView = false;
    m_boardAdapter.m_IsPreviewer = true;   // Force display 3D models, regardless the 3D viewer options

    m_boardAdapter.m_Cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    // Build the 3D canvas
    m_preview3DCanvas = new EDA_3D_CANVAS( m_chooserPanel->m_RightPanel,
                                           OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                           m_boardAdapter, m_currentCamera,
                                           PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    m_chooserPanel->m_RightPanelSizer->Add( m_preview3DCanvas, 1, wxEXPAND, 5 );
    m_chooserPanel->m_RightPanel->Layout();

    BOARD_DESIGN_SETTINGS& dummy_bds = dummyBoard->GetDesignSettings();
    dummy_bds.SetBoardThickness( pcbIUScale.mmToIU( 1.6 ) );
    dummy_bds.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );
    BOARD_STACKUP& dummy_board_stackup = dummyBoard->GetDesignSettings().GetStackupDescriptor();
    dummy_board_stackup.RemoveAll();
    dummy_board_stackup.BuildDefaultStackupList( &dummy_bds, 2 );
}


void FOOTPRINT_CHOOSER_FRAME::toggleBottomSplit( wxCommandEvent& event )
{
    m_showDescription = !m_showDescription;

    m_toggleDescription->Check( m_showDescription );

    m_chooserPanel->GetDetailsPanel()->Show( m_showDescription );

    if( !m_showDescription )
    {
        m_chooserPanel->GetVerticalSpliter()->SetMinimumPaneSize( GetSize().GetHeight() );
        m_chooserPanel->GetVerticalSpliter()->SetSashPosition(
                GetSize().GetHeight() + m_chooserPanel->GetDetailsPanel()->GetSize().GetHeight() );

        m_chooserPanel->GetVerticalSpliter()->GetWindow2()->Hide();
        m_chooserPanel->GetVerticalSpliter()->SetSashInvisible();

        m_toggleDescription->SetBitmap( KiBitmapBundle( BITMAPS::text_visibility ) );
    }
    else
    {
        m_chooserPanel->GetVerticalSpliter()->SetMinimumPaneSize( 80 );
        m_chooserPanel->GetVerticalSpliter()->GetWindow2()->Show();
        m_chooserPanel->GetVerticalSpliter()->SetSashInvisible( false );

        m_toggleDescription->SetBitmap( KiBitmapBundle( BITMAPS::text_visibility_off ) );
    }

    m_chooserPanel->GetVerticalSpliter()->UpdateSize();

    m_chooserPanel->Layout();
    m_chooserPanel->Refresh();
}


void FOOTPRINT_CHOOSER_FRAME::on3DviewReq( wxCommandEvent& event )
{
    if( m_show3DMode == true )
    {
        if( m_showFpMode == true )
        {
            m_show3DMode = false;
            m_grButton3DView->Check( m_show3DMode );
            updatePanelsVisibility();
        }
    }
    else
    {
        if( m_show3DViewer->IsChecked() )
        {
            Show3DViewerFrame();
        }
        else
        {
            // Close 3D viewer frame, if it is still enabled
            EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();

            if( viewer3D )
                viewer3D->Close( true );
        }

        m_show3DMode = true;
        m_grButton3DView->Check( m_show3DMode );
        updatePanelsVisibility();
    }
}


void FOOTPRINT_CHOOSER_FRAME::onFpViewReq( wxCommandEvent& event )
{
    if( m_showFpMode == true )
    {
        if( m_show3DMode == true )
        {
            m_showFpMode = false;
            m_grButtonFpView->Check( m_showFpMode );
            updatePanelsVisibility();
        }
    }
    else
    {
        m_showFpMode = true;
        m_grButtonFpView->Check( m_showFpMode );
        updatePanelsVisibility();
    }
}


void FOOTPRINT_CHOOSER_FRAME::updateViews()
{
    EDA_3D_VIEWER_FRAME* viewer3D = Get3DViewerFrame();

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
    viewFpPanel->Show( m_showFpMode );
    m_preview3DCanvas->Show( m_show3DMode );

    updateViews();
}


void FOOTPRINT_CHOOSER_FRAME::setupUIConditions()
{
    PCB_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

    // clang-format off
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::toggleGrid,            CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::cursorSmallCrosshairs, CHECK( cond.CursorSmallCrosshairs() ) );
    mgr->SetConditions( ACTIONS::cursorFullCrosshairs,  CHECK( cond.CursorFullCrosshairs() ) );
    mgr->SetConditions( ACTIONS::cursor45Crosshairs,    CHECK( cond.Cursor45Crosshairs() ) );

    mgr->SetConditions( PCB_ACTIONS::showPadNumbers,    CHECK( cond.PadNumbersDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,    CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,      CHECK( !cond.TextFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines,  CHECK( !cond.GraphicsFillDisplay() ) );

#undef CHECK
    // clang-format on
}


