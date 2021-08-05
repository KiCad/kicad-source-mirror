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

#include "panel_3D_opengl_options.h"
#include <widgets/color_swatch.h>
#include <3d_canvas/board_adapter.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <3d_viewer/tools/eda_3d_controller.h>

PANEL_3D_OPENGL_OPTIONS::PANEL_3D_OPENGL_OPTIONS( EDA_3D_VIEWER_FRAME* aFrame, wxWindow* aParent ) :
        PANEL_3D_OPENGL_OPTIONS_BASE( aParent ),
        m_settings( aFrame->GetAdapter() ),
        m_canvas( aFrame->GetCanvas() )
{
    m_selectionColorSwatch->SetDefaultColor( COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    m_selectionColorSwatch->SetSupportsOpacity( false );
}


bool PANEL_3D_OPENGL_OPTIONS::TransferDataToWindow()
{
    m_checkBoxCuThickness->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) );
    m_checkBoxBoundingBoxes->SetValue( m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );
    m_checkBoxHighlightOnRollOver->SetValue( m_settings.GetFlag( FL_HIGHLIGHT_ROLLOVER_ITEM ) );

    m_choiceAntiAliasing->SetSelection( static_cast<int>( m_settings.GetAntiAliasingMode() ) );
    m_selectionColorSwatch->SetSwatchColor( COLOR4D( m_settings.m_OpenGlSelectionColor.r,
                                                     m_settings.m_OpenGlSelectionColor.g,
                                                     m_settings.m_OpenGlSelectionColor.b,
                                                     1.0 ),
                                            false );

    m_checkBoxDisableAAMove->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveThickness->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveVias->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE ) );
    m_checkBoxDisableMoveHoles->SetValue(
            m_settings.GetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE ) );

    return true;
}


bool PANEL_3D_OPENGL_OPTIONS::TransferDataFromWindow()
{
    m_settings.SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, m_checkBoxCuThickness->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX, m_checkBoxBoundingBoxes->GetValue() );
    m_settings.SetFlag( FL_HIGHLIGHT_ROLLOVER_ITEM, m_checkBoxHighlightOnRollOver->GetValue() );

    m_settings.SetAntiAliasingMode(
            static_cast<ANTIALIASING_MODE>( m_choiceAntiAliasing->GetSelection() ) );
    m_settings.m_OpenGlSelectionColor = SFVEC3F( m_selectionColorSwatch->GetSwatchColor().r,
                                                 m_selectionColorSwatch->GetSwatchColor().g,
                                                 m_selectionColorSwatch->GetSwatchColor().b );

    m_settings.SetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE,
                        m_checkBoxDisableAAMove->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveThickness->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveVias->GetValue() );
    m_settings.SetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE,
                        m_checkBoxDisableMoveHoles->GetValue() );

    return true;
}
