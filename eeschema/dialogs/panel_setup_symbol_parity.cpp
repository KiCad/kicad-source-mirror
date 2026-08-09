/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <kiface_base.h>
#include <panel_setup_symbol_parity.h>


PANEL_SETUP_SYMBOL_PARITY::PANEL_SETUP_SYMBOL_PARITY( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  ) :
        PANEL_SETUP_SYMBOL_PARITY_BASE( aWindow ),
        m_frame( aFrame )
{
}


bool PANEL_SETUP_SYMBOL_PARITY::TransferDataToWindow()
{
    SYMBOL_PARITY_SETTINGS& settings = m_frame->Schematic().Settings().m_SymbolParity;

    m_missingFields->SetValue( settings.m_MissingFields );
    m_extraFields->SetValue( settings.m_ExtraFields );
    m_fieldTextOpt->SetValue( settings.m_FieldTexts );
    m_fieldVisibilitiesOpt->SetValue( settings.m_FieldVisibilities );
    m_fieldStyleOpt->SetValue( settings.m_FieldStyles );
    m_fieldPositionsOpt->SetValue( settings.m_FieldPositions );

    m_pinVisibilitiesOpt->SetValue( settings.m_PinVisibilities );
    m_altPinFunctionsOpt->SetValue( settings.m_PinAltFunctions );

    m_excludeFromBoardOpt->SetValue( settings.m_ExcludeFromBoardFlags );
    m_DNPOpt->SetValue( settings.m_DNPFlags );
    m_excludeFromBOMOpt->SetValue( settings.m_ExcludeFromBOMFlags );
    m_excludeFromPosFilesOpt->SetValue( settings.m_ExcludeFromPosFileFlags );

    return true;
}


bool PANEL_SETUP_SYMBOL_PARITY::TransferDataFromWindow()
{
    SYMBOL_PARITY_SETTINGS& settings = m_frame->Schematic().Settings().m_SymbolParity;

    settings.m_MissingFields = m_missingFields->GetValue();
    settings.m_ExtraFields = m_extraFields->GetValue();
    settings.m_FieldTexts = m_fieldTextOpt->GetValue();
    settings.m_FieldVisibilities = m_fieldVisibilitiesOpt->GetValue();
    settings.m_FieldStyles = m_fieldStyleOpt->GetValue();
    settings.m_FieldPositions = m_fieldPositionsOpt->GetValue();

    settings.m_PinVisibilities = m_pinVisibilitiesOpt->GetValue();
    settings.m_PinAltFunctions = m_altPinFunctionsOpt->GetValue();

    settings.m_ExcludeFromBoardFlags = m_excludeFromBoardOpt->GetValue();
    settings.m_DNPFlags = m_DNPOpt->GetValue();
    settings.m_ExcludeFromBOMFlags = m_excludeFromBOMOpt->GetValue();
    settings.m_ExcludeFromPosFileFlags = m_excludeFromPosFilesOpt->GetValue();

    return true;
}


void PANEL_SETUP_SYMBOL_PARITY::ImportSettingsFrom( SYMBOL_PARITY_SETTINGS& aSettings )
{
    m_missingFields->SetValue( aSettings.m_MissingFields );
    m_extraFields->SetValue( aSettings.m_ExtraFields );
    m_fieldTextOpt->SetValue( aSettings.m_FieldTexts );
    m_fieldVisibilitiesOpt->SetValue( aSettings.m_FieldVisibilities );
    m_fieldStyleOpt->SetValue( aSettings.m_FieldStyles );
    m_fieldPositionsOpt->SetValue( aSettings.m_FieldPositions );

    m_pinVisibilitiesOpt->SetValue( aSettings.m_PinVisibilities );
    m_altPinFunctionsOpt->SetValue( aSettings.m_PinAltFunctions );

    m_excludeFromBoardOpt->SetValue( aSettings.m_ExcludeFromBoardFlags );
    m_DNPOpt->SetValue( aSettings.m_DNPFlags );
    m_excludeFromBOMOpt->SetValue( aSettings.m_ExcludeFromBOMFlags);
    m_excludeFromPosFilesOpt->SetValue( aSettings.m_ExcludeFromPosFileFlags );
}
