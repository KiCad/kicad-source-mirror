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

#include <pcb_track.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    Track width test. As the name says, checks width of the tracks (including segments and arcs)
    Errors generated:
    - DRCE_TRACK_WIDTH
*/

class DRC_TEST_PROVIDER_TRACK_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TRACK_WIDTH()
    {}

    virtual ~DRC_TEST_PROVIDER_TRACK_WIDTH() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "width" ); };
};


bool DRC_TEST_PROVIDER_TRACK_WIDTH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_WIDTH ) )
    {
        REPORT_AUX( wxT( "Track width violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( TRACK_WIDTH_CONSTRAINT ) )
    {
        REPORT_AUX( wxT( "No track width constraints found. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking track widths..." ) ) )
        return false;       // DRC cancelled

    auto checkTrackWidth =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_WIDTH ) )
                    return false;

                int      actual;
                VECTOR2I p0;

                if( item->Type() == PCB_ARC_T )
                {
                    PCB_ARC* arc = static_cast<PCB_ARC*>( item );

                    actual = arc->GetWidth();
                    p0 = arc->GetStart();
                }
                else if( item->Type() == PCB_TRACE_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

                    actual = track->GetWidth();
                    p0 = ( track->GetStart() + track->GetEnd() ) / 2;
                }
                else
                {
                    return true;
                }

                auto constraint = m_drcEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, item, nullptr,
                                                          item->GetLayer() );
                bool fail_min = false;
                bool fail_max = false;
                int  constraintWidth = 0;

                if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                {
                    if( constraint.Value().HasMin() && actual < constraint.Value().Min() )
                    {
                        fail_min        = true;
                        constraintWidth = constraint.Value().Min();
                    }

                    if( constraint.Value().HasMax() && actual > constraint.Value().Max() )
                    {
                        fail_max        = true;
                        constraintWidth = constraint.Value().Max();
                    }
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACK_WIDTH );
                    wxString constraintName = constraint.GetName();

                    if( fail_min )
                    {
                        if( constraint.m_ImplicitMin )
                            constraintName = _( "board setup constraints" );

                        drcItem->SetErrorDetail( formatMsg( _( "(%s min width %s; actual %s)" ),
                                                            constraintName,
                                                            constraintWidth,
                                                            actual ) );
                    }
                    else
                    {
                        drcItem->SetErrorDetail( formatMsg( _( "(%s max width %s; actual %s)" ),
                                                            constraintName,
                                                            constraintWidth,
                                                            actual ) );
                    }

                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, p0, item->GetLayer() );
                }

                return true;
            };

    const int progressDelta = 250;
    int       ii = 0;

    for( PCB_TRACK* item : m_drcEngine->GetBoard()->Tracks() )
    {
        if( !reportProgress( ii++, m_drcEngine->GetBoard()->Tracks().size(), progressDelta ) )
            break;

        if( !checkTrackWidth( item ) )
            break;
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TRACK_WIDTH> dummy;
}
