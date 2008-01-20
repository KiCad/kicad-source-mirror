
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

 
/*  This source file implements export and import capabilities to the 
    specctra dsn file format.  The grammar for that file format is documented
    fairly well.  There are classes for each major type of descriptor in the
    spec.
    
    Since there are so many classes in here, it may be helpful to generate 
    the Doxygen directory:
    
    $ cd <kicadSourceRoot>
    $ doxygen
    
    Then you can view the html documentation in the <kicadSourceRoot>/doxygen
    directory.  The main class in this file is SPECCTRA_DB and its main
    functions are LoadPCB(), LoadSESSION(), and ExportPCB().
    
    Wide use is made of boost::ptr_vector<> and std::vector<> template classes.
    If the contained object is small, then std::vector tends to be used.
    If the contained object is large, or variable size, then boost::ptr_vector
    cannot be beat.
*/    


#include <cstdarg>
#include <cstdio>

#include <boost/ptr_container/ptr_vector.hpp>

#include <wx/ffile.h>
#include "fctsys.h"
#include "pcbstruct.h"
#include "dsn.h"


//#include <time.h>


#define EDA_BASE            // build_version.h behavior
#undef  COMMON_GLOBL
#define COMMON_GLOBL        // build_version.h behavior
#include "build_version.h"




namespace DSN {

#define NESTWIDTH           4   ///< how many spaces per nestLevel    
    
    
class SPECCTRA_DB;
class PCB;


/**
 * Class OUTPUTFORMATTER
 * is an interface (abstract class) used to output ASCII text.  The destination
 * of the ASCII text is up to the implementer.
 */
class OUTPUTFORMATTER
{

// When used on a C++ function, we must account for the "this" pointer,
// so increase the STRING-INDEX and FIRST-TO_CHECK by one.
// See http://docs.freebsd.org/info/gcc/gcc.info.Function_Attributes.html
// Then to get format checking during the compile, compile with -Wall or -Wformat
#define PRINTF_FUNC       __attribute__ ((format (printf, 3, 4)))
    
public:
    
    /**
     * Function print
     * formats and writes text to the output stream.
     *
     * @param nestLevel The multiple of spaces to preceed the output with. 
     * @param fmt A printf() style format string.
     * @param ... a variable list of parameters that will get blended into 
     *  the output under control of the format string.
     * @return int - the number of characters output.
     * @throw IOError, if there is a problem outputting, such as a full disk.
     */
    virtual int PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IOError ) = 0;

    /**
     * Function GetQuoteChar
     * returns the quote character as a single character string for a given 
     * input wrapee string.  Often the return value is "" the null string if
     * there are no delimiters in the input string.  If you want the quote_char
     * to be assuredly not "", then pass in "(" as the wrappee.
     * @param wrapee A string that might need wrapping on each end.
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
    
    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER according to the 
     * SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IOError )
    {
        out->Print( nestLevel, " %f %f", x, y ); 
    }
};

typedef std::vector<std::string>    STRINGS;
typedef std::vector<POINT>          POINTS;

struct PROPERTY
{
    std::string name;
    std::string value;

    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER according to the 
     * SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IOError )
    {
        const char* quoteName  = out->GetQuoteChar( name.c_str() );
        const char* quoteValue = out->GetQuoteChar( value.c_str() );
        
        out->Print( nestLevel, "(%s%s%s %s%s%s)\n", 
                   quoteName,  name.c_str(), quoteName,
                   quoteValue, value.c_str(), quoteValue );
    }
};
typedef std::vector<PROPERTY>       PROPERTIES;
    

/**
 * Class ELEM
 * is a base class for any DSN element class. 
 * See class ELEM_HOLDER also.
 */ 
class ELEM
{
protected:    
    DSN_T           type;
    ELEM*           parent;
    
public:

    ELEM( DSN_T aType, ELEM* aParent = 0 );
    
    virtual ~ELEM();
    
    DSN_T   Type() { return type; }

    
    /**
     * Function GetUnits
     * returns the units for this section.  Derived classes may override this
     * to check for section specific overrides.
     * @return DSN_T - one of the allowed values to &lt;unit_descriptor&gt;
     */
    virtual DSN_T   GetUnits()
    {
        if( parent )
            return parent->GetUnits();
        
        return T_inch;
    }

    
    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER according to the 
     * SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    virtual void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );

    
    /**
     * Function FormatContents
     * writes the contents as ASCII out to an OUTPUTFORMATTER according to the 
     * SPECCTRA DSN format.  This is the same as Format() except that the outer
     * wrapper is not included.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        // overridden in ELEM_HOLDER
    }
};


/**
 * Class ELEM_HOLDER
 * is a holder for any DSN class.  It can contain other
 * class instances, including classes derived from this class.
 */ 
class ELEM_HOLDER : public ELEM
{
    //  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;
    
    ELEM_ARRAY      kids;      ///< ELEM pointers

public:    
    
    ELEM_HOLDER( DSN_T aType, ELEM* aParent = 0 ) :
        ELEM( aType, aParent )
    {
    }

    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
    
    
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
 * It is not actually a parser, but rather corresponds to &lt;parser_descriptor&gt;
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
    bool        generated_by_freeroute;
    
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
        generated_by_freeroute = false;
        
        host_cad = "Kicad's PCBNEW";
        host_version = CONV_TO_UTF8(g_BuildVersion);
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
};


/**
 * Class UNIT_RES
 * is a holder for either a T_unit or T_resolution object which are usually
 * mutually exclusive in the dsn grammar, except within the T_pcb level.
 */
class UNIT_RES : public ELEM
{
    friend class SPECCTRA_DB;
    
    DSN_T       units;
    int         value;

public:
    UNIT_RES( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        units = T_inch;
        value = 2540000;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( type == T_unit )
            out->Print( nestLevel, "(%s %s)\n", LEXER::GetTokenText( Type() ), 
                       LEXER::GetTokenText(units) ); 

        else    // T_resolution            
            out->Print( nestLevel, "(%s %s %d)\n", LEXER::GetTokenText( Type() ), 
                       LEXER::GetTokenText(units), value ); 
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
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char*  quote = out->GetQuoteChar( layer_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s %f %f %f %f)\n", 
                   LEXER::GetTokenText( Type() ),
                   quote, layer_id.c_str(), quote,
                   point0.x, point0.y,
                   point1.x, point1.y );
    }
};


/**
 * Class RULE
 * corresponds to the &lt;rule_descriptor&gt; in the specctra dsn spec.
 */
class RULE : public ELEM
{
    friend class SPECCTRA_DB;

    STRINGS     rules;      ///< rules are saved in std::string form.

public:

    RULE( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s ", LEXER::GetTokenText( Type() ) );
        
        bool singleLine;
        
        if( rules.size() == 1 )
        {
            singleLine = true;
            out->Print( 0, "%s)", rules.begin()->c_str() );
        }
        
        else
        {
            singleLine = false;
            for( STRINGS::const_iterator i = rules.begin();  i!=rules.end(); ++i )
                out->Print( nestLevel, "%s\n", i->c_str() );
            out->Print( nestLevel, ")" );
        }
        
        if( nestLevel || !singleLine )
            out->Print( 0, "\n" );
    }
};


#if 0
class PLACE_RULE : public RULE
{
    friend class SPECCTRA_DB;
    
    DSN_T       object_type;
    DSN_T       image_type;
    
    /*  T_spacing, T_permit_orient, T_permit_side & T_opposite_side are
        all stored in the kids list.
    */
    
public:

    PLACE_RULE( ELEM* aParent ) :
        RULE( aParent, T_place_rule )
    {
        object_type = T_NONE;
        image_type  = T_NONE;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( object_type != T_NONE )
        {
            if( object_type == T_pcb )
                out->Print( nestLevel, "(object_type %s", 
                                         LEXER::GetTokenText( object_type ) );
            else
                out->Print( nestLevel, "(object_type image_set %s",
                                         LEXER::GetTokenText( object_type ) );
            
            if( image_type != T_NONE )
                out->Print( 0, " (image_type %s)", LEXER::GetTokenText( image_type ) );
            out->Print( 0, ")\n" ); 
        }
        
        RULE::FormatContents( out, nestLevel );
    }
};
#endif


class LAYER_RULE : public ELEM
{
    friend class SPECCTRA_DB;
    
    STRINGS         layer_ids;
    RULE*           rule;
    
public:

    LAYER_RULE( ELEM* aParent ) :
        ELEM( T_layer_rule, aParent )
    {
        rule = 0;
    }
    ~LAYER_RULE()
    {
        delete rule;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s", LEXER::GetTokenText( Type() ) );
        
        for( STRINGS::const_iterator i=layer_ids.begin();  i!=layer_ids.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }
        out->Print( 0 , "\n" );
        
        if( rule )
            rule->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};

typedef boost::ptr_vector<LAYER_RULE>   LAYER_RULES;


/**
 * Class PATH
 * supports both the &lt;path_descriptor&gt; and the &lt;polygon_descriptor&gt; per
 * the specctra dsn spec.
 */
class PATH : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     layer_id;
    double          aperture_width;

    POINTS          points;                   
    
    DSN_T           aperture_type;
    
public:

    PATH( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        aperture_width = 0.0;
        aperture_type  = T_round;
    }
    
    ~PATH()
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
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


///  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
typedef boost::ptr_vector<PATH> PATHS;


class BOUNDARY : public ELEM
{
    friend class SPECCTRA_DB;
 
    // only one or the other of these two is used, not both
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
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        if( rectangle )
            rectangle->Format( out, nestLevel+1 );
        else
        {
            for( PATHS::iterator i=paths.begin();  i!=paths.end();  ++i )
                i->Format( out, nestLevel+1 );
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class CIRCLE : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string layer_id;
    
    double      diameter;
    POINT       vertex;
    
public:    
    CIRCLE( ELEM* aParent ) :
        ELEM( T_circle, aParent )
    {
        diameter = 0.0;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %f %f %f)\n", LEXER::GetTokenText( Type() ) ,
                                 quote, layer_id.c_str(), quote,
                                 diameter, vertex.x, vertex.y );
    }
};


class QARC : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string layer_id;
    double      aperture_width;
    POINT       vertex[3];

public:    
    QARC( ELEM* aParent ) :
        ELEM( T_qarc, aParent )
    {
        aperture_width = 0.0;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %f\n", LEXER::GetTokenText( Type() ) ,
                                 quote, layer_id.c_str(), quote,
                                 aperture_width);
        
        for( int i=0;  i<3;  ++i )
            out->Print( nestLevel+1, "%f %f\n",  vertex[i].x, vertex[i].y );
        
        out->Print( nestLevel, ")\n" );
    }
};


class WINDOW : public ELEM
{
    friend class SPECCTRA_DB;

    //----- only one of these is used, like a union -----
    PATH*           path;           ///< used for both path and polygon
    RECTANGLE*      rectangle;
    CIRCLE*         circle;
    QARC*           qarc;
    //---------------------------------------------------

public:
    
    WINDOW( ELEM* aParent ) :
        ELEM( T_window, aParent )
    {
        path = 0;
        rectangle = 0;
        circle = 0;
        qarc = 0;
    }
    
    ~WINDOW()
    {
        delete path;
        delete rectangle;
        delete circle;
        delete qarc;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        // these are mutually exclusive
        if( rectangle )
            rectangle->Format( out, nestLevel );
        
        else if( path )
            path->Format( out, nestLevel );
        
        else if( circle )
            circle->Format( out, nestLevel );

        else if( qarc )
            qarc->Format( out, nestLevel );
    }
};

typedef boost::ptr_vector<WINDOW>   WINDOWS;


/**
 * Class KEEPOUT
 * is used for <keepout_descriptor> and <plane_descriptor>.
 */
class KEEPOUT : public ELEM
{
    friend class SPECCTRA_DB;

protected:    
    std::string     name;
    int             sequence_number;
    RULE*           rules;
    RULE*           place_rules;
    
    WINDOWS         windows;
    
    //----- only one of these is used, like a union -----
    PATH*           path;           ///< used for both path and polygon
    RECTANGLE*      rectangle;
    CIRCLE*         circle;
    QARC*           qarc;
    //---------------------------------------------------
    
public:

    /**
     * Constructor KEEPOUT
     * requires a DSN_T because this class is used for T_place_keepout, T_via_keepout,
     * T_wire_keepout, T_bend_keepout, and T_elongate_keepout as well as T_keepout.
     */
    KEEPOUT( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        rules = 0;
        place_rules = 0;

        path = 0;
        rectangle = 0;
        circle = 0;
        qarc = 0;
        
        sequence_number = -1;
    }
    
    ~KEEPOUT()
    {
        delete rules;
        delete place_rules;

        delete path;
        delete rectangle;
        delete circle;
        delete qarc;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        if( name.size() )
        {
            const char* quote = out->GetQuoteChar( name.c_str() );
            out->Print( nestLevel+1, "%s%s%s\n", quote, name.c_str(), quote );
        }

        if( sequence_number != -1 )
            out->Print( nestLevel+1, "(sequence_number %d)\n", sequence_number );
        
        // these are mutually exclusive
        if( rectangle )
            rectangle->Format( out, nestLevel+1 );
        
        else if( path )
            path->Format( out, nestLevel+1 );
        
        else if( circle )
            circle->Format( out, nestLevel+1 );

        else if( qarc )
            qarc->Format( out, nestLevel+1 );

        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( place_rules )
            place_rules->Format( out, nestLevel+1 );

        for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
            i->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class VIA : public ELEM
{
    friend class SPECCTRA_DB;

    STRINGS     padstacks;
    STRINGS     spares;

public:

    VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );
        
        for( STRINGS::iterator i=padstacks.begin();  i!=padstacks.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel+1, "%s%s%s\n", quote, i->c_str(), quote );
        }
        
        if( spares.size() )
        {
            out->Print( nestLevel+1, "(spare\n" );

            for( STRINGS::iterator i=spares.begin();  i!=spares.end();  ++i )
            {
                const char* quote = out->GetQuoteChar( i->c_str() );
                out->Print( nestLevel+2, "%s%s%s\n", quote, i->c_str(), quote );
            }
            
            out->Print( nestLevel+1, ")\n" );
        }
        
        out->Print( nestLevel, ")\n" );
    }
};


class CLASSES : public ELEM
{
    friend class SPECCTRA_DB;

    STRINGS         class_ids;
    
public:
    CLASSES( ELEM* aParent ) :
        ELEM( T_classes, aParent )
    {
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( STRINGS::iterator i=class_ids.begin();  i!=class_ids.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, i->c_str(), quote );
        }
    }
};


class CLASS_CLASS : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;
    
    CLASSES*        classes;
    
    /*  rule | layer_rule are put into the kids container.
    */
    

public:
    
    /**
     * Constructor CLASS_CLASS
     * @param aType May be either T_class_class or T_region_class_class
     */
    CLASS_CLASS( ELEM* aParent, DSN_T aType ) :
        ELEM_HOLDER( aType, aParent )
    {
        classes = 0;
    }
    
    ~CLASS_CLASS()
    {
        delete classes;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( classes )
            classes->Format( out, nestLevel );

        // format the kids
        ELEM_HOLDER::FormatContents( out, nestLevel );
    }
};


class CONTROL : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;

    bool    via_at_smd;
    bool    via_at_smd_grid_on;
    
public:
    CONTROL( ELEM* aParent ) :
        ELEM_HOLDER( T_control, aParent )
    {
        via_at_smd = false;
        via_at_smd_grid_on = false;
    }
   
    ~CONTROL()
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        //if( via_at_smd )
        {
            out->Print( nestLevel+1, "(via_at_smd %s", via_at_smd ? "on" : "off" );
            if( via_at_smd_grid_on )
                out->Print( 0, " grid %s", via_at_smd_grid_on ? "on" : "off" );
            
            out->Print( 0, ")\n" );
        }

        for( int i=0;  i<Length();  ++i )
        {
            At(i)->Format( out, nestLevel+1 );
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class LAYER : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string name;
    DSN_T       layer_type; ///< one of: T_signal, T_power, T_mixed, T_jumper
    int         direction;
    int         cost;       ///< [forbidden | high | medium | low | free | <positive_integer > | -1]
    int         cost_type;  ///< T_length | T_way 
    RULE*       rules;
    STRINGS     use_net;
    
    PROPERTIES  properties;
    
public:

    LAYER( ELEM* aParent ) :
        ELEM( T_layer, aParent )
    {
        layer_type = T_signal;
        direction  = -1;
        cost       = -1;
        cost_type  = -1;
        
        rules = 0;
    }
    
    ~LAYER()
    {
        delete rules;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( name.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                       quote, name.c_str(), quote );

        out->Print( nestLevel+1, "(type %s)\n", LEXER::GetTokenText( layer_type ) );

        if( properties.size() )
        {
            out->Print( nestLevel+1, "(property \n" );
            
            for( PROPERTIES::iterator i = properties.begin();  i != properties.end();  ++i )
            {
                i->Format( out, nestLevel+2 );
            }
            out->Print( nestLevel+1, ")\n" );
        }
        
        if( direction != -1 )
            out->Print( nestLevel+1, "(direction %s)\n", 
                       LEXER::GetTokenText( (DSN_T)direction ) );

        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( cost != -1 )
        {
            if( cost < 0 )
                out->Print( nestLevel+1, "(cost %d", -cost );   // positive integer, stored as negative
            else
                out->Print( nestLevel+1, "(cost %s", LEXER::GetTokenText( (DSN_T)cost ) );
            
            if( cost_type != -1 )
                out->Print( 0, " (type %s)", LEXER::GetTokenText( (DSN_T)cost_type ) );
            
            out->Print( 0, ")\n" );
        }

        if( use_net.size() )
        {
            out->Print( nestLevel+1, "(use_net" );
            for( STRINGS::const_iterator i = use_net.begin();  i!=use_net.end(); ++i )
            {
                const char* quote = out->GetQuoteChar( i->c_str() );
                out->Print( 0, " %s%s%s",  quote, i->c_str(), quote );
            }
            out->Print( 0, ")\n" );
        }
        
        out->Print( nestLevel, ")\n" ); 
    }
};


class LAYER_PAIR : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     layer_id0;
    std::string     layer_id1;

    double          layer_weight;    
    
public:
    LAYER_PAIR( ELEM* aParent ) :
        ELEM( T_layer_pair, aParent )
    {
        layer_weight = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote0 = out->GetQuoteChar( layer_id0.c_str() );
        const char* quote1 = out->GetQuoteChar( layer_id1.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s %s%s%s %f)\n", LEXER::GetTokenText( Type() ),
               quote0, layer_id0.c_str(), quote0,
               quote1, layer_id1.c_str(), quote1,
               layer_weight );
    }
};


class LAYER_NOISE_WEIGHT : public ELEM
{
    friend class SPECCTRA_DB;
    
    typedef boost::ptr_vector<LAYER_PAIR>  LAYER_PAIRS;
    LAYER_PAIRS     layer_pairs;
    
public:

    LAYER_NOISE_WEIGHT( ELEM* aParent ) :
        ELEM( T_layer_noise_weight, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );
        
        for( LAYER_PAIRS::iterator i=layer_pairs.begin(); i!=layer_pairs.end();  ++i )
            i->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};


class PLANE : public KEEPOUT
{
    friend class SPECCTRA_DB;

public:    
    PLANE( ELEM* aParent ) :
        KEEPOUT( aParent, T_plane )
   {}
};


/**
 * Class TOKPROP
 * is a container for a single property whose value is another DSN_T token.
 * The name of the property is obtained from the DSN_T Type().
 */
class TOKPROP : public ELEM
{
    friend class SPECCTRA_DB;

    DSN_T       value;

public:

    TOKPROP( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s %s)\n", LEXER::GetTokenText( Type() ),
                   LEXER::GetTokenText( value ) );
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
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( value.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s)\n", LEXER::GetTokenText( Type() ),
                                quote, value.c_str(), quote );
    }
};


class REGION : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;

    std::string     region_id;

    //-----<mutually exclusive>--------------------------------------
    RECTANGLE*      rectangle;
    PATH*           polygon;
    //-----</mutually exclusive>-------------------------------------

    /* region_net | region_class | region_class_class are all mutually
       exclusive and are put into the kids container.
    */
    
    RULE*           rules;    

public:
    REGION( ELEM* aParent ) :
        ELEM_HOLDER( T_region, aParent )
    {
        rectangle = 0;
        polygon = 0;
        rules = 0;
    }

    ~REGION()
    {
        delete rectangle;
        delete polygon;
        delete rules;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( region_id.size() )
        {
            const char* quote = out->GetQuoteChar( region_id.c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, region_id.c_str(), quote );
        }
        
        if( rectangle )
            rectangle->Format( out, nestLevel );
        
        if( polygon )
            polygon->Format( out, nestLevel );

        ELEM_HOLDER::FormatContents( out, nestLevel );
        
        if( rules )
            rules->Format( out, nestLevel );
    }
};


class GRID : public ELEM
{
    friend class SPECCTRA_DB;

    DSN_T       grid_type;      ///< T_via | T_wire | T_via_keepout | T_place | T_snap

    float       dimension;
    
    DSN_T       direction;      ///< T_x | T_y | -1 for both
    
    float       offset;
    
    DSN_T       image_type;
    
public:
    
    GRID( ELEM* aParent ) :
        ELEM( T_grid, aParent )
    {
        grid_type = T_via;
        direction = T_NONE;
        dimension = 0.0;
        offset    = 0.0;
        image_type= T_NONE;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s %s %f",
                   LEXER::GetTokenText( Type() ),
                   LEXER::GetTokenText( grid_type ), dimension );
     
        if( grid_type == T_place )
        {
            if( image_type==T_smd || image_type==T_pin )
                out->Print( 0, " (image_type %s)", LEXER::GetTokenText( image_type ) );
        }
        else
        {
            if( direction==T_x || direction==T_y )
                out->Print( 0, " (direction %s)", LEXER::GetTokenText( direction ) );
        }
        
        if( offset != 0.0 )
            out->Print( 0, " (offset %f)", offset );
        
        out->Print( 0, ")\n");
    }
};


class STRUCTURE : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;
    
    UNIT_RES*   unit;
    
    typedef boost::ptr_vector<LAYER>    LAYERS;
    LAYERS      layers;

    LAYER_NOISE_WEIGHT*  layer_noise_weight;
    
    BOUNDARY*   boundary;
    BOUNDARY*   place_boundary;
    VIA*        via;
    CONTROL*    control;
    RULE*       rules;
    
    typedef boost::ptr_vector<KEEPOUT>  KEEPOUTS;
    KEEPOUTS    keepouts;

    typedef boost::ptr_vector<PLANE>    PLANES;
    PLANES      planes;

    typedef boost::ptr_vector<REGION>   REGIONS;
    REGIONS     regions;
    
    RULE*       place_rules;
    
    typedef boost::ptr_vector<GRID>     GRIDS;
    GRIDS       grids;
    
public:

    STRUCTURE( ELEM* aParent ) :
        ELEM_HOLDER( T_structure, aParent )
    {
        unit = 0;
        layer_noise_weight = 0;
        boundary = 0;
        place_boundary = 0;
        via = 0;
        control = 0;
        rules = 0;
        place_rules = 0;
    }
    
    ~STRUCTURE()
    {
        delete unit;
        delete layer_noise_weight;
        delete boundary;
        delete place_boundary;
        delete via;
        delete control;
        delete rules;
        delete place_rules;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( unit )
            unit->Format( out, nestLevel );
        
        for( LAYERS::iterator i=layers.begin();  i!=layers.end();  ++i )
            i->Format( out, nestLevel ); 

        if( layer_noise_weight )
            layer_noise_weight->Format( out, nestLevel );
        
        if( boundary )
            boundary->Format( out, nestLevel );

        if( place_boundary )
            place_boundary->Format( out, nestLevel );

        for( PLANES::iterator i=planes.begin();  i!=planes.end();  ++i )
            i->Format( out, nestLevel );

        for( REGIONS::iterator i=regions.begin();  i!=regions.end();  ++i )
            i->Format( out, nestLevel );
        
        for( KEEPOUTS::iterator i=keepouts.begin();  i!=keepouts.end();  ++i )
            i->Format( out, nestLevel );
        
        if( via )
            via->Format( out, nestLevel );
        
        if( control )
            control->Format( out, nestLevel );
        
        for( int i=0; i<Length();  ++i )
        {
            At(i)->Format( out, nestLevel );
        }
        
        if( rules )
            rules->Format( out, nestLevel );

        if( place_rules )
            place_rules->Format( out, nestLevel );
        
        for( GRIDS::iterator i=grids.begin();  i!=grids.end();  ++i )
            i->Format( out, nestLevel );
    }
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


class PLACE : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     component_id;       ///< reference designator
    
    DSN_T           side;
    
    bool            isRotated;
    float           rotation;
    
    bool            hasVertex;
    POINT           vertex;
    
    DSN_T           mirror;
    DSN_T           status;

    std::string     logical_part;
    
    RULE*           place_rules;
    
    PROPERTIES      properties;
    
    DSN_T           lock_type;

    //-----<mutually exclusive>--------------    
    RULE*           rules;
    REGION*         region;
    //-----</mutually exclusive>-------------    

    std::string     part_number;
    
public:

    PLACE( ELEM* aParent ) :
        ELEM( T_place, aParent )
    {
        side = T_NONE;
        isRotated = false;
        hasVertex = false;
        
        mirror = T_NONE;
        status = T_NONE;
        place_rules = 0;
        
        lock_type = T_NONE;
        rules = 0;
        region = 0;
    }

    ~PLACE()
    {
        delete place_rules;
        delete rules;
        delete region;
    }
    
    void SetVertex( const POINT& aVertex )
    {
        vertex = aVertex;
        hasVertex = true;
    }

    void SetRotation( double aRotation )
    {
        rotation = (float) aRotation;
        isRotated = (aRotation != 0.0);
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
};


class COMPONENT : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     image_id;
    
    typedef boost::ptr_vector<PLACE>    PLACES;
    PLACES          places;
    
public:
    COMPONENT( ELEM* aParent ) :
        ELEM( T_component, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( image_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, image_id.c_str(), quote );

        for( PLACES::iterator i=places.begin();  i!=places.end();  ++i )
            i->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};


class PLACEMENT : public ELEM
{
    friend class SPECCTRA_DB;

    UNIT_RES*   unit;
    
    DSN_T       flip_style;
    
    typedef boost::ptr_vector<COMPONENT> COMPONENTS;
    COMPONENTS  components;
    
public:    
    PLACEMENT( ELEM* aParent ) :
        ELEM( T_placement, aParent )
    {
        unit = 0;
        flip_style = T_NONE;
    }

    ~PLACEMENT()
    {
        delete unit;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( unit )
            unit->Format( out, nestLevel );
        
        if( flip_style != T_NONE )
        {
            out->Print( nestLevel, "(place_control (flip_style %s))\n",
                       LEXER::GetTokenText( flip_style ) );
        }
        
        for( COMPONENTS::iterator i=components.begin();  i!=components.end();  ++i )
            i->Format( out, nestLevel );
    }
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


class SHAPE : public ELEM
{
    friend class SPECCTRA_DB;

    DSN_T           connect;
    
    //----- only one of these is used, like a union -----
    PATH*           path;           ///< used for both path and polygon
    RECTANGLE*      rectangle;
    CIRCLE*         circle;
    QARC*           qarc;
    //---------------------------------------------------
    
    WINDOWS         windows;
    
    
public:
    SHAPE( ELEM* aParent, DSN_T aType = T_shape ) :
        ELEM( aType, aParent )
    {
        connect = T_on;
        
        path = 0;
        rectangle = 0;
        circle = 0;
        qarc = 0;
    }
    
    ~SHAPE()
    {
        delete path;
        delete rectangle;
        delete circle;
        delete qarc;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( path )
            path->Format( out, nestLevel );
        
        else if( rectangle )
            rectangle->Format( out, nestLevel );
        
        else if( circle )
            circle->Format( out, nestLevel );
        
        else if( qarc )
            qarc->Format( out, nestLevel );
        
        if( connect == T_off )
            out->Print( nestLevel, "(connect %s)\n", LEXER::GetTokenText( connect ) );
        
        for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
            i->Format( out, nestLevel );
    }
};


class PIN : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     padstack_id;
    float           rotation;
    bool            isRotated;
    std::string     pin_id;
    POINT           vertex;
    
public:
    PIN( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
        rotation = 0.0;
        isRotated = false;
    }

    void SetRotation( double aRotation )
    {
        rotation = (float) aRotation;
        isRotated = (aRotation != 0.0);
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );
        if( isRotated )
            out->Print( nestLevel, "(pin %s%s%s (rotate %1.2f)", 
                                     quote, padstack_id.c_str(), quote,
                                     rotation
                                     );
        else
            out->Print( nestLevel, "(pin %s%s%s", quote, padstack_id.c_str(), quote );
        
        quote = out->GetQuoteChar( pin_id.c_str() );
        out->Print( 0, " %s%s%s %f %f)\n", quote, pin_id.c_str(), quote, 
                   vertex.x, vertex.y );
    }
};


class IMAGE : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;
    
    std::string     image_id;
    DSN_T           side;
    UNIT_RES*       unit;
    
    /*  The grammar spec says only one outline is supported, but I am seeing
        *.dsn examples with multiple outlines.  So the outlines will go into 
        the kids list. 
    */
    
    typedef boost::ptr_vector<PIN>  PINS;
    PINS            pins;

    RULE*           rules;
    RULE*           place_rules;
    
public:
    
    IMAGE( ELEM* aParent ) :
        ELEM_HOLDER( T_image, aParent )
    {
        side = T_both;
        unit = 0;
        rules = 0;
        place_rules = 0;
    }
    ~IMAGE()
    {
        delete unit;
        delete rules;
        delete place_rules;
    }

    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( image_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s", LEXER::GetTokenText( Type() ),
                                quote, image_id.c_str(), quote );

        if( side != T_both )
            out->Print( 0, " (side %s)", LEXER::GetTokenText( side ) );
        
        out->Print( 0, "\n");
        
        if( unit )
            unit->Format( out, nestLevel+1 );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel+1 );
    
        for( PINS::iterator i=pins.begin();  i!=pins.end();  ++i )
            i->Format( out, nestLevel+1 );

        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( place_rules )
            place_rules->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
    
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


class PADSTACK : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;

    std::string     padstack_id;    
    UNIT_RES*       unit;

    /* The shapes are stored in the kids list */

    DSN_T           rotate;
    DSN_T           absolute;
    DSN_T           attach;
    std::string     via_id;
    
    RULE*           rules;

public:
    
    PADSTACK( ELEM* aParent ) :
        ELEM_HOLDER( T_padstack, aParent )
    {
        unit = 0;
        rotate = T_on;
        absolute = T_off;
        rules = 0;
        attach = T_off;
    }
    ~PADSTACK()
    {
        delete unit;
        delete rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, padstack_id.c_str(), quote );

        if( unit )
            unit->Format( out, nestLevel+1 );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel+1 );

        out->Print( nestLevel+1, "%s", "" );
        
        // spec for <attach_descriptor> says default is on, so
        // print the off condition to override this.        
        if( attach == T_off )
            out->Print( 0, "(attach off)" );
        else if( attach == T_on )
        {
            const char* quote = out->GetQuoteChar( via_id.c_str() );
            out->Print( 0, "(attach on (use_via %s%s%s))",
                       quote, via_id.c_str(), quote );
        }
        
        if( rotate == T_off )   // print the non-default
            out->Print( 0, "(rotate %s)", LEXER::GetTokenText( rotate ) );

        if( absolute == T_on )  // print the non-default
            out->Print( 0, "(absolute %s)", LEXER::GetTokenText( absolute ) );

        out->Print( 0, "\n" );
        
        if( rules )
            rules->Format( out, nestLevel+1 );
        
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
 * Class LIBRARY
 * corresponds to the &lt;library_descriptor&gt; in the specctra dsn specification.
 * Only unit_descriptor, image_descriptors, and padstack_descriptors are 
 * included as children at this time.
 */
class LIBRARY : public ELEM
{
    friend class SPECCTRA_DB;

    UNIT_RES*       unit;
    
    typedef boost::ptr_vector<IMAGE>    IMAGES;
    IMAGES          images;

    typedef boost::ptr_vector<PADSTACK> PADSTACKS;
    PADSTACKS       padstacks;

public:

    LIBRARY( ELEM* aParent ) :
        ELEM( T_library, aParent )
    {
        unit = 0;
    }
    ~LIBRARY()
    {
        delete unit;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( unit )
            unit->Format( out, nestLevel );
        
        for( IMAGES::iterator i=images.begin();  i!=images.end();  ++i )
            i->Format( out, nestLevel );

        for( PADSTACKS::iterator i=padstacks.begin();  i!=padstacks.end();  ++i )
            i->Format( out, nestLevel );
    }
    
    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


/**
 * Class PIN_REF
 * corresponds to the &lt;pin_reference&gt; definition in the specctra dsn spec.
 */
class PIN_REF : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     component_id;
    std::string     pin_id;
    
public:

    PIN_REF( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        // only print the newline if there is a nest level, and make
        // the quotes unconditional on this one.
        const char* newline = nestLevel ? "\n" : "";
        out->Print( nestLevel, "\"%s\"-\"%s\"%s", 
                component_id.c_str(), pin_id.c_str(), newline );
    }
};
typedef std::vector<PIN_REF>   PIN_REFS;


class FROMTO : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     fromText;
    std::string     toText;
    
    DSN_T           fromto_type;
    std::string     net_id;
    RULE*           rules;
//    std::string     circuit;
    LAYER_RULES     layer_rules;
    

public:    
    FROMTO( ELEM* aParent ) :
        ELEM( T_fromto, aParent )
    {
        rules = 0;
        fromto_type  = T_NONE;
    }
    ~FROMTO()
    {
        delete rules;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        // no quoting on these two, the lexer preserved the quotes on input
        out->Print( nestLevel, "(%s %s %s ", 
                 LEXER::GetTokenText( Type() ), fromText.c_str(), toText.c_str() );

        if( type != T_NONE )
            out->Print( 0, "(type %s)", LEXER::GetTokenText( fromto_type ) );
        
        if( net_id.size() )
        {
            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( 0, "(net %s%s%s)",  quote, net_id.c_str(), quote );
        }

        bool singleLine = true;
        
        if( rules || layer_rules.size() )
        {
            out->Print( 0, "\n" );
            singleLine = false;
        }
        
        if( rules )
            rules->Format( out, nestLevel+1 );
        
        /*
        if( circuit.size() )
            out->Print( nestLevel, "%s\n", circuit.c_str() );
        */
     
        for( LAYER_RULES::iterator i=layer_rules.begin();  i!=layer_rules.end();  ++i )
            i->Format( out, nestLevel+1 );
        
        out->Print( singleLine ? 0 : nestLevel, ")" );
        if( nestLevel || !singleLine )
            out->Print( 0, "\n" );
    }
};


/**
 * Class COMP_ORDER
 * corresponds to the &lt;component_order_descriptor&gt;
 */
class COMP_ORDER : public ELEM
{
    friend class SPECCTRA_DB;

    STRINGS         placement_ids;
    
public:    
    COMP_ORDER( ELEM* aParent ) :
        ELEM( T_comp_order, aParent )
    {
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s", LEXER::GetTokenText( Type() ) );
        
        for( STRINGS::iterator i=placement_ids.begin();  i!=placement_ids.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }
        
        out->Print( 0, ")" );
        if( nestLevel )
            out->Print( 0, "\n" );
    }
};


class NET : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     net_id;
    bool            unassigned;
    int             net_number;

    DSN_T           pins_type;      ///< T_pins | T_order
    
    PIN_REFS        pins;

    DSN_T           type;           ///< T_fix | T_normal
    
    DSN_T           supply;         ///< T_power | T_ground
    
    RULE*           rules;
    
    LAYER_RULES     layer_rules;
    
    FROMTO*         fromto;
    
    COMP_ORDER*     comp_order;
    
public:

    NET( ELEM* aParent ) :
        ELEM( T_net, aParent )
    {
        unassigned = false;
        net_number = T_NONE;
        pins_type = T_pins;
        
        type = T_NONE;
        supply = T_NONE;
        
        rules = 0;
        fromto = 0;
        comp_order = 0;
    }
    
    ~NET()
    {
        delete rules;
        delete fromto;
        delete comp_order;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( net_id.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s ", LEXER::GetTokenText( Type() ), 
                   quote, net_id.c_str(), quote );
        
        if( unassigned )
            out->Print( 0, "(unassigned)" );
        
        if( net_number != T_NONE )
            out->Print( 0, "(net_number %d)", net_number );
        
        out->Print( 0, "\n" );
        
        out->Print( nestLevel+1, "(%s\n", LEXER::GetTokenText( pins_type ) );
        for( PIN_REFS::iterator i=pins.begin();  i!=pins.end();  ++i )
            i->Format( out, nestLevel+2 ); 
        out->Print( nestLevel+1, ")\n" );

        if( comp_order )
            comp_order->Format( out, nestLevel+1 );
        
        if( type != T_NONE )
            out->Print( nestLevel+1, "(type %s)\n", LEXER::GetTokenText( type ) );
        
        if( rules )
            rules->Format( out, nestLevel+1 );
        
        for( LAYER_RULES::iterator i=layer_rules.begin();  i!=layer_rules.end();  ++i )
            i->Format( out, nestLevel+1 );

        if( fromto )
            fromto->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};


class TOPOLOGY : public ELEM
{
    friend class SPECCTRA_DB;
    
    typedef boost::ptr_vector<FROMTO>       FROMTOS;
    FROMTOS         fromtos;
  
    typedef boost::ptr_vector<COMP_ORDER>   COMP_ORDERS;
    COMP_ORDERS     comp_orders;

public:    
    TOPOLOGY( ELEM* aParent ) :
        ELEM( T_topology, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( FROMTOS::iterator i=fromtos.begin();  i!=fromtos.end();  ++i )
            i->Format( out, nestLevel );
        
        for( COMP_ORDERS::iterator i=comp_orders.begin();  i!=comp_orders.end();  ++i )
            i->Format( out, nestLevel );
    }
};


class CLASS : public ELEM
{
    friend class SPECCTRA_DB;
    
    std::string     class_id;
    
    STRINGS         net_ids;
    
    /// <circuit_descriptor> list
    STRINGS         circuit;
    
    RULE*           rules;
    
    LAYER_RULES     layer_rules;
    
    TOPOLOGY*       topology;
    
public:

    CLASS( ELEM* aParent ) :
        ELEM( T_class, aParent )
    {
        rules = 0;
        topology = 0;
    }
    ~CLASS()
    {
        delete rules;
        delete topology;
    }

    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( class_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s", LEXER::GetTokenText( Type() ),
                                 quote, class_id.c_str(), quote );

        const int NETGAP = 2;
        const int RIGHTMARGIN = 92;
        
        int perRow=RIGHTMARGIN;
        for( STRINGS::iterator i=net_ids.begin();  i!=net_ids.end();  ++i )
        {
            quote = out->GetQuoteChar( i->c_str() );
            int slength = strlen( i->c_str() );
            if( *quote!='\0' )
                slength += 2;

            if( perRow + slength + NETGAP > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perRow = 0;
                perRow += out->Print( nestLevel+1, "%s%s%s", 
                                     quote, i->c_str(), quote );
            }
            else
            {
                perRow += out->Print( 0, "%*c%s%s%s", NETGAP, ' ',
                                     quote, i->c_str(), quote );
            }
        }
        out->Print( 0, "\n" );

        for( STRINGS::iterator i=circuit.begin();  i!=circuit.end();  ++i )
            out->Print( nestLevel+1, "%s\n", i->c_str() );

        for( LAYER_RULES::iterator i=layer_rules.begin();  i!=layer_rules.end();  ++i )
            i->Format( out, nestLevel+1 );
        
        if( topology )
            topology->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};
    

class NETWORK : public ELEM
{
    friend class SPECCTRA_DB;

    typedef boost::ptr_vector<NET>  NETS;
    NETS        nets;
    
    typedef boost::ptr_vector<CLASS> CLASSLIST;
    CLASSLIST   classes;
    
    
public:

    NETWORK( ELEM* aParent ) :
        ELEM( T_network, aParent )
    {
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( NETS::iterator i=nets.begin();  i!=nets.end();  ++i )
            i->Format( out, nestLevel );
        
        for( CLASSLIST::iterator i=classes.begin();  i!=classes.end();  ++i )
            i->Format( out, nestLevel );
    }
};


class CONNECT : public ELEM
{
};


/**
 * Class WIRE
 * corresponds to &lt;wire_shape_descriptor&gt; in the specctra dsn spec.
 */
class WIRE : public ELEM
{
    friend class SPECCTRA_DB;

    //----- only one of these is used, like a union -----
    PATH*           path;           ///< used for both path and polygon
    RECTANGLE*      rectangle;
    CIRCLE*         circle;
    QARC*           qarc;
    //---------------------------------------------------
    
    std::string     net_id;
    int             turret;
    DSN_T           type;
    DSN_T           attr;
    std::string     shield;
    WINDOWS         windows;
    CONNECT*        connect;
    bool            supply;
    
public:
    WIRE( ELEM* aParent ) :
        ELEM( T_wire, aParent )
    {
        path = 0;
        rectangle = 0;
        circle = 0;
        qarc = 0;
        connect = 0;
        
        turret = -1;
        type = T_NONE;
        attr = T_NONE;
        supply = false;
    }
    
    ~WIRE()
    {
        delete path;
        delete rectangle;
        delete circle;
        delete qarc;
        delete connect;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        // these are mutually exclusive
        if( rectangle )
            rectangle->Format( out, nestLevel );
        
        else if( path )
            path->Format( out, nestLevel );
        
        else if( circle )
            circle->Format( out, nestLevel );

        else if( qarc )
            qarc->Format( out, nestLevel );
        
        if( net_id.size() )
        {
            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( nestLevel, "(net %s%s%s)\n", 
                       quote, net_id.c_str(), quote );
        }
        
        if( turret >= 0 )
            out->Print( nestLevel, "(turrent %d)\n", turret );

        if( type != T_NONE )
            out->Print( nestLevel, "(type %s)\n", LEXER::GetTokenText( type ) );
        
        if( attr != T_NONE )
            out->Print( nestLevel, "(attr %s)\n", LEXER::GetTokenText( attr ) );
        
        if( shield.size() )
        {
            const char* quote = out->GetQuoteChar( shield.c_str() );
            out->Print( nestLevel, "(shield %s%s%s)\n", 
                       quote, shield.c_str(), quote );
        }
        
        for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
            i->Format( out, nestLevel );

        if( connect )
            connect->Format( out, nestLevel );
        
        if( supply )
            out->Print( nestLevel, "(supply)\n" );
    }
};
typedef boost::ptr_vector<WIRE>     WIRES;


/**
 * Class WIRE_VIA
 * corresponds to &lt;wire_via_descriptor&gt; in the specctra dsn spec.
 */
class WIRE_VIA : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     padstack_id;
    POINTS          vertexes;
    std::string     net_id;
    int             via_number;
    DSN_T           type;
    DSN_T           attr;
    std::string     virtual_pin_name;
    STRINGS         contact_layers;
    bool            supply;
    
public:
    WIRE_VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
        via_number = -1;
        type = T_NONE;
        attr = T_NONE;
        supply = false;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s", LEXER::GetTokenText( Type() ),
                   quote, padstack_id.c_str(), quote );

        const int RIGHTMARGIN = 80;        
        int perLine=RIGHTMARGIN;
        for( POINTS::iterator i=vertexes.begin();  i!=vertexes.end();  ++i )
        {
            if( perLine >= RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = 0;
                perLine += out->Print( nestLevel+1, "%f %f", i->x, i->y ); 
            }
            else
            {
                perLine += out->Print( 0, "    %f %f", i->x, i->y );
            }
        }
        out->Print( 0, "\n" );
        
        if( net_id.size() )
        {
            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( nestLevel+1, "(net %s%s%s)\n", quote, net_id.c_str(), quote ); 
        }
        
        if( type != T_NONE )
            out->Print( nestLevel+1, "(type %s)\n", LEXER::GetTokenText( type ) );
        
        if( attr != T_NONE )
        {
            if( attr == T_virtual_pin )
            {
                const char* quote = out->GetQuoteChar( virtual_pin_name.c_str() );
                out->Print( nestLevel+1, "(attr virtual_pin %s%s%s)\n",
                           quote, virtual_pin_name.c_str(), quote );
            }
            else
                out->Print( nestLevel+1, "(attr %s)\n", LEXER::GetTokenText( attr ) );
        }
        
        if( contact_layers.size() )
        {
            out->Print( nestLevel+1, "(contact\n" );
            for( STRINGS::iterator i=contact_layers.begin();  i!=contact_layers.end();  ++i )
            {
                const char* quote = out->GetQuoteChar( i->c_str() );
                out->Print( nestLevel+2, "%s%s%s\n", quote, i->c_str(), quote );
            }
            out->Print( nestLevel+1, ")\n" );
        }
        
        if( supply )
            out->Print( nestLevel+1, "(supply)\n" );
        
        out->Print( nestLevel, ")\n" );
    }
};
typedef boost::ptr_vector<WIRE_VIA>      WIRE_VIAS;


/**
 * Class WIRING
 * corresponds to &lt;wiring_descriptor&gt; in the specctra dsn spec.
 */
class WIRING : public ELEM
{
    friend class SPECCTRA_DB;
    
    UNIT_RES*   unit;
    WIRES       wires;
    WIRE_VIAS   wire_vias;    

public:

    WIRING( ELEM* aParent ) :
        ELEM( T_wiring, aParent )
    {
        unit = 0;
    }
    ~WIRING()
    {
        delete unit;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( unit )
            unit->Format( out, nestLevel );

        for( WIRES::iterator i=wires.begin();  i!=wires.end();  ++i )
            i->Format( out, nestLevel );
        
        for( WIRE_VIAS::iterator i=wire_vias.begin();  i!=wire_vias.end();  ++i )
            i->Format( out, nestLevel );
    }

    DSN_T   GetUnits()
    {
        if( unit )
            return unit->GetUnits();
        
        return ELEM::GetUnits();
    }
};


class PCB : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     pcbname;    
    PARSER*         parser;
    UNIT_RES*       resolution;
    UNIT_RES*       unit;
    STRUCTURE*      structure;
    PLACEMENT*      placement;
    LIBRARY*        library;
    NETWORK*        network;
    WIRING*         wiring;
    
public:
    
    PCB( ELEM* aParent = 0 ) :
        ELEM( T_pcb, aParent )
    {
        parser = 0;
        resolution = 0;
        unit = 0;
        structure = 0;
        placement = 0;
        library = 0;
        network = 0;
        wiring = 0;
    }
    
    ~PCB()
    {
        delete parser;
        delete resolution;
        delete unit;
        delete structure;
        delete placement;
        delete library;
        delete network;
        delete wiring;
    }
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( pcbname.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, pcbname.c_str(), quote );
        
        if( parser )
            parser->Format( out, nestLevel+1 );
        
        if( resolution )
            resolution->Format( out, nestLevel+1 );

        if( unit )
            unit->Format( out, nestLevel+1 );

        if( structure )
            structure->Format( out, nestLevel+1 );
        
        if( placement )
            placement->Format( out, nestLevel+1 );
        
        if( library )
            library->Format( out, nestLevel+1 );

        if( network )
            network->Format( out, nestLevel+1 );
        
        if( wiring )
            wiring->Format( out, nestLevel+1 );
        
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


class ANCESTOR : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     filename;    
    std::string     comment;
    time_t          time_stamp;

    
public:
    ANCESTOR( ELEM* aParent ) :
        ELEM( T_ancestor, aParent )
    {
        time_stamp = time(NULL);
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        char    temp[80];
        struct  tm* tmp;
        
        tmp = localtime( &time_stamp );
        strftime( temp, sizeof(temp), "%b %d %H : %M : %S %Y", tmp );
        
        // format the time first to temp
        // filename may be empty, so quote it just in case.
        out->Print( nestLevel, "(%s \"%s\" (created_time %s)\n", 
                     LEXER::GetTokenText( Type() ),
                     filename.c_str(),
                     temp );
        
        if( comment.size() )
        {
            const char* quote = out->GetQuoteChar( comment.c_str() );
            out->Print( nestLevel+1, "(comment %s%s%s)\n", 
                       quote, comment.c_str(), quote );
        }
        
        out->Print( nestLevel, ")\n" );
    }
};
typedef boost::ptr_vector<ANCESTOR>     ANCESTORS;


class HISTORY : public ELEM
{
    friend class SPECCTRA_DB;

    ANCESTORS       ancestors;
    time_t          time_stamp;   
    STRINGS         comments;
    
public:

    HISTORY( ELEM* aParent ) :
        ELEM( T_history, aParent )
    {
        time_stamp = time(NULL);
    }
    ~HISTORY()
    {
        ;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( ANCESTORS::iterator i=ancestors.begin();  i!=ancestors.end();  ++i )
            i->Format( out, nestLevel );
        
        char    temp[80];
        struct  tm* tmp;
        
        tmp = localtime( &time_stamp );
        strftime( temp, sizeof(temp), "%b %d %H : %M : %S %Y", tmp );
    
        // format the time first to temp
        out->Print( nestLevel, "(self (created_time %s)\n", temp );
        
        for( STRINGS::iterator i=comments.begin();  i!=comments.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel+1, "(comment %s%s%s)\n", 
                       quote, i->c_str(), quote );
        }
        
        out->Print( nestLevel, ")\n" );
    }
};


class ROUTE : public ELEM
{
    friend class SPECCTRA_DB;

    UNIT_RES*       resolution;
    PARSER*         parser;
    STRUCTURE*      structure;
    LIBRARY*        library;
    NETWORK*        network;
//    TEST_POINTS*    test_points;    

public:
    
    ROUTE( ELEM* aParent ) :
        ELEM( T_route, aParent )
    {
        resolution = 0;
        parser = 0;
        structure = 0;
        library = 0;
        network = 0;
    }
    ~ROUTE()
    {
        delete resolution;
        delete parser;
        delete structure;
        delete library;
        delete network;
//        delete test_points;
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( resolution )
            resolution->Format( out, nestLevel );
        
        if( parser )
            parser->Format( out, nestLevel );
        
        if( structure )
            structure->Format( out, nestLevel );
        
        if( library )
            library->Format( out, nestLevel );
        
        if( network )
            library->Format( out, nestLevel );
        
//        if( test_poinst )
//            test_points->Format( out, nestLevel );
    }
};


/**
 * Struct PIN_PAIR
 * is used within the WAS_IS class below to hold a pair of PIN_REFs and
 * corresponds to the (pins was is) construct within the specctra dsn spec.
 */
struct PIN_PAIR
{
    PIN_PAIR( ELEM* aParent = 0 ) :
        was( aParent ),
        is( aParent )
    {
    }
    
    PIN_REF     was;
    PIN_REF     is;
};
typedef std::vector<PIN_PAIR>   PIN_PAIRS;


/**
 * Class WAS_IS
 * corresponds to the &lt;was_is_descriptor&gt; in the specctra dsn spec.
 */
class WAS_IS : public ELEM
{
    friend class SPECCTRA_DB;

    PIN_PAIRS       pin_pairs;

public:
    WAS_IS( ELEM* aParent ) :
        ELEM( T_was_is, aParent )
    {
    }
    
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( PIN_PAIRS::iterator i=pin_pairs.begin();  i!=pin_pairs.end();  ++i )
        {
            out->Print( nestLevel, "(pins " );
            i->was.Format( out, 0 );
            out->Print( 0, " " );
            i->is.Format( out, 0 );
            out->Print( 0, ")\n" );
        }
    }
};


/**
 * Class SESSION
 * corresponds to the &lt;session_file_descriptor&gt; in the specctra dsn spec.
 */
class SESSION : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     session_id;
    std::string     base_design;
    
    HISTORY*        history;    
    STRUCTURE*      structure;
    PLACEMENT*      placement;
    WAS_IS*         was_is;
    ROUTE*          route;

/*  not supported:
    FLOOR_PLAN*         floor_plan;
    NET_PIN_CHANGES*    net_pin_changes;
    SWAP_HISTORY*       swap_history;
*/

public:
    
    SESSION( ELEM* aParent = 0 ) :
        ELEM( T_pcb, aParent )
    {
        history = 0;
        structure = 0;
        placement = 0;
        was_is = 0;
        route = 0;
    }
    ~SESSION()
    {
        delete history;
        delete structure;
        delete placement;
        delete was_is;
        delete route;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        const char* quote = out->GetQuoteChar( session_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, session_id.c_str(), quote );
        
        out->Print( nestLevel+1, "(base_design \"%s\")\n", base_design.c_str() );
        
        if( history )
            history->Format( out, nestLevel+1 );
        
        if( structure )
            structure->Format( out, nestLevel+1 );
        
        if( placement )
            placement->Format( out, nestLevel+1 );

        if( was_is )
            was_is->Format( out, nestLevel+1 );
        
        if( route )
            route->Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};


/**
 * Class SPECCTRA_DB
 * holds a DSN data tree, usually coming from a DSN file.
 */
class SPECCTRA_DB : public OUTPUTFORMATTER
{
    LEXER*      lexer;
    
    PCB*        pcb;

    SESSION*    session;    

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
     * Function needLEFT
     * calls nextTok() and then verifies that the token read in is a T_LEFT.
     * If it is not, an IOError is thrown.
     * @throw IOError, if the next token is not a T_LEFT
     */
    void needLEFT() throw( IOError );

    /**
     * Function needRIGHT
     * calls nextTok() and then verifies that the token read in is a T_RIGHT.
     * If it is not, an IOError is thrown.
     * @throw IOError, if the next token is not a T_RIGHT
     */
    void needRIGHT() throw( IOError );

    /**
     * Function needSYMBOL
     * calls nextTok() and then verifies that the token read in 
     * satisfies bool isSymbol().
     * If not, an IOError is thrown.
     * @throw IOError, if the next token does not satisfy isSymbol()
     */
    void needSYMBOL() throw( IOError );

    /**
     * Function readCOMPnPIN
     * reads a &lt;pin_reference&gt; and splits it into the two parts which are
     * on either side of the hyphen.  This function is specialized because
     * pin_reference may or may not be using double quotes.  Both of these
     * are legal:  U2-14 or "U2"-"14".  The lexer treats the first one as a 
     * single T_SYMBOL, so in that case we have to split it into two here.
     * <p>
     * The caller should have already read in the first token comprizing the 
     * pin_reference and it will be tested through lexer->CurTok().
     *
     * @param component_id Where to put the text preceeding the '-' hyphen.
     * @param pin_d Where to put the text which trails the '-'.
     * @throw IOError, if the next token or two do no make up a pin_reference,
     * or there is an error reading from the input stream.
     */
    void readCOMPnPIN( std::string* component_id, std::string* pid_id ) throw( IOError );


    /**
     * Function readTIME
     * reads a &lt;time_stamp&gt; which consists of 8 lexer tokens:
     * "month date hour : minute : second year".
     * This function is specialized because time_stamps occur more than
     * once in a session file.
     * <p>
     * The caller should not have already read in the first token comprizing the 
     * time stamp.
     *
     * @param time_stamp Where to put the parsed time value.
     * @throw IOError, if the next token or 8 do no make up a time stamp,
     * or there is an error reading from the input stream.
     */
    void readTIME( time_t* time_stamp ) throw( IOError );

    
    /**
     * Function expecting
     * throws an IOError exception with an input file specific error message.
     * @param DSN_T The token type which was expected at the current input location.
     * @throw IOError with the location within the input file of the problem.
     */
    void expecting( DSN_T ) throw( IOError );
    void expecting( const char* text ) throw( IOError );
    void unexpected( DSN_T aTok ) throw( IOError );
    void unexpected( const char* text ) throw( IOError );
    
    void doPCB( PCB* growth ) throw(IOError);
    void doPARSER( PARSER* growth ) throw(IOError);
    void doRESOLUTION( UNIT_RES* growth ) throw(IOError);
    void doUNIT( UNIT_RES* growth ) throw( IOError );    
    void doSTRUCTURE( STRUCTURE* growth ) throw( IOError );
    void doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth ) throw( IOError );
    void doLAYER_PAIR( LAYER_PAIR* growth ) throw( IOError );    
    void doBOUNDARY( BOUNDARY* growth ) throw( IOError );
    void doRECTANGLE( RECTANGLE* growth ) throw( IOError );
    void doPATH( PATH* growth ) throw( IOError );
    void doSTRINGPROP( STRINGPROP* growth ) throw( IOError );
    void doTOKPROP( TOKPROP* growth ) throw( IOError );
    void doVIA( VIA* growth ) throw( IOError );
    void doCONTROL( CONTROL* growth ) throw( IOError );    
    void doLAYER( LAYER* growth ) throw( IOError );
    void doRULE( RULE* growth ) throw( IOError );
    void doKEEPOUT( KEEPOUT* growth ) throw( IOError );
    void doCIRCLE( CIRCLE* growth ) throw( IOError );
    void doQARC( QARC* growth ) throw( IOError );
    void doWINDOW( WINDOW* growth ) throw( IOError );
    void doREGION( REGION* growth ) throw( IOError );
    void doCLASS_CLASS( CLASS_CLASS* growth ) throw( IOError );    
    void doLAYER_RULE( LAYER_RULE* growth ) throw( IOError );
    void doCLASSES( CLASSES* growth ) throw( IOError );
    void doGRID( GRID* growth ) throw( IOError );
    void doPLACE( PLACE* growth ) throw( IOError );
    void doCOMPONENT( COMPONENT* growth ) throw( IOError );
    void doPLACEMENT( PLACEMENT* growth ) throw( IOError );
    void doPROPERTIES( PROPERTIES* growth ) throw( IOError );
    void doPADSTACK( PADSTACK* growth ) throw( IOError );
    void doSHAPE( SHAPE* growth ) throw( IOError );
    void doIMAGE( IMAGE* growth ) throw( IOError );
    void doLIBRARY( LIBRARY* growth ) throw( IOError );
    void doPIN( PIN* growth ) throw( IOError );
    void doNET( NET* growth ) throw( IOError );
    void doNETWORK( NETWORK* growth ) throw( IOError );
    void doCLASS( CLASS* growth ) throw( IOError );
    void doTOPOLOGY( TOPOLOGY* growth ) throw( IOError );
    void doFROMTO( FROMTO* growth ) throw( IOError );
    void doCOMP_ORDER( COMP_ORDER* growth ) throw( IOError );
    void doWIRE( WIRE* growth ) throw( IOError );
    void doWIRE_VIA( WIRE_VIA* growth ) throw( IOError );
    void doWIRING( WIRING* growth ) throw( IOError );
    void doSESSION( SESSION* growth ) throw( IOError );
    void doANCESTOR( ANCESTOR* growth ) throw( IOError );
    void doHISTORY( HISTORY* growth ) throw( IOError );
    void doROUTE( ROUTE* growth ) throw( IOError );
    void doWAS_IS( WAS_IS* growth ) throw( IOError );    
    
public:

    SPECCTRA_DB()
    {
        lexer = 0;
        pcb   = 0;
        session = 0;
        fp    = 0;
        quote_char += '"';
    }

    ~SPECCTRA_DB()
    {
        delete lexer;
        delete pcb;
        delete session;
        
        if( fp )
            fclose( fp );
    }

    
    //-----<OUTPUTFORMATTER>-------------------------------------------------
    int PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IOError );
    
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
    void SetPCB( PCB* aPcb )
    {
        delete pcb;
        pcb = aPcb;
    }

    /**
     * Function SetSESSION
     * deletes any existing SESSION and replaces it with the given one.
     */
    void SetSESSION( SESSION* aSession )
    {
        delete session;
        session = aSession;
    }
    
    
    /**
     * Function LoadPCB
     * is a recursive descent parser for a SPECCTRA DSN "design" file.
     * A design file is nearly a full description of a PCB (seems to be 
     * missing only the silkscreen stuff).
     *
     * @param filename The name of the dsn file to load.
     * @throw IOError if there is a lexer or parser error. 
     */
    void LoadPCB( const wxString& filename ) throw( IOError );

    
    /**
     * Function LoadSESSION
     * is a recursive descent parser for a SPECCTRA DSN "session" file.
     * A session file is file that is fed back from the router to the layout
     * tool (PCBNEW) and should be used to update a BOARD object with the new
     * tracks, vias, and component locations. 
     *
     * @param filename The name of the dsn file to load.
     * @throw IOError if there is a lexer or parser error. 
     */
    void LoadSESSION( const wxString& filename ) throw( IOError );

    
    void ThrowIOError( const wxChar* fmt, ... ) throw( IOError );
    
    
    /**
     * Function ExportPCB
     * writes the given BOARD out as a SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     * @param aBoard The BOARD to save.
     */
    void ExportPCB( wxString aFilename, BOARD* aBoard );

    
    /**
     * Function ExportSESSION
     * writes the internal session out as a SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     */
    void ExportSESSION( wxString aFilename );
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

void SPECCTRA_DB::expecting( const char* text ) throw( IOError )
{
    wxString    errText( _("Expecting") );
    errText << wxT(" '") << CONV_FROM_UTF8(text) << wxT("'");
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

void SPECCTRA_DB::needLEFT() throw( IOError )
{
    DSN_T tok = nextTok();
    if( tok != T_LEFT )
        expecting( T_LEFT );
}

void SPECCTRA_DB::needRIGHT() throw( IOError )
{
    DSN_T tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}

void SPECCTRA_DB::needSYMBOL() throw( IOError )
{
    DSN_T tok = nextTok();
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );
}


void SPECCTRA_DB::readCOMPnPIN( std::string* component_id, std::string* pin_id ) throw( IOError )
{
    DSN_T tok;
    
    static const char pin_def[] = "<pin_reference>::=<component_id>-<pin_id>"; 
    
    if( !isSymbol( lexer->CurTok() ) )
        expecting( pin_def );

    // case for:  A12-14, i.e. no wrapping quotes.  This should be a single
    // token, so split it.    
    if( lexer->CurTok() != T_STRING )
    {
        const char* toktext = lexer->CurText();
        const char* dash    = strchr( toktext, '-' );
        
        if( !dash )
            expecting( pin_def );
        
        while( toktext != dash )
            *component_id += *toktext++;

        ++toktext;  // skip the dash

        while( *toktext )
            *pin_id += *toktext++;
    }
    
    // quoted string:  "U12"-"14" or "U12"-14,  3 tokens in either case
    else
    {
        *component_id = lexer->CurText();

        tok = nextTok();
        if( tok!=T_DASH )
            expecting( pin_def );
        
        nextTok();          // accept anything after the dash.
        *pin_id = lexer->CurText();
    }
}


void SPECCTRA_DB::readTIME( time_t* time_stamp ) throw( IOError )
{
    DSN_T tok;
    
    std::string     builder;
    
    static const char time_toks[] = "<month> <day> <hour> : <minute> : <second> <year>"; 

    needSYMBOL();       // month
    builder += lexer->CurText();
    builder += ' ';
    
    tok = nextTok();    // day
    if( tok != T_NUMBER )
        expecting( time_toks );
    builder += lexer->CurText();
    builder += ' ';
    
    tok = nextTok();    // hour
    if( tok != T_NUMBER )
        expecting( time_toks );
    builder += lexer->CurText();
    builder += ' ';

    // : colon    
    needSYMBOL();
    if( *lexer->CurText() != ':' || strlen( lexer->CurText() )!=1 )
        expecting( time_toks );
    builder += *lexer->CurText();
    builder += ' ';

    tok = nextTok();    // minute
    if( tok != T_NUMBER )
        expecting( time_toks );
    builder += lexer->CurText();
    builder += ' ';
    
    // : colon    
    needSYMBOL();
    if( *lexer->CurText() != ':' || strlen( lexer->CurText() )!=1 )
        expecting( time_toks );
    builder += *lexer->CurText();
    builder += ' ';

    tok = nextTok();    // second
    if( tok != T_NUMBER )
        expecting( time_toks );
    builder += lexer->CurText();
    builder += ' ';
    
    tok = nextTok();    // year
    if( tok != T_NUMBER )
        expecting( time_toks );
    builder += lexer->CurText();

    struct tm mytime;
    
    if( strptime( builder.c_str(), "%b %d %H : %M : %S %Y", &mytime ) 
       != builder.c_str() + strlen(builder.c_str() ) )
    {
        expecting( time_toks );
    }
    
    *time_stamp = mktime( &mytime ); 
}


void SPECCTRA_DB::LoadPCB( const wxString& filename ) throw( IOError )
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
    
    doPCB( pcb );
}


void SPECCTRA_DB::LoadSESSION( const wxString& filename ) throw( IOError )
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
    
    if( nextTok() != T_session )
        expecting( T_session );

    SetSESSION( new SESSION() );
    
    doSESSION( session );
}


void SPECCTRA_DB::doPCB( PCB* growth ) throw( IOError )
{
    DSN_T tok;

    /*  <design_descriptor >::=
        (pcb <pcb_id >
          [<parser_descriptor> ]
          [<capacitance_resolution_descriptor> ]
          [<conductance_resolution_descriptor> ]
          [<current_resolution_descriptor> ]
          [<inductance_resolution_descriptor> ]
          [<resistance_resolution_descriptor> ]
          [<resolution_descriptor> ]
          [<time_resolution_descriptor> ]
          [<voltage_resolution_descriptor> ]
          [<unit_descriptor> ]
          [<structure_descriptor> | <file_descriptor> ]
          [<placement_descriptor> | <file_descriptor> ]
          [<library_descriptor> | <file_descriptor> ]
          [<floor_plan_descriptor> | <file_descriptor> ]
          [<part_library_descriptor> | <file_descriptor> ]
          [<network_descriptor> | <file_descriptor> ]
          [<wiring_descriptor> ]
          [<color_descriptor> ]
        )
    */
    
    needSYMBOL();
    growth->pcbname = lexer->CurText();    
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_parser:
            if( growth->parser )
                unexpected( tok );
            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;
            
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->resolution )
                unexpected( tok );
            growth->resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->resolution );
            break;
            
        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;

        case T_placement:
            if( growth->placement )
                unexpected( tok );
            growth->placement = new PLACEMENT( growth );
            doPLACEMENT( growth->placement );
            break;
            
        case T_library:
            if( growth->library )
                unexpected( tok );
            growth->library = new LIBRARY( growth );
            doLIBRARY( growth->library );
            break;
            
        case T_network:
            if( growth->network )
                unexpected( tok );
            growth->network = new NETWORK( growth );
            doNETWORK( growth->network );
            break;

        case T_wiring:
            if( growth->wiring )
                unexpected( tok );
            growth->wiring = new WIRING( growth );
            doWIRING( growth->wiring );
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
                expecting( "on|off" );
            lexer->SetSpaceInQuotedTokens( tok==T_on );
            growth->space_in_quoted_tokens = (tok==T_on);
            break;
            
        case T_host_cad:
            needSYMBOL();
            growth->host_cad = lexer->CurText();
            break;
            
        case T_host_version:
            needSYMBOL();
            growth->host_version = lexer->CurText();
            break;

        case T_constant:
            needSYMBOL();
            growth->const_id1 = lexer->CurText();
            needSYMBOL();
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
                    expecting( "testpoint|guides|image_conductor" );
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
                expecting( "on|off" );
            growth->case_sensitive = (tok==T_on);
            break;

        case T_via_rotate_first:    // [(via_rotate_first [on | off])]
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->via_rotate_first = (tok==T_on);
            break;

        case T_generated_by_freeroute:
            growth->generated_by_freeroute = true;
            break;
            
        default:
            unexpected( lexer->CurText() );
        }

        needRIGHT();        
    }
}


void SPECCTRA_DB::doRESOLUTION( UNIT_RES* growth ) throw(IOError)
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
        expecting( "inch|mil|cm|mm|um" );
    }
    
    tok = nextTok();
    if( tok != T_NUMBER )
        expecting( T_NUMBER );

    growth->value = atoi( lexer->CurText() );
    
    needRIGHT();
}


void SPECCTRA_DB::doUNIT( UNIT_RES* growth ) throw(IOError)
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
        expecting( "inch|mil|cm|mm|um" );
    }
    
    needRIGHT();
}


void SPECCTRA_DB::doLAYER_PAIR( LAYER_PAIR* growth ) throw( IOError )
{
    needSYMBOL();
    growth->layer_id0 = lexer->CurText();

    needSYMBOL();
    growth->layer_id1 = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->layer_weight = strtod( lexer->CurText(), 0 );

    needRIGHT();
}


void SPECCTRA_DB::doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth ) throw( IOError )
{
    DSN_T   tok;
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        if( nextTok() != T_layer_pair )
            expecting( T_layer_pair );
        
        LAYER_PAIR* layer_pair = new LAYER_PAIR( growth );
        growth->layer_pairs.push_back( layer_pair );
        doLAYER_PAIR( layer_pair );
    }
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
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->unit );
            break;

        case T_layer_noise_weight:
            growth->layer_noise_weight = new LAYER_NOISE_WEIGHT( growth );
            doLAYER_NOISE_WEIGHT( growth->layer_noise_weight );
            break;            
            
        case T_place_boundary:
L_place:            
            if( growth->place_boundary )
                unexpected( tok );
            growth->place_boundary = new BOUNDARY( growth, T_place_boundary );
            doBOUNDARY( growth->place_boundary );
            break;
            
        case T_boundary:
            if( growth->boundary )
            {
                if( growth->place_boundary )
                    unexpected( tok );
                goto L_place;
            }
            growth->boundary = new BOUNDARY( growth );
            doBOUNDARY( growth->boundary );
            break;

        case T_plane:
            PLANE* plane;
            plane = new PLANE( growth );
            growth->planes.push_back( plane );
            doKEEPOUT( plane );
            break;

        case T_region:
            REGION* region;
            region = new REGION( growth );
            growth->regions.push_back( region );
            doREGION( region );
            break;
            
        case T_snap_angle:
            STRINGPROP* stringprop;
            stringprop = new STRINGPROP( growth, T_snap_angle ); 
            growth->Append( stringprop );
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

        case T_layer:
            LAYER* layer;
            layer = new LAYER( growth );
            growth->layers.push_back( layer );
            doLAYER( layer );
            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_place_rule:
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;            
            
        case T_keepout:
/* @todo            
        case T_place_keepout:
        case T_via_keepout:
        case T_wire_keepout:
        case T_bend_keepout:
        case T_elongate_keepout:
*/
            KEEPOUT* keepout;
            keepout = new KEEPOUT( growth, tok );
            growth->keepouts.push_back( keepout );
            doKEEPOUT( keepout );
            break;

        case T_grid:
            GRID* grid;
            grid = new GRID( growth );
            growth->grids.push_back( grid );
            doGRID( grid );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doKEEPOUT( KEEPOUT* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok==T_SYMBOL || tok==T_STRING )
    {
        growth->name = lexer->CurText();
        tok = nextTok();
    }
    
    if( tok!=T_LEFT )    
        expecting( T_LEFT );

    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        
        switch( tok )
        {
        case T_sequence_number:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->sequence_number = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;
            
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
            
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;
            
        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;

        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
    }
}


void SPECCTRA_DB::doWINDOW( WINDOW* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    while( tok != T_RIGHT )
    {
        if( tok!=T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
            
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
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
        if( growth->paths.size() )
            unexpected( "rect when path already encountered" );
    
        growth->rectangle = new RECTANGLE( growth );
        doRECTANGLE( growth->rectangle );
        needRIGHT();
    }
    else if( tok == T_path )
    {
        if( growth->rectangle )
            unexpected( "path when rect already encountered" );

        for(;;)
        {
            if( tok != T_path )
                expecting( T_path );
                    
            PATH* path = new PATH( growth, T_path ) ;
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
        expecting( "rect|path" );
}


void SPECCTRA_DB::doPATH( PATH* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    if( !isSymbol( tok ) )
        expecting( "layer_id" );
    
    growth->layer_id = lexer->CurText();

    if( nextTok() != T_NUMBER )
        expecting( "aperture_width" );
    
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
            expecting( "round|square" );

        growth->aperture_type = tok;
        
        needRIGHT();
    }
}


void SPECCTRA_DB::doRECTANGLE( RECTANGLE* growth ) throw( IOError )
{
    needSYMBOL();
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

    needRIGHT();
}


void SPECCTRA_DB::doCIRCLE( CIRCLE* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needSYMBOL();
    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->diameter = strtod( lexer->CurText(), 0 );
    
    tok = nextTok();
    if( tok == T_NUMBER )
    {
        growth->vertex.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex.y = strtod( lexer->CurText(), 0 );
        
        tok = nextTok();
    }

    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doQARC( QARC* growth ) throw( IOError )
{
    needSYMBOL();
    growth->layer_id = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->aperture_width = strtod( lexer->CurText(), 0 );
    
    for( int i=0;  i<3;  ++i )
    {
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex[i].x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->vertex[i].y = strtod( lexer->CurText(), 0 );
    }

    needRIGHT();    
}


void SPECCTRA_DB::doSTRINGPROP( STRINGPROP* growth ) throw( IOError )
{
    needSYMBOL();
    growth->value = lexer->CurText();
    needRIGHT();
}


void SPECCTRA_DB::doTOKPROP( TOKPROP* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok<0 )
        unexpected( lexer->CurText() );

    growth->value = tok;

    needRIGHT();    
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
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_via_at_smd:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->via_at_smd = (tok==T_on);
            needRIGHT();
            break;
            
        case T_off_grid:
        case T_route_to_fanout_only:
        case T_force_to_terminal_point:
        case T_same_net_checking:
        case T_checking_trim_by_pin:
        case T_noise_calculation:
        case T_noise_accumulation:
        case T_include_pins_in_crosstalk:
        case T_bbv_ctr2ctr:
        case T_average_pair_length:
        case T_crosstalk_model:
        case T_roundoff_rotation:
        case T_microvia:
        case T_reroute_order_viols:
            TOKPROP* tokprop;
            tokprop = new TOKPROP( growth, tok ) ;
            growth->Append( tokprop );
            doTOKPROP( tokprop );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doPROPERTIES( PROPERTIES* growth ) throw( IOError )
{
    DSN_T       tok;
    PROPERTY    property;  // construct it once here, append multiple times.

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        needSYMBOL();
        property.name = lexer->CurText();

        needSYMBOL();        
        property.value = lexer->CurText();
        
        growth->push_back( property );

        needRIGHT();        
    }
}


void SPECCTRA_DB::doLAYER( LAYER* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol(tok) )
        expecting(T_SYMBOL);

    growth->name = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_type:
            tok = nextTok();
            if( tok!=T_signal && tok!=T_power && tok!=T_mixed && tok!=T_jumper )
                expecting( "signal|power|mixed|jumper" );
            growth->layer_type = tok;
            if( nextTok()!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_property:
            doPROPERTIES( &growth->properties );
            break;
            
        case T_direction:
            tok = nextTok();
            switch( tok )
            {
            case T_horizontal:
            case T_vertical:
            case T_orthogonal:
            case T_positive_diagonal:
            case T_negative_diagonal:
            case T_diagonal:
            case T_off:
                growth->direction = tok;
                break;
            default:
                expecting( "horizontal|vertical|orthogonal|positive_diagonal|negative_diagonal|diagonal|off" );
            }
            if( nextTok()!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_cost:
            tok = nextTok();
            switch( tok )
            {
            case T_forbidden:
            case T_high:
            case T_medium:
            case T_low:
            case T_free:
                growth->cost = tok;
                break;
            case T_NUMBER:
                // store as negative so we can differentiate between 
                // DSN_T (positive) and T_NUMBER (negative)
                growth->cost = -atoi( lexer->CurText() );   
                break;
            default:
                expecting( "forbidden|high|medium|low|free|<positive_integer>|-1" );
            }
            tok = nextTok();
            if( tok == T_LEFT )
            {
                if( nextTok() != T_type )
                    unexpected( lexer->CurText() );
                
                tok = nextTok();
                if( tok!=T_length && tok!=T_way )
                    expecting( "length|way" );
                
                growth->cost_type = tok;
                if( nextTok()!=T_RIGHT )
                    expecting(T_RIGHT);
                
                tok = nextTok();
            }
            if( tok!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_use_net:
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( !isSymbol(tok) )
                    expecting( T_SYMBOL );
                
                growth->use_net.push_back( lexer->CurText() );
            }
            break;
            
        default:
            unexpected( lexer->CurText() );            
        }
    }
}


void SPECCTRA_DB::doRULE( RULE* growth ) throw( IOError )
{
    std::string     builder;
    int             bracketNesting = 1; // we already saw the opening T_LEFT
    DSN_T           tok = T_NONE;

    while( bracketNesting!=0 && tok!=T_EOF )
    {
        tok = nextTok();
        
        if( tok==T_LEFT)
            ++bracketNesting;
        
        else if( tok==T_RIGHT )
            --bracketNesting;

        if( bracketNesting >= 1 )
        {
            if( lexer->PrevTok()!=T_LEFT && tok!=T_RIGHT && (tok!=T_LEFT || bracketNesting>2) )
                builder += ' ';

            if( tok==T_STRING )
                builder += quote_char;
            
            builder += lexer->CurText();
            
            if( tok==T_STRING )
                builder += quote_char;
        }

        // When the nested rule is closed with a T_RIGHT and we are back down
        // to bracketNesting == 1, (inside the <rule_descriptor> but outside
        // the last rule).  Then save the last rule and clear the string builder.
        if( bracketNesting == 1 )
        {
           growth->rules.push_back( builder );
           builder.clear();
        }
    }
    
    if( tok==T_EOF )
        unexpected( T_EOF );
}


#if 0
void SPECCTRA_DB::doPLACE_RULE( PLACE_RULE* growth, bool expect_object_type ) throw( IOError )
{
    /*   (place_rule [<structure_place_rule_object> ]
         {[<spacing_descriptor> |
         <permit_orient_descriptor> |
         <permit_side_descriptor> |
         <opposite_side_descriptor> ]}
         )
    */
    
    DSN_T   tok = nextTok();
    
    if( tok!=T_LEFT )
        expecting( T_LEFT );
    
    tok = nextTok();
    if( tok==T_object_type )
    {
        if( !expect_object_type )
            unexpected( tok );
        
        /*  [(object_type
              [pcb |
              image_set [large | small | discrete | capacitor | resistor]
              [(image_type [smd | pin])]]
            )]
        */
        
        tok = nextTok();
        switch( tok )
        {
        case T_pcb:
            growth->object_type = tok;
            break;
            
        case T_image_set:
            tok = nextTok();
            switch( tok )
            {
            case T_large:
            case T_small:
            case T_discrete:
            case T_capacitor:
            case T_resistor:
                growth->object_type = tok;
                break;
            default:
                unexpected( lexer->CurText() );
            }
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
        if( tok == T_LEFT )
        {
            tok = nextTok();
            if( tok != T_image_type )
                expecting( T_image_type );
            
            tok = nextTok();
            if( tok!=T_smd && tok!=T_pin )
                expecting( "smd|pin" );
            
            needRIGHT();
            
            tok = nextTok();
        }
        
        if( tok != T_RIGHT )
            expecting( T_RIGHT );
        
        tok = nextTok();
    }

    /*  {[<spacing_descriptor> | 
        <permit_orient_descriptor> | 
        <permit_side_descriptor> | <opposite_side_descriptor> ]}
    */
    doRULE( growth );
}
#endif


void SPECCTRA_DB::doREGION( REGION* growth ) throw( IOError )
{
    DSN_T tok = nextTok();
    
    if( isSymbol(tok) )
    {
        growth->region_id = lexer->CurText();
        tok = nextTok();
    }

    for(;;)
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_polygon:
            growth->polygon = new PATH( growth, T_polygon );
            doPATH( growth->polygon );
            break;
            
        case T_region_net:
        case T_region_class:
            STRINGPROP* stringprop;
            stringprop = new STRINGPROP( growth, tok );
            growth->Append( stringprop );
            doSTRINGPROP( stringprop );
            break;            

        case T_region_class_class:
            CLASS_CLASS* class_class;
            class_class = new CLASS_CLASS( growth, tok );
            growth->Append( class_class );
            doCLASS_CLASS( class_class ); 
            break;
            
        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }

        tok = nextTok();
        if( tok == T_RIGHT )
        {
            if( !growth->rules )
                expecting( T_rule );
            break;
        }
    }
}


void SPECCTRA_DB::doCLASS_CLASS( CLASS_CLASS* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok != T_LEFT )
        expecting( T_LEFT );
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        switch( tok )
        {
        case T_classes:
            if( growth->classes )
                unexpected( tok );
            growth->classes = new CLASSES( growth );
            doCLASSES( growth->classes );
            break;
            
        case T_rule:
            // only T_class_class takes a T_rule
            if( growth->Type() == T_region_class_class )
                unexpected( tok );
            RULE* rule;
            rule = new RULE( growth, T_rule );
            growth->Append( rule );
            doRULE( rule );
            break;
            
        case T_layer_rule:
            // only T_class_class takes a T_layer_rule
            if( growth->Type() == T_region_class_class )
                unexpected( tok );
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->Append( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        default:            
            unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCLASSES( CLASSES* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    // require at least 2 class_ids
    
    if( !isSymbol( tok ) )
        expecting( "class_id" );
    
    growth->class_ids.push_back( lexer->CurText() );
    
    do
    {
        tok = nextTok();
        if( !isSymbol( tok ) )
            expecting( "class_id" );
        
        growth->class_ids.push_back( lexer->CurText() );
        
    } while( (tok = nextTok()) != T_RIGHT );
}


void SPECCTRA_DB::doGRID( GRID* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    switch( tok )
    {
    case T_via:
    case T_wire:
    case T_via_keepout:
    case T_snap:
    case T_place:
        growth->grid_type = tok;
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->dimension = strtod( lexer->CurText(), 0 );
        tok = nextTok();
        if( tok == T_LEFT )
        {
            while( (tok=nextTok()) != T_RIGHT )
            {
                if( tok==T_direction )
                {
                    if( growth->grid_type == T_place )
                        unexpected( tok );
                    tok = nextTok();
                    if( tok!=T_x && tok!=T_y )
                        unexpected( lexer->CurText() );
                    growth->direction = tok;
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
                else if( tok==T_offset )
                {
                    if( growth->grid_type == T_place )
                        unexpected( tok );
                    
                    if( nextTok() != T_NUMBER )
                        expecting( T_NUMBER );
                    
                    growth->offset = strtod( lexer->CurText(), 0 );
                    
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
                else if( tok==T_image_type )
                {
                    if( growth->grid_type != T_place )
                        unexpected( tok );
                    tok = nextTok();
                    if( tok!=T_smd && tok!=T_pin )
                        unexpected( lexer->CurText() );
                    growth->image_type = tok;
                    if( nextTok() != T_RIGHT )
                        expecting(T_RIGHT);
                }
            }
        }
        break;

    default:
        unexpected( tok );
    }
}


void SPECCTRA_DB::doLAYER_RULE( LAYER_RULE* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needSYMBOL();
    
    do
    {
        growth->layer_ids.push_back( lexer->CurText() );
        
    }  while( isSymbol(tok = nextTok()) );
 
    if( tok != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_rule )
        expecting( T_rule );
    
    growth->rule = new RULE( growth, T_rule );
    doRULE( growth->rule );
    
    needRIGHT();
}


void SPECCTRA_DB::doPLACE( PLACE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( "component_id" );
    
    growth->component_id = lexer->CurText();    
    
    tok = nextTok();
    if( tok == T_NUMBER )
    {
        POINT   point;
        
        point.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        point.y = strtod( lexer->CurText(), 0 );
        
        growth->SetVertex( point );
        
        tok = nextTok();
        if( tok!=T_front && tok!=T_back )
            expecting( "front|back" );
        growth->side = tok;
        
        if( nextTok() != T_NUMBER )
            expecting( "rotation" );
        growth->SetRotation( strtod( lexer->CurText(), 0)  );
    }

    while( (tok = nextTok()) != T_RIGHT )
    {
        switch( tok )
        {
        case T_mirror:
            tok = nextTok();
            if( tok==T_x || tok==T_y || tok==T_xy || tok==T_off )
                growth->mirror = tok;
            else
                expecting("x|y|xy|off");
            break;
           
        case T_status:
            tok = nextTok();
            if( tok==T_added || tok==T_deleted || tok==T_substituted )
                growth->status = tok;
            else
                expecting("added|deleted|substituted");
            break;
            
        case T_logical_part:
            if( growth->logical_part.size() )
                unexpected( tok );
            tok = nextTok();
            if( !isSymbol( tok ) )
                expecting( "logical_part_id");
            growth->logical_part = lexer->CurText();
            break;
            
        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, T_place_rule );
            doRULE( growth->place_rules );
            break;
            
        case T_property:
            if( growth->properties.size() )
                unexpected( tok );
            doPROPERTIES( &growth->properties );
            break;
            
        case T_lock_type:
            tok = nextTok();
            if( tok==T_position || tok==T_gate || tok==T_subgate || tok==T_pin )
                growth->lock_type = tok;
            else
                expecting("position|gate|subgate|pin");
            break;

        case T_rule:
            if( growth->rules || growth->region )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_region:
            if( growth->rules || growth->region )
                unexpected( tok );
            growth->region = new REGION( growth );
            doREGION( growth->region );
            break;

        case T_pn:
            if( growth->part_number.size() )
                unexpected( tok );
            growth->part_number = lexer->CurText();
            break;
            
        default:
            unexpected( tok );
        }
    }
}


void SPECCTRA_DB::doCOMPONENT( COMPONENT* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( "image_id" );
    growth->image_id = lexer->CurText();

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_place:
            PLACE* place;
            place = new PLACE( growth );
            growth->places.push_back( place );
            doPLACE( place );
            break;
            
        default:
            unexpected(tok);
        }
    }
}


void SPECCTRA_DB::doPLACEMENT( PLACEMENT* growth ) throw( IOError )
{
    DSN_T   tok;
    
    needLEFT();
    
    tok = nextTok();
    if( tok==T_unit || tok==T_resolution )
    {
        growth->unit = new UNIT_RES( growth, tok );
        if( tok==T_resolution )
            doRESOLUTION( growth->unit );
        else
            doUNIT( growth->unit );
        
        if( nextTok() != T_LEFT )
            expecting( T_LEFT );
        tok = nextTok();
    }
    
    if( tok == T_place_control )
    {
        if( nextTok() != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        if( tok != T_flip_style )
            expecting( T_flip_style );
        
        tok = nextTok();
        if( tok==T_mirror_first || tok==T_rotate_first )
            growth->flip_style = tok;
        else
            expecting("mirror_first|rotate_first");

        needRIGHT();
        needRIGHT();
        needLEFT();        
        tok = nextTok();
    }

    while( tok == T_component )
    {
        COMPONENT* component = new COMPONENT( growth );
        growth->components.push_back( component );
        doCOMPONENT( component );
        
        tok = nextTok();
        if( tok == T_RIGHT )
            return;

        else if( tok == T_LEFT )        
            tok = nextTok();
    }
    
    unexpected( lexer->CurText() );
}


void SPECCTRA_DB::doPADSTACK( PADSTACK* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  (padstack <padstack_id >
        [<unit_descriptor> ]
        {(shape <shape_descriptor>
            [<reduced_shape_descriptor> ]
            [(connect [on | off])]
            [{<window_descriptor> }]
        )}
        [<attach_descriptor> ]
        [{<pad_via_site_descriptor> }]
        [(rotate [on | off])]
        [(absolute [on | off])]
        [(rule <clearance_descriptor> )])
    */
    
    if( !isSymbol( tok ) )
        expecting( "padstack_id" );
    
    growth->padstack_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;

        case T_rotate:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->rotate = tok;
            needRIGHT();
            break;

        case T_absolute:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->absolute = tok;
            needRIGHT();
            break;
            
        case T_shape:
            SHAPE* shape;
            shape = new SHAPE( growth );
            growth->Append( shape );
            doSHAPE( shape );
            break;

        case T_attach:
            tok = nextTok();
            if( tok!=T_off && tok!=T_on )
                expecting( "off|on" );
            growth->attach = tok;
            tok = nextTok();
            if( tok == T_LEFT )
            {
                if( nextTok() != T_use_via )
                    expecting( T_use_via );
                
                needSYMBOL();
                growth->via_id = lexer->CurText();

                needRIGHT();
                needRIGHT();                
            }
            break;
            
        /*
        case T_via_site:        not supported
            break;
        */            
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doSHAPE( SHAPE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  (shape <shape_descriptor>
         [<reduced_shape_descriptor> ]
         [(connect [on | off])]
         [{<window_descriptor> }])
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
            if( growth->rectangle || growth->circle || growth->path || growth->qarc )
                unexpected( tok );
        default: ;
        }
        
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
        
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;

        case T_connect:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( "on|off" );
            growth->connect = tok;
            needRIGHT();
            break;

        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doIMAGE( IMAGE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  <image_descriptor >::=
        (image <image_id >
           [(side [front | back | both])]
           [<unit_descriptor> ]
           [<outline_descriptor> ]
           {(pin <padstack_id > [(rotate <rotation> )]
              [<reference_descriptor> | <pin_array_descriptor> ]
              [<user_property_descriptor> ])}
           [{<conductor_shape_descriptor> }]
           [{<conductor_via_descriptor> }]
           [<rule_descriptor> ]
           [<place_rule_descriptor> ]
           [{<keepout_descriptor> }]
        [<image_property_descriptor> ]
        )
    */

    if( !isSymbol( tok ) )
        expecting( "image_id" );
    
    growth->image_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;

        case T_side:
            tok = nextTok();
            if( tok!=T_front && tok!=T_back && tok!=T_both )
                expecting( "front|back|both" );
            growth->side = tok;
            needRIGHT();
            break;

        case T_outline:
            SHAPE* outline;
            outline = new SHAPE( growth, T_outline );   // use SHAPE for T_outline
            growth->Append( outline );
            doSHAPE( outline );
            break;

        case T_pin:
            PIN* pin;
            pin = new PIN( growth );
            growth->pins.push_back( pin );
            doPIN( pin );
            break;
            
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, tok );
            doRULE( growth->rules );
            break;

        case T_place_rule:
            if( growth->place_rules )
                unexpected( tok );
            growth->place_rules = new RULE( growth, tok );
            doRULE( growth->place_rules );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doPIN( PIN* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  (pin <padstack_id > [(rotate <rotation> )]
          [<reference_descriptor> | <pin_array_descriptor> ]
          [<user_property_descriptor> ])
    */

    if( !isSymbol( tok ) )
        expecting( "padstack_id" );
    
    growth->padstack_id = lexer->CurText();
    
    tok = nextTok();
    if( tok == T_LEFT )
    {
        tok = nextTok();
        if( tok != T_rotate )
            expecting( T_rotate );
        
        if( nextTok() != T_NUMBER )
            expecting( T_NUMBER );
        growth->SetRotation( strtod( lexer->CurText(), 0 ) );
        needRIGHT();
        tok = nextTok();
    }
    
    if( !isSymbol(tok) && tok!=T_NUMBER )
        expecting( "pin_id" );

    growth->pin_id = lexer->CurText();

    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->vertex.x = strtod( lexer->CurText(), 0 );
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->vertex.x = strtod( lexer->CurText(), 0 );

    if( nextTok() != T_RIGHT )
        unexpected( lexer->CurText() );
}


void SPECCTRA_DB::doLIBRARY( LIBRARY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <library_descriptor >::=
        (library
           [<unit_descriptor> ]
           {<image_descriptor> }
           [{<jumper_descriptor> }]
           {<padstack_descriptor> }
           {<via_array_template_descriptor> }
           [<directory_descriptor> ]
           [<extra_image_directory_descriptor> ]
           [{<family_family_descriptor> }]
           [{<image_image_descriptor> }]
        )
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unit:
            if( growth->unit )
                unexpected( tok );
            growth->unit = new UNIT_RES( growth, tok );
            doUNIT( growth->unit );
            break;
            
        case T_padstack:
            PADSTACK* padstack;
            padstack = new PADSTACK( growth );
            growth->padstacks.push_back( padstack );
            doPADSTACK( padstack );
            break;

        case T_image:
            IMAGE*  image;
            image = new IMAGE( growth );
            growth->images.push_back( image );
            doIMAGE( image );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doNET( NET* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    /*  <net_descriptor >::=
        (net <net_id >
          [(unassigned)]
          [(net_number <integer >)]
          [(pins {<pin_reference> }) | (order {<pin_reference> })]
          [<component_order_descriptor> ]
          [(type [fix | normal])]
          [<user_property_descriptor> ]
          [<circuit_descriptor> ]
          [<rule_descriptor> ]
          [{<layer_rule_descriptor> }]
          [<fromto_descriptor> ]
          [(expose {<pin_reference> })]
          [(noexpose {<pin_reference> })]
          [(source {<pin_reference> })]
          [(load {<pin_reference> })]
          [(terminator {<pin_reference> })]
          [(supply [power | ground])]
        )
    */

    if( !isSymbol( tok ) )
        expecting( "net_id" );
    
    growth->net_id = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_unassigned:
            growth->unassigned = true;
            needRIGHT();
            break;

        case T_net_number:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->net_number = atoi( lexer->CurText() );
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            break;

        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_normal )
                expecting( "fix|normal" );
            growth->type = tok;
            needRIGHT();
            break;

        case T_pins:
        case T_order:
            growth->pins_type = tok;
            {
                PIN_REF     empty( growth );
                while( (tok = nextTok()) != T_RIGHT )
                {
                    // copy the empty one, then fill its copy later thru pin_ref.                
                    growth->pins.push_back( empty );

                    PIN_REF* pin_ref = &growth->pins.back();

                    readCOMPnPIN( &pin_ref->component_id, &pin_ref->pin_id );
                }
            }
            break;

        case T_fromto:
            if( growth->fromto )
                unexpected( tok );
            growth->fromto = new FROMTO( growth );
            doFROMTO( growth->fromto );
            break;

        case T_comp_order:
            if( growth->comp_order )
                unexpected( tok );
            growth->comp_order = new COMP_ORDER( growth );
            doCOMP_ORDER( growth->comp_order );
            break;
            
        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doTOPOLOGY( TOPOLOGY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <topology_descriptor >::=
        (topology {[<fromto_descriptor> |
        <component_order_descriptor> ]})
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_fromto:
            FROMTO* fromto;
            fromto = new FROMTO( growth );
            growth->fromtos.push_back( fromto );
            doFROMTO( fromto );
            break;
        
        case T_comp_order:
            COMP_ORDER*  comp_order;
            comp_order = new COMP_ORDER( growth );
            growth->comp_orders.push_back( comp_order );
            doCOMP_ORDER( comp_order );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doCLASS( CLASS* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <class_descriptor >::=
        (class
           <class_id > {[{<net_id >} | {<composite_name_list> }]}
           [<circuit_descriptor> ]
           [<rule_descriptor> ]
           [{<layer_rule_descriptor> }]
           [<topology_descriptor> ]
        )
    */

    needSYMBOL();
    
    growth->class_id = lexer->CurText();

    // do net_ids, do not support <composite_name_list>s at this time
    while( isSymbol(tok = nextTok()) )
    {
        growth->net_ids.push_back( lexer->CurText() );
    }
    
    
    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;
            
        case T_topology:
            if( growth->topology )
                unexpected( tok );
            growth->topology = new TOPOLOGY( growth );
            doTOPOLOGY( growth->topology );
            break;
            
        default:    // handle all the circuit_descriptor here as strings
            {
                std::string     builder;
                int             bracketNesting = 1; // we already saw the opening T_LEFT
                DSN_T           tok = T_NONE;
            
                builder += '(';
                builder += lexer->CurText();
                
                while( bracketNesting!=0 && tok!=T_EOF )
                {
                    tok = nextTok();
                    
                    if( tok==T_LEFT)
                        ++bracketNesting;
                    
                    else if( tok==T_RIGHT )
                        --bracketNesting;
            
                    if( bracketNesting >= 1 )
                    {
                        if( lexer->PrevTok() != T_LEFT && tok!=T_RIGHT )
                            builder += ' ';
            
                        if( tok==T_STRING )
                            builder += quote_char;
                        
                        builder += lexer->CurText();
                        
                        if( tok==T_STRING )
                            builder += quote_char;
                    }
            
                    // When the nested rule is closed with a T_RIGHT and we are back down
                    // to bracketNesting == 0, then save the builder and break;
                    if( bracketNesting == 0 )
                    {
                        builder += ')';
                       growth->circuit.push_back( builder );
                       break;
                    }
                }
                
                if( tok==T_EOF )
                    unexpected( T_EOF );
            }                                   // scope bracket
        }                                       // switch
        
        tok = nextTok();
        
    } // while
}


void SPECCTRA_DB::doNETWORK( NETWORK* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <network_descriptor >::=
        (network
          {<net_descriptor>}
          [{<class_descriptor> }]
          [{<class_class_descriptor> }]
          [{<group_descriptor> }]
          [{<group_set_descriptor> }]
          [{<pair_descriptor> }]
          [{<bundle_descriptor> }]
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_net:
            NET* net;
            net = new NET( growth );
            growth->nets.push_back( net );
            doNET( net );
            break;
        
        case T_class:
            CLASS*  myclass;
            myclass = new CLASS( growth );
            growth->classes.push_back( myclass );
            doCLASS( myclass );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doCOMP_ORDER( COMP_ORDER* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <component_order_descriptor >::=
        (comp_order {<placement_id> })
    */

    while( isSymbol(tok = nextTok()) )
    {
        growth->placement_ids.push_back( lexer->CurText() );
    }
    
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doFROMTO( FROMTO* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <fromto_descriptor >::=
        {(fromto
           [<pin_reference> | <virtual_pin_descriptor> ] | <component_id >]
           [<pin_reference> | <virtual_pin_descriptor> | <component_id >]
           [(type [fix | normal | soft])]
           [(net <net_id >)]
           [<rule_descriptor> ]
           [<circuit_descriptor> ]
           [{<layer_rule_descriptor> }]
        )}
    */

    
    // read the first two grammar items in as 2 single tokens, i.e. do not
    // split apart the <pin_reference>s into 3 separate tokens.  Do this by
    // turning off the string delimiter in the lexer.
    
    int old = lexer->SetStringDelimiter( 0 );
    
    if( !isSymbol(nextTok() ) )
    {
        lexer->SetStringDelimiter( old );
        expecting( T_SYMBOL );
    }
    growth->fromText = lexer->CurText();
    
    if( !isSymbol(nextTok() ) )
    {
        lexer->SetStringDelimiter( old );
        expecting( T_SYMBOL );
    }
    growth->toText = lexer->CurText();

    lexer->SetStringDelimiter( old );
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_normal && tok!=T_soft )
                expecting( "fix|normal|soft" );
            growth->fromto_type = tok;
            needRIGHT();
            break;
        
        case T_rule:
            if( growth->rules )
                unexpected( tok );
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;

        case T_layer_rule:
            LAYER_RULE* layer_rule;
            layer_rule = new LAYER_RULE( growth );
            growth->layer_rules.push_back( layer_rule );
            doLAYER_RULE( layer_rule );
            break;

        case T_net:
            if( growth->net_id.size() )
                unexpected( tok );
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        // circuit descriptor not supported at this time
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE( WIRE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <wire_shape_descriptor >::=
        (wire
          <shape_descriptor>
          [(net <net_id >)]
          [(turret <turret#> )]
          [(type [fix | route | normal | protect])]
          [(attr [test | fanout | bus | jumper])]
          [(shield <net_id >)]
          [{<window_descriptor> }]
          [(connect
             (terminal <object_type> [<pin_reference> ])
             (terminal <object_type> [<pin_reference> ])
          )]
          [(supply)]
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_rect:
        case T_circle:
        case T_path:
        case T_polygon:
        case T_qarc:
            if( growth->rectangle || growth->circle || growth->path || growth->qarc )
                unexpected( tok );
        default: ;
        }
        
        switch( tok )
        {
        case T_rect:
            growth->rectangle = new RECTANGLE( growth );
            doRECTANGLE( growth->rectangle );
            break;
            
        case T_circle:
            growth->circle = new CIRCLE( growth );
            doCIRCLE( growth->circle );
            break;
        
        case T_path:
        case T_polygon:
            growth->path = new PATH( growth, tok );
            doPATH( growth->path );
            break;
            
        case T_qarc:
            growth->qarc = new QARC( growth );
            doQARC( growth->qarc );
            break;

        case T_net:
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        case T_turret:
            if( nextTok() != T_NUMBER )
                expecting( T_NUMBER );
            growth->turret = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_route && tok!=T_normal && tok!=T_protect )
                expecting( "fix|route|normal|protect" );
            growth->type = tok;
            needRIGHT();
            break;

        case T_attr:
            tok = nextTok();
            if( tok!=T_test && tok!=T_fanout && tok!=T_bus && tok!=T_jumper )
                expecting( "test|fanout|bus|jumper" );
            growth->attr = tok;
            needRIGHT();
            break;

        case T_shield:
            needSYMBOL();
            growth->shield = lexer->CurText();
            needRIGHT();
            break;
            
        case T_window:
            WINDOW* window;
            window = new WINDOW( growth );
            growth->windows.push_back( window );
            doWINDOW( window );
            break;
            
        case T_connect:
            if( growth->connect )
                unexpected( tok );
/* @todo            
            growth->connect = new CONNECT( growth );
            doCONNECT( growth->connect );
*/            
            break;
            
        case T_supply:
            growth->supply = true;
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWIRE_VIA( WIRE_VIA* growth ) throw( IOError )
{
    DSN_T   tok;
    POINT   point;
    
    /*  <wire_via_descriptor >::=
        (via
           <padstack_id > {<vertex> }
           [(net <net_id >)]
           [(via_number <via#> )]
           [(type [fix | route | normal | protect])]
           [(attr [test | fanout | jumper |
              virtual_pin <virtual_pin_name> ])]
           [(contact {<layer_id >})]
           [(supply)]
        )
        (virtual_pin
           <virtual_pin_name> <vertex> (net <net_id >)
        )
    */

    needSYMBOL();
    growth->padstack_id = lexer->CurText();

    while( (tok = nextTok()) == T_NUMBER )
    {
        point.x = strtod( lexer->CurText(), 0 );
        
        if( nextTok() != T_NUMBER )
            expecting( "vertex.y" );
        
        point.y = strtod( lexer->CurText(), 0 );
        
        growth->vertexes.push_back( point );
    }

    while( tok != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_net:
            needSYMBOL();
            growth->net_id = lexer->CurText();
            needRIGHT();
            break;
            
        case T_via_number:
            if( nextTok() != T_NUMBER )
                expecting( "<via#>" );
            growth->via_number = atoi( lexer->CurText() );
            needRIGHT();
            break;
            
        case T_type:
            tok = nextTok();
            if( tok!=T_fix && tok!=T_route && tok!=T_normal && tok!=T_protect )
                expecting( "fix|route|normal|protect" );
            growth->type = tok;
            needRIGHT();
            break;
            
        case T_attr:
            tok = nextTok();
            if( tok!=T_test && tok!=T_fanout && tok!=T_jumper && tok!=T_virtual_pin )
                expecting( "test|fanout|jumper|virtual_pin" );
            growth->attr = tok;
            if( tok == T_virtual_pin )
            {
                needSYMBOL();
                growth->virtual_pin_name = lexer->CurText();
            }
            needRIGHT();
            break;
            
        case T_contact:
            needSYMBOL();
            tok = T_SYMBOL;
            while( isSymbol(tok) )
            {
                growth->contact_layers.push_back( lexer->CurText() );
                tok = nextTok();
            }
            if( tok != T_RIGHT )
                expecting( T_RIGHT );
            break;
            
        case T_supply:
            growth->supply = true;
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
        
        tok = nextTok();
    }
}


void SPECCTRA_DB::doWIRING( WIRING* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <wiring_descriptor >::=
        (wiring
          [<unit_descriptor> | <resolution_descriptor> | null]
          {<wire_descriptor> }
          [<test_points_descriptor> ]
          {[<supply_pin_descriptor> ]}
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_wire:
            WIRE* wire;
            wire = new WIRE( growth );
            growth->wires.push_back( wire );
            doWIRE( wire );
            break;

        case T_via:
            WIRE_VIA* wire_via;
            wire_via = new WIRE_VIA( growth );
            growth->wire_vias.push_back( wire_via );
            doWIRE_VIA( wire_via );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doANCESTOR( ANCESTOR* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <ancestor_file_descriptor >::=
          (ancestor <file_path_name> (created_time <time_stamp> )
          [(comment <comment_string> )])
    */
    
    needSYMBOL();
    growth->filename = lexer->CurText();
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_created_time:
            readTIME( &growth->time_stamp );
            needRIGHT();
            break;
            
        case T_comment:
            needSYMBOL();
            growth->comment = lexer->CurText();
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doHISTORY( HISTORY* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <history_descriptor >::=
        (history [{<ancestor_file_descriptor> }] <self_descriptor> )
    */
    
    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_ancestor:
            ANCESTOR* ancestor;
            ancestor = new ANCESTOR( growth );
            growth->ancestors.push_back( ancestor );
            doANCESTOR( ancestor );
            break;
            
        case T_self:
            while( (tok = nextTok()) != T_RIGHT )
            {
                if( tok != T_LEFT )
                    expecting( T_LEFT );

                tok = nextTok();                
                switch( tok )
                {
                case T_created_time:
                    readTIME( &growth->time_stamp );
                    needRIGHT();
                    break;
                
                case T_comment:
                    needSYMBOL();
                    growth->comments.push_back( lexer->CurText() );
                    needRIGHT();
                    break;
                    
                default:
                    unexpected( lexer->CurText() );
                }
            }
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doSESSION( SESSION* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <session_file_descriptor >::=
        (session <session_id >
          (base_design <path/filename >)
          [<history_descriptor> ]
          [<session_structure_descriptor> ]
          [<placement_descriptor> ]
          [<floor_plan_descriptor> ]
          [<net_pin_changes_descriptor> ]
          [<was_is_descriptor> ]
          <swap_history_descriptor> ]
          [<route_descriptor> ]
        )
    */
    
    needSYMBOL();
    growth->session_id = lexer->CurText();

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_base_design:
            needSYMBOL();
            growth->base_design = lexer->CurText();
            needRIGHT();
            break;
            
        case T_history:
            if( growth->history )
                unexpected( tok );
            growth->history = new HISTORY( growth );
            doHISTORY( growth->history );
            break;
            
        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;
            
        case T_placement:
            if( growth->placement )
                unexpected( tok );
            growth->placement = new PLACEMENT( growth );
            doPLACEMENT( growth->placement );
            break;
            
        case T_was_is:
            if( growth->was_is )
                unexpected( tok );
            growth->was_is = new WAS_IS( growth );
            doWAS_IS( growth->was_is );
            break;

        case T_routes:
            if( growth->route )
                unexpected( tok );
            growth->route = new ROUTE( growth );
            doROUTE( growth->route );
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doWAS_IS( WAS_IS* growth ) throw( IOError )
{
    DSN_T       tok;
    PIN_PAIR    empty( growth );
    PIN_PAIR*   pin_pair;

    /*  <was_is_descriptor >::=
        (was_is {(pins <pin_reference> <pin_reference> )})
    */

    // none of the pins is ok too
    while( (tok = nextTok()) != T_RIGHT )
    {
                
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_pins:
            // copy the empty one, then fill its copy later thru pin_pair.                
            growth->pin_pairs.push_back( empty );
            pin_pair= &growth->pin_pairs.back();
            
            needSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->was.component_id, &pin_pair->was.pin_id );
            
            needSYMBOL();       // readCOMPnPIN() expects 1st token to have been read
            readCOMPnPIN( &pin_pair->is.component_id, &pin_pair->is.pin_id );
            
            needRIGHT();
            break;
            
        default:
            unexpected( lexer->CurText() );
        }
    }
}


void SPECCTRA_DB::doROUTE( ROUTE* growth ) throw( IOError )
{
    DSN_T   tok;

    /*  <route_descriptor >::=
        (routes
           <resolution_descriptor>
           <parser_descriptor>
           <structure_out_descriptor>
           <library_out_descriptor>
           <network_out_descriptor>
           <test_points_descriptor>
        )
    */

    while( (tok = nextTok()) != T_RIGHT )
    {
        if( tok != T_LEFT )
            expecting( T_LEFT );
        
        tok = nextTok();
        switch( tok )
        {
        case T_resolution:
            if( growth->resolution )
                unexpected( tok );
            growth->resolution = new UNIT_RES( growth, tok );
            doRESOLUTION( growth->resolution );
            break;

        case T_parser:
            if( growth->parser )
                unexpected( tok );
            growth->parser = new PARSER( growth );
            doPARSER( growth->parser );
            break;

        case T_structure:
            if( growth->structure )
                unexpected( tok );
            growth->structure = new STRUCTURE( growth );
            doSTRUCTURE( growth->structure );
            break;

        case T_library:
            if( growth->library )
                unexpected( tok );
            growth->library = new LIBRARY( growth );
            doLIBRARY( growth->library );
            break;
                    
        case T_network:
            if( growth->network )
                unexpected( tok );
            growth->network = new NETWORK( growth );
            doNETWORK( growth->network );
            break;

        default:
            unexpected( lexer->CurText() );
        }
    }
}


int SPECCTRA_DB::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{
    va_list     args;

    va_start( args, fmt );
    
    int result = 0;
    int total  = 0;
    
    for( int i=0; i<nestLevel;  ++i )
    {
        result = fprintf( fp, "%*c", NESTWIDTH, ' ' );
        if( result < 0 )
            break;
        
        total += result;
    }
    
    if( result<0 || (result=vfprintf( fp, fmt, args ))<0 )
        ThrowIOError( _("System file error writing to file \"%s\""), filename.GetData() );
    
    va_end( args );
    
    total += result;
    return total;
}


const char* SPECCTRA_DB::GetQuoteChar( const char* wrapee ) 
{
    // I include '#' so a symbol is not confused with a comment.  We intend
    // to wrap any symbol starting with a '#'.
    // Our LEXER class handles comments, and comments appear to be an extension
    // to the SPECCTRA DSN specification. 
    if( *wrapee == '#' )
        return quote_char.c_str();

    bool    isNumber = true;
    
    for(  ; *wrapee;  ++wrapee )
    {
        // if the string to be wrapped (wrapee) has a delimiter in it, 
        // return the quote_char so caller wraps the wrapee.
        if( strchr( "\t ()", *wrapee ) )
            return quote_char.c_str();
        
        if( !strchr( "01234567890.-+", *wrapee ) )
            isNumber = false;
    }
    
    if( isNumber )
        return quote_char.c_str();
    
    return "";      // can use an unwrapped string.
}


void SPECCTRA_DB::ExportPCB( wxString filename, BOARD* aBoard )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }
    
    // copy the BOARD to an empty PCB here.

    if( pcb )
        pcb->Format( this, 0 );

    fclose( fp );
    fp = 0;
}


void SPECCTRA_DB::ExportSESSION( wxString filename )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    if( session )
        session->Format( this, 0 );    
    
    fclose( fp );
    fp = 0;
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


void ELEM::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) ); 

    FormatContents( out, nestLevel+1 );
    
    out->Print( nestLevel, ")\n" ); 
}


void ELEM_HOLDER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    for( int i=0;  i<Length();  ++i )
    {
        At(i)->Format( out, nestLevel );
    }
}


int ELEM_HOLDER::FindElem( DSN_T aType, int instanceNum )
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

void PARSER::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    out->Print( nestLevel, "(string_quote %c)\n", string_quote );
    out->Print( nestLevel, "(space_in_quoted_tokens %s)\n", space_in_quoted_tokens ? "on" : "off" );
    out->Print( nestLevel, "(host_cad \"%s\")\n", host_cad.c_str() ); 
    out->Print( nestLevel, "(host_version \"%s\")\n", host_version.c_str() );
    
    if( const_id1.length()>0 || const_id2.length()>0 )
        out->Print( nestLevel, "(constant %c%s%c %c%s%c)\n", 
            string_quote, const_id1.c_str(), string_quote,
            string_quote, const_id2.c_str(), string_quote );

    if( routes_include_testpoint || routes_include_guides || routes_include_image_conductor )
        out->Print( nestLevel, "(routes_include%s%s%s)\n",
                   routes_include_testpoint ? " testpoint" : "",
                   routes_include_guides ? " guides" : "",
                   routes_include_image_conductor ? " image_conductor" : "");
    
    if( wires_include_testpoint )
        out->Print( nestLevel, "(wires_include testpoint)\n" );
        
    if( !via_rotate_first )
        out->Print( nestLevel, "(via_rotate_first off)\n" );
    
    out->Print( nestLevel, "(case_sensitive %s)\n", case_sensitive ? "on" : "off" );
}


void PLACE::Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    bool        useMultiLine;
    
    const char* quote = out->GetQuoteChar( component_id.c_str() );

    if( place_rules || properties.size() || lock_type!=T_NONE || rules 
        || region || part_number.size() )
    {
        useMultiLine = true;
        
        out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                                quote, component_id.c_str(), quote );
    
        out->Print( nestLevel+1, "%s", "" );
    }
    else
    {
        useMultiLine = false;
        
        out->Print( nestLevel, "(%s %s%s%s", LEXER::GetTokenText( Type() ),
                                quote, component_id.c_str(), quote );
    }

    if( hasVertex )
        out->Print( 0, " %f %f", vertex.x, vertex.y );
    
    if( side != T_NONE )
        out->Print( 0, " %s", LEXER::GetTokenText( side ) );
    
    if( isRotated )
        out->Print( 0, " %f", rotation );
    
    if( mirror != T_NONE )
        out->Print( 0, " (mirror %s)", LEXER::GetTokenText( mirror ) );
                   
    if( status != T_NONE )
        out->Print( 0, " (status %s)", LEXER::GetTokenText( status ) );
    
    if( logical_part.size() )
    {
        quote = out->GetQuoteChar( logical_part.c_str() );
        out->Print( 0, " (logical_part %s%s%s)", 
                   quote, logical_part.c_str(), quote );
    }

    if( useMultiLine )
    {
        out->Print( 0, "\n" );
        if( place_rules )
        {
            place_rules->Format( out, nestLevel+1 );
        }
        
        if( properties.size() )
        {
            out->Print( nestLevel+1, "(property \n" );
            
            for( PROPERTIES::const_iterator i = properties.begin();
                i != properties.end();  ++i )
            {
                i->Format( out, nestLevel+2 );
            }
            out->Print( nestLevel+1, ")\n" );
        }
        if( lock_type != T_NONE )
            out->Print( nestLevel+1, "(lock_type %s)\n", 
                       LEXER::GetTokenText(lock_type) );
        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( region )
            region->Format( out, nestLevel+1 );
        
        if( part_number.size() )
        {
            const char* quote = out->GetQuoteChar( part_number.c_str() );
            out->Print( nestLevel+1, "(PN %s%s%s)\n",
                       quote, part_number.c_str(), quote );
        }
    }
    else
        out->Print( 0, ")\n" );
}


} // namespace DSN


using namespace DSN;

// unit test this source file

int main( int argc, char** argv )
{
//    wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
//    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );
    wxString    filename( wxT("/tmp/testdesigns/test.ses") );

    SPECCTRA_DB     db;
    bool            failed = false;
    
    try 
    {
//        db.LoadPCB( filename );
        db.LoadSESSION( filename );
    } 
    catch( IOError ioe )
    {
        printf( "%s\n", CONV_TO_UTF8(ioe.errorText) );
        failed = true;
    }

    if( !failed )    
        printf("loaded OK\n");

//    db.SetPCB( SPECCTRA_DB::MakePCB() );
    

    // export what we read in, making this test program basically a beautifier
    db.ExportSESSION( wxT("/tmp/export.ses") );
//    db.ExportPCB( wxT("/tmp/export.dsn"), 0 ); 
    
}


//EOF
