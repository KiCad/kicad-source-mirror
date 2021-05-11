/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_3D_view_option_base.h"
#include <3d_canvas/board_adapter.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <3d_viewer/tools/3d_controller.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>

class DIALOG_3D_VIEW_OPTIONS : public DIALOG_3D_VIEW_OPTIONS_BASE
{
public:
    explicit DIALOG_3D_VIEW_OPTIONS( EDA_3D_VIEWER* aParent );

    EDA_3D_VIEWER* GetParent()
    {
        return static_cast<EDA_3D_VIEWER*>( DIALOG_SHIM::GetParent() );
    }

    void OnCheckEnableAnimation( wxCommandEvent& WXUNUSED( event ) ) override;
    void OnLightsResetToDefaults( wxCommandEvent& event ) override;

    /// Automatically called when clicking on the OK button
    bool TransferDataFromWindow() override;

    /// Automatically called after creating the dialog
    bool TransferDataToWindow() override;

    void TransferColorDataToWindow();

private:
    BOARD_ADAPTER& m_settings;
    EDA_3D_CANVAS* m_canvas;
};


void EDA_3D_VIEWER::Install3DViewOptionDialog( wxCommandEvent& event )
{
    DIALOG_3D_VIEW_OPTIONS dlg( this );

    if( dlg.ShowModal() == wxID_OK )
    {
        NewDisplay( true );
    }
}


DIALOG_3D_VIEW_OPTIONS::DIALOG_3D_VIEW_OPTIONS( EDA_3D_VIEWER* aParent ) :
        DIALOG_3D_VIEW_OPTIONS_BASE( aParent ),
        m_settings( aParent->GetAdapter() ),
        m_canvas( aParent->GetCanvas() )
{
    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_3D_VIEW_OPTIONS::OnCheckEnableAnimation( wxCommandEvent& event )
{
    m_staticAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
    m_sliderAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
}


void DIALOG_3D_VIEW_OPTIONS::OnLightsResetToDefaults( wxCommandEvent& event )
{
    m_settings.m_RtCameraLightColor = SFVEC3F( 0.2f );
    m_settings.m_RtLightColorTop = SFVEC3F( 0.247f );
    m_settings.m_RtLightColorBottom = SFVEC3F( 0.247f );

    const std::vector<int> default_elevation =
    {
        67,  67,  67,  67, -67, -67, -67, -67,
    };

    const std::vector<int> default_azimuth =
    {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    for( size_t i = 0; i < m_settings.m_RtLightSphericalCoords.size(); ++i )
    {
        m_settings.m_RtLightColor[i] = SFVEC3F( 0.168f );

        m_settings.m_RtLightSphericalCoords[i].x =
                ( (float) default_elevation[i] + 90.0f ) / 180.0f;

        m_settings.m_RtLightSphericalCoords[i].y = (float) default_azimuth[i] / 180.0f;
    }

    TransferColorDataToWindow();
}


void DIALOG_3D_VIEW_OPTIONS::TransferColorDataToWindow()
{
    auto Transfer_color = [] ( const SFVEC3F& aSource, wxColourPickerCtrl *aTarget )
    {
        aTarget->SetColour( wxColour( aSource.r * 255, aSource.g * 255, aSource.b * 255, 255 ) );
    };

    Transfer_color( m_settings.m_RtCameraLightColor, m_colourPickerCameraLight );
    Transfer_color( m_settings.m_RtLightColorTop, m_colourPickerTopLight );
    Transfer_color( m_settings.m_RtLightColorBottom, m_colourPickerBottomLight );

    Transfer_color( m_settings.m_RtLightColor[0], m_colourPickerLight1 );
    Transfer_color( m_settings.m_RtLightColor[1], m_colourPickerLight2 );
    Transfer_color( m_settings.m_RtLightColor[2], m_colourPickerLight3 );
    Transfer_color( m_settings.m_RtLightColor[3], m_colourPickerLight4 );

    Transfer_color( m_settings.m_RtLightColor[4], m_colourPickerLight5 );
    Transfer_color( m_settings.m_RtLightColor[5], m_colourPickerLight6 );
    Transfer_color( m_settings.m_RtLightColor[6], m_colourPickerLight7 );
    Transfer_color( m_settings.m_RtLightColor[7], m_colourPickerLight8 );

    Transfer_color( m_settings.m_OpenGlSelectionColor, m_colourPickerSelection );

    m_spinCtrlLightElevation1->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[0].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation2->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[1].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation3->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[2].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation4->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[3].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation5->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[4].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation6->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[5].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation7->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[6].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation8->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[7].x * 180.0f - 90.0f ) );

    m_spinCtrlLightAzimuth1->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[0].y * 180.0f ) );
    m_spinCtrlLightAzimuth2->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[1].y * 180.0f ) );
    m_spinCtrlLightAzimuth3->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[2].y * 180.0f ) );
    m_spinCtrlLightAzimuth4->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[3].y * 180.0f ) );
    m_spinCtrlLightAzimuth5->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[4].y * 180.0f ) );
    m_spinCtrlLightAzimuth6->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[5].y * 180.0f ) );
    m_spinCtrlLightAzimuth7->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[6].y * 180.0f ) );
    m_spinCtrlLightAzimuth8->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[7].y * 180.0f ) );
}


bool DIALOG_3D_VIEW_OPTIONS::TransferDataToWindow()
{
    // Check/uncheck checkboxes
    m_checkBoxRealisticMode->SetValue( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );
    m_checkBoxBoardBody->SetValue( m_settings.GetFlag( FL_SHOW_BOARD_BODY ) );
    m_checkBoxAreas->SetValue( m_settings.GetFlag( FL_ZONE ) );

    m_checkBox3DshapesTH->SetValue( m_settings.GetFlag( FL_FP_ATTRIBUTES_NORMAL ) );
    m_checkBox3DshapesSMD->SetValue( m_settings.GetFlag( FL_FP_ATTRIBUTES_NORMAL_INSERT ) );
    m_checkBox3DshapesVirtual->SetValue( m_settings.GetFlag( FL_FP_ATTRIBUTES_VIRTUAL ) );

    m_checkBoxSilkscreen->SetValue( m_settings.GetFlag( FL_SILKSCREEN ) );
    m_checkBoxSolderMask->SetValue( m_settings.GetFlag( FL_SOLDERMASK ) );
    m_checkBoxSolderpaste->SetValue( m_settings.GetFlag( FL_SOLDERPASTE ) );
    m_checkBoxAdhesive->SetValue( m_settings.GetFlag( FL_ADHESIVE ) );
    m_checkBoxComments->SetValue( m_settings.GetFlag( FL_COMMENTS ) );
    m_checkBoxECO->SetValue( m_settings.GetFlag( FL_ECO ) );
    m_checkBoxSubtractMaskFromSilk->SetValue( m_settings.GetFlag( FL_SUBTRACT_MASK_FROM_SILK ) );
    m_checkBoxClipSilkOnViaAnnulus->SetValue( m_settings.GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) );
    m_checkBoxRenderPlatedPadsAsPlated->SetValue(
            m_settings.GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) );

    // OpenGL options
    m_checkBoxCuThickness->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) );
    m_checkBoxBoundingBoxes->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );
    m_checkBoxHighlightOnRollOver->SetValue( m_settings.GetFlag( FL_HIGHLIGHT_ROLLOVER_ITEM ) );
    m_checkBoxDisableAAMove->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveThickness->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveVias->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveHoles->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE ) );
    m_choiceAntiAliasing->SetSelection( static_cast<int>( m_settings.GetAntiAliasingMode() ) );

    // Raytracing options
    m_checkBoxRaytracing_renderShadows->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) );
    m_checkBoxRaytracing_addFloor->SetValue( m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR ) );
    m_checkBoxRaytracing_showRefractions->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS ) );
    m_checkBoxRaytracing_showReflections->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS ) );
    m_checkBoxRaytracing_postProcessing->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) );
    m_checkBoxRaytracing_antiAliasing->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) );
    m_checkBoxRaytracing_proceduralTextures->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) );

    m_spinCtrl_NrSamples_Shadows->SetValue( m_settings.m_RtShadowSampleCount );
    m_spinCtrl_NrSamples_Reflections->SetValue( m_settings.m_RtReflectionSampleCount );
    m_spinCtrl_NrSamples_Refractions->SetValue( m_settings.m_RtRefractionSampleCount );

    m_spinCtrlDouble_SpreadFactor_Shadows->SetValue( m_settings.m_RtSpreadShadows * 100.0f );
    m_spinCtrlDouble_SpreadFactor_Reflections->SetValue(
            m_settings.m_RtSpreadReflections * 100.0f );
    m_spinCtrlDouble_SpreadFactor_Refractions->SetValue(
            m_settings.m_RtSpreadRefractions * 100.0f );

    m_spinCtrlRecursiveLevel_Reflections->SetValue( m_settings.m_RtRecursiveReflectionCount );
    m_spinCtrlRecursiveLevel_Refractions->SetValue( m_settings.m_RtRecursiveRefractionCount );

    TransferColorDataToWindow();

    // Camera Options
    m_checkBoxEnableAnimation->SetValue( m_canvas->AnimationEnabledGet() );
    m_sliderAnimationSpeed->SetValue( m_canvas->MovingSpeedMultiplierGet() );
    m_staticAnimationSpeed->Enable( m_canvas->AnimationEnabledGet() );
    m_sliderAnimationSpeed->Enable( m_canvas->AnimationEnabledGet() );

    EDA_3D_CONTROLLER* ctrlTool = GetParent()->GetToolManager()->GetTool<EDA_3D_CONTROLLER>();
    m_spinCtrlRotationAngle->SetValue( ctrlTool->GetRotationIncrement() );

    return true;
}


bool DIALOG_3D_VIEW_OPTIONS::TransferDataFromWindow()
{
    // Set render mode
    m_settings.SetFlag( FL_USE_REALISTIC_MODE, m_checkBoxRealisticMode->GetValue() );

    // Set visibility of items
    m_settings.SetFlag( FL_SHOW_BOARD_BODY, m_checkBoxBoardBody->GetValue() );
    m_settings.SetFlag( FL_ZONE, m_checkBoxAreas->GetValue() );
    m_settings.SetFlag( FL_SUBTRACT_MASK_FROM_SILK, m_checkBoxSubtractMaskFromSilk->GetValue() );
    m_settings.SetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS, m_checkBoxClipSilkOnViaAnnulus->GetValue() );
    m_settings.SetFlag( FL_RENDER_PLATED_PADS_AS_PLATED,
                        m_checkBoxRenderPlatedPadsAsPlated->GetValue() );

    // Set 3D shapes visibility
    m_settings.SetFlag( FL_FP_ATTRIBUTES_NORMAL, m_checkBox3DshapesTH->GetValue() );
    m_settings.SetFlag( FL_FP_ATTRIBUTES_NORMAL_INSERT, m_checkBox3DshapesSMD->GetValue() );
    m_settings.SetFlag( FL_FP_ATTRIBUTES_VIRTUAL, m_checkBox3DshapesVirtual->GetValue() );

    // Set Layer visibility
    m_settings.SetFlag( FL_SILKSCREEN, m_checkBoxSilkscreen->GetValue() );
    m_settings.SetFlag( FL_SOLDERMASK, m_checkBoxSolderMask->GetValue() );
    m_settings.SetFlag( FL_SOLDERPASTE, m_checkBoxSolderpaste->GetValue() );
    m_settings.SetFlag( FL_ADHESIVE, m_checkBoxAdhesive->GetValue() );
    m_settings.SetFlag( FL_COMMENTS, m_checkBoxComments->GetValue() );
    m_settings.SetFlag( FL_ECO, m_checkBoxECO->GetValue( ) );

    // OpenGL options
    m_settings.SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, m_checkBoxCuThickness->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX, m_checkBoxBoundingBoxes->GetValue() );
    m_settings.SetFlag( FL_HIGHLIGHT_ROLLOVER_ITEM, m_checkBoxHighlightOnRollOver->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE, m_checkBoxDisableAAMove->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveThickness->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveVias->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveHoles->GetValue() );
    m_settings.SetAntiAliasingMode(
            static_cast<ANTIALIASING_MODE>( m_choiceAntiAliasing->GetSelection() ) );

    // Raytracing options
    m_settings.SetFlag( FL_RENDER_RAYTRACING_SHADOWS,
                        m_checkBoxRaytracing_renderShadows->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_BACKFLOOR,
                        m_checkBoxRaytracing_addFloor->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFRACTIONS,
                        m_checkBoxRaytracing_showRefractions->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFLECTIONS,
                        m_checkBoxRaytracing_showReflections->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING,
                        m_checkBoxRaytracing_postProcessing->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING,
                        m_checkBoxRaytracing_antiAliasing->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES,
                        m_checkBoxRaytracing_proceduralTextures->GetValue() );

    m_settings.m_RtShadowSampleCount = m_spinCtrl_NrSamples_Shadows->GetValue();
    m_settings.m_RtReflectionSampleCount = m_spinCtrl_NrSamples_Reflections->GetValue();
    m_settings.m_RtRefractionSampleCount= m_spinCtrl_NrSamples_Refractions->GetValue();

    m_settings.m_RtSpreadShadows =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Shadows->GetValue() ) / 100.0f;
    m_settings.m_RtSpreadReflections =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Reflections->GetValue() ) / 100.0f;
    m_settings.m_RtSpreadRefractions =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Refractions->GetValue() ) / 100.0f;

    m_settings.m_RtRecursiveReflectionCount = m_spinCtrlRecursiveLevel_Reflections->GetValue();
    m_settings.m_RtRecursiveRefractionCount = m_spinCtrlRecursiveLevel_Refractions->GetValue();

    auto Transfer_color = [] ( SFVEC3F& aTarget, wxColourPickerCtrl *aSource )
    {
        const wxColour color = aSource->GetColour();

        aTarget = SFVEC3F( color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f );
    };

    Transfer_color( m_settings.m_RtCameraLightColor, m_colourPickerCameraLight );
    Transfer_color( m_settings.m_RtLightColorTop, m_colourPickerTopLight );
    Transfer_color( m_settings.m_RtLightColorBottom, m_colourPickerBottomLight );

    Transfer_color( m_settings.m_RtLightColor[0], m_colourPickerLight1 );
    Transfer_color( m_settings.m_RtLightColor[1], m_colourPickerLight2 );
    Transfer_color( m_settings.m_RtLightColor[2], m_colourPickerLight3 );
    Transfer_color( m_settings.m_RtLightColor[3], m_colourPickerLight4 );
    Transfer_color( m_settings.m_RtLightColor[4], m_colourPickerLight5 );
    Transfer_color( m_settings.m_RtLightColor[5], m_colourPickerLight6 );
    Transfer_color( m_settings.m_RtLightColor[6], m_colourPickerLight7 );
    Transfer_color( m_settings.m_RtLightColor[7], m_colourPickerLight8 );

    Transfer_color( m_settings.m_OpenGlSelectionColor, m_colourPickerSelection );

    m_settings.m_RtLightSphericalCoords[0].x =
            ( m_spinCtrlLightElevation1->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[1].x =
            ( m_spinCtrlLightElevation2->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[2].x =
            ( m_spinCtrlLightElevation3->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[3].x =
            ( m_spinCtrlLightElevation4->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[4].x =
            ( m_spinCtrlLightElevation5->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[5].x =
            ( m_spinCtrlLightElevation6->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[6].x =
            ( m_spinCtrlLightElevation7->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[7].x =
            ( m_spinCtrlLightElevation8->GetValue() + 90.0f ) / 180.0f;

    m_settings.m_RtLightSphericalCoords[0].y = m_spinCtrlLightAzimuth1->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[1].y = m_spinCtrlLightAzimuth2->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[2].y = m_spinCtrlLightAzimuth3->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[3].y = m_spinCtrlLightAzimuth4->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[4].y = m_spinCtrlLightAzimuth5->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[5].y = m_spinCtrlLightAzimuth6->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[6].y = m_spinCtrlLightAzimuth7->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[7].y = m_spinCtrlLightAzimuth8->GetValue() / 180.0f;

    for( size_t i = 0; i < m_settings.m_RtLightSphericalCoords.size(); ++i )
    {
        m_settings.m_RtLightSphericalCoords[i].x =
                glm::clamp( m_settings.m_RtLightSphericalCoords[i].x, 0.0f, 1.0f );

        m_settings.m_RtLightSphericalCoords[i].y =
                glm::clamp( m_settings.m_RtLightSphericalCoords[i].y, 0.0f, 2.0f );
    }

    // Camera Options
    m_canvas->AnimationEnabledSet( m_checkBoxEnableAnimation->GetValue() );
    m_canvas->MovingSpeedMultiplierSet( m_sliderAnimationSpeed->GetValue() );

    EDA_3D_CONTROLLER* ctrlTool = GetParent()->GetToolManager()->GetTool<EDA_3D_CONTROLLER>();
    ctrlTool->SetRotationIncrement( m_spinCtrlRotationAngle->GetValue() );

    return true;
}
