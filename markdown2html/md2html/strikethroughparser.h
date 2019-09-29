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
 * StrikeThroughParser
 *
 * @class
 */
class StrikeThroughParser : public LineParser
{
public:
    /**
     * Parse
     *
     * From Markdown: `text ~~text~~`
     *
     * To HTML: `text <s>text</s>`
     *
     * @method
     * @param {std::string&} line The line to interpret
     * @return {void}
     */
    void Parse( std::string& line ) override
    {
        static std::regex re(
                "(?!.*`.*|.*<code>.*)\\~\\~(?!.*`.*|.*<\\/code>.*)([^\\~]*)\\~\\~(?!.*`.*|.*<\\/code>.*)" );
        static std::string replacement = "<s>$1</s>";

        line = std::regex_replace( line, re, replacement );
    }
};    // class StrikeThroughParser

// -----------------------------------------------------------------------------
}    // namespace maddy
