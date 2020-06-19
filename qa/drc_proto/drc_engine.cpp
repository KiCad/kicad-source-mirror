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
    for( DRC_ITEM* item : m_drcItems )
        delete item;
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
                parser.Parse( m_ruleConditions, m_rules );
            }
            catch( PARSE_ERROR& pe )
            {
                // Don't leave possibly malformed stuff around for us to trip over
                m_ruleConditions.clear();
                m_rules.clear();

                //wxSafeYield( m_editFrame );
                //m_editFrame->ShowBoardSetupDialog( _( "Rules" ), pe.What(), ID_RULES_EDITOR,
                  //                                 pe.lineNumber, pe.byteIndex );

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


static const int drc_debug_level = 10;

void test::drc_dbg( int level, const char* fmt, ... )
{
#ifdef DEBUG
    if(level < drc_debug_level) // fixme: tom's debugging.
    {
        va_list ap;
        va_start( ap, fmt );
        fprintf( stderr, "drc: " );
        vfprintf( stderr, fmt, ap );
        va_end( ap );
    }
#endif
}


bool test::DRC_ENGINE::CompileRules()
{
    ReportAux( wxString::Format( "Compiling Rules (%d rules, %d conditions): ", m_rules.size(), m_ruleConditions.size() ) );

    for( auto provider : m_testProviders )
    {
        ReportAux( wxString::Format( "- Provider: '%s': ", provider->GetName() ) );

        for ( auto id : provider->GetMatchingRuleIds() )
        {
            if( m_ruleMap.find(id) == m_ruleMap.end() )
                m_ruleMap[id] = new RULE_SET;
            
            m_ruleMap[ id ]->provider = provider;
            m_ruleMap[ id ]->defaultRule = nullptr;

            for( auto rule : m_rules )
            {   
                if( rule->GetTestProviderName() == provider->GetName() )
                {
                   ReportAux( wxString::Format( "   |- Rule: '%s' ", rule->m_Name.c_str() ) );

                   if( rule->IsEnabled() )
                   {
                        auto rcons = new RULE_WITH_CONDITIONS;

                        if( rule->GetPriority() == 0 )
                        {
                            m_ruleMap[ id ]->defaultRule = rule;
                            continue;
                        }

                       for( auto condition : m_ruleConditions )
                       {
                           if( condition->m_TargetRuleName == rule->GetName() )
                           {
                                rcons->conditions.push_back( condition );
                                
                                bool compileOk = condition->Compile();

                                ReportAux( wxString::Format( "       |- condition: '%s' compile: %s", condition->m_TargetRuleName, compileOk ? "OK" : "ERROR") );

                                rcons->rule = rule;
                                m_ruleMap[ id ]->sortedRules.push_back( rcons );
                           }
                       }
                   }
                }
            }
        }
    }

    return true;
}


void test::DRC_ENGINE::RunTests( )
{
    //m_largestClearance = m_designSettings->GetBiggestClearanceValue();

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
        drc_dbg(0, "Running test provider: '%s'\n", (const char *) provider->GetName().c_str() );
        ReportAux( wxString::Format( "Run DRC provider: '%s'", provider->GetName() ) );
        provider->Run();
    }
}


test::DRC_RULE* test::DRC_ENGINE::EvalRulesForItems( test::DRC_RULE_ID_T ruleID, BOARD_ITEM* a, BOARD_ITEM* b  )
{
    test::DRC_RULE* rv;
    auto ruleset = m_ruleMap[ ruleID ];

    for( auto rcond : ruleset->sortedRules )
    {
        for( auto condition : rcond->conditions )
        {
            bool result = condition->EvaluateFor( a, b );
            if( result )
            {
                drc_dbg(8, "   -> rule '%s' matches, triggered by condition '%s'\n", (const char*) rcond->rule->m_Name.c_str(), (const char*) condition->m_Expression );
                return rcond->rule;
            }
        }
    }

    if( ruleset->defaultRule )
    {
        drc_dbg(8, "   -> default rule '%s' matches\n", (const char*) ruleset->defaultRule->m_Name.c_str() );
        return ruleset->defaultRule;
    }

    assert(false); // should never hapen
    return nullptr;
}


void test::DRC_ENGINE::Report( DRC_ITEM* aItem, ::MARKER_PCB *aMarker )
{
    m_drcItems.push_back( aItem );
    m_markers.push_back( aMarker );
    if( m_reporter )
    {
        m_reporter->Report ( wxString::Format( "Test '%s': violation of rule '%s' : %s (code %d)", 
         aItem->GetViolatingTest()->GetName(),
         aItem->GetViolatingRule()->GetName(),
         aItem->GetErrorMessage(),
         aItem->GetErrorCode() ), RPT_SEVERITY_ERROR /* fixme */ );
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


