/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/utf8.h>

// I really did not want to be dependent on wxWidgets in richio
// but the errorText needs to be wide char so wxString rules.
#include <cstdio>
#include <wx/string.h>
#include <wx/stream.h>

#include <ki_exception.h>
#include <kicommon.h>
#include <io/kicad/kicad_io_utils.h>

/**
 * This is like sprintf() but the output is appended to a std::string instead of to a
 * character array.
 *
 * @param aResult is the string to append to, previous text is not clear()ed.
 * @param aFormat is a printf() style format string.
 * @return the count of bytes appended to the result string, no terminating nul is included.
 */
KICOMMON_API int
#if defined(__GNUG__)
    __attribute__ ((format (printf, 2, 3)))
#endif
    StrPrintf( std::string* aResult, const char* aFormat, ... );


/**
 * This is like sprintf() but the output is returned in a std::string instead of to a
 * character array.
 *
 * @param format is a printf() style format string.
 * @return std::string - the result of the sprintf().
 */
KICOMMON_API std::string
#if defined(__GNUG__)
    __attribute__ ((format (printf, 1, 2)))
#endif
    StrPrintf( const char* format, ... );


/**
 * Nominally opens a file and reads it into a string.  But unlike other facilities, this handles
 * mis-encoded Wine-written files on macOS.
 *
 * @param aFilePath
 * @param aReadType
 * @throw IO_ERROR if the file can't be opened
 * @return the file contents
 */
KICOMMON_API wxString SafeReadFile( const wxString& aFilePath, const wxString& aReadType );


#define LINE_READER_LINE_DEFAULT_MAX        1000000
#define LINE_READER_LINE_INITIAL_SIZE       5000

/**
 * An abstract class from which implementation specific LINE_READERs may be derived to
 * read single lines of text and manage a line number counter.
 */
class KICOMMON_API LINE_READER
{
public:

    /**
     * Build a line reader and fixes the length of the maximum supported line length
     * to @a aMaxLineLength.
     */
    LINE_READER( unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    virtual ~LINE_READER();

    /**
     * Read a line of text into the buffer and increments the line number counter.
     *
     * If the line is larger than the maximum length passed to the constructor, then an
     * exception is thrown.  The line is nul terminated.
     *
     * @return The beginning of the read line, or NULL if EOF.
     * @throw IO_ERROR when a line is too long.
     */
    virtual char* ReadLine() = 0;

    /**
     * Returns the name of the source of the lines in an abstract sense.
     *
     * This may be a file or it may be the clipboard or any other source of lines of text.
     * The returned string is useful for reporting error messages.
     */
    virtual const wxString& GetSource() const
    {
        return m_source;
    }

    /**
     * Return a pointer to the last line that was read in.
     */
    char* Line() const
    {
        return m_line;
    }

    /**
     * A casting operator that returns a char* pointer to the start of the line buffer.
     */
    operator char* () const
    {
        return Line();
    }

    /**
     * Return the line number of the last line read from this LINE_READER.
     *
     * Lines start from 1.
     */
    virtual unsigned LineNumber() const
    {
        return m_lineNum;
    }

    /**
     * Return the number of bytes in the last line read from this LINE_READER.
     */
    unsigned Length() const
    {
        return m_length;
    }

protected:
    /**
     * Will expand the capacity of @a line up to maxLineLength but not greater, so
     * be careful about making assumptions of @a capacity after calling this.
     */
    void        expandCapacity( unsigned aNewsize );

    unsigned    m_length;         ///< no. bytes in line before trailing nul.
    unsigned    m_lineNum;

    char*       m_line;           ///< the read line of UTF8 text
    unsigned    m_capacity;       ///< no. bytes allocated for line.

    unsigned    m_maxLineLength;  ///< maximum allowed capacity using resizing.

    wxString    m_source;         ///< origin of text lines, e.g. filename or "clipboard"
};


/**
 * A LINE_READER that reads from an open file.
 *
 * File must be already open so that this class can exist without any UI policy.
 */
class KICOMMON_API FILE_LINE_READER : public LINE_READER
{
public:
    /**
     * Take @a aFileName and the size of the desired line buffer and opens the file and
     * assumes the obligation to close it.
     *
     * @param aFileName is the name of the file to open and to use for error reporting purposes.
     * @param aStartingLineNumber is the initial line number to report on error, and is
     *  accessible here for the case where multiple DSNLEXERs are reading from the
     *  same file in sequence, all from the same open file (with @a doOwn = false).
     *  Internally it is incremented by one after each ReadLine(), so the first
     *  reported line number will always be one greater than what is provided here.
     * @param aMaxLineLength is the number of bytes to use in the line buffer.
     *
     * @throw IO_ERROR if @a aFileName cannot be opened.
     */
    FILE_LINE_READER( const wxString& aFileName, unsigned aStartingLineNumber = 0,
                      unsigned aMaxLineLength = LINE_READER_LINE_DEFAULT_MAX );

    /**
     * Take an open FILE and the size of the desired line buffer and takes ownership of
     * the open file, i.e. assumes the obligation to close it.
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
     * May or may not close the open file, depending on @a doOwn in constructor.
     */
    ~FILE_LINE_READER();

    char* ReadLine() override;

    /**
     * Rewind the file and resets the line number back to zero.
     *
     * Line number will go to 1 on first ReadLine().
     */
    void Rewind()
    {
        rewind( m_fp );
        m_lineNum = 0;
    }

    long int FileLength();
    long int CurPos();

protected:
    bool    m_iOwn; ///< if I own the file, I'll promise to close it, else not.
    FILE*   m_fp;   ///< I may own this file, but might not.
};


/**
 * Is a #LINE_READER that reads from a multiline 8 bit wide std::string
 */
class KICOMMON_API STRING_LINE_READER : public LINE_READER
{
protected:
    std::string     m_lines;
    size_t          m_ndx;

public:

    /**
     * Construct a string line reader.
     *
     * @param aString is a source string consisting of one or more lines
     * of text, where multiple lines are separated with a '\n' character.
     * The last line does not necessarily need a trailing '\n'.
     * @param aSource describes the source of aString for error reporting purposes
     *  can be anything meaningful, such as wxT( "clipboard" ).
     */
    STRING_LINE_READER( const std::string& aString, const wxString& aSource );

    /**
     * Construct a string line reader.
     *
     * Allows for a continuation of the reading of a stream started by another
     * STRING_LINE_READER.  Any stream offset and source name are used from
     */
    STRING_LINE_READER( const STRING_LINE_READER& aStartingPoint );

    char* ReadLine() override;
};


/**
 * A #LINE_READER that reads from a wxInputStream object.
 */
class KICOMMON_API INPUTSTREAM_LINE_READER : public LINE_READER
{
public:
    /**
     * Construct a #LINE_READER from a wxInputStream object.
     *
     * @param aStream A pointer to a wxInputStream object to read.
     * @param aSource The name of the stream source, for error reporting purposes.
     */
    INPUTSTREAM_LINE_READER( wxInputStream* aStream, const wxString& aSource );

    char* ReadLine() override;

protected:
    wxInputStream* m_stream;        ///< The input stream to read.  No ownership of this pointer.
};


#define OUTPUTFMTBUFZ    500        ///< default buffer size for any OUTPUT_FORMATTER

/**
 * An interface used to output 8 bit text in a convenient way.
 *
 * The primary interface is "printf() - like" but with support for indentation control.  The
 * destination of the 8 bit wide text is up to the implementer.
 * <p>
 * The implementer only has to implement the write() function, but can also optionally
 * re-implement GetQuoteChar().
 * <p>
 * If you want to output a wxString, then use TO_UTF8() on it before passing it as an
 * argument to Print().
 * <p>
 * Since this is an abstract interface, only classes derived from this one may actually be
 * used.
 */
class KICOMMON_API OUTPUTFORMATTER
{
protected:
    OUTPUTFORMATTER( int aReserve = OUTPUTFMTBUFZ, char aQuoteChar = '"' ) :
            m_buffer( aReserve, '\0' )
    {
        quoteChar[0] = aQuoteChar;
        quoteChar[1] = '\0';
    }

    /**
     * Perform quote character need determination according to the Specctra DSN specification.

     * @param wrapee A string that might need wrapping on each end.
     * @param quote_char A single character C string which provides the current
     *                   quote character, should it be needed by the wrapee.
     *
     * @return the quote_char as a single character string, or "" if the wrapee does not need
     *         to be wrapped.
     */
    static const char* GetQuoteChar( const char* wrapee, const char* quote_char );

    /**
     * Should be coded in the interface implementation (derived) classes.
     *
     * @param aOutBuf is the start of a byte buffer to write.
     * @param aCount  tells how many bytes to write.
     * @throw IO_ERROR, if there is a problem outputting, such as a full disk.
     */
    virtual void write( const char* aOutBuf, int aCount ) = 0;

#if defined(__GNUG__)   // The GNU C++ compiler defines this

    // When used on a C++ function, we must account for the "this" pointer,
    // so increase the STRING-INDEX and FIRST-TO_CHECK by one.
    // See http://docs.freebsd.org/info/gcc/gcc.info.Function_Attributes.html
    // Then to get format checking during the compile, compile with -Wall or -Wformat
#define PRINTF_FUNC_N __attribute__( ( format( printf, 3, 4 ) ) )
#define PRINTF_FUNC __attribute__( ( format( printf, 2, 3 ) ) )
#else
#define PRINTF_FUNC_N     // nothing
#define PRINTF_FUNC       // nothing
#endif

public:
    /**
     * This is a polymorphic class that can validly be handled by a pointer to the base class.
     */
    virtual ~OUTPUTFORMATTER() {}

    /**
     * Format and write text to the output stream.
     *
     * @param nestLevel The multiple of spaces to precede the output with.
     * @param fmt A printf() style format string.
     * @param ... a variable list of parameters that will get blended into
     *  the output under control of the format string.
     * @return int - the number of characters output.
     * @throw IO_ERROR, if there is a problem outputting, such as a full disk.
     */
    int PRINTF_FUNC_N Print( int nestLevel, const char* fmt, ... );

    /**
     * Format and write text to the output stream.
     *
     * @param fmt A printf() style format string.
     * @param ... a variable list of parameters that will get blended into
     *  the output under control of the format string.
     * @return int - the number of characters output.
     * @throw IO_ERROR, if there is a problem outputting, such as a full disk.
     */
    int PRINTF_FUNC Print( const char* fmt, ... );

    /**
     * Perform quote character need determination.
     *
     * It returns the quote character as a single character string for a given input wrapee
     * string.  If the wrappee does not need to be quoted, the return value is "" (the null
     * string), such as when there are no delimiters in the input wrapee string.  If you want
     * the quote_char to be assuredly not "", then pass in "(" as the wrappee.
     * <p>
     * Implementations are free to override the default behavior, which is to call the static
     * function of the same name.
     *
     * @param wrapee A string that might need wrapping on each end.
     * @return the quote_char as a single character string, or "" if the wrapee does not need
     *         to be wrapped.
     */
    virtual const char* GetQuoteChar( const char* wrapee ) const;

    /**
     * Check \a aWrapee input string for a need to be quoted (e.g. contains a ')' character
     * or a space), and for \" double quotes within the string that need to be escaped such
     * that the DSNLEXER will correctly parse the string from a file later.
     *
     * @param aWrapee is a string that might need wrapping in double quotes, and it might need
     *                to have its internal content escaped, or not.
     * @return a std::string- whose c_str() function can be called for passing to printf()
     *         style functions that output UTF8 encoded s-expression streams.
     *
     * @throw IO_ERROR, if there is any kind of problem with the input string.
     */
     virtual std::string Quotes( const std::string& aWrapee ) const;

     std::string Quotew( const wxString& aWrapee ) const;

    /**
     * Performs any cleanup needed at the end of a write.
     * @return true if all is well
     */
    virtual bool Finish() { return true; }

private:
    std::vector<char>   m_buffer;
    char                quoteChar[2];

    int sprint( const char* fmt, ... );
    int vprint( const char* fmt, va_list ap );

};


/**
 * Implement an #OUTPUTFORMATTER to a memory buffer.
 *
 * After Print()ing the string is available through GetString()
 */
class KICOMMON_API STRING_FORMATTER : public OUTPUTFORMATTER
{
public:
    /**
     * Reserve space in the buffer.
     */
    STRING_FORMATTER( int aReserve = OUTPUTFMTBUFZ, char aQuoteChar = '"' ) :
        OUTPUTFORMATTER( aReserve, aQuoteChar )
    {
    }

    /**
     * Clear the buffer and empties the internal string.
     */
    void Clear()
    {
        m_mystring.clear();
    }

    /**
     * Removes whitespace, '(', and ')' from the string.
     */
    void StripUseless();

    const std::string& GetString()
    {
        return m_mystring;
    }

    std::string& MutableString()
    {
        return m_mystring;
    }

protected:
    void write( const char* aOutBuf, int aCount ) override;

private:
    std::string m_mystring;
};


/**
 * Used for text file output.
 *
 * It is about 8 times faster than STREAM_OUTPUTFORMATTER for file streams.
 */
class KICOMMON_API FILE_OUTPUTFORMATTER : public OUTPUTFORMATTER
{
public:

    /**
     * @param aFileName is the full filename to open and save to as a text file.
     * @param aMode is what you would pass to wxFopen()'s mode, defaults to wxT( "wt" )
     *              for text files that are to be created here and now.
     * @param aQuoteChar is a char used for quoting problematic strings (with whitespace or
     *                   special characters in them).
     * @throw IO_ERROR if the file cannot be opened.
     */
    FILE_OUTPUTFORMATTER( const wxString& aFileName, const wxChar* aMode = wxT( "wt" ),
                          char aQuoteChar = '"' );

    ~FILE_OUTPUTFORMATTER();

protected:
    void write( const char* aOutBuf, int aCount ) override;

    FILE*       m_fp;               ///< takes ownership
    wxString    m_filename;
};


class KICOMMON_API PRETTIFIED_FILE_OUTPUTFORMATTER : public OUTPUTFORMATTER
{
public:
    PRETTIFIED_FILE_OUTPUTFORMATTER( const wxString& aFileName,
            KICAD_FORMAT::FORMAT_MODE aFormatMode = KICAD_FORMAT::FORMAT_MODE::NORMAL,
            const wxChar* aMode = wxT( "wt" ), char aQuoteChar = '"' );

    ~PRETTIFIED_FILE_OUTPUTFORMATTER();

    /**
     * Performs prettification and writes the stored buffer to the file.
     * @return true if the write succeeded.
     */
    bool Finish() override;

protected:
    void write( const char* aOutBuf, int aCount ) override;

private:
    FILE* m_fp;
    std::string m_buf;
    KICAD_FORMAT::FORMAT_MODE m_mode;
};


#endif // RICHIO_H_
