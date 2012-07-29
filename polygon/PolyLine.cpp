// PolyLine.cpp ... implementation of CPolyLine class from FreePCB.

//
// implementation for kicad and kbool polygon clipping library
//
#include <math.h>
#include <vector>
#include <algorithm>

#include <fctsys.h>
#include <common.h>     // KiROUND

#include <PolyLine.h>
#include <bezier_curves.h>
#include <polygon_test_point_inside.h>

CPolyLine::CPolyLine()
{
    m_hatchStyle    = NO_HATCH;
    m_hatchPitch    = 0;
    m_width     = 0;
    m_utility   = 0;
    m_Kbool_Poly_Engine = NULL;
}


// destructor, removes display elements
//
CPolyLine::~CPolyLine()
{
    UnHatch();

    if( m_Kbool_Poly_Engine )
        delete m_Kbool_Poly_Engine;
}


/**
 * Function armBoolEng
 * Initialise parameters used in kbool
 * @param aBooleng = pointer to the Bool_Engine to initialise
 * @param aConvertHoles = mode for holes when a boolean operation is made
 *   true: holes are linked into outer contours by double overlapping segments
 *   false: holes are not linked: in this mode contours are added clockwise
 *          and polygons added counter clockwise are holes (default)
 */
static void armBoolEng( Bool_Engine* aBooleng, bool aConvertHoles = false );

/**
 * Function NormalizeWithKbool
 * Use the Kbool Library to clip contours: if outlines are crossing, the self-crossing polygon
 * is converted to non self-crossing polygon by adding extra points at the crossing locations
 * and reordering corners
 * if more than one outside contour are found, extra CPolyLines will be created
 * because copper areas have only one outside contour
 * Therefore, if this results in new CPolyLines, return them as std::vector pa
 * @param aExtraPolyList: pointer on a std::vector<CPolyLine*> to store extra CPolyLines
 * @param bRetainArcs == true, try to retain arcs in polys
 * @return number of external contours, or -1 if error
 */
int CPolyLine::NormalizeWithKbool( std::vector<CPolyLine*>* aExtraPolyList, bool bRetainArcs )
{
    std::vector<CArc>   arc_array;
    std::vector <void*> hole_array; // list of holes
    std::vector<int>*   hole;       // used to store corners for a given hole
    CPolyLine*          polyline;
    int n_ext_cont = 0;             // CPolyLine count

    /* Creates a bool engine from this CPolyLine.
     * Normalized outlines and holes will be in m_Kbool_Poly_Engine
     * If some polygons are self crossing, after running the Kbool Engine, self crossing polygons
     * will be converted in non self crossing polygons by inserting extra points at the crossing locations
     * True holes are combined if possible
     */
    if( bRetainArcs )
        MakeKboolPoly( &arc_array );
    else
        MakeKboolPoly( NULL );

    UnHatch();

    /* now, recreate polys
     * if more than one outside contour are found, extra CPolyLines will be created
     * because copper areas have only one outside contour
     * the first outside contour found is the new "this" outside contour
     * if others outside contours are found we create new CPolyLines
     * Note: if there are holes in polygons, we must store them
     * and when all outside contours are found, search the corresponding outside contour for each hole
     */
    while( m_Kbool_Poly_Engine->StartPolygonGet() )
    {
        // See if the current polygon is flagged as a hole
        if( m_Kbool_Poly_Engine->GetPolygonPointEdgeType() == KB_INSIDE_EDGE )
        {
            hole = new std::vector<int>;
            hole_array.push_back( hole );

            while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )    // store hole
            {
                int x   = (int) m_Kbool_Poly_Engine->GetPolygonXPoint();
                int y   = (int) m_Kbool_Poly_Engine->GetPolygonYPoint();
                hole->push_back( x );
                hole->push_back( y );
            }

            m_Kbool_Poly_Engine->EndPolygonGet();
        }
        else if( n_ext_cont == 0 )
        {
            // first external contour, replace this poly
            m_CornersList.clear();
            m_SideStyle.clear();
            bool first = true;

            while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )
            {
                // foreach point in the polygon
                int x   = (int) m_Kbool_Poly_Engine->GetPolygonXPoint();
                int y   = (int) m_Kbool_Poly_Engine->GetPolygonYPoint();

                if( first )
                {
                    first = false;
                    Start( GetLayer(), x, y, GetHatchStyle() );
                }
                else
                    AppendCorner( x, y );
            }

            m_Kbool_Poly_Engine->EndPolygonGet();
            Close();
            n_ext_cont++;
        }
        else if( aExtraPolyList )                                               // a new outside contour is found: create a new CPolyLine
        {
            polyline = new CPolyLine;                                           // create new poly
            aExtraPolyList->push_back( polyline );                              // put it in array
            bool first = true;

            while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )                // read next external contour
            {
                int x   = (int) m_Kbool_Poly_Engine->GetPolygonXPoint();
                int y   = (int) m_Kbool_Poly_Engine->GetPolygonYPoint();

                if( first )
                {
                    first = false;
                    polyline->Start( GetLayer(), x, y, GetHatchStyle() );
                }
                else
                    polyline->AppendCorner( x, y );
            }

            m_Kbool_Poly_Engine->EndPolygonGet();
            polyline->Close( STRAIGHT, false );
            n_ext_cont++;
        }
    }

    // now add cutouts to the corresponding CPolyLine(s)
    for( unsigned ii = 0; ii < hole_array.size(); ii++ )
    {
        hole        = (std::vector<int>*)hole_array[ii];
        polyline    = NULL;

        if( n_ext_cont == 1 )
        {
            polyline = this;
        }
        else
        {
            // find the polygon that contains this hole
            // testing one corner inside is enought because a hole is entirely inside the polygon
            // so we test only the first corner
            int x   = (*hole)[0];
            int y   = (*hole)[1];

            if( TestPointInside( x, y ) )
                polyline = this;
            else if( aExtraPolyList )
            {
                for( int ext_ic = 0; ext_ic<n_ext_cont - 1; ext_ic++ )
                {
                    if( (*aExtraPolyList)[ext_ic]->TestPointInside( x, y ) )
                    {
                        polyline = (*aExtraPolyList)[ext_ic];
                        break;
                    }
                }
            }
        }

        if( !polyline )
            wxASSERT( 0 );
        else
        {
            for( unsigned ii = 0; ii< (*hole).size(); ii++ )
            {
                int x   = (*hole)[ii]; ii++;
                int y   = (*hole)[ii];
                polyline->AppendCorner( x, y, STRAIGHT, false );
            }

            polyline->Close( STRAIGHT, false );
        }
    }

    if( bRetainArcs )
        RestoreArcs( &arc_array, aExtraPolyList );

    delete m_Kbool_Poly_Engine;
    m_Kbool_Poly_Engine = NULL;

    // free hole list
    for( unsigned ii = 0; ii < hole_array.size(); ii++ )
        delete (std::vector<int>*)hole_array[ii];

    return n_ext_cont;
}


/**
 * Function MakeKboolPoly
 * fill a kbool engine with a closed polyline contour
 * approximates arcs with multiple straight-line segments
 *  combining intersecting contours if possible
 * @param arc_array : return corners computed from arcs approximations in arc_array
 * @return error: 0 if Ok, 1 if error
 */
int CPolyLine::MakeKboolPoly( std::vector<CArc>* arc_array )
{
    if( m_Kbool_Poly_Engine )
    {
        delete m_Kbool_Poly_Engine;
        m_Kbool_Poly_Engine = NULL;
    }

    if( !GetClosed() )
        return 1; // error

    int n_arcs = 0;
    int polycount = GetContoursCount();
    int last_contour = polycount - 1;

    if( arc_array )
        arc_array->clear();

    int iarc = 0;

    for( int icont = 0; icont<=last_contour; icont++ )
    {
        // Fill a kbool engine for this contour,
        // and combine it with previous contours
        Bool_Engine* booleng = new Bool_Engine();
        armBoolEng( booleng, false );

        // first, calculate number of vertices in contour
        int n_vertices = 0;
        int ic_st   = GetContourStart( icont );
        int ic_end  = GetContourEnd( icont );

        if( !booleng->StartPolygonAdd( GROUP_B ) )
        {
            wxASSERT( 0 );
            return 1;    // error
        }

        for( int ic = ic_st; ic<=ic_end; ic++ )
        {
            int style = m_SideStyle[ic];

            if( style == STRAIGHT )
                n_vertices++;
            else
            {
                // style is ARC_CW or ARC_CCW
                int n = CArc::ARC_STEPS;
                n_vertices += n;
                n_arcs++;
            }
        }

        // now enter this contour to booleng
        int ivtx = 0;

        for( int ic = ic_st; ic<=ic_end; ic++ )
        {
            int style   = m_SideStyle[ic];
            int x1      = m_CornersList[ic].x;
            int y1      = m_CornersList[ic].y;
            int x2, y2;

            if( ic < ic_end )
            {
                x2  = m_CornersList[ic + 1].x;
                y2  = m_CornersList[ic + 1].y;
            }
            else
            {
                x2  = m_CornersList[ic_st].x;
                y2  = m_CornersList[ic_st].y;
            }

            if( style == STRAIGHT )
            {
                booleng->AddPoint( x1, y1 );
                ivtx++;
            }
            else
            {
                // style is arc_cw or arc_ccw
                int     n;                      // number of steps for arcs
                n = CArc::ARC_STEPS;
                double  xo, yo, theta1, theta2, a, b;
                a   = fabs( (double) (x1 - x2) );
                b   = fabs( (double) (y1 - y2) );

                if( style == CPolyLine::ARC_CW )
                {
                    // clockwise arc (ie.quadrant of ellipse)
                    if( x2 > x1 && y2 > y1 )
                    {
                        // first quadrant, draw second quadrant of ellipse
                        xo      = x2;
                        yo      = y1;
                        theta1  = M_PI;
                        theta2  = M_PI / 2.0;
                    }
                    else if( x2 < x1 && y2 > y1 )
                    {
                        // second quadrant, draw third quadrant of ellipse
                        xo      = x1;
                        yo      = y2;
                        theta1  = 3.0 * M_PI / 2.0;
                        theta2  = M_PI;
                    }
                    else if( x2 < x1 && y2 < y1 )
                    {
                        // third quadrant, draw fourth quadrant of ellipse
                        xo      = x2;
                        yo      = y1;
                        theta1  = 2.0 * M_PI;
                        theta2  = 3.0 * M_PI / 2.0;
                    }
                    else
                    {
                        xo      = x1; // fourth quadrant, draw first quadrant of ellipse
                        yo      = y2;
                        theta1  = M_PI / 2.0;
                        theta2  = 0.0;
                    }
                }
                else
                {
                    // counter-clockwise arc
                    if( x2 > x1 && y2 > y1 )
                    {
                        xo      = x1; // first quadrant, draw fourth quadrant of ellipse
                        yo      = y2;
                        theta1  = 3.0 * M_PI / 2.0;
                        theta2  = 2.0 * M_PI;
                    }
                    else if( x2 < x1 && y2 > y1 )
                    {
                        xo      = x2; // second quadrant
                        yo      = y1;
                        theta1  = 0.0;
                        theta2  = M_PI / 2.0;
                    }
                    else if( x2 < x1 && y2 < y1 )
                    {
                        xo      = x1; // third quadrant
                        yo      = y2;
                        theta1  = M_PI / 2.0;
                        theta2  = M_PI;
                    }
                    else
                    {
                        xo      = x2; // fourth quadrant
                        yo      = y1;
                        theta1  = M_PI;
                        theta2  = 3.0 * M_PI / 2.0;
                    }
                }

                // now write steps for arc
                if( arc_array )
                {
                    CArc new_arc;
                    new_arc.style   = style;
                    new_arc.n_steps = n;
                    new_arc.xi  = x1;
                    new_arc.yi  = y1;
                    new_arc.xf  = x2;
                    new_arc.yf  = y2;
                    arc_array->push_back( new_arc );
                    iarc++;
                }

                for( int is = 0; is<n; is++ )
                {
                    double  theta   = theta1 + ( (theta2 - theta1) * (double) is ) / n;
                    double  x       = xo + a* cos( theta );
                    double  y       = yo + b* sin( theta );

                    if( is == 0 )
                    {
                        x   = x1;
                        y   = y1;
                    }

                    booleng->AddPoint( x, y );
                    ivtx++;
                }
            }
        }

        if( n_vertices != ivtx )
        {
            wxASSERT( 0 );
        }

        // close list added to the bool engine
        booleng->EndPolygonAdd();

        /* now combine polygon to the previous polygons.
         * note: the first polygon is the outline contour, and others are holes inside the first polygon
         * The first polygon is ORed with nothing, but is is a trick to sort corners (vertex)
         * clockwise with the kbool engine.
         * Others polygons are substract to the outline and corners will be ordered counter clockwise
         * by the kbool engine
         */
        if( icont != 0 )    // substract hole to outside ( if the outline contour is take in account)
        {
            booleng->Do_Operation( BOOL_A_SUB_B );
        }
        else       // add outside or add holes if we do not use the outline contour
        {
            booleng->Do_Operation( BOOL_OR );
        }

        // now use result as new polygon (delete the old one if exists)
        if( m_Kbool_Poly_Engine )
            delete m_Kbool_Poly_Engine;

        m_Kbool_Poly_Engine = booleng;
    }

    return 0;
}


/**
 * Function armBoolEng
 * Initialise parameters used in kbool
 * @param aBooleng = pointer to the Bool_Engine to initialise
 * @param aConvertHoles = mode for holes when a boolean operation is made
 *   true: in resulting polygon, holes are linked into outer contours by double overlapping segments
 *   false: in resulting polygons, holes are not linked: they are separate polygons
 */
void armBoolEng( Bool_Engine* aBooleng, bool aConvertHoles )
{
    // set some global vals to arm the boolean engine

    // input points are scaled up with GetDGrid() * GetGrid()

    // DGRID is only meant to make fractional parts of input data which
    /*
     *  The input data scaled up with DGrid is related to the accuracy the user has in his input data.
     *  User data with a minimum accuracy of 0.001, means set the DGrid to 1000.
     *  The input data may contain data with a minimum accuracy much smaller, but by setting the DGrid
     *  everything smaller than 1/DGrid is rounded.
     *
     *  DGRID is only meant to make fractional parts of input data which can be
     *  doubles, part of the integers used in vertexes within the boolean algorithm.
     *  And therefore DGRID bigger than 1 is not usefull, you would only loose accuracy.
     *  Within the algorithm all input data is multiplied with DGRID, and the result
     *  is rounded to an integer.
     */
    double DGRID = 1000.0;      // round coordinate X or Y value in calculations to this (initial value = 1000.0 in kbool example)
                                // kbool uses DGRID to convert float user units to integer
                                // kbool unit = (int)(user unit * DGRID)
                                // Note: in kicad, coordinates are already integer so DGRID could be set to 1
                                // we can choose 1.0,
                                // but choose DGRID = 1000.0 solves some filling problems
// (perhaps because this allows a better precision in kbool internal calculations

    double MARGE = 1.0 / DGRID;         // snap with in this range points to lines in the intersection routines
                                        // should always be >= 1/DGRID  a  MARGE >= 10/DGRID is ok
                                        // this is also used to remove small segments and to decide when
                                        // two segments are in line. ( initial value = 0.001 )
                                        // For kicad we choose MARGE = 1/DGRID

    double CORRECTIONFACTOR = 0.0;      // correct the polygons by this number: used in BOOL_CORRECTION operation
                                        // this operation shrinks a polygon if CORRECTIONFACTOR < 0
                                        // or stretch it if CORRECTIONFACTOR > 0
                                        // the size change is CORRECTIONFACTOR (holes are correctly handled)
    double  CORRECTIONABER  = 1.0;      // the accuracy for the rounded shapes used in correction
    double  ROUNDFACTOR     = 1.5;      // when will we round the correction shape to a circle
    double  SMOOTHABER      = 10.0;     // accuracy when smoothing a polygon
    double  MAXLINEMERGE    = 1000.0;   // leave as is, segments of this length in smoothen


    /*
     *    Grid makes sure that the integer data used within the algorithm has room for extra intersections
     *    smaller than the smallest number within the input data.
     *    The input data scaled up with DGrid is related to the accuracy the user has in his input data.
     *    Another scaling with Grid is applied on top of it to create space in the integer number for
     *    even smaller numbers.
     */
    int GRID = (int) ( 10000.0 / DGRID );    // initial value = 10000 in kbool example but we use

    // 10000/DGRID because the scaling is made by DGRID
    // on integer pcbnew units and the global scaling
    // ( GRID*DGRID) must be < 30000 to avoid overflow
    // in calculations (made in long long in kbool)
    if( GRID <= 1 ) // Cannot be null!
        GRID = 1;

    aBooleng->SetMarge( MARGE );
    aBooleng->SetGrid( GRID );
    aBooleng->SetDGrid( DGRID );
    aBooleng->SetCorrectionFactor( CORRECTIONFACTOR );
    aBooleng->SetCorrectionAber( CORRECTIONABER );
    aBooleng->SetSmoothAber( SMOOTHABER );
    aBooleng->SetMaxlinemerge( MAXLINEMERGE );
    aBooleng->SetRoundfactor( ROUNDFACTOR );
    aBooleng->SetWindingRule( true );           // This is the default kbool value

    if( aConvertHoles )
    {
#if 1                                                   // Can be set to 1 for kbool version >= 2.1, must be set to 0 for previous versions
         // SetAllowNonTopHoleLinking() exists only in kbool >= 2.1
        aBooleng->SetAllowNonTopHoleLinking( false );   // Default = true, but i have problems (filling errors) when true
#endif
        aBooleng->SetLinkHoles( true );                 // holes will be connected by double overlapping segments
        aBooleng->SetOrientationEntryMode( false );     // all polygons are contours, not holes
    }
    else
    {
        aBooleng->SetLinkHoles( false );                // holes will not be connected by double overlapping segments
        aBooleng->SetOrientationEntryMode( true );      // holes are entered counter clockwise
    }
}


int CPolyLine::NormalizeAreaOutlines( std::vector<CPolyLine*>* pa, bool bRetainArcs )
{
    return NormalizeWithKbool( pa, bRetainArcs );
}


// Restore arcs to a polygon where they were replaced with steps
// If pa != NULL, also use polygons in pa array
//
int CPolyLine::RestoreArcs( std::vector<CArc>* arc_array, std::vector<CPolyLine*>* pa )
{
    // get poly info
    int n_polys = 1;

    if( pa )
        n_polys += pa->size();

    CPolyLine* poly;

    // undraw polys and clear m_utility flag for all corners
    for( int ip = 0; ip<n_polys; ip++ )
    {
        if( ip == 0 )
            poly = this;
        else
            poly = (*pa)[ip - 1];

        poly->UnHatch();

        for( int ic = 0; ic<poly->GetNumCorners(); ic++ )
            poly->SetUtility( ic, 0 );

        // clear m_utility flag
    }

    // find arcs and replace them
    bool    bFound;
    int     arc_start   = 0;
    int     arc_end     = 0;

    for( unsigned iarc = 0; iarc<arc_array->size(); iarc++ )
    {
        int arc_xi  = (*arc_array)[iarc].xi;
        int arc_yi  = (*arc_array)[iarc].yi;
        int arc_xf  = (*arc_array)[iarc].xf;
        int arc_yf  = (*arc_array)[iarc].yf;
        int n_steps = (*arc_array)[iarc].n_steps;
        int style   = (*arc_array)[iarc].style;
        bFound = false;

        // loop through polys
        for( int ip = 0; ip<n_polys; ip++ )
        {
            if( ip == 0 )
                poly = this;
            else
                poly = (*pa)[ip - 1];

            int polycount = poly->GetContoursCount();

            for( int icont = 0; icont < polycount; icont++ )
            {
                int ic_start    = poly->GetContourStart( icont );
                int ic_end      = poly->GetContourEnd( icont );

                if( (ic_end - ic_start) > n_steps )
                {
                    for( int ic = ic_start; ic<=ic_end; ic++ )
                    {
                        int ic_next = ic + 1;

                        if( ic_next > ic_end )
                            ic_next = ic_start;

                        int xi  = poly->GetX( ic );
                        int yi  = poly->GetY( ic );

                        if( xi == arc_xi && yi == arc_yi )
                        {
                            // test for forward arc
                            int ic2 = ic + n_steps;

                            if( ic2 > ic_end )
                                ic2 = ic2 - ic_end + ic_start - 1;

                            int xf  = poly->GetX( ic2 );
                            int yf  = poly->GetY( ic2 );

                            if( xf == arc_xf && yf == arc_yf )
                            {
                                // arc from ic to ic2
                                bFound      = true;
                                arc_start   = ic;
                                arc_end     = ic2;
                            }
                            else
                            {
                                // try reverse arc
                                ic2 = ic - n_steps;

                                if( ic2 < ic_start )
                                    ic2 = ic2 - ic_start + ic_end + 1;

                                xf  = poly->GetX( ic2 );
                                yf  = poly->GetY( ic2 );

                                if( xf == arc_xf && yf == arc_yf )
                                {
                                    // arc from ic2 to ic
                                    bFound      = true;
                                    arc_start   = ic2;
                                    arc_end     = ic;
                                    style       = 3 - style;
                                }
                            }

                            if( bFound )
                            {
                                poly->m_SideStyle[arc_start] = style;

                                // mark corners for deletion from arc_start+1 to arc_end-1
                                for( int i = arc_start + 1; i!=arc_end; )
                                {
                                    if( i > ic_end )
                                        i = ic_start;

                                    poly->SetUtility( i, 1 );

                                    if( i == ic_end )
                                        i = ic_start;
                                    else
                                        i++;
                                }

                                break;
                            }
                        }

                        if( bFound )
                            break;
                    }
                }

                if( bFound )
                    break;
            }
        }

        if( bFound )
            (*arc_array)[iarc].bFound = true;
    }

    // now delete all marked corners
    for( int ip = 0; ip<n_polys; ip++ )
    {
        if( ip == 0 )
            poly = this;
        else
            poly = (*pa)[ip - 1];

        for( int ic = poly->GetNumCorners() - 1; ic>=0; ic-- )
        {
            if( poly->GetUtility( ic ) )
                poly->DeleteCorner( ic, false );
        }
    }

    return 0;
}


// initialize new polyline
// set layer, width, selection box size, starting point, id and pointer
//
// if sel_box = 0, don't create selection elements at all
//
// if polyline is board outline, enter with:
// id.type = ID_BOARD
// id.st = ID_BOARD_OUTLINE
// id.i = 0
// ptr = NULL
//
// if polyline is copper area, enter with:
// id.type = ID_NET;
// id.st = ID_AREA
// id.i = index to area
// ptr = pointer to net
//
void CPolyLine::Start( int layer, int x, int y, int hatch )
{
    m_layer = layer;
    SetHatchStyle( (enum hatch_style) hatch );
    CPolyPt poly_pt( x, y );
    poly_pt.end_contour = false;

    m_CornersList.push_back( poly_pt );
    m_SideStyle.push_back( 0 );
}


// add a corner to unclosed polyline
//
void CPolyLine::AppendCorner( int x, int y, int style, bool bDraw )
{
    UnHatch();
    CPolyPt poly_pt( x, y );
    poly_pt.end_contour = false;

    // add entries for new corner and side
    m_CornersList.push_back( poly_pt );
    m_SideStyle.push_back( style );

    if( m_CornersList.size() > 0 && !m_CornersList[m_CornersList.size() - 1].end_contour )
        m_SideStyle[m_CornersList.size() - 1] = style;

    if( bDraw )
        Hatch();
}


// close last polyline contour
//
void CPolyLine::Close( int style, bool bDraw )
{
    if( GetClosed() )
    {
        wxASSERT( 0 );
    }

    UnHatch();
    m_SideStyle[m_CornersList.size() - 1] = style;
    m_CornersList[m_CornersList.size() - 1].end_contour = true;

    if( bDraw )
        Hatch();
}


// move corner of polyline
//
void CPolyLine::MoveCorner( int ic, int x, int y )
{
    UnHatch();
    m_CornersList[ic].x = x;
    m_CornersList[ic].y = y;
    Hatch();
}


// delete corner and adjust arrays
//
void CPolyLine::DeleteCorner( int ic, bool bDraw )
{
    UnHatch();
    int     icont   = GetContour( ic );
    int     istart  = GetContourStart( icont );
    int     iend    = GetContourEnd( icont );
    bool    bClosed = icont < GetContoursCount() - 1 || GetClosed();

    if( !bClosed )
    {
        // open contour, must be last contour
        m_CornersList.erase( m_CornersList.begin() + ic );

        if( ic != istart )
            m_SideStyle.erase( m_SideStyle.begin() + ic - 1 );
    }
    else
    {
        // closed contour
        m_CornersList.erase( m_CornersList.begin() + ic );
        m_SideStyle.erase( m_SideStyle.begin() + ic );

        if( ic == iend )
            m_CornersList[ic - 1].end_contour = true;
    }

    if( bClosed && GetContourSize( icont ) < 3 )
    {
        // delete the entire contour
        RemoveContour( icont );
    }

    if( bDraw )
        Hatch();
}


/******************************************/
void CPolyLine::RemoveContour( int icont )
/******************************************/

/**
 * Function RemoveContour
 * @param icont = contour number to remove
 * remove a contour only if there is more than 1 contour
 */
{
    UnHatch();
    int istart  = GetContourStart( icont );
    int iend    = GetContourEnd( icont );

    int polycount = GetContoursCount();

    if( icont == 0 && polycount == 1 )
    {
        // remove the only contour
        wxASSERT( 0 );
    }
    else if( icont == polycount - 1 )
    {
        // remove last contour
        m_CornersList.erase( m_CornersList.begin() + istart, m_CornersList.end() );
        m_SideStyle.erase( m_SideStyle.begin() + istart, m_SideStyle.end() );
    }
    else
    {
        // remove closed contour
        for( int ic = iend; ic>=istart; ic-- )
        {
            m_CornersList.erase( m_CornersList.begin() + ic );
            m_SideStyle.erase( m_SideStyle.begin() + ic );
        }
    }

    Hatch();
}


CPolyLine* CPolyLine::Chamfer( unsigned int aDistance )
{
    CPolyLine* newPoly = new CPolyLine;

    if( !aDistance )
    {
        newPoly->Copy( this );
        return newPoly;
    }

    int polycount = GetContoursCount();

    for( int contour = 0; contour < polycount; contour++ )
    {
        unsigned int    startIndex  = GetContourStart( contour );
        unsigned int    endIndex    = GetContourEnd( contour );

        for( unsigned int index = startIndex; index <= endIndex; index++ )
        {
            int         x1, y1, nx, ny;
            long long   xa, ya, xb, yb;

            x1  = m_CornersList[index].x;
            y1  = m_CornersList[index].y;

            if( index == startIndex )
            {
                xa  = m_CornersList[endIndex].x - x1;
                ya  = m_CornersList[endIndex].y - y1;
            }
            else
            {
                xa  = m_CornersList[index - 1].x - x1;
                ya  = m_CornersList[index - 1].y - y1;
            }

            if( index == endIndex )
            {
                xb  = m_CornersList[startIndex].x - x1;
                yb  = m_CornersList[startIndex].y - y1;
            }
            else
            {
                xb  = m_CornersList[index + 1].x - x1;
                yb  = m_CornersList[index + 1].y - y1;
            }

            unsigned int    lena        = (unsigned int) sqrt( (double) (xa * xa + ya * ya) );
            unsigned int    lenb        = (unsigned int) sqrt( (double) (xb * xb + yb * yb) );
            unsigned int    distance    = aDistance;

            // Chamfer one half of an edge at most
            if( 0.5 * lena < distance )
                distance = (unsigned int) (0.5 * (double) lena);

            if( 0.5 * lenb < distance )
                distance = (unsigned int) (0.5 * (double) lenb);

            nx  = (int) ( (double) (distance * xa) / sqrt( (double) (xa * xa + ya * ya) ) );
            ny  = (int) ( (double) (distance * ya) / sqrt( (double) (xa * xa + ya * ya) ) );

            if( index == startIndex )
                newPoly->Start( GetLayer(), x1 + nx, y1 + ny, GetHatchStyle() );
            else
                newPoly->AppendCorner( x1 + nx, y1 + ny );

            nx  = (int) ( (double) (distance * xb) / sqrt( (double) (xb * xb + yb * yb) ) );
            ny  = (int) ( (double) (distance * yb) / sqrt( (double) (xb * xb + yb * yb) ) );
            newPoly->AppendCorner( x1 + nx, y1 + ny );
        }

        newPoly->Close();
    }

    return newPoly;
}


CPolyLine* CPolyLine::Fillet( unsigned int aRadius, unsigned int aSegments )
{
    CPolyLine* newPoly = new CPolyLine;

    if( !aRadius )
    {
        newPoly->Copy( this );
        return newPoly;
    }

    int polycount = GetContoursCount();

    for( int contour = 0; contour < polycount; contour++ )
    {
        unsigned int    startIndex  = GetContourStart( contour );
        unsigned int    endIndex    = GetContourEnd( contour );

        for( unsigned int index = startIndex; index <= endIndex; index++ )
        {
            int         x1, y1; // Current vertex
            long long   xa, ya; // Previous vertex
            long long   xb, yb; // Next vertex
            double      nx, ny;

            x1  = m_CornersList[index].x;
            y1  = m_CornersList[index].y;

            if( index == startIndex )
            {
                xa  = m_CornersList[endIndex].x - x1;
                ya  = m_CornersList[endIndex].y - y1;
            }
            else
            {
                xa  = m_CornersList[index - 1].x - x1;
                ya  = m_CornersList[index - 1].y - y1;
            }

            if( index == endIndex )
            {
                xb  = m_CornersList[startIndex].x - x1;
                yb  = m_CornersList[startIndex].y - y1;
            }
            else
            {
                xb  = m_CornersList[index + 1].x - x1;
                yb  = m_CornersList[index + 1].y - y1;
            }

            double          lena    = sqrt( (double) (xa * xa + ya * ya) );
            double          lenb    = sqrt( (double) (xb * xb + yb * yb) );
            double          cosine  = ( xa * xb + ya * yb ) / ( lena * lenb );

            unsigned int    radius  = aRadius;
            double          denom   = sqrt( 2.0 / ( 1 + cosine ) - 1 );

            // Limit rounding distance to one half of an edge
            if( 0.5 * lena * denom < radius )
                radius = (unsigned int) (0.5 * lena * denom);

            if( 0.5 * lenb * denom < radius )
                radius = (unsigned int) (0.5 * lenb * denom);

            // Calculate fillet arc absolute center point (xc, yx)
            double  k       = radius / sqrt( .5 * ( 1 - cosine ) );
            double  lenab   = sqrt( ( xa / lena + xb / lenb ) * ( xa / lena + xb / lenb ) +
                                    ( ya / lena + yb / lenb ) * ( ya / lena + yb / lenb ) );
            double  xc  = x1 + k * ( xa / lena + xb / lenb ) / lenab;
            double  yc  = y1 + k * ( ya / lena + yb / lenb ) / lenab;

            // Calculate arc start and end vectors
            k = radius / sqrt( 2 / ( 1 + cosine ) - 1 );
            double  xs  = x1 + k * xa / lena - xc;
            double  ys  = y1 + k * ya / lena - yc;
            double  xe  = x1 + k * xb / lenb - xc;
            double  ye  = y1 + k * yb / lenb - yc;

            // Cosine of arc angle
            double  argument = ( xs * xe + ys * ye ) / ( radius * radius );

            if( argument < -1 ) // Just in case...
                argument = -1;
            else if( argument > 1 )
                argument = 1;

            double  arcAngle = acos( argument );

            // Calculate the number of segments
            double  tempSegments = (double) aSegments * ( arcAngle / ( 2 * M_PI ) );

            if( tempSegments - (int) tempSegments > 0 )
                tempSegments++;

            unsigned int    segments = (unsigned int) tempSegments;

            double          deltaAngle  = arcAngle / segments;
            double          startAngle  = atan2( -ys, xs );

            // Flip arc for inner corners
            if( xa * yb - ya * xb <= 0 )
                deltaAngle *= -1;

            nx  = xc + xs + 0.5;
            ny  = yc + ys + 0.5;

            if( index == startIndex )
                newPoly->Start( GetLayer(), (int) nx, (int) ny, GetHatchStyle() );
            else
                newPoly->AppendCorner( (int) nx, (int) ny );

            unsigned int nVertices = 0;

            for( unsigned int j = 0; j < segments; j++ )
            {
                nx  = xc + cos( startAngle + (j + 1) * deltaAngle ) * radius + 0.5;
                ny  = yc - sin( startAngle + (j + 1) * deltaAngle ) * radius + 0.5;
                newPoly->AppendCorner( (int) nx, (int) ny );
                nVertices++;
            }
        }

        newPoly->Close();
    }

    return newPoly;
}


/******************************************/
void CPolyLine::RemoveAllContours( void )
/******************************************/

/**
 * function RemoveAllContours
 * removes all corners from the lists.
 * Others params are not chnaged
 */
{
    m_CornersList.clear();
    m_SideStyle.clear();
}


/**
 * Function InsertCorner
 * insert a new corner between two existing corners
 * @param ic = index for the insertion point: the corner is inserted AFTER ic
 * @param x, y = coordinates corner to insert
 */
void CPolyLine::InsertCorner( int ic, int x, int y )
{
    UnHatch();

    if( (unsigned) (ic) >= m_CornersList.size() )
    {
        m_CornersList.push_back( CPolyPt( x, y ) );
        m_SideStyle.push_back( STRAIGHT );
    }
    else
    {
        m_CornersList.insert( m_CornersList.begin() + ic + 1, CPolyPt( x, y ) );
        m_SideStyle.insert( m_SideStyle.begin() + ic + 1, STRAIGHT );
    }

    if( (unsigned) (ic + 1) < m_CornersList.size() )
    {
        if( m_CornersList[ic].end_contour )
        {
            m_CornersList[ic + 1].end_contour   = true;
            m_CornersList[ic].end_contour       = false;
        }
    }

    Hatch();
}


// undraw polyline by removing all graphic elements from display list
//
void CPolyLine::UnHatch()
{
    m_HatchLines.clear();
}


int CPolyLine::GetEndContour( int ic )
{
    return m_CornersList[ic].end_contour;
}


CRect CPolyLine::GetBounds()
{
    CRect r = GetCornerBounds();

    r.left      -= m_width / 2;
    r.right     += m_width / 2;
    r.bottom    -= m_width / 2;
    r.top       += m_width / 2;
    return r;
}


CRect CPolyLine::GetCornerBounds()
{
    CRect r;

    r.left  = r.bottom = INT_MAX;
    r.right = r.top = INT_MIN;

    for( unsigned i = 0; i<m_CornersList.size(); i++ )
    {
        r.left      = min( r.left, m_CornersList[i].x );
        r.right     = max( r.right, m_CornersList[i].x );
        r.bottom    = min( r.bottom, m_CornersList[i].y );
        r.top       = max( r.top, m_CornersList[i].y );
    }

    return r;
}


CRect CPolyLine::GetCornerBounds( int icont )
{
    CRect r;

    r.left  = r.bottom = INT_MAX;
    r.right = r.top = INT_MIN;
    int istart  = GetContourStart( icont );
    int iend    = GetContourEnd( icont );

    for( int i = istart; i<=iend; i++ )
    {
        r.left      = min( r.left, m_CornersList[i].x );
        r.right     = max( r.right, m_CornersList[i].x );
        r.bottom    = min( r.bottom, m_CornersList[i].y );
        r.top       = max( r.top, m_CornersList[i].y );
    }

    return r;
}


int CPolyLine::GetNumCorners()
{
    return m_CornersList.size();
}


int CPolyLine::GetNumSides()
{
    if( GetClosed() )
        return m_CornersList.size();
    else
        return m_CornersList.size() - 1;
}


int CPolyLine::GetContoursCount()
{
    int ncont = 0;

    if( !m_CornersList.size() )
        return 0;

    for( unsigned ic = 0; ic < m_CornersList.size(); ic++ )
        if( m_CornersList[ic].end_contour )
            ncont++;



    if( !m_CornersList[m_CornersList.size() - 1].end_contour )
        ncont++;

    return ncont;
}


int CPolyLine::GetContour( int ic )
{
    int ncont = 0;

    for( int i = 0; i<ic; i++ )
    {
        if( m_CornersList[i].end_contour )
            ncont++;
    }

    return ncont;
}


int CPolyLine::GetContourStart( int icont )
{
    if( icont == 0 )
        return 0;

    int ncont = 0;

    for( unsigned i = 0; i<m_CornersList.size(); i++ )
    {
        if( m_CornersList[i].end_contour )
        {
            ncont++;

            if( ncont == icont )
                return i + 1;
        }
    }

    wxASSERT( 0 );
    return 0;
}


int CPolyLine::GetContourEnd( int icont )
{
    if( icont < 0 )
        return 0;

    if( icont == GetContoursCount() - 1 )
        return m_CornersList.size() - 1;

    int ncont = 0;

    for( unsigned i = 0; i<m_CornersList.size(); i++ )
    {
        if( m_CornersList[i].end_contour )
        {
            if( ncont == icont )
                return i;

            ncont++;
        }
    }

    wxASSERT( 0 );
    return 0;
}


int CPolyLine::GetContourSize( int icont )
{
    return GetContourEnd( icont ) - GetContourStart( icont ) + 1;
}


void CPolyLine::SetSideStyle( int is, int style )
{
    UnHatch();
    wxPoint p1, p2;

    if( is == (int) (m_CornersList.size() - 1) )
    {
        p1.x    = m_CornersList[m_CornersList.size() - 1].x;
        p1.y    = m_CornersList[m_CornersList.size() - 1].y;
        p2.x    = m_CornersList[0].x;
        p2.y    = m_CornersList[0].y;
    }
    else
    {
        p1.x    = m_CornersList[is].x;
        p1.y    = m_CornersList[is].y;
        p2.x    = m_CornersList[is + 1].x;
        p2.y    = m_CornersList[is + 1].y;
    }

    if( p1.x == p2.x || p1.y == p2.y )
        m_SideStyle[is] = STRAIGHT;
    else
        m_SideStyle[is] = style;

    Hatch();
}


int CPolyLine::GetSideStyle( int is )
{
    return m_SideStyle[is];
}


int CPolyLine::GetClosed()
{
    if( m_CornersList.size() == 0 )
        return 0;
    else
        return m_CornersList[m_CornersList.size() - 1].end_contour;
}


// Creates hatch lines inside the outline of the complex polygon
//
// sort function used in ::Hatch to sort points by descending wxPoint.x values
bool sort_ends_by_descending_X( const wxPoint& ref, const wxPoint& tst )
{
    return tst.x < ref.x;
}


void CPolyLine::Hatch()
{
    m_HatchLines.clear();

    if( m_hatchStyle == NO_HATCH || m_hatchPitch == 0 )
        return;

    if( !GetClosed() ) // If not closed, the poly is beeing created and not finalised. Not not hatch
        return;

    // define range for hatch lines
    int min_x   = m_CornersList[0].x;
    int max_x   = m_CornersList[0].x;
    int min_y   = m_CornersList[0].y;
    int max_y   = m_CornersList[0].y;

    for( unsigned ic = 1; ic < m_CornersList.size(); ic++ )
    {
        if( m_CornersList[ic].x < min_x )
            min_x = m_CornersList[ic].x;

        if( m_CornersList[ic].x > max_x )
            max_x = m_CornersList[ic].x;

        if( m_CornersList[ic].y < min_y )
            min_y = m_CornersList[ic].y;

        if( m_CornersList[ic].y > max_y )
            max_y = m_CornersList[ic].y;
    }

    // Calculate spacing betwwen 2 hatch lines
    int spacing;

    if( m_hatchStyle == DIAGONAL_EDGE )
        spacing = m_hatchPitch;
    else
        spacing = m_hatchPitch * 2;

    // set the "lenght" of hatch lines (the lenght on horizontal axis)
    double  hatch_line_len = m_hatchPitch;

    // To have a better look, give a slope depending on the layer
    int     layer = GetLayer();
    int     slope_flag = (layer & 1) ? 1 : -1;  // 1 or -1
    double  slope = 0.707106 * slope_flag;      // 45 degrees slope
    int     max_a, min_a;

    if( slope_flag == 1 )
    {
        max_a   = (int) (max_y - slope * min_x);
        min_a   = (int) (min_y - slope * max_x);
    }
    else
    {
        max_a   = (int) (max_y - slope * max_x);
        min_a   = (int) (min_y - slope * min_x);
    }

    min_a = (min_a / spacing) * spacing;

    // calculate an offset depending on layer number,
    // for a better look of hatches on a multilayer board
    int offset = (layer * 7) / 8;
    min_a += offset;

    // now calculate and draw hatch lines
    int nc = m_CornersList.size();

    // loop through hatch lines
    #define MAXPTS 200      // Usually we store only few values per one hatch line
                            // depending on the compexity of the zone outline

    static std::vector <wxPoint> pointbuffer;
    pointbuffer.clear();
    pointbuffer.reserve( MAXPTS + 2 );

    for( int a = min_a; a < max_a; a += spacing )
    {
        // get intersection points for this hatch line

        // Note: because we should have an even number of intersections with the
        // current hatch line and the zone outline (a closed polygon,
        // or a set of closed polygons), if an odd count is found
        // we skip this line (should not occur)
        pointbuffer.clear();
        int i_start_contour = 0;

        for( int ic = 0; ic<nc; ic++ )
        {
            double  x, y, x2, y2;
            int     ok;

            if( m_CornersList[ic].end_contour || ( ic == (int) (m_CornersList.size() - 1) ) )
            {
                ok = FindLineSegmentIntersection( a, slope,
                                                  m_CornersList[ic].x, m_CornersList[ic].y,
                                                  m_CornersList[i_start_contour].x,
                                                  m_CornersList[i_start_contour].y,
                                                  m_SideStyle[ic],
                                                  &x, &y, &x2, &y2 );
                i_start_contour = ic + 1;
            }
            else
            {
                ok = FindLineSegmentIntersection( a, slope,
                                                  m_CornersList[ic].x, m_CornersList[ic].y,
                                                  m_CornersList[ic + 1].x, m_CornersList[ic + 1].y,
                                                  m_SideStyle[ic],
                                                  &x, &y, &x2, &y2 );
            }

            if( ok )
            {
                wxPoint point( (int) x, (int) y );
                pointbuffer.push_back( point );
            }

            if( ok == 2 )
            {
                wxPoint point( (int) x2, (int) y2 );
                pointbuffer.push_back( point );
            }

            if( pointbuffer.size() >= MAXPTS )    // overflow
            {
                wxASSERT( 0 );
                break;
            }
        }

        // ensure we have found an even intersection points count
        // because intersections are the ends of segments
        // inside the polygon(s) and a segment has 2 ends.
        // if not, this is a strange case (a bug ?) so skip this hatch
        if( pointbuffer.size() % 2 != 0 )
            continue;

        // sort points in order of descending x (if more than 2) to
        // ensure the starting point and the ending point of the same segment
        // are stored one just after the other.
        if( pointbuffer.size() > 2 )
            sort( pointbuffer.begin(), pointbuffer.end(), sort_ends_by_descending_X );

        // creates lines or short segments inside the complex polygon
        for( unsigned ip = 0; ip < pointbuffer.size(); ip += 2 )
        {
            double dx = pointbuffer[ip + 1].x - pointbuffer[ip].x;

            // Push only one line for diagonal hatch,
            // or for small lines < twice the line len
            // else push 2 small lines
            if( m_hatchStyle == DIAGONAL_FULL || fabs( dx ) < 2 * hatch_line_len )
            {
                m_HatchLines.push_back( CSegment( pointbuffer[ip], pointbuffer[ip + 1] ) );
            }
            else
            {
                double  dy      = pointbuffer[ip + 1].y - pointbuffer[ip].y;
                double  slope   = dy / dx;

                if( dx > 0 )
                    dx = hatch_line_len;
                else
                    dx = -hatch_line_len;

                double  x1  = pointbuffer[ip].x + dx;
                double  x2  = pointbuffer[ip + 1].x - dx;
                double  y1  = pointbuffer[ip].y + dx * slope;
                double  y2  = pointbuffer[ip + 1].y - dx * slope;

                m_HatchLines.push_back( CSegment( pointbuffer[ip].x,
                                                  pointbuffer[ip].y,
                                                  KiROUND( x1 ), KiROUND( y1 ) ) );

                m_HatchLines.push_back( CSegment( pointbuffer[ip + 1].x,
                                                  pointbuffer[ip + 1].y,
                                                  KiROUND( x2 ), KiROUND( y2 ) ) );
            }
        }
    }
}


// test to see if a point is inside polyline
//
bool CPolyLine::TestPointInside( int px, int py )
{
    if( !GetClosed() )
    {
        wxASSERT( 0 );
    }

    // Test all polygons.
    // Since the first is the main outline, and other are holes,
    // if the tested point is inside only one contour, it is inside the whole polygon
    // (in fact inside the main outline, and outside all holes).
    // if inside 2 contours (the main outline + an hole), it is outside the poly.
    int     polycount   = GetContoursCount();
    bool    inside      = false;

    for( int icont = 0; icont < polycount; icont++ )
    {
        int istart  = GetContourStart( icont );
        int iend    = GetContourEnd( icont );

        // Test this polygon:
        if( TestPointInsidePolygon( m_CornersList, istart, iend, px, py ) ) // test point inside the current polygon
            inside = not inside;
    }

    return inside;
}


// copy data from another poly, but don't draw it
//
void CPolyLine::Copy( CPolyLine* src )
{
    UnHatch();
    m_hatchStyle    = src->m_hatchStyle;
    m_hatchPitch    = src->m_hatchPitch;
    // copy corners, using vector copy
    m_CornersList = src->m_CornersList;
    // copy side styles, using vector copy
    m_SideStyle = src->m_SideStyle;
}


/*******************************************/
bool CPolyLine::IsCutoutContour( int icont )
/*******************************************/

/*
 * return true if the corner icont is inside the outline (i.e it is a hole)
 */
{
    int ncont = GetContour( icont );

    if( ncont == 0 ) // the first contour is the main outline, not an hole
        return false;

    return true;
}


void CPolyLine::MoveOrigin( int x_off, int y_off )
{
    UnHatch();

    for( int ic = 0; ic < GetNumCorners(); ic++ )
    {
        SetX( ic, GetX( ic ) + x_off );
        SetY( ic, GetY( ic ) + y_off );
    }

    Hatch();
}


// Set various parameters:
// the calling function should UnHatch() before calling them,
// and Draw() after
//
void CPolyLine::SetX( int ic, int x )
{
    m_CornersList[ic].x = x;
}


void CPolyLine::SetY( int ic, int y )
{
    m_CornersList[ic].y = y;
}


void CPolyLine::SetEndContour( int ic, bool end_contour )
{
    m_CornersList[ic].end_contour = end_contour;
}


void CPolyLine::AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num )
{
    // get radius
    double  r = sqrt( (double) (xi - xc) * (xi - xc) + (double) (yi - yc) * (yi - yc) );

    // get angles of start and finish
    double  th_i    = atan2( (double) (yi - yc), (double) (xi - xc) );
    double  th_f    = atan2( (double) (yf - yc), (double) (xf - xc) );
    double  th_d    = (th_f - th_i) / (num - 1);
    double  theta   = th_i;

    // generate arc
    for( int ic = 0; ic<num; ic++ )
    {
        int x   = KiROUND( xc + r * cos( theta ) );
        int y   = KiROUND( yc + r * sin( theta ) );
        AppendCorner( x, y, STRAIGHT, 0 );
        theta += th_d;
    }

    Close( STRAIGHT );
}


// Bezier Support
void CPolyLine::AppendBezier( int x1, int y1, int x2, int y2, int x3, int y3 )
{
    std::vector<wxPoint> bezier_points;

    bezier_points = Bezier2Poly( x1, y1, x2, y2, x3, y3 );

    for( unsigned int i = 0; i < bezier_points.size(); i++ )
        AppendCorner( bezier_points[i].x, bezier_points[i].y );
}


void CPolyLine::AppendBezier( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 )
{
    std::vector<wxPoint> bezier_points;

    bezier_points = Bezier2Poly( x1, y1, x2, y2, x3, y3, x4, y4 );

    for( unsigned int i = 0; i < bezier_points.size(); i++ )
        AppendCorner( bezier_points[i].x, bezier_points[i].y );
}


/*
 * Function Distance
 * Calculates the distance between a segment and a polygon (with holes):
 * param aStart is the starting point of the segment.
 * param aEnd is the ending point of the segment.
 * param aWidth is the width of the segment.
 * return distance between the segment and outline.
 *               0 if segment intersects or is inside
 */
int CPolyLine::Distance( wxPoint aStart, wxPoint aEnd, int aWidth )
{
    // We calculate the min dist between the segment and each outline segment
    // However, if the segment to test is inside the outline, and does not cross
    // any edge, it can be seen outside the polygon.
    // Therefore test if a segment end is inside ( testing only one end is enough )
    if( TestPointInside( aStart.x, aStart.y ) )
        return 0;

    int distance    = INT_MAX;
    int polycount   = GetContoursCount();

    for( int icont = 0; icont < polycount; icont++ )
    {
        int ic_start    = GetContourStart( icont );
        int ic_end      = GetContourEnd( icont );

        // now test spacing between area outline and segment
        for( int ic2 = ic_start; ic2 <= ic_end; ic2++ )
        {
            int bx1 = GetX( ic2 );
            int by1 = GetY( ic2 );
            int bx2, by2;

            if( ic2 == ic_end )
            {
                bx2 = GetX( ic_start );
                by2 = GetY( ic_start );
            }
            else
            {
                bx2 = GetX( ic2 + 1 );
                by2 = GetY( ic2 + 1 );
            }

            int bstyle = GetSideStyle( ic2 );
            int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle, 0,
                                                 aStart.x, aStart.y, aEnd.x, aEnd.y,
                                                 CPolyLine::STRAIGHT, aWidth,
                                                 1,    // min clearance, should be > 0
                                                 NULL, NULL );

            if( distance > d )
                distance = d;

            if( distance <= 0 )
                return 0;
        }
    }

    return distance;
}


/*
 * Function Distance
 * Calculates the distance between a point and polygon (with holes):
 * param aPoint is the coordinate of the point.
 * return distance between the point and outline.
 *               0 if the point is inside
 */
int CPolyLine::Distance( const wxPoint& aPoint )
{
    // We calculate the dist between the point and each outline segment
    // If the point is inside the outline, the dist is 0.
    if( TestPointInside( aPoint.x, aPoint.y ) )
        return 0;

    int distance    = INT_MAX;
    int polycount   = GetContoursCount();

    for( int icont = 0; icont < polycount; icont++ )
    {
        int ic_start    = GetContourStart( icont );
        int ic_end      = GetContourEnd( icont );

        // now test spacing between area outline and segment
        for( int ic2 = ic_start; ic2 <= ic_end; ic2++ )
        {
            int bx1 = GetX( ic2 );
            int by1 = GetY( ic2 );
            int bx2, by2;

            if( ic2 == ic_end )
            {
                bx2 = GetX( ic_start );
                by2 = GetY( ic_start );
            }
            else
            {
                bx2 = GetX( ic2 + 1 );
                by2 = GetY( ic2 + 1 );
            }

            // Here we expect only straight lines for vertices
            // (no arcs, not yet supported in Pcbnew)
            int d = KiROUND( GetPointToLineSegmentDistance( aPoint.x, aPoint.y,
                                                            bx1, by1, bx2, by2 ) );


            if( distance > d )
                distance = d;

            if( distance <= 0 )
                return 0;
        }
    }

    return distance;
}


/**
 * Function CopyPolysListToKiPolygonWithHole
 * converts the outline contours aPolysList to a KI_POLYGON_WITH_HOLES
 *
 * @param aPolysList = the list of corners of contours
 * @param aPolygoneWithHole = a KI_POLYGON_WITH_HOLES to populate
 */
void CopyPolysListToKiPolygonWithHole( const std::vector<CPolyPt>&  aPolysList,
                                       KI_POLYGON_WITH_HOLES&       aPolygoneWithHole )
{
    unsigned    corners_count = aPolysList.size();

    std::vector<KI_POLY_POINT> cornerslist;
    KI_POLYGON  poly;

    // Enter main outline: this is the first contour
    unsigned    ic = 0;

    while( ic < corners_count )
    {
        const CPolyPt& corner = aPolysList[ic++];
        cornerslist.push_back( KI_POLY_POINT( corner.x, corner.y ) );

        if( corner.end_contour )
            break;
    }

    aPolygoneWithHole.set( cornerslist.begin(), cornerslist.end() );

    // Enter holes: they are next contours (when exist)
    if( ic < corners_count )
    {
        KI_POLYGON_SET holePolyList;

        while( ic < corners_count )
        {
            cornerslist.clear();

            while( ic < corners_count )
            {
                const CPolyPt& corner = aPolysList[ic++];
                cornerslist.push_back( KI_POLY_POINT( corner.x, corner.y ) );

                if( corner.end_contour )
                    break;
            }

            bpl::set_points( poly, cornerslist.begin(), cornerslist.end() );
            holePolyList.push_back( poly );
        }

        aPolygoneWithHole.set_holes( holePolyList.begin(), holePolyList.end() );
    }
}

/**
 * Function ConvertPolysListWithHolesToOnePolygon
 * converts the outline contours aPolysListWithHoles with holes to one polygon
 * with no holes (only one contour)
 * holes are linked to main outlines by overlap segments, to give only one polygon
 *
 * @param aPolysListWithHoles = the list of corners of contours (haing holes
 * @param aOnePolyList = a polygon with no holes
 */
void ConvertPolysListWithHolesToOnePolygon( const std::vector<CPolyPt>&  aPolysListWithHoles,
                                            std::vector<CPolyPt>&  aOnePolyList )
{
    unsigned corners_count = aPolysListWithHoles.size();
    int      polycount = 0;

    for( unsigned ii = 0; ii < corners_count; ii++ )
    {
        const CPolyPt& corner = aPolysListWithHoles[ii];

        if( corner.end_contour )
            polycount++;
    }

    // If polycount<= 1, there is no holes found.
    if( polycount<= 1 )
    {
        aOnePolyList = aPolysListWithHoles;
        return;
    }

    // Holes are found: convert them to only one polygon with overlap segments
    KI_POLYGON_SET polysholes;
    KI_POLYGON_SET mainpoly;
    KI_POLYGON poly_tmp;
    std::vector<KI_POLY_POINT> cornerslist;
    corners_count = aPolysListWithHoles.size();

    unsigned ic    = 0;
    // enter main outline
    while( ic < corners_count )
    {
        const CPolyPt& corner = aPolysListWithHoles[ic++];
        cornerslist.push_back( KI_POLY_POINT( corner.x, corner.y ) );

        if( corner.end_contour )
            break;
    }
    bpl::set_points( poly_tmp, cornerslist.begin(), cornerslist.end() );
    mainpoly.push_back( poly_tmp );

    while( ic < corners_count )
    {
        cornerslist.clear();
        {
            while( ic < corners_count )
            {
                const CPolyPt& corner = aPolysListWithHoles[ic++];
                cornerslist.push_back( KI_POLY_POINT( corner.x, corner.y ) );

                if( corner.end_contour )
                    break;
            }

            bpl::set_points( poly_tmp, cornerslist.begin(), cornerslist.end() );
            polysholes.push_back( poly_tmp );
        }
    }

    mainpoly -= polysholes;

    // copy polygon with no holes to destination
    // We should have only one polygon in list
    wxASSERT( mainpoly.size() != 1 );

    {
        KI_POLYGON& poly_nohole = mainpoly[0];
        CPolyPt   corner( 0, 0, false );

        for( unsigned jj = 0; jj < poly_nohole.size(); jj++ )
        {
            KI_POLY_POINT point = *(poly_nohole.begin() + jj);
            corner.x = point.x();
            corner.y = point.y();
            corner.end_contour = false;
            aOnePolyList.push_back( corner );
        }

        corner.end_contour = true;
        aOnePolyList.pop_back();
        aOnePolyList.push_back( corner );
    }
}
