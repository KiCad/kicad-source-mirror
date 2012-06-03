
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <cstdio>
#include <cstdlib>         // bsearch()
#include <cctype>


#include <dsnlexer.h>

//#include "fctsys.h"
//#include "pcbnew.h"

//#define STANDALONE  1       // enable this for stand alone testing.

static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;

    int ret = strcmp( k1->name, k2->name );
    return ret;
}


//-----<DSNLEXER>-------------------------------------------------------------

void DSNLEXER::init()
{
    curTok  = DSN_NONE;
    prevTok = DSN_NONE;

    stringDelimiter = '"';

    specctraMode = false;
    space_in_quoted_tokens = false;

    commentsAreTokens = false;
}


DSNLEXER::DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
                    FILE* aFile, const wxString& aFilename ) :
    iOwnReaders( true ),
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
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    STRING_LINE_READER* stringReader = new STRING_LINE_READER( aClipboardTxt, aSource.IsEmpty() ?
                                        wxString( _( "clipboard" ) ) : aSource );
    PushReader( stringReader );
    init();
}


DSNLEXER::DSNLEXER( const KEYWORD* aKeywordTable, unsigned aKeywordCount,
                    LINE_READER* aLineReader ) :
    iOwnReaders( false ),
    keywords( aKeywordTable ),
    keywordCount( aKeywordCount )
{
    if( aLineReader )
        PushReader( aLineReader );
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


int DSNLEXER::findToken( const std::string& tok )
{
    // convert to lower case once, this should be faster than using strcasecmp()
    // for each test in compare().
    lowercase.clear();

    for( std::string::const_iterator iter = tok.begin();  iter!=tok.end();  ++iter )
        lowercase += (char) tolower( *iter );

    KEYWORD search;

    search.name = lowercase.c_str();

    // a boost hashtable might be a few percent faster, depending on
    // hashtable size and quality of the hash function.

    const KEYWORD* findings = (const KEYWORD*) bsearch( &search,
                                   keywords, keywordCount,
                                   sizeof(KEYWORD), compare );
    if( findings )
        return findings->token;
    else
        return -1;
}


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
    wxString    errText( _("Expecting") );
    errText << wxT(" ") << GetTokenString( aTok );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Expecting( const char* text ) throw( IO_ERROR )
{
    wxString    errText( _("Expecting") );
    errText << wxT(" '") << wxString::FromUTF8( text ) << wxT("'");
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Unexpected( int aTok ) throw( IO_ERROR )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" ") << GetTokenString( aTok );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Duplicate( int aTok ) throw( IO_ERROR )
{
    wxString    errText;

    errText.Printf( _("%s is a duplicate"), GetTokenString( aTok ).GetData() );
    THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
}


void DSNLEXER::Unexpected( const char* text ) throw( IO_ERROR )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" '") << wxString::FromUTF8( text ) << wxT("'");
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
        wxString    errText;

        errText.Printf( _("need a NUMBER for '%s'"), wxString::FromUTF8( aExpectation ).GetData() );
        THROW_PARSE_ERROR( errText, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }
    return tok;
}


/**
 * Function isSpace
 * strips the upper bits of the int to ensure the value passed to C++ %isspace() is
 * in the range of 0-255
 */
static inline bool isSpace( int cc )
{
    // make sure int passed to ::isspace() is 0-255
    return ::isspace( cc & 0xff );
}


int DSNLEXER::NextTok() throw( IO_ERROR )
{
    const char*   cur  = next;
    const char*   head = cur;

    prevTok = curTok;

    if( curTok != DSN_EOF )
    {
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
            while( cur<limit && isSpace(*cur) )
                ++cur;

            // If the first non-blank character is #, this line is a comment.
            // Comments cannot follow any other token on the same line.
            if( cur<limit && *cur=='#' )
            {
                if( commentsAreTokens )
                {
                    // save the entire line, including new line as the current token.
                    // the '#' character may not be at offset zero.
                    curText = start;      // entire line is the token
                    cur     = start;      // ensure a good curOffset below
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
            while( cur<limit && isSpace(*cur) )
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

            if( head<limit && *head!=')' && *head!='(' && !isSpace(*head) )
            {
                THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            curTok = DSN_QUOTE_DEF;
            goto exit;
        }

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

        // handle DSN_NUMBER
        if( strchr( "+-.0123456789", *cur ) )
        {
            head = cur+1;
            while( head<limit && strchr( ".0123456789", *head )  )
                ++head;

            if( (head<limit && isSpace(*head)) || *head==')' || *head=='(' || head==limit )
            {
                curText.clear();
                curText.append( cur, head );
                curTok = DSN_NUMBER;
                goto exit;
            }

            // else it was something like +5V, fall through below
        }

        // a quoted string, will return DSN_STRING
        if( *cur == stringDelimiter )
        {
            // Non-specctraMode, understands and deciphers escaped \, \r, \n, and \".
            // Strips off leading and trailing double quotes
            if( !specctraMode )
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
                wxString errtxt(_("Un-terminated delimited string") );
                THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            else    // specctraMode DSN_STRING
            {
                ++cur;  // skip over the leading delimiter: ",', or $

                head = cur;

                while( head<limit  &&  !isStringTerminator( *head ) )
                    ++head;

                if( head >= limit )
                {
                    wxString errtxt(_("Un-terminated delimited string") );
                    THROW_PARSE_ERROR( errtxt, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
                }

                curText.clear();
                curText.append( cur, head );

                ++head;     // skip over the trailing delimiter

                curTok  = DSN_STRING;
                goto exit;
            }
        }

        // Maybe it is a token we will find in the token table.
        // If not, then call it a DSN_SYMBOL.
        {
            head = cur+1;
            while( head<limit && !isSpace( *head ) && *head!=')' && *head!='(' )
                ++head;

            curText.clear();
            curText.append( cur, head );

            int found = findToken( curText );

            if( found != -1 )
                curTok = found;

            else if( 0 == curText.compare( "string_quote" ) )
                curTok = DSN_STRING_QUOTE;

            else                    // unrecogized token, call it a symbol
                curTok = DSN_SYMBOL;
        }
    }

exit:   // single point of exit, no returns elsewhere please.

    curOffset = cur - start;

    next = head;

    // printf("tok:\"%s\"\n", curText.c_str() );
    return curTok;
}

