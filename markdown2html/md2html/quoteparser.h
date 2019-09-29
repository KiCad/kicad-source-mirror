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
 * QuoteParser
 *
 * @class
 */
class QuoteParser : public BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} parseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} getBlockParserForLineCallback
     */
    QuoteParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : BlockParser( aParseLineCallback, aGetBlockParserForLineCallback ),
        isStarted( false ),
        isFinished( false )
    {}

    /**
     * IsStartingLine
     *
     * A quote starts with `> `.
     *
     * @method
     * @param {const std::string&} line
     * @return {bool}
     */
    static bool IsStartingLine( const std::string& line )
    {
        static std::regex re( "^\\>.*" );

        return std::regex_match( line, re );
    }

    /**
     * AddLine
     *
     * Adding a line which has to be parsed.
     *
     * @method
     * @param {std::string&} line
     * @return {void}
     */
    void AddLine( std::string& line ) override
    {
        if( !this->isStarted )
        {
            this->result << "<blockquote>";
            this->isStarted = true;
        }

        bool finish = false;

        if( line.empty() )
        {
            finish = true;
        }

        this->parseBlock( line );

        if( this->isInlineBlockAllowed() && !this->childParser )
        {
            this->childParser = this->getBlockParserForLine( line );
        }

        if( this->childParser )
        {
            this->childParser->AddLine( line );

            if( this->childParser->IsFinished() )
            {
                this->result << this->childParser->GetResult().str();
                this->childParser = nullptr;
            }

            return;
        }

        if( this->isLineParserAllowed() )
        {
            this->parseLine( line );
        }

        if( finish )
        {
            this->result << "</blockquote>";
            this->isFinished = true;
        }

        this->result << line;
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
        static std::regex lineRegexWithSpace( "^\\> " );

        line = std::regex_replace( line, lineRegexWithSpace, "" );
        static std::regex lineRegexWithoutSpace( "^\\>" );
        line = std::regex_replace( line, lineRegexWithoutSpace, "" );

        if( !line.empty() )
        {
            line += " ";
        }
    }

private:
    bool    isStarted;
    bool    isFinished;
};    // class QuoteParser

// -----------------------------------------------------------------------------
}    // namespace maddy
