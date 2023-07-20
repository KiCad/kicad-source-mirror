/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_preview_3d_model.h"
#include <dialogs/dialog_unit_entry.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/eda_3d_actions.h>
#include <tools/eda_3d_controller.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <common_ogl/ogl_attr_list.h>
#include <gal/dpi_scaling.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/wx_infobar.h>
#include <eda_3d_viewer_settings.h>
#include <board_design_settings.h>

PANEL_PREVIEW_3D_MODEL::PANEL_PREVIEW_3D_MODEL( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                                FOOTPRINT* aFootprint,
                                                std::vector<FP_3DMODEL>* aParentModelList ) :
        PANEL_PREVIEW_3D_MODEL_BASE( aParent, wxID_ANY ),
        m_parentFrame( aFrame ),
        m_previewPane( nullptr ),
        m_infobar( nullptr ),
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    m_userUnits = m_parentFrame->GetUserUnits();

    m_dummyBoard = new BOARD();

    m_dummyBoard->SetProject( &aFrame->Prj(), true );

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    m_bodyStyleShowAll = true;

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
    m_bpvTop->SetBitmap( KiBitmap( BITMAPS::axis3d_top ) );
    m_bpvFront->SetBitmap( KiBitmap( BITMAPS::axis3d_front ) );
    m_bpvBack->SetBitmap( KiBitmap( BITMAPS::axis3d_back ) );
    m_bpvLeft->SetBitmap( KiBitmap( BITMAPS::axis3d_left ) );
    m_bpvRight->SetBitmap( KiBitmap( BITMAPS::axis3d_right ) );
    m_bpvBottom->SetBitmap( KiBitmap( BITMAPS::axis3d_bottom ) );
    m_bpvISO->SetBitmap( KiBitmap( BITMAPS::ortho ) );
    m_bpvBodyStyle->SetBitmap( KiBitmap( BITMAPS::axis3d ) );
    m_bpUpdate->SetBitmap( KiBitmap( BITMAPS::reload ) );
    m_bpSettings->SetBitmap( KiBitmap( BITMAPS::options_3drender ) );

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

    m_parentModelList = aParentModelList;

    m_dummyFootprint = new FOOTPRINT( *aFootprint );
    m_dummyFootprint->SetParentGroup( nullptr );

    // Ensure the footprint is shown like in Fp editor: rot 0, not flipped
    // to avoid mistakes when setting the3D shape position/rotation
    if( m_dummyFootprint->IsFlipped() )
        m_dummyFootprint->Flip( m_dummyFootprint->GetPosition(), false );

    m_dummyFootprint->SetOrientation( ANGLE_0 );


    m_dummyBoard->Add( m_dummyFootprint );

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( this,
                                       OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                       m_boardAdapter, m_currentCamera,
                                       aFrame->Prj().Get3DCacheManager() );

    m_boardAdapter.SetBoard( m_dummyBoard );
    m_boardAdapter.m_IsBoardView = false;
    m_boardAdapter.m_IsPreviewer = true;   // Force display 3D models, regardless the 3D viewer options

    loadSettings();

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

    m_SizerPanelView->Add( m_infobar, 0, wxEXPAND, 0 );
    m_SizerPanelView->Add( m_previewPane, 1, wxEXPAND, 5 );

    for( wxEventType eventType : { wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT } )
    {
        Connect( eventType, wxMenuEventHandler( PANEL_PREVIEW_3D_MODEL::OnMenuEvent ), nullptr,
                 this );
    }

    aFrame->Connect( EDA_EVT_UNITS_CHANGED,
                     wxCommandEventHandler( PANEL_PREVIEW_3D_MODEL::onUnitsChanged ),
                     nullptr, this );

#ifdef __WXOSX__
    // Call layout once to get the proper button sizes after the bitmaps have been set
    Layout();

    // The rounded-button style used has a small border on the left/right sides.
    // This is automatically fixed in wx for buttons with a bitmap < 20, but not
    // when the bitmap is set to be 26x26.
    wxSize borderFix = wxSize( 4, 4 );

    m_bpvTop->SetMinSize( m_bpvTop->GetSize() + borderFix );
    m_bpvFront->SetMinSize( m_bpvFront->GetSize() + borderFix );
    m_bpvBack->SetMinSize( m_bpvBack->GetSize() + borderFix );
    m_bpvLeft->SetMinSize( m_bpvLeft->GetSize() + borderFix );
    m_bpvRight->SetMinSize( m_bpvRight->GetSize() + borderFix );
    m_bpvBottom->SetMinSize( m_bpvBottom->GetSize() + borderFix );
    m_bpvISO->SetMinSize( m_bpvISO->GetSize() + borderFix );
    m_bpUpdate->SetMinSize( m_bpUpdate->GetSize() + borderFix );
    m_bpSettings->SetMinSize( m_bpSettings->GetSize() + borderFix );
#endif
}


PANEL_PREVIEW_3D_MODEL::~PANEL_PREVIEW_3D_MODEL()
{
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

    const DPI_SCALING dpi{ settings, this };
    m_previewPane->SetScaleFactor( dpi.GetScaleFactor() );

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    if( colors )
    {
        auto set =
                [] ( const COLOR4D& aColor, SFVEC4F& aTarget )
                {
                    aTarget.r = aColor.r;
                    aTarget.g = aColor.g;
                    aTarget.b = aColor.b;
                    aTarget.a = aColor.a;
                };

        set( colors->GetColor( LAYER_3D_BACKGROUND_BOTTOM ), m_boardAdapter.m_BgColorBot );
        set( colors->GetColor( LAYER_3D_BACKGROUND_TOP ), m_boardAdapter.m_BgColorTop );
        set( colors->GetColor( LAYER_3D_BOARD ), m_boardAdapter.m_BoardBodyColor );
        set( colors->GetColor( LAYER_3D_COPPER ), m_boardAdapter.m_CopperColor );
        set( colors->GetColor( LAYER_3D_SILKSCREEN_BOTTOM ), m_boardAdapter.m_SilkScreenColorBot );
        set( colors->GetColor( LAYER_3D_SILKSCREEN_TOP ), m_boardAdapter.m_SilkScreenColorTop );
        set( colors->GetColor( LAYER_3D_SOLDERMASK_BOTTOM ), m_boardAdapter.m_SolderMaskColorBot );
        set( colors->GetColor( LAYER_3D_SOLDERMASK_TOP ), m_boardAdapter.m_SolderMaskColorTop );
        set( colors->GetColor( LAYER_3D_SOLDERPASTE ), m_boardAdapter.m_SolderPasteColor );
    }

    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    if( cfg )
    {
        // Save the 3D viewer render settings, to restore it after closing the preview
        m_initialRender = cfg->m_Render;

        m_boardAdapter.m_Cfg = cfg;

        m_previewPane->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_previewPane->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_previewPane->SetProjectionMode( cfg->m_Camera.projection_mode );

        // Ensure the board body is always shown, and do not use the settings of the 3D viewer
        cfg->m_Render.show_soldermask = m_bodyStyleShowAll;
        cfg->m_Render.show_solderpaste = m_bodyStyleShowAll;
        cfg->m_Render.show_zones = m_bodyStyleShowAll;
        cfg->m_Render.show_board_body = m_bodyStyleShowAll;
        cfg->m_Render.realistic = m_bodyStyleShowAll;
    }
}


/**
 * Ensure -MAX_ROTATION <= rotation <= MAX_ROTATION.
 *
 * @param \a aRotation will be normalized between -MAX_ROTATION and MAX_ROTATION.
 */
static double rotationFromString( const wxString& aValue )
{
    double rotation = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::DEGREES, aValue );

    if( rotation > MAX_ROTATION )
    {
        int n = KiROUND( rotation / MAX_ROTATION );
        rotation -= MAX_ROTATION * n;
    }
    else if( rotation < -MAX_ROTATION )
    {
        int n = KiROUND( -rotation / MAX_ROTATION );
        rotation += MAX_ROTATION * n;
    }

    return rotation;
}


wxString PANEL_PREVIEW_3D_MODEL::formatScaleValue( double aValue )
{
    return wxString::Format( wxT( "%.4f" ),
                             aValue );
}


wxString PANEL_PREVIEW_3D_MODEL::formatRotationValue( double aValue )
{
    return wxString::Format( wxT( "%.2f%s" ),
                             aValue,
                             EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
}


wxString PANEL_PREVIEW_3D_MODEL::formatOffsetValue( double aValue )
{
    // Convert from internal units (mm) to user units
    if( m_userUnits == EDA_UNITS::INCHES )
        aValue /= 25.4;
    else if( m_userUnits == EDA_UNITS::MILS )
        aValue /= 25.4 / 1e3;

    return wxString::Format( wxT( "%.6f%s" ),
                             aValue,
                             EDA_UNIT_UTILS::GetText( m_userUnits ) );
}


void PANEL_PREVIEW_3D_MODEL::SetSelectedModel( int idx )
{
    if( m_parentModelList && idx >= 0 && idx < (int) m_parentModelList->size() )
    {
        m_selected = idx;
        const FP_3DMODEL& modelInfo = m_parentModelList->at( (unsigned) m_selected );

        // Use ChangeValue() instead of SetValue().  It's not the user making the change, so we
        // don't want to generate wxEVT_GRID_CELL_CHANGED events.
        xscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.x ) );
        yscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.y ) );
        zscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.z ) );

        xrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.x ) );
        yrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.y ) );
        zrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.z ) );

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


void PANEL_PREVIEW_3D_MODEL::updateOrientation( wxCommandEvent &event )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        FP_3DMODEL* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Scale.x = EDA_UNIT_UTILS::UI::DoubleValueFromString(
                pcbIUScale, EDA_UNITS::UNSCALED, xscale->GetValue() );
        modelInfo->m_Scale.y = EDA_UNIT_UTILS::UI::DoubleValueFromString(
                pcbIUScale, EDA_UNITS::UNSCALED, yscale->GetValue() );
        modelInfo->m_Scale.z = EDA_UNIT_UTILS::UI::DoubleValueFromString(
                pcbIUScale, EDA_UNITS::UNSCALED, zscale->GetValue() );

        modelInfo->m_Rotation.x = rotationFromString( xrot->GetValue() );
        modelInfo->m_Rotation.y = rotationFromString( yrot->GetValue() );
        modelInfo->m_Rotation.z = rotationFromString( zrot->GetValue() );

        modelInfo->m_Offset.x = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           xoff->GetValue() )
                                / pcbIUScale.IU_PER_MM;
        modelInfo->m_Offset.y = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           yoff->GetValue() )
                                / pcbIUScale.IU_PER_MM;
        modelInfo->m_Offset.z = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                           zoff->GetValue() )
                                / pcbIUScale.IU_PER_MM;

        // Update the dummy footprint for the preview
        UpdateDummyFootprint( false );
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
    }
}


void PANEL_PREVIEW_3D_MODEL::setBodyStyleView( wxCommandEvent& event )
{
    // turn ON or OFF options to show the board body if OFF, soder paste, soldermask
    // and board body are hidden, to allows a good view of the 3D model and its pads.
    EDA_3D_VIEWER_SETTINGS* cfg = m_boardAdapter.m_Cfg;

    if( !cfg )
        return;

    m_bodyStyleShowAll = !m_bodyStyleShowAll;

    cfg->m_Render.show_soldermask = m_bodyStyleShowAll;
    cfg->m_Render.show_solderpaste = m_bodyStyleShowAll;
    cfg->m_Render.show_zones = m_bodyStyleShowAll;
    cfg->m_Render.show_board_body = m_bodyStyleShowAll;
    cfg->m_Render.realistic = m_bodyStyleShowAll;

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::View3DSettings( wxCommandEvent& event )
{
    BOARD_DESIGN_SETTINGS bds = m_dummyBoard->GetDesignSettings();
    int                   thickness = bds.GetBoardThickness();

    WX_UNIT_ENTRY_DIALOG dlg( m_parentFrame, _( "Board thickness:" ),
                              _( "3D Preview Options" ), thickness );

    if( dlg.ShowModal() != wxID_OK )
        return;

    bds.SetBoardThickness( (int) dlg.GetValue() );

    BOARD_STACKUP& boardStackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    boardStackup.RemoveAll();
    boardStackup.BuildDefaultStackupList( &bds, 2 );

    UpdateDummyFootprint( true );

    m_previewPane->ReloadRequest();
    m_previewPane->Refresh();
}


void PANEL_PREVIEW_3D_MODEL::doIncrementScale( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    double curr_value = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::UNSCALED,
                                                                   textCtrl->GetValue() );

    curr_value += ( SCALE_INCREMENT * aSign );
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementRotation( wxSpinEvent& aEvent, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) aEvent.GetEventObject();
    wxTextCtrl* textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    double curr_value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::DEGREES,
                                                                   textCtrl->GetValue() );

    curr_value += ( ROTATION_INCREMENT * aSign );
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::doIncrementOffset( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step_mm = OFFSET_INCREMENT_MM;
    double curr_value_mm =
            EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                       textCtrl->GetValue() )
            / pcbIUScale.IU_PER_MM;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCHES )
    {
        step_mm = 25.4*OFFSET_INCREMENT_MIL/1000;
    }

    curr_value_mm += ( step_mm * aSign );
    curr_value_mm = std::max( -MAX_OFFSET, curr_value_mm );
    curr_value_mm = std::min( curr_value_mm, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelScale( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = SCALE_INCREMENT;

    if( event.ShiftDown( ) )
        step = SCALE_INCREMENT_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::UNSCALED,
                                                                   textCtrl->GetValue() );

    curr_value += step;
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelRot( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = ROTATION_INCREMENT_WHEEL;

    if( event.ShiftDown( ) )
        step = ROTATION_INCREMENT_WHEEL_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, EDA_UNITS::DEGREES,
                                                                   textCtrl->GetValue() );

    curr_value += step;
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREVIEW_3D_MODEL::onMouseWheelOffset( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step_mm = OFFSET_INCREMENT_MM;

    if( event.ShiftDown( ) )
        step_mm = OFFSET_INCREMENT_MM_FINE;

    if( m_userUnits == EDA_UNITS::MILS || m_userUnits == EDA_UNITS::INCHES )
    {
        step_mm = 25.4*OFFSET_INCREMENT_MIL/1000.0;

        if( event.ShiftDown( ) )
            step_mm = 25.4*OFFSET_INCREMENT_MIL_FINE/1000.0;
    }

    if( event.GetWheelRotation() >= 0 )
        step_mm = -step_mm;

    double curr_value_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                      textCtrl->GetValue() )
                           / pcbIUScale.IU_PER_MM;

    curr_value_mm += step_mm;
    curr_value_mm = std::max( -MAX_OFFSET, curr_value_mm );
    curr_value_mm = std::min( curr_value_mm, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value_mm ) );
}


void PANEL_PREVIEW_3D_MODEL::onUnitsChanged( wxCommandEvent& aEvent )
{
    double xoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                xoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;
    double yoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                yoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;
    double zoff_mm = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, m_userUnits,
                                                                zoff->GetValue() )
                     / pcbIUScale.IU_PER_MM;

    PCB_BASE_FRAME* frame = static_cast<PCB_BASE_FRAME*>( aEvent.GetClientData() );
    m_userUnits = frame->GetUserUnits();

    xoff->SetValue( formatOffsetValue( xoff_mm ) );
    yoff->SetValue( formatOffsetValue( yoff_mm ) );
    zoff->SetValue( formatOffsetValue( zoff_mm ) );

    aEvent.Skip();
}


void PANEL_PREVIEW_3D_MODEL::UpdateDummyFootprint( bool aReloadRequired )
{
    m_dummyFootprint->Models().clear();

    for( FP_3DMODEL& model : *m_parentModelList )
    {
        if( model.m_Show )
            m_dummyFootprint->Models().push_back( model );
    }

    if( aReloadRequired )
        m_previewPane->ReloadRequest();

    m_previewPane->Request_refresh();
}
