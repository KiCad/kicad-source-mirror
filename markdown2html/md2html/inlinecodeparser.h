/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <string>
#include <regex>

#include "lineparser.h"

// -----------------------------------------------------------------------------

namespace maddy {
// -----------------------------------------------------------------------------

/**
 * InlineCodeParser
 *
 * @class
 */
class InlineCodeParser : public LineParser
{
public:
    /**
     * Parse
     *
     * From Markdown: `text `some code``
     *
     * To HTML: `text <code>some code</code>`
     *
     * @method
     * @param {std::string&} line The line to interpret
     * @return {void}
     */
    void Parse( std::string& line ) override
    {
        static std::regex re( "`([^`]*)`" );
        static std::string replacement = "<code>$1</code>";

        line = std::regex_replace( line, re, replacement );
    }
};    // class InlineCodeParser

// -----------------------------------------------------------------------------
}    // namespace maddy
