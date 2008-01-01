
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
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

#include <boost/ptr_container/ptr_vector.hpp>

#include <wx/ffile.h>
#include "fctsys.h"
#include "pcbstruct.h"

#include "dsn.h"

namespace DSN {


/**
 * Class ELEM
 * is a base class for any DSN element.  It is not a parent node so it 
 * cannot contain other elements but it can be extended to hold fields 
 * for any DSN element which contains no other elements, only fields.
class ELEM
{
protected:    
    DSN_T   type;

public:    
    
    ELEM( DSN_T aType ) :
       type( aType )
    {
    }
    
    virtual ~ELEM()
    {
        // printf("~ELEM(%p %d)\n", this, Type() );
    }
    
};
 */ 



/**
 * Class ELEM
 * is a holder for any DSN element.  It can contain other
 * elements, including elements derived from this class.
 */ 
class ELEM
{
protected:    
    DSN_T   type;

    //  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;
    
    ELEM_ARRAY                      kids;      ///< of pointers                   
   
    
public:

    ELEM( DSN_T aType ) :
       type( aType )
    {
    }
    
    virtual ~ELEM()
    {
        // printf("~ELEM(%p %d)\n", this, Type() );
    }

    DSN_T   Type() { return type; }
    
    //-----< list operations >--------------------------------------------

    /**
     * Function FindElem
     * finds a particular instance number of a given type of ELEM.
     * @param aType The type of ELEM to find
     * @param instanceNum The instance number of to find: 0 for first, 1 for second, etc.
     * @return int - The index into the kids array or -1 if not found.
     */
    int FindElem( DSN_T aType, int instanceNum )
    {
        int repeats=0;
        for( unsigned i=0;  i<kids.size();  ++i )
        {
            if( kids[i].Type() == aType )
            {
                if( repeats == instanceNum )
                    return i;
                ++repeats;
            }
        }
        return -1;
    }

    
    /**
     * Function Length
     * returns the number ELEMs in this ELEM.
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
        ELEM_ARRAY::auto_type ret = kids.replace( aIndex, aElem );
        return ret.release();
    }

    ELEM*   Remove( int aIndex )
    {
        ELEM_ARRAY::auto_type ret = kids.release( kids.begin()+aIndex );
        return ret.release();
    }

/*    
    ELEM& operator[]( int aIndex )
    {
        return kids[aIndex];
    }

    const ELEM& operator[]( int aIndex ) const
    {
        return kids[aIndex];
    }
*/    
    
    void    Insert( int aIndex, ELEM* aElem )
    {
        kids.insert( kids.begin()+aIndex, aElem );
    }
    
    ELEM* operator[]( int aIndex )
    {
        // we have varying sized object and are using polymorphism, so we
        // must return a pointer not a reference.
        return &kids[aIndex];
    }
    
    void    Delete( int aIndex )
    {
        kids.erase( kids.begin()+aIndex );
    }
};
   

/**
 * Class SPECCTRA_DB
 * holds a DSN data tree, usually coming from a DSN file.
 */
class SPECCTRA_DB
{
    LEXER*  lexer;
    
    ELEM*   tree;    

    FILE*   fp;
    
    
    /**
     * Function print
     * formats and writes text to the output stream.
     * @param fmt A printf style format string.
     * @param ... a variable list of parameters that will get blended into 
     *  the output under control of the format string.
     */
    void print( const char* fmt, ... );

    /**
     * Function nextTok
     * returns the next token from the lexer.
     */
    DSN_T   nextTok();

    void    expecting( DSN_T ) throw( IOError );
    void    expecting( const wxChar* text ) throw( IOError );
    
    void doPCB( ELEM* growth ) throw(IOError);
    void doPARSER( ELEM* growth ) throw(IOError);
    void doRESOLUTION( ELEM* growth ) throw(IOError);
    
    
public:

    SPECCTRA_DB()
    {
        lexer = 0;
        tree  = 0;
        fp    = 0;
    }

    ~SPECCTRA_DB()
    {
        delete lexer;
        delete tree;
        
        if( fp )
            fclose( fp );
    }

    
    /**
     * Function Load
     * is a recursive descent parser for a DSN file.
     * @param filename The name of the dsn file to load.
     * @throw IOError if there is a lexer or parser error. 
     */
    void Load( const wxString& filename ) throw( IOError );

    void ThrowIOError( const wxChar* fmt, ... ) throw( IOError );
    
    
    /**
     * Function Export
     * writes the given BOARD out as a SPECTRA DSN format file.
     * @param aBoard The BOARD to save.
     */
    void Export( BOARD* aBoard );
};


void SPECCTRA_DB::ThrowIOError( const wxChar* fmt, ... ) throw( IOError )
{
    wxString    errText;
    va_list     args;

    va_start( args, fmt );
    errText.PrintfV( fmt, args );
    va_end( args );
    
    throw IOError( errText );
}


void SPECCTRA_DB::expecting( DSN_T aTok ) throw( IOError )
{
    wxString    errText( _("Expecting") );
    
    errText << wxT(" ") << lexer->GetTokenText( aTok );
    
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}

void SPECCTRA_DB::expecting( const wxChar* text ) throw( IOError )
{
    wxString    errText( _("Expecting") );
    
    errText << wxT(" '") << text << wxT("'");
    
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}


DSN_T SPECCTRA_DB::nextTok()
{
    return lexer->NextTok();
}


void SPECCTRA_DB::Load( const wxString& filename ) throw( IOError )
{
    wxFFile     file( filename.c_str() );
    
    if( !file.IsOpened() )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }
    
    delete lexer;  
    lexer = 0;
    
    lexer = new LEXER( file.fp(), filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_pcb )
        expecting( T_pcb );

    delete tree;
    tree = 0;
    
    tree = new ELEM( T_pcb );
    
    doPCB( tree );
}


void SPECCTRA_DB::doPCB( ELEM* growth ) throw( IOError )
{
    ELEM* child;
    DSN_T   tok = nextTok();
    
    switch( tok )
    {
    case T_SYMBOL:
    case T_STRING:
        break;
        
    default:
        expecting( T_STRING );
    }

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_parser:
            child = new ELEM( T_parser );
            growth->Append( child );
            doPARSER( child );
            break;
            
        case T_unit:
            child = new ELEM( T_unit );
            growth->Append( child );
            break;
            
        case T_resolution:
            child = new ELEM( T_resolution );
            growth->Append( child );
            doRESOLUTION( child );
            break;
            
        case T_structure:
        case T_placement:
        case T_library:
            break;
            
        default:
            expecting( wxT("parser | unit | resolution | structure | placement | library") );
        }
    }
    
    tok = nextTok();
    if( tok != T_EOF )
        expecting( T_EOF );
}


void SPECCTRA_DB::doPARSER( ELEM* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_string_quote:
            tok = nextTok();
            if( tok != T_QUOTE_DEF )
                expecting( T_QUOTE_DEF );
            lexer->SetStringDelimiter( (unsigned char) *lexer->CurText() );
            break;
            
        case T_space_in_quoted_tokens:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( _("on or off") );
            lexer->SetSpaceInQuotedTokens( tok==T_on );
            break;
            
        case T_host_cad:
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            // @todo
            break;
            
        case T_host_version:
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            // @todo
            break;

        case T_constant:
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            // @todo
            break;

        case T_write_resolution:   // [(writee_resolution {<character> <positive_integer >})]
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( tok!=T_SYMBOL )
                    expecting( T_SYMBOL );
                tok = nextTok();
                if( tok!=T_NUMBER )
                    expecting( T_NUMBER );
                // @todo
            }
            continue;   // we ate the T_RIGHT

        case T_routes_include:  // [(routes_include {[testpoint | guides | image_conductor]})]
            while( (tok = nextTok()) != T_RIGHT )
            {
                switch( tok )
                {
                case T_testpoint:
                case T_guide:
                case T_image_conductor:
                    // @todo
                    break;
                default:
                    expecting( _("testpoint, guides, or image_conductor") );
                }
            }
            continue;   // we ate the T_RIGHT

        case T_wires_include:   // [(wires_include testpoint)]
            tok = nextTok();
            if( tok != T_testpoint )
                expecting( T_testpoint );
            // @todo
            break;
            
        case T_case_sensitive:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( _("on or off") );
            // @todo
            break;

        case T_via_rotate_first:    // [(via_rotate_first [on | off])]
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( _("on or off") );
            // @todo
            break;
            
            
        default:
            expecting( wxT("parser_descriptor contents") );
        }
        
        tok = nextTok();
        if( tok != T_RIGHT )
            expecting( T_RIGHT );
    }
    
}

void SPECCTRA_DB::doRESOLUTION( ELEM* growth ) throw(IOError)
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_inch:
    case T_mil:
    case T_cm:
    case T_mm:
    case T_um:
        // @todo
        break;
    default:
        expecting( wxT("inch, mil, cm, mm, or um") );
    }
    
    tok = nextTok();
    if( tok != T_NUMBER )
        expecting( T_NUMBER );
    
    tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::print( const char* fmt, ... )
{
    va_list     args;

    va_start( args, fmt );
    vfprintf( fp, fmt, args );
    va_end( args );
}



void SPECCTRA_DB::Export( BOARD* aBoard )
{
}


} // namespace DSN


using namespace DSN;

// a test to verify some of the list management functions.

int main( int argc, char** argv )
{
#if 0    
    ELEM parent( T_pcb );

    ELEM* child = new ELEM( T_absolute );
    
    parent.Append( child );

    parent[0]->Test();
    
    child->Append( new ELEM( T_absolute ) );
#else

    wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
//    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );

    SPECCTRA_DB     db;
    
    try 
    {
        db.Load( filename );
    } 
    catch( IOError ioe )
    {
        printf( "%s\n", CONV_TO_UTF8(ioe.errorText) );
        exit(1);
    }
    
    printf("loaded OK\n");
    
#endif
    
}


//EOF
