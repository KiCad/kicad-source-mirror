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

#include <wx/ffile.h>
#include <wx/string.h>

#include <sch_screen.h>
#include <sch_marker.h>
#include <schematic.h>
#include <string_utils.h>
#include <build_version.h>
#include "erc_report.h"
#include <eda_units.h>
#include <erc/erc.h>
#include <fstream>
#include <macros.h>
#include <nlohmann/json.hpp>
#include <rc_json_schema.h>


ERC_REPORT::ERC_REPORT( SCHEMATIC* aSchematic, EDA_UNITS aReportUnits ) :
        m_sch( aSchematic ),
        m_reportUnits( aReportUnits )
{
}


wxString ERC_REPORT::GetTextReport()
{
    UNITS_PROVIDER unitsProvider( schIUScale, m_reportUnits );

    wxString msg = wxString::Format( _( "ERC report (%s, Encoding UTF8)\n" ),
                                     GetISO8601CurrentDateTime() );

    std::map<KIID, EDA_ITEM*> itemMap;

    int            err_count = 0;
    int            warn_count = 0;
    int            total_count = 0;
    SCH_SHEET_LIST sheetList = m_sch->GetSheets();

    sheetList.FillItemMap( itemMap );

    ERC_SETTINGS& settings = m_sch->ErcSettings();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        msg << wxString::Format( _( "\n***** Sheet %s\n" ), sheetList[i].PathHumanReadable() );

        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            const SCH_MARKER* marker = static_cast<const SCH_MARKER*>( aItem );
            RC_ITEM*          item = marker->GetRCItem().get();
            SEVERITY          severity = settings.GetSeverity( item->GetErrorCode() );

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            switch( severity )
            {
            case RPT_SEVERITY_ERROR: err_count++; break;
            case RPT_SEVERITY_WARNING: warn_count++; break;
            default: break;
            }

            msg << marker->GetRCItem()->ShowReport( &unitsProvider, severity, itemMap );
        }
    }

    msg << wxString::Format( _( "\n ** ERC messages: %d  Errors %d  Warnings %d\n" ), total_count,
                             err_count, warn_count );

    return msg;
}


bool ERC_REPORT::WriteTextReport( const wxString& aFullFileName )
{
    wxFFile file( aFullFileName, wxT( "wt" ) );

    if( !file.IsOpened() )
        return false;

    file.Write( GetTextReport() );

    // wxFFile dtor will close the file.
    return true;
}


bool ERC_REPORT::WriteJsonReport( const wxString& aFullFileName )
{
    std::ofstream jsonFileStream( aFullFileName.fn_str() );

    UNITS_PROVIDER            unitsProvider( pcbIUScale, m_reportUnits );
    std::map<KIID, EDA_ITEM*> itemMap;

    RC_JSON::ERC_REPORT reportHead;
    wxFileName          fn( m_sch->GetFileName() );
    reportHead.$schema = "https://schemas.kicad.org/erc.v1.json";
    reportHead.source = fn.GetFullName();
    reportHead.date = GetISO8601CurrentDateTime();
    reportHead.kicad_version = GetMajorMinorPatchVersion();
    reportHead.coordinate_units = EDA_UNIT_UTILS::GetLabel( m_reportUnits );

    int            err_count = 0;
    int            warn_count = 0;
    int            total_count = 0;
    SCH_SHEET_LIST sheetList = m_sch->GetSheets();
    sheetList.FillItemMap( itemMap );

    ERC_SETTINGS& settings = m_sch->ErcSettings();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        RC_JSON::ERC_SHEET jsonSheet;
        jsonSheet.path = sheetList[i].PathHumanReadable();
        jsonSheet.uuid_path = sheetList[i].Path().AsString();

        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            const SCH_MARKER* marker = static_cast<const SCH_MARKER*>( aItem );
            RC_ITEM*          item = marker->GetRCItem().get();
            SEVERITY          severity = settings.GetSeverity( item->GetErrorCode() );

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            switch( severity )
            {
            case RPT_SEVERITY_ERROR:   err_count++;  break;
            case RPT_SEVERITY_WARNING: warn_count++; break;
            default: break;
            }

            RC_JSON::VIOLATION violation;
            marker->GetRCItem()->GetJsonViolation( violation, &unitsProvider, severity, itemMap );

            jsonSheet.violations.push_back( violation );
        }

        reportHead.sheets.push_back( jsonSheet );
    }


    nlohmann::json saveJson = nlohmann::json( reportHead );
    jsonFileStream << std::setw( 4 ) << saveJson << std::endl;
    jsonFileStream.flush();
    jsonFileStream.close();

    return true;
}