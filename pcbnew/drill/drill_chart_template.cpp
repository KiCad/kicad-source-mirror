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
#include <set>
#include <fstream>

#include <json_common.h>

#include <wx/translation.h>


bool DRILL_CHART_COLUMN::operator==( const DRILL_CHART_COLUMN& aOther ) const
{
    return m_Id == aOther.m_Id && m_Heading == aOther.m_Heading && m_Align == aOther.m_Align
           && m_Width == aOther.m_Width;
}


bool DRILL_CHART_FILTER::operator==( const DRILL_CHART_FILTER& aOther ) const
{
    return m_Plated == aOther.m_Plated && m_NonPlated == aOther.m_NonPlated
           && m_Vias == aOther.m_Vias && m_Slots == aOther.m_Slots
           && m_Backdrills == aOther.m_Backdrills && m_Castellated == aOther.m_Castellated;
}


DRILL_CHART_TEMPLATE::DRILL_CHART_TEMPLATE() :
        m_version( 1 ),
        m_units( DRILL_CHART_UNITS::MM ),
        m_precision( 3 ),
        m_showTotals( true )
{
}


bool ValidateDrillChartColumns( std::vector<DRILL_CHART_COLUMN>& aColumns )
{
    std::set<DRILL_CHART_COLUMN_ID> seen;
    int64_t                         total = 0;

    for( DRILL_CHART_COLUMN& col : aColumns )
    {
        // A repeat adds no information and multiplies the generated cell count
        if( !seen.insert( col.m_Id ).second )
            return false;

        col.m_Width = std::clamp( col.m_Width, 0, DRILL_CHART_MAX_COLUMN_WIDTH );
        total += col.m_Width;
    }

    return !aColumns.empty() && total <= DRILL_CHART_MAX_TOTAL_WIDTH;
}


LSET DrillDocumentationLayers()
{
    // Drops mask, paste, adhesive and silk, which print on the finished board, plus the
    // courtyard layers, which DRC reads as constraints
    LSET layers = LSET::AllNonCuMask() & ~LSET::AllBoardTechMask();

    layers.reset( Edge_Cuts );
    layers.reset( Margin );
    layers.reset( F_CrtYd );
    layers.reset( B_CrtYd );
    layers.reset( Rescue );

    return layers;
}


namespace
{

// Headings are stored as literals rather than translated at build time, so regenerating a
// chart under a different UI language cannot alter an approved drawing
const struct DEFAULT_COLUMN
{
    DRILL_CHART_COLUMN_ID id;
    const char*           heading;
    DRILL_CHART_ALIGN     align;
} g_defaultColumns[] = {
    { DRILL_CHART_COLUMN_ID::SYMBOL, "SYM", DRILL_CHART_ALIGN::CENTER },
    { DRILL_CHART_COLUMN_ID::DRILL_DIAMETER, "DRILL DIA", DRILL_CHART_ALIGN::RIGHT },
    { DRILL_CHART_COLUMN_ID::SLOT_SIZE, "SLOT W x L", DRILL_CHART_ALIGN::RIGHT },
    { DRILL_CHART_COLUMN_ID::PLATING, "PLATED", DRILL_CHART_ALIGN::CENTER },
    { DRILL_CHART_COLUMN_ID::OPERATION_COUNT, "OPS", DRILL_CHART_ALIGN::RIGHT },
    { DRILL_CHART_COLUMN_ID::LAYER_SPAN, "FROM / TO", DRILL_CHART_ALIGN::CENTER },
};

} // namespace


bool DrillChartDefaultColumn( DRILL_CHART_COLUMN_ID aId, DRILL_CHART_COLUMN& aColumn )
{
    for( const DEFAULT_COLUMN& def : g_defaultColumns )
    {
        if( def.id != aId )
            continue;

        aColumn.m_Id = def.id;
        aColumn.m_Heading = wxString::FromUTF8( def.heading );
        aColumn.m_Align = def.align;
        return true;
    }

    return false;
}


DRILL_CHART_TEMPLATE DRILL_CHART_TEMPLATE::MakeDefault()
{
    DRILL_CHART_TEMPLATE tmpl;

    tmpl.SetName( wxT( "Default" ) );

    for( const DEFAULT_COLUMN& def : g_defaultColumns )
    {
        DRILL_CHART_COLUMN col;
        DrillChartDefaultColumn( def.id, col );
        tmpl.Columns().push_back( col );
    }

    return tmpl;
}


namespace
{
struct CHART_TOKEN
{
    const char* token;
    int         value;
};

const CHART_TOKEN unitsTokens[] = {
    { "mm", (int) DRILL_CHART_UNITS::MM },
    { "inch", (int) DRILL_CHART_UNITS::INCH },
};

const CHART_TOKEN columnTokens[] = {
    { "symbol", (int) DRILL_CHART_COLUMN_ID::SYMBOL },
    { "drill_diameter", (int) DRILL_CHART_COLUMN_ID::DRILL_DIAMETER },
    { "slot_size", (int) DRILL_CHART_COLUMN_ID::SLOT_SIZE },
    { "plating", (int) DRILL_CHART_COLUMN_ID::PLATING },
    { "operation_count", (int) DRILL_CHART_COLUMN_ID::OPERATION_COUNT },
    { "site_count", (int) DRILL_CHART_COLUMN_ID::SITE_COUNT },
    { "layer_span", (int) DRILL_CHART_COLUMN_ID::LAYER_SPAN },
    { "operation", (int) DRILL_CHART_COLUMN_ID::OPERATION },
    { "protection", (int) DRILL_CHART_COLUMN_ID::PROTECTION },
    { "backdrill_stub", (int) DRILL_CHART_COLUMN_ID::BACKDRILL_STUB },
    { "aspect_ratio", (int) DRILL_CHART_COLUMN_ID::ASPECT_RATIO },
    { "description", (int) DRILL_CHART_COLUMN_ID::DESCRIPTION },
};

const CHART_TOKEN alignTokens[] = {
    { "left", (int) DRILL_CHART_ALIGN::LEFT },
    { "center", (int) DRILL_CHART_ALIGN::CENTER },
    { "right", (int) DRILL_CHART_ALIGN::RIGHT },
};

template <size_t N>
const char* chartTokenFor( const CHART_TOKEN ( &aMap )[N], int aValue )
{
    for( const CHART_TOKEN& entry : aMap )
    {
        if( entry.value == aValue )
            return entry.token;
    }

    return aMap[0].token;
}

template <size_t N>
bool chartValueFor( const CHART_TOKEN ( &aMap )[N], const wxString& aToken, int& aValue )
{
    for( const CHART_TOKEN& entry : aMap )
    {
        if( aToken.IsSameAs( wxString::FromUTF8( entry.token ) ) )
        {
            aValue = entry.value;
            return true;
        }
    }

    return false;
}
} // namespace


const char* DrillChartUnitsToken( DRILL_CHART_UNITS aUnits )
{
    return chartTokenFor( unitsTokens, (int) aUnits );
}


const char* DrillChartColumnToken( DRILL_CHART_COLUMN_ID aId )
{
    return chartTokenFor( columnTokens, (int) aId );
}


const char* DrillChartAlignToken( DRILL_CHART_ALIGN aAlign )
{
    return chartTokenFor( alignTokens, (int) aAlign );
}


bool DrillChartUnitsFromToken( const wxString& aToken, DRILL_CHART_UNITS& aUnits )
{
    int v = 0;

    if( !chartValueFor( unitsTokens, aToken, v ) )
        return false;

    aUnits = (DRILL_CHART_UNITS) v;
    return true;
}


bool DrillChartColumnFromToken( const wxString& aToken, DRILL_CHART_COLUMN_ID& aId )
{
    int v = 0;

    if( !chartValueFor( columnTokens, aToken, v ) )
        return false;

    aId = (DRILL_CHART_COLUMN_ID) v;
    return true;
}


bool DrillChartAlignFromToken( const wxString& aToken, DRILL_CHART_ALIGN& aAlign )
{
    int v = 0;

    if( !chartValueFor( alignTokens, aToken, v ) )
        return false;

    aAlign = (DRILL_CHART_ALIGN) v;
    return true;
}


bool DRILL_CHART_TEMPLATE::SaveToFile( const wxString& aPath, wxString* aError ) const
{
    nlohmann::json js;
    js["name"] = m_name.ToUTF8();
    js["version"] = m_version;
    js["units"] = DrillChartUnitsToken( m_units );
    js["precision"] = m_precision;
    js["show_totals"] = m_showTotals;
    js["columns"] = nlohmann::json::array();

    for( const DRILL_CHART_COLUMN& col : m_columns )
    {
        nlohmann::json entry;
        entry["id"] = DrillChartColumnToken( col.m_Id );
        entry["heading"] = col.m_Heading.ToUTF8();
        entry["align"] = DrillChartAlignToken( col.m_Align );
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
            DrillChartUnitsFromToken( wxString::FromUTF8( js["units"].get<std::string>() ),
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

                DRILL_CHART_COLUMN col;

                // Skipped rather than defaulted, so a template written by a newer KiCad does
                // not silently turn an unknown column into the symbol column
                if( !DrillChartColumnFromToken( wxString::FromUTF8( entry["id"].get<std::string>() ),
                                                col.m_Id ) )
                {
                    continue;
                }

                if( entry.contains( "heading" ) && entry["heading"].is_string() )
                    col.m_Heading = wxString::FromUTF8( entry["heading"].get<std::string>() );

                if( entry.contains( "align" ) && entry["align"].is_string() )
                {
                    DrillChartAlignFromToken( wxString::FromUTF8( entry["align"].get<std::string>() ),
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

    if( !ValidateDrillChartColumns( loaded.m_columns ) )
    {
        if( aError )
            *aError = _( "Template has no usable columns." );

        return false;
    }

    *this = std::move( loaded );
    return true;
}
