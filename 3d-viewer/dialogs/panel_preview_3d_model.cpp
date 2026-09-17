/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <3d_rendering/opengl/render_3d_opengl.h> // Must be included before any GL header

#include <cmath>

#include "panel_preview_3d_model.h"
#include <dialogs/dialog_unit_entry.h>
#include <libeval/numeric_evaluator.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/eda_3d_actions.h>
#include <tools/eda_3d_controller.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <common_ogl/ogl_attr_list.h>
#include <dpi_scaling_common.h>
#include <footprint.h>
#include <pad.h>
#include <3d_math.h>
#include <core/profile.h>
#include <geometry/shape_poly_set.h>
#include <widgets/text_ctrl_eval.h>
#include <libraries/library_manager.h>
#include <footprint_library_adapter.h>
#include <scoped_set_reset.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>
#include <cmath>
#include <limits>
#include <lset.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/wx_infobar.h>
#include <widgets/std_bitmap_button.h>
#include <eda_3d_viewer_settings.h>
#include <board_design_settings.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <3d_spacenav/spnav_viewer_plugin.h>
#else
#include <3d_navlib/nl_footprint_properties_plugin.h>
#endif

/// Duration and frame interval of the slide from the old placement to the aligned one.
static constexpr double ALIGN_ANIMATION_SECONDS = 1.0;
static constexpr int    ALIGN_ANIMATION_INTERVAL_MS = 16;


static wxString evaluateTextCtrl( const wxString& aValue )
{
    // NUMERIC_EVALUATOR doesn't handle UTF-8 multi-byte characters properly,
    // so skip evaluation if the string contains non-ASCII characters (e.g., degree symbols)
    for( wxUniChar c : aValue )
    {
        if( !c.IsAscii() )
            return aValue;
    }

    // Attempt to evaluate formula; if successful return result, otherwise return original
    NUMERIC_EVALUATOR eval( EDA_UNITS::UNSCALED );

    if( eval.Process( aValue ) )
        return eval.Result();

    return aValue;
}


/**
 * Normalize a rotation in degrees to the half-open range (-MAX_ROTATION, MAX_ROTATION].
 *
 * Matches the convention used by EDA_ANGLE::Normalize180(), so 198 maps to -162
 * and 540 maps to 180.
 */
static double normalizeRotation( double aRotation )
{
    double normalized = std::fmod( aRotation, 2.0 * MAX_ROTATION );

    if( normalized <= -MAX_ROTATION )
        normalized += 2.0 * MAX_ROTATION;
    else if( normalized > MAX_ROTATION )
        normalized -= 2.0 * MAX_ROTATION;

    if( normalized == -0.0 )
        normalized = 0.0;

    return normalized;
}


static double rotationFromString( const wxString& aValue )
{
    double rotation = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::DEGREES, aValue );

    return normalizeRotation( rotation );
}


PANEL_PREVIEW_3D_MODEL::PANEL_PREVIEW_3D_MODEL( wxWindow* aParent, PCB_BASE_FRAME* aFrame, FOOTPRINT* aFootprint,
                                                std::vector<FP_3DMODEL>* aParentModelList ) :
        PANEL_PREVIEW_3D_MODEL_BASE( aParent, PANEL_PREVIEW_3D_MODEL_ID ),
        m_parentFrame( aFrame ),
        m_previewPane( nullptr ),
        m_infobar( nullptr ),
        m_alignmentInfoBar( nullptr ),
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    m_userUnits = m_parentFrame->GetUserUnits();

    m_dummyBoard = new BOARD();

    m_dummyBoard->SetProject( &aFrame->Prj(), true );
    m_dummyBoard->SetEmbeddedFilesDelegate( aFrame->GetBoard() );

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    BOARD_DESIGN_SETTINGS parent_bds = aFrame->GetDesignSettings();
    BOARD_DESIGN_SETTINGS& dummy_bds = m_dummyBoard->GetDesignSettings();
    dummy_bds.SetBoardThickness( parent_bds.GetBoardThickness() );
    dummy_bds.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );
    BOARD_STACKUP& dummy_board_stackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    dummy_board_stackup.RemoveAll();
    dummy_board_stackup.BuildDefaultStackupList( &dummy_bds, 2 );

    m_selected = -1;

    m_previewLabel->SetFont( KIUI::GetStatusFont( this ) );

    // Set the bitmap of 3D view buttons:
    m_bpvTop->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_top ) );
    m_bpvFront->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_front ) );
    m_bpvBack->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_back ) );
    m_bpvLeft->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_left ) );
    m_bpvRight->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_right ) );
    m_bpvBottom->SetBitmap( KiBitmapBundle( BITMAPS::axis3d_bottom ) );
    m_bpvISO->SetBitmap( KiBitmapBundle( BITMAPS::ortho ) );
    m_bpvBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::show_board_body ) );
    m_bpUpdate->SetBitmap( KiBitmapBundle( BITMAPS::reload ) );
    m_bpSettings->SetBitmap( KiBitmapBundle( BITMAPS::options_3drender ) );

    // Set the min and max values of spin buttons (mandatory on Linux)
    // They are not used, so they are set to min and max 32 bits int values
    // (the min and max values supported by a wxSpinButton)
    // It avoids blocking the up or down arrows when reaching this limit after
    // a few clicks.
    wxSpinButton* spinButtonList[] =
    {
        m_spinXscale, m_spinYscale, m_spinZscale,
        m_spinXrot, m_spinYrot, m_spinZrot,
        m_spinXoffset,m_spinYoffset, m_spinZoffset
    };

    for( wxSpinButton* button : spinButtonList )
        button->SetRange(INT_MIN, INT_MAX );

    for( TEXT_CTRL_EVAL* rotCtrl : { xrot, yrot, zrot } )
    {
        rotCtrl->SetCustomEval(
                [&]( TEXT_CTRL_EVAL* aCtrl )
                {
                    double value = rotationFromString( evaluateTextCtrl( aCtrl->GetValue() ) );
                    aCtrl->SetValue( formatRotationValue( value ) );
                } );
    }

    for( TEXT_CTRL_EVAL* scaleCtrl : { xscale, yscale, zscale } )
    {
        scaleCtrl->SetCustomEval(
                [&]( TEXT_CTRL_EVAL* aCtrl )
                {
                    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                              evaluateTextCtrl( aCtrl->GetValue() ) );
                    aCtrl->SetValue( formatScaleValue( value ) );
                } );
    }

    m_parentModelList = aParentModelList;

    m_dummyFootprint = new FOOTPRINT( *aFootprint );
    m_dummyFootprint->SetParentGroup( nullptr );

    // Ensure the footprint is shown like in Fp editor: rot 0, not flipped
    // to avoid mistakes when setting the3D shape position/rotation
    if( m_dummyFootprint->IsFlipped() )
        m_dummyFootprint->Flip( m_dummyFootprint->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    m_dummyFootprint->SetOrientation( ANGLE_0 );

    m_dummyBoard->Add( m_dummyFootprint );

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                       m_boardAdapter, m_currentCamera,
                                       PROJECT_PCB::Get3DCacheManager( &aFrame->Prj() ) );

    try
    {
#if defined(__linux__) || defined(__FreeBSD__)
        m_spaceMouse = std::make_unique<SPNAV_VIEWER_PLUGIN>( m_previewPane );
#else
        m_spaceMouse = std::make_unique<NL_FOOTPRINT_PROPERTIES_PLUGIN>( m_previewPane );
#endif
        m_spaceMouse->SetFocus( true );
    }
    catch( const std::exception& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), wxS( "%s" ), e.what() );
    }
    catch( ... )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "Unknown exception during SpaceMouse initialization" ) );
    }

    m_boardAdapter.SetBoard( m_dummyBoard );
    m_boardAdapter.m_IsBoardView = false;

    // Force display 3D models, regardless the 3D viewer options.
    m_boardAdapter.m_IsPreviewer = true;

    loadSettings();

    // Don't show placeholder models in the footprint properties 3D preview
    if( m_boardAdapter.m_Cfg )
        m_boardAdapter.m_Cfg->m_Render.show_missing_models = false;

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_dummyBoard, nullptr, nullptr, nullptr, this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_previewPane->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    m_infobar = new WX_INFOBAR( this );
    m_previewPane->SetInfoBar( m_infobar );
    m_alignmentInfoBar = new WX_INFOBAR( this );

    m_SizerPanelView->Add( m_infobar, 0, wxEXPAND, 0 );
    m_SizerPanelView->Add( m_alignmentInfoBar, 0, wxEXPAND, 0 );
    m_SizerPanelView->Add( m_previewPane, 1, wxEXPAND, 5 );

    for( wxEventType eventType : { wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT } )
        Connect( eventType, wxMenuEventHandler( PANEL_PREVIEW_3D_MODEL::OnMenuEvent ), nullptr, this );

    aFrame->Connect( EDA_EVT_UNITS_CHANGED, wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL::onUnitsChanged ),
                     nullptr, this );

    Bind( wxCUSTOM_PANEL_SHOWN_EVENT, &PANEL_PREVIEW_3D_MODEL::onPanelShownEvent, this );
    Bind( wxEVT_CHAR_HOOK, &PANEL_PREVIEW_3D_MODEL::onAlignKey, this );
    m_previewPane->Bind( wxEVT_CHAR_HOOK, &PANEL_PREVIEW_3D_MODEL::onAlignKey, this );

    m_alignAnimTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &PANEL_PREVIEW_3D_MODEL::onAlignAnimation, this, m_alignAnimTimer.GetId() );
}


PANEL_PREVIEW_3D_MODEL::~PANEL_PREVIEW_3D_MODEL()
{
    m_alignAnimTimer.Stop();
    m_previewPane->SetPickHandler( {} );
    m_previewPane->SetHoverHandler( {} );
    restoreAlignmentView();

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    // Restore the 3D viewer Render settings, that can be modified by the panel tools
    if( m_boardAdapter.m_Cfg )
        m_boardAdapter.m_Cfg->m_Render = m_initialRender;

    delete m_dummyBoard;
    delete m_previewPane;
}


void PANEL_PREVIEW_3D_MODEL::OnMenuEvent( wxMenuEvent& aEvent )
{
    if( !m_toolDispatcher )
        aEvent.Skip();
    else
        m_toolDispatcher->DispatchWxEvent( aEvent );
}


void PANEL_PREVIEW_3D_MODEL::loadSettings()
{
    wxCHECK_RET( m_previewPane, wxT( "Cannot load settings to null canvas" ) );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        // Save the 3D viewer render settings, to restore it after closing the preview
        m_initialRender = cfg->m_Render;

        m_boardAdapter.m_Cfg = cfg;

        m_previewPane->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_previewPane->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_previewPane->SetProjectionMode( cfg->m_Camera.projection_mode );
    }
}


wxString PANEL_PREVIEW_3D_MODEL::formatScaleValue( double aValue )
{
    return wxString::Format( wxT( "%.4f" ),
                             aValue );
}


wxString PANEL_PREVIEW_3D_MODEL::formatRotationValue( double aValue )
{
    // Sigh.  Did we really need differentiated +/- 0.0?
    if( aValue == -0.0 )
        aValue = 0.0;

    return wxString::Format( wxT( "%.2f%s" ),
                             aValue,
                             EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
}


wxString PANEL_PREVIEW_3D_MODEL::formatOffsetValue( double aValue )
{
    // Convert from internal units (mm) to user units
    if( m_userUnits == EDA_UNITS::INCH )
        aValue /= 25.4;
    else if( m_userUnits == EDA_UNITS::MILS )
        aValue /= 25.4 / 1e3;

    return wxString::Format( wxT( "%.6f%s" ),
                             aValue,
                             EDA_UNIT_UTILS::GetText( m_userUnits ) );
}


bool PANEL_PREVIEW_3D_MODEL::canAlign() const
{
    if( m_extrudedBody || !m_parentModelList || m_selected < 0
        || m_selected >= (int) m_parentModelList->size() )
    {
        return false;
    }

    const FP_3DMODEL& model = m_parentModelList->at( m_selected );
    const std::string extension = wxFileName( model.m_Filename ).GetExt().utf8_string();

    return model.m_Show
           && compareFileExtensions( extension, { FILEEXT::StepFileExtension, FILEEXT::StepFileAbrvExtension,
                                                  FILEEXT::StepZFileAbrvExtension } )
           && std::isfinite( model.m_Scale.x ) && model.m_Scale.x != 0.0
           && std::isfinite( model.m_Scale.y ) && model.m_Scale.y != 0.0
           && std::isfinite( model.m_Scale.z ) && model.m_Scale.z != 0.0;
}


void PANEL_PREVIEW_3D_MODEL::onAlignUpdateUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( canAlign() );
}


const S3DMODEL* PANEL_PREVIEW_3D_MODEL::alignmentModel()
{
    if( !canAlign() )
        return nullptr;

    wxString basePath;

    try
    {
        auto row = PROJECT_PCB::FootprintLibAdapter( m_dummyBoard->GetProject() )
                           ->GetRow( m_dummyFootprint->GetFPID().GetLibNickname() );

        if( row )
            basePath = LIBRARY_MANAGER::GetFullURI( *row, true );
    }
    catch( const IO_ERROR& )
    {
        // Models may have absolute paths even when the footprint library is unavailable.
    }

    syncLocalEmbeddedFiles();
    return m_boardAdapter.Get3dCacheManager()->GetModel(
            m_parentModelList->at( m_selected ).m_Filename, basePath,
            { m_dummyFootprint->GetEmbeddedFiles(), m_dummyBoard->GetEmbeddedFiles() } );
}


void PANEL_PREVIEW_3D_MODEL::refreshAlignmentView()
{
    SCOPED_SET_RESET<bool> updating( m_alignUpdating, true );
    UpdateDummyFootprint( true );
}


void PANEL_PREVIEW_3D_MODEL::restoreAlignmentView()
{
    m_previewPane->JoinBgWorker();

    if( m_alignBodyShown )
    {
        if( EXTRUDED_3D_BODY* body = m_dummyFootprint->GetExtrudedBody() )
            body->m_show = *m_alignBodyShown;

        m_alignBodyShown.reset();
    }

    m_boardAdapter.SetVisibilityOverride( std::nullopt );
    m_alignLayers.reset();
}


void PANEL_PREVIEW_3D_MODEL::stopPicking()
{
    m_previewPane->SetPickHandler( {} );
    m_previewPane->SetHoverHandler( {} );
    restoreAlignmentView();
    m_alignButton->SetLabel( _( "Align" ) );
    Layout();
    refreshAlignmentView();
}


void PANEL_PREVIEW_3D_MODEL::dismissAlignmentInfoBar()
{
    if( !m_alignmentInfoBar->IsShown() )
        return;

    // Dismiss() does not hide the bar when its notebook page is already hidden.
    if( m_alignmentInfoBar->IsShownOnScreen() )
        m_alignmentInfoBar->Dismiss();
    else
        m_alignmentInfoBar->Hide();

    Layout();
}


void PANEL_PREVIEW_3D_MODEL::cancelAlignment()
{
    dismissAlignmentInfoBar();

    if( m_alignAnimating || !m_alignFaceTriangles.empty() )
    {
        stopAlignmentAnimation();
        m_alignFaceTriangles.clear();
        m_alignHoverRegion.reset();
        m_alignHoverPad = nullptr;
        updateAlignmentOverlays();
    }

    if( m_alignState == ALIGN_STATE::IDLE )
        return;

    const bool picking = m_alignState != ALIGN_STATE::SOLVED;
    m_alignState = ALIGN_STATE::IDLE;
    m_alignRegions.clear();
    m_alignTriangleRegions.clear();
    m_alignSolutions.clear();
    m_alignGeometry = nullptr;

    if( picking )
        stopPicking();
}


void PANEL_PREVIEW_3D_MODEL::onAlignKey( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE
        && ( m_alignState == ALIGN_STATE::PICK_MODEL || m_alignState == ALIGN_STATE::PICK_FOOTPRINT ) )
    {
        cancelAlignment();
        return;
    }

    aEvent.Skip();
}


void PANEL_PREVIEW_3D_MODEL::onAlign( wxCommandEvent& aEvent )
{
    if( m_alignState == ALIGN_STATE::PICK_MODEL || m_alignState == ALIGN_STATE::PICK_FOOTPRINT )
    {
        cancelAlignment();
        return;
    }

    if( !canAlign() )
        return;

    if( m_alignState == ALIGN_STATE::SOLVED && !m_alignSolutions.empty() )
    {
        m_alignSolution = ( m_alignSolution + 1 ) % m_alignSolutions.size();
        applyAlignment();
        return;
    }

    const S3DMODEL* geometry = alignmentModel();

    if( !geometry )
        return;

    const auto& scale = m_parentModelList->at( m_selected ).m_Scale;
    m_alignRegions = MODEL_ALIGN::BuildRegions( *geometry, glm::dvec3( scale.x, scale.y, scale.z ) );

    if( m_alignRegions.empty() )
        return;

    m_alignGeometry = geometry;
    m_alignTriangleRegions.clear();

    for( size_t i = 0; i < m_alignRegions.size(); ++i )
    {
        for( const std::array<unsigned int, 2>& source : m_alignRegions[i].sourceTriangles )
            m_alignTriangleRegions.emplace( source, i );
    }

    if( EXTRUDED_3D_BODY* body = m_dummyFootprint->GetExtrudedBody() )
    {
        m_alignBodyShown = body->m_show;
        body->m_show = false;
    }

    m_alignLayers = m_boardAdapter.GetVisibleLayers();
    auto layers = *m_alignLayers;

    for( int layer : { LAYER_3D_COPPER_TOP, LAYER_3D_COPPER_BOTTOM, LAYER_3D_PLATED_BARRELS, LAYER_3D_SOLDERPASTE,
                      LAYER_3D_SOLDERMASK_TOP, LAYER_3D_SOLDERMASK_BOTTOM, LAYER_3D_BOARD } )
    {
        layers.reset( layer );
    }

    m_boardAdapter.SetVisibilityOverride( layers );
    m_alignState = ALIGN_STATE::PICK_MODEL;
    m_alignButton->SetLabel( _( "Cancel alignment" ) );
    Layout();
    m_previewPane->SetPickHandler( [this]( const RAY& aRay ) { return pickAlignment( aRay ); } );
    m_previewPane->SetHoverHandler( [this]( const std::optional<RAY>& aRay ) { hoverAlignment( aRay ); } );
    m_alignmentInfoBar->ShowMessage( _( "Click the model face that sits on the board. Press Esc to cancel." ),
                                     wxICON_INFORMATION );
    refreshAlignmentView();
}


bool PANEL_PREVIEW_3D_MODEL::pickAlignment( const RAY& aRay )
{
    m_previewPane->JoinBgWorker();

    if( !canAlign() )
    {
        cancelAlignment();
        return true;
    }

    if( m_alignState == ALIGN_STATE::PICK_MODEL )
    {
        std::optional<size_t> region = alignmentRegionAt( aRay );

        if( !region )
            return true;

        m_alignSeed = *region;
        m_alignFaceTriangles = alignmentRegionTriangles( m_alignSeed );
        m_alignHoverRegion.reset();
        m_alignState = ALIGN_STATE::PICK_FOOTPRINT;
        auto layers = *m_alignLayers;
        layers.set( LAYER_3D_COPPER_TOP );
        layers.reset( LAYER_3D_COPPER_BOTTOM );
        layers.reset( LAYER_3D_SOLDERMASK_TOP );
        layers.reset( LAYER_3D_SOLDERPASTE );
        m_boardAdapter.SetVisibilityOverride( layers );
        m_alignmentInfoBar->ShowMessage( _( "Click the footprint pad for that contact. Press Esc to cancel." ),
                                         wxICON_INFORMATION );
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_TOP );
        refreshAlignmentView();
        updateAlignmentOverlays();
        return true;
    }

    if( m_alignState != ALIGN_STATE::PICK_FOOTPRINT )
        return false;

    PAD* clicked = m_previewPane->PickFootprintPad( aRay, *m_dummyFootprint );

    if( !clicked || ( clicked->GetAttribute() != PAD_ATTRIB::SMD
                     && clicked->GetAttribute() != PAD_ATTRIB::PTH ) )
    {
        return true;
    }

    std::vector<MODEL_ALIGN::PAD> pads;
    size_t clickedIndex = 0;
    const VECTOR2I origin = m_dummyFootprint->GetPosition();

    for( const PAD* pad : m_dummyFootprint->Pads() )
    {
        if( pad == clicked )
            clickedIndex = pads.size();

        const VECTOR2I pos = pad->GetPosition() - origin;
        const VECTOR2I size = pad->GetSize( F_Cu );
        const VECTOR2I drill = pad->GetDrillSize();
        MODEL_ALIGN::PAD item;
        item.number = pad->GetNumber().utf8_string();
        item.position = glm::dvec2( pos.x, -pos.y ) / pcbIUScale.IU_PER_MM;
        item.size = glm::dvec2( size.x, size.y ) / pcbIUScale.IU_PER_MM;
        item.drill = glm::dvec2( drill.x, drill.y ) / pcbIUScale.IU_PER_MM;
        item.rotation = pad->GetOrientationDegrees();
        item.attribute = pad->GetAttribute() == PAD_ATTRIB::SMD ? MODEL_ALIGN::PAD_ATTRIBUTE::SMD
                         : pad->GetAttribute() == PAD_ATTRIB::PTH ? MODEL_ALIGN::PAD_ATTRIBUTE::THROUGH_HOLE
                                                                : MODEL_ALIGN::PAD_ATTRIBUTE::OTHER;
        pads.push_back( std::move( item ) );
    }

    const auto& rotation = m_parentModelList->at( m_selected ).m_Rotation;
    m_alignSolutions = MODEL_ALIGN::SolveAlignment(
            m_alignRegions, m_alignSeed, MODEL_ALIGN::BuildPadGroup( pads, clickedIndex ), pads,
            glm::dvec3( -rotation.x, -rotation.y, -rotation.z ) );

    if( m_alignSolutions.empty() )
    {
        cancelAlignment();
        return true;
    }

    m_alignState = ALIGN_STATE::SOLVED;
    m_alignSolution = 0;
    m_alignHoverPad = nullptr;
    stopPicking();
    applyAlignment();

    if( m_alignSolutions.size() == 1 )
    {
        // Nothing to cycle through, so leave the session behind and keep the result.
        m_alignState = ALIGN_STATE::IDLE;
        m_alignRegions.clear();
        m_alignTriangleRegions.clear();
        m_alignSolutions.clear();
        m_alignGeometry = nullptr;
    }

    return true;
}


void PANEL_PREVIEW_3D_MODEL::applyAlignment()
{
    const MODEL_ALIGN::ALIGN_SOLUTION& solution = m_alignSolutions.at( m_alignSolution );
    const FP_3DMODEL&                  model = m_parentModelList->at( m_selected );
    const VECTOR3D                     fromRotation = model.m_Rotation;
    const VECTOR3D                     fromOffset = model.m_Offset;

    xrot->ChangeValue( formatRotationValue( normalizeRotation( solution.rotation.x ) ) );
    yrot->ChangeValue( formatRotationValue( normalizeRotation( solution.rotation.y ) ) );
    zrot->ChangeValue( formatRotationValue( normalizeRotation( solution.rotation.z ) ) );
    xoff->ChangeValue( formatOffsetValue( solution.offset.x ) );
    yoff->ChangeValue( formatOffsetValue( solution.offset.y ) );
    zoff->ChangeValue( formatOffsetValue( solution.offset.z ) );

    {
        SCOPED_SET_RESET<bool> updating( m_alignUpdating, true );
        wxCommandEvent         event;
        updateOrientation( event );
    }

    if( m_alignSeed < m_alignRegions.size() )
        m_alignFaceTriangles = alignmentRegionTriangles( m_alignSeed );

    startAlignmentAnimation( fromRotation, fromOffset );

    if( m_alignSolutions.size() > 1 )
    {
        m_alignmentInfoBar->ShowMessage( wxString::Format( _( "Solution %d of %d, press Align to cycle." ),
                                                          (int) m_alignSolution + 1,
                                                          (int) m_alignSolutions.size() ),
                                         wxICON_INFORMATION );
    }
    else
    {
        dismissAlignmentInfoBar();
    }
}


glm::mat4 PANEL_PREVIEW_3D_MODEL::alignmentModelMatrix() const
{
    const FP_3DMODEL& model = m_parentModelList->at( m_selected );
    const VECTOR3D&   rotation = m_alignAnimating ? m_alignAnimRotation : model.m_Rotation;
    const VECTOR3D&   offset = m_alignAnimating ? m_alignAnimOffset : model.m_Offset;

    // The region vertices already carry the model scale.
    return m_boardAdapter.GetFootprintMatrix( *m_dummyFootprint )
           * CalcModelMatrix( SFVEC3F( offset.x, offset.y, offset.z ),
                              SFVEC3F( rotation.x, rotation.y, rotation.z ), SFVEC3F( 1.0f ) );
}


std::vector<SFVEC3F> PANEL_PREVIEW_3D_MODEL::alignmentRegionTriangles( size_t aRegion ) const
{
    const MODEL_ALIGN::REGION& region = m_alignRegions.at( aRegion );
    std::vector<SFVEC3F>       triangles;

    triangles.reserve( region.triangles.size() * 3 );

    for( const std::array<unsigned int, 3>& triangle : region.triangles )
    {
        for( unsigned int index : triangle )
        {
            const glm::dvec3& vertex = region.vertices[index];
            triangles.emplace_back( vertex.x, vertex.y, vertex.z );
        }
    }

    return triangles;
}


std::vector<SFVEC3F> PANEL_PREVIEW_3D_MODEL::alignmentPadTriangles( const PAD& aPad ) const
{
    const PCB_LAYER_ID layer = m_dummyFootprint->IsFlipped() ? B_Cu : F_Cu;
    const float        units = m_boardAdapter.BiuTo3dUnits();
    const float        z = m_boardAdapter.GetFootprintZPos( m_dummyFootprint->IsFlipped() );
    SHAPE_POLY_SET       outline = *aPad.GetEffectivePolygon( layer, ERROR_INSIDE );
    std::vector<SFVEC3F> triangles;

    outline.CacheTriangulation( false );

    for( unsigned int i = 0; i < outline.TriangulatedPolyCount(); ++i )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* polygon = outline.TriangulatedPolygon( i );

        for( size_t j = 0; j < polygon->GetTriangleCount(); ++j )
        {
            VECTOR2I a;
            VECTOR2I b;
            VECTOR2I c;
            polygon->GetTriangle( (int) j, a, b, c );

            for( const VECTOR2I& point : { a, b, c } )
                triangles.emplace_back( point.x * units, -point.y * units, z );
        }
    }

    return triangles;
}


void PANEL_PREVIEW_3D_MODEL::updateAlignmentOverlays()
{
    std::vector<EDA_3D_CANVAS::OVERLAY> overlays;

    if( !m_parentModelList || m_selected < 0 || m_selected >= (int) m_parentModelList->size() )
    {
        m_previewPane->SetOverlays( {} );
        m_previewPane->Request_refresh();
        return;
    }

    if( !m_alignFaceTriangles.empty() )
    {
        EDA_3D_CANVAS::OVERLAY overlay;
        overlay.transform = alignmentModelMatrix();
        overlay.vertices = m_alignFaceTriangles;
        overlay.color = SFVEC4F( 0.05f, 0.85f, 0.25f, 0.35f );
        overlay.alwaysVisible = true;
        overlays.push_back( std::move( overlay ) );
    }

    if( m_alignHoverRegion && m_alignState == ALIGN_STATE::PICK_MODEL )
    {
        EDA_3D_CANVAS::OVERLAY overlay;
        overlay.transform = alignmentModelMatrix();
        overlay.vertices = alignmentRegionTriangles( *m_alignHoverRegion );
        overlay.color = SFVEC4F( 0.35f, 1.0f, 0.45f, 0.30f );
        overlays.push_back( std::move( overlay ) );
    }

    if( m_alignHoverPad && m_alignState == ALIGN_STATE::PICK_FOOTPRINT )
    {
        EDA_3D_CANVAS::OVERLAY overlay;
        overlay.vertices = alignmentPadTriangles( *m_alignHoverPad );
        overlay.color = SFVEC4F( 0.35f, 1.0f, 0.45f, 0.45f );
        overlay.alwaysVisible = true;
        overlays.push_back( std::move( overlay ) );
    }

    m_previewPane->SetOverlays( std::move( overlays ) );
    m_previewPane->Request_refresh();
}


std::optional<size_t> PANEL_PREVIEW_3D_MODEL::alignmentRegionAt( const RAY& aRay )
{
    if( !m_alignGeometry || !canAlign() )
        return std::nullopt;

    std::optional<EDA_3D_CANVAS::MODEL_HIT> hit =
            m_previewPane->PickModel( aRay, *m_alignGeometry, m_parentModelList->at( m_selected ),
                                      *m_dummyFootprint );

    if( !hit )
        return std::nullopt;

    auto region = m_alignTriangleRegions.find( { hit->mesh, hit->triangle / 3 } );

    if( region == m_alignTriangleRegions.end() )
        return std::nullopt;

    return region->second;
}


void PANEL_PREVIEW_3D_MODEL::hoverAlignment( const std::optional<RAY>& aRay )
{
    std::optional<size_t> region;
    const PAD*            pad = nullptr;

    if( aRay && canAlign() )
    {
        if( m_alignState == ALIGN_STATE::PICK_MODEL )
            region = alignmentRegionAt( *aRay );
        else if( m_alignState == ALIGN_STATE::PICK_FOOTPRINT )
            pad = m_previewPane->PickFootprintPad( *aRay, *m_dummyFootprint );
    }

    if( pad && pad->GetAttribute() != PAD_ATTRIB::SMD && pad->GetAttribute() != PAD_ATTRIB::PTH )
        pad = nullptr;

    if( region == m_alignHoverRegion && pad == m_alignHoverPad )
        return;

    m_alignHoverRegion = region;
    m_alignHoverPad = pad;
    updateAlignmentOverlays();
}


void PANEL_PREVIEW_3D_MODEL::startAlignmentAnimation( const VECTOR3D& aRotation, const VECTOR3D& aOffset )
{
    const FP_3DMODEL& model = m_parentModelList->at( m_selected );

    m_alignAnimFromRotation = aRotation;
    m_alignAnimFromOffset = aOffset;
    m_alignAnimRotation = aRotation;
    m_alignAnimOffset = aOffset;

    if( !m_previewPane->GetAnimationEnabled()
        || ( aRotation == model.m_Rotation && aOffset == model.m_Offset ) )
    {
        m_alignFaceTriangles.clear();
        updateAlignmentOverlays();
        return;
    }

    m_alignAnimating = true;
    m_alignAnimStart = GetRunningMicroSecs();
    m_alignAnimTimer.Start( ALIGN_ANIMATION_INTERVAL_MS, wxTIMER_CONTINUOUS );
    tickAlignmentAnimation();
}


void PANEL_PREVIEW_3D_MODEL::stopAlignmentAnimation()
{
    m_alignAnimTimer.Stop();
    m_alignAnimating = false;
}


void PANEL_PREVIEW_3D_MODEL::onAlignAnimation( wxTimerEvent& aEvent )
{
    tickAlignmentAnimation();
}


void PANEL_PREVIEW_3D_MODEL::tickAlignmentAnimation()
{
    if( !m_alignAnimating )
        return;

    const FP_3DMODEL& model = m_parentModelList->at( m_selected );
    const double      elapsed = ( GetRunningMicroSecs() - m_alignAnimStart ) / 1e6;
    const double      t = std::clamp( elapsed / ALIGN_ANIMATION_SECONDS, 0.0, 1.0 );
    const float       eased = (float) ( t * t * ( 3.0 - 2.0 * t ) );

    const SFVEC3F rotation = InterpolateModelRotation(
            SFVEC3F( m_alignAnimFromRotation.x, m_alignAnimFromRotation.y, m_alignAnimFromRotation.z ),
            SFVEC3F( model.m_Rotation.x, model.m_Rotation.y, model.m_Rotation.z ), eased );

    m_alignAnimRotation = VECTOR3D( rotation.x, rotation.y, rotation.z );
    m_alignAnimOffset = VECTOR3D( std::lerp( m_alignAnimFromOffset.x, model.m_Offset.x, (double) eased ),
                                  std::lerp( m_alignAnimFromOffset.y, model.m_Offset.y, (double) eased ),
                                  std::lerp( m_alignAnimFromOffset.z, model.m_Offset.z, (double) eased ) );

    if( t >= 1.0 )
    {
        // Land on the stored angles rather than on a re-decomposed equivalent of them.
        m_alignAnimRotation = model.m_Rotation;
        m_alignAnimOffset = model.m_Offset;
    }

    if( m_dummySelectedModel < m_dummyFootprint->Models().size() )
    {
        FP_3DMODEL& preview = m_dummyFootprint->Models()[m_dummySelectedModel];
        preview.m_Rotation = m_alignAnimRotation;
        preview.m_Offset = m_alignAnimOffset;
    }

    if( t >= 1.0 )
    {
        stopAlignmentAnimation();
        m_alignFaceTriangles.clear();
    }

    updateAlignmentOverlays();
}


void PANEL_PREVIEW_3D_MODEL::SetSelectedModel( int idx )
{
    cancelAlignment();

    if( m_parentModelList && idx >= 0 && idx < (int) m_parentModelList->size() )
    {
        m_selected = idx;
        const FP_3DMODEL& modelInfo = m_parentModelList->at( (unsigned) m_selected );

        // Use ChangeValue() instead of SetValue().  It's not the user making the change, so we
        // don't want to generate wxEVT_GRID_CELL_CHANGED events.
        xscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.x ) );
        yscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.y ) );
        zscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.z ) );

        // Rotation is stored in the file as positive-is-CW, but we use positive-is-CCW in the GUI
        // to match the rest of KiCad
        xrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.x ) );
        yrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.y ) );
        zrot->ChangeValue( formatRotationValue( -modelInfo.m_Rotation.z ) );

        xoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.x ) );
        yoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.y ) );
        zoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.z ) );

        m_opacity->SetValue( modelInfo.m_Opacity * 100.0 );
    }
    else
    {
        m_selected = -1;

        xscale->ChangeValue( wxEmptyString );
        yscale->ChangeValue( wxEmptyString );
        zscale->ChangeValue( wxEmptyString );

        xrot->ChangeValue( wxEmptyString );
        yrot->ChangeValue( wxEmptyString );
        zrot->ChangeValue( wxEmptyString );

        xoff->ChangeValue( wxEmptyString );
        yoff->ChangeValue( wxEmptyString );
        zoff->ChangeValue( wxEmptyString );

        m_opacity->SetValue( 100 );
    }
}


void PANEL_PREVIEW_3D_MODEL::SetExtrusionTransformMode( EXTRUDED_3D_BODY* aBody )
{
    cancelAlignment();
    m_extrudedBody = aBody;

    if( aBody )
    {
        xscale->ChangeValue( formatScaleValue( aBody->m_scale.x ) );
        yscale->ChangeValue( formatScaleValue( aBody->m_scale.y ) );
        zscale->ChangeValue( formatScaleValue( aBody->m_scale.z ) );

        xrot->ChangeValue( formatRotationValue( -aBody->m_rotation.x ) );
        yrot->ChangeValue( formatRotationValue( -aBody->m_rotation.y ) );
        zrot->ChangeValue( formatRotationValue( -aBody->m_rotation.z ) );

        xoff->ChangeValue( formatOffsetValue( aBody->m_offset.x ) );
        yoff->ChangeValue( formatOffsetValue( aBody->m_offset.y ) );
        zoff->ChangeValue( formatOffsetValue( aBody->m_offset.z ) );

        m_opacity->SetValue( 100 );
        m_opacity->Enable( false );
    }
    else
    {
        m_opacity->Enable( true );
    }
}


void PANEL_PREVIEW_3D_MODEL::updateOrientation( wxCommandEvent &event )
{
    if( m_extrudedBody )
    {
        m_extrudedBody->m_scale.x = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                               evaluateTextCtrl( xscale->GetValue() ) );
        m_extrudedBody->m_scale.y = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                               evaluateTextCtrl( yscale->GetValue() ) );
        m_extrudedBody->m_scale.z = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                               evaluateTextCtrl( zscale->GetValue() ) );

        m_extrudedBody->m_rotation.x = -rotationFromString( evaluateTextCtrl( xrot->GetValue() ) );
        m_extrudedBody->m_rotation.y = -rotationFromString( evaluateTextCtrl( yrot->GetValue() ) );
        m_extrudedBody->m_rotation.z = -rotationFromString( evaluateTextCtrl( zrot->GetValue() ) );

        m_extrudedBody->m_offset.x = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                                evaluateTextCtrl( xoff->GetValue() ) )
                                     / pcbIUScale.IU_PER_MM;
        m_extrudedBody->m_offset.y = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                                evaluateTextCtrl( yoff->GetValue() ) )
                                     / pcbIUScale.IU_PER_MM;
        m_extrudedBody->m_offset.z = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                                evaluateTextCtrl( zoff->GetValue() ) )
                                     / pcbIUScale.IU_PER_MM;

        UpdateDummyFootprint( true );
        onModify();
    }
    else if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );
        const VECTOR3D previousScale = modelInfo->m_Scale;
        const VECTOR3D previousRotation = modelInfo->m_Rotation;
        const VECTOR3D previousOffset = modelInfo->m_Offset;

        modelInfo->m_Scale.x = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                          evaluateTextCtrl( xscale->GetValue() ) );
        modelInfo->m_Scale.y = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                          evaluateTextCtrl( yscale->GetValue() ) );
        modelInfo->m_Scale.z = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED,
                                                                          evaluateTextCtrl( zscale->GetValue() ) );

        // Rotation is stored in the file as positive-is-CW, but we use positive-is-CCW in the GUI
        // to match the rest of KiCad
        modelInfo->m_Rotation.x = -rotationFromString( evaluateTextCtrl( xrot->GetValue() ) );
        modelInfo->m_Rotation.y = -rotationFromString( evaluateTextCtrl( yrot->GetValue() ) );
        modelInfo->m_Rotation.z = -rotationFromString( evaluateTextCtrl( zrot->GetValue() ) );

        modelInfo->m_Offset.x = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           evaluateTextCtrl( xoff->GetValue() ) )
                                / pcbIUScale.IU_PER_MM;
        modelInfo->m_Offset.y = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           evaluateTextCtrl( yoff->GetValue() ) )
                                / pcbIUScale.IU_PER_MM;
        modelInfo->m_Offset.z = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           evaluateTextCtrl( zoff->GetValue() ) )
                                / pcbIUScale.IU_PER_MM;

        if( previousScale == modelInfo->m_Scale && previousRotation == modelInfo->m_Rotation
            && previousOffset == modelInfo->m_Offset )
        {
            return;
        }

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
        onModify();
    }
}


void PANEL_PREVIEW_3D_MODEL::onOpacitySlider( wxCommandEvent& event )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Opacity = m_opacity->GetValue() / 100.0;

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
        onModify();
    }
}


void PANEL_PREVIEW_3D_MODEL::setBodyStyleView( wxCommandEvent& event )
{
    cancelAlignment();

    // turn ON or OFF options to show the board body if OFF, solder paste, soldermask
    // and board body are hidden, to allows a good view of the 3D model and its pads.
    EDA_3D_VIEWER_SETTINGS* cfg = m_boardAdapter.m_Cfg;

    if( !cfg )
        return;

    cfg->m_Render.preview_show_board_body = !cfg->m_Render.preview_show_board_body;

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::View3DSettings( wxCommandEvent& event )
{
    BOARD_DESIGN_SETTINGS bds = m_dummyBoard->GetDesignSettings();
    int                   thickness = bds.GetBoardThickness();

    WX_UNIT_ENTRY_DIALOG dlg( m_parentFrame, _( "3D Preview Options" ), _( "Board thickness:" ), thickness );

    if( dlg.ShowModal() != wxID_OK )
        return;

    bds.SetBoardThickness( dlg.GetValue() );

    BOARD_STACKUP& boardStackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    boardStackup.RemoveAll();
    boardStackup.BuildDefaultStackupList( &bds, 2 );

    UpdateDummyFootprint( true );

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::doIncrementScale( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = dynamic_cast<wxSpinButton*>( event.GetEventObject() );

    wxCHECK( spinCtrl, /* void */ );

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    double step = SCALE_INCREMENT;

    if( wxGetMouseState().ShiftDown( ) )
        step = SCALE_INCREMENT_FINE;

    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED, textCtrl->GetValue() );

    value += ( step * aSign );
    value = std::max( 1/MAX_SCALE, value );
    value = std::min( value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementRotation( wxSpinEvent& aEvent, double aSign )
{
    wxSpinButton* spinCtrl = dynamic_cast<wxSpinButton*>( aEvent.GetEventObject() );

    wxCHECK( spinCtrl, /* void */ );

    wxTextCtrl* textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    double step = ROTATION_INCREMENT;

    if( wxGetMouseState().ShiftDown( ) )
        step = ROTATION_INCREMENT_FINE;

    double value = rotationFromString( textCtrl->GetValue() );

    value = normalizeRotation( value + step * aSign );

    textCtrl->SetValue( formatRotationValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementOffset( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = dynamic_cast<wxSpinButton*>( event.GetEventObject() );

    wxCHECK( spinCtrl, /* void */ );

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step_mm = OFFSET_INCREMENT_MM;

    if( wxGetMouseState().ShiftDown( ) )
        step_mm = OFFSET_INCREMENT_MM_FINE;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCH )
    {
        step_mm = 25.4*OFFSET_INCREMENT_MIL/1000;

        if( wxGetMouseState().ShiftDown( ) )
            step_mm = 25.4*OFFSET_INCREMENT_MIL_FINE/1000;;
    }

    double value_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits, textCtrl->GetValue() )
                      / pcbIUScale.IU_PER_MM;

    value_mm += ( step_mm * aSign );
    value_mm = std::max( -MAX_OFFSET, value_mm );
    value_mm = std::min( value_mm, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelScale( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( event.GetEventObject() );

    wxCHECK( textCtrl, /* void */ );

    double step = SCALE_INCREMENT;

    if( event.ShiftDown( ) )
        step = SCALE_INCREMENT_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::UNSCALED, textCtrl->GetValue() );

    value += step;
    value = std::max( 1/MAX_SCALE, value );
    value = std::min( value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelRot( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( event.GetEventObject() );

    wxCHECK( textCtrl, /* void */ );

    double step = ROTATION_INCREMENT;

    if( event.ShiftDown( ) )
        step = ROTATION_INCREMENT_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double value = rotationFromString( textCtrl->GetValue() );

    value = normalizeRotation( value + step );

    textCtrl->SetValue( formatRotationValue( value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelOffset( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( event.GetEventObject() );

    wxCHECK( textCtrl, /* void */ );

    double step_mm = OFFSET_INCREMENT_MM;

    if( event.ShiftDown( ) )
        step_mm = OFFSET_INCREMENT_MM_FINE;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCH )
    {
        step_mm = 25.4*OFFSET_INCREMENT_MIL/1000.0;

        if( event.ShiftDown( ) )
            step_mm = 25.4*OFFSET_INCREMENT_MIL_FINE/1000.0;
    }

    if( event.GetWheelRotation() >= 0 )
        step_mm = -step_mm;

    double value_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits, textCtrl->GetValue() )
                      / pcbIUScale.IU_PER_MM;

    value_mm += step_mm;
    value_mm = std::max( -MAX_OFFSET, value_mm );
    value_mm = std::min( value_mm, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::onUnitsChanged( wxCommandEvent& aEvent )
{
    double xoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits, xoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;
    double yoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits, yoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;
    double zoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits, zoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;

    PCB_BASE_FRAME* frame = static_cast<PCB_BASE_FRAME*>( aEvent.GetClientData() );
    m_userUnits = frame->GetUserUnits();

    xoff->SetValue( formatOffsetValue( xoff_mm ) );
    yoff->SetValue( formatOffsetValue( yoff_mm ) );
    zoff->SetValue( formatOffsetValue( zoff_mm ) );

    aEvent.Skip();
}


void PANEL_PREVIEW_3D_MODEL::onPanelShownEvent( wxCommandEvent& aEvent )
{
    if( !aEvent.GetInt() )
        cancelAlignment();

    if( m_spaceMouse )
    {
        m_spaceMouse->SetFocus( static_cast<bool>( aEvent.GetInt() ) );
    }

    aEvent.Skip();
}


void PANEL_PREVIEW_3D_MODEL::UpdateDummyFootprint( bool aReloadRequired )
{
    // The background hit-test rebuild reads the same footprint and embedded files.
    m_previewPane->JoinBgWorker();

    if( !m_alignUpdating )
        cancelAlignment();

    m_dummyFootprint->Models().clear();
    m_dummySelectedModel = std::numeric_limits<size_t>::max();

    for( FP_3DMODEL& model : *m_parentModelList )
    {
        if( m_alignState == ALIGN_STATE::PICK_FOOTPRINT
            || ( m_alignState == ALIGN_STATE::PICK_MODEL
                 && &model != &m_parentModelList->at( m_selected ) ) )
        {
            continue;
        }

        if( model.m_Show )
        {
            if( m_selected >= 0 && &model == &m_parentModelList->at( m_selected ) )
                m_dummySelectedModel = m_dummyFootprint->Models().size();

            m_dummyFootprint->Models().push_back( model );
        }
    }

    syncLocalEmbeddedFiles();

    if( m_extrudedBody && !m_dummyFootprint->HasExtrudedBody() )
        m_extrudedBody = nullptr;

    if( aReloadRequired )
        m_previewPane->ReloadRequest();

    m_previewPane->Request_refresh();
}


void PANEL_PREVIEW_3D_MODEL::SetEmbeddedFilesDelegate( EMBEDDED_FILES* aDelegate )
{
    cancelAlignment();
    m_localEmbeddedFiles = aDelegate;
    syncLocalEmbeddedFiles();
}


void PANEL_PREVIEW_3D_MODEL::syncLocalEmbeddedFiles()
{
    m_previewPane->JoinBgWorker();
    m_dummyFootprint->ClearEmbeddedFiles();

    if( m_localEmbeddedFiles )
    {
        for( const auto& [name, file] : m_localEmbeddedFiles->EmbeddedFileMap() )
        {
            m_dummyFootprint->AddFile(
                    new EMBEDDED_FILES::EMBEDDED_FILE( *file ) );
        }
    }
}


void PANEL_PREVIEW_3D_MODEL::onModify()
{
    KIWAY_HOLDER* kiwayHolder = dynamic_cast<KIWAY_HOLDER*>( wxGetTopLevelParent( this ) );

    if( kiwayHolder && kiwayHolder->GetType() == KIWAY_HOLDER::DIALOG )
        static_cast<DIALOG_SHIM*>( kiwayHolder )->OnModify();
}
