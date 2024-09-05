/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/construction_manager.h>


CONSTRUCTION_MANAGER::CONSTRUCTION_MANAGER( KIGFX::CONSTRUCTION_GEOM& aHelper ) :
        m_constructionGeomPreview( aHelper )
{
}

void CONSTRUCTION_MANAGER::updateView()
{
    if( m_updateCallback )
    {
        bool showAnything = m_persistentConstructionBatch || !m_temporaryConstructionBatches.empty()
                            || ( m_snapLineOrigin && m_snapLineEnd );

        m_updateCallback( showAnything );
    }
}

void CONSTRUCTION_MANAGER::AddConstructionItems( CONSTRUCTION_ITEM_BATCH aBatch,
                                                 bool                    aIsPersistent )
{
    if( aIsPersistent )
    {
        // We only keep one previous persistent batch for the moment
        m_persistentConstructionBatch = std::move( aBatch );
    }
    else
    {
        bool anyNewItems = false;
        for( CONSTRUCTION_ITEM& item : aBatch )
        {
            if( m_involvedItems.count( item.Item ) == 0 )
            {
                anyNewItems = true;
                break;
            }
        }

        // If there are no new items involved, don't bother adding the batch
        if( !anyNewItems )
        {
            return;
        }

        // We only keep up to one previous temporary batch and the current one
        // we could make this a setting if we want to keep more, but it gets cluttered
        const int maxTempItems = 2;

        while( m_temporaryConstructionBatches.size() >= maxTempItems )
        {
            m_temporaryConstructionBatches.pop_front();
        }

        m_temporaryConstructionBatches.emplace_back( std::move( aBatch ) );
    }

    // Refresh what items are drawn

    m_constructionGeomPreview.ClearDrawables();
    m_involvedItems.clear();

    const auto addBatchItems = [&]( const CONSTRUCTION_ITEM_BATCH& aBatchToAdd, bool aPersistent )
    {
        for( const CONSTRUCTION_ITEM& item : aBatchToAdd )
        {
            // Only show the item if it's not already involved
            // (avoid double-drawing the same item)
            if( m_involvedItems.count( item.Item ) == 0 )
            {
                m_involvedItems.insert( item.Item );

                for( const KIGFX::CONSTRUCTION_GEOM::DRAWABLE& construction : item.Constructions )
                {
                    m_constructionGeomPreview.AddDrawable( construction, aPersistent );
                }
            }
        }
    };

    if( m_persistentConstructionBatch )
    {
        addBatchItems( *m_persistentConstructionBatch, true );
    }

    for( const CONSTRUCTION_ITEM_BATCH& batch : m_temporaryConstructionBatches )
    {
        addBatchItems( batch, false );
    }

    updateView();
}

bool CONSTRUCTION_MANAGER::InvolvesAllGivenRealItems( const std::vector<EDA_ITEM*>& aItems ) const
{
    for( EDA_ITEM* item : aItems )
    {
        // Null items (i.e. construction items) are always considered involved
        if( item && m_involvedItems.count( item ) == 0 )
        {
            return false;
        }
    }

    return true;
}

void CONSTRUCTION_MANAGER::SetSnapLineOrigin( const VECTOR2I& aOrigin )
{
    // Setting the origin clears the snap line as the end point is no longer valid
    ClearSnapLine();
    m_snapLineOrigin = aOrigin;
}

void CONSTRUCTION_MANAGER::SetSnapLineEnd( const OPT_VECTOR2I& aSnapEnd )
{
    if( m_snapLineOrigin && aSnapEnd != m_snapLineEnd )
    {
        m_snapLineEnd = aSnapEnd;

        if( m_snapLineEnd )
            m_constructionGeomPreview.SetSnapLine( SEG{ *m_snapLineOrigin, *m_snapLineEnd } );
        else
            m_constructionGeomPreview.ClearSnapLine();

        updateView();
    }
}

void CONSTRUCTION_MANAGER::ClearSnapLine()
{
    m_snapLineOrigin.reset();
    m_snapLineEnd.reset();
    m_constructionGeomPreview.ClearSnapLine();
    updateView();
}

void CONSTRUCTION_MANAGER::SetSnappedAnchor( const VECTOR2I& aAnchorPos )
{
    if( m_snapLineOrigin )
    {
        if( aAnchorPos.x == m_snapLineOrigin->x || aAnchorPos.y == m_snapLineOrigin->y )
        {
            SetSnapLineEnd( aAnchorPos );
        }
        else
        {
            // Snapped to something that is not the snap line origin, so
            // this anchor is now the new snap line origin
            SetSnapLineOrigin( aAnchorPos );
        }
    }
    else
    {
        // If there's no snap line, start one
        m_snapLineOrigin = aAnchorPos;
    }
}

std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH>
CONSTRUCTION_MANAGER::GetConstructionItems() const
{
    std::vector<CONSTRUCTION_ITEM_BATCH> batches;

    if( m_persistentConstructionBatch )
    {
        batches.push_back( *m_persistentConstructionBatch );
    }

    for( const CONSTRUCTION_ITEM_BATCH& batch : m_temporaryConstructionBatches )
    {
        batches.push_back( batch );
    }

    if( m_snapLineOrigin )
    {
        CONSTRUCTION_ITEM_BATCH batch;

        CONSTRUCTION_ITEM& snapPointItem = batch.emplace_back( CONSTRUCTION_ITEM{
                SOURCE::FROM_SNAP_LINE,
                nullptr,
                {},
        } );

        snapPointItem.Constructions.push_back(
                LINE{ *m_snapLineOrigin, *m_snapLineOrigin + VECTOR2I( 100000, 0 ) } );
        snapPointItem.Constructions.push_back(
                LINE{ *m_snapLineOrigin, *m_snapLineOrigin + VECTOR2I( 0, 100000 ) } );

        batches.push_back( std::move( batch ) );
    }

    return batches;
}


/**
 * Check if the cursor has moved far enough away from the snap line origin to escape snapping
 * in the X direction.
 *
 * This is defined as within aEscapeRange of the snap line origin, and within aLongRangeEscapeAngle
 * of the vertical line passing through the snap line origin.
 */
static bool pointHasEscapedSnapLineX( const VECTOR2I& aCursor, const VECTOR2I& aSnapLineOrigin,
                                      int aEscapeRange, EDA_ANGLE aLongRangeEscapeAngle )
{
    if( std::abs( aCursor.x - aSnapLineOrigin.x ) < aEscapeRange )
    {
        return false;
    }
    EDA_ANGLE angle = EDA_ANGLE( aCursor - aSnapLineOrigin ) + EDA_ANGLE( 90, DEGREES_T );
    return std::abs( angle.Normalize90() ) > aLongRangeEscapeAngle;
}

/**
 * As above, but for the Y direction.
 */
static bool pointHasEscapedSnapLineY( const VECTOR2I& aCursor, const VECTOR2I& aSnapLineOrigin,
                                      int aEscapeRange, EDA_ANGLE aLongRangeEscapeAngle )
{
    if( std::abs( aCursor.y - aSnapLineOrigin.y ) < aEscapeRange )
    {
        return false;
    }
    EDA_ANGLE angle = EDA_ANGLE( aCursor - aSnapLineOrigin );
    return std::abs( angle.Normalize90() ) > aLongRangeEscapeAngle;
}


OPT_VECTOR2I CONSTRUCTION_MANAGER::GetNearestSnapLinePoint( const VECTOR2I&    aCursor,
                                                            const VECTOR2I&    aNearestGrid,
                                                            std::optional<int> aDistToNearest,
                                                            int                aSnapRange ) const
{
    // return std::nullopt;
    if( m_snapLineOrigin )
    {
        bool     snapLine = false;
        VECTOR2I bestSnapPoint = aNearestGrid;

        // If there's no snap anchor, or it's too far away, prefer the grid
        const bool gridBetterThanNearest = !aDistToNearest || *aDistToNearest > aSnapRange;

        // The escape range is how far you go before the snap line is de-activated.
        // Make this a bit more forgiving than the snap range, as you can easily cancel
        // deliberately with a mouse move.
        // These are both a bit arbitrary, and can be adjusted as preferred
        const int       escapeRange = 2 * aSnapRange;
        const EDA_ANGLE longRangeEscapeAngle( 3, DEGREES_T );

        const bool escapedX = pointHasEscapedSnapLineX( aCursor, *m_snapLineOrigin, escapeRange,
                                                        longRangeEscapeAngle );
        const bool escapedY = pointHasEscapedSnapLineY( aCursor, *m_snapLineOrigin, escapeRange,
                                                        longRangeEscapeAngle );

        /// Allows de-snapping from the line if you are closer to another snap point
        /// Or if you have moved far enough away from the line
        if( !escapedX && gridBetterThanNearest )
        {
            bestSnapPoint.x = m_snapLineOrigin->x;
            snapLine = true;
        }

        if( !escapedY && gridBetterThanNearest )
        {
            bestSnapPoint.y = m_snapLineOrigin->y;
            snapLine = true;
        }

        if( snapLine )
        {
            return bestSnapPoint;
        }
    }

    return std::nullopt;
}