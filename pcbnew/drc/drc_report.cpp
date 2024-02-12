/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/string.h>

#include <board.h>
#include <board_design_settings.h>
#include <build_version.h>
#include "drc_report.h"
#include <drc/drc_item.h>
#include <fstream>
#include <macros.h>
#include <nlohmann/json.hpp>
#include <rc_json_schema.h>


DRC_REPORT::DRC_REPORT( BOARD* aBoard, EDA_UNITS aReportUnits,
                        std::shared_ptr<RC_ITEMS_PROVIDER> aMarkersProvider,
                        std::shared_ptr<RC_ITEMS_PROVIDER> aRatsnestProvider,
                        std::shared_ptr<RC_ITEMS_PROVIDER> aFpWarningsProvider) :
        m_board( aBoard ),
        m_reportUnits( aReportUnits ),
        m_markersProvider( std::move( aMarkersProvider ) ),
        m_ratsnestProvider( std::move( aRatsnestProvider ) ),
        m_fpWarningsProvider( std::move( aFpWarningsProvider ) )
{

}


bool DRC_REPORT::WriteTextReport( const wxString& aFullFileName )
{
    FILE* fp = wxFopen( aFullFileName, wxT( "w" ) );

    if( fp == nullptr )
        return false;

    std::map<KIID, EDA_ITEM*> itemMap;
    m_board->FillItemMap( itemMap );

    UNITS_PROVIDER         unitsProvider( pcbIUScale, m_reportUnits );
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    int                    count;

    wxFileName fn( m_board->GetFileName() );
    fprintf( fp, "** Drc report for %s **\n", TO_UTF8( fn.GetFullName() ) );

    fprintf( fp, "** Created on %s **\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

    count = m_markersProvider->GetCount();

    fprintf( fp, "\n** Found %d DRC violations **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_markersProvider->GetItem( i );
        SEVERITY                        severity = item->GetParent()->GetSeverity();

        if( severity == RPT_SEVERITY_EXCLUSION )
            severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }

    count = m_ratsnestProvider->GetCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_ratsnestProvider->GetItem( i );
        SEVERITY                        severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }

    count = m_fpWarningsProvider->GetCount();

    fprintf( fp, "\n** Found %d Footprint errors **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_fpWarningsProvider->GetItem( i );
        SEVERITY                        severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( &unitsProvider, severity, itemMap ) ) );
    }


    fprintf( fp, "\n** End of Report **\n" );

    fclose( fp );

    return true;
}


bool DRC_REPORT::WriteJsonReport( const wxString& aFullFileName )
{
    std::ofstream jsonFileStream( aFullFileName.fn_str() );

    UNITS_PROVIDER            unitsProvider( pcbIUScale, m_reportUnits );
    BOARD_DESIGN_SETTINGS&    bds = m_board->GetDesignSettings();
    std::map<KIID, EDA_ITEM*> itemMap;
    m_board->FillItemMap( itemMap );

    RC_JSON::DRC_REPORT reportHead;

    wxFileName fn( m_board->GetFileName() );
    reportHead.source = fn.GetFullName();
    reportHead.date = GetISO8601CurrentDateTime();
    reportHead.kicad_version = GetMajorMinorPatchVersion();
    reportHead.coordinate_units = EDA_UNIT_UTILS::GetLabel( m_reportUnits );

    for( int i = 0; i < m_markersProvider->GetCount(); ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_markersProvider->GetItem( i );
        SEVERITY                        severity = item->GetParent()->GetSeverity();

        if( severity == RPT_SEVERITY_EXCLUSION )
            severity = bds.GetSeverity( item->GetErrorCode() );

        RC_JSON::VIOLATION violation;
        item->GetJsonViolation( violation, &unitsProvider, severity, itemMap );

        reportHead.violations.push_back( violation );
    }

    for( int i = 0; i < m_ratsnestProvider->GetCount(); ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_ratsnestProvider->GetItem( i );
        SEVERITY                        severity = bds.GetSeverity( item->GetErrorCode() );

        RC_JSON::VIOLATION violation;
        item->GetJsonViolation( violation, &unitsProvider, severity, itemMap );

        reportHead.unconnected_items.push_back( violation );
    }


    for( int i = 0; i < m_fpWarningsProvider->GetCount(); ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_fpWarningsProvider->GetItem( i );
        SEVERITY                        severity = bds.GetSeverity( item->GetErrorCode() );

        RC_JSON::VIOLATION violation;
        item->GetJsonViolation( violation, &unitsProvider, severity, itemMap );

        reportHead.schematic_parity.push_back( violation );
    }


    nlohmann::json saveJson = nlohmann::json( reportHead );
    jsonFileStream << std::setw( 4 ) << saveJson << std::endl;
    jsonFileStream.flush();
    jsonFileStream.close();

    return true;
}