
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

// Something in either <boost/property_tree/ptree.hpp> causes a bunch of compiler
// errors in <wx/msw/winundef.h> version 2.9 on MinGW.
#include <macros.h>

#include <boost/property_tree/ptree.hpp>

#include <cassert>
#include <ptree.h>

typedef PTREE::const_iterator           CITER;
typedef PTREE::iterator                 ITER;

#if defined( DEBUG )
#define D( x ) x
#else
#define D( x )
#endif

#define CTL_OMIT_NL ( 1 << 0 )
#define CTL_IN_ATTRS ( 1 << 1 )


//-----<Scan>------------------------------------------------------------------

/**
 * Read a sexpr list from the input stream into a new node with key aLexer->CurText().
 */
inline void scanList( PTREE* aTree, DSNLEXER* aLexer )
{
    assert( aLexer->CurTok() == DSN_LEFT );

    int tok = aLexer->NextTok();

    const char* key = aLexer->CurText();

    PTREE* list = &aTree->push_back( PTREE::value_type( key, PTREE() ) )->second;

    if( tok != DSN_RIGHT )
    {
        while( ( tok = aLexer->NextTok() ) != DSN_RIGHT )
        {
            if( tok == DSN_EOF )
                aLexer->Unexpected( DSN_EOF );

            Scan( list, aLexer );
        }
    }
}


inline void scanAtom( PTREE* aTree, const DSNLEXER* aLexer )
{
    const char* key = aLexer->CurText();

    aTree->push_back( PTREE::value_type( key, PTREE() ) );
}


void Scan( PTREE* aTree, DSNLEXER* aLexer )
{
    int tok  = aLexer->CurTok();

    // conditionally read first token.
    if( tok == DSN_NONE )
        tok = aLexer->NextTok();

    if( tok == DSN_EOF )
    {
        aLexer->Unexpected( DSN_EOF );
    }

    if( tok == DSN_LEFT )
    {
        scanList( aTree, aLexer );
    }
    else
    {
        scanAtom( aTree, aLexer );
    }
}


//-----<Format>------------------------------------------------------------------

inline bool isAtom( const CPTREE& aTree )
{
    return aTree.size() == 0 && aTree.data().size() == 0;
}


inline bool isLast( const CPTREE& aTree, CITER it )
{
    CITER next = it;
    ++next;
    return next == aTree.end();
}


inline CITER next( CITER it )
{
    CITER n = it;
    return ++n;
}


static void formatNode( OUTPUTFORMATTER* out, int aNestLevel, int aCtl,
        const std::string& aKey, const CPTREE& aTree );


static void formatList( OUTPUTFORMATTER* out, int aNestLevel, int aCtl, const CPTREE& aTree )
{
    for( CITER it = aTree.begin(); it != aTree.end(); ++it )
    {
        // Processing a tree which was read in with xml_parser?
        if( it->first == "<xmlattr>" )
        {
            formatList( out, aNestLevel, aCtl | CTL_IN_ATTRS, it->second );
            continue;
        }

        int ctl = 0;

        if( isLast( aTree, it ) )   // is "it" the last one?
        {
            //if( !( aCtl & CTL_IN_ATTRS ) )
                ctl = CTL_OMIT_NL;
        }
        else if( isAtom( next( it )->second ) )
        {
            /* if( !( aCtl & CTL_IN_ATTRS ) ) */
                ctl = CTL_OMIT_NL;
        }

        formatNode( out, aNestLevel+1, ctl, it->first, it->second );
    }
}


static void formatNode( OUTPUTFORMATTER* out, int aNestLevel, int aCtl,
        const std::string& aKey, const CPTREE& aTree )

{
    if( !isAtom( aTree ) )     // is a list, not an atom
    {
        int ctl = CTL_OMIT_NL;

        // aTree is list and its first child is a list
        if( aTree.size() && !isAtom( aTree.begin()->second ) && !aTree.data().size() )
            ctl = 0;

        out->Print( aNestLevel, "(%s%s", out->Quotes( aKey ).c_str(),
                    ctl & CTL_OMIT_NL ? "" : "\n" );

        if( aTree.data().size() )       // sexpr format does not use data()
        {
            out->Print( 0, " %s%s",
                        out->Quotes( aTree.data() ).c_str(),
                        aTree.size() ? "\n" : "" );
        }

        formatList( out, aNestLevel, aCtl, aTree );

        out->Print( 0, ")%s", aCtl & CTL_OMIT_NL ? "" : "\n" );
    }

    else            // is an atom, not a list
    {
        out->Print( 0, " %s", out->Quotes( aKey ).c_str() );
    }
}


void Format( OUTPUTFORMATTER* out, int aNestLevel, int aCtl, const CPTREE& aTree )
{
    if( aTree.size() == 1 && !aTree.data().size() )
    {
        // The topmost node is basically only a container for the document root.
        // It anchors the paths which traverse the tree deeper.
        CITER   it = aTree.begin();
        formatNode( out, aNestLevel, aCtl, it->first, it->second );
    }
    else
    {
        // This is not expected, neither for sexpr nor xml.
        formatNode( out, aNestLevel, aCtl, "", aTree );
    }
}

