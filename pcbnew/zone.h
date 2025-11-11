/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef ZONE_H
#define ZONE_H


#include <mutex>
#include <vector>
#include <map>
#include <gr_basic.h>
#include <board_item.h>
#include <board_connected_item.h>
#include <layer_ids.h>
#include <lset.h>
#include <geometry/shape_poly_set.h>
#include <zone_settings.h>
#include <teardrop/teardrop_types.h>


class LINE_READER;
class PCB_EDIT_FRAME;
class BOARD;
class ZONE;
class MSG_PANEL_ITEM;


/**
 * A struct recording the isolated and single-pad islands within a zone.  Each array holds
 * indexes into the outlines of a SHAPE_POLY_SET for a zone fill on a particular layer.
 *
 * Isolated outlines are those whose *connectivity cluster* contains no pads.  These generate
 * DRC violations.
 *
 * Single-connection outlines are those with a *direct* connection to only a single item.  These
 * participate in thermal spoke counting as a pad spoke to an *otherwise* unconnected island
 * provides no connectivity to the pad.
 */
struct ISOLATED_ISLANDS
{
    std::vector<int> m_IsolatedOutlines;
    std::vector<int> m_SingleConnectionOutlines;
};


/**
 * Handle a list of polygons defining a copper zone.
 *
 * A zone is described by a main polygon, a time stamp, a layer or a layer set, and a net name.
 * Other polygons inside the main polygon are holes in the zone.
 */
class ZONE : public BOARD_CONNECTED_ITEM
{
public:
    ZONE( BOARD_ITEM_CONTAINER* parent );

    ZONE( const ZONE& aZone );
    ZONE& operator=( const ZONE &aOther );

    ~ZONE();

    void CopyFrom( const BOARD_ITEM* aOther ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == PCB_ZONE_T;
    }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    /**
     * Not all ZONEs are *really* BOARD_CONNECTED_ITEMs....
     */
    bool IsConnected() const override
    {
        return !GetIsRuleArea();
    }

    /**
     * Copy aZone data to me
     */
    void InitDataFromSrcInCopyCtor( const ZONE& aZone, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    /**
     * For rule areas which exclude footprints (and therefore participate in courtyard conflicts
     * during move).
     */
    bool IsConflicting() const;

    /**
     * @return a VECTOR2I, position of the first point of the outline
     */
    VECTOR2I GetPosition() const override;
    void     SetPosition( const VECTOR2I& aPos ) override {}

    /**
     * @param aPriority is the priority level.
     */
    void SetAssignedPriority( unsigned aPriority ) { m_priority = aPriority; }

    /**
     * @return the priority level of this zone.
     */
    unsigned GetAssignedPriority() const { return m_priority; }

    bool HigherPriority( const ZONE* aOther ) const;

    bool SameNet( const ZONE* aOther ) const;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;
    wxString GetFriendlyName() const override;

    void SetLayerSet( const LSET& aLayerSet ) override;
    virtual LSET GetLayerSet() const override { return m_layerSet; }

    /**
     * Set the zone to be on the aLayerSet layers and only remove the fill polygons
     * from the unused layers, while keeping the fills on the layers in both the old
     * and new layer sets.
     */
    void SetLayerSetAndRemoveUnusedFills( const LSET& aLayerSet );

    ZONE_LAYER_PROPERTIES& LayerProperties( PCB_LAYER_ID aLayer )
    {
        return m_layerProperties[aLayer];
    }

    std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>& LayerProperties() { return m_layerProperties; }

    const std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>& LayerProperties() const
    {
        return m_layerProperties;
    }

    void SetLayerProperties( const std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>& aOther );

    const wxString& GetZoneName() const { return m_zoneName; }
    void SetZoneName( const wxString& aName ) { m_zoneName = aName; }

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return BOARD_ITEM::Matches( GetZoneName(), aSearchData );
    }

    /**
     * @return the bounding box of the zone outline.
     */
    const BOX2I GetBoundingBox() const override;

    /**
     * Used to preload the zone bounding box cache so we don't have to worry about mutex-locking
     * it each time.
     */
    void CacheBoundingBox();

    /**
     * @return the zone's clearance in internal units.
     */
    std::optional<int> GetLocalClearance() const override;
    void SetLocalClearance( std::optional<int> aClearance ) { m_ZoneClearance = aClearance.value_or( 0 ); };

    /**
     * Return any local clearances set in the "classic" (ie: pre-rule) system.
     *
     * @param aSource [out] optionally reports the source as a user-readable string.
     * @return the clearance in internal units.
     */
    std::optional<int> GetLocalClearance( wxString* aSource ) const override
    {
        if( m_isRuleArea )
            return std::optional<int>();

        if( aSource )
            *aSource = _( "zone" );

        return GetLocalClearance();
    }

    /**
     * @return true if this zone is on a copper layer, false if on a technical layer.
     */
    bool IsOnCopperLayer() const override;

    virtual void SetLayer( PCB_LAYER_ID aLayer ) override;

    virtual PCB_LAYER_ID GetLayer() const override;

    // Return the first layer in GUI sequence.
    PCB_LAYER_ID GetFirstLayer() const;

    virtual bool IsOnLayer( PCB_LAYER_ID ) const override;

    virtual std::vector<int> ViewGetLayers() const override;

    double ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

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

    /**
     * Compute the area currently occupied by the zone fill.
     *
     * @return the currently filled area
     */
    double CalculateFilledArea();

    /**
     * Compute the area of the zone outline (not the filled area).
     * @return the currently calculated area
     */
    double CalculateOutlineArea();

    /**
     * This area is cached from the most recent call to CalculateFilledArea().
     *
     * @return the filled area
     */
    double GetFilledArea()
    {
        return m_area;
    }

    /**
     * This area is cached from the most recent call to CalculateOutlineArea().
     *
     * @return the outline area
     */
    double GetOutlineArea()
    {
        return m_outlinearea;
    }

    std::mutex& GetLock()
    {
        return m_lock;
    }

    int GetFillFlag( PCB_LAYER_ID aLayer )
    {
        return m_fillFlags.test( aLayer );
    }

    void SetFillFlag( PCB_LAYER_ID aLayer, bool aFlag ) { m_fillFlags.set( aLayer, aFlag ); }

    bool IsFilled() const { return m_isFilled; }
    void SetIsFilled( bool isFilled ) { m_isFilled = isFilled; }

    bool NeedRefill() const { return m_needRefill; }
    void SetNeedRefill( bool aNeedRefill ) { m_needRefill = aNeedRefill; }

    ZONE_CONNECTION GetPadConnection() const { return m_PadConnection; }
    void SetPadConnection( ZONE_CONNECTION aPadConnection ) { m_PadConnection = aPadConnection; }

    int GetMinThickness() const { return m_ZoneMinThickness; }
    void SetMinThickness( int aMinThickness )
    {
        m_ZoneMinThickness = aMinThickness;
        m_hatchThickness   = std::max( m_hatchThickness, aMinThickness );
        m_hatchGap         = std::max( m_hatchGap, aMinThickness );
        SetNeedRefill( true );
    }

    int GetHatchThickness() const { return m_hatchThickness; }
    void SetHatchThickness( int aThickness ) { m_hatchThickness = aThickness; }

    int GetHatchGap() const { return m_hatchGap; }
    void SetHatchGap( int aStep ) { m_hatchGap = aStep; }

    EDA_ANGLE GetHatchOrientation() const { return m_hatchOrientation; }
    void SetHatchOrientation( const EDA_ANGLE& aStep ) { m_hatchOrientation = aStep; }

    int GetHatchSmoothingLevel() const { return m_hatchSmoothingLevel; }
    void SetHatchSmoothingLevel( int aLevel ) { m_hatchSmoothingLevel = aLevel; }

    double GetHatchSmoothingValue() const { return m_hatchSmoothingValue; }
    void SetHatchSmoothingValue( double aValue ) { m_hatchSmoothingValue = aValue; }

    double GetHatchHoleMinArea() const { return m_hatchHoleMinArea; }
    void SetHatchHoleMinArea( double aPct ) { m_hatchHoleMinArea = aPct; }

    int GetHatchBorderAlgorithm() const { return m_hatchBorderAlgorithm; }
    void SetHatchBorderAlgorithm( int aAlgo ) { m_hatchBorderAlgorithm = aAlgo; }

    ///
    int GetLocalFlags() const { return m_localFlgs; }
    void SetLocalFlags( int aFlags ) { m_localFlgs = aFlags; }

    SHAPE_POLY_SET* Outline() { return m_Poly; }
    const SHAPE_POLY_SET* Outline() const { return m_Poly; }

    void SetOutline( SHAPE_POLY_SET* aOutline ) { m_Poly = aOutline; }

    // @copydoc BOARD_ITEM::GetEffectiveShape
    virtual std::shared_ptr<SHAPE>
    GetEffectiveShape( PCB_LAYER_ID aLayer = UNDEFINED_LAYER,
                       FLASHING aFlash = FLASHING::DEFAULT ) const override;

    /**
     * Test if a point is near an outline edge or a corner of this zone.
     *
     * @param aPosition the VECTOR2I to test
     * @return true if a hit, else false
     */
    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    /**
     * Test if the given VECTOR2I is within the bounds of a filled area of this zone.
     *
     * @param aLayer is the layer to test on
     * @param aRefPos A VECTOR2I to test
     * @param aAccuracy Expand the distance by which the areas are expanded for the hittest
     * @return true if a hit, else false
     */
    bool HitTestFilledArea( PCB_LAYER_ID aLayer, const VECTOR2I& aRefPos, int aAccuracy = 0 ) const;

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

    /**
     * Some intersecting zones, despite being on the same layer with the same net, cannot be
     * merged due to other parameters such as fillet radius.  The copper pour will end up
     * effectively merged though, so we need to do some calculations with them in mind.
     */
    void GetInteractingZones( PCB_LAYER_ID aLayer, std::vector<ZONE*>* aSameNetCollidingZones,
                              std::vector<ZONE*>* aOtherNetIntersectingZones ) const;

    /**
     * Convert solid areas full shapes to polygon set
     * (the full shape is the polygon area with a thick outline)
     * Used in 3D view
     * Arcs (ends of segments) are approximated by segments
     *
     * @param aLayer is the layer of the zone to retrieve
     * @param aBuffer = a buffer to store the polygons
     * @param aError = Maximum error allowed between true arc and polygon approx
     */
    void TransformSolidAreasShapesToPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aBuffer ) const;

    /**
     * Convert the outlines shape to a polygon with no holes
     * inflated (optional) by max( aClearanceValue, the zone clearance)
     * (holes are linked to external outline by overlapping segments)
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments.
     *
     * @param aBuffer is a buffer to store the polygon
     * @param aClearance is the min clearance around outlines
     * @param aBoardOutline is the board outline (if a valid one exists; nullptr otherwise)
     */
    void TransformSmoothedOutlineToPolygon( SHAPE_POLY_SET& aBuffer, int aClearance,
                                            int aError, ERROR_LOC aErrorLoc,
                                            SHAPE_POLY_SET* aBoardOutline ) const;

    /**
     * Convert the zone shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     *
     * @param aLayer is the layer of the filled zone to retrieve
     * @param aBuffer is a buffer to store the polygon
     * @param aClearance is the clearance around the pad
     * @param aError is the maximum deviation from true circle
     * @param ignoreLineWidth is used for edge cut items where the line width is only for
     *                        visualization
     */
    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                  int aClearance, int aError, ERROR_LOC aErrorLoc,
                                  bool ignoreLineWidth = false ) const override;

    /**
     * Test if the given VECTOR2I is near a corner.
     *
     * @param  refPos     is the VECTOR2I to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out, optional] is the index of the closest vertex found when return
     *                    value is true
     * @return true if some corner was found to be closer to refPos than aClearance; false
     *         otherwise.
     */
    bool HitTestForCorner( const VECTOR2I& refPos, int aAccuracy,
                           SHAPE_POLY_SET::VERTEX_INDEX* aCornerHit = nullptr ) const;

    /**
     * Test if the given VECTOR2I is near a segment defined by 2 corners.
     *
     * @param  refPos     is the VECTOR2I to test.
     * @param  aAccuracy  increase the item bounding box by this amount.
     * @param  aCornerHit [out, optional] is the index of the closest vertex found when return
     *                    value is true.
     * @return true if some edge was found to be closer to refPos than aClearance.
     */
    bool HitTestForEdge( const VECTOR2I& refPos, int aAccuracy,
                         SHAPE_POLY_SET::VERTEX_INDEX* aCornerHit = nullptr ) const;

    /**
     * @copydoc EDA_ITEM::HitTest(const BOX2I& aRect, bool aContained, int aAccuracy) const
     */
    bool HitTest( const BOX2I& aRect, bool aContained = true, int aAccuracy = 0 ) const override;

    /**
     * @copydoc EDA_ITEM::HitTest(const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
     */
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

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
    void Move( const VECTOR2I& offset ) override;

    /**
     * Move the outline Edge.
     *
     * @param offset is moving vector
     * @param aEdge is start point of the outline edge
     */
    void MoveEdge( const VECTOR2I& offset, int aEdge );

    /**
     * Rotate the outlines.
     *
     * @param aCentre is rot centre
     */
    void Rotate( const VECTOR2I& aCentre, const EDA_ANGLE& aAngle ) override;

    /**
     * Flip this object, i.e. change the board side for this object
     * (like Mirror() but changes layer).
     *
     * @param aCentre is the rotation point.
     * @param aFlipDirection is the direction of the flip.
     */
    virtual void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /**
     * Mirror the outlines relative to a given horizontal axis the layer is not changed.
     *
     * @param aMirrorRef is axis position
     * @param aFlipDirection is the direction of the flip.
     */
    void Mirror( const VECTOR2I& aMirrorRef, FLIP_DIRECTION aFlipDirection ) override;

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
    bool AppendCorner( VECTOR2I aPosition, int aHoleIdx, bool aAllowDuplication = false );

    ZONE_BORDER_DISPLAY_STYLE GetHatchStyle() const { return m_borderStyle; }
    void SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE aStyle ) { m_borderStyle = aStyle; }

    bool HasFilledPolysForLayer( PCB_LAYER_ID aLayer ) const
    {
        return m_FilledPolysList.count( aLayer ) > 0;
    }

    /**
     * @return a reference to the list of filled polygons.
     */
    const std::shared_ptr<SHAPE_POLY_SET>& GetFilledPolysList( PCB_LAYER_ID aLayer ) const
    {
        wxASSERT( m_FilledPolysList.count( aLayer ) );
        return m_FilledPolysList.at( aLayer );
    }

    SHAPE_POLY_SET* GetFill( PCB_LAYER_ID aLayer )
    {
        wxASSERT( m_FilledPolysList.count( aLayer ) );
        return m_FilledPolysList.at( aLayer ).get();
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
        m_FilledPolysList[aLayer] = std::make_shared<SHAPE_POLY_SET>( aPolysList );
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
    void AddPolygon( std::vector<VECTOR2I>& aPolygon );

    void AddPolygon( const SHAPE_LINE_CHAIN& aPolygon );

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override;
    ZONE* Clone( PCB_LAYER_ID aLayer ) const;

    /**
     * @return true if the zone is a teardrop area
     */
    bool IsTeardropArea() const { return m_teardropType != TEARDROP_TYPE::TD_NONE; }

    /**
     * Set the type of teardrop if the zone is a teardrop area
     * for non teardrop area, the type must be TEARDROP_TYPE::TD_NONE
     */
    void SetTeardropAreaType( TEARDROP_TYPE aType ) { m_teardropType = aType; }

    /**
     * @return the type of the teardrop ( has meaning only if the zone is a teardrop area)
     */
    TEARDROP_TYPE GetTeardropAreaType() const { return m_teardropType; }

    /**
     * Accessor to determine if any keepout parameters are set
     */
    bool HasKeepoutParametersSet() const
    {
        return m_doNotAllowTracks || m_doNotAllowVias || m_doNotAllowPads || m_doNotAllowFootprints
               || m_doNotAllowZoneFills;
    }

    /**
     * Accessors to parameters used in Rule Area zones:
     */
    bool GetIsRuleArea() const                    { return m_isRuleArea; }
    void SetIsRuleArea( bool aEnable )            { m_isRuleArea = aEnable; }
    bool GetPlacementAreaEnabled() const          { return m_placementAreaEnabled; }
    void SetPlacementAreaEnabled( bool aEnabled ) { m_placementAreaEnabled = aEnabled; }

    wxString GetPlacementAreaSource() const                     { return m_placementAreaSource; }
    void SetPlacementAreaSource( const wxString& aSource )      { m_placementAreaSource = aSource; }
    PLACEMENT_SOURCE_T GetPlacementAreaSourceType() const       { return m_placementAreaSourceType; }
    void SetPlacementAreaSourceType( PLACEMENT_SOURCE_T aType ) { m_placementAreaSourceType = aType; }

    bool GetDoNotAllowZoneFills() const  { return m_doNotAllowZoneFills; }
    bool GetDoNotAllowVias() const       { return m_doNotAllowVias; }
    bool GetDoNotAllowTracks() const     { return m_doNotAllowTracks; }
    bool GetDoNotAllowPads() const       { return m_doNotAllowPads; }
    bool GetDoNotAllowFootprints() const { return m_doNotAllowFootprints; }

    void SetDoNotAllowZoneFills( bool aEnable )  { m_doNotAllowZoneFills = aEnable; }
    void SetDoNotAllowVias( bool aEnable )       { m_doNotAllowVias = aEnable; }
    void SetDoNotAllowTracks( bool aEnable )     { m_doNotAllowTracks = aEnable; }
    void SetDoNotAllowPads( bool aEnable )       { m_doNotAllowPads = aEnable; }
    void SetDoNotAllowFootprints( bool aEnable ) { m_doNotAllowFootprints = aEnable; }

    ISLAND_REMOVAL_MODE GetIslandRemovalMode() const   { return m_islandRemovalMode; }
    void SetIslandRemovalMode( ISLAND_REMOVAL_MODE aRemove ) { m_islandRemovalMode = aRemove; }

    long long int GetMinIslandArea() const       { return m_minIslandArea; }
    void SetMinIslandArea( long long int aArea ) { m_minIslandArea = aArea; }

    /**
     * HatchBorder related methods
     */

    /**
     * @return the zone hatch pitch in iu.
     */
    int GetBorderHatchPitch() const        { return m_borderHatchPitch; }
    void SetBorderHatchPitch( int aPitch ) { m_borderHatchPitch = aPitch; }

    /**
     * @return the default hatch pitch in internal units.
     */
    static int GetDefaultHatchPitch();

    /**
     * Set all hatch parameters for the zone.
     *
     * @param  aBorderHatchStyle   is the style of the hatch, specified as one of HATCH_STYLE
                                   possible values.
     * @param  aBorderHatchPitch   is the hatch pitch in iu.
     * @param  aRebuildBorderHatch is a flag to indicate whether to re-hatch after having set the
     *                       previous parameters.
     */
    void SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE aBorderHatchStyle, int aBorderHatchPitch,
                                bool aRebuilBorderdHatch );

    /**
     * Clear the zone's hatch.
     */
    void UnHatchBorder();

    /**
     * Compute the hatch lines depending on the hatch parameters and stores it in the zone's
     * attribute m_borderHatchLines.
     */
    void HatchBorder();

    const std::vector<SEG>& GetHatchLines() const { return m_borderHatchLines; }

    /**
     * Build the hash value of m_FilledPolysList, and store it internally in m_filledPolysHash.
     * Used in zone filling calculations, to know if m_FilledPolysList is up to date.
     */
    void BuildHashValue( PCB_LAYER_ID aLayer );

    /**
     * @return the hash value previously calculated by BuildHashValue().
     */
    HASH_128 GetHashValue( PCB_LAYER_ID aLayer );

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const ZONE& aOther ) const;
    bool operator==( const BOARD_ITEM& aOther ) const override;

#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }

    void SetFillPoly( PCB_LAYER_ID aLayer, SHAPE_POLY_SET* aPoly )
    {
        m_FilledPolysList[ aLayer ] = std::make_shared<SHAPE_POLY_SET>( *aPoly );
        SetFillFlag( aLayer, true );
    }

#endif

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

protected:
    SHAPE_POLY_SET*       m_Poly;                ///< Outline of the zone.
    int                   m_cornerSmoothingType;
    unsigned int          m_cornerRadius;

    /// An optional unique name for this zone, used for identifying it in DRC checking
    wxString              m_zoneName;

    LSET                  m_layerSet;

    std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES> m_layerProperties;

    /* Priority: when a zone outline is inside and other zone, if its priority is higher
     * the other zone priority, it will be created inside.
     * if priorities are equal, a DRC error is set
     */
    unsigned              m_priority;

    /* A zone outline can be a keepout zone.
     * It will be never filled, and DRC should test for pads, tracks and vias
     */
    bool m_isRuleArea;

    /**
     * Placement rule area data
     */
    bool                  m_placementAreaEnabled;
    PLACEMENT_SOURCE_T    m_placementAreaSourceType;
    wxString              m_placementAreaSource;

    /* A zone outline can be a teardrop zone with different rules for priority
     * (always bigger priority than copper zones) and never removed from a
     * copper zone having the same netcode
     */
    TEARDROP_TYPE         m_teardropType;

    /* For keepout zones only:
     * what is not allowed inside the keepout ( pads, tracks and vias )
     */
    bool                  m_doNotAllowZoneFills;
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
    long long int         m_minIslandArea;

    /** True when a zone was filled, false after deleting the filled areas. */
    bool                  m_isFilled;

    /**
     * False when a zone was refilled, true after changes in zone params.
     * m_needRefill = false does not imply filled areas are up to date, just
     * the zone was refilled after edition, and does not need refilling
     */
    bool             m_needRefill;

    int              m_thermalReliefGap;        // Width of the gap in thermal reliefs.
    int              m_thermalReliefSpokeWidth; // Width of the copper bridge in thermal reliefs.

    ZONE_FILL_MODE   m_fillMode;                // fill with POLYGONS vs HATCH_PATTERN
    int              m_hatchThickness;          // thickness of lines (if 0 -> solid shape)
    int              m_hatchGap;                // gap between lines (0 -> solid shape
    EDA_ANGLE        m_hatchOrientation;        // orientation of grid lines
    int              m_hatchSmoothingLevel;     // 0 = no smoothing
                                                // 1 = fillet
                                                // 2 = arc low def
                                                // 3 = arc high def
    double           m_hatchSmoothingValue;     // hole chamfer/fillet size (ratio of hole size)
    double           m_hatchHoleMinArea;        // min size before holes are dropped (ratio)
    int              m_hatchBorderAlgorithm;    // 0 = use min zone thickness
                                                // 1 = use hatch thickness
    int              m_localFlgs;               // Variable used in polygon calculations.

    /* set of filled polygons used to draw a zone as a filled area.
     * from outlines (m_Poly) but unlike m_Poly these filled polygons have no hole
     * (they are all in one piece)  In very simple cases m_FilledPolysList is same
     * as m_Poly.  In less simple cases (when m_Poly has holes) m_FilledPolysList is
     * a polygon equivalent to m_Poly, without holes but with extra outline segment
     * connecting "holes" with external main outline.  In complex cases an outline
     * described by m_Poly can have many filled areas
     */
    std::map<PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>> m_FilledPolysList;

    /// Temp variables used while filling
    LSET                                   m_fillFlags;

    /// A hash value used in zone filling calculations to see if the filled areas are up to date
    std::map<PCB_LAYER_ID, HASH_128>       m_filledPolysHash;

    ZONE_BORDER_DISPLAY_STYLE m_borderStyle;       // border display style, see enum above
    int                       m_borderHatchPitch;  // for DIAGONAL_EDGE, distance between 2 lines
    std::vector<SEG>          m_borderHatchLines;  // hatch lines

    /// For each layer, a set of insulated islands that were not removed
    std::map<PCB_LAYER_ID, std::set<int>> m_insulatedIslands;

    double                    m_area;              // The filled zone area
    double                    m_outlinearea;       // The outline zone area

    /// Lock used for multi-threaded filling on multi-layer zones
    std::mutex                m_lock;
};


#ifndef SWIG
DECLARE_ENUM_TO_WXANY( ZONE_CONNECTION )
DECLARE_ENUM_TO_WXANY( ZONE_FILL_MODE )
DECLARE_ENUM_TO_WXANY( ISLAND_REMOVAL_MODE )
DECLARE_ENUM_TO_WXANY( PLACEMENT_SOURCE_T )
#endif

#endif  // ZONE_H
