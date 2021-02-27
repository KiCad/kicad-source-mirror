/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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
#include <track.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

/*
    Via/pad annular ring width test. Checks if there's sufficient copper ring around
    PTH/NPTH holes (vias/pads)
    Errors generated:
    - DRCE_ANNULUS

    Todo:
    - check pad holes too.
    - pad stack support (different IAR/OAR values depending on layer)
*/

class DRC_TEST_PROVIDER_ANNULUS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ANNULUS()
    {
    }

    virtual ~DRC_TEST_PROVIDER_ANNULUS()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "annulus";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests pad/via annular rings";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_ANNULUS::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
    {
        reportAux( "Annular width violations ignored. Skipping check." );
        return true;    // continue with other tests
    }

    const int delta = 250;  // This is the number of tests between 2 calls to the progress bar

    if( !m_drcEngine->HasRulesForConstraintType( ANNULAR_WIDTH_CONSTRAINT ) )
    {
        reportAux( "No annulus constraints found. Tests not run." );
        return true;    // continue with other tests
    }

    if( !reportPhase( _( "Checking via annular rings..." ) ) )
        return false;   // DRC cancelled

    auto checkAnnulus =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ANNULAR_WIDTH ) )
                    return false;

                int  v_min = 0;
                int  v_max = 0;
                VIA* via = dyn_cast<VIA*>( item );

                // fixme: check minimum IAR/OAR ring for THT pads too
                if( !via )
                    return true;

                // PADSTACKS TODO: once we have padstacks we'll need to run this per-layer....
                auto constraint = m_drcEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, via, nullptr,
                                                          UNDEFINED_LAYER );
                int  annulus = ( via->GetWidth() - via->GetDrillValue() ) / 2;
                bool fail_min = false;
                bool fail_max = false;

                if( constraint.Value().HasMin() )
                {
                    v_min = constraint.Value().Min();
                    fail_min = annulus < v_min;
                }

                if( constraint.Value().HasMax() )
                {
                    v_max = constraint.Value().Max();
                    fail_max = annulus > v_max;
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ANNULAR_WIDTH );

                    if( fail_min )
                        m_msg.Printf( _( "(%s min annular width %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), v_min ),
                                      MessageTextFromValue( userUnits(), annulus ) );

                    if( fail_max )
                        m_msg.Printf( _( "(%s max annular width %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), v_max ),
                                      MessageTextFromValue( userUnits(), annulus ) );

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, via->GetPosition() );
                }

                return true;
            };

    BOARD* board = m_drcEngine->GetBoard();
    int    ii = 0;

    for( TRACK* item : board->Tracks() )
    {
        if( !reportProgress( ii++, board->Tracks().size(), delta ) )
            break;

        if( !checkAnnulus( item ) )
            return false;   // DRC cancelled
    }

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_ANNULUS::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_ANNULUS::GetConstraintTypes() const
{
    return { ANNULAR_WIDTH_CONSTRAINT };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ANNULUS> dummy;
}
