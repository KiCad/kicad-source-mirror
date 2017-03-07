/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/shape_poly_set.h>
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

    /**
     * Zone hatch styles
     */
    typedef enum HATCH_STYLE { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE } HATCH_STYLE;

    ZONE_CONTAINER( BOARD* parent );

    ZONE_CONTAINER( const ZONE_CONTAINER& aZone );
    ZONE_CONTAINER& operator=( const ZONE_CONTAINER &aOther );

    ~ZONE_CONTAINER();

    /**
     * Function GetPosition
     *
     * Returns a reference to the first corner of the polygon set.
     *
     * \warning The implementation of this function relies on the fact that wxPoint and VECTOR2I
     * have the same layout. If you intend to use the returned reference directly, please note
     * that you are _only_ allowed to use members x and y. Any use on anything that is not one of
     * these members will have undefined behaviour.
     *
     * @return a wxPoint, position of the first point of the outline
     */
    const wxPoint& GetPosition() const override;
    void SetPosition( const wxPoint& aPos ) override {}

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

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList ) override;

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
               const wxPoint&  offset = ZeroOffset ) override;

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
    const EDA_RECT GetBoundingBox() const override;

    int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const override;

    /**
     * Function TestForCopperIslandAndRemoveInsulatedIslands
     * Remove insulated copper islands found in m_FilledPolysList.
     * @param aPcb = the board to analyze
     */
    void TestForCopperIslandAndRemoveInsulatedIslands( BOARD* aPcb );

    /**
     * Function IsOnCopperLayer
     * @return true if this zone is on a copper layer, false if on a technical layer
     */
    bool IsOnCopperLayer() const
    {
        return  IsCopperLayer( GetLayer() );
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

    int GetSelectedCorner() const
    {
        // Transform relative indices to global index
        int globalIndex;
        m_Poly->GetGlobalIndex( *m_CornerSelection, globalIndex );

        return globalIndex;
    }

    void SetSelectedCorner( int aCorner )
    {
        SHAPE_POLY_SET::VERTEX_INDEX selectedCorner;

        // If the global index of the corner is correct, assign it to m_CornerSelection
        if( m_Poly->GetRelativeIndices( aCorner, &selectedCorner ) )
        {
            if( m_CornerSelection == nullptr )
                m_CornerSelection = new SHAPE_POLY_SET::VERTEX_INDEX;

            *m_CornerSelection = selectedCorner;
        }
        else
            throw( std::out_of_range( "aCorner-th vertex does not exist" ) );
    }

    ///
    // Like HitTest but selects the current corner to be operated on
    void SetSelectedCorner( const wxPoint& aPosition );

    int GetLocalFlags() const { return m_localFlgs; }
    void SetLocalFlags( int aFlags ) { m_localFlgs = aFlags; }

    std::vector <SEGMENT>& FillSegments() { return m_FillSegmList; }
    const std::vector <SEGMENT>& FillSegments() const { return m_FillSegmList; }

    SHAPE_POLY_SET* Outline() { return m_Poly; }
    const SHAPE_POLY_SET* Outline() const { return const_cast< SHAPE_POLY_SET* >( m_Poly ); }

    void SetOutline( SHAPE_POLY_SET* aOutline ) { m_Poly = aOutline; }

    /**
     * Function HitTest
     * tests if a point is near an outline edge or a corner of this zone.
     * @param aPosition the wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition ) const override;

    /**
     * Function HitTest
     * tests if a point is inside the zone area, i.e. inside the main outline
     * and outside holes.
     * @param aPosition : the wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTestInsideZone( const wxPoint& aPosition ) const
    {
        return m_Poly->Contains( VECTOR2I( aPosition ), 0 );
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
    void TransformSolidAreasShapesToPolygonSet( SHAPE_POLY_SET& aCornerBuffer,
                                                int             aCircleToSegmentsCount,
                                                double          aCorrectionFactor ) const;
    /**
     * Function BuildFilledSolidAreasPolygons
     * Build the filled solid areas data from real outlines (stored in m_Poly)
     * The solid areas can be more than one on copper layers, and do not have holes
      ( holes are linked by overlapping segments to the main outline)
     * in order to have drawable (and plottable) filled polygons
     * @return true if OK, false if the solid polygons cannot be built
     * @param aPcb: the current board (can be NULL for non copper zones)
     * @param aCornerBuffer: A reference to a buffer to store polygon corners, or NULL
     * if NULL (default:
     * - m_FilledPolysList is used to store solid areas polygons.
     * - on copper layers, tracks and other items shapes of other nets are
     * removed from solid areas
     * if not null:
     * Only the zone outline (with holes, if any) is stored in aOutlineBuffer
     * with holes linked. Therefore only one polygon is created
     *
     * When aOutlineBuffer is not null, his function calls
     * AddClearanceAreasPolygonsToPolysList() to add holes for pads and tracks
     * and other items not in net.
     */
    bool BuildFilledSolidAreasPolygons( BOARD* aPcb, SHAPE_POLY_SET* aOutlineBuffer = NULL );

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
     * _NG version uses SHAPE_POLY_SET instead of Boost.Polygon
     */
    void AddClearanceAreasPolygonsToPolysList( BOARD* aPcb );
    void AddClearanceAreasPolygonsToPolysList_NG( BOARD* aPcb );


     /**
     * Function TransformOutlinesShapeWithClearanceToPolygon
     * Convert the outlines shape to a polygon with no holes
     * inflated (optional) by max( aClearanceValue, the zone clearance)
     * (holes are linked to external outline by overlapping segments)
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aMinClearanceValue = the min clearance around outlines
     * @param aUseNetClearance = true to use a clearance which is the max value between
     *          aMinClearanceValue and the net clearance
     *          false to use aMinClearanceValue only
     * if both aMinClearanceValue = 0 and aUseNetClearance = false: create the zone outline polygon.
     */
    void TransformOutlinesShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                        int aMinClearanceValue,
                                                        bool aUseNetClearance );
    /**
     * Function HitTestForCorner
     * tests if the given wxPoint is near a corner.
     * @param  refPos     is the wxPoint to test.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return bool - true if some corner was found to be closer to refPos than aClearance; false
     *              otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos, SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Function HitTestForCorner
     * tests if the given wxPoint is near a corner.
     * @param  refPos     is the wxPoint to test.
     * @return bool - true if some corner was found to be closer to refPos than aClearance; false
     *              otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos ) const;

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * @param  refPos     is the wxPoint to test.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return bool - true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos, SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * @param  refPos     is the wxPoint to test.
     * @return bool - true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos ) const;

    /** @copydoc BOARD_ITEM::HitTest(const EDA_RECT& aRect,
     *                               bool aContained = true, int aAccuracy ) const
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const override;

    /**
     * Function FillZoneAreasWithSegments
     *  Fill sub areas in a zone with segments with m_ZoneMinThickness width
     * A scan is made line per line, on the whole filled areas, with a step of m_ZoneMinThickness.
     * all intersecting points with the horizontal infinite line and polygons to fill are calculated
     * a list of SEGZONE items is built, line per line
     * @return true if success, false on error
     */
    bool FillZoneAreasWithSegments();

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
    void Move( const wxPoint& offset ) override;

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
    void Rotate( const wxPoint& centre, double angle ) override;

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * (like Mirror() but changes layer)
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre ) override;

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
    wxString GetClass() const override
    {
        return wxT( "ZONE_CONTAINER" );
    }

    /** Access to m_Poly parameters
     */


    int GetNumCorners( void ) const
    {
        return m_Poly->TotalVertices();
    }

    /**
     * Function Iterate
     * returns an iterator to visit all points of the zone's main outline without holes.
     * @return SHAPE_POLY_SET::ITERATOR - an iterator to visit the zone vertices without holes.
     */
    SHAPE_POLY_SET::ITERATOR Iterate()
    {
        return m_Poly->Iterate();
    }

    /**
     * Function IterateWithHoles
     * returns an iterator to visit all points of the zone's main outline with holes.
     * @return SHAPE_POLY_SET::ITERATOR - an iterator to visit the zone vertices with holes.
     */
    SHAPE_POLY_SET::ITERATOR IterateWithHoles()
    {
        return m_Poly->IterateWithHoles();
    }

    /**
     * Function CIterateWithHoles
     * returns an iterator to visit all points of the zone's main outline with holes.
     * @return SHAPE_POLY_SET::ITERATOR - an iterator to visit the zone vertices with holes.
     */
    SHAPE_POLY_SET::CONST_ITERATOR CIterateWithHoles() const
    {
        return m_Poly->CIterateWithHoles();
    }

    void RemoveAllContours( void )
    {
        m_Poly->RemoveAllContours();
    }

    const VECTOR2I& GetCornerPosition( int aCornerIndex ) const
    {
        SHAPE_POLY_SET::VERTEX_INDEX index;

        // Convert global to relative indices
        if( !m_Poly->GetRelativeIndices( aCornerIndex, &index ) )
            throw( std::out_of_range( "aCornerIndex-th vertex does not exist" ) );

        return m_Poly->CVertex( index );
    }

    void SetCornerPosition( int aCornerIndex, wxPoint new_pos )
    {
        SHAPE_POLY_SET::VERTEX_INDEX relativeIndices;

        // Convert global to relative indices
        if( m_Poly->GetRelativeIndices( aCornerIndex, &relativeIndices ) )
        {
            m_Poly->Vertex( relativeIndices ).x = new_pos.x;
            m_Poly->Vertex( relativeIndices ).y = new_pos.y;
        }
        else
            throw( std::out_of_range( "aCornerIndex-th vertex does not exist" ) );
    }

    /**
     * Function NewHole
     * creates a new hole on the zone; i.e., a new contour on the zone's outline.
     */
    void NewHole()
    {
        m_Poly->NewHole();
    }

    /**
     * Function AppendCorner
     * @param position          is the position of the new corner.
     * @param aAllowDuplication is a flag to indicate whether it is allowed to add this corner
     *                          even if it is duplicated.
     */
    void AppendCorner( wxPoint position, bool aAllowDuplication = false )
    {
        if( m_Poly->OutlineCount() == 0 )
            m_Poly->NewOutline();

        m_Poly->Append( position.x, position.y, -1, -1, aAllowDuplication );
    }

    HATCH_STYLE GetHatchStyle() const
    {
        return m_hatchStyle;
    }

    void SetHatchStyle( HATCH_STYLE aStyle )
    {
        m_hatchStyle = aStyle;
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
    const SHAPE_POLY_SET& GetFilledPolysList() const
    {
        return m_FilledPolysList;
    }

   /**
     * Function AddFilledPolysList
     * sets the list of filled polygons.
     */
    void AddFilledPolysList( SHAPE_POLY_SET& aPolysList )
    {
        m_FilledPolysList = aPolysList;
    }

    /**
     * Function GetSmoothedPoly
     * returns a pointer to the corner-smoothed version of
     * m_Poly if it exists, otherwise it returns m_Poly.
     * @return SHAPE_POLY_SET* - pointer to the polygon.
     */
    SHAPE_POLY_SET* GetSmoothedPoly() const
    {
        if( m_smoothedPoly )
            return m_smoothedPoly;
        else
            return m_Poly;
    };

    void SetCornerSmoothingType( int aType ) { m_cornerSmoothingType = aType; };

    int  GetCornerSmoothingType() const { return m_cornerSmoothingType; };

    void SetCornerRadius( unsigned int aRadius );

    unsigned int GetCornerRadius() const { return m_cornerRadius; };

    void AddPolygon( std::vector< wxPoint >& aPolygon );

    void AddFilledPolygon( SHAPE_POLY_SET& aPolygon )
    {
        m_FilledPolysList.Append( aPolygon );
    }

    void AddFillSegments( std::vector< SEGMENT >& aSegments )
    {
        m_FillSegmList.insert( m_FillSegmList.end(), aSegments.begin(), aSegments.end() );
    }

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

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

    /**
     * Hatch related methods
     */

    /**
     * Function GetHatchPitch
     * @return int - the zone hatch pitch in iu.
     */
    int GetHatchPitch() const;

    /**
     * Function GetDefaultHatchPitchMils
     * @return int - the default hatch pitch in mils.
     *
     * \todo This value is hardcoded, but it should be user configurable.
     */
    static int GetDefaultHatchPitchMils() { return 20; }

    /**
     * Function SetHatch
     * sets all hatch parameters for the zone.
     * @param  aHatchStyle   is the style of the hatch, specified as one of HATCH_STYLE possible
     *                       values.
     * @param  aHatchPitch   is the hatch pitch in iu.
     * @param  aRebuildHatch is a flag to indicate whether to re-hatch after having set the
     *                       previous parameters.
     */
    void SetHatch( int aHatchStyle, int aHatchPitch, bool aRebuildHatch );

    /**
     * Function SetHatchPitch
     * sets the hatch pitch parameter for the zone.
     * @param  aPitch is the hatch pitch in iu.
     */
    void SetHatchPitch( int aPitch );

    /**
     * Function UnHatch
     * clears the zone's hatch.
     */
    void   UnHatch();

    /**
     * Function Hatch
     * computes the hatch lines depending on the hatch parameters and stores it in the zone's
     * attribute m_HatchLines.
     */
    void   Hatch();

    const std::vector<SEG>& GetHatchLines() const { return m_HatchLines; }


#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif



private:
    void buildFeatureHoleList( BOARD* aPcb, SHAPE_POLY_SET& aFeatures );

    SHAPE_POLY_SET*       m_Poly;                ///< Outline of the zone.
    SHAPE_POLY_SET*       m_smoothedPoly;        // Corner-smoothed version of m_Poly
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

    /// The index of the corner being moved or nullptr if no corner is selected.
    SHAPE_POLY_SET::VERTEX_INDEX* m_CornerSelection;

    /// Variable used in polygon calculations.
    int                   m_localFlgs;

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
    SHAPE_POLY_SET        m_FilledPolysList;

    HATCH_STYLE           m_hatchStyle;     // hatch style, see enum above
    int                   m_hatchPitch;     // for DIAGONAL_EDGE, distance between 2 hatch lines
    std::vector<SEG>      m_HatchLines;     // hatch lines

    /**
     * Union to handle conversion between references to wxPoint and to VECTOR2I.
     *
     * The function GetPosition(), that returns a reference to a wxPoint, needs some existing
     * wxPoint object that it can point to. The header of this function cannot be changed, as it
     * overrides the function from the base class BOARD_ITEM. This made sense when ZONE_CONTAINER
     * was implemented using the legacy CPolyLine class, that worked with wxPoints. However,
     * m_Poly is now a SHAPE_POLY_SET, whose corners are objects of type VECTOR2I, not wxPoint.
     * Thus, we cannot directly reference the first corner of m_Poly, so a modified version of it
     * that can be read as a wxPoint needs to be handled.
     * Taking advantage of the fact that both wxPoint and VECTOR2I have the same memory layout
     * (two integers: x, y), this union let us convert a reference to a VECTOR2I into a reference
     * to a wxPoint.
     *
     * The idea is the following: in GetPosition(), m_Poly->GetCornerPosition( 0 ) returns a
     * reference to the first corner of the polygon set. If we retrieve its memory direction, we
     * can tell the compiler to cast that pointer to a WX_VECTOR_CONVERTER pointer. We can finally
     * shape that memory layout as a wxPoint picking the wx member of the union.
     *
     * Although this solution is somewhat unstable, as it relies on the fact that the memory
     * layout is exactly the same, it is the best attempt to keep backwards compatibility while
     * using the new SHAPE_POLY_SET.
     */
    typedef union {
        wxPoint wx;
        VECTOR2I vector;
    } WX_VECTOR_CONVERTER;

    // Sanity check: assure that the conversion VECTOR2I->wxPoint using the previous union is
    // correct, making sure that the access for x and y attributes is still safe.
    static_assert(offsetof(wxPoint,x) == offsetof(VECTOR2I,x),
                  "wxPoint::x and VECTOR2I::x have different offsets");

    static_assert(offsetof(wxPoint,y) == offsetof(VECTOR2I,y),
                  "wxPoint::y and VECTOR2I::y have different offsets");

};


#endif  // CLASS_ZONE_H_
