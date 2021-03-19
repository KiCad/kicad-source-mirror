/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ZONE_H
#define ZONE_H


#include <mutex>
#include <vector>
#include <gr_basic.h>
#include <board_item.h>
#include <board_connected_item.h>
#include <layers_id_colors_and_visibility.h>
#include <geometry/shape_poly_set.h>
#include <zone_settings.h>


class EDA_RECT;
class LINE_READER;
class PCB_EDIT_FRAME;
class BOARD;
class ZONE;
class MSG_PANEL_ITEM;

typedef std::vector<SEG> ZONE_SEGMENT_FILL;

/**
 * Handle a list of polygons defining a copper zone.
 *
 * A zone is described by a main polygon, a time stamp, a layer or a layer set, and a net name.
 * Other polygons inside the main polygon are holes in the zone.
 *
 * a item ZONE is living in a board
 * a variant FP_ZONE is living in a footprint
 */
class ZONE : public BOARD_CONNECTED_ITEM
{
public:

    /**
     * The ctor to build ZONE, but compatible with FP_ZONE requirement.
     * if aInFP is true, a FP_ZONE is actually built
     * (same item, but with a specific type id:
     * The type is PCB_ZONE_T for a ZONE
     * The type is PCB_FP_ZONE_T for a FP_ZONE
     */
    ZONE( BOARD_ITEM_CONTAINER* parent, bool aInFP = false );

    ZONE( const ZONE& aZone );
    ZONE& operator=( const ZONE &aOther );

    ~ZONE();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_ZONE_T;
    }

    /**
     * Not all ZONEs are *really* BOARD_CONNECTED_ITEMs....
     */
    bool IsConnected() const override
    {
        return !GetIsRuleArea();
    }

    NETCLASS* GetNetClass() const override
    {
        if( GetIsRuleArea() )
            return nullptr;

        return BOARD_CONNECTED_ITEM::GetNetClass();
    }

    wxString GetNetClassName() const override
    {
        if( GetIsRuleArea() )
            return "UNDEFINED";

        return BOARD_CONNECTED_ITEM::GetNetClassName();
    }
    /**
     * Copy aZone data to me
     */
    void InitDataFromSrcInCopyCtor( const ZONE& aZone );

    /**
     * @return a wxPoint, position of the first point of the outline
     */
    wxPoint GetPosition() const override;
    void SetPosition( const wxPoint& aPos ) override {}

    /**
     * @param aPriority is the priority level.
     */
    void SetPriority( unsigned aPriority ) { m_priority = aPriority; }

    /**
     * @return the priority level of this zone.
     */
    unsigned GetPriority() const { return m_priority; }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void SetLayerSet( LSET aLayerSet ) override;
    virtual LSET GetLayerSet() const override;

    wxString GetZoneName() const { return m_zoneName; }
    void SetZoneName( const wxString& aName ) { m_zoneName = aName; }

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override
    {
        return BOARD_ITEM::Matches( GetZoneName(), aSearchData );
    }

    /**
     * @return an EDA_RECT that is the bounding box of the zone outline.
     */
    const EDA_RECT GetBoundingBox() const override;

    /**
     * ONLY TO BE USED BY CLIENTS WHICH SET UP THE CACHE!
     */
    const EDA_RECT GetCachedBoundingBox() const { return m_bboxCache; }
    void CacheBoundingBox() { m_bboxCache = GetBoundingBox(); }

    /**
     * Return any local clearances set in the "classic" (ie: pre-rule) system.  These are
     * things like zone clearance which are NOT an override.
     *
     * @param aSource [out] optionally reports the source as a user-readable string
     * @return the clearance in internal units.
     */
    int GetLocalClearance( wxString* aSource ) const override;

    int GetLocalClearance() const { return GetLocalClearance( nullptr ); }
    void SetLocalClearance( int aClearance ) { m_ZoneClearance = aClearance; }

    /**
     * @return true if this zone is on a copper layer, false if on a technical layer.
     */
    bool IsOnCopperLayer() const override;

    /**
     * Test if this zone shares a common layer with the given layer set.
     */
    bool CommonLayerExists( const LSET aLayerSet ) const;

    virtual void SetLayer( PCB_LAYER_ID aLayer ) override;

    virtual PCB_LAYER_ID GetLayer() const override;

    virtual bool IsOnLayer( PCB_LAYER_ID ) const override;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;

    void SetFillMode( ZONE_FILL_MODE aFillMode ) { m_fillMode = aFillMode; }
    ZONE_FILL_MODE GetFillMode() const { return m_fillMode; }

    void SetThermalReliefGap( int aThermalReliefGap )
    {
        if( m_thermalReliefGap != aThermalReliefGap )
            SetNeedRefill( true );

        m_thermalReliefGap = aThermalReliefGap;
    }
    int GetThermalReliefGap() const { return m_thermalReliefGap; }
    int GetThermalReliefGap( PAD* aPad, wxString* aSource = nullptr ) const;

    void SetThermalReliefSpokeWidth( int aThermalReliefSpokeWidth )
    {
        if( m_thermalReliefSpokeWidth != aThermalReliefSpokeWidth )
            SetNeedRefill( true );

        m_thermalReliefSpokeWidth = aThermalReliefSpokeWidth;
    }
    int GetThermalReliefSpokeWidth() const { return m_thermalReliefSpokeWidth; }
    int GetThermalReliefSpokeWidth( PAD* aPad, wxString* aSource = nullptr ) const;

    /**
     * Compute the area currently occupied by the zone fill.
     *
     * @return the currently filled area
     */
    double CalculateFilledArea();

    /**
     * This area is cached from the most recent call to CalculateFilledArea().
     *
     * @return the filled area
     */
    double GetFilledArea()
    {
        return m_area;
    }

    std::mutex& GetLock()
    {
        return m_lock;
    }

    int GetFillFlag( PCB_LAYER_ID aLayer )
    {
        return m_fillFlags.count( aLayer ) ? m_fillFlags[ aLayer ] : false;
    }
    void SetFillFlag( PCB_LAYER_ID aLayer, bool aFlag ) { m_fillFlags[ aLayer ] = aFlag; }

    bool IsFilled() const { return m_isFilled; }
    void SetIsFilled( bool isFilled ) { m_isFilled = isFilled; }

    bool NeedRefill() const { return m_needRefill; }
    void SetNeedRefill( bool aNeedRefill ) { m_needRefill = aNeedRefill; }

    ZONE_CONNECTION GetPadConnection( PAD* aPad, wxString* aSource = nullptr ) const;
    ZONE_CONNECTION GetPadConnection() const { return m_PadConnection; }
    void SetPadConnection( ZONE_CONNECTION aPadConnection ) { m_PadConnection = aPadConnection; }

    int GetMinThickness() const { return m_ZoneMinThickness; }
    void SetMinThickness( int aMinThickness )
    {
        if( m_ZoneMinThickness != aMinThickness )
            SetNeedRefill( true );

        m_ZoneMinThickness = aMinThickness;
    }

    int GetHatchThickness() const { return m_hatchThickness; }
    void SetHatchThickness( int aThickness ) { m_hatchThickness = aThickness; }

    int GetHatchGap() const { return m_hatchGap; }
    void SetHatchGap( int aStep ) { m_hatchGap = aStep; }

    double GetHatchOrientation() const { return m_hatchOrientation; }
    void SetHatchOrientation( double aStep ) { m_hatchOrientation = aStep; }

    int GetHatchSmoothingLevel() const { return m_hatchSmoothingLevel; }
    void SetHatchSmoothingLevel( int aLevel ) { m_hatchSmoothingLevel = aLevel; }

    double GetHatchSmoothingValue() const { return m_hatchSmoothingValue; }
    void SetHatchSmoothingValue( double aValue ) { m_hatchSmoothingValue = aValue; }

    double GetHatchHoleMinArea() const { return m_hatchHoleMinArea; }
    void SetHatchHoleMinArea( double aPct ) { m_hatchHoleMinArea = aPct; }

    int GetHatchBorderAlgorithm() const { return m_hatchBorderAlgorithm; }
    void SetHatchBorderAlgorithm( int aAlgo ) { m_hatchBorderAlgorithm = aAlgo; }

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

    ZONE_SEGMENT_FILL& FillSegments( PCB_LAYER_ID aLayer )
    {
        wxASSERT( m_FillSegmList.count( aLayer ) );
        return m_FillSegmList.at( aLayer );
    }

    const ZONE_SEGMENT_FILL& FillSegments( PCB_LAYER_ID aLayer ) const
    {
        wxASSERT( m_FillSegmList.count( aLayer ) );
        return m_FillSegmList.at( aLayer );
    }

    SHAPE_POLY_SET* Outline() { return m_Poly; }
    const SHAPE_POLY_SET* Outline() const { return m_Poly; }

    void SetOutline( SHAPE_POLY_SET* aOutline ) { m_Poly = aOutline; }

    // @copydoc BOARD_ITEM::GetEffectiveShape
    virtual std::shared_ptr<SHAPE>
    GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER ) const override;

    /**
     * Test if a point is near an outline edge or a corner of this zone.
     *
     * @param aPosition the wxPoint to test
     * @return true if a hit, else false
     */
    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Test if the given wxPoint is within the bounds of a filled area of this zone.
     *
     * @param aLayer is the layer to test on
     * @param aRefPos A wxPoint to test
     * @param aAccuracy Expand the distance by which the areas are expanded for the hittest
     * @return true if a hit, else false
     */
    bool HitTestFilledArea( PCB_LAYER_ID aLayer, const wxPoint &aRefPos, int aAccuracy = 0 ) const;

    /**
     * Test if the given point is contained within a cutout of the zone.
     *
     * @param aRefPos is the point to test
     * @param aOutlineIdx is the index of the outline containing the cutout
     * @param aHoleIdx is the index of the hole
     * @return true if aRefPos is inside a zone cutout
     */
    bool HitTestCutout( const VECTOR2I& aRefPos, int* aOutlineIdx = nullptr,
                        int* aHoleIdx = nullptr ) const;

    bool HitTestCutout( const wxPoint& aRefPos, int* aOutlineIdx = nullptr,
                        int* aHoleIdx = nullptr ) const
    {
        return HitTestCutout( VECTOR2I( aRefPos.x, aRefPos.y ), aOutlineIdx, aHoleIdx );
    }

    /**
     * Some intersecting zones, despite being on the same layer with the same net, cannot be
     * merged due to other parameters such as fillet radius.  The copper pour will end up
     * effectively merged though, so we need to do some calculations with them in mind.
     */
    void GetInteractingZones( PCB_LAYER_ID aLayer, std::vector<ZONE*>* aZones ) const;

    /**
     * Convert solid areas full shapes to polygon set
     * (the full shape is the polygon area with a thick outline)
     * Used in 3D view
     * Arcs (ends of segments) are approximated by segments
     *
     * @param aLayer is the layer of the zone to retrieve
     * @param aCornerBuffer = a buffer to store the polygons
     * @param aError = Maximum error allowed between true arc and polygon approx
     */
    void TransformSolidAreasShapesToPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aCornerBuffer,
                                             int aError = ARC_HIGH_DEF ) const;

    /**
     * Convert the outlines shape to a polygon with no holes
     * inflated (optional) by max( aClearanceValue, the zone clearance)
     * (holes are linked to external outline by overlapping segments)
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments.
     *
     * @param aCornerBuffer is a buffer to store the polygon
     * @param aClearance is the min clearance around outlines
     * @param aBoardOutline is the board outline (if a valid one exists; nullptr otherwise)
     */
    void TransformSmoothedOutlineToPolygon( SHAPE_POLY_SET& aCornerBuffer, int aClearance,
                                            SHAPE_POLY_SET* aBoardOutline ) const;

    /**
     * Convert the zone shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     *
     * @param aLayer is the layer of the filled zone to retrieve
     * @param aCornerBuffer is a buffer to store the polygon
     * @param aClearanceValue is the clearance around the pad
     * @param aError is the maximum deviation from true circle
     * @param ignoreLineWidth is used for edge cut items where the line width is only
     * for visualization
     */
    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                               PCB_LAYER_ID aLayer, int aClearanceValue,
                                               int aError, ERROR_LOC aErrorLoc,
                                               bool ignoreLineWidth = false ) const override;

    /**
     * Test if the given wxPoint is near a corner.
     *
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return true if some corner was found to be closer to refPos than aClearance; false
     *         otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos, int aAccuracy,
                           SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Test if the given wxPoint is near a corner.
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @return true if some corner was found to be closer to refPos than aClearance; false
     *         otherwise.
     */
    bool HitTestForCorner( const wxPoint& refPos, int aAccuracy ) const;

    /**
     * Test if the given wxPoint is near a segment defined by 2 corners.
     *
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out] is the index of the closest vertex found, useless when return
     *                    value is false.
     * @return true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos, int aAccuracy,
                         SHAPE_POLY_SET::VERTEX_INDEX& aCornerHit ) const;

    /**
     * Test if the given wxPoint is near a segment defined by 2 corners.
     *
     * @param  refPos     is the wxPoint to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @return true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const wxPoint& refPos, int aAccuracy ) const;

    /**
     * @copydoc BOARD_ITEM::HitTest(const EDA_RECT& aRect,
     *                              bool aContained = true, int aAccuracy) const
     */
    bool HitTest( const EDA_RECT& aRect, bool aContained = true, int aAccuracy = 0 ) const override;

    /**
     * Removes the zone filling.
     *
     * @return true if a previous filling is removed, false if no change (when no filling found).
     */
    bool UnFill();

    /* Geometric transformations: */

    /**
     * Move the outlines
     *
     * @param offset is moving vector
     */
    void Move( const wxPoint& offset ) override;

    /**
     * Move the outline Edge.
     *
     * @param offset is moving vector
     * @param aEdge is start point of the outline edge
     */
    void MoveEdge( const wxPoint& offset, int aEdge );

    /**
     * Move the outlines.
     *
     * @param aCentre is rot centre
     * @param aAngle is in 0.1 degree
     */
    void Rotate( const wxPoint& aCentre, double aAngle ) override;

    /**
     * Flip this object, i.e. change the board side for this object
     * (like Mirror() but changes layer).
     *
     * @param aCentre is the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre, bool aFlipLeftRight ) override;

    /**
     * Mirror the outlines relative to a given horizontal axis the layer is not changed.
     *
     * @param aMirrorRef is axis position
     * @param aMirrorLeftRight mirror across Y axis (otherwise mirror across X)
     */
    void Mirror( const wxPoint& aMirrorRef, bool aMirrorLeftRight );

    /**
     * @return the class name.
     */
    wxString GetClass() const override
    {
        return wxT( "ZONE" );
    }

    /**
     * Access to m_Poly parameters
     */
    int GetNumCorners( void ) const
    {
        return m_Poly->TotalVertices();
    }

    /**
     * Return an iterator to visit all points of the zone's main outline without holes.
     *
     * @return an iterator to visit the zone vertices without holes.
     */
    SHAPE_POLY_SET::ITERATOR Iterate()
    {
        return m_Poly->Iterate();
    }

    /**
     * Return an iterator to visit all points of the zone's main outline with holes.
     *
     * @return an iterator to visit the zone vertices with holes.
     */
    SHAPE_POLY_SET::ITERATOR IterateWithHoles()
    {
        return m_Poly->IterateWithHoles();
    }

    /**
     * Return an iterator to visit all points of the zone's main outline with holes.
     *
     * @return an iterator to visit the zone vertices with holes.
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
            if( m_Poly->CVertex( relativeIndices ).x != new_pos.x
                    || m_Poly->CVertex( relativeIndices ).y != new_pos.y )
            {
                SetNeedRefill( true );
                m_Poly->SetVertex( relativeIndices, new_pos );
            }
        }
        else
        {
            throw( std::out_of_range( "aCornerIndex-th vertex does not exist" ) );
        }
    }

    /**
     * Create a new hole on the zone; i.e., a new contour on the zone's outline.
     */
    void NewHole()
    {
        m_Poly->NewHole();
    }

    /**
     * Add a new corner to the zone outline (to the main outline or a hole)
     *
     * @param aPosition         is the position of the new corner.
     * @param aHoleIdx          is the index of the hole (-1 for the main outline, >= 0 for hole).
     * @param aAllowDuplication is a flag to indicate whether it is allowed to add this corner
     *                          even if it is duplicated.
     * @return true if the corner was added, false if error (aHoleIdx > hole count -1)
     */
    bool AppendCorner( wxPoint aPosition, int aHoleIdx, bool aAllowDuplication = false );

    ZONE_BORDER_DISPLAY_STYLE GetHatchStyle() const { return m_borderStyle; }
    void SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE aStyle ) { m_borderStyle = aStyle; }

    /**
     * Test if 2 zones are equivalent.
     *
     * Zones are equivalent if they have same parameters and same outline info.
     *
     * @note Filling is not taken into account.
     *
     * @param aZoneToCompare is the zone to compare with "this"
     */
    bool IsSame( const ZONE &aZoneToCompare );

    bool HasFilledPolysForLayer( PCB_LAYER_ID aLayer ) const
    {
        return m_FilledPolysList.count( aLayer ) > 0;
    }

    /**
     * @return a reference to the list of filled polygons.
     */
    const SHAPE_POLY_SET& GetFilledPolysList( PCB_LAYER_ID aLayer ) const
    {
        wxASSERT( m_FilledPolysList.count( aLayer ) );
        return m_FilledPolysList.at( aLayer );
    }

    /**
     * Create a list of triangles that "fill" the solid areas used for instance to draw
     * these solid areas on OpenGL.
     */
    void CacheTriangulation( PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    /**
     * Set the list of filled polygons.
     */
    void SetFilledPolysList( PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aPolysList )
    {
        m_FilledPolysList[aLayer] = aPolysList;
    }

    /**
     * Set the list of filled polygons.
     */
    void SetRawPolysList( PCB_LAYER_ID aLayer, const SHAPE_POLY_SET& aPolysList )
    {
        m_RawPolysList[aLayer] = aPolysList;
    }

    /**
     * Check if a given filled polygon is an insulated island.
     *
     * @param aLayer is the layer to test
     * @param aPolyIdx is an index into m_FilledPolysList[aLayer]
     * @return true if the given polygon is insulated (i.e. has no net connection)
     */
    bool IsIsland( PCB_LAYER_ID aLayer, int aPolyIdx ) const;

    void SetIsIsland( PCB_LAYER_ID aLayer, int aPolyIdx )
    {
        m_insulatedIslands[aLayer].insert( aPolyIdx );
    }

    bool BuildSmoothedPoly( SHAPE_POLY_SET& aSmoothedPoly, PCB_LAYER_ID aLayer,
                            SHAPE_POLY_SET* aBoardOutline,
                            SHAPE_POLY_SET* aSmoothedPolyWithApron = nullptr ) const;

    void SetCornerSmoothingType( int aType ) { m_cornerSmoothingType = aType; };

    int  GetCornerSmoothingType() const { return m_cornerSmoothingType; }

    void SetCornerRadius( unsigned int aRadius );

    unsigned int GetCornerRadius() const { return m_cornerRadius; }

    bool GetFilledPolysUseThickness() const { return m_fillVersion == 5; }
    bool GetFilledPolysUseThickness( PCB_LAYER_ID aLayer ) const;

    int GetFillVersion() const { return m_fillVersion; }
    void SetFillVersion( int aVersion ) { m_fillVersion = aVersion; }

    /**
     * Remove a cutout from the zone.
     *
     * @param aOutlineIdx is the zone outline the hole belongs to
     * @param aHoleIdx is the hole in the outline to remove
     */
    void RemoveCutout( int aOutlineIdx, int aHoleIdx );

    /**
     * Add a polygon to the zone outline.
     *
     * If the zone outline is empty, this is the main outline.  Otherwise it is a hole
     * inside the main outline.
     */
    void AddPolygon( std::vector< wxPoint >& aPolygon );

    void AddPolygon( const SHAPE_LINE_CHAIN& aPolygon );

    void SetFillSegments( PCB_LAYER_ID aLayer, const ZONE_SEGMENT_FILL& aSegments )
    {
        m_FillSegmList[aLayer] = aSegments;
    }

    SHAPE_POLY_SET& RawPolysList( PCB_LAYER_ID aLayer )
    {
        wxASSERT( m_RawPolysList.count( aLayer ) );
        return m_RawPolysList.at( aLayer );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

    /**
     * Accessors to parameters used in Rule Area zones:
     */
    bool GetIsRuleArea() const { return m_isRuleArea; }
    bool GetDoNotAllowCopperPour() const { return m_doNotAllowCopperPour; }
    bool GetDoNotAllowVias() const { return m_doNotAllowVias; }
    bool GetDoNotAllowTracks() const { return m_doNotAllowTracks; }
    bool GetDoNotAllowPads() const { return m_doNotAllowPads; }
    bool GetDoNotAllowFootprints() const { return m_doNotAllowFootprints; }
    bool IsKeepout() const;
    bool KeepoutAll() const;

    void SetIsRuleArea( bool aEnable ) {m_isRuleArea = aEnable; }
    void SetDoNotAllowCopperPour( bool aEnable ) { m_doNotAllowCopperPour = aEnable; }
    void SetDoNotAllowVias( bool aEnable ) { m_doNotAllowVias = aEnable; }
    void SetDoNotAllowTracks( bool aEnable ) { m_doNotAllowTracks = aEnable; }
    void SetDoNotAllowPads( bool aEnable ) { m_doNotAllowPads = aEnable; }
    void SetDoNotAllowFootprints( bool aEnable ) { m_doNotAllowFootprints = aEnable; }

    const ISLAND_REMOVAL_MODE GetIslandRemovalMode() const { return m_islandRemovalMode; }
    void SetIslandRemovalMode( ISLAND_REMOVAL_MODE aRemove ) {
        m_islandRemovalMode = aRemove; }

    long long int GetMinIslandArea() const { return m_minIslandArea; }
    void SetMinIslandArea( long long int aArea ) { m_minIslandArea = aArea; }

    /**
     * HatchBorder related methods
     */

    /**
     * @return the zone hatch pitch in iu.
     */
    int GetBorderHatchPitch() const;

    /**
     * @return the default hatch pitch in internal units.
     */
    static int GetDefaultHatchPitch();

    /**
     * Set all hatch parameters for the zone.
     *
     * @param  aHatchStyle   is the style of the hatch, specified as one of HATCH_STYLE possible
     *                       values.
     * @param  aHatchPitch   is the hatch pitch in iu.
     * @param  aRebuildHatch is a flag to indicate whether to re-hatch after having set the
     *                       previous parameters.
     */
    void SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE aHatchStyle, int aHatchPitch,
                                bool aRebuildHatch );

    /**
     * Set the hatch pitch parameter for the zone.
     *
     * @param aPitch is the hatch pitch in iu.
     */
    void SetHatchPitch( int aPitch );

    /**
     * Clear the zone's hatch.
     */
    void UnHatchBorder();

    /**
     * Compute the hatch lines depending on the hatch parameters and stores it in the zone's
     * attribute m_borderHatchLines.
     */
    void   HatchBorder();

    const std::vector<SEG>& GetHatchLines() const { return m_borderHatchLines; }

    bool   GetHV45() const { return m_hv45; }
    void   SetHV45( bool aConstrain ) { m_hv45 = aConstrain; }

    /**
     * Build the hash value of m_FilledPolysList, and store it internally in m_filledPolysHash.
     * Used in zone filling calculations, to know if m_FilledPolysList is up to date.
     */
    void BuildHashValue( PCB_LAYER_ID aLayer );

    /**
     * @return the hash value previously calculated by BuildHashValue().
     */
    MD5_HASH GetHashValue( PCB_LAYER_ID aLayer );

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

    virtual void SwapData( BOARD_ITEM* aImage ) override;

protected:
    SHAPE_POLY_SET*       m_Poly;                ///< Outline of the zone.
    int                   m_cornerSmoothingType;
    unsigned int          m_cornerRadius;

    /// An optional unique name for this zone, used for identifying it in DRC checking
    wxString              m_zoneName;

    LSET                  m_layerSet;

    /* Priority: when a zone outline is inside and other zone, if its priority is higher
     * the other zone priority, it will be created inside.
     * if priorities are equal, a DRC error is set
     */
    unsigned              m_priority;

    /* A zone outline can be a keepout zone.
     * It will be never filled, and DRC should test for pads, tracks and vias
     */
    bool m_isRuleArea;

    /* For keepout zones only:
     * what is not allowed inside the keepout ( pads, tracks and vias )
     */
    bool                  m_doNotAllowCopperPour;
    bool                  m_doNotAllowVias;
    bool                  m_doNotAllowTracks;
    bool                  m_doNotAllowPads;
    bool                  m_doNotAllowFootprints;

    ZONE_CONNECTION       m_PadConnection;
    int                   m_ZoneClearance;           // Clearance value in internal units.
    int                   m_ZoneMinThickness;        // Minimum thickness value in filled areas.
    int                   m_fillVersion;             // See BOARD_DESIGN_SETTINGS for version
                                                     // differences.
    ISLAND_REMOVAL_MODE   m_islandRemovalMode;

    /**
     * When island removal mode is set to AREA, islands below this area will be removed.
     * If this value is negative, all islands will be removed.
     */
    long long int    m_minIslandArea;

    /** True when a zone was filled, false after deleting the filled areas. */
    bool             m_isFilled;

    /**
     * False when a zone was refilled, true after changes in zone params.
     * m_needRefill = false does not imply filled areas are up to date, just
     * the zone was refilled after edition, and does not need refilling
     */
    bool             m_needRefill;

    int              m_thermalReliefGap;        // Width of the gap in thermal reliefs.
    int              m_thermalReliefSpokeWidth; // Width of the copper bridge in thermal reliefs.


    /**
     * How to fill areas:
     *
     * ZONE_FILL_MODE::POLYGONS => use solid polygons
     * ZONE_FILL_MODE::HATCH_PATTERN => use a grid pattern as shape
     */
    ZONE_FILL_MODE   m_fillMode;
    int              m_hatchThickness;          // thickness of lines (if 0 -> solid shape)
    int              m_hatchGap;                // gap between lines (0 -> solid shape
    double           m_hatchOrientation;        // orientation in degrees of grid lines
    int              m_hatchSmoothingLevel;     // 0 = no smoothing
                                                // 1 = fillet
                                                // 2 = arc low def
                                                // 3 = arc high def
    double           m_hatchSmoothingValue;     // hole chamfer/fillet size (ratio of hole size)
    double           m_hatchHoleMinArea;        // min size before holes are dropped (ratio)
    int              m_hatchBorderAlgorithm;    // 0 = use min zone thickness
                                                // 1 = use hatch thickness

    /// The index of the corner being moved or nullptr if no corner is selected.
    SHAPE_POLY_SET::VERTEX_INDEX* m_CornerSelection;

    int              m_localFlgs;               // Variable used in polygon calculations.

    /**
     * Segments used to fill the zone (#m_FillMode ==1 ), when fill zone by segment is used.
     * In this case the segments have #m_ZoneMinThickness width.
     */
    std::map<PCB_LAYER_ID, ZONE_SEGMENT_FILL> m_FillSegmList;

    /* set of filled polygons used to draw a zone as a filled area.
     * from outlines (m_Poly) but unlike m_Poly these filled polygons have no hole
     * (they are all in one piece)  In very simple cases m_FilledPolysList is same
     * as m_Poly.  In less simple cases (when m_Poly has holes) m_FilledPolysList is
     * a polygon equivalent to m_Poly, without holes but with extra outline segment
     * connecting "holes" with external main outline.  In complex cases an outline
     * described by m_Poly can have many filled areas
     */
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> m_FilledPolysList;
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> m_RawPolysList;

    /// Temp variables used while filling
    EDA_RECT                               m_bboxCache;
    std::map<PCB_LAYER_ID, bool>           m_fillFlags;

    /// A hash value used in zone filling calculations to see if the filled areas are up to date
    std::map<PCB_LAYER_ID, MD5_HASH>       m_filledPolysHash;

    ZONE_BORDER_DISPLAY_STYLE m_borderStyle;       // border display style, see enum above
    int                       m_borderHatchPitch;  // for DIAGONAL_EDGE, distance between 2 lines
    std::vector<SEG>          m_borderHatchLines;  // hatch lines

    /// For each layer, a set of insulated islands that were not removed
    std::map<PCB_LAYER_ID, std::set<int>> m_insulatedIslands;

    bool                      m_hv45;              // constrain edges to horiz, vert or 45º

    double                    m_area;              // The filled zone area

    /// Lock used for multi-threaded filling on multi-layer zones
    std::mutex m_lock;
};


/**
 * A specialization of ZONE for use in footprints.
 */
class FP_ZONE : public ZONE
{
public:
    FP_ZONE( BOARD_ITEM_CONTAINER* aParent );
    FP_ZONE( const FP_ZONE& aZone );
    FP_ZONE& operator=( const FP_ZONE &aOther );

    EDA_ITEM* Clone() const override;

    double ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const override;
};

#endif  // ZONE_H
