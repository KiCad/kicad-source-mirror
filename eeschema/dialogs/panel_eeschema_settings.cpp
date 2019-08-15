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
#include <sch_edit_frame.h>

#include "panel_eeschema_settings.h"


PANEL_EESCHEMA_SETTINGS::PANEL_EESCHEMA_SETTINGS( SCH_EDIT_FRAME* aFrame, wxWindow* aWindow ) :
        PANEL_EESCHEMA_SETTINGS_BASE( aWindow ),
        m_frame( aFrame )
{}


bool PANEL_EESCHEMA_SETTINGS::TransferDataToWindow()
{
    m_choiceUnits->SetSelection( m_frame->GetUserUnits() == INCHES ? 0 : 1 );

    m_textSizeCtrl->SetValue( StringFromValue( INCHES, GetDefaultTextSize(), false, true ) );
    m_hPitchCtrl->SetValue( StringFromValue( INCHES, m_frame->GetRepeatStep().x, false, true ) );
    m_vPitchCtrl->SetValue( StringFromValue( INCHES, m_frame->GetRepeatStep().y, false, true ) );
    m_spinRepeatLabel->SetValue( m_frame->GetRepeatDeltaLabel() );

    m_checkHVOrientation->SetValue( m_frame->GetForceHVLines() );
    m_footprintPreview->SetValue( m_frame->GetShowFootprintPreviews() );

    m_checkAutoplaceFields->SetValue( m_frame->GetAutoplaceFields() );
    m_checkAutoplaceJustify->SetValue( m_frame->GetAutoplaceJustify() );
    m_checkAutoplaceAlign->SetValue( m_frame->GetAutoplaceAlign() );

    m_mouseDragIsDrag->SetValue( m_frame->GetDragActionIsMove() );

    return true;
}


bool PANEL_EESCHEMA_SETTINGS::TransferDataFromWindow()
{
    m_frame->SetUserUnits( m_choiceUnits->GetSelection() == 0 ? INCHES : MILLIMETRES );

    int textSize = ValueFromString( INCHES, m_textSizeCtrl->GetValue(), true );

    if( textSize != GetDefaultTextSize() )
    {
        SetDefaultTextSize( textSize );
        m_frame->SaveProjectSettings( false );
    }

    m_frame->SetRepeatStep( wxPoint( ValueFromString( INCHES, m_hPitchCtrl->GetValue(), true ),
                                     ValueFromString( INCHES, m_vPitchCtrl->GetValue(), true ) ) );
    m_frame->SetRepeatDeltaLabel( m_spinRepeatLabel->GetValue() );

    m_frame->SetForceHVLines( m_checkHVOrientation->GetValue() );
    m_frame->SetShowFootprintPreviews( m_footprintPreview->GetValue() );

    m_frame->SetAutoplaceFields( m_checkAutoplaceFields->GetValue() );
    m_frame->SetAutoplaceJustify( m_checkAutoplaceJustify->GetValue() );
    m_frame->SetAutoplaceAlign( m_checkAutoplaceAlign->GetValue() );

    m_frame->SetDragActionIsMove( !m_mouseDragIsDrag->GetValue() );

    return true;
}


