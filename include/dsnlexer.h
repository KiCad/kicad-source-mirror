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

#ifndef DSNLEXER_H_
#define DSNLEXER_H_

#include <stdio.h>
#include <string>
#include <vector>
#include <hashtables.h>

#include <richio.h>

#ifndef SWIG
/**
 * Struct KEYWORD
 * holds a keyword string and its unique integer token.
 */
struct KEYWORD
{
    const char* name;       ///< unique keyword.
    int         token;      ///< a zero based index into an array of KEYWORDs
};
#endif

// something like this macro can be used to help initialize a KEYWORD table.
// see SPECCTRA_DB::keywords[] as an example.

//#define TOKDEF(x)    { #x, T_##x }


/**
 * Enum DSN_SYNTAX_T
 * lists all the DSN lexer's tokens that are supported in lexing.  It is up
 * to the parser if it wants also to support them.
 */
enum DSN_SYNTAX_T {
    DSN_NONE = -11,
    DSN_COMMENT = -10,
    DSN_STRING_QUOTE = -9,
    DSN_QUOTE_DEF = -8,
    DSN_DASH = -7,
    DSN_SYMBOL = -6,
    DSN_NUMBER = -5,
    DSN_RIGHT = -4,           // right bracket, ')'
    DSN_LEFT = -3,            // left bracket, '('
    DSN_STRING = -2,          // a quoted string, stripped of the quotes
    DSN_EOF = -1              // special case for end of file
};


/**
 * Class DSNLEXER
 * implements a lexical analyzer for the SPECCTRA DSN file format.  It
 * reads lexical tokens from the current LINE_READER through the NextTok()
 * function.
 */
class DSNLEXER
{
#ifndef SWIG
protected:
    bool                iOwnReaders;            ///< on readerStack, should I delete them?
    const char*         start;
    const char*         next;
    const char*         limit;
    char                dummy[1];               ///< when there is no reader.

    typedef std::vector<LINE_READER*>  READER_STACK;

    READER_STACK        readerStack;            ///< all the LINE_READERs by pointer.
    LINE_READER*        reader;                 ///< no ownership. ownership is via readerStack, maybe, if iOwnReaders

    bool                specctraMode;           ///< if true, then:
                                                ///< 1) stringDelimiter can be changed
                                                ///< 2) Kicad quoting protocol is not in effect
                                                ///< 3) space_in_quoted_tokens is functional
                                                ///< else not.

    char                stringDelimiter;
    bool                space_in_quoted_tokens; ///< blank spaces within quoted strings

    bool                commentsAreTokens;      ///< true if should return comments as tokens

    int                 prevTok;                ///< curTok from previous NextTok() call.
    int                 curOffset;              ///< offset within current line of the current token

    int                 curTok;                 ///< the current token obtained on last NextTok()
    std::string         curText;                ///< the text of the current token

    const KEYWORD*      keywords;               ///< table sorted by CMake for bsearch()
    unsigned            keywordCount;           ///< count of keywords table
    KEYWORD_MAP         keyword_hash;           ///< fast, specialized "C string" hashtable

    void init();

    int readLine() throw( IO_ERROR )
    {
        if( reader )
        {
            reader->ReadLine();

            unsigned len = reader->Length();

            // start may have changed in ReadLine(), which can resize and
            // relocate reader's line buffer.
            start = reader->Line();

            next  = start;
            limit = next + len;

            return len;
        }
        return 0;
    }

    /**
     * Function findToken
     * takes aToken string and looks up the string in the keywords table.
     *
     * @param aToken is a string to lookup in the keywords table.
     * @return int - with a value from the enum DSN_T matching the keyword text,
     *         or DSN_SYMBOL if @a aToken is not in the kewords table.
     */
    int findToken( const std::string& aToken );

    bool isStringTerminator( char cc )
    {
        if( !space_in_quoted_tokens && cc==' ' )
            return true;

        if( cc == stringDelimiter )
            return true;

        return false;
    }

#endif

public:

    /**
     * Constructor ( FILE*, const wxString& )
     * intializes a DSN lexer and prepares to read from aFile which
     * is already open and has aFilename.
     *
     * @param aKeywordTable is an array of KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     * @param aFile is an open file, which will be closed when this is destructed.
     * @param aFileName is the name of the file
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
              FILE* aFile, const wxString& aFileName );

    /**
     * Constructor ( const KEYWORD*, unsigned, const std::string&, const wxString& )
     * intializes a DSN lexer and prepares to read from @a aSExpression.
     *
     * @param aKeywordTable is an array of KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     * @param aSExpression is text to feed through a STRING_LINE_READER
     * @param aSource is a description of aSExpression, used for error reporting.
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
              const std::string& aSExpression, const wxString& aSource = wxEmptyString );

    /**
     * Constructor ( const std::string&, const wxString& )
     * intializes a DSN lexer and prepares to read from @a aSExpression.  Use this
     * one without a keyword table with the DOM parser in ptree.h.
     *
     * @param aSExpression is text to feed through a STRING_LINE_READER
     * @param aSource is a description of aSExpression, used for error reporting.
     */
    DSNLEXER( const std::string& aSExpression, const wxString& aSource = wxEmptyString );

    /**
     * Constructor ( LINE_READER* )
     * intializes a DSN lexer and prepares to read from @a aLineReader which
     * is already open, and may be in use by other DSNLEXERs also.  No ownership
     * is taken of @a aLineReader. This enables it to be used by other DSNLEXERs also.
     *
     * @param aKeywordTable is an array of KEYWORDS holding \a aKeywordCount.  This
     *  token table need not contain the lexer separators such as '(' ')', etc.
     *
     * @param aKeywordCount is the count of tokens in aKeywordTable.
     *
     * @param aLineReader is any subclassed instance of LINE_READER, such as
     *  STRING_LINE_READER or FILE_LINE_READER.  No ownership is taken.
     */
    DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
              LINE_READER* aLineReader = NULL );

    virtual ~DSNLEXER();

    /**
     * Useable only for DSN lexers which share the same LINE_READER
     * Synchronizes the pointers handling the data read by the LINE_READER
     * Allows 2 DNSLEXER to share the same current line, when switching from a
     * DNSLEXER to an other DNSLEXER
     * @param aLexer = the model
     * @return true if the sync can be made ( at least the same line reader )
     */
    bool SyncLineReaderWith( DSNLEXER& aLexer );

    /**
     * Function SetSpecctraMode
     * changes the behavior of this lexer into or out of "specctra mode".  If
     * specctra mode, then:
     * 1) stringDelimiter can be changed
     * 2) Kicad quoting protocol is not in effect
     * 3) space_in_quoted_tokens is functional
     * else none of the above are true.  The default mode is non-specctra mode, meaning:
     * 1) stringDelimiter cannot be changed
     * 2) Kicad quoting protocol is in effect
     * 3) space_in_quoted_tokens is not functional
     */
    void SetSpecctraMode( bool aMode );

    /**
     * Function PushReader
     * manages a stack of LINE_READERs in order to handle nested file inclusion.
     * This function pushes aLineReader onto the top of a stack of LINE_READERs and makes
     * it the current LINE_READER with its own GetSource(), line number and line text.
     * A grammar must be designed such that the "include" token (whatever its various names),
     * and any of its parameters are not followed by anything on that same line,
     * because PopReader always starts reading from a new line upon returning to
     * the original LINE_READER.
     */
    void PushReader( LINE_READER* aLineReader );

    /**
     * Function PopReader
     * deletes the top most LINE_READER from an internal stack of LINE_READERs and
     * in the case of FILE_LINE_READER this means the associated FILE is closed.
     * The most recently used former LINE_READER on the stack becomes the
     * current LINE_READER and its previous position in its input stream and the
     * its latest line number should pertain.  PopReader always starts reading
     * from a new line upon returning to the previous LINE_READER.  A pop is only
     * possible if there are at least 2 LINE_READERs on the stack, since popping
     * the last one is not supported.
     *
     * @return LINE_READER* - is the one that was in use before the pop, or NULL
     *   if there was not at least two readers on the stack and therefore the
     *   pop failed.
     */
    LINE_READER* PopReader();

    // Some functions whose return value is best overloaded to return an enum
    // in a derived class.
    //-----<overload return values to tokens>------------------------------

    /**
     * Function NextTok
     * returns the next token found in the input file or DSN_EOF when reaching
     * the end of file.  Users should wrap this function to return an enum
     * to aid in grammar debugging while running under a debugger, but leave
     * this lower level function returning an int (so the enum does not collide
     * with another usage).
     * @return int - the type of token found next.
     * @throw IO_ERROR - only if the LINE_READER throws it.
     */
    int NextTok() throw( IO_ERROR );

    /**
     * Function NeedSYMBOL
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol().
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy IsSymbol()
     */
    int NeedSYMBOL() throw( IO_ERROR );

    /**
     * Function NeedSYMBOLorNUMBER
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol() or tok==DSN_NUMBER.
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy the above test
     */
    int NeedSYMBOLorNUMBER() throw( IO_ERROR );

    /**
     * Function NeedNUMBER
     * calls NextTok() and then verifies that the token read is type DSN_NUMBER.
     * If not, and IO_ERROR is thrown using text from aExpectation.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy the above test
     */
    int NeedNUMBER( const char* aExpectation ) throw( IO_ERROR );

    /**
     * Function CurTok
     * returns whatever NextTok() returned the last time it was called.
     */
    int CurTok()
    {
        return curTok;
    }

    /**
     * Function PrevTok
     * returns whatever NextTok() returned the 2nd to last time it was called.
     */
    int PrevTok()
    {
        return prevTok;
    }

    //-----</overload return values to tokens>-----------------------------


    /**
     * Function SetStringDelimiter
     * changes the string delimiter from the default " to some other character
     * and returns the old value.
     * @param aStringDelimiter The character in lowest 8 bits.
     * @return int - The old delimiter in the lowest 8 bits.
     */
    char SetStringDelimiter( char aStringDelimiter )
    {
        int old = stringDelimiter;
        if( specctraMode )
            stringDelimiter = aStringDelimiter;
        return old;
    }

    /**
     * Function SetSpaceInQuotedTokens
     * changes the setting controlling whether a space in a quoted string is
     * a terminator.
     * @param val If true, means
     */
    bool SetSpaceInQuotedTokens( bool val )
    {
        bool old = space_in_quoted_tokens;
        if( specctraMode )
            space_in_quoted_tokens = val;
        return old;
    }

    /**
     * Function SetCommentsAreTokens
     * changes the handling of comments.  If set true, comments are returns
     * as single line strings with a terminating newline, else they are
     * consumed by the lexer and not returned.
     */
    bool SetCommentsAreTokens( bool val )
    {
        bool old = commentsAreTokens;
        commentsAreTokens = val;
        return old;
    }

    /**
     * Function ReadCommentLines
     * checks the next sequence of tokens and reads them into a wxArrayString
     * if they are comments.  Reading continues until a non-comment token is
     * encountered, and such last read token remains as CurTok() and as CurText().
     * No push back or "un get" mechanism is used for this support.  Upon return
     * you simply avoid calling NextTok() for the next token, but rather CurTok().
     *
     * @return wxArrayString* - heap allocated block of comments, or NULL if none;
     *   caller owns the allocation and must delete if not NULL.
     */
    wxArrayString* ReadCommentLines() throw( IO_ERROR );

    /**
     * Function IsSymbol
     * tests a token to see if it is a symbol.  This means it cannot be a
     * special delimiter character such as DSN_LEFT, DSN_RIGHT, DSN_QUOTE, etc.  It may
     * however, coincidentally match a keyword and still be a symbol.
     */
    static bool IsSymbol( int aTok );

    /**
     * Function Expecting
     * throws an IO_ERROR exception with an input file specific error message.
     * @param aTok is the token/keyword type which was expected at the current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Expecting( int aTok ) throw( IO_ERROR );

    /**
     * Function Expecting
     * throws an IO_ERROR exception with an input file specific error message.
     * @param aTokenList is the token/keyword type which was expected at the
     *         current input location, e.g.  "pin|graphic|property"
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Expecting( const char* aTokenList ) throw( IO_ERROR );

    /**
     * Function Unexpected
     * throws an IO_ERROR exception with an input file specific error message.
     * @param aTok is the token/keyword type which was not expected at the
     *         current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Unexpected( int aTok ) throw( IO_ERROR );

    /**
     * Function Unexpected
     * throws an IO_ERROR exception with an input file specific error message.
     * @param aToken is the token which was not expected at the
     *         current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Unexpected( const char* aToken ) throw( IO_ERROR );

    /**
     * Function Duplicate
     * throws an IO_ERROR exception with a message saying specifically that aTok
     * is a duplicate of one already seen in current context.
     * @param aTok is the token/keyword type which was not expected at the
     *         current input location.
     * @throw IO_ERROR with the location within the input file of the problem.
     */
    void Duplicate( int aTok ) throw( IO_ERROR );

    /**
     * Function NeedLEFT
     * calls NextTok() and then verifies that the token read in is a DSN_LEFT.
     * If it is not, an IO_ERROR is thrown.
     * @throw IO_ERROR, if the next token is not a DSN_LEFT
     */
    void NeedLEFT() throw( IO_ERROR );

    /**
     * Function NeedRIGHT
     * calls NextTok() and then verifies that the token read in is a DSN_RIGHT.
     * If it is not, an IO_ERROR is thrown.
     * @throw IO_ERROR, if the next token is not a DSN_RIGHT
     */
    void NeedRIGHT() throw( IO_ERROR );

    /**
     * Function GetTokenText
     * returns the C string representation of a DSN_T value.
     */
    const char* GetTokenText( int aTok );

    /**
     * Function GetTokenString
     * returns a quote wrapped wxString representation of a token value.
     */
    wxString GetTokenString( int aTok );

    static const char* Syntax( int aTok );

    /**
     * Function CurText
     * returns a pointer to the current token's text.
     */
    const char* CurText()
    {
        return curText.c_str();
    }

    /**
     * Function CurStr
     * returns a reference to current token in std::string form.
     */
    const std::string& CurStr()
    {
        return curText;
    }

    /**
     * Function FromUTF8
     * returns the current token text as a wxString, assuming that the input
     * byte stream is UTF8 encoded.
     */
    wxString FromUTF8()
    {
        return wxString::FromUTF8( curText.c_str() );
    }

    /**
     * Function CurLineNumber
     * returns the current line number within my LINE_READER
     */
    int CurLineNumber()
    {
        return reader->LineNumber();
    }

    /**
     * Function CurLine
     * returns the current line of text, from which the CurText() would return
     * its token.
     */
    const char* CurLine()
    {
        return (const char*)(*reader);
    }

    /**
     * Function CurFilename
     * returns the current LINE_READER source.
     * @return const wxString& - the source of the lines of text,
     *   e.g. a filename or "clipboard".
     */
    const wxString& CurSource()
    {
        return reader->GetSource();
    }

    /**
     * Function CurOffset
     * returns the byte offset within the current line, using a 1 based index.
     * @return int - a one based index into the current line.
     */
    int CurOffset()
    {
        return curOffset + 1;
    }
};

#endif  // DSNLEXER_H_
