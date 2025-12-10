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

#include "tool/construction_manager.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

#include <wx/timer.h>
#include <wx/debug.h>
#include <wx/log.h>

#include <advanced_config.h>
#include <math/util.h>
#include <hash.h>
#include <trace_helpers.h>


/**
 * A helper class to manage the activation of a "proposal" after a timeout.
 *
 * When a proposal is made, a timer starts. If no new proposal is made and the proposal
 * is not canceled before the timer expires, the proposal is "accepted" via a callback.
 *
 * Proposals are "tagged" with a hash - this is used to avoid reproposing the same thing
 * multiple times.
 *
 * @tparam T The type of the proposal, which will be passed to the callback (by value)
 */
template <typename T>
class ACTIVATION_HELPER
{
public:
    using ACTIVATION_CALLBACK = std::function<void( T&& )>;

    ACTIVATION_HELPER( std::chrono::milliseconds aTimeout, ACTIVATION_CALLBACK aCallback ) :
            m_timeout( aTimeout ),
            m_callback( std::move( aCallback ) )
    {
        m_timer.Bind( wxEVT_TIMER, &ACTIVATION_HELPER::onTimerExpiry, this );
    }

    ~ACTIVATION_HELPER()
    {
        // Hold the lock while shutting down to prevent a propoal being accepted
        // while state is being destroyed.
        std::unique_lock<std::mutex> lock( m_mutex );
        m_timer.Stop();
        m_timer.Unbind( wxEVT_TIMER, &ACTIVATION_HELPER::onTimerExpiry, this );

        // Should be redundant to inhibiting timer callbacks, but make it explicit.
        m_pendingProposalTag.reset();
    }

    void ProposeActivation( T&& aProposal, std::size_t aProposalTag, bool aAcceptImmediately )
    {
        std::unique_lock<std::mutex> lock( m_mutex );

        if( m_lastAcceptedProposalTag.has_value() && aProposalTag == *m_lastAcceptedProposalTag )
        {
            // This proposal was accepted last time
            // (could be made optional if we want to allow re-accepting the same proposal)
            return;
        }

        if( m_pendingProposalTag.has_value() && aProposalTag == *m_pendingProposalTag )
        {
            // This proposal is already pending
            return;
        }

        m_pendingProposalTag = aProposalTag;
        m_lastProposal = std::move( aProposal );

        if( aAcceptImmediately )
        {
            // Synchonously accept the proposal
            lock.unlock();
            acceptPendingProposal();
        }
        else
        {
            m_timer.Start( m_timeout.count(), wxTIMER_ONE_SHOT );
        }
    }

    void CancelProposal()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_pendingProposalTag.reset();
        m_timer.Stop();
    }

private:
    /**
     * Timer expiry callback in the UI thread.
     */
    void onTimerExpiry( wxTimerEvent& aEvent )
    {
        acceptPendingProposal();
    }

    void acceptPendingProposal()
    {
        std::unique_lock<std::mutex> lock( m_mutex );

        if( m_pendingProposalTag )
        {
            m_lastAcceptedProposalTag = m_pendingProposalTag;
            m_pendingProposalTag.reset();

            // Move out from the locked variable
            T proposalToAccept = std::move( m_lastProposal );
            lock.unlock();

            // Call the callback (outside the lock)
            // This is all in the UI thread now, so it won't be concurrent
            m_callback( std::move( proposalToAccept ) );
        }
    }

    mutable std::mutex m_mutex;

    /// Activation timeout in milliseconds.
    std::chrono::milliseconds m_timeout;

    /// The last proposal tag that was made.
    std::optional<std::size_t> m_pendingProposalTag;

    /// The last proposal that was accepted.
    std::optional<std::size_t> m_lastAcceptedProposalTag;

    /// The most recently-proposed item.
    T m_lastProposal;

    /// Callback to call when the proposal is accepted.
    ACTIVATION_CALLBACK m_callback;

    wxTimer m_timer;
};


struct CONSTRUCTION_MANAGER::PENDING_BATCH
{
    CONSTRUCTION_ITEM_BATCH Batch;
    bool                    IsPersistent;
};


CONSTRUCTION_MANAGER::CONSTRUCTION_MANAGER( CONSTRUCTION_VIEW_HANDLER& aHelper ) :
        m_viewHandler( aHelper ),
        m_persistentConstructionBatch(),
        m_temporaryConstructionBatches(),
        m_involvedItems(),
        m_batchesMutex()
{
    const std::chrono::milliseconds acceptanceTimeout(
            ADVANCED_CFG::GetCfg().m_ExtensionSnapTimeoutMs );

    m_activationHelper = std::make_unique<ACTIVATION_HELPER<std::unique_ptr<PENDING_BATCH>>>(
            acceptanceTimeout,
            [this]( std::unique_ptr<PENDING_BATCH>&& aAccepted )
            {
                // This shouldn't be possible (probably indicates a race in destruction of something)
                // but at least avoid blowing up acceptConstructionItems.
                wxCHECK_MSG( aAccepted != nullptr, void(), "Null proposal accepted" );

                acceptConstructionItems( std::move( aAccepted ) );
            } );
}


CONSTRUCTION_MANAGER::~CONSTRUCTION_MANAGER()
{
}


/**
 * Construct a hash based on the sources of the items in the batch.
 */
static std::size_t
HashConstructionBatchSources( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH& aBatch,
                              bool                                                 aIsPersistent )
{
    std::size_t hash = hash_val( aIsPersistent );

    for( const CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM& item : aBatch )
    {
        hash_combine( hash, item.Source, item.Item );
    }
    return hash;
}


void CONSTRUCTION_MANAGER::ProposeConstructionItems(
        std::unique_ptr<CONSTRUCTION_ITEM_BATCH> aBatch, bool aIsPersistent )
{
    if( aBatch->empty() )
    {
        // There's no point in proposing an empty batch
        // It would just clear existing construction items for nothing new
        return;
    }

    bool acceptImmediately = false;

    {
        std::lock_guard<std::mutex> lock( m_batchesMutex );

        if( aIsPersistent )
        {
            acceptImmediately = true;
        }
        else
        {
            // If the batch is temporary, we can accept it immediately if there's room
            acceptImmediately = m_temporaryConstructionBatches.size() < getMaxTemporaryBatches();
        }
    }

    auto pendingBatch =
            std::make_unique<PENDING_BATCH>( PENDING_BATCH{ std::move( *aBatch ), aIsPersistent } );
    const std::size_t hash = HashConstructionBatchSources( pendingBatch->Batch, aIsPersistent );

    // Immediate or not, propose the batch via the activation helper as this handles duplicates
    m_activationHelper->ProposeActivation( std::move( pendingBatch ), hash, acceptImmediately );
}


void CONSTRUCTION_MANAGER::CancelProposal()
{
    m_activationHelper->CancelProposal();
}


unsigned CONSTRUCTION_MANAGER::getMaxTemporaryBatches() const
{
    // We only keep up to one previous temporary batch and the current one
    // we could make this a setting if we want to keep more, but it gets cluttered
    return 2;
}


void CONSTRUCTION_MANAGER::acceptConstructionItems( std::unique_ptr<PENDING_BATCH> aAcceptedBatch )
{
    const auto getInvolved = [&]( const CONSTRUCTION_ITEM_BATCH& aBatchToAdd )
    {
        for( const CONSTRUCTION_ITEM& item : aBatchToAdd )
        {
            // Only show the item if it's not already involved
            // (avoid double-drawing the same item)
            if( m_involvedItems.count( item.Item ) == 0 )
            {
                m_involvedItems.insert( item.Item );
            }
        }
    };

    // Copies for use outside the lock
    std::vector<CONSTRUCTION_ITEM_BATCH> persistentBatches, temporaryBatches;
    {
        std::lock_guard<std::mutex> lock( m_batchesMutex );

        if( aAcceptedBatch->IsPersistent )
        {
            // We only keep one previous persistent batch for the moment
            m_persistentConstructionBatch = std::move( aAcceptedBatch->Batch );
        }
        else
        {
            bool anyNewItems = false;
            for( CONSTRUCTION_ITEM& item : aAcceptedBatch->Batch )
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

            while( m_temporaryConstructionBatches.size() >= getMaxTemporaryBatches() )
            {
                m_temporaryConstructionBatches.pop_front();
            }

            m_temporaryConstructionBatches.emplace_back( std::move( aAcceptedBatch->Batch ) );
        }

        m_involvedItems.clear();

        // Copy the batches for use outside the lock
        if( m_persistentConstructionBatch )
        {
            getInvolved( *m_persistentConstructionBatch );
            persistentBatches.push_back( *m_persistentConstructionBatch );
        }

        for( const CONSTRUCTION_ITEM_BATCH& batch : m_temporaryConstructionBatches )
        {
            getInvolved( batch );
            temporaryBatches.push_back( batch );
        }
    }

    KIGFX::CONSTRUCTION_GEOM& geom = m_viewHandler.GetViewItem();
    geom.ClearDrawables();

    const auto addDrawables =
            [&]( const std::vector<CONSTRUCTION_ITEM_BATCH>& aBatches, bool aIsPersistent )
    {
        for( const CONSTRUCTION_ITEM_BATCH& batch : aBatches )
        {
            for( const CONSTRUCTION_ITEM& item : batch )
            {
                for( const CONSTRUCTION_ITEM::DRAWABLE_ENTRY& drawable : item.Constructions )
                {
                    geom.AddDrawable( drawable.Drawable, aIsPersistent, drawable.LineWidth );
                }
            }
        }
    };

    addDrawables( persistentBatches, true );
    addDrawables( temporaryBatches, false );

    m_viewHandler.updateView();
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


void CONSTRUCTION_MANAGER::GetConstructionItems(
        std::vector<CONSTRUCTION_ITEM_BATCH>& aToExtend ) const
{
    std::lock_guard<std::mutex> lock( m_batchesMutex );
    if( m_persistentConstructionBatch )
    {
        aToExtend.push_back( *m_persistentConstructionBatch );
    }

    for( const CONSTRUCTION_ITEM_BATCH& batch : m_temporaryConstructionBatches )
    {
        aToExtend.push_back( batch );
    }
}


bool CONSTRUCTION_MANAGER::HasActiveConstruction() const
{
    std::lock_guard<std::mutex> lock( m_batchesMutex );
    return m_persistentConstructionBatch.has_value() || !m_temporaryConstructionBatches.empty();
}


SNAP_LINE_MANAGER::SNAP_LINE_MANAGER( CONSTRUCTION_VIEW_HANDLER& aViewHandler ) :
        m_viewHandler( aViewHandler ), m_snapManager( static_cast<SNAP_MANAGER*>( &aViewHandler ) )
{
    wxASSERT( m_snapManager );
    SetDirections( { VECTOR2I( 1, 0 ), VECTOR2I( 0, 1 ) } );
}


static VECTOR2I normalizeDirection( const VECTOR2I& aDir )
{
    if( aDir.x == 0 && aDir.y == 0 )
        return VECTOR2I( 0, 0 );

    int dx = aDir.x;
    int dy = aDir.y;

    int gcd = std::gcd( std::abs( dx ), std::abs( dy ) );

    if( gcd > 0 )
    {
        dx /= gcd;
        dy /= gcd;
    }

    if( dx < 0 || ( dx == 0 && dy < 0 ) )
    {
        dx = -dx;
        dy = -dy;
    }

    return VECTOR2I( dx, dy );
}


static std::optional<int> findDirectionIndex( const std::vector<VECTOR2I>& aDirections,
                                              const VECTOR2I&               aDelta )
{
    VECTOR2I normalized = normalizeDirection( aDelta );

    if( normalized.x == 0 && normalized.y == 0 )
        return std::nullopt;

    for( size_t i = 0; i < aDirections.size(); ++i )
    {
        if( aDirections[i] == normalized )
            return static_cast<int>( i );
    }

    return std::nullopt;
}


void SNAP_LINE_MANAGER::SetDirections( const std::vector<VECTOR2I>& aDirections )
{
    std::vector<VECTOR2I> uniqueDirections;
    uniqueDirections.reserve( aDirections.size() );

    for( const VECTOR2I& direction : aDirections )
    {
        VECTOR2I normalized = normalizeDirection( direction );

        if( normalized.x == 0 && normalized.y == 0 )
            continue;

        if( std::find( uniqueDirections.begin(), uniqueDirections.end(), normalized )
                == uniqueDirections.end() )
        {
            uniqueDirections.push_back( normalized );
        }
    }

    if( uniqueDirections != m_directions )
    {
        m_directions = std::move( uniqueDirections );
        m_activeDirection.reset();

        if( m_snapLineOrigin && m_snapLineEnd )
        {
            if( !findDirectionIndex( m_directions, *m_snapLineEnd - *m_snapLineOrigin ) )
                m_snapLineEnd.reset();
        }

        if( m_directions.empty() )
        {
            ClearSnapLine();
            return;
        }

        notifyGuideChange();
    }
}


void SNAP_LINE_MANAGER::SetSnapLineOrigin( const VECTOR2I& aOrigin )
{
    if( m_snapLineOrigin && *m_snapLineOrigin == aOrigin && !m_snapLineEnd )
    {
        notifyGuideChange();
        return;
    }

    m_snapLineOrigin = aOrigin;
    m_snapLineEnd.reset();
    m_activeDirection.reset();
    m_viewHandler.GetViewItem().ClearSnapLine();
    notifyGuideChange();
}


void SNAP_LINE_MANAGER::SetSnapLineEnd( const OPT_VECTOR2I& aSnapEnd )
{
    if( m_snapLineOrigin && aSnapEnd != m_snapLineEnd )
    {
        m_snapLineEnd = aSnapEnd;

        if( m_snapLineEnd )
            m_activeDirection = findDirectionIndex( m_directions, *m_snapLineEnd - *m_snapLineOrigin );
        else
            m_activeDirection.reset();

        if( m_snapLineEnd )
            m_viewHandler.GetViewItem().SetSnapLine( SEG{ *m_snapLineOrigin, *m_snapLineEnd } );
        else
            m_viewHandler.GetViewItem().ClearSnapLine();

        notifyGuideChange();
    }
}


void SNAP_LINE_MANAGER::ClearSnapLine()
{
    m_snapLineOrigin.reset();
    m_snapLineEnd.reset();
    m_activeDirection.reset();
    m_viewHandler.GetViewItem().ClearSnapLine();
    notifyGuideChange();
}


void SNAP_LINE_MANAGER::SetSnappedAnchor( const VECTOR2I& aAnchorPos )
{
    if( m_snapLineOrigin.has_value() )
    {
        if( findDirectionIndex( m_directions, aAnchorPos - *m_snapLineOrigin ) )
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
        SetSnapLineOrigin( aAnchorPos );
    }
}


OPT_VECTOR2I SNAP_LINE_MANAGER::GetNearestSnapLinePoint( const VECTOR2I&    aCursor,
                                                        const VECTOR2I&    aNearestGrid,
                                                        std::optional<int> aDistToNearest,
                                                        int                aSnapRange,
                                                        const VECTOR2D&    aGridSize,
                                                        const VECTOR2I&    aGridOrigin ) const
{
    wxLogTrace( traceSnap, "GetNearestSnapLinePoint: cursor=(%d, %d), nearestGrid=(%d, %d), distToNearest=%s, snapRange=%d",
                aCursor.x, aCursor.y, aNearestGrid.x, aNearestGrid.y,
                aDistToNearest ? wxString::Format( "%d", *aDistToNearest ) : wxString( "none" ), aSnapRange );

    if( !m_snapLineOrigin || m_directions.empty() )
    {
        wxLogTrace( traceSnap, "  No snap line origin or no directions, returning nullopt" );
        return std::nullopt;
    }

    const bool gridBetterThanNearest = !aDistToNearest || *aDistToNearest > aSnapRange;
    const bool gridActive = aGridSize.x > 0 && aGridSize.y > 0;

    wxLogTrace( traceSnap, "  snapLineOrigin=(%d, %d), directions count=%zu, gridBetterThanNearest=%d, gridActive=%d",
                m_snapLineOrigin->x, m_snapLineOrigin->y, m_directions.size(), gridBetterThanNearest, gridActive );

    if( !gridBetterThanNearest )
    {
        wxLogTrace( traceSnap, "  Grid not better than nearest, returning nullopt" );
        return std::nullopt;
    }

    const int       escapeRange = 2 * aSnapRange;
    const EDA_ANGLE longRangeEscapeAngle( 4, DEGREES_T );

    wxLogTrace( traceSnap, "  escapeRange=%d, longRangeEscapeAngle=%.1f deg",
                escapeRange, longRangeEscapeAngle.AsDegrees() );

    const VECTOR2D origin( *m_snapLineOrigin );
    const VECTOR2D cursor( aCursor );
    const VECTOR2D delta = cursor - origin;

    double                        bestPerpDistance = std::numeric_limits<double>::max();
    std::optional<VECTOR2I>       bestSnapPoint;

    for( size_t ii = 0; ii < m_directions.size(); ++ii )
    {
        const VECTOR2I& direction = m_directions[ii];
        VECTOR2D dirVector( direction );
        double   dirLength = dirVector.EuclideanNorm();

        if( dirLength == 0.0 )
        {
            wxLogTrace( traceSnap, "    Direction %zu: zero length, skipping", ii );
            continue;
        }

        VECTOR2D dirUnit = dirVector / dirLength;

        double    distanceAlong = delta.Dot( dirUnit );
        VECTOR2D  projection = origin + dirUnit * distanceAlong;
        VECTOR2D  offset = delta - dirUnit * distanceAlong;
        double    perpDistance = offset.EuclideanNorm();

        wxLogTrace( traceSnap, "    Direction %zu: dir=(%d, %d), perpDist=%.1f, distAlong=%.1f",
                    ii, direction.x, direction.y, perpDistance, distanceAlong );

        if( perpDistance > aSnapRange )
        {
            wxLogTrace( traceSnap, "      perpDistance > snapRange, skipping" );
            continue;
        }

        bool escaped = false;

        if( perpDistance >= escapeRange )
        {
            EDA_ANGLE deltaAngle( delta );
            EDA_ANGLE directionAngle( dirVector );
            double    angleDiff = ( deltaAngle - directionAngle ).Normalize180().AsDegrees();

            wxLogTrace( traceSnap, "      In escape range: deltaAngle=%.1f, dirAngle=%.1f, angleDiff=%.1f",
                        deltaAngle.AsDegrees(), directionAngle.AsDegrees(), angleDiff );

            if( std::abs( angleDiff ) > longRangeEscapeAngle.AsDegrees() )
            {
                escaped = true;
                wxLogTrace( traceSnap, "      ESCAPED (angle diff too large)" );
            }
        }

        if( escaped )
        {
            wxLogTrace( traceSnap, "      Not updating (escaped)" );
            continue;
        }

        // Now snap the projection to the grid if the grid is active
        VECTOR2D snapPoint = projection;

        if( gridActive )
        {
            // For horizontal/vertical lines, snap to grid intersections
            if( direction.x == 0 && direction.y != 0 )
            {
                // Vertical line: keep origin X, snap Y to grid
                snapPoint.x = origin.x;
                snapPoint.y = aNearestGrid.y;
                wxLogTrace( traceSnap, "      Vertical line: snapping to grid Y, snapPoint=(%.1f, %.1f)",
                            snapPoint.x, snapPoint.y );
            }
            else if( direction.y == 0 && direction.x != 0 )
            {
                // Horizontal line: snap X to grid, keep origin Y
                snapPoint.x = aNearestGrid.x;
                snapPoint.y = origin.y;
                wxLogTrace( traceSnap, "      Horizontal line: snapping to grid X, snapPoint=(%.1f, %.1f)",
                            snapPoint.x, snapPoint.y );
            }
            else
            {
                // Diagonal line: find nearest grid intersection along the line
                VECTOR2D gridOriginD( aGridOrigin );
                VECTOR2D relProjection = projection - gridOriginD;

                // Find nearby grid points (check 3x3 grid around projection)
                double   bestGridScore = std::numeric_limits<double>::max();
                VECTOR2D bestGridPoint = projection;

                for( int dx = -1; dx <= 1; ++dx )
                {
                    for( int dy = -1; dy <= 1; ++dy )
                    {
                        double gridX = std::round( relProjection.x / aGridSize.x ) * aGridSize.x + dx * aGridSize.x;
                        double gridY = std::round( relProjection.y / aGridSize.y ) * aGridSize.y + dy * aGridSize.y;
                        VECTOR2D gridPt( gridX + gridOriginD.x, gridY + gridOriginD.y );

                        // Calculate perpendicular distance from grid point to construction line
                        VECTOR2D gridDelta = gridPt - origin;
                        double   gridDistAlong = gridDelta.Dot( dirUnit );
                        VECTOR2D gridProjection = origin + dirUnit * gridDistAlong;
                        double   gridPerpDist = ( gridPt - gridProjection ).EuclideanNorm();

                        // Also consider distance from cursor
                        double distFromCursor = ( gridPt - cursor ).EuclideanNorm();

                        // Prefer grid points that are close to the line and close to cursor
                        double score = gridPerpDist + distFromCursor * 0.1;

                        if( score < bestGridScore )
                        {
                        bestGridScore = score;
                            bestGridPoint = gridPt;
                        }
                    }
                }

                snapPoint = bestGridPoint;
                wxLogTrace( traceSnap, "      Diagonal line: snapping to grid intersection, snapPoint=(%.1f, %.1f)",
                            snapPoint.x, snapPoint.y );
            }
        }
        else
        {
            wxLogTrace( traceSnap, "      Grid not active, using projection" );
        }

        if( perpDistance < bestPerpDistance )
        {
            bestPerpDistance = perpDistance;
            bestSnapPoint = KiROUND( snapPoint );
            wxLogTrace( traceSnap, "      NEW BEST: perpDist=%.1f, snapPoint=(%d, %d)",
                        bestPerpDistance, bestSnapPoint->x, bestSnapPoint->y );
        }
        else
        {
            wxLogTrace( traceSnap, "      Not updating (perpDist=%.1f >= bestPerp=%.1f)",
                        perpDistance, bestPerpDistance );
        }
    }

    if( bestSnapPoint )
    {
        wxLogTrace( traceSnap, "  RETURNING bestSnapPoint=(%d, %d)", bestSnapPoint->x, bestSnapPoint->y );
        return *bestSnapPoint;
    }

    wxLogTrace( traceSnap, "  RETURNING nullopt (no valid snap found)" );
    return std::nullopt;
}


SNAP_MANAGER::SNAP_MANAGER( KIGFX::CONSTRUCTION_GEOM& aHelper ) :
        CONSTRUCTION_VIEW_HANDLER( aHelper ), m_snapLineManager( *this ),
        m_constructionManager( *this ), m_snapGuideColor( KIGFX::COLOR4D::WHITE ),
        m_snapGuideHighlightColor( KIGFX::COLOR4D::WHITE )
{
}


void SNAP_MANAGER::updateView()
{
    if( m_updateCallback )
    {
        bool showAnything = m_constructionManager.HasActiveConstruction()
                            || m_snapLineManager.HasCompleteSnapLine()
                            || ( m_snapLineManager.GetSnapLineOrigin()
                                 && !m_snapLineManager.GetDirections().empty() );

        m_updateCallback( showAnything );
    }
}


void SNAP_MANAGER::SetSnapGuideColors( const KIGFX::COLOR4D& aBase, const KIGFX::COLOR4D& aHighlight )
{
    m_snapGuideColor = aBase;
    m_snapGuideHighlightColor = aHighlight;
    UpdateSnapGuides();
}


void SNAP_MANAGER::UpdateSnapGuides()
{
    std::vector<KIGFX::CONSTRUCTION_GEOM::SNAP_GUIDE> guides;

    const OPT_VECTOR2I& origin = m_snapLineManager.GetSnapLineOrigin();
    const std::vector<VECTOR2I>& directions = m_snapLineManager.GetDirections();

    if( origin && !directions.empty() )
    {
        const std::optional<int> activeDirection = m_snapLineManager.GetActiveDirection();
        const int                 guideLength = 500000;

        for( size_t ii = 0; ii < directions.size(); ++ii )
        {
            const VECTOR2I& direction = directions[ii];

            if( direction.x == 0 && direction.y == 0 )
                continue;

            VECTOR2I scaled = direction * guideLength;

            KIGFX::CONSTRUCTION_GEOM::SNAP_GUIDE guide;
            guide.Segment = SEG( *origin - scaled, *origin + scaled );

            if( activeDirection && *activeDirection == static_cast<int>( ii ) )
            {
                guide.LineWidth = 5;
                guide.Color = m_snapGuideHighlightColor;
            }
            else
            {
                guide.LineWidth = 1;
                guide.Color = m_snapGuideColor;
            }

            guides.push_back( guide );
        }
    }

    GetViewItem().SetSnapGuides( std::move( guides ) );
    updateView();
}


void SNAP_LINE_MANAGER::notifyGuideChange()
{
    if( m_snapManager )
        m_snapManager->UpdateSnapGuides();
}


std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH>
SNAP_MANAGER::GetConstructionItems() const
{
    std::vector<CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH> batches;

    m_constructionManager.GetConstructionItems( batches );

    if( const OPT_VECTOR2I& snapLineOrigin = m_snapLineManager.GetSnapLineOrigin();
        snapLineOrigin.has_value() )
    {
        CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM_BATCH batch;

        CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM& snapPointItem =
                batch.emplace_back( CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM{
                        CONSTRUCTION_MANAGER::SOURCE::FROM_SNAP_LINE,
                        nullptr,
                        {},
                } );

        const std::vector<VECTOR2I>& directions = m_snapLineManager.GetDirections();
        const std::optional<int>     activeDirection = m_snapLineManager.GetActiveDirection();

        for( size_t ii = 0; ii < directions.size(); ++ii )
        {
            const VECTOR2I& direction = directions[ii];

            VECTOR2I scaledDirection = direction * 100000;

            CONSTRUCTION_MANAGER::CONSTRUCTION_ITEM::DRAWABLE_ENTRY entry;
            entry.Drawable = LINE{ *snapLineOrigin, *snapLineOrigin + scaledDirection };
            entry.LineWidth = ( activeDirection && *activeDirection == static_cast<int>( ii ) ) ? 2 : 1;

            snapPointItem.Constructions.push_back( entry );
        }

        if( !snapPointItem.Constructions.empty() )
            batches.push_back( std::move( batch ) );
    }

    return batches;
}


void CONSTRUCTION_MANAGER::Clear()
{
    std::lock_guard<std::mutex> lock( m_batchesMutex );

    m_persistentConstructionBatch.reset();
    m_temporaryConstructionBatches.clear();
    m_involvedItems.clear();
    CancelProposal();
}


void SNAP_MANAGER::Clear()
{
    m_snapLineManager.ClearSnapLine();
    m_constructionManager.Clear();
    UpdateSnapGuides();
}
