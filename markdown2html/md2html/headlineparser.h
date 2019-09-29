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
 * HeadlineParser
 *
 * From Markdown:
 *
 * ```
 * # Headline 1
 * ## Headline 2
 * ### Headline 3
 * #### Headline 4
 * ##### Headline 5
 * ###### Headline 6
 * ```
 *
 * To HTML:
 *
 * ```
 * <h1>Headline 1</h1>
 * <h2>Headline 2</h2>
 * <h3>Headline 3</h3>
 * <h4>Headline 4</h4>
 * <h5>Headline 5</h5>
 * <h6>Headline 6</h6>
 * ```
 *
 * @class
 */
class HeadlineParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} aParseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} aGetBlockParserForLineCallback
     */
    HeadlineParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback )
    {}

    /**
     * IsStartingLine
     *
     * If the line starts with 1 - 6 `#`, then it is a headline.
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        static std::regex re( "^(?:#){1,6} (.*)" );

        return std::regex_match( line, re );
    }

    /**
     * IsFinished
     *
     * The headline is always only one line long, so this method always returns
     * true.
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
        static std::vector<std::regex> hlRegex =
        {
            std::regex( "^# (.*)" ),
            std::regex( "^(?:#){2} (.*)" ),
            std::regex( "^(?:#){3} (.*)" ),
            std::regex( "^(?:#){4} (.*)" ),
            std::regex( "^(?:#){5} (.*)" ),
            std::regex( "^(?:#){6} (.*)" )
        };
        static std::vector<std::string> hlReplacement =
        {
            "<h1>$1</h1>",
            "<h2>$1</h2>",
            "<h3>$1</h3>",
            "<h4>$1</h4>",
            "<h5>$1</h5>",
            "<h6>$1</h6>"
        };

        for( uint8_t i = 0; i < 6; ++i )
        {
            line = std::regex_replace( line, hlRegex[i], hlReplacement[i] );
        }
    }
};    // class HeadlineParser

// -----------------------------------------------------------------------------
}    // namespace maddy
