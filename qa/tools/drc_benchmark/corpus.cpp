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

#include "corpus.h"

#include <fstream>
#include <sstream>

#include <wx/filename.h>
#include <wx/utils.h>

#include <json_common.h>


bool CORPUS::IsConfigured()
{
    wxString root;

    if( !wxGetEnv( wxT( "KICAD_DRC_BENCH_CORPUS" ), &root ) || root.IsEmpty() )
        return false;

    return wxFileName::DirExists( root );
}


wxString CORPUS::Root()
{
    wxString root;

    if( !wxGetEnv( wxT( "KICAD_DRC_BENCH_CORPUS" ), &root ) )
        return wxEmptyString;

    return root;
}


/// Resolve a manifest-relative path against the corpus root and return an absolute path.
static wxString resolveAgainstRoot( const wxString& aRoot, const wxString& aRelative )
{
    if( aRelative.IsEmpty() )
        return wxEmptyString;

    wxFileName fn( aRelative );

    if( fn.IsAbsolute() )
        return fn.GetFullPath();

    wxFileName resolved( aRoot, wxEmptyString );
    resolved.SetFullName( wxEmptyString );

    wxFileName joined( aRoot + wxFileName::GetPathSeparator() + aRelative );
    joined.MakeAbsolute();

    return joined.GetFullPath();
}


bool CORPUS::Load( std::vector<CORPUS_ENTRY>& aEntries, wxString& aError )
{
    aEntries.clear();
    aError.clear();

    if( !IsConfigured() )
        return true;

    wxString   root = Root();
    wxFileName manifest( root, wxT( "corpus.json" ) );

    if( !manifest.Exists() )
    {
        aError = wxString::Format( wxT( "corpus.json not found in %s" ), root );
        return false;
    }

    std::ifstream in( manifest.GetFullPath().fn_str() );

    if( !in.is_open() )
    {
        aError = wxString::Format( wxT( "cannot open %s" ), manifest.GetFullPath() );
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();

    nlohmann::json doc;

    try
    {
        doc = nlohmann::json::parse( buffer.str() );
    }
    catch( const std::exception& e )
    {
        aError = wxString::Format( wxT( "corpus.json parse error: %s" ), e.what() );
        return false;
    }

    if( !doc.is_array() )
    {
        aError = wxT( "corpus.json must be a JSON array of entries" );
        return false;
    }

    for( const nlohmann::json& node : doc )
    {
        if( !node.is_object() || !node.contains( "board" ) )
            continue;

        CORPUS_ENTRY entry;

        auto getString = [&]( const char* aKey ) -> wxString
        {
            if( node.contains( aKey ) && node[aKey].is_string() )
                return wxString::FromUTF8( node[aKey].get<std::string>().c_str() );

            return wxEmptyString;
        };

        entry.board = resolveAgainstRoot( root, getString( "board" ) );
        entry.rules = resolveAgainstRoot( root, getString( "rules" ) );
        entry.tier = getString( "tier" );
        entry.source = getString( "source" );
        entry.notes = getString( "notes" );

        if( node.contains( "quick" ) && node["quick"].is_boolean() )
            entry.quick = node["quick"].get<bool>();

        aEntries.push_back( entry );
    }

    return true;
}


const char* ConstraintTypeName( DRC_CONSTRAINT_T aType )
{
    switch( aType )
    {
    case NULL_CONSTRAINT:                    return "null";
    case CLEARANCE_CONSTRAINT:               return "clearance";
    case CREEPAGE_CONSTRAINT:                return "creepage";
    case HOLE_CLEARANCE_CONSTRAINT:          return "hole_clearance";
    case HOLE_TO_HOLE_CONSTRAINT:            return "hole_to_hole";
    case EDGE_CLEARANCE_CONSTRAINT:          return "edge_clearance";
    case HOLE_SIZE_CONSTRAINT:               return "hole_size";
    case COURTYARD_CLEARANCE_CONSTRAINT:     return "courtyard_clearance";
    case SILK_CLEARANCE_CONSTRAINT:          return "silk_clearance";
    case TEXT_HEIGHT_CONSTRAINT:             return "text_height";
    case TEXT_THICKNESS_CONSTRAINT:          return "text_thickness";
    case TRACK_WIDTH_CONSTRAINT:             return "track_width";
    case TRACK_SEGMENT_LENGTH_CONSTRAINT:    return "track_segment_length";
    case ANNULAR_WIDTH_CONSTRAINT:           return "annular_width";
    case ZONE_CONNECTION_CONSTRAINT:         return "zone_connection";
    case THERMAL_RELIEF_GAP_CONSTRAINT:      return "thermal_relief_gap";
    case THERMAL_SPOKE_WIDTH_CONSTRAINT:     return "thermal_spoke_width";
    case MIN_RESOLVED_SPOKES_CONSTRAINT:     return "min_resolved_spokes";
    case SOLDER_MASK_EXPANSION_CONSTRAINT:   return "solder_mask_expansion";
    case SOLDER_PASTE_ABS_MARGIN_CONSTRAINT: return "solder_paste_abs_margin";
    case SOLDER_PASTE_REL_MARGIN_CONSTRAINT: return "solder_paste_rel_margin";
    case DISALLOW_CONSTRAINT:                return "disallow";
    case VIA_DIAMETER_CONSTRAINT:            return "via_diameter";
    case LENGTH_CONSTRAINT:                  return "length";
    case NET_CHAIN_LENGTH_CONSTRAINT:        return "net_chain_length";
    case NET_CHAIN_STUB_LENGTH_CONSTRAINT:   return "net_chain_stub_length";
    case NET_CHAIN_RETURN_PATH_CONSTRAINT:   return "net_chain_return_path";
    case SKEW_CONSTRAINT:                    return "skew";
    case DIFF_PAIR_GAP_CONSTRAINT:           return "diff_pair_gap";
    case MAX_UNCOUPLED_CONSTRAINT:           return "max_uncoupled";
    case DIFF_PAIR_INTRA_SKEW_CONSTRAINT:    return "diff_pair_intra_skew";
    case VIA_COUNT_CONSTRAINT:               return "via_count";
    case PHYSICAL_CLEARANCE_CONSTRAINT:      return "physical_clearance";
    case PHYSICAL_HOLE_CLEARANCE_CONSTRAINT: return "physical_hole_clearance";
    case ASSERTION_CONSTRAINT:               return "assertion";
    case CONNECTION_WIDTH_CONSTRAINT:        return "connection_width";
    case TRACK_ANGLE_CONSTRAINT:             return "track_angle";
    case VIA_DANGLING_CONSTRAINT:            return "via_dangling";
    case BRIDGED_MASK_CONSTRAINT:            return "bridged_mask";
    case SOLDER_MASK_SLIVER_CONSTRAINT:      return "solder_mask_sliver";
    }

    return "?";
}


const std::vector<DRC_CONSTRAINT_T>& AllConstraintTypes()
{
    static const std::vector<DRC_CONSTRAINT_T> types = {
        CLEARANCE_CONSTRAINT,             CREEPAGE_CONSTRAINT,
        HOLE_CLEARANCE_CONSTRAINT,        HOLE_TO_HOLE_CONSTRAINT,
        EDGE_CLEARANCE_CONSTRAINT,        HOLE_SIZE_CONSTRAINT,
        COURTYARD_CLEARANCE_CONSTRAINT,   SILK_CLEARANCE_CONSTRAINT,
        TEXT_HEIGHT_CONSTRAINT,           TEXT_THICKNESS_CONSTRAINT,
        TRACK_WIDTH_CONSTRAINT,           TRACK_SEGMENT_LENGTH_CONSTRAINT,
        ANNULAR_WIDTH_CONSTRAINT,         ZONE_CONNECTION_CONSTRAINT,
        THERMAL_RELIEF_GAP_CONSTRAINT,    THERMAL_SPOKE_WIDTH_CONSTRAINT,
        MIN_RESOLVED_SPOKES_CONSTRAINT,   SOLDER_MASK_EXPANSION_CONSTRAINT,
        SOLDER_PASTE_ABS_MARGIN_CONSTRAINT, SOLDER_PASTE_REL_MARGIN_CONSTRAINT,
        DISALLOW_CONSTRAINT,              VIA_DIAMETER_CONSTRAINT,
        LENGTH_CONSTRAINT,                NET_CHAIN_LENGTH_CONSTRAINT,
        NET_CHAIN_STUB_LENGTH_CONSTRAINT, NET_CHAIN_RETURN_PATH_CONSTRAINT,
        SKEW_CONSTRAINT,                  DIFF_PAIR_GAP_CONSTRAINT,
        MAX_UNCOUPLED_CONSTRAINT,         DIFF_PAIR_INTRA_SKEW_CONSTRAINT,
        VIA_COUNT_CONSTRAINT,             PHYSICAL_CLEARANCE_CONSTRAINT,
        PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, ASSERTION_CONSTRAINT,
        CONNECTION_WIDTH_CONSTRAINT,      TRACK_ANGLE_CONSTRAINT,
        VIA_DANGLING_CONSTRAINT,          BRIDGED_MASK_CONSTRAINT,
        SOLDER_MASK_SLIVER_CONSTRAINT
    };

    return types;
}


const std::vector<wxString>& AllPredicateNames()
{
    // Mirrors the RegisterFunc entries in pcbexpr_functions.cpp. The expensive geometry and
    // per-item lookups the optimizer watches are listed first for readability, but the order is
    // not load-bearing.
    static const std::vector<wxString> names = {
        wxT( "intersectsCourtyard" ),     wxT( "intersectsFrontCourtyard" ),
        wxT( "intersectsBackCourtyard" ), wxT( "insideCourtyard" ),
        wxT( "insideFrontCourtyard" ),    wxT( "insideBackCourtyard" ),
        wxT( "intersectsArea" ),          wxT( "enclosedByArea" ),
        wxT( "insideArea" ),              wxT( "getField" ),
        wxT( "hasNetclass" ),             wxT( "hasExactNetclass" ),
        wxT( "hasComponentClass" ),       wxT( "memberOf" ),
        wxT( "memberOfSheet" ),           wxT( "memberOfSheetOrChildren" ),
        wxT( "memberOfFootprint" ),       wxT( "memberOfGroup" ),
        wxT( "fromTo" ),                  wxT( "inDiffPair" ),
        wxT( "isCoupledDiffPair" ),       wxT( "inNetChain" ),
        wxT( "inNetChainClass" ),         wxT( "hasNetChain" ),
        wxT( "existsOnLayer" ),           wxT( "isPlated" ),
        wxT( "isMicroVia" ),              wxT( "isBlindVia" ),
        wxT( "isBuriedVia" ),             wxT( "isBlindBuriedVia" )
    };

    return names;
}


std::vector<wxString> ScanPredicatesInRules( const wxString& aRulesText )
{
    std::vector<wxString> found;

    for( const wxString& name : AllPredicateNames() )
    {
        // The grammar always invokes a predicate as name( ... ), so anchor on the open paren to
        // avoid matching a predicate name that only appears inside a comment or a longer token.
        size_t searchFrom = 0;

        while( true )
        {
            size_t pos = aRulesText.find( name, searchFrom );

            if( pos == wxString::npos )
                break;

            size_t after = pos + name.length();

            while( after < aRulesText.length() && wxIsspace( aRulesText[after] ) )
                ++after;

            if( after < aRulesText.length() && aRulesText[after] == '(' )
            {
                found.push_back( name );
                break;
            }

            searchFrom = pos + 1;
        }
    }

    return found;
}
