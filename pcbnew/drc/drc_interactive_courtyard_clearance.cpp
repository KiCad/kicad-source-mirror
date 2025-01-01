/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <drc/drc_interactive_courtyard_clearance.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <pad.h>
#include <zone.h>
#include <geometry/shape_segment.h>
#include <footprint.h>

void DRC_INTERACTIVE_COURTYARD_CLEARANCE::testCourtyardClearances()
{
    std::vector<BOX2I> fpBBBoxes( m_FpInMove.size() );
    BOX2I              movingBBox;

    for( size_t i = 0; i < m_FpInMove.size(); i++ )
    {
        FOOTPRINT* fpB = m_FpInMove[i];

        BOX2I bbox = fpB->GetBoundingBox( true );
        movingBBox.Merge( bbox );
        fpBBBoxes[i] = bbox;
    }

    movingBBox.Inflate( m_largestCourtyardClearance );

    for( FOOTPRINT* fpA: m_board->Footprints() )
    {
        if( fpA->IsSelected() )
            continue;

        BOX2I fpABBox = fpA->GetBoundingBox( true );

        if( !movingBBox.Intersects( fpABBox ) )
            continue;

        const SHAPE_POLY_SET& frontA = fpA->GetCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backA = fpA->GetCourtyard( B_CrtYd );

        if( frontA.OutlineCount() == 0 && backA.OutlineCount() == 0 )
             // No courtyards defined and no hole testing against other footprint's courtyards
            continue;

        BOX2I frontABBox = frontA.BBoxFromCaches();
        BOX2I backABBox = backA.BBoxFromCaches();

        frontABBox.Inflate( m_largestCourtyardClearance );
        backABBox.Inflate( m_largestCourtyardClearance );

        for( size_t inMoveId = 0; inMoveId < m_FpInMove.size(); inMoveId++ )
        {
            FOOTPRINT*            fpB = m_FpInMove[inMoveId];
            const SHAPE_POLY_SET& frontB = fpB->GetCourtyard( F_CrtYd );
            const SHAPE_POLY_SET& backB = fpB->GetCourtyard( B_CrtYd );

            const BOX2I fpBBBox = fpBBBoxes[inMoveId];
            const BOX2I frontBBBox = frontB.BBoxFromCaches();
            const BOX2I backBBBox = backB.BBoxFromCaches();

            int      clearance;
            int      actual;
            VECTOR2I pos;

            if( frontA.OutlineCount() > 0 && frontB.OutlineCount() > 0
                    && frontABBox.Intersects( frontBBBox ) )
            {
                // Currently, do not use DRC engine for calculation time reasons
                // DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
                // constraint.GetValue().Min();
                clearance = 0;

                if( frontA.Collide( &frontB, clearance, &actual, &pos ) )
                {
                    m_itemsInConflict.insert( fpA );
                    m_itemsInConflict.insert( fpB );
                 }
            }

            if( backA.OutlineCount() > 0 && backB.OutlineCount() > 0
                    && backABBox.Intersects( backBBBox ) )
            {
                // Currently, do not use DRC engine for calculation time reasons
                // DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
                // constraint.GetValue().Min();
                clearance = 0;

                if( backA.Collide( &backB, clearance, &actual, &pos ) )
                {
                    m_itemsInConflict.insert( fpA );
                    m_itemsInConflict.insert( fpB );
                }
            }

            // Now test if a pad hole of some other footprint is inside the courtyard area
            // of the moved footprint
            auto testPadAgainstCourtyards =
                    [&]( const PAD* pad, FOOTPRINT* footprint ) -> bool
                    {
                        if( pad->HasHole() )
                        {
                            std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();
                            const SHAPE_POLY_SET& front = footprint->GetCachedCourtyard( F_CrtYd );
                            const SHAPE_POLY_SET& back = footprint->GetCachedCourtyard( B_CrtYd );

                            if( front.OutlineCount() > 0 && front.Collide( hole.get(), 0 ) )
                                return true;
                            else if( back.OutlineCount() > 0 && back.Collide( hole.get(), 0 ) )
                                return true;
                        }

                        return false;
                    };

            bool skipNextCmp = false;

            if( ( frontA.OutlineCount() > 0 && frontABBox.Intersects( fpBBBox ) )
                || ( backA.OutlineCount() > 0 && backABBox.Intersects( fpBBBox ) ) )
            {
                for( const PAD* padB : fpB->Pads() )
                {
                    if( testPadAgainstCourtyards( padB, fpA ) )
                    {
                        m_itemsInConflict.insert( fpA );
                        m_itemsInConflict.insert( fpB );
                        skipNextCmp = true;
                        break;
                    }
                }
            }

            if( skipNextCmp )
                continue;       // fpA and fpB are already in list

            if( ( frontB.OutlineCount() > 0 && frontBBBox.Intersects( fpABBox ) )
                || ( backB.OutlineCount() > 0 && backBBBox.Intersects( fpABBox ) ) )
            {
                for( const PAD* padA : fpA->Pads() )
                {
                    if( testPadAgainstCourtyards( padA, fpB ) )
                    {
                        m_itemsInConflict.insert( fpA );
                        m_itemsInConflict.insert( fpB );
                        break;
                    }
                }
            }
        }
    }

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->GetIsRuleArea() || !zone->HasKeepoutParametersSet()
            || !zone->GetDoNotAllowFootprints() )
        {
            continue;
        }

        bool disallowFront = ( zone->GetLayerSet() & LSET::FrontMask() ).any();
        bool disallowBack = ( zone->GetLayerSet() & LSET::BackMask() ).any();

        for( FOOTPRINT* fp : m_FpInMove )
        {
            if( disallowFront )
            {
                const SHAPE_POLY_SET& frontCourtyard = fp->GetCourtyard( F_CrtYd );

                if( !frontCourtyard.IsEmpty() )
                {
                    if( zone->Outline()->Collide( &frontCourtyard.Outline( 0 ) ) )
                    {
                        m_itemsInConflict.insert( fp );
                        m_itemsInConflict.insert( zone );
                        break;
                    }
                }
            }

            if( disallowBack )
            {
                const SHAPE_POLY_SET& backCourtyard = fp->GetCourtyard( B_CrtYd );

                if( !backCourtyard.IsEmpty() )
                {
                    if( zone->Outline()->Collide( &backCourtyard.Outline( 0 ) ) )
                    {
                        m_itemsInConflict.insert( fp );
                        m_itemsInConflict.insert( zone );
                        break;
                    }
                }
            }
        }
    }
}


void DRC_INTERACTIVE_COURTYARD_CLEARANCE::Init( BOARD* aBoard )
{
    m_board = aBoard;

    // Update courtyard data and clear the COURTYARD_CONFLICT flag
    for( FOOTPRINT* fp: m_board->Footprints() )
    {
        fp->ClearFlags( COURTYARD_CONFLICT );
        fp->BuildCourtyardCaches();
    }
}


bool DRC_INTERACTIVE_COURTYARD_CLEARANCE::Run()
{
    m_itemsInConflict.clear();
    m_largestCourtyardClearance = 0;

    DRC_CONSTRAINT constraint;

    if( m_drcEngine->QueryWorstConstraint( COURTYARD_CLEARANCE_CONSTRAINT, constraint ) )
        m_largestCourtyardClearance = constraint.GetValue().Min();

    testCourtyardClearances();

    return true;
}


void DRC_INTERACTIVE_COURTYARD_CLEARANCE::UpdateConflicts( KIGFX::VIEW* aView,
                                                           bool aHighlightMoved )
{
    // Ensure the "old" conflicts are cleared
    for( BOARD_ITEM* item: m_lastItemsInConflict )
    {
        item->ClearFlags(COURTYARD_CONFLICT );
        aView->Update( item );
        aView->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
    }

    m_lastItemsInConflict.clear();

    for( BOARD_ITEM* item: m_itemsInConflict )
    {
        if( aHighlightMoved || !alg::contains( m_FpInMove, item ) )
        {
            if( !item->HasFlag( COURTYARD_CONFLICT ) )
            {
                item->SetFlags( COURTYARD_CONFLICT );
                aView->Update( item );
                aView->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
            }

            m_lastItemsInConflict.push_back( item );
        }
    }
}


void DRC_INTERACTIVE_COURTYARD_CLEARANCE::ClearConflicts( KIGFX::VIEW* aView )
{
    for( BOARD_ITEM* item: m_lastItemsInConflict )
    {
        item->ClearFlags( COURTYARD_CONFLICT );
        aView->Update( item );
        aView->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
    }
}


