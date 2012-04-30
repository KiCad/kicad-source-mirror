/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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


// This is wanting to be an s-expression C++ parser generator.  Feed it a sample
// file and maybe someday it will generate a C++ file which uses DSNLEXER to
// parse the described grammar OK.

// Until then, it is a non-specctra mode s-expression beautifier.


#include <assert.h>
#include <richio.h>
#include <dsnlexer.h>
#include <macros.h>
#include <boost/ptr_container/ptr_vector.hpp>


// http://sexpr.sourceforge.net/  see comments about graphviz
// http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx

#define D(x)   x
//#define D(x)


/**
 * Class ELEM
 */
class ELEM
{
protected:
    int             token;
    std::string     text;

    typedef boost::ptr_vector<ELEM> ELEMS;
    typedef ELEMS::const_iterator   ELEMS_CITER;
    typedef ELEMS::iterator         ELEMS_ITER;

    ELEMS           kids;      ///< ELEM pointers

public:

    // there are two constructors, one for a list, one for an atom

    /// List constructor
    ELEM( int aToken ) :
        token( aToken ),
        text( "" )
    {
        // D( printf( "ELEM%p: list\n", this ); )
    }

    /// Atom constructor
    ELEM( const std::string& aText, int aToken ) :
        token( aToken ),
        text( aText )
    {
        // D( printf( "ELEM%p: '%s'\n", this, text.c_str() ); )
    }

    int  Token() const { return token; }

    const char* Text() { return text.c_str(); }


    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel = 0, int aControlBits = 0 );

#define CTL_OMIT_NL         (1<<0)

    /**
     * Function Length
     * returns the number of ELEMs in this ELEM.
     * @return int - the count of children
     */
    int     Length() const
    {
        return kids.size();
    }

    void    Append( ELEM* aElem )
    {
        kids.push_back( aElem );
    }

    ELEM*   Replace( int aIndex, ELEM* aElem )
    {
        ELEMS::auto_type ret = kids.replace( aIndex, aElem );
        return ret.release();
    }

    ELEM*   Remove( int aIndex )
    {
        ELEMS::auto_type ret = kids.release( kids.begin()+aIndex );
        return ret.release();
    }

    void    Insert( int aIndex, ELEM* aElem )
    {
        kids.insert( kids.begin()+aIndex, aElem );
    }

    ELEM* At( int aIndex ) const
    {
        const ELEM&  ref = kids.at( aIndex );
        return (ELEM*) &ref;
    }

    ELEM* operator[]( int aIndex ) const
    {
        return At( aIndex );
    }

    void    Delete( int aIndex )
    {
        kids.erase( kids.begin()+aIndex );
    }
};


void ELEM::Format( OUTPUTFORMATTER* out, int nestLevel, int ctl )
{
    if( token == DSN_LEFT )     // this is a list
    {
        out->Print( nestLevel, "(" );

        const int count = Length();
        for( int i=0; i<count;  ++i )
        {
            ELEM* cur  = At( i );
            ELEM* next = i < count-1 ? At( i+1 ) : NULL;

            if( i > 0 )
                out->Print( 0, " " );

            if( next && next->token == DSN_LEFT )
            {
                cur->Format( out, nestLevel+1, 0 );
            }
            else
            {
                cur->Format( out, nestLevel+1, CTL_OMIT_NL );
            }
        }

        out->Print( 0, ")%s", ctl & CTL_OMIT_NL ? "" : "\n" );
    }
    else    // this is an atom
    {
        const char* s = out->Quotes( text ).c_str();
        out->Print( 0, "%s%s", s, ctl & CTL_OMIT_NL ? "" : "\n" );
    }
}

ELEM* Scan( DSNLEXER* lex );
ELEM* ScanList( DSNLEXER* lex );
ELEM* ScanAtom( DSNLEXER* lex );


void usage()
{
    fprintf( stderr, "Usage: parser_gen <grammar_s-expression_file>\n" );
    exit( 1 );
}


static KEYWORD empty_keywords[] = {};


ELEM* Scan( DSNLEXER* lex )
{
    ELEM*   elem = NULL;
    int     tok  = lex->CurTok();

    // conditionally read first token.
    if( tok == DSN_NONE )
        tok = lex->NextTok();

    if( tok == DSN_EOF )
    {
        lex->Unexpected( DSN_EOF );
    }

    if( tok == DSN_LEFT )
    {
        elem = ScanList( lex );
    }
    else
    {
        elem = ScanAtom( lex );
    }

    return elem;
}


/**
 * Function ScanList
 * reads and returns a sexpList from the input stream.
 */
ELEM* ScanList( DSNLEXER* lex )
{
    int     tok;
    ELEM*   list = NULL;

    assert( lex->CurTok() == DSN_LEFT );

    list = new ELEM( DSN_LEFT );

    while( ( tok = lex->NextTok() ) != DSN_RIGHT )
    {
        if( tok == DSN_EOF )
            lex->Unexpected( DSN_EOF );

        ELEM* elem = Scan( lex );
        list->Append( elem );
    }

    return list;
}


ELEM* ScanAtom( DSNLEXER* lex )
{
    return new ELEM( lex->CurText(), lex->CurTok() );
}


int main( int argc, char** argv )
{
    if( argc != 2 )
    {
        usage();
    }

    FILE*   fp = fopen( argv[1], "rt" );
    if( !fp )
    {
        fprintf( stderr, "Unable to open '%s'\n", argv[1] );
        usage();
    }

    DSNLEXER   lexer( empty_keywords, 0, fp, wxString( FROM_UTF8( argv[1] ) ) );

    try
    {
        ELEM* elem = Scan( &lexer );

        if( elem )
        {
            STRING_FORMATTER sf;

            elem->Format( &sf, 0 );

            printf( "%s", sf.GetString().c_str() );
        }
    }
    catch( IO_ERROR ioe )
    {
        fprintf( stderr, "%s\n", TO_UTF8( ioe.errorText ) );
    }
}

