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

#ifndef GRID_HELPER_H
#define GRID_HELPER_H

#include <vector>
#include <optional>

#include <geometry/point_types.h>
#include <math/vector2d.h>
#include <preview_items/anchor_debug.h>
#include <preview_items/snap_indicator.h>
#include <preview_items/construction_geom.h>
#include <tool/construction_manager.h>
#include <tool/selection.h>
#include <origin_viewitem.h>

class TOOL_MANAGER; // Forward declaration to avoid hard dependency in tests

class EDA_ITEM;

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

    VECTOR2I computeNearest( const VECTOR2I& aPoint, const VECTOR2I& aGrid,
                             const VECTOR2I& aOffset ) const;

protected:
    void showConstructionGeometry( bool aShow );

    SNAP_MANAGER& getSnapManager() { return m_snapManager; }

    void updateSnapPoint( const TYPED_POINT2I& aPoint );

    /**
     * Enable the anchor debug if permitted and return it
     *
     * Returns nullptr if not permitted by the advancd config
     */
    KIGFX::ANCHOR_DEBUG* enableAndGetAnchorDebug();

    std::vector<ANCHOR>     m_anchors;

    TOOL_MANAGER*           m_toolMgr;
    std::optional<VECTOR2I> m_auxAxis;

    int                     m_maskTypes;      // Mask of allowed snap types

    bool                    m_enableSnap;     // Allow snapping to other items on the layers
    bool                    m_enableGrid;     // If true, allow snapping to grid
    bool                    m_enableSnapLine; // Allow drawing lines from snap points
    std::optional<ANCHOR>   m_snapItem;       // Pointer to the currently snapped item in m_anchors
                                              //   (NULL if not snapped)
    VECTOR2I                m_skipPoint;      // When drawing a line, we avoid snapping to the
                                              //   source point
    KIGFX::SNAP_INDICATOR   m_viewSnapPoint;
    KIGFX::ORIGIN_VIEWITEM  m_viewAxis;

    // Manual grid parameters used when no TOOL_MANAGER is provided
    VECTOR2D                m_manualGrid;
    VECTOR2D                m_manualVisibleGrid;
    VECTOR2I                m_manualOrigin;
    bool                    m_manualGridSnapping;

private:
    /// Show construction geometry (if any) on the canvas.
    KIGFX::CONSTRUCTION_GEOM m_constructionGeomPreview;

    /// Manage the construction geometry, snap lines, reference points, etc.
    SNAP_MANAGER m_snapManager;

    /// #VIEW_ITEM for visualising anchor points, if enabled.
    std::unique_ptr<KIGFX::ANCHOR_DEBUG> m_anchorDebug;
};

#endif
