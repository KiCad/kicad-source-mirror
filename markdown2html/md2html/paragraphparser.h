/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <string>

#include "blockparser.h"

// -----------------------------------------------------------------------------

namespace maddy {
// -----------------------------------------------------------------------------

/**
 * ParagraphParser
 *
 * @class
 */
class ParagraphParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} parseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
     */
    ParagraphParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback ),
        isStarted( false ),
        isFinished( false )
    {}

    /**
     * IsStartingLine
     *
     * If the line is not empty, it will be a paragraph. So this block parser has
     * to always run as the last one!
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        return !line.empty();
    }

    /**
     * IsFinished
     *
     * An empty line will end the paragraph.
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
        return false;
    }

    bool isLineParserAllowed() const override
    {
        return true;
    }

    void parseBlock( std::string& line ) override
    {
        if( !this->isStarted )
        {
            line = "<p>" + line + " ";
            this->isStarted = true;
            return;
        }

        if( line.empty() )
        {
            line += "</p>";
            this->isFinished = true;
            return;
        }

        line += " ";
    }

private:
    bool    isStarted;
    bool    isFinished;
};    // class ParagraphParser

// -----------------------------------------------------------------------------
}    // namespace maddy
