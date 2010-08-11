
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

LINE_READER::LINE_READER( unsigned aMaxLineLength )
{
    lineNum = 0;
    maxLineLength = aMaxLineLength;

    // the real capacity is 10 bytes larger than requested.
    capacity = aMaxLineLength + 10;

    line = new char[capacity];

    line[0] = '\0';
    length  = 0;
}


FILE_LINE_READER::FILE_LINE_READER( FILE* aFile,  unsigned aMaxLineLength ) :
    LINE_READER( aMaxLineLength )
{
    fp = aFile;
}


int FILE_LINE_READER::ReadLine() throw (IOError)
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


int STRING_LINE_READER::ReadLine() throw (IOError)
{
    size_t      nlOffset = source.find( '\n', ndx );
    size_t      advance;

    if( nlOffset == std::string::npos )
        advance = source.length() - ndx;
    else
        advance = nlOffset - ndx + 1;     // include the newline, so +1

    if( advance )
    {
        if( advance > maxLineLength )
            throw IOError( _("Line length exceeded") );

        wxASSERT( ndx + advance <= source.length() );

        memcpy( line, &source[ndx], advance );

        ++lineNum;
        ndx += advance;
    }

    length = advance;
    line[advance] = 0;

    return advance;
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


int OUTPUTFORMATTER::vprint( const char* fmt,  va_list ap )  throw( IOError )
{
    int ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    if( ret >= (int) buffer.size() )
    {
        buffer.reserve( ret+200 );
        ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    }

    if( ret > 0 )
        write( &buffer[0], ret );

    return ret;
}


int OUTPUTFORMATTER::sprint( const char* fmt, ... )  throw( IOError )
{
    va_list     args;

    va_start( args, fmt );
    int ret = vprint( fmt, args);
    va_end( args );

    return ret;
}


int OUTPUTFORMATTER::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{
#define NESTWIDTH           2   ///< how many spaces per nestLevel

    va_list     args;

    va_start( args, fmt );

    int result = 0;
    int total  = 0;

    for( int i=0; i<nestLevel;  ++i )
    {
        // no error checking needed, an exception indicates an error.
        result = sprint( "%*c", NESTWIDTH, ' ' );

        total += result;
    }

    // no error checking needed, an exception indicates an error.
    result = vprint( fmt, args );

    va_end( args );

    total += result;
    return total;
}


const char* OUTPUTFORMATTER::Quoted( std::string* aWrapee ) throw( IOError )
{
    // derived class's notion of what a quote character is
    char quote          = *GetQuoteChar( "(" );

    // Will the string be wrapped based on its interior content?
    const char* squote  = GetQuoteChar( aWrapee->c_str() );

    // Search the interior of the string for 'quote' chars
    // and replace them as found with duplicated quotes.
    // Note that necessarily any string which has internal quotes will
    // also be wrapped in quotes later in this function.
    for( unsigned i=0;  i<aWrapee->size();  ++i )
    {
        if( (*aWrapee)[i] == quote )
        {
            aWrapee->insert( aWrapee->begin()+i, quote );
            ++i;
        }
        else if( (*aWrapee)[0]=='\r' || (*aWrapee)[0]=='\n' )
        {
            // In a desire to maintain accurate line number reporting within DSNLEXER
            // a decision was made to make all S-expression strings be on a single
            // line.  You can embedd \n (human readable) in the text but not
            // '\n' which is 0x0a.
            throw IOError( _( "S-expression string has newline" ) );
        }
    }

    if( *squote )
    {
        // wrap the beginning and end of the string in a quote.
        aWrapee->insert( aWrapee->begin(), quote );
        aWrapee->insert( aWrapee->end(), quote );
    }

    return aWrapee->c_str();
}


//-----<STRING_FORMATTER>----------------------------------------------------

void STRING_FORMATTER::write( const char* aOutBuf, int aCount ) throw( IOError )
{
    mystring.append( aOutBuf, aCount );
}

void STRING_FORMATTER::StripUseless()
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


//-----<STREAM_OUTPUTFORMATTER>--------------------------------------

const char* STREAM_OUTPUTFORMATTER::GetQuoteChar( const char* wrapee )
{
    return OUTPUTFORMATTER::GetQuoteChar( wrapee, quoteChar );
}


void STREAM_OUTPUTFORMATTER::write( const char* aOutBuf, int aCount ) throw( IOError )
{
    int lastWrite;

    // This might delay awhile if you were writing to say a socket, but for
    // a file it should only go through the loop once.
    for( int total = 0;  total<aCount;  total += lastWrite )
    {
        lastWrite = os.Write( aOutBuf, aCount ).LastWrite();

        if( !os.IsOk() )
        {
            throw IOError( _( "OUTPUTSTREAM_OUTPUTFORMATTER write error" ) );
        }
    }
}

