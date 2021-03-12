/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <painter.h>
#include <pgm_base.h>
#include <eeschema_settings.h>
#include "panel_eeschema_editing_options.h"


PANEL_EESCHEMA_EDITING_OPTIONS::PANEL_EESCHEMA_EDITING_OPTIONS( SCH_EDIT_FRAME* aFrame,
                                                                wxWindow* aWindow ) :
        PANEL_EESCHEMA_EDITING_OPTIONS_BASE( aWindow ),
        m_frame( aFrame ),
        m_hPitch( aFrame, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits ),
        m_vPitch( aFrame, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits )
{
    // Make the color swatch show "Clear Color" instead
    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_backgroundColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

#ifdef __WXOSX_MAC__
    m_mouseCmdsOSX->Show( true );
    m_mouseCmdsWinLin->Show( false );
#else
    m_mouseCmdsWinLin->Show( true );
    m_mouseCmdsOSX->Show( false );
#endif
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataToWindow()
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();

    m_hPitch.SetValue( Mils2iu( cfg->m_Drawing.default_repeat_offset_x ) );
    m_vPitch.SetValue( Mils2iu( cfg->m_Drawing.default_repeat_offset_y ) );
    m_spinLabelRepeatStep->SetValue( cfg->m_Drawing.repeat_label_increment );

    COLOR_SETTINGS* settings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = settings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetSwatchBackground( schematicBackground );
    m_borderColorSwatch->SetSwatchColor( cfg->m_Drawing.default_sheet_border_color, false );

    m_backgroundColorSwatch->SetSwatchBackground( schematicBackground );
    m_backgroundColorSwatch->SetSwatchColor( cfg->m_Drawing.default_sheet_background_color, false );

    m_checkHVOrientation->SetValue( cfg->m_Drawing.hv_lines_only );
    m_footprintPreview->SetValue( cfg->m_Appearance.footprint_preview );
    m_navigatorStaysOpen->SetValue( cfg->m_Appearance.navigator_stays_open );

    m_checkAutoplaceFields->SetValue( cfg->m_AutoplaceFields.enable );
    m_checkAutoplaceJustify->SetValue( cfg->m_AutoplaceFields.allow_rejustify );
    m_checkAutoplaceAlign->SetValue( cfg->m_AutoplaceFields.align_to_grid );

    m_mouseDragIsDrag->SetValue( !cfg->m_Input.drag_is_move );
    m_cbPinSelectionOpt->SetValue( cfg->m_Selection.select_pin_selects_symbol );

    m_cbAutoStartWires->SetValue( cfg->m_Drawing.auto_start_wires );

    return true;
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataFromWindow()
{
    EESCHEMA_SETTINGS* cfg = m_frame->eeconfig();

    cfg->m_Drawing.default_sheet_border_color = m_borderColorSwatch->GetSwatchColor();
    cfg->m_Drawing.default_sheet_background_color = m_backgroundColorSwatch->GetSwatchColor();

    cfg->m_Drawing.default_repeat_offset_x = Iu2Mils( (int) m_hPitch.GetValue() );
    cfg->m_Drawing.default_repeat_offset_y = Iu2Mils( (int) m_vPitch.GetValue() );
    cfg->m_Drawing.repeat_label_increment = m_spinLabelRepeatStep->GetValue();

    cfg->m_Drawing.hv_lines_only = m_checkHVOrientation->GetValue();
    cfg->m_Appearance.footprint_preview = m_footprintPreview->GetValue();
    cfg->m_Appearance.navigator_stays_open = m_navigatorStaysOpen->GetValue();

    cfg->m_AutoplaceFields.enable = m_checkAutoplaceFields->GetValue();
    cfg->m_AutoplaceFields.allow_rejustify = m_checkAutoplaceJustify->GetValue();
    cfg->m_AutoplaceFields.align_to_grid = m_checkAutoplaceAlign->GetValue();

    cfg->m_Input.drag_is_move = !m_mouseDragIsDrag->GetValue();
    cfg->m_Selection.select_pin_selects_symbol = m_cbPinSelectionOpt->GetValue();

    cfg->m_Drawing.auto_start_wires = m_cbAutoStartWires->GetValue();

    m_frame->SaveProjectSettings();

    return true;
}


