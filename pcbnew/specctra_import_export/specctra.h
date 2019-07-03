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

#ifndef SPECCTRA_H_
#define SPECCTRA_H_


//  see http://www.boost.org/libs/ptr_container/doc/ptr_sequence_adapter.html
#include <boost/ptr_container/ptr_vector.hpp>

//  see http://www.boost.org/libs/ptr_container/doc/ptr_set.html
#include <boost/ptr_container/ptr_set.hpp>

#include <fctsys.h>
#include <specctra_import_export/specctra_lexer.h>
#include <pcbnew.h>

#include <memory>

// all outside the DSN namespace:
class BOARD;
class TRACK;
class VIA;
class NETCLASS;
class MODULE;
class SHAPE_POLY_SET;

typedef DSN::T  DSN_T;


/**
    This source file implements export and import capabilities to the
    specctra dsn file format.  The grammar for that file format is documented
    fairly well.  There are classes for each major type of descriptor in the
    spec.

    Since there are so many classes in here, it may be helpful to generate
    the Doxygen directory:

    $ cd &ltkicadSourceRoot&gt
    $ doxygen

    Then you can view the html documentation in the &ltkicadSourceRoot&gt/doxygen
    directory.  The main class in this file is SPECCTRA_DB and its main
    functions are LoadPCB(), LoadSESSION(), and ExportPCB().

    Wide use is made of boost::ptr_vector&lt&gt and std::vector&lt&gt template classes.
    If the contained object is small, then std::vector tends to be used.
    If the contained object is large, variable size, or would require writing
    an assignment operator() or copy constructore, then boost::ptr_vector
    cannot be beat.
*/
namespace DSN {


class SPECCTRA_DB;


/**
 * Function GetTokenText
 * is in the DSN namespace and returns the C string representing a
 * SPECCTRA_DB::keyword.  We needed a non-instanance function to get at
 * the SPECCTRA_DB::keyword[] and class SPECCTRA_DB is not defined yet.
 */
const char* GetTokenText( T aTok );


/**
 * Struct POINT
 * is a holder for a point in the SPECCTRA DSN coordinate system.  It can also
 * be used to hold a distance (vector really) from some origin.
 */
struct POINT
{
    double  x;
    double  y;

    POINT() { x=0.0; y=0.0; }

    POINT( double aX, double aY ) :
        x(aX), y(aY)
    {
    }

    bool operator==( const POINT& other ) const
    {
        return x==other.x && y==other.y;
    }

    bool operator!=( const POINT& other ) const
    {
        return !( *this == other );
    }

    POINT& operator+=( const POINT& other )
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    POINT& operator=( const POINT& other )
    {
        x = other.x;
        y = other.y;
        return *this;
    }

    /**
     * Function FixNegativeZero
     * will change negative zero to positive zero in the IEEE floating point
     * storage format.  Basically turns off the sign bit if the mantissa and
     * exponent say the value is otherwise zero.
     */
    void FixNegativeZero()
    {
        if( x == -0.0 )
            x = 0.0;
        if( y == -0.0 )
            y = 0.0;
    }

    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER according to the
     * SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const
    {
        out->Print( nestLevel, " %.6g %.6g", x, y );
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
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const
    {
        const char* quoteName  = out->GetQuoteChar( name.c_str() );
        const char* quoteValue = out->GetQuoteChar( value.c_str() );

        out->Print( nestLevel, "(%s%s%s %s%s%s)\n",
                   quoteName,  name.c_str(), quoteName,
                   quoteValue, value.c_str(), quoteValue );
    }
};
typedef std::vector<PROPERTY>       PROPERTIES;


class UNIT_RES;

/**
 * Class ELEM
 * is a base class for any DSN element class.
 * See class ELEM_HOLDER also.
 */
class ELEM
{
    friend class SPECCTRA_DB;

protected:
    DSN_T           type;
    ELEM*           parent;


    /**
     * Function makeHash
     * returns a string which uniquely represents this ELEM amoung other
     * ELEMs of the same derived class as "this" one.
     * It is not useable for all derived classes, only those which plan for
     * it by implementing a FormatContents() function that captures all info
     * which will be used in the subsequent string compare.  THIS SHOULD
     * NORMALLY EXCLUDE THE TYPENAME, AND INSTANCE NAME OR ID AS WELL.
     */
    std::string makeHash()
    {
        sf.Clear();
        FormatContents( &sf, 0 );
        sf.StripUseless();

        return sf.GetString();
    }

    // avoid creating this for every compare, make static.
    static STRING_FORMATTER  sf;


public:

    ELEM( DSN_T aType, ELEM* aParent = 0 );

    virtual ~ELEM();

    DSN_T   Type() const { return type; }

    const char* Name() const;


    /**
     * Function GetUnits
     * returns the units for this section.  Derived classes may override this
     * to check for section specific overrides.
     * @return UNIT_RES* - from a local or parent scope
     */
    virtual UNIT_RES* GetUnits() const;

    /**
     * Function Format
     * writes this object as ASCII out to an OUTPUTFORMATTER according to the
     * SPECCTRA DSN format.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    virtual void Format( OUTPUTFORMATTER* out, int nestLevel );


    /**
     * Function FormatContents
     * writes the contents as ASCII out to an OUTPUTFORMATTER according to the
     * SPECCTRA DSN format.  This is the same as Format() except that the outer
     * wrapper is not included.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel )
    {
        // overridden in ELEM_HOLDER
    }

    void SetParent( ELEM* aParent )
    {
        parent = aParent;
    }
};


/**
 * Class ELEM_HOLDER
 * is a holder for any DSN class.  It can contain other
 * class instances, including classes derived from this class.
 */
class ELEM_HOLDER : public ELEM
{
    friend class SPECCTRA_DB;

    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;

    ELEM_ARRAY      kids;      ///< ELEM pointers

public:

    ELEM_HOLDER( DSN_T aType, ELEM* aParent = 0 ) :
        ELEM( aType, aParent )
    {
    }

    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override;


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

    ELEM*   At( int aIndex ) const
    {
        // we have varying sized objects and are using polymorphism, so we
        // must return a pointer not a reference.
        return (ELEM*) &kids[aIndex];
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

    /// This holds pairs of strings, one pair for each constant definition
    STRINGS     constants;

    std::string host_cad;
    std::string host_version;


public:

    PARSER( ELEM* aParent );

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override;
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

    /**
     * A static instance which holds the default units of T_inch and 2540000.
     * See page 108 of the specctra spec, May 2000.
     */
    static UNIT_RES Default;

    UNIT_RES( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        units = T_inch;
        value = 2540000;
    }

    DSN_T   GetEngUnits() const  { return units; }
    int     GetValue() const  { return value; }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( type == T_unit )
            out->Print( nestLevel, "(%s %s)\n", Name(),
                       GetTokenText(units) );

        else    // T_resolution
            out->Print( nestLevel, "(%s %s %d)\n", Name(),
                       GetTokenText(units), value );
    }
};


class RECTANGLE : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     layer_id;

    POINT           point0;         ///< one of two opposite corners
    POINT           point1;

public:

    RECTANGLE( ELEM* aParent ) :
        ELEM( T_rect, aParent )
    {
    }

    void SetLayerId( const char* aLayerId )
    {
        layer_id = aLayerId;
    }

    void SetCorners( const POINT& aPoint0, const POINT& aPoint1 )
    {
        point0 = aPoint0;
        point0.FixNegativeZero();

        point1 = aPoint1;
        point1.FixNegativeZero();
    }

    POINT GetOrigin() { return point0; }
    POINT GetEnd() { return point1; }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );

        out->Print( nestLevel, "(%s %s%s%s %.6g %.6g %.6g %.6g)%s",
                   Name(),
                   quote, layer_id.c_str(), quote,
                   point0.x, point0.y,
                   point1.x, point1.y,
                   newline );
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s", Name() );

        bool singleLine;

        if( rules.size() == 1 )
        {
            singleLine = true;
            out->Print( 0, " %s)", rules.begin()->c_str() );
        }

        else
        {
            out->Print( 0, "\n" );
            singleLine = false;
            for( STRINGS::const_iterator i = rules.begin();  i!=rules.end(); ++i )
                out->Print( nestLevel+1, "%s\n", i->c_str() );
            out->Print( nestLevel, ")" );
        }

        if( nestLevel || !singleLine )
            out->Print( 0, "\n" );
    }
};


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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s", Name() );

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

    PATH( ELEM* aParent, DSN_T aType = T_path ) :
        ELEM( aType, aParent )
    {
        aperture_width = 0.0;
        aperture_type  = T_round;
    }

    void AppendPoint( const POINT& aPoint )
    {
        points.push_back( aPoint );
    }

    POINTS& GetPoints() {return points; }

    void SetLayerId( const char* aLayerId )
    {
        layer_id = aLayerId;
    }

    void SetAperture( double aWidth )
    {
        aperture_width = aWidth;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );

        const int RIGHTMARGIN = 70;
        int perLine = out->Print( nestLevel, "(%s %s%s%s %.6g",
                               Name(),
                               quote, layer_id.c_str(), quote,
                               aperture_width );

        int wrapNest = std::max( nestLevel+1, 6 );
        for( unsigned i=0;  i<points.size();  ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( wrapNest, "%s", "" );
            }
            else
                perLine += out->Print( 0, "  " );

            perLine += out->Print( 0, "%.6g %.6g", points[i].x, points[i].y );
        }

        if( aperture_type == T_square )
        {
            out->Print( 0, "(aperture_type square)" );
        }

        out->Print( 0, ")%s", newline );
    }
};
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

    /**
     * GetCorners fills aBuffer with a list of coordinates (x,y) of corners
     */
    void GetCorners( std::vector<double>& aBuffer )
    {
        if( rectangle )
        {
            aBuffer.push_back( rectangle->GetOrigin().x );
            aBuffer.push_back( rectangle->GetOrigin().y );

            aBuffer.push_back( rectangle->GetOrigin().x );
            aBuffer.push_back( rectangle->GetEnd().y );

            aBuffer.push_back( rectangle->GetEnd().x );
            aBuffer.push_back( rectangle->GetEnd().y );

            aBuffer.push_back( rectangle->GetEnd().x );
            aBuffer.push_back( rectangle->GetOrigin().y );
        }
        else
        {
            for( PATHS::iterator i=paths.begin();  i!=paths.end();  ++i )
            {
                POINTS& plist = i->GetPoints();
                for( unsigned jj = 0; jj < plist.size(); jj++ )
                {
                    aBuffer.push_back( plist[jj].x );
                    aBuffer.push_back( plist[jj].y );
                }
            }
        }
    }


    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s\n", Name() );

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
    POINT       vertex;     // POINT's constructor sets to (0,0)

public:
    CIRCLE( ELEM* aParent ) :
        ELEM( T_circle, aParent )
    {
        diameter = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %.6g", Name(),
                                quote, layer_id.c_str(), quote,
                                diameter );

        if( vertex.x!=0.0 || vertex.y!=0.0 )
            out->Print( 0, " %.6g %.6g)%s", vertex.x, vertex.y, newline );
        else
            out->Print( 0, ")%s", newline );
    }

    void SetLayerId( const char* aLayerId )
    {
        layer_id = aLayerId;
    }

    void SetDiameter( double aDiameter )
    {
        diameter = aDiameter;
    }

    void SetVertex( const POINT& aVertex )
    {
        vertex = aVertex;
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %.6g", Name() ,
                                 quote, layer_id.c_str(), quote,
                                 aperture_width);

        for( int i=0;  i<3;  ++i )
            out->Print( 0, "  %.6g %.6g",  vertex[i].x, vertex[i].y );

        out->Print( 0, ")%s", newline );
    }

    void SetLayerId( const char* aLayerId )
    {
        layer_id = aLayerId;
    }
    void SetStart( const POINT& aStart )
    {
        vertex[0] = aStart;
        // no -0.0 on the printouts!
        vertex[0].FixNegativeZero();
    }
    void SetEnd( const POINT& aEnd )
    {
        vertex[1] = aEnd;
        // no -0.0 on the printouts!
        vertex[1].FixNegativeZero();
    }
    void SetCenter( const POINT& aCenter )
    {
        vertex[2] = aCenter;
        // no -0.0 on the printouts!
        vertex[2].FixNegativeZero();
    }
};


class WINDOW : public ELEM
{
    friend class SPECCTRA_DB;

protected:
    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*       shape;

public:

    WINDOW( ELEM* aParent, DSN_T aType = T_window ) :
        ELEM( aType, aParent )
    {
        shape = 0;
    }

    ~WINDOW()
    {
        delete shape;
    }

    void SetShape( ELEM* aShape )
    {
        delete shape;
        shape = aShape;

        if( aShape )
        {
            wxASSERT(aShape->Type()==T_rect || aShape->Type()==T_circle
                     || aShape->Type()==T_qarc || aShape->Type()==T_path
                     || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( shape )
            shape->Format( out, 0 );

        out->Print( 0, ")\n" );
    }
};
typedef boost::ptr_vector<WINDOW>   WINDOWS;


/**
 * Class KEEPOUT
 * is used for &lt;keepout_descriptor&gt; and &lt;plane_descriptor&gt;.
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

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*           shape;

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
        shape = 0;

        sequence_number = -1;
    }

    ~KEEPOUT()
    {
        delete rules;
        delete place_rules;
        delete shape;
    }

    void SetShape( ELEM* aShape )
    {
        delete shape;
        shape = aShape;

        if( aShape )
        {
            wxASSERT(aShape->Type()==T_rect || aShape->Type()==T_circle
                     || aShape->Type()==T_qarc || aShape->Type()==T_path
                     || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void AddWindow( WINDOW* aWindow )
    {
        aWindow->SetParent( this );
        windows.push_back( aWindow );
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* newline = "\n";

        out->Print( nestLevel, "(%s", Name() );

        if( name.size() )
        {
            const char* quote = out->GetQuoteChar( name.c_str() );
            out->Print( 0, " %s%s%s", quote, name.c_str(), quote );
        }
        // Could be not needed:
#if 1
        else
            out->Print( 0, " \"\"" );   // the zone with no name or net_code == 0
#endif

        if( sequence_number != -1 )
            out->Print( 0, " (sequence_number %d)", sequence_number );

        if( shape )
        {
            out->Print( 0, " " );
            shape->Format( out, 0 );
        }

        if( rules )
        {
            out->Print( 0, "%s", newline );
            newline = "";
            rules->Format( out, nestLevel+1 );
        }

        if( place_rules )
        {
            out->Print( 0, "%s", newline );
            newline = "";
            place_rules->Format( out, nestLevel+1 );
        }

        if( windows.size() )
        {
            out->Print( 0, "%s", newline );
            newline = "";

            for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
                i->Format( out, nestLevel+1 );

            out->Print( nestLevel, ")\n" );
        }
        else
            out->Print( 0, ")\n" );
    }
};
typedef boost::ptr_vector<KEEPOUT>  KEEPOUTS;


/**
 * Class VIA
 * corresponds to the &lt;via_descriptor&gt; in the specctra dsn spec.
 */
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

    void AppendVia( const char* aViaName )
    {
        padstacks.push_back( aViaName );
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const int RIGHTMARGIN = 80;
        int perLine = out->Print( nestLevel, "(%s", Name() );

        for( STRINGS::iterator i=padstacks.begin();  i!=padstacks.end();  ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "");
            }

            const char* quote = out->GetQuoteChar( i->c_str() );
            perLine += out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }

        if( spares.size() )
        {
            out->Print( 0, "\n" );

            perLine = out->Print( nestLevel+1, "(spare" );

            for( STRINGS::iterator i=spares.begin();  i!=spares.end();  ++i )
            {
                if( perLine > RIGHTMARGIN )
                {
                    out->Print( 0, "\n" );
                    perLine = out->Print( nestLevel+2, "%s", "");
                }
                const char* quote = out->GetQuoteChar( i->c_str() );
                perLine += out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
            }

            out->Print( 0, ")" );
        }

        out->Print( 0, ")\n" );
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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
     * @param aParent - Parent element of the object.
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s\n", Name() );

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
    int         cost;       ///< [forbidden | high | medium | low | free | \<positive_integer\> | -1]
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( name.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
                       quote, name.c_str(), quote );

        out->Print( nestLevel+1, "(type %s)\n", GetTokenText( layer_type ) );

        if( properties.size() )
        {
            out->Print( nestLevel+1, "(property\n" );

            for( PROPERTIES::iterator i = properties.begin();  i != properties.end();  ++i )
            {
                i->Format( out, nestLevel+2 );
            }
            out->Print( nestLevel+1, ")\n" );
        }

        if( direction != -1 )
            out->Print( nestLevel+1, "(direction %s)\n",
                       GetTokenText( (DSN_T)direction ) );

        if( rules )
            rules->Format( out, nestLevel+1 );

        if( cost != -1 )
        {
            if( cost < 0 )
                out->Print( nestLevel+1, "(cost %d", -cost );   // positive integer, stored as negative
            else
                out->Print( nestLevel+1, "(cost %s", GetTokenText( (DSN_T)cost ) );

            if( cost_type != -1 )
                out->Print( 0, " (type %s)", GetTokenText( (DSN_T)cost_type ) );

            out->Print( 0, ")\n" );
        }

        if( use_net.size() )
        {
            out->Print( nestLevel+1, "(use_net" );
            for( STRINGS::const_iterator i = use_net.begin();  i!=use_net.end(); ++i )
            {
                quote = out->GetQuoteChar( i->c_str() );
                out->Print( 0, " %s%s%s",  quote, i->c_str(), quote );
            }
            out->Print( 0, ")\n" );
        }

        out->Print( nestLevel, ")\n" );
    }
};

typedef boost::ptr_vector<LAYER>    LAYERS;


class SPECCTRA_LAYER_PAIR : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     layer_id0;
    std::string     layer_id1;

    double          layer_weight;

public:
    SPECCTRA_LAYER_PAIR( ELEM* aParent ) :
        ELEM( T_layer_pair, aParent )
    {
        layer_weight = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote0 = out->GetQuoteChar( layer_id0.c_str() );
        const char* quote1 = out->GetQuoteChar( layer_id1.c_str() );

        out->Print( nestLevel, "(%s %s%s%s %s%s%s %.6g)\n", Name(),
               quote0, layer_id0.c_str(), quote0,
               quote1, layer_id1.c_str(), quote1,
               layer_weight );
    }
};
typedef boost::ptr_vector<SPECCTRA_LAYER_PAIR>  SPECCTRA_LAYER_PAIRS;


class LAYER_NOISE_WEIGHT : public ELEM
{
    friend class SPECCTRA_DB;

    SPECCTRA_LAYER_PAIRS     layer_pairs;

public:

    LAYER_NOISE_WEIGHT( ELEM* aParent ) :
        ELEM( T_layer_noise_weight, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s\n", Name() );

        for( SPECCTRA_LAYER_PAIRS::iterator i=layer_pairs.begin(); i!=layer_pairs.end();  ++i )
            i->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }
};


/**
 * Class COPPER_PLANE
 * corresponds to a &lt;plane_descriptor&gt; in the specctra dsn spec.
 */
class COPPER_PLANE : public KEEPOUT
{
    friend class SPECCTRA_DB;

public:
    COPPER_PLANE( ELEM* aParent ) :
        KEEPOUT( aParent, T_plane )
    {}
};
typedef boost::ptr_vector<COPPER_PLANE>    COPPER_PLANES;


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
        // Do not leave uninitialized members
        value = T_NONE;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s %s)\n", Name(),
                   GetTokenText( value ) );
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( value.c_str() );

        out->Print( nestLevel, "(%s %s%s%s)\n", Name(),
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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
    double      dimension;
    DSN_T       direction;      ///< T_x | T_y | -1 for both
    double      offset;
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s %s %.6g",
                   Name(),
                   GetTokenText( grid_type ), dimension );

        if( grid_type == T_place )
        {
            if( image_type==T_smd || image_type==T_pin )
                out->Print( 0, " (image_type %s)", GetTokenText( image_type ) );
        }
        else
        {
            if( direction==T_x || direction==T_y )
                out->Print( 0, " (direction %s)", GetTokenText( direction ) );
        }

        if( offset != 0.0 )
            out->Print( 0, " (offset %.6g)", offset );

        out->Print( 0, ")\n");
    }
};


class STRUCTURE_OUT : public ELEM
{
    friend class SPECCTRA_DB;

    LAYERS      layers;
    RULE*       rules;

public:
    STRUCTURE_OUT( ELEM* aParent ) :
        ELEM( T_structure_out, aParent )
    {
        rules = 0;
    }

    ~STRUCTURE_OUT()
    {
        delete rules;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        for( LAYERS::iterator i=layers.begin();  i!=layers.end();  ++i )
            i->Format( out, nestLevel );

        if( rules )
            rules->Format( out, nestLevel );
    }
};


class STRUCTURE : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;

    UNIT_RES*   unit;

    LAYERS      layers;

    LAYER_NOISE_WEIGHT*  layer_noise_weight;

    BOUNDARY*   boundary;
    BOUNDARY*   place_boundary;
    VIA*        via;
    CONTROL*    control;
    RULE*       rules;

    KEEPOUTS    keepouts;

    COPPER_PLANES      planes;

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

    void SetBOUNDARY( BOUNDARY *aBoundary )
    {
        delete boundary;
        boundary = aBoundary;
        if( boundary )
        {
            boundary->SetParent( this );
        }
    }

    void SetPlaceBOUNDARY( BOUNDARY *aBoundary )
    {
        delete place_boundary;
        place_boundary = aBoundary;
        if( place_boundary )
            place_boundary->SetParent( this );
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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

        for( COPPER_PLANES::iterator i=planes.begin();  i!=planes.end();  ++i )
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

    UNIT_RES* GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }
};


/**
 * Class PLACE
 * implements the &lt;placement_reference&gt; in the specctra dsn spec.
 */
class PLACE : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     component_id;       ///< reference designator

    DSN_T           side;

    double          rotation;

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
        side = T_front;

        rotation = 0.0;

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
        vertex.FixNegativeZero();
        hasVertex = true;
    }

    void SetRotation( double aRotation )
    {
        rotation = aRotation;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override;
};
typedef boost::ptr_vector<PLACE>    PLACES;


/**
 * Class COMPONENT
 * implements the &lt;component_descriptor&gt; in the specctra dsn spec.
 */
class COMPONENT : public ELEM
{
    friend class SPECCTRA_DB;

//    std::string     hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string     image_id;
    PLACES          places;

public:
    COMPONENT( ELEM* aParent ) :
        ELEM( T_component, aParent )
    {
    }

    const std::string& GetImageId() const  { return image_id; }
    void SetImageId( const std::string& aImageId )
    {
        image_id = aImageId;
    }


    /**
     * Function Compare
     * compares two objects of this type and returns <0, 0, or >0.
     */
//    static int Compare( IMAGE* lhs, IMAGE* rhs );

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( image_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
                                quote, image_id.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        for( PLACES::iterator i=places.begin();  i!=places.end();  ++i )
            i->Format( out, nestLevel );
    }
};
typedef boost::ptr_vector<COMPONENT> COMPONENTS;


class PLACEMENT : public ELEM
{
    friend class SPECCTRA_DB;

    UNIT_RES*   unit;

    DSN_T       flip_style;

    COMPONENTS  components;

public:
    PLACEMENT( ELEM* aParent ) :
        ELEM( T_placement, aParent )
    {
        unit = 0;
        flip_style = DSN_T( T_NONE );
    }

    ~PLACEMENT()
    {
        delete unit;
    }

    /**
     * Function LookupCOMPONENT
     * looks up a COMPONENT by name.  If the name is not found, a new
     * COMPONENT is added to the components container.  At any time the
     * names in the component container should remain unique.
     * @return COMPONENT* - an existing or new
     */
    COMPONENT* LookupCOMPONENT( const std::string& imageName )
    {
        for( unsigned i=0; i<components.size();  ++i )
        {
            if( 0 == components[i].GetImageId().compare( imageName ) )
                return &components[i];
        }

        COMPONENT* added = new COMPONENT(this);
        components.push_back( added );
        added->SetImageId( imageName );
        return added;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( unit )
            unit->Format( out, nestLevel );

        if( flip_style != DSN_T( T_NONE ) )
        {
            out->Print( nestLevel, "(place_control (flip_style %s))\n",
                       GetTokenText( flip_style ) );
        }

        for( COMPONENTS::iterator i=components.begin();  i!=components.end();  ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }
};


/**
 * Class SHAPE
 * corresponds to the "(shape ..)" element in the specctra dsn spec.
 * It is not a &lt;shape_descriptor&gt;, which is one of things that this
 * elements contains, i.e. in its "shape" field.  This class also implements
 * the "(outline ...)" element as a dual personality.
 */
class SHAPE : public WINDOW
{
    friend class SPECCTRA_DB;

    DSN_T           connect;

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    ELEM*           shape;      // inherited from WINDOW
    */

    WINDOWS         windows;

public:

    /**
     * Constructor SHAPE
     * alternatively takes a DSN_T aType of T_outline
     */
    SHAPE( ELEM* aParent, DSN_T aType = T_shape ) :
        WINDOW( aParent, aType )
    {
        connect = T_on;
    }

    void SetConnect( DSN_T aConnect )
    {
        connect = aConnect;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( shape )
            shape->Format( out, 0 );

        if( connect == T_off )
            out->Print( 0, "(connect %s)", GetTokenText( connect ) );

        if( windows.size() )
        {
            out->Print( 0, "\n" );

            for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
                i->Format( out, nestLevel+1 );

            out->Print( nestLevel, ")\n" );
        }
        else
            out->Print( 0, ")\n" );
    }
};


class PIN : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     padstack_id;
    double          rotation;
    bool            isRotated;
    std::string     pin_id;
    POINT           vertex;

    int             kiNetCode;      ///< KiCad netcode


public:
    PIN( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
        rotation = 0.0;
        isRotated = false;
        kiNetCode = 0;
    }

    void SetRotation( double aRotation )
    {
        rotation = aRotation;
        isRotated = (aRotation != 0.0);
    }

    void SetVertex( const POINT& aPoint )
    {
        vertex = aPoint;
        vertex.FixNegativeZero();
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );
        if( isRotated )
            out->Print( nestLevel, "(pin %s%s%s (rotate %.6g)",
                                     quote, padstack_id.c_str(), quote,
                                     rotation
                                     );
        else
            out->Print( nestLevel, "(pin %s%s%s", quote, padstack_id.c_str(), quote );

        quote = out->GetQuoteChar( pin_id.c_str() );
        out->Print( 0, " %s%s%s %.6g %.6g)\n", quote, pin_id.c_str(), quote,
                   vertex.x, vertex.y );
    }
};
typedef boost::ptr_vector<PIN>  PINS;


class LIBRARY;
class IMAGE : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;
    friend class LIBRARY;

    std::string     hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string     image_id;
    DSN_T           side;
    UNIT_RES*       unit;

    /*  The grammar spec says only one outline is supported, but I am seeing
        *.dsn examples with multiple outlines.  So the outlines will go into
        the kids list.
    */

    PINS            pins;

    RULE*           rules;
    RULE*           place_rules;

    KEEPOUTS        keepouts;

    int             duplicated;     ///< no. times this image_id is duplicated

public:

    IMAGE( ELEM* aParent ) :
        ELEM_HOLDER( T_image, aParent )
    {
        side = T_both;
        unit = 0;
        rules = 0;
        place_rules = 0;
        duplicated = 0;
    }
    ~IMAGE()
    {
        delete unit;
        delete rules;
        delete place_rules;
    }

    /**
     * Function Compare
     * compares two objects of this type and returns <0, 0, or >0.
     */
    static int Compare( IMAGE* lhs, IMAGE* rhs );

    std::string GetImageId()
    {
        if( duplicated )
        {
            char    buf[32];

            std::string ret = image_id;
            ret += "::";
            sprintf( buf, "%d", duplicated );
            ret += buf;
            return ret;
        }

        return image_id;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        std::string imageId = GetImageId();

        const char* quote = out->GetQuoteChar( imageId.c_str() );

        out->Print( nestLevel, "(%s %s%s%s", Name(),
                                quote, imageId.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    // this is here for makeHash()
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( side != T_both )
            out->Print( 0, " (side %s)", GetTokenText( side ) );

        out->Print( 0, "\n");

        if( unit )
            unit->Format( out, nestLevel );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel );

        for( PINS::iterator i=pins.begin();  i!=pins.end();  ++i )
            i->Format( out, nestLevel );

        if( rules )
            rules->Format( out, nestLevel );

        if( place_rules )
            place_rules->Format( out, nestLevel );

        for( KEEPOUTS::iterator i=keepouts.begin();  i!=keepouts.end();  ++i )
            i->Format( out, nestLevel );
    }


    UNIT_RES* GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }
};
typedef boost::ptr_vector<IMAGE>    IMAGES;


/**
 * Class PADSTACK
 * holds either a via or a pad definition.
 */
class PADSTACK : public ELEM_HOLDER
{
    friend class SPECCTRA_DB;

    std::string     hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string     padstack_id;
    UNIT_RES*       unit;

    /* The shapes are stored in the kids list */

    DSN_T           rotate;
    DSN_T           absolute;
    DSN_T           attach;
    std::string     via_id;

    RULE*           rules;

public:

    /**
     * Constructor PADSTACK()
     * cannot take ELEM* aParent because PADSTACKSET confuses this with a
     * copy constructor and causes havoc.  Instead set parent with
     * LIBRARY::AddPadstack()
     */
    PADSTACK() :
        ELEM_HOLDER( T_padstack, NULL )
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

    const std::string& GetPadstackId()
    {
        return padstack_id;
    }

    /**
     * Function Compare
     * compares two objects of this type and returns <0, 0, or >0.
     */
    static int Compare( PADSTACK* lhs, PADSTACK* rhs );


    void SetPadstackId( const char* aPadstackId )
    {
        padstack_id = aPadstackId;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
                                quote, padstack_id.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }


    // this factored out for use by Compare()
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( unit )
            unit->Format( out, nestLevel );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel );

        out->Print( nestLevel, "%s", "" );

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
            out->Print( 0, "(rotate %s)", GetTokenText( rotate ) );

        if( absolute == T_on )  // print the non-default
            out->Print( 0, "(absolute %s)", GetTokenText( absolute ) );

        out->Print( 0, "\n" );

        if( rules )
            rules->Format( out, nestLevel );
    }


    UNIT_RES* GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }
};
typedef boost::ptr_vector<PADSTACK> PADSTACKS;


/**
 * Function operator<
 * is used by the PADSTACKSET boost::ptr_set below
 */
inline bool operator<( const PADSTACK& lhs, const PADSTACK& rhs )
{
    return PADSTACK::Compare( (PADSTACK*) &lhs, (PADSTACK*) &rhs ) < 0;
}


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
    IMAGES          images;

    PADSTACKS       padstacks;      ///< all except vias, which are in 'vias'
    PADSTACKS       vias;

public:

    LIBRARY( ELEM* aParent, DSN_T aType = T_library ) :
        ELEM( aType, aParent )
    {
        unit = 0;
//        via_start_index = -1;       // 0 or greater means there is at least one via
    }
    ~LIBRARY()
    {
        delete unit;
    }

    void AddPadstack( PADSTACK* aPadstack )
    {
        aPadstack->SetParent( this );
        padstacks.push_back( aPadstack );
    }

/*
    void SetViaStartIndex( int aIndex )
    {
        via_start_index = aIndex;
    }
    int GetViaStartIndex()
    {
        return via_start_index;
    }
*/


    /**
     * Function FindIMAGE
     * searches this LIBRARY for an image which matches the argument.
     * @return int - if found the index into the images list, else -1.
     */
    int FindIMAGE( IMAGE* aImage )
    {
        unsigned i;
        for( i=0;  i<images.size();  ++i )
        {
            if( 0 == IMAGE::Compare( aImage, &images[i] ) )
                return (int) i;
        }

        // There is no match to the IMAGE contents, but now generate a unique
        // name for it.
        int dups = 1;
        for( i=0;  i<images.size();  ++i )
        {
            if( 0 == aImage->image_id.compare( images[i].image_id ) )
                aImage->duplicated = dups++;
        }

        return -1;
    }


    /**
     * Function AppendIMAGE
     * adds the image to the image list.
     */
    void AppendIMAGE( IMAGE* aImage )
    {
        aImage->SetParent( this );
        images.push_back( aImage );
    }

    /**
     * Function LookupIMAGE
     * will add the image only if one exactly like it does not already exist
     * in the image container.
     * @return IMAGE* - the IMAGE which is registered in the LIBRARY that
     *           matches the argument, and it will be either the argument or
     *           a previous image which is a duplicate.
     */
    IMAGE* LookupIMAGE( IMAGE* aImage )
    {
        int ndx = FindIMAGE( aImage );
        if( ndx == -1 )
        {
            AppendIMAGE( aImage );
            return aImage;
        }
        return &images[ndx];
    }

    /**
     * Function FindVia
     * searches this LIBRARY for a via which matches the argument.
     * @return int - if found the index into the padstack list, else -1.
     */
    int FindVia( PADSTACK* aVia )
    {
        for( unsigned i=0;  i<vias.size();  ++i )
        {
            if( 0 == PADSTACK::Compare( aVia, &vias[i] ) )
                return int( i );
        }
        return -1;
    }

    /**
     * Function AppendVia
     * adds \a aVia to the internal via container.
     */
    void AppendVia( PADSTACK* aVia )
    {
        aVia->SetParent( this );
        vias.push_back( aVia );
    }


    /**
     * Function AppendPADSTACK
     * adds the padstack to the padstack container.
     */
    void AppendPADSTACK( PADSTACK* aPadstack )
    {
        aPadstack->SetParent( this );
        padstacks.push_back( aPadstack );
    }

    /**
     * Function LookupVia
     * will add the via only if one exactly like it does not already exist
     * in the padstack container.
     * @return PADSTACK* - the PADSTACK which is registered in the LIBRARY that
     *           matches the argument, and it will be either the argument or
     *           a previous padstack which is a duplicate.
     */
    PADSTACK* LookupVia( PADSTACK* aVia )
    {
        int ndx = FindVia( aVia );
        if( ndx == -1 )
        {
            AppendVia( aVia );
            return aVia;
        }
        return &vias[ndx];
    }

    /**
     * Function FindPADSTACK
     * searches the padstack container by name.
     * @return PADSTACK* - The PADSTACK with a matching name if it exists, else NULL.
     */
    PADSTACK* FindPADSTACK( const std::string& aPadstackId )
    {
        for( unsigned i=0;  i<padstacks.size();  ++i )
        {
            PADSTACK* ps = &padstacks[i];
            if( 0 == ps->GetPadstackId().compare( aPadstackId ) )
                return ps;
        }
        return NULL;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( unit )
            unit->Format( out, nestLevel );

        for( IMAGES::iterator i=images.begin();  i!=images.end();  ++i )
            i->Format( out, nestLevel );

        for( PADSTACKS::iterator i=padstacks.begin();  i!=padstacks.end();  ++i )
            i->Format( out, nestLevel );

        for( PADSTACKS::iterator i=vias.begin();  i!=vias.end();  ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }
};


/**
 * Class PIN_REF
 * corresponds to the &lt;pin_reference&gt; definition in the specctra dsn spec.
 */
struct PIN_REF : public ELEM
{
    std::string     component_id;
    std::string     pin_id;

    PIN_REF( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
    }


    /**
     * Function FormatIt
     * is like Format() but is not virual and returns the number of characters
     * that were output.
     */
    int FormatIt( OUTPUTFORMATTER* out, int nestLevel )
    {
        // only print the newline if there is a nest level, and make
        // the quotes unconditional on this one.
        const char* newline = nestLevel ? "\n" : "";

        const char* cquote = out->GetQuoteChar( component_id.c_str() );
        const char* pquote = out->GetQuoteChar( pin_id.c_str() );

        return out->Print( nestLevel, "%s%s%s-%s%s%s%s",
                cquote, component_id.c_str(), cquote,
                pquote, pin_id.c_str(), pquote,
                newline );
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
        fromto_type  = DSN_T( T_NONE );
    }
    ~FROMTO()
    {
        delete rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        // no quoting on these two, the lexer preserved the quotes on input
        out->Print( nestLevel, "(%s %s %s ",
                 Name(), fromText.c_str(), toText.c_str() );

        if( fromto_type != DSN_T( T_NONE ) )
            out->Print( 0, "(type %s)", GetTokenText( fromto_type ) );

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
typedef boost::ptr_vector<FROMTO>       FROMTOS;


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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s", Name() );

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
typedef boost::ptr_vector<COMP_ORDER>   COMP_ORDERS;

/**
 * Class NET
 * corresponds to a &lt;net_descriptor&gt;
 * in the DSN spec.
 */
class NET : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     net_id;
    bool            unassigned;
    int             net_number;

    DSN_T           pins_type;      ///< T_pins | T_order, type of field 'pins' below
    PIN_REFS        pins;

    PIN_REFS        expose;
    PIN_REFS        noexpose;
    PIN_REFS        source;
    PIN_REFS        load;
    PIN_REFS        terminator;

    DSN_T           type;           ///< T_fix | T_normal

    DSN_T           supply;         ///< T_power | T_ground

    RULE*           rules;

    LAYER_RULES     layer_rules;

    FROMTOS         fromtos;

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
        comp_order = 0;
    }

    ~NET()
    {
        delete rules;
        delete comp_order;
    }

    int FindPIN_REF( const std::string& aComponent )
    {
        for( unsigned i=0;  i<pins.size();  ++i )
        {
            if( 0 == aComponent.compare( pins[i].component_id ) )
                return int(i);
        }
        return -1;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( net_id.c_str() );
        const char* space = " ";

        out->Print( nestLevel, "(%s %s%s%s", Name(),
                   quote, net_id.c_str(), quote );

        if( unassigned )
        {
            out->Print( 0, "%s(unassigned)", space );
            space = "";         // only needed one space
        }

        if( net_number != T_NONE )
        {
            out->Print( 0, "%s(net_number %d)", space, net_number );
            // space = "";
        }

        out->Print( 0, "\n" );

        if( pins.size() )
        {
            const int RIGHTMARGIN = 80;
            int perLine = out->Print( nestLevel+1, "(%s", GetTokenText( pins_type ) );

            for( PIN_REFS::iterator i=pins.begin();  i!=pins.end();  ++i )
            {
                if( perLine > RIGHTMARGIN )
                {
                    out->Print( 0, "\n");
                    perLine = out->Print( nestLevel+2, "%s", "" );
                }
                else
                    perLine += out->Print( 0, " " );

                perLine += i->FormatIt( out, 0 );
            }
            out->Print( 0, ")\n" );
        }

        if( comp_order )
            comp_order->Format( out, nestLevel+1 );

        if( type != T_NONE )
            out->Print( nestLevel+1, "(type %s)\n", GetTokenText( type ) );

        if( rules )
            rules->Format( out, nestLevel+1 );

        for( LAYER_RULES::iterator i=layer_rules.begin();  i!=layer_rules.end();  ++i )
            i->Format( out, nestLevel+1 );

        for( FROMTOS::iterator i=fromtos.begin();  i!=fromtos.end();  ++i )
            i->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }
};
typedef boost::ptr_vector<NET>  NETS;


class TOPOLOGY : public ELEM
{
    friend class SPECCTRA_DB;

    FROMTOS         fromtos;

    COMP_ORDERS     comp_orders;

public:
    TOPOLOGY( ELEM* aParent ) :
        ELEM( T_topology, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        for( FROMTOS::iterator i=fromtos.begin();  i!=fromtos.end();  ++i )
            i->Format( out, nestLevel );

        for( COMP_ORDERS::iterator i=comp_orders.begin();  i!=comp_orders.end();  ++i )
            i->Format( out, nestLevel );
    }
};


/**
 * Class CLASS
 * corresponds to the &lt;class_descriptor&gt; in the specctra spec.
 */
class CLASS : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     class_id;

    STRINGS         net_ids;

    /// circuit descriptor list
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


    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( class_id.c_str() );

        int perLine = out->Print( nestLevel, "(%s %s%s%s",
                              Name(),
                              quote, class_id.c_str(), quote );

        const int RIGHTMARGIN = 72;

        for( STRINGS::iterator i=net_ids.begin();  i!=net_ids.end();  ++i )
        {
            const char* space = " ";
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
                space = ""; // no space at first net_id of the line
            }

            quote = out->GetQuoteChar( i->c_str() );
            perLine += out->Print( 0, "%s%s%s%s", space, quote, i->c_str(), quote );
        }

        bool newLine = false;
        if( circuit.size() || rules || layer_rules.size() || topology )
        {
            out->Print( 0, "\n" );
            newLine = true;
        }

        if( circuit.size() )
        {
            out->Print( nestLevel+1, "(circuit\n" );
            for( STRINGS::iterator i=circuit.begin();  i!=circuit.end();  ++i )
                out->Print( nestLevel+2, "%s\n", i->c_str() );
            out->Print( nestLevel+1, ")\n" );
        }

        if( rules )
            rules->Format( out, nestLevel+1 );

        for( LAYER_RULES::iterator i=layer_rules.begin();  i!=layer_rules.end();  ++i )
            i->Format( out, nestLevel+1 );

        if( topology )
            topology->Format( out, nestLevel+1 );

        out->Print( newLine ? nestLevel : 0, ")\n" );
    }
};
typedef boost::ptr_vector<CLASS> CLASSLIST;


class NETWORK : public ELEM
{
    friend class SPECCTRA_DB;

    NETS        nets;
    CLASSLIST   classes;


public:

    NETWORK( ELEM* aParent ) :
        ELEM( T_network, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        for( NETS::iterator i=nets.begin();  i!=nets.end();  ++i )
            i->Format( out, nestLevel );

        for( CLASSLIST::iterator i=classes.begin();  i!=classes.end();  ++i )
            i->Format( out, nestLevel );
    }
};


class CONNECT : public ELEM
{
    // @todo not completed.

public:
    CONNECT( ELEM* aParent ) :
        ELEM( T_connect, aParent ) {}
};


/**
 * Class WIRE
 * corresponds to &lt;wire_shape_descriptor&gt; in the specctra dsn spec.
 */
class WIRE : public ELEM
{
    friend class SPECCTRA_DB;

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*           shape;

    std::string     net_id;
    int             turret;
    DSN_T           wire_type;
    DSN_T           attr;
    std::string     shield;
    WINDOWS         windows;
    CONNECT*        connect;
    bool            supply;

public:
    WIRE( ELEM* aParent ) :
        ELEM( T_wire, aParent )
    {
        shape = 0;
        connect = 0;

        turret = -1;
        wire_type = T_NONE;
        attr = T_NONE;
        supply = false;
    }

    ~WIRE()
    {
        delete shape;
        delete connect;
    }

    void SetShape( ELEM* aShape )
    {
        delete shape;
        shape = aShape;

        if( aShape )
        {
            wxASSERT(aShape->Type()==T_rect || aShape->Type()==T_circle
                     || aShape->Type()==T_qarc || aShape->Type()==T_path
                     || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( shape )
            shape->Format( out, 0 );

        if( net_id.size() )
        {
            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( 0, "(net %s%s%s)",
                       quote, net_id.c_str(), quote );
        }

        if( turret >= 0 )
            out->Print( 0, "(turrent %d)", turret );

        if( wire_type != T_NONE )
            out->Print( 0, "(type %s)", GetTokenText( wire_type ) );

        if( attr != T_NONE )
            out->Print( 0, "(attr %s)", GetTokenText( attr ) );

        if( shield.size() )
        {
            const char* quote = out->GetQuoteChar( shield.c_str() );
            out->Print( 0, "(shield %s%s%s)",
                       quote, shield.c_str(), quote );
        }

        if( windows.size() )
        {
            out->Print( 0, "\n" );

            for( WINDOWS::iterator i=windows.begin();  i!=windows.end();  ++i )
                i->Format( out, nestLevel+1 );
        }

        if( connect )
            connect->Format( out, 0 );

        if( supply )
            out->Print( 0, "(supply)" );

        out->Print( 0, ")\n" );
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
    DSN_T           via_type;
    DSN_T           attr;
    std::string     virtual_pin_name;
    STRINGS         contact_layers;
    bool            supply;


public:
    WIRE_VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
        via_number = -1;
        via_type = T_NONE;
        attr = T_NONE;
        supply = false;
    }

    const std::string& GetPadstackId()
    {
        return padstack_id;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( padstack_id.c_str() );

        const int RIGHTMARGIN = 80;
        int perLine = out->Print( nestLevel, "(%s %s%s%s",
                       Name(),
                       quote, padstack_id.c_str(), quote );

        for( POINTS::iterator i=vertexes.begin();  i!=vertexes.end();  ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            else
                perLine += out->Print( 0, "  " );

            perLine += out->Print( 0, "%.6g %.6g", i->x, i->y );
        }

        if( net_id.size() || via_number!=-1 || via_type!=T_NONE || attr!=T_NONE || supply)
            out->Print( 0, " " );

        if( net_id.size() )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            quote = out->GetQuoteChar( net_id.c_str() );
            perLine += out->Print( 0, "(net %s%s%s)", quote, net_id.c_str(), quote );
        }

        if( via_number != -1 )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            perLine += out->Print( 0, "(via_number %d)", via_number );
        }

        if( via_type != T_NONE )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            perLine += out->Print( 0, "(type %s)", GetTokenText( via_type ) );
        }

        if( attr != T_NONE )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            if( attr == T_virtual_pin )
            {
                quote = out->GetQuoteChar( virtual_pin_name.c_str() );
                perLine += out->Print( 0, "(attr virtual_pin %s%s%s)",
                           quote, virtual_pin_name.c_str(), quote );
            }
            else
                perLine += out->Print( 0, "(attr %s)", GetTokenText( attr ) );
        }

        if( supply )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            perLine += out->Print( 0, "(supply)" );
        }

        if( contact_layers.size() )
        {
            out->Print( 0, "\n" );
            out->Print( nestLevel+1, "(contact\n" );

            for( STRINGS::iterator i=contact_layers.begin();  i!=contact_layers.end();  ++i )
            {
                quote = out->GetQuoteChar( i->c_str() );
                out->Print( nestLevel+2, "%s%s%s\n", quote, i->c_str(), quote );
            }
            out->Print( nestLevel+1, "))\n" );
        }
        else
            out->Print( 0, ")\n" );
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( unit )
            unit->Format( out, nestLevel );

        for( WIRES::iterator i=wires.begin();  i!=wires.end();  ++i )
            i->Format( out, nestLevel );

        for( WIRE_VIAS::iterator i=wire_vias.begin();  i!=wire_vias.end();  ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES*  GetUnits() const override
    {
        if( unit )
            return unit;

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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( pcbname.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
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

    UNIT_RES*  GetUnits() const override
    {
        if( unit )
            return unit;

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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        char    temp[80];
        struct  tm* tmp;

        tmp = localtime( &time_stamp );
        strftime( temp, sizeof(temp), "%b %d %H : %M : %S %Y", tmp );

        // format the time first to temp
        // filename may be empty, so quote it just in case.
        out->Print( nestLevel, "(%s \"%s\" (created_time %s)\n",
                     Name(),
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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


/**
 * Class SUPPLY_PIN
 * corresponds to the &lt;supply_pin_descriptor&gt; in the specctra dsn spec.
*/
class SUPPLY_PIN : public ELEM
{
    friend class SPECCTRA_DB;

    PIN_REFS        pin_refs;
    std::string     net_id;

public:
    SUPPLY_PIN( ELEM* aParent ) :
        ELEM( T_supply_pin, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        bool singleLine = pin_refs.size() <= 1;
        out->Print( nestLevel, "(%s", Name() );

        if( singleLine )
        {
            out->Print( 0, "%s", " " );
            pin_refs.begin()->Format( out, 0 );
        }
        else
        {
            for( PIN_REFS::iterator i=pin_refs.begin();  i!=pin_refs.end();  ++i )
                i->FormatIt( out, nestLevel+1 );
        }

        if( net_id.size() )
        {
            const char* newline = singleLine ? "" : "\n";

            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( singleLine ? 0 : nestLevel+1,
                       " (net %s%s%s)%s", quote, net_id.c_str(), quote, newline );
        }

        out->Print( singleLine ? 0 : nestLevel, ")\n");
    }
};
typedef boost::ptr_vector<SUPPLY_PIN>   SUPPLY_PINS;


/**
 * Class NET_OUT
 * corresponds to the &lt;net_out_descriptor&gt; of the specctra dsn spec.
 */
class NET_OUT : public ELEM
{
    friend class SPECCTRA_DB;

    std::string     net_id;
    int             net_number;
    RULE*           rules;
    WIRES           wires;
    WIRE_VIAS       wire_vias;
    SUPPLY_PINS     supply_pins;


public:
    NET_OUT( ELEM* aParent ) :
        ELEM( T_net_out, aParent )
    {
        rules = 0;
        net_number = -1;
    }
    ~NET_OUT()
    {
        delete rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( net_id.c_str() );

        // cannot use Type() here, it is T_net_out and we need "(net "
        out->Print( nestLevel, "(net %s%s%s\n",
                   quote, net_id.c_str(), quote );

        if( net_number>= 0 )
            out->Print( nestLevel+1, "(net_number %d)\n", net_number );

        if( rules )
            rules->Format( out, nestLevel+1 );

        for( WIRES::iterator i=wires.begin();  i!=wires.end();  ++i )
            i->Format( out, nestLevel+1 );

        for( WIRE_VIAS::iterator i=wire_vias.begin();  i!=wire_vias.end();  ++i )
            i->Format( out, nestLevel+1 );

        for( SUPPLY_PINS::iterator i=supply_pins.begin();  i!=supply_pins.end();  ++i )
            i->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }
};
typedef boost::ptr_vector<NET_OUT>      NET_OUTS;


class ROUTE : public ELEM
{
    friend class SPECCTRA_DB;

    UNIT_RES*       resolution;
    PARSER*         parser;
    STRUCTURE_OUT*  structure_out;
    LIBRARY*        library;
    NET_OUTS        net_outs;
//    TEST_POINTS*    test_points;

public:

    ROUTE( ELEM* aParent ) :
        ELEM( T_route, aParent )
    {
        resolution = 0;
        parser = 0;
        structure_out = 0;
        library = 0;
    }
    ~ROUTE()
    {
        delete resolution;
        delete parser;
        delete structure_out;
        delete library;
//        delete test_points;
    }

    UNIT_RES*  GetUnits() const override
    {
        if( resolution )
            return resolution;

        return ELEM::GetUnits();
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        if( resolution )
            resolution->Format( out, nestLevel );

        if( parser )
            parser->Format( out, nestLevel );

        if( structure_out )
            structure_out->Format( out, nestLevel );

        if( library )
            library->Format( out, nestLevel );

        if( net_outs.size() )
        {
            out->Print( nestLevel, "(network_out\n" );
            for( NET_OUTS::iterator i=net_outs.begin();  i!=net_outs.end();  ++i )
                i->Format( out, nestLevel+1 );
            out->Print( nestLevel, ")\n" );
        }

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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel )  override
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
        ELEM( T_session, aParent )
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

    void Format( OUTPUTFORMATTER* out, int nestLevel )  override
    {
        const char* quote = out->GetQuoteChar( session_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", Name(),
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

typedef boost::ptr_set<PADSTACK>    PADSTACKSET;


/**
 * Class SPECCTRA_DB
 * holds a DSN data tree, usually coming from a DSN file. Is essentially a
 * SPECCTRA_PARSER class.
 */
class SPECCTRA_DB : public SPECCTRA_LEXER
{
    /// specctra DSN keywords
    static const KEYWORD keywords[];
    static const unsigned keywordCount;

    PCB*            pcb;
    SESSION*        session;
    wxString        filename;
    std::string     quote_char;

    bool            modulesAreFlipped;

    STRING_FORMATTER sf;

    STRINGS         layerIds;       ///< indexed by PCB layer number

    /// maps BOARD layer number to PCB layer numbers
    std::vector<int> kicadLayer2pcb;

    /// maps PCB layer number to BOARD layer numbers
    std::vector<PCB_LAYER_ID>   pcbLayer2kicad;

    /// used during FromSESSION() only, memory for it is not owned here.
    UNIT_RES*       routeResolution;

    /// a copy to avoid passing as an argument, memory for it is not owned here.
    BOARD*          sessionBoard;

    static const KICAD_T scanPADs[];

    PADSTACKSET     padstackset;

    /// we don't want ownership here permanently, so we don't use boost::ptr_vector
    std::vector<NET*>   nets;

    /// specctra cu layers, 0 based index:
    int     m_top_via_layer;
    int     m_bot_via_layer;


    /**
     * Function buildLayerMaps
     * creates a few data translation structures for layer name and number
     * mapping between the DSN::PCB structure and the KiCad BOARD structure.
     * @param aBoard The BOARD to create the maps for.
     */
    void buildLayerMaps( BOARD* aBoard );

    /**
     * Function findLayerName
     * returns the PCB layer index for a given layer name, within the specctra session
     * file.
     *
     * @return int - the layer index within the specctra session file, or -1 if
     *  aLayerName is not found.
     */
    int findLayerName( const std::string& aLayerName ) const;

    /**
     * Function readCOMPnPIN
     * reads a &lt;pin_reference&gt; and splits it into the two parts which are
     * on either side of the hyphen.  This function is specialized because
     * pin_reference may or may not be using double quotes.  Both of these
     * are legal:  U2-14 or "U2"-"14".  The lexer treats the first one as a
     * single T_SYMBOL, so in that case we have to split it into two here.
     * <p>
     * The caller should have already read in the first token comprizing the
     * pin_reference and it will be tested through CurTok().
     *
     * @param component_id Where to put the text preceeding the '-' hyphen.
     * @param pid_id Where to put the text which trails the '-'.
     * @throw IO_ERROR, if the next token or two do no make up a pin_reference,
     * or there is an error reading from the input stream.
     */
    void readCOMPnPIN( std::string* component_id, std::string* pid_id );

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
     * @throw IO_ERROR, if the next token or 8 do no make up a time stamp,
     * or there is an error reading from the input stream.
     */
    void readTIME( time_t* time_stamp );

    void doPCB( PCB* growth );
    void doPARSER( PARSER* growth );
    void doRESOLUTION( UNIT_RES* growth );
    void doUNIT( UNIT_RES* growth );
    void doSTRUCTURE( STRUCTURE* growth );
    void doSTRUCTURE_OUT( STRUCTURE_OUT* growth );
    void doLAYER_NOISE_WEIGHT( LAYER_NOISE_WEIGHT* growth );
    void doSPECCTRA_LAYER_PAIR( SPECCTRA_LAYER_PAIR* growth );
    void doBOUNDARY( BOUNDARY* growth );
    void doRECTANGLE( RECTANGLE* growth );
    void doPATH( PATH* growth );
    void doSTRINGPROP( STRINGPROP* growth );
    void doTOKPROP( TOKPROP* growth );
    void doVIA( VIA* growth );
    void doCONTROL( CONTROL* growth );
    void doLAYER( LAYER* growth );
    void doRULE( RULE* growth );
    void doKEEPOUT( KEEPOUT* growth );
    void doCIRCLE( CIRCLE* growth );
    void doQARC( QARC* growth );
    void doWINDOW( WINDOW* growth );
    void doCONNECT( CONNECT* growth );
    void doREGION( REGION* growth );
    void doCLASS_CLASS( CLASS_CLASS* growth );
    void doLAYER_RULE( LAYER_RULE* growth );
    void doCLASSES( CLASSES* growth );
    void doGRID( GRID* growth );
    void doPLACE( PLACE* growth );
    void doCOMPONENT( COMPONENT* growth );
    void doPLACEMENT( PLACEMENT* growth );
    void doPROPERTIES( PROPERTIES* growth );
    void doPADSTACK( PADSTACK* growth );
    void doSHAPE( SHAPE* growth );
    void doIMAGE( IMAGE* growth );
    void doLIBRARY( LIBRARY* growth );
    void doPIN( PIN* growth );
    void doNET( NET* growth );
    void doNETWORK( NETWORK* growth );
    void doCLASS( CLASS* growth );
    void doTOPOLOGY( TOPOLOGY* growth );
    void doFROMTO( FROMTO* growth );
    void doCOMP_ORDER( COMP_ORDER* growth );
    void doWIRE( WIRE* growth );
    void doWIRE_VIA( WIRE_VIA* growth );
    void doWIRING( WIRING* growth );
    void doSESSION( SESSION* growth );
    void doANCESTOR( ANCESTOR* growth );
    void doHISTORY( HISTORY* growth );
    void doROUTE( ROUTE* growth );
    void doWAS_IS( WAS_IS* growth );
    void doNET_OUT( NET_OUT* growth );
    void doSUPPLY_PIN( SUPPLY_PIN* growth );

    //-----<FromBOARD>-------------------------------------------------------

    /**
     * Function fillBOUNDARY
     * makes the board perimeter for the DSN file by filling the BOUNDARY element
     * in the specctra element tree.
     * @param aBoard The BOARD to get information from in order to make the BOUNDARY.
     * @param aBoundary The empty BOUNDARY to fill in.
     */
    void fillBOUNDARY( BOARD* aBoard, BOUNDARY* aBoundary );

    /**
     * Function makeIMAGE
     * allocates an IMAGE on the heap and creates all the PINs according
     * to the D_PADs in the MODULE.
     * @param aBoard The owner of the MODULE.
     * @param aModule The MODULE from which to build the IMAGE.
     * @return IMAGE* - not tested for duplication yet.
     */
    IMAGE* makeIMAGE( BOARD* aBoard, MODULE* aModule );

    /**
     * Function makePADSTACK
     * creates a PADSTACK which matches the given pad.  Only pads which do not
     * satisfy the function isKeepout() should be passed to this function.
     * @param aBoard The owner of the MODULE.
     * @param aPad The D_PAD which needs to be made into a PADSTACK.
     * @return PADSTACK* - The created padstack, including its padstack_id.
     */
    PADSTACK* makePADSTACK( BOARD* aBoard, D_PAD* aPad );

    /**
     * Function makeVia
     * makes a round through hole PADSTACK using the given KiCad diameter in deci-mils.
     * @param aCopperDiameter The diameter of the copper pad.
     * @param aDrillDiameter The drill diameter, used on re-import of the session file.
     * @param aTopLayer The DSN::PCB top most layer index.
     * @param aBotLayer The DSN::PCB bottom most layer index.
     * @return PADSTACK* - The padstack, which is on the heap only, user must save
     *  or delete it.
     */
    PADSTACK* makeVia( int aCopperDiameter, int aDrillDiameter,
                       int aTopLayer, int aBotLayer );

    /**
     * Function makeVia
     * makes any kind of PADSTACK using the given KiCad VIA.
     * @param aVia The VIA to build the padstack from.
     * @return PADSTACK* - The padstack, which is on the heap only, user must save
     *  or delete it.
     */
    PADSTACK* makeVia( const ::VIA* aVia );

    /**
     * Function deleteNETs
     * deletes all the NETs that may be in here.
     */
    void deleteNETs()
    {
        for( unsigned n=0;  n<nets.size();  ++n )
            delete nets[n];

        nets.clear();
    }

    /**
     * Function exportNETCLASS
     * exports \a aNetClass to the DSN file.
     */
    void exportNETCLASS( const std::shared_ptr<NETCLASS>& aNetClass, BOARD* aBoard );

    //-----</FromBOARD>------------------------------------------------------

    //-----<FromSESSION>-----------------------------------------------------

    /**
     * Function makeTRACK
     * creates a TRACK form the PATH and BOARD info.
     */
    TRACK* makeTRACK( PATH* aPath, int aPointIndex, int aNetcode );

    /**
     * Function makeVIA
     * instantiates a KiCad VIA on the heap and initializes it with internal
     * values consistent with the given PADSTACK, POINT, and netcode.
     */
    ::VIA* makeVIA( PADSTACK* aPadstack, const POINT& aPoint, int aNetCode, int aViaDrillDefault );

    //-----</FromSESSION>----------------------------------------------------

public:

    SPECCTRA_DB() :
        SPECCTRA_LEXER( 0 )         // LINE_READER* == NULL, no DSNLEXER::PushReader()
    {
        // The LINE_READER will be pushed from an automatic instantiation,
        // we don't own it:
        wxASSERT( !iOwnReaders );

        pcb   = 0;
        session = 0;
        quote_char += '"';
        modulesAreFlipped = false;

        SetSpecctraMode( true );

        // Avoid not initialized members:
        routeResolution = NULL;
        sessionBoard = NULL;
        m_top_via_layer = 0;
        m_bot_via_layer = 0;
    }

    virtual ~SPECCTRA_DB()
    {
        delete pcb;
        delete session;

        deleteNETs();
    }

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
    PCB*  GetPCB()  { return pcb; }

    /**
     * Function SetSESSION
     * deletes any existing SESSION and replaces it with the given one.
     */
    void SetSESSION( SESSION* aSession )
    {
        delete session;
        session = aSession;
    }
    SESSION* GetSESSION() { return session; }

    /**
     * Function LoadPCB
     * is a recursive descent parser for a SPECCTRA DSN "design" file.
     * A design file is nearly a full description of a PCB (seems to be
     * missing only the silkscreen stuff).
     *
     * @param aFilename The name of the dsn file to load.
     * @throw IO_ERROR if there is a lexer or parser error.
     */
    void LoadPCB( const wxString& aFilename );

    /**
     * Function LoadSESSION
     * is a recursive descent parser for a SPECCTRA DSN "session" file.
     * A session file is a file that is fed back from the router to the layout
     * tool (Pcbnew) and should be used to update a BOARD object with the new
     * tracks, vias, and component locations.
     *
     * @param aFilename The name of the dsn file to load.
     * @throw IO_ERROR if there is a lexer or parser error.
     */
    void LoadSESSION( const wxString& aFilename );

    /**
     * Function ExportPCB
     * writes the internal PCB instance out as a SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     * @param aNameChange If true, causes the pcb's name to change to "aFilename"
     *          and also to to be changed in the output file.
     * @throw IO_ERROR, if an i/o error occurs saving the file.
     */
    void ExportPCB( const wxString& aFilename,  bool aNameChange=false );

    /**
     * Function FromBOARD
     * adds the entire BOARD to the PCB but does not write it out.  Note that the BOARD given 
     * to this function must have all the MODULEs on the component side of the BOARD.
     *
     * See PCB_EDIT_FRAME::ExportToSpecctra() for an example before calling this function.
     *
     * @param aBoard The BOARD to convert to a PCB.
     */
    void FromBOARD( BOARD* aBoard );

    /**
     * Function FromSESSION
     * adds the entire SESSION info to a BOARD but does not write it out.  The BOARD given to 
     * this function will have all its tracks and via's replaced, and all its components are 
     * subject to being moved.
     *
     * @param aBoard The BOARD to merge the SESSION information into.
     */
    void FromSESSION( BOARD* aBoard );

    /**
     * Function ExportSESSION
     * writes the internal SESSION instance out as a SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     */
    void ExportSESSION( const wxString& aFilename );

    /**
     * Function FlipMODULEs
     * flips the modules which are on the back side of the board to the front.
     */
    void FlipMODULEs( BOARD* aBoard );

    /**
     * Function RevertMODULEs
     * flips the modules which were on the back side of the board back to the back.
     */
    void RevertMODULEs( BOARD* aBoard );
};


}           // namespace DSN

#endif      // SPECCTRA_H_

//EOF
