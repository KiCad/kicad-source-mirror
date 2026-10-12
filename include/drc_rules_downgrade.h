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
#include <vector>

#include <wx/intl.h>

#include <dsnlexer.h>
#include <ki_exception.h>
#include <downgrade_target.h>
#include <drc_rules_release_vocabulary.h>

// The old rules parsers are all-or-nothing. One unknown keyword anywhere makes the target drop
// every custom rule and refuse to run DRC. So the export filters the file: rules the target
// cannot parse are commented out and reported, surviving rules are kept byte-identical.

/// The vocabulary a target release could parse, generated from its release tags by
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


/// A function a release parses but answers differently, which the vocabulary cannot say.
inline constexpr const char* DRC_RULE_SEMANTIC_BREAKS[] = {
    // Releases before 10.0.7 test a zone item's fill rather than its outline.
    "intersectsarea",
};


/// The generated arrays are sorted, so membership is a binary search.
inline bool DrcVocabularyHas( const char* const* aWords, size_t aCount, const std::string& aWord )
{
    return std::binary_search( aWords, aWords + aCount, aWord );
}


struct DRC_RULES_FILTER_RESULT
{
    wxString              m_text;    ///< The filtered rules file text
    std::vector<wxString> m_dropped; ///< Names of the rules the target cannot parse
    wxString              m_error;   ///< Invalid source syntax, do not write m_text
};


struct DRC_RULE_TOKEN
{
    int         m_type;   ///< A DSNLEXER token type
    std::string m_text;   ///< The token as the lexer reports it, strings without their quotes
    size_t      m_offset; ///< Byte offset of the token's first character in the source
};


/// A top-level block of the rules file, as byte span and token range.
struct DRC_RULE_BLOCK
{
    size_t      m_start = 0; ///< Byte offset of the opening parenthesis
    size_t      m_end = 0;   ///< Byte offset just past the closing parenthesis
    size_t      m_first = 0; ///< Index of the opening parenthesis token
    size_t      m_last = 0;  ///< Index of the closing parenthesis token
    std::string m_head;      ///< The word after the opening parenthesis, "rule" or "version"
    wxString    m_name;      ///< The name after a "rule" head, for the report
};


/// Tokenize with the lexer the rule readers use, so comments, strings and symbols end exactly
/// where they end for the readers. Comments stay as tokens so every byte of the source is placed.
inline bool TokenizeDrcRules( const std::string& aText, std::vector<DRC_RULE_TOKEN>& aTokens, wxString& aError )
{
    std::vector<size_t> lineStarts{ 0 };

    for( size_t i = 0; i < aText.size(); i++ )
    {
        if( aText[i] == '\n' )
            lineStarts.push_back( i + 1 );
    }

    DSNLEXER lexer( aText, wxT( "drc_rules_downgrade" ) );
    lexer.SetCommentsAreTokens( true );

    try
    {
        for( int type = lexer.NextTok(); type != DSN_EOF; type = lexer.NextTok() )
        {
            size_t offset = lineStarts[lexer.CurLineNumber() - 1] + lexer.CurOffset() - 1;

            // The lexer places a string at its first character, after the opening quote.
            if( type == DSN_STRING )
                offset--;

            aTokens.push_back( { type, lexer.CurText(), offset } );
        }
    }
    catch( const IO_ERROR& error )
    {
        aError = error.Problem();
        return false;
    }

    return true;
}


/// Group the tokens into top-level blocks. Anything but a comment outside a block is invalid for
/// the readers, and so is a comment that does not start its line.
inline bool SplitDrcRuleBlocks( const std::vector<DRC_RULE_TOKEN>& aTokens, std::vector<DRC_RULE_BLOCK>& aBlocks,
                                wxString& aError )
{
    int depth = 0;

    for( size_t i = 0; i < aTokens.size(); i++ )
    {
        const DRC_RULE_TOKEN& token = aTokens[i];

        if( token.m_type == DSN_SYMBOL && token.m_text[0] == '#' )
        {
            aError = _( "Design rule comments must start on a separate line." );
            return false;
        }

        if( token.m_type == DSN_COMMENT )
            continue;

        if( depth == 0 )
        {
            if( token.m_type != DSN_LEFT )
            {
                aError = _( "Design rules contain text outside a rule." );
                return false;
            }

            DRC_RULE_BLOCK block;
            block.m_start = token.m_offset;
            block.m_first = i;

            auto nextCode = [&aTokens]( size_t aFrom )
            {
                while( aFrom < aTokens.size() && aTokens[aFrom].m_type == DSN_COMMENT )
                    aFrom++;

                return aFrom;
            };

            size_t head = nextCode( i + 1 );

            if( head < aTokens.size() && aTokens[head].m_type == DSN_SYMBOL )
                block.m_head = aTokens[head].m_text;

            size_t name = nextCode( head + 1 );

            if( block.m_head == "rule" && name < aTokens.size() && aTokens[name].m_type != DSN_LEFT
                && aTokens[name].m_type != DSN_RIGHT )
            {
                block.m_name = wxString::FromUTF8( aTokens[name].m_text.c_str() );
            }

            aBlocks.push_back( block );
        }

        if( token.m_type == DSN_LEFT )
        {
            depth++;
        }
        else if( token.m_type == DSN_RIGHT )
        {
            depth--;

            if( depth == 0 )
            {
                aBlocks.back().m_end = token.m_offset + 1;
                aBlocks.back().m_last = i;
            }
        }
    }

    if( depth != 0 )
    {
        aError = _( "Unbalanced parentheses in design rules." );
        return false;
    }

    return true;
}


/// True if the rule uses something the target's parser or evaluator cannot handle.
inline bool DrcRuleUnsupportedByTarget( const std::vector<DRC_RULE_TOKEN>& aTokens, const DRC_RULE_BLOCK& aBlock,
                                        const DRC_RULE_VOCABULARY& aVocabulary )
{
    std::vector<std::string> nodes;
    std::vector<std::string> words;
    std::string              expressions;
    std::string              values;
    bool                     headPending = false;
    bool                     kindPending = false;
    bool                     namePending = false;

    // String literals inside an expression are not names, so only their quotes are kept as gaps.
    auto addExpression = [&expressions]( const std::string& aExpression )
    {
        char quote = 0;

        for( size_t j = 0; j < aExpression.size(); j++ )
        {
            char c = aExpression[j];

            if( quote )
            {
                if( c == '\\' && j + 1 < aExpression.size() )
                    j++;
                else if( c == quote )
                    quote = 0;
            }
            else if( c == '\'' || c == '"' )
            {
                quote = c;
                expressions += ' ';
            }
            else
            {
                expressions += c;
            }
        }

        expressions += '\n';
    };

    for( size_t i = aBlock.m_first; i <= aBlock.m_last; i++ )
    {
        const DRC_RULE_TOKEN& token = aTokens[i];

        if( token.m_type == DSN_COMMENT )
            continue;

        if( token.m_type == DSN_LEFT )
        {
            nodes.emplace_back();
            headPending = true;
            continue;
        }

        if( token.m_type == DSN_RIGHT )
        {
            // A node with no head is not grammar for any reader.
            if( headPending )
                return true;

            nodes.pop_back();
            continue;
        }

        if( token.m_text.find( "${" ) != std::string::npos )
        {
            // A text variable is syntax, and a target without it fails to read the rule at all.
            if( !aVocabulary.m_textVariables )
                return true;

            std::string folded = token.m_text;

            for( char& c : folded )
                c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );

            // Releases before 10.0.6 read a component class selector as a reference designator.
            if( folded.find( "${class:" ) != std::string::npos )
                return true;
        }

        if( headPending )
        {
            headPending = false;

            if( token.m_type != DSN_SYMBOL )
                return true;

            nodes.back() = token.m_text;
            words.push_back( token.m_text );
            kindPending = token.m_text == "constraint";
            namePending = token.m_text == "rule";
            continue;
        }

        const std::string& node = nodes.back();

        if( namePending )
        {
            namePending = false;
        }
        else if( kindPending )
        {
            // The constraint kind is grammar, and three kinds change how what follows is read.
            kindPending = false;
            words.push_back( token.m_text );

            if( token.m_text == "assertion" || token.m_text == "disallow" || token.m_text == "zone_connection" )
                nodes.back() = token.m_text;
        }
        else if( node == "condition" || node == "assertion" )
        {
            addExpression( token.m_text );
        }
        else if( node == "min" || node == "max" || node == "opt" )
        {
            values += token.m_text + ' ';
        }
        else if( node == "layer" )
        {
            // A layer node names a board layer, which is not grammar.
        }
        else if( node == "disallow" || node == "zone_connection" )
        {
            // The readers accept these items quoted, so they are keywords in disguise.
            words.push_back( token.m_text );
        }
        else if( token.m_type != DSN_STRING && token.m_type != DSN_NUMBER )
        {
            words.push_back( token.m_text );
        }
    }

    for( const std::string& word : words )
    {
        // A dotted name is a layer or similar, never grammar.
        if( word.find( '.' ) != std::string::npos )
            continue;

        if( !DrcVocabularyHas( aVocabulary.m_tokens, aVocabulary.m_tokenCount, word ) )
            return true;
    }

    auto isWord = []( char c )
    {
        return std::isalnum( static_cast<unsigned char>( c ) ) || c == '_';
    };

    std::string folded = expressions;

    for( char& c : folded )
        c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );

    // Inside an expression a word followed by a parenthesis is a function call, and a word after
    // a dot is a property. Both must exist in the target.
    for( size_t i = 0; i < folded.size(); i++ )
    {
        if( !std::isalpha( static_cast<unsigned char>( folded[i] ) ) && folded[i] != '_' )
            continue;

        if( i > 0 && isWord( folded[i - 1] ) )
            continue;

        size_t end = i;

        while( end < folded.size() && isWord( folded[end] ) )
            end++;

        const std::string word = folded.substr( i, end - i );

        size_t next = end;

        while( next < folded.size() && std::isspace( static_cast<unsigned char>( folded[next] ) ) )
            next++;

        size_t prev = i;

        while( prev > 0 && std::isspace( static_cast<unsigned char>( folded[prev - 1] ) ) )
            prev--;

        if( next < folded.size() && folded[next] == '(' )
        {
            for( const char* broken : DRC_RULE_SEMANTIC_BREAKS )
            {
                if( word == broken )
                    return true;
            }

            if( !DrcVocabularyHas( aVocabulary.m_functions, aVocabulary.m_functionCount, word ) )
                return true;
        }
        else if( prev > 0 && folded[prev - 1] == '.' )
        {
            // Parent navigation is new, but the old standalone Parent string property is supported.
            if( word == "parent" && next < folded.size() && folded[next] == '.' )
                return true;

            if( !DrcVocabularyHas( aVocabulary.m_properties, aVocabulary.m_propertyCount, word ) )
                return true;
        }

        i = end;
    }

    // A unit is the letters after a number, glued to it or not. It must be one the target's
    // value resolver knows.
    const std::string numeric = values + '\n' + expressions;

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
            && !DrcVocabularyHas( aVocabulary.m_units, aVocabulary.m_unitCount, numeric.substr( i, end - i ) ) )
        {
            return true;
        }

        i = end;
    }

    return false;
}


/// Prefix every line, so the target reads the whole block as comments.
inline std::string CommentOutDrcBlock( const std::string& aBlock )
{
    std::string out = "# ";

    for( char c : aBlock )
    {
        out += c;

        if( c == '\n' )
            out += "# ";
    }

    return out;
}


/// Filter a .kicad_dru file for the target. Kept rules stay byte-identical, unsupported rules
/// become comments and are listed by name. On a syntax error m_error is set, write nothing.
inline DRC_RULES_FILTER_RESULT FilterDrcRulesForTarget( const wxString& aRulesText, const DOWNGRADE_TARGET& aTarget )
{
    std::string text = aRulesText.ToStdString( wxConvUTF8 );

    DRC_RULES_FILTER_RESULT result;

    const DRC_RULE_VOCABULARY* vocabulary = FindDrcRuleVocabulary( aTarget );

    if( !vocabulary )
    {
        result.m_error = wxString::Format( _( "No design rule vocabulary is recorded for %s." ), aTarget.m_name );
        return result;
    }

    std::vector<DRC_RULE_TOKEN> tokens;
    std::vector<DRC_RULE_BLOCK> blocks;

    if( !TokenizeDrcRules( text, tokens, result.m_error ) || !SplitDrcRuleBlocks( tokens, blocks, result.m_error ) )
        return result;

    const wxString header = wxT( "(version 1)" );
    const wxString eol = text.find( "\r\n" ) != std::string::npos ? wxT( "\r\n" ) : wxT( "\n" );

    size_t consumed = 0;
    bool   headerWritten = false;
    bool   commentedOut = false;

    auto endsOnBlankLine = []( const wxString& aText )
    {
        int lineStart = aText.Find( '\n', true );
        return aText.Mid( lineStart == wxNOT_FOUND ? 0 : lineStart + 1 ).Trim().IsEmpty();
    };

    for( const DRC_RULE_BLOCK& block : blocks )
    {
        // A comment travels with the block after it, so a dropped rule takes its comment along.
        wxString prefix = wxString::FromUTF8( text.substr( consumed, block.m_start - consumed ).c_str() );
        std::string body = text.substr( block.m_start, block.m_end - block.m_start );
        consumed = block.m_end;

        // The version header is rewritten in place, so a comment above it stays above it.
        if( block.m_head == "version" )
        {
            result.m_text += prefix;

            if( !headerWritten )
                result.m_text += header;

            headerWritten = true;
            continue;
        }

        if( !headerWritten )
        {
            result.m_text += header + eol;
            headerWritten = true;
        }

        bool unsupported = block.m_head != "rule" || DrcRuleUnsupportedByTarget( tokens, block, *vocabulary );

        result.m_text += prefix;

        // A comment runs to the end of its line, so a commented block and whatever follows it
        // each start on a line of their own.
        if( ( unsupported || commentedOut ) && !endsOnBlankLine( result.m_text ) )
            result.m_text += eol;

        if( unsupported )
        {
            result.m_dropped.push_back( block.m_name );
            result.m_text += wxT( "# Not supported by " ) + aTarget.m_name + wxT( ":" ) + eol;
            result.m_text += wxString::FromUTF8( CommentOutDrcBlock( body ).c_str() );
        }
        else
        {
            result.m_text += wxString::FromUTF8( body.c_str() );
        }

        commentedOut = unsupported;
    }

    if( !headerWritten )
        result.m_text += header + eol;

    // Comments after the last block, which by now include every commented rule that was last.
    result.m_text += wxString::FromUTF8( text.substr( consumed ).c_str() );

    if( !result.m_text.EndsWith( wxT( "\n" ) ) )
        result.m_text += eol;

    return result;
}

#endif // DRC_RULES_DOWNGRADE_H
