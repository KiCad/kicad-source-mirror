/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef PCB_GRIDITEM_H
#define PCB_GRIDITEM_H

#include <board_item.h>
#include <geometry/grid_geometry.h>


class EDA_ANGLE;
class SHAPE_LINE_CHAIN;


enum class PCB_GRIDITEM_TYPE
{
    CARTESIAN = 0,
    POLAR = 1
};


struct PCB_GRIDITEM_AFFECTS
{
    bool cursor = true;    ///< Replace the display grid for cursor snapping inside coverage.
    bool routing = true;   ///< Used by the router as a local routing frame.
    bool placement = true; ///< Used by edit/move tools for placement snap.

    bool All() const { return cursor && routing && placement; }
    void SetAll( bool aState ) { cursor = routing = placement = aState; }

    bool operator==( const PCB_GRIDITEM_AFFECTS& aOther ) const
    {
        return cursor == aOther.cursor && routing == aOther.routing && placement == aOther.placement;
    }
};


enum class PCB_GRIDITEM_ROLE
{
    CURSOR,
    ROUTING,
    PLACEMENT
};


class PCB_GRIDITEM : public BOARD_ITEM
{
public:
    PCB_GRIDITEM( BOARD_ITEM* aParent );

    static inline bool ClassOf( const EDA_ITEM* aItem ) { return aItem && PCB_GRIDITEM_T == aItem->Type(); }

    wxString GetClass() const override { return wxT( "PCB_GRIDITEM" ); }

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

    void     SetPosition( const VECTOR2I& aPos ) override { m_pos = aPos; }
    VECTOR2I GetPosition() const override { return m_pos; }

    EDA_ANGLE GetOrientation() const { return m_orientation; }
    void      SetOrientation( const EDA_ANGLE& aAngle ) { m_orientation = aAngle; }
    double    GetOrientationDegrees() const { return m_orientation.AsDegrees(); }
    void      SetOrientationDegrees( double aDeg ) { m_orientation = EDA_ANGLE( aDeg, DEGREES_T ); }

    void              SetGridItemType( PCB_GRIDITEM_TYPE aType ) { m_type = aType; }
    PCB_GRIDITEM_TYPE GetGridItemType() const { return m_type; }

    // Half-extent: distance from centre to edge.  The grid is symmetric about the centre,
    // so extents are stored as magnitudes - a negative input describes the same box.
    void     SetExtent( const VECTOR2I& aExtent ) { m_extent = { std::abs( aExtent.x ), std::abs( aExtent.y ) }; }
    VECTOR2I GetExtent() const { return m_extent; }
    void SetSpacing( const VECTOR2I& aSpacing )
    {
        m_spacing = { std::max( aSpacing.x, 1 ), std::max( aSpacing.y, 1 ) };
    }
    VECTOR2I GetSpacing() const { return m_spacing; }

    // Property-panel accessors - 2x to show total width to the user.
    int  GetExtentX() const { return m_extent.x * 2; }
    void SetExtentX( int aX ) { m_extent.x = std::abs( aX / 2 ); }
    int  GetExtentY() const { return m_extent.y * 2; }
    void SetExtentY( int aY ) { m_extent.y = std::abs( aY / 2 ); }

    int  GetSpacingX() const { return m_spacing.x; }
    void SetSpacingX( int aX ) { m_spacing.x = std::max( aX, 1 ); }
    int  GetSpacingY() const { return m_spacing.y; }
    void SetSpacingY( int aY ) { m_spacing.y = std::max( aY, 1 ); }

    int  GetRadiusExtent() const { return m_extent.x; }
    void SetRadiusExtent( int aR ) { m_extent.x = std::abs( aR ); }
    int  GetRadiusSpacing() const { return m_spacing.x; }
    void SetRadiusSpacing( int aR ) { m_spacing.x = std::max( aR, 1 ); }

    EDA_ANGLE GetPhiExtent() const { return m_phiExtent; }
    EDA_ANGLE GetPhiSpacing() const { return m_phiSpacing; }
    double    GetPhiExtentDegrees() const { return m_phiExtent.AsDegrees(); }
    void      SetPhiExtentDegrees( double aDeg )
    {
        m_phiExtent = EDA_ANGLE( std::max( std::abs( aDeg ), 0.001 ), DEGREES_T );
    }
    double    GetPhiSpacingDegrees() const { return m_phiSpacing.AsDegrees(); }
    void      SetPhiSpacingDegrees( double aDeg )
    {
        m_phiSpacing = EDA_ANGLE( std::max( std::abs( aDeg ), 0.001 ), DEGREES_T );
    }

    /// Priority 0 is reserved for the global background grid; user-assigned
    /// 0 is bumped to 1.
    void     SetAssignedPriority( unsigned aPriority ) { m_priority = aPriority ? aPriority : 1u; }
    unsigned GetAssignedPriority() const { return m_priority; }

    // Every Nth line drawn as a major tick.  0 = no subdivisions.
    void     SetTickInterval( unsigned aInterval ) { m_tickInterval = aInterval; }
    unsigned GetTickInterval() const { return m_tickInterval; }

    PCB_GRIDITEM_AFFECTS&       Affects() { return m_affects; }
    const PCB_GRIDITEM_AFFECTS& Affects() const { return m_affects; }

    void SetAffectsCursor( bool aOn ) { m_affects.cursor = aOn; }
    bool GetAffectsCursor() const { return m_affects.cursor; }
    void SetAffectsRouting( bool aOn ) { m_affects.routing = aOn; }
    bool GetAffectsRouting() const { return m_affects.routing; }
    void SetAffectsPlacement( bool aOn ) { m_affects.placement = aOn; }
    bool GetAffectsPlacement() const { return m_affects.placement; }

    /**
     * Project this grid into a GRID_GEOMETRY (doubles, radians) for shared math.
     */
    GRID_GEOMETRY AsGridGeometry() const;

    double GetCoverageArea() const { return AsGridGeometry().Area(); }

    /**
     * @return true if aPos is within aTolerance of this grid's coverage region.
     */
    bool HitTestArea( const VECTOR2I& aPos, int aTolerance = 0 ) const
    {
        return AsGridGeometry().Contains( VECTOR2D( aPos ), (double) aTolerance );
    }

    LSET GetLayerSet() const override { return LSET::AllLayersMask(); }
    void SetLayerSet( const LSET& aLayers ) override
    {
        // Grid items live on every layer; ignore attempts to assign a layer set.
    }

    std::vector<int> ViewGetLayers() const override;
    double           ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const override;

    void Move( const VECTOR2I& aMoveVector ) override { m_pos += aMoveVector; }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override;
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    // Grid items have no physical footprint on any layer - silence the base-class warning.
    void TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance, int aError,
                                  ERROR_LOC aErrorLoc, bool ignoreLineWidth = false ) const override
    {
    }

    const BOX2I GetBoundingBox() const override;

    std::shared_ptr<SHAPE> GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash = FLASHING::DEFAULT ) const override;

    wxString  GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;
    BITMAPS   GetMenuImage() const override;
    EDA_ITEM* Clone() const override;
    void      GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;
    double    Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const PCB_GRIDITEM& aOther ) const;
    bool operator==( const BOARD_ITEM& aOther ) const override;

#if defined( DEBUG )
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    /// Build a closed world-space outline polygon (rect or wedge).  Shared by
    /// HitTest(BOX2I) and GetBoundingBox(); polar arc is sampled at 32 segments.
    SHAPE_LINE_CHAIN buildOutlineWorld() const;

    PCB_GRIDITEM_TYPE m_type;
    VECTOR2I          m_pos;
    VECTOR2I          m_extent;  // cartesian half (x, y); polar (rMax, unused)
    VECTOR2I          m_spacing; // cartesian (dx, dy);    polar (dr,   unused)
    EDA_ANGLE         m_orientation;
    EDA_ANGLE         m_phiExtent;  // polar only
    EDA_ANGLE         m_phiSpacing; // polar only

    COLOR4D              m_color;
    unsigned             m_priority = 1;     ///< Higher wins in overlap resolution; 0 reserved for global grid.
    unsigned             m_tickInterval = 0; ///< 0 = no major ticks; N = every Nth line is major.
    PCB_GRIDITEM_AFFECTS m_affects;
};


class BOARD;

/**
 * Pick the grid item active for aRole at aPos.  Priority-descending; smaller
 * coverage area wins ties.
 *
 * @return nullptr if no applicable grid covers aPos.
 */
PCB_GRIDITEM* FindActiveGridAt( const BOARD& aBoard, const VECTOR2I& aPos, PCB_GRIDITEM_ROLE aRole );


#endif
