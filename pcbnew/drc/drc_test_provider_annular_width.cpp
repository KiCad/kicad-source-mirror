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

#include <common.h>
#include <pcb_track.h>
#include <pad.h>
#include <footprint.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
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
*/


class DRC_TEST_PROVIDER_ANNULAR_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ANNULAR_WIDTH()
    {}

    virtual ~DRC_TEST_PROVIDER_ANNULAR_WIDTH() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "annular_width" ); };
};


bool DRC_TEST_PROVIDER_ANNULAR_WIDTH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
    {
        REPORT_AUX( wxT( "Annular width violations ignored. Skipping check." ) );
        return true;    // continue with other tests
    }

    const int progressDelta = 500;

    if( !m_drcEngine->HasRulesForConstraintType( ANNULAR_WIDTH_CONSTRAINT ) )
    {
        REPORT_AUX( wxT( "No annular width constraints found. Tests not run." ) );
        return true;    // continue with other tests
    }

    if( !reportPhase( _( "Checking pad & via annular rings..." ) ) )
        return false;   // DRC cancelled

    auto calcEffort =
            []( BOARD_ITEM* item ) -> size_t
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

                    size_t effort = 0;

                    pad->Padstack().ForEachUniqueLayer(
                            [&pad, &effort]( PCB_LAYER_ID aLayer )
                            {
                                if( pad->GetOffset( aLayer ) == VECTOR2I( 0, 0 ) )
                                {
                                    switch( pad->GetShape( aLayer ) )
                                    {
                                    case PAD_SHAPE::CHAMFERED_RECT:
                                        if( pad->GetChamferRectRatio( aLayer ) > 0.30 )
                                            break;

                                        KI_FALLTHROUGH;

                                    case PAD_SHAPE::CIRCLE:
                                    case PAD_SHAPE::OVAL:
                                    case PAD_SHAPE::RECTANGLE:
                                    case PAD_SHAPE::ROUNDRECT:
                                        effort += 1;
                                        break;

                                    default:
                                        break;
                                    }
                                }

                                effort += 5;
                            } );

                    return effort;
                }

                default:
                    return 0;
                }
            };

    auto getPadAnnulusPts =
            []( PAD* pad, PCB_LAYER_ID aLayer, DRC_CONSTRAINT& constraint,
                const std::vector<const PAD*>& sameNumPads, VECTOR2I* ptA, VECTOR2I* ptB )
            {
                bool handled = false;

                if( pad->GetOffset( aLayer ) == VECTOR2I( 0, 0 ) )
                {
                    int xDist = KiROUND( ( pad->GetSizeX() - pad->GetDrillSizeX() ) / 2.0 );
                    int yDist = KiROUND( ( pad->GetSizeY() - pad->GetDrillSizeY() ) / 2.0 );

                    if( yDist < xDist )
                    {
                        *ptA = pad->GetPosition() - VECTOR2I( 0, pad->GetDrillSizeY() / 2 );
                        *ptB = pad->GetPosition() - VECTOR2I( 0, pad->GetSizeY() / 2 );
                    }
                    else
                    {
                        *ptA = pad->GetPosition() - VECTOR2I( pad->GetDrillSizeX() / 2, 0 );
                        *ptB = pad->GetPosition() - VECTOR2I( pad->GetSizeX() / 2, 0 );
                    }

                    RotatePoint( *ptA, pad->GetPosition(), pad->GetOrientation() );
                    RotatePoint( *ptB, pad->GetPosition(), pad->GetOrientation() );

                    switch( pad->GetShape( aLayer ) )
                    {
                    case PAD_SHAPE::CHAMFERED_RECT:
                        handled = pad->GetChamferRectRatio( aLayer ) <= 0.30;
                        break;

                    case PAD_SHAPE::CIRCLE:
                    case PAD_SHAPE::OVAL:
                    case PAD_SHAPE::RECTANGLE:
                    case PAD_SHAPE::ROUNDRECT:
                        handled = true;

                        break;

                    default:
                        break;
                    }
                }

                if( !handled || !sameNumPads.empty() )
                {
                    // Slow (but general purpose) method.
                    SHAPE_POLY_SET padOutline;
                    std::shared_ptr<SHAPE_SEGMENT> slot = pad->GetEffectiveHoleShape();

                    pad->TransformShapeToPolygon( padOutline, aLayer, 0, pad->GetMaxError(), ERROR_INSIDE );

                    if( sameNumPads.empty() )
                    {
                        if( !padOutline.Collide( pad->GetPosition() ) )
                        {
                            // Hole outside pad
                            *ptA = pad->GetPosition();
                            *ptB = pad->GetPosition();
                        }
                        else
                        {
                            padOutline.NearestPoints( slot.get(), *ptA, *ptB );
                        }
                    }
                    else if( constraint.Value().HasMin() )
                    {
                        SHAPE_POLY_SET aggregatePadOutline = padOutline;
                        SHAPE_POLY_SET otherPadHoles;
                        SHAPE_POLY_SET slotPolygon;

                        slot->TransformToPolygon( slotPolygon, 0, ERROR_INSIDE );

                        for( const PAD* sameNumPad : sameNumPads )
                        {
                            // Construct the full pad with outline and hole.
                            sameNumPad->TransformShapeToPolygon( aggregatePadOutline, aLayer, 0, pad->GetMaxError(),
                                                                 ERROR_OUTSIDE );

                            sameNumPad->TransformHoleToPolygon( otherPadHoles, 0, pad->GetMaxError(), ERROR_INSIDE );
                        }

                        aggregatePadOutline.BooleanSubtract( otherPadHoles );

                        if( !aggregatePadOutline.Collide( pad->GetPosition() ) )
                        {
                            // Hole outside pad
                            *ptA = pad->GetPosition();
                            *ptB = pad->GetPosition();
                        }
                        else
                        {
                            aggregatePadOutline.NearestPoints( slot.get(), *ptA, *ptB );
                        }
                    }
                }
            };

    auto checkConstraint =
            [&]( DRC_CONSTRAINT& constraint, BOARD_ITEM* item, const VECTOR2I& ptA, const VECTOR2I& ptB,
                 PCB_LAYER_ID aLayer )
            {
                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    return;

                int  v_min = 0;
                int  v_max = 0;
                bool fail_min = false;
                bool fail_max = false;
                int  width = ( ptA - ptB ).EuclideanNorm();

                if( constraint.Value().HasMin() )
                {
                    v_min = constraint.Value().Min();
                    fail_min = width < v_min;
                }

                if( constraint.Value().HasMax() )
                {
                    v_max = constraint.Value().Max();
                    fail_max = width > v_max;
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ANNULAR_WIDTH );

                    if( fail_min )
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s min annular width %s; actual %s)" ),
                                                            constraint.GetName(),
                                                            v_min,
                                                            width ) );
                    }

                    if( fail_max )
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s max annular width %s; actual %s)" ),
                                                            constraint.GetName(),
                                                            v_max,
                                                            width ) );
                    }

                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );
                    reportTwoPointGeometry( drcItem, item->GetPosition(), ptA, ptB, aLayer );
                }
            };

    auto checkAnnularWidth =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
                    return false;

                if( item->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );

                    via->Padstack().ForEachUniqueLayer(
                            [&]( PCB_LAYER_ID aLayer )
                            {
                                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, item,
                                                                          nullptr, aLayer );

                                VECTOR2I ptA = via->GetPosition() - VECTOR2I( via->GetDrillValue() / 2, 0 );
                                VECTOR2I ptB = via->GetPosition() - VECTOR2I( via->GetWidth( aLayer ) / 2, 0 );
                                checkConstraint( constraint, via, ptA, ptB, aLayer );
                            } );
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( !pad->HasHole() || pad->GetAttribute() != PAD_ATTRIB::PTH )
                        return true;

                    std::vector<const PAD*> sameNumPads;

                    if( const FOOTPRINT* fp = static_cast<const FOOTPRINT*>( pad->GetParent() ) )
                        sameNumPads = fp->GetPads( pad->GetNumber(), pad );

                    pad->Padstack().ForEachUniqueLayer(
                            [&]( PCB_LAYER_ID aLayer )
                            {
                                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, item,
                                                                          nullptr, aLayer );

                                VECTOR2I ptA;
                                VECTOR2I ptB;
                                getPadAnnulusPts( pad, aLayer, constraint, sameNumPads, &ptA, &ptB );
                                checkConstraint( constraint, pad, ptA, ptB, aLayer );
                            } );
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

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ANNULAR_WIDTH> dummy;
}
