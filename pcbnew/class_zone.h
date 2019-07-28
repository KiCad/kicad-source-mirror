/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#ifndef CLASS_ZONE_H_
#define CLASS_ZONE_H_


#include <vector>
#include <gr_basic.h>
#include <class_board_item.h>
#include <board_connected_item.h>
#include <layers_id_colors_and_visibility.h>
#include <geometry/shape_poly_set.h>
#include <zone_settings.h>


class EDA_RECT;
class LINE_READER;
class PCB_EDIT_FRAME;
class BOARD;
class ZONE_CONTAINER;
class MSG_PANEL_ITEM;

typedef std::vector<SEG> ZONE_SEGMENT_FILL;

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

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_ZONE_AREA_T;
    }

    /**
     * @return a wxPoint, position of the first point of the outline
     */
    const wxPoint GetPosition() const override;
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

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

    void SetLayerSet( LSET aLayerSet );

    virtual LSET GetLayerSet() const override;

    /**
     * Function Print
     * Prints the zone outline.
     * @param aFrame = current Frame
     * @param DC = current Device Context
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     * @param offset = Draw offset (usually wxPoint(0,0))
     */
    void Print( PCB_BASE_FRAME* aFrame, wxDC* DC, const wxPoint&  offset = ZeroOffset ) override;

    /**
     * Function PrintFilledArea
     * Draws the filled  area for this zone (polygon list .m_FilledPolysList)
     * @param aFrame = current Frame
     * @param DC = current Device Context
     * @param offset = Draw offset (usually wxPoint(0,0))
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     */
    void PrintFilledArea( PCB_BASE_FRAME* aFrame, wxDC* DC, const wxPoint&  offset = ZeroOffset );

    /** Function GetBoundingBox (virtual)
     * @return an EDA_RECT that is the bounding box of the zone outline
     */
    const EDA_RECT GetBoundingBox() const override;

    int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const override;

    /**
     * Function IsOnCopperLayer
     * @return true if this zone is on a copper layer, false if on a technical layer
     */
    bool IsOnCopperLayer() const override;

    /**
     * Function CommonLayerExist
     * Test if this zone shares a common layer with the given layer set
     */
    bool CommonLayerExists( const LSET aLayerSet ) const;

    virtual void SetLayer( PCB_LAYER_ID aLayer ) override;

    virtual PCB_LAYER_ID GetLayer() const override;

    virtual bool IsOnLayer( PCB_LAYER_ID ) const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    void SetFillMode( ZONE_FILL_MODE aFillMode ) { m_FillMode = aFillMode; }
    ZONE_FILL_MODE GetFillMode() const { return m_FillMode; }

    void SetThermalReliefGap( int aThermalReliefGap )   { m_ThermalReliefGap = aThermalReliefGap; }
    int GetThermalReliefGap( D_PAD* aPad = NULL ) const;

    void SetThermalReliefCopperBridge( int aThermalReliefCopperBridge )
    {
        if( m_ThermalReliefCopperBridge != aThermalReliefCopperBridge )
            SetNeedRefill( true );

        m_ThermalReliefCopperBridge = aThermalReliefCopperBridge;
    }
    int GetThermalReliefCopperBridge( D_PAD* aPad = NULL ) const;

    bool IsFilled() const { return m_IsFilled; }
    void SetIsFilled( bool isFilled ) { m_IsFilled = isFilled; }

    bool NeedRefill() const { return m_needRefill; }
    void SetNeedRefill( bool aNeedRefill ) { m_needRefill = aNeedRefill; }

    int GetZoneClearance() const { return m_ZoneClearance; }
    void SetZoneClearance( int aZoneClearance ) { m_ZoneClearance = aZoneClearance; }

    ZoneConnection GetPadConnection( D_PAD* aPad = NULL ) const;
    void SetPadConnection( ZoneConnection aPadConnection ) { m_PadConnection = aPadConnection; }

    int GetMinThickness() const { return m_ZoneMinThickness; }
    void SetMinThickness( int aMinThickness )
    {
        if( m_ZoneMinThickness != aMinThickness )
            SetNeedRefill( true );

        m_ZoneMinThickness = aMinThickness;
    }

    int GetHatchFillTypeThickness() const { return m_HatchFillTypeThickness; }
    void SetHatchFillTypeThickness( int aThickness ) { m_HatchFillTypeThickness = aThickness; }

    int GetHatchFillTypeGap() const { return m_HatchFillTypeGap; }
    void SetHatchFillTypeGap( int aStep ) { m_HatchFillTypeGap = aStep; }

    double GetHatchFillTypeOrientation() const { return m_HatchFillTypeOrientation; }
    void SetHatchFillTypeOrientation( double aStep ) { m_HatchFillTypeOrientation = aStep; }

    int GetHatchFillTypeSmoothingLevel() const { return m_HatchFillTypeSmoothingLevel; }
    void SetHatchFillTypeSmoothingLevel( int aLevel ) { m_HatchFillTypeSmoothingLevel = aLevel; }

    double GetHatchFillTypeSmoothingValue() const { return m_HatchFillTypeSmoothingValue; }
    void SetHatchFillTypeSmoothingValue( double aValue ) { m_HatchFillTypeSmoothingValue = aValue; }

    int GetSelectedCorner() const
    {
        // Transform relative indices to global index
        int globalIndex = -1;

        if( m_CornerSelection )
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
    void SetSelectedCorner( const wxPoint& aPosition, int aAccuracy );

    int GetLocalFlags() const { return m_localFlgs; }
    void SetLocalFlags( int aFlags ) { m_localFlgs = aFlags; }

    ZONE_SEGMENT_FILL& FillSegments() { return m_FillSegmList; }
    const ZONE_SEGMENT_FILL& FillSegments() const { return m_FillSegmList; }

    SHAPE_POLY_SET* Outline() { return m_Poly; }
    const SHAPE_POLY_SET* Outline() const { return const_cast< SHAPE_POLY_SET* >( m_Poly ); }

    void SetOutline( SHAPE_POLY_SET* aOutline ) { m_Poly = aOutline; }

    /**
     * Function HitTest
     * tests if a point is near an outline edge or a corner of this zone.
     * @param aPosition the wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Function HitTestFilledArea
     * tests if the given wxPoint is within the bounds of a filled area of this zone.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTestFilledArea( const wxPoint& aRefPos ) const;

    /**
     * Some intersecting zones, despite being on the same layer with the same net, cannot be
     * merged due to other parameters such as fillet radius.  The copper pour will end up
     * effectively merged though, so we want to keep the corners of such intersections sharp.
     */
    void GetColinearCorners( BOARD* aBoard, std::set<VECTOR2I>& colinearCorners );

    /**
     * Function TransformSolidAreasShapesToPolygonSet
     * Convert solid areas full shapes to polygon set
     * (the full shape is the polygon area with a thick outline)
     * Used in 3D view
     * Arcs (ends of segments) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygons
     * @param aError = Maximum error allowed between true arc and polygon approx
     */
    void TransformSolidAreasShapesToPolygonSet(
            SHAPE_POLY_SET& aCornerBuffer, int aError = ARC_HIGH_DEF ) const;

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
     * @param aPreserveCorners an optional set of corners which should not be smoothed.
     * if both aMinClearanceValue = 0 and aUseNetClearance = false: create the zone outline polygon.
     */
    void TransformOutlinesShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                int aMinClearanceValue, bool aUseNetClearance,
                                std::set<VECTOR2I>* aPreserveCorners = nullptr ) const;

    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the zone shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aError = the maximum deviation from true circle
     * @param ignoreLineWidth = used for edge cut items where the line width is only
     * for visualization
     */
    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, int aClearanceValue,
            int aError = ARC_HIGH_DEF, bool ignoreLineWidth = false ) const override;

    /**
     * Function HitTestForCorner
     * tests if the given wxPoint is near a corner.
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return bool - true if some corner was found to be closer to refPos than aClearance; false
     *              otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos, int aAccuracy,
                           SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Function HitTestForCorner
     * tests if the given wxPoint is near a corner.
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @return bool - true if some corner was found to be closer to refPos than aClearance; false
     *              otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos, int aAccuracy ) const;

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return bool - true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos, int aAccuracy,
                         SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @return bool - true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos, int aAccuracy ) const;

    /** @copydoc BOARD_ITEM::HitTest(const EDA_RECT& aRect,
     *                               bool aContained = true, int aAccuracy ) const
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const override;


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
    virtual void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    /**
     * Function Mirror
     * Mirror the outlines , relative to a given horizontal axis
     * the layer is not changed
     * @param aMirrorRef = axis position
     * @param aMirrorLeftRight mirror across Y axis (otherwise mirror across X)
     */
    void Mirror( const wxPoint& aMirrorRef, bool aMirrorLeftRight );

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
            if( m_Poly->Vertex( relativeIndices ).x != new_pos.x ||
                m_Poly->Vertex( relativeIndices ).y != new_pos.y )
                SetNeedRefill( true );

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
     * Add a new corner to the zone outline (to the main outline or a hole)
     * @param aPosition         is the position of the new corner.
     * @param aHoleIdx          is the index of the hole (-1 for the main outline, >= 0 for hole).
     * @param aAllowDuplication is a flag to indicate whether it is allowed to add this corner
     *                          even if it is duplicated.
     * @return true if the corner was added, false if error (aHoleIdx > hole count -1)
     */
    bool AppendCorner( wxPoint aPosition, int aHoleIdx, bool aAllowDuplication = false );

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

    /** (re)create a list of triangles that "fill" the solid areas.
     * used for instance to draw these solid areas on opengl
     */
    void CacheTriangulation();

   /**
     * Function SetFilledPolysList
     * sets the list of filled polygons.
     */
    void SetFilledPolysList( SHAPE_POLY_SET& aPolysList )
    {
        m_FilledPolysList = aPolysList;
    }

    /**
      * Function SetFilledPolysList
      * sets the list of filled polygons.
      */
    void SetRawPolysList( SHAPE_POLY_SET& aPolysList )
    {
        m_RawPolysList = aPolysList;
    }


    /**
     * Function GetSmoothedPoly
     * returns a pointer to the corner-smoothed version of m_Poly.
     * @param aPreserveCorners - set of corners which should /not/ be smoothed
     * @return SHAPE_POLY_SET* - pointer to the polygon.
     */
    bool BuildSmoothedPoly( SHAPE_POLY_SET& aSmoothedPoly,
                            std::set<VECTOR2I>* aPreserveCorners ) const;

    void SetCornerSmoothingType( int aType ) { m_cornerSmoothingType = aType; };

    int  GetCornerSmoothingType() const { return m_cornerSmoothingType; }

    void SetCornerRadius( unsigned int aRadius );

    unsigned int GetCornerRadius() const { return m_cornerRadius; }

    bool GetFilledPolysUseThickness() const { return m_FilledPolysUseThickness; }
    void SetFilledPolysUseThickness( bool aOption ) { m_FilledPolysUseThickness = aOption; }


    /**
     * add a polygon to the zone outline
     * if the zone outline is empty, this is the main outline
     * else it is a hole inside the main outline
     */
    void AddPolygon( std::vector< wxPoint >& aPolygon );

    void AddPolygon( const SHAPE_LINE_CHAIN& aPolygon );

    void SetFillSegments( const ZONE_SEGMENT_FILL& aSegments )
    {
        m_FillSegmList = aSegments;
    }

    SHAPE_POLY_SET& RawPolysList()
    {
        return m_RawPolysList;
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

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
     * @return int - the default hatch pitch in internal units.
     */
    static int GetDefaultHatchPitch();

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

    bool   GetHV45() const { return m_hv45; }
    void   SetHV45( bool aConstrain ) { m_hv45 = aConstrain; }

    /** @return the hash value previously calculated by BuildHashValue().
     * used in zone filling calculations
     */
    MD5_HASH GetHashValue() { return m_filledPolysHash; }

    /** Build the hash value of m_FilledPolysList, and store it internally
     *  in m_filledPolysHash.
     *  Used in zone filling calculations, to know if m_FilledPolysList is up to date.
     */
    void BuildHashValue() { m_filledPolysHash = m_FilledPolysList.GetHash(); }



#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    virtual void SwapData( BOARD_ITEM* aImage ) override;

private:

    SHAPE_POLY_SET*       m_Poly;                ///< Outline of the zone.
    int                   m_cornerSmoothingType;
    unsigned int          m_cornerRadius;

    LSET                  m_layerSet;

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
    bool                  m_FilledPolysUseThickness;    ///< outline of filled polygons have thickness.

    /** True when a zone was filled, false after deleting the filled areas. */
    bool                  m_IsFilled;

    /** False when a zone was refilled, true after changes in zone params.
     * m_needRefill = false does not imply filled areas are up to date, just
     * the zone was refilled after edition, and does not need refilling
     */
    bool                  m_needRefill;

    ///< Width of the gap in thermal reliefs.
    int                   m_ThermalReliefGap;

    ///< Width of the copper bridge in thermal reliefs.
    int                   m_ThermalReliefCopperBridge;


    /** How to fill areas:
     * ZFM_POLYGONS => use solid polygons
     * ZFM_HATCH_PATTERN => use a grid pattern as shape
     */
    ZONE_FILL_MODE        m_FillMode;

    /// Grid style shape: thickness of lines (if 0 -> solid shape)
    int m_HatchFillTypeThickness;

    /// Grid style shape: dist between center of lines (grid size) (0 -> solid shape)
    int m_HatchFillTypeGap;

    /// Grid style shape: orientation in degrees of the grid lines
    double m_HatchFillTypeOrientation;

    /// Grid pattern smoothing type, similar to corner smoothing type
    ///< 0 = no smoothing, 1 = fillet, >= 2 = arc
    int  m_HatchFillTypeSmoothingLevel;

    /// Grid pattern smoothing value for smoothing shape size calculations
    /// this is the ratio between the gap and the chamfer size
    double m_HatchFillTypeSmoothingValue;

    /// The index of the corner being moved or nullptr if no corner is selected.
    SHAPE_POLY_SET::VERTEX_INDEX* m_CornerSelection;

    /// Variable used in polygon calculations.
    int                   m_localFlgs;

    /** Segments used to fill the zone (#m_FillMode ==1 ), when fill zone by segment is used.
     *  In this case the segments have #m_ZoneMinThickness width.
     */
    ZONE_SEGMENT_FILL          m_FillSegmList;

    /* set of filled polygons used to draw a zone as a filled area.
     * from outlines (m_Poly) but unlike m_Poly these filled polygons have no hole
     * (they are all in one piece)  In very simple cases m_FilledPolysList is same
     * as m_Poly.  In less simple cases (when m_Poly has holes) m_FilledPolysList is
     * a polygon equivalent to m_Poly, without holes but with extra outline segment
     * connecting "holes" with external main outline.  In complex cases an outline
     * described by m_Poly can have many filled areas
     */
    SHAPE_POLY_SET        m_FilledPolysList;
    SHAPE_POLY_SET        m_RawPolysList;
    MD5_HASH              m_filledPolysHash;    // A hash value used in zone filling calculations
                                                // to see if the filled areas are up to date

    HATCH_STYLE           m_hatchStyle;     // hatch style, see enum above
    int                   m_hatchPitch;     // for DIAGONAL_EDGE, distance between 2 hatch lines
    std::vector<SEG>      m_HatchLines;     // hatch lines
    std::vector<int>      m_insulatedIslands;

    bool                  m_hv45;           // constrain edges to horizontal, vertical or 45º
};


#endif  // CLASS_ZONE_H_
