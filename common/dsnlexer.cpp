
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <cstdio>
#include <cstdlib>         // bsearch()
#include <cctype>

#include <macros.h>
#include <fctsys.h>
#include <dsnlexer.h>


//#define STANDALONE  1       // enable this for stand alone testing.

#define FMT_CLIPBOARD       _( "clipboard" )


//-----<DSNLEXER>-------------------------------------------------------------

void DSNLEXER::init()
{
    curTok  = DSN_NONE;
    prevTok = DSN_NONE;

    stringDelimiter = '"';

    specctraMode = false;
    space_in_quoted_tokens = false;
    commentsAreTokens = false;

    curOffset = 0;

#if 1
    if( keywordCount > 11 )
    {
        // resize the hashtable bucket count
        keyword_hash.reserve( keywordCount );
    }

    // fill the specialized "C string" hashtable from keywords[]
    const KEYWORD*  it  = keywords;
    const KEYWORD*  end = it + keywordCount;

    for( ; it < end; ++it )
    {
        keyword_hash[it->name] = it->token;
    }
#endif
}


DSNLEXER::DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
                    FILE* aFile, const wxString& aFilename ) :
    iOwnReaders( true ),
    start( NULL ),
    next( NULL ),
    limit( NULL ),
    reader( NULL ),
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    FILE_LINE_READER* fileReader = new FILE_LINE_READER( aFile, aFilename );
    PushReader( fileReader );
    init();
}


DSNLEXER::DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
                    const std::string& aClipboardTxt, const wxString& aSource ) :
    iOwnReaders( true ),
    start( NULL ),
    next( NULL ),
    limit( NULL ),
    reader( NULL ),
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    STRING_LINE_READER* stringReader = new STRING_LINE_READER( aClipboardTxt, aSource.IsEmpty() ?
                                        wxString( FMT_CLIPBOARD ) : aSource );
    PushReader( stringReader );
    init();
}


DSNLEXER::DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
                    LINE_READER* aLineReader ) :
    iOwnReaders( false ),
    start( NULL ),
    next( NULL ),
    limit( NULL ),
    reader( NULL ),
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    if( aLineReader )
        PushReader( aLineReader );
    init();
}


static const KEYWORD empty_keywords[1] = {};

DSNLEXER::DSNLEXER( const std::string& aSExpression, const wxString& aSource ) :
    iOwnReaders( true ),
    start( NULL ),
    next( NULL ),
    limit( NULL ),
    reader( NULL ),
    keywords( empty_keywords ),
    keywordCount( 0 )
{
    STRING_LINE_READER* stringReader = new STRING_LINE_READER( aSExpression, aSource.IsEmpty() ?
                                        wxString( FMT_CLIPBOARD ) : aSource );
    PushReader( stringReader );
    init();
}


DSNLEXER::~DSNLEXER()
{
    if( iOwnReaders )
    {
        // delete the LINE_READERs from the stack, since I own them.
        for( READER_STACK::iterator it = readerStack.begin(); it!=readerStack.end();  ++it )
            delete *it;
    }
}

void DSNLEXER::SetSpecctraMode( bool aMode )
{
    specctraMode = aMode;
    if( aMode )
    {
        // specctra mode defaults, some of which can still be changed in this mode.
        space_in_quoted_tokens = true;
    }
    else
    {
        space_in_quoted_tokens = false;
        stringDelimiter = '"';
    }
}


bool DSNLEXER::SyncLineReaderWith( DSNLEXER& aLexer )
{
    // Synchronize the pointers handling the data read by the LINE_READER
    // only if aLexer shares the same LINE_READER, because only in this case
    // the char buffer is be common

    if( reader != aLexer.reader )
        return false;

    // We can synchronize the pointers which handle the data currently read
    start = aLexer.start;
    next = aLexer.next;
    limit = aLexer.limit;

    // Sync these parameters is not mandatory, but could help
    // for instance in debug
    curText = aLexer.curText;
    curOffset = aLexer.curOffset;

    return true;
}


void DSNLEXER::PushReader( LINE_READER* aLineReader )
{
    readerStack.push_back( aLineReader );
    reader = aLineReader;
    start  = (const char*) (*reader);

    // force a new readLine() as first thing.
    limit = start;
    next  = start;
}


LINE_READER* DSNLEXER::PopReader()
{
    LINE_READER*    ret = 0;

    if( readerStack.size() )
    {
        ret = reader;
        readerStack.pop_back();

        if( readerStack.size() )
        {
            reader = readerStack.back();
            start  = reader->Line();

            // force a new readLine() as first thing.
            limit = start;
            next  = start;
        }
        else
        {
            reader = 0;
            start  = dummy;
            limit  = dummy;
            limit  = dummy;
        }
    }
    return ret;
}


#if 0
static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;

    int ret = strcmp( k1->name, k2->name );
    return ret;
}

int DSNLEXER::findToken( const std::string& tok )
{
    KEYWORD search;

    search.name = tok.c_str();

    const KEYWORD* findings = (const KEYWORD*) bsearch( &search,
                                   keywords, keywordCount,
                                   sizeof(KEYWORD), compare );
    if( findings )
        return findings->token;
    else
        return DSN_SYMBOL;      // not a keyword, some arbitrary symbol.
}

#else

inline int DSNLEXER::findToken( const std::string& tok )
{
    KEYWORD_MAP::const_iterator it = keyword_hash.find( tok.c_str() );
    if( it != keyword_hash.end() )
        return it->second;

    return DSN_SYMBOL;      // not a keyword, some arbitrary symbol.
}
#endif


const char* DSNLEXER::Syntax( int aTok )
{
    const char* ret;

    switch( aTok )
    {
    case DSN_NONE:
        ret = "NONE";
        break;
    case DSN_STRING_QUOTE:
        ret = "string_quote";   // a special DSN syntax token, see specctra spec.
        break;
    case DSN_QUOTE_DEF:
        ret = "quoted text delimiter";
        break;
    case DSN_DASH:
        ret = "-";
        break;
    case DSN_SYMBOL:
        ret = "symbol";
        break;
    case DSN_NUMBER:
        ret = "number";
        break;
    case DSN_RIGHT:
        ret = ")";
        break;
    case DSN_LEFT:
        ret = "(";
        break;
    case DSN_STRING:
        ret = "quoted string";
        break;
    case DSN_EOF:
        ret = "end of input";
        break;
    default:
        ret = "???";
    }

    return ret;
}


const char* DSNLEXER::GetTokenText( int aTok )
{
    const char* ret;

    if( aTok < 0 )
    {
        return Syntax( aTok );
    }
    else if( (unsigned) aTok < keywordCount )
    {
        ret = keywords[aTok].name;
    }
    else
        ret = "token too big";

    return ret;
}


wxString DSNLEXER::GetTokenString( int aTok )
{
    wxString    ret;

    ret << wxT("'") << wxString::FromUTF8( GetTokenText(aTok) ) << wxT("'");

    return ret;
}


bool DSNLEXER::IsSymbol( int aTok )
{
    // This is static and not inline to reduce code space.

    // if aTok is >= 0, then it is a coincidental match to a keyword.
    return     aTok==DSN_SYMBOL
            || aTok==DSN_STRING
            || aTok>=0
            ;
}


void DSNLEXER::Expecting( int aTok ) throw( IO_ERROR )
{
    wxString errText = wxString::Format(
        _("Expecting '%s'"), GetChars( GetTokenString( aTok ) ) );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Expecting( const char* text ) throw( IO_ERROR )
{
    wxString errText = wxString::Format(
        _("Expecting '%s'"), GetChars( wxString::FromUTF8( text ) ) );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Unexpected( int aTok ) throw( IO_ERROR )
{
    wxString errText = wxString::Format(
        _("Unexpected '%s'"), GetChars( GetTokenString( aTok ) ) );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Duplicate( int aTok ) throw( IO_ERROR )
{
    wxString errText = wxString::Format(
        _("%s is a duplicate"), GetTokenString( aTok ).GetData() );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Unexpected( const char* text ) throw( IO_ERROR )
{
    wxString errText = wxString::Format(
        _("Unexpected '%s'"), GetChars( wxString::FromUTF8( text ) ) );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::NeedLEFT() throw( IO_ERROR )
{
    int tok = NextTok();
    if( tok != DSN_LEFT )
        Expecting( DSN_LEFT );
}


void DSNLEXER::NeedRIGHT() throw( IO_ERROR )
{
    int tok = NextTok();
    if( tok != DSN_RIGHT )
        Expecting( DSN_RIGHT );
}


int DSNLEXER::NeedSYMBOL() throw( IO_ERROR )
{
    int tok = NextTok();
    if( !IsSymbol( tok ) )
        Expecting( DSN_SYMBOL );
    return tok;
}


int DSNLEXER::NeedSYMBOLorNUMBER() throw( IO_ERROR )
{
    int  tok = NextTok();
    if( !IsSymbol( tok ) && tok!=DSN_NUMBER )
        Expecting( "symbol|number" );
    return tok;
}


int DSNLEXER::NeedNUMBER( const char* aExpectation ) throw( IO_ERROR )
{
    int tok = NextTok();
    if( tok != DSN_NUMBER )
    {
        wxString errText = wxString::Format(
            _("need a NUMBER for '%s'"), wxString::FromUTF8( aExpectation ).GetData() );
        THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }
    return tok;
}


/**
 * Function isSpace
 * tests for whitespace.  Our whitespace, by our definition, is a subset of ASCII,
 * i.e. no bytes with MSB on can be considered whitespace, since they are likely part
 * of a multibyte UTF8 character.
 */
static bool isSpace( char cc )
{
    // cc is signed, so it is often negative.
    // Treat negative as large positive to exclude rapidly.
    if( (unsigned char) cc <= ' ' )
    {
        switch( (unsigned char) cc )
        {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case '\0':              // PCAD s-expression files have this.
            return true;
        }
    }
    return false;
}


inline bool isDigit( char cc )
{
    return '0' <= cc && cc <= '9';
}


/// return true if @a cc is an s-expression separator character
inline bool isSep( char cc )
{
    return isSpace( cc ) || cc=='(' || cc==')';
}


/**
 * Function isNumber
 * returns true if the next sequence of text is a number:
 * either an integer, fixed point, or float with exponent.  Stops scanning
 * at the first non-number character, even if it is not whitespace.
 *
 * @param cp is the start of the current token.
 * @param limit is the end of the current token.
 *
 * @return bool - true if input token is a number, else false.
 */
static bool isNumber( const char* cp, const char* limit )
{
    // regex for a float: "^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?" i.e. any number,
    // code traversal manually here:

    bool sawNumber = false;

    if( cp < limit && ( *cp=='-' || *cp=='+' ) )
        ++cp;

    while( cp < limit && isDigit( *cp ) )
    {
        ++cp;
        sawNumber = true;
    }

    if( cp < limit && *cp == '.' )
    {
        ++cp;

        while( cp < limit && isDigit( *cp ) )
        {
            ++cp;
            sawNumber = true;
        }
    }

    if( sawNumber )
    {
        if( cp < limit && ( *cp=='E' || *cp=='e' ) )
        {
            ++cp;

            sawNumber = false;  // exponent mandates at least one digit thereafter.

            if( cp < limit && ( *cp=='-' || *cp=='+' )  )
                ++cp;

            while( cp < limit && isDigit( *cp ) )
            {
                ++cp;
                sawNumber = true;
            }
        }
    }

    return sawNumber && cp==limit;
}


int DSNLEXER::NextTok() throw( IO_ERROR )
{
    const char*   cur  = next;
    const char*   head = cur;

    prevTok = curTok;

    if( curTok == DSN_EOF )
        goto exit;

    if( cur >= limit )
    {
L_read:
        // blank lines are returned as "\n" and will have a len of 1.
        // EOF will have a len of 0 and so is detectable.
        int len = readLine();
        if( len == 0 )
        {
            cur = start;        // after readLine(), since start can change, set cur offset to start
            curTok = DSN_EOF;
            goto exit;
        }

        cur = start;    // after readLine() since start can change.

        // skip leading whitespace
        while( cur<limit && isSpace( *cur ) )
            ++cur;

        // If the first non-blank character is #, this line is a comment.
        // Comments cannot follow any other token on the same line.
        if( cur<limit && *cur=='#' )
        {
            if( commentsAreTokens )
            {
                // Grab the entire current line [excluding end of line char(s)] as the
                // current token.  The '#' character may not be at offset zero.

                while( limit[-1] == '\n' || limit[-1] == '\r' )
                    --limit;

                curText.clear();
                curText.append( start, limit );

                cur     = start;        // ensure a good curOffset below
                curTok  = DSN_COMMENT;
                head    = limit;        // do a readLine() on next call in here.
                goto exit;
            }
            else
                goto L_read;
        }
    }
    else
    {
        // skip leading whitespace
        while( cur<limit && isSpace( *cur ) )
            ++cur;
    }

    if( cur >= limit )
        goto L_read;

    if( *cur == '(' )
    {
        curText = *cur;
        curTok = DSN_LEFT;
        head = cur+1;
        goto exit;
    }

    if( *cur == ')' )
    {
        curText = *cur;
        curTok = DSN_RIGHT;
        head = cur+1;
        goto exit;
    }

    // Non-specctraMode, understands and deciphers escaped \, \r, \n, and \".
    // Strips off leading and trailing double quotes
    if( !specctraMode )
    {
        // a quoted string, will return DSN_STRING
        if( *cur == stringDelimiter )
        {
            // copy the token, character by character so we can remove doubled up quotes.
            curText.clear();

            ++cur;  // skip over the leading delimiter, which is always " in non-specctraMode

            head = cur;

            while( head<limit )
            {
                // ESCAPE SEQUENCES:
                if( *head =='\\' )
                {
                    char    tbuf[8];
                    char    c;
                    int     i;

                    if( ++head >= limit )
                        break;  // throw exception at L_unterminated

                    switch( *head++ )
                    {
                    case '"':
                    case '\\':  c = head[-1];   break;
                    case 'a':   c = '\x07';     break;
                    case 'b':   c = '\x08';     break;
                    case 'f':   c = '\x0c';     break;
                    case 'n':   c = '\n';       break;
                    case 'r':   c = '\r';       break;
                    case 't':   c = '\x09';     break;
                    case 'v':   c = '\x0b';     break;

                    case 'x':   // 1 or 2 byte hex escape sequence
                        for( i=0; i<2; ++i )
                        {
                            if( !isxdigit( head[i] ) )
                                break;
                            tbuf[i] = head[i];
                        }
                        tbuf[i] = '\0';
                        if( i > 0 )
                            c = (char) strtoul( tbuf, NULL, 16 );
                        else
                            c = 'x';   // a goofed hex escape sequence, interpret as 'x'
                        head += i;
                        break;

                    default:    // 1-3 byte octal escape sequence
                        --head;
                        for( i=0; i<3; ++i )
                        {
                            if( head[i] < '0' || head[i] > '7' )
                                break;
                            tbuf[i] = head[i];
                        }
                        tbuf[i] = '\0';
                        if( i > 0 )
                            c = (char) strtoul( tbuf, NULL, 8 );
                        else
                            c = '\\';   // a goofed octal escape sequence, interpret as '\'
                        head += i;
                        break;
                    }

                    curText += c;
                }

                else if( *head == '"' )     // end of the non-specctraMode DSN_STRING
                {
                    curTok = DSN_STRING;
                    ++head;                 // omit this trailing double quote
                    goto exit;
                }

                else
                    curText += *head++;

            }   // while

            // L_unterminated:
            wxString errtxt( _( "Un-terminated delimited string" ) );
            THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }
    }

    else    // is specctraMode, tests in this block should not occur in KiCad mode.
    {
        /*  get the dash out of a <pin_reference> which is embedded for example
            like:  U2-14 or "U2"-"14"
            This is detectable by a non-space immediately preceeding the dash.
        */
        if( *cur == '-' && cur>start && !isSpace( cur[-1] ) )
        {
            curText = '-';
            curTok = DSN_DASH;
            head = cur+1;
            goto exit;
        }

        // switching the string_quote character
        if( prevTok == DSN_STRING_QUOTE )
        {
            static const wxString errtxt( _("String delimiter must be a single character of ', \", or $"));

            char cc = *cur;
            switch( cc )
            {
            case '\'':
            case '$':
            case '"':
                break;
            default:
                THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            curText = cc;

            head = cur+1;

            if( head<limit && !isSep( *head ) )
            {
                THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            curTok = DSN_QUOTE_DEF;
            goto exit;
        }

        // specctraMode DSN_STRING
        if( *cur == stringDelimiter )
        {
            ++cur;  // skip over the leading delimiter: ",', or $

            head = cur;

            while( head<limit  &&  !isStringTerminator( *head ) )
                ++head;

            if( head >= limit )
            {
                wxString errtxt( _( "Un-terminated delimited string" ) );
                THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            curText.clear();
            curText.append( cur, head );

            ++head;     // skip over the trailing delimiter

            curTok  = DSN_STRING;
            goto exit;
        }
    }           // specctraMode

    // non-quoted token, read it into curText.
    curText.clear();

    head = cur;
    while( head<limit && !isSep( *head ) )
        curText += *head++;

    if( isNumber( curText.c_str(), curText.c_str() + curText.size() ) )
    {
        curTok = DSN_NUMBER;
        goto exit;
    }

    if( specctraMode && curText == "string_quote" )
    {
        curTok = DSN_STRING_QUOTE;
        goto exit;
    }

    curTok = findToken( curText );

exit:   // single point of exit, no returns elsewhere please.

    curOffset = cur - start;

    next = head;

    // printf("tok:\"%s\"\n", curText.c_str() );
    return curTok;
}


wxArrayString* DSNLEXER::ReadCommentLines() throw( IO_ERROR )
{
    wxArrayString*  ret = 0;
    bool            cmt_setting = SetCommentsAreTokens( true );
    int             tok = NextTok();

    if( tok == DSN_COMMENT )
    {
        ret = new wxArrayString();

        do
        {
            ret->Add( FromUTF8() );
        }
        while( ( tok = NextTok() ) == DSN_COMMENT );
    }

    SetCommentsAreTokens( cmt_setting );

    return ret;
}
