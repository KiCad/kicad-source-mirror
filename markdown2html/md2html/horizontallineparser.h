/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <string>
#include <regex>

#include "blockparser.h"

// -----------------------------------------------------------------------------

namespace maddy {
// -----------------------------------------------------------------------------

/**
 * HorizontalLineParser
 *
 * From Markdown: `---`
 *
 * To HTML: `<hr/>`
 *
 * @class
 */
class HorizontalLineParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} aParseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} aGetBlockParserForLineCallback
     */
    HorizontalLineParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback ),
        lineRegex( "^---$" )
    {}

    /**
     * IsStartingLine
     *
     * If the line has exact three dashes `---`, then it is a horizontal line.
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        static std::regex re( "^---$" );

        return std::regex_match( line, re );
    }

    /**
     * IsFinished
     *
     * The horizontal line is always only one line long, so this method always
     * returns true.
     *
     * @method
     * @return {bool}
     */
    bool IsFinished() const override
    {
        return true;
    }

protected:
    bool isInlineBlockAllowed() const override
    {
        return false;
    }

    bool isLineParserAllowed() const override
    {
        return false;
    }

    void parseBlock( std::string& line ) override
    {
        static std::string replacement = "<hr/>";

        line = std::regex_replace( line, lineRegex, replacement );
    }

private:
    std::regex lineRegex;
};    // class HorizontalLineParser

// -----------------------------------------------------------------------------
}    // namespace maddy
