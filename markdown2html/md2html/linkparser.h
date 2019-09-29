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
 * LinkParser
 *
 * Has to be used after the `ImageParser`.
 *
 * @class
 */
class LinkParser : public LineParser
{
public:
    /**
     * Parse
     *
     * From Markdown: `[text](http://example.com)`
     *
     * To HTML: `<a href="http://example.com">text</a>`
     *
     * @method
     * @param {std::string&} line The line to interpret
     * @return {void}
     */
    void Parse( std::string& line ) override
    {
        static std::regex re( "\\[([^\\]]*)\\]\\(([^\\]]*)\\)" );
        static std::string replacement = "<a href=\"$2\">$1</a>";

        line = std::regex_replace( line, re, replacement );
    }
};    // class LinkParser

// -----------------------------------------------------------------------------
}    // namespace maddy
