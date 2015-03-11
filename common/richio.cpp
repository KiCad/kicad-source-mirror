
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#include <richio.h>


// Fall back to getc() when getc_unlocked() is not available on the target platform.
#if !defined( HAVE_FGETC_NOLOCK )
#define getc_unlocked getc
#endif


// This file defines 3 classes and some functions useful for working with text files
// and is named "richio" after its author, Richard Hollenbeck, aka Dick Hollenbeck.



static int vprint( std::string* result, const char* format, va_list ap )
{
    char    msg[512];
    size_t  len = vsnprintf( msg, sizeof(msg), format, ap );

    if( len < sizeof(msg) )     // the output fit into msg
    {
        result->append( msg, msg + len );
    }
    else
    {
        // output was too big, so now incur the expense of allocating
        // a buf for holding suffient characters.

        std::vector<char>   buf;

        buf.reserve( len+1 );   // reserve(), not resize() which writes. +1 for trailing nul.

        len = vsnprintf( &buf[0], len+1, format, ap );

        result->append( &buf[0], &buf[0] + len );
    }

    return len;
}


int StrPrintf( std::string* result, const char* format, ... )
{
    va_list     args;

    va_start( args, format );
    int ret = vprint( result, format, args );
    va_end( args );

    return ret;
}


std::string StrPrintf( const char* format, ... )
{
    std::string ret;
    va_list     args;

    va_start( args, format );
    int ignore = vprint( &ret, format, args );
    (void) ignore;
    va_end( args );

    return ret;
}


void IO_ERROR::init( const char* aThrowersFile, const char* aThrowersLoc, const wxString& aMsg )
{
    errorText.Printf( IO_FORMAT, aMsg.GetData(),
        wxString::FromUTF8( aThrowersFile ).GetData(),
        wxString::FromUTF8( aThrowersLoc ).GetData() );
}


void PARSE_ERROR::init( const char* aThrowersFile, const char* aThrowersLoc,
           const wxString& aMsg, const wxString& aSource,
           const char* aInputLine,
           int aLineNumber, int aByteIndex )
{
    // save inpuLine, lineNumber, and offset for UI (.e.g. Sweet text editor)
    inputLine  = aInputLine;
    lineNumber = aLineNumber;
    byteIndex  = aByteIndex;

    errorText.Printf( PARSE_FORMAT, aMsg.GetData(), aSource.GetData(),
        aLineNumber, aByteIndex,
        wxString::FromUTF8( aThrowersFile ).GetData(),
        wxString::FromUTF8( aThrowersLoc ).GetData() );
}


//-----<LINE_READER>------------------------------------------------------

LINE_READER::LINE_READER( unsigned aMaxLineLength ) :
    length( 0 ),
    lineNum( 0 ),
    line( NULL ),
    capacity( 0 ),
    maxLineLength( aMaxLineLength )
{
    if( aMaxLineLength != 0 )
    {
        // start at the INITIAL size, expand as needed up to the MAX size in maxLineLength
        capacity = LINE_READER_LINE_INITIAL_SIZE;

        // but never go above user's aMaxLineLength, and leave space for trailing nul
        if( capacity > aMaxLineLength+1 )
            capacity = aMaxLineLength+1;

        line = new char[capacity];

        line[0] = '\0';
    }
}


LINE_READER::~LINE_READER()
{
    delete[] line;
}


void LINE_READER::expandCapacity( unsigned newsize )
{
    // length can equal maxLineLength and nothing breaks, there's room for
    // the terminating nul. cannot go over this.
    if( newsize > maxLineLength+1 )
        newsize = maxLineLength+1;

    if( newsize > capacity )
    {
        capacity = newsize;

        // resize the buffer, and copy the original data
        char* bigger = new char[capacity];

        wxASSERT( capacity >= length+1 );

        memcpy( bigger, line, length );
        bigger[length] = 0;

        delete[] line;
        line = bigger;
    }
}


FILE_LINE_READER::FILE_LINE_READER( const wxString& aFileName,
            unsigned aStartingLineNumber,
            unsigned aMaxLineLength ) throw( IO_ERROR ) :
    LINE_READER( aMaxLineLength ),
    iOwn( true )
{
    fp = wxFopen( aFileName, wxT( "rt" ) );
    if( !fp )
    {
        wxString msg = wxString::Format(
            _( "Unable to open filename '%s' for reading" ), aFileName.GetData() );
        THROW_IO_ERROR( msg );
    }

    setvbuf( fp, NULL, _IOFBF, BUFSIZ * 8 );

    source  = aFileName;
    lineNum = aStartingLineNumber;
}


FILE_LINE_READER::FILE_LINE_READER( FILE* aFile, const wxString& aFileName,
                    bool doOwn,
                    unsigned aStartingLineNumber,
                    unsigned aMaxLineLength ) :
    LINE_READER( aMaxLineLength ),
    iOwn( doOwn ),
    fp( aFile )
{
    if( doOwn && ftell( aFile ) == 0L )
    {
#ifndef __WXMAC__
        setvbuf( fp, NULL, _IOFBF, BUFSIZ * 8 );
#endif
    }
    source  = aFileName;
    lineNum = aStartingLineNumber;
}


FILE_LINE_READER::~FILE_LINE_READER()
{
    if( iOwn && fp )
        fclose( fp );
}


char* FILE_LINE_READER::ReadLine() throw( IO_ERROR )
{
    length = 0;

    for(;;)
    {
        if( length >= maxLineLength )
            THROW_IO_ERROR( _( "Maximum line length exceeded" ) );

        if( length >= capacity )
            expandCapacity( capacity * 2 );

        // faster, POSIX compatible fgetc(), no locking.
        int cc = getc_unlocked( fp );
        if( cc == EOF )
            break;

        line[ length++ ] = (char) cc;

        if( cc == '\n' )
            break;
    }

    line[ length ] = 0;

    // lineNum is incremented even if there was no line read, because this
    // leads to better error reporting when we hit an end of file.
    ++lineNum;

    return length ? line : NULL;
}


STRING_LINE_READER::STRING_LINE_READER( const std::string& aString, const wxString& aSource ) :
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    lines( aString ),
    ndx( 0 )
{
    // Clipboard text should be nice and _use multiple lines_ so that
    // we can report _line number_ oriented error messages when parsing.
    source = aSource;
}


STRING_LINE_READER::STRING_LINE_READER( const STRING_LINE_READER& aStartingPoint ) :
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    lines( aStartingPoint.lines ),
    ndx( aStartingPoint.ndx )
{
    // since we are keeping the same "source" name, for error reporting purposes
    // we need to have the same notion of line number and offset.

    source  = aStartingPoint.source;
    lineNum = aStartingPoint.lineNum;
}


char* STRING_LINE_READER::ReadLine() throw( IO_ERROR )
{
    size_t  nlOffset = lines.find( '\n', ndx );

    if( nlOffset == std::string::npos )
        length = lines.length() - ndx;
    else
        length = nlOffset - ndx + 1;     // include the newline, so +1

    if( length )
    {
        if( length >= maxLineLength )
            THROW_IO_ERROR( _("Line length exceeded") );

        if( length+1 > capacity )   // +1 for terminating nul
            expandCapacity( length+1 );

        wxASSERT( ndx + length <= lines.length() );

        memcpy( line, &lines[ndx], length );

        ndx += length;
    }

    ++lineNum;      // this gets incremented even if no bytes were read

    line[length] = 0;

    return length ? line : NULL;
}


INPUTSTREAM_LINE_READER::INPUTSTREAM_LINE_READER( wxInputStream* aStream, const wxString& aSource ) :
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    m_stream( aStream )
{
    source = aSource;
}


char* INPUTSTREAM_LINE_READER::ReadLine() throw( IO_ERROR )
{
    length  = 0;

    for(;;)
    {
        if( length >= maxLineLength )
            THROW_IO_ERROR( _( "Maximum line length exceeded" ) );

        if( length + 1 > capacity )
            expandCapacity( capacity * 2 );

        // this read may fail, docs say to test LastRead() before trusting cc.
        char cc = m_stream->GetC();

        if( !m_stream->LastRead() )
            break;

        line[ length++ ] = cc;

        if( cc == '\n' )
            break;
    }

    line[ length ] = 0;

    // lineNum is incremented even if there was no line read, because this
    // leads to better error reporting when we hit an end of file.
    ++lineNum;

    return length ? line : NULL;
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

    if( strlen( wrapee ) == 0 )
        return quote_char;

    bool isFirst = true;

    for(  ; *wrapee;  ++wrapee, isFirst = false )
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


const char* OUTPUTFORMATTER::GetQuoteChar( const char* wrapee )
{
    return GetQuoteChar( wrapee, quoteChar );
}

int OUTPUTFORMATTER::vprint( const char* fmt,  va_list ap )  throw( IO_ERROR )
{
    int ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    if( ret >= (int) buffer.size() )
    {
        buffer.resize( ret + 2000 );
        ret = vsnprintf( &buffer[0], buffer.size(), fmt, ap );
    }

    if( ret > 0 )
        write( &buffer[0], ret );

    return ret;
}


int OUTPUTFORMATTER::sprint( const char* fmt, ... )  throw( IO_ERROR )
{
    va_list     args;

    va_start( args, fmt );
    int ret = vprint( fmt, args);
    va_end( args );

    return ret;
}


int OUTPUTFORMATTER::Print( int nestLevel, const char* fmt, ... ) throw( IO_ERROR )
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


std::string OUTPUTFORMATTER::Quotes( const std::string& aWrapee ) throw( IO_ERROR )
{
    static const char quoteThese[] = "\t ()\n\r";

    if( !aWrapee.size() ||  // quote null string as ""
        aWrapee[0]=='#' ||  // quote a potential s-expression comment, so it is not a comment
        aWrapee[0]=='"' ||  // NextTok() will travel through DSN_STRING path anyway, then must apply escapes
        aWrapee.find_first_of( quoteThese ) != std::string::npos )
    {
        std::string ret;

        ret.reserve( aWrapee.size()*2 + 2 );

        ret += '"';

        for( std::string::const_iterator it = aWrapee.begin(); it!=aWrapee.end(); ++it )
        {
            switch( *it )
            {
            case '\n':
                ret += '\\';
                ret += 'n';
                break;
            case '\r':
                ret += '\\';
                ret += 'r';
                break;
            case '\\':
                ret += '\\';
                ret += '\\';
                break;
            case '"':
                ret += '\\';
                ret += '"';
                break;
            default:
                ret += *it;
            }
        }

        ret += '"';

        return ret;
    }

    return aWrapee;
}


std::string OUTPUTFORMATTER::Quotew( const wxString& aWrapee ) throw( IO_ERROR )
{
    // wxStrings are always encoded as UTF-8 as we convert to a byte sequence.
    // The non-virutal function calls the virtual workhorse function, and if
    // a different quoting or escaping strategy is desired from the standard,
    // a derived class can overload Quotes() above, but
    // should never be a reason to overload this Quotew() here.
    return Quotes( (const char*) aWrapee.utf8_str() );
}


//-----<STRING_FORMATTER>----------------------------------------------------

void STRING_FORMATTER::write( const char* aOutBuf, int aCount ) throw( IO_ERROR )
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

//-----<FILE_OUTPUTFORMATTER>----------------------------------------

FILE_OUTPUTFORMATTER::FILE_OUTPUTFORMATTER( const wxString& aFileName,
        const wxChar* aMode,  char aQuoteChar ) throw( IO_ERROR ) :
    OUTPUTFORMATTER( OUTPUTFMTBUFZ, aQuoteChar ),
    m_filename( aFileName )
{
    m_fp = wxFopen( aFileName, aMode );

    if( !m_fp )
    {
        wxString msg = wxString::Format(
                            _( "cannot open or save file '%s'" ),
                            m_filename.GetData() );
        THROW_IO_ERROR( msg );
    }
}


FILE_OUTPUTFORMATTER::~FILE_OUTPUTFORMATTER()
{
    if( m_fp )
        fclose( m_fp );
}


void FILE_OUTPUTFORMATTER::write( const char* aOutBuf, int aCount ) throw( IO_ERROR )
{
    if( 1 != fwrite( aOutBuf, aCount, 1, m_fp ) )
    {
        wxString msg = wxString::Format(
                            _( "error writing to file '%s'" ),
                            m_filename.GetData() );
        THROW_IO_ERROR( msg );
    }
}


//-----<STREAM_OUTPUTFORMATTER>--------------------------------------

void STREAM_OUTPUTFORMATTER::write( const char* aOutBuf, int aCount ) throw( IO_ERROR )
{
    int lastWrite;

    // This might delay awhile if you were writing to say a socket, but for
    // a file it should only go through the loop once.
    for( int total = 0;  total<aCount;  total += lastWrite )
    {
        lastWrite = os.Write( aOutBuf, aCount ).LastWrite();

        if( !os.IsOk() )
        {
            THROW_IO_ERROR( _( "OUTPUTSTREAM_OUTPUTFORMATTER write error" ) );
        }
    }
}

