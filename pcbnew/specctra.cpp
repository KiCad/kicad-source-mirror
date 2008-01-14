
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
    functions are Load() and Export(). 
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
     * @throw IOError if there is a problem outputting, such as a full disk.
     */
    virtual void PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IOError ) = 0;

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
typedef std::vector<PROPERTY>       PROPERTYS;
    

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
     * SPECCTRA DSN format.  This is the same as Format() except that the outter
     * wrapper is not included.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
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

    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
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
        
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
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
 * corresponds to the <rule_descriptor> in the specctra dsn spec.
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( STRINGS::const_iterator i = rules.begin();  i!=rules.end(); ++i )
            out->Print( nestLevel, "%s\n", i->c_str() );
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
    
    void FormatContent( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        for( STRINGS::const_iterator i=layer_ids.begin();  i!=layer_ids.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, i->c_str(), quote );
        }
        
        if( rule )
            rule->Format( out, nestLevel );
    }
};


/**
 * Class PATH
 * supports both the <path_descriptor> and the <polygon_descriptor> per
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


//  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
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
            for( unsigned i=0;  i<paths.size();  ++i )
            {
                paths[i].Format( out, nestLevel+1 );
            }
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
    
    void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        out->Print( nestLevel, "(%s\n", LEXER::GetTokenText( Type() ) );

        // these are mutually exclusive
        if( rectangle )
            rectangle->Format( out, nestLevel+1 );
        
        if( path )
            path->Format( out, nestLevel+1 );
        
        if( circle )
            circle->Format( out, nestLevel+1 );

        if( qarc )
            qarc->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" ); 
    }
};


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
    
    typedef boost::ptr_vector<WINDOW>   WINDOWS;
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
        
        if( path )
            path->Format( out, nestLevel+1 );
        
        if( circle )
            circle->Format( out, nestLevel+1 );

        if( qarc )
            qarc->Format( out, nestLevel+1 );

        if( rules )
            rules->Format( out, nestLevel+1 );
        
        if( place_rules )
            place_rules->Format( out, nestLevel+1 );

        for( unsigned i=0;  i<windows.size();  ++i )
            windows[i].Format( out, nestLevel+1 );
        
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
        for( STRINGS::const_iterator i=class_ids.begin();  i!=class_ids.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, i->c_str(), quote );
        }
    }
};


class CLASS_CLASS : public ELEM
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
        ELEM( aType, aParent )
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
        ELEM::FormatContents( out, nestLevel );
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
    
    PROPERTYS   properties;
    
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
            
            for( PROPERTYS::const_iterator i = properties.begin();
                i != properties.end();  ++i )
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
        
        for( unsigned i=0; i<layer_pairs.size();  ++i )
            layer_pairs[i].Format( out, nestLevel+1 );
        
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
        const char* quote_char = out->GetQuoteChar( value.c_str() );
        
        out->Print( nestLevel, "(%s %s%s%s)\n", LEXER::GetTokenText( Type() ),
                                quote_char, value.c_str(), quote_char );
    }
};


class REGION : public ELEM
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
        ELEM( T_region, aParent )
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

        ELEM::FormatContents( out, nestLevel );
        
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


class STRUCTURE : public ELEM
{
    friend class SPECCTRA_DB;
    
    UNIT*       unit;
    RESOLUTION* resolution;
    
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
        ELEM( T_structure, aParent )
    {
        unit = 0;
        resolution = 0;
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
        delete resolution;
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

        if( resolution )
            resolution->Format( out, nestLevel );
        
        for( unsigned i=0;  i<layers.size();  ++i )
            layers[i].Format( out, nestLevel ); 

        if( layer_noise_weight )
            layer_noise_weight->Format( out, nestLevel );
        
        if( boundary )
            boundary->Format( out, nestLevel );

        if( place_boundary )
            place_boundary->Format( out, nestLevel );

        for( unsigned i=0;  i<planes.size();  ++i )
            planes[i].Format( out, nestLevel );

        for( unsigned i=0;  i<regions.size();  ++i )
            regions[i].Format( out, nestLevel );
        
        for( unsigned i=0;  i<keepouts.size();  ++i )
            keepouts[i].Format( out, nestLevel );
        
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
        
        for( unsigned i=0;  i<grids.size();  ++i )
            grids[i].Format( out, nestLevel );
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
    
    PROPERTYS       properties;
    
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
    
    void SetVertext( const POINT& aVertex )
    {
        vertex = aVertex;
        hasVertex = true;
    }

    void SetRotation( float aRotation )
    {
        rotation = aRotation;
        isRotated = true;
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

        for( unsigned i=0;  i<places.size();  ++i )
            places[i].Format( out, nestLevel+1 );
        
        out->Print( nestLevel, ")\n" );
    }
};


class PLACEMENT : public ELEM
{
    friend class SPECCTRA_DB;

    DSN_T       unit;
    int         resolution;
    DSN_T       flip_style;
    
    typedef boost::ptr_vector<COMPONENT> COMPONENTS;
    COMPONENTS  components;

    
public:    
    PLACEMENT( ELEM* aParent ) :
        ELEM( T_placement, aParent )
    {
        unit = T_NONE;
        resolution = -1;
        flip_style = T_NONE;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
    {
        if( unit != T_NONE )
        {
            if( resolution >= 0 )
                out->Print( nestLevel, "(resolution %s %d)\n", 
                           LEXER::GetTokenText( unit ),
                           resolution );
            else
                out->Print( nestLevel, "(unit %s)\n",
                           LEXER::GetTokenText( unit ) );
        }

        if( flip_style != T_NONE )
        {
            out->Print( nestLevel, "(place_control (flip_style %s))\n",
                       LEXER::GetTokenText( flip_style ) );
        }
        
        for( unsigned i=0;  i<components.size();  ++i )
            components[i].Format( out, nestLevel );
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
    PLACEMENT*      placement;
    
public:
    
    PCB() :
        ELEM( T_pcb )
    {
        parser = 0;
        resolution = 0;
        unit = 0;
        structure = 0;
        placement = 0;
    }
    
    ~PCB()
    {
        delete parser;
        delete resolution;
        delete unit;
        delete structure;
        delete placement;
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
//    void doPLACE_RULE( PLACE_RULE* growth, bool expect_object_type = false ) throw( IOError );

    
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
    void PRINTF_FUNC Print( int nestLevel, const char* fmt, ... ) throw( IOError );
    
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

/*            
        case T_placement:
        case T_library:
            break;
*/            
            
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
        expecting( wxT("inch|mil|cm|mm|um") );
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
        expecting( wxT("inch|mil|cm|mm|um") );
    }
    
    tok = nextTok();
    if( tok != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::doLAYER_PAIR( LAYER_PAIR* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );
    growth->layer_id0 = lexer->CurText();

    tok = nextTok();        
    if( !isSymbol( tok ) )
        expecting( T_SYMBOL );
    growth->layer_id1 = lexer->CurText();
    
    if( nextTok() != T_NUMBER )
        expecting( T_NUMBER );
    growth->layer_weight = strtod( lexer->CurText(), 0 );

    if( nextTok() != T_RIGHT )
        expecting( T_RIGHT );
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
            if( growth->unit || growth->resolution )
                unexpected( T_unit );
            growth->unit = new UNIT( growth );
            doUNIT( growth->unit );
            break;
            
        case T_resolution:
            if( growth->unit || growth->resolution )
                unexpected( T_resolution );
            growth->resolution = new RESOLUTION( growth );
            doRESOLUTION( growth->resolution );
            break;

        case T_layer_noise_weight:
            growth->layer_noise_weight = new LAYER_NOISE_WEIGHT( growth );
            doLAYER_NOISE_WEIGHT( growth->layer_noise_weight );
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
            if( nextTok() != T_RIGHT )
                expecting( T_RIGHT );
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
        if( nextTok() != T_RIGHT )
            expecting( T_RIGHT );
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
        expecting( wxT("rect|path") );
}


void SPECCTRA_DB::doPATH( PATH* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();

    if( !isSymbol( tok ) )
        expecting( wxT("<layer_id>") );
    
    growth->layer_id = lexer->CurText();

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


void SPECCTRA_DB::doCIRCLE( CIRCLE* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( !isSymbol(tok) )
        expecting( T_SYMBOL );
    
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
    DSN_T   tok = nextTok();
    
    if( !isSymbol(tok) )
        expecting( T_SYMBOL );
    
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


void SPECCTRA_DB::doTOKPROP( TOKPROP* growth ) throw( IOError )
{
    DSN_T   tok = nextTok();
    
    if( tok<0 )
        unexpected( lexer->CurText() );

    growth->value = tok;
    
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
        if( tok != T_LEFT )
            expecting( T_LEFT );

        tok = nextTok();
        switch( tok )
        {
        case T_via_at_smd:
            tok = nextTok();
            if( tok!=T_on && tok!=T_off )
                expecting( wxT("on|off") );
            growth->via_at_smd = (tok==T_on);
            if( nextTok() != T_RIGHT )
                expecting( T_RIGHT );
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
                expecting( wxT("signal|power|mixed|jumper") );
            growth->layer_type = tok;
            if( nextTok()!=T_RIGHT )
                expecting(T_RIGHT);
            break;

        case T_rule:
            growth->rules = new RULE( growth, T_rule );
            doRULE( growth->rules );
            break;
            
        case T_property:
            {
                PROPERTY property;  // construct it once here, append multiple times.
    
                while( (tok = nextTok()) != T_RIGHT )
                {
                    if( tok != T_LEFT )
                        expecting( T_LEFT );
                    
                    tok = nextTok();
                    if( !isSymbol(tok) )
                        expecting( T_SYMBOL );
                    property.name = lexer->CurText();
                    
                    tok = nextTok();
                    if( !isSymbol(tok) )
                        expecting( T_SYMBOL );
                    property.value = lexer->CurText();
                    
                    growth->properties.push_back( property );
                    
                    if( nextTok() != T_RIGHT )
                        expecting( T_RIGHT );
                }
            }
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
                expecting( wxT("horizontal|vertical|orthogonal|positive_diagonal|negative_diagonal|diagonal|off") );
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
                expecting( wxT("forbidden|high|medium|low|free|<positive_integer>|-1") );
            }
            tok = nextTok();
            if( tok == T_LEFT )
            {
                if( nextTok() != T_type )
                    unexpected( lexer->CurText() );
                
                tok = nextTok();
                if( tok!=T_length && tok!=T_way )
                    expecting( wxT("length|way") );
                
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
            if( lexer->PrevTok() != T_LEFT && tok!=T_RIGHT )
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
                expecting( wxT("smd|pin") );
            
            if( nextTok() != T_RIGHT )
                expecting( T_RIGHT );
            
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
        expecting( wxT("<class_id>") );
    
    growth->class_ids.push_back( lexer->CurText() );
    
    do
    {
        tok = nextTok();
        if( !isSymbol( tok ) )
            expecting( wxT("<class_id>") );
        
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
    DSN_T   tok = nextTok();
    
    if( !isSymbol(tok ) )
       expecting( wxT("<layer_id>") );

    do
    {
        growth->layer_ids.push_back( lexer->CurText() );
        
    }  while( isSymbol(tok = nextTok()) );
 
    if( nextTok() != T_LEFT )
        expecting( T_LEFT );
    
    if( nextTok() != T_rule )
        expecting( T_rule );
    
    growth->rule = new RULE( growth, T_rule );
    doRULE( growth->rule );
    
    if( nextTok() != T_RIGHT )
        expecting( T_RIGHT );
}


void SPECCTRA_DB::Print( int nestLevel, const char* fmt, ... ) throw( IOError )
{
    va_list     args;

    va_start( args, fmt );
    
    int ret = 0;
    
    for( int i=0; i<nestLevel;  ++i )
    {
        ret = fprintf( fp, "    " );
        if( ret < 0 )
            break;
    }
    
    if( ret<0 || vfprintf( fp, fmt, args )<0 )
        ThrowIOError( _("System file error writing to file \"%s\""), filename.GetData() );
    
    va_end( args );
}


const char* SPECCTRA_DB::GetQuoteChar( const char* wrapee ) 
{
    // I include '#' so a symbol is not confused with a comment.  We intend
    // to wrap any symbol starting with a '#'.
    // Our LEXER class handles comments, and comments appear to be an extension
    // to the SPECCTRA DSN specification. 
    if( *wrapee == '#' )
        return quote_char.c_str();
    
    while( *wrapee )
    {
        // if the string to be wrapped (wrapee) has a delimiter in it, 
        // return the quote_char so caller wraps the wrapee.
        if( strchr( "\t ()", *wrapee++ ) )
            return quote_char.c_str();
    }
    return "";      // can use an unwrapped string.
}


void SPECCTRA_DB::Export( wxString filename, BOARD* aBoard )
{
    fp = wxFopen( filename, wxT("w") );
    
    if( !fp )
    {
        ThrowIOError( _("Unable to open file \"%s\""), filename.GetData() );  
    }

    tree->Format( this, 0 );    
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


void ELEM::FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError )
{
    for( int i=0;  i<Length();  ++i )
    {
        At(i)->Format( out, nestLevel );
    }
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
    const char* quote = out->GetQuoteChar( component_id.c_str() );
    
    out->Print( nestLevel, "(%s %s%s%s\n", LEXER::GetTokenText( Type() ),
                            quote, component_id.c_str(), quote );

    out->Print( nestLevel+1, "%s", "" );
    
    if( hasVertex )
        out->Print( 0, "%f %f ", vertex.x, vertex.y );
    
    if( side != T_NONE )
        out->Print( 0, "%s ", LEXER::GetTokenText( side ) );
    
    if( isRotated )
        out->Print( 0, "%f ", rotation );
    
    if( mirror != T_NONE )
        out->Print( 0, "(mirror %s)", LEXER::GetTokenText( mirror ) );
                   
    if( status != T_NONE )
        out->Print( 0, "(status %s)", LEXER::GetTokenText( status ) );
    
    if( logical_part.size() )
    {
        quote = out->GetQuoteChar( logical_part.c_str() );
        out->Print( 0, "(logical_part %s%s%s)", 
                   quote, logical_part.c_str(), quote );
    }

    if( place_rules || properties.size() || lock_type!=T_NONE || rules || region || part_number.size() )
    {
        out->Print( 0, "\n" );
        if( place_rules )
        {
            place_rules->Format( out, nestLevel+1 );
        }
        if( properties.size() )
        {
            out->Print( nestLevel+1, "(property \n" );
            
            for( PROPERTYS::const_iterator i = properties.begin();
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
    
}


//EOF
