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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRID_HELPER_H
#define GRID_HELPER_H

#include <vector>
#include <optional>

#include <geometry/point_types.h>
#include <geometry/seg.h>
#include <kiid.h>
#include <math/vector2d.h>
#include <preview_items/anchor_debug.h>
#include <preview_items/snap_indicator.h>
#include <preview_items/construction_geom.h>
#include <snap/snap_resolver.h>
#include <tool/construction_manager.h>
#include <tool/selection.h>
#include <origin_viewitem.h>

class TOOL_MANAGER; // Forward declaration to avoid hard dependency in tests

class EDA_ITEM;

/**
 * The snap identity of a document item.
 *
 * Both editors must agree on this, or the same item earns different stable ids and the resolver's
 * retention and hysteresis stop matching across a frame.  The snap library can't do it itself
 * because it doesn't link against #KIID.
 */
inline SNAP_TARGET_ID SnapTargetId( const KIID& aId )
{
    return aId.AsBytes();
}

enum GRID_HELPER_GRIDS : int
{
    // When the item doesn't match an override, use the current user grid
    GRID_CURRENT,

    GRID_CONNECTABLE,
    GRID_WIRES,
    GRID_VIAS,
    GRID_TEXT,
    GRID_GRAPHICS
};

class GRID_HELPER
{
    friend void TEST_CLEAR_ANCHORS( GRID_HELPER& helper );

public:
    GRID_HELPER();
    GRID_HELPER( TOOL_MANAGER* aToolMgr, int aConstructionLayer );
    virtual ~GRID_HELPER();

    VECTOR2I GetGrid() const;
    VECTOR2D GetVisibleGrid() const;
    VECTOR2I GetOrigin() const;

    /**
     * Reset all internal state.  Used to remove any dangling pointers to items
     * that have been deleted.
     */
    virtual void FullReset()
    {
        m_constructionGeomPreview.ClearSnapLine();
        m_snapManager.Clear();
        m_anchors.clear();
        m_retainedAngleBranch.reset();
        m_stickySnapIds.clear();
        m_layoutReferencePreference = {};
    }

    // Manual setters used when no TOOL_MANAGER/View is available (e.g. in tests)
    void SetGridSize( const VECTOR2D& aGrid ) { m_manualGrid = aGrid; }
    void SetVisibleGridSize( const VECTOR2D& aGrid ) { m_manualVisibleGrid = aGrid; }
    void SetOrigin( const VECTOR2I& aOrigin ) { m_manualOrigin = aOrigin; }
    void SetGridSnapping( bool aEnable ) { m_manualGridSnapping = aEnable; }

    void SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin = VECTOR2I( 0, 0 ) );

    virtual VECTOR2I Align( const VECTOR2I& aPoint, GRID_HELPER_GRIDS aGrid ) const
    {
        return Align( aPoint, GetGridSize( aGrid ), GetOrigin() );
    }

    virtual VECTOR2I AlignGrid( const VECTOR2I& aPoint, GRID_HELPER_GRIDS aGrid ) const
    {
        return AlignGrid( aPoint, GetGridSize( aGrid ), GetOrigin() );
    }

    virtual VECTOR2I Align( const VECTOR2I& aPoint ) const;
    virtual VECTOR2I Align( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                            const VECTOR2D& aOffset ) const;

    VECTOR2I AlignGrid( const VECTOR2I& aPoint ) const;
    VECTOR2I AlignGrid( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                        const VECTOR2D& aOffset ) const;

    /**
     * Gets the coarsest grid that applies to a selecion of items.
     */
    virtual GRID_HELPER_GRIDS GetSelectionGrid( const SELECTION& aSelection ) const;

    /**
     * Get the coarsest grid that applies to an item.
     */
    virtual GRID_HELPER_GRIDS GetItemGrid( const EDA_ITEM* aItem ) const { return GRID_CURRENT; }

    /**
     * Return the size of the specified grid.
     */
    virtual VECTOR2D GetGridSize( GRID_HELPER_GRIDS aGrid ) const;

    void SetSkipPoint( const VECTOR2I& aPoint )
    {
        m_skipPoint = aPoint;
    }

    /**
     * Clear the skip point by setting it to an unreachable position, thereby preventing matching.
     */
    void ClearSkipPoint()
    {
        m_skipPoint = VECTOR2I( std::numeric_limits<int>::min(), std::numeric_limits<int>::min() );
    }

    void SetSnap( bool aSnap ) { m_enableSnap = aSnap; }
    bool GetSnap() const { return m_enableSnap; }

    void SetUseGrid( bool aSnapToGrid ) { m_enableGrid = aSnapToGrid; }
    bool GetUseGrid() const { return m_enableGrid; }

    void SetSnapLine( bool aSnap ) { m_enableSnapLine = aSnap; }
    void SetSnapLineDirections( const std::vector<VECTOR2I>& aDirections );
    void SetSnapLineOrigin( const VECTOR2I& aOrigin );
    void SetSnapLineEnd( const std::optional<VECTOR2I>& aEnd );
    void ClearSnapLine();
    std::optional<VECTOR2I> SnapToConstructionLines( const VECTOR2I& aPoint,
                                                     const VECTOR2I& aNearestGrid,
                                                     const VECTOR2D& aGrid,
                                                     double aSnapRange ) const;

    void SetMask( int aMask ) { m_maskTypes = aMask; }
    void SetMaskFlag( int aFlag ) { m_maskTypes |= aFlag; }
    void ClearMaskFlag( int aFlag ) { m_maskTypes = m_maskTypes & ~aFlag; }

    std::optional<VECTOR2I> GetSnappedPoint() const;

    /**
     * Restrict snapping to a set of angle branches emanating from an origin.
     *
     * Passing an empty origin clears the restriction and any retained branch so the next resolve
     * starts fresh.
     */
    void SetAngleRestriction( std::optional<VECTOR2I> aOrigin, double aStepDegrees )
    {
        m_angleOrigin = aOrigin;
        m_angleStepDegrees = aStepDegrees;

        if( !m_angleOrigin )
            m_retainedAngleBranch.reset();
    }

    void SetPointEditProfile( bool aEnabled ) { m_pointEditProfile = aEnabled; }

    void SetStationarySelfGeometry( std::vector<VECTOR2I> aPoints, std::vector<SEG> aSegments )
    {
        m_stationarySelfPoints = std::move( aPoints );
        m_stationarySelfSegments = std::move( aSegments );
    }

    void SetFeasibilityCallback( SNAP_RESOLVER::FEASIBILITY_CALLBACK aCallback )
    {
        m_feasibilityCallback = std::move( aCallback );
    }

    enum ANCHOR_FLAGS
    {
        CORNER = 1,
        OUTLINE = 2,
        SNAPPABLE = 4,
        ORIGIN = 8,
        VERTICAL = 16,
        HORIZONTAL = 32,

        // This anchor comes from 'constructed' geometry (e.g. an intersection
        // with something else), and not from some intrinsic point of an item
        // (e.g. an endpoint)
        CONSTRUCTED = 64,
        ALL = CORNER | OUTLINE | SNAPPABLE | ORIGIN | VERTICAL | HORIZONTAL | CONSTRUCTED
    };

protected:
    struct ANCHOR
    {
        /**
         * @param aPos The position of the anchor.
         * @param aFlags The flags for the anchor - this is a bitfield of ANCHOR_FLAGS,
         *               specifying the type of anchor (which may be used to filter out
         *               unwanted anchors per the settings).
         * @param aPointTypes The point types that this anchor represents in geometric terms.
         * @param aItem The item to which the anchor belongs.
         */
        ANCHOR( const VECTOR2I& aPos, int aFlags, int aPointTypes, std::vector<EDA_ITEM*> aItems ) :
                pos( aPos ), flags( aFlags ), pointTypes( aPointTypes ),
                items( std::move( aItems ) )
        {
        }

        VECTOR2I  pos;
        int       flags;
        int       pointTypes;

        /// Items that are associated with this anchor (can be more than one, e.g. for an
        /// intersection).
        std::vector<EDA_ITEM*> items;

        double Distance( const VECTOR2I& aP ) const
        {
            return VECTOR2D( (double) aP.x - pos.x, (double) aP.y - pos.y ).EuclideanNorm();
        }

        bool InvolvesItem( const EDA_ITEM& aItem ) const
        {
            return std::find( items.begin(), items.end(), &aItem ) != items.end();
        }
    };

    void addAnchor( const VECTOR2I& aPos, int aFlags, EDA_ITEM* aItem,
                    int aPointTypes = POINT_TYPE::PT_NONE )
    {
        addAnchor( aPos, aFlags, std::vector<EDA_ITEM*>{ aItem }, aPointTypes );
    }

    void addAnchor( const VECTOR2I& aPos, int aFlags, std::vector<EDA_ITEM*> aItems,
                    int aPointTypes )
    {
        if( ( aFlags & m_maskTypes ) == aFlags )
            m_anchors.emplace_back( ANCHOR( aPos, aFlags, aPointTypes, std::move( aItems ) ) );
    }

    void clearAnchors()
    {
        m_anchors.clear();
    }

    /**
     * Check whether it is possible to use the grid -- this depends both on local grid helper
     * settings and global (tool manager) KiCad settings.
     */
    bool canUseGrid() const;

    VECTOR2I computeNearest( const VECTOR2I& aPoint, const VECTOR2I& aGrid, const VECTOR2I& aOffset ) const;

    /// World-space snap thresholds derived from the screen-space snap radius.
    struct SNAP_RANGES
    {
        double scale;             ///< SNAP_SCREEN_RADIUS in world units
        int    range;             ///< Snap radius, optionally clamped to the visible grid
        int    in;                ///< Distance at which a candidate is picked up
        int    out;               ///< Distance at which a held candidate is released
        double rankingHysteresis; ///< Resolver ranking stickiness, as a fraction of the radius
    };

    /**
     * Compute the snap thresholds.
     *
     * The radius is clamped to the visible grid so visible grid points always remain reachable.
     * @see https://gitlab.com/kicad/code/kicad/-/issues/5638
     * @see https://gitlab.com/kicad/code/kicad/-/issues/7125
     * @see https://gitlab.com/kicad/code/kicad/-/issues/12303
     */
    SNAP_RANGES computeSnapRanges( bool aClampToVisibleGrid ) const;

    /**
     * Emit the angle-restriction snap candidates (the two branches bracketing the cursor angle)
     * into the current frame. Shared by the derived helpers so the stringly-typed "angle" protocol
     * lives in one place.
     */
    void emitAngleBranchCandidates( std::vector<SNAP_CANDIDATE>& aCandidates, const VECTOR2I& aOrigin,
                                    double aSnapScale ) const;

    /**
     * Emit the point editor's unchanged self-geometry and the independent grid axes.
     *
     * The two editors disagree on when the grid may be used, so the caller supplies that gate
     * rather than the base guessing at it.
     */
    void emitSelfAndGridCandidates( std::vector<SNAP_CANDIDATE>& aCandidates, const SNAP_SOURCE_CONTEXT& aContext,
                                    const VECTOR2I& aOrigin, const VECTOR2I& aNearestGrid, double aSnapScale,
                                    int aSnapRange, bool aUseGrid ) const;

    SNAP_RESOLVER::TRACE_CALLBACK snapTraceCallback( const SNAP_SOURCE_CONTEXT& aContext ) const;

    /**
     * Record which snaps the resolver accepted so they can be re-biased on the next resolve, and
     * separately track the accepted angle branch for the next frame.
     */
    void retainAcceptedSnaps( const SNAP_RESULT& aResult );

protected:
    void applySnapResultGuides( const SNAP_RESULT& aResult );

    void showConstructionGeometry( bool aShow );

    SNAP_MANAGER& getSnapManager() { return m_snapManager; }

    void updateSnapPoint( const TYPED_POINT2I& aPoint );

    /**
     * Enable the anchor debug if permitted and return it
     *
     * Returns nullptr if not permitted by the advancd config
     */
    KIGFX::ANCHOR_DEBUG* enableAndGetAnchorDebug();

    std::vector<ANCHOR> m_anchors;

    TOOL_MANAGER*           m_toolMgr;
    std::optional<VECTOR2I> m_auxAxis;

    int m_maskTypes; // Mask of allowed snap types

    bool                  m_enableSnap;     // Allow snapping to other items on the layers
    bool                  m_enableGrid;     // If true, allow snapping to grid
    bool                  m_enableSnapLine; // Allow drawing lines from snap points
    std::optional<ANCHOR> m_snapItem;       // Pointer to the currently snapped item in m_anchors
                                            //   (NULL if not snapped)
    VECTOR2I m_skipPoint;                   // When drawing a line, we avoid snapping to the
                                            //   source point
    KIGFX::SNAP_INDICATOR  m_viewSnapPoint;
    KIGFX::ORIGIN_VIEWITEM m_viewAxis;

    // Manual grid parameters used when no TOOL_MANAGER is provided
    VECTOR2D m_manualGrid;
    VECTOR2D m_manualVisibleGrid;
    VECTOR2I m_manualOrigin;
    bool     m_manualGridSnapping;

    // Snap-resolver integration state shared by the derived helpers
    std::optional<VECTOR2I>             m_angleOrigin;
    double                              m_angleStepDegrees = 0.0;
    std::optional<SNAP_STABLE_ID>       m_retainedAngleBranch;
    std::vector<SNAP_STABLE_ID>         m_stickySnapIds;
    SNAP_RESOLVER::FEASIBILITY_CALLBACK m_feasibilityCallback;
    std::vector<VECTOR2I>               m_stationarySelfPoints;
    std::vector<SEG>                    m_stationarySelfSegments;
    bool                                m_pointEditProfile = false;
    SNAP_REFERENCE_PREFERENCE           m_layoutReferencePreference;

    /**
     * Classify the point a drag was started from against the moving object's bounds.
     *
     * Layout inference prefers a stationary feature matching the one the user grabbed, so a
     * left-edge grab aligns to left edges and a centre grab aligns to centres.  An anchor-point
     * grab (a pad centre or a pin end) instead prefers the stationary anchor points of the same
     * kind, which is what makes pad-to-pad and pin-to-pin alignment feel deliberate.
     *
     * @param aPoint The drag origin.
     * @param aBounds The bounds of everything being moved.
     * @param aAnchorPoint True when the drag origin is a published anchor point.
     */
    static SNAP_REFERENCE_PREFERENCE classifyReference( const VECTOR2I& aPoint, const BOX2I& aBounds,
                                                        bool aAnchorPoint );

    /// Record the classified drag reference and trace it.
    void setLayoutReference( const VECTOR2I& aPoint, const std::optional<BOX2I>& aBounds, bool aAnchorPoint );

private:
    /// Show construction geometry (if any) on the canvas.
    KIGFX::CONSTRUCTION_GEOM m_constructionGeomPreview;

    /// Manage the construction geometry, snap lines, reference points, etc.
    SNAP_MANAGER m_snapManager;

    /// #VIEW_ITEM for visualising anchor points, if enabled.
    std::unique_ptr<KIGFX::ANCHOR_DEBUG> m_anchorDebug;
};

#endif
