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
 * StrongParser
 *
 * Has to be used before the `EmphasizedParser`.
 *
 * @class
 */
class StrongParser : public LineParser
{
public:
    /**
     * Parse
     *
     * From Markdown: `text **text** __text__`
     *
     * To HTML: `text <strong>text</strong> <strong>text</strong>`
     *
     * @method
     * @param {std::string&} line The line to interpret
     * @return {void}
     */
    void Parse( std::string& line ) override
    {
        static std::vector<std::regex> res
        {
            std::regex {
                "(?!.*`.*|.*<code>.*)\\*\\*(?!.*`.*|.*<\\/code>.*)([^\\*\\*]*)\\*\\*(?!.*`.*|.*<\\/code>.*)"
            },
            std::regex {
                "(?!.*`.*|.*<code>.*)__(?!.*`.*|.*<\\/code>.*)([^__]*)__(?!.*`.*|.*<\\/code>.*)"
            }
        };
        static std::string replacement = "<strong>$1</strong>";

        for( const auto& re : res )
        {
            line = std::regex_replace( line, re, replacement );
        }
    }
};    // class StrongParser

// -----------------------------------------------------------------------------
}    // namespace maddy
