/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

 /**
  * @file board_stackup_reporter.cpp
  */

#include "wx/string.h"

#include <base_units.h>

#include "class_board_stackup.h"
#include "stackup_predefined_prms.h"

#include "board_stackup_reporter.h"


wxString BuildStackupReport( BOARD_STACKUP& aStackup, EDA_UNITS_T aUnits )
{
    // Build a ascii representation of stackup and copy it in the clipboard
    wxString report;

    wxString txt;
    int row = 0;
    LOCALE_IO toggle;   // toggles on the C locale to write floating values, then off.

    for( const auto item : aStackup.GetList() )
    {
        // Skip stackup items useless for the current board
        if( !item->m_Enabled )
        {
            row++;
            continue;
        }

        txt.Printf( "layer \"%s\" type \"%s\"", item->m_LayerName, item->m_TypeName );
        report << txt;

        if( item->HasEpsilonRValue() )
        {
            txt.Printf( " EpsilonR %s", item->FormatEpsilonR() );
            report << txt;
        }

        if( item->HasLossTangentValue() )
        {
            txt.Printf( " LossTg %s", item->FormatLossTangent() );
            report << txt;
        }

        if( item->IsMaterialEditable() )
        {
            txt.Printf( " Material \"%s\"", item->m_Material );
            report << txt;
        }

        if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC )
        {
            txt.Printf( " \"%s\"", item->m_TypeName );
            report << txt;
        }

        if( item->IsThicknessEditable() )
        {
            txt.Printf( " Thickness %s",
                        StringFromValue( aUnits, item->m_Thickness, true, true ) );
            report << txt;

            if( item->m_Type == BS_ITEM_TYPE_DIELECTRIC && item->m_ThicknessLocked )
            {
                txt.Printf( " Locked" );
                report << txt;
            }
        }

        if( item->IsColorEditable() )
        {
            txt.Printf( " Color \"%s\"", item->m_Color );
            report << txt;
        }
        row++;

        report << '\n';
    }

    // Finish and other options:
    txt.Printf( "Finish \"%s\"", aStackup.m_FinishType );
    report << txt;

    if( aStackup.m_HasDielectricConstrains )
        report << " Option \"Impedance Controlled\"";

    if( aStackup.m_EdgePlating )
        report << " Option \"Plated edges\"";

    if( aStackup.m_CastellatedPads )
        report << " Option \"Castellated Pads\"";

    if( aStackup.m_EdgeConnectorConstraints != BS_EDGE_CONNECTOR_NONE )
    {
        wxString conn_txt = "yes";

        if( aStackup.m_EdgeConnectorConstraints == BS_EDGE_CONNECTOR_BEVELLED )
            conn_txt << ",bevelled";

        txt.Printf( " EdgeConnector \"%s\"", conn_txt );
        report << txt;
    }

    report << '\n';

    return report;

}
