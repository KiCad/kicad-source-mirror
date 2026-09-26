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

#include <drill/drill_chart_template.h>

#include <algorithm>
#include <fstream>

#include <i18n_utility.h>
#include <json_common.h>

#include <wx/translation.h>


const GENERATED_TABLE_SCHEMA& DrillChartSchema()
{
    // Headings are _HKI() literals, translated only when DefaultColumn() hands one to a UI
    // or a fresh chart, so regenerating a chart under a different language cannot alter an
    // approved drawing already on disk
    static const GENERATED_TABLE_SCHEMA schema(
            { { (int) DRILL_CHART_COLUMN_ID::SYMBOL, "symbol", _HKI( "SYM" ), GENERATED_TABLE_ALIGN::CENTER, true,
                _HKI( "Symbol" ) },
              { (int) DRILL_CHART_COLUMN_ID::DRILL_DIAMETER, "drill_diameter", _HKI( "DRILL DIA" ),
                GENERATED_TABLE_ALIGN::RIGHT, true, _HKI( "Drill diameter" ) },
              { (int) DRILL_CHART_COLUMN_ID::SLOT_SIZE, "slot_size", _HKI( "SLOT W x L" ), GENERATED_TABLE_ALIGN::RIGHT,
                true, _HKI( "Slot size" ) },
              { (int) DRILL_CHART_COLUMN_ID::PLATING, "plating", _HKI( "PLATED" ), GENERATED_TABLE_ALIGN::CENTER, true,
                _HKI( "Plating" ) },
              { (int) DRILL_CHART_COLUMN_ID::OPERATION_COUNT, "operation_count", _HKI( "OPS" ),
                GENERATED_TABLE_ALIGN::RIGHT, true, _HKI( "Operation count" ) },
              { (int) DRILL_CHART_COLUMN_ID::SITE_COUNT, "site_count", _HKI( "SITE COUNT" ),
                GENERATED_TABLE_ALIGN::RIGHT, false, _HKI( "Site count" ) },
              { (int) DRILL_CHART_COLUMN_ID::LAYER_SPAN, "layer_span", _HKI( "FROM / TO" ),
                GENERATED_TABLE_ALIGN::CENTER, true, _HKI( "Layer span" ) },
              { (int) DRILL_CHART_COLUMN_ID::OPERATION, "operation", _HKI( "OPERATION" ), GENERATED_TABLE_ALIGN::LEFT,
                false, _HKI( "Operation" ) },
              { (int) DRILL_CHART_COLUMN_ID::PROTECTION, "protection", _HKI( "PROTECTION" ),
                GENERATED_TABLE_ALIGN::LEFT, false, _HKI( "Protection" ) },
              { (int) DRILL_CHART_COLUMN_ID::BACKDRILL_STUB, "backdrill_stub", _HKI( "BACKDRILL STUB" ),
                GENERATED_TABLE_ALIGN::RIGHT, false, _HKI( "Backdrill stub" ) },
              { (int) DRILL_CHART_COLUMN_ID::ASPECT_RATIO, "aspect_ratio", _HKI( "ASPECT RATIO" ),
                GENERATED_TABLE_ALIGN::RIGHT, false, _HKI( "Aspect ratio" ) },
              { (int) DRILL_CHART_COLUMN_ID::DESCRIPTION, "description", _HKI( "DESCRIPTION" ),
                GENERATED_TABLE_ALIGN::LEFT, false, _HKI( "Description" ) } },
            GENERATED_TABLE_UNITS::MM, 3 );

    return schema;
}


bool DRILL_CHART_FILTER::operator==( const DRILL_CHART_FILTER& aOther ) const
{
    return m_Plated == aOther.m_Plated && m_NonPlated == aOther.m_NonPlated
           && m_Vias == aOther.m_Vias && m_Slots == aOther.m_Slots
           && m_Backdrills == aOther.m_Backdrills && m_Castellated == aOther.m_Castellated;
}


DRILL_CHART_TEMPLATE::DRILL_CHART_TEMPLATE() :
        m_version( 1 ),
        m_units( DrillChartSchema().DefaultUnits() ),
        m_precision( DrillChartSchema().DefaultPrecision() ),
        m_showTotals( true )
{
}


DRILL_CHART_TEMPLATE DRILL_CHART_TEMPLATE::MakeDefault()
{
    DRILL_CHART_TEMPLATE tmpl;

    tmpl.SetName( wxT( "Default" ) );
    tmpl.Columns() = DrillChartSchema().Defaults();

    return tmpl;
}


bool DRILL_CHART_TEMPLATE::SaveToFile( const wxString& aPath, wxString* aError ) const
{
    nlohmann::json js;
    js["name"] = m_name.ToUTF8();
    js["version"] = m_version;
    js["units"] = GeneratedTableUnitsToken( m_units );
    js["precision"] = m_precision;
    js["show_totals"] = m_showTotals;
    js["columns"] = nlohmann::json::array();

    for( const GENERATED_TABLE_COLUMN& col : m_columns )
    {
        nlohmann::json entry;
        entry["id"] = DrillChartSchema().Token( col.m_Id );
        entry["heading"] = col.m_Heading.ToUTF8();
        entry["align"] = GeneratedTableAlignToken( col.m_Align );
        entry["width"] = col.m_Width;
        js["columns"].push_back( entry );
    }

    try
    {
        std::ofstream out( aPath.fn_str() );

        if( !out.is_open() )
        {
            if( aError )
                *aError = wxString::Format( _( "Cannot write '%s'." ), aPath );

            return false;
        }

        out << js.dump( 2 ) << std::endl;

        // failbit does not throw by default, so a full disk would otherwise leave a
        // truncated template and report success
        if( !out.good() )
        {
            if( aError )
                *aError = wxString::Format( _( "Error writing '%s'." ), aPath );

            return false;
        }
    }
    catch( const std::exception& e )
    {
        if( aError )
            *aError = wxString::FromUTF8( e.what() );

        return false;
    }

    return true;
}


bool DRILL_CHART_TEMPLATE::LoadFromFile( const wxString& aPath, wxString* aError )
{
    DRILL_CHART_TEMPLATE loaded;

    // Typed accessors inside the try. Nlohmann throws on a wrong-typed field and it would
    // otherwise escape through a dialog handler
    try
    {
        std::ifstream in( aPath.fn_str() );

        if( !in.is_open() )
        {
            if( aError )
                *aError = wxString::Format( _( "Cannot read '%s'." ), aPath );

            return false;
        }

        nlohmann::json js;
        in >> js;

        if( !js.is_object() )
        {
            if( aError )
                *aError = _( "Template is not a JSON object." );

            return false;
        }

        if( js.contains( "name" ) && js["name"].is_string() )
            loaded.m_name = wxString::FromUTF8( js["name"].get<std::string>() );

        if( js.contains( "version" ) && js["version"].is_number_integer() )
            loaded.m_version = js["version"].get<int>();

        if( js.contains( "units" ) && js["units"].is_string() )
        {
            GeneratedTableUnitsFromToken( wxString::FromUTF8( js["units"].get<std::string>() ),
                                          loaded.m_units );
        }

        if( js.contains( "precision" ) && js["precision"].is_number_integer() )
            loaded.m_precision = std::clamp( js["precision"].get<int>(), 0, 6 );

        if( js.contains( "show_totals" ) && js["show_totals"].is_boolean() )
            loaded.m_showTotals = js["show_totals"].get<bool>();

        if( js.contains( "columns" ) && js["columns"].is_array() )
        {
            for( const nlohmann::json& entry : js["columns"] )
            {
                if( !entry.is_object() || !entry.contains( "id" ) || !entry["id"].is_string() )
                    continue;

                // Skipped rather than defaulted, so a template written by a newer KiCad does
                // not silently turn an unknown column into the symbol column
                const GENERATED_TABLE_COLUMN_DEF* def = DrillChartSchema().FindToken(
                        wxString::FromUTF8( entry["id"].get<std::string>() ) );

                if( !def )
                    continue;

                GENERATED_TABLE_COLUMN col;
                col.m_Id = def->m_Id;

                if( entry.contains( "heading" ) && entry["heading"].is_string() )
                    col.m_Heading = wxString::FromUTF8( entry["heading"].get<std::string>() );

                if( entry.contains( "align" ) && entry["align"].is_string() )
                {
                    GeneratedTableAlignFromToken( wxString::FromUTF8( entry["align"].get<std::string>() ),
                                                  col.m_Align );
                }

                if( entry.contains( "width" ) && entry["width"].is_number_integer() )
                    col.m_Width = entry["width"].get<int>();

                loaded.m_columns.push_back( col );
            }
        }
    }
    catch( const std::exception& e )
    {
        if( aError )
            *aError = wxString::FromUTF8( e.what() );

        return false;
    }

    if( !DrillChartSchema().Validate( loaded.m_columns ) )
    {
        if( aError )
            *aError = _( "Template has no usable columns." );

        return false;
    }

    *this = std::move( loaded );
    return true;
}
