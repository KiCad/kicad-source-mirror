
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <cstdarg>

#include "richio.h"

// This file defines 3 classes useful for working with DSN text files and is named
// "richio" after its author, Richard Hollenbeck, aka Dick Hollenbeck.


//-----<LINE_READER>------------------------------------------------------

LINE_READER::LINE_READER( FILE* aFile,  unsigned aMaxLineLength )
{
    fp = aFile;
    lineNum = 0;
    maxLineLength = aMaxLineLength;

    // the real capacity is 10 bytes larger than requested.
    capacity = aMaxLineLength + 10;

    line = new char[capacity];

    line[0] = '\0';
    length  = 0;
}


int LINE_READER::ReadLine() throw (IOError)
{
    const char* p = fgets( line, capacity, fp );

    if( !p )
    {
        line[0] = 0;
        length  = 0;
    }
    else
    {
        length = strlen( line );

        if( length > maxLineLength )
            throw IOError( _("Line length exceeded") );

        ++lineNum;
    }

    return length;
}



//-----<OUTPUTFORMATTER>----------------------------------------------------

// factor out a common GetQuoteChar

const char* OUTPUTFORMATTER::GetQuoteChar( const char* wrapee, const char* quote_char )
{
    // Include '#' so a symbol is not confused with a comment.  We intend
    // to wrap any symbol starting with a '#'.
    // Our LEXER class handles comments, and comments appear to be an extension
    // to the SPECCTRA DSN specification.
    if( *wrapee == '#' )
        return quote_char;

    if( strlen(wrapee)==0 )
        return quote_char;

    bool isFirst = true;

    for(  ; *wrapee;  ++wrapee, isFirst=false )
    {
        static const char quoteThese[] = "\t ()"
            "%"     // per Alfons of freerouting.net, he does not like this unquoted as of 1-Feb-2008
            "{}"    // guessing that these are problems too
            ;

        // if the string to be wrapped (wrapee) has a delimiter in it,
        // return the quote_char so caller wraps the wrapee.
        if( strchr( quoteThese, *wrapee ) )
            return quote_char;

        if( !isFirst  &&  '-' == *wrapee )
            return quote_char;
    }

    return "";  // caller does not need to wrap, can use an unwrapped string.
}


//-----<STRINGFORMATTER>----------------------------------------------------

const char* STRINGFORMATTER::GetQuoteChar( const char* wrapee )
{
    // for what we are using STRINGFORMATTER for at this time, we can return
    // the nul string always.

    return "";
//    return OUTPUTFORMATTER::GetQuoteChar( const char* wrapee, "\"" );
}

int STRINGFORMATTER::vprint( const char* fmt,  va_list ap )
{
    int ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    if( ret >= (int) buffer.size() )
    {
        buffer.reserve( ret+200 );
        ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    }

    if( ret > 0 )
        mystring.append( (const char*) &buffer[0] );

    return ret;
}


int STRINGFORMATTER::sprint( const char* fmt, ... )
{
    va_list     args;

    va_start( args, fmt );
    int ret = vprint( fmt, args);
    va_end( args );

    return ret;
}


int STRINGFORMATTER::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{

#define NESTWIDTH           2   ///< how many spaces per nestLevel

    va_list     args;

    va_start( args, fmt );

    int result = 0;
    int total  = 0;

    for( int i=0; i<nestLevel;  ++i )
    {
        result = sprint( "%*c", NESTWIDTH, ' ' );
        if( result < 0 )
            break;

        total += result;
    }

    if( result<0 || (result=vprint( fmt, args ))<0 )
    {
        throw IOError( _("Error writing to STRINGFORMATTER") );
    }

    va_end( args );

    total += result;
    return total;
}


void STRINGFORMATTER::StripUseless()
{
    std::string  copy = mystring;

    mystring.clear();

    for( std::string::iterator i=copy.begin();  i!=copy.end();  ++i )
    {
        if( !isspace( *i ) && *i!=')' && *i!='(' && *i!='"' )
        {
            mystring += *i;
        }
    }
}

