/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <kiway.h>
#include <project.h>
#include <kicad_plugin.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <msgpanel.h>
#include <fp_lib_table.h>
#include <bitmaps.h>
#include <gal/graphics_abstraction_layer.h>
#include <eda_dockart.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <footprint_edit_frame.h>
#include <footprint_viewer_frame.h>
#include <wildcards_and_files_ext.h>
#include <pcb_layer_widget.h>
#include <invoke_pcb_dialog.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/zoom_tool.h>
#include <footprint_tree_pane.h>
#include <widgets/lib_tree.h>
#include <fp_lib_table.h>
#include <footprint_info_impl.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_modedit_settings.h>
#include <dialogs/panel_modedit_defaults.h>
#include <dialogs/panel_modedit_display_options.h>
#include <panel_hotkeys_editor.h>
#include <tools/position_relative_tool.h>
#include <widgets/progress_reporter.h>
#include "tools/selection_tool.h"
#include "tools/edit_tool.h"
#include "tools/drawing_tool.h"
#include "tools/point_editor.h"
#include "tools/pcbnew_control.h"
#include "tools/footprint_editor_tools.h"
#include "tools/placement_tool.h"
#include "tools/pcbnew_picker_tool.h"
#include "tools/pad_tool.h"
#include "tools/pcb_actions.h"


BEGIN_EVENT_TABLE( FOOTPRINT_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( FOOTPRINT_EDIT_FRAME::OnCloseWindow )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_EDIT_FRAME::CloseModuleEditor )
    EVT_MENU( wxID_EXIT, FOOTPRINT_EDIT_FRAME::OnExitKiCad )

    EVT_SIZE( FOOTPRINT_EDIT_FRAME::OnSize )

    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_EDIT_FRAME::OnSelectGrid )

    EVT_TOOL( ID_MODEDIT_SAVE_PNG, FOOTPRINT_EDIT_FRAME::OnSaveFootprintAsPng )

    EVT_TOOL( ID_MODEDIT_CHECK, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )
    EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, FOOTPRINT_EDIT_FRAME::LoadModuleFromBoard )
    EVT_TOOL( ID_ADD_FOOTPRINT_TO_BOARD, FOOTPRINT_EDIT_FRAME::Process_Special_Functions )

    // popup commands
    EVT_MENU( ID_GRID_SETTINGS, FOOTPRINT_EDIT_FRAME::OnGridSettings )

    // UI update events.
    EVT_UPDATE_UI( ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard )
    EVT_UPDATE_UI( ID_ADD_FOOTPRINT_TO_BOARD,
                   FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard )

    EVT_UPDATE_UI( ID_GEN_IMPORT_GRAPHICS_FILE, FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected )

END_EVENT_TABLE()

static const wxChar defaultLibWidthEntry[] =        wxT( "ModeditLibWidth" );

FOOTPRINT_EDIT_FRAME::FOOTPRINT_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                            EDA_DRAW_PANEL_GAL::GAL_TYPE aBackend ) :
    PCB_BASE_EDIT_FRAME( aKiway, aParent, FRAME_PCB_MODULE_EDITOR, wxEmptyString,
                         wxDefaultPosition, wxDefaultSize,
                         KICAD_DEFAULT_DRAWFRAME_STYLE, GetFootprintEditorFrameName() )
{
    m_showBorderAndTitleBlock = false;   // true to show the frame references
    m_FrameSize = ConvertDialogToPixels( wxSize( 500, 350 ) );    // default in case of no prefs
    m_canvasType = aBackend;
    m_AboutTitle = "ModEdit";

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_modedit_xpm ) );
    SetIcon( icon );

    // Create GAL canvas
    if( aBackend == EDA_DRAW_PANEL_GAL::GAL_TYPE_UNKNOWN )
        m_canvasType = LoadCanvasTypeSetting();
    else
        m_canvasType = aBackend;

    PCB_DRAW_PANEL_GAL* drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                            GetGalDisplayOptions(), m_canvasType );
    SetCanvas( drawPanel );

    SetBoard( new BOARD() );
    // In modedit, the default net clearance is not known.
    // (it depends on the actual board)
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance( 0 );

    // Don't show the default board solder mask clearance in the footprint editor.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    // restore the last footprint from the project, if any
    restoreLastFootprint();

    // Ensure all layers and items are visible:
    // In footprint editor, some layers have no meaning or
    // cannot be used, but we show all of them, at least to be able
    // to edit a bad layer
    GetBoard()->SetVisibleAlls();

    // However the "no net" mark on pads is useless, because there is
    // no net in footprint editor: make it non visible
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    m_Layers = new PCB_LAYER_WIDGET( this, GetCanvas(), true );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );
    GetGalDisplayOptions().m_axesEnabled = true;

    SetScreen( new PCB_SCREEN( GetPageSettings().GetSizeIU() ) );
    GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );

    GetScreen()->AddGrid( m_UserGridSize, EDA_UNITS_T::UNSCALED_UNITS, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );

    // In modedit, set the default paper size to A4:
    // this should be OK for all footprint to plot/print
    SetPageSettings( PAGE_INFO( PAGE_INFO::A4 ) );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    setupTools();

    initLibraryTree();
    m_treePane = new FOOTPRINT_TREE_PANE( this );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    m_Layers->ReFill();
    m_Layers->ReFillRender();

    GetScreen()->m_Active_Layer = F_SilkS;
    m_Layers->SelectLayer( F_SilkS );
    m_Layers->OnLayerSelected();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "Footprints" ).Left().Layer(1)
                      .Caption( _( "Libraries" ) ).MinSize( 250, 400 )
                      .BestSize( m_defaultLibWidth, -1 ) );

    m_auimgr.AddPane( m_drawToolBar, EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(1) );
    m_auimgr.AddPane( m_Layers, EDA_PANE().Palette().Name( "LayersManager" ).Right().Layer(3)
                      .Caption( _( "Layers Manager" ) ).PaneBorder( false )
                      .MinSize( 80, -1 ).BestSize( m_Layers->GetBestSize() ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    GetCanvas()->GetGAL()->SetGridSize( VECTOR2D( GetScreen()->GetGridSize() ) );
    GetCanvas()->GetView()->SetScale( GetZoomLevelCoeff() / GetScreen()->GetZoom() );
    ActivateGalCanvas();

    m_auimgr.Update();
    GetToolManager()->RunAction( ACTIONS::gridPreset, true, m_LastGridSizeId );
    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, false );
    updateTitle();
    InitExitKey();

    Raise();            // On some window managers, this is needed
    Show( true );
}


FOOTPRINT_EDIT_FRAME::~FOOTPRINT_EDIT_FRAME()
{
    // save the footprint in the PROJECT
    retainLastFootprint();

    delete m_Layers;
}



void FOOTPRINT_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas (Cairo / OpenGL).
    PCB_BASE_FRAME::SwitchCanvas( aCanvasType );

    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    GetCanvas()->GetGAL()->SetGridSize( VECTOR2D( GetScreen()->GetGridSize() ) );

    // The base class method *does not reinit* the layers manager. We must upate the layer
    // widget to match board visibility states, both layers and render columns, and and some
    // settings dependent on the canvas.
    UpdateUserInterface();
}


void FOOTPRINT_EDIT_FRAME::HardRedraw()
{
    SyncLibraryTree( true );
    GetCanvas()->ForceRefresh();
}


void FOOTPRINT_EDIT_FRAME::ToggleSearchTree()
{
    auto& treePane = m_auimgr.GetPane( m_treePane );
    treePane.Show( !IsSearchTreeShown() );
    m_auimgr.Update();
}


bool FOOTPRINT_EDIT_FRAME::IsSearchTreeShown()
{
    return m_auimgr.GetPane( m_treePane ).IsShown();
}


BOARD_ITEM_CONTAINER* FOOTPRINT_EDIT_FRAME::GetModel() const
{
    return GetBoard()->GetFirstModule();
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetTreeFPID() const
{
    return m_treePane->GetLibTree()->GetSelectedLibId();
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetTargetFPID() const
{
    LIB_ID id = GetTreeFPID();

    if( id.GetLibNickname().empty() )
        return GetLoadedFPID();

    return id;
}


LIB_ID FOOTPRINT_EDIT_FRAME::GetLoadedFPID() const
{
    MODULE* module = GetBoard()->GetFirstModule();

    if( module )
        return LIB_ID( module->GetFPID().GetLibNickname(), m_footprintNameWhenLoaded );
    else
        return LIB_ID();
}


bool FOOTPRINT_EDIT_FRAME::IsCurrentFPFromBoard() const
{
    MODULE* module = GetBoard()->GetFirstModule();

    return ( module && module->GetLink() > 0 );
}


void FOOTPRINT_EDIT_FRAME::retainLastFootprint()
{
    LIB_ID id = GetLoadedFPID();

    if( id.IsValid() )
    {
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_NICKNAME, id.GetLibNickname() );
        Prj().SetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FPNAME, id.GetLibItemName() );
    }
}


void FOOTPRINT_EDIT_FRAME::restoreLastFootprint()
{
    const wxString& curFootprintName = Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_FPNAME );
    const wxString& curNickname =  Prj().GetRString( PROJECT::PCB_FOOTPRINT_EDITOR_NICKNAME );

    if( curNickname.Length() && curFootprintName.Length() )
    {
        LIB_ID id;
        id.SetLibNickname( curNickname );
        id.SetLibItemName( curFootprintName );

        MODULE* module = loadFootprint( id );

        if( module )
            AddModuleToBoard( module );
    }
}


void FOOTPRINT_EDIT_FRAME::AddModuleToBoard( MODULE* aFootprint )
{
    m_revertModule.reset( (MODULE*) aFootprint->Clone() );

    m_footprintNameWhenLoaded = aFootprint->GetFPID().GetLibItemName();

    // Pads are always editable in Footprint Editor
    aFootprint->SetPadsLocked( false );

    PCB_BASE_EDIT_FRAME::AddModuleToBoard( aFootprint );
}


const wxChar* FOOTPRINT_EDIT_FRAME::GetFootprintEditorFrameName()
{
    return FOOTPRINT_EDIT_FRAME_NAME;
}


BOARD_DESIGN_SETTINGS& FOOTPRINT_EDIT_FRAME::GetDesignSettings() const
{
    return GetBoard()->GetDesignSettings();
}


void FOOTPRINT_EDIT_FRAME::SetDesignSettings( const BOARD_DESIGN_SETTINGS& aSettings )
{
    GetBoard()->SetDesignSettings( aSettings );
}


const PCB_PLOT_PARAMS& FOOTPRINT_EDIT_FRAME::GetPlotSettings() const
{
    // get the settings from the parent editor, not our BOARD.
    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) Kiway().Player( FRAME_PCB, true );
    wxASSERT( parentFrame );

    return parentFrame->GetPlotSettings();
}


void FOOTPRINT_EDIT_FRAME::SetPlotSettings( const PCB_PLOT_PARAMS& aSettings )
{
    // set the settings into parent editor, not our BOARD.
    PCB_BASE_FRAME* parentFrame = (PCB_BASE_FRAME*) Kiway().Player( FRAME_PCB, true );
    wxASSERT( parentFrame );

    parentFrame->SetPlotSettings( aSettings );
}


void FOOTPRINT_EDIT_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );
    wxConfigLoadSetups( aCfg, GetConfigurationSettings() );

    m_configSettings.Load( aCfg );  // mainly, load the color config

    // Ensure some params are valid
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    // Usually, graphic items are drawn on F_SilkS or F_Fab layer
    // Force these layers if not default
    if( ( settings.m_RefDefaultlayer != F_SilkS ) && ( settings.m_RefDefaultlayer != F_Fab ) )
        settings.m_RefDefaultlayer = F_SilkS;

    if( ( settings.m_ValueDefaultlayer != F_SilkS ) && ( settings.m_ValueDefaultlayer != F_Fab ) )
        settings.m_ValueDefaultlayer = F_Fab;

    aCfg->Read( defaultLibWidthEntry, &m_defaultLibWidth, 250 );
}


void FOOTPRINT_EDIT_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    m_configSettings.Save( aCfg );

    PCB_BASE_FRAME::SaveSettings( aCfg );
    wxConfigSaveSetups( aCfg, GetConfigurationSettings() );

    aCfg->Write( defaultLibWidthEntry, m_treePane->GetSize().x );
}


double FOOTPRINT_EDIT_FRAME::BestZoom()
{
    EDA_RECT    ibbbox  = GetBoardBoundingBox();

    double sizeX = (double) ibbbox.GetWidth();
    double sizeY = (double) ibbbox.GetHeight();

    wxPoint centre = ibbbox.Centre();

    // Reserve a 20% margin around "board" bounding box.
    double margin_scale_factor = 1.2;
    return bestZoom( sizeX, sizeY, margin_scale_factor, centre );
}


void FOOTPRINT_EDIT_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( GetScreen()->IsModify() && GetBoard()->GetFirstModule() )
    {
        wxString footprintName = GetBoard()->GetFirstModule()->GetFPID().GetLibItemName();
        wxString msg = _( "Save changes to \"%s\" before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, footprintName ),
                    [&]() -> bool { return SaveFootprint( GetBoard()->GetFirstModule() ); } ) )
        {
            Event.Veto();
            return;
        }
    }

    GetCanvas()->SetEventDispatcher( NULL );
    GetCanvas()->StopDrawing();

    // Do not show the layer manager during closing to avoid flicker
    // on some platforms (Windows) that generate useless redraw of items in
    // the Layer Manger
    m_auimgr.GetPane( "LayersManager" ).Show( false );

    Clear_Pcb( false );

    //close the editor
    Destroy();
}


void FOOTPRINT_EDIT_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void FOOTPRINT_EDIT_FRAME::CloseModuleEditor( wxCommandEvent& Event )
{
    Close();
}


void FOOTPRINT_EDIT_FRAME::OnUpdateModuleSelected( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->GetFirstModule() != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLoadModuleFromBoard( wxUpdateUIEvent& aEvent )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    aEvent.Enable( frame && frame->GetBoard()->GetFirstModule() != NULL );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateInsertModuleInBoard( wxUpdateUIEvent& aEvent )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    MODULE* module_in_edit = GetBoard()->GetFirstModule();
    bool canInsert = frame && module_in_edit && !module_in_edit->GetLink();

    // If the source was deleted, the module can inserted but not updated in the board.
    if( frame && module_in_edit && module_in_edit->GetLink() ) // this is not a new module
    {
        BOARD*  mainpcb = frame->GetBoard();
        canInsert = true;

        // search if the source module was not deleted:
        for( auto source_module : mainpcb->Modules() )
        {
            if( module_in_edit->GetLink() == source_module->GetTimeStamp() )
            {
                canInsert = false;
                break;
            }
        }
    }

    aEvent.Enable( canInsert );
}


void FOOTPRINT_EDIT_FRAME::ReFillLayerWidget()
{

    m_Layers->Freeze();
    m_Layers->ReFill();
    m_Layers->Thaw();

    wxAuiPaneInfo& lyrs = m_auimgr.GetPane( m_Layers );

    wxSize bestz = m_Layers->GetBestSize();

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_Layers->SetSize( bestz );
}


void FOOTPRINT_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    PCB_BASE_EDIT_FRAME::ShowChangedLanguage();

    // We have 2 panes to update.
    // For some obscure reason, the AUI manager hides the first modified pane.
    // So force show panes
    wxAuiPaneInfo& tree_pane_info = m_auimgr.GetPane( m_treePane );
    bool tree_shown = tree_pane_info.IsShown();
    tree_pane_info.Caption( _( "Libraries" ) );

    wxAuiPaneInfo& lm_pane_info = m_auimgr.GetPane( m_Layers );
    bool lm_shown = lm_pane_info.IsShown();
    lm_pane_info.Caption( _( "Layers Manager" ) );

    // update the layer manager
    m_Layers->SetLayersManagerTabsText();
    UpdateUserInterface();

    // Now restore the visibility:
    lm_pane_info.Show( lm_shown );
    tree_pane_info.Show( tree_shown );
    m_auimgr.Update();
}


void FOOTPRINT_EDIT_FRAME::OnModify()
{
    PCB_BASE_FRAME::OnModify();
    Update3DView( false );
    m_treePane->GetLibTree()->Refresh();
}


void FOOTPRINT_EDIT_FRAME::updateTitle()
{
    wxString title = _( "Footprint Editor" );
    LIB_ID   fpid = GetLoadedFPID();
    bool     writable = true;

    if( IsCurrentFPFromBoard() )
    {
        title += wxString::Format( wxT( " \u2014 %s [from %s.%s]" ),
                GetBoard()->GetFirstModule()->GetReference(), Prj().GetProjectName(),
                PcbFileExtension );
    }
    else if( fpid.IsValid() )
    {
        try
        {
            writable = Prj().PcbFootprintLibs()->IsFootprintLibWritable( fpid.GetLibNickname() );
        }
        catch( const IO_ERROR& )
        {
            // best efforts...
        }

        // Note: don't used GetLoadedFPID(); footprint name may have been edited
        title += wxString::Format( wxT( " \u2014 %s %s" ),
                FROM_UTF8( GetBoard()->GetFirstModule()->GetFPID().Format().c_str() ),
                writable ? wxString( wxEmptyString ) : _( "[Read Only]" ) );
    }
    else if( !fpid.GetLibItemName().empty() )
    {
        // Note: don't used GetLoadedFPID(); footprint name may have been edited
        title += wxString::Format( wxT( " \u2014 %s %s" ),
                FROM_UTF8( GetBoard()->GetFirstModule()->GetFPID().GetLibItemName().c_str() ),
                _( "[Unsaved]" ) );
    }

    SetTitle( title );
}


void FOOTPRINT_EDIT_FRAME::UpdateUserInterface()
{
    // Update the layer manager and other widgets from the board setup
    // (layer and items visibility, colors ...)

    // Update the layer manager
    m_Layers->Freeze();
    ReFillLayerWidget();
    m_Layers->ReFillRender();

    // update the layer widget to match board visibility states.
    m_Layers->SyncLayerVisibilities();
    GetCanvas()->SyncLayersVisibility( m_Pcb );
    m_Layers->SelectLayer( GetActiveLayer() );
    m_Layers->OnLayerSelected();

    m_Layers->Thaw();
}


void FOOTPRINT_EDIT_FRAME::updateView()
{
    GetCanvas()->UseColorScheme( &Settings().Colors() );
    GetCanvas()->DisplayBoard( GetBoard() );
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    updateTitle();
}


void FOOTPRINT_EDIT_FRAME::initLibraryTree()
{
    FP_LIB_TABLE*   fpTable = Prj().PcbFootprintLibs();

    WX_PROGRESS_REPORTER progressReporter( this, _( "Loading Footprint Libraries" ), 2 );
    GFootprintList.ReadFootprintFiles( fpTable, NULL, &progressReporter );
    progressReporter.Show( false );

    if( GFootprintList.GetErrorCount() )
        GFootprintList.DisplayErrors( this );

    m_adapter = FP_TREE_SYNCHRONIZING_ADAPTER::Create( this, fpTable );
    auto adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );

    adapter->AddLibraries();
}


void FOOTPRINT_EDIT_FRAME::SyncLibraryTree( bool aProgress )
{
    FP_LIB_TABLE* fpTable = Prj().PcbFootprintLibs();
    auto          adapter = static_cast<FP_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );
    LIB_ID        target = GetTargetFPID();
    bool          targetSelected = ( target == m_treePane->GetLibTree()->GetSelectedLibId() );

    // Sync FOOTPRINT_INFO list to the libraries on disk
    if( aProgress )
    {
        WX_PROGRESS_REPORTER progressReporter( this, _( "Updating Footprint Libraries" ), 2 );
        GFootprintList.ReadFootprintFiles( fpTable, NULL, &progressReporter );
        progressReporter.Show( false );
    }
    else
    {
        GFootprintList.ReadFootprintFiles( fpTable, NULL, NULL );
    }

    // Sync the LIB_TREE to the FOOTPRINT_INFO list
    adapter->Sync();

    m_treePane->GetLibTree()->Unselect();
    m_treePane->Regenerate();

    if( target.IsValid() )
    {
        if( adapter->FindItem( target ) )
        {
            if( targetSelected )
                m_treePane->GetLibTree()->SelectLibId( target );
            else
                m_treePane->GetLibTree()->CenterLibId( target );
        }
        else
        {
            // Try to focus on parent
            target.SetLibItemName( wxEmptyString );
            m_treePane->GetLibTree()->CenterLibId( target );
        }
    }
}


void FOOTPRINT_EDIT_FRAME::FocusOnLibID( const LIB_ID& aLibID )
{
    m_treePane->GetLibTree()->SelectLibId( aLibID );
}


bool FOOTPRINT_EDIT_FRAME::IsGridVisible() const
{
    return IsElementVisible( LAYER_GRID );
}


void FOOTPRINT_EDIT_FRAME::SetGridVisibility(bool aVisible)
{
    SetElementVisibility( LAYER_GRID, aVisible );
}


bool FOOTPRINT_EDIT_FRAME::IsElementVisible( GAL_LAYER_ID aElement ) const
{
    return GetBoard()->IsElementVisible( aElement );
}


void FOOTPRINT_EDIT_FRAME::SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState )
{
    GetCanvas()->GetView()->SetLayerVisible( aElement , aNewState );
    GetBoard()->SetElementVisibility( aElement, aNewState );
    m_Layers->SetRenderState( aElement, aNewState );
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLayerAlpha( wxUpdateUIEvent & )
{
    m_Layers->SyncLayerAlphaIndicators();
}


void FOOTPRINT_EDIT_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                               PANEL_HOTKEYS_EDITOR* aHotkeysPanel )
{
    wxTreebook* book = aParent->GetTreebook();

    book->AddPage( new PANEL_MODEDIT_SETTINGS( this, aParent ), _( "Footprint Editor" ) );
    book->AddSubPage( new PANEL_MODEDIT_DISPLAY_OPTIONS( this, aParent ), _( "Display Options" ) );
    book->AddSubPage( new PANEL_MODEDIT_DEFAULTS( this, aParent ), _( "Default Values" ) );

    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


void FOOTPRINT_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    GetCanvas()->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new SELECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new POINT_EDITOR );
    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new MODULE_EDITOR_TOOLS );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new PCBNEW_PICKER_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );

    m_toolManager->GetTool<PCBNEW_PICKER_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<PCBNEW_CONTROL>()->SetEditModules( true );
    m_toolManager->GetTool<PAD_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<SELECTION_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<EDIT_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<DRAWING_TOOL>()->SetEditModules( true );
    m_toolManager->GetTool<POINT_EDITOR>()->SetEditModules( true );

    m_toolManager->InitTools();

    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );
}


void FOOTPRINT_EDIT_FRAME::ActivateGalCanvas()
{
    PCB_BASE_EDIT_FRAME::ActivateGalCanvas();

    // Be sure the axis are enabled:
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    updateView();

    // Ensure the m_Layers settings are using the canvas type:
    UpdateUserInterface();
}


void FOOTPRINT_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    if( aEnvVarsChanged )
        SyncLibraryTree( true );

    Layout();
    SendSizeEvent();
}


void FOOTPRINT_EDIT_FRAME::OnSaveFootprintAsPng( wxCommandEvent& event )
{
    wxString   fullFileName;

    LIB_ID id = GetLoadedFPID();

    if( id.empty() )
    {
        wxMessageBox( _( "No footprint selected." ) );
        return;
    }

    wxFileName fn( id.GetLibItemName() );
    fn.SetExt( "png" );

    wxString projectPath = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Footprint Image File Name" ), projectPath,
                      fn.GetFullName(), PngFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetPath().IsEmpty() )
        return;

    // calling wxYield is mandatory under Linux, after closing the file selector dialog
    // to refresh the screen before creating the PNG or JPEG image from screen
    wxYield();
    SaveCanvasImageToFile( this, dlg.GetPath() );
}


