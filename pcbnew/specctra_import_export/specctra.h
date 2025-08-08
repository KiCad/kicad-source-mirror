/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <specctra_import_export/specctra_lexer.h>

#include <map>
#include <memory>

#include <core/typeinfo.h>
#include <geometry/shape_poly_set.h>
#include <layer_ids.h>

// all outside the DSN namespace:
class BOARD;
class PAD;
class PCB_TRACK;
class PCB_ARC;
class PCB_VIA;
class NETCLASS;
class FOOTPRINT;
class SHAPE_POLY_SET;

typedef DSN::T  DSN_T;


/**
 * This source file implements export and import capabilities to the
 * specctra dsn file format.  The grammar for that file format is documented
 * fairly well.  There are classes for each major type of descriptor in the
 * spec.
 *
 * Since there are so many classes in here, it may be helpful to generate
 * the Doxygen directory:
 *
 * $ cd &ltkicadSourceRoot&gt
 * $ doxygen
 *
 * Then you can view the html documentation in the &ltkicadSourceRoot&gt/doxygen
 * directory.  The main class in this file is SPECCTRA_DB and its main
 * functions are LoadPCB(), LoadSESSION(), and ExportPCB().
 *
 * Wide use is made of boost::ptr_vector&lt&gt and std::vector&lt&gt template classes.
 * If the contained object is small, then std::vector tends to be used.
 * If the contained object is large, variable size, or would require writing
 * an assignment operator() or copy constructor, then boost::ptr_vector
 * cannot be beat.
 */
namespace DSN {


class SPECCTRA_DB;


/**
 * @brief Helper method to export board to DSN file
 * @param aBoard board object
 * @param aFullFilename specctra file name
 */
void ExportBoardToSpecctraFile( BOARD* aBoard, const wxString& aFullFilename );


/**
 * The DSN namespace and returns the C string representing a SPECCTRA_DB::keyword.
 *
 * We needed a non-instance function to get at the SPECCTRA_DB::keyword[] and class
 * SPECCTRA_DB is not defined yet.
 */
const char* GetTokenText( T aTok );


/**
 * A point in the SPECCTRA DSN coordinate system.
 *
 * It can also be used to hold a distance (vector really) from some origin.
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
     * Change negative zero to positive zero in the IEEE floating point storage format.
     *
     * Basically turns off the sign bit if the mantissa and exponent say the value is otherwise
     * zero.
     */
    void FixNegativeZero()
    {
        if( x == -0.0 )
            x = 0.0;

        if( y == -0.0 )
            y = 0.0;
    }

    /**
     * Write this object as ASCII out to an OUTPUTFORMATTER according to the SPECCTRA DSN format.
     *
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to precede the output with.
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
     * Write this object as ASCII out to an OUTPUTFORMATTER according to the SPECCTRA DSN format.
     *
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to precede the output with.
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
 * A base class for any DSN element class.
 *
 * See class #ELEM_HOLDER also.
 */
class ELEM
{
public:

    ELEM( DSN_T aType, ELEM* aParent = nullptr );

    virtual ~ELEM();

    DSN_T   Type() const { return type; }

    const char* Name() const;


    /**
     * Return the units for this section.
     *
     * Derived classes may override this to check for section specific overrides.
     *
     * @return an element from a local or parent scope.
     */
    virtual UNIT_RES* GetUnits() const;

    /**
     * Write this object as ASCII out to an OUTPUTFORMATTER according to the SPECCTRA DSN format.
     *
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to precede the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    virtual void Format( OUTPUTFORMATTER* out, int nestLevel );

    /**
     * Write the contents as ASCII out to an OUTPUTFORMATTER according to the SPECCTRA DSN format.
     *
     * This is the same as Format() except that the outer wrapper is not included.
     *
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to precede the output with.
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

protected:

    /**
     * Return a string which uniquely represents this ELEM among other ELEMs of the same
     * derived class as "this" one.
     *
     * It is not usable for all derived classes, only those which plan for
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

    DSN_T           type;
    ELEM*           parent;

private:
    friend class SPECCTRA_DB;
};


/**
 * A holder for any DSN class.
 *
 * It can contain other class instances, including classes derived from this class.
 */
class ELEM_HOLDER : public ELEM
{
public:

    ELEM_HOLDER( DSN_T aType, ELEM* aParent = nullptr ) :
        ELEM( aType, aParent )
    {
    }

    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override;


    //-----< list operations >--------------------------------------------

    /**
     * Find a particular instance number of a given type of ELEM.
     *
     * @param aType The type of ELEM to find
     * @param instanceNum The instance number of to find: 0 for first, 1 for second, etc.
     * @return int - The index into the kids array or -1 if not found.
     */
    int FindElem( DSN_T aType, int instanceNum = 0 );


    /**
     * Return the number of ELEMs in this holder.
     *
     * @return the count of children elements.
     */
    int Length() const
    {
        return kids.size();
    }

    void Append( ELEM* aElem )
    {
        kids.push_back( aElem );
    }

    ELEM* Replace( int aIndex, ELEM* aElem )
    {
        ELEM_ARRAY::auto_type ret = kids.replace( aIndex, aElem );
        return ret.release();
    }

    ELEM* Remove( int aIndex )
    {
        ELEM_ARRAY::auto_type ret = kids.release( kids.begin() + aIndex );
        return ret.release();
    }

    void Insert( int aIndex, ELEM* aElem ) { kids.insert( kids.begin() + aIndex, aElem ); }

    ELEM* At( int aIndex ) const
    {
        // we have varying sized objects and are using polymorphism, so we
        // must return a pointer not a reference.
        return (ELEM*) &kids[aIndex];
    }

    ELEM* operator[]( int aIndex ) const
    {
        return At( aIndex );
    }

    void Delete( int aIndex ) { kids.erase( kids.begin() + aIndex ); }

private:
    friend class SPECCTRA_DB;

    typedef boost::ptr_vector<ELEM> ELEM_ARRAY;

    ELEM_ARRAY      kids;      ///< ELEM pointers
};


/**
 * A configuration record per the SPECCTRA DSN file spec.
 *
 * It is not actually a parser, but rather corresponds to &lt;parser_descriptor&gt;
 */
class PARSER : public ELEM
{
public:

    PARSER( ELEM* aParent );

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override;

private:
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
};


/**
 * A holder for either a T_unit or T_resolution object which are usually
 * mutually exclusive in the dsn grammar, except within the T_pcb level.
 */
class UNIT_RES : public ELEM
{
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

    DSN_T GetEngUnits() const  { return units; }
    int GetValue() const  { return value; }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( type == T_unit )
            out->Print( nestLevel, "(%s %s)\n", Name(), GetTokenText( units ) );
        else    // T_resolution
            out->Print( nestLevel, "(%s %s %d)\n", Name(), GetTokenText( units ), value );
    }

private:
    friend class SPECCTRA_DB;

    DSN_T       units;
    int         value;
};


class RECTANGLE : public ELEM
{
public:

    RECTANGLE( ELEM* aParent ) :
        ELEM( T_rect, aParent )
    {
    }

    void SetLayerId( std::string& aLayerId )
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

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
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

private:
    friend class SPECCTRA_DB;

    std::string     layer_id;

    POINT           point0;         ///< one of two opposite corners
    POINT           point1;
};


/**
 * A &lt;rule_descriptor&gt; in the specctra dsn spec.
 */
class RULE : public ELEM
{
public:

    RULE( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s", Name() );

        bool singleLine;

        if( m_rules.size() == 1 )
        {
            singleLine = true;
            out->Print( 0, " %s)", m_rules.begin()->c_str() );
        }

        else
        {
            out->Print( 0, "\n" );
            singleLine = false;

            for( STRINGS::const_iterator i = m_rules.begin();  i != m_rules.end(); ++i )
                out->Print( nestLevel+1, "%s\n", i->c_str() );

            out->Print( nestLevel, ")" );
        }

        if( nestLevel || !singleLine )
            out->Print( 0, "\n" );
    }

private:
    friend class SPECCTRA_DB;

    STRINGS      m_rules;      ///< rules are saved in std::string form.
};


class LAYER_RULE : public ELEM
{
public:

    LAYER_RULE( ELEM* aParent ) :
        ELEM( T_layer_rule, aParent )
    {
        m_rule = nullptr;
    }

    ~LAYER_RULE()
    {
        delete m_rule;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s", Name() );

        for( STRINGS::const_iterator i = m_layer_ids.begin(); i != m_layer_ids.end(); ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }

        out->Print( 0 , "\n" );

        if( m_rule )
            m_rule->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    STRINGS m_layer_ids;
    RULE*   m_rule;
};


typedef boost::ptr_vector<LAYER_RULE>   LAYER_RULES;


/**
 * Support both the &lt;path_descriptor&gt; and the &lt;polygon_descriptor&gt; per
 * the specctra dsn spec.
 */
class PATH : public ELEM
{
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

    void SetLayerId( const std::string& aLayerId )
    {
        layer_id = aLayerId;
    }

    void SetAperture( double aWidth )
    {
        aperture_width = aWidth;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );

        const int RIGHTMARGIN = 70;
        int perLine = out->Print( nestLevel, "(%s %s%s%s %.6g",
                                  Name(),
                                  quote, layer_id.c_str(), quote,
                                  aperture_width );

        int wrapNest = std::max( nestLevel+1, 6 );

        for( unsigned i = 0; i < points.size(); ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( wrapNest, "%s", "" );
            }
            else
            {
                perLine += out->Print( 0, "  " );
            }

            perLine += out->Print( 0, "%.6g %.6g", points[i].x, points[i].y );
        }

        if( aperture_type == T_square )
        {
            out->Print( 0, "(aperture_type square)" );
        }

        out->Print( 0, ")%s", newline );
    }

private:
    friend class SPECCTRA_DB;

    std::string     layer_id;
    double          aperture_width;

    POINTS          points;
    DSN_T           aperture_type;
};

typedef boost::ptr_vector<PATH> PATHS;


class BOUNDARY : public ELEM
{
public:

    BOUNDARY( ELEM* aParent, DSN_T aType = T_boundary ) :
        ELEM( aType, aParent )
    {
        rectangle = nullptr;
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


    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s\n", Name() );

        if( rectangle )
            rectangle->Format( out, nestLevel+1 );
        else
        {
            for( PATHS::iterator i = paths.begin(); i != paths.end(); ++i )
                i->Format( out, nestLevel+1 );
        }

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    // only one or the other of these two is used, not both
    PATHS           paths;
    RECTANGLE*      rectangle;
};


class CIRCLE : public ELEM
{
public:
    CIRCLE( ELEM* aParent ) :
        ELEM( T_circle, aParent )
    {
        diameter = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %.6g", Name(), quote, layer_id.c_str(), quote,
                    diameter );

        if( vertex.x!=0.0 || vertex.y!=0.0 )
            out->Print( 0, " %.6g %.6g)%s", vertex.x, vertex.y, newline );
        else
            out->Print( 0, ")%s", newline );
    }

    void SetLayerId( const std::string& aLayerId )
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

private:
    friend class SPECCTRA_DB;

    std::string layer_id;

    double      diameter;
    POINT       vertex;     // POINT's constructor sets to (0,0)
};


class QARC : public ELEM
{
public:
    QARC( ELEM* aParent ) :
        ELEM( T_qarc, aParent )
    {
        aperture_width = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* newline = nestLevel ? "\n" : "";

        const char* quote = out->GetQuoteChar( layer_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s %.6g", Name(), quote, layer_id.c_str(), quote,
                                 aperture_width );

        for( int i=0;  i<3;  ++i )
            out->Print( 0, "  %.6g %.6g", vertex[i].x, vertex[i].y );

        out->Print( 0, ")%s", newline );
    }

    void SetLayerId( std::string& aLayerId )
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

private:
    friend class SPECCTRA_DB;

    std::string layer_id;
    double      aperture_width;
    POINT       vertex[3];
};


class WINDOW : public ELEM
{
public:

    WINDOW( ELEM* aParent, DSN_T aType = T_window ) :
        ELEM( aType, aParent )
    {
        shape = nullptr;
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
            wxASSERT( aShape->Type()==T_rect
                        || aShape->Type()==T_circle
                        || aShape->Type()==T_qarc
                        || aShape->Type()==T_path
                        || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( shape )
            shape->Format( out, 0 );

        out->Print( 0, ")\n" );
    }

protected:
    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*       shape;

private:
    friend class SPECCTRA_DB;
};

typedef boost::ptr_vector<WINDOW>   WINDOWS;


/**
 * Used for &lt;keepout_descriptor&gt; and &lt;plane_descriptor&gt;.
 */
class KEEPOUT : public ELEM
{
public:

    /**
     * Require a DSN_T because this class is used for T_place_keepout, T_via_keepout,
     * T_wire_keepout, T_bend_keepout, and T_elongate_keepout as well as T_keepout.
     */
    KEEPOUT( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        m_rules = nullptr;
        m_place_rules = nullptr;
        m_shape = nullptr;

        m_sequence_number = -1;
    }

    ~KEEPOUT()
    {
        delete m_rules;
        delete m_place_rules;
        delete m_shape;
    }

    void SetShape( ELEM* aShape )
    {
        delete m_shape;
        m_shape = aShape;

        if( aShape )
        {
            wxASSERT( aShape->Type()==T_rect
                            || aShape->Type()==T_circle
                            || aShape->Type()==T_qarc
                            || aShape->Type()==T_path
                            || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void AddWindow( WINDOW* aWindow )
    {
        aWindow->SetParent( this );
        m_windows.push_back( aWindow );
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* newline = "\n";

        out->Print( nestLevel, "(%s", Name() );

        if( m_name.size() )
        {
            const char* quote = out->GetQuoteChar( m_name.c_str() );
            out->Print( 0, " %s%s%s", quote, m_name.c_str(), quote );
        }
        // Could be not needed:
#if 1
        else
        {
            out->Print( 0, " \"\"" );   // the zone with no name or net_code == 0
        }
#endif

        if( m_sequence_number != -1 )
            out->Print( 0, " (sequence_number %d)", m_sequence_number );

        if( m_shape )
        {
            out->Print( 0, " " );
            m_shape->Format( out, 0 );
        }

        if( m_rules )
        {
            out->Print( 0, "%s", newline );
            newline = "";
            m_rules->Format( out, nestLevel+1 );
        }

        if( m_place_rules )
        {
            out->Print( 0, "%s", newline );
            newline = "";
            m_place_rules->Format( out, nestLevel+1 );
        }

        if( m_windows.size() )
        {
            out->Print( 0, "%s", newline );
            newline = "";

            for( WINDOWS::iterator i = m_windows.begin(); i != m_windows.end(); ++i )
                i->Format( out, nestLevel+1 );

            out->Print( nestLevel, ")\n" );
        }
        else
        {
            out->Print( 0, ")\n" );
        }
    }

protected:
    std::string     m_name;
    int             m_sequence_number;
    RULE*           m_rules;
    RULE*           m_place_rules;

    WINDOWS         m_windows;

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*           m_shape;

private:
    friend class SPECCTRA_DB;
};

typedef boost::ptr_vector<KEEPOUT>  KEEPOUTS;


/**
 * A &lt;via_descriptor&gt; in the specctra dsn spec.
 */
class VIA : public ELEM
{
public:

    VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
    }

    void AppendVia( const char* aViaName )
    {
        m_padstacks.push_back( aViaName );
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const int RIGHTMARGIN = 80;
        int perLine = out->Print( nestLevel, "(%s", Name() );

        for( STRINGS::iterator i = m_padstacks.begin(); i != m_padstacks.end(); ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "");
            }

            const char* quote = out->GetQuoteChar( i->c_str() );
            perLine += out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }

        if( m_spares.size() )
        {
            out->Print( 0, "\n" );

            perLine = out->Print( nestLevel+1, "(spare" );

            for( STRINGS::iterator i = m_spares.begin(); i != m_spares.end(); ++i )
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

private:
    friend class SPECCTRA_DB;

    STRINGS     m_padstacks;
    STRINGS     m_spares;
};


class CLASSES : public ELEM
{
public:
    CLASSES( ELEM* aParent ) :
        ELEM( T_classes, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( STRINGS::iterator i = class_ids.begin(); i != class_ids.end(); ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, i->c_str(), quote );
        }
    }

private:
    friend class SPECCTRA_DB;

    STRINGS         class_ids;
};


class CLASS_CLASS : public ELEM_HOLDER
{
public:

    /**
     * @param aParent is the parent element of the object.
     * @param aType May be either T_class_class or T_region_class_class
     */
    CLASS_CLASS( ELEM* aParent, DSN_T aType ) :
        ELEM_HOLDER( aType, aParent )
    {
        classes = nullptr;
    }

    ~CLASS_CLASS()
    {
        delete classes;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( classes )
            classes->Format( out, nestLevel );

        // format the kids
        ELEM_HOLDER::FormatContents( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    CLASSES*        classes;

    // rule | layer_rule are put into the kids container.
};


class CONTROL : public ELEM_HOLDER
{
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

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s\n", Name() );

        out->Print( nestLevel+1, "(via_at_smd %s", via_at_smd ? "on" : "off" );

        if( via_at_smd_grid_on )
            out->Print( 0, " grid %s", via_at_smd_grid_on ? "on" : "off" );

        out->Print( 0, ")\n" );

        for( int i = 0; i < Length(); ++i )
            At(i)->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    bool    via_at_smd;
    bool    via_at_smd_grid_on;
};


class LAYER : public ELEM
{
public:
    LAYER( ELEM* aParent ) :
        ELEM( T_layer, aParent )
    {
        layer_type = T_signal;
        direction  = -1;
        cost       = -1;
        cost_type  = -1;

        rules = nullptr;
    }

    ~LAYER()
    {
        delete rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( name.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, name.c_str(), quote );

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
            out->Print( nestLevel+1, "(direction %s)\n", GetTokenText( (DSN_T)direction ) );

        if( rules )
            rules->Format( out, nestLevel+1 );

        if( cost != -1 )
        {
            if( cost < 0 )
                // positive integer, stored as negative.
                out->Print( nestLevel+1, "(cost %d", -cost );
            else
                out->Print( nestLevel+1, "(cost %s", GetTokenText( (DSN_T)cost ) );

            if( cost_type != -1 )
                out->Print( 0, " (type %s)", GetTokenText( (DSN_T)cost_type ) );

            out->Print( 0, ")\n" );
        }

        if( use_net.size() )
        {
            out->Print( nestLevel+1, "(use_net" );

            for( STRINGS::const_iterator i = use_net.begin(); i != use_net.end(); ++i )
            {
                quote = out->GetQuoteChar( i->c_str() );
                out->Print( 0, " %s%s%s",  quote, i->c_str(), quote );
            }

            out->Print( 0, ")\n" );
        }

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string name;
    DSN_T       layer_type; ///< one of: T_signal, T_power, T_mixed, T_jumper
    int         direction;

    ///< [forbidden | high | medium | low | free | \<positive_integer\> | -1]
    int         cost;
    int         cost_type;  ///< T_length | T_way
    RULE*       rules;
    STRINGS     use_net;

    PROPERTIES  properties;
};

typedef boost::ptr_vector<LAYER>    LAYERS;


class SPECCTRA_LAYER_PAIR : public ELEM
{
public:
    SPECCTRA_LAYER_PAIR( ELEM* aParent ) :
        ELEM( T_layer_pair, aParent )
    {
        layer_weight = 0.0;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote0 = out->GetQuoteChar( layer_id0.c_str() );
        const char* quote1 = out->GetQuoteChar( layer_id1.c_str() );

        out->Print( nestLevel, "(%s %s%s%s %s%s%s %.6g)\n", Name(), quote0, layer_id0.c_str(),
                    quote0, quote1, layer_id1.c_str(), quote1, layer_weight );
    }

private:
    friend class SPECCTRA_DB;

    std::string     layer_id0;
    std::string     layer_id1;

    double          layer_weight;
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

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s\n", Name() );

        for( SPECCTRA_LAYER_PAIRS::iterator i = layer_pairs.begin(); i != layer_pairs.end(); ++i )
            i->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }
};


/**
 * A &lt;plane_descriptor&gt; in the specctra dsn spec.
 */
class COPPER_PLANE : public KEEPOUT
{
public:
    COPPER_PLANE( ELEM* aParent ) :
        KEEPOUT( aParent, T_plane )
    {}

private:
    friend class SPECCTRA_DB;
};

typedef boost::ptr_vector<COPPER_PLANE>    COPPER_PLANES;


/**
 * A container for a single property whose value is another DSN_T token.
 *
 * The name of the property is obtained from the DSN_T Type().
 */
class TOKPROP : public ELEM
{
public:
    TOKPROP( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
        // Do not leave uninitialized members
        value = T_NONE;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s %s)\n", Name(), GetTokenText( value ) );
    }

private:
    friend class SPECCTRA_DB;

    DSN_T       value;
};


/**
 * A container for a single property whose value is a string.
 *
 * The name of the property is obtained from the DSN_T.
 */
class STRINGPROP : public ELEM
{
public:
    STRINGPROP( ELEM* aParent, DSN_T aType ) :
        ELEM( aType, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( value.c_str() );

        out->Print( nestLevel, "(%s %s%s%s)\n",
                               Name(),
                               quote, value.c_str(), quote );
    }

private:
    friend class SPECCTRA_DB;

    std::string     value;
};


class REGION : public ELEM_HOLDER
{
public:
    REGION( ELEM* aParent ) :
        ELEM_HOLDER( T_region, aParent )
    {
        m_rectangle = nullptr;
        m_polygon = nullptr;
        m_rules = nullptr;
    }

    ~REGION()
    {
        delete m_rectangle;
        delete m_polygon;
        delete m_rules;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_region_id.size() )
        {
            const char* quote = out->GetQuoteChar( m_region_id.c_str() );
            out->Print( nestLevel, "%s%s%s\n", quote, m_region_id.c_str(), quote );
        }

        if( m_rectangle )
            m_rectangle->Format( out, nestLevel );

        if( m_polygon )
            m_polygon->Format( out, nestLevel );

        ELEM_HOLDER::FormatContents( out, nestLevel );

        if( m_rules )
            m_rules->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_region_id;

    //-----<mutually exclusive>--------------------------------------
    RECTANGLE*      m_rectangle;
    PATH*           m_polygon;
    //-----</mutually exclusive>-------------------------------------

    /* region_net | region_class | region_class_class are all mutually
       exclusive and are put into the kids container.
    */

    RULE*           m_rules;
};


class GRID : public ELEM
{
public:
    GRID( ELEM* aParent ) :
        ELEM( T_grid, aParent )
    {
        m_grid_type  = T_via;
        m_direction  = T_NONE;
        m_dimension  = 0.0;
        m_offset     = 0.0;
        m_image_type = T_NONE;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s %s %.6g", Name(), GetTokenText( m_grid_type ), m_dimension );

        if( m_grid_type == T_place )
        {
            if( m_image_type == T_smd || m_image_type == T_pin )
                out->Print( 0, " (image_type %s)", GetTokenText( m_image_type ) );
        }
        else
        {
            if( m_direction == T_x || m_direction == T_y )
                out->Print( 0, " (direction %s)", GetTokenText( m_direction ) );
        }

        if( m_offset != 0.0 )
            out->Print( 0, " (offset %.6g)", m_offset );

        out->Print( 0, ")\n");
    }

private:
    friend class SPECCTRA_DB;

    DSN_T       m_grid_type;      ///< T_via | T_wire | T_via_keepout | T_place | T_snap
    double      m_dimension;
    DSN_T       m_direction;      ///< T_x | T_y | -1 for both
    double      m_offset;
    DSN_T       m_image_type;
};


class STRUCTURE_OUT : public ELEM
{
public:
    STRUCTURE_OUT( ELEM* aParent ) :
        ELEM( T_structure_out, aParent )
    {
        m_rules = nullptr;
    }

    ~STRUCTURE_OUT()
    {
        delete m_rules;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( LAYERS::iterator i = m_layers.begin(); i != m_layers.end(); ++i )
            i->Format( out, nestLevel );

        if( m_rules )
            m_rules->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    LAYERS      m_layers;
    RULE*       m_rules;
};


class STRUCTURE : public ELEM_HOLDER
{
public:
    STRUCTURE( ELEM* aParent ) :
        ELEM_HOLDER( T_structure, aParent )
    {
        m_unit = nullptr;
        m_layer_noise_weight = nullptr;
        m_boundary = nullptr;
        m_place_boundary = nullptr;
        m_via = nullptr;
        m_control = nullptr;
        m_rules = nullptr;
        m_place_rules = nullptr;
    }

    ~STRUCTURE()
    {
        delete m_unit;
        delete m_layer_noise_weight;
        delete m_boundary;
        delete m_place_boundary;
        delete m_via;
        delete m_control;
        delete m_rules;
        delete m_place_rules;
    }

    void SetBOUNDARY( BOUNDARY *aBoundary )
    {
        delete m_boundary;
        m_boundary = aBoundary;

        if( m_boundary )
            m_boundary->SetParent( this );
    }

    void SetPlaceBOUNDARY( BOUNDARY *aBoundary )
    {
        delete m_place_boundary;
        m_place_boundary = aBoundary;

        if( m_place_boundary )
            m_place_boundary->SetParent( this );
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_unit )
            m_unit->Format( out, nestLevel );

        for( LAYERS::iterator i=m_layers.begin();  i!=m_layers.end();  ++i )
            i->Format( out, nestLevel );

        if( m_layer_noise_weight )
            m_layer_noise_weight->Format( out, nestLevel );

        if( m_boundary )
            m_boundary->Format( out, nestLevel );

        if( m_place_boundary )
            m_place_boundary->Format( out, nestLevel );

        for( COPPER_PLANES::iterator i=m_planes.begin();  i!=m_planes.end();  ++i )
            i->Format( out, nestLevel );

        for( REGIONS::iterator i=m_regions.begin();  i!=m_regions.end();  ++i )
            i->Format( out, nestLevel );

        for( KEEPOUTS::iterator i=m_keepouts.begin();  i!=m_keepouts.end();  ++i )
            i->Format( out, nestLevel );

        if( m_via )
            m_via->Format( out, nestLevel );

        if( m_control )
            m_control->Format( out, nestLevel );

        for( int i=0; i<Length();  ++i )
        {
            At(i)->Format( out, nestLevel );
        }

        if( m_rules )
            m_rules->Format( out, nestLevel );

        if( m_place_rules )
            m_place_rules->Format( out, nestLevel );

        for( GRIDS::iterator i=m_grids.begin();  i!=m_grids.end();  ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    UNIT_RES*           m_unit;

    LAYERS              m_layers;

    LAYER_NOISE_WEIGHT* m_layer_noise_weight;

    BOUNDARY*           m_boundary;
    BOUNDARY*           m_place_boundary;
    VIA*                m_via;
    CONTROL*            m_control;
    RULE*               m_rules;

    KEEPOUTS            m_keepouts;

    COPPER_PLANES       m_planes;

    typedef boost::ptr_vector<REGION>   REGIONS;
    REGIONS             m_regions;

    RULE*                m_place_rules;

    typedef boost::ptr_vector<GRID>     GRIDS;
    GRIDS                m_grids;
};


/**
 * Implement a &lt;placement_reference&gt; in the specctra dsn spec.
 */
class PLACE : public ELEM
{
public:
    PLACE( ELEM* aParent ) :
        ELEM( T_place, aParent )
    {
        m_side = T_front;

        m_rotation = 0.0;

        m_hasVertex = false;

        m_mirror = T_NONE;
        m_status = T_NONE;

        m_place_rules = nullptr;

        m_lock_type = T_NONE;
        m_rules = nullptr;
        m_region = nullptr;
    }

    ~PLACE()
    {
        delete m_place_rules;
        delete m_rules;
        delete m_region;
    }

    void SetVertex( const POINT& aVertex )
    {
        m_vertex = aVertex;
        m_vertex.FixNegativeZero();
        m_hasVertex = true;
    }

    void SetRotation( double aRotation )
    {
        m_rotation = aRotation;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override;

private:
    friend class SPECCTRA_DB;

    std::string     m_component_id;       ///< reference designator

    DSN_T           m_side;

    double          m_rotation;

    bool            m_hasVertex;
    POINT           m_vertex;

    DSN_T           m_mirror;
    DSN_T           m_status;

    std::string     m_logical_part;

    RULE*           m_place_rules;

    PROPERTIES      m_properties;

    DSN_T           m_lock_type;

    //-----<mutually exclusive>--------------
    RULE*           m_rules;
    REGION*         m_region;
    //-----</mutually exclusive>-------------

    std::string     m_part_number;
};

typedef boost::ptr_vector<PLACE>    PLACES;


/**
 * Implement a &lt;component_descriptor&gt; in the specctra dsn spec.
 */
class COMPONENT : public ELEM
{
public:
    COMPONENT( ELEM* aParent ) :
        ELEM( T_component, aParent )
    {
    }

    const std::string& GetImageId() const  { return m_image_id; }
    void SetImageId( const std::string& aImageId )
    {
        m_image_id = aImageId;
    }


    /**
     * Compare two objects of this type and returns <0, 0, or >0.
     */
//    static int Compare( IMAGE* lhs, IMAGE* rhs );

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_image_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, m_image_id.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( PLACES::iterator i=m_places.begin();  i!=m_places.end();  ++i )
            i->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

//    std::string     m_hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string     m_image_id;
    PLACES          m_places;
};

typedef boost::ptr_vector<COMPONENT> COMPONENTS;


class PLACEMENT : public ELEM
{
public:
    PLACEMENT( ELEM* aParent ) :
        ELEM( T_placement, aParent )
    {
        m_unit = nullptr;
        m_flip_style = DSN_T( T_NONE );
    }

    ~PLACEMENT()
    {
        delete m_unit;
    }

    /**
     * Look up a COMPONENT by name.
     *
     * If the name is not found, a new COMPONENT is added to the components container.  At any
     * time the names in the component container should remain unique.
     *
     * @return existing or new COMPONENT.
     */
    COMPONENT* LookupCOMPONENT( const std::string& imageName )
    {
        for( unsigned i = 0; i < m_components.size(); ++i )
        {
            if( 0 == m_components[i].GetImageId().compare( imageName ) )
                return &m_components[i];
        }

        COMPONENT* added = new COMPONENT(this);
        m_components.push_back( added );
        added->SetImageId( imageName );
        return added;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_unit )
            m_unit->Format( out, nestLevel );

        if( m_flip_style != DSN_T( T_NONE ) )
        {
            out->Print( nestLevel, "(place_control (flip_style %s))\n",
                        GetTokenText( m_flip_style ) );
        }

        for( COMPONENTS::iterator i = m_components.begin(); i != m_components.end(); ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    UNIT_RES*   m_unit;

    DSN_T       m_flip_style;

    COMPONENTS  m_components;
};


/**
 * A "(shape ..)" element in the specctra dsn spec.
 *
 * It is not a &lt;shape_descriptor&gt;, which is one of things that this
 * elements contains, i.e. in its "shape" field.  This class also implements
 * the "(outline ...)" element as a dual personality.
 */
class SHAPE : public WINDOW
{
public:
    /**
     * Takes a DSN_T aType of T_outline
     */
    SHAPE( ELEM* aParent, DSN_T aType = T_shape ) :
        WINDOW( aParent, aType )
    {
        m_connect = T_on;
    }

    void SetConnect( DSN_T aConnect )
    {
        m_connect = aConnect;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( shape )
            shape->Format( out, 0 );

        if( m_connect == T_off )
            out->Print( 0, "(connect %s)", GetTokenText( m_connect ) );

        if( m_windows.size() )
        {
            out->Print( 0, "\n" );

            for( WINDOWS::iterator i=m_windows.begin();  i!=m_windows.end();  ++i )
                i->Format( out, nestLevel+1 );

            out->Print( nestLevel, ")\n" );
        }
        else
        {
            out->Print( 0, ")\n" );
        }
    }

private:
    friend class SPECCTRA_DB;

    DSN_T           m_connect;

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    ELEM*           shape;      // inherited from WINDOW
    */

    WINDOWS         m_windows;
};


class PIN : public ELEM
{
public:
    PIN( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
        m_rotation = 0.0;
        m_isRotated = false;
        m_kiNetCode = 0;
    }

    void SetRotation( double aRotation )
    {
        m_rotation = aRotation;
        m_isRotated = (aRotation != 0.0);
    }

    void SetVertex( const POINT& aPoint )
    {
        m_vertex = aPoint;
        m_vertex.FixNegativeZero();
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_padstack_id.c_str() );
        if( m_isRotated )
            out->Print( nestLevel, "(pin %s%s%s (rotate %.6g)", quote, m_padstack_id.c_str(), quote,
                        m_rotation );
        else
            out->Print( nestLevel, "(pin %s%s%s", quote, m_padstack_id.c_str(), quote );

        quote = out->GetQuoteChar( m_pin_id.c_str() );
        out->Print( 0, " %s%s%s %.6g %.6g)\n", quote, m_pin_id.c_str(), quote, m_vertex.x, m_vertex.y );
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_padstack_id;
    double          m_rotation;
    bool            m_isRotated;
    std::string     m_pin_id;
    POINT           m_vertex;

    int             m_kiNetCode;      ///< KiCad netcode
};

typedef boost::ptr_vector<PIN>  PINS;


class LIBRARY;

class IMAGE : public ELEM_HOLDER
{
public:
    IMAGE( ELEM* aParent ) :
        ELEM_HOLDER( T_image, aParent )
    {
        m_side = T_both;
        m_unit = nullptr;
        m_rules = nullptr;
        m_place_rules = nullptr;
        m_duplicated = 0;
    }

    ~IMAGE()
    {
        delete m_unit;
        delete m_rules;
        delete m_place_rules;
    }

    /**
     * Compare two objects of this type and returns <0, 0, or >0.
     */
    static int Compare( IMAGE* lhs, IMAGE* rhs );

    std::string GetImageId()
    {
        if( m_duplicated )
        {
            return m_image_id + "::" + std::to_string( m_duplicated );
        }

        return m_image_id;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        std::string imageId = GetImageId();

        const char* quote = out->GetQuoteChar( imageId.c_str() );

        out->Print( nestLevel, "(%s %s%s%s", Name(), quote, imageId.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    // this is here for makeHash()
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_side != T_both )
            out->Print( 0, " (side %s)", GetTokenText( m_side ) );

        out->Print( 0, "\n");

        if( m_unit )
            m_unit->Format( out, nestLevel );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel );

        for( PINS::iterator i=m_pins.begin();  i!=m_pins.end();  ++i )
            i->Format( out, nestLevel );

        if( m_rules )
            m_rules->Format( out, nestLevel );

        if( m_place_rules )
            m_place_rules->Format( out, nestLevel );

        for( KEEPOUTS::iterator i=m_keepouts.begin();  i!=m_keepouts.end();  ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;
    friend class LIBRARY;

    std::string     m_hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string     m_image_id;
    DSN_T           m_side;
    UNIT_RES*       m_unit;

    /*  The grammar spec says only one outline is supported, but I am seeing
        *.dsn examples with multiple outlines.  So the outlines will go into
        the kids list.
    */

    PINS            m_pins;

    RULE*           m_rules;
    RULE*           m_place_rules;

    KEEPOUTS        m_keepouts;

    int             m_duplicated;     ///< no. times this image_id is duplicated
};

typedef boost::ptr_vector<IMAGE>    IMAGES;


/**
 * Hold either a via or a pad definition.
 */
class PADSTACK : public ELEM_HOLDER
{
public:
    /**
     * Cannot take ELEM* aParent because PADSTACKSET confuses this with a
     * copy constructor and causes havoc.  Instead set parent with
     * LIBRARY::AddPadstack()
     */
    PADSTACK() :
        ELEM_HOLDER( T_padstack, nullptr )
    {
        m_unit = nullptr;
        m_rotate = T_on;
        m_absolute = T_off;
        m_rules = nullptr;
        m_attach = T_off;
    }

    ~PADSTACK()
    {
        delete m_unit;
        delete m_rules;
    }

    const std::string& GetPadstackId()
    {
        return m_padstack_id;
    }

    /**
     * Compare two objects of this type and returns <0, 0, or >0.
     */
    static int Compare( PADSTACK* lhs, PADSTACK* rhs );

    void SetPadstackId( const char* aPadstackId )
    {
        m_padstack_id = aPadstackId;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_padstack_id.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, m_padstack_id.c_str(), quote );

        FormatContents( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    // this factored out for use by Compare()
    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_unit )
            m_unit->Format( out, nestLevel );

        // format the kids, which in this class are the shapes
        ELEM_HOLDER::FormatContents( out, nestLevel );

        out->Print( nestLevel, "%s", "" );

        // spec for <attach_descriptor> says default is on, so
        // print the off condition to override this.
        if( m_attach == T_off )
        {
            out->Print( 0, "(attach off)" );
        }
        else if( m_attach == T_on )
        {
            const char* quote = out->GetQuoteChar( m_via_id.c_str() );

            out->Print( 0, "(attach on (use_via %s%s%s))", quote, m_via_id.c_str(), quote );
        }

        if( m_rotate == T_off )   // print the non-default
            out->Print( 0, "(rotate %s)", GetTokenText( m_rotate ) );

        if( m_absolute == T_on )  // print the non-default
            out->Print( 0, "(absolute %s)", GetTokenText( m_absolute ) );

        out->Print( 0, "\n" );

        if( m_rules )
            m_rules->Format( out, nestLevel );
    }


    UNIT_RES* GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    std::string  m_hash;       ///< a hash string used by Compare(), not Format()ed/exported.

    std::string  m_padstack_id;
    UNIT_RES*    m_unit;

    /* The shapes are stored in the kids list */

    DSN_T        m_rotate;
    DSN_T        m_absolute;
    DSN_T        m_attach;
    std::string  m_via_id;

    RULE*        m_rules;
};

typedef boost::ptr_vector<PADSTACK> PADSTACKS;


/**
 * Used by the PADSTACKSET boost::ptr_set below.
 */
inline bool operator<( const PADSTACK& lhs, const PADSTACK& rhs )
{
    return PADSTACK::Compare( (PADSTACK*) &lhs, (PADSTACK*) &rhs ) < 0;
}


/**
 * A &lt;library_descriptor&gt; in the specctra dsn specification.
 *
 * Only unit_descriptor, image_descriptors, and padstack_descriptors are
 * included as children at this time.
 */
class LIBRARY : public ELEM
{
public:
    LIBRARY( ELEM* aParent, DSN_T aType = T_library ) :
        ELEM( aType, aParent )
    {
        m_unit = nullptr;
//        via_start_index = -1;       // 0 or greater means there is at least one via
    }

    ~LIBRARY()
    {
        delete m_unit;
    }

    void AddPadstack( PADSTACK* aPadstack )
    {
        aPadstack->SetParent( this );
        m_padstacks.push_back( aPadstack );
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
     * Search this LIBRARY for an image which matches the argument.
     *
     * @return index of image if found, else -1.
     */
    int FindIMAGE( IMAGE* aImage )
    {
        unsigned i;

        for( i=0;  i<m_images.size();  ++i )
        {
            if( 0 == IMAGE::Compare( aImage, &m_images[i] ) )
                return (int) i;
        }

        // There is no match to the IMAGE contents, but now generate a unique
        // name for it.
        int dups = 1;

        for( i=0;  i<m_images.size();  ++i )
        {
            if( 0 == aImage->m_image_id.compare( m_images[i].m_image_id ) )
                aImage->m_duplicated = dups++;
        }

        return -1;
    }


    /**
     * Add the image to the image list.
     */
    void AppendIMAGE( IMAGE* aImage )
    {
        aImage->SetParent( this );
        m_images.push_back( aImage );
    }

    /**
     * Add the image only if one exactly like it does not already exist in the image container.
     *
     * @return the IMAGE which is registered in the LIBRARY that matches the argument, and it
     *         will be either the argument or a previous image which is a duplicate.
     */
    IMAGE* LookupIMAGE( IMAGE* aImage )
    {
        int ndx = FindIMAGE( aImage );

        if( ndx == -1 )
        {
            AppendIMAGE( aImage );
            return aImage;
        }

        return &m_images[ndx];
    }

    /**
     * Search this LIBRARY for a via which matches the argument.
     *
     * @return the index found in the padstack list, else -1.
     */
    int FindVia( PADSTACK* aVia )
    {
        for( unsigned i = 0; i < m_vias.size(); ++i )
        {
            if( 0 == PADSTACK::Compare( aVia, &m_vias[i] ) )
                return int( i );
        }

        return -1;
    }

    /**
     * Add \a aVia to the internal via container.
     */
    void AppendVia( PADSTACK* aVia )
    {
        aVia->SetParent( this );
        m_vias.push_back( aVia );
    }


    /**
     * Add the padstack to the padstack container.
     */
    void AppendPADSTACK( PADSTACK* aPadstack )
    {
        aPadstack->SetParent( this );
        m_padstacks.push_back( aPadstack );
    }

    /**
     * Add the via only if one exactly like it does not already exist in the padstack container.
     *
     * @return  the PADSTACK which is registered in the LIBRARY that matches the argument, and
     *          it will be either the argument or a previous padstack which is a duplicate.
     */
    PADSTACK* LookupVia( PADSTACK* aVia )
    {
        int ndx = FindVia( aVia );

        if( ndx == -1 )
        {
            AppendVia( aVia );
            return aVia;
        }

        return &m_vias[ndx];
    }

    /**
     * Search the padstack container by name.
     *
     * @return The PADSTACK with a matching name if it exists, else nullptr.
     */
    PADSTACK* FindPADSTACK( const std::string& aPadstackId )
    {
        for( unsigned i = 0; i < m_padstacks.size(); ++i )
        {
            PADSTACK* ps = &m_padstacks[i];

            if( ps->GetPadstackId().compare( aPadstackId ) == 0 )
                return ps;
        }

        return nullptr;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( m_unit )
            m_unit->Format( out, nestLevel );

        for( IMAGES::iterator i = m_images.begin(); i != m_images.end(); ++i )
            i->Format( out, nestLevel );

        for( PADSTACKS::iterator i = m_padstacks.begin(); i != m_padstacks.end(); ++i )
            i->Format( out, nestLevel );

        for( PADSTACKS::iterator i = m_vias.begin(); i != m_vias.end(); ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES* GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    UNIT_RES*       m_unit;
    IMAGES          m_images;

    PADSTACKS       m_padstacks;      ///< all except vias, which are in 'vias'
    PADSTACKS       m_vias;
};


/**
 * A &lt;pin_reference&gt; definition in the specctra dsn spec.
 */
struct PIN_REF : public ELEM
{
    PIN_REF( ELEM* aParent ) :
        ELEM( T_pin, aParent )
    {
    }


    /**
     * Like Format() but is not virtual.
     *
     * @return the number of characters that were output.
     */
    int FormatIt( OUTPUTFORMATTER* out, int nestLevel )
    {
        // only print the newline if there is a nest level, and make
        // the quotes unconditional on this one.
        const char* newline = nestLevel ? "\n" : "";

        const char* cquote = out->GetQuoteChar( component_id.c_str() );
        const char* pquote = out->GetQuoteChar( pin_id.c_str() );

        return out->Print( nestLevel, "%s%s%s-%s%s%s%s", cquote, component_id.c_str(), cquote,
                                      pquote, pin_id.c_str(), pquote, newline );
    }

    std::string     component_id;
    std::string     pin_id;
};

typedef std::vector<PIN_REF>   PIN_REFS;


class FROMTO : public ELEM
{
public:
    FROMTO( ELEM* aParent ) :
        ELEM( T_fromto, aParent )
    {
        m_rules = nullptr;
        m_fromto_type  = DSN_T( T_NONE );
    }
    ~FROMTO()
    {
        delete m_rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        // no quoting on these two, the lexer preserved the quotes on input
        out->Print( nestLevel, "(%s %s %s ",
                 Name(), m_fromText.c_str(), m_toText.c_str() );

        if( m_fromto_type != DSN_T( T_NONE ) )
            out->Print( 0, "(type %s)", GetTokenText( m_fromto_type ) );

        if( m_net_id.size() )
        {
            const char* quote = out->GetQuoteChar( m_net_id.c_str() );
            out->Print( 0, "(net %s%s%s)",  quote, m_net_id.c_str(), quote );
        }

        bool singleLine = true;

        if( m_rules || m_layer_rules.size() )
        {
            out->Print( 0, "\n" );
            singleLine = false;
        }

        if( m_rules )
            m_rules->Format( out, nestLevel+1 );

        /*
        if( circuit.size() )
            out->Print( nestLevel, "%s\n", circuit.c_str() );
        */

        for( LAYER_RULES::iterator i = m_layer_rules.begin(); i != m_layer_rules.end(); ++i )
            i->Format( out, nestLevel+1 );

        out->Print( singleLine ? 0 : nestLevel, ")" );

        if( nestLevel || !singleLine )
            out->Print( 0, "\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_fromText;
    std::string     m_toText;

    DSN_T           m_fromto_type;
    std::string     m_net_id;
    RULE*           m_rules;
//    std::string     m_circuit;
    LAYER_RULES     m_layer_rules;
};

typedef boost::ptr_vector<FROMTO>       FROMTOS;


/**
 * The &lt;component_order_descriptor&gt;
 */
class COMP_ORDER : public ELEM
{
public:
    COMP_ORDER( ELEM* aParent ) :
        ELEM( T_comp_order, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s", Name() );

        for( STRINGS::iterator i = m_placement_ids.begin(); i != m_placement_ids.end(); ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( 0, " %s%s%s", quote, i->c_str(), quote );
        }

        out->Print( 0, ")" );

        if( nestLevel )
            out->Print( 0, "\n" );
    }

private:
    friend class SPECCTRA_DB;

    STRINGS         m_placement_ids;
};

typedef boost::ptr_vector<COMP_ORDER>   COMP_ORDERS;

/**
 * A &lt;net_descriptor&gt; in the DSN spec.
 */
class NET : public ELEM
{
public:
    NET( ELEM* aParent ) :
        ELEM( T_net, aParent )
    {
        m_unassigned = false;
        m_net_number = T_NONE;
        m_pins_type = T_pins;

        m_type = T_NONE;
        m_supply = T_NONE;

        m_rules = nullptr;
        m_comp_order = nullptr;
    }

    ~NET()
    {
        delete m_rules;
        delete m_comp_order;
    }

    int FindPIN_REF( const std::string& aComponent )
    {
        for( unsigned i = 0; i < m_pins.size(); ++i )
        {
            if( aComponent.compare( m_pins[i].component_id ) == 0 )
                return int(i);
        }

        return -1;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_net_id.c_str() );
        const char* space = " ";

        out->Print( nestLevel, "(%s %s%s%s", Name(), quote, m_net_id.c_str(), quote );

        if( m_unassigned )
        {
            out->Print( 0, "%s(unassigned)", space );
            space = "";         // only needed one space
        }

        if( m_net_number != T_NONE )
        {
            out->Print( 0, "%s(net_number %d)", space, m_net_number );
            // space = "";
        }

        out->Print( 0, "\n" );

        if( m_pins.size() )
        {
            const int RIGHTMARGIN = 80;
            int perLine = out->Print( nestLevel+1, "(%s", GetTokenText( m_pins_type ) );

            for( PIN_REFS::iterator i = m_pins.begin(); i != m_pins.end(); ++i )
            {
                if( perLine > RIGHTMARGIN )
                {
                    out->Print( 0, "\n");
                    perLine = out->Print( nestLevel+2, "%s", "" );
                }
                else
                {
                    perLine += out->Print( 0, " " );
                }

                perLine += i->FormatIt( out, 0 );
            }

            out->Print( 0, ")\n" );
        }

        if( m_comp_order )
            m_comp_order->Format( out, nestLevel+1 );

        if( m_type != T_NONE )
            out->Print( nestLevel+1, "(type %s)\n", GetTokenText( m_type ) );

        if( m_rules )
            m_rules->Format( out, nestLevel+1 );

        for( LAYER_RULES::iterator i = m_layer_rules.begin(); i != m_layer_rules.end(); ++i )
            i->Format( out, nestLevel+1 );

        for( FROMTOS::iterator i = m_fromtos.begin(); i != m_fromtos.end(); ++i )
            i->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_net_id;
    bool            m_unassigned;
    int             m_net_number;

    DSN_T           m_pins_type;      ///< T_pins | T_order, type of field 'pins' below
    PIN_REFS        m_pins;

    PIN_REFS        m_expose;
    PIN_REFS        m_noexpose;
    PIN_REFS        m_source;
    PIN_REFS        m_load;
    PIN_REFS        m_terminator;

    DSN_T           m_type;           ///< T_fix | T_normal

    DSN_T           m_supply;         ///< T_power | T_ground

    RULE*           m_rules;

    LAYER_RULES     m_layer_rules;

    FROMTOS         m_fromtos;

    COMP_ORDER*     m_comp_order;
};

typedef boost::ptr_vector<NET>  NETS;


class TOPOLOGY : public ELEM
{
public:
    TOPOLOGY( ELEM* aParent ) :
        ELEM( T_topology, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( FROMTOS::iterator i = m_fromtos.begin(); i != m_fromtos.end(); ++i )
            i->Format( out, nestLevel );

        for( COMP_ORDERS::iterator i = m_comp_orders.begin(); i != m_comp_orders.end(); ++i )
            i->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    FROMTOS         m_fromtos;

    COMP_ORDERS     m_comp_orders;
};


/**
 * The &lt;class_descriptor&gt; in the specctra spec.
 */
class CLASS : public ELEM
{
public:
    CLASS( ELEM* aParent ) :
        ELEM( T_class, aParent )
    {
        m_rules = nullptr;
        m_topology = nullptr;
    }

    ~CLASS()
    {
        delete m_rules;
        delete m_topology;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_class_id.c_str() );

        int perLine = out->Print( nestLevel, "(%s %s%s%s", Name(), quote, m_class_id.c_str(), quote );

        const int RIGHTMARGIN = 72;

        for( STRINGS::iterator i=m_net_ids.begin();  i!=m_net_ids.end();  ++i )
        {
            const char* space = " ";

            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
                space = ""; // no space at first net_id of the line
            }

            // Allegro PCB Router (Specctra) doesn't like empty net names
            if( i->empty() )
                continue;

            quote = out->GetQuoteChar( i->c_str() );
            perLine += out->Print( 0, "%s%s%s%s", space, quote, i->c_str(), quote );
        }

        bool newLine = false;

        if( m_circuit.size() || m_rules || m_layer_rules.size() || m_topology )
        {
            out->Print( 0, "\n" );
            newLine = true;
        }

        if( m_circuit.size() )
        {
            out->Print( nestLevel+1, "(circuit\n" );

            for( STRINGS::iterator i = m_circuit.begin(); i != m_circuit.end(); ++i )
                out->Print( nestLevel + 2, "%s\n", i->c_str() );

            out->Print( nestLevel+1, ")\n" );
        }

        if( m_rules )
            m_rules->Format( out, nestLevel+1 );

        for( LAYER_RULES::iterator i = m_layer_rules.begin(); i != m_layer_rules.end(); ++i )
            i->Format( out, nestLevel + 1 );

        if( m_topology )
            m_topology->Format( out, nestLevel+1 );

        out->Print( newLine ? nestLevel : 0, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_class_id;

    STRINGS         m_net_ids;

    /// circuit descriptor list
    STRINGS         m_circuit;

    RULE*           m_rules;

    LAYER_RULES     m_layer_rules;

    TOPOLOGY*       m_topology;
};

typedef boost::ptr_vector<CLASS> CLASSLIST;


class NETWORK : public ELEM
{
public:
    NETWORK( ELEM* aParent ) :
        ELEM( T_network, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( NETS::iterator i = m_nets.begin(); i != m_nets.end(); ++i )
            i->Format( out, nestLevel );

        for( CLASSLIST::iterator i = m_classes.begin(); i != m_classes.end(); ++i )
            i->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    NETS        m_nets;
    CLASSLIST   m_classes;
};


class CONNECT : public ELEM
{
    // @todo not completed.

public:
    CONNECT( ELEM* aParent ) :
        ELEM( T_connect, aParent ) {}
};


/**
 * A &lt;wire_shape_descriptor&gt; in the specctra dsn spec.
 */
class WIRE : public ELEM
{
public:
    WIRE( ELEM* aParent ) :
        ELEM( T_wire, aParent )
    {
        m_shape = nullptr;
        m_connect = nullptr;

        m_turret = -1;
        m_wire_type = T_NONE;
        m_attr = T_NONE;
        m_supply = false;
    }

    ~WIRE()
    {
        delete m_shape;
        delete m_connect;
    }

    void SetShape( ELEM* aShape )
    {
        delete m_shape;
        m_shape = aShape;

        if( aShape )
        {
            wxASSERT(aShape->Type()==T_rect || aShape->Type()==T_circle
                     || aShape->Type()==T_qarc || aShape->Type()==T_path
                     || aShape->Type()==T_polygon);

            aShape->SetParent( this );
        }
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        out->Print( nestLevel, "(%s ", Name() );

        if( m_shape )
            m_shape->Format( out, 0 );

        if( m_net_id.size() )
        {
            const char* quote = out->GetQuoteChar( m_net_id.c_str() );
            out->Print( 0, "(net %s%s%s)", quote, m_net_id.c_str(), quote );
        }

        if( m_turret >= 0 )
            out->Print( 0, "(turrent %d)", m_turret );

        if( m_wire_type != T_NONE )
            out->Print( 0, "(type %s)", GetTokenText( m_wire_type ) );

        if( m_attr != T_NONE )
            out->Print( 0, "(attr %s)", GetTokenText( m_attr ) );

        if( m_shield.size() )
        {
            const char* quote = out->GetQuoteChar( m_shield.c_str() );
            out->Print( 0, "(shield %s%s%s)", quote, m_shield.c_str(), quote );
        }

        if( m_windows.size() )
        {
            out->Print( 0, "\n" );

            for( WINDOWS::iterator i = m_windows.begin(); i != m_windows.end(); ++i )
                i->Format( out, nestLevel + 1 );
        }

        if( m_connect )
            m_connect->Format( out, 0 );

        if( m_supply )
            out->Print( 0, "(supply)" );

        out->Print( 0, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    /*  <shape_descriptor >::=
        [<rectangle_descriptor> |
        <circle_descriptor> |
        <polygon_descriptor> |
        <path_descriptor> |
        <qarc_descriptor> ]
    */
    ELEM*           m_shape;

    std::string     m_net_id;
    int             m_turret;
    DSN_T           m_wire_type;
    DSN_T           m_attr;
    std::string     m_shield;
    WINDOWS         m_windows;
    CONNECT*        m_connect;
    bool            m_supply;
};

typedef boost::ptr_vector<WIRE>     WIRES;


/**
 * A &lt;wire_via_descriptor&gt; in the specctra dsn spec.
 */
class WIRE_VIA : public ELEM
{
public:
    WIRE_VIA( ELEM* aParent ) :
        ELEM( T_via, aParent )
    {
        m_via_number = -1;
        m_via_type = T_NONE;
        m_attr = T_NONE;
        m_supply = false;
    }

    const std::string& GetPadstackId()
    {
        return m_padstack_id;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_padstack_id.c_str() );

        const int RIGHTMARGIN = 80;
        int perLine = out->Print( nestLevel, "(%s %s%s%s", Name(), quote, m_padstack_id.c_str(),
                                  quote );

        for( POINTS::iterator i = m_vertexes.begin(); i != m_vertexes.end(); ++i )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }
            else
            {
                perLine += out->Print( 0, "  " );
            }

            perLine += out->Print( 0, "%.6g %.6g", i->x, i->y );
        }

        if( m_net_id.size() || m_via_number!=-1 || m_via_type!=T_NONE || m_attr!=T_NONE || m_supply)
            out->Print( 0, " " );

        if( m_net_id.size() )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }

            quote = out->GetQuoteChar( m_net_id.c_str() );
            perLine += out->Print( 0, "(net %s%s%s)", quote, m_net_id.c_str(), quote );
        }

        if( m_via_number != -1 )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }

            perLine += out->Print( 0, "(via_number %d)", m_via_number );
        }

        if( m_via_type != T_NONE )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }

            perLine += out->Print( 0, "(type %s)", GetTokenText( m_via_type ) );
        }

        if( m_attr != T_NONE )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }

            if( m_attr == T_virtual_pin )
            {
                quote = out->GetQuoteChar( m_virtual_pin_name.c_str() );
                perLine += out->Print( 0, "(attr virtual_pin %s%s%s)", quote,
                                       m_virtual_pin_name.c_str(), quote );
            }
            else
            {
                perLine += out->Print( 0, "(attr %s)", GetTokenText( m_attr ) );
            }
        }

        if( m_supply )
        {
            if( perLine > RIGHTMARGIN )
            {
                out->Print( 0, "\n" );
                perLine = out->Print( nestLevel+1, "%s", "" );
            }

            perLine += out->Print( 0, "(supply)" );
        }

        if( m_contact_layers.size() )
        {
            out->Print( 0, "\n" );
            out->Print( nestLevel+1, "(contact\n" );

            for( STRINGS::iterator i = m_contact_layers.begin(); i != m_contact_layers.end(); ++i )
            {
                quote = out->GetQuoteChar( i->c_str() );
                out->Print( nestLevel+2, "%s%s%s\n", quote, i->c_str(), quote );
            }

            out->Print( nestLevel+1, "))\n" );
        }
        else
        {
            out->Print( 0, ")\n" );
        }
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_padstack_id;
    POINTS          m_vertexes;
    std::string     m_net_id;
    int             m_via_number;
    DSN_T           m_via_type;
    DSN_T           m_attr;
    std::string     m_virtual_pin_name;
    STRINGS         m_contact_layers;
    bool            m_supply;
};

typedef boost::ptr_vector<WIRE_VIA>      WIRE_VIAS;


/**
 * A &lt;wiring_descriptor&gt; in the specctra dsn spec.
 */
class WIRING : public ELEM
{
public:
    WIRING( ELEM* aParent ) :
        ELEM( T_wiring, aParent )
    {
        unit = nullptr;
    }

    ~WIRING()
    {
        delete unit;
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        if( unit )
            unit->Format( out, nestLevel );

        for( WIRES::iterator i = wires.begin(); i != wires.end(); ++i )
            i->Format( out, nestLevel );

        for( WIRE_VIAS::iterator i = wire_vias.begin(); i != wire_vias.end(); ++i )
            i->Format( out, nestLevel );
    }

    UNIT_RES*  GetUnits() const override
    {
        if( unit )
            return unit;

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    UNIT_RES*   unit;
    WIRES       wires;
    WIRE_VIAS   wire_vias;
};


class PCB : public ELEM
{
public:
    PCB( ELEM* aParent = nullptr ) :
        ELEM( T_pcb, aParent )
    {
        m_parser = nullptr;
        m_resolution = nullptr;
        m_unit = nullptr;
        m_structure = nullptr;
        m_placement = nullptr;
        m_library = nullptr;
        m_network = nullptr;
        m_wiring = nullptr;
    }

    ~PCB()
    {
        delete m_parser;
        delete m_resolution;
        delete m_unit;
        delete m_structure;
        delete m_placement;
        delete m_library;
        delete m_network;
        delete m_wiring;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( m_pcbname.c_str() );

        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, m_pcbname.c_str(), quote );

        if( m_parser )
            m_parser->Format( out, nestLevel+1 );

        if( m_resolution )
            m_resolution->Format( out, nestLevel+1 );

        if( m_unit )
            m_unit->Format( out, nestLevel+1 );

        if( m_structure )
            m_structure->Format( out, nestLevel+1 );

        if( m_placement )
            m_placement->Format( out, nestLevel+1 );

        if( m_library )
            m_library->Format( out, nestLevel+1 );

        if( m_network )
            m_network->Format( out, nestLevel+1 );

        if( m_wiring )
            m_wiring->Format( out, nestLevel+1 );

        out->Print( nestLevel, ")\n" );
    }

    UNIT_RES*  GetUnits() const override
    {
        if( m_unit )
            return m_unit;

        if( m_resolution )
            return m_resolution->GetUnits();

        return ELEM::GetUnits();
    }

private:
    friend class SPECCTRA_DB;

    std::string     m_pcbname;
    PARSER*         m_parser;
    UNIT_RES*       m_resolution;
    UNIT_RES*       m_unit;
    STRUCTURE*      m_structure;
    PLACEMENT*      m_placement;
    LIBRARY*        m_library;
    NETWORK*        m_network;
    WIRING*         m_wiring;
};


class ANCESTOR : public ELEM
{
public:
    ANCESTOR( ELEM* aParent ) :
        ELEM( T_ancestor, aParent )
    {
        time_stamp = time(nullptr);
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        char    temp[80];
        struct  tm* tmp;

        tmp = localtime( &time_stamp );
        strftime( temp, sizeof(temp), "%b %d %H : %M : %S %Y", tmp );

        // format the time first to temp
        // filename may be empty, so quote it just in case.
        out->Print( nestLevel, "(%s \"%s\" (created_time %s)\n", Name(), filename.c_str(), temp );

        if( comment.size() )
        {
            const char* quote = out->GetQuoteChar( comment.c_str() );
            out->Print( nestLevel+1, "(comment %s%s%s)\n", quote, comment.c_str(), quote );
        }

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string     filename;
    std::string     comment;
    time_t          time_stamp;
};

typedef boost::ptr_vector<ANCESTOR>     ANCESTORS;


class HISTORY : public ELEM
{
public:
    HISTORY( ELEM* aParent ) :
        ELEM( T_history, aParent )
    {
        time_stamp = time(nullptr);
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( ANCESTORS::iterator i=ancestors.begin();  i!=ancestors.end();  ++i )
            i->Format( out, nestLevel );

        char    temp[80];
        struct  tm* tmp;

        tmp = localtime( &time_stamp );
        strftime( temp, sizeof( temp ), "%b %d %H : %M : %S %Y", tmp );

        // format the time first to temp
        out->Print( nestLevel, "(self (created_time %s)\n", temp );

        for( STRINGS::iterator i=comments.begin();  i!=comments.end();  ++i )
        {
            const char* quote = out->GetQuoteChar( i->c_str() );
            out->Print( nestLevel+1, "(comment %s%s%s)\n", quote, i->c_str(), quote );
        }

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    ANCESTORS       ancestors;
    time_t          time_stamp;
    STRINGS         comments;
};


/**
 * A &lt;supply_pin_descriptor&gt; in the specctra dsn spec.
 */
class SUPPLY_PIN : public ELEM
{
public:
    SUPPLY_PIN( ELEM* aParent ) :
        ELEM( T_supply_pin, aParent )
    {
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
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
            for( PIN_REFS::iterator i = pin_refs.begin(); i != pin_refs.end(); ++i )
                i->FormatIt( out, nestLevel + 1 );
        }

        if( net_id.size() )
        {
            const char* newline = singleLine ? "" : "\n";

            const char* quote = out->GetQuoteChar( net_id.c_str() );
            out->Print( singleLine ? 0 : nestLevel+1, " (net %s%s%s)%s",
                        quote, net_id.c_str(), quote, newline );
        }

        out->Print( singleLine ? 0 : nestLevel, ")\n");
    }

private:
    friend class SPECCTRA_DB;

    PIN_REFS        pin_refs;
    std::string     net_id;
};

typedef boost::ptr_vector<SUPPLY_PIN>   SUPPLY_PINS;


/**
 * A &lt;net_out_descriptor&gt; of the specctra dsn spec.
 */
class NET_OUT : public ELEM
{
public:
    NET_OUT( ELEM* aParent ) :
        ELEM( T_net_out, aParent )
    {
        rules = nullptr;
        net_number = -1;
    }

    ~NET_OUT()
    {
        delete rules;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( net_id.c_str() );

        // cannot use Type() here, it is T_net_out and we need "(net "
        out->Print( nestLevel, "(net %s%s%s\n", quote, net_id.c_str(), quote );

        if( net_number>= 0 )
            out->Print( nestLevel+1, "(net_number %d)\n", net_number );

        if( rules )
            rules->Format( out, nestLevel+1 );

        for( WIRES::iterator i = wires.begin(); i != wires.end(); ++i )
            i->Format( out, nestLevel + 1 );

        for( WIRE_VIAS::iterator i = wire_vias.begin(); i != wire_vias.end(); ++i )
            i->Format( out, nestLevel + 1 );

        for( SUPPLY_PINS::iterator i = supply_pins.begin(); i != supply_pins.end(); ++i )
            i->Format( out, nestLevel + 1 );

        out->Print( nestLevel, ")\n" );
    }

private:
    friend class SPECCTRA_DB;

    std::string     net_id;
    int             net_number;
    RULE*           rules;
    WIRES           wires;
    WIRE_VIAS       wire_vias;
    SUPPLY_PINS     supply_pins;
};

typedef boost::ptr_vector<NET_OUT>      NET_OUTS;


class ROUTE : public ELEM
{
public:
    ROUTE( ELEM* aParent ) :
        ELEM( T_route, aParent )
    {
        resolution = nullptr;
        parser = nullptr;
        structure_out = nullptr;
        library = nullptr;
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

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
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

            for( NET_OUTS::iterator i = net_outs.begin(); i != net_outs.end(); ++i )
                i->Format( out, nestLevel + 1 );

            out->Print( nestLevel, ")\n" );
        }

//        if( test_poinst )
//            test_points->Format( out, nestLevel );
    }

private:
    friend class SPECCTRA_DB;

    UNIT_RES*       resolution;
    PARSER*         parser;
    STRUCTURE_OUT*  structure_out;
    LIBRARY*        library;
    NET_OUTS        net_outs;
//    TEST_POINTS*    test_points;
};


/**
 * Used within the WAS_IS class below to hold a pair of PIN_REFs and corresponds to the (pins
 * was is) construct within the specctra dsn spec.
 */
struct PIN_PAIR
{
    PIN_PAIR( ELEM* aParent = nullptr ) :
        was( aParent ),
        is( aParent )
    {
    }

    PIN_REF     was;
    PIN_REF     is;
};

typedef std::vector<PIN_PAIR>   PIN_PAIRS;


/**
 * A &lt;was_is_descriptor&gt; in the specctra dsn spec.
 */
class WAS_IS : public ELEM
{
public:
    WAS_IS( ELEM* aParent ) :
        ELEM( T_was_is, aParent )
    {
    }

    void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        for( PIN_PAIRS::iterator i = pin_pairs.begin(); i != pin_pairs.end(); ++i )
        {
            out->Print( nestLevel, "(pins " );
            i->was.Format( out, 0 );
            out->Print( 0, " " );
            i->is.Format( out, 0 );
            out->Print( 0, ")\n" );
        }
    }

private:
    friend class SPECCTRA_DB;

    PIN_PAIRS       pin_pairs;
};


/**
 * A &lt;session_file_descriptor&gt; in the specctra dsn spec.
 */
class SESSION : public ELEM
{
public:
    SESSION( ELEM* aParent = nullptr ) :
        ELEM( T_session, aParent )
    {
        history = nullptr;
        structure = nullptr;
        placement = nullptr;
        was_is = nullptr;
        route = nullptr;
    }

    ~SESSION()
    {
        delete history;
        delete structure;
        delete placement;
        delete was_is;
        delete route;
    }

    void Format( OUTPUTFORMATTER* out, int nestLevel ) override
    {
        const char* quote = out->GetQuoteChar( session_id.c_str() );
        out->Print( nestLevel, "(%s %s%s%s\n", Name(), quote, session_id.c_str(), quote );

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

private:
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
};

typedef boost::ptr_set<PADSTACK>    PADSTACKSET;


/**
 * A DSN data tree, usually coming from a DSN file. Is essentially a SPECCTRA_PARSER class.
 */
class SPECCTRA_DB : public SPECCTRA_LEXER
{
public:

    SPECCTRA_DB() :
        SPECCTRA_LEXER( 0 )         // LINE_READER* == nullptr, no DSNLEXER::PushReader()
    {
        // The LINE_READER will be pushed from an automatic instantiation,
        // we don't own it:
        wxASSERT( !iOwnReaders );

        m_pcb   = nullptr;
        m_session = nullptr;
        m_quote_char += '"';
        m_footprintsAreFlipped = false;

        SetSpecctraMode( true );

        // Avoid not initialized members:
        m_routeResolution = nullptr;
        m_sessionBoard = nullptr;
        m_top_via_layer = 0;
        m_bot_via_layer = 0;
    }

    virtual ~SPECCTRA_DB()
    {
        delete m_pcb;
        delete m_session;

        deleteNETs();
    }

    /**
     * Make a PCB with all the default ELEMs and parts on the heap.
     */
    static PCB* MakePCB();

    /**
     * Delete any existing PCB and replaces it with the given one.
     */
    void SetPCB( PCB* aPcb )
    {
        delete m_pcb;
        m_pcb = aPcb;
    }

    PCB* GetPCB()  { return m_pcb; }

    /**
     * Delete any existing SESSION and replaces it with the given one.
     */
    void SetSESSION( SESSION* aSession )
    {
        delete m_session;
        m_session = aSession;
    }

    SESSION* GetSESSION() { return m_session; }

    /**
     * A recursive descent parser for a SPECCTRA DSN "design" file.
     *
     * A design file is nearly a full description of a PCB (seems to be
     * missing only the silkscreen stuff).
     *
     * @param aFilename The name of the dsn file to load.
     * @throw IO_ERROR if there is a lexer or parser error.
     */
    void LoadPCB( const wxString& aFilename );

    /**
     * A recursive descent parser for a SPECCTRA DSN "session" file.
     *
     * A session file is a file that is fed back from the router to the layout
     * tool (Pcbnew) and should be used to update a BOARD object with the new
     * tracks, vias, and component locations.
     *
     * @param aFilename The name of the dsn file to load.
     * @throw IO_ERROR if there is a lexer or parser error.
     */
    void LoadSESSION( const wxString& aFilename );

    /**
     * Write the internal PCB instance out as a SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     * @param aNameChange If true, causes the pcb's name to change to "aFilename"
     *                    and also to to be changed in the output file.
     * @throw IO_ERROR, if an i/o error occurs saving the file.
     */
    void ExportPCB( const wxString& aFilename,  bool aNameChange=false );

    /**
     * Add the entire BOARD to the PCB but does not write it out.
     *
     * @note The #BOARD given to this function must have all the FOOTPRINTs on the component
     *       side of the BOARD.
     *
     * @see PCB_EDIT_FRAME::ExportToSpecctra() for an example before calling this function.
     *
     * @param aBoard The BOARD to convert to a PCB.
     */
    void FromBOARD( BOARD* aBoard );

    /**
     * Add the entire #SESSION info to a #BOARD but does not write it out.
     *
     * The #BOARD given to this function will have all its tracks and via's replaced, and all
     * its components are subject to being moved.
     *
     * @param aBoard The #BOARD to merge the #SESSION information into.
     */
    void FromSESSION( BOARD* aBoard );

    /**
     * Write the internal #SESSION instance out as a #SPECTRA DSN format file.
     *
     * @param aFilename The file to save to.
     */
    void ExportSESSION( const wxString& aFilename );

    /**
     * Build the board outlines and store it in m_brd_outlines.
     *
     * Because it calls GetBoardPolygonOutlines() it *must be* called before flipping footprints
     *
     * @return false if the board outlines cannot be built (not closed outlines)
     */
    bool BuiltBoardOutlines( BOARD* aBoard  );

    /**
     * Flip the footprints which are on the back side of the board to the front.
     */
    void FlipFOOTPRINTs( BOARD* aBoard );

    /**
     * Flip the footprints which were on the back side of the board back to the back.
     */
    void RevertFOOTPRINTs( BOARD* aBoard );

private:
    /**
     * Create a few data translation structures for layer name and number mapping between the
     * DSN::PCB structure and the KiCad #BOARD structure.
     *
     * @param aBoard The #BOARD to create the maps for.
     */
    void buildLayerMaps( BOARD* aBoard );

    /**
     * Return the PCB layer index for a given layer name, within the specctra sessionfile.
     *
     * @return the layer index within the specctra session file, or -1 if \a aLayerName is not
     *         found.
     */
    int findLayerName( const std::string& aLayerName ) const;

    /**
     * Read a &lt;pin_reference&gt; and splits it into the two parts which are on either side of
     * the hyphen.
     *
     * This function is specialized because pin_reference may or may not be using double quotes.
     * Both of these are legal:  U2-14 or "U2"-"14".  The lexer treats the first one as a single
     * T_SYMBOL, so in that case we have to split it into two here.
     * <p>
     * The caller should have already read in the first token comprising the pin_reference and
     * it will be tested through CurTok().
     * </p>
     * @param component_id Where to put the text preceding the '-' hyphen.
     * @param pid_id Where to put the text which trails the '-'.
     * @throw IO_ERROR, if the next token or two do no make up a pin_reference,
     *        or there is an error reading from the input stream.
     */
    void readCOMPnPIN( std::string* component_id, std::string* pid_id );

    /**
     * Read a &lt;time_stamp&gt; which consists of 8 lexer tokens:
     * "month date hour : minute : second year".
     *
     * This function is specialized because time_stamps occur more than once in a session file.
     * The caller should not have already read in the first token comprising the time stamp.
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
     * Make the board perimeter for the DSN file by filling the BOUNDARY element
     * in the specctra element tree.
     *
     * @param aBoard The BOARD to get information from in order to make the BOUNDARY.
     * @param aBoundary The empty BOUNDARY to fill in.
     */
    void fillBOUNDARY( BOARD* aBoard, BOUNDARY* aBoundary );

    /**
     * Allocates an I#MAGE on the heap and creates all the PINs according to the PADs in the
     * FOOTPRINT.
     *
     * @param aBoard The owner of the FOOTPRINT.
     * @param aFootprint The FOOTPRINT from which to build the IMAGE.
     * @return not tested for duplication yet.
     */
    IMAGE* makeIMAGE( BOARD* aBoard, FOOTPRINT* aFootprint );

    /**
     * Create a #PADSTACK which matches the given pad.
     *
     * @note Only pads which do not satisfy the function isKeepout() should be passed to this
     * function.
     *
     * @param aBoard The owner of the PAD's footprint.
     * @param aPad The PAD which needs to be made into a PADSTACK.
     * @return The created padstack, including its m_padstack_id.
     */
    PADSTACK* makePADSTACK( BOARD* aBoard, PAD* aPad );

    /**
     * Make a round through hole #PADSTACK using the given KiCad diameter in deci-mils.
     *
     * @param aCopperDiameter The diameter of the copper pad.
     * @param aDrillDiameter The drill diameter, used on re-import of the session file.
     * @param aTopLayer The DSN::PCB top most layer index.
     * @param aBotLayer The DSN::PCB bottom most layer index.
     * @return The padstack, which is on the heap only, user must save or delete it.
     */
    PADSTACK* makeVia( int aCopperDiameter, int aDrillDiameter,
                       int aTopLayer, int aBotLayer );

    /**
     * Make any kind of #PADSTACK using the given KiCad #VIA.
     *
     * @param aVia The #VIA to build the padstack from.
     * @return The padstack, which is on the heap only, user must save or delete it.
     */
    PADSTACK* makeVia( const ::PCB_VIA* aVia );

    /**
     * Delete all the NETs that may be in here.
     */
    void deleteNETs()
    {
        for( unsigned n = 0; n < m_nets.size(); ++n )
            delete m_nets[n];

        m_nets.clear();
    }

    /**
     * Export \a aNetClass to the DSN file.
     */
    void exportNETCLASS( const NETCLASS* aNetClass, const BOARD* aBoard );

    //-----</FromBOARD>------------------------------------------------------

    //-----<FromSESSION>-----------------------------------------------------

    /**
     * Create a #TRACK form the #PATH and #BOARD info.
     */
    PCB_TRACK* makeTRACK( WIRE* wire, PATH* aPath, int aPointIndex, int aNetcode );

    /**
     * Create an #ARC form the #PATH and #BOARD info.
     */
    PCB_ARC* makeARC( WIRE* wire, QARC* aQarc, int aNetcode );

    /**
     * Instantiate a KiCad #VIA on the heap and initializes it with internal
     * values consistent with the given #PADSTACK, #POINT, and netcode.
     */
    PCB_VIA* makeVIA( WIRE_VIA*aVia, PADSTACK* aPadstack, const POINT& aPoint, int aNetCode,
                      int aViaDrillDefault );

    //-----</FromSESSION>----------------------------------------------------

    /// specctra DSN keywords
    static const KEYWORD keywords[];

    PCB*              m_pcb;
    SHAPE_POLY_SET    m_brd_outlines;       // the board outlines for DSN export
    SESSION*          m_session;
    wxString          m_filename;
    std::string       m_quote_char;

    bool              m_footprintsAreFlipped;

    STRING_FORMATTER  m_sf;

    STRINGS           m_layerIds;        ///< indexed by PCB layer number

    std::map<PCB_LAYER_ID, int> m_kicadLayer2pcb; ///< maps BOARD layer number to PCB layer numbers
    std::map<int, PCB_LAYER_ID> m_pcbLayer2kicad;  ///< maps PCB layer number to BOARD layer numbers

    /// used during FromSESSION() only, memory for it is not owned here.
    UNIT_RES*         m_routeResolution;

    /// a copy to avoid passing as an argument, memory for it is not owned here.
    BOARD*            m_sessionBoard;

    static const KICAD_T scanPADs[];

    PADSTACKSET       m_padstackset;

    /// we don't want ownership here permanently, so we don't use boost::ptr_vector
    std::vector<NET*> m_nets;

    /// specctra cu layers, 0 based index:
    int               m_top_via_layer;
    int               m_bot_via_layer;
};

/**
 * @brief Helper method to import SES file to a board
 *
 * @param aBoard board object
 * @param aFullFilename specctra file name
 */

bool ImportSpecctraSession( BOARD* aBoard, const wxString& fullFileName );

}           // namespace DSN

#endif      // SPECCTRA_H_
