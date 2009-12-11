
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <cstdio>
#include <cstdlib>         // bsearch()
#include <cctype>


#include "dsnlexer.h"

#include "fctsys.h"
#include "pcbnew.h"


static int compare( const void* a1, const void* a2 )
{
    const KEYWORD* k1 = (const KEYWORD*) a1;
    const KEYWORD* k2 = (const KEYWORD*) a2;

    int ret = strcmp( k1->name, k2->name );
    return ret;
}


//-----<DSNLEXER>-------------------------------------------------------------

DSNLEXER::DSNLEXER( FILE* aFile, const wxString& aFilename,
    const KEYWORD* aKeywordTable, unsigned aKeywordCount ) :
        reader( aFile, 4096 )

{
    keywords = aKeywordTable;
    keywordCount = aKeywordCount;

    curTok = DSN_NONE;
    stringDelimiter = '"';
    filename = aFilename;

    space_in_quoted_tokens = true;

    // "start" should never change until we change the reader.  The DSN
    // format spec supports an include file mechanism but we can add that later
    // using a std::stack to hold a stack of LINE_READERs to track nesting.
    start = (char*) reader;

    limit = start;
    next  = start;
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
        ret = "end of file";
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


void DSNLEXER::ThrowIOError( wxString aText, int charOffset ) throw (IOError)
{
    // append to aText, do not overwrite
    aText << wxT(" ") << _("in file") << wxT(" \"") << filename
          << wxT("\" ") << _("on line") << wxT(" ") << reader.LineNumber()
          << wxT(" ") << _("at offset") << wxT(" ") << charOffset;

    throw IOError( aText );
}


/**
 * Function isspace
 * strips the upper bits of the int to ensure the value passed to ::isspace() is
 * in the range of 0-255
 */
static inline bool isSpace( int cc )
{
    // make sure int passed to ::isspace() is 0-255
    return ::isspace( cc & 0xff );
}


int DSNLEXER::NextTok() throw (IOError)
{
    char*   cur  = next;
    char*   head = cur;

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
                curTok = DSN_EOF;
                goto exit;
            }

            cur = start;

            // skip leading whitespace
            while( cur<limit && isSpace(*cur) )
                ++cur;

            // if the first non-blank character is #, this line is a comment.
            if( cur<limit && *cur=='#' )
                goto L_read;
        }
        else
        {
            // skip leading whitespace
            while( cur<limit && isSpace(*cur) )
                ++cur;
        }

        if( cur >= limit )
            goto L_read;

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
                ThrowIOError( errtxt, CurOffset() );
            }

            curText = cc;

            head = cur+1;

            if( head<limit && *head!=')' && *head!='(' && !isSpace(*head) )
            {
                ThrowIOError( errtxt, CurOffset() );
            }

            curTok = DSN_QUOTE_DEF;
            goto exit;
        }

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

        // a quoted string
        if( *cur == stringDelimiter )
        {
            ++cur;  // skip over the leading delimiter: ",', or $

            head = cur;

            while( head<limit &&  !isStringTerminator( *head ) )
                ++head;

            if( head >= limit )
            {
                wxString errtxt(_("Un-terminated delimited string") );
                ThrowIOError( errtxt, CurOffset() );
            }

            curText.clear();
            curText.append( cur, head );

            ++head;     // skip over the trailing delimiter

            curTok  = DSN_STRING;
            goto exit;
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



#if 0 && defined(STANDALONE)

// stand alone testing


int main( int argc, char** argv )
{

//  wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );

    FILE*   fp = wxFopen( filename, wxT("r") );

    if( !fp )
    {
        fprintf( stderr, "unable to open file \"%s\"\n",
                (const char*) filename.mb_str() );
        exit(1);
    }

    // this won't compile without a token table.
    DSNLEXER  lexer( fp, filename );

    try
    {
        int tok;
        while( (tok = lexer.NextTok()) != DSN_EOF )
        {
            printf( "%-3d %s\n", tok, lexer.CurText() );
        }
    }
    catch( IOError ioe )
    {
        printf( "%s\n", (const char*) ioe.errorText.mb_str() );
    }

    fclose( fp );

    return 0;
}

#endif
