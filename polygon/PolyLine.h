/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Few parts of this code come from  FreePCB, released under the GNU General Public License V2.
 * (see http://www.freepcb.com/ )
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


/**
 * @file PolyLine.h
 * @note definition of CPolyLine class
 */

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

#include <wx/gdicmn.h>                          // for wxPoint definition
#include <layers_id_colors_and_visibility.h>    // for LAYER_NUM definition
#include <class_eda_rect.h>                     // for EDA_RECT definition
#include <polygons_defs.h>
#include <clipper.hpp>

class CSegment
{
public:
    wxPoint m_Start;
    wxPoint m_End;

    CSegment() { };
    CSegment( const wxPoint& aStart, const wxPoint& aEnd )
    {
        m_Start = aStart;
        m_End   = aEnd;
    }

    CSegment( int x0, int y0, int x1, int y1 )
    {
        m_Start.x   = x0; m_Start.y = y0;
        m_End.x     = x1; m_End.y = y1;
    }
};

class CPolyPt : public wxPoint
{
public:
    CPolyPt( int aX = 0, int aY = 0, bool aEnd = false, int aUtility = 0 ) :
        wxPoint( aX, aY ), end_contour( aEnd ), m_flags( aUtility )
    {}

    // / Pure copy constructor is here to dis-ambiguate from the
    // / specialized CPolyPt( const wxPoint& ) constructor version below.
    CPolyPt( const CPolyPt& aPt ) :
        wxPoint( aPt.x, aPt.y ), end_contour( aPt.end_contour ), m_flags( aPt.m_flags )
    {}

    CPolyPt( const wxPoint& aPoint ) :
        wxPoint( aPoint ), end_contour( false ), m_flags( 0 )
    {}


    bool    end_contour;
    int     m_flags;

    bool operator ==( const CPolyPt& cpt2 ) const
    { return (x == cpt2.x) && (y == cpt2.y) && (end_contour == cpt2.end_contour); }

    bool operator !=( CPolyPt& cpt2 ) const
    { return (x != cpt2.x) || (y != cpt2.y) || (end_contour != cpt2.end_contour); }
};

/**
 * CPOLYGONS_LIST handle a list of contours (polygons corners).
 * Each corner is a CPolyPt item.
 * The last cornet of each contour has its end_contour member = true
 */
class CPOLYGONS_LIST
{
private:
    std::vector <CPolyPt> m_cornersList;    // array of points for corners
public:
    CPOLYGONS_LIST() {};

    CPolyPt& operator [](int aIdx) {return m_cornersList[aIdx]; }

    // Accessor:
    const std::vector <CPolyPt>& GetList() const {return m_cornersList;}
    int        GetX( int ic ) const { return m_cornersList[ic].x; }
    void       SetX( int ic, int aValue ) { m_cornersList[ic].x = aValue; }
    int        GetY( int ic ) const { return m_cornersList[ic].y; }
    void       SetY( int ic, int aValue ) { m_cornersList[ic].y = aValue; }
    int        GetUtility( int ic ) const { return m_cornersList[ic].m_flags; }
    void       SetFlag( int ic, int aFlag )
    {
        m_cornersList[ic].m_flags = aFlag;
    }

    bool       IsEndContour( int ic ) const
    {
        return m_cornersList[ic].end_contour;
    }

    void        SetEndContour( int ic, bool end_contour )
    {
        m_cornersList[ic].end_contour = end_contour;
    }

    const wxPoint&  GetPos( int ic ) const { return m_cornersList[ic]; }
    const CPolyPt&  GetCorner( int ic ) const { return m_cornersList[ic]; }

    // vector <> methods
    void reserve( int aSize ) { m_cornersList.reserve( aSize ); }


    void RemoveAllContours( void ) { m_cornersList.clear(); }
    CPolyPt& GetLastCorner() { return m_cornersList.back(); }

    unsigned GetCornersCount() const { return m_cornersList.size(); }

    void DeleteCorner( int aIdx )
    {
        m_cornersList.erase( m_cornersList.begin() + aIdx );
    }

    void DeleteCorners( int aIdFirstCorner, int aIdLastCorner )
    {
        m_cornersList.erase( m_cornersList.begin() + aIdFirstCorner,
                             m_cornersList.begin() + aIdLastCorner + 1 );
    }

    void Append( const CPOLYGONS_LIST& aList )
    {
        m_cornersList.insert( m_cornersList.end(),
                              aList.m_cornersList.begin(),
                              aList.m_cornersList.end() );
    }

    void Append( const CPolyPt& aItem )
    {
        m_cornersList.push_back( aItem );
    }

    void Append( const wxPoint& aItem )
    {
        CPolyPt item( aItem );

        m_cornersList.push_back( aItem );
    }

    void InsertCorner( int aPosition, const CPolyPt& aItem )
    {
        m_cornersList.insert( m_cornersList.begin() + aPosition + 1, aItem );
    }

    /**
     * function AddCorner
     * add a corner to the list
     */
    void    AddCorner( const CPolyPt& aCorner )
    {
        m_cornersList.push_back( aCorner );
    }

    /**
     * function CloseLastContour
     * Set the .end_contour member of the last corner in list to true
     */
    void    CloseLastContour()
    {
        if( m_cornersList.size() > 0 )
            m_cornersList.back().end_contour = true;
    }

    /**
     * Function ExportTo
     * Copy all contours to a KI_POLYGON_SET, each contour is exported
     * to a KI_POLYGON
     * @param aPolygons = the KI_POLYGON_SET to populate
     */
    void    ExportTo( KI_POLYGON_SET& aPolygons ) const;

    /**
     * Function ExportTo
     * Copy the contours to a KI_POLYGON_WITH_HOLES
     * The first contour is the main outline, others are holes
     * @param aPolygoneWithHole = the KI_POLYGON_WITH_HOLES to populate
     */
    void    ExportTo( KI_POLYGON_WITH_HOLES& aPolygoneWithHole ) const;

    /**
     * Function ExportTo
     * Copy all contours to a ClipperLib::Paths, each contour is exported
     * to a ClipperLib::Path
     * @param aPolygons = the ClipperLib::Paths to populate
     */
    void    ExportTo( ClipperLib::Paths& aPolygons ) const;

    /**
     * Function ImportFrom
     * Copy all polygons from a KI_POLYGON_SET in list
     * @param aPolygons = the KI_POLYGON_SET to import
     */
    void    ImportFrom( KI_POLYGON_SET& aPolygons );

    /**
     * Function ImportFrom
     * Copy all polygons from a ClipperLib::Paths in list
     * @param aPolygons = the ClipperLib::Paths to import
     */
    void    ImportFrom( ClipperLib::Paths& aPolygons );

    /**
     * Function InflateOutline
     * Inflate the outline stored in m_cornersList.
     * The first polygon is the external outline. It is inflated
     * The other polygons are holes. they are deflated
     * @param aResult = the Inflated outline
     * @param aInflateValue = the Inflate value. when < 0, this is a deflate transform
     * @param aLinkHoles = if true, aResult contains only one polygon,
     * with holes linked by overlapping segments
     */
    void InflateOutline( CPOLYGONS_LIST& aResult, int aInflateValue, bool aLinkHoles );
};

class CPolyLine
{
public:
    enum HATCH_STYLE { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE };    // hatch styles

    // constructors/destructor
    CPolyLine();
    CPolyLine( const CPolyLine& aCPolyLine);
    ~CPolyLine();

    /**
     * Function ImportSettings
     * Copy settings (layer, hatch styles) from aPoly
     * @param aPoly is the CPolyLine to import settings
     */
    void        ImportSettings( const CPolyLine* aPoly );

    // functions for modifying the CPolyLine contours

    /* initialize a contour
     * set layer, hatch style, and starting point
     */
    void        Start( LAYER_NUM layer, int x, int y, int hatch );

    void        AppendCorner( int x, int y );
    void        InsertCorner( int ic, int x, int y );

    /**
     * Function DeleteCorner
     * remove the given corner. if it is the last point of a contour
     * keep the controur closed by modifying the previous corner
     * @param ic = the index of the corner to delete
     */
    void        DeleteCorner( int ic );
    void        MoveCorner( int ic, int x, int y );

    /**
     * function CloseLastContour
     * Set the .end_contour member of the last corner
     *  of the last contour to true
     */
    void        CloseLastContour()
    {
        m_CornersList.CloseLastContour();
    }

    void        RemoveContour( int icont );

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
    bool        IsPolygonSelfIntersecting();

    /**
     * Function Chamfer
     * returns a chamfered version of a polygon.
     * @param aDistance is the chamfering distance.
     * @return CPolyLine* - Pointer to new polygon.
     */
    CPolyLine*  Chamfer( unsigned int aDistance );

    /**
     * Function Fillet
     * returns a filleted version of a polygon.
     * @param aRadius is the fillet radius.
     * @param aSegments is the number of segments / fillet.
     * @return CPolyLine* - Pointer to new polygon.
     */
    CPolyLine*  Fillet( unsigned int aRadius, unsigned int aSegments );

    /**
     * Function RemoveNullSegments
     * Removes corners which create a null segment edge
     * (i.e. when 2 successive corners are at the same location)
     * @return the count of removed corners.
     */
    int         RemoveNullSegments();

    void        RemoveAllContours( void );

    // Remove or create hatch
    void        UnHatch();
    void        Hatch();

    // Transform functions
    void        MoveOrigin( int x_off, int y_off );

    // misc. functions
    /**
     * @return the full bounding box of polygons
     */
    const EDA_RECT GetBoundingBox();

    /**
     * @return the bounding box of a given polygon
     * @param icont = the index of the polygon contour
     * (0 = main contour, 1 ... n = other contours, usually holes)
     */
    const EDA_RECT GetBoundingBox( int icont );

    void        Copy( const CPolyLine* src );
    bool        TestPointInside( int x, int y );

    /**
     * @return true if the corner aCornerIdx is on a hole inside the main outline
     * and false if it is on the main outline
     */
    bool        IsCutoutContour( int aCornerIdx );

    /**
     * Function AppendArc.
     * Adds segments to current contour to approximate the given arc
     */
    void        AppendArc( int xi, int yi, int xf, int yf, int xc, int yc, int num );

    // access functions
    void       SetLayer( LAYER_NUM aLayer ) { m_layer = aLayer; }
    LAYER_NUM  GetLayer() const { return m_layer; }

    int GetCornersCount() const
    {
        return m_CornersList.GetCornersCount();
    }
    int         GetClosed();
    int         GetContoursCount();
    int         GetContour( int ic );
    int         GetContourStart( int icont );
    int         GetContourEnd( int icont );
    int         GetContourSize( int icont );

    int        GetX( int ic ) const { return m_CornersList.GetX( ic ); }
    int        GetY( int ic ) const { return m_CornersList.GetY( ic ); }
    bool       IsEndContour( int ic ) const
    { return m_CornersList.IsEndContour( ic ); }

    const wxPoint& GetPos( int ic ) const { return m_CornersList.GetPos( ic ); }

    int GetEndContour( int ic );

    int        GetUtility( int ic ) const { return m_CornersList.GetUtility( ic ); };
    void       SetUtility( int ic, int aFlag ) { m_CornersList.SetFlag( ic, aFlag ); };

    int        GetHatchPitch() const { return m_hatchPitch; }
    static int GetDefaultHatchPitchMils() { return 20; }    // default hatch pitch value in mils

    enum HATCH_STYLE GetHatchStyle() const { return m_hatchStyle; }
    void       SetHatch( int aHatchStyle, int aHatchPitch, bool aRebuildHatch )
    {
        SetHatchPitch( aHatchPitch );
        m_hatchStyle = (enum HATCH_STYLE) aHatchStyle;

        if( aRebuildHatch )
            Hatch();
    }

    void    SetX( int ic, int x )
    {
        m_CornersList.SetX( ic, x );
    }

    void    SetY( int ic, int y )
    {
        m_CornersList.SetY( ic, y );
    }

    void    SetEndContour( int ic, bool end_contour )
    {
        m_CornersList.SetEndContour( ic, end_contour );
    }

    void       SetHatchStyle( enum HATCH_STYLE style )
    {
        m_hatchStyle = style;
    }

    void       SetHatchPitch( int pitch ) { m_hatchPitch = pitch; }

    /**
     * Function NormalizeAreaOutlines
     * Convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s)
     * @param aNewPolygonList = a std::vector<CPolyLine*> reference where to store new CPolyLine
     * needed by the normalization
     * @return the polygon count (always >= 1, because there is at least one polygon)
     * There are new polygons only if the polygon count  is > 1
     */
    int     NormalizeAreaOutlines( std::vector<CPolyLine*>* aNewPolygonList );

    // Bezier Support
    void    AppendBezier( int x1, int y1, int x2, int y2, int x3, int y3 );
    void    AppendBezier( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 );

    /**
     * Function Distance
     * Calculates the distance between a point and the zone:
     * @param aPoint the coordinate of the point.
     * @return int = distance between the point and outline.
     *               0 if the point is inside
     */
    int     Distance( const wxPoint& aPoint );

    /**
     * Function Distance
     * Calculates the distance between a segment and the zone:
     * @param aStart the starting point of the segment.
     * @param aEnd  the ending point of the segment.
     * @param aWidth  the width of the segment.
     * @return int = distance between the segment and outline.
     *               0 if segment intersects or is inside
     */
    int     Distance( wxPoint aStart, wxPoint aEnd, int aWidth );

    /**
     * Function HitTestForEdge
     * test is the point aPos is near (< aDistMax ) a vertex
     * @param aPos = the reference point
     * @param aDistMax = the max distance between a vertex and the reference point
     * @return int = the index of the first corner of the vertex, or -1 if not found.
     */
    int HitTestForEdge( const wxPoint& aPos, int aDistMax ) const;

    /**
     * Function HitTestForCorner
     * test is the point aPos is near (< aDistMax ) a corner
     * @param aPos = the reference point
     * @param aDistMax = the max distance between a vertex and the corner
     * @return int = the index of corner of the, or -1 if not found.
     */
    int HitTestForCorner( const wxPoint& aPos, int aDistMax ) const;

private:
    LAYER_NUM           m_layer;            // layer to draw on
    enum HATCH_STYLE    m_hatchStyle;       // hatch style, see enum above
    int                 m_hatchPitch;       // for DIAGONAL_EDGE hatched outlines, basic distance between 2 hatch lines
                                            // and the len of eacvh segment
                                            // for DIAGONAL_FULL, the pitch is twice this value
    int                 m_flags;            // a flag used in some calculations
public:
    CPOLYGONS_LIST          m_CornersList;  // array of points for corners
    std::vector <CSegment>  m_HatchLines;   // hatch lines showing the polygon area
};

/**
 * Function ConvertPolysListWithHolesToOnePolygon
 * converts the outline contours aPolysListWithHoles with holes to one polygon
 * with no holes (only one contour)
 * holes are linked to main outlines by overlap segments, to give only one polygon
 *
 * @param aPolysListWithHoles = the list of corners of contours
 *                             (main outline and holes)
 * @param aOnePolyList = a polygon with no holes
 */
void    ConvertPolysListWithHolesToOnePolygon( const CPOLYGONS_LIST&    aPolysListWithHoles,
                                               CPOLYGONS_LIST&          aOnePolyList );

/**
 * Function ConvertOnePolygonToPolysListWithHoles
 * converts the outline aOnePolyList (only one contour,
 * holes are linked by overlapping segments) to
 * to one main polygon and holes (polygons inside main polygon)
 * @param aOnePolyList = a polygon with no holes
 * @param aPolysListWithHoles = the list of corners of contours
 *                             (main outline and holes)
 */
void    ConvertOnePolygonToPolysListWithHoles( const CPOLYGONS_LIST&    aOnePolyList,
                                               CPOLYGONS_LIST&          aPolysListWithHoles );


#endif    // #ifndef POLYLINE_H
