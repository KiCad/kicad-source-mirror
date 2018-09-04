/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_screen.h>
#include <lib_edit_frame.h>
#include <sch_view.h>
#include <sch_painter.h>

#include "panel_libedit_settings.h"


PANEL_LIBEDIT_SETTINGS::PANEL_LIBEDIT_SETTINGS( LIB_EDIT_FRAME* aFrame, wxWindow* aWindow ) :
        PANEL_LIBEDIT_SETTINGS_BASE( aWindow ),
        m_frame( aFrame )
{}


bool PANEL_LIBEDIT_SETTINGS::TransferDataToWindow()
{
    m_lineWidthCtrl->SetValue( StringFromValue( INCHES, GetDefaultLineThickness(), false, true ) );
    m_pinLengthCtrl->SetValue( StringFromValue( INCHES, m_frame->GetDefaultPinLength(), false, true ) );
    m_pinNumSizeCtrl->SetValue( StringFromValue( INCHES, m_frame->GetPinNumDefaultSize(), false, true ) );
    m_pinNameSizeCtrl->SetValue( StringFromValue( INCHES, m_frame->GetPinNameDefaultSize(), false, true ) );
    m_hPitchCtrl->SetValue( StringFromValue( INCHES, m_frame->GetRepeatStep().x, false, true ) );
    m_vPitchCtrl->SetValue( StringFromValue( INCHES, m_frame->GetRepeatStep().y, false, true ) );
    m_choicePinDisplacement->SetSelection( m_frame->GetRepeatPinStep() == 50 ? 1 : 0 );
    m_spinRepeatLabel->SetValue( m_frame->GetRepeatDeltaLabel() );

    m_checkShowPinElectricalType->SetValue( m_frame->GetShowElectricalType() );

    return true;
}


bool PANEL_LIBEDIT_SETTINGS::TransferDataFromWindow()
{
    SetDefaultLineThickness( ValueFromString( INCHES, m_lineWidthCtrl->GetValue(), true ) );
    m_frame->SetDefaultPinLength( ValueFromString( INCHES, m_pinLengthCtrl->GetValue(), true ) );
    m_frame->SetPinNumDefaultSize( ValueFromString( INCHES, m_pinNumSizeCtrl->GetValue(), true ) );
    m_frame->SetPinNameDefaultSize( ValueFromString( INCHES, m_pinNameSizeCtrl->GetValue(), true ) );
    m_frame->SetRepeatStep( wxPoint( ValueFromString( INCHES, m_hPitchCtrl->GetValue(), true ),
                                     ValueFromString( INCHES, m_vPitchCtrl->GetValue(), true ) ) );
    m_frame->SetRepeatPinStep( m_choicePinDisplacement->GetSelection() == 1 ? 50 : 100 );
    m_frame->SetRepeatDeltaLabel( m_spinRepeatLabel->GetValue() );

    m_frame->SetShowElectricalType( m_checkShowPinElectricalType->GetValue() );

    m_frame->GetRenderSettings()->m_ShowPinsElectricalType = m_frame->GetShowElectricalType();
    m_frame->GetCanvas()->Refresh();

    return true;
}


