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

#ifndef DRC_RULES_DOWNGRADE_H
#define DRC_RULES_DOWNGRADE_H

#include <cctype>
#include <string>
#include <vector>

#include <wx/intl.h>

#include <downgrade_scan.h>
#include <downgrade_target.h>

// The old rules parsers are all-or-nothing. One unknown keyword anywhere makes the target drop
// every custom rule and refuse to run DRC. So the export filters the file: rules the target
// cannot parse are removed and reported, surviving rules are kept byte-identical.

/// Rule-file tokens dated by the board format of the first release that parses them. A rule
/// using a token newer than the target is dropped. Tokens dated 99999999 have no released
/// parser yet and drop for every target.
static constexpr DATED_TOKEN DRC_RULE_TOKENS[] = {
    // Constraints and disallow kinds added in 10.0
    { "bridged_mask", 20260206 },
    { "solder_mask_expansion", 20260206 },
    { "solder_mask_sliver", 20260206 },
    { "solder_paste_abs_margin", 20260206 },
    { "solder_paste_rel_margin", 20260206 },
    { "via_dangling", 20260206 },
    { "through_via", 20260206 },
    { "blind_via", 20260206 },
    // Expression functions added in 10.0
    { "isBlindVia", 20260206 },
    { "isBuriedVia", 20260206 },
    { "memberOfSheetOrChildren", 20260206 },
    { "hasExactNetclass", 20260206 },
    // Added after 10.0
    { "net_chain_length", 99999999 },
    { "stub_length", 99999999 },
    { "return_path", 99999999 },
    { "inNetChain", 99999999 },
    { "hasNetChain", 99999999 },
    { "inNetChainClass", 99999999 },
    { "microvia_aspect_ratio", 99999999 },
    { "microvia_stack_depth", 99999999 },
    { "intersectsKeepout", 99999999 },
    { "isStackedVia", 99999999 },
    { "customProperty", 99999999 },
    { "hasCustomProperty", 99999999 },
    // Released readers test zone fills rather than the source's zone outlines.
    { "intersectsArea", 99999999 },
};

/// Property names exposed by PCB expressions, normalized like the evaluator's underscore aliases.
/// A property absent from the released registry makes the whole rules file fail to compile.
static constexpr DATED_TOKEN DRC_RULE_PROPERTIES[] = {
    // Released in 10.0
    { "corner_radius", 20260206 },
    { "fill", 20260206 },
    { "auto_thickness", 20260206 },
    { "target_delay", 20260206 },
    { "target_skew_delay", 20260206 },
    { "top_counterbore_depth", 20260206 },
    { "top_countersink_angle", 20260206 },
    { "bottom_counterbore_depth", 20260206 },
    { "bottom_countersink_angle", 20260206 },
    { "backdrill_mode", 20260206 },
    { "bottom_backdrill_size", 20260206 },
    { "top_backdrill_size", 20260206 },
    { "pad_to_die_delay", 20260206 },
    { "show_text", 20260206 },
    { "text_size", 20260206 },
    { "barcode_type", 20260206 },
    { "error_correction", 20260206 },
    { "margin_x", 20260206 },
    { "margin_y", 20260206 },
    { "column_width", 20260206 },
    { "row_height", 20260206 },
    { "capping", 20260206 },
    { "filling", 20260206 },
    { "keep_out_zone_fills", 20260206 },
    { "hatch_orientation", 20260206 },
    // Added after 10.0
    { "major_radius", 99999999 },
    { "minor_radius", 99999999 },
    { "ellipse_rotation", 99999999 },
    { "arc_start_angle", 99999999 },
    { "arc_end_angle", 99999999 },
    { "start_shape", 99999999 },
    { "start_length", 99999999 },
    { "start_width", 99999999 },
    { "start_stroke_width", 99999999 },
    { "end_shape", 99999999 },
    { "end_length", 99999999 },
    { "end_width", 99999999 },
    { "end_stroke_width", 99999999 },
    { "driving", 99999999 },
    { "scale_x", 99999999 },
    { "scale_y", 99999999 },
    { "footprint_type", 99999999 },
    { "exclude_from_simulation", 99999999 },
    { "start_layer", 99999999 },
    { "end_layer", 99999999 },
    { "style", 99999999 },
    { "use_netclass_values", 99999999 },
    { "via_diameter", 99999999 },
    { "via_hole", 99999999 },
    { "pitch", 99999999 },
    { "capped", 99999999 },
    { "drill", 99999999 },
    { "layout", 99999999 },
    { "mode", 99999999 },
    { "seed", 99999999 },
    { "guarded_net", 99999999 },
    { "simulation_electrical_type", 99999999 },
    { "value_mode", 99999999 },
    { "show_totals", 99999999 },
    { "offset_x", 99999999 },
    { "offset_y", 99999999 },
    { "symbol_size", 99999999 },
    { "hole_span", 99999999 },
    { "outline_slots", 99999999 },
    { "guide_cross", 99999999 },
    { "decimal_places", 99999999 },
    { "grid_type", 99999999 },
    { "extent_x", 99999999 },
    { "extent_y", 99999999 },
    { "spacing_x", 99999999 },
    { "spacing_y", 99999999 },
    { "radius_extent", 99999999 },
    { "radius_spacing", 99999999 },
    { "phi_extent", 99999999 },
    { "phi_spacing", 99999999 },
    { "tick_interval", 99999999 },
    { "affects_cursor_snap", 99999999 },
    { "affects_routing", 99999999 },
    { "affects_placement", 99999999 },
    { "automatically_update_net", 99999999 },
    { "front_tenting", 99999999 },
    { "back_tenting", 99999999 },
    { "front_covering", 99999999 },
    { "back_covering", 99999999 },
    { "front_plugging", 99999999 },
    { "back_plugging", 99999999 },
    { "thieving_pattern", 99999999 },
    { "thieving_element_size", 99999999 },
    { "thieving_gap", 99999999 },
    { "thieving_line_width", 99999999 },
    { "thieving_stagger", 99999999 },
    { "thieving_orientation", 99999999 },
};


struct DRC_RULES_FILTER_RESULT
{
    wxString              m_text;    ///< The filtered rules file text
    std::vector<wxString> m_dropped; ///< Names of the rules the target cannot parse
    wxString              m_error;   ///< Invalid source syntax; do not write m_text
};


/// The rule name following a "(rule" head, for the report.
inline wxString ExtractDrcRuleName( const std::string& aBlock )
{
    size_t pos = aBlock.find( "(rule" );

    if( pos == std::string::npos )
        return wxEmptyString;

    pos += 5;

    while( pos < aBlock.size() && std::isspace( static_cast<unsigned char>( aBlock[pos] ) ) )
        pos++;

    if( pos >= aBlock.size() )
        return wxEmptyString;

    std::string name;

    if( aBlock[pos] == '"' )
    {
        for( pos++; pos < aBlock.size() && aBlock[pos] != '"'; pos++ )
        {
            if( aBlock[pos] == '\\' && pos + 1 < aBlock.size() )
                pos++;

            name += aBlock[pos];
        }
    }
    else
    {
        while( pos < aBlock.size() && !std::isspace( static_cast<unsigned char>( aBlock[pos] ) ) && aBlock[pos] != '('
               && aBlock[pos] != ')' )
        {
            name += aBlock[pos++];
        }
    }

    return wxString::FromUTF8( name.c_str() );
}


/// Drop # comments, keeping quoted strings intact, so a comment cannot trip the token or
/// unit scans below.
inline std::string StripDrcComments( const std::string& aText )
{
    std::string out;
    bool        inString = false;

    for( size_t i = 0; i < aText.size(); i++ )
    {
        char c = aText[i];

        if( inString )
        {
            if( c == '\\' && i + 1 < aText.size() )
            {
                out += c;
                out += aText[++i];
                continue;
            }

            if( c == '"' )
                inString = false;

            out += c;
            continue;
        }

        if( c == '"' )
        {
            inString = true;
        }
        else if( c == '#' )
        {
            while( i < aText.size() && aText[i] != '\n' )
                i++;

            if( i >= aText.size() )
                break;

            c = '\n';
        }

        out += c;
    }

    return out;
}


/// True if the rule text uses something the target's parser or evaluator cannot handle.
inline bool DrcRuleUnsupportedByTarget( const std::string& aRawBlock, const DOWNGRADE_TARGET& aTarget )
{
    std::string              block = StripDrcComments( aRawBlock );
    std::string              conditions;
    std::string              values;
    std::vector<std::string> nodes;

    for( size_t i = 0; i < block.size(); i++ )
    {
        if( block[i] == '(' )
        {
            size_t start = ++i;

            while( i < block.size() && !std::isspace( static_cast<unsigned char>( block[i] ) ) && block[i] != '('
                   && block[i] != ')' )
                i++;

            std::string node = block.substr( start, i - start );

            if( node == "constraint" )
            {
                size_t kind = i;

                while( kind < block.size() && std::isspace( static_cast<unsigned char>( block[kind] ) ) )
                    kind++;

                size_t end = kind;

                while( end < block.size() && !std::isspace( static_cast<unsigned char>( block[end] ) )
                       && block[end] != '(' && block[end] != ')' )
                {
                    end++;
                }

                if( block.substr( kind, end - kind ) == "assertion" )
                    node = "assertion";
            }

            nodes.push_back( std::move( node ) );
            i--;
        }
        else if( block[i] == ')' )
        {
            if( !nodes.empty() )
                nodes.pop_back();
        }
        else if( block[i] == '"' )
        {
            std::string value;

            while( ++i < block.size() && block[i] != '"' )
            {
                if( block[i] == '\\' && i + 1 < block.size() )
                    i++;

                value += block[i];
            }

            if( !nodes.empty() && ( nodes.back() == "condition" || nodes.back() == "assertion" ) )
            {
                char quote = 0;

                for( size_t j = 0; j < value.size(); j++ )
                {
                    char c = value[j];

                    if( quote )
                    {
                        if( c == '\\' && j + 1 < value.size() )
                            j++;
                        else if( c == quote )
                            quote = 0;
                    }
                    else if( c == '\'' || c == '"' )
                    {
                        quote = c;
                        conditions += ' ';
                    }
                    else
                    {
                        conditions += c;
                    }
                }

                conditions += '\n';
            }
            else if( !nodes.empty() && ( nodes.back() == "min" || nodes.back() == "max" || nodes.back() == "opt" ) )
            {
                values += value + '\n';
            }
        }
    }

    // Rule names are labels, including when the source uses an unquoted name.
    size_t name = block.find( "(rule" );

    if( name != std::string::npos )
    {
        name += 5;

        while( name < block.size() && std::isspace( static_cast<unsigned char>( block[name] ) ) )
            name++;

        if( name < block.size() && block[name] != '"' )
        {
            size_t end = name;

            while( end < block.size() && !std::isspace( static_cast<unsigned char>( block[end] ) ) && block[end] != '('
                   && block[end] != ')' )
                end++;

            block.replace( name, end - name, end - name, ' ' );
        }
    }

    const std::string structural = StripSexprStrings( block );

    std::string foldedConditions = conditions;

    for( char& c : foldedConditions )
        c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );

    for( const DATED_TOKEN& token : DRC_RULE_TOKENS )
    {
        if( aTarget.m_boardVersion >= token.m_introducedIn )
            continue;

        if( ContainsSexprValue( structural, token.m_token ) )
            return true;

        std::string function = token.m_token;

        for( char& c : function )
            c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );

        for( size_t pos = foldedConditions.find( function ); pos != std::string::npos;
             pos = foldedConditions.find( function, pos + 1 ) )
        {
            if( pos > 0
                && ( std::isalnum( static_cast<unsigned char>( foldedConditions[pos - 1] ) )
                     || foldedConditions[pos - 1] == '_' ) )
                continue;

            size_t after = pos + function.size();

            while( after < foldedConditions.size()
                   && std::isspace( static_cast<unsigned char>( foldedConditions[after] ) ) )
                after++;

            if( after < foldedConditions.size() && foldedConditions[after] == '(' )
                return true;
        }
    }

    for( size_t pos = 0; pos < foldedConditions.size(); pos++ )
    {
        if( foldedConditions[pos] != '.' )
            continue;

        size_t begin = pos + 1;

        while( begin < foldedConditions.size()
               && std::isspace( static_cast<unsigned char>( foldedConditions[begin] ) ) )
        {
            begin++;
        }

        size_t end = begin;

        while( end < foldedConditions.size()
               && ( std::isalnum( static_cast<unsigned char>( foldedConditions[end] ) )
                    || foldedConditions[end] == '_' ) )
        {
            end++;
        }

        const std::string property = foldedConditions.substr( begin, end - begin );

        // Parent navigation is new, but the old standalone Parent string property is supported.
        if( property == "parent" )
        {
            size_t next = end;

            while( next < foldedConditions.size()
                   && std::isspace( static_cast<unsigned char>( foldedConditions[next] ) ) )
            {
                next++;
            }

            if( next < foldedConditions.size() && foldedConditions[next] == '.' )
                return true;
        }

        for( const DATED_TOKEN& token : DRC_RULE_PROPERTIES )
        {
            if( aTarget.m_boardVersion < token.m_introducedIn && property == token.m_token )
                return true;
        }

        pos = end > begin ? end - 1 : pos;
    }

    // Time and angle value units appeared in 10.0.
    if( aTarget.m_boardVersion < 20260206 )
    {
        auto isWordChar = []( char c )
        {
            return std::isalnum( static_cast<unsigned char>( c ) ) || c == '_';
        };

        const std::string numeric = structural + '\n' + values + conditions;

        for( size_t i = 1; i + 1 < numeric.size(); i++ )
        {
            size_t before = i;

            while( before > 0 && std::isspace( static_cast<unsigned char>( numeric[before - 1] ) ) )
                before--;

            if( before == 0
                || ( !std::isdigit( static_cast<unsigned char>( numeric[before - 1] ) )
                     && numeric[before - 1] != '.' ) )
                continue;

            if( ( numeric[i] == 'f' || numeric[i] == 'p' ) && numeric[i + 1] == 's'
                && ( i + 2 >= numeric.size() || !isWordChar( numeric[i + 2] ) ) )
            {
                return true;
            }

            if( numeric.compare( i, 3, "deg" ) == 0 && ( i + 3 >= numeric.size() || !isWordChar( numeric[i + 3] ) ) )
                return true;
        }
    }

    return false;
}


/// Match the full-line comment syntax of DSNLEXER, used by both target readers.
inline bool IsDrcCommentAtStartOfLine( const std::string& aText, size_t aOffset )
{
    while( aOffset > 0 && aText[aOffset - 1] != '\n' )
    {
        if( !std::isspace( static_cast<unsigned char>( aText[--aOffset] ) ) )
            return false;
    }

    return true;
}


/// Filter a .kicad_dru file for the target. Surviving rules are kept byte-identical, dropped
/// rules are listed by name, and the version header is rewritten to what the target writes.
/// A syntax error sets m_error; callers must refuse to write the partial result.
inline DRC_RULES_FILTER_RESULT FilterDrcRulesForTarget( const wxString& aRulesText, const DOWNGRADE_TARGET& aTarget )
{
    std::string text = aRulesText.ToStdString( wxConvUTF8 );

    DRC_RULES_FILTER_RESULT result;
    result.m_text = wxT( "(version 1)\n" );

    size_t i = 0;

    while( i < text.size() )
    {
        // A comment travels with the block after it, so a dropped rule takes its comment along.
        size_t prefixStart = i;

        while( i < text.size() && text[i] != '(' )
        {
            if( text[i] == '#' )
            {
                if( !IsDrcCommentAtStartOfLine( text, i ) )
                {
                    result.m_error = _( "Design rule comments must start on a separate line." );
                    return result;
                }

                while( i < text.size() && text[i] != '\n' )
                    i++;
            }
            else
            {
                i++;
            }
        }

        if( i >= text.size() )
            break;

        std::string prefix = text.substr( prefixStart, i - prefixStart );

        size_t blockStart = i;
        int    depth = 0;
        bool   inString = false;

        for( ; i < text.size(); i++ )
        {
            char c = text[i];

            if( inString )
            {
                if( c == '\\' && i + 1 < text.size() )
                    i++;
                else if( c == '"' )
                    inString = false;

                continue;
            }

            // Comments can contain unmatched parentheses, quotes, and escapes. Preserve
            // their bytes in the block, but never interpret them as s-expression syntax.
            if( c == '#' )
            {
                // Match DSNLEXER: inline comments are invalid in both target readers.
                // Refuse them instead of claiming an invalid rules file was exported.
                if( !IsDrcCommentAtStartOfLine( text, i ) )
                {
                    result.m_error = _( "Design rule comments must start on a separate line." );
                    return result;
                }

                while( i < text.size() && text[i] != '\n' )
                    i++;
            }
            else if( c == '"' )
                inString = true;
            else if( c == '(' )
                depth++;
            else if( c == ')' && --depth == 0 )
            {
                i++;
                break;
            }
        }

        std::string block = text.substr( blockStart, i - blockStart );

        if( depth != 0 || inString )
        {
            result.m_error = _( "Unbalanced parentheses or unterminated string in design rules." );
            return result;
        }

        // The version header is rewritten, never copied.
        if( block.compare( 0, 8, "(version" ) == 0 )
            continue;

        bool isRule = block.compare( 0, 5, "(rule" ) == 0
                      && ( block.size() <= 5
                           || ( !std::isalnum( static_cast<unsigned char>( block[5] ) ) && block[5] != '_' ) );

        if( !isRule || DrcRuleUnsupportedByTarget( block, aTarget ) )
        {
            result.m_dropped.push_back( ExtractDrcRuleName( block ) );
            continue;
        }

        result.m_text += wxString::FromUTF8( ( prefix + block ).c_str() );
    }

    result.m_text += wxT( "\n" );

    return result;
}

#endif // DRC_RULES_DOWNGRADE_H
