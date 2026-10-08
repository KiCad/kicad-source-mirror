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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <project/project_file_downgrade.h>
#include <common.h>
#include <settings/bom_settings.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include <wx/intl.h>


namespace
{
struct LEGACY_DRC_ERROR
{
    const char* m_protoName;
    const char* m_settingsKey;
    bool        m_requiresTen = false;
};

// Frozen 9.0 and 10.0 DRC_ITEM keys, ordered by the protobuf DrcErrorType values.
// The current error names cannot be lowercased to recover these legacy identifiers.
constexpr LEGACY_DRC_ERROR legacyDrcErrors[] = {
    { "DRCET_UNKNOWN", nullptr },
    { "DRCET_UNCONNECTED_ITEMS", "unconnected_items" },
    { "DRCET_SHORTING_ITEMS", "shorting_items" },
    { "DRCET_ALLOWED_ITEMS", "items_not_allowed" },
    { "DRCET_TEXT_ON_EDGECUTS", "text_on_edge_cuts" },
    { "DRCET_CLEARANCE", "clearance" },
    { "DRCET_CREEPAGE", "creepage" },
    { "DRCET_TRACKS_CROSSING", "tracks_crossing" },
    { "DRCET_EDGE_CLEARANCE", "copper_edge_clearance" },
    { "DRCET_ZONES_INTERSECT", "zones_intersect" },
    { "DRCET_ISOLATED_COPPER", "isolated_copper" },
    { "DRCET_STARVED_THERMAL", "starved_thermal" },
    { "DRCET_DANGLING_VIA", "via_dangling" },
    { "DRCET_DANGLING_TRACK", "track_dangling" },
    { "DRCET_DRILLED_HOLES_TOO_CLOSE", "hole_to_hole" },
    { "DRCET_DRILLED_HOLES_COLOCATED", "holes_co_located" },
    { "DRCET_HOLE_CLEARANCE", "hole_clearance" },
    { "DRCET_CONNECTION_WIDTH", "connection_width" },
    { "DRCET_TRACK_WIDTH", "track_width" },
    { "DRCET_TRACK_ANGLE", "track_angle" },
    { "DRCET_TRACK_SEGMENT_LENGTH", "track_segment_length" },
    { "DRCET_ANNULAR_WIDTH", "annular_width" },
    { "DRCET_DRILL_OUT_OF_RANGE", "drill_out_of_range" },
    { "DRCET_VIA_DIAMETER", "via_diameter" },
    { "DRCET_PADSTACK", "padstack" },
    { "DRCET_PADSTACK_INVALID", "padstack_invalid" },
    { "DRCET_MICROVIA_DRILL_OUT_OF_RANGE", "microvia_drill_out_of_range" },
    { "DRCET_OVERLAPPING_FOOTPRINTS", "courtyards_overlap" },
    { "DRCET_MISSING_COURTYARD", "missing_courtyard" },
    { "DRCET_MALFORMED_COURTYARD", "malformed_courtyard" },
    { "DRCET_PTH_IN_COURTYARD", "pth_inside_courtyard" },
    { "DRCET_NPTH_IN_COURTYARD", "npth_inside_courtyard" },
    { "DRCET_DISABLED_LAYER_ITEM", "item_on_disabled_layer" },
    { "DRCET_INVALID_OUTLINE", "invalid_outline" },
    { "DRCET_MISSING_FOOTPRINT", "missing_footprint" },
    { "DRCET_DUPLICATE_FOOTPRINT", "duplicate_footprints" },
    { "DRCET_NET_CONFLICT", "net_conflict" },
    { "DRCET_EXTRA_FOOTPRINT", "extra_footprint" },
    { "DRCET_SCHEMATIC_PARITY", "footprint_symbol_mismatch" },
    { "DRCET_SCHEMATIC_FIELDS_PARITY", "footprint_symbol_field_mismatch", true },
    { "DRCET_FOOTPRINT_FILTERS", "footprint_filters_mismatch" },
    { "DRCET_LIB_FOOTPRINT_ISSUES", "lib_footprint_issues" },
    { "DRCET_LIB_FOOTPRINT_MISMATCH", "lib_footprint_mismatch" },
    { "DRCET_UNRESOLVED_VARIABLE", "unresolved_variable" },
    { "DRCET_ASSERTION_FAILURE", "assertion_failure" },
    { "DRCET_GENERIC_WARNING", "generic_warning" },
    { "DRCET_GENERIC_ERROR", "generic_error" },
    { "DRCET_COPPER_SLIVER", "copper_sliver" },
    { "DRCET_SILK_CLEARANCE", "silk_overlap" },
    { "DRCET_SILK_MASK_CLEARANCE", "silk_over_copper" },
    { "DRCET_SILK_EDGE_CLEARANCE", "silk_edge_clearance" },
    { "DRCET_SOLDERMASK_BRIDGE", "solder_mask_bridge" },
    { "DRCET_TEXT_HEIGHT", "text_height" },
    { "DRCET_TEXT_THICKNESS", "text_thickness" },
    { "DRCET_LENGTH_OUT_OF_RANGE", "length_out_of_range" },
    { "DRCET_SKEW_OUT_OF_RANGE", "skew_out_of_range" },
    { "DRCET_VIA_COUNT_OUT_OF_RANGE", "too_many_vias" },
    { "DRCET_DIFF_PAIR_GAP_OUT_OF_RANGE", "diff_pair_gap_out_of_range" },
    { "DRCET_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG", "diff_pair_uncoupled_length_too_long" },
    { "DRCET_FOOTPRINT", "footprint" },
    { "DRCET_FOOTPRINT_TYPE_MISMATCH", "footprint_type_mismatch" },
    { "DRCET_PAD_TH_WITH_NO_HOLE", "through_hole_pad_without_hole" },
    { "DRCET_MIRRORED_TEXT_ON_FRONT_LAYER", "mirrored_text_on_front_layer" },
    { "DRCET_NONMIRRORED_TEXT_ON_BACK_LAYER", "nonmirrored_text_on_back_layer" },
    { "DRCET_MISSING_TUNING_PROFILE", "missing_tuning_profile", true },
    { "DRCET_TUNING_PROFILE_IMPLICIT_RULES", "tuning_profile_track_geometries", true },
    { "DRCET_TRACK_ON_POST_MACHINED_LAYER", "track_on_post_machined_layer", true },
    { "DRCET_TRACK_NOT_CENTERED_ON_VIA", "track_not_centered_on_via", true },
};


constexpr LEGACY_DRC_ERROR legacyErcErrors[] = {
    { "ERCET_UNKNOWN", nullptr },
    { "ERCET_DUPLICATE_SHEET_NAME", "duplicate_sheet_names" },
    { "ERCET_ENDPOINT_OFF_GRID", "endpoint_off_grid" },
    { "ERCET_PIN_NOT_CONNECTED", "pin_not_connected" },
    { "ERCET_PIN_NOT_DRIVEN", "pin_not_driven" },
    { "ERCET_POWERPIN_NOT_DRIVEN", "power_pin_not_driven" },
    { "ERCET_HIERARCHICAL_LABEL", "hier_label_mismatch" },
    { "ERCET_NOCONNECT_CONNECTED", "no_connect_connected" },
    { "ERCET_NOCONNECT_NOT_CONNECTED", "no_connect_dangling" },
    { "ERCET_LABEL_NOT_CONNECTED", "label_dangling" },
    { "ERCET_SIMILAR_LABELS", "similar_labels" },
    { "ERCET_SIMILAR_POWER", "similar_power" },
    { "ERCET_SIMILAR_LABEL_AND_POWER", "similar_label_and_power" },
    { "ERCET_SINGLE_GLOBAL_LABEL", "single_global_label" },
    { "ERCET_SAME_LOCAL_GLOBAL_LABEL", "same_local_global_label" },
    { "ERCET_SAME_LOCAL_GLOBAL_POWER", nullptr },
    { "ERCET_DIFFERENT_UNIT_FP", "different_unit_footprint" },
    { "ERCET_MISSING_POWER_INPUT_PIN", "missing_power_pin" },
    { "ERCET_MISSING_INPUT_PIN", "missing_input_pin" },
    { "ERCET_MISSING_BIDI_PIN", "missing_bidi_pin" },
    { "ERCET_MISSING_UNIT", "missing_unit" },
    { "ERCET_DIFFERENT_UNIT_NET", "different_unit_net" },
    { "ERCET_BUS_ALIAS_CONFLICT", "bus_definition_conflict" },
    { "ERCET_DRIVER_CONFLICT", "multiple_net_names" },
    { "ERCET_BUS_ENTRY_CONFLICT", "net_not_bus_member" },
    { "ERCET_BUS_TO_BUS_CONFLICT", "bus_to_bus_conflict" },
    { "ERCET_BUS_TO_NET_CONFLICT", "bus_to_net_conflict" },
    { "ERCET_GROUND_PIN_NOT_GROUND", "ground_pin_not_ground", true },
    { "ERCET_LABEL_SINGLE_PIN", "isolated_pin_label", true },
    { "ERCET_UNRESOLVED_VARIABLE", "unresolved_variable" },
    { "ERCET_UNDEFINED_NETCLASS", "undefined_netclass" },
    { "ERCET_SIMULATION_MODEL", "simulation_model_issue" },
    { "ERCET_WIRE_DANGLING", "wire_dangling" },
    { "ERCET_LIB_SYMBOL_ISSUES", "lib_symbol_issues" },
    { "ERCET_LIB_SYMBOL_MISMATCH", "lib_symbol_mismatch" },
    { "ERCET_FOOTPRINT_LINK_ISSUES", "footprint_link_issues" },
    { "ERCET_FOOTPRINT_FILTERS", "footprint_filter" },
    { "ERCET_UNANNOTATED", "unannotated" },
    { "ERCET_EXTRA_UNITS", "extra_units" },
    { "ERCET_DIFFERENT_UNIT_VALUE", "unit_value_mismatch" },
    { "ERCET_DUPLICATE_REFERENCE", "duplicate_reference" },
    { "ERCET_BUS_ENTRY_NEEDED", "bus_entry_needed" },
    { "ERCET_FOUR_WAY_JUNCTION", "four_way_junction" },
    { "ERCET_LABEL_MULTIPLE_WIRES", "label_multiple_wires" },
    { "ERCET_UNCONNECTED_WIRE_ENDPOINT", "unconnected_wire_endpoint" },
    { "ERCET_STACKED_PIN_SYNTAX", "stacked_pin_name", true },
    { "ERCET_PIN_MAP_BAD_PAD", nullptr },
    { "ERCET_PIN_MAP_UNMAPPED_PIN", nullptr },
    { "ERCET_PIN_MAP_DUPLICATE_PAD", nullptr },
    { "ERCET_PIN_MAP_STALE_PIN", nullptr },
    { "ERCET_EMPTY_LABEL_NAME", nullptr },
    { "ERCET_VARIANT_SYMBOL_INVALID", nullptr },
    { "ERCET_VARIANT_SYMBOL_INCOMPATIBLE", nullptr },
    { "ERCET_DUPLICATE_PIN_ERROR", "duplicate_pins" },
    { "ERCET_PIN_TO_PIN_WARNING", "pin_to_pin" },
    { "ERCET_PIN_TO_PIN_ERROR", "pin_to_pin" },
    { "ERCET_ANNOTATION_ACTION", nullptr },
    { "ERCET_GENERIC_WARNING", "generic-warning" },
    { "ERCET_GENERIC_ERROR", "generic-error" },
    { "ERCET_FIELD_NAME_WHITESPACE", "field_name_whitespace", true },
    { "ERCET_WIRED_IMPLICIT_POWER", nullptr },
};


bool isUuid( const std::string& aText )
{
    if( aText.size() != 36 )
        return false;

    for( size_t i = 0; i < aText.size(); i++ )
    {
        if( i == 8 || i == 13 || i == 18 || i == 23 )
        {
            if( aText[i] != '-' )
                return false;
        }
        else if( !std::isxdigit( static_cast<unsigned char>( aText[i] ) ) )
        {
            return false;
        }
    }

    return true;
}


bool parseCoordinate( const nlohmann::json& aValue, std::string& aText, int64_t aDivisor = 1 )
{
    if( !aValue.is_number_integer() && !aValue.is_string() )
        return false;

    std::string text = aValue.is_string() ? aValue.get<std::string>() : aValue.dump();
    int64_t     value = 0;
    auto        parsed = std::from_chars( text.data(), text.data() + text.size(), value );

    if( parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || value % aDivisor != 0
        || value / aDivisor < std::numeric_limits<int32_t>::min()
        || value / aDivisor > std::numeric_limits<int32_t>::max() )
    {
        return false;
    }

    aText = std::to_string( value / aDivisor );
    return true;
}


std::string legacyLayerName( const nlohmann::json& aLayer, bool aPreTen )
{
    // Protobuf accepts both its canonical enum strings and numeric enum values.
    static constexpr const char* layers[] = {
        nullptr,     nullptr,     nullptr,     "F.Cu",    "In1.Cu",  "In2.Cu",  "In3.Cu",  "In4.Cu",    "In5.Cu",
        "In6.Cu",    "In7.Cu",    "In8.Cu",    "In9.Cu",  "In10.Cu", "In11.Cu", "In12.Cu", "In13.Cu",   "In14.Cu",
        "In15.Cu",   "In16.Cu",   "In17.Cu",   "In18.Cu", "In19.Cu", "In20.Cu", "In21.Cu", "In22.Cu",   "In23.Cu",
        "In24.Cu",   "In25.Cu",   "In26.Cu",   "In27.Cu", "In28.Cu", "In29.Cu", "In30.Cu", "B.Cu",      "B.Adhes",
        "F.Adhes",   "B.Paste",   "F.Paste",   "B.SilkS", "F.SilkS", "B.Mask",  "F.Mask",  "Dwgs.User", "Cmts.User",
        "Eco1.User", "Eco2.User", "Edge.Cuts", "Margin",  "B.CrtYd", "F.CrtYd", "B.Fab",   "F.Fab",     "User.1",
        "User.2",    "User.3",    "User.4",    "User.5",  "User.6",  "User.7",  "User.8",  "User.9",    nullptr,
        "User.10",   "User.11",   "User.12",   "User.13", "User.14", "User.15", "User.16", "User.17",   "User.18",
        "User.19",   "User.20",   "User.21",   "User.22", "User.23", "User.24", "User.25", "User.26",   "User.27",
        "User.28",   "User.29",   "User.30",   "User.31", "User.32", "User.33", "User.34", "User.35",   "User.36",
        "User.37",   "User.38",   "User.39",   "User.40", "User.41", "User.42", "User.43", "User.44",   "User.45"
    };

    int index = -1;

    if( aLayer.is_number_integer() && aLayer >= 0 && aLayer < std::size( layers ) )
        index = aLayer.get<int>();
    else if( aLayer.is_string() )
    {
        std::string name = aLayer.get<std::string>();

        for( size_t i = 3; i < std::size( layers ); i++ )
        {
            if( !layers[i] )
                continue;

            std::string proto = "BL_" + std::string( layers[i] );
            std::replace( proto.begin(), proto.end(), '.', '_' );

            if( name == proto )
                index = static_cast<int>( i );
        }
    }

    if( index < 3 || ( aPreTen && index > 61 ) || !layers[index] )
        return {};

    return layers[index];
}


bool downgradeProtoExclusion( const nlohmann::json& aEntry, bool aPreTen, nlohmann::json& aResult )
{
    if( !aEntry.is_object() || !aEntry.contains( "marker" ) || !aEntry["marker"].is_object()
        || ( aEntry.contains( "comment" ) && !aEntry["comment"].is_string() ) )
    {
        return false;
    }

    const nlohmann::json& marker = aEntry["marker"];

    if( !marker.contains( "error_type" ) )
        return false;

    const LEGACY_DRC_ERROR* error = nullptr;
    const nlohmann::json&   type = marker["error_type"];

    if( type.is_string() )
    {
        for( const LEGACY_DRC_ERROR& candidate : legacyDrcErrors )
        {
            if( type == candidate.m_protoName )
                error = &candidate;
        }
    }
    else if( type.is_number_integer() && type >= 0 && type < std::size( legacyDrcErrors ) )
    {
        error = &legacyDrcErrors[type.get<size_t>()];
    }

    if( !error || !error->m_settingsKey || ( aPreTen && error->m_requiresTen ) )
        return false;

    std::string x = "0";
    std::string y = "0";

    if( marker.contains( "position" ) )
    {
        const nlohmann::json& position = marker["position"];

        if( !position.is_object() || ( position.contains( "x_nm" ) && !parseCoordinate( position["x_nm"], x ) )
            || ( position.contains( "y_nm" ) && !parseCoordinate( position["y_nm"], y ) ) )
        {
            return false;
        }
    }

    std::vector<std::string> ids;

    if( marker.contains( "items" ) )
    {
        if( !marker["items"].is_array() || marker["items"].size() > 2 )
            return false;

        for( const nlohmann::json& item : marker["items"] )
        {
            if( !item.is_object() || !item.contains( "value" ) || !item["value"].is_string()
                || !isUuid( item["value"].get<std::string>() ) )
            {
                return false;
            }

            ids.push_back( item["value"].get<std::string>() );
        }
    }

    const std::string nil = "00000000-0000-0000-0000-000000000000";
    std::string       main = ids.empty() ? nil : ids[0];
    std::string       aux = ids.size() < 2 ? nil : ids[1];
    std::string       key = error->m_settingsKey;
    std::string       value = key + '|' + x + '|' + y + '|';

    if( key == "copper_sliver" || key == "generic_warning" || key == "generic_error" || key == "starved_thermal"
        || ( key == "unconnected_items" && !aPreTen ) )
    {
        std::string layer = marker.contains( "layer" ) ? legacyLayerName( marker["layer"], aPreTen ) : "";

        if( layer.empty() )
            return false;

        if( key == "unconnected_items" )
            value += layer + "|4|" + main + '|' + aux;
        else if( key == "starved_thermal" )
            value += main + '|' + aux + '|' + layer;
        else
        {
            if( ids.size() > 1 )
                return false;

            value += main + '|' + layer;
        }
    }
    else
    {
        // The ProtoJSON form cannot distinguish a drawing-sheet unresolved variable from
        // an item-less board marker. The old empty-ID representation would change its type.
        if( key == "unresolved_variable" && ids.empty() )
            return false;

        value += main + '|' + aux;
    }

    aResult = nlohmann::json::array( { value, aEntry.value( "comment", "" ) } );
    return true;
}


bool downgradeLegacyExclusion( const nlohmann::json& aEntry, bool aPreTen, nlohmann::json& aResult )
{
    std::string marker;
    std::string comment;

    if( aEntry.is_string() )
        marker = aEntry.get<std::string>();
    else if( aEntry.is_array() && !aEntry.empty() && aEntry[0].is_string()
             && ( aEntry.size() == 1 || ( aEntry.size() == 2 && aEntry[1].is_string() ) ) )
    {
        marker = aEntry[0].get<std::string>();

        if( aEntry.size() == 2 )
            comment = aEntry[1].get<std::string>();
    }
    else
        return false;

    std::vector<std::string> parts;
    size_t                   start = 0;

    for( size_t end = marker.find( '|' ); end != std::string::npos; end = marker.find( '|', start ) )
    {
        parts.push_back( marker.substr( start, end - start ) );
        start = end + 1;
    }

    parts.push_back( marker.substr( start ) );

    if( parts.size() < 5 )
        return false;

    bool supported = parts[0] == "hole_near_hole";

    for( const LEGACY_DRC_ERROR& error : legacyDrcErrors )
    {
        if( error.m_settingsKey && parts[0] == error.m_settingsKey && !( aPreTen && error.m_requiresTen ) )
            supported = true;
    }

    std::string coordinate;

    if( !supported || !parseCoordinate( parts[1], coordinate ) || !parseCoordinate( parts[2], coordinate ) )
        return false;

    if( parts[0] == "unconnected_items" && parts.size() == 7 )
    {
        if( !isUuid( parts[5] ) || !isUuid( parts[6] ) )
            return false;

        if( aPreTen )
            marker = parts[0] + '|' + parts[1] + '|' + parts[2] + '|' + parts[5] + '|' + parts[6];
    }
    else if( parts[0] == "starved_thermal" )
    {
        if( parts.size() != 6 || !isUuid( parts[3] ) || !isUuid( parts[4] ) )
            return false;
    }
    else if( parts[0] == "copper_sliver" || parts[0] == "generic_warning" || parts[0] == "generic_error" )
    {
        if( parts.size() != 5 || !isUuid( parts[3] ) )
            return false;
    }
    else if( parts.size() != 5
             || !( ( isUuid( parts[3] ) && isUuid( parts[4] ) )
                   || ( parts[0] == "unresolved_variable" && parts[3].empty() && parts[4].empty() ) ) )
    {
        return false;
    }

    aResult = nlohmann::json::array( { marker, comment } );
    return true;
}


bool legacyErcPath( const nlohmann::json& aPath, std::string& aText )
{
    if( !aPath.is_object() || !aPath.contains( "path" ) || !aPath["path"].is_array() || aPath["path"].empty() )
        return false;

    for( const nlohmann::json& part : aPath["path"] )
    {
        if( !part.is_object() || !part.contains( "value" ) || !part["value"].is_string()
            || !isUuid( part["value"].get<std::string>() ) )
        {
            return false;
        }

        aText += '/' + part["value"].get<std::string>();
    }

    return true;
}


bool ercChildNeedsTextDowngrade( const wxString& aText, const nlohmann::json& aDoc, bool aPreTen,
                                 std::set<std::string>& aVisiting )
{
    bool changed = aPreTen && ( aText.Contains( wxT( "\\${" ) ) || aText.Contains( wxT( "\\@{" ) ) );
    std::function<bool( wxString* )> scan = [&]( wxString* aToken )
    {
        const wxString token = aToken->AfterLast( ':' );
        changed |= token.StartsWith( wxT( "PROPERTY." ) ) || token == wxT( "SYMBOL_IS_POWER" )
                   || token == wxT( "SYMBOL_IS_LOCAL_POWER" );

        if( aDoc.contains( "text_variables" ) && aDoc["text_variables"].is_object() )
        {
            const nlohmann::json& variables = aDoc["text_variables"];
            const std::string     key = aToken->ToStdString( wxConvUTF8 );

            if( variables.contains( key ) && variables[key].is_string() && aVisiting.insert( key ).second )
            {
                changed |= ercChildNeedsTextDowngrade( wxString::FromUTF8( variables[key].get<std::string>() ), aDoc,
                                                       aPreTen, aVisiting );
                aVisiting.erase( key );
            }
        }

        return false;
    };

    const wxString scanned = ExpandTextVars( aText, &scan, INTERNAL );
    return changed || ( aPreTen && scanned.Contains( wxT( "@{" ) ) );
}


bool downgradeProtoErcExclusion( const nlohmann::json& aEntry, const nlohmann::json& aDoc, bool aPreTen,
                                 nlohmann::json& aResult )
{
    if( !aEntry.contains( "marker" ) || !aEntry["marker"].is_object()
        || ( aEntry.contains( "comment" ) && !aEntry["comment"].is_string() ) )
    {
        return false;
    }

    const nlohmann::json& marker = aEntry["marker"];

    if( !marker.contains( "error_type" ) )
        return false;

    const LEGACY_DRC_ERROR* error = nullptr;
    const nlohmann::json&   type = marker["error_type"];

    if( type.is_string() )
    {
        for( const LEGACY_DRC_ERROR& candidate : legacyErcErrors )
        {
            if( type == candidate.m_protoName )
                error = &candidate;
        }
    }
    else if( type.is_number_integer() && type >= 0 && type < std::size( legacyErcErrors ) )
        error = &legacyErcErrors[type.get<size_t>()];

    if( !error || !error->m_settingsKey || ( aPreTen && error->m_requiresTen ) )
        return false;

    std::string x = "0";
    std::string y = "0";

    if( marker.contains( "position" ) )
    {
        const nlohmann::json& position = marker["position"];

        // Schematic internal units are 100 nm. Non-integral conversions cannot keep identity.
        if( !position.is_object() || ( position.contains( "x_nm" ) && !parseCoordinate( position["x_nm"], x, 100 ) )
            || ( position.contains( "y_nm" ) && !parseCoordinate( position["y_nm"], y, 100 ) ) )
        {
            return false;
        }
    }

    const std::string        nil = "00000000-0000-0000-0000-000000000000";
    std::vector<std::string> ids;

    if( marker.contains( "items" ) )
    {
        if( !marker["items"].is_array() || marker["items"].size() > 2 )
            return false;

        for( const nlohmann::json& item : marker["items"] )
        {
            if( !item.is_object() || !item.contains( "value" ) || !item["value"].is_string()
                || !isUuid( item["value"].get<std::string>() ) )
            {
                return false;
            }

            ids.push_back( item["value"].get<std::string>() );
        }
    }

    std::string main = ids.empty() ? nil : ids[0];
    std::string aux = ids.size() < 2 ? nil : ids[1];
    std::string key = error->m_settingsKey;

    if( marker.contains( "child" ) )
    {
        const nlohmann::json& child = marker["child"];

        if( ( key != "generic-warning" && key != "generic-error" && key != "unresolved_variable" ) || ids.empty()
            || main == nil || !child.is_object() || !child.contains( "text_value" )
            || !child["text_value"].is_string() )
        {
            return false;
        }

        aux = child["text_value"].get<std::string>();

        std::set<std::string> visiting;

        if( aux.empty() || aux.find( '|' ) != std::string::npos || marker.contains( "aux_item_sheet_path" )
            || ercChildNeedsTextDowngrade( wxString::FromUTF8( aux ), aDoc, aPreTen, visiting ) )
            return false;
    }

    std::string value = key + '|' + x + '|' + y + '|' + main + '|' + aux;

    if( marker.contains( "aux_item_sheet_path" ) && !marker.contains( "main_item_sheet_path" ) )
        return false;

    for( const char* name : { "sheet_specific_path", "main_item_sheet_path", "aux_item_sheet_path" } )
    {
        std::string path;

        if( marker.contains( name ) && !legacyErcPath( marker[name], path ) )
            return false;

        value += '|' + path;
    }

    aResult = nlohmann::json::array( { value, aEntry.value( "comment", "" ) } );
    return true;
}


bool downgradeLegacyErcExclusion( const nlohmann::json& aEntry, const nlohmann::json& aDoc, bool aPreTen,
                                  nlohmann::json& aResult )
{
    std::string marker;
    std::string comment;

    if( aEntry.is_string() )
        marker = aEntry.get<std::string>();
    else if( aEntry.is_array() && !aEntry.empty() && aEntry[0].is_string()
             && ( aEntry.size() == 1 || ( aEntry.size() == 2 && aEntry[1].is_string() ) ) )
    {
        marker = aEntry[0].get<std::string>();

        if( aEntry.size() == 2 )
            comment = aEntry[1].get<std::string>();
    }
    else
        return false;

    std::vector<std::string> parts;
    size_t                   start = 0;

    for( size_t end = marker.find( '|' ); end != std::string::npos; end = marker.find( '|', start ) )
    {
        parts.push_back( marker.substr( start, end - start ) );
        start = end + 1;
    }

    parts.push_back( marker.substr( start ) );

    if( parts.size() != 5 && parts.size() != 8 )
        return false;

    bool supported = aPreTen && parts[0] == "global_label_dangling";

    for( const LEGACY_DRC_ERROR& error : legacyErcErrors )
    {
        if( error.m_settingsKey && parts[0] == error.m_settingsKey && !( aPreTen && error.m_requiresTen ) )
            supported = true;
    }

    std::string coordinate;

    if( !supported || !parseCoordinate( parts[1], coordinate ) || !parseCoordinate( parts[2], coordinate )
        || !isUuid( parts[3] ) )
    {
        return false;
    }

    bool childText = parts[0] == "generic-warning" || parts[0] == "generic-error" || parts[0] == "unresolved_variable";

    if( !childText && !isUuid( parts[4] ) )
        return false;

    std::set<std::string> visiting;

    if( childText && ercChildNeedsTextDowngrade( wxString::FromUTF8( parts[4] ), aDoc, aPreTen, visiting ) )
        return false;

    aResult = nlohmann::json::array( { marker, comment } );
    return true;
}


void downgradeErcSettings( nlohmann::json& aDoc, bool aPreTen, COMPATIBILITY_REPORT& aReport )
{
    if( !aDoc.contains( "erc" ) || !aDoc["erc"].is_object() )
        return;

    nlohmann::json& settings = aDoc["erc"];

    if( settings.contains( "erc_exclusions" ) && !settings["erc_exclusions"].is_array() )
    {
        if( !settings["erc_exclusions"].is_null() && !settings["erc_exclusions"].empty() )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "ERC exclusions" ),
                         _( "The target cannot represent this exclusion. It is removed." ), 1 );

        settings["erc_exclusions"] = nlohmann::json::array();
    }

    if( settings.contains( "erc_exclusions" ) && settings["erc_exclusions"].is_array() )
    {
        nlohmann::json exclusions = nlohmann::json::array();

        for( const nlohmann::json& entry : settings["erc_exclusions"] )
        {
            nlohmann::json converted;

            bool ok = entry.is_object() ? downgradeProtoErcExclusion( entry, aDoc, aPreTen, converted )
                                        : downgradeLegacyErcExclusion( entry, aDoc, aPreTen, converted );

            if( ok )
                exclusions.push_back( std::move( converted ) );
            else
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "ERC exclusions" ),
                             _( "The target cannot represent this exclusion. It is removed." ), 1 );
        }

        settings["erc_exclusions"] = std::move( exclusions );
    }

    if( settings.contains( "meta" ) && settings["meta"].is_object() )
        settings["meta"]["version"] = 0;
}


bool hasNonDefaultSettings( const nlohmann::json& aSettings, const nlohmann::json& aDefaults )
{
    if( aSettings == aDefaults )
        return false;

    if( !aSettings.is_object() )
        return !aSettings.empty() && !aSettings.is_null();

    for( const auto& [key, value] : aSettings.items() )
    {
        if( !aDefaults.contains( key ) || value != aDefaults[key] )
            return true;
    }

    return false;
}


bool hasConfiguredProjectSettings( const nlohmann::json& aSettings, const nlohmann::json& aDefaults,
                                   bool aSectionRoot = true )
{
    if( aSettings == aDefaults || aSettings.is_null() || aSettings.empty() )
        return false;

    if( !aSettings.is_object() || !aDefaults.is_object() )
        return true;

    for( const auto& [key, value] : aSettings.items() )
    {
        if( aSectionRoot && key == "meta" )
            continue;

        if( aDefaults.contains( key ) )
        {
            if( hasConfiguredProjectSettings( value, aDefaults[key], false ) )
                return true;
        }
        else if( !value.is_null() && !value.empty() )
        {
            return true;
        }
    }

    return false;
}


void downgradeBomSettings( nlohmann::json& aSettings, COMPATIBILITY_REPORT& aReport )
{
    auto downgradePreset = [&]( nlohmann::json& aPreset )
    {
        if( !aPreset.is_object() )
            return;

        if( aPreset.contains( "filter_scope" ) && aPreset["filter_scope"] != "reference"
            && aPreset.contains( "filter_string" ) && aPreset["filter_string"].is_string()
            && !aPreset["filter_string"].get<std::string>().empty() )
        {
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "BOM field filters" ),
                         _( "The target can only filter references. The field filter is removed." ), 1 );
            aPreset["filter_string"] = "";
        }

        aPreset.erase( "filter_scope" );
    };

    auto downgradeFormat = [&]( nlohmann::json& aFormat )
    {
        if( !aFormat.is_object() )
            return;

        if( aFormat.contains( "include_byte_order_mark" ) && aFormat["include_byte_order_mark"] == true )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "BOM byte order marks" ),
                         _( "The target cannot retain the BOM byte order mark option." ), 1 );

        aFormat.erase( "include_byte_order_mark" );
    };

    if( aSettings.contains( "bom_settings" ) )
        downgradePreset( aSettings["bom_settings"] );

    if( aSettings.contains( "bom_presets" ) && aSettings["bom_presets"].is_array() )
    {
        for( nlohmann::json& preset : aSettings["bom_presets"] )
            downgradePreset( preset );
    }

    if( aSettings.contains( "bom_fmt_settings" ) )
        downgradeFormat( aSettings["bom_fmt_settings"] );

    if( aSettings.contains( "bom_fmt_presets" ) && aSettings["bom_fmt_presets"].is_array() )
    {
        for( nlohmann::json& format : aSettings["bom_fmt_presets"] )
            downgradeFormat( format );
    }
}


void downgradeOutputSettings( nlohmann::json& aDoc, COMPATIBILITY_REPORT& aReport )
{
    if( aDoc.contains( "schematic" ) && aDoc["schematic"].is_object() )
        downgradeBomSettings( aDoc["schematic"], aReport );

    if( !aDoc.contains( "board" ) || !aDoc["board"].is_object() )
        return;

    nlohmann::json& board = aDoc["board"];

    if( board.contains( "ipc2581" ) && board["ipc2581"].is_object() )
    {
        nlohmann::json&      ipc = board["ipc2581"];
        bool                 changed = false;
        const nlohmann::json defaults = {
            { "mode", "" }, { "sections", "" }, { "custom_sections", false }, { "net_names", "" }, { "ref_des", "" }
        };

        for( const auto& [key, value] : defaults.items() )
        {
            if( ipc.contains( key ) )
            {
                changed |= ipc[key] != value;
                ipc.erase( key );
            }
        }

        if( changed )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "IPC-2581 output customization" ),
                         _( "The target cannot retain the selected output sections or naming rules." ), 1 );
    }

    if( board.contains( "idf_export" ) )
    {
        const nlohmann::json defaults = { { "units", 0 },
                                          { "origin_mode", 2 },
                                          { "user_origin_x", 0.0 },
                                          { "user_origin_y", 0.0 },
                                          { "include_unspecified", true },
                                          { "include_dnp", true },
                                          { "calculate_height_from_models", true },
                                          { "part_number_field", "Value" } };

        if( hasNonDefaultSettings( board["idf_export"], defaults ) )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "IDF output customization" ),
                         _( "The target cannot retain the selected IDF export settings." ), 1 );

        board.erase( "idf_export" );
    }

    if( board.contains( "design_settings" ) && board["design_settings"].is_object() )
    {
        nlohmann::json&      settings = board["design_settings"];
        const nlohmann::json defaults = { { "bom_export_filename", "${PROJECTNAME}.csv" },
                                          { "bom_settings", BOM_PRESET::DefaultEditing() },
                                          { "bom_fmt_settings", BOM_FMT_PRESET::CSV() },
                                          { "bom_presets", nlohmann::json::array() },
                                          { "bom_fmt_presets", nlohmann::json::array() } };
        bool                 changed = false;

        for( const auto& [key, value] : defaults.items() )
        {
            if( !settings.contains( key ) )
                continue;

            changed |= hasNonDefaultSettings( settings[key], value );
            settings.erase( key );
        }

        if( changed )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "PCB BOM output customization" ),
                         _( "The target cannot retain the PCB BOM export settings." ), 1 );
    }
}


void downgradeBoardSettings( nlohmann::json& aDoc, bool aPreTen, COMPATIBILITY_REPORT& aReport )
{
    if( !aDoc.contains( "board" ) || !aDoc["board"].is_object() || !aDoc["board"].contains( "design_settings" )
        || !aDoc["board"]["design_settings"].is_object() )
    {
        return;
    }

    nlohmann::json& settings = aDoc["board"]["design_settings"];

    if( settings.contains( "drc_exclusions" ) )
    {
        nlohmann::json exclusions = nlohmann::json::array();

        if( !settings["drc_exclusions"].is_array() )
        {
            if( !settings["drc_exclusions"].is_null() && !settings["drc_exclusions"].empty() )
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "DRC exclusions" ),
                             _( "The target cannot represent this exclusion. It is removed." ), 1 );
        }
        else
        {
            for( const nlohmann::json& entry : settings["drc_exclusions"] )
            {
                nlohmann::json converted;
                bool           ok = entry.is_object() ? downgradeProtoExclusion( entry, aPreTen, converted )
                                                      : downgradeLegacyExclusion( entry, aPreTen, converted );

                if( ok )
                    exclusions.push_back( std::move( converted ) );
                else
                    aReport.Add( DOWNGRADE_BUCKET::DROP, _( "DRC exclusions" ),
                                 _( "The target cannot represent this exclusion. It is removed." ), 1 );
            }
        }

        settings["drc_exclusions"] = std::move( exclusions );
    }

    if( settings.contains( "via_stack_presets" ) )
    {
        if( !settings["via_stack_presets"].empty() && !settings["via_stack_presets"].is_null() )
            aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Microvia stack presets" ), _( "Removed from the project file." ),
                         1 );

        settings.erase( "via_stack_presets" );
    }

    if( settings.contains( "teardrop_parameters" ) && settings["teardrop_parameters"].is_array() )
    {
        for( nlohmann::json& entry : settings["teardrop_parameters"] )
        {
            if( !entry.is_object() )
                continue;

            if( entry.contains( "td_enabled" ) && entry["td_enabled"] == true )
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Teardrop enable defaults" ),
                             _( "The target cannot retain enabled teardrop defaults." ), 1 );

            entry.erase( "td_enabled" );
        }
    }

    if( settings.contains( "meta" ) && settings["meta"].is_object() )
        settings["meta"]["version"] = 2;
}
} // namespace


void DowngradeProjectFileJson( nlohmann::json& aDoc, const DOWNGRADE_TARGET& aTarget, COMPATIBILITY_REPORT& aReport )
{
    auto knownSchema = [&]( const nlohmann::json& aSettings, int aMaximum, const wxString& aName )
    {
        if( aSettings.is_object() && aSettings.contains( "meta" ) && aSettings["meta"].is_object()
            && aSettings["meta"].contains( "version" ) && aSettings["meta"]["version"].is_number()
            && aSettings["meta"]["version"] > aMaximum )
        {
            aReport.Add( DOWNGRADE_BUCKET::BLOCK, _( "Newer project settings schema" ),
                         wxString::Format( _( "The %s schema is newer than the exporter has reviewed." ), aName ) );
            return false;
        }

        return true;
    };

    bool known = knownSchema( aDoc, 4, _( "project" ) );

    if( aDoc.contains( "net_settings" ) )
        known &= knownSchema( aDoc["net_settings"], 5, _( "net settings" ) );

    if( aDoc.contains( "tuning_profiles" ) )
        known &= knownSchema( aDoc["tuning_profiles"], 2, _( "tuning profiles" ) );

    if( aDoc.contains( "component_class_settings" ) )
        known &= knownSchema( aDoc["component_class_settings"], 0, _( "component class settings" ) );

    if( aDoc.contains( "erc" ) )
        known &= knownSchema( aDoc["erc"], 1, _( "ERC settings" ) );

    if( aDoc.contains( "board" ) && aDoc["board"].is_object() && aDoc["board"].contains( "design_settings" ) )
        known &= knownSchema( aDoc["board"]["design_settings"], 3, _( "board design settings" ) );

    if( !known )
        return;

    // KiCad 10 introduced board format 20260206. Earlier targets also lose its settings.
    bool preTen = aTarget.m_boardVersion < 20260206;
    bool tuningLoss = false;

    downgradeBoardSettings( aDoc, preTen, aReport );
    downgradeErcSettings( aDoc, preTen, aReport );
    downgradeOutputSettings( aDoc, aReport );

    if( aDoc.contains( "net_settings" ) )
    {
        nlohmann::json& net = aDoc["net_settings"];

        if( net.contains( "net_chain_netclasses" ) )
        {
            if( !net["net_chain_netclasses"].empty() && !net["net_chain_netclasses"].is_null() )
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Net chain netclass assignments" ),
                             _( "Removed from the project file." ), 1 );

            net.erase( "net_chain_netclasses" );
        }

        if( net.contains( "net_chain_classes" ) )
        {
            bool empty = net["net_chain_classes"].is_null() || net["net_chain_classes"].empty();

            net.erase( "net_chain_classes" );

            if( !empty )
            {
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Net chain classes" ), _( "Removed from the project file." ),
                             1 );
            }
        }

        if( preTen )
        {
            if( net.contains( "classes" ) && net["classes"].is_array() )
            {
                for( nlohmann::json& netclass : net["classes"] )
                {
                    if( netclass.is_object() )
                    {
                        if( netclass.contains( "tuning_profile" ) && netclass["tuning_profile"].is_string()
                            && !netclass["tuning_profile"].get_ref<const std::string&>().empty() )
                        {
                            tuningLoss = true;
                        }

                        netclass.erase( "tuning_profile" );
                    }
                }
            }

            if( net.contains( "meta" ) && net["meta"].is_object() )
                net["meta"]["version"] = 4;
        }
    }

    if( aDoc.contains( "tuning_profiles" ) )
    {
        if( preTen )
        {
            const nlohmann::json defaults = {
                { "tuning_profiles_impedance_geometric", nlohmann::json::array() },
            };
            tuningLoss |= hasConfiguredProjectSettings( aDoc["tuning_profiles"], defaults );
            aDoc.erase( "tuning_profiles" );
        }
        else
        {
            nlohmann::json& profiles = aDoc["tuning_profiles"];

            if( profiles.contains( "tuning_profiles_impedance_geometric" )
                && profiles["tuning_profiles_impedance_geometric"].is_array() )
            {
                for( nlohmann::json& entry : profiles["tuning_profiles_impedance_geometric"] )
                {
                    if( !entry.is_object() )
                        continue;

                    if( entry.contains( "frequency" ) && entry["frequency"].is_number()
                        && entry["frequency"].get<double>() != 1e9 )
                    {
                        aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Tuning profile frequencies" ),
                                     _( "The target uses its default calculation frequency." ), 1 );
                    }

                    if( entry.contains( "model_solder_mask" ) && entry["model_solder_mask"].is_boolean()
                        && entry["model_solder_mask"].get<bool>() )
                    {
                        aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Tuning profile solder mask modelling" ),
                                     _( "The target cannot include solder mask in the calculation." ), 1 );
                    }

                    entry.erase( "frequency" );
                    entry.erase( "model_solder_mask" );

                    if( entry.contains( "net_chain_bridge_prop_delay" )
                        && entry["net_chain_bridge_prop_delay"].is_number()
                        && entry["net_chain_bridge_prop_delay"].get<double>() != 0 )
                    {
                        aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Net chain bridge propagation delay" ),
                                     _( "The target cannot model delay across net chain bridges." ), 1 );
                    }

                    entry.erase( "net_chain_bridge_prop_delay" );
                }
            }

            if( profiles.contains( "meta" ) && profiles["meta"].is_object() )
                profiles["meta"]["version"] = 0;
        }
    }

    if( tuningLoss )
    {
        aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Time-domain tuning profiles" ),
                     _( "Removed saved profiles or netclass profile selections from the project file." ), 1 );
    }

    if( preTen )
    {
        // Layer preset render layers are names. Older readers skip names they do not know.
        if( aDoc.contains( "component_class_settings" ) )
        {
            const nlohmann::json defaults = {
                { "assignments", nlohmann::json::array() },
                { "sheet_component_classes", { { "enabled", false } } },
            };
            bool configured = hasConfiguredProjectSettings( aDoc["component_class_settings"], defaults );
            aDoc.erase( "component_class_settings" );

            if( configured )
            {
                aReport.Add( DOWNGRADE_BUCKET::DROP, _( "Component class assignment rules" ),
                             _( "Removed from the project file." ), 1 );
            }
        }

        if( aDoc.contains( "schematic" ) && aDoc["schematic"].is_object() )
        {
            // More than one top-level sheet blocks the schematic export before this runs.
            aDoc["schematic"].erase( "top_level_sheets" );

            // The 9.0 schematic writer carries the aliases inside every sheet instead.
            aDoc["schematic"].erase( "bus_aliases" );

            // The schematic export drops the variant registry, so the descriptions go too.
            aDoc["schematic"].erase( "variants" );
        }
    }

    // Both released targets use project schema 3. Rewind after converting nested settings.
    if( aDoc.contains( "meta" ) && aDoc["meta"].is_object() )
        aDoc["meta"]["version"] = 3;
}
