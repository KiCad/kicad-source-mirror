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
#include <math_for_graphics.h>
#include <polygon_test_point_inside.h>

enum m_SideStyle { STRAIGHT };                 // side styles


CPolyLine::CPolyLine()
{
    m_hatchStyle    = NO_HATCH;
    m_hatchPitch    = 0;
    m_layer     = 0;
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
void armBoolEng( Bool_Engine* aBooleng, bool aConvertHoles = false );

/**
 * Function NormalizeWithKbool
 * Use the Kbool Library to clip contours: if outlines are crossing, the self-crossing polygon
 * is converted to non self-crossing polygon by adding extra points at the crossing locations
 * and reordering corners
 * if more than one outside contour are found, extra CPolyLines will be created
 * because copper areas have only one outside contour
 * Therefore, if this results in new CPolyLines, return them as std::vector pa
 * @param aExtraPolyList: pointer on a std::vector<CPolyLine*> to store extra CPolyLines
 * (when after normalization, there is more than one polygon with holes)
 * @return number of external contours, or -1 if error
 */
int CPolyLine::NormalizeWithKbool( std::vector<CPolyLine*>* aExtraPolyList )
{
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
    MakeKboolPoly();

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
            CloseLastContour();
            n_ext_cont++;
        }
        else if( aExtraPolyList )                                   // a new outside contour is found: create a new CPolyLine
        {
            polyline = new CPolyLine;
            polyline->SetLayer( GetLayer() );
            polyline->SetHatchStyle( GetHatchStyle() );
            polyline->SetHatchPitch( GetHatchPitch() );
            aExtraPolyList->push_back( polyline );                  // put it in array
            bool first = true;

            while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )    // read next external contour
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
            polyline->CloseLastContour();
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
                polyline->AppendCorner( x, y );
            }

            polyline->CloseLastContour();
        }
    }

    delete m_Kbool_Poly_Engine;
    m_Kbool_Poly_Engine = NULL;

    // free hole list
    for( unsigned ii = 0; ii < hole_array.size(); ii++ )
        delete (std::vector<int>*)hole_array[ii];

    return n_ext_cont;
}


/**
 * Function AddPolygonsToBoolEng
 * Add a CPolyLine to a kbool engine, preparing a boolean op between polygons
 * @param aBooleng : pointer on a bool engine (handle a set of polygons)
 * @param aGroup : group to fill (aGroup = GROUP_A or GROUP_B) operations are made between GROUP_A and GROUP_B
 */
int CPolyLine::AddPolygonsToBoolEng( Bool_Engine* aBooleng, GroupType aGroup )
{
    int count = 0;

    /* Convert the current polyline contour to a kbool polygon: */
    MakeKboolPoly();

    /* add the resulting kbool set of polygons to the current kcool engine */
    while( m_Kbool_Poly_Engine->StartPolygonGet() )
    {
        if( aBooleng->StartPolygonAdd( GROUP_A ) )
        {
            while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )
            {
                int x = (int) m_Kbool_Poly_Engine->GetPolygonXPoint();
                int y = (int) m_Kbool_Poly_Engine->GetPolygonYPoint();
                aBooleng->AddPoint( x, y );
                count++;
            }

            aBooleng->EndPolygonAdd();
        }
        m_Kbool_Poly_Engine->EndPolygonGet();
    }

    delete m_Kbool_Poly_Engine;
    m_Kbool_Poly_Engine = NULL;

    return count;
}
/**
 * Function MakeKboolPoly
 * fill a kbool engine with a closed polyline contour
 * normalize self-intersecting contours
 * @return error: 0 if Ok, 1 if error
 */
int CPolyLine::MakeKboolPoly()
{
    if( m_Kbool_Poly_Engine )
    {
        delete m_Kbool_Poly_Engine;
        m_Kbool_Poly_Engine = NULL;
    }

    if( !GetClosed() )
        return 1; // error

    int polycount = GetContoursCount();
    int last_contour = polycount - 1;

    for( int icont = 0; icont <= last_contour; icont++ )
    {
        // Fill a kbool engine for this contour,
        // and combine it with previous contours
        Bool_Engine* booleng = new Bool_Engine();
        armBoolEng( booleng, false );

        if( m_Kbool_Poly_Engine )  // a previous contour exists. Put it in new engine
        {
            while( m_Kbool_Poly_Engine->StartPolygonGet() )
            {
                if( booleng->StartPolygonAdd( GROUP_A ) )
                {
                    while( m_Kbool_Poly_Engine->PolygonHasMorePoints() )
                    {
                        int x = (int) m_Kbool_Poly_Engine->GetPolygonXPoint();
                        int y = (int) m_Kbool_Poly_Engine->GetPolygonYPoint();
                        booleng->AddPoint( x, y );
                    }

                    booleng->EndPolygonAdd();
                }
                m_Kbool_Poly_Engine->EndPolygonGet();
            }
        }

        int ic_st   = GetContourStart( icont );
        int ic_end  = GetContourEnd( icont );

        if( !booleng->StartPolygonAdd( GROUP_B ) )
        {
            wxASSERT( 0 );
            return 1;    // error
        }

        // Enter this contour to booleng
        for( int ic = ic_st; ic <= ic_end; ic++ )
        {
            int x1      = m_CornersList[ic].x;
            int y1      = m_CornersList[ic].y;
            booleng->AddPoint( x1, y1 );
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


/**
 * Function NormalizeAreaOutlines
 * Convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s)
 * @param aNewPolygonList = a std::vector<CPolyLine*> reference where to store new CPolyLine
 * needed by the normalization
 * @return the polygon count (always >= 1, becuse there is at lesat one polygon)
 * There are new polygons only if the polygon count  is > 1
 */
int CPolyLine::NormalizeAreaOutlines( std::vector<CPolyLine*>* aNewPolygonList )
{
    return NormalizeWithKbool( aNewPolygonList );
}


/* initialize a contour
 * set layer, hatch style, and starting point
 */
void CPolyLine::Start( int layer, int x, int y, int hatch )
{
    m_layer = layer;
    SetHatchStyle( (enum HATCH_STYLE) hatch );
    CPolyPt poly_pt( x, y );
    poly_pt.end_contour = false;

    m_CornersList.push_back( poly_pt );
}


// add a corner to unclosed polyline
//
void CPolyLine::AppendCorner( int x, int y )
{
    UnHatch();
    CPolyPt poly_pt( x, y );
    poly_pt.end_contour = false;

    // add entries for new corner
    m_CornersList.push_back( poly_pt );
}


// close last polyline contour
//
void CPolyLine::CloseLastContour()
{
    m_CornersList[m_CornersList.size() - 1].end_contour = true;
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
void CPolyLine::DeleteCorner( int ic )
{
    UnHatch();
    int     icont   = GetContour( ic );
    int     iend    = GetContourEnd( icont );
    bool    closed = icont < GetContoursCount() - 1 || GetClosed();

    if( !closed )
    {
        // open contour, must be last contour
        m_CornersList.erase( m_CornersList.begin() + ic );
    }
    else
    {
        // closed contour
        m_CornersList.erase( m_CornersList.begin() + ic );

        if( ic == iend )
            m_CornersList[ic - 1].end_contour = true;
    }

    if( closed && GetContourSize( icont ) < 3 )
    {
        // delete the entire contour
        RemoveContour( icont );
    }
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
    }
    else
    {
        // remove closed contour
        for( int ic = iend; ic>=istart; ic-- )
        {
            m_CornersList.erase( m_CornersList.begin() + ic );
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

        newPoly->CloseLastContour();
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

        newPoly->CloseLastContour();
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
    }
    else
    {
        m_CornersList.insert( m_CornersList.begin() + ic + 1, CPolyPt( x, y ) );
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
                                                  STRAIGHT,
                                                  &x, &y, &x2, &y2 );
                i_start_contour = ic + 1;
            }
            else
            {
                ok = FindLineSegmentIntersection( a, slope,
                                                  m_CornersList[ic].x, m_CornersList[ic].y,
                                                  m_CornersList[ic + 1].x, m_CornersList[ic + 1].y,
                                                  STRAIGHT,
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

/*
 * AppendArc:
 * adds segments to current contour to approximate the given arc
 */
void CPolyLine::AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num )
{
    // get radius
    double  radius = hypot( (double) (xi - xc), (double) (yi - yc) );

    // get angles of start and finish
    double  th_i    = atan2( (double) (yi - yc), (double) (xi - xc) );
    double  th_f    = atan2( (double) (yf - yc), (double) (xf - xc) );
    double  th_d    = (th_f - th_i) / (num - 1);
    double  theta   = th_i;

    // generate arc
    for( int ic = 0; ic < num; ic++ )
    {
        int x   = KiROUND( xc + radius * cos( theta ) );
        int y   = KiROUND( yc + radius * sin( theta ) );
        AppendCorner( x, y );
        theta += th_d;
    }

    CloseLastContour();
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

            int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, STRAIGHT, 0,
                                                 aStart.x, aStart.y, aEnd.x, aEnd.y,
                                                 STRAIGHT, aWidth,
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

/**
 * Function IsPolygonSelfIntersecting
 * Test a CPolyLine for self-intersection of vertex (all contours).
 *
 * @return :
 *  false if no intersecting sides
 *  true if intersecting sides
 * When a CPolyLine is self intersectic, it need to be normalized.
 * (converted to non intersecting polygons)
 */
bool CPolyLine::IsPolygonSelfIntersecting()
{
    // first, check for sides intersecting other sides
    int                n_cont  = GetContoursCount();

    // make bounding rect for each contour
    std::vector<CRect> cr;
    cr.reserve( n_cont );

    for( int icont = 0; icont<n_cont; icont++ )
        cr.push_back( GetCornerBounds( icont ) );

    for( int icont = 0; icont<n_cont; icont++ )
    {
        int is_start = GetContourStart( icont );
        int is_end   = GetContourEnd( icont );

        for( int is = is_start; is<=is_end; is++ )
        {
            int is_prev = is - 1;

            if( is_prev < is_start )
                is_prev = is_end;

            int is_next = is + 1;

            if( is_next > is_end )
                is_next = is_start;

            int x1i   = GetX( is );
            int y1i   = GetY( is );
            int x1f   = GetX( is_next );
            int y1f   = GetY( is_next );

            // check for intersection with any other sides
            for( int icont2 = icont; icont2<n_cont; icont2++ )
            {
                if( cr[icont].left > cr[icont2].right
                    || cr[icont].bottom > cr[icont2].top
                    || cr[icont2].left > cr[icont].right
                    || cr[icont2].bottom > cr[icont].top )
                {
                    // rectangles don't overlap, do nothing
                }
                else
                {
                    int is2_start = GetContourStart( icont2 );
                    int is2_end   = GetContourEnd( icont2 );

                    for( int is2 = is2_start; is2<=is2_end; is2++ )
                    {
                        int is2_prev = is2 - 1;

                        if( is2_prev < is2_start )
                            is2_prev = is2_end;

                        int is2_next = is2 + 1;

                        if( is2_next > is2_end )
                            is2_next = is2_start;

                        if( icont != icont2
                           || ( is2 != is && is2 != is_prev && is2 != is_next &&
                                is != is2_prev && is != is2_next )
                          )
                        {
                            int x2i    = GetX( is2 );
                            int y2i    = GetY( is2 );
                            int x2f    = GetX( is2_next );
                            int y2f    = GetY( is2_next );
                            int ret    = FindSegmentIntersections( x1i, y1i, x1f, y1f,
                                                                   STRAIGHT,
                                                                   x2i, y2i, x2f, y2f,
                                                                   STRAIGHT );
                            if( ret )
                            {
                                // intersection between non-adjacent sides
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}
