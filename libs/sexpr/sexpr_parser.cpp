/*
 * Copyright (C) 2016 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sexpr/sexpr_parser.h"
#include "sexpr/sexpr_exception.h"
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <stdlib.h>     /* strtod */

#include <fstream>
#include <streambuf>
#include <wx/file.h>

#include <macros.h>

namespace SEXPR
{
    const std::string PARSER::whitespaceCharacters = " \t\n\r\b\f\v";

    PARSER::PARSER() : m_lineNumber( 1 )
    {
    }

    PARSER::~PARSER()
    {
    }

    std::unique_ptr<SEXPR> PARSER::Parse( const std::string& aString )
    {
        std::string::const_iterator it = aString.begin();
        return parseString( aString, it );
    }

    std::unique_ptr<SEXPR> PARSER::ParseFromFile( const std::string& aFileName )
    {
        std::string str = GetFileContents( aFileName );

        std::string::const_iterator it = str.begin();
        return parseString( str, it );
    }

    std::string PARSER::GetFileContents( const std::string &aFileName )
    {
        std::string str;

        // the filename is not always a UTF7 string, so do not use ifstream
        // that do not work with unicode chars.
        wxString fname( FROM_UTF8( aFileName.c_str() ) );
        wxFile file( fname );
        size_t length = file.Length();

        if( length <= 0 )
        {
            throw PARSE_EXCEPTION( "Error occurred attempting to read in file or empty file" );
        }


        str.resize( length );
        file.Read( &str[0], str.length() );

        return str;
    }

    std::unique_ptr<SEXPR> PARSER::parseString(
            const std::string& aString, std::string::const_iterator& it )
    {
        for( ; it != aString.end(); ++it )
        {
            if( *it == '\n' )
                m_lineNumber++;

            if( whitespaceCharacters.find(*it) != std::string::npos )
                continue;

            if( *it == '(' )
            {
                std::advance( it, 1 );

                auto list = std::make_unique<SEXPR_LIST>( m_lineNumber );

                while( it != aString.end() && *it != ')' )
                {
                    //there may be newlines in between atoms of a list, so detect these here
                    if( *it == '\n' )
                        m_lineNumber++;

                    if( whitespaceCharacters.find(*it) != std::string::npos )
                    {
                        std::advance( it, 1 );
                        continue;
                    }

                    std::unique_ptr<SEXPR> item = parseString( aString, it );
                    list->AddChild( item.release() );
                }

                if( it != aString.end() )
                    std::advance( it, 1 );

                return list;
            }
            else if( *it == ')' )
            {
                return NULL;
            }
            else if( *it == '"' )
            {
                size_t startPos = std::distance(aString.begin(), it) + 1;
                size_t closingPos = startPos > 0 ? startPos - 1 : startPos;

                // find the closing quote character, be sure it is not escaped
                do
                {
                    closingPos = aString.find_first_of( '"', closingPos + 1 );
                }
                while( closingPos != std::string::npos
                        && ( closingPos > 0 && aString[closingPos - 1] == '\\' ) );

                if( closingPos != std::string::npos )
                {
                    auto str = std::make_unique<SEXPR_STRING>(
                            aString.substr( startPos, closingPos - startPos ), m_lineNumber );
                    std::advance( it, closingPos - startPos + 2 );

                    return str;
                }
                else
                {
                    throw PARSE_EXCEPTION("missing closing quote");
                }
            }
            else
            {
                size_t startPos = std::distance( aString.begin(), it );
                size_t closingPos = aString.find_first_of( whitespaceCharacters + "()", startPos );

                std::string tmp = aString.substr( startPos, closingPos - startPos );


                if( closingPos != std::string::npos )
                {
                    if( tmp.find_first_not_of( "0123456789." ) == std::string::npos ||
                        ( tmp.size() > 1 && tmp[0] == '-'
                          && tmp.find_first_not_of( "0123456789.", 1 ) == std::string::npos ) )
                    {
                        std::unique_ptr<SEXPR> res;

                        if( tmp.find( '.' ) != std::string::npos )
                        {
                            res = std::make_unique<SEXPR_DOUBLE>(
                                    strtod( tmp.c_str(), nullptr ), m_lineNumber );
                            //floating point type
                        }
                        else
                        {
                            res = std::make_unique<SEXPR_INTEGER>(
                                    strtoll( tmp.c_str(), nullptr, 0 ), m_lineNumber );
                        }

                        std::advance( it, closingPos - startPos );
                        return res;
                    }
                    else
                    {
                        auto str = std::make_unique<SEXPR_SYMBOL>( tmp, m_lineNumber );
                        std::advance( it, closingPos - startPos );

                        return str;
                    }
                }
                else
                {
                    throw PARSE_EXCEPTION( "format error" );
                }
            }
        }

        return nullptr;
    }
}
