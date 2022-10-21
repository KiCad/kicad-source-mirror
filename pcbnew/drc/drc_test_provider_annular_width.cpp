/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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
                        case PAD_SHAPE::RECT:
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

                int annularWidth = 0;

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
                        case PAD_SHAPE::RECT:
                        case PAD_SHAPE::ROUNDRECT:
                            annularWidth = std::min( pad->GetSizeX() - pad->GetDrillSizeX(),
                                                     pad->GetSizeY() - pad->GetDrillSizeY() ) / 2;
                            handled = true;
                            break;

                        default:
                            break;
                        }
                    }

                    if( !handled )
                    {
                        // Slow (but general purpose) method.
                        SHAPE_POLY_SET padOutline;

                        pad->TransformShapeToPolygon( padOutline, UNDEFINED_LAYER, 0, maxError,
                                                      ERROR_INSIDE );

                        if( !padOutline.Collide( pad->GetPosition() ) )
                        {
                            // Hole outside pad
                            annularWidth = 0;
                        }
                        else
                        {
                            std::shared_ptr<SHAPE_SEGMENT> slot = pad->GetEffectiveHoleShape();

                            // Disable is-inside test in SquaredDistance
                            padOutline.Outline( 0 ).SetClosed( false );

                            SEG::ecoord dist_sq = padOutline.SquaredDistance( slot->GetSeg() );
                            annularWidth = sqrt( dist_sq ) -  slot->GetWidth() / 2;
                        }
                    }

                    break;
                }

                default:
                    return true;
                }

                // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );
                int  v_min = 0;
                int  v_max = 0;
                bool fail_min = false;
                bool fail_max = false;

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
