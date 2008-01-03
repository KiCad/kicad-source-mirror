
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

    /**
     * Function GetQuoteChar
     * returns the quote character as a single character string for a given 
     * input wrapee string.  Often the return value is "" the null string if
     * there are no delimiters in the input string.  If you want the quote_char
     * to be assuredly not "", then pass in "(" as the wrappee.
     * @param wrapee A string might need wrapping on each end.
     * @return const char* - the quote_char as a single character string, or ""
     *   if the wrapee does not need to be wrapped.
     */
    virtual const char* GetQuoteChar( const char* wrapee ) = 0;
};
 

struct POINT
{
    float  x;
    float  y;
    
    POINT() { x=0.0; y=0.0; }
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

    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
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


class RECTANGLE : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     layer_id;

    POINT           point0;
    POINT           point1;
    
public:

    RECTANGLE( ELEM* aParent ) :
        ELEM( T_rect, aParent )
    {
    }
    
    ~RECTANGLE()
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char*  quote = out->GetQuoteChar( layer_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s %f %f %f %f)\n", 
                   LEXER::GetTokenText( Type() ),
                   quote, layer_id.c_str(), quote,
                   point0.x, point0.y,
                   point1.x, point1.y );
    }
};


class PATH : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     layer_id;
    double          aperture_width;

    typedef std::vector<POINT> POINTS;
    
    POINTS          points;                   
    
    DSN_T           aperture_type;
    
public:

    PATH( ELEM* aParent ) :
        ELEM( T_path, aParent )
    {
        aperture_width = 0.0;
        aperture_type  = T_round;
    }
    
    ~PATH()
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s %f\n", LEXER::GetTokenText( Type() ),
                                 quote, layer_id.c_str(), quote, 
                                 aperture_width );

        for( unsigned i=0; i<points.size();  ++i )
        {
            out->Print( nestLevel+1, "%f %f\n", points[i].x, points[i].y );
        }
        
        if( aperture_type == T_square )
            out->Print( nestLevel+1, "(aperture_type square)\n" );

        out->Print( nestLevel, ")\n" ); 
    }
};


class BOUNDARY : public ELEM
{
    friend class SPECCTRA_DB;
 
    //  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
    typedef boost::ptr_vector<PATH> PATHS;
    
    PATHS           paths;
    RECTANGLE*      rectangle;
    
public:

    BOUNDARY( ELEM* aParent, DSN_T aType = T_boundary ) :
        ELEM( aType, aParent )
    {
        rectangle = 0;
    }
    
    ~BOUNDARY()
    {
        delete rectangle;
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        if( rectangle )
            rectangle->Save( out, nestLevel+1 );
        else
        {
            for( unsigned i=0; i<paths.size();  ++i )
            {
                paths[i].Save( out, nestLevel+1 );
            }
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class VIA : public ELEM
{
    friend class SPECCTRA_DB;

    typedef std::vector<std::string>    STRINGS;
    
    STRINGS     padstacks;
    STRINGS     spares;

public:

    VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );
        
        for( STRINGS::const_iterator i=padstacks.begin();
            i!=padstacks.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            
            out->Print( nestLevel+1, "%s%s%s\n", quote, i->c_str(), quote );
        }
        
        if( spares.size() )
        {
            out->Print( nestLevel+1, "(spare\n" );

            for( STRINGS::const_iterator i=spares.begin();
                i!=spares.end();  ++i )
            {
                const char* quote = out->GetQuoteChar( i->c_str() );
                
                out->Print( nestLevel+2, "%s%s%s\n", quote, i->c_str(), quote );
            }
            
            out->Print( nestLevel+1, ")\n" );
        }
        
        out->Print( nestLevel, ")\n" );
    }
};


class CONTROL : public ELEM
{
    friend class SPECCTRA_DB;

    bool    via_at_smd;
    bool    via_at_smd_grid_on;

    
public:

    CONTROL( ELEM* aParent ) :
        ELEM( T_control, aParent )
    {
        via_at_smd = false;
        via_at_smd_grid_on = false;
    }
    
    ~CONTROL()
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        //if( via_at_smd )
        {
            out->Print( nestLevel+1, "(via_at_smd %s", via_at_smd ? "on" : "off" );
            if( via_at_smd_grid_on )
                out->Print( 0, " grid %s", via_at_smd_grid_on ? "on" : "off" );
            out->Print( 0, ")\n" );
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class STRUCTURE : public ELEM
{
    friend class SPECCTRA_DB;
    
    UNIT*       unit;
    BOUNDARY*   boundary;
    BOUNDARY*   place_boundary;
    VIA*        via;
    CONTROL*    control;
    
public:

    STRUCTURE( ELEM* aParent ) :
        ELEM( T_structure, aParent )
    {
        unit = 0;
        boundary = 0;
        place_boundary = 0;
        via = 0;
        control = 0;
    }
    
    ~STRUCTURE()
    {
        delete unit;
        delete boundary;
        delete place_boundary;
        delete via;
        delete control;
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );
        
        if( unit )
            unit->Save( out, nestLevel+1 );

        if( boundary )
            boundary->Save( out, nestLevel+1 );

        if( place_boundary )
            place_boundary->Save( out, nestLevel+1 );

        if( via )
            via->Save( out, nestLevel+1 );
        
        if( control )
            control->Save( out, nestLevel+1 );
        
        for( int i=0; i<Length();  ++i )
        {
            At(i)->Save( out, nestLevel+1 );
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


/**
 * Class BOOLPROP
 * is a container for a single property whose value is a boolean (on|off).
 * The name of the property is obtained from the DSN_T.
 */
class BOOLPROP : public ELEM
{
    friend class SPECCTRA_DB;

    bool    value;

public:

    BOOLPROP( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s %s)\n", LEXER::GetTokenText( Type() ),
                   value ? "on" : "off" );
    }
};


/**
 * Class STRINGPROP
 * is a container for a single property whose value is a string.
 * The name of the property is obtained from the DSN_T.
 */
class STRINGPROP : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     value;

public:

    STRINGPROP( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote_char = out->GetQuoteChar( value.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s)\n", LEXER::GetTokenText( Type() ),
                                quote_char, value.c_str(), quote_char );
    }
};


class PCB : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     pcbname;    
    PARSER*         parser;
    RESOLUTION*     resolution;
    UNIT*           unit;
    STRUCTURE*      structure;
    
public:
    
    PCB() :
        ELEM( T_pcb )
    {
        parser = 0;
        resolution = 0;
        unit = 0;
        structure = 0;
    }
    
    ~PCB()
    {
        delete parser;
        delete resolution;
        delete unit;
        delete structure;
    }
    
    void Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( pcbname.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, pcbname.c_str(), quote );
        
        if( parser )
            parser->Save( out, nestLevel+1 );
        
        if( resolution )
            resolution->Save( out, nestLevel+1 );

        if( unit )
            unit->Save( out, nestLevel+1 );

        if( structure )
            structure->Save( out, nestLevel+1 );
        
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
    
    std::string quote_char;
    
    
    /**
     * Function nextTok
     * returns the next token from the lexer.
     */
    DSN_T   nextTok();

    
    static bool isSymbol( DSN_T aTok )
    {
        // if aTok is >= 0, then it might be a coincidental match to a keyword.
        return aTok==T_SYMBOL || aTok==T_STRING || aTok>=0;
    }

    
    /**
     * Function expecting
     * throws an IOError exception with an input file specific error message.
     * @param DSN_T The token type which was expected at the current input location.
     * @throw IOError with the location within the input file of the problem.
     */
    void    expecting( DSN_T ) throw( IOError );
    void    expecting( const wxChar* text ) throw( IOError );
    void    unexpected( DSN_T aTok ) throw( IOError );
    void    unexpected( const char* text ) throw( IOError );
    
    void doPCB( PCB* growth ) throw(IOError);
    void doPARSER( PARSER* growth ) throw(IOError);
    void doRESOLUTION( RESOLUTION* growth ) throw(IOError);
    void doUNIT( UNIT* growth ) throw( IOError );    
    void doSTRUCTURE( STRUCTURE* growth ) throw( IOError );    
    void doBOUNDARY( BOUNDARY* growth ) throw( IOError );
    void doRECTANGLE( RECTANGLE* growth ) throw( IOError );
    void doPATH( PATH* growth ) throw( IOError );
    void doSTRINGPROP( STRINGPROP* growth ) throw( IOError );
    void doBOOLPROP( STRINGPROP* growth ) throw( IOError );
    void doVIA( VIA* growth ) throw( IOError );
    void doCONTROL( CONTROL* growth ) throw( IOError );    

    
public:

    SPECCTRA_DB()
    {
        lexer = 0;
        tree  = 0;
        fp    = 0;
        quote_char += '"';
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
    const char* GetQuoteChar( const char* wrapee ); 
    //-----</OUTPUTFORMATTER>------------------------------------------------

    /**
     * Function MakePCB
     * makes a PCB with all the default ELEMs and parts on the heap.
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



//-----<SPECCTRA_DB>-------------------------------------------------

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

void SPECCTRA_DB::unexpected( DSN_T aTok ) throw( IOError )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" ") << LEXER::GetTokenString( aTok );
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}

void SPECCTRA_DB::unexpected( const char* text ) throw( IOError )
{
    wxString    errText( _("Unexpected") );
    errText << wxT(" '") << CONV_FROM_UTF8(text) << wxT("'");
    lexer->ThrowIOError( errText, lexer->CurOffset() ); 
}


DSN_T SPECCTRA_DB::nextTok()
{
    DSN_T ret = lexer->NextTok();
    return ret;
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
    
    if( !isSymbol(tok) )
        expecting( T_SYMBOL );

    growth->pcbname = lexer->CurText();    
    
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
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;
            
        case T_placement:
        case T_library:
            break;
            
        default:
            unexpected( lexer->CurText() );
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
            growth->string_quote = *lexer->CurText();
            quote_char = lexer->CurText(); 
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
            growth->const_id1 = lexer->CurText();
            tok = nextTok();
            if( tok!=T_STRING && tok!=T_SYMBOL )
                expecting( T_SYMBOL );
            growth->const_id2 = lexer->CurText();
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
                    growth->routes_include_testpoint = true;
                    break;
                case T_guide:
                    growth->routes_include_guides = true;
                    break;
                case T_image_conductor:
                    growth->routes_include_image_conductor = true;
                    break;
                default:
                    expecting( wxT("testpoint|guides|image_conductor") );
                }
            }
            continue;   // we ate the T_RIGHT

        case T_wires_include:   // [(wires_include testpoint)]
            tok = nextTok();
            if( tok != T_testpoint )
                expecting( T_testpoint );
            growth->routes_include_testpoint = true;
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
            growth->via_rotate_first = (tok==T_on);
            break;
            
        default:
            unexpected( lexer->CurText() );
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

void SPECCTRA_DB::doSTRUCTURE( STRUCTURE* growth ) throw(IOError)
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( T_unit );
            growth->unit = new UNIT( growth );
            doUNIT( growth->unit );
            break;

        case T_place_boundary:
L_place:            
            if( growth->place_boundary )
                unexpected( T_place_boundary );
            growth->place_boundary = new BOUNDARY( growth, T_place_boundary );
            doBOUNDARY( growth->place_boundary );
            break;
            
        case T_boundary:
            if( growth->boundary )
            {
                if( growth->place_boundary )
                    unexpected( T_boundary );
                goto L_place;
            }
            growth->boundary = new BOUNDARY( growth );
            doBOUNDARY( growth->boundary );
            break;

        case T_snap_angle:
            STRINGPROP* stringprop;
            growth->Append( stringprop = new STRINGPROP( growth, T_snap_angle ) );
            doSTRINGPROP( stringprop );
            break;

        case T_via:
            growth->via = new VIA( growth );
            doVIA( growth->via );
            break;
            
        case T_control:
            growth->control = new CONTROL( growth );
            doCONTROL( growth->control );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doBOUNDARY( BOUNDARY* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok != T_LEFT )
        expecting( T_LEFT );
        
    tok = nextTok();
    if( tok == T_rect )
    {
        growth->rectangle = new RECTANGLE( growth );
        doRECTANGLE( growth->rectangle );
        if( nextTok() != T_RIGHT )
            expecting( T_RIGHT );
    }
    else if( tok == T_path )
    {
        for(;;)
        {
            if( tok != T_path )
                expecting( T_path );
                    
            PATH* path = new PATH( growth ) ;
            growth->paths.push_back( path );
            
            doPATH( path );

            tok = nextTok();
            if( tok == T_RIGHT )
                break;

            if( tok != T_LEFT )
                expecting(T_LEFT);

            tok = nextTok();            
        }
    }
    else
        expecting( wxT("rect|path") );
}

void SPECCTRA_DB::doPATH( PATH* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    switch( tok )
    {
    case T_SYMBOL:
    case T_STRING:
        
    case T_pcb:         // reserved layer names
    case T_power:
    case T_signal:
        growth->layer_id = lexer->CurText();
        break;
        
    default:
        expecting( T_SYMBOL );
    }

    if( nextTok() != T_NUMBER )
        expecting( wxT("<aperture_width>") );
    
    growth->aperture_width = strtod( lexer->CurText(), NULL );

    POINT   ptTemp;
    
    tok = nextTok();
    
    do
    {
        if( tok != T_NUMBER )
            expecting( T_NUMBER );
        ptTemp.x = strtod( lexer->CurText(), NULL );
    
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        ptTemp.y = strtod( lexer->CurText(), NULL );
        
        growth->points.push_back( ptTemp );
           
    } while( (tok = nextTok())!=T_RIGHT && tok!=T_LEFT );
    
    if( tok == T_LEFT )
    {
        if( nextTok() != T_aperture_type )
            expecting( T_aperture_type );
        
        tok = nextTok();
        if( tok!=T_round && tok!=T_square )
            expecting( wxT("round|square") );

        growth->aperture_type = tok;
        
        if( nextTok() != T_RIGHT )
            expecting( T_RIGHT );
    }
}


void SPECCTRA_DB::doRECTANGLE( RECTANGLE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );

    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point0.x = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point0.y = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point1.x = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->point1.y = strtod( lexer->CurText(), NULL );

    if( nextTok() != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doSTRINGPROP( STRINGPROP* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );

    growth->value = lexer->CurText();
    
    if( nextTok() != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doBOOLPROP( STRINGPROP* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok!=T_on && tok!=T_off )
        expecting( wxT("on|off") );

    growth->value = (tok==T_on);
    
    if( nextTok() != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doVIA( VIA* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            if( nextTok() != T_spare )
                expecting( T_spare );
            
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( !isSymbol( tok ) )
                    expecting( T_SYMBOL );
                
                growth->spares.push_back( lexer->CurText() );
            }
        }
        else if( isSymbol( tok ) )
        {
            growth->padstacks.push_back( lexer->CurText() );
        }
        else
            unexpected( lexer->CurText() );
    }
}


void SPECCTRA_DB::doCONTROL( CONTROL* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok == T_LEFT )
        {
            tok = nextTok();
            
            switch( tok )
            {
            case T_via_at_smd:
                tok = nextTok();
                if( tok!=T_on && tok!=T_off )
                    expecting( wxT("on|off") );
                growth->via_at_smd = (tok==T_on);
                break;
                
            default:
                unexpected( lexer->CurText() );
            }
        }
    }
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


const char* SPECCTRA_DB::GetQuoteChar( const char* wrapee ) 
{
    while( *wrapee )
    {
        // if the string to be wrapped, the wrapee has a delimiter in it, 
        // use the quote_char
        if( strchr( "\t ()", *wrapee++ ) )
            return quote_char.c_str();
    }
    return "";      // can use and unwrapped string.
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


//-----<ELEM>---------------------------------------------------------------

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


//-----<PARSER>-----------------------------------------------------------

void PARSER::Save( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) ); 
    out->Print( nestLevel+1, "(string_quote %c)\n", string_quote );
    out->Print( nestLevel+1, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
    out->Print( nestLevel+1, "(host_cad \"%s\")\n", host_cad.c_str() ); 
    out->Print( nestLevel+1, "(host_version \"%s\")\n", host_version.c_str() );
    
    if( const_id1.length()>0 || const_id2.length()>0 )
        out->Print( nestLevel+1, "(constant %c%s%c %c%s%c)\n", 
            string_quote, const_id1.c_str(), string_quote,
            string_quote, const_id2.c_str(), string_quote );

    if( routes_include_testpoint || routes_include_guides || routes_include_image_conductor )
        out->Print( nestLevel+1, "(routes_include%s%s%s)\n",
                   routes_include_testpoint ? " testpoint" : "",
                   routes_include_guides ? " guides" : "",
                   routes_include_image_conductor ? " image_conductor" : "");
    
    if( wires_include_testpoint )
        out->Print( nestLevel+1, "(wires_include testpoint)\n" );
        
    if( !via_rotate_first )
        out->Print( nestLevel+1, "(via_rotate_first off)\n" );
    
    out->Print( nestLevel+1, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
    out->Print( nestLevel, ")\n" ); 
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
