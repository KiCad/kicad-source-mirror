/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#include <widgets/color_swatch.h>
#include "panel_3D_opengl_options.h"


PANEL_3D_OPENGL_OPTIONS::PANEL_3D_OPENGL_OPTIONS( wxWindow* aParent ) :
        PANEL_3D_OPENGL_OPTIONS_BASE( aParent )
{
    m_selectionColorSwatch->SetDefaultColor( COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    m_selectionColorSwatch->SetSupportsOpacity( false );
}


void PANEL_3D_OPENGL_OPTIONS::loadSettings( EDA_3D_VIEWER_SETTINGS* aCfg )
{
    m_checkBoxCuThickness->SetValue( aCfg->m_Render.opengl_copper_thickness );
    m_checkBoxBoundingBoxes->SetValue( aCfg->m_Render.show_model_bbox );
    m_checkBoxHighlightOnRollOver->SetValue( aCfg->m_Render.highlight_on_rollover );

    m_choiceAntiAliasing->SetSelection( static_cast<int>( aCfg->m_Render.opengl_AA_mode ) );
    m_selectionColorSwatch->SetSwatchColor( aCfg->m_Render.opengl_selection_color, false );

    m_checkBoxDisableAAMove->SetValue( aCfg->m_Render.opengl_AA_disableOnMove );
    m_checkBoxDisableMoveThickness->SetValue( aCfg->m_Render.opengl_thickness_disableOnMove );
    m_checkBoxDisableMoveVias->SetValue( aCfg->m_Render.opengl_microvias_disableOnMove );
    m_checkBoxDisableMoveHoles->SetValue( aCfg->m_Render.opengl_holes_disableOnMove );
}


bool PANEL_3D_OPENGL_OPTIONS::TransferDataToWindow()
{
    loadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );
    return true;
}


bool PANEL_3D_OPENGL_OPTIONS::TransferDataFromWindow()
{
    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        cfg->m_Render.opengl_copper_thickness = m_checkBoxCuThickness->GetValue();
        cfg->m_Render.show_model_bbox = m_checkBoxBoundingBoxes->GetValue();
        cfg->m_Render.highlight_on_rollover = m_checkBoxHighlightOnRollOver->GetValue();

        cfg->m_Render.opengl_AA_mode = static_cast<ANTIALIASING_MODE>( m_choiceAntiAliasing->GetSelection() );
        cfg->m_Render.opengl_selection_color = m_selectionColorSwatch->GetSwatchColor();

        cfg->m_Render.opengl_AA_disableOnMove = m_checkBoxDisableAAMove->GetValue();
        cfg->m_Render.opengl_thickness_disableOnMove = m_checkBoxDisableMoveThickness->GetValue();
        cfg->m_Render.opengl_microvias_disableOnMove = m_checkBoxDisableMoveVias->GetValue();
        cfg->m_Render.opengl_holes_disableOnMove = m_checkBoxDisableMoveHoles->GetValue();
    }

    return true;
}


void PANEL_3D_OPENGL_OPTIONS::ResetPanel()
{
    EDA_3D_VIEWER_SETTINGS cfg;
    cfg.Load();                     // Loading without a file will init to defaults

    loadSettings( &cfg );
}
