// PolyLine.h ... definition of CPolyLine class

//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of
// each contour.
//
// When used for copper (or technical layers) areas, the first contour is the outer edge
// of the area, subsequent ones are "holes" in the copper.

#ifndef POLYLINE_H
#define POLYLINE_H

#include <vector>

#include "kbool/include/kbool/booleng.h"
#include "pad_shapes.h"

// inflection modes for DS_LINE and DS_LINE_VERTEX, used in math_for_graphics.cpp
enum
{
    IM_NONE = 0,
    IM_90_45,
    IM_45_90,
    IM_90
};



/**
 * Function ArmBoolEng
 * Initialise parameters used in kbool
 * @param aBooleng = pointer to the Bool_Engine to initialise
 * @param aConvertHoles = mode for holes when a boolean operation is made
 *   true: holes are linked into outer contours by double overlapping segments
 *   false: holes are not linked: in this mode contours are added clockwise
 *          and polygons added counter clockwise are holes (default)
 */
void ArmBoolEng( Bool_Engine* aBooleng, bool aConvertHoles = false );


#define PCBU_PER_MIL 10
#define NM_PER_MIL   10 // 25400

#define to_int( x ) wxRound( (x) )
#ifndef MIN
#define MIN( x1, x2 ) ( (x1) > (x2) ? (x2) : (x1) )
#endif
#ifndef MAX
#define MAX( x1, x2 ) ( (x1) > (x2) ? (x1) : (x2) )
#endif

class CRect
{
public:
    int left, right, top, bottom;
};

class CPoint
{
public:
    int x, y;
public:
    CPoint( void ) { x = y = 0; };
    CPoint( int i, int j ) { x = i; y = j; };
};

class CSegment
{
public:
    int xi, yi, xf, yf;
    CSegment() { };
    CSegment( int x0, int y0, int x1, int y1 )
    {
        xi = x0; yi = y0; xf = x1; yf = y1;
    }
};


#include "math_for_graphics.h"

class CArc
{
public:
    enum { MAX_STEP = 50 * 25400 };     // max step is 20 mils
    enum { MIN_STEPS = 18 };            // min step is 5 degrees
    int  style;
    int  xi, yi, xf, yf;
    int  n_steps;   // number of straight-line segments in gpc_poly
    bool bFound;
};

class CPolyPt
{
public:
    CPolyPt( int qx = 0, int qy = 0, bool qf = false )
    { x = qx; y = qy; end_contour = qf; utility = 0; };
    int  x;
    int  y;
    bool end_contour;
    int  utility;

    bool operator == (const CPolyPt& cpt2 ) const
    { return (x == cpt2.x) && (y == cpt2.y) && (end_contour == cpt2.end_contour); }

    bool operator != (CPolyPt& cpt2 ) const
    { return (x != cpt2.x) || (y != cpt2.y) || (end_contour != cpt2.end_contour); }
};

#include "polygon_test_point_inside.h"

class CPolyLine
{
public:
    enum { STRAIGHT, ARC_CW, ARC_CCW };                 // side styles
    enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE };    // hatch styles

    // constructors/destructor
    CPolyLine();
    ~CPolyLine();

    // functions for modifying polyline
    void       Start( int layer, int x, int y, int hatch );
    void       AppendCorner( int x, int y, int style = STRAIGHT, bool bDraw = false );
    void       InsertCorner( int ic, int x, int y );
    void       DeleteCorner( int ic, bool bDraw = false );
    void       MoveCorner( int ic, int x, int y );
    void       Close( int style = STRAIGHT, bool bDraw = false );
    void       RemoveContour( int icont );

    /**
     * Function Chamfer
     * returns a chamfered version of a polygon.
     * @param aDistance is the chamfering distance.
     * @return CPolyLine* - Pointer to new polygon.
     */
    CPolyLine* Chamfer( unsigned int aDistance );

    /**
     * Function Fillet
     * returns a filleted version of a polygon.
     * @param aDistance is the fillet radius.
     * @param aSegments is the number of segments / fillet.
     * @return CPolyLine* - Pointer to new polygon.
     */
    CPolyLine* Fillet( unsigned int aRadius, unsigned int aSegments );

    void       RemoveAllContours( void );

    // drawing functions
    void       Undraw();
    void       Draw();
    void       Hatch();
    void       MoveOrigin( int x_off, int y_off );

    // misc. functions
    CRect      GetBounds();
    CRect      GetCornerBounds();
    CRect      GetCornerBounds( int icont );
    void       Copy( CPolyLine* src );
    bool       TestPointInside( int x, int y );
    bool       IsCutoutContour( int icont );
    void       AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );

    // access functions
    int        GetLayer() { return m_layer; }
    int        GetNumCorners();
    int        GetNumSides();
    int        GetClosed();
    int        GetNumContours();
    int        GetContour( int ic );
    int        GetContourStart( int icont );
    int        GetContourEnd( int icont );
    int        GetContourSize( int icont );
    int        GetX( int ic );
    int        GetY( int ic );
    int        GetEndContour( int ic );

    int        GetUtility( int ic ) { return corner[ic].utility; };
    void       SetUtility( int ic, int utility ) { corner[ic].utility = utility; };
    int        GetSideStyle( int is );

    int        GetHatchStyle() { return m_HatchStyle; }
    void       SetHatch( int hatch ) { Undraw(); m_HatchStyle = hatch; Draw(); };
    void       SetX( int ic, int x );
    void       SetY( int ic, int y );
    void       SetEndContour( int ic, bool end_contour );
    void       SetSideStyle( int is, int style );

    int        RestoreArcs( std::vector<CArc> * arc_array, std::vector<CPolyLine*> * pa = NULL );

    int NormalizeAreaOutlines( std::vector<CPolyLine*> * pa = NULL,
                               bool                      bRetainArcs = false );

    // KBOOL functions

    /**
     * Function AddPolygonsToBoolEng
     * and edges contours to a kbool engine, preparing a boolean op between polygons
     * @param aStart_contour: starting contour number (-1 = all, 0 is the outlines of zone, > 1 = holes in zone
     * @param aEnd_contour: ending contour number (-1 = all after  aStart_contour)
     * @param arc_array: arc connverted to poly (NULL if not exists)
     * @param aBooleng : pointer on a bool engine (handle a set of polygons)
     * @param aGroup : group to fill (aGroup = GROUP_A or GROUP_B) operations are made between GROUP_A and GROUP_B
     */
    int AddPolygonsToBoolEng( Bool_Engine*        aBooleng,
                              GroupType           aGroup,
                              int                 aStart_contour = -1,
                              int                 aEnd_contour = -1,
                              std::vector<CArc> * arc_array = NULL );

    /**
     * Function MakeKboolPoly
     * fill a kbool engine with a closed polyline contour
     * approximates arcs with multiple straight-line segments
     * @param aStart_contour: starting contour number (-1 = all, 0 is the outlines of zone, > 1 = holes in zone
     * @param aEnd_contour: ending contour number (-1 = all after  aStart_contour)
     *  combining intersecting contours if possible
     * @param arc_array : return data on arcs in arc_array
     * @param aConvertHoles = mode for holes when a boolean operation is made
     *   true: holes are linked into outer contours by double overlapping segments
     *   false: holes are not linked: in this mode contours are added clockwise
     *          and polygons added counter clockwise are holes (default)
     * @return error: 0 if Ok, 1 if error
     */
    int MakeKboolPoly( int                 aStart_contour = -1,
                       int                 aEnd_contour = -1,
                       std::vector<CArc> * arc_array = NULL,
                       bool aConvertHoles = false);

    /**
     * Function NormalizeWithKbool
     * Use the Kbool Library to clip contours: if outlines are crossing, the self-crossing polygon
     * is converted to non self-crossing polygon by adding extra points at the crossing locations
     * and reordering corners
     * if more than one outside contour are found, extra CPolyLines will be created
     * because copper areas have only one outside contour
     * Therefore, if this results in new CPolyLines, return them as std::vector pa
     * @param aExtraPolyList: pointer on a std::vector<CPolyLine*> to store extra CPolyLines
     * @param bRetainArcs == false, try to retain arcs in polys
     * @return number of external contours, or -1 if error
     */
    int NormalizeWithKbool( std::vector<CPolyLine*> * aExtraPolyList, bool bRetainArcs );

    /**
     * Function GetKboolEngine
     * @return the current used Kbool Engine (after normalization using kbool)
     */
    Bool_Engine* GetKboolEngine( ) { return  m_Kbool_Poly_Engine; }
    /**
     * Function FreeKboolEngine
     * delete the current used Kbool Engine (free memory after normalization using kbool)
     */
    void FreeKboolEngine( ) { delete m_Kbool_Poly_Engine; m_Kbool_Poly_Engine = NULL; }


private:
    int m_layer;    // layer to draw on
    int m_Width;    // lines width when drawing. Provided but not really used
    int utility;
public:
    std::vector <CPolyPt>  corner;              // array of points for corners
    std::vector <int>      side_style;          // array of styles for sides
    int m_HatchStyle;                           // hatch style, see enum above
    std::vector <CSegment> m_HatchLines;        // hatch lines
private:
    Bool_Engine*           m_Kbool_Poly_Engine; // polygons set in kbool engine data
    bool bDrawn;

    // Bezier Support
public:
    void AppendBezier(int x1, int y1, int x2, int y2, int x3, int y3);
    void AppendBezier(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
};

#endif  // #ifndef POLYLINE_H
