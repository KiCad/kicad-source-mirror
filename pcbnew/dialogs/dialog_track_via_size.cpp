/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014  CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

/**
 * Push and Shove router track width and via size dialog.
 */

#include "dialog_track_via_size.h"
#include <base_units.h>
#include <confirm.h>
#include <widgets/text_ctrl_eval.h>
#include <optional>
#include <eda_draw_frame.h>
#include <pcb_track.h>

#include "board_design_settings.h"

const int minSize = (int) ( 0.01 * pcbIUScale.IU_PER_MM );

DIALOG_TRACK_VIA_SIZE::DIALOG_TRACK_VIA_SIZE( EDA_DRAW_FRAME* aParent,
                                              BOARD_DESIGN_SETTINGS& aSettings ) :
    DIALOG_TRACK_VIA_SIZE_BASE( aParent ),
    m_trackWidth( aParent, m_trackWidthLabel, m_trackWidthText, m_trackWidthUnits, minSize ),
    m_viaDiameter( aParent, m_viaDiameterLabel, m_viaDiameterText, m_viaDiameterUnits, minSize ),
    m_viaDrill( aParent, m_viaDrillLabel, m_viaDrillText, m_viaDrillUnits, minSize ),
    m_settings( aSettings )
{
    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_TRACK_VIA_SIZE::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::optional<int> viaDiameter = m_viaDiameter.GetIntValue();
    std::optional<int> viaDrill = m_viaDrill.GetIntValue();

    if( std::optional<PCB_VIA::VIA_PARAMETER_ERROR> error =
                PCB_VIA::ValidateViaParameters( viaDiameter, viaDrill, std::nullopt, std::nullopt,
                                                std::nullopt, std::nullopt, std::nullopt, // secondary drill
                                                std::nullopt, std::nullopt, std::nullopt, // tertiary drill
                                                0 ) )
    {
        DisplayError( GetParent(), error->m_Message );

        if( error->m_Field == PCB_VIA::VIA_PARAMETER_ERROR::FIELD::DRILL )
            m_viaDrillText->SetFocus();
        else if( error->m_Field == PCB_VIA::VIA_PARAMETER_ERROR::FIELD::DIAMETER )
            m_viaDiameterText->SetFocus();

        return false;
    }

    // Store dialog values to the router settings
    m_settings.SetCustomTrackWidth( m_trackWidth.GetIntValue() );
    m_settings.SetCustomViaSize( m_viaDiameter.GetIntValue() );
    m_settings.SetCustomViaDrill( m_viaDrill.GetIntValue() );

    return true;
}


bool DIALOG_TRACK_VIA_SIZE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Load router settings to dialog fields
    m_trackWidth.SetValue( m_settings.GetCustomTrackWidth() );
    m_viaDiameter.SetValue( m_settings.GetCustomViaSize() );
    m_viaDrill.SetValue( m_settings.GetCustomViaDrill() );

    return true;
}

