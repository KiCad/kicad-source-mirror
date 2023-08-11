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

#include <3d_enums.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eda_3d_viewer_settings.h>
#include "panel_3D_display_options.h"


PANEL_3D_DISPLAY_OPTIONS::PANEL_3D_DISPLAY_OPTIONS( wxWindow* aParent ) :
        PANEL_3D_DISPLAY_OPTIONS_BASE( aParent )
{
}


void PANEL_3D_DISPLAY_OPTIONS::OnCheckEnableAnimation( wxCommandEvent& event )
{
    m_staticAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
    m_sliderAnimationSpeed->Enable( m_checkBoxEnableAnimation->GetValue() );
}


void PANEL_3D_DISPLAY_OPTIONS::loadViewSettings( EDA_3D_VIEWER_SETTINGS* aCfg )
{
    // Check/uncheck checkboxes
    m_checkBoxRealisticMode->SetValue( aCfg->m_Render.realistic );
    m_checkBoxBoardBody->SetValue( aCfg->m_Render.show_board_body );
    m_checkBoxAreas->SetValue( aCfg->m_Render.show_zones );

    if( aCfg->m_Render.show_silkscreen_top == aCfg->m_Render.show_silkscreen_bottom )
        m_checkBoxSilkscreen->SetValue( aCfg->m_Render.show_silkscreen_top );
    else
        m_checkBoxSilkscreen->Set3StateValue( wxCHK_UNDETERMINED );

    if( aCfg->m_Render.show_soldermask_top == aCfg->m_Render.show_soldermask_bottom )
        m_checkBoxSolderMask->SetValue( aCfg->m_Render.show_soldermask_top );
    else
        m_checkBoxSolderMask->Set3StateValue( wxCHK_UNDETERMINED );

    m_checkBoxSolderpaste->SetValue( aCfg->m_Render.show_solderpaste );
    m_checkBoxAdhesive->SetValue( aCfg->m_Render.show_adhesive );

    if( aCfg->m_Render.show_comments == aCfg->m_Render.show_drawings )
        m_checkBoxComments->SetValue( aCfg->m_Render.show_comments );
    else
        m_checkBoxComments->Set3StateValue( wxCHK_UNDETERMINED );

    if( aCfg->m_Render.show_eco1 == aCfg->m_Render.show_eco2 )
        m_checkBoxECO->SetValue( aCfg->m_Render.show_eco1 );
    else
        m_checkBoxECO->Set3StateValue( wxCHK_UNDETERMINED );

    m_checkBoxSubtractMaskFromSilk->SetValue( aCfg->m_Render.subtract_mask_from_silk );
    m_checkBoxClipSilkOnViaAnnulus->SetValue( aCfg->m_Render.clip_silk_on_via_annulus );
    m_checkBoxRenderPlatedPadsAsPlated->SetValue( aCfg->m_Render.renderPlatedPadsAsPlated );

    m_materialProperties->SetSelection( static_cast<int>( aCfg->m_Render.material_mode ) );

    // Camera Options
    m_checkBoxEnableAnimation->SetValue( aCfg->m_Camera.animation_enabled );
    m_sliderAnimationSpeed->SetValue( aCfg->m_Camera.moving_speed_multiplier );
    m_staticAnimationSpeed->Enable( aCfg->m_Camera.animation_enabled );
    m_sliderAnimationSpeed->Enable( aCfg->m_Camera.animation_enabled );
    m_spinCtrlRotationAngle->SetValue( aCfg->m_Camera.rotation_increment );
}


bool PANEL_3D_DISPLAY_OPTIONS::TransferDataToWindow()
{
    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    loadViewSettings( cfg );

    return true;
}


bool PANEL_3D_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    // Set render mode
    cfg->m_Render.realistic = m_checkBoxRealisticMode->GetValue();

    // Set visibility of items
    cfg->m_Render.show_board_body = m_checkBoxBoardBody->GetValue();
    cfg->m_Render.show_zones = m_checkBoxAreas->GetValue();
    cfg->m_Render.subtract_mask_from_silk = m_checkBoxSubtractMaskFromSilk->GetValue();
    cfg->m_Render.clip_silk_on_via_annulus = m_checkBoxClipSilkOnViaAnnulus->GetValue();
    cfg->m_Render.renderPlatedPadsAsPlated = m_checkBoxRenderPlatedPadsAsPlated->GetValue();

    cfg->m_Render.material_mode = static_cast<MATERIAL_MODE>( m_materialProperties->GetSelection() );

    // Set Layer visibility
    if( m_checkBoxSilkscreen->Get3StateValue() != wxCHK_UNDETERMINED )
    {
        cfg->m_Render.show_silkscreen_top = m_checkBoxSilkscreen->GetValue();
        cfg->m_Render.show_silkscreen_bottom = m_checkBoxSilkscreen->GetValue();
    }

    if( m_checkBoxSolderMask->Get3StateValue() != wxCHK_UNDETERMINED )
    {
        cfg->m_Render.show_soldermask_top = m_checkBoxSolderMask->GetValue();
        cfg->m_Render.show_soldermask_bottom = m_checkBoxSolderMask->GetValue();
    }

    cfg->m_Render.show_solderpaste = m_checkBoxSolderpaste->GetValue();
    cfg->m_Render.show_adhesive = m_checkBoxAdhesive->GetValue();

    if( m_checkBoxComments->Get3StateValue() != wxCHK_UNDETERMINED )
    {
        cfg->m_Render.show_comments = m_checkBoxComments->GetValue();
        cfg->m_Render.show_drawings = m_checkBoxComments->GetValue();
    }

    if( m_checkBoxECO->Get3StateValue() != wxCHK_UNDETERMINED )
    {
        cfg->m_Render.show_eco1 = m_checkBoxECO->GetValue( );
        cfg->m_Render.show_eco2 = m_checkBoxECO->GetValue( );
    }

    // Camera Options
    cfg->m_Camera.animation_enabled = m_checkBoxEnableAnimation->GetValue();
    cfg->m_Camera.moving_speed_multiplier = m_sliderAnimationSpeed->GetValue();
    cfg->m_Camera.rotation_increment = m_spinCtrlRotationAngle->GetValue();

    return true;
}


void PANEL_3D_DISPLAY_OPTIONS::ResetPanel()
{
    EDA_3D_VIEWER_SETTINGS cfg;
    cfg.Load();                     // Loading without a file will init to defaults

    loadViewSettings( &cfg );
}
