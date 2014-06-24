
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_zone.h
 * @brief Classes to handle copper zones
 */

#ifndef CLASS_ZONE_H_
#define CLASS_ZONE_H_


#include <vector>
#include <gr_basic.h>
#include <class_board_item.h>
#include <class_board_connected_item.h>
#include <layers_id_colors_and_visibility.h>
#include <PolyLine.h>
#include <class_zone_settings.h>


class EDA_RECT;
class LINE_READER;
class EDA_DRAW_PANEL;
class PCB_EDIT_FRAME;
class BOARD;
class ZONE_CONTAINER;
class MSG_PANEL_ITEM;


/**
 * Struct SEGMENT
 * is a simple container used when filling areas with segments
 */
struct SEGMENT
{
    wxPoint m_Start;        // starting point of a segment
    wxPoint m_End;          // ending point of a segment

    SEGMENT() {}

    SEGMENT( const wxPoint& aStart, const wxPoint& aEnd )
    {
        m_Start = aStart;
        m_End = aEnd;
    }
};


/**
 * Class ZONE_CONTAINER
 * handles a list of polygons defining a copper zone.
 * A zone is described by a main polygon, a time stamp, a layer, and a net name.
 * Other polygons inside the main polygon are holes in the zone.
 */
class ZONE_CONTAINER : public BOARD_CONNECTED_ITEM
{
public:

    ZONE_CONTAINER( BOARD* parent );

    ZONE_CONTAINER( const ZONE_CONTAINER& aZone );

    ~ZONE_CONTAINER();

    /**
     * Function GetPosition
     * @return a wxPoint, position of the first point of the outline
     */
    const wxPoint& GetPosition() const;             // was overload
    void SetPosition( const wxPoint& aPos )     {}  // was overload

    /**
     * Function SetPriority
     * @param aPriority = the priority level
     */
    void SetPriority( unsigned aPriority ) { m_priority = aPriority; }

    /**
     * Function GetPriority
     * @return the priority level of this zone
     */
    unsigned GetPriority() const { return m_priority; }

    /**
     * Function copy
     * copy useful data from the source.
     * flags and linked list pointers are NOT copied
     */
    void Copy( ZONE_CONTAINER* src );

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    /**
     * Function Draw
     * Draws the zone outline.
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     * @param offset = Draw offset (usually wxPoint(0,0))
     */
    void Draw( EDA_DRAW_PANEL* panel,
               wxDC*           DC,
               GR_DRAWMODE     aDrawMode,
               const wxPoint&  offset = ZeroOffset );

    /**
     * Function DrawDrawFilledArea
     * Draws the filled  area for this zone (polygon list .m_FilledPolysList)
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param offset = Draw offset (usually wxPoint(0,0))
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     */
    void DrawFilledArea( EDA_DRAW_PANEL* panel,
                         wxDC*           DC,
                         GR_DRAWMODE     aDrawMode,
                         const wxPoint&  offset = ZeroOffset );

    /**
     * Function DrawWhileCreateOutline
     * Draws the zone outline when it is created.
     * The moving edges are in XOR graphic mode, old segment in draw_mode graphic mode
     * (usually GR_OR).  The closing edge has its own shape.
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param draw_mode = draw mode: OR, XOR ..
     */
    void DrawWhileCreateOutline( EDA_DRAW_PANEL* panel, wxDC* DC,
                                 GR_DRAWMODE draw_mode = GR_OR );

    /** Function GetBoundingBox (virtual)
     * @return an EDA_RECT that is the bounding box of the zone outline
     */
    const EDA_RECT GetBoundingBox() const;

    int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const;

    /**
     * Function TestForCopperIslandAndRemoveInsulatedIslands
     * Remove insulated copper islands found in m_FilledPolysList.
     * @param aPcb = the board to analyze
     */
    void TestForCopperIslandAndRemoveInsulatedIslands( BOARD* aPcb );

    /**
     * Function CalculateSubAreaBoundaryBox
     * Calculates the bounding box of a a filled area ( list of CPolyPt )
     * use m_FilledPolysList as list of CPolyPt (that are the corners of one or more
     * polygons or filled areas )
     * @return an EDA_RECT as bounding box
     * @param aIndexStart = index of the first corner of a polygon (filled area)
     *                      in m_FilledPolysList
     * @param aIndexEnd = index of the last corner of a polygon in m_FilledPolysList
     */
    EDA_RECT CalculateSubAreaBoundaryBox( int aIndexStart, int aIndexEnd );

    /**
     * Function IsOnCopperLayer
     * @return true if this zone is on a copper layer, false if on a technical layer
     */
    bool IsOnCopperLayer() const
    {
        return  LSET::AllNonCuMask()[GetLayer()];
    }

    /// How to fill areas: 0 = use filled polygons, 1 => fill with segments.
    void SetFillMode( int aFillMode )                   { m_FillMode = aFillMode; }
    int GetFillMode() const                             { return m_FillMode; }

    void SetThermalReliefGap( int aThermalReliefGap )   { m_ThermalReliefGap = aThermalReliefGap; }
    int GetThermalReliefGap( D_PAD* aPad = NULL ) const;

    void SetThermalReliefCopperBridge( int aThermalReliefCopperBridge )
    {
        m_ThermalReliefCopperBridge = aThermalReliefCopperBridge;
    }
    int GetThermalReliefCopperBridge( D_PAD* aPad = NULL ) const;

    void SetArcSegmentCount( int aArcSegCount ) { m_ArcToSegmentsCount = aArcSegCount; }
    int GetArcSegmentCount() const { return m_ArcToSegmentsCount; }

    bool IsFilled() const { return m_IsFilled; }
    void SetIsFilled( bool isFilled ) { m_IsFilled = isFilled; }

    int GetZoneClearance() const { return m_ZoneClearance; }
    void SetZoneClearance( int aZoneClearance ) { m_ZoneClearance = aZoneClearance; }

    ZoneConnection GetPadConnection( D_PAD* aPad = NULL ) const;
    void SetPadConnection( ZoneConnection aPadConnection ) { m_PadConnection = aPadConnection; }

    int GetMinThickness() const { return m_ZoneMinThickness; }
    void SetMinThickness( int aMinThickness ) { m_ZoneMinThickness = aMinThickness; }

    int GetSelectedCorner() const { return m_CornerSelection; }
    void SetSelectedCorner( int aCorner ) { m_CornerSelection = aCorner; }

    ///
    // Like HitTest but selects the current corner to be operated on
    void SetSelectedCorner( const wxPoint& aPosition );

    int GetLocalFlags() const { return m_localFlgs; }
    void SetLocalFlags( int aFlags ) { m_localFlgs = aFlags; }

    std::vector <SEGMENT>& FillSegments() { return m_FillSegmList; }
    const std::vector <SEGMENT>& FillSegments() const { return m_FillSegmList; }

    CPolyLine* Outline() { return m_Poly; }
    const CPolyLine* Outline() const { return const_cast< CPolyLine* >( m_Poly ); }

    void SetOutline( CPolyLine* aOutline ) { m_Poly = aOutline; }

    /**
     * Function HitTest
     * tests if a point is near an outline edge or a corner of this zone.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition ) const;

    /**
     * Function HitTest
     * tests if a point is inside the zone area, i.e. inside the main outline
     * and outside holes.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTestInsideZone( const wxPoint& aPosition ) const
    {
        return m_Poly->TestPointInside( aPosition.x, aPosition.y );
    }

    /**
     * Function HitTestFilledArea
     * tests if the given wxPoint is within the bounds of a filled area of this zone.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTestFilledArea( const wxPoint& aRefPos ) const;

     /**
     * Function TransformSolidAreasShapesToPolygonSet
     * Convert solid areas full shapes to polygon set
     * (the full shape is the polygon area with a thick outline)
     * Used in 3D view
     * Arcs (ends of segments) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygons
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to arcs radius to roughly
     * keep arc radius when approximated by segments
     */
    void TransformSolidAreasShapesToPolygonSet( CPOLYGONS_LIST& aCornerBuffer,
                                                int                    aCircleToSegmentsCount,
                                                double                 aCorrectionFactor );
    /**
     * Function BuildFilledSolidAreasPolygons
     * Build the filled solid areas data from real outlines (stored in m_Poly)
     * The solid areas can be more thna one on copper layers, and do not have holes
      ( holes are linked by overlapping segments to the main outline)
     * in order to have drawable (and plottable) filled polygons
     * @param aPcb: the current board (can be NULL for non copper zones)
     * @param aCornerBuffer: A reference to a buffer to store polygon corners, or NULL
     * if NULL (default:
     * - m_FilledPolysList is used to store solid areas polygons.
     * - on copper layers, tracks and other items shapes of other nets are
     * removed from solid areas
     * if not null:
     * Only the zone outline (with holes, if any) are stored in aCornerBuffer
     * with holes linked. Therfore only one polygon is created
     * @return true if OK, false if the solid areas cannot be calculated
     * This function calls AddClearanceAreasPolygonsToPolysList()
     * to add holes for pads and tracks and other items not in net.
     */
    bool BuildFilledSolidAreasPolygons( BOARD* aPcb, CPOLYGONS_LIST* aCornerBuffer = NULL );

    /**
     * Function CopyPolygonsFromKiPolygonListToFilledPolysList
     * Copy polygons stored in aKiPolyList to m_FilledPolysList
     * The previous m_FilledPolysList contents is replaced.
     * @param aKiPolyList = a KI_POLYGON_SET containing polygons.
     */
    void CopyPolygonsFromKiPolygonListToFilledPolysList( KI_POLYGON_SET& aKiPolyList );

    /**
     * Function CopyPolygonsFromFilledPolysListToKiPolygonList
     * Copy polygons from m_FilledPolysList to aKiPolyList
     * @param aKiPolyList = a KI_POLYGON_SET to fill by polygons.
     */
    void CopyPolygonsFromFilledPolysListToKiPolygonList( KI_POLYGON_SET& aKiPolyList );

    /**
     * Function AddClearanceAreasPolygonsToPolysList
     * Add non copper areas polygons (pads and tracks with clearance)
     * to a filled copper area
     * used in BuildFilledSolidAreasPolygons when calculating filled areas in a zone
     * Non copper areas are pads and track and their clearance area
     * The filled copper area must be computed before
     * BuildFilledSolidAreasPolygons() call this function just after creating the
     *  filled copper area polygon (without clearance areas
     * @param aPcb: the current board
     */
    void AddClearanceAreasPolygonsToPolysList( BOARD* aPcb );


     /**
     * Function TransformOutlinesShapeWithClearanceToPolygon
     * Convert the outlines shape to a polygon with no holes
     * inflated (optional) by max( aClearanceValue, the zone clearance)
     * (holes are linked to external outline by overlapping segments)
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aAddClearance = true to add a clearance area to the polygon
     *                      false to create the outline polygon.
     */
    void TransformOutlinesShapeWithClearanceToPolygon( CPOLYGONS_LIST& aCornerBuffer,
                                               int                    aClearanceValue,
                                               bool                   aAddClearance );
    /**
     * Function HitTestForCorner
     * tests if the given wxPoint near a corner
     * Set m_CornerSelection to -1 if nothing found, or index of corner
     * @return true if found
     * @param refPos : A wxPoint to test
     */
    int HitTestForCorner( const wxPoint& refPos ) const;

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * Set m_CornerSelection to -1 if nothing found, or index of the starting corner of vertice
     * @return true if found
     * @param refPos : A wxPoint to test
     */
    int HitTestForEdge( const wxPoint& refPos ) const;

    /** @copydoc BOARD_ITEM::HitTest(const EDA_RECT& aRect,
     *                               bool aContained = true, int aAccuracy ) const
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const;

    /**
     * Function FillZoneAreasWithSegments
     *  Fill sub areas in a zone with segments with m_ZoneMinThickness width
     * A scan is made line per line, on the whole filled areas, with a step of m_ZoneMinThickness.
     * all intersecting points with the horizontal infinite line and polygons to fill are calculated
     * a list of SEGZONE items is built, line per line
     * @return number of segments created
     */
    int FillZoneAreasWithSegments();

    /**
     * Function UnFill
     * Removes the zone filling
     * @return true if a previous filling is removed, false if no change
     * (when no filling found)
     */
    bool UnFill();

    /* Geometric transformations: */

    /**
     * Function Move
     * Move the outlines
     * @param offset = moving vector
     */
    void Move( const wxPoint& offset );

    /**
     * Function MoveEdge
     * Move the outline Edge
     * @param offset = moving vector
     * @param aEdge = start point of the outline edge
     */
    void MoveEdge( const wxPoint& offset, int aEdge );

    /**
     * Function Rotate
     * Move the outlines
     * @param centre = rot centre
     * @param angle = in 0.1 degree
     */
    void Rotate( const wxPoint& centre, double angle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * (like Mirror() but changes layer)
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    /**
     * Function Mirror
     * Mirror the outlines , relative to a given horizontal axis
     * the layer is not changed
     * @param mirror_ref = vertical axis position
     */
    void Mirror( const wxPoint& mirror_ref );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "ZONE_CONTAINER" );
    }

    /** Access to m_Poly parameters
     */


    int GetNumCorners( void ) const
    {
        return m_Poly->GetCornersCount();
    }

    void RemoveAllContours( void )
    {
        m_Poly->RemoveAllContours();
    }

    const wxPoint& GetCornerPosition( int aCornerIndex ) const
    {
        return m_Poly->GetPos( aCornerIndex );
    }

    void SetCornerPosition( int aCornerIndex, wxPoint new_pos )
    {
        m_Poly->SetX( aCornerIndex, new_pos.x );
        m_Poly->SetY( aCornerIndex, new_pos.y );
    }

    void AppendCorner( wxPoint position )
    {
        m_Poly->AppendCorner( position.x, position.y );
    }

    int GetHatchStyle() const
    {
        return m_Poly->GetHatchStyle();
    }

    void SetHatchStyle( CPolyLine::HATCH_STYLE aStyle )
    {
        m_Poly->SetHatchStyle( aStyle );
    }

    /**
     * Function IsSame
     * tests if 2 zones are equivalent:
     * 2 zones are equivalent if they have same parameters and same outlines
     * info, filling is not taken into account
     * @param aZoneToCompare = zone to compare with "this"
     */
    bool IsSame( const ZONE_CONTAINER &aZoneToCompare );

   /**
     * Function ClearFilledPolysList
     * clears the list of filled polygons.
     */
    void ClearFilledPolysList()
    {
        m_FilledPolysList.RemoveAllContours();
    }

   /**
     * Function GetFilledPolysList
     * returns a reference to the list of filled polygons.
     * @return Reference to the list of filled polygons.
     */
    const CPOLYGONS_LIST& GetFilledPolysList() const
    {
        return m_FilledPolysList;
    }

   /**
     * Function AddFilledPolysList
     * sets the list of filled polygons.
     */
    void AddFilledPolysList( CPOLYGONS_LIST& aPolysList )
    {
        m_FilledPolysList = aPolysList;
    }

    /**
     * Function GetSmoothedPoly
     * returns a pointer to the corner-smoothed version of
     * m_Poly if it exists, otherwise it returns m_Poly.
     * @return CPolyLine* - pointer to the polygon.
     */
    CPolyLine* GetSmoothedPoly() const
    {
        if( m_smoothedPoly )
            return m_smoothedPoly;
        else
            return m_Poly;
    };

    void SetCornerSmoothingType( int aType ) { m_cornerSmoothingType = aType; };

    int  GetCornerSmoothingType() const { return m_cornerSmoothingType; };

    void SetCornerRadius( unsigned int aRadius )
    {
        m_cornerRadius = aRadius;
        if( m_cornerRadius > (unsigned int) Mils2iu( MAX_ZONE_CORNER_RADIUS_MILS ) )
            m_cornerRadius = Mils2iu( MAX_ZONE_CORNER_RADIUS_MILS );
    };

    unsigned int GetCornerRadius() const { return m_cornerRadius; };

    void AddPolygon( std::vector< wxPoint >& aPolygon );

    void AddFilledPolygon( CPOLYGONS_LIST& aPolygon )
    {
        m_FilledPolysList.Append( aPolygon );
    }

    void AddFillSegments( std::vector< SEGMENT >& aSegments )
    {
        m_FillSegmList.insert( m_FillSegmList.end(), aSegments.begin(), aSegments.end() );
    }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_zone_xpm; }

    virtual EDA_ITEM* Clone() const;

    /**
     * Accessors to parameters used in Keepout zones:
     */
    bool GetIsKeepout() const { return m_isKeepout; }
    bool GetDoNotAllowCopperPour() const { return m_doNotAllowCopperPour; }
    bool GetDoNotAllowVias() const { return m_doNotAllowVias; }
    bool GetDoNotAllowTracks() const { return m_doNotAllowTracks; }

    void SetIsKeepout( bool aEnable ) { m_isKeepout = aEnable; }
    void SetDoNotAllowCopperPour( bool aEnable ) { m_doNotAllowCopperPour = aEnable; }
    void SetDoNotAllowVias( bool aEnable ) { m_doNotAllowVias = aEnable; }
    void SetDoNotAllowTracks( bool aEnable ) { m_doNotAllowTracks = aEnable; }


#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); }    // override
#endif


private:
    CPolyLine*            m_Poly;                ///< Outline of the zone.
    CPolyLine*            m_smoothedPoly;        // Corner-smoothed version of m_Poly
    int                   m_cornerSmoothingType;
    unsigned int          m_cornerRadius;

    /* Priority: when a zone outline is inside and other zone, if its priority is higher
     * the other zone priority, it will be created inside.
     * if priorities are equal, a DRC error is set
     */
    unsigned              m_priority;

    /* A zone outline can be a keepout zone.
     * It will be never filled, and DRC should test for pads, tracks and vias
     */
    bool                  m_isKeepout;

    /* For keepout zones only:
     * what is not allowed inside the keepout ( pads, tracks and vias )
     */
    bool                  m_doNotAllowCopperPour;
    bool                  m_doNotAllowVias;
    bool                  m_doNotAllowTracks;

    ZoneConnection        m_PadConnection;
    int                   m_ZoneClearance;           ///< Clearance value in internal units.
    int                   m_ZoneMinThickness;        ///< Minimum thickness value in filled areas.

    /** The number of segments to convert a circle to a polygon.  Valid values are
        #ARC_APPROX_SEGMENTS_COUNT_LOW_DEF or #ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF. */
    int                   m_ArcToSegmentsCount;

    /** True when a zone was filled, false after deleting the filled areas. */
    bool                  m_IsFilled;

    ///< Width of the gap in thermal reliefs.
    int                   m_ThermalReliefGap;

    ///< Width of the copper bridge in thermal reliefs.
    int                   m_ThermalReliefCopperBridge;


    /// How to fill areas: 0 => use filled polygons, 1 => fill with segments.
    int                   m_FillMode;

    /// The index of the corner being moved or -1 if no corner is selected.
    int                   m_CornerSelection;

    int                   m_localFlgs;                ///< Flags used in polygon calculations.


    /** Segments used to fill the zone (#m_FillMode ==1 ), when fill zone by segment is used.
     *  In this case the segments have #m_ZoneMinThickness width.
     */
    std::vector <SEGMENT> m_FillSegmList;

    /* set of filled polygons used to draw a zone as a filled area.
     * from outlines (m_Poly) but unlike m_Poly these filled polygons have no hole
     * (they are all in one piece)  In very simple cases m_FilledPolysList is same
     * as m_Poly.  In less simple cases (when m_Poly has holes) m_FilledPolysList is



     * a polygon equivalent to m_Poly, without holes but with extra outline segment
     * connecting "holes" with external main outline.  In complex cases an outline
     * described by m_Poly can have many filled areas
     */
    CPOLYGONS_LIST m_FilledPolysList;
};


#endif  // CLASS_ZONE_H_
