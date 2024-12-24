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
*/


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

    auto checkAnnularWidth =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
                    return false;

                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );

                int  minAnnularWidth = INT_MAX;
                int  maxAnnularWidth = 0;
                int  v_min = 0;
                int  v_max = 0;
                bool fail_min = false;
                bool fail_max = false;


                switch( item->Type() )
                {
                case PCB_VIA_T:
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );
                    int drill = via->GetDrillValue();

                    via->Padstack().ForEachUniqueLayer(
                        [&]( PCB_LAYER_ID aLayer )
                        {
                            int layerWidth = ( via->GetWidth( aLayer ) - drill ) / 2;
                            minAnnularWidth = std::min( minAnnularWidth, layerWidth );
                            maxAnnularWidth = std::max( maxAnnularWidth, layerWidth );
                        } );
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

                    pad->Padstack().ForEachUniqueLayer(
                        [&]( PCB_LAYER_ID aLayer )
                        {
                            int annularWidth = 0;

                            if( pad->GetOffset( aLayer ) == VECTOR2I( 0, 0 ) )
                            {
                                VECTOR2I padSize = pad->GetSize( aLayer );

                                switch( pad->GetShape( aLayer ) )
                                {
                                case PAD_SHAPE::CIRCLE:
                                    annularWidth = ( padSize.x - pad->GetDrillSizeX() ) / 2;

                                    // If there are more pads with the same number.  Check to see if the
                                    // pad is embedded inside another pad with the same number below.
                                    if( sameNumPads.empty() )
                                        handled = true;

                                    break;

                                case PAD_SHAPE::CHAMFERED_RECT:
                                    if( pad->GetChamferRectRatio( aLayer ) > 0.30 )
                                        break;

                                    KI_FALLTHROUGH;

                                case PAD_SHAPE::OVAL:
                                case PAD_SHAPE::RECTANGLE:
                                case PAD_SHAPE::ROUNDRECT:
                                    annularWidth = std::min( padSize.x - pad->GetDrillSizeX(),
                                                             padSize.y - pad->GetDrillSizeY() ) / 2;

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

                                pad->TransformShapeToPolygon( padOutline, aLayer, 0, maxError,
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
                                    SHAPE_POLY_SET otherPadHoles;
                                    SHAPE_POLY_SET slotPolygon;

                                    slot->TransformToPolygon( slotPolygon, 0, ERROR_INSIDE );

                                    for( const PAD* sameNumPad : sameNumPads )
                                    {
                                        // Construct the full pad with outline and hole.
                                        sameNumPad->TransformShapeToPolygon(
                                                otherPadOutline, PADSTACK::ALL_LAYERS, 0, maxError,
                                                ERROR_OUTSIDE );

                                        sameNumPad->TransformHoleToPolygon(
                                                otherPadHoles, 0, maxError, ERROR_INSIDE );
                                    }

                                    otherPadOutline.BooleanSubtract( otherPadHoles );

                                    // If the pad hole under test intersects with another pad outline,
                                    // the annular width calculated above is used.
                                    bool intersects = false;

                                    for( int i = 0; i < otherPadOutline.OutlineCount() && !intersects; i++ )
                                    {
                                        intersects |= slotPolygon.COutline( 0 ).Intersects( otherPadOutline.COutline( i ) );

                                        for( int j = 0; j < otherPadOutline.HoleCount( i ) && !intersects; j++ )
                                        {
                                            intersects |= slotPolygon.COutline( 0 ).Intersects( otherPadOutline.CHole( i, j ) );
                                        }
                                    }

                                    if( !intersects )
                                    {
                                        // Determine the effective annular width if the pad hole under
                                        // test lies withing the boundary of another pad outline.
                                        int effectiveWidth = std::numeric_limits<int>::max();

                                        for( int ii = 0; ii < otherPadOutline.OutlineCount(); ii++ )
                                        {
                                            if( slot->Collide( &otherPadOutline.Outline( ii ), 0 ) )
                                            {
                                                if( effectiveWidth > annularWidth )
                                                    annularWidth = effectiveWidth;
                                            }
                                        }
                                    }
                                }
                            }

                            maxAnnularWidth = std::max( maxAnnularWidth, annularWidth );
                            minAnnularWidth = std::min( minAnnularWidth, annularWidth );
                        } );

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
                    fail_min = minAnnularWidth < v_min;
                }

                if( constraint.Value().HasMax() )
                {
                    v_max = constraint.Value().Max();
                    fail_max = maxAnnularWidth > v_max;
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
                                         minAnnularWidth );
                    }

                    if( fail_max )
                    {
                        msg = formatMsg( _( "(%s max annular width %s; actual %s)" ),
                                         constraint.GetName(),
                                         v_max,
                                         maxAnnularWidth );
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
