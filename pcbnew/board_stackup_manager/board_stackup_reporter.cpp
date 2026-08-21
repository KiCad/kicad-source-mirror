/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2026 Krishna Swaroop <krishna.swaroop@pixxel.co.in>
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

/**
 * @file board_stackup_reporter.cpp
 */

#include "wx/string.h"
#include "wx/sstream.h"

#include <base_units.h>
#include <io/csv.h>
#include <locale_io.h>
#include <vector>

#include "board_stackup.h"
#include "board_stackup_reporter.h"


static wxString layerDisplayName( const BOARD_STACKUP_ITEM* aItem )
{
    if( aItem->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        return aItem->FormatDielectricLayerName();

    return aItem->GetLayerName();
}


static void appendCsvProperties( std::vector<std::vector<wxString>>& aRows, BOARD_STACKUP& aStackup,
                                 const STACKUP_CSV_OPTIONS& aOptions )
{
    auto appendProperty = [&]( const wxString& aName, const wxString& aValue )
    {
        aRows.push_back( { aName, aValue } );
    };

    if( aOptions.includeFinish )
        appendProperty( wxT( "Finish" ), aStackup.m_FinishType );

    if( aOptions.includeBoardOptions )
    {
        if( aStackup.m_HasDielectricConstrains )
            appendProperty( wxT( "Impedance Controlled" ), wxT( "yes" ) );

        if( aStackup.m_EdgePlating )
            appendProperty( wxT( "Plated edges" ), wxT( "yes" ) );

        if( aStackup.m_EdgeConnectorConstraints != BS_EDGE_CONNECTOR_NONE )
        {
            wxString conn = wxT( "yes" );

            if( aStackup.m_EdgeConnectorConstraints == BS_EDGE_CONNECTOR_BEVELLED )
                conn << wxT( ",bevelled" );

            appendProperty( wxT( "EdgeConnector" ), conn );
        }
    }
}


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


wxString BuildStackupCsv( BOARD_STACKUP& aStackup, EDA_UNITS aUnits, const STACKUP_CSV_OPTIONS& aOptions )
{
    std::vector<std::vector<wxString>> rows;
    LOCALE_IO                          toggle;

    std::vector<wxString> header = { wxT( "Layer" ), wxT( "Type" ), wxT( "Sublayer" ) };

    if( aOptions.includeThickness )
    {
        header.push_back( wxT( "Thickness" ) );
        header.push_back( wxT( "Locked" ) );
    }

    if( aOptions.includeMaterial )
        header.push_back( wxT( "Material" ) );

    if( aOptions.includeColor )
        header.push_back( wxT( "Color" ) );

    if( aOptions.includeEpsilonR )
        header.push_back( wxT( "EpsilonR" ) );

    if( aOptions.includeLossTangent )
        header.push_back( wxT( "LossTg" ) );

    rows.push_back( header );

    for( const BOARD_STACKUP_ITEM* item : aStackup.GetList() )
    {
        if( !item->IsEnabled() )
            continue;

        int sublayerCount = item->GetSublayersCount();

        if( sublayerCount <= 0 )
            sublayerCount = 1;

        for( int idx = 0; idx < sublayerCount; idx++ )
        {
            const bool            hasSublayerData = item->GetSublayersCount() > 0;
            std::vector<wxString> row;

            row.push_back( layerDisplayName( item ) );
            row.push_back( item->GetTypeName() );

            if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC && item->GetSublayersCount() > 1 )
                row.push_back( wxString::Format( wxT( "%d/%d" ), idx + 1, item->GetSublayersCount() ) );
            else
                row.push_back( wxEmptyString );

            if( aOptions.includeThickness )
            {
                wxString thickness;

                if( item->IsThicknessEditable() && hasSublayerData )
                {
                    thickness =
                            EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, aUnits, item->GetThickness( idx ), true );
                }

                row.push_back( thickness );

                const bool locked =
                        item->GetType() == BS_ITEM_TYPE_DIELECTRIC && hasSublayerData && item->IsThicknessLocked( idx );
                row.push_back( locked ? wxT( "yes" ) : wxEmptyString );
            }

            if( aOptions.includeMaterial )
            {
                row.push_back( ( item->IsMaterialEditable() && hasSublayerData ) ? item->GetMaterial( idx )
                                                                                 : wxString() );
            }

            if( aOptions.includeColor )
                row.push_back( item->IsColorEditable() ? item->GetColor() : wxString() );

            if( aOptions.includeEpsilonR )
            {
                row.push_back( ( item->HasEpsilonRValue() && hasSublayerData ) ? item->FormatEpsilonR( idx )
                                                                               : wxString() );
            }

            if( aOptions.includeLossTangent )
            {
                row.push_back( ( item->HasLossTangentValue() && hasSublayerData ) ? item->FormatLossTangent( idx )
                                                                                  : wxString() );
            }

            rows.push_back( row );
        }
    }

    if( aOptions.includeFinish || aOptions.includeBoardOptions )
    {
        rows.push_back( {} );
        rows.push_back( { wxT( "Property" ), wxT( "Value" ) } );
        appendCsvProperties( rows, aStackup, aOptions );
    }

    wxStringOutputStream os;
    CSV_WRITER           writer( os );
    writer.WriteLines( rows );

    return os.GetString();
}
