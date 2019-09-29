/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <regex>
#include <string>

#include "blockparser.h"

// -----------------------------------------------------------------------------

namespace maddy {
// -----------------------------------------------------------------------------

/**
 * UnorderedListParser
 *
 * @class
 */
class UnorderedListParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} aParseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} aGetBlockParserForLineCallback
     */
    UnorderedListParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback ),
        isStarted( false ),
        isFinished( false )
    {}

    /**
     * IsStartingLine
     *
     * An unordered list starts with `* `.
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        static std::regex re( "^\\* .*" );

        return std::regex_match( line, re );
    }

    /**
     * IsFinished
     *
     * @method
     * @return {bool}
     */
    bool IsFinished() const override
    {
        return this->isFinished;
    }

protected:
    bool isInlineBlockAllowed() const override
    {
        return true;
    }

    bool isLineParserAllowed() const override
    {
        return true;
    }

    void parseBlock( std::string& line ) override
    {
        bool isStartOfNewListItem = IsStartingLine( line );
        uint32_t indentation = getIndentationWidth( line );

        static std::regex lineRegex( "^(\\* )" );

        line = std::regex_replace( line, lineRegex, "" );

        if( !this->isStarted )
        {
            line = "<ul><li>" + line;
            this->isStarted = true;
            return;
        }

        if( indentation >= 2 )
        {
            line = line.substr( 2 );
            return;
        }

        if(
            line.empty()
            || line.find( "</li><li>" ) != std::string::npos
            || line.find( "</li></ol>" ) != std::string::npos
            || line.find( "</li></ul>" ) != std::string::npos
            )
        {
            line = "</li></ul>" + line;
            this->isFinished = true;
            return;
        }

        if( isStartOfNewListItem )
        {
            line = "</li><li>" + line;
        }
    }

private:
    bool    isStarted;
    bool    isFinished;
};    // class UnorderedListParser

// -----------------------------------------------------------------------------
}    // namespace maddy
