/**************/
/* dcode.h */
/**************/

#ifndef _DCODE_H_
#define _DCODE_H_

#include <vector>
#include <set>

#include "base_struct.h"


/**
 * Enum APERTURE_T
 * is the set of all gerber aperture types allowed, according to page 16 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
enum APERTURE_T {
    APT_CIRCLE  = 'C',
    APT_LINE    = 'L',
    APT_RECT    = 'R',
    APT_OVAL    = '0',
    APT_POLYGON = 'P',
    APT_MACRO   = 'M'
};

// In aperture definition, round, oval and rectangular flashed shapes
// can have a hole (ropund or rectangular)
// this option is stored in .m_DrillShape D_CODE member
enum APERTURE_DEF_HOLETYPE {
    APT_DEF_NO_HOLE = 0,
    APT_DEF_ROUND_HOLE,
    APT_DEF_RECT_HOLE
};

/* define min and max values for D Codes values.
 * note: values >= 0 and > FIRST_DCODE can be used for specila purposes
 */
#define FIRST_DCODE     10
#define LAST_DCODE      999
#define TOOLS_MAX_COUNT (LAST_DCODE + 1)

class D_CODE;


/**
 * Class DCODE_PARAM
 * holds a parameter for a DCODE or an "aperture macro" as defined within
 * standard RS274X.  The \a value field can be a constant, i.e. "immediate"
 * parameter or it may not be used if this param is going to defer to the
 * referencing aperture macro.  In that case, the \a index field is an index
 * into the aperture macro's parameters.
 */
class DCODE_PARAM
{
public:
    DCODE_PARAM() :
        index( -1 ),
        value( 0.0 )
    {}

    double GetValue( const D_CODE* aDcode ) const;

    void SetValue( double aValue )
    {
        value = aValue;
        index = -1;
    }


    /**
     * Function IsImmediate
     * tests if this DCODE_PARAM holds an immediate parameter or is a pointer
     * into a parameter held by an owning D_CODE.
     */
    bool IsImmediate() const { return index == -1; }

    unsigned GetIndex() const
    {
        return (unsigned) index;
    }


    void SetIndex( int aIndex )
    {
        index = aIndex;
    }


private:
    int    index;       ///< if -1, then \a value field is an immediate value,
                        //   else this is an index into parent's
                        //   D_CODE.m_am_params.
    double value;       ///< if IsImmediate()==true then use the value, else
                        //   not used.
};


/**
 * Enum AM_PRIMITIVE_ID
 * is the set of all "aperture macro primitives" (primitive numbers).  See
 * Table 3 in http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
enum AM_PRIMITIVE_ID {
    AMP_CIRCLE = 1,
    AMP_LINE2  = 2,
    AMP_LINE20 = 20,
    AMP_LINE_CENTER = 21,
    AMP_LINE_LOWER_LEFT = 22,
    AMP_EOF     = 3,
    AMP_OUTLINE = 4,
    AMP_POLYGON = 5,
    AMP_MOIRE   = 6,
    AMP_THERMAL = 7,
};


typedef std::vector<DCODE_PARAM> DCODE_PARAMS;

/**
 * Struct AM_PRIMITIVE
 * holds an aperture macro primitive as given in Table 3 of
 * http://gerbv.sourceforge.net/docs/rs274xrevd_e.pdf
 */
struct AM_PRIMITIVE
{
    AM_PRIMITIVE_ID primitive_id;       ///< The primitive type
    DCODE_PARAMS    params;             ///< A sequence of parameters used by
                                        //   the primitive

    /**
     * Function GetExposure
     * returns the first parameter in integer form.  Some but not all primitives
     * use the first parameter as an exposure control.
     */
    int GetExposure() const
    {
        // No D_CODE* for GetValue()
        wxASSERT( params.size() && params[0].IsImmediate() );
        return (int) params[0].GetValue( NULL );
    }
};


typedef std::vector<AM_PRIMITIVE> AM_PRIMITIVES;

/**
 * Struct APERTURE_MACRO
 * helps support the "aperture macro" defined within standard RS274X.
 */
struct APERTURE_MACRO
{
    wxString      name;             ///< The name of the aperture macro
    AM_PRIMITIVES primitives;       ///< A sequence of AM_PRIMITIVEs
};


/**
 * Struct APERTURE_MACRO_less_than
 * is used by std:set<APERTURE_MACRO> instantiation which uses
 * APERTURE_MACRO.name as its key.
 */
struct APERTURE_MACRO_less_than
{
    // a "less than" test on two APERTURE_MACROs (.name wxStrings)
    bool operator()( const APERTURE_MACRO& am1, const APERTURE_MACRO& am2 ) const
    {
        return am1.name.Cmp( am2.name ) < 0;  // case specific wxString compare
    }
};


/**
 * Type APERTURE_MACRO_SET
 * is a sorted collection of APERTURE_MACROS whose key is the name field in
 * the APERTURE_MACRO.
 */
typedef std::set<APERTURE_MACRO, APERTURE_MACRO_less_than> APERTURE_MACRO_SET;
typedef std::pair<APERTURE_MACRO_SET::iterator, bool>      APERTURE_MACRO_SET_PAIR;


/**
 * Class D_CODE
 * holds a gerber DCODE definition.
 */
class D_CODE
{
    friend class DCODE_PARAM;

    APERTURE_MACRO* m_Macro;    ///< no ownership, points to
                                //   GERBER.m_aperture_macros element

    /**
     * parameters used only when this D_CODE holds a reference to an aperture
     * macro, and these parameters would customize the macro.
     */
    DCODE_PARAMS          m_am_params;

    std::vector <wxPoint> m_PolyCorners;    /* Polygon used to draw AMP_POLYGON shape and some other
                                             * complex shapes which are converted to polygon
                                             * (shapes with hole, rotated rectangles ...
                                             */

public:
    wxSize                m_Size;                   /* Horizontal and vertical dimensions. */
    APERTURE_T            m_Shape;                  /* shape ( Line, rectangle, circle , oval .. ) */
    int                   m_Num_Dcode;              /* D code ( >= 10 ) */
    wxSize                m_Drill;                  /* dimension of the hole (if any) */
    APERTURE_DEF_HOLETYPE m_DrillShape;             /* shape of the hole (0 = no hole, round = 1, rect = 2) */
    double                m_Rotation;               /* shape rotation in degrees */
    int                   m_EdgesCount;             /* in apeture definition Polygon only: number of edges for the polygon */
    bool                  m_InUse;                  /* FALSE if not used */
    bool                  m_Defined;                /* FALSE if not defined */
    wxString              m_SpecialDescr;

public:
    D_CODE( int num_dcode );
    ~D_CODE();
    void Clear_D_CODE_Data();

    void AppendParam( double aValue )
    {
        DCODE_PARAM param;

        param.SetValue( aValue );

        m_am_params.push_back( param );
    }


    void SetMacro( APERTURE_MACRO* aMacro )
    {
        m_Macro = aMacro;
    }


    APERTURE_MACRO* GetMacro() { return m_Macro; }

    /**
     * Function ShowApertureType
     * returns a character string telling what type of aperture type \a aType is.
     * @param aType The aperture type to show.
     */
    static const wxChar* ShowApertureType( APERTURE_T aType );

    /** function DrawFlashedShape
     * Draw the dcode shape for flashed items.
     * When an item is flashed, the DCode shape is the shape of the item
     */
    void                 DrawFlashedShape( EDA_Rect* aClipBox, wxDC* aDC, int aColor,
                                           wxPoint aShapePos, bool aFilledShape );

    /** function DrawFlashedPolygon
     * a helper function used id ::Draw to draw the polygon stored ion m_PolyCorners
     * Draw some Apertures shapes when they are defined as filled polygons.
     * APT_POLYGON is always a polygon, but some complex shapes are also converted to
     * polygons (shapes with holes, some rotated shapes)
     */
    void                 DrawFlashedPolygon( EDA_Rect* aClipBox, wxDC* aDC, int aColor,
                                             bool aFilled, const wxPoint& aPosition );

    /** function ConvertShapeToPolygon
     * convert a shape to an equivalent polygon.
     * Arcs and circles are approximated by segments
     * Useful when a shape is not a graphic primitive (shape with hole,
     * rotated shape ... ) and cannot be easily drawn.
     */
    void                 ConvertShapeToPolygon();
};


inline double DCODE_PARAM::GetValue( const D_CODE* aDcode ) const
{
    if( IsImmediate() )
        return value;
    else
    {
        // the first one was numbered 1, not zero, as in $1, see page 19 of spec.
        unsigned ndx = GetIndex() - 1;
        wxASSERT( aDcode );

        // get the parameter from the aDcode
        if( ndx < aDcode->m_am_params.size() )
            return aDcode->m_am_params[ndx].GetValue( NULL );
        else
        {
            wxASSERT( GetIndex() - 1 < aDcode->m_am_params.size() );
            return 0.0;
        }
    }
}


#endif  // ifndef _DCODE_H_
