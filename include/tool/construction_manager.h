/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <preview_items/construction_geom.h>

template <typename T>
class ACTIVATION_HELPER;
class EDA_ITEM;


/**
 * Interface wrapper for the construction geometry preview with a callback to signal the
 * view owner that the view needs to be updated.
 */
class CONSTRUCTION_VIEW_HANDLER
{
public:
    CONSTRUCTION_VIEW_HANDLER( KIGFX::CONSTRUCTION_GEOM& aHelper ) :
            m_constructionGeomPreview( aHelper )
    {
    }

    virtual void updateView() = 0;

    KIGFX::CONSTRUCTION_GEOM& GetViewItem() { return m_constructionGeomPreview; }

private:
    // An (external) construction helper view item, that this manager adds/removes
    // construction objects to/from.
    KIGFX::CONSTRUCTION_GEOM& m_constructionGeomPreview;
};


/**
 * A class that manages the geometry of a "snap line".
 *
 * This is a line that has a start point (the "snap origin") and an end point (the "snap end").
 * The end can only be set if the origin is set. If the origin is set, the end will be unset.
 */
class SNAP_MANAGER;


class SNAP_LINE_MANAGER
{
public:
    SNAP_LINE_MANAGER( CONSTRUCTION_VIEW_HANDLER& aViewHandler );

    /**
     * The snap point is a special point that is located at the last point the cursor
     * snapped to.
     *
     * If it is set, the construction manager may add extra construction geometry to the helper
     * extending from the snap point origin to the cursor, which is the 'snap line'.
     */
    void SetSnapLineOrigin( const VECTOR2I& aOrigin );

    /**
     * Set the end point of the snap line.
     *
     * Passing std::nullopt will unset the end point, but keep the origin.
     */
    void SetSnapLineEnd( const OPT_VECTOR2I& aSnapPoint );

    /**
     * Clear the snap line origin and end points.
     */
    void ClearSnapLine();

    const OPT_VECTOR2I& GetSnapLineOrigin() const { return m_snapLineOrigin; }

    bool HasCompleteSnapLine() const { return m_snapLineOrigin && m_snapLineEnd; }

    /**
     * Inform this manager that an anchor snap has been made.
     *
     * This will also update the start or end of the snap line as appropriate.
     */
    void SetSnappedAnchor( const VECTOR2I& aAnchorPos );

    /**
     * If the snap line is active, return the best snap point that is closest to the cursor.
     *
     * If there's no active snap line, return std::nullopt.
     *
     * If there's a snap very near, use that otherwise, use the grid point.
     * With this point, snap to it on an H/V axis.
     *
     * Then, if there's a grid point near, snap to it on an H/V axis.
     *
     * @param aCursor The cursor position.
     * @param aNearestGrid The nearest grid point to the cursor.
     * @param aDistToNearest The distance to the nearest non-grid snap point, if any.
     * @param snapRange The snap range.
     * @param aGridSize The grid size (for snapping to grid intersections).
     * @param aGridOrigin The grid origin (for snapping to grid intersections).
     */
    OPT_VECTOR2I GetNearestSnapLinePoint( const VECTOR2I& aCursor, const VECTOR2I& aNearestGrid,
                                          std::optional<int> aDistToNearest, int snapRange,
                                          const VECTOR2D& aGridSize = VECTOR2D( 0, 0 ),
                                          const VECTOR2I& aGridOrigin = VECTOR2I( 0, 0 ) ) const;

    void SetDirections( const std::vector<VECTOR2I>& aDirections );

    const std::vector<VECTOR2I>& GetDirections() const { return m_directions; }

    std::optional<int> GetActiveDirection() const { return m_activeDirection; }

private:
    // If a snap point is "active", extra construction geometry is added to the helper
    // extending from the snap point to the cursor.
    OPT_VECTOR2I m_snapLineOrigin;
    OPT_VECTOR2I m_snapLineEnd;

    std::vector<VECTOR2I> m_directions;
    std::optional<int>    m_activeDirection;

    // The view handler to update when the snap line changes
    CONSTRUCTION_VIEW_HANDLER& m_viewHandler;
    SNAP_MANAGER*               m_snapManager;

    void notifyGuideChange();
};


/**
 * A class that manages "construction" objects and geometry.
 *
 * These are things like line extensions, arc centers, etc.
 */
class CONSTRUCTION_MANAGER
{
public:
    CONSTRUCTION_MANAGER( CONSTRUCTION_VIEW_HANDLER& aViewHandler );
    ~CONSTRUCTION_MANAGER();

    enum class SOURCE
    {
        FROM_ITEMS,
        FROM_SNAP_LINE,
    };

    /**
     * Items to be used for the construction of "virtual" anchors, for example, when snapping to
     * a point involving an _extension_ of an existing line or arc.
     *
     * One item can have multiple construction items (e.g. an arc can have a circle and centre
     * point).
     */
    struct CONSTRUCTION_ITEM
    {
        SOURCE    Source;
        EDA_ITEM* Item;

        struct DRAWABLE_ENTRY
        {
            KIGFX::CONSTRUCTION_GEOM::DRAWABLE Drawable;
            int                               LineWidth = 1;
        };

        std::vector<DRAWABLE_ENTRY> Constructions;
    };

    // A single batch of construction items. Once batch contains all the items (and associated
    // construction geometry) that should be shown for one point of interest.
    // More than one batch may be shown on the screen at the same time.
    using CONSTRUCTION_ITEM_BATCH = std::vector<CONSTRUCTION_ITEM>;

    /**
     * Add a batch of construction items to the helper.
     *
     * @param aBatch The batch of construction items to add.
     * @param aIsPersistent If true, the batch is considered "persistent" and will always be shown
     *                      (and it will replace any previous persistent batch).
     *                      If false, the batch is temporary and may be pushed out by other batches.
     */
    void ProposeConstructionItems( std::unique_ptr<CONSTRUCTION_ITEM_BATCH> aBatch,
                                   bool                                     aIsPersistent );

    /**
     * Cancel outstanding proposals for new geometry.
     */
    void CancelProposal();

    /**
     * Clear all construction items.
     */
    void Clear();

    /**
     * Check if all 'real' (non-null = constructed) the items in the batch are in the list of items
     * currently 'involved' in an active construction.
     */
    bool InvolvesAllGivenRealItems( const std::vector<EDA_ITEM*>& aItems ) const;

    /**
     * Get the list of additional geometry items that should be considered.
     */
    void GetConstructionItems( std::vector<CONSTRUCTION_ITEM_BATCH>& aToExtend ) const;

    bool HasActiveConstruction() const;

private:
    struct PENDING_BATCH;

    void acceptConstructionItems( std::unique_ptr<PENDING_BATCH> aAcceptedBatchHash );

    /**
     * How many batches of temporary construction items can be active at once.
     *
     * This is to prevent too much clutter.
     */
    unsigned getMaxTemporaryBatches() const;

    CONSTRUCTION_VIEW_HANDLER& m_viewHandler;

    /// Within one "operation", there is one set of construction items that are
    /// "persistent", and are always shown. Usually the original item and any
    /// extensions.
    std::optional<CONSTRUCTION_ITEM_BATCH> m_persistentConstructionBatch;

    /// Temporary construction items are added and removed as needed.
    std::deque<CONSTRUCTION_ITEM_BATCH> m_temporaryConstructionBatches;

    /// Set of all items for which construction geometry has been added.
    std::set<EDA_ITEM*> m_involvedItems;

    std::unique_ptr<ACTIVATION_HELPER<std::unique_ptr<PENDING_BATCH>>> m_activationHelper;

    /// Protects the persistent and temporary construction batches.
    mutable std::mutex m_batchesMutex;
};


/**
 * A SNAP_MANAGER glues together the snap line manager and construction manager.,
 * along with some other state. It provides information for generating snap
 * anchors based on this state, as well as keeping the state of visible
 * construction geometry involved in that process.
 *
 * Probably only used by GRID_HELPERs, but it's neater to keep it separate,
 * as there's quite a bit of state to manage.
 *
 * This is also where you may wish to add other 'virtual' snapping state,
 * such as 'equal-space' snapping, etc.
 */
class SNAP_MANAGER : public CONSTRUCTION_VIEW_HANDLER
{
public:
    using GFX_UPDATE_CALLBACK = std::function<void( bool aShowAnything )>;

    SNAP_MANAGER( KIGFX::CONSTRUCTION_GEOM& aHelper );

    /**
     * Set the callback to call when the construction geometry changes and a view may need updating.
     */
    void SetUpdateCallback( GFX_UPDATE_CALLBACK aCallback ) { m_updateCallback = aCallback; }

    SNAP_LINE_MANAGER& GetSnapLineManager() { return m_snapLineManager; }
    const SNAP_LINE_MANAGER& GetSnapLineManager() const { return m_snapLineManager; }

    CONSTRUCTION_MANAGER& GetConstructionManager() { return m_constructionManager; }

    /**
     * Set the reference-only points - these are points that are not snapped to, but can still
     * be used for connection to the snap line.
     */
    void SetReferenceOnlyPoints( std::vector<VECTOR2I> aPoints )
    {
        m_referenceOnlyPoints = std::move( aPoints );
    }

    const std::vector<VECTOR2I>& GetReferenceOnlyPoints() const { return m_referenceOnlyPoints; }

    /**
     * Get a list of all the active construction geometry, computed from
     * the combined state of the snap line and construction manager.
     *
     * This can be combined with other external geometry to compute snap anchors.
     */
    std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH> GetConstructionItems() const;

    void SetSnapGuideColors( const KIGFX::COLOR4D& aBase, const KIGFX::COLOR4D& aHighlight );
    void UpdateSnapGuides();

    void Clear();

public:
    void updateView() override;

    GFX_UPDATE_CALLBACK m_updateCallback;

    SNAP_LINE_MANAGER    m_snapLineManager;
    CONSTRUCTION_MANAGER m_constructionManager;

    std::vector<VECTOR2I> m_referenceOnlyPoints;
    KIGFX::COLOR4D        m_snapGuideColor;
    KIGFX::COLOR4D        m_snapGuideHighlightColor;
};
