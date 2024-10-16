/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2023 KiCad Developers.
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

#include <core/thread_pool.h>
#include "geometry/eda_angle.h"
#include <numbers>
#include <pcb_track.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <connectivity/connectivity_data.h>


/*
    Track angle test. Checks the angle between two connected track segments. 
    Errors generated:
    - DRCE_TRACK_ANGLE
*/

class DRC_TEST_PROVIDER_TRACK_ANGLE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TRACK_ANGLE()
    {}

    virtual ~DRC_TEST_PROVIDER_TRACK_ANGLE()
    {}

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "angle" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests track angles" );
    }
};


bool DRC_TEST_PROVIDER_TRACK_ANGLE::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_ANGLE ) )
    {
        reportAux( wxT( "Track angle violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( TRACK_ANGLE_CONSTRAINT ) )
    {
        reportAux( wxT( "No track angle constraints found. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking track angles..." ) ) )
        return false;       // DRC cancelled

    auto checkTrackAngle =
            [&]( PCB_TRACK* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_ANGLE ) )
                {
                    return false;
                }

                if( item->Type() != PCB_TRACE_T )
                {
                    return true;
                }

                SEG      segment   = SEG( item->GetStart(), item->GetEnd() );

                std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_drcEngine->GetBoard()->GetConnectivity();

                for( PCB_TRACK* other : connectivity->GetConnectedTracks( item ) )
                {
                    if( other->Type() != PCB_TRACE_T )
                    {
                        continue;
                    }

                    SEG other_segment = SEG( other->GetStart(), other->GetEnd() );

                    OPT_VECTOR2I intersection_opt = segment.Intersect( other_segment );

                    if( !intersection_opt.has_value() )
                    {
                        continue;
                    }

                    VECTOR2I p0 = intersection_opt.value();

                    if( m_drcEngine->GetBoard()->GetPad( p0, { item->GetLayer() } ) )
                    {
                        continue;
                    }

                    auto constraint = m_drcEngine->EvalRules( TRACK_ANGLE_CONSTRAINT, item, other,
                                                              item->GetLayer() );

                    VECTOR2D  direction       = VECTOR2D( item->GetEnd() - item->GetStart() ).Resize( 1 );
                    VECTOR2D  other_direction = VECTOR2D( other->GetEnd() - other->GetStart() ).Resize( 1 );
                    EDA_ANGLE actual;
                    bool angle_below_90 = false;

                    if( segment.B == p0 )
                    {
                        direction *= -1;
                    }
                    else if( segment.A != p0 )
                    {
                        angle_below_90 = true;
                    }

                    if( other_segment.B == p0 )
                    {
                        other_direction *= -1;
                    }
                    else if( other_segment.A != p0 )
                    {
                        angle_below_90 = true;
                    }

                    actual = EDA_ANGLE::Arccos( direction.Dot( other_direction ) );
                    if( angle_below_90 && actual > 90 )
                    {
                        actual -= 90;
                    }

                    bool fail_min = false;
                    bool fail_max = false;
                    EDA_ANGLE constraintAngle = 0;

                    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                    {
                        if( constraint.Value().HasMin() && actual.AsDegrees() < constraint.Value().Min() )
                        {
                            fail_min        = true;
                            constraintAngle = EDA_ANGLE( constraint.Value().Min(), DEGREES_T );
                        }

                        if( constraint.Value().HasMax() && actual.AsDegrees() > constraint.Value().Max() )
                        {
                            fail_max        = true;
                            constraintAngle = EDA_ANGLE( constraint.Value().Max(), DEGREES_T );
                        }
                    }

                    if( fail_min || fail_max )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACK_ANGLE );
                        wxString constraintName = constraint.GetName();
                        wxString msg;

                        if( fail_min )
                        {
                            msg = formatMsg( _( "(%s min angle %s; actual %s)" ),
                                             constraintName,
                                             constraintAngle,
                                             actual );
                        }
                        else
                        {
                            msg = formatMsg( _( "(%s max angle %s; actual %s)" ),
                                             constraintName,
                                             constraintAngle,
                                             actual );
                        }

                        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
                        drcItem->SetItems( item , other );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, p0, item->GetLayer() );
                    }
                }

                return true;
            };

    const int progressDelta = 250;
    int       ii = 0;

    thread_pool&                   tp = GetKiCadThreadPool();
    std::vector<std::future<bool>> returns;

    returns.reserve( m_drcEngine->GetBoard()->Tracks().size() );

    for( PCB_TRACK* item : m_drcEngine->GetBoard()->Tracks() )
    {
        returns.emplace_back( tp.submit( checkTrackAngle, item ) );
    }

    for( std::future<bool>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( ii++, m_drcEngine->GetBoard()->Tracks().size(), progressDelta );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TRACK_ANGLE> dummy;
}
