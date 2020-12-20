/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <config.h> // HAVE_FGETC_NOLOCK

#include <richio.h>
#include <errno.h>

#include <wx/file.h>
#include <wx/translation.h>


// Fall back to getc() when getc_unlocked() is not available on the target platform.
#if !defined( HAVE_FGETC_NOLOCK )
#define getc_unlocked getc
#endif


static int vprint( std::string* result, const char* format, va_list ap )
{
    char    msg[512];
    // This function can call vsnprintf twice.
    // But internally, vsnprintf retrieves arguments from the va_list identified by arg as if
    // va_arg was used on it, and thus the state of the va_list is likely to be altered by the call.
    // see: www.cplusplus.com/reference/cstdio/vsnprintf
    // we make a copy of va_list ap for the second call, if happens
    va_list tmp;
    va_copy( tmp, ap );

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

        len = vsnprintf( &buf[0], len+1, format, tmp );

        result->append( &buf[0], &buf[0] + len );
    }

    va_end( tmp );      // Release the temporary va_list, initialised from ap

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


//-----<LINE_READER>------------------------------------------------------

LINE_READER::LINE_READER( unsigned aMaxLineLength ) :
                m_length( 0 ), m_lineNum( 0 ), m_line( NULL ),
                m_capacity( 0 ), m_maxLineLength( aMaxLineLength )
{
    if( aMaxLineLength != 0 )
    {
        // start at the INITIAL size, expand as needed up to the MAX size in maxLineLength
        m_capacity = LINE_READER_LINE_INITIAL_SIZE;

        // but never go above user's aMaxLineLength, and leave space for trailing nul
        if( m_capacity > aMaxLineLength+1 )
            m_capacity = aMaxLineLength+1;

        // Be sure there is room for a null EOL char, so reserve at least capacity+1 bytes
        // to ensure capacity line lenght and avoid corner cases
        // Use capacity+5 to cover and corner case
        m_line = new char[m_capacity+5];

        m_line[0] = '\0';
    }
}


LINE_READER::~LINE_READER()
{
    delete[] m_line;
}


void LINE_READER::expandCapacity( unsigned aNewsize )
{
    // m_length can equal maxLineLength and nothing breaks, there's room for
    // the terminating nul. cannot go over this.
    if( aNewsize > m_maxLineLength+1 )
        aNewsize = m_maxLineLength+1;

    if( aNewsize > m_capacity )
    {
        m_capacity = aNewsize;

        // resize the buffer, and copy the original data
        // Be sure there is room for the null EOL char, so reserve capacity+1 bytes
        // to ensure capacity line lenght. Use capacity+5 to cover and corner case
        char* bigger = new char[m_capacity+5];

        wxASSERT( m_capacity >= m_length+1 );

        memcpy( bigger, m_line, m_length );
        bigger[m_length] = 0;

        delete[] m_line;
        m_line = bigger;
    }
}


FILE_LINE_READER::FILE_LINE_READER( const wxString& aFileName,
            unsigned aStartingLineNumber, unsigned aMaxLineLength ):
    LINE_READER( aMaxLineLength ), m_iOwn( true )
{
    m_fp = wxFopen( aFileName, wxT( "rt" ) );

    if( !m_fp )
    {
        wxString msg = wxString::Format(
            _( "Unable to open filename \"%s\" for reading" ), aFileName.GetData() );
        THROW_IO_ERROR( msg );
    }

    m_source  = aFileName;
    m_lineNum = aStartingLineNumber;
}


FILE_LINE_READER::FILE_LINE_READER( FILE* aFile, const wxString& aFileName,
                    bool doOwn,
                    unsigned aStartingLineNumber,
                    unsigned aMaxLineLength ) :
    LINE_READER( aMaxLineLength ), m_iOwn( doOwn ), m_fp( aFile )
{
    m_source  = aFileName;
    m_lineNum = aStartingLineNumber;
}


FILE_LINE_READER::~FILE_LINE_READER()
{
    if( m_iOwn && m_fp )
        fclose( m_fp );
}


char* FILE_LINE_READER::ReadLine()
{
    m_length = 0;

    for(;;)
    {
        if( m_length >= m_maxLineLength )
            THROW_IO_ERROR( _( "Maximum line length exceeded" ) );

        if( m_length >= m_capacity )
            expandCapacity( m_capacity * 2 );

        // faster, POSIX compatible fgetc(), no locking.
        int cc = getc_unlocked( m_fp );

        if( cc == EOF )
            break;

        m_line[ m_length++ ] = (char) cc;

        if( cc == '\n' )
            break;
    }

    m_line[ m_length ] = 0;

    // m_lineNum is incremented even if there was no line read, because this
    // leads to better error reporting when we hit an end of file.
    ++m_lineNum;

    return m_length ? m_line : NULL;
}


STRING_LINE_READER::STRING_LINE_READER( const std::string& aString, const wxString& aSource ):
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    m_lines( aString ), m_ndx( 0 )
{
    // Clipboard text should be nice and _use multiple lines_ so that
    // we can report _line number_ oriented error messages when parsing.
    m_source = aSource;
}


STRING_LINE_READER::STRING_LINE_READER( const STRING_LINE_READER& aStartingPoint ):
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    m_lines( aStartingPoint.m_lines ),
    m_ndx( aStartingPoint.m_ndx )
{
    // since we are keeping the same "source" name, for error reporting purposes
    // we need to have the same notion of line number and offset.

    m_source  = aStartingPoint.m_source;
    m_lineNum = aStartingPoint.m_lineNum;
}


char* STRING_LINE_READER::ReadLine()
{
    size_t  nlOffset = m_lines.find( '\n', m_ndx );

    if( nlOffset == std::string::npos )
        m_length = m_lines.length() - m_ndx;
    else
        m_length = nlOffset - m_ndx + 1;     // include the newline, so +1

    if( m_length )
    {
        if( m_length >= m_maxLineLength )
            THROW_IO_ERROR( _("Line length exceeded") );

        if( m_length+1 > m_capacity )   // +1 for terminating nul
            expandCapacity( m_length+1 );

        wxASSERT( m_ndx + m_length <= m_lines.length() );

        memcpy( m_line, &m_lines[m_ndx], m_length );
        m_ndx += m_length;
    }

    ++m_lineNum;      // this gets incremented even if no bytes were read
    m_line[m_length] = 0;

    return m_length ? m_line : NULL;
}


INPUTSTREAM_LINE_READER::INPUTSTREAM_LINE_READER( wxInputStream* aStream, const wxString& aSource ) :
    LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
    m_stream( aStream )
{
    m_source = aSource;
}


char* INPUTSTREAM_LINE_READER::ReadLine()
{
    m_length  = 0;

    for(;;)
    {
        if( m_length >= m_maxLineLength )
            THROW_IO_ERROR( _( "Maximum line length exceeded" ) );

        if( m_length + 1 > m_capacity )
            expandCapacity( m_capacity * 2 );

        // this read may fail, docs say to test LastRead() before trusting cc.
        char cc = m_stream->GetC();

        if( !m_stream->LastRead() )
            break;

        m_line[ m_length++ ] = cc;

        if( cc == '\n' )
            break;
    }

    m_line[ m_length ] = 0;

    // m_lineNum is incremented even if there was no line read, because this
    // leads to better error reporting when we hit an end of file.
    ++m_lineNum;

    return m_length ? m_line : NULL;
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


const char* OUTPUTFORMATTER::GetQuoteChar( const char* wrapee ) const
{
    return GetQuoteChar( wrapee, quoteChar );
}

int OUTPUTFORMATTER::vprint( const char* fmt, va_list ap )
{
    // This function can call vsnprintf twice.
    // But internally, vsnprintf retrieves arguments from the va_list identified by arg as if
    // va_arg was used on it, and thus the state of the va_list is likely to be altered by the call.
    // see: www.cplusplus.com/reference/cstdio/vsnprintf
    // we make a copy of va_list ap for the second call, if happens
    va_list tmp;
    va_copy( tmp, ap );
    int ret = vsnprintf( &m_buffer[0], m_buffer.size(), fmt, ap );

    if( ret >= (int) m_buffer.size() )
    {
        m_buffer.resize( ret + 1000 );
        ret = vsnprintf( &m_buffer[0], m_buffer.size(), fmt, tmp );
    }

    va_end( tmp );      // Release the temporary va_list, initialised from ap

    if( ret > 0 )
        write( &m_buffer[0], ret );

    return ret;
}


int OUTPUTFORMATTER::sprint( const char* fmt, ... )
{
    va_list     args;

    va_start( args, fmt );
    int ret = vprint( fmt, args);
    va_end( args );

    return ret;
}


int OUTPUTFORMATTER::Print( int nestLevel, const char* fmt, ... )
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


std::string OUTPUTFORMATTER::Quotes( const std::string& aWrapee ) const
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


std::string OUTPUTFORMATTER::Quotew( const wxString& aWrapee ) const
{
    // wxStrings are always encoded as UTF-8 as we convert to a byte sequence.
    // The non-virutal function calls the virtual workhorse function, and if
    // a different quoting or escaping strategy is desired from the standard,
    // a derived class can overload Quotes() above, but
    // should never be a reason to overload this Quotew() here.
    return Quotes( (const char*) aWrapee.utf8_str() );
}


//-----<STRING_FORMATTER>----------------------------------------------------

void STRING_FORMATTER::write( const char* aOutBuf, int aCount )
{
    m_mystring.append( aOutBuf, aCount );
}

void STRING_FORMATTER::StripUseless()
{
    std::string  copy = m_mystring;

    m_mystring.clear();

    for( std::string::iterator i=copy.begin();  i!=copy.end();  ++i )
    {
        if( !isspace( *i ) && *i!=')' && *i!='(' && *i!='"' )
        {
            m_mystring += *i;
        }
    }
}

//-----<FILE_OUTPUTFORMATTER>----------------------------------------

FILE_OUTPUTFORMATTER::FILE_OUTPUTFORMATTER( const wxString& aFileName, const wxChar* aMode,
                                            char aQuoteChar ):
    OUTPUTFORMATTER( OUTPUTFMTBUFZ, aQuoteChar ),
    m_filename( aFileName )
{
    m_fp = wxFopen( aFileName, aMode );

    if( !m_fp )
        THROW_IO_ERROR( strerror( errno ) );
}


FILE_OUTPUTFORMATTER::~FILE_OUTPUTFORMATTER()
{
    if( m_fp )
        fclose( m_fp );
}


void FILE_OUTPUTFORMATTER::write( const char* aOutBuf, int aCount )
{
    if( fwrite( aOutBuf, (unsigned) aCount, 1, m_fp ) != 1 )
        THROW_IO_ERROR( strerror( errno ) );
}


//-----<STREAM_OUTPUTFORMATTER>--------------------------------------

void STREAM_OUTPUTFORMATTER::write( const char* aOutBuf, int aCount )
{
    int lastWrite;

    // This might delay awhile if you were writing to say a socket, but for
    // a file it should only go through the loop once.
    for( int total = 0;  total<aCount;  total += lastWrite )
    {
        lastWrite = m_os.Write( aOutBuf, aCount ).LastWrite();

        if( !m_os.IsOk() )
        {
            THROW_IO_ERROR( _( "OUTPUTSTREAM_OUTPUTFORMATTER write error" ) );
        }
    }
}

