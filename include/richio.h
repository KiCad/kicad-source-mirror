/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef RICHIO_H_
#define RICHIO_H_

// This file defines 3 classes useful for working with DSN text files and is named
// "richio" after its author, Richard Hollenbeck, aka Dick Hollenbeck.


#include <vector>
#include <utf8.h>

// I really did not want to be dependent on wxWidgets in richio
// but the errorText needs to be wide char so wxString rules.
#include <wx/wx.h>
#include <stdio.h>


/**
 * Function StrPrintf
 * is like sprintf() but the output is appended to a std::string instead of to a
 * character array.
 * @param aResult is the string to append to, previous text is not clear()ed.
 * @param aFormat is a printf() style format string.
 * @return int - the count of bytes appended to the result string, no terminating
 *           nul is included.
 */
int
#if defined(__GNUG__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    StrPrintf( std::string* aResult, const char* aFormat, ... );


/**
 * Function StrPrintf
 * is like sprintf() but the output is returned in a std::string instead of to a
 * character array.
 * @param format is a printf() style format string.
 * @return std::string - the result of the sprintf().
 */
std::string
#if defined(__GNUG__)
    __attribute__ ((format (printf, 1, 2)))
#endif
    StrPrintf( const char* format, ... );


/**
 * @ingroup exception_types
 * @{
 */


#define IO_FORMAT       _( "IO_ERROR: %s\nfrom %s : %s" )
#define PARSE_FORMAT    _( "PARSE_ERROR: %s in input/source\n'%s'\nline %d  offset %d\nfrom %s : %s" )

// references:
// http://stackoverflow.com/questions/2670816/how-can-i-use-the-compile-time-constant-line-in-a-string
#define STRINGIFY(x)    #x
#define TOSTRING(x)     STRINGIFY(x)

// use one of the following __LOC__ defs, depending on whether your
// compiler supports __func__ or not, and how it handles __LINE__
#define __LOC__         ((std::string(__FUNCTION__) + "() : line ") + TOSTRING(__LINE__)).c_str()
//#define __LOC__         TOSTRING(__LINE__)

/// macro which captures the "call site" values of __FILE_ & __LOC__
#define THROW_IO_ERROR( msg )   throw IO_ERROR( __FILE__, __LOC__, msg )

/**
 * Struct IO_ERROR
 * is a class used to hold an error message and may be used when throwing exceptions
 * containing meaningful error messages.
 * @author Dick Hollenbeck
 */
struct IO_ERROR // : std::exception
{
    wxString    errorText;

    /**
     * Constructor
     *
     * @param aThrowersFile is the __FILE__ preprocessor macro but generated
     *  at the source file of thrower.
     *
     * @param aThrowersLoc can be either a function name, such as __func__
     *   or a stringified __LINE__ preprocessor macro but generated
     *   at the source function of the thrower, or concatonation.  Use macro
     *   THROW_IO_ERROR() to wrap a call to this constructor at the call site.
     *
     * @param aMsg is error text that will be streamed through wxString.Printf()
     *  using the format string IO_FORMAT above.
     */
    explicit IO_ERROR( const char* aThrowersFile,
              const char* aThrowersLoc,
              const wxString& aMsg )
    {
        init( aThrowersFile, aThrowersLoc, aMsg );
    }

    explicit IO_ERROR( const char* aThrowersFile,
              const char* aThrowersLoc,
              const std::string& aMsg )
    {
        init( aThrowersFile, aThrowersLoc, wxString::FromUTF8( aMsg.c_str() ) );
    }

    explicit IO_ERROR( const char* aThrowersFile,
              const char* aThrowersLoc,
              const char* aMsg )
    {
        init( aThrowersFile, aThrowersLoc, wxString::FromUTF8( aMsg ) );
    }

    /// handle the case where _() is passed as aMsg.
    explicit IO_ERROR( const char* aThrowersFile,
              const char* aThrowersLoc,
              const wxChar* aMsg )
    {
        init( aThrowersFile, aThrowersLoc, wxString( aMsg ) );
    }

    void init( const char* aThrowersFile, const char* aThrowersLoc, const wxString& aMsg );

    IO_ERROR() {}

    // Destructor is virtual because PARSE_ERROR is derived from it and
    // boost::ptr_vector lists consisting of both will need a virtual destructor.
    virtual ~IO_ERROR() throw ( /*none*/ ){}
};


/**
 * Struct PARSE_ERROR
 * contains a filename or source description, a problem input line, a line number,
 * a byte offset, and an error message which contains the the caller's report and his
 * call site information: CPP source file, function, and line number.
 * @author Dick Hollenbeck
 */
struct PARSE_ERROR : public IO_ERROR
{
    // wxString errorText is still public from IO_ERROR

    int         lineNumber;     ///< at which line number, 1 based index.
    int         byteIndex;      ///< at which byte offset within the line, 1 based index

    /// problem line of input [say, from a LINE_READER].
    /// this is brought up in original byte format rather than wxString form, incase
    /// there was a problem with the encoding, in which case converting to wxString is
    /// not reliable in this context.
    std::string inputLine;

    /**
     * Constructor
     * which is normally called via the macro THROW_PARSE_ERROR so that
     * __FILE__ and __LOC__ can be captured from the call site.
     */
    PARSE_ERROR( const char* aThrowersFile, const char* aThrowersLoc,
                 const wxString& aMsg, const wxString& aSource,
                 const char* aInputLine,
                 int aLineNumber, int aByteIndex ) :
        IO_ERROR()
    {
        init( aThrowersFile, aThrowersLoc, aMsg, aSource, aInputLine, aLineNumber, aByteIndex );
    }

    void init( const char* aThrowersFile, const char* aThrowersLoc,
               const wxString& aMsg, const wxString& aSource,
               const char* aInputLine,
               int aLineNumber, int aByteIndex );

    ~PARSE_ERROR() throw ( /*none*/ ){}
};


#define THROW_PARSE_ERROR( aMsg, aSource, aInputLine, aLineNumber, aByteIndex )  \
        throw PARSE_ERROR( __FILE__, __LOC__, aMsg, aSource, aInputLine, aLineNumber, aByteIndex )


/** @} exception_types */


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
    unsigned    lineNum;

    char*       line;           ///< the read line of UTF8 text
    unsigned    capacity;       ///< no. bytes allocated for line.

    unsigned    maxLineLength;  ///< maximum allowed capacity using resizing.

    wxString    source;         ///< origin of text lines, e.g. filename or "clipboard"

    /**
     * Function expandCapacity
     * will expand the capacity of @a line up to maxLineLength but not greater, so
     * be careful about making assumptions of @a capacity after calling this.
     */
    void        expandCapacity( unsigned newsize );


public:

    /**
     * Constructor LINE_READER
     * builds a line reader and fixes the length of the maximum supported
     * line length to @a aMaxLineLength.
     */
    LINE_READER( unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    virtual ~LINE_READER();

    /**
     * Function ReadLine
     * reads a line of text into the buffer and increments the line number
     * counter.  If the line is larger than aMaxLineLength passed to the
     * constructor, then an exception is thrown.  The line is nul terminated.
     * @return char* - The beginning of the read line, or NULL if EOF.
     * @throw IO_ERROR when a line is too long.
     */
    virtual char* ReadLine() throw( IO_ERROR ) = 0;

    /**
     * Function GetSource
     * returns the name of the source of the lines in an abstract sense.
     * This may be a file or it may be the clipboard or any other source
     * of lines of text.  The returned string is useful for reporting error
     * messages.
     */
    virtual const wxString& GetSource() const
    {
        return source;
    }

    /**
     * Function Line
     * returns a pointer to the last line that was read in.
     */
    char* Line() const
    {
        return line;
    }

    /**
     * Operator char*
     * is a casting operator that returns a char* pointer to the start of the
     * line buffer.
     */
    operator char* () const
    {
        return Line();
    }

    /**
     * Function Line Number
     * returns the line number of the last line read from this LINE_READER.  Lines
     * start from 1.
     */
    virtual unsigned LineNumber() const
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

    bool    iOwn;   ///< if I own the file, I'll promise to close it, else not.
    FILE*   fp;     ///< I may own this file, but might not.

public:

    /**
     * Constructor FILE_LINE_READER
     * takes @a aFileName and the size of the desired line buffer and opens
     * the file and assumes the obligation to close it.
     *
     * @param aFileName is the name of the file to open and to use for error reporting purposes.
     *
     * @param aStartingLineNumber is the initial line number to report on error, and is
     *  accessible here for the case where multiple DSNLEXERs are reading from the
     *  same file in sequence, all from the same open file (with @a doOwn = false).
     *  Internally it is incremented by one after each ReadLine(), so the first
     *  reported line number will always be one greater than what is provided here.
     *
     * @param aMaxLineLength is the number of bytes to use in the line buffer.
     *
     * @throw IO_ERROR if @a aFileName cannot be opened.
     */
    FILE_LINE_READER( const wxString& aFileName,
            unsigned aStartingLineNumber = 0,
            unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX ) throw( IO_ERROR );

    /**
     * Constructor FILE_LINE_READER
     * takes an open FILE and the size of the desired line buffer and takes
     * ownership of the open file, i.e. assumes the obligation to close it.
     *
     * @param aFile is an open file.
     * @param aFileName is the name of the file for error reporting purposes.
     * @param doOwn if true, means I should close the open file, else not.
     * @param aStartingLineNumber is the initial line number to report on error, and is
     *  accessible here for the case where multiple DSNLEXERs are reading from the
     *  same file in sequence, all from the same open file (with @a doOwn = false).
     *  Internally it is incremented by one after each ReadLine(), so the first
     *  reported line number will always be one greater than what is provided here.
     * @param aMaxLineLength is the number of bytes to use in the line buffer.
     */
    FILE_LINE_READER( FILE* aFile, const wxString& aFileName, bool doOwn = true,
            unsigned aStartingLineNumber = 0,
            unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    /**
     * Destructor
     * may or may not close the open file, depending on @a doOwn in constructor.
     */
    ~FILE_LINE_READER();

    char* ReadLine() throw( IO_ERROR );   // see LINE_READER::ReadLine() description

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
     *  can be anything meaninful, such as wxT( "clipboard" ).
     */
    STRING_LINE_READER( const std::string& aString, const wxString& aSource );

    /**
     * Constructor STRING_LINE_READER( const STRING_LINE_READER& )
     * allows for a continuation of the reading of a stream started by another
     * STRING_LINE_READER.  Any stream offset and source name are used from
     * @a aStartingPoint.
     */
    STRING_LINE_READER( const STRING_LINE_READER& aStartingPoint );

    char* ReadLine() throw( IO_ERROR );    // see LINE_READER::ReadLine() description
};


/**
 * Class INPUTSTREAM_LINE_READER
 * is a LINE_READER that reads from a wxInputStream object.
 */
class INPUTSTREAM_LINE_READER : public LINE_READER
{
protected:
    wxInputStream* m_stream;   //< The input stream to read.  No ownership of this pointer.

public:

    /**
     * Constructor WXINPUTSTREAM_LINE_READER
     *
     * @param aStream A pointer to a wxInputStream object to read.
     * @param aSource The name of the stream source, for error reporting purposes.
     */
    INPUTSTREAM_LINE_READER( wxInputStream* aStream, const wxString& aSource );

    char* ReadLine() throw( IO_ERROR );    // see LINE_READER::ReadLine() description
};


#define OUTPUTFMTBUFZ    500        ///< default buffer size for any OUTPUT_FORMATTER

/**
 * Class OUTPUTFORMATTER
 * is an important interface (abstract class) used to output 8 bit text in
 * a convenient way. The primary interface is "printf() - like" but
 * with support for indentation control.  The destination of the 8 bit
 * wide text is up to the implementer.
 * <p>
 * The implementer only has to implement the write() function, but can
 * also optionally re-implement GetQuoteChar().
 * <p>
 * If you want to output a wxString, then use TO_UTF8() on it
 * before passing it as an argument to Print().
 * <p>
 * Since this is an abstract interface, only classes derived from
 * this one may actually be used.
 */
class OUTPUTFORMATTER
{
    std::vector<char>   buffer;
    char                quoteChar[2];

    int sprint( const char* fmt, ... )  throw( IO_ERROR );
    int vprint( const char* fmt,  va_list ap )  throw( IO_ERROR );


protected:
    OUTPUTFORMATTER( int aReserve = OUTPUTFMTBUFZ, char aQuoteChar = '"' ) :
            buffer( aReserve, '\0' )
    {
        quoteChar[0] = aQuoteChar;
        quoteChar[1] = '\0';
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
    virtual const char* GetQuoteChar( const char* wrapee );

    /**
     * Function Quotes
     * checks \a aWrapee input string for a need to be quoted
     * (e.g. contains a ')' character or a space), and for \" double quotes
     * within the string that need to be escaped such that the DSNLEXER
     * will correctly parse the string from a file later.
     *
     * @param aWrapee is a string that might need wraping in double quotes,
     *  and it might need to have its internal content escaped, or not.
     *
     * @return std::string - whose c_str() function can be called for passing
     *   to printf() style functions that output UTF8 encoded s-expression streams.
     *
     * @throw IO_ERROR, if there is any kind of problem with the input string.
     */
     virtual std::string Quotes( const std::string& aWrapee ) throw( IO_ERROR );

     std::string Quotew( const wxString& aWrapee ) throw( IO_ERROR );

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
    STRING_FORMATTER( int aReserve = OUTPUTFMTBUFZ, char aQuoteChar = '"' ) :
        OUTPUTFORMATTER( aReserve, aQuoteChar )
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

    const std::string& GetString()
    {
        return mystring;
    }

protected:
    //-----<OUTPUTFORMATTER>------------------------------------------------
    void write( const char* aOutBuf, int aCount ) throw( IO_ERROR );
    //-----</OUTPUTFORMATTER>-----------------------------------------------
};


/**
 * Class FILE_OUTPUTFORMATTER
 * may be used for text file output.  It is about 8 times faster than
 * STREAM_OUTPUTFORMATTER for file streams.
 */
class FILE_OUTPUTFORMATTER : public OUTPUTFORMATTER
{
public:

    /**
     * Constructor
     * @param aFileName is the full filename to open and save to as a text file.
     * @param aMode is what you would pass to wxFopen()'s mode, defaults to wxT( "wt" )
     *      for text files that are to be created here and now.
     * @param aQuoteChar is a char used for quoting problematic strings
            (with whitespace or special characters in them).
     * @throw IO_ERROR if the file cannot be opened.
     */
    FILE_OUTPUTFORMATTER(   const wxString& aFileName,
                            const wxChar* aMode = wxT( "wt" ),
                            char aQuoteChar = '"' )
        throw( IO_ERROR );

    ~FILE_OUTPUTFORMATTER();

protected:
    //-----<OUTPUTFORMATTER>------------------------------------------------
    void write( const char* aOutBuf, int aCount ) throw( IO_ERROR );
    //-----</OUTPUTFORMATTER>-----------------------------------------------

    FILE*       m_fp;               ///< takes ownership
    wxString    m_filename;
};


/**
 * Class STREAM_OUTPUTFORMATTER
 * implements OUTPUTFORMATTER to a wxWidgets wxOutputStream.  The stream is
 * neither opened nor closed by this class.
 */
class STREAM_OUTPUTFORMATTER : public OUTPUTFORMATTER
{
    wxOutputStream& os;

public:
    /**
     * Constructor STREAM_OUTPUTFORMATTER
     * can take any number of wxOutputStream derivations, so it can write
     * to a file, socket, or zip file.
     */
    STREAM_OUTPUTFORMATTER( wxOutputStream& aStream, char aQuoteChar = '"' ) :
        OUTPUTFORMATTER( OUTPUTFMTBUFZ, aQuoteChar ),
        os( aStream )
    {
    }

protected:
    //-----<OUTPUTFORMATTER>------------------------------------------------
    void write( const char* aOutBuf, int aCount ) throw( IO_ERROR );
    //-----</OUTPUTFORMATTER>-----------------------------------------------
};

#endif // RICHIO_H_
