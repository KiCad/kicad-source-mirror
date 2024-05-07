/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022, 2024 KiCad Developers.
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

#include <common.h>
#include <pcb_track.h>
#include <pad.h>
#include <footprint.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <macros.h>
#include <convert_basic_shapes_to_polygon.h>
#include <board_design_settings.h>

/*
    Via/pad annular ring width test. Checks if there's sufficient copper ring around
    PTH/NPTH holes (vias/pads)
    Errors generated:
    - DRCE_ANNULAR_WIDTH

    Todo:
    - check pad holes too.
    - pad stack support (different IAR/OAR values depending on layer)
*/

/**
 * Find the nearest collision point between two shape line chains.
 *
 * @note This collision test only tests the shape line chain segments (outline) by setting the
 *       shape closed status to false.
 *
 * @param aLhs is the left hand shape line chain to run the collision test on.
 * @param aRhs is the right hand shape line chain the run the collision test against \a aLhs.
 * @param aClearance is the collision clearance between the two shape line changes.
 * @param[out] aDistance is an optional pointer to store the nearest collision distance.
 * @param[out] aPt1 is an optional pointer to store the nearest collision point.
 * @retrun true if a collision occurs between \a aLhs and \a aRhs otherwise false.
 */
static inline bool collide( const SHAPE_LINE_CHAIN& aLhs, const SHAPE_LINE_CHAIN& aRhs,
                            int aClearance, int* aDistance = nullptr, VECTOR2I* aPt1 = nullptr )
{
    wxCHECK( aLhs.PointCount() && aRhs.PointCount(), false );

    VECTOR2I pt1;
    bool retv = false;
    int dist = std::numeric_limits<int>::max();
    int tmp = dist;

    SHAPE_LINE_CHAIN lhs( aLhs );
    SHAPE_LINE_CHAIN rhs( aRhs );

    lhs.SetClosed( false );
    lhs.Append( lhs.CPoint( 0 ) );
    rhs.SetClosed( false );
    rhs.Append( rhs.CPoint( 0 ) );

    for( int i = 0; i < rhs.SegmentCount(); i ++ )
    {
        if( lhs.Collide( rhs.CSegment( i ), tmp, &tmp, &pt1 ) )
        {
            retv = true;

            if( tmp < dist )
                dist = tmp;

            if( aDistance )
                *aDistance = dist;

            if( aPt1 )
                *aPt1 = pt1;
        }
    }

    return retv;
}


static bool collide( const SHAPE_POLY_SET& aLhs, const SHAPE_LINE_CHAIN& aRhs, int aClearance,
                     int* aDistance = nullptr, VECTOR2I* aPt1 = nullptr )
{
    VECTOR2I pt1;
    bool retv = false;
    int tmp = std::numeric_limits<int>::max();
    int dist = tmp;

    for( int i = 0; i < aLhs.OutlineCount(); i++ )
    {
        if( collide( aLhs.Outline( i ), aRhs, aClearance, &tmp, &pt1 ) )
        {
            retv = true;

            if( tmp < dist )
            {
                dist = tmp;

                if( aDistance )
                    *aDistance = dist;

                if( aPt1 )
                    *aPt1 = pt1;
            }
        }

        for( int j = 0; j < aLhs.HoleCount( i ); i++ )
        {
            if( collide( aLhs.CHole( i, j ), aRhs, aClearance, &tmp, &pt1 ) )
            {
                retv = true;

                if( tmp < dist )
                {
                    dist = tmp;

                    if( aDistance )
                        *aDistance = dist;

                    if( aPt1 )
                        *aPt1 = pt1;
                }
            }
        }
    }

    return retv;
}


class DRC_TEST_PROVIDER_ANNULAR_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ANNULAR_WIDTH()
    {
    }

    virtual ~DRC_TEST_PROVIDER_ANNULAR_WIDTH()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "annular_width" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests pad/via annular rings" );
    }
};


bool DRC_TEST_PROVIDER_ANNULAR_WIDTH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
    {
        reportAux( wxT( "Annular width violations ignored. Skipping check." ) );
        return true;    // continue with other tests
    }

    const int progressDelta = 500;

    if( !m_drcEngine->HasRulesForConstraintType( ANNULAR_WIDTH_CONSTRAINT ) )
    {
        reportAux( wxT( "No annular width constraints found. Tests not run." ) );
        return true;    // continue with other tests
    }

    if( !reportPhase( _( "Checking pad & via annular rings..." ) ) )
        return false;   // DRC cancelled

    int maxError = m_drcEngine->GetBoard()->GetDesignSettings().m_MaxError;

    auto calcEffort =
            []( BOARD_ITEM* item )
            {
                switch( item->Type() )
                {
                case PCB_VIA_T:
                    return 1;

                case PCB_PAD_T:
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( !pad->HasHole() || pad->GetAttribute() != PAD_ATTRIB::PTH )
                        return 0;

                    if( pad->GetOffset() == VECTOR2I( 0, 0 ) )
                    {
                        switch( pad->GetShape() )
                        {
                        case PAD_SHAPE::CHAMFERED_RECT:
                            if( pad->GetChamferRectRatio() > 0.30 )
                                break;

                            KI_FALLTHROUGH;

                        case PAD_SHAPE::CIRCLE:
                        case PAD_SHAPE::OVAL:
                        case PAD_SHAPE::RECTANGLE:
                        case PAD_SHAPE::ROUNDRECT:
                            return 1;

                        default:
                            break;
                        }
                    }

                    return 5;
                }

                default:
                    return 0;
                }
            };

    auto checkAnnularWidth =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
                    return false;

                // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );

                int  annularWidth = 0;
                int  v_min = 0;
                int  v_max = 0;
                bool fail_min = false;
                bool fail_max = false;


                switch( item->Type() )
                {
                case PCB_VIA_T:
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );
                    annularWidth = ( via->GetWidth() - via->GetDrillValue() ) / 2;
                    break;
                }

                case PCB_PAD_T:
                {
                    PAD* pad = static_cast<PAD*>( item );
                    bool handled = false;

                    if( !pad->HasHole() || pad->GetAttribute() != PAD_ATTRIB::PTH )
                        return true;

                    std::vector<const PAD*> sameNumPads;

                    const FOOTPRINT* fp = static_cast<const FOOTPRINT*>( pad->GetParent() );

                    if( fp )
                        sameNumPads = fp->GetPads( pad->GetNumber(), pad );

                    if( pad->GetOffset() == VECTOR2I( 0, 0 ) )
                    {
                        switch( pad->GetShape() )
                        {
                        case PAD_SHAPE::CIRCLE:
                            annularWidth = ( pad->GetSizeX() - pad->GetDrillSizeX() ) / 2;

                            // If there are more pads with the same number.  Check to see if the
                            // pad is embedded inside another pad with the same number below.
                            if( sameNumPads.empty() )
                                handled = true;

                            break;

                        case PAD_SHAPE::CHAMFERED_RECT:
                            if( pad->GetChamferRectRatio() > 0.30 )
                                break;

                            KI_FALLTHROUGH;

                        case PAD_SHAPE::OVAL:
                        case PAD_SHAPE::RECTANGLE:
                        case PAD_SHAPE::ROUNDRECT:
                            annularWidth = std::min( pad->GetSizeX() - pad->GetDrillSizeX(),
                                                     pad->GetSizeY() - pad->GetDrillSizeY() ) / 2;

                            // If there are more pads with the same number.  Check to see if the
                            // pad is embedded inside another pad with the same number below.
                            if( sameNumPads.empty() )
                                handled = true;

                            break;

                        default:
                            break;
                        }
                    }

                    if( !handled )
                    {
                        // Slow (but general purpose) method.
                        SEG::ecoord dist_sq;
                        SHAPE_POLY_SET padOutline;
                        std::shared_ptr<SHAPE_SEGMENT> slot = pad->GetEffectiveHoleShape();

                        pad->TransformShapeToPolygon( padOutline, UNDEFINED_LAYER, 0, maxError,
                                                      ERROR_INSIDE );

                        if( sameNumPads.empty() )
                        {
                            if( !padOutline.Collide( pad->GetPosition() ) )
                            {
                                // Hole outside pad
                                annularWidth = 0;
                            }
                            else
                            {
                                // Disable is-inside test in SquaredDistance
                                padOutline.Outline( 0 ).SetClosed( false );

                                dist_sq = padOutline.SquaredDistanceToSeg( slot->GetSeg() );
                                annularWidth = sqrt( dist_sq ) - slot->GetWidth() / 2;
                            }
                        }
                        else if( constraint.Value().HasMin()
                               && ( annularWidth < constraint.Value().Min() ) )
                        {
                            SHAPE_POLY_SET otherPadOutline;
                            SHAPE_POLY_SET slotPolygon;

                            slot->TransformToPolygon( slotPolygon, 0, ERROR_INSIDE );

                            for( const PAD* sameNumPad : sameNumPads )
                            {
                                // Construct the full pad with outline and hole.
                                sameNumPad->TransformShapeToPolygon( otherPadOutline,
                                                                     UNDEFINED_LAYER, 0, maxError,
                                                                     ERROR_OUTSIDE );

                                sameNumPad->TransformHoleToPolygon( otherPadOutline, 0, maxError,
                                                                    ERROR_INSIDE );


                                // If the pad hole under test intersects with another pad outline,
                                // the annular width calculated above is used.
                                bool intersects = false;

                                for( int i = 0; i < otherPadOutline.OutlineCount() && !intersects; i++ )
                                {
                                    intersects |= slotPolygon.COutline( 0 ).Intersects( otherPadOutline.COutline( i ) );
                                    if( intersects )
                                        continue;

                                    for( int j = 0; j < otherPadOutline.HoleCount( i ) && !intersects; j++ )
                                    {
                                        intersects |= slotPolygon.COutline( 0 ).Intersects( otherPadOutline.CHole( i, j ) );
                                        if( intersects )
                                            continue;
                                    }
                                }

                                if( intersects )
                                    continue;

                                // Determine the effective annular width if the pad hole under
                                // test lies withing the boundary of another pad outline.
                                int effectiveWidth = std::numeric_limits<int>::max();

                                if( collide( otherPadOutline, slotPolygon.Outline( 0 ),
                                             effectiveWidth, &effectiveWidth ) )
                                {
                                    if( effectiveWidth > annularWidth )
                                        annularWidth = effectiveWidth;
                                }
                            }
                        }
                    }

                    break;
                }

                default:
                    return true;
                }

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    return true;

                if( constraint.Value().HasMin() )
                {
                    v_min = constraint.Value().Min();
                    fail_min = annularWidth < v_min;
                }

                if( constraint.Value().HasMax() )
                {
                    v_max = constraint.Value().Max();
                    fail_max = annularWidth > v_max;
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ANNULAR_WIDTH );
                    wxString msg;

                    if( fail_min )
                    {
                        msg = formatMsg( _( "(%s min annular width %s; actual %s)" ),
                                         constraint.GetName(),
                                         v_min,
                                         annularWidth );
                    }

                    if( fail_max )
                    {
                        msg = formatMsg( _( "(%s max annular width %s; actual %s)" ),
                                         constraint.GetName(),
                                         v_max,
                                         annularWidth );
                    }

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition(), item->GetLayer() );
                }

                return true;
            };

    BOARD* board = m_drcEngine->GetBoard();
    size_t ii = 0;
    size_t total = 0;

    for( PCB_TRACK* item : board->Tracks() )
        total += calcEffort( item );

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            total += calcEffort( pad );
    }

    for( PCB_TRACK* item : board->Tracks() )
    {
        ii += calcEffort( item );

        if( !reportProgress( ii, total, progressDelta ) )
            return false;   // DRC cancelled

        if( !checkAnnularWidth( item ) )
            break;
    }

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            ii += calcEffort( pad );

            if( !reportProgress( ii, total, progressDelta ) )
                return false;   // DRC cancelled

            if( !checkAnnularWidth( pad ) )
                break;
        }
    }

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ANNULAR_WIDTH> dummy;
}
