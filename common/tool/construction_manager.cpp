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

#include "tool/construction_manager.h"

#include <chrono>
#include <condition_variable>
#include <thread>

#include <advanced_config.h>
#include <hash.h>

/**
 * A helper class to manage the activation of a "proposal" after a timeout.
 *
 * When a proposal is made, a timer starts. If no new proposal is made and the proposal
 * is not cancelled before the timer expires, the proposal is "accepted" via a callback.
 *
 * Propos
 *
 * @tparam T The type of the proposal, which will be passed to the callback (by value)
 */
template <typename T>
class ACTIVATION_HELPER
{
public:
    using ACTIVATION_CALLBACK = std::function<void( T&& )>;

    ACTIVATION_HELPER( std::chrono::milliseconds aTimeout, ACTIVATION_CALLBACK aCallback ) :
            m_timeout( aTimeout ), m_callback( std::move( aCallback ) ), m_stop( false ),
            m_thread( &ACTIVATION_HELPER::ProposalCheckFunction, this )
    {
    }

    ~ACTIVATION_HELPER()
    {
        // Stop the delay thread and wait for it
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_stop = true;
            m_cv.notify_all();
        }

        if( m_thread.joinable() )
        {
            m_thread.join();
        }
    }

    void ProposeActivation( T&& aProposal, std::size_t aProposalTag )
    {
        std::lock_guard<std::mutex> lock( m_mutex );

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
        m_proposalDeadline = std::chrono::steady_clock::now() + m_timeout;
        m_cv.notify_all();
    }

    void CancelProposal()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_pendingProposalTag.reset();
        m_cv.notify_all();
    }

    void ProposalCheckFunction()
    {
        while( !m_stop )
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if( !m_stop && !m_pendingProposalTag.has_value() )
            {
                // No active proposal - wait for one (unlocks while waiting)
                m_cv.wait( lock );
            }

            if( !m_stop && m_pendingProposalTag.has_value() )
            {
                // Active proposal - wait for timeout
                auto now = std::chrono::steady_clock::now();

                if( m_cv.wait_for( lock, m_proposalDeadline - now ) == std::cv_status::timeout )
                {
                    // See if the timeout was extended for a new proposal
                    now = std::chrono::steady_clock::now();
                    if( now < m_proposalDeadline )
                    {
                        // Extended - wait for the new deadline
                        continue;
                    }

                    // See if there is still a proposal to accept
                    // (could have been cancelled in the meantime)
                    if( m_pendingProposalTag )
                    {
                        m_lastAcceptedProposalTag = m_pendingProposalTag;
                        m_pendingProposalTag.reset();

                        T proposalToAccept = std::move( m_lastProposal );
                        lock.unlock();
                        // Call the callback (outside the lock)
                        m_callback( std::move( proposalToAccept ) );
                    }
                }
            }
        }
    }

private:
    mutable std::mutex m_mutex;

    // Activation timeout in milliseconds
    std::chrono::milliseconds m_timeout;

    std::chrono::time_point<std::chrono::steady_clock> m_proposalDeadline;

    ///< The last proposal tag that was made
    std::optional<std::size_t> m_pendingProposalTag;

    ///< The last proposal that was accepted
    std::optional<std::size_t> m_lastAcceptedProposalTag;

    // The most recently-proposed item
    T m_lastProposal;

    ///< Callback to call when the proposal is accepted
    ACTIVATION_CALLBACK     m_callback;
    std::condition_variable m_cv;
    std::atomic<bool>       m_stop;
    // The thread must be constructed last, as it starts running immediately
    std::thread             m_thread;
};


struct CONSTRUCTION_MANAGER::PENDING_BATCH
{
    CONSTRUCTION_ITEM_BATCH Batch;
    bool                    IsPersistent;
};


CONSTRUCTION_MANAGER::CONSTRUCTION_MANAGER( CONSTRUCTION_VIEW_HANDLER& aHelper ) :
        m_viewHandler( aHelper )
{
    const std::chrono::milliseconds acceptanceTimeout(
            ADVANCED_CFG::GetCfg().m_ExtensionSnapTimeoutMs );

    m_activationHelper = std::make_unique<ACTIVATION_HELPER<std::unique_ptr<PENDING_BATCH>>>(
            acceptanceTimeout,
            [this]( std::unique_ptr<PENDING_BATCH>&& aAccepted )
            {
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

    const std::size_t hash = HashConstructionBatchSources( *aBatch, aIsPersistent );

    m_activationHelper->ProposeActivation(
            std::make_unique<PENDING_BATCH>( PENDING_BATCH{ std::move( *aBatch ), aIsPersistent } ),
            hash );
}


void CONSTRUCTION_MANAGER::CancelProposal()
{
    m_activationHelper->CancelProposal();
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

            // We only keep up to one previous temporary batch and the current one
            // we could make this a setting if we want to keep more, but it gets cluttered
            const int maxTempItems = 2;

            while( m_temporaryConstructionBatches.size() >= maxTempItems )
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
                for( const KIGFX::CONSTRUCTION_GEOM::DRAWABLE& drawable : item.Constructions )
                {
                    geom.AddDrawable( drawable, aIsPersistent );
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
        m_viewHandler( aViewHandler )
{
}

void SNAP_LINE_MANAGER::SetSnapLineOrigin( const VECTOR2I& aOrigin )
{
    // Setting the origin clears the snap line as the end point is no longer valid
    ClearSnapLine();
    m_snapLineOrigin = aOrigin;
}

void SNAP_LINE_MANAGER::SetSnapLineEnd( const OPT_VECTOR2I& aSnapEnd )
{
    if( m_snapLineOrigin && aSnapEnd != m_snapLineEnd )
    {
        m_snapLineEnd = aSnapEnd;

        if( m_snapLineEnd )
            m_viewHandler.GetViewItem().SetSnapLine( SEG{ *m_snapLineOrigin, *m_snapLineEnd } );
        else
            m_viewHandler.GetViewItem().ClearSnapLine();

        m_viewHandler.updateView();
    }
}

void SNAP_LINE_MANAGER::ClearSnapLine()
{
    m_snapLineOrigin.reset();
    m_snapLineEnd.reset();
    m_viewHandler.GetViewItem().ClearSnapLine();
    m_viewHandler.updateView();
}

void SNAP_LINE_MANAGER::SetSnappedAnchor( const VECTOR2I& aAnchorPos )
{
    if( m_snapLineOrigin.has_value() )
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
        SetSnapLineOrigin( aAnchorPos );
    }
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


OPT_VECTOR2I SNAP_LINE_MANAGER::GetNearestSnapLinePoint( const VECTOR2I&    aCursor,
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
        const EDA_ANGLE longRangeEscapeAngle( 4, DEGREES_T );

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


SNAP_MANAGER::SNAP_MANAGER( KIGFX::CONSTRUCTION_GEOM& aHelper ) :
        CONSTRUCTION_VIEW_HANDLER( aHelper ), m_snapLineManager( *this ),
        m_constructionManager( *this )
{
}


void SNAP_MANAGER::updateView()
{
    if( m_updateCallback )
    {
        bool showAnything = m_constructionManager.HasActiveConstruction()
                            || m_snapLineManager.HasCompleteSnapLine();

        m_updateCallback( showAnything );
    }
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

        // One horizontal and one vertical infinite line from the snap point
        snapPointItem.Constructions.push_back(
                LINE{ *snapLineOrigin, *snapLineOrigin + VECTOR2I( 100000, 0 ) } );
        snapPointItem.Constructions.push_back(
                LINE{ *snapLineOrigin, *snapLineOrigin + VECTOR2I( 0, 100000 ) } );

        batches.push_back( std::move( batch ) );
    }

    return batches;
}
