/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_3D_display_options.h"
#include <3d_canvas/board_adapter.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <3d_viewer/tools/eda_3d_controller.h>
#include <tool/tool_manager.h>


PANEL_3D_DISPLAY_OPTIONS::PANEL_3D_DISPLAY_OPTIONS( EDA_3D_VIEWER_FRAME* aFrame, wxWindow* aParent ) :
        PANEL_3D_DISPLAY_OPTIONS_BASE( aParent ),
        m_frame( aFrame ),
        m_settings( aFrame->GetAdapter() ),
        m_canvas( aFrame->GetCanvas() )
{
}


void PANEL_3D_DISPLAY_OPTIONS::OnCheckEnableAnimation( wxCommandEvent& event )
{
    m_staticAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
    m_sliderAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
}


bool PANEL_3D_DISPLAY_OPTIONS::TransferDataToWindow()
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

    switch( m_settings.GetMaterialMode() )
    {
    default:
    case MATERIAL_MODE::NORMAL:       m_materialProperties->SetSelection( 0 ); break;
    case MATERIAL_MODE::DIFFUSE_ONLY: m_materialProperties->SetSelection( 1 ); break;
    case MATERIAL_MODE::CAD_MODE:     m_materialProperties->SetSelection( 2 ); break;
    }

    // Camera Options
    m_checkBoxEnableAnimation->SetValue( m_canvas->GetAnimationEnabled() );
    m_sliderAnimationSpeed->SetValue( m_canvas->GetMovingSpeedMultiplier() );
    m_staticAnimationSpeed->Enable( m_canvas->GetAnimationEnabled() );
    m_sliderAnimationSpeed->Enable( m_canvas->GetAnimationEnabled() );

    EDA_3D_CONTROLLER* ctrlTool = m_frame->GetToolManager()->GetTool<EDA_3D_CONTROLLER>();
    m_spinCtrlRotationAngle->SetValue( ctrlTool->GetRotationIncrement() );

    return true;
}


bool PANEL_3D_DISPLAY_OPTIONS::TransferDataFromWindow()
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

    switch( m_materialProperties->GetSelection() )
    {
    default:
    case 0: m_settings.SetMaterialMode( MATERIAL_MODE::NORMAL );       break;
    case 1: m_settings.SetMaterialMode( MATERIAL_MODE::DIFFUSE_ONLY ); break;
    case 2: m_settings.SetMaterialMode( MATERIAL_MODE::CAD_MODE );     break;
    }

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

    // Camera Options
    m_canvas->SetAnimationEnabled( m_checkBoxEnableAnimation->GetValue());
    m_canvas->SetMovingSpeedMultiplier( m_sliderAnimationSpeed->GetValue());

    EDA_3D_CONTROLLER* ctrlTool = m_frame->GetToolManager()->GetTool<EDA_3D_CONTROLLER>();
    ctrlTool->SetRotationIncrement( m_spinCtrlRotationAngle->GetValue() );

    // The 3D scene will be rebuilt by the caller

    return true;
}
