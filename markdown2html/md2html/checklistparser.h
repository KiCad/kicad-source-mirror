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
 * ChecklistParser
 *
 * @class
 */
class ChecklistParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} aParseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} aGetBlockParserForLineCallback
     */
    ChecklistParser( std::function<void(std::string&)> aParseLineCallback,
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
        static std::regex re( "^- \\[[x| ]\\] .*" );

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

        static std::regex lineRegex( "^(- )" );

        line = std::regex_replace( line, lineRegex, "" );

        static std::regex emptyBoxRegex( "^\\[ \\]" );
        static std::string emptyBoxReplacement = "<input type=\"checkbox\"/>";
        line = std::regex_replace( line, emptyBoxRegex, emptyBoxReplacement );

        static std::regex boxRegex( "^\\[x\\]" );
        static std::string boxReplacement = "<input type=\"checkbox\" checked=\"checked\"/>";
        line = std::regex_replace( line, boxRegex, boxReplacement );

        if( !this->isStarted )
        {
            line = "<ul class=\"checklist\"><li><label>" + line;
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
            || line.find( "</label></li><li><label>" ) != std::string::npos
            || line.find( "</label></li></ul>" ) != std::string::npos
            )
        {
            line = "</label></li></ul>" + line;
            this->isFinished = true;
            return;
        }

        if( isStartOfNewListItem )
        {
            line = "</label></li><li><label>" + line;
        }
    }

private:
    bool    isStarted;
    bool    isFinished;
};    // class ChecklistParser

// -----------------------------------------------------------------------------
}    // namespace maddy
