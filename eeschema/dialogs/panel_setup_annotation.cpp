/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <refdes_tracker.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <project/project_file.h>

#include "panel_setup_annotation.h"


PANEL_SETUP_ANNOTATION::PANEL_SETUP_ANNOTATION( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  ) :
        PANEL_SETUP_ANNOTATION_BASE( aWindow ),
        m_frame( aFrame )
{
}

int getRefStyleMenuIndex( int aSubpartIdSeparator, int aFirstSubpartId )
{
    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    switch( aSubpartIdSeparator )
    {
    default:
    case 0:   return 0;
    case '.': return aFirstSubpartId == '1' ? 4 : 1;
    case '-': return aFirstSubpartId == '1' ? 5 : 2;
    case '_': return aFirstSubpartId == '1' ? 6 : 3;
    }
}


bool PANEL_SETUP_ANNOTATION::TransferDataToWindow()
{
    SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();

    m_choiceSeparatorRefId->SetSelection( getRefStyleMenuIndex( settings.m_SubpartIdSeparator,
                                                                settings.m_SubpartFirstId ) );

    m_checkReuseRefdes->SetValue( settings.m_refDesTracker->GetReuseRefDes() );

    return true;
}


bool PANEL_SETUP_ANNOTATION::TransferDataFromWindow()
{
    SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();

    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    switch( m_choiceSeparatorRefId->GetSelection() )
    {
    default:
    case 0: settings.m_SubpartFirstId = 'A'; settings.m_SubpartIdSeparator = 0;   break;
    case 1: settings.m_SubpartFirstId = 'A'; settings.m_SubpartIdSeparator = '.'; break;
    case 2: settings.m_SubpartFirstId = 'A'; settings.m_SubpartIdSeparator = '-'; break;
    case 3: settings.m_SubpartFirstId = 'A'; settings.m_SubpartIdSeparator = '_'; break;
    case 4: settings.m_SubpartFirstId = '1'; settings.m_SubpartIdSeparator = '.'; break;
    case 5: settings.m_SubpartFirstId = '1'; settings.m_SubpartIdSeparator = '-'; break;
    case 6: settings.m_SubpartFirstId = '1'; settings.m_SubpartIdSeparator = '_'; break;
    }

    settings.m_refDesTracker->SetReuseRefDes( m_checkReuseRefdes->GetValue() );
    return true;
}


void PANEL_SETUP_ANNOTATION::ImportSettingsFrom( SCHEMATIC_SETTINGS& aSettings )
{
    m_choiceSeparatorRefId->SetSelection( getRefStyleMenuIndex( aSettings.m_SubpartIdSeparator,
                                                                aSettings.m_SubpartFirstId ) );

    m_checkReuseRefdes->SetValue( aSettings.m_refDesTracker->GetReuseRefDes() );
}


void PANEL_SETUP_ANNOTATION::ResetPanel()
{
    // Reset the panel to the default values
    m_choiceSeparatorRefId->SetSelection( 0 );
    m_checkReuseRefdes->SetValue( true );
}
