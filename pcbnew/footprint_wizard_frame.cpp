/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "footprint_wizard_frame.h"

#include <wx/listbox.h>
#include <wx/numformatter.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>

#include <json_common.h>

#include <kiface_base.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <gal/gal_display_options.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <board.h>
#include <project/net_settings.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <pcbnew_id.h>
#include <widgets/aui_json_serializer.h>
#include <widgets/footprint_wizard_properties_panel.h>

#include <nlohmann/json.hpp>
#include <wx/wupdlock.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_tools.h>
#include <tool/common_control.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_control.h>
#include <tools/pcb_actions.h>
#include <tools/footprint_wizard_tools.h>
#include <toolbars_footprint_wizard.h>


BEGIN_EVENT_TABLE( FOOTPRINT_WIZARD_FRAME, PCB_BASE_EDIT_FRAME )

    // Window events
    EVT_SIZE( FOOTPRINT_WIZARD_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_WIZARD_FRAME::OnActivate )

     // Toolbar events
    EVT_TOOL( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard )
    EVT_TOOL( ID_FOOTPRINT_WIZARD_DONE, FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint )

END_EVENT_TABLE()


// Note: our FOOTPRINT_WIZARD_FRAME is always modal.

FOOTPRINT_WIZARD_FRAME::FOOTPRINT_WIZARD_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                                FRAME_T aFrameType ) :
        PCB_BASE_EDIT_FRAME( aKiway, aParent, aFrameType, _( "Footprint Wizard" ),
                             wxDefaultPosition, wxDefaultSize,
                             aParent ? KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT
                             : KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP,
                             FOOTPRINT_WIZARD_FRAME_NAME ),
        m_wizardListShown( false )
{
    wxASSERT( aFrameType == FRAME_FOOTPRINT_WIZARD );

    // This frame is always show modal:
    SetModal( true );

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::module_wizard ) );
    SetIcon( icon );

    m_currentWizard = nullptr;
    m_wizardManager = std::make_unique<FOOTPRINT_WIZARD_MANAGER>();

    // Create the GAL canvas.
    // Must be created before calling LoadSettings() that needs a valid GAL canvas
    PCB_DRAW_PANEL_GAL* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ),
                                                                m_frameSize,
                                                                GetGalDisplayOptions(),
                                                                EDA_DRAW_PANEL_GAL::GAL_FALLBACK );
    SetCanvas( gal_drawPanel );

    SetBoard( new BOARD() );

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );
    GetScreen()->m_Center = true;      // Center coordinate origins on screen.

    LoadSettings( config() );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    // Set some display options here, because the FOOTPRINT_WIZARD_FRAME
    // does not have a config menu to do that:

    // the footprint wizard frame has no config menu. so use some settings
    // from the caller, or force some options:
    PCB_BASE_FRAME* caller = dynamic_cast<PCB_BASE_FRAME*>( aParent );

    if( caller )
        SetUserUnits( caller->GetUserUnits() );

    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 0 );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCB_CONTROL );
    m_toolManager->RegisterTool( new PCB_SELECTION_TOOL );  // for std context menus (zoom & grid)
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new FOOTPRINT_WIZARD_TOOLS );
    m_toolManager->InitTools();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "common.InteractiveSelection" );

    // Create the toolbars
    m_toolbarSettings = GetToolbarSettings<FOOTPRINT_WIZARD_TOOLBAR_SETTINGS>( "fpwizard-toolbars" );
    configureToolbars();
    RecreateToolbars();

    // Create the parameters panel
    m_parametersPanel = new FOOTPRINT_WIZARD_PROPERTIES_PANEL( this, this );

    ReCreateParameterList();

    // Create the build message box
    m_buildMessageBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER );

    DisplayWizardInfos();

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6)
                      .BestSize( -1, m_msgFrameHeight ) );

    m_auimgr.AddPane( m_parametersPanel, EDA_PANE().Palette().Name( "Params" ).Left().Position(0)
                      .Caption( _( "Parameters" ) ).MinSize( 200, 320 ) );
    m_auimgr.AddPane( m_buildMessageBox, EDA_PANE().Palette().Name( "Output" ).Left().Position(1)
                      .CaptionVisible( false ).MinSize( 120, -1 ) );

    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );

    auto& galOpts = GetGalDisplayOptions();
    galOpts.SetCursorMode( KIGFX::CROSS_HAIR_MODE::FULLSCREEN_CROSS );
    galOpts.m_forceDisplayCursor = true;
    galOpts.m_axesEnabled = true;

    // Switch to the canvas type set in config
    resolveCanvasType();
    GetCanvas()->SwitchBackend( m_canvasType );
    ActivateGalCanvas();

    updateView();

    SetActiveLayer( F_Cu );
    GetToolManager()->PostAction( ACTIONS::zoomFitScreen );

    // Do not Run a dialog here: on some Window Managers, it creates issues.
    // Reason: the FOOTPRINT_WIZARD_FRAME is run as modal;
    // It means the call to FOOTPRINT_WIZARD_FRAME::ShowModal will change the
    // Event Loop Manager, and stop the one created by the dialog.
    // It does not happen on all W.M., perhaps due to the way the order events are called
    // See the call in onActivate instead
}


FOOTPRINT_WIZARD_FRAME::~FOOTPRINT_WIZARD_FRAME()
{
    GetCanvas()->StopDrawing();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

    // Be sure a active tool (if exists) is deactivated:
    if( m_toolManager )
        m_toolManager->DeactivateTool();

    EDA_3D_VIEWER_FRAME* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->Destroy();

    // Now this frame can be deleted
}


void FOOTPRINT_WIZARD_FRAME::doCloseWindow()
{
    SaveSettings( config() );

    if( IsModal() )
    {
        // Only dismiss a modal frame once, so that the return values set by
        // the prior DismissModal() are not bashed for ShowModal().
        if( !IsDismissed() )
            DismissModal( false );
    }
}


void FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint( wxCommandEvent& aEvent )
{
    DismissModal( true );
    Close();
}


void FOOTPRINT_WIZARD_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


COLOR_SETTINGS* FOOTPRINT_WIZARD_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetFootprintEditorSettings();
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


void FOOTPRINT_WIZARD_FRAME::updateView()
{
    GetCanvas()->UpdateColors();
    GetCanvas()->DisplayBoard( GetBoard() );
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    UpdateMsgPanel();
}


void FOOTPRINT_WIZARD_FRAME::UpdateMsgPanel()
{
    BOARD_ITEM* footprint = GetBoard()->GetFirstFootprint();

    if( footprint )
    {
        std::vector<MSG_PANEL_ITEM> items;

        footprint->GetMsgPanelInfo( this, items );
        SetMsgPanel( items );
    }
    else
    {
        ClearMsgPanel();
    }
}


void FOOTPRINT_WIZARD_FRAME::ReCreateParameterList()
{
    if( !m_parametersPanel )
        return;

    m_parametersPanel->RebuildParameters( GetMyWizard() );

    ReCreateHToolbar();
    DisplayWizardInfos();
    GetCanvas()->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    PCB_BASE_FRAME::LoadSettings( cfg );

    m_auiPerspective = cfg->m_FootprintViewer.perspective;
    m_viewerAuiState = std::make_unique<nlohmann::json>( cfg->m_FootprintViewer.aui_state );
}


void FOOTPRINT_WIZARD_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    PCB_BASE_FRAME::SaveSettings( cfg );

#if wxCHECK_VERSION( 3, 3, 0 )
    {
        WX_AUI_JSON_SERIALIZER serializer( m_auimgr );
        nlohmann::json state = serializer.Serialize();

        if( state.is_null() || state.empty() )
            cfg->m_FootprintViewer.aui_state = nlohmann::json();
        else
            cfg->m_FootprintViewer.aui_state = state;

        cfg->m_FootprintViewer.perspective.clear();
    }
#else
    cfg->m_FootprintViewer.perspective = m_auimgr.SavePerspective().ToStdString();
    cfg->m_FootprintViewer.aui_state = nlohmann::json();
#endif
}


WINDOW_SETTINGS* FOOTPRINT_WIZARD_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg ) )
        return &cfg->m_FootprintWizard;

    wxFAIL_MSG( wxT( "FOOTPRINT_CHOOSER not running with PCBNEW_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


void FOOTPRINT_WIZARD_FRAME::OnActivate( wxActivateEvent& event )
{
    // Ensure we do not have old selection:
    if( !event.GetActive() )
        return;

    if( !m_wizardListShown )
    {
        m_wizardListShown = true;
        wxPostEvent( this, wxCommandEvent( wxEVT_TOOL, ID_FOOTPRINT_WIZARD_SELECT_WIZARD ) );
    }

    // TODO(JE) re-evaluate below
#if 0
    // Currently, we do not have a way to see if a Python wizard has changed,
    // therefore the lists of parameters and option has to be rebuilt
    // This code could be enabled when this way exists
    bool footprintWizardsChanged = false;

    if( footprintWizardsChanged )
    {
        // If we are here, the library list has changed, rebuild it
        ReCreateParameterList();
        DisplayWizardInfos();
    }
#endif
}


void FOOTPRINT_WIZARD_FRAME::Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle )
{
    wxString wizardName = m_currentWizard ? m_currentWizard->Info().meta.name : _( "no wizard selected" );
    wxString frm3Dtitle;
    frm3Dtitle.Printf( _( "3D Viewer [%s]" ), wizardName );
    PCB_BASE_FRAME::Update3DView( aMarkDirty, aRefresh, &frm3Dtitle );
}


BOARD_ITEM_CONTAINER* FOOTPRINT_WIZARD_FRAME::GetModel() const
{
    return GetBoard()->GetFirstFootprint();
}
