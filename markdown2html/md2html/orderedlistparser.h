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
 * OrderedListParser
 *
 * @class
 */
class OrderedListParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} parseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
     */
    OrderedListParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback ),
        isStarted( false ),
        isFinished( false )
    {}

    /**
     * IsStartingLine
     *
     * An ordered list starts with `1. `.
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        static std::regex re( "^1\\. .*" );

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
        bool isStartOfNewListItem = this->isStartOfNewListItem( line );
        uint32_t indentation = getIndentationWidth( line );

        static std::regex orderedlineRegex( "^1\\. " );

        line = std::regex_replace( line, orderedlineRegex, "" );
        static std::regex unorderedlineRegex( "^(\\* )" );
        line = std::regex_replace( line, unorderedlineRegex, "" );

        if( !this->isStarted )
        {
            line = "<ol><li>" + line;
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
            line = "</li></ol>" + line;
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

    bool isStartOfNewListItem( const std::string& line ) const
    {
        static std::regex re( "^(?:1\\. |\\* ).*" );

        return std::regex_match( line, re );
    }
};    // class OrderedListParser

// -----------------------------------------------------------------------------
}    // namespace maddy
