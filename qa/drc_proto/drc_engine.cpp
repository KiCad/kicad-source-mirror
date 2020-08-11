/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2020 KiCad Developers, see change_log.txt for contributors.
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

// fixme - way too much includes
#include <fctsys.h>
#include <pcbnew.h>

#include <reporter.h>
#include <widgets/progress_reporter.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_rule_parser.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_test_provider.h>


test::DRC_ENGINE::DRC_ENGINE( BOARD* aBoard, BOARD_DESIGN_SETTINGS *aSettings ) : 
    m_board( aBoard ),
    m_designSettings ( aSettings ),
    m_reporter( nullptr ),
    m_progressReporter( nullptr )
    {

    }

test::DRC_ENGINE::~DRC_ENGINE()
{
}

test::DRC_REPORT::~DRC_REPORT()
{
    for( auto item : m_entries )
    {
        if ( item.m_marker )
            delete item.m_marker;
    }
}

/*void test::DRC_ENGINE::AddMarker( MARKER_PCB* aMarker )
{
    if( m_designSettings->Ignore( aMarker->GetRCItem()->GetErrorCode() ) )
    {
        delete aMarker;
        return;
    }

    m_markers.push_back( aMarker );
}*/

bool test::DRC_ENGINE::LoadRules( wxFileName aPath )
{
    
    if( aPath.FileExists() )
    {
        m_ruleConditions.clear();
        m_rules.clear();

        FILE* fp = wxFopen( aPath.GetFullPath(), wxT( "rt" ) );

        if( fp )
        {
            try
            {
                DRC_RULES_PARSER parser( m_board, fp, aPath.GetFullPath() );
                parser.Parse( m_rules, m_reporter );
            }
            catch( PARSE_ERROR& pe )
            {
                // Don't leave possibly malformed stuff around for us to trip over
                m_ruleConditions.clear();
                m_rules.clear();

                //wxSafeYield( m_editFrame );
                //m_editFrame->ShowBoardSetupDialog( _( "Rules" ), pe.What(), ID_RULES_EDITOR,
                //                                   pe.lineNumber, pe.byteIndex );

                throw;

                return false;
            }
        }
    }

    return true;
}


test::DRC_TEST_PROVIDER *drcCreateClearanceTestProvider( test::DRC_ENGINE *engine );



void test::DRC_ENGINE::inferImplicitRules()
{
    ReportAux( wxString::Format( "Inferring implicit rules (per-item/class overrides, etc...)" ) );
}


bool test::DRC_ENGINE::CompileRules()
{
    ReportAux( wxString::Format( "Compiling Rules (%d rules, %d conditions): ", m_rules.size(), m_ruleConditions.size() ) );

    for( auto provider : m_testProviders )
    {
        ReportAux( wxString::Format( "- Provider: '%s': ", provider->GetName() ) );

        for ( auto id : provider->GetMatchingConstraintIds() )
        {
            if( m_ruleMap.find(id) == m_ruleMap.end() )
                m_ruleMap[id] = new RULE_SET;

            m_ruleMap[ id ]->provider = provider;

            for( auto rule : m_rules )
            {
                drc_dbg(10, "Scan provider %s rule %s",  provider->GetName() );

                if( ! rule->IsEnabled() )
                    continue;

                for( auto& constraint : rule->Constraints() )
                {
                    if( constraint.GetType() != id )
                        continue;

                   ReportAux( wxString::Format( "   |- Rule: '%s' ", rule->m_Name ) );

                        auto rcons = new RULE_WITH_CONDITIONS;

                        if( rule->IsConditional() )
                        {
                            test::DRC_RULE_CONDITION* condition = rule->Condition();
                            rcons->conditions.push_back( condition );

                            bool compileOk = condition->Compile( nullptr, 0, 0 ); // fixme

                            ReportAux( wxString::Format( "       |- condition: '%s' compile: %s", condition->GetExpression(), compileOk ? "OK" : "ERROR") );
                        }

                        rcons->rule = rule;
                        m_ruleMap[ id ]->sortedRules.push_back( rcons );

                }
            }
        }
    }

    return true;
}


void test::DRC_ENGINE::RunTests( )
{
    //m_largestClearance = m_designSettings->GetBiggestClearanceValue();

    m_drcReport.reset( new test::DRC_REPORT );
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( auto provider : m_testProviders )
    {
        ReportAux( wxString::Format( "Create DRC provider: '%s'", provider->GetName() ) );
        provider->SetDRCEngine( this );
    }


    inferImplicitRules();
    CompileRules();

    for( auto provider : m_testProviders )
    {
        bool skipProvider = false;

        for( auto ruleID : provider->GetMatchingConstraintIds() )
        {
            if( !HasCorrectRulesForId( ruleID ) )
            {
                ReportAux( wxString::Format( "DRC provider '%s' has no rules provided. Skipping run.", provider->GetName() ) );
                skipProvider = true;
                break;
            }
        }

        if( skipProvider )
            continue;

        drc_dbg(0, "Running test provider: '%s'\n", provider->GetName() );
        ReportAux( wxString::Format( "Run DRC provider: '%s'", provider->GetName() ) );
        provider->Run();
    }
}


const test::DRC_CONSTRAINT& test::DRC_ENGINE::EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T aConstraintId, BOARD_ITEM* a, BOARD_ITEM* b, PCB_LAYER_ID aLayer )
{
    test::DRC_RULE* rv;
    auto ruleset = m_ruleMap[ aConstraintId ];

    for( auto rcond : ruleset->sortedRules )
    {
        for( auto condition : rcond->conditions )
        {
            drc_dbg( 8, "   -> check condition '%s'\n",
                    condition->GetExpression() );

            bool result = condition->EvaluateFor( a, b, aLayer ); // FIXME: need the actual layer
            if( result )
            {
                drc_dbg( 8, "   -> rule '%s' matches, triggered by condition '%s'\n",
                        rcond->rule->GetName(),
                        condition->GetExpression() );
                for( const DRC_CONSTRAINT& c : rcond->rule->Constraints() )
                {
                    if( c.GetType() == aConstraintId )
                        return c;
                }
            }
        }
    }

    assert(false); // should never hapen
}


void test::DRC_ENGINE::Report( std::shared_ptr<DRC_ITEM> aItem, ::MARKER_PCB *aMarker )
{
    m_drcReport->AddItem( aItem, aMarker );
    
    if( m_reporter )
    {
        m_reporter->Report ( wxString::Format( "Test '%s': violation of rule '%s' : %s (code %d)", 
        aItem->GetViolatingTest()->GetName(),
        aItem->GetViolatingRule()->GetName(),
        aItem->GetErrorMessage(),
        aItem->GetErrorCode() ), RPT_SEVERITY_ERROR /* fixme */ );

        wxString violatingItemsStr = "Violating items: ";

        if( aMarker )
        {
           m_reporter->Report( wxString::Format( "  |- violating position (%d, %d)", aMarker->GetPos().x, aMarker->GetPos().y ),
           RPT_SEVERITY_ERROR /* fixme */  );
        }
    }
}

void test::DRC_ENGINE::ReportAux ( const wxString& aStr )
{
    if( !m_reporter )
        return;

    m_reporter->Report( aStr, RPT_SEVERITY_INFO );
}


void test::DRC_ENGINE::ReportProgress( double aProgress )
{
    if( !m_progressReporter )
        return;

    m_progressReporter->SetCurrentProgress( aProgress );
}

void test::DRC_ENGINE::ReportStage ( const wxString& aStageName, int index, int total )
{
    if( !m_progressReporter )
        return;

    m_progressReporter->BeginPhase( index ); // fixme: coalesce all stages/test providers 
}

#if 0
test::DRC_CONSTRAINT test::DRC_ENGINE::GetWorstGlobalConstraint( test::DRC_CONSTRAINT_TYPE_T ruleID )
{
    DRC_CONSTRAINT rv;

    rv.m_Value.SetMin( std::numeric_limits<int>::max() );
    rv.m_Value.SetMax( std::numeric_limits<int>::min() );
    for( auto rule : QueryRulesById( ruleID ) )
    {
        auto mm = rule->GetConstraint().m_Value;
        if( mm.HasMax() )
            rv.m_Value.SetMax( std::max( mm.Max(), rv.m_Value.Max() ) );
        if( mm.HasMin() )
            rv.m_Value.SetMin( std::min( mm.Min(), rv.m_Value.Min() ) );
    }

    return rv;
}
#endif

std::vector<test::DRC_CONSTRAINT> test::DRC_ENGINE::QueryConstraintsById( test::DRC_CONSTRAINT_TYPE_T constraintID )
{
    std::vector<test::DRC_CONSTRAINT> rv;

    for( auto rule : m_ruleMap[constraintID]->sortedRules )
    {
        assert( rule );
        assert( rule->rule );

        for( const DRC_CONSTRAINT& c : rule->constraints )
            if( c.GetType() == constraintID )
                rv.push_back( c );
    }

    return rv;
}


bool test::DRC_ENGINE::HasCorrectRulesForId( test::DRC_CONSTRAINT_TYPE_T ruleID )
{
    return m_ruleMap[ruleID]->sortedRules.size() != 0;
}


bool test::DRC_ENGINE::QueryWorstConstraint( test::DRC_CONSTRAINT_TYPE_T aConstraintId, test::DRC_CONSTRAINT& aConstraint, test::DRC_CONSTRAINT_QUERY_T aQueryType )
{
    if( aQueryType == DRCCQ_LARGEST_MINIMUM )
    {
        int worst = 0;
        for( const auto constraint : QueryConstraintsById( aConstraintId ) )
        {
            if( constraint.GetValue().HasMin() )
            {
                int current = constraint.GetValue().Min();

                if( current > worst )
                {
                    worst = current;
                    aConstraint = constraint;
                }
            }
        }

        return worst > 0;
    }

    return false;
}
