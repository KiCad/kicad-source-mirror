
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

#include <boost/ptr_container/ptr_vector.hpp>

#include <wx/ffile.h>
#include "fctsys.h"
#include "pcbstruct.h"
#include "dsn.h"



#define EDA_BASE            // build_version.h behavior
#undef  COMMON_GLOBL
#define COMMON_GLOBL        // build_version.h behavior
#include "build_version.h"


namespace DSN {

    
class SPECCTRA_DB;
class PCB;


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
 * Class OUTPUTFORMATTER
 * is an interface (abstract class) used to output ASCII text.
 */
class OUTPUTFORMATTER
{
public:

    /**
     * Function print
     * formats and writes text to the output stream.
     *
     * @param nestLevel The multiple of spaces to preceed the output with. 
     * @param fmt A printf style format string.
     * @param ... a variable list of parameters that will get blended into 
     *  the output under control of the format string.
     * @throw IOError if there is a problem outputting, such as a full disk.
     */
    virtual void Print( int nestLevel, const char* fmt, ... ) throw( IOError ) = 0;
};
 
 

/**
 * Class ELEM
 * is a holder for any DSN element.  It can contain other
 * elements, including elements derived from this class.
 */ 
class ELEM
{
protected:    
    DSN_T           type;

    ELEM*           parent;
    
    //  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;
    
    ELEM_ARRAY      kids;      ///< ELEM pointers                   

public:

    ELEM( DSN_T aType, ELEM* aParent = 0 );
    
    virtual ~ELEM();
    
    DSN_T   Type() { return type; }

    
    /**
     * Function GetUnits
     * returns the units for this section.  Derived classes may override this
     * to check for section specific overrides.
     * @return DSN_T - one of the allowed values to <unit_descriptor>
     */
    virtual DSN_T   GetUnits()
    {
        if( parent )
            return parent->GetUnits();
        
        return T_inch;
    }

    
    /**
     * Function Save
     * writes this object out in ASCII form according to the SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    virtual void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );

    
    //-----< list operations >--------------------------------------------

    /**
     * Function FindElem
     * finds a particular instance number of a given type of ELEM.
     * @param aType The type of ELEM to find
     * @param instanceNum The instance number of to find: 0 for first, 1 for second, etc.
     * @return int - The index into the kids array or -1 if not found.
     */
    int FindElem( DSN_T aType, int instanceNum = 0 );

    
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
        ELEM_ARRAY::auto_type ret = kids.replace( aIndex, aElem );
        return ret.release();
    }

    ELEM*   Remove( int aIndex )
    {
        ELEM_ARRAY::auto_type ret = kids.release( kids.begin()+aIndex );
        return ret.release();
    }

    void    Insert( int aIndex, ELEM* aElem )
    {
        kids.insert( kids.begin()+aIndex, aElem );
    }
    
    ELEM*   At( int aIndex )
    {
        // we have varying sized objects and are using polymorphism, so we
        // must return a pointer not a reference.
        return &kids[aIndex];
    }
    
    ELEM* operator[]( int aIndex )
    {
        return At( aIndex );
    }
    
    void    Delete( int aIndex )
    {
        kids.erase( kids.begin()+aIndex );
    }
};



ELEM::ELEM( DSN_T aType, ELEM* aParent ) :
   type( aType ),
   parent( aParent )
{
}


ELEM::~ELEM()
{
}


void ELEM::Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) ); 
    
    for( int i=0;  i<Length();  ++i )
    {
        At(i)->Save( out, nestLevel+1 );
    }
    
    out->Print( nestLevel, ")\n" ); 
}


int ELEM::FindElem( DSN_T aType, int instanceNum )
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
 * Class PARSER
 * is simply a configuration record per the SPECCTRA DSN file spec.
 * It is not actually a parser.
 */
class PARSER : public ELEM
{
    friend class SPECCTRA_DB;

    char        string_quote;
    bool        space_in_quoted_tokens;
    bool        case_sensitive;
    bool        wires_include_testpoint;
    bool        routes_include_testpoint;
    bool        routes_include_guides;
    bool        routes_include_image_conductor;
    bool        via_rotate_first;
    
    std::string const_id1;
    std::string const_id2;
    
    std::string host_cad;
    std::string host_version;

    
public:

    PARSER( ELEM* aParent ) :
        ELEM( T_parser, aParent )
    {
        string_quote = '"';
        space_in_quoted_tokens = false;

        case_sensitive = false;
        wires_include_testpoint = false;
        routes_include_testpoint = false;
        routes_include_guides = false;
        routes_include_image_conductor = false;
        via_rotate_first = true;
        
        host_cad = "Kicad's PCBNEW";
        host_version = CONV_TO_UTF8(g_BuildVersion);
    }

    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) ); 
        out->Print( nestLevel+1, "(string_quote %c)\n", string_quote );
        out->Print( nestLevel+1, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
        out->Print( nestLevel+1, "(host_cad \"%s\")\n", host_cad.c_str() ); 
        out->Print( nestLevel+1, "(host_version \"%s\")\n", host_version.c_str() );
        out->Print( nestLevel+1, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
        out->Print( nestLevel, ")\n" ); 
    }
    
};


class RESOLUTION : public ELEM
{
    friend class SPECCTRA_DB;
    
    DSN_T       units;
    int         value;

public:
    RESOLUTION( ELEM* aParent ) :
        ELEM( T_resolution, aParent )
    {
        units = T_inch;
        value = 2540000;
    }

    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s %s %d)\n", LEXER::GetTokenText( Type() ), 
                   LEXER::GetTokenText(units), value ); 
    }
    
    DSN_T   GetUnits()
    {
        return units;
    }
};


class UNIT : public ELEM
{
    friend class SPECCTRA_DB;
    
    DSN_T       units;

public:
    UNIT( ELEM* aParent ) :
        ELEM( T_unit, aParent )
    {
        units = T_inch;
    }
        
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s %s)\n", LEXER::GetTokenText( Type() ), 
                   LEXER::GetTokenText(units) ); 
    }
    
    DSN_T   GetUnits()
    {
        return units;
    }
};


class PCB : public ELEM
{
    friend class SPECCTRA_DB;
    
    PARSER*         parser;
    RESOLUTION*     resolution;
    UNIT*           unit;
    
public:
    
    PCB() :
        ELEM( T_pcb )
    {
        parser = 0;
        resolution = 0;
        unit = 0;
    }
    
    ~PCB()
    {
        delete parser;
        delete resolution;
        delete unit;
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );
        
        if( parser )
            parser->Save( out, nestLevel+1 );
        
        if( resolution )
            resolution->Save( out, nestLevel+1 );

        if( unit )
            unit->Save( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" ); 
    }
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        if( resolution )
            return resolution->GetUnits();
        
        return ELEM::GetUnits();
    }
    
};



/**
 * Class SPECCTRA_DB
 * holds a DSN data tree, usually coming from a DSN file.
 */
class SPECCTRA_DB : public OUTPUTFORMATTER
{
    LEXER*      lexer;
    
    PCB*        tree;    

    FILE*       fp;

    wxString    filename;
    
    
    /**
     * Function nextTok
     * returns the next token from the lexer.
     */
    DSN_T   nextTok();

    void    expecting( DSN_T ) throw( IOError );
    void    expecting( const wxChar* text ) throw( IOError );
    
    void doPCB( PCB* growth ) throw(IOError);
    void doPARSER( PARSER* growth ) throw(IOError);
    void doRESOLUTION( RESOLUTION* growth ) throw(IOError);
    void doUNIT( UNIT* growth ) throw( IOError );    
    
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

    
    //-----<OUTPUTFORMATTER>-------------------------------------------------
    void Print( int nestLevel, const char* fmt, ... ) throw( IOError );
    //-----</OUTPUTFORMATTER>------------------------------------------------

    /**
     * Function MakePCB
     * makes PCB with all the default ELEMs and parts on the heap.
     */
    static PCB* MakePCB();

    
    /**
     * Function SetPCB
     * deletes any existing PCB and replaces it with the given one.
     */
    void SetPCB( const PCB* aPcb )
    {
        delete tree;
        tree = (PCB*) aPcb;
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
    void Export( wxString, BOARD* aBoard );
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
    
    errText << wxT(" ") << LEXER::GetTokenString( aTok );
    
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
    wxFFile     file;
    
    FILE*       fp = wxFopen( filename, wxT("r") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    file.Attach( fp );      // "exception safe" way to close the file.
    
    delete lexer;  
    lexer = 0;
    
    lexer = new LEXER( file.fp(), filename );

    if( nextTok() != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_pcb )
        expecting( T_pcb );

    SetPCB( new PCB() );
    
    doPCB( tree );
}


void SPECCTRA_DB::doPCB( PCB* growth ) throw( IOError )
{
    DSN_T tok = nextTok();
    
    if( tok!=T_SYMBOL && tok!=T_STRING )
        expecting( T_SYMBOL );
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_parser:
            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;
            
        case T_unit:
            growth->unit = new UNIT( growth );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            growth->resolution = new RESOLUTION( growth );
            doRESOLUTION( growth->resolution );
            break;
            
        case T_structure:
        case T_placement:
        case T_library:
            break;
            
        default:
            expecting( wxT("parser|unit|resolution|structure|placement|library") );
        }
    }
    
    tok = nextTok();
    if( tok != T_EOF )
        expecting( T_EOF );
}


void SPECCTRA_DB::doPARSER( PARSER* growth ) throw( IOError )
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
                expecting( wxT("on|off") );
            lexer->SetSpaceInQuotedTokens( tok==T_on );
            growth->space_in_quoted_tokens = (tok==T_on);
            break;
            
        case T_host_cad:
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            growth->host_cad = lexer->CurText();
            break;
            
        case T_host_version:
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            growth->host_version = lexer->CurText();
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
                expecting( wxT("on|off") );
            growth->case_sensitive = (tok==T_on);
            break;

        case T_via_rotate_first:    // [(via_rotate_first [on | off])]
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( wxT("on|off") );
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

void SPECCTRA_DB::doRESOLUTION( RESOLUTION* growth ) throw(IOError)
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_inch:
    case T_mil:
    case T_cm:
    case T_mm:
    case T_um:
        growth->units = tok;
        break;
    default:
        expecting( wxT("inch | mil | cm | mm | um") );
    }
    
    tok = nextTok();
    if( tok != T_NUMBER )
        expecting( T_NUMBER );

    growth->value = atoi( lexer->CurText() );
    
    tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}

void SPECCTRA_DB::doUNIT( UNIT* growth ) throw(IOError)
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_inch:
    case T_mil:
    case T_cm:
    case T_mm:
    case T_um:
        growth->units = tok;
        break;
    default:
        expecting( wxT("inch | mil | cm | mm | um") );
    }
    
    tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{
    va_list     args;

    va_start( args, fmt );
    
    int ret = 0;
    
    for( int i=0; i<nestLevel;  ++i )
    {
        ret = fprintf( fp, "  " );
        if( ret < 0 )
            break;
    }
    
    if( ret<0 || vfprintf( fp, fmt, args )<0 )
        ThrowIOError( _("System file error writing to file \"%s\""), filename.GetData() );
    
    va_end( args );
}


void SPECCTRA_DB::Export( wxString filename, BOARD* aBoard )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    tree->Save( this, 0 );    
}


PCB* SPECCTRA_DB::MakePCB()
{
    PCB*    pcb = new PCB();
    
    pcb->parser = new PARSER( pcb );

    return pcb;
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
    bool            failed = false;
    
    try 
    {
        db.Load( filename );
    } 
    catch( IOError ioe )
    {
        printf( "%s\n", CONV_TO_UTF8(ioe.errorText) );
        failed = true;
    }

    if( !failed )    
        printf("loaded OK\n");

//    db.SetPCB( SPECCTRA_DB::MakePCB() );
    
    db.Export( wxT("/tmp/export.dsn"), 0 );
    
#endif
    
}


//EOF
