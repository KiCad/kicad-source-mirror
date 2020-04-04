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
#include <sch_edit_frame.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <painter.h>
#include <pgm_base.h>
#include "panel_eeschema_settings.h"


PANEL_EESCHEMA_SETTINGS::PANEL_EESCHEMA_SETTINGS( SCH_EDIT_FRAME* aFrame, wxWindow* aWindow ) :
        PANEL_EESCHEMA_SETTINGS_BASE( aWindow ),
        m_frame( aFrame ),
        m_defaultTextSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, true ),
        m_hPitch( aFrame, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits, true ),
        m_vPitch( aFrame, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits, true )
{}


bool PANEL_EESCHEMA_SETTINGS::TransferDataToWindow()
{
    m_choiceUnits->SetSelection( m_frame->GetUserUnits() == EDA_UNITS::INCHES ? 0 : 1 );

    m_defaultTextSize.SetValue( m_frame->GetDefaultTextSize() );
    m_hPitch.SetValue( m_frame->GetRepeatStep().x );
    m_vPitch.SetValue( m_frame->GetRepeatStep().y );
    m_spinLabelRepeatStep->SetValue( m_frame->GetRepeatDeltaLabel() );

    COLOR_SETTINGS* settings = m_frame->GetColorSettings();
    COLOR4D         schematicBackground = settings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetSwatchBackground( schematicBackground );
    m_borderColorSwatch->SetSwatchColor( m_frame->GetDefaultSheetBorderColor(), false );

    m_backgroundColorSwatch->SetSwatchBackground( schematicBackground );
    m_backgroundColorSwatch->SetSwatchColor( m_frame->GetDefaultSheetBackgroundColor(), false );

    m_checkHVOrientation->SetValue( m_frame->GetForceHVLines() );
    m_footprintPreview->SetValue( m_frame->GetShowFootprintPreviews() );
    m_navigatorStaysOpen->SetValue( m_frame->GetNavigatorStaysOpen() );

    m_checkAutoplaceFields->SetValue( m_frame->GetAutoplaceFields() );
    m_checkAutoplaceJustify->SetValue( m_frame->GetAutoplaceJustify() );
    m_checkAutoplaceAlign->SetValue( m_frame->GetAutoplaceAlign() );

    m_mouseDragIsDrag->SetValue( !m_frame->GetDragActionIsMove() );

    m_cbPinSelectionOpt->SetValue( m_frame->GetSelectPinSelectSymbol() );

    return true;
}


bool PANEL_EESCHEMA_SETTINGS::TransferDataFromWindow()
{
    m_frame->SetUserUnits( m_choiceUnits->GetSelection() == 0 ? EDA_UNITS::INCHES
                                                              : EDA_UNITS::MILLIMETRES );

    m_frame->SetDefaultTextSize( (int) m_defaultTextSize.GetValue() );

    m_frame->SetDefaultSheetBorderColor( m_borderColorSwatch->GetSwatchColor() );
    m_frame->SetDefaultSheetBackgroundColor( m_backgroundColorSwatch->GetSwatchColor() );

    m_frame->SetRepeatStep( wxPoint( (int) m_hPitch.GetValue(),  (int) m_vPitch.GetValue() ) );
    m_frame->SetRepeatDeltaLabel( m_spinLabelRepeatStep->GetValue() );

    m_frame->SetForceHVLines( m_checkHVOrientation->GetValue() );
    m_frame->SetShowFootprintPreviews( m_footprintPreview->GetValue() );
    m_frame->SetNavigatorStaysOpen( m_navigatorStaysOpen->GetValue() );

    m_frame->SetAutoplaceFields( m_checkAutoplaceFields->GetValue() );
    m_frame->SetAutoplaceJustify( m_checkAutoplaceJustify->GetValue() );
    m_frame->SetAutoplaceAlign( m_checkAutoplaceAlign->GetValue() );

    m_frame->SetDragActionIsMove( !m_mouseDragIsDrag->GetValue() );

    m_frame->SetSelectPinSelectSymbol( m_cbPinSelectionOpt->GetValue() );

    m_frame->SaveProjectSettings();

    return true;
}


