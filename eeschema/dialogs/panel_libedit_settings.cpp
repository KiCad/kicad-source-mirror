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

#include <fctsys.h>
#include <lib_edit_frame.h>
#include <sch_painter.h>

#include "panel_libedit_settings.h"


PANEL_LIBEDIT_SETTINGS::PANEL_LIBEDIT_SETTINGS( LIB_EDIT_FRAME* aFrame, wxWindow* aWindow ) :
        PANEL_LIBEDIT_SETTINGS_BASE( aWindow ),
        m_frame( aFrame ),
        m_lineWidth( aFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits, true ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
        m_pinLength( aFrame, m_pinLengthLabel, m_pinLengthCtrl, m_pinLengthUnits, true ),
        m_pinNameSize( aFrame, m_pinNameSizeLabel, m_pinNameSizeCtrl, m_pinNameSizeUnits, true ),
        m_pinNumberSize( aFrame, m_pinNumSizeLabel, m_pinNumSizeCtrl, m_pinNumSizeUnits, true ),
        m_hPitch( aFrame, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits, true ),
        m_vPitch( aFrame, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits, true )
{}


bool PANEL_LIBEDIT_SETTINGS::TransferDataToWindow()
{
    m_lineWidth.SetValue( m_frame->GetDefaultLineWidth() );
    m_textSize.SetValue( m_frame->GetDefaultTextSize() );
    m_pinLength.SetValue( m_frame->GetDefaultPinLength() );
    m_pinNumberSize.SetValue( m_frame->GetPinNumDefaultSize() );
    m_pinNameSize.SetValue( m_frame->GetPinNameDefaultSize() );
    m_hPitch.SetValue( m_frame->GetRepeatStep().x );
    m_vPitch.SetValue( m_frame->GetRepeatStep().y );
    m_choicePinDisplacement->SetSelection( m_frame->GetRepeatPinStep() == Iu2Mils( 50 ) ? 1 : 0 );
    m_spinRepeatLabel->SetValue( m_frame->GetRepeatDeltaLabel() );

    m_checkShowPinElectricalType->SetValue( m_frame->GetShowElectricalType() );

    return true;
}


bool PANEL_LIBEDIT_SETTINGS::TransferDataFromWindow()
{
    m_frame->SetDefaultLineWidth( (int) m_lineWidth.GetValue() );
    m_frame->SetDefaultTextSize( (int) m_textSize.GetValue() );
    m_frame->SetDefaultPinLength( (int) m_pinLength.GetValue() );
    m_frame->SetPinNumDefaultSize( (int) m_pinNumberSize.GetValue() );
    m_frame->SetPinNameDefaultSize( (int) m_pinNameSize.GetValue() );
    m_frame->SetRepeatStep( wxPoint( (int) m_hPitch.GetValue(), (int) m_vPitch.GetValue() ) );
    m_frame->SetRepeatPinStep( m_choicePinDisplacement->GetSelection() == 1 ? Mils2iu( 50 )
                                                                            : Mils2iu( 100 ) );
    m_frame->SetRepeatDeltaLabel( m_spinRepeatLabel->GetValue() );

    m_frame->SetShowElectricalType( m_checkShowPinElectricalType->GetValue() );

    m_frame->GetRenderSettings()->m_ShowPinsElectricalType = m_frame->GetShowElectricalType();
    m_frame->GetCanvas()->Refresh();

    return true;
}


