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

#include <wx/ffile.h>
#include <wx/string.h>

#include <locale_io.h>
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
#include <json_common.h>
#include <rc_json_schema.h>
#include <widgets/report_severity.h>


ERC_REPORT::ERC_REPORT( SCHEMATIC* aSchematic, EDA_UNITS aReportUnits,
                        std::shared_ptr<RC_ITEMS_PROVIDER> aMarkersProvider ) :
        m_sch( aSchematic ),
        m_reportUnits( aReportUnits ),
        m_markersProvider( std::move( aMarkersProvider ) ),
        m_reportedSeverities( 0 )
{
    if( !m_markersProvider )
    {
        // When no provider is supplied, fall back to creating one with default severities.
        // This allows test code to get a basic report without needing to set up a provider.
        m_markersProvider = std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( m_sch );
        m_markersProvider->SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );
    }

    m_reportedSeverities = m_markersProvider->GetSeverities();
}


wxString ERC_REPORT::GetTextReport()
{
    // We need the global LOCALE_IO here in order to
    // write the report in the c-locale.
    LOCALE_IO      locale;
    UNITS_PROVIDER unitsProvider( schIUScale, m_reportUnits );

    wxString msg = wxString::Format( wxT( "ERC report (%s, Encoding UTF8)\n" ),
                                     GetISO8601CurrentDateTime() );

    msg += wxString::Format( wxT( "Report includes: %s\n" ),
                             formatSeverities( m_reportedSeverities ) );

    std::map<KIID, EDA_ITEM*> itemMap;

    int            err_count = 0;
    int            warn_count = 0;
    int            total_count = 0;
    SCH_SHEET_LIST sheetList = m_sch->BuildSheetListSortedByPageNumbers();

    sheetList.FillItemMap( itemMap );

    ERC_SETTINGS& settings = m_sch->ErcSettings();

    std::map<SCH_SHEET_PATH, std::vector<ERC_ITEM*>> orderedItems;

    for( int i = 0; i < m_markersProvider->GetCount(); ++i )
    {
        if( auto item = dynamic_cast<ERC_ITEM*>( m_markersProvider->GetItem( i ).get() ) )
        {
            if( item->MainItemHasSheetPath() )
                orderedItems[item->GetMainItemSheetPath()].emplace_back( item );
            else
                orderedItems[sheetList[0]].emplace_back( item );
        }
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        msg << wxString::Format( wxT( "\n***** Sheet %s\n" ), sheetList[i].PathHumanReadable() );

        for( ERC_ITEM* item : orderedItems[sheetList[i]] )
        {
            SEVERITY severity = settings.GetSeverity( item->GetErrorCode() );

            total_count++;

            switch( severity )
            {
            case RPT_SEVERITY_ERROR:   err_count++;  break;
            case RPT_SEVERITY_WARNING: warn_count++; break;
            default:                                 break;
            }

            msg << item->ShowReport( &unitsProvider, severity, itemMap );
        }
    }

    msg << wxString::Format( wxT( "\n ** ERC messages: %d  Errors %d  Warnings %d\n" ),
                             total_count,
                             err_count,
                             warn_count );

    msg << wxT( "\n ** Ignored checks:\n" );

    bool hasIgnored = false;

    for( const RC_ITEM& item : ERC_ITEM::GetItemsWithSeverities() )
    {
        int code = item.GetErrorCode();

        if( code > 0 && settings.GetSeverity( code ) == RPT_SEVERITY_IGNORE )
        {
            msg << wxString::Format( wxT( "    - %s\n" ), item.GetErrorMessage( false ) );
            hasIgnored = true;
        }
    }

    if( !hasIgnored )
        msg << wxT( "    - " ) << wxT( "None" ) << wxT( "\n" );

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

    // Document which severities are included in this report
    if( m_reportedSeverities & RPT_SEVERITY_ERROR )
        reportHead.included_severities.push_back( wxS( "error" ) );

    if( m_reportedSeverities & RPT_SEVERITY_WARNING )
        reportHead.included_severities.push_back( wxS( "warning" ) );

    if( m_reportedSeverities & RPT_SEVERITY_EXCLUSION )
        reportHead.included_severities.push_back( wxS( "exclusion" ) );

    SCH_SHEET_LIST sheetList = m_sch->Hierarchy();
    sheetList.FillItemMap( itemMap );

    ERC_SETTINGS& settings = m_sch->ErcSettings();

    std::map<SCH_SHEET_PATH, std::vector<ERC_ITEM*>> orderedItems;

    for( int i = 0; i < m_markersProvider->GetCount(); ++i )
    {
        if( auto item = dynamic_cast<ERC_ITEM*>( m_markersProvider->GetItem( i ).get() ) )
        {
            if( item->MainItemHasSheetPath() )
                orderedItems[item->GetMainItemSheetPath()].emplace_back( item );
            else
                orderedItems[sheetList[0]].emplace_back( item );
        }
    }

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        RC_JSON::ERC_SHEET jsonSheet;
        jsonSheet.path = sheetList[i].PathHumanReadable();
        jsonSheet.uuid_path = sheetList[i].Path().AsString();

        for( ERC_ITEM* item : orderedItems[sheetList[i]] )
        {
            SEVERITY severity = settings.GetSeverity( item->GetErrorCode() );

            RC_JSON::VIOLATION violation;
            item->GetJsonViolation( violation, &unitsProvider, severity, itemMap );

            jsonSheet.violations.push_back( violation );
        }

        reportHead.sheets.push_back( jsonSheet );
    }

    for( const RC_ITEM& item : ERC_ITEM::GetItemsWithSeverities() )
    {
        int code = item.GetErrorCode();

        if( code > 0 && settings.GetSeverity( code ) == RPT_SEVERITY_IGNORE )
        {
            RC_JSON::IGNORED_CHECK ignoredCheck;
            ignoredCheck.key = item.GetSettingsKey();
            ignoredCheck.description = item.GetErrorMessage( false );
            reportHead.ignored_checks.push_back( ignoredCheck );
        }
    }

    nlohmann::json saveJson = nlohmann::json( reportHead );
    jsonFileStream << std::setw( 4 ) << saveJson << std::endl;
    jsonFileStream.flush();
    jsonFileStream.close();

    return true;
}
