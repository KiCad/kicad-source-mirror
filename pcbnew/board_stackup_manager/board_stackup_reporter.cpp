/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <locale_io.h>

#include "board_stackup.h"
#include "stackup_predefined_prms.h"

#include "board_stackup_reporter.h"


wxString BuildStackupReport( BOARD_STACKUP& aStackup, EDA_UNITS aUnits )
{
    // Build a ascii representation of stackup and copy it in the clipboard
    wxString report;

    wxString txt;
    LOCALE_IO toggle;   // toggles on the C locale to write floating values, then off.

    for( const BOARD_STACKUP_ITEM* item : aStackup.GetList() )
    {
        // Skip stackup items useless for the current board
        if( !item->IsEnabled() )
            continue;

        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            wxString sublayer_text;

            if( item->GetSublayersCount() )
            {
                sublayer_text.Printf( wxT( "\n  sublayer \"1/%d\"" ),
                                      item->GetSublayersCount() );
            }

            txt.Printf( wxT( "layer \"%s\" type \"%s\"%s" ),
                        item->FormatDielectricLayerName(),
                        item->GetTypeName(), sublayer_text );
        }
        else
        {
            txt.Printf( wxT( "layer \"%s\" type \"%s\"" ),
                        item->GetLayerName(),
                        item->GetTypeName() );
        }

        report << txt;

        if( item->IsColorEditable() )
        {
            txt.Printf( wxT( " Color \"%s\"" ), item->GetColor() );
            report << txt;
        }

        for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
        {
            if( idx )    // not printed for the main (first) layer.
            {
                txt.Printf( wxT( "\n  sublayer \"%d/%d\"" ), idx+1, item->GetSublayersCount() );
                report << txt;
            }

            if( item->IsThicknessEditable() )
            {
                txt.Printf( wxT( " Thickness %s" ),
                            EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, aUnits, item->GetThickness( idx ), true ) );
                report << txt;

                if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && item->IsThicknessLocked( idx ) )
                {
                    txt.Printf( wxT( " Locked" ) );
                    report << txt;
                }
            }

            if( item->IsMaterialEditable() )
            {
                txt.Printf( wxT( " Material \"%s\"" ), item->GetMaterial( idx ) );
                report << txt;
            }

            if( item->HasEpsilonRValue() )
            {
                txt.Printf( wxT( " EpsilonR %s" ), item->FormatEpsilonR( idx ) );
                report << txt;
            }

            if( item->HasLossTangentValue() )
            {
                txt.Printf( wxT( " LossTg %s" ), item->FormatLossTangent( idx ) );
                report << txt;
            }
        }

        report << '\n';
    }

    // Finish and other options:
    txt.Printf( wxT( "Finish \"%s\"" ), aStackup.m_FinishType );
    report << txt;

    if( aStackup.m_HasDielectricConstrains )
        report << wxT( " Option \"Impedance Controlled\"" );

    if( aStackup.m_EdgePlating )
        report << wxT( " Option \"Plated edges\"" );

    if( aStackup.m_EdgeConnectorConstraints != BS_EDGE_CONNECTOR_NONE )
    {
        wxString conn_txt = wxT( "yes" );

        if( aStackup.m_EdgeConnectorConstraints == BS_EDGE_CONNECTOR_BEVELLED )
            conn_txt << wxT( ",bevelled" );

        txt.Printf( wxT( " EdgeConnector \"%s\"" ), conn_txt );
        report << txt;
    }

    report << '\n';

    return report;

}
