/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <widgets/ui_common.h>
#include <settings/settings_manager.h>
#include <gerbview_settings.h>

#include "panel_gerbview_excellon_settings.h"


PANEL_GERBVIEW_EXCELLON_SETTINGS::PANEL_GERBVIEW_EXCELLON_SETTINGS( wxWindow* aParent ) :
        PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE( aParent, wxID_ANY )
{
    wxFont helpFont = KIUI::GetInfoFont( this ).Italic();
    m_fileFormatHelp->SetFont( helpFont );
    m_coordsFormatHelp->SetFont( helpFont );
    m_hint1->SetFont( helpFont );
    m_hint2->SetFont( helpFont );
}


bool PANEL_GERBVIEW_EXCELLON_SETTINGS::TransferDataToWindow( )
{
    if( GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" ) )
    {
        EXCELLON_DEFAULTS  curr_settings;

        cfg->GetExcellonDefaults( curr_settings );
        applySettingsToPanel( curr_settings );
    }

    return true;
}


bool PANEL_GERBVIEW_EXCELLON_SETTINGS::TransferDataFromWindow()
{
    if( GERBVIEW_SETTINGS* cfg = GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" ) )
    {
        cfg->m_ExcellonDefaults.m_UnitsMM = m_rbMM->GetValue();
        cfg->m_ExcellonDefaults.m_LeadingZero = m_rbLZ->GetValue();
        // The first value of these param is 2, not 0
        #define FIRST_VALUE 2
        cfg->m_ExcellonDefaults.m_MmIntegerLen = m_choiceIntegerMM->GetSelection()+FIRST_VALUE;
        cfg->m_ExcellonDefaults.m_MmMantissaLen = m_choiceMantissaMM->GetSelection()+FIRST_VALUE;
        cfg->m_ExcellonDefaults.m_InchIntegerLen = m_choiceIntegerInch->GetSelection()+FIRST_VALUE;
        cfg->m_ExcellonDefaults.m_InchMantissaLen = m_choiceMantissaInch->GetSelection()+FIRST_VALUE;
    }

    return true;
}


void PANEL_GERBVIEW_EXCELLON_SETTINGS::ResetPanel()
{
    EXCELLON_DEFAULTS defaults;
    applySettingsToPanel( defaults );
}


void PANEL_GERBVIEW_EXCELLON_SETTINGS::applySettingsToPanel( const EXCELLON_DEFAULTS& aSettings )
{
    if( aSettings.m_UnitsMM )
        m_rbMM->SetValue( true );
    else
        m_rbInches->SetValue( true );

    if( aSettings.m_LeadingZero )
        m_rbLZ->SetValue( true );
    else
        m_rbTZ->SetValue( true );

    // The first value of these param is 2, not 0
    #define FIRST_VALUE 2
    m_choiceIntegerMM->SetSelection( aSettings.m_MmIntegerLen-FIRST_VALUE );
    m_choiceMantissaMM->SetSelection( aSettings.m_MmMantissaLen-FIRST_VALUE );
    m_choiceIntegerInch->SetSelection( aSettings.m_InchIntegerLen-FIRST_VALUE );
    m_choiceMantissaInch->SetSelection( aSettings.m_InchMantissaLen-FIRST_VALUE );
}

