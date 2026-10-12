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

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include <wx/intl.h>

#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <drc_rules_release_vocabulary.h>

// The old rules parsers are all-or-nothing. One unknown keyword anywhere makes the target drop
// every custom rule and refuse to run DRC. So the export filters the file: rules the target
// cannot parse are removed and reported, surviving rules are kept byte-identical.

/// The vocabulary a target release could parse, generated from its branch by
/// tools/generate_drc_rules_vocabulary.py. A release without one cannot be exported to.
inline const DRC_RULE_VOCABULARY* FindDrcRuleVocabulary( const DOWNGRADE_TARGET& aTarget )
{
    for( const DRC_RULE_VOCABULARY& vocabulary : DRC_RULE_VOCABULARIES )
    {
        if( aTarget.m_id == vocabulary.m_targetId )
            return &vocabulary;
    }

    return nullptr;
}


/// A function a release parses but must still drop, because the result it gives differs from
/// the one the rule was written against. The vocabulary cannot express that, so it is kept here
/// with the reason. Tokens dated 99999999 drop for every target.
static constexpr DATED_TOKEN DRC_RULE_SEMANTIC_BREAKS[] = {
    // Releases before 10.0.7 test zone fills rather than the source's zone outlines.
    { "intersectsarea", 99999999 },
};


/// The generated arrays are sorted, so membership is a binary search.
inline bool DrcVocabularyHas( const char* const* aWords, size_t aCount, const std::string& aWord )
{
    return std::binary_search( aWords, aWords + aCount, aWord,
                               []( std::string_view aLeft, std::string_view aRight )
                               {
                                   return aLeft < aRight;
                               } );
}


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

    const DRC_RULE_VOCABULARY* vocabulary = FindDrcRuleVocabulary( aTarget );

    if( !vocabulary )
        return true;

    auto isWord = []( char c )
    {
        return std::isalnum( static_cast<unsigned char>( c ) ) || c == '_';
    };

    // Every bareword left in the rule must be in the target's grammar. Rule names are blanked
    // and strings stripped above, a unit stuck to a number is not a word, and a dotted name
    // like a layer is left alone.
    for( size_t i = 0; i < structural.size(); i++ )
    {
        if( !std::isalpha( static_cast<unsigned char>( structural[i] ) ) && structural[i] != '_' )
            continue;

        if( i > 0 && ( isWord( structural[i - 1] ) || structural[i - 1] == '.' ) )
            continue;

        size_t end = i;

        while( end < structural.size() && isWord( structural[end] ) )
            end++;

        if( end < structural.size() && structural[end] == '.' )
        {
            i = end;
            continue;
        }

        const std::string word = structural.substr( i, end - i );

        if( !DrcVocabularyHas( vocabulary->m_tokens, vocabulary->m_tokenCount, word ) )
            return true;

        // A layer node names a board layer, and a min, max or opt node holds a value whose unit
        // may stand apart from its number. Neither is grammar.
        if( i > 0 && structural[i - 1] == '('
            && ( word == "layer" || word == "min" || word == "max" || word == "opt" ) )
        {
            while( end < structural.size() && structural[end] != ')' )
                end++;
        }

        i = end;
    }

    // Inside a condition a word followed by a parenthesis is a function call, and a word after
    // a dot is a property. Both must exist in the target.
    for( size_t i = 0; i < foldedConditions.size(); i++ )
    {
        if( !std::isalpha( static_cast<unsigned char>( foldedConditions[i] ) ) && foldedConditions[i] != '_' )
            continue;

        if( i > 0 && isWord( foldedConditions[i - 1] ) )
            continue;

        size_t end = i;

        while( end < foldedConditions.size() && isWord( foldedConditions[end] ) )
            end++;

        const std::string word = foldedConditions.substr( i, end - i );

        size_t next = end;

        while( next < foldedConditions.size() && std::isspace( static_cast<unsigned char>( foldedConditions[next] ) ) )
            next++;

        size_t prev = i;

        while( prev > 0 && std::isspace( static_cast<unsigned char>( foldedConditions[prev - 1] ) ) )
            prev--;

        if( next < foldedConditions.size() && foldedConditions[next] == '(' )
        {
            for( const DATED_TOKEN& token : DRC_RULE_SEMANTIC_BREAKS )
            {
                if( aTarget.m_boardVersion < token.m_introducedIn && word == token.m_token )
                    return true;
            }

            if( !DrcVocabularyHas( vocabulary->m_functions, vocabulary->m_functionCount, word ) )
                return true;
        }
        else if( prev > 0 && foldedConditions[prev - 1] == '.' )
        {
            // Parent navigation is new, but the old standalone Parent string property is supported.
            if( word == "parent" && next < foldedConditions.size() && foldedConditions[next] == '.' )
                return true;

            if( !DrcVocabularyHas( vocabulary->m_properties, vocabulary->m_propertyCount, word ) )
                return true;
        }

        i = end;
    }

    // A unit is the letters after a number, glued to it or not. It must be one the target's
    // value resolver knows. An unquoted layer name is blanked first so it is never read as one.
    std::string numeric = structural;

    for( size_t at = numeric.find( "(layer" ); at != std::string::npos; at = numeric.find( "(layer", at + 1 ) )
    {
        size_t close = numeric.find( ')', at );

        if( close != std::string::npos )
            numeric.replace( at + 6, close - at - 6, close - at - 6, ' ' );
    }

    numeric += '\n' + values + conditions;

    for( size_t i = 0; i < numeric.size(); i++ )
    {
        if( !std::isalpha( static_cast<unsigned char>( numeric[i] ) ) )
            continue;

        if( i > 0 && ( std::isalpha( static_cast<unsigned char>( numeric[i - 1] ) ) || numeric[i - 1] == '_' ) )
            continue;

        size_t before = i;

        while( before > 0 && std::isspace( static_cast<unsigned char>( numeric[before - 1] ) ) )
            before--;

        size_t end = i;

        while( end < numeric.size() && isWord( numeric[end] ) )
            end++;

        if( before > 0 && std::isdigit( static_cast<unsigned char>( numeric[before - 1] ) )
            && !DrcVocabularyHas( vocabulary->m_units, vocabulary->m_unitCount, numeric.substr( i, end - i ) ) )
        {
            return true;
        }

        i = end;
    }

    // A text variable is syntax, and a target without it fails to read the rule at all.
    if( !vocabulary->m_textVariables && block.find( "${" ) != std::string::npos )
        return true;

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

    if( !FindDrcRuleVocabulary( aTarget ) )
    {
        result.m_error = wxString::Format( _( "No design rule vocabulary is recorded for %s." ), aTarget.m_name );
        return result;
    }

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
