
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
 * Struct IOError
 * is a class used to hold an error message and may be used to throw exceptions
 * containing meaningful error messages.
 */
struct IOError
{
    wxString    errorText;

    IOError( const wxChar* aMsg ) :
        errorText( aMsg )
    {
    }

    IOError( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }
};


/**
 * Class LINE_READER
 * reads single lines of text into its buffer and increments a line number counter.
 * It throws an exception if a line is too long.
 */
class LINE_READER
{
protected:
    unsigned            length;
    int                 lineNum;
    char*               line;
    unsigned            maxLineLength;
    unsigned            capacity;

public:
    LINE_READER( unsigned aMaxLineLength );

    virtual ~LINE_READER()
    {
        delete[] line;
    }

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number
     * counter.  If the line is larger than the buffer size, then an exception
     * is thrown.
     * @return int - The number of bytes read, 0 at end of file.
     * @throw IOError only when a line is too long.
     */
    virtual int ReadLine() throw (IOError) = 0;

    operator char* ()
    {
        return line;
    }

    int LineNumber()
    {
        return lineNum;
    }

    unsigned Length()
    {
        return length;
    }
};


/**
 * Class FILE_LINE_READER
 * is a LINE_READER that reads from an open file. File must be already open
 * so that this class can exist without and UI policy.
 */
class FILE_LINE_READER : public LINE_READER
{
protected:

    FILE*               fp;     ///< no ownership, no close on destruction

public:
    /**
     * Constructor LINE_READER
     * takes an open FILE and the size of the desired line buffer.
     * @param aFile An open file in "ascii" mode, not binary mode.
     * @param aMaxLineLength The number of bytes to use in the line buffer.
     */
    FILE_LINE_READER( FILE* aFile,  unsigned aMaxLineLength );

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number
     * counter.  If the line is larger than the buffer size, then an exception
     * is thrown.
     * @return int - The number of bytes read, 0 at end of file.
     * @throw IOError only when a line is too long.
     */
    int ReadLine() throw (IOError);

    /**
     * Function Rewind
     * a wrapper to the standard function rewind.
     * also clear the current line number
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
    std::string     source;
    size_t          ndx;

public:

    /**
     * Constructor STRING_LINE_READER( const std::string& aString )
     * @param aString is a source string consisting of one or more lines
     * of text, where multiple lines are separated with a '\n' character.
     * The last line does not necessarily need a trailing '\n'.
     */
    STRING_LINE_READER( const std::string& aString ) :
        LINE_READER( 4096 ),
        source( aString ),
        ndx( 0 )
    {
        // Clipboard text should be nice and _use multiple lines_ so that
        // we can report _line number_ oriented error messages when parsing.
        // Therefore a line of 4096 characters max seems more than adequate.
    }

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number
     * counter.  If the line is larger than the buffer size, then an exception
     * is thrown.
     * @return int - The number of bytes read, 0 at end of file.
     * @throw IOError only when a line is too long.
     */
    int ReadLine() throw (IOError);
};


/**
 * Class OUTPUTFORMATTER
 * is an important interface (abstract) class used to output UTF8 text in a convenient
 * way. The primary interface is "printf() - like" but with support for indentation
 * control.  The destination of the 8 bit wide text is up to the implementer.
 * <p>
 * The implementer only has to implement the write() function, but can also optionaly
 * re-implement GetQuoteChar().
 * <p>
 * If you want to output a wxString, then use CONV_TO_UTF8() on it before passing
 * it as an argument to Print().
 * <p>
 * Since this is an abstract interface, only classes derived from this one
 * may actually be used.
 */
class OUTPUTFORMATTER
{
    std::vector<char>       buffer;

    int sprint( const char* fmt, ... )  throw( IOError );
    int vprint( const char* fmt,  va_list ap )  throw( IOError );


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
     * @throw IOError, if there is a problem outputting, such as a full disk.
     */
    virtual void write( const char* aOutBuf, int aCount ) throw( IOError ) = 0;

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
     * @param nestLevel The multiple of spaces to preceed the output with.
     * @param fmt A printf() style format string.
     * @param ... a variable list of parameters that will get blended into
     *  the output under control of the format string.
     * @return int - the number of characters output.
     * @throw IOError, if there is a problem outputting, such as a full disk.
     */
    int PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IOError );

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
     * checks \a aWrappee input string for a need to be quoted
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
    virtual const char* Quoted( std::string* aWrapee );
        thinking about using wxCharBuffer* instead.
     */

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
    void write( const char* aOutBuf, int aCount ) throw( IOError );
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
    void write( const char* aOutBuf, int aCount ) throw( IOError );
    //-----</OUTPUTFORMATTER>-----------------------------------------------
};

#endif // RICHIO_H_
