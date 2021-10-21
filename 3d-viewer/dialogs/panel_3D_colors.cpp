/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_3D_colors.h"
#include <widgets/color_swatch.h>
#include <settings/settings_manager.h>
#include <pgm_base.h>


PANEL_3D_COLORS::PANEL_3D_COLORS( EDA_3D_VIEWER_FRAME* aFrame, wxWindow* aParent ) :
        PANEL_3D_COLORS_BASE( aParent ),
        m_boardAdapter( aFrame->GetAdapter() )
{
    m_backgroundTop->SetDefaultColor( m_boardAdapter.g_DefaultBackgroundTop );
    m_backgroundBottom->SetDefaultColor( m_boardAdapter.g_DefaultBackgroundBot );

    m_silkscreenTop->SetUserColors( &m_boardAdapter.g_SilkscreenColors );
    m_silkscreenTop->SetDefaultColor( m_boardAdapter.g_DefaultSilkscreen );
    m_silkscreenBottom->SetUserColors( &m_boardAdapter.g_SilkscreenColors );
    m_silkscreenBottom->SetDefaultColor( m_boardAdapter.g_DefaultSilkscreen );

    m_solderMaskTop->SetUserColors( &m_boardAdapter.g_MaskColors );
    m_solderMaskTop->SetDefaultColor( m_boardAdapter.g_DefaultSolderMask );
    m_solderMaskBottom->SetUserColors( &m_boardAdapter.g_MaskColors );
    m_solderMaskBottom->SetDefaultColor( m_boardAdapter.g_DefaultSolderMask );

    m_solderPaste->SetUserColors( &m_boardAdapter.g_PasteColors );
    m_solderPaste->SetDefaultColor( m_boardAdapter.g_DefaultSolderPaste );

    m_surfaceFinish->SetUserColors( &m_boardAdapter.g_FinishColors );
    m_surfaceFinish->SetDefaultColor( m_boardAdapter.g_DefaultSurfaceFinish );

    m_boardBody->SetUserColors( &m_boardAdapter.g_BoardColors );
    m_boardBody->SetDefaultColor( m_boardAdapter.g_DefaultBoardBody );
}


bool PANEL_3D_COLORS::TransferDataToWindow()
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    m_backgroundTop->SetSupportsOpacity( false );
    m_backgroundBottom->SetSupportsOpacity( false );
    m_silkscreenTop->SetSupportsOpacity( false );
    m_silkscreenBottom->SetSupportsOpacity( false );
    m_solderMaskTop->SetBackgroundColour( *wxWHITE );
    m_solderMaskBottom->SetBackgroundColour( *wxWHITE );
    m_solderPaste->SetSupportsOpacity( false );
    m_surfaceFinish->SetSupportsOpacity( false );
    m_boardBody->SetBackgroundColour( *wxWHITE );

    m_backgroundTop->SetSwatchColor(    colors->GetColor( LAYER_3D_BACKGROUND_TOP ),    false );
    m_backgroundBottom->SetSwatchColor( colors->GetColor( LAYER_3D_BACKGROUND_BOTTOM ), false );
    m_silkscreenTop->SetSwatchColor(    colors->GetColor( LAYER_3D_SILKSCREEN_TOP ),    false );
    m_silkscreenBottom->SetSwatchColor( colors->GetColor( LAYER_3D_SILKSCREEN_BOTTOM ), false );
    m_solderMaskTop->SetSwatchColor(    colors->GetColor( LAYER_3D_SOLDERMASK_TOP ),    false );
    m_solderMaskBottom->SetSwatchColor( colors->GetColor( LAYER_3D_SOLDERMASK_BOTTOM ), false );
    m_solderPaste->SetSwatchColor(      colors->GetColor( LAYER_3D_SOLDERPASTE ),       false );
    m_surfaceFinish->SetSwatchColor(    colors->GetColor( LAYER_3D_COPPER ),            false );
    m_boardBody->SetSwatchColor(        colors->GetColor( LAYER_3D_BOARD ),             false );

    if( colors->GetUseBoardStackupColors() )
        m_boardStackupRB->SetValue( true );
    else
        m_specificColorsRB->SetValue( true );

    return true;
}


bool PANEL_3D_COLORS::TransferDataFromWindow()
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    colors->SetColor( LAYER_3D_BACKGROUND_TOP,    m_backgroundTop->GetSwatchColor() );
    colors->SetColor( LAYER_3D_BACKGROUND_BOTTOM, m_backgroundBottom->GetSwatchColor() );
    colors->SetColor( LAYER_3D_SILKSCREEN_TOP,    m_silkscreenTop->GetSwatchColor() );
    colors->SetColor( LAYER_3D_SILKSCREEN_BOTTOM, m_silkscreenBottom->GetSwatchColor() );
    colors->SetColor( LAYER_3D_SOLDERMASK_TOP,    m_solderMaskTop->GetSwatchColor() );
    colors->SetColor( LAYER_3D_SOLDERMASK_BOTTOM, m_solderMaskBottom->GetSwatchColor() );
    colors->SetColor( LAYER_3D_SOLDERPASTE,       m_solderPaste->GetSwatchColor() );
    colors->SetColor( LAYER_3D_COPPER,            m_surfaceFinish->GetSwatchColor() );
    colors->SetColor( LAYER_3D_BOARD,             m_boardBody->GetSwatchColor() );

    colors->SetUseBoardStackupColors( m_boardStackupRB->GetValue() );

    Pgm().GetSettingsManager().SaveColorSettings( colors, "3d_viewer" );

    return true;
}
