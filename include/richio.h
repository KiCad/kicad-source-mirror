
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

#ifndef RICHIO_H_
#define RICHIO_H_


// This file defines 3 classes useful for working with DSN text files and is named
// "richio" after its author, Richard Hollenbeck, aka Dick Hollenbeck.


#include <string>
#include <vector>

// I really did not want to be dependent on wxWidgets in richio
// but the errorText needs to be wide char so wxString rules.
#include <wx/wx.h>
#include <cstdio>       // FILE



/**
 * Struct IO_ERROR
 * is a class used to hold an error message and may be used to throw exceptions
 * containing meaningful error messages.
 */
struct IO_ERROR
{
    wxString    errorText;

    IO_ERROR( const wxChar* aMsg ) :
        errorText( aMsg )
    {
    }

    IO_ERROR( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }
};

#define LINE_READER_LINE_DEFAULT_MAX        100000
#define LINE_READER_LINE_INITIAL_SIZE       5000

/**
 * Class LINE_READER
 * is an abstract class from which implementation specific LINE_READERs may
 * be derived to read single lines of text and manage a line number counter.
 */
class LINE_READER
{
protected:
    unsigned    length;         ///< no. bytes in line before trailing nul.
    int         lineNum;

    char*       line;           ///< the read line of UTF8 text
    unsigned    capacity;       ///< no. bytes allocated for line.

    unsigned    maxLineLength;  ///< maximum allowed capacity using resizing.

    wxString    source;         ///< origin of text lines, e.g. filename or "clipboard"

    void        expandCapacity( unsigned newsize );

public:

    /**
     * Constructor LINE_READER
     * builds a line reader and fixes the length of the maximum supported
     * line length to @a aMaxLineLength.
     */
    LINE_READER( unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    virtual ~LINE_READER()
    {
        delete[] line;
    }

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number
     * counter.  If the line is larger than aMaxLineLength passed to the
     * constructor, then an exception is thrown.  The line is nul terminated.
     * @return unsigned - The number of bytes read, 0 at end of file.
     * @throw IO_ERROR when a line is too long.
     */
    virtual unsigned ReadLine() throw( IO_ERROR ) = 0;

    /**
     * Function GetSource
     * returns the name of the source of the lines in an abstract sense.
     * This may be a file or it may be the clipboard or any other source
     * of lines of text.  The returned string is useful for reporting error
     * messages.
     */
    const wxString& GetSource() const
    {
        return source;
    }

    /**
     * Operator char*
     * is a casting operator that returns a char* pointer to the start of the
     * line buffer.
     */
    operator char* () const
    {
        return line;
    }

    /**
     * Function Line Number
     * returns the line number of the last line read from this LINE_READER.  Lines
     * start from 1.
     */
    int LineNumber() const
    {
        return lineNum;
    }

    /**
     * Function Length
     * returns the number of bytes in the last line read from this LINE_READER.
     */
    unsigned Length() const
    {
        return length;
    }
};


/**
 * Class FILE_LINE_READER
 * is a LINE_READER that reads from an open file. File must be already open
 * so that this class can exist without any UI policy.
 */
class FILE_LINE_READER : public LINE_READER
{
protected:

    FILE*   fp;     ///< I own this file

public:

    /**
     * Constructor FILE_LINE_READER
     * takes an open FILE and the size of the desired line buffer and takes
     * ownership of the open file, i.e. assumes the obligation to close it.
     *
     * @param aFile is an open file.
     * @param aFileName is the name of the file for error reporting purposes.
     * @param aMaxLineLength is the number of bytes to use in the line buffer.
     */
    FILE_LINE_READER( FILE* aFile,  const wxString& aFileName, unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    ~FILE_LINE_READER()
    {
        if( fp )
            fclose( fp );
    }

    unsigned ReadLine() throw( IO_ERROR );   // see LINE_READER::ReadLine() description

    /**
     * Function Rewind
     * rewinds the file and resets the line number back to zero.  Line number
     * will go to 1 on first ReadLine().
     */
    void Rewind()
    {
        rewind( fp );
        lineNum = 0;
    }
};


/**
 * Class STRING_LINE_READER
 * is a LINE_READER that reads from a multiline 8 bit wide std::string
 */
class STRING_LINE_READER : public LINE_READER
{
protected:
    std::string     lines;
    size_t          ndx;

public:

    /**
     * Constructor STRING_LINE_READER( const std::string&, const wxString& )
     *
     * @param aString is a source string consisting of one or more lines
     * of text, where multiple lines are separated with a '\n' character.
     * The last line does not necessarily need a trailing '\n'.
     *
     * @param aSource describes the source of aString for error reporting purposes
     *  can be anything meaninful, such as wxT( "cliboard" ).
     */
    STRING_LINE_READER( const std::string& aString, const wxString& aSource ) :
        LINE_READER( LINE_READER_LINE_DEFAULT_MAX ),
        lines( aString ),
        ndx( 0 )
    {
        // Clipboard text should be nice and _use multiple lines_ so that
        // we can report _line number_ oriented error messages when parsing.
        source = aSource;
    }

     unsigned ReadLine() throw( IO_ERROR );    // see LINE_READER::ReadLine() description
};


/**
 * Class OUTPUTFORMATTER
 * is an important interface (abstract) class used to output UTF8 text in
 * a convenient way. The primary interface is "printf() - like" but
 * with support for indentation control.  The destination of the 8 bit
 * wide text is up to the implementer.
 * <p>
 * The implementer only has to implement the write() function, but can
 * also optionally re-implement GetQuoteChar().
 * <p>
 * If you want to output a wxString, then use CONV_TO_UTF8() on it
 * before passing it as an argument to Print().
 * <p>
 * Since this is an abstract interface, only classes derived from
 * this one may actually be used.
 */
class OUTPUTFORMATTER
{
    std::vector<char>       buffer;

    int sprint( const char* fmt, ... )  throw( IO_ERROR );
    int vprint( const char* fmt,  va_list ap )  throw( IO_ERROR );


protected:
    OUTPUTFORMATTER( int aReserve = 300 ) :
            buffer( aReserve, '\0' )
    {
    }

    virtual ~OUTPUTFORMATTER() {}

    /**
     * Function GetQuoteChar
     * performs quote character need determination according to the Specctra DSN
     * specification.

     * @param wrapee A string that might need wrapping on each end.
     * @param quote_char A single character C string which provides the current
     *          quote character, should it be needed by the wrapee.
     *
     * @return const char* - the quote_char as a single character string, or ""
     *   if the wrapee does not need to be wrapped.
     */
    static const char* GetQuoteChar( const char* wrapee, const char* quote_char );

    /**
     * Function write
     * should be coded in the interface implementation (derived) classes.
     *
     * @param aOutBuf is the start of a byte buffer to write.
     * @param aCount  tells how many bytes to write.
     * @throw IO_ERROR, if there is a problem outputting, such as a full disk.
     */
    virtual void write( const char* aOutBuf, int aCount ) throw( IO_ERROR ) = 0;

#if defined(__GNUG__)   // The GNU C++ compiler defines this

    // When used on a C++ function, we must account for the "this" pointer,
    // so increase the STRING-INDEX and FIRST-TO_CHECK by one.
    // See http://docs.freebsd.org/info/gcc/gcc.info.Function_Attributes.html
    // Then to get format checking during the compile, compile with -Wall or -Wformat
#define PRINTF_FUNC       __attribute__ ((format (printf, 3, 4)))

#else
#define PRINTF_FUNC       // nothing
#endif

public:

    //-----<interface functions>------------------------------------------

    /**
     * Function Print
     * formats and writes text to the output stream.
     *
     * @param nestLevel The multiple of spaces to precede the output with.
     * @param fmt A printf() style format string.
     * @param ... a variable list of parameters that will get blended into
     *  the output under control of the format string.
     * @return int - the number of characters output.
     * @throw IO_ERROR, if there is a problem outputting, such as a full disk.
     */
    int PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IO_ERROR );

    /**
     * Function GetQuoteChar
     * performs quote character need determination.
     * It returns the quote character as a single character string for a given
     * input wrapee string.  If the wrappee does not need to be quoted,
     * the return value is "" (the null string), such as when there are no
     * delimiters in the input wrapee string.  If you want the quote_char
     * to be assuredly not "", then pass in "(" as the wrappee.
     * <p>
     * Implementations are free to override the default behavior, which is to
     * call the static function of the same name.

     * @param wrapee A string that might need wrapping on each end.
     * @return const char* - the quote_char as a single character string, or ""
     *   if the wrapee does not need to be wrapped.
     */
    virtual const char* GetQuoteChar( const char* wrapee )
    {
        return GetQuoteChar( wrapee, "\"" );
    }

    /**
     * Function Quoted
     * checks \a aWrapee input string for a need to be quoted
     * (e.g. contains a ')' character or a space), and for \" double quotes
     * within the string that need to be doubled up such that the DSNLEXER
     * will correctly parse the string from a file later.
     *
     * @param aWrapee is a string that might need wraping in double quotes,
     *  and it might need to have its internal quotes doubled up, or not.
     *  Caller's copy may be modified, or not.
     *
     * @return const char* - useful for passing to printf() style functions that
     *  must output utf8 streams.
     * @throw IO_ERROR, if aWrapee has any \r or \n bytes in it which is
     *        illegal according to the DSNLEXER who does not ever want them
     *        within a string.
     */
    virtual const char* Quoted( std::string* aWrapee )  throw( IO_ERROR );

    //-----</interface functions>-----------------------------------------
};


/**
 * Class STRING_FORMATTER
 * implements OUTPUTFORMATTER to a memory buffer.  After Print()ing the
 * string is available through GetString()
*/
class STRING_FORMATTER : public OUTPUTFORMATTER
{
    std::string             mystring;

public:

    /**
     * Constructor STRING_FORMATTER
     * reserves space in the buffer
     */
    STRING_FORMATTER( int aReserve = 300 ) :
        OUTPUTFORMATTER( aReserve )
    {
    }

    /**
     * Function Clear
     * clears the buffer and empties the internal string.
     */
    void Clear()
    {
        mystring.clear();
    }

    /**
     * Function StripUseless
     * removes whitespace, '(', and ')' from the mystring.
     */
    void StripUseless();

    std::string GetString()
    {
        return mystring;
    }

    //-----<OUTPUTFORMATTER>------------------------------------------------
protected:
    void write( const char* aOutBuf, int aCount ) throw( IO_ERROR );
    //-----</OUTPUTFORMATTER>-----------------------------------------------
};


/**
 * Class STREAM_OUTPUTFORMATTER
 * implements OUTPUTFORMATTER to a wxWidgets wxOutputStream.  The stream is
 * neither opened nor closed by this class.
 */
class STREAM_OUTPUTFORMATTER : public OUTPUTFORMATTER
{
    wxOutputStream& os;
    char            quoteChar[2];

public:
    /**
     * Constructor STREAM_OUTPUTFORMATTER
     * can take any number of wxOutputStream derivations, so it can write
     * to a file, socket, or zip file.
     */
    STREAM_OUTPUTFORMATTER( wxOutputStream& aStream, char aQuoteChar = '"' ) :
        os( aStream )
    {
        quoteChar[0] = aQuoteChar;
        quoteChar[1] = 0;
    }

    //-----<OUTPUTFORMATTER>------------------------------------------------
    const char* GetQuoteChar( const char* wrapee );

protected:
    void write( const char* aOutBuf, int aCount ) throw( IO_ERROR );
    //-----</OUTPUTFORMATTER>-----------------------------------------------
};

#endif // RICHIO_H_
