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
 * EmphasizedParser
 *
 * Has to be used after the `StrongParser`.
 *
 * @class
 */
class EmphasizedParser : public LineParser
{
public:
    /**
     * Parse
     *
     * From Markdown: `text _text_`
     *
     * To HTML: `text <em>text</em>`
     *
     * @method
     * @param {std::string&} line The line to interpret
     * @return {void}
     */
    void Parse( std::string& line ) override
    {
        static std::regex re(
                "(?!.*`.*|.*<code>.*)_(?!.*`.*|.*<\\/code>.*)([^_]*)_(?!.*`.*|.*<\\/code>.*)" );
        static std::string replacement = "<em>$1</em>";

        line = std::regex_replace( line, re, replacement );
    }
};    // class EmphasizedParser

// -----------------------------------------------------------------------------
}    // namespace maddy
