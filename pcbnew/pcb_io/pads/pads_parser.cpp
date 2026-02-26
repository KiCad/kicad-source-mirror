/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pads_parser.h"
#include <io/pads/pads_common.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <limits>
#include <wx/log.h>

namespace PADS_IO
{

/**
 * Expand a shortcut format string like "PRE{n1-n2}" into individual names.
 *
 * Format: PREFIX{start-end} expands to PREFIX+start, PREFIX+(start+1), ..., PREFIX+end
 * Example: "C{2-5}" expands to ["C2", "C3", "C4", "C5"]
 *
 * @param aPattern The pattern string to expand
 * @return Vector of expanded names, or single-element vector with original if not a pattern
 */
static std::vector<std::string> expandShortcutPattern( const std::string& aPattern )
{
    std::vector<std::string> result;

    size_t braceStart = aPattern.find( '{' );
    size_t braceEnd = aPattern.find( '}' );

    if( braceStart == std::string::npos || braceEnd == std::string::npos || braceEnd <= braceStart )
    {
        result.push_back( aPattern );
        return result;
    }

    std::string prefix = aPattern.substr( 0, braceStart );
    std::string suffix = ( braceEnd + 1 < aPattern.length() ) ? aPattern.substr( braceEnd + 1 ) : "";
    std::string range = aPattern.substr( braceStart + 1, braceEnd - braceStart - 1 );

    size_t dashPos = range.find( '-' );

    if( dashPos == std::string::npos )
    {
        result.push_back( aPattern );
        return result;
    }

    int start = PADS_COMMON::ParseInt( range.substr( 0, dashPos ), INT_MIN, "shortcut range" );
    int end = PADS_COMMON::ParseInt( range.substr( dashPos + 1 ), INT_MIN, "shortcut range" );

    if( start == INT_MIN || end == INT_MIN )
    {
        result.push_back( aPattern );
        return result;
    }

    static constexpr int MAX_EXPANSION = 10000;

    if( std::abs( end - start ) > MAX_EXPANSION )
    {
        wxLogWarning( wxT( "PADS Import: shortcut range {%d-%d} exceeds limit, skipped" ),
                      start, end );
        result.push_back( aPattern );
        return result;
    }

    for( int i = start; i <= end; ++i )
    {
        result.push_back( prefix + std::to_string( i ) + suffix );
    }

    return result;
}


PARSER::PARSER()
{
}

PARSER::~PARSER()
{
}

void PARSER::Parse( const wxString& aFileName )
{
    std::ifstream file( aFileName.ToStdString() );
    if( !file.is_open() )
    {
        throw std::runtime_error( "Could not open file " + aFileName.ToStdString() );
    }

    std::string line;

    // Read header
    if( !readLine( file, line ) )
    {
        throw std::runtime_error( "Empty file" );
    }

    // Parse header line format:
    // PCB files: !PADS-product-version-units[-mode][-encoding]!
    //   Example: !PADS-POWERPCB-V9.4-MILS!
    // Library files: *PADS-LIBRARY-type-Vversion*
    //   Example: *PADS-LIBRARY-PCB-DECALS-V9*
    m_is_basic_units = false;
    m_file_header.file_type = PADS_FILE_TYPE::PCB;

    // Check for library file format (uses * delimiters)
    if( line.size() > 2 && line[0] == '*' && line.back() == '*' )
    {
        std::string header = line.substr( 1, line.size() - 2 );
        m_file_header.product = header;

        // Detect library type from header
        if( header.find( "LIBRARY-LINE-ITEMS" ) != std::string::npos ||
            header.find( "LIBRARY-LINE" ) != std::string::npos )
        {
            m_file_header.file_type = PADS_FILE_TYPE::LIB_LINE;
        }
        else if( header.find( "LIBRARY-SCH-DECALS" ) != std::string::npos )
        {
            m_file_header.file_type = PADS_FILE_TYPE::LIB_SCH_DECAL;
        }
        else if( header.find( "LIBRARY-PCB-DECALS" ) != std::string::npos ||
                 header.find( "LIBRARY-DECALS" ) != std::string::npos )
        {
            m_file_header.file_type = PADS_FILE_TYPE::LIB_PCB_DECAL;
        }
        else if( header.find( "LIBRARY-PART-TYPES" ) != std::string::npos )
        {
            m_file_header.file_type = PADS_FILE_TYPE::LIB_PART_TYPE;
        }

        // Extract version from library header (e.g., V9 from *PADS-LIBRARY-PCB-DECALS-V9*)
        size_t v_pos = header.rfind( "-V" );

        if( v_pos != std::string::npos )
        {
            m_file_header.version = header.substr( v_pos + 1 );
        }

        // Library files default to mils
        m_parameters.units = UNIT_TYPE::MILS;
    }
    else if( line.size() > 2 && line[0] == '!' )
    {
        // PCB file format: !PADS-product-version-units[-mode][-encoding]! [description...]
        // Find the closing '!' to extract just the header marker, ignoring any trailing text
        size_t close_pos = line.find( '!', 1 );

        if( close_pos == std::string::npos )
            close_pos = line.size();

        std::string header = line.substr( 1, close_pos - 1 );

        // Split by '-'
        std::vector<std::string> parts;
        size_t start = 0;
        size_t pos = 0;

        while( ( pos = header.find( '-', start ) ) != std::string::npos )
        {
            parts.push_back( header.substr( start, pos - start ) );
            start = pos + 1;
        }

        parts.push_back( header.substr( start ) );

        // Parse parts: PADS, product, version, units, [mode], [encoding]
        if( parts.size() >= 4 )
        {
            // First part should be "PADS"
            m_file_header.product = parts[1];
            m_file_header.version = parts[2];
            m_file_header.units = parts[3];

            if( parts.size() >= 5 )
                m_file_header.mode = parts[4];

            if( parts.size() >= 6 )
                m_file_header.encoding = parts[5];
        }
        else if( parts.size() >= 2 )
        {
            // Simpler format
            m_file_header.product = parts[0];

            if( parts.size() >= 2 )
                m_file_header.version = parts[1];

            if( parts.size() >= 3 )
                m_file_header.units = parts[2];
        }

        // Set units based on parsed header
        if( m_file_header.units == "BASIC" )
        {
            m_is_basic_units = true;
        }
        else if( m_file_header.units == "MILS" || m_file_header.units == "MIL" )
        {
            m_parameters.units = UNIT_TYPE::MILS;
        }
        else if( m_file_header.units == "MM" || m_file_header.units == "METRIC" )
        {
            m_parameters.units = UNIT_TYPE::METRIC;
        }
        else if( m_file_header.units == "INCH" || m_file_header.units == "INCHES" )
        {
            m_parameters.units = UNIT_TYPE::INCHES;
        }
    }
    else if( line.find( "BASIC" ) != std::string::npos )
    {
        m_is_basic_units = true;
    }

    while( readLine( file, line ) )
    {
        if( line.empty() ) continue;

        if( line.rfind( "*PCB*", 0 ) == 0 )
        {
            parseSectionPCB( file );
        }
        else if( line.rfind( "*PART*", 0 ) == 0 )
        {
            parseSectionPARTS( file );
        }
        else if( line.rfind( "*NET*", 0 ) == 0 )
        {
            parseSectionNETS( file );
        }
        else if( line.rfind( "*ROUTE*", 0 ) == 0 )
        {
            parseSectionROUTES( file );
        }
        else if( line.rfind( "*TEXT*", 0 ) == 0 )
        {
            parseSectionTEXT( file );
        }
        else if( line.rfind( "*BOARD*", 0 ) == 0 )
        {
            parseSectionBOARD( file );
        }
        else if( line.rfind( "*LINES*", 0 ) == 0 )
        {
            parseSectionLINES( file );
        }
        else if( line.rfind( "*VIA*", 0 ) == 0 )
        {
            parseSectionVIA( file );
        }
        else if( line.rfind( "*POUR*", 0 ) == 0 )
        {
            parseSectionPOUR( file );
        }
        else if( line.rfind( "*PARTDECAL*", 0 ) == 0 )
        {
            parseSectionPARTDECAL( file );
        }
        else if( line.rfind( "*PARTTYPE*", 0 ) == 0 )
        {
            parseSectionPARTTYPE( file );
        }
        else if( line.rfind( "*REUSE*", 0 ) == 0 )
        {
            parseSectionREUSE( file );
        }
        else if( line.rfind( "*CLUSTER*", 0 ) == 0 )
        {
            parseSectionCLUSTER( file );
        }
        else if( line.rfind( "*JUMPER*", 0 ) == 0 )
        {
            parseSectionJUMPER( file );
        }
        else if( line.rfind( "*TESTPOINT*", 0 ) == 0 )
        {
            parseSectionTESTPOINT( file );
        }
        else if( line.rfind( "*NETCLASS*", 0 ) == 0 || line.rfind( "*NETDEF*", 0 ) == 0 )
        {
            parseSectionNETCLASS( file );
        }
        else if( line.rfind( "*DIFFPAIR*", 0 ) == 0 || line.rfind( "*DIFFPAIRS*", 0 ) == 0 )
        {
            parseSectionDIFFPAIR( file );
        }
        else if( line.rfind( "LAYER MILS", 0 ) == 0 || line.rfind( "LAYER METRIC", 0 ) == 0 )
        {
            parseSectionLAYERDEFS( file );
        }
        else if( line.rfind( "*MISC*", 0 ) == 0 )
        {
            parseSectionMISC( file );
        }
    }
}

bool PARSER::readLine( std::ifstream& aStream, std::string& aLine )
{
    if( m_pushed_line )
    {
        aLine = *m_pushed_line;
        m_pushed_line.reset();
        return true;
    }

    while( std::getline( aStream, aLine ) )
    {
        // Trim whitespace
        aLine.erase( 0, aLine.find_first_not_of( " \t\r\n" ) );
        aLine.erase( aLine.find_last_not_of( " \t\r\n" ) + 1 );

        if( aLine.empty() ) continue;
        if( aLine.rfind( "*REMARK*", 0 ) == 0 ) continue;
        return true;
    }
    return false;
}

void PARSER::pushBackLine( const std::string& aLine )
{
    m_pushed_line = aLine;
}

void PARSER::parseSectionPCB( std::ifstream& aStream )
{
    std::string line;
    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::istringstream iss( line );
        std::string token;
        iss >> token;

        if( token == "UNITS" )
        {
            std::string val;
            iss >> val;

            if( val == "0" ) m_parameters.units = UNIT_TYPE::MILS;
            else if( val == "1" ) m_parameters.units = UNIT_TYPE::METRIC;
            else if( val == "2" ) m_parameters.units = UNIT_TYPE::INCHES;
        }
        else if( token == "USERGRID" )
        {
            iss >> m_parameters.user_grid;
        }
        else if( token == "MAXIMUMLAYER" )
        {
            iss >> m_parameters.layer_count;
        }
        else if( token == "ORIGIN" )
        {
            iss >> m_parameters.origin.x >> m_parameters.origin.y;
        }
        else if( token == "THERLINEWID" )
        {
            iss >> m_parameters.thermal_line_width;
        }
        else if( token == "THERSMDWID" )
        {
            iss >> m_parameters.thermal_smd_width;
        }
        else if( token == "THERFLAGS" )
        {
            std::string flags_str;
            iss >> flags_str;

            try
            {
                m_parameters.thermal_flags = std::stoi( flags_str, nullptr, 0 );
            }
            catch( const std::exception& )
            {
                m_parameters.thermal_flags = 0;
            }
        }
        else if( token == "DRLOVERSIZE" )
        {
            iss >> m_parameters.drill_oversize;
        }
        else if( token == "VIAPSHVIA" )
        {
            iss >> m_parameters.default_signal_via;
        }
        else if( token == "STMINCLEAR" )
        {
            iss >> m_parameters.thermal_min_clearance;
        }
        else if( token == "STMINSPOKES" )
        {
            iss >> m_parameters.thermal_min_spokes;
        }
        else if( token == "MINCLEAR" )
        {
            iss >> m_design_rules.min_clearance;
        }
        else if( token == "DEFAULTCLEAR" )
        {
            iss >> m_design_rules.default_clearance;
        }
        else if( token == "MINTRACKWID" )
        {
            iss >> m_design_rules.min_track_width;
        }
        else if( token == "DEFAULTTRACKWID" )
        {
            iss >> m_design_rules.default_track_width;
        }
        else if( token == "MINVIASIZE" )
        {
            iss >> m_design_rules.min_via_size;
        }
        else if( token == "DEFAULTVIASIZE" )
        {
            iss >> m_design_rules.default_via_size;
        }
        else if( token == "MINVIADRILL" )
        {
            iss >> m_design_rules.min_via_drill;
        }
        else if( token == "DEFAULTVIADRILL" )
        {
            iss >> m_design_rules.default_via_drill;
        }
        else if( token == "HOLEHOLE" )
        {
            iss >> m_design_rules.hole_to_hole;
        }
        else if( token == "SILKCLEAR" )
        {
            iss >> m_design_rules.silk_clearance;
        }
        else if( token == "MASKCLEAR" )
        {
            iss >> m_design_rules.mask_clearance;
        }
    }
}

void PARSER::parseSectionPARTS( std::ifstream& aStream )
{
    std::string line;
    while( readLine( aStream, line ) )
    {
        if( line.find( "*REMARK*" ) == 0 )
            continue;

        if( line[0] == '*' )
        {
             pushBackLine( line );
             break;
        }

        // Skip attribute lines and other non-part lines
        if( line.rfind( "}", 0 ) == 0 ||
            line.rfind( "{", 0 ) == 0 )
        {
            continue;
        }

        std::istringstream iss( line );
        PART part;
        part.location.x = 0.0;
        part.location.y = 0.0;

        std::string name_token, parttype_string;
        iss >> name_token >> parttype_string >> part.location.x >> part.location.y >> part.rotation;

        if( iss.fail() )
        {
            continue;
        }

        // Check for shortcut format: PRE{n1-n2}
        // Example: C{2-20} with same attributes creates C2 through C20
        std::vector<std::string> expanded_names = expandShortcutPattern( name_token );
        bool is_shortcut = ( expanded_names.size() > 1 );
        part.name = expanded_names[0];

        // Check for explicit decal override using @ syntax
        // Format: PARTTYPE@DECAL_NAME means use DECAL_NAME instead of looking up from PARTTYPE
        size_t at_pos = parttype_string.find( '@' );

        if( at_pos != std::string::npos )
        {
            // Explicit decal specified after @
            part.part_type = parttype_string.substr( 0, at_pos );
            part.decal = parttype_string.substr( at_pos + 1 );
            part.explicit_decal = true;
        }
        else
        {
            // No @ - could be a direct decal name or a part type name
            // Store as decal for now, resolution happens in pcb_io_pads.cpp
            // Split on ':' to get primary and alternates (for direct decal lists)
            size_t pos = 0;
            size_t colon_pos = 0;
            bool first = true;

            while( ( colon_pos = parttype_string.find( ':', pos ) ) != std::string::npos )
            {
                std::string decal_name = parttype_string.substr( pos, colon_pos - pos );

                if( first )
                {
                    part.decal = decal_name;
                    first = false;
                }
                else
                {
                    part.alternate_decals.push_back( decal_name );
                }

                pos = colon_pos + 1;
            }

            // Handle the last (or only) decal name
            std::string last_decal = parttype_string.substr( pos );

            if( first )
            {
                part.decal = last_decal;
            }
            else
            {
                part.alternate_decals.push_back( last_decal );
            }
        }

        // Read all remaining tokens
        std::vector<std::string> tokens;
        std::string token;
        while( iss >> token )
        {
            tokens.push_back( token );
        }

        int labels = 0;

        // Process tokens for flags and label count
        // Format per REMARK: GLUE MIRROR ALT CLSTID CLSTATTR BROTHERID LABELS
        // GLUE: U (unglued) or G (glued)
        // MIRROR: N (normal/top) or M (mirrored/bottom)
        // ALT: Alternate decal index (0-based, -1 or missing = use primary)
        for( size_t i = 0; i < tokens.size(); ++i )
        {
            const std::string& t = tokens[i];

            if( t == "G" )
                part.glued = true;
            else if( t == "M" )
                part.bottom_layer = true;

            // U = unglued (default), N = normal/not-mirrored (default)
            // These are defaults so we don't need to explicitly handle them

            // Parse ALT field (token index 2 after GLUE and MIRROR)
            // ALT field is 0-indexed in PADS format
            if( i == 2 )
            {
                int alt = PADS_COMMON::ParseInt( t, -1, "PART ALT" );

                if( alt >= 0 )
                    part.alt_decal_index = alt;
            }

            // The last token is the label count
            if( i == tokens.size() - 1 )
            {
                try
                {
                    size_t pos = 0;
                    labels = std::stoi( t, &pos );

                    if( pos != t.length() )
                        labels = 0;
                }
                catch( const std::exception& )
                {
                    labels = 0;
                }
            }
        }

        // Check for optional .REUSE. line following part header
        // Format: .REUSE. instance part
        if( readLine( aStream, line ) )
        {
            if( line.find( ".REUSE." ) == 0 )
            {
                std::istringstream riss( line );
                std::string reuse_keyword;
                riss >> reuse_keyword >> part.reuse_instance >> part.reuse_part;
            }
            else
            {
                pushBackLine( line );
            }
        }

        for( int i = 0; i < labels; ++i )
        {
            ATTRIBUTE attr;
            if( !readLine( aStream, line ) ) break;

            std::stringstream iss_attr( line );
            std::string visible_str;
            std::string mirrored_str;
            std::string right_reading_str;

            // VISIBLE XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST [RIGHTREADING]
            if( iss_attr >> visible_str >> attr.x >> attr.y >> attr.orientation >> attr.level
                >> attr.height >> attr.width >> mirrored_str >> attr.hjust >> attr.vjust )
            {
                attr.visible = ( visible_str == "VALUE" || visible_str == "FULL_NAME"
                                 || visible_str == "NAME" || visible_str == "FULL_BOTH"
                                 || visible_str == "BOTH" );
                attr.mirrored = ( mirrored_str == "M" );
                iss_attr >> right_reading_str;
                attr.right_reading = ( right_reading_str == "Y" || right_reading_str == "ORTHO" );
            }

            // Line 2: Font
            if( !readLine( aStream, line ) ) break;
            attr.font_info = line;

            // Line 3: Name
            if( !readLine( aStream, line ) ) break;
            attr.name = line;

            part.attributes.push_back( attr );
        }

        // Add the first part (or only part if not a shortcut)
        m_parts.push_back( part );

        // If this was a shortcut pattern, create additional parts with same attributes
        // but different reference designators
        if( is_shortcut )
        {
            for( size_t i = 1; i < expanded_names.size(); ++i )
            {
                PART additional_part = part;
                additional_part.name = expanded_names[i];
                m_parts.push_back( additional_part );
            }
        }
    }
}

void PARSER::parseSectionNETS( std::ifstream& aStream )
{
    // Implementation for NETS
    // Format: *NET* NETNAME
    // REF.PIN REF.PIN ... [.REUSE. instance rsignal]
    // Supports shortcut format: PRE{n1-n2}.{pin1-pin2} expands to multiple pins
    std::string line;
    NET* current_net = nullptr;

    // Helper lambda to parse a pin token that may have .REUSE. suffix
    auto parsePinToken = []( const std::string& token, NET_PIN& pin ) -> bool
    {
        size_t dot_pos = token.find( '.' );

        if( dot_pos == std::string::npos )
            return false;

        pin.ref_des = token.substr( 0, dot_pos );
        pin.pin_name = token.substr( dot_pos + 1 );
        return true;
    };

    // Helper lambda to expand shortcut format tokens like U{4-8}.{7-8}
    // Returns a vector of expanded pins
    auto expandShortcutPin = []( const std::string& token ) -> std::vector<std::string>
    {
        std::vector<std::string> results;

        // Check if this contains any {n-m} range patterns
        if( token.find( '{' ) == std::string::npos )
        {
            results.push_back( token );
            return results;
        }

        // Parse the token to find all range patterns
        // Format: PREFIX{start-end}MIDDLE{start-end}SUFFIX...
        struct RangePart
        {
            std::string prefix;
            int start = 0;
            int end = 0;
            bool is_range = false;
        };

        std::vector<RangePart> parts;
        size_t pos = 0;
        std::string current_prefix;

        while( pos < token.size() )
        {
            if( token[pos] == '{' )
            {
                size_t close_pos = token.find( '}', pos );

                if( close_pos == std::string::npos )
                {
                    // Malformed, return as-is
                    results.push_back( token );
                    return results;
                }

                std::string range_str = token.substr( pos + 1, close_pos - pos - 1 );
                size_t dash_pos = range_str.find( '-' );

                if( dash_pos != std::string::npos )
                {
                    RangePart part;
                    part.prefix = current_prefix;
                    part.is_range = true;

                    part.start = PADS_COMMON::ParseInt( range_str.substr( 0, dash_pos ),
                                                        INT_MIN, "net range" );
                    part.end = PADS_COMMON::ParseInt( range_str.substr( dash_pos + 1 ),
                                                      INT_MIN, "net range" );

                    if( part.start == INT_MIN || part.end == INT_MIN )
                    {
                        results.push_back( token );
                        return results;
                    }

                    parts.push_back( part );
                    current_prefix.clear();
                }
                else
                {
                    // Single value in braces, treat as literal
                    current_prefix += range_str;
                }

                pos = close_pos + 1;
            }
            else
            {
                current_prefix += token[pos];
                pos++;
            }
        }

        // Add any trailing text as a final non-range part
        if( !current_prefix.empty() || parts.empty() )
        {
            RangePart final_part;
            final_part.prefix = current_prefix;
            final_part.is_range = false;
            final_part.start = 0;
            final_part.end = 0;
            parts.push_back( final_part );
        }

        // Generate all combinations
        // Start with empty string
        results.push_back( "" );

        for( const auto& part : parts )
        {
            std::vector<std::string> new_results;

            if( part.is_range )
            {
                for( const auto& base : results )
                {
                    int step = ( part.start <= part.end ) ? 1 : -1;

                    for( int i = part.start; step > 0 ? i <= part.end : i >= part.end; i += step )
                    {
                        new_results.push_back( base + part.prefix + std::to_string( i ) );
                    }
                }
            }
            else
            {
                for( const auto& base : results )
                {
                    new_results.push_back( base + part.prefix );
                }
            }

            results = std::move( new_results );
        }

        return results;
    };

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::istringstream iss( line );
        std::string token;
        iss >> token;

        if( token == "SIGNAL" )
        {
            NET net;
            iss >> net.name;
            m_nets.push_back( net );
            current_net = &m_nets.back();

            // Parse remaining tokens on this line
            std::string pin_token;

            while( iss >> pin_token )
            {
                // Check for .REUSE. suffix
                if( pin_token == ".REUSE." )
                {
                    // Read instance and signal for the previous pin
                    std::string instance, rsignal;

                    if( ( iss >> instance >> rsignal ) && !current_net->pins.empty() )
                    {
                        current_net->pins.back().reuse_instance = instance;
                        current_net->pins.back().reuse_signal = rsignal;
                    }

                    continue;
                }

                // Expand shortcut format and add all resulting pins
                for( const auto& expanded : expandShortcutPin( pin_token ) )
                {
                    NET_PIN pin;

                    if( parsePinToken( expanded, pin ) )
                        current_net->pins.push_back( pin );
                }
            }
        }
        else
        {
            // Continuation of pins for current net
            if( current_net )
            {
                do
                {
                    // Check for .REUSE. suffix
                    if( token == ".REUSE." )
                    {
                        std::string instance, rsignal;

                        if( ( iss >> instance >> rsignal ) && !current_net->pins.empty() )
                        {
                            current_net->pins.back().reuse_instance = instance;
                            current_net->pins.back().reuse_signal = rsignal;
                        }

                        continue;
                    }

                    // Expand shortcut format and add all resulting pins
                    for( const auto& expanded : expandShortcutPin( token ) )
                    {
                        NET_PIN pin;

                        if( parsePinToken( expanded, pin ) )
                            current_net->pins.push_back( pin );
                    }

                } while( iss >> token );
            }
        }
    }
}

void PARSER::parseSectionVIA( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            return;
        }

        std::stringstream iss( line );
        std::string name;
        double drill = 0.0;
        int stacklines = 0;

        if( !( iss >> name >> drill >> stacklines ) )
            continue;

        VIA_DEF def;
        def.name = name;
        def.drill = drill;

        // Parse optional drill_start and drill_end for blind/buried vias
        int drill_start_val = 0;
        int drill_end_val = 0;

        if( iss >> drill_start_val >> drill_end_val )
        {
            def.drill_start = drill_start_val;
            def.drill_end = drill_end_val;
        }

        int min_layer = INT_MAX;
        int max_layer = INT_MIN;

        for( int i = 0; i < stacklines; ++i )
        {
            if( !readLine( aStream, line ) )
                break;

            std::stringstream iss2( line );
            int level = 0;
            double size = 0.0;
            std::string shape;

            if( !( iss2 >> level >> size >> shape ) )
                continue;

            PAD_STACK_LAYER layer_data;
            layer_data.layer = level;
            layer_data.shape = shape;
            layer_data.sizeA = size;
            layer_data.plated = true;

            // Parse shape-specific parameters per PADS spec
            if( shape == "R" || shape == "S" )
            {
                // Round or Square pad: level size shape [corner]
                // Negative corner = chamfered, positive = rounded, zero = square
                double corner = 0;

                if( shape == "S" && ( iss2 >> corner ) )
                {
                    if( corner < 0 )
                    {
                        layer_data.corner_radius = -corner;
                        layer_data.chamfered = true;
                    }
                    else
                    {
                        layer_data.corner_radius = corner;
                    }
                }
            }
            else if( shape == "RA" || shape == "SA" )
            {
                // Anti-pad shapes: level size shape (no additional params)
                // These define clearance shapes in planes
            }
            else if( shape == "A" )
            {
                // Annular pad: level size shape inner_diameter
                double intd = 0;

                if( iss2 >> intd )
                    layer_data.inner_diameter = intd;
            }
            else if( shape == "OF" )
            {
                // Oval finger: level size shape orientation length offset
                double ori = 0, length = 0, offset = 0;

                if( iss2 >> ori >> length >> offset )
                {
                    layer_data.rotation = ori;
                    layer_data.sizeB = length;
                    layer_data.finger_offset = offset;
                }
            }
            else if( shape == "RF" )
            {
                // Rectangular finger: level size shape orientation length offset
                // Per reference parser: rotation is first, then length (becomes sizeB), then offset
                double ori = 0, length = 0, offset = 0;

                if( iss2 >> ori >> length >> offset )
                {
                    layer_data.rotation = ori;
                    layer_data.sizeB = length;
                    layer_data.finger_offset = offset;
                }
            }
            else if( shape == "RT" || shape == "ST" )
            {
                // Thermal pads: level size shape orientation inner_diam spoke_width spoke_count
                double ori = 0, intd = 0, spkwid = 0;
                int spknum = 4;

                if( iss2 >> ori >> intd >> spkwid >> spknum )
                {
                    layer_data.thermal_spoke_orientation = ori;
                    layer_data.thermal_outer_diameter = intd;
                    layer_data.thermal_spoke_width = spkwid;
                    layer_data.thermal_spoke_count = spknum;
                }
            }
            else if( shape == "O" || shape == "OC" )
            {
                // Odd shape (O) or Odd Circle (OC): level size shape
                // These use custom pad shapes defined elsewhere
                // No additional parameters, just store the shape type
            }
            else if( shape == "RC" )
            {
                // Rectangular with Corner: level size RC orientation length offset [corner]
                // Similar to RF but with optional corner radius
                double ori = 0, length = 0, offset = 0, corner = 0;

                if( iss2 >> ori >> length >> offset )
                {
                    layer_data.rotation = ori;
                    layer_data.sizeB = length;
                    layer_data.finger_offset = offset;

                    if( iss2 >> corner )
                    {
                        if( corner < 0 )
                        {
                            layer_data.corner_radius = -corner;
                            layer_data.chamfered = true;
                        }
                        else
                        {
                            layer_data.corner_radius = corner;
                        }
                    }
                }
            }

            def.stack.push_back( layer_data );

            // Map special layer numbers to copper layer indices.
            // Non-copper layers (soldermask, silkscreen, etc.) must not
            // affect via type classification or pad size.
            int effective_layer = level;

            if( level == -2 )
                effective_layer = 1;
            else if( level == -1 )
                effective_layer = m_parameters.layer_count;

            bool is_copper = ( effective_layer >= 1
                               && effective_layer <= m_parameters.layer_count );

            if( is_copper )
            {
                if( size > def.size )
                    def.size = size;

                if( effective_layer < min_layer )
                    min_layer = effective_layer;

                if( effective_layer > max_layer )
                    max_layer = effective_layer;
            }

            // PADS layer 25 = top soldermask, 28 = bottom soldermask
            if( level == 25 )
                def.has_mask_front = true;
            else if( level == 28 )
                def.has_mask_back = true;
        }

        // Determine layer span and via type
        if( min_layer <= max_layer )
        {
            def.start_layer = min_layer;
            def.end_layer = max_layer;

            int layer_count = m_parameters.layer_count;
            bool starts_at_surface = ( min_layer == 1 || max_layer == layer_count );
            bool ends_at_surface = ( max_layer == layer_count || min_layer == 1 );
            bool is_full_span = ( min_layer == 1 && max_layer == layer_count );
            int span = max_layer - min_layer;

            if( is_full_span )
            {
                def.via_type = VIA_TYPE::THROUGH;
            }
            else if( span == 1 && ( min_layer == 1 || max_layer == layer_count ) )
            {
                def.via_type = VIA_TYPE::MICROVIA;
            }
            else if( starts_at_surface || ends_at_surface )
            {
                def.via_type = VIA_TYPE::BLIND;
            }
            else
            {
                def.via_type = VIA_TYPE::BURIED;
            }
        }

        m_via_defs[name] = def;
    }

    // If no signal via was specified in the header, fall back to the first definition
    if( m_parameters.default_signal_via.empty() && !m_via_defs.empty() )
        m_parameters.default_signal_via = m_via_defs.begin()->first;
}

void PARSER::parseSectionPOUR( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            return;
        }

        // Parse Header
        // NAME TYPE XLOC YLOC PIECES FLAGS [OWNERNAME SIGNAME [HATCHGRID HATCHRAD [PRIORITY]]]
        std::stringstream iss( line );
        std::string name, type;
        double x = 0.0, y = 0.0;
        int pieces = 0, flags = 0;

        if( !( iss >> name >> type >> x >> y >> pieces >> flags ) )
            continue;

        std::string owner, signame;
        double hatchgrid = 0.0, hatchrad = 0.0;
        int priority = 0;

        if( iss >> owner >> signame )
        {
            iss >> hatchgrid >> hatchrad >> priority;
        }

        for( int i = 0; i < pieces; ++i )
        {
            if( !readLine( aStream, line ) )
                break;

            // PIECETYPE CORNERS ARCS WIDTH LEVEL [THERMALS]
            // PIECETYPE: POLY, SEG, CIRCLE, CUTOUT, CIRCUT, POCUT
            std::stringstream iss2( line );
            std::string poly_type;
            int corners = 0, arcs = 0;
            double width = 0.0;
            int level = 0;

            if( !( iss2 >> poly_type >> corners >> arcs >> width >> level ) )
                continue;

            POUR pour;
            pour.name = name;
            pour.net_name = signame;
            pour.layer = level;
            pour.priority = priority;
            pour.width = width;
            pour.is_cutout = ( poly_type == "POCUT" || poly_type == "CUTOUT"
                               || poly_type == "CIRCUT" );
            pour.owner_pour = owner;
            pour.hatch_grid = hatchgrid;
            pour.hatch_width = hatchrad;

            // The header TYPE field (POUROUT, HATOUT, VOIDOUT, PADTHERM, VIATHERM)
            // determines the record's role. The piece-level poly_type (POLY, SEG, etc.)
            // only describes the geometry shape.
            if( type == "HATOUT" )
            {
                pour.style = POUR_STYLE::HATCHED;
            }
            else if( type == "VOIDOUT" )
            {
                pour.style = POUR_STYLE::VOIDOUT;
                pour.is_cutout = true;
            }
            else if( type == "PADTHERM" )
            {
                pour.thermal_type = THERMAL_TYPE::PAD;
            }
            else if( type == "VIATHERM" )
            {
                pour.thermal_type = THERMAL_TYPE::VIA;
            }

            // Handle different piece types
            if( poly_type == "CIRCLE" || poly_type == "CIRCUT" )
            {
                // Circle piece: one line with center and radius info
                // Format: xloc yloc radius
                if( !readLine( aStream, line ) )
                    break;

                std::stringstream iss3( line );
                double cx = 0.0, cy = 0.0, radius = 0.0;

                if( iss3 >> cx >> cy >> radius )
                {
                    // Create arc representing full circle
                    ARC arc{};
                    arc.cx = x + cx;
                    arc.cy = y + cy;
                    arc.radius = radius;
                    arc.start_angle = 0.0;
                    arc.delta_angle = 360.0;
                    pour.points.emplace_back( x + cx + radius, y + cy, arc );
                }
            }
            else if( poly_type == "SEG" )
            {
                // Segment piece: pairs of points defining line segments
                for( int j = 0; j < corners; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    std::stringstream iss3( line );
                    double px = 0.0, py = 0.0;

                    if( iss3 >> px >> py )
                    {
                        pour.points.emplace_back( x + px, y + py );
                    }
                }
            }
            else
            {
                // Polygon piece types: POLY, POCUT, HATOUT, POUROUT, VOIDOUT,
                // PADTHERM, VIATHERM.
                //
                // Total data lines = corners + arcs. Lines with 4 values
                // (cx cy beginAngle sweepAngle) define an arc center and
                // angles. The following line gives the arc endpoint.
                int totalLines = corners + arcs;
                bool nextIsArcEndpoint = false;
                ARC pendingArc{};

                for( int j = 0; j < totalLines; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    std::stringstream iss3( line );
                    double px = 0.0, py = 0.0;

                    if( !( iss3 >> px >> py ) )
                        continue;

                    int angle1 = 0, angle2 = 0;

                    if( iss3 >> angle1 >> angle2 )
                    {
                        // Arc center line. The two angles are begin angle
                        // (direction from center to the previous vertex) and
                        // sweep angle, both in tenths of degrees.
                        pendingArc = ARC{};
                        pendingArc.cx = x + px;
                        pendingArc.cy = y + py;
                        pendingArc.start_angle = angle1 / 10.0;
                        pendingArc.delta_angle = angle2 / 10.0;

                        if( !pour.points.empty() )
                        {
                            double dx = pour.points.back().x - pendingArc.cx;
                            double dy = pour.points.back().y - pendingArc.cy;
                            pendingArc.radius = std::sqrt( dx * dx + dy * dy );
                        }

                        nextIsArcEndpoint = true;
                    }
                    else if( nextIsArcEndpoint )
                    {
                        if( pendingArc.radius == 0.0 )
                        {
                            double dx = ( x + px ) - pendingArc.cx;
                            double dy = ( y + py ) - pendingArc.cy;
                            pendingArc.radius = std::sqrt( dx * dx + dy * dy );
                        }

                        pour.points.emplace_back( x + px, y + py, pendingArc );
                        nextIsArcEndpoint = false;
                    }
                    else
                    {
                        pour.points.emplace_back( x + px, y + py );
                    }
                }
            }

            m_pours.push_back( pour );
        }
    }
}

void PARSER::parseSectionPARTDECAL( std::ifstream& aStream )
{
    std::string line;
    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            return;
        }

        // Header: NAME UNITS ORIX ORIY PIECES TERMINALS STACKS TEXT LABELS
        std::stringstream iss( line );
        std::string name, units;
        double orix = 0.0, oriy = 0.0;
        int pieces = 0, terminals = 0, stacks = 0, text_cnt = 0, labels = 0;

        if( !( iss >> name >> units >> orix >> oriy >> pieces >> terminals >> stacks >> text_cnt >> labels ) )
            continue;

        PART_DECAL decal;
        decal.name = name;
        decal.units = units;

        // Parse Pieces (Graphics)
        for( int i = 0; i < pieces; ++i )
        {
            if( !readLine( aStream, line ) ) break;

            // PIECETYPE CORNERS WIDTHHGHT LINESTYLE LEVEL [RESTRICTIONS]
            std::stringstream iss2( line );
            std::string type;
            int corners = 0;
            double width = 0;
            int level = 0;

            if( !( iss2 >> type >> corners >> width ) )
            {
                // Should not happen if line is valid
                continue;
            }

            // Try to read optional fields
            // Some formats have LINESTYLE LEVEL, others just LEVEL
            int val1 = 0;
            if( iss2 >> val1 )
            {
                int val2 = 0;
                if( iss2 >> val2 )
                {
                    level = val2;
                }
                else
                {
                    level = val1;
                }
            }

            DECAL_ITEM item;
            item.type = type;
            item.width = width;
            item.layer = level;

            // Handle TAG piece type (no coordinates, used for grouping copper/cutouts)
            if( type == "TAG" )
            {
                // Level is used as open/close flag: 1=open group, 0=close group
                item.is_tag_open = ( level == 1 );
                item.is_tag_close = ( level == 0 );
                decal.items.push_back( item );
                continue;
            }

            // Parse pinnum for copper pieces (COPCLS, COPOPN, COPCIR, COPCUT, COPCCO)
            // Format includes [pinnum] at the end for copper associated with a pin
            if( type.find( "COP" ) == 0 )
            {
                std::string remaining;
                std::getline( iss2, remaining );

                // Check for pinnum in remaining tokens
                std::istringstream rem_ss( remaining );
                int pinnum_val = -1;

                if( rem_ss >> pinnum_val )
                    item.pinnum = pinnum_val;
            }

            // Parse restrictions for keepout pieces (KPTCLS, KPTCIR)
            if( type.find( "KPT" ) == 0 )
            {
                std::string restrictions;

                if( iss2 >> restrictions )
                    item.restrictions = restrictions;
            }

            for( int j = 0; j < corners; ++j )
            {
                if( !readLine( aStream, line ) )
                    break;

                std::stringstream iss3( line );
                double px = 0.0, py = 0.0;

                if( !( iss3 >> px >> py ) )
                    continue;

                // Per PADS spec, arc format is: x1 y1 ab aa ax1 ay1 ax2 ay2
                // where x1,y1 = arc start point, ab = begin angle (tenths of deg),
                // aa = sweep angle (tenths of deg), ax1,ay1/ax2,ay2 = bounding box
                int startAngleTenths = 0, deltaAngleTenths = 0;
                double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                if( iss3 >> startAngleTenths >> deltaAngleTenths
                    >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                {
                    double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                    double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                    double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                    double startAngle = startAngleTenths / 10.0;
                    double deltaAngle = deltaAngleTenths / 10.0;

                    // Calculate arc start point (center + radius at start angle)
                    double startAngleRad = startAngle * M_PI / 180.0;
                    double startX = cx + radius * std::cos( startAngleRad );
                    double startY = cy + radius * std::sin( startAngleRad );

                    // Calculate arc endpoint (center + radius at end angle)
                    double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                    double endX = cx + radius * std::cos( endAngleRad );
                    double endY = cy + radius * std::sin( endAngleRad );

                    // Add arc start as a regular point (connects from previous point)
                    item.points.emplace_back( startX, startY );

                    ARC arc{};
                    arc.cx = cx;
                    arc.cy = cy;
                    arc.radius = radius;
                    arc.start_angle = startAngle;
                    arc.delta_angle = deltaAngle;

                    // Add arc end with arc data (draws the arc from start to end)
                    item.points.emplace_back( endX, endY, arc );
                }
                else
                {
                    item.points.emplace_back( px, py );
                }
            }

            decal.items.push_back( item );
        }

        // Parse Text/Labels
        // The header says how many text/labels.
        // In the example:
        // VALUE ...
        // Regular ...
        // Part Type
        // VALUE ...
        // Regular ...
        // Ref.Des.

        // Each text/label seems to take 3 lines?
        // VALUE line, Font line, Content line.

        for( int i = 0; i < text_cnt + labels; ++i )
        {
             std::string line1, line2, line3;
             if( !readLine( aStream, line1 ) ) break;
             if( !readLine( aStream, line2 ) ) break;
             if( !readLine( aStream, line3 ) ) break;

             ATTRIBUTE attr;
             // Line 1: VALUE X Y ...
             std::stringstream ss( line1 );
             std::string type_token;
             ss >> type_token;

             // First token is visibility type: VALUE, FULL_NAME, NAME, FULL_BOTH, BOTH, NONE
             std::string mirrored_str, right_reading_str;

             if( ss >> attr.x >> attr.y >> attr.orientation >> attr.level
                 >> attr.height >> attr.width >> mirrored_str >> attr.hjust >> attr.vjust )
             {
                 attr.visible = ( type_token == "VALUE" || type_token == "FULL_NAME"
                                  || type_token == "NAME" || type_token == "FULL_BOTH"
                                  || type_token == "BOTH" );
                 attr.mirrored = ( mirrored_str == "M" );
                 ss >> right_reading_str;
                 attr.right_reading = ( right_reading_str == "Y" || right_reading_str == "ORTHO" );
             }

             attr.font_info = line2;
             attr.name = line3;

             decal.attributes.push_back( attr );
        }        // Parse Terminals (T lines)
        // T-150  -110  -150  -110  1
        // Format: T X Y NMX NMY PINNUM
        // Wait, the example has: T-150 -110 -150 -110 1
        // It seems to be T<X> <Y> <NMX> <NMY> <PINNUM>
        // Note: T is attached to X coordinate sometimes? "T-150"

        for( int i = 0; i < terminals; ++i )
        {
            if( !readLine( aStream, line ) ) break;

            // Handle T prefix
            size_t t_pos = line.find( 'T' );
            if( t_pos != std::string::npos )
            {
                line[t_pos] = ' '; // Replace T with space
            }

            std::stringstream iss_t( line );
            TERMINAL term;
            double nmx = 0.0, nmy = 0.0;

            if( iss_t >> term.x >> term.y >> nmx >> nmy >> term.name )
            {
                decal.terminals.push_back( term );
            }
        }

        // Parse Stacks (PAD definitions)
        // PAD <PIN_INDEX> <STACK_LINES>
        // Then <STACK_LINES> lines of data.

        for( int i = 0; i < stacks; ++i )
        {
            if( !readLine( aStream, line ) )
                break;

            std::stringstream iss_pad( line );
            std::string token;
            int pin_idx = 0;
            int stack_lines = 0;
            iss_pad >> token >> pin_idx >> stack_lines;

            if( token != "PAD" )
                continue;

            // Parse optional P (plated) or N (non-plated) after stack_lines
            std::string plated_token;
            bool default_plated = true;
            double header_drill = 0.0;

            if( iss_pad >> plated_token )
            {
                if( plated_token == "P" )
                    default_plated = true;
                else if( plated_token == "N" )
                    default_plated = false;
                else
                {
                    header_drill = PADS_COMMON::ParseDouble( plated_token, 0.0, "pad drill" );
                }
            }

            // Parse optional slotted drill parameters from header
            double header_slot_ori = 0.0;
            double header_slot_len = 0.0;
            double header_slot_off = 0.0;

            if( iss_pad >> header_slot_ori >> header_slot_len >> header_slot_off )
            {
                // Got slotted drill from header
            }

            std::vector<PAD_STACK_LAYER> stack;

            for( int j = 0; j < stack_lines; ++j )
            {
                if( !readLine( aStream, line ) )
                    break;

                std::stringstream line_ss( line );

                int layer = 0;
                double size = 0.0;
                std::string shape;

                if( !( line_ss >> layer >> size >> shape ) )
                    continue;

                PAD_STACK_LAYER layer_data;
                layer_data.layer = layer;
                layer_data.sizeA = size;
                layer_data.sizeB = size;
                layer_data.shape = shape;
                layer_data.plated = default_plated;
                layer_data.drill = header_drill;
                layer_data.slot_orientation = header_slot_ori;
                layer_data.slot_length = header_slot_len;
                layer_data.slot_offset = header_slot_off;

                // Parse shape-specific parameters per PADS specification
                if( shape == "R" )
                {
                    // Round pad: level size R
                    // No additional shape params, may have drill after
                }
                else if( shape == "S" )
                {
                    // Square pad: level size S [corner]
                    // Negative corner = chamfered, positive = rounded, zero = square
                    double corner = 0.0;

                    if( line_ss >> corner )
                    {
                        if( corner < 0 )
                        {
                            layer_data.corner_radius = -corner;
                            layer_data.chamfered = true;
                        }
                        else
                        {
                            layer_data.corner_radius = corner;
                        }
                    }
                }
                else if( shape == "RA" || shape == "SA" )
                {
                    // Anti-pad shapes: level size RA/SA (no additional params)
                    // These define clearance shapes in plane layers
                }
                else if( shape == "A" )
                {
                    // Annular pad: level size A inner_diameter
                    double intd = 0.0;

                    if( line_ss >> intd )
                        layer_data.inner_diameter = intd;
                }
                else if( shape == "OF" )
                {
                    // Oval finger: level size OF orientation length offset
                    double ori = 0.0, length = 0.0, offset = 0.0;

                    if( line_ss >> ori >> length >> offset )
                    {
                        layer_data.rotation = ori;
                        layer_data.sizeB = length;
                        layer_data.finger_offset = offset;
                    }
                }
                else if( shape == "RF" )
                {
                    // Rectangular finger: level size RF orientation length offset [corner]
                    // Per PADS spec, corner radius exists for square and rectangular finger shapes.
                    double ori = 0.0, length = 0.0, offset = 0.0;

                    if( line_ss >> ori >> length >> offset )
                    {
                        layer_data.rotation = ori;
                        layer_data.sizeB = length;
                        layer_data.finger_offset = offset;

                        double corner = 0.0;

                        if( line_ss >> corner )
                        {
                            if( corner < 0 )
                            {
                                layer_data.corner_radius = -corner;
                                layer_data.chamfered = true;
                            }
                            else
                            {
                                layer_data.corner_radius = corner;
                            }
                        }
                    }
                }
                else if( shape == "RT" || shape == "ST" )
                {
                    // Thermal pads: level size RT/ST orientation inner_diam spoke_width spoke_count
                    double ori = 0.0, outsize = 0.0, spkwid = 0.0;
                    int spknum = 4;

                    if( line_ss >> ori >> outsize >> spkwid >> spknum )
                    {
                        layer_data.thermal_spoke_orientation = ori;
                        layer_data.thermal_outer_diameter = outsize;
                        layer_data.thermal_spoke_width = spkwid;
                        layer_data.thermal_spoke_count = spknum;
                    }
                }
                else if( shape == "O" || shape == "OC" )
                {
                    // Odd shape (O) or Odd Circle (OC): level size shape
                    // These use custom pad shapes defined elsewhere
                    // No additional parameters, just store the shape type
                }
                else if( shape == "RC" )
                {
                    // Rectangular with Corner: level size RC orientation length offset [corner]
                    // Similar to RF but with optional corner radius
                    double ori = 0.0, length = 0.0, offset = 0.0, corner = 0.0;

                    if( line_ss >> ori >> length >> offset )
                    {
                        layer_data.rotation = ori;
                        layer_data.sizeB = length;
                        layer_data.finger_offset = offset;

                        if( line_ss >> corner )
                        {
                            if( corner < 0 )
                            {
                                layer_data.corner_radius = -corner;
                                layer_data.chamfered = true;
                            }
                            else
                            {
                                layer_data.corner_radius = corner;
                            }
                        }
                    }
                }

                // For some shapes, additional tokens may be drill and plated
                // Read remaining tokens
                std::vector<std::string> remaining;
                std::string token_rem;

                while( line_ss >> token_rem )
                    remaining.push_back( token_rem );

                // Parse remaining tokens for drill, plated, and slotted drill
                if( !remaining.empty() )
                {
                    size_t idx = 0;

                    // Check for drill value (numeric)
                    double drill_val = PADS_COMMON::ParseDouble( remaining[idx],
                                                                  -1.0, "pad layer drill" );

                    if( drill_val >= 0.0 )
                    {
                        layer_data.drill = drill_val;
                        idx++;
                    }

                    // Check for plated flag
                    if( idx < remaining.size() )
                    {
                        if( remaining[idx] == "P" || remaining[idx] == "Y" )
                        {
                            layer_data.plated = true;
                            idx++;
                        }
                        else if( remaining[idx] == "N" )
                        {
                            layer_data.plated = false;
                            idx++;
                        }
                    }

                    // Check for slotted drill parameters
                    if( idx + 2 < remaining.size() )
                    {
                        layer_data.slot_orientation =
                                PADS_COMMON::ParseDouble( remaining[idx], 0.0, "slot params" );
                        layer_data.slot_length =
                                PADS_COMMON::ParseDouble( remaining[idx + 1], 0.0, "slot params" );
                        layer_data.slot_offset =
                                PADS_COMMON::ParseDouble( remaining[idx + 2], 0.0, "slot params" );
                    }
                }

                stack.push_back( layer_data );
            }

            decal.pad_stacks[pin_idx] = stack;
        }

        m_decals[name] = decal;
    }
}

void PARSER::parseSectionROUTES( std::ifstream& aStream )
{
    std::string line;
    ROUTE* current_route = nullptr;
    TRACK current_track;
    bool in_track = false;
    bool prev_is_plane_connection = false;
    ARC_POINT last_plane_connection_pt;
    int last_plane_connection_layer = 0;
    double last_plane_connection_width = 0;
    bool last_plane_on_copper = false;
    std::string default_via_name;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
             if( line.rfind( "*SIGNAL*", 0 ) == 0 )
             {
                 if( in_track && current_route )
                 {
                     current_route->tracks.push_back( current_track );
                     current_track.points.clear();
                     in_track = false;
                 }

                 prev_is_plane_connection = false;

                 std::istringstream iss( line );
                 std::string token;
                 iss >> token; // *SIGNAL*

                 std::string net_name;
                 iss >> net_name;

                 // Parse optional flags and default via
                 default_via_name.clear();

                 while( iss >> token )
                 {
                     if( !token.empty() && token.back() == ';' )
                         token.pop_back();

                     if( m_via_defs.count( token ) )
                         default_via_name = token;
                 }

                 m_routes.push_back( ROUTE() );
                 current_route = &m_routes.back();
                 current_route->net_name = net_name;
                 continue;
             }

             pushBackLine( line );
             break;
        }

        // Parse pin pair lines (start with non-digit/non-sign)
        // Format: "REF.PIN                          REF.PIN"
        // These indicate which pins are connected by the following route segments
        if( !isdigit( line[0] ) && line[0] != '-' && line[0] != '+' )
        {
            if( in_track && current_route )
            {
                current_route->tracks.push_back( current_track );
                current_track.points.clear();
                in_track = false;
            }

            prev_is_plane_connection = false;

            // Parse pin pairs from this line and add to current route
            if( current_route )
            {
                std::istringstream pin_iss( line );
                std::string pin_token;

                while( pin_iss >> pin_token )
                {
                    size_t dot_pos = pin_token.find( '.' );

                    if( dot_pos != std::string::npos )
                    {
                        NET_PIN pin;
                        pin.ref_des = pin_token.substr( 0, dot_pos );
                        pin.pin_name = pin_token.substr( dot_pos + 1 );

                        // Check if pin already exists (avoid duplicates)
                        bool found = false;

                        for( const auto& existing : current_route->pins )
                        {
                            if( existing.ref_des == pin.ref_des &&
                                existing.pin_name == pin.pin_name )
                            {
                                found = true;
                                break;
                            }
                        }

                        if( !found )
                            current_route->pins.push_back( pin );
                    }
                }
            }

            continue;
        }

        std::istringstream iss( line );
        ARC_POINT pt;
        int layer = 0;
        double width = 0.0;
        int flags = 0;
        iss >> pt.x >> pt.y >> layer >> width >> flags;

        if( iss.fail() )
            continue;

        // SEGMENTWIDTH is already in mils (not 1/256 mil units as previously thought)

        // Parse FLAGS and optional arc direction / via name
        // Format: FLAGS [ARCDIR/VIANAME] [POWER] [TEARDROP ...] [JUMPER ...]
        // ARCDIR can be CW (clockwise) or CCW (counter-clockwise)
        // POWER indicates a connection through a power/ground plane (not a discrete track)
        std::string token;
        std::string via_name;
        std::string arc_dir;

        // Per PADS spec, layer 0 means "unrouted portion" (virtual connection through
        // a plane or ratline). Only layer 0 makes a segment non-physical. Flag 0x100
        // ("plane thermal") and the THERMAL keyword describe pad/via thermal relief
        // style and do not affect whether the track segment exists.
        bool is_unrouted = ( layer == 0 );
        bool is_plane_connection = is_unrouted;
        TEARDROP teardrop;
        JUMPER_MARKER jumper;
        bool has_teardrop = false;
        bool has_jumper = false;
        bool has_power = false;

        while( iss >> token )
        {
            // Check for arc direction
            if( token == "CW" || token == "CCW" )
            {
                arc_dir = token;
                continue;
            }

            // POWER indicates a connection through a power/ground plane.
            // In PADS files, "POWER" often doubles as a via definition name.
            // THERMAL describes pad/via thermal relief style.
            if( token == "POWER" )
            {
                has_power = true;

                if( m_via_defs.count( token ) )
                    via_name = token;

                continue;
            }

            if( token == "THERMAL" )
                continue;

            // Check for via name
            if( m_via_defs.count( token ) )
            {
                via_name = token;
                continue;
            }

            // Parse TEARDROP: TEARDROP [P width length [flags]] [N width length [flags]]
            if( token == "TEARDROP" )
            {
                has_teardrop = true;
                std::string td_token;

                while( iss >> td_token )
                {
                    if( td_token == "P" )
                    {
                        teardrop.has_pad_teardrop = true;
                        iss >> teardrop.pad_width >> teardrop.pad_length;

                        // Check for optional flags (numeric)
                        std::streampos pos = iss.tellg();
                        int td_flags = 0;

                        if( iss >> td_flags )
                        {
                            teardrop.pad_flags = td_flags;
                        }
                        else
                        {
                            iss.clear();
                            iss.seekg( pos );
                        }
                    }
                    else if( td_token == "N" )
                    {
                        teardrop.has_net_teardrop = true;
                        iss >> teardrop.net_width >> teardrop.net_length;

                        std::streampos pos = iss.tellg();
                        int td_flags = 0;

                        if( iss >> td_flags )
                        {
                            teardrop.net_flags = td_flags;
                        }
                        else
                        {
                            iss.clear();
                            iss.seekg( pos );
                        }
                    }
                    else
                    {
                        // Not a teardrop param, push back for further parsing
                        // Since we can't push back easily, break and handle below
                        if( td_token == "CW" || td_token == "CCW" )
                            arc_dir = td_token;
                        else if( td_token == "POWER" )
                        {
                            has_power = true;

                            if( m_via_defs.count( td_token ) )
                                via_name = td_token;
                        }
                        else if( td_token == "THERMAL" )
                            ;
                        else if( m_via_defs.count( td_token ) )
                            via_name = td_token;

                        break;
                    }
                }

                continue;
            }

            // Parse JUMPER: jumper_name S|E
            // Jumper names are typically followed by S (start) or E (end)
            std::streampos pos = iss.tellg();
            std::string jumper_flag;

            if( iss >> jumper_flag )
            {
                if( jumper_flag == "S" || jumper_flag == "E" )
                {
                    has_jumper = true;
                    jumper.name = token;
                    jumper.is_start = ( jumper_flag == "S" );
                    jumper.x = pt.x;
                    jumper.y = pt.y;
                    continue;
                }
                else
                {
                    // Not a jumper, restore stream position
                    iss.clear();
                    iss.seekg( pos );
                }
            }
            else
            {
                iss.clear();
                iss.seekg( pos );
            }

            // Skip REUSE tokens
            if( token == "REUSE" || token == ".REUSE." )
            {
                // Skip the instance name that follows
                std::string instance;
                iss >> instance;
                continue;
            }
        }

        // If an arc direction was specified, mark this as an arc point
        // The arc center will be calculated from the previous and next points
        if( !arc_dir.empty() )
        {
            // For route arcs, we need previous point to calculate arc parameters
            // The arc goes from previous point to this point with given direction
            // PADS uses CW/CCW to indicate arc direction
            // We'll store a placeholder arc and the loader will need to compute it
            pt.is_arc = true;

            // Delta angle sign indicates direction: positive = CCW, negative = CW
            pt.arc.delta_angle = ( arc_dir == "CCW" ) ? 90.0 : -90.0;
        }

        // Per PADS spec: Layer 0 means "unrouted portion" - these are NOT physical tracks.
        // Layer 65 indicates the end of route/connection at a component pin.
        // Vias are only created when an explicit via token (STANDARDVIA, etc.) is present.

        int effective_layer = layer;
        bool is_pad_connection = ( layer == 65 );

        // Layer 0 means "unrouted" - this segment goes through a plane or is a ratline.
        // We still need an effective layer for via purposes, so use current track layer if available.
        if( is_unrouted && in_track )
            effective_layer = current_track.layer;

        // Create via at this point if a via token was present.
        // This must happen before plane connection handling since plane connection points
        // with vias (STANDARDVIA + THERMAL) would otherwise skip via creation.
        if( !via_name.empty() && current_route )
        {
            VIA via;
            via.name = via_name;
            via.location = { pt.x, pt.y };
            current_route->vias.push_back( via );
        }

        // POWER on a real copper layer means a via to the inner power/ground plane.
        // Routes often stub out to a POWER point and backtrack, with the via providing
        // the connection to the plane. Normally the POWER token itself names a via
        // definition (handled above), but if not, create an implicit via with the
        // route's default via type.
        if( has_power && via_name.empty() && !is_unrouted && !is_pad_connection && current_route )
        {
            VIA implicit_via;

            if( !default_via_name.empty() )
                implicit_via.name = default_via_name;
            else if( !m_parameters.default_signal_via.empty() )
                implicit_via.name = m_parameters.default_signal_via;

            implicit_via.location = { pt.x, pt.y };
            current_route->vias.push_back( implicit_via );
        }

        // Store teardrop if present
        if( has_teardrop && current_route )
        {
            current_route->teardrops.push_back( teardrop );
        }

        // Store jumper marker if present
        if( has_jumper && current_route )
        {
            current_route->jumpers.push_back( jumper );
        }

        // Handle plane connections (POWER or THERMAL markers)
        // Segments between consecutive plane connection points are virtual connections through
        // copper pours and should not be created as discrete tracks.
        if( is_plane_connection && prev_is_plane_connection )
        {
            // Both current and previous points are plane connections - skip this segment.
            // The connection is made through the copper pour, not a discrete track.
            // Save this point as a potential track start/end if it's on a real copper layer
            // (not layer 0 / unrouted). Copper-layer plane points are where signals transition
            // between physical tracks and the pour.
            if( !is_unrouted )
            {
                last_plane_connection_pt = pt;
                last_plane_connection_layer = effective_layer;
                last_plane_connection_width = width;
                last_plane_on_copper = true;
            }

            prev_is_plane_connection = true;
            continue;
        }

        if( is_plane_connection && !prev_is_plane_connection )
        {
            // Transitioning from track to plane - add this point to complete the track
            // then end the track (no further segments until we exit the plane)
            if( in_track )
            {
                current_track.points.push_back( pt );

                if( current_route && current_track.points.size() > 1 )
                    current_route->tracks.push_back( current_track );

                current_track.points.clear();
                in_track = false;
            }

            // Save this plane connection point as a potential track start for the next
            // non-plane segment, if it's on a real copper layer (not layer 0 / unrouted).
            // Copper-layer plane points mark where signals transition between tracks and pours.
            last_plane_on_copper = !is_unrouted;

            if( last_plane_on_copper )
            {
                last_plane_connection_pt = pt;
                last_plane_connection_layer = effective_layer;
                last_plane_connection_width = width;
            }

            prev_is_plane_connection = true;
            continue;
        }

        if( !is_plane_connection && prev_is_plane_connection )
        {
            // Transitioning from plane to track. Start a new track from the last copper-layer
            // plane point if it was on the same layer as the current point.
            if( in_track && current_route && current_track.points.size() > 1 )
                current_route->tracks.push_back( current_track );

            prev_is_plane_connection = false;

            if( is_pad_connection )
            {
                in_track = false;
                continue;
            }

            current_track.points.clear();

            if( last_plane_on_copper && last_plane_connection_layer == effective_layer )
            {
                current_track.layer = effective_layer;
                current_track.width = std::max( width, last_plane_connection_width );
                current_track.points.push_back( last_plane_connection_pt );
                current_track.points.push_back( pt );
            }
            else
            {
                // Layers differ. Create an implicit via if same location.
                // POWER vias are already handled by the central POWER handler above.
                if( !has_power && via_name.empty() && current_route && last_plane_on_copper &&
                    std::abs( pt.x - last_plane_connection_pt.x ) < 0.001 &&
                    std::abs( pt.y - last_plane_connection_pt.y ) < 0.001 )
                {
                    VIA implicit_via;

                    if( !default_via_name.empty() )
                        implicit_via.name = default_via_name;
                    else if( !m_parameters.default_signal_via.empty() )
                        implicit_via.name = m_parameters.default_signal_via;

                    implicit_via.location = { pt.x, pt.y };
                    current_route->vias.push_back( implicit_via );
                }

                current_track.layer = effective_layer;
                current_track.width = width;
                current_track.points.push_back( pt );
            }

            last_plane_on_copper = false;
            in_track = true;
            continue;
        }

        // Layer 65 is a special pad connection marker. Add the final point to terminate the
        // track at the pad, then stop building this track segment.
        if( is_pad_connection )
        {
            if( in_track && !current_track.points.empty() )
            {
                current_track.points.push_back( pt );

                if( current_route && current_track.points.size() > 1 )
                    current_route->tracks.push_back( current_track );

                current_track.points.clear();
                in_track = false;
            }

            continue;
        }

        // Normal track building (neither current nor previous is plane connection)
        prev_is_plane_connection = false;

        if( !in_track )
        {
            current_track.layer = effective_layer;
            current_track.width = width;
            current_track.points.clear();
            current_track.points.push_back( pt );
            in_track = true;
        }
        else
        {
            bool layer_changed = ( effective_layer != current_track.layer );
            bool width_changed = ( std::abs( width - current_track.width ) > 0.001 );

            if( layer_changed || width_changed )
            {
                // Check if we should connect to this point
                bool connect = true;

                if( layer_changed && via_name.empty() )
                {
                    bool same_location =
                            ( pt.x == current_track.points.back().x &&
                              pt.y == current_track.points.back().y );

                    if( same_location )
                    {
                        // Same location layer change implies an implicit via.
                        // POWER vias are already created in the central POWER handler.
                        if( !has_power && current_route )
                        {
                            VIA implicit_via;
                            implicit_via.name =
                                    default_via_name.empty() ? "STANDARDVIA" : default_via_name;
                            implicit_via.location = { pt.x, pt.y };
                            current_route->vias.push_back( implicit_via );
                        }
                    }
                    else if( !has_power )
                    {
                        // Different location without POWER, treat as jump/ratline
                        connect = false;
                    }
                    // POWER at different location: via already created, keep connected
                }

                if( connect )
                {
                    current_track.points.push_back( pt );
                }

                if( current_route )
                    current_route->tracks.push_back( current_track );

                // Start new track from current point
                ARC_POINT prev_pt = pt;

                current_track.layer = effective_layer;
                current_track.width = width;
                current_track.points.clear();
                current_track.points.push_back( prev_pt );
            }
            else
            {
                current_track.points.push_back( pt );
            }
        }
    }

    if( in_track && current_route )
    {
        current_route->tracks.push_back( current_track );
    }
}

void PARSER::parseSectionTEXT( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        // Format: X Y ORI LEVEL HEIGHT WIDTH M HJUST VJUST [NDIM] [.REUSE. instance]
        // HJUST: LEFT, CENTER, RIGHT
        // VJUST: UP, CENTER, DOWN
        std::istringstream iss( line );
        TEXT text;

        iss >> text.location.x >> text.location.y >> text.rotation >> text.layer
            >> text.height >> text.width;

        if( iss.fail() )
            continue;

        std::string mirrored;
        iss >> mirrored;
        text.mirrored = ( mirrored == "M" );

        // Parse optional hjust and vjust
        iss >> text.hjust >> text.vjust;

        // Parse optional ndim and .REUSE. instance
        std::string token;

        if( iss >> token )
        {
            if( token == ".REUSE." )
            {
                iss >> text.reuse_instance;
            }
            else
            {
                text.ndim = PADS_COMMON::ParseInt( token, 0, "text ndim" );

                if( iss >> token && token == ".REUSE." )
                {
                    iss >> text.reuse_instance;
                }
            }
        }

        // Read Font line
        // Format: fontstyle[:fontheight:fontdescent] fontface
        if( readLine( aStream, line ) )
        {
            std::istringstream fiss( line );
            std::string font_style_part;

            fiss >> font_style_part;

            size_t colon_pos = font_style_part.find( ':' );

            if( colon_pos != std::string::npos )
            {
                text.font_style = font_style_part.substr( 0, colon_pos );
                std::string remaining = font_style_part.substr( colon_pos + 1 );

                size_t second_colon = remaining.find( ':' );

                if( second_colon != std::string::npos )
                {
                    text.font_height = PADS_COMMON::ParseDouble(
                            remaining.substr( 0, second_colon ), 0.0, "font height" );
                    text.font_descent = PADS_COMMON::ParseDouble(
                            remaining.substr( second_colon + 1 ), 0.0, "font descent" );
                }
                else
                {
                    text.font_height = PADS_COMMON::ParseDouble( remaining, 0.0, "font height" );
                }
            }
            else
            {
                text.font_style = font_style_part;
            }

            // Extract font face (after angle brackets or rest of line)
            size_t bracket_start = line.find( '<' );
            size_t bracket_end = line.find( '>' );

            if( bracket_start != std::string::npos && bracket_end != std::string::npos )
            {
                text.font_face = line.substr( bracket_start + 1, bracket_end - bracket_start - 1 );
            }
            else
            {
                std::string rest;
                std::getline( fiss, rest );

                if( !rest.empty() && rest[0] == ' ' )
                    rest = rest.substr( 1 );

                text.font_face = rest;
            }
        }

        // Read Content line
        if( readLine( aStream, line ) )
        {
            // Standard PADS format uses literal backslash-n for line breaks
            size_t pos = 0;

            while( ( pos = line.find( "\\n", pos ) ) != std::string::npos )
            {
                line.replace( pos, 2, "\n" );
                pos += 1;
            }

            // EasyEDA exports (mode "250L") encode newlines as underscores
            if( m_file_header.mode == "250L" )
                std::replace( line.begin(), line.end(), '_', '\n' );

            text.content = line;
            m_texts.push_back( text );
        }
    }
}

void PARSER::parseSectionBOARD( std::ifstream& aStream )
{
    // The *BOARD* section uses the same format as LINES section with linetype=BOARD
    // Format: name BOARD xloc yloc pieces flags [text]
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::istringstream iss( line );
        std::string name, type;
        double xloc = 0.0, yloc = 0.0;
        int pieces = 0;
        iss >> name >> type >> xloc >> yloc >> pieces;

        // Parse all pieces for this board outline entry
        for( int i = 0; i < pieces; ++i )
        {
            if( !readLine( aStream, line ) )
                break;

            if( line[0] == '*' )
            {
                pushBackLine( line );
                return;
            }

            std::istringstream piss( line );
            std::string shape_type;
            int corners = 0;
            double width = 0.0;
            int linestyle = 0, level = 0;
            piss >> shape_type >> corners >> width >> linestyle >> level;

            // Handle CLOSED, OPEN, CIRCLE, BRDCLS (board cutout), BRDCIR (circular cutout)
            if( shape_type == "CLOSED" || shape_type == "OPEN" || shape_type == "BRDCLS" )
            {
                POLYLINE polyline;
                polyline.layer = 0;
                polyline.width = width;
                polyline.closed = ( shape_type == "CLOSED" || shape_type == "BRDCLS" );

                for( int j = 0; j < corners; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    if( line[0] == '*' )
                    {
                        pushBackLine( line );
                        return;
                    }

                    std::istringstream ciss( line );
                    double dx = 0.0, dy = 0.0;
                    ciss >> dx >> dy;

                    // Per PADS spec, arc format is: x1 y1 ab aa ax1 ay1 ax2 ay2
                    // where x1,y1 = arc start point; center = bounding box midpoint
                    int startAngleTenths = 0, deltaAngleTenths = 0;
                    double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                    if( ciss >> startAngleTenths >> deltaAngleTenths
                        >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                    {
                        double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                        double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                        double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                        double startAngle = startAngleTenths / 10.0;
                        double deltaAngle = deltaAngleTenths / 10.0;

                        double startAngleRad = startAngle * M_PI / 180.0;
                        double startX = cx + radius * std::cos( startAngleRad );
                        double startY = cy + radius * std::sin( startAngleRad );

                        double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                        double endX = cx + radius * std::cos( endAngleRad );
                        double endY = cy + radius * std::sin( endAngleRad );

                        polyline.points.emplace_back( xloc + startX, yloc + startY );

                        ARC arc{};
                        arc.cx = xloc + cx;
                        arc.cy = yloc + cy;
                        arc.radius = radius;
                        arc.start_angle = startAngle;
                        arc.delta_angle = deltaAngle;

                        polyline.points.emplace_back( xloc + endX, yloc + endY, arc );
                    }
                    else
                    {
                        polyline.points.emplace_back( xloc + dx, yloc + dy );
                    }
                }

                if( !polyline.points.empty() )
                    m_board_outlines.push_back( polyline );
            }
            else if( shape_type == "CIRCLE" || shape_type == "BRDCIR" )
            {
                // Circle format: 2 coordinates define opposite ends of diameter
                POLYLINE polyline;
                polyline.layer = 0;
                polyline.width = width;
                polyline.closed = true;

                double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

                if( readLine( aStream, line ) )
                {
                    std::istringstream c1( line );
                    c1 >> x1 >> y1;
                }

                if( corners >= 2 && readLine( aStream, line ) )
                {
                    std::istringstream c2( line );
                    c2 >> x2 >> y2;
                }

                // Calculate center and radius from diameter endpoints
                double cx = xloc + ( x1 + x2 ) / 2.0;
                double cy = yloc + ( y1 + y2 ) / 2.0;
                double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 ) + ( y2 - y1 ) * ( y2 - y1 ) ) / 2.0;

                // Create full circle arc
                ARC arc{};
                arc.cx = cx;
                arc.cy = cy;
                arc.radius = radius;
                arc.start_angle = 0.0;
                arc.delta_angle = 360.0;

                polyline.points.emplace_back( cx + radius, cy, arc );

                if( !polyline.points.empty() )
                    m_board_outlines.push_back( polyline );
            }
            else
            {
                // Unknown shape type, skip corners
                for( int j = 0; j < corners; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    if( line[0] == '*' )
                    {
                        pushBackLine( line );
                        return;
                    }
                }
            }
        }
    }
}

void PARSER::parseSectionLINES( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
             pushBackLine( line );
             break;
        }

        // Header format: name type xloc yloc pieces flags [text [signame]]
        std::istringstream iss( line );
        std::string name, type;
        double xloc = 0.0, yloc = 0.0;
        int pieces = 0, flags = 0, textCount = 0;
        std::string signame;

        iss >> name >> type >> xloc >> yloc >> pieces >> flags;

        // Try to read optional text count and signal name (for COPPER type).
        // Standard format: pieces flags textcount signame
        // EasyEDA format:  pieces flags signame (no text count)
        if( iss >> textCount )
        {
            iss >> signame;
        }
        else
        {
            iss.clear();
            iss >> signame;
        }

        // Check for optional .REUSE. line after header
        std::string reuse_instance, reuse_signal;

        if( readLine( aStream, line ) )
        {
            if( line.find( ".REUSE." ) != std::string::npos )
            {
                std::istringstream riss( line );
                std::string reuse_keyword;
                riss >> reuse_keyword >> reuse_instance >> reuse_signal;
            }
            else
            {
                pushBackLine( line );
            }
        }

        if( type == "BOARD" )
        {
            for( int i=0; i<pieces; ++i )
            {
                if( !readLine( aStream, line ) ) break;
                if( line[0] == '*' ) { pushBackLine( line ); return; }

                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                double width = 0.0;
                int piece_flags = 0;
                int level = 0;
                piss >> shape_type >> corners >> width >> piece_flags >> level;

                if( shape_type == "CLOSED" || shape_type == "OPEN" || shape_type == "BRDCLS" )
                {
                    POLYLINE polyline;
                    polyline.layer = 0;  // Board outline is layer-agnostic
                    polyline.width = width;
                    polyline.closed = ( shape_type == "CLOSED" || shape_type == "BRDCLS" );

                    for( int j = 0; j < corners; ++j )
                    {
                        if( !readLine( aStream, line ) )
                            break;

                        if( line[0] == '*' )
                        {
                            pushBackLine( line );
                            return;
                        }

                        std::istringstream ciss( line );
                        double dx = 0.0, dy = 0.0;
                        ciss >> dx >> dy;

                        // Per PADS spec, arc format is: x1 y1 ab aa ax1 ay1 ax2 ay2
                        // where x1,y1 = arc start point; center = bounding box midpoint
                        int startAngleTenths = 0, deltaAngleTenths = 0;
                        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                        if( ciss >> startAngleTenths >> deltaAngleTenths
                            >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                        {
                            double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                            double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                            double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                            double startAngle = startAngleTenths / 10.0;
                            double deltaAngle = deltaAngleTenths / 10.0;

                            double startAngleRad = startAngle * M_PI / 180.0;
                            double startX = cx + radius * std::cos( startAngleRad );
                            double startY = cy + radius * std::sin( startAngleRad );

                            double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                            double endX = cx + radius * std::cos( endAngleRad );
                            double endY = cy + radius * std::sin( endAngleRad );

                            polyline.points.emplace_back( xloc + startX, yloc + startY );

                            ARC arc{};
                            arc.cx = xloc + cx;
                            arc.cy = yloc + cy;
                            arc.radius = radius;
                            arc.start_angle = startAngle;
                            arc.delta_angle = deltaAngle;

                            polyline.points.emplace_back( xloc + endX, yloc + endY, arc );
                        }
                        else
                        {
                            polyline.points.emplace_back( xloc + dx, yloc + dy );
                        }
                    }

                    if( !polyline.points.empty() )
                        m_board_outlines.push_back( polyline );
                }
                else if( shape_type == "CIRCLE" || shape_type == "BRDCIR" )
                {
                    // Circle: 2 coordinates define opposite ends of diameter
                    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

                    if( readLine( aStream, line ) )
                    {
                        std::istringstream c1( line );
                        c1 >> x1 >> y1;
                    }

                    if( corners >= 2 && readLine( aStream, line ) )
                    {
                        std::istringstream c2( line );
                        c2 >> x2 >> y2;
                    }

                    double cx = xloc + ( x1 + x2 ) / 2.0;
                    double cy = yloc + ( y1 + y2 ) / 2.0;
                    double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 ) +
                                               ( y2 - y1 ) * ( y2 - y1 ) ) / 2.0;

                    POLYLINE polyline;
                    polyline.layer = 0;
                    polyline.width = width;
                    polyline.closed = true;

                    ARC arc{};
                    arc.cx = cx;
                    arc.cy = cy;
                    arc.radius = radius;
                    arc.start_angle = 0.0;
                    arc.delta_angle = 360.0;

                    polyline.points.emplace_back( cx + radius, cy, arc );

                    if( !polyline.points.empty() )
                        m_board_outlines.push_back( polyline );
                }
                else
                {
                     for( int j=0; j<corners; ++j )
                     {
                         if( !readLine( aStream, line ) ) break;
                         if( line[0] == '*' ) { pushBackLine( line ); return; }
                     }
                }
            }
        }
        else if( name.rfind( "DIM", 0 ) == 0 && type == "LINES" )
        {
            // Dimension annotation with BASPNT (base points), ARWLN/ARWHD (arrows),
            // and EXTLN (extension lines).
            //
            // BASPNT pairs define the measurement endpoints. There are typically two
            // BASPNT shapes: the first defines the start point (usually at origin),
            // and the second defines the end point as an offset from the dimension origin.
            //
            // ARWLN defines the crossbar position (Y for horizontal, X for vertical).
            //
            // For a proper linear dimension, we extract:
            // - Start point from first BASPNT (first coordinate of the pair)
            // - End point from second BASPNT (first coordinate of the pair)
            // - Crossbar position from ARWLN (used to compute height)
            DIMENSION dim;
            dim.name = name;
            dim.x = xloc;
            dim.y = yloc;

            double baspnt1_x = 0, baspnt1_y = 0;
            double baspnt2_x = 0, baspnt2_y = 0;
            double arwln_x = 0, arwln_y = 0;
            int baspnt_count = 0;
            bool hasArwln = false;

            for( int i = 0; i < pieces; ++i )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    break;
                }

                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                double width = 0;
                int piece_flags = 0;
                int level = 0;
                piss >> shape_type >> corners >> width >> piece_flags >> level;

                dim.layer = level;

                for( int j = 0; j < corners; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    if( line[0] == '*' )
                    {
                        pushBackLine( line );
                        break;
                    }

                    std::istringstream ciss( line );
                    double dx = 0.0, dy = 0.0;
                    ciss >> dx >> dy;

                    // BASPNT defines measurement endpoints. First BASPNT is start,
                    // second BASPNT is end. Only capture the first point of each pair.
                    if( shape_type == "BASPNT" && j == 0 )
                    {
                        if( baspnt_count == 0 )
                        {
                            baspnt1_x = xloc + dx;
                            baspnt1_y = yloc + dy;
                        }
                        else if( baspnt_count == 1 )
                        {
                            baspnt2_x = xloc + dx;
                            baspnt2_y = yloc + dy;
                        }

                        baspnt_count++;
                    }

                    // ARWLN1 first point: crossbar position
                    if( shape_type == "ARWLN1" && j == 0 )
                    {
                        arwln_x = xloc + dx;
                        arwln_y = yloc + dy;
                        hasArwln = true;
                    }
                }
            }

            // Build measurement points from BASPNT positions.
            if( baspnt_count >= 2 )
            {
                // Determine if this is a horizontal or vertical dimension based on
                // which axis has the larger offset between the two BASPNT points.
                double dx = std::abs( baspnt2_x - baspnt1_x );
                double dy = std::abs( baspnt2_y - baspnt1_y );
                bool isHorizontal = dx > dy;

                dim.is_horizontal = isHorizontal;

                POINT pt1{}, pt2{};
                pt1.x = baspnt1_x;
                pt1.y = baspnt1_y;
                pt2.x = baspnt2_x;
                pt2.y = baspnt2_y;

                // Store crossbar position for height calculation
                if( hasArwln )
                {
                    if( isHorizontal )
                        dim.crossbar_pos = arwln_y;
                    else
                        dim.crossbar_pos = arwln_x;
                }

                dim.points.push_back( pt1 );
                dim.points.push_back( pt2 );
            }

            // Parse text items for this dimension (same 3-line format as board text).
            // The first text is used as the dimension value label.
            for( int t = 0; t < textCount; ++t )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    break;
                }

                std::istringstream tiss( line );
                double tx = 0.0, ty = 0.0;
                tiss >> tx >> ty;

                if( tiss.fail() )
                {
                    readLine( aStream, line );
                    readLine( aStream, line );
                    continue;
                }

                double trot = 0.0;
                int tlayer = 0;
                double theight = 0.0, twidth = 0.0;
                tiss >> trot >> tlayer >> theight >> twidth;

                // Font line
                if( !readLine( aStream, line ) )
                    break;

                // Content line
                if( !readLine( aStream, line ) )
                    break;

                if( t == 0 )
                {
                    dim.text = line;
                    dim.text_height = theight;
                    dim.text_width = twidth;
                    dim.rotation = trot;
                }
            }

            textCount = 0;

            if( !dim.points.empty() )
                m_dimensions.push_back( dim );
        }
        else if( type == "KEEPOUT" || type == "RESTRICTVIA" || type == "RESTRICTROUTE"
                 || type == "RESTRICTAREA" || type == "PLACEMENT_KEEPOUT" )
        {
            // Parse keepout area definition
            KEEPOUT keepout;

            // Set defaults based on type name (fallback if no restriction codes in piece)
            if( type == "KEEPOUT" || type == "RESTRICTAREA" )
            {
                keepout.type = KEEPOUT_TYPE::ALL;
                keepout.no_traces = true;
                keepout.no_vias = true;
                keepout.no_copper = true;
            }
            else if( type == "RESTRICTVIA" )
            {
                keepout.type = KEEPOUT_TYPE::VIA;
                keepout.no_traces = false;
                keepout.no_vias = true;
                keepout.no_copper = false;
            }
            else if( type == "RESTRICTROUTE" )
            {
                keepout.type = KEEPOUT_TYPE::ROUTE;
                keepout.no_traces = true;
                keepout.no_vias = false;
                keepout.no_copper = false;
            }
            else if( type == "PLACEMENT_KEEPOUT" )
            {
                keepout.type = KEEPOUT_TYPE::PLACEMENT;
                keepout.no_traces = false;
                keepout.no_vias = false;
                keepout.no_copper = false;
                keepout.no_components = true;
            }

            for( int i = 0; i < pieces; ++i )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    break;
                }

                // Piece format: PIECETYPE CORNERS WIDTH FLAGS LEVEL [RESTRICTIONS]
                // RESTRICTIONS is a string containing: P H R C V T A
                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                double width = 0;
                int piece_flags = 0;
                int level = 0;
                std::string restrictions;
                piss >> shape_type >> corners >> width >> piece_flags >> level >> restrictions;

                if( level > 0 )
                    keepout.layers.push_back( level );

                // Parse restriction codes if present
                // Per PADS spec: P=Placement, H=Height, R=Trace/copper, C=Copper pour,
                // V=Via/jumper, T=Test point, A=Accordion
                // Only override defaults from type name if explicit restrictions are specified
                if( !restrictions.empty() )
                {
                    // Check if this looks like a restriction code string (contains letters)
                    bool has_restriction_codes = false;

                    for( char c : restrictions )
                    {
                        if( std::isalpha( c ) )
                        {
                            has_restriction_codes = true;
                            break;
                        }
                    }

                    if( has_restriction_codes )
                    {
                        // Clear all defaults and set based on explicit restriction codes
                        keepout.no_traces = false;
                        keepout.no_vias = false;
                        keepout.no_copper = false;
                        keepout.no_components = false;
                        keepout.height_restriction = false;
                        keepout.no_test_points = false;
                        keepout.no_accordion = false;

                        for( char c : restrictions )
                        {
                            switch( c )
                            {
                            case 'P':
                                keepout.no_components = true;
                                break;

                            case 'H':
                                keepout.height_restriction = true;
                                keepout.max_height = width;
                                break;

                            case 'R':
                                keepout.no_traces = true;
                                break;

                            case 'C':
                                keepout.no_copper = true;
                                break;

                            case 'V':
                                keepout.no_vias = true;
                                break;

                            case 'T':
                                keepout.no_test_points = true;
                                break;

                            case 'A':
                                keepout.no_accordion = true;
                                break;

                            default:
                                break;
                            }
                        }
                    }
                }

                // Handle KPTCIR (circle keepout) differently from KPTCLS (polygon keepout)
                if( shape_type == "KPTCIR" )
                {
                    // Circle format: 2 coordinates define opposite ends of diameter
                    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

                    if( readLine( aStream, line ) )
                    {
                        std::istringstream c1( line );
                        c1 >> x1 >> y1;
                    }

                    if( corners >= 2 && readLine( aStream, line ) )
                    {
                        std::istringstream c2( line );
                        c2 >> x2 >> y2;
                    }

                    // Calculate center and radius from diameter endpoints
                    double cx = xloc + ( x1 + x2 ) / 2.0;
                    double cy = yloc + ( y1 + y2 ) / 2.0;
                    double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 ) +
                                               ( y2 - y1 ) * ( y2 - y1 ) ) / 2.0;

                    // Create full circle arc for keepout outline
                    ARC arc{};
                    arc.cx = cx;
                    arc.cy = cy;
                    arc.radius = radius;
                    arc.start_angle = 0.0;
                    arc.delta_angle = 360.0;

                    keepout.outline.emplace_back( cx + radius, cy, arc );
                }
                else
                {
                    // KPTCLS or other polygon piece types
                    for( int j = 0; j < corners; ++j )
                    {
                        if( !readLine( aStream, line ) )
                            break;

                        if( line[0] == '*' )
                        {
                            pushBackLine( line );
                            break;
                        }

                        std::istringstream ciss( line );
                        double dx = 0.0, dy = 0.0;
                        ciss >> dx >> dy;

                        // Per PADS spec, arc format is: x1 y1 ab aa ax1 ay1 ax2 ay2
                        // where x1,y1 = arc start point; center = bounding box midpoint
                        int startAngleTenths = 0, deltaAngleTenths = 0;
                        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                        if( ciss >> startAngleTenths >> deltaAngleTenths
                            >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                        {
                            double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                            double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                            double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                            double startAngle = startAngleTenths / 10.0;
                            double deltaAngle = deltaAngleTenths / 10.0;

                            double startAngleRad = startAngle * M_PI / 180.0;
                            double startX = cx + radius * std::cos( startAngleRad );
                            double startY = cy + radius * std::sin( startAngleRad );

                            double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                            double endX = cx + radius * std::cos( endAngleRad );
                            double endY = cy + radius * std::sin( endAngleRad );

                            keepout.outline.emplace_back( xloc + startX, yloc + startY );

                            ARC arc{};
                            arc.cx = xloc + cx;
                            arc.cy = yloc + cy;
                            arc.radius = radius;
                            arc.start_angle = startAngle;
                            arc.delta_angle = deltaAngle;

                            keepout.outline.emplace_back( xloc + endX, yloc + endY, arc );
                        }
                        else
                        {
                            keepout.outline.emplace_back( xloc + dx, yloc + dy );
                        }
                    }
                }
            }

            if( !keepout.outline.empty() )
                m_keepouts.push_back( keepout );
        }
        else if( type == "COPPER" || type == "COPCUT" )
        {
            // Parse copper shape definition
            // Header already parsed: name type xloc yloc pieces flags [text [signame]]
            // signame was parsed earlier if present

            for( int i = 0; i < pieces; ++i )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    return;
                }

                // Piece format: PIECETYPE CORNERS WIDTH FLAGS LEVEL
                // PIECETYPE: COPOPN (polyline), COPCLS (filled polygon), COPCIR (filled circle),
                //            COPCUT (polygon void), COPCCO (circle void), CIRCUR (circle void for COPCUT)
                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                double width = 0;
                int piece_flags = 0;
                int level = 0;
                piss >> shape_type >> corners >> width >> piece_flags >> level;

                COPPER_SHAPE copper;
                copper.name = name;
                copper.layer = level;
                copper.width = width;
                copper.net_name = signame;

                copper.filled = ( shape_type == "COPCLS" || shape_type == "COPCIR" );
                copper.is_cutout = ( shape_type == "COPCUT" || shape_type == "COPCCO" ||
                                     shape_type == "CIRCUR" || type == "COPCUT" );

                // Handle circle shapes specially
                if( shape_type == "COPCIR" || shape_type == "COPCCO" || shape_type == "CIRCUR" )
                {
                    // Circle: 2 coordinates define opposite ends of diameter
                    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

                    if( readLine( aStream, line ) )
                    {
                        std::istringstream c1( line );
                        c1 >> x1 >> y1;
                    }

                    if( corners >= 2 && readLine( aStream, line ) )
                    {
                        std::istringstream c2( line );
                        c2 >> x2 >> y2;
                    }

                    double cx = xloc + ( x1 + x2 ) / 2.0;
                    double cy = yloc + ( y1 + y2 ) / 2.0;
                    double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 ) +
                                               ( y2 - y1 ) * ( y2 - y1 ) ) / 2.0;

                    ARC arc{};
                    arc.cx = cx;
                    arc.cy = cy;
                    arc.radius = radius;
                    arc.start_angle = 0.0;
                    arc.delta_angle = 360.0;

                    copper.outline.emplace_back( cx + radius, cy, arc );
                }
                else
                {
                    // COPOPN, COPCLS, COPCUT - polygon shapes
                    for( int j = 0; j < corners; ++j )
                    {
                        if( !readLine( aStream, line ) )
                            break;

                        if( line[0] == '*' )
                        {
                            pushBackLine( line );
                            break;
                        }

                        std::istringstream ciss( line );
                        double dx = 0.0, dy = 0.0;
                        ciss >> dx >> dy;

                        // Per PADS spec, arc format is: x1 y1 ab aa ax1 ay1 ax2 ay2
                        // where x1,y1 = arc start point; center = bounding box midpoint
                        int startAngleTenths = 0, deltaAngleTenths = 0;
                        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                        if( ciss >> startAngleTenths >> deltaAngleTenths
                            >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                        {
                            double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                            double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                            double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                            double startAngle = startAngleTenths / 10.0;
                            double deltaAngle = deltaAngleTenths / 10.0;

                            double startAngleRad = startAngle * M_PI / 180.0;
                            double startX = cx + radius * std::cos( startAngleRad );
                            double startY = cy + radius * std::sin( startAngleRad );

                            double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                            double endX = cx + radius * std::cos( endAngleRad );
                            double endY = cy + radius * std::sin( endAngleRad );

                            copper.outline.emplace_back( xloc + startX, yloc + startY );

                            ARC arc{};
                            arc.cx = xloc + cx;
                            arc.cy = yloc + cy;
                            arc.radius = radius;
                            arc.start_angle = startAngle;
                            arc.delta_angle = deltaAngle;

                            copper.outline.emplace_back( xloc + endX, yloc + endY, arc );
                        }
                        else
                        {
                            copper.outline.emplace_back( xloc + dx, yloc + dy );
                        }
                    }
                }

                if( !copper.outline.empty() )
                    m_copper_shapes.push_back( copper );
            }
        }
        else if( type == "LINES" )
        {
            // Generic 2D graphic lines (non-dimension LINES items)
            for( int i = 0; i < pieces; ++i )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    return;
                }

                // Piece format: PIECETYPE CORNERS WIDTH FLAGS LEVEL
                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                double width = 0;
                int piece_flags = 0;
                int level = 0;
                piss >> shape_type >> corners >> width >> piece_flags >> level;

                GRAPHIC_LINE graphic;
                graphic.name = name;
                graphic.layer = level;
                graphic.width = width;
                graphic.reuse_instance = reuse_instance;

                // Determine if closed based on shape type
                graphic.closed = ( shape_type == "CLOSED" || shape_type == "CIRCLE" );

                if( shape_type == "CIRCLE" )
                {
                    // Circle: 2 coordinates define opposite ends of diameter
                    double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;

                    if( readLine( aStream, line ) )
                    {
                        std::istringstream c1( line );
                        c1 >> x1 >> y1;
                    }

                    if( corners >= 2 && readLine( aStream, line ) )
                    {
                        std::istringstream c2( line );
                        c2 >> x2 >> y2;
                    }

                    double cx = xloc + ( x1 + x2 ) / 2.0;
                    double cy = yloc + ( y1 + y2 ) / 2.0;
                    double radius = std::sqrt( ( x2 - x1 ) * ( x2 - x1 ) +
                                               ( y2 - y1 ) * ( y2 - y1 ) ) / 2.0;

                    ARC arc{};
                    arc.cx = cx;
                    arc.cy = cy;
                    arc.radius = radius;
                    arc.start_angle = 0.0;
                    arc.delta_angle = 360.0;

                    graphic.points.emplace_back( cx + radius, cy, arc );
                }
                else
                {
                    // OPEN or CLOSED polyline
                    for( int j = 0; j < corners; ++j )
                    {
                        if( !readLine( aStream, line ) )
                            break;

                        if( line[0] == '*' )
                        {
                            pushBackLine( line );
                            break;
                        }

                        std::istringstream ciss( line );
                        double dx = 0.0, dy = 0.0;
                        ciss >> dx >> dy;

                        // Check for arc parameters
                        int startAngleTenths = 0, deltaAngleTenths = 0;
                        double bboxMinX = 0.0, bboxMinY = 0.0, bboxMaxX = 0.0, bboxMaxY = 0.0;

                        if( ciss >> startAngleTenths >> deltaAngleTenths
                            >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY )
                        {
                            double cx = ( bboxMinX + bboxMaxX ) / 2.0;
                            double cy = ( bboxMinY + bboxMaxY ) / 2.0;
                            double radius = ( bboxMaxX - bboxMinX ) / 2.0;
                            double startAngle = startAngleTenths / 10.0;
                            double deltaAngle = deltaAngleTenths / 10.0;

                            double startAngleRad = startAngle * M_PI / 180.0;
                            double startX = cx + radius * std::cos( startAngleRad );
                            double startY = cy + radius * std::sin( startAngleRad );

                            double endAngleRad = ( startAngle + deltaAngle ) * M_PI / 180.0;
                            double endX = cx + radius * std::cos( endAngleRad );
                            double endY = cy + radius * std::sin( endAngleRad );

                            graphic.points.emplace_back( xloc + startX, yloc + startY );

                            ARC arc{};
                            arc.cx = xloc + cx;
                            arc.cy = yloc + cy;
                            arc.radius = radius;
                            arc.start_angle = startAngle;
                            arc.delta_angle = deltaAngle;

                            graphic.points.emplace_back( xloc + endX, yloc + endY, arc );
                        }
                        else
                        {
                            graphic.points.emplace_back( xloc + dx, yloc + dy );
                        }
                    }
                }

                if( !graphic.points.empty() )
                    m_graphic_lines.push_back( graphic );
            }
        }
        else
        {
            // Skip unknown types
            for( int i = 0; i < pieces; ++i )
            {
                if( !readLine( aStream, line ) )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    return;
                }

                std::istringstream piss( line );
                std::string shape_type;
                int corners = 0;
                piss >> shape_type >> corners;

                for( int j = 0; j < corners; ++j )
                {
                    if( !readLine( aStream, line ) )
                        break;

                    if( line[0] == '*' )
                    {
                        pushBackLine( line );
                        return;
                    }
                }
            }
        }

        // Parse text items that follow the pieces (3 lines each: properties, font, content).
        // Text coordinates are relative to the drawing item origin.
        for( int t = 0; t < textCount; ++t )
        {
            if( !readLine( aStream, line ) )
                break;

            if( line[0] == '*' )
            {
                pushBackLine( line );
                return;
            }

            std::istringstream tiss( line );
            TEXT text;

            tiss >> text.location.x >> text.location.y >> text.rotation >> text.layer
                 >> text.height >> text.width;

            if( tiss.fail() )
            {
                // Consume remaining 2 lines and continue
                readLine( aStream, line );
                readLine( aStream, line );
                continue;
            }

            text.location.x += xloc;
            text.location.y += yloc;

            std::string mirrored;
            tiss >> mirrored;
            text.mirrored = ( mirrored == "M" );
            tiss >> text.hjust >> text.vjust;

            // Font line
            if( !readLine( aStream, line ) )
                break;

            if( line[0] == '*' )
            {
                pushBackLine( line );
                return;
            }

            size_t bracket_start = line.find( '<' );
            size_t bracket_end = line.find( '>' );

            if( bracket_start != std::string::npos && bracket_end != std::string::npos )
                text.font_face = line.substr( bracket_start + 1, bracket_end - bracket_start - 1 );

            std::istringstream fiss( line );
            std::string font_style_part;
            fiss >> font_style_part;

            size_t colon_pos = font_style_part.find( ':' );

            if( colon_pos != std::string::npos )
                text.font_style = font_style_part.substr( 0, colon_pos );
            else
                text.font_style = font_style_part;

            // Content line
            if( !readLine( aStream, line ) )
                break;

            if( line[0] == '*' )
            {
                pushBackLine( line );
                return;
            }

            text.content = line;
            m_texts.push_back( text );
        }
    }
}

void PARSER::parseSectionPARTTYPE( std::ifstream& aStream )
{
    std::string line;
    PART_TYPE* currentPartType = nullptr;
    GATE_DEF* currentGate = nullptr;

    // Helper to parse pin electrical type character
    auto parsePinElecType = []( char c ) -> PIN_ELEC_TYPE {
        switch( c )
        {
        case 'S': return PIN_ELEC_TYPE::SOURCE;
        case 'B': return PIN_ELEC_TYPE::BIDIRECTIONAL;
        case 'C': return PIN_ELEC_TYPE::OPEN_COLLECTOR;
        case 'T': return PIN_ELEC_TYPE::TRISTATE;
        case 'L': return PIN_ELEC_TYPE::LOAD;
        case 'Z': return PIN_ELEC_TYPE::TERMINATOR;
        case 'P': return PIN_ELEC_TYPE::POWER;
        case 'G': return PIN_ELEC_TYPE::GROUND;
        default:  return PIN_ELEC_TYPE::UNDEFINED;
        }
    };

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        if( line.empty() )
            continue;

        // Gate line: G gateswap pins
        if( line.rfind( "G ", 0 ) == 0 && currentPartType )
        {
            std::istringstream gss( line );
            std::string g_keyword;
            int gateSwap = 0, pinCount = 0;
            gss >> g_keyword >> gateSwap >> pinCount;

            GATE_DEF gate;
            gate.gate_swap_type = gateSwap;
            currentPartType->gates.push_back( gate );
            currentGate = &currentPartType->gates.back();
            continue;
        }

        // SIGPIN pinno width signm
        if( line.rfind( "SIGPIN", 0 ) == 0 && currentPartType )
        {
            std::istringstream sss( line );
            std::string keyword;
            SIGPIN sigpin;

            sss >> keyword >> sigpin.pin_number >> sigpin.width >> sigpin.signal_name;

            if( !sigpin.pin_number.empty() )
                currentPartType->signal_pins.push_back( sigpin );

            continue;
        }

        // Check if this line contains pin definitions (format: pinnumber.swptyp.pintyp[.funcname])
        // These follow a gate definition. Pin definition tokens have at least 3 dot-separated parts.
        // Part type header lines may also contain dots in the name (e.g., "CAPSMT0.1UF0402X7R50V")
        // but their first token won't have 3+ parts, so we check for that.
        if( line.find( '.' ) != std::string::npos && currentPartType )
        {
            // First check if this could be a part type header line with a dot in the name.
            // Part type headers have format: NAME DECAL CLASS ATTRS ... where NAME may contain dots
            // but the first dot-separated segment will have <3 parts.
            std::stringstream check_ss( line );
            std::string first_token;
            check_ss >> first_token;

            int dot_count = 0;

            for( char c : first_token )
            {
                if( c == '.' )
                    dot_count++;
            }

            // If first token has <2 dots, this could be a part type header, not a pin definition
            if( dot_count < 2 )
            {
                // Fall through to part type header parsing below
            }
            else
            {
            std::stringstream ss( line );
            std::string token;

            while( ss >> token )
            {
                // Parse pin format: PINNAME.SWAPTYPE.PINTYPE[.FUNCNAME] or PINNAME.PADINDEX.TYPE.NET
                std::vector<std::string> parts;
                size_t start = 0;
                size_t pos = 0;

                while( ( pos = token.find( '.', start ) ) != std::string::npos )
                {
                    parts.push_back( token.substr( start, pos - start ) );
                    start = pos + 1;
                }

                parts.push_back( token.substr( start ) );

                if( parts.size() >= 3 )
                {
                    // Check if this is a gate pin definition or pad stack mapping
                    // Gate pin: pinnumber.swaptype.pintype[.funcname]
                    // Pad map: pinname.padindex.type.netname

                    bool isNumericSecond = !parts[1].empty() &&
                        std::all_of( parts[1].begin(), parts[1].end(), ::isdigit );

                    if( currentGate && parts[2].size() == 1 && !isNumericSecond )
                    {
                        // This looks like a gate pin definition
                        GATE_PIN gpin;
                        gpin.pin_number = parts[0];
                        gpin.swap_type = PADS_COMMON::ParseInt( parts[1], 0, "gate pin swap" );

                        if( !parts[2].empty() )
                            gpin.elec_type = parsePinElecType( parts[2][0] );

                        if( parts.size() >= 4 )
                            gpin.func_name = parts[3];

                        currentGate->pins.push_back( gpin );
                    }
                    else if( isNumericSecond )
                    {
                        int padIdx = PADS_COMMON::ParseInt( parts[1], -1, "pad index" );

                        if( padIdx >= 0 )
                            currentPartType->pin_pad_map[parts[0]] = padIdx;
                    }
                }
            }

            continue;
            }
        }

        // Attribute block enclosed in braces
        if( line[0] == '{' && currentPartType )
        {
            while( readLine( aStream, line ) )
            {
                if( line.empty() || line[0] == '}' )
                    break;

                if( line[0] == '*' )
                {
                    pushBackLine( line );
                    return;
                }

                std::string attrName, attrValue;

                if( line[0] == '"' )
                {
                    size_t endQuote = line.find( '"', 1 );

                    if( endQuote != std::string::npos )
                    {
                        attrName = line.substr( 1, endQuote - 1 );
                        attrValue = line.substr( endQuote + 1 );
                    }
                }
                else
                {
                    std::istringstream attrSS( line );
                    attrSS >> attrName;
                    std::getline( attrSS >> std::ws, attrValue );
                }

                if( !attrValue.empty() && attrValue[0] == ' ' )
                    attrValue = attrValue.substr( 1 );

                if( !attrName.empty() && !attrValue.empty() )
                    currentPartType->attributes[attrName] = attrValue;
            }

            continue;
        }

        if( line[0] == '{' || line[0] == '}' )
            continue;

        // Part type definition line: NAME DECAL CLASS ATTRS GATES SIGS PINSEQ STATE
        std::stringstream ss( line );
        std::string name, decal;
        ss >> name >> decal;

        if( !name.empty() && name[0] != 'G' )
        {
            PART_TYPE pt;
            pt.name = name;
            pt.decal_name = decal;
            m_part_types[name] = pt;
            currentPartType = &m_part_types[name];
            currentGate = nullptr;
        }
    }
}


void PARSER::parseSectionREUSE( std::ifstream& aStream )
{
    std::string line;
    REUSE_BLOCK* currentBlock = nullptr;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::stringstream ss( line );
        std::string keyword;
        ss >> keyword;

        if( keyword == "TYPE" )
        {
            std::string typename_val;
            std::getline( ss, typename_val );

            if( !typename_val.empty() && typename_val[0] == ' ' )
                typename_val = typename_val.substr( 1 );

            REUSE_BLOCK block;
            block.name = typename_val;
            m_reuse_blocks[typename_val] = block;
            currentBlock = &m_reuse_blocks[typename_val];
        }
        else if( keyword == "TIMESTAMP" && currentBlock )
        {
            long timestamp = 0;
            ss >> timestamp;
            currentBlock->timestamp = timestamp;
        }
        else if( keyword == "PART_NAMING" && currentBlock )
        {
            std::string naming;
            std::getline( ss, naming );

            if( !naming.empty() && naming[0] == ' ' )
                naming = naming.substr( 1 );

            currentBlock->part_naming = naming;
        }
        else if( keyword == "PART" && currentBlock )
        {
            std::string partname;
            std::getline( ss, partname );

            if( !partname.empty() && partname[0] == ' ' )
                partname = partname.substr( 1 );

            currentBlock->part_names.push_back( partname );
        }
        else if( keyword == "NET_NAMING" && currentBlock )
        {
            std::string naming;
            std::getline( ss, naming );

            if( !naming.empty() && naming[0] == ' ' )
                naming = naming.substr( 1 );

            currentBlock->net_naming = naming;
        }
        else if( keyword == "NET" && currentBlock )
        {
            int merge_flag = 0;
            std::string netname;

            ss >> merge_flag;
            std::getline( ss, netname );

            if( !netname.empty() && netname[0] == ' ' )
                netname = netname.substr( 1 );

            REUSE_NET net;
            net.merge = ( merge_flag == 1 );
            net.name = netname;
            currentBlock->nets.push_back( net );
        }
        else if( keyword == "REUSE" && currentBlock )
        {
            REUSE_INSTANCE instance;
            ss >> instance.instance_name;

            std::string next_token;
            ss >> next_token;

            if( next_token == "PREFIX" || next_token == "SUFFIX" )
            {
                std::string param;
                ss >> param;
                instance.part_naming = next_token + " " + param;
                ss >> next_token;
            }
            else if( next_token == "START" || next_token == "INCREMENT" )
            {
                std::string num;
                ss >> num;
                instance.part_naming = next_token + " " + num;
                ss >> next_token;
            }
            else if( next_token == "NEXT" )
            {
                instance.part_naming = next_token;
                ss >> next_token;
            }

            if( next_token == "PREFIX" || next_token == "SUFFIX" )
            {
                std::string param;
                ss >> param;
                instance.net_naming = next_token + " " + param;
            }
            else if( next_token == "START" || next_token == "INCREMENT" )
            {
                std::string num;
                ss >> num;
                instance.net_naming = next_token + " " + num;
            }
            else if( next_token == "NEXT" )
            {
                instance.net_naming = next_token;
            }

            std::string glued_str;
            ss >> instance.location.x >> instance.location.y >> instance.rotation >> glued_str;

            instance.glued = ( glued_str == "Y" || glued_str == "YES" || glued_str == "1" );
            currentBlock->instances.push_back( instance );
        }
    }
}


void PARSER::parseSectionCLUSTER( std::ifstream& aStream )
{
    std::string line;
    CLUSTER* currentCluster = nullptr;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::stringstream ss( line );
        std::string firstToken;
        ss >> firstToken;

        // Check if this is a new cluster definition or a member line
        // Cluster definition format varies but typically starts with name/id
        if( firstToken.empty() )
            continue;

        // Try parsing as cluster ID
        bool isNumeric = !firstToken.empty() &&
                         std::all_of( firstToken.begin(), firstToken.end(), ::isdigit );

        if( isNumeric )
        {
            // This could be a cluster ID starting a new cluster
            CLUSTER cluster;
            cluster.id = PADS_COMMON::ParseInt( firstToken, 0, "CLUSTER" );

            // Read optional cluster name
            std::string name;

            if( ss >> name )
                cluster.name = name;
            else
                cluster.name = "Cluster_" + firstToken;

            m_clusters.push_back( cluster );
            currentCluster = &m_clusters.back();
        }
        else if( currentCluster )
        {
            // Could be a net name or segment reference belonging to current cluster
            // PADS format varies - add to net_names or segment_refs based on content
            if( firstToken.find( '.' ) != std::string::npos )
            {
                // Looks like a segment reference (e.g., "NET.1")
                currentCluster->segment_refs.push_back( firstToken );
            }
            else
            {
                // Treat as net name
                currentCluster->net_names.push_back( firstToken );
            }

            // Continue reading additional items on the same line
            std::string item;

            while( ss >> item )
            {
                if( item.find( '.' ) != std::string::npos )
                    currentCluster->segment_refs.push_back( item );
                else
                    currentCluster->net_names.push_back( item );
            }
        }
    }
}


void PARSER::parseSectionJUMPER( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        // Jumper header format: name flags minlen maxlen lenincr lcount padstack [end_padstack]
        // flags: V=via enabled, N=no via, W=wirebond, D=display silk, G=glued
        std::stringstream ss( line );
        std::string name, flags;
        double minlen = 0.0, maxlen = 0.0, lenincr = 0.0;
        int lcount = 0;
        std::string padstack, end_padstack;

        if( !( ss >> name >> flags >> minlen >> maxlen >> lenincr >> lcount >> padstack ) )
            continue;

        ss >> end_padstack;

        JUMPER_DEF jumper;
        jumper.name = name;
        jumper.min_length = minlen;
        jumper.max_length = maxlen;
        jumper.length_increment = lenincr;
        jumper.padstack = padstack;
        jumper.end_padstack = end_padstack;

        // Parse flags
        for( char c : flags )
        {
            switch( c )
            {
            case 'V': jumper.via_enabled = true; break;
            case 'N': jumper.via_enabled = false; break;
            case 'W': jumper.wirebond = true; break;
            case 'D': jumper.display_silk = true; break;
            case 'G': jumper.glued = true; break;
            default: break;
            }
        }

        // Parse label entries (each label is 2 lines)
        for( int i = 0; i < lcount; ++i )
        {
            ATTRIBUTE attr;

            // Line 1: VISIBLE X Y ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST [RIGHTREADING]
            if( !readLine( aStream, line ) )
                break;

            std::stringstream ss_attr( line );
            std::string visible_str, mirrored_str, right_reading_str;

            if( ss_attr >> visible_str >> attr.x >> attr.y >> attr.orientation >> attr.level
                >> attr.height >> attr.width >> mirrored_str >> attr.hjust >> attr.vjust )
            {
                attr.visible = ( visible_str == "VALUE" || visible_str == "FULL_NAME" ||
                                 visible_str == "NAME" || visible_str == "FULL_BOTH" ||
                                 visible_str == "BOTH" );
                attr.mirrored = ( mirrored_str == "M" || mirrored_str == "1" );
                ss_attr >> right_reading_str;
                attr.right_reading = ( right_reading_str == "Y" || right_reading_str == "ORTHO" );
            }

            // Line 2: Font info
            if( !readLine( aStream, line ) )
                break;

            attr.font_info = line;

            jumper.labels.push_back( attr );
        }

        m_jumper_defs.push_back( jumper );
    }
}


void PARSER::parseSectionTESTPOINT( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::stringstream ss( line );
        std::string type;
        ss >> type;

        if( type.empty() )
            continue;

        // Format: TYPE X Y SIDE NETNAME SYMBOLNAME
        // TYPE is VIA or PIN
        TEST_POINT tp;
        tp.type = type;

        ss >> tp.x >> tp.y >> tp.side >> tp.net_name >> tp.symbol_name;

        if( !tp.net_name.empty() )
        {
            m_test_points.push_back( tp );
        }
    }
}


void PARSER::parseSectionNETCLASS( std::ifstream& aStream )
{
    std::string line;
    NET_CLASS_DEF currentClass;
    bool inClass = false;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            // Save the last class if we were building one
            if( inClass && !currentClass.name.empty() )
                m_net_classes.push_back( currentClass );

            pushBackLine( line );
            break;
        }

        std::stringstream ss( line );
        std::string token;
        ss >> token;

        if( token.empty() )
            continue;

        // Check for class name definition (typically first token without a keyword)
        if( token == "CLASS" || token == "NETCLASS" )
        {
            // Save previous class if any
            if( inClass && !currentClass.name.empty() )
                m_net_classes.push_back( currentClass );

            // Start new class
            currentClass = NET_CLASS_DEF();
            ss >> currentClass.name;
            inClass = true;
        }
        else if( token == "CLEARANCE" && inClass )
        {
            ss >> currentClass.clearance;
        }
        else if( token == "TRACKWIDTH" && inClass )
        {
            ss >> currentClass.track_width;
        }
        else if( token == "VIASIZE" && inClass )
        {
            ss >> currentClass.via_size;
        }
        else if( token == "VIADRILL" && inClass )
        {
            ss >> currentClass.via_drill;
        }
        else if( token == "DIFFPAIRGAP" && inClass )
        {
            ss >> currentClass.diff_pair_gap;
        }
        else if( token == "DIFFPAIRWIDTH" && inClass )
        {
            ss >> currentClass.diff_pair_width;
        }
        else if( token == "NET" && inClass )
        {
            // Net assignment: NET netname
            std::string netName;
            ss >> netName;

            if( !netName.empty() )
                currentClass.net_names.push_back( netName );
        }
        else if( !token.empty() && token[0] != '#' )
        {
            // Check if this looks like a class name (no keyword prefix) in some formats
            if( !inClass || ( inClass && currentClass.name.empty() ) )
            {
                // Save previous if any
                if( inClass && !currentClass.name.empty() )
                    m_net_classes.push_back( currentClass );

                currentClass = NET_CLASS_DEF();
                currentClass.name = token;
                inClass = true;
            }
        }
    }

    // Save final class
    if( inClass && !currentClass.name.empty() )
        m_net_classes.push_back( currentClass );
}


void PARSER::parseSectionDIFFPAIR( std::ifstream& aStream )
{
    std::string line;

    while( readLine( aStream, line ) )
    {
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::stringstream ss( line );
        std::string token;
        ss >> token;

        if( token.empty() )
            continue;

        // Differential pair format can vary. Common patterns:
        // DIFFPAIR name positive_net negative_net gap width
        // or keyword-based like:
        // PAIR name
        // POS positive_net
        // NEG negative_net
        // GAP value
        // WIDTH value

        if( token == "DIFFPAIR" || token == "PAIR" )
        {
            DIFF_PAIR_DEF dp;
            ss >> dp.name;

            // Try to read the nets inline
            std::string posNet, negNet;
            ss >> posNet >> negNet;

            if( !posNet.empty() )
                dp.positive_net = posNet;

            if( !negNet.empty() )
                dp.negative_net = negNet;

            // Try to read gap and width inline
            double gap = 0.0, width = 0.0;

            if( ss >> gap )
                dp.gap = gap;

            if( ss >> width )
                dp.width = width;

            if( !dp.name.empty() )
                m_diff_pairs.push_back( dp );
        }
        else if( token == "POS" && !m_diff_pairs.empty() )
        {
            ss >> m_diff_pairs.back().positive_net;
        }
        else if( token == "NEG" && !m_diff_pairs.empty() )
        {
            ss >> m_diff_pairs.back().negative_net;
        }
        else if( token == "GAP" && !m_diff_pairs.empty() )
        {
            ss >> m_diff_pairs.back().gap;
        }
        else if( ( token == "WIDTH" || token == "TRACKWIDTH" ) && !m_diff_pairs.empty() )
        {
            ss >> m_diff_pairs.back().width;
        }
    }
}


void PARSER::parseSectionLAYERDEFS( std::ifstream& aStream )
{
    std::string line;
    int braceDepth = 0;
    int currentLayerNum = -1;
    LAYER_INFO currentLayer;
    bool inLayerBlock = false;

    // Helper to parse LAYER_TYPE string to enum
    auto parseLayerType = []( const std::string& typeStr ) -> PADS_LAYER_FUNCTION {
        if( typeStr == "ROUTING" )
            return PADS_LAYER_FUNCTION::ROUTING;
        else if( typeStr == "PLANE" )
            return PADS_LAYER_FUNCTION::PLANE;
        else if( typeStr == "MIXED" )
            return PADS_LAYER_FUNCTION::MIXED;
        else if( typeStr == "UNASSIGNED" )
            return PADS_LAYER_FUNCTION::UNASSIGNED;
        else if( typeStr == "SOLDER_MASK" )
            return PADS_LAYER_FUNCTION::SOLDER_MASK;
        else if( typeStr == "PASTE_MASK" )
            return PADS_LAYER_FUNCTION::PASTE_MASK;
        else if( typeStr == "SILK_SCREEN" )
            return PADS_LAYER_FUNCTION::SILK_SCREEN;
        else if( typeStr == "ASSEMBLY" )
            return PADS_LAYER_FUNCTION::ASSEMBLY;
        else if( typeStr == "DOCUMENTATION" )
            return PADS_LAYER_FUNCTION::DOCUMENTATION;
        else if( typeStr == "DRILL" )
            return PADS_LAYER_FUNCTION::DRILL;
        return PADS_LAYER_FUNCTION::UNKNOWN;
    };

    while( readLine( aStream, line ) )
    {
        if( line.empty() )
            continue;

        // Stop if we hit a new section marker
        if( line[0] == '*' )
        {
            pushBackLine( line );
            break;
        }

        std::istringstream iss( line );
        std::string token;
        iss >> token;

        if( token == "{" )
        {
            braceDepth++;
            continue;
        }

        if( token == "}" )
        {
            braceDepth--;

            // Closing a layer block, save if we have valid data
            if( inLayerBlock && braceDepth == 1 )
            {
                if( currentLayerNum >= 0 )
                {
                    currentLayer.number = currentLayerNum;

                    // Determine if copper based on layer number and type
                    currentLayer.is_copper = ( currentLayer.layer_type == PADS_LAYER_FUNCTION::ROUTING ||
                                               currentLayer.layer_type == PADS_LAYER_FUNCTION::PLANE ||
                                               currentLayer.layer_type == PADS_LAYER_FUNCTION::MIXED );
                    currentLayer.required = currentLayer.is_copper;
                    m_layer_defs[currentLayerNum] = currentLayer;
                }

                inLayerBlock = false;
                currentLayerNum = -1;
            }

            // Exiting the outer LAYER block
            if( braceDepth <= 0 )
                break;

            continue;
        }

        if( token == "LAYER" )
        {
            int layerNum = -1;
            iss >> layerNum;

            if( !iss.fail() && layerNum >= 0 )
            {
                // Starting a new layer definition
                currentLayerNum = layerNum;
                currentLayer = LAYER_INFO();
                currentLayer.number = layerNum;
                currentLayer.layer_type = PADS_LAYER_FUNCTION::UNKNOWN;
                inLayerBlock = true;
            }
        }
        else if( token == "LAYER_NAME" && inLayerBlock )
        {
            // Read the rest of the line as the layer name
            std::string name;
            std::getline( iss >> std::ws, name );
            currentLayer.name = name;
        }
        else if( token == "LAYER_TYPE" && inLayerBlock )
        {
            std::string typeStr;
            iss >> typeStr;
            currentLayer.layer_type = parseLayerType( typeStr );
        }
        else if( token == "LAYER_THICKNESS" && inLayerBlock )
        {
            iss >> currentLayer.layer_thickness;
        }
        else if( token == "COPPER_THICKNESS" && inLayerBlock )
        {
            iss >> currentLayer.copper_thickness;
        }
        else if( token == "DIELECTRIC" && inLayerBlock )
        {
            iss >> currentLayer.dielectric_constant;
        }
    }
}


void PARSER::parseSectionMISC( std::ifstream& aStream )
{
    // The MISC section contains various optional data:
    // - NET_CLASS DATA (net class definitions with member nets)
    // - GROUP DATA (pin pair groups)
    // - ASSOCIATED NET DATA (associated net pairs)
    // - DIF_PAIR definitions with extended parameters
    // - DESIGN_RULES / RULE_SET (hierarchical design rules)
    // - ATTRIBUTES DICTIONARY (attribute type definitions)
    //
    // We parse DIF_PAIR and NET_CLASS definitions as they're most relevant for KiCad.

    std::string line;
    int braceDepth = 0;
    bool inDifPair = false;
    bool inNetClassData = false;
    bool inNetClass = false;
    bool inDefaultRuleSet = false;
    bool inClearanceRule = false;
    int defaultRuleSetDepth = -1;
    int clearanceRuleDepth = -1;
    bool foundDefaultRules = false;
    DIFF_PAIR_DEF currentDiffPair;
    NET_CLASS_DEF currentNetClass;

    while( readLine( aStream, line ) )
    {
        if( line.empty() )
            continue;

        // Stop at next section marker
        if( line[0] == '*' && braceDepth == 0 )
        {
            pushBackLine( line );
            break;
        }

        // Track brace depth for nested structures
        for( char c : line )
        {
            if( c == '{' )
                braceDepth++;
            else if( c == '}' )
            {
                braceDepth--;

                if( braceDepth == 0 && inDifPair )
                {
                    // End of DIF_PAIR block
                    if( !currentDiffPair.name.empty() )
                        m_diff_pairs.push_back( currentDiffPair );

                    inDifPair = false;
                    currentDiffPair = DIFF_PAIR_DEF();
                }

                if( braceDepth == 1 && inNetClass )
                {
                    // End of NET_CLASS block (inside NET_CLASS DATA)
                    if( !currentNetClass.name.empty() )
                        m_net_classes.push_back( currentNetClass );

                    inNetClass = false;
                    currentNetClass = NET_CLASS_DEF();
                }

                if( braceDepth == 0 && inNetClassData )
                {
                    // End of NET_CLASS DATA block
                    inNetClassData = false;
                }

                if( inClearanceRule && braceDepth < clearanceRuleDepth )
                {
                    inClearanceRule = false;

                    // Fall back to struct defaults if no values were parsed
                    if( m_design_rules.default_clearance
                        == std::numeric_limits<double>::max() )
                    {
                        m_design_rules.default_clearance = DESIGN_RULES().default_clearance;
                    }

                    m_design_rules.min_clearance = m_design_rules.default_clearance;

                    if( m_design_rules.copper_edge_clearance
                        == std::numeric_limits<double>::max() )
                    {
                        m_design_rules.copper_edge_clearance =
                                m_design_rules.default_clearance;
                    }

                    foundDefaultRules = true;
                }

                if( inDefaultRuleSet && braceDepth < defaultRuleSetDepth )
                    inDefaultRuleSet = false;
            }
        }

        std::istringstream iss( line );
        std::string token;
        iss >> token;

        // LAYER DATA block contains per-layer definitions (name, type, etc.)
        // which may appear inside *MISC* instead of as a standalone section.
        if( token == "LAYER" )
        {
            std::string secondToken;
            iss >> secondToken;

            if( secondToken == "DATA" )
            {
                parseSectionLAYERDEFS( aStream );
                continue;
            }
        }

        // Check for NET_CLASS DATA section header
        if( token == "NET_CLASS" )
        {
            std::string secondToken;
            iss >> secondToken;

            if( secondToken == "DATA" )
            {
                // Entering NET_CLASS DATA section
                inNetClassData = true;
            }
            else if( inNetClassData && !secondToken.empty() )
            {
                // Starting a new NET_CLASS definition inside NET_CLASS DATA
                // Format: NET_CLASS class_name
                if( inNetClass && !currentNetClass.name.empty() )
                    m_net_classes.push_back( currentNetClass );

                currentNetClass = NET_CLASS_DEF();
                currentNetClass.name = secondToken;
                inNetClass = true;
            }
        }
        else if( inNetClass && token == "NET" )
        {
            // Add net to current net class
            std::string netName;
            iss >> netName;

            if( !netName.empty() )
                currentNetClass.net_names.push_back( netName );
        }
        else if( token == "RULE_SET" && !foundDefaultRules )
        {
            // RULE_SET (1) is the default clearance rule set.
            // Parse it to extract board-level design rule defaults.
            std::string ruleNum;
            iss >> ruleNum;

            if( ruleNum == "(1)" )
            {
                inDefaultRuleSet = true;
                defaultRuleSetDepth = braceDepth;
            }
        }
        else if( inDefaultRuleSet && token == "CLEARANCE_RULE" )
        {
            inClearanceRule = true;
            clearanceRuleDepth = braceDepth;
            m_design_rules.default_clearance = std::numeric_limits<double>::max();
            m_design_rules.copper_edge_clearance = std::numeric_limits<double>::max();
        }
        else if( inClearanceRule )
        {
            double val = 0.0;
            iss >> val;

            if( !iss.fail() && val > 0.0 )
            {
                if( token == "MIN_TRACK_WIDTH" )
                {
                    m_design_rules.min_track_width = val;
                }
                else if( token == "REC_TRACK_WIDTH" )
                {
                    m_design_rules.default_track_width = val;
                }
                else if( token == "DRILL_TO_DRILL" )
                {
                    m_design_rules.hole_to_hole = val;
                }
                else if( token == "OUTLINE_TO_TRACK" || token == "OUTLINE_TO_VIA"
                         || token == "OUTLINE_TO_PAD" || token == "OUTLINE_TO_COPPER"
                         || token == "OUTLINE_TO_SMD" )
                {
                    m_design_rules.copper_edge_clearance =
                            std::min( m_design_rules.copper_edge_clearance, val );
                }
                else if( token.rfind( "SAME_NET_", 0 ) == 0 || token == "BODY_TO_BODY"
                         || token == "MAX_TRACK_WIDTH"
                         || token.rfind( "TEXT_TO_", 0 ) == 0
                         || token.rfind( "COPPER_TO_", 0 ) == 0 )
                {
                    // Exclude same-net spacings, physical body clearances, text
                    // clearances, and copper-pour clearances from the inter-net
                    // copper clearance.  In PADS the copper-pour (zone) clearance
                    // is resolved through the net/netclass rules, so it must not
                    // override the board-level default.
                }
                else if( token == "TRACK_TO_TRACK" || token.rfind( "VIA_TO_", 0 ) == 0
                         || token.rfind( "PAD_TO_", 0 ) == 0
                         || token.rfind( "SMD_TO_", 0 ) == 0
                         || token.rfind( "DRILL_TO_", 0 ) == 0 )
                {
                    // Standard inter-net copper clearance values determine the
                    // board-level default clearance.
                    m_design_rules.default_clearance =
                            std::min( m_design_rules.default_clearance, val );
                }
            }
        }
        else if( token == "DIF_PAIR" )
        {
            // Save previous diff pair if any
            if( inDifPair && !currentDiffPair.name.empty() )
                m_diff_pairs.push_back( currentDiffPair );

            currentDiffPair = DIFF_PAIR_DEF();
            iss >> currentDiffPair.name;
            inDifPair = true;
        }
        else if( inDifPair )
        {
            if( token == "NET" )
            {
                std::string netName;
                iss >> netName;

                // Assign to positive net first, then negative
                if( currentDiffPair.positive_net.empty() )
                    currentDiffPair.positive_net = netName;
                else if( currentDiffPair.negative_net.empty() )
                    currentDiffPair.negative_net = netName;
            }
            else if( token == "GAP" )
            {
                iss >> currentDiffPair.gap;
            }
            else if( token == "WIDTH" )
            {
                iss >> currentDiffPair.width;
            }
            else if( token == "CONNECTION" )
            {
                // CONNECTION format: ref.pin,ref.pin
                // This defines a pin pair for the diff pair
                // For now just skip - main net assignment is more important
            }
            else if( token == "ASSOCIATED" )
            {
                // ASSOCIATED NET netname - for associated net pairs
                std::string keyword, netName;
                iss >> keyword >> netName;

                if( keyword == "NET" )
                {
                    if( currentDiffPair.positive_net.empty() )
                        currentDiffPair.positive_net = netName;
                    else if( currentDiffPair.negative_net.empty() )
                        currentDiffPair.negative_net = netName;
                }
            }
        }

        // Parse per-instance attribute blocks: PART <refdes> { key value ... }
        // These appear inside ATTRIBUTE VALUES {...} at variable brace depth.
        if( token == "PART" && !inDifPair && !inNetClass )
        {
            std::string partName;
            iss >> partName;

            if( !partName.empty() )
            {
                // Save brace depth before consuming the block's own { ... }
                int savedDepth = braceDepth;
                auto& attrs = m_part_instance_attrs[partName];

                while( readLine( aStream, line ) )
                {
                    if( line.empty() )
                        continue;

                    if( line[0] == '}' )
                        break;

                    if( line[0] == '{' )
                        continue;

                    if( line[0] == '*' )
                    {
                        pushBackLine( line );
                        clampDesignRuleSentinels();
                        return;
                    }

                    std::string attrName, attrValue;

                    if( line[0] == '"' )
                    {
                        size_t endQuote = line.find( '"', 1 );

                        if( endQuote != std::string::npos )
                        {
                            attrName = line.substr( 1, endQuote - 1 );
                            attrValue = line.substr( endQuote + 1 );
                        }
                    }
                    else
                    {
                        std::istringstream attrSS( line );
                        attrSS >> attrName;
                        std::getline( attrSS >> std::ws, attrValue );
                    }

                    if( !attrValue.empty() && attrValue[0] == ' ' )
                        attrValue = attrValue.substr( 1 );

                    if( !attrName.empty() && !attrValue.empty() )
                        attrs[attrName] = attrValue;
                }

                // Restore to the depth before the PART block's braces
                braceDepth = savedDepth;
            }

            continue;
        }

        // Skip other MISC subsections (ATTRIBUTES DICTIONARY, DESIGN_RULES, etc.)
    }

    clampDesignRuleSentinels();
}


void PARSER::clampDesignRuleSentinels()
{
    DESIGN_RULES defaults;

    if( m_design_rules.default_clearance == std::numeric_limits<double>::max() )
        m_design_rules.default_clearance = defaults.default_clearance;

    if( m_design_rules.copper_edge_clearance == std::numeric_limits<double>::max() )
        m_design_rules.copper_edge_clearance = defaults.copper_edge_clearance;
}


std::vector<LAYER_INFO> PARSER::GetLayerInfos() const
{
    std::vector<LAYER_INFO> layers;

    int layerCount = m_parameters.layer_count;

    if( layerCount < 1 )
        layerCount = 2;

    // Helper to check if a layer number is a copper layer
    auto isCopperLayer = [&]( int num ) {
        return num >= 1 && num <= layerCount;
    };

    // Helper to get layer type from parsed defs or default
    auto getLayerDef = [&]( int num ) -> const LAYER_INFO* {
        auto it = m_layer_defs.find( num );
        return it != m_layer_defs.end() ? &it->second : nullptr;
    };

    // Add copper layers with parsed info if available
    for( int i = 1; i <= layerCount; ++i )
    {
        const LAYER_INFO* parsed = getLayerDef( i );

        if( parsed )
        {
            layers.push_back( *parsed );
        }
        else
        {
            // Generate default copper layer info
            LAYER_INFO info;
            info.number = i;
            info.layer_type = PADS_LAYER_FUNCTION::ROUTING;
            info.is_copper = true;
            info.required = true;

            if( i == 1 )
                info.name = "Top";
            else if( i == layerCount )
                info.name = "Bottom";
            else
                info.name = "Inner " + std::to_string( i - 1 );

            layers.push_back( info );
        }
    }

    // Add non-copper layers from parsed definitions
    for( const auto& [num, layerDef] : m_layer_defs )
    {
        if( !isCopperLayer( num ) )
        {
            layers.push_back( layerDef );
        }
    }

    // If no non-copper layers were parsed, add default fallbacks
    if( m_layer_defs.empty() )
    {
        // Standard non-copper layers (common PADS layer numbers)
        layers.push_back( { 21, "Assembly Top", PADS_LAYER_FUNCTION::ASSEMBLY, false, false } );
        layers.push_back( { 22, "Assembly Bottom", PADS_LAYER_FUNCTION::ASSEMBLY, false, false } );
        layers.push_back( { 25, "Solder Mask Top", PADS_LAYER_FUNCTION::SOLDER_MASK, false, false } );
        layers.push_back( { 26, "Silkscreen Top", PADS_LAYER_FUNCTION::SILK_SCREEN, false, false } );
        layers.push_back( { 27, "Silkscreen Bottom", PADS_LAYER_FUNCTION::SILK_SCREEN, false, false } );
        layers.push_back( { 28, "Solder Mask Bottom", PADS_LAYER_FUNCTION::SOLDER_MASK, false, false } );
        layers.push_back( { 29, "Paste Top", PADS_LAYER_FUNCTION::PASTE_MASK, false, false } );
        layers.push_back( { 30, "Paste Bottom", PADS_LAYER_FUNCTION::PASTE_MASK, false, false } );
    }

    return layers;
}

} // namespace PADS_IO
