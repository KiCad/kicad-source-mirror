/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers.
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

#include <pcb_text.h>
#include <fp_text.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    Text dimensions tests.
    Errors generated:
    - DRCE_TEXT_HEIGHT
    - DRCE_TEXT_THICKNESS
*/

class DRC_TEST_PROVIDER_TEXT_DIMS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TEXT_DIMS()
    {
    }

    virtual ~DRC_TEST_PROVIDER_TEXT_DIMS()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "text_dimensions";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests text height and thickness";
    }
};


bool DRC_TEST_PROVIDER_TEXT_DIMS::Run()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar
    int       count = 0;
    int       ii = 0;

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_THICKNESS ) )
    {
        reportAux( "Text dimension violations ignored. Tests not run." );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( TEXT_HEIGHT_CONSTRAINT )
            && !m_drcEngine->HasRulesForConstraintType( TEXT_THICKNESS_CONSTRAINT ) )
    {
        reportAux( "No text height or text thickness constraints found. Tests not run." );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking text dimensions..." ) ) )
        return false;       // DRC cancelled

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            };

    auto checkTextDims =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT )
                        && m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_THICKNESS ) )
                {
                    return false;
                }

                if( !reportProgress( ii++, count, delta ) )
                    return false;

                PCB_TEXT*      textItem = static_cast<PCB_TEXT*>( item );
                DRC_CONSTRAINT constraint;

                if( !textItem->IsVisible() )
                    return true;

                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_HEIGHT ) )
                {
                    constraint = m_drcEngine->EvalRules( TEXT_HEIGHT_CONSTRAINT, item, nullptr,
                                                         item->GetLayer() );
                    bool fail_min = false;
                    bool fail_max = false;
                    int  constraintHeight;
                    int  actual = textItem->GetTextHeight();

                    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                    {
                        if( constraint.Value().HasMin() && actual < constraint.Value().Min() )
                        {
                            fail_min         = true;
                            constraintHeight = constraint.Value().Min();
                        }

                        if( constraint.Value().HasMax() && actual > constraint.Value().Max() )
                        {
                            fail_max         = true;
                            constraintHeight = constraint.Value().Max();
                        }
                    }

                    if( fail_min || fail_max )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_HEIGHT );

                        if( fail_min )
                        {
                            m_msg.Printf( _( "(%s min height %s; actual %s)" ),
                                          constraint.GetName(),
                                          MessageTextFromValue( userUnits(), constraintHeight ),
                                          MessageTextFromValue( userUnits(), actual ) );
                        }
                        else
                        {
                            m_msg.Printf( _( "(%s max height %s; actual %s)" ),
                                          constraint.GetName(),
                                          MessageTextFromValue( userUnits(), constraintHeight ),
                                          MessageTextFromValue( userUnits(), actual ) );
                        }

                        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
                        drcItem->SetItems( item );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, item->GetPosition() );
                    }
                }

                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_THICKNESS ) )
                {
                    constraint = m_drcEngine->EvalRules( TEXT_THICKNESS_CONSTRAINT, item, nullptr,
                                                          item->GetLayer() );
                    bool fail_min = false;
                    bool fail_max = false;
                    int  constraintThickness;
                    int  actual = textItem->GetTextThickness();

                    if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                    {
                        if( constraint.Value().HasMin() && actual < constraint.Value().Min() )
                        {
                            fail_min            = true;
                            constraintThickness = constraint.Value().Min();
                        }

                        if( constraint.Value().HasMax() && actual > constraint.Value().Max() )
                        {
                            fail_max            = true;
                            constraintThickness = constraint.Value().Max();
                        }
                    }

                    if( fail_min || fail_max )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TEXT_THICKNESS );

                        if( fail_min )
                        {
                            m_msg.Printf( _( "(%s min thickness %s; actual %s)" ),
                                          constraint.GetName(),
                                          MessageTextFromValue( userUnits(), constraintThickness ),
                                          MessageTextFromValue( userUnits(), actual ) );
                        }
                        else
                        {
                            m_msg.Printf( _( "(%s max thickness %s; actual %s)" ),
                                          constraint.GetName(),
                                          MessageTextFromValue( userUnits(), constraintThickness ),
                                          MessageTextFromValue( userUnits(), actual ) );
                        }

                        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
                        drcItem->SetItems( item );
                        drcItem->SetViolatingRule( constraint.GetParentRule() );

                        reportViolation( drcItem, item->GetPosition() );
                    }
                }

                return true;
            };

    static const std::vector<KICAD_T> itemTypes = { PCB_TEXT_T, PCB_FP_TEXT_T };

    forEachGeometryItem( itemTypes, LSET::AllLayersMask(), countItems );
    forEachGeometryItem( itemTypes, LSET::AllLayersMask(), checkTextDims );

    reportRuleStatistics();

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TEXT_DIMS> dummy;
}
