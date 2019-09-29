/*
 * This project is licensed under the MIT license. For more information see the
 * LICENSE file.
 */
#pragma once

// -----------------------------------------------------------------------------

#include <functional>
#include <sstream>
#include <string>
// windows compatibility includes
#include <cctype>
#include <algorithm>

// -----------------------------------------------------------------------------

namespace maddy {
// -----------------------------------------------------------------------------

/**
 * BlockParser
 *
 * The code expects every child to have the following static function to be
 * implemented:
 * `static bool IsStartingLine(const std::string& line)`
 *
 * @class
 */
class BlockParser
{
public:
    /**
     * ctor
     *
     * @method
     * @param {std::function<void(std::string&)>} aParseLineCallback
     * @param {std::function<std::shared_ptr<BlockParser>(const std::string& line)>} aGetBlockParserForLineCallback
     */
    BlockParser( std::function<void(std::string&)> aParseLineCallback,
            std::function<std::shared_ptr<BlockParser>(const std::string& line)> aGetBlockParserForLineCallback
            )
        : result( "", std::ios_base::ate | std::ios_base::in | std::ios_base::out ),
        childParser( nullptr ),
        parseLineCallback( aParseLineCallback ),
        getBlockParserForLineCallback( aGetBlockParserForLineCallback )
    {}

    /**
     * dtor
     *
     * @method
     */
    virtual ~BlockParser() {}

    /**
     * AddLine
     *
     * Adding a line which has to be parsed.
     *
     * @method
     * @param {std::string&} line
     * @return {void}
     */
    virtual void AddLine( std::string& line )
    {
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

        this->result << line;
    }

    /**
     * IsFinished
     *
     * Check if the BlockParser is done
     *
     * @method
     * @return {bool}
     */
    virtual bool IsFinished() const = 0;

    /**
     * GetResult
     *
     * Get the parsed HTML output.
     *
     * @method
     * @return {std::stringstream}
     */
    std::stringstream& GetResult()
    {
        return this->result;
    }

    /**
     * Clear
     *
     * Clear the result to reuse the parser object.
     *
     * It is only used by one test for now.
     *
     * @method
     * @return {void}
     */
    void Clear()
    {
        this->result.str( "" );
    }

protected:
    std::stringstream result;
    std::shared_ptr<BlockParser> childParser;

    virtual bool    isInlineBlockAllowed() const = 0;
    virtual bool    isLineParserAllowed() const = 0;
    virtual void    parseBlock( std::string& line ) = 0;

    void parseLine( std::string& line )
    {
        if( parseLineCallback )
        {
            parseLineCallback( line );
        }
    }

    uint32_t getIndentationWidth( const std::string& line ) const
    {
        bool hasMetNonSpace = false;

        uint32_t indentation = static_cast<uint32_t>(
            std::count_if(
                    line.begin(),
                    line.end(),
                    [&hasMetNonSpace]( unsigned char c )
            {
                if( hasMetNonSpace )
                {
                    return false;
                }

                if( std::isspace( c ) )
                {
                    return true;
                }

                hasMetNonSpace = true;
                return false;
            }
                    )
            );

        return indentation;
    }

    std::shared_ptr<BlockParser> getBlockParserForLine( const std::string& line )
    {
        if( getBlockParserForLineCallback )
        {
            return getBlockParserForLineCallback( line );
        }

        return nullptr;
    }

private:
    std::function<void(std::string&)> parseLineCallback;
    std::function<std::shared_ptr<BlockParser>(const std::string& line)>
    getBlockParserForLineCallback;
};    // class BlockParser

// -----------------------------------------------------------------------------
}    // namespace maddy
