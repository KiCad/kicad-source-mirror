/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <symbol_edit_frame.h>
#include <sch_painter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>

#include "panel_sym_editing_options.h"


PANEL_SYM_EDITING_OPTIONS::PANEL_SYM_EDITING_OPTIONS( SYMBOL_EDIT_FRAME* aFrame,
                                                      wxWindow* aWindow ) :
        PANEL_SYM_EDITING_OPTIONS_BASE( aWindow ),
        m_frame( aFrame ),
        m_lineWidth( aFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_pinLength( aFrame, m_pinLengthLabel, m_pinLengthCtrl, m_pinLengthUnits ),
        m_pinNameSize( aFrame, m_pinNameSizeLabel, m_pinNameSizeCtrl, m_pinNameSizeUnits ),
        m_pinNumberSize( aFrame, m_pinNumSizeLabel, m_pinNumSizeCtrl, m_pinNumSizeUnits ),
        m_hPitch( aFrame, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits ),
        m_vPitch( aFrame, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits )
{}


bool PANEL_SYM_EDITING_OPTIONS::TransferDataToWindow()
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

    m_lineWidth.SetValue( Mils2iu( settings->m_Defaults.line_width ) );
    m_textSize.SetValue( Mils2iu( settings->m_Defaults.text_size ) );
    m_pinLength.SetValue( Mils2iu( settings->m_Defaults.pin_length ) );
    m_pinNumberSize.SetValue( Mils2iu( settings->m_Defaults.pin_num_size ) );
    m_pinNameSize.SetValue( Mils2iu( settings->m_Defaults.pin_name_size ) );
    m_hPitch.SetValue( Mils2iu( settings->m_Repeat.x_step ) );
    m_vPitch.SetValue( Mils2iu( settings->m_Repeat.y_step ) );
    m_choicePinDisplacement->SetSelection( settings->m_Repeat.pin_step == 50 ? 1 : 0 );
    m_spinRepeatLabel->SetValue( settings->m_Repeat.label_delta );

    m_cbShowPinElectricalType->SetValue( m_frame->GetRenderSettings()->m_ShowPinsElectricalType );

    return true;
}


bool PANEL_SYM_EDITING_OPTIONS::TransferDataFromWindow()
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();

    settings->m_Defaults.line_width = Iu2Mils( (int) m_lineWidth.GetValue() );
    settings->m_Defaults.text_size = Iu2Mils( (int) m_textSize.GetValue() );
    settings->m_Defaults.pin_length = Iu2Mils( (int) m_pinLength.GetValue() );
    settings->m_Defaults.pin_num_size = Iu2Mils( (int) m_pinNumberSize.GetValue() );
    settings->m_Defaults.pin_name_size = Iu2Mils( (int) m_pinNameSize.GetValue() );
    settings->m_Repeat.x_step = Iu2Mils( (int) m_hPitch.GetValue() );
    settings->m_Repeat.y_step = Iu2Mils( (int) m_vPitch.GetValue() );
    settings->m_Repeat.label_delta = m_spinRepeatLabel->GetValue();
    settings->m_Repeat.pin_step = m_choicePinDisplacement->GetSelection() == 1 ? 50 : 100;

    m_frame->GetRenderSettings()->m_ShowPinsElectricalType = m_cbShowPinElectricalType->GetValue();
    m_frame->GetCanvas()->Refresh();

    return true;
}


