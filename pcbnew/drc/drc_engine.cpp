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

#include <drc/drc_engine.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include "drc.h"

void drcPrintDebugMessage( int level, wxString msg, const char *function, int line )
{
    wxString valueStr;

    if( wxGetEnv( "DRC_DEBUG", &valueStr ) )
    {
        int setLevel = wxAtoi( valueStr );

        if( level <= setLevel )
        {
            printf("%-30s:%d | %s\n", function, line, (const char *) msg.c_str() );
        }
    }
}


DRC_ENGINE::DRC_ENGINE( BOARD* aBoard, BOARD_DESIGN_SETTINGS *aSettings ) :
    m_designSettings ( aSettings ),
    m_board( aBoard ),
    m_worksheet( nullptr ),
    m_schematicNetlist( nullptr ),
    m_reporter( nullptr ),
    m_progressReporter( nullptr )
{
    m_errorLimits.resize( DRCE_LAST );
}


DRC_ENGINE::~DRC_ENGINE()
{
}


DRC_REPORT::~DRC_REPORT()
{
}


/*void DRC_ENGINE::AddMarker( MARKER_PCB* aMarker )
{
    if( m_designSettings->Ignore( aMarker->GetRCItem()->GetErrorCode() ) )
    {
        delete aMarker;
        return;
    }

    m_markers.push_back( aMarker );
}*/


DRC_RULE* DRC_ENGINE::createInferredRule( const wxString& name, std::set<BOARD_ITEM*> items, int priority )
{
    DRC_RULE *rule = new DRC_RULE;

    rule->m_Name = name;
    if (! items.empty() )
        rule->FillSpecificItemSet( items );

    rule->SetPriority( priority );

    addRule( rule );

    return rule;
}


void DRC_ENGINE::inferLegacyRules()
{
    int priorityRangeMin = INT_MIN + 10000;
    int priorityRangeMax = INT_MAX - 10000;

    ReportAux( wxString::Format( "Inferring implicit rules (per-item/class overrides, etc...)" ) );

    // 1) global defaults

    DRC_RULE* rule = createInferredRule( "inferred-defaults", {}, priorityRangeMin );
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    DRC_CONSTRAINT clearanceConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_CLEARANCE );
    clearanceConstraint.Value().SetMin( bds.m_MinClearance );
    rule->AddConstraint( clearanceConstraint );

    DRC_CONSTRAINT widthConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_TRACK_WIDTH );
    widthConstraint.Value().SetMin( bds.m_TrackMinWidth );
    rule->AddConstraint( widthConstraint );

    DRC_CONSTRAINT drillConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE );
    drillConstraint.Value().SetMin( bds.m_MinThroughDrill );
    rule->AddConstraint( drillConstraint );

    DRC_CONSTRAINT annulusConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_ANNULUS_WIDTH );
    annulusConstraint.Value().SetMin( bds.m_ViasMinAnnulus );
    rule->AddConstraint( annulusConstraint );
    
    DRC_CONSTRAINT diameterConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
    diameterConstraint.Value().SetMin( bds.m_ViasMinSize );
    rule->AddConstraint( diameterConstraint );

    DRC_CONSTRAINT edgeClearanceConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE );
    edgeClearanceConstraint.Value().SetMin( bds.m_CopperEdgeClearance );
    rule->AddConstraint( edgeClearanceConstraint );

    DRC_CONSTRAINT holeClearanceConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE );
    holeClearanceConstraint.Value().SetMin( bds.m_HoleToHoleMin );
    rule->AddConstraint( holeClearanceConstraint );

    DRC_CONSTRAINT courtyardClearanceConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE );
    holeClearanceConstraint.Value().SetMin( 0 );
    rule->AddConstraint( courtyardClearanceConstraint );

    // 2) micro-via specific defaults (new DRC doesn't treat microvias in any special way)

    priorityRangeMin++;

    auto isMicroViaCondition = new DRC_RULE_CONDITION ( "A.type == 'Via' && A.isMicroVia()" );
    DRC_RULE* uViaRule = createInferredRule( "inferred-microvia-defaults", {}, priorityRangeMin  );

    uViaRule->m_Condition = isMicroViaCondition;

    DRC_CONSTRAINT uViaDrillConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE );
    uViaDrillConstraint.Value().SetMin( bds.m_MicroViasMinDrill );
    uViaRule->AddConstraint( uViaDrillConstraint );

    DRC_CONSTRAINT uViaDiameterConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
    uViaDiameterConstraint.Value().SetMin( bds.m_MicroViasMinSize );
    uViaRule->AddConstraint( uViaDiameterConstraint );

    auto isBlindBuriedViaCondition = new DRC_RULE_CONDITION ( "A.type == 'Via' && A.isBlindBuriedVia()" );
    DRC_RULE* blindBuriedViaRule = createInferredRule( "inferred-blind-buried-via-defaults", {}, priorityRangeMin );

    DRC_CONSTRAINT disallowConstraint( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_DISALLOW );

    blindBuriedViaRule->m_Condition = isBlindBuriedViaCondition;

    if( !bds.m_MicroViasAllowed )
    {
        uViaRule->AddConstraint( disallowConstraint );
    }

    if( !bds.m_BlindBuriedViaAllowed )
    {
        blindBuriedViaRule->AddConstraint( disallowConstraint );
    }

    // 3) per-netclass rules

    struct NETCLASS_ENTRY
    {
        wxString name;
        int clearance;
        int width;
    };

    std::vector<NETCLASS_ENTRY> netclassesByClearance, netclassesByWidth;

    m_board->SynchronizeNetsAndNetClasses();

    // fixme: make this conditional for standalone tests
    bds.SetNetClasses( nullptr ); // load legacy

    std::vector<NETCLASSPTR> netclasses;

    netclasses.push_back( bds.GetNetClasses().GetDefault() );
    
    for( const auto& netclass : bds.GetNetClasses() )
        netclasses.push_back( netclass.second );

    ReportAux( wxString::Format( "Importing %d legacy net classes", (int) netclasses.size() ) );

    int i = 0;

    for( const NETCLASSPTR& nc : netclasses )
    {
        wxString className = nc->GetName();

        const auto expr = wxString::Format( "A.NetClass == '%s' || B.NetClass == '%s'", className, className );

        auto inNetclassCondition = new DRC_RULE_CONDITION ( expr );
    
        DRC_RULE* netclassRule = createInferredRule( wxString::Format( "inferred-netclass-clearance-%s", className ),
            {}, priorityRangeMin + i );

        netclassRule->m_Condition = inNetclassCondition;

        DRC_CONSTRAINT ncClearanceConstraint ( DRC_CONSTRAINT_TYPE_CLEARANCE );
        ncClearanceConstraint.Value().SetMin( nc->GetClearance() );
        netclassRule->AddConstraint( ncClearanceConstraint );

        netclassRule = createInferredRule( wxString::Format( "inferred-netclass-width-%s", className ),
            {}, priorityRangeMin + i );

        netclassRule->m_Condition = inNetclassCondition;

        DRC_CONSTRAINT ncWidthConstraint ( DRC_CONSTRAINT_TYPE_TRACK_WIDTH );
        ncWidthConstraint.Value().SetMin( nc->GetTrackWidth() );
        netclassRule->AddConstraint( ncWidthConstraint );

        // TODO: should we import diff pair gaps/widths here?
        i++;
    }

    //clearanceConstraint.SetMin( )

    //rule->AddConstraint( )

}

static wxString formatConstraint( const DRC_CONSTRAINT& constraint )
{
    struct Formatter
    {
        DRC_CONSTRAINT_TYPE_T type;
        wxString              name;
        std::function<wxString(const DRC_CONSTRAINT&)> formatter;
    };

    auto formatMinMax = []( const DRC_CONSTRAINT& c ) -> wxString {
        wxString str;
        const auto value = c.GetValue();

        if ( value.HasMin() )
            str += wxString::Format(" min: %d", value.Min() );
        if ( value.HasOpt() )
            str += wxString::Format(" opt: %d", value.Opt() );
        if ( value.HasMax() )
            str += wxString::Format(" max: %d", value.Max() );

        return str;
    };

    std::vector<Formatter> formats = {
        { DRC_CONSTRAINT_TYPE_UNKNOWN, "unknown", nullptr },
        { DRC_CONSTRAINT_TYPE_CLEARANCE, "clearance", formatMinMax },
        { DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE, "hole_clearance", formatMinMax },
        { DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE, "edge_clearance", formatMinMax },
        { DRC_CONSTRAINT_TYPE_HOLE_SIZE, "hole_size", formatMinMax },
        { DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE, "courtyard_clearance", formatMinMax },
        { DRC_CONSTRAINT_TYPE_SILK_TO_PAD, "silk_to_pad", formatMinMax },
        { DRC_CONSTRAINT_TYPE_SILK_TO_SILK, "silk_to_silk", formatMinMax },
        { DRC_CONSTRAINT_TYPE_TRACK_WIDTH, "track_width", formatMinMax },
        { DRC_CONSTRAINT_TYPE_ANNULUS_WIDTH, "annulus_width", formatMinMax },
        { DRC_CONSTRAINT_TYPE_DISALLOW, "disallow", nullptr }, // fixme
        { DRC_CONSTRAINT_TYPE_VIA_DIAMETER, "via_diameter", formatMinMax }
    };

    for( auto& fmt : formats)
    {
        if( fmt.type == constraint.m_Type )
        {
            wxString rv = fmt.name + " ";
            if( fmt.formatter )
                rv += fmt.formatter( constraint );
            return rv;
        }
    }

    return "?";
}


bool DRC_ENGINE::CompileRules()
{
    ReportAux( wxString::Format( "Compiling Rules (%d rules, %d conditions): ",
                                 (int) m_rules.size(),
                                 (int) m_ruleConditions.size() ) );

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "- Provider: '%s': ", provider->GetName() ) );
        drc_dbg(7, "do prov %s", provider->GetName() );

        for ( DRC_CONSTRAINT_TYPE_T id : provider->GetMatchingConstraintIds() )
        {
            drc_dbg( 7, "do id %d", id );

            if( m_constraintMap.find( id ) == m_constraintMap.end() )
                m_constraintMap[ id ] = new CONSTRAINT_SET;

            m_constraintMap[ id ]->provider = provider;

            for( DRC_RULE* rule : m_rules )
            {
                DRC_RULE_CONDITION* condition = nullptr;
                bool compileOk = false;
                std::vector<DRC_CONSTRAINT> matchingConstraints;
                drc_dbg(7, "Scan provider %s, rule %s",  provider->GetName(), rule->m_Name );

                if( rule->m_Condition && !rule->m_Condition->GetExpression().IsEmpty() )
                {
                    condition = rule->m_Condition;
                    compileOk = condition->Compile( nullptr, 0, 0 ); // fixme
                }

                for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
                {
                    drc_dbg(7, "scan constraint id %d\n", constraint.m_Type );

                    if( constraint.m_Type != id )
                        continue;

                    CONSTRAINT_WITH_CONDITIONS* rcons = new CONSTRAINT_WITH_CONDITIONS;

                    if( condition )
                        rcons->conditions.push_back( condition );

                    matchingConstraints.push_back( constraint );

                    rcons->constraint = constraint;
                    rcons->parentRule = rule;
                    m_constraintMap[ id ]->sortedConstraints.push_back( rcons );
                }

                if( !matchingConstraints.empty() )
                {
                    ReportAux( wxString::Format( "   |- Rule: '%s' ",
                                                 rule->m_Name ) );

                    if( condition )
                    {
                        ReportAux( wxString::Format( "       |- condition: '%s' compile: %s",
                                                     condition->GetExpression(),
                                                     compileOk ? "OK" : "ERROR" ) );
                    }

                    for (const DRC_CONSTRAINT& constraint : matchingConstraints )
                    {
                        ReportAux( wxString::Format( "       |- constraint: %s",
                                                     formatConstraint( constraint ) ) );
                    }
                }
            }
        }
    }

    return true;
}


bool DRC_ENGINE::LoadRules( wxFileName aPath )
{
    NULL_REPORTER nullReporter;
    REPORTER*     reporter = m_reporter ? m_reporter : &nullReporter;

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
                parser.Parse( m_rules, reporter );
            }
            catch( PARSE_ERROR& pe )
            {
                // Don't leave possibly malformed stuff around for us to trip over
                m_ruleConditions.clear();
                m_rules.clear();

                // JEY TODO
                //wxSafeYield( m_editFrame );
                //m_editFrame->ShowBoardSetupDialog( _( "Rules" ), pe.What(), ID_RULES_EDITOR,
                //                                   pe.lineNumber, pe.byteIndex );

                return false;
            }
        }
    }

    return true;
}


void DRC_ENGINE::InitEngine( wxFileName aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "Create DRC provider: '%s'", provider->GetName() ) );
        provider->SetDRCEngine( this );
    }

    LoadRules( aRulePath );
    inferLegacyRules();

    CompileRules();

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = INT_MAX;
}


void DRC_ENGINE::RunTests( )
{
    m_drcReport = std::make_shared<DRC_REPORT>();

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( m_designSettings->Ignore( ii ) )
            m_errorLimits[ ii ] = 0;
        else
            m_errorLimits[ ii ] = INT_MAX;
    }

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        bool skipProvider = false;
        auto matchingConstraints = provider->GetMatchingConstraintIds();

        if( matchingConstraints.size() )
        {
            for( auto ruleID : matchingConstraints )
            {
                if( !HasCorrectRulesForId( ruleID ) )
                {
                    ReportAux( wxString::Format( "DRC provider '%s' has no rules provided. Skipping run.",
                                                 provider->GetName() ) );
                    skipProvider = true;
                    break;
                }
            }
        }

        if( skipProvider )
            continue;

        drc_dbg(0, "Running test provider: '%s'\n", provider->GetName() );
        ReportAux( wxString::Format( "Run DRC provider: '%s'", provider->GetName() ) );
        provider->Run();
    }
}


DRC_CONSTRAINT DRC_ENGINE::EvalRulesForItems( DRC_CONSTRAINT_TYPE_T aConstraintId,
                                              BOARD_ITEM* a, BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                                              REPORTER* aReporter )
{
#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }

    // Local overrides take precedence
    if( aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE )
    {
        BOARD_CONNECTED_ITEM* ac = a->IsConnected() ? static_cast<BOARD_CONNECTED_ITEM*>( a )
                                                    : nullptr;
        BOARD_CONNECTED_ITEM* bc = b->IsConnected() ? static_cast<BOARD_CONNECTED_ITEM*>( b )
                                                    : nullptr;
        int overrideA = 0;
        int overrideB = 0;

        if( ac && ac->GetLocalClearanceOverrides( nullptr ) > 0 )
        {
            overrideA = ac->GetLocalClearanceOverrides( &m_msg );

            REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                      a->GetSelectMenuText( aReporter->GetUnits() ),
                                      StringFromValue( aReporter->GetUnits(), overrideA, true ) ) );
        }

        if( bc && bc->GetLocalClearanceOverrides( nullptr ) > 0 )
        {
            overrideB = bc->GetLocalClearanceOverrides( &m_msg );

            REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                      b->GetSelectMenuText( aReporter->GetUnits() ),
                                      StringFromValue( aReporter->GetUnits(), overrideB, true ) ) );

            // If both overridden, report on which wins
            if( overrideA )
            {
                REPORT( wxString::Format( _( "Clearance: %s." ),
                                          std::max( overrideA, overrideB ) ) );
            }
        }

        if( overrideA || overrideB )
        {
            DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_CLEARANCE, m_msg );
            constraint.m_Value.SetMin( std::max( overrideA, overrideB ) );
            return constraint;
        }
    }

    auto ruleset = m_constraintMap[ aConstraintId ];

    for( const CONSTRAINT_WITH_CONDITIONS* rcond : ruleset->sortedConstraints )
    {
        DRC_RULE* rule = rcond->parentRule;
        REPORT( wxString::Format( _( "Checking rule \"%s\"." ), rule->m_Name ) );

        if( aLayer != UNDEFINED_LAYER && !rule->m_LayerCondition.test( aLayer ) )
        {
            REPORT( wxString::Format( _( "Rule layer \"%s\" not matched." ),
                                      rule->m_LayerSource ) );
            REPORT( "Rule not applied." );

            continue;
        }

        if( rcond->conditions.size() == 0 )  // uconditional
        {
            REPORT( _( "Unconditional constraint; rule applied." ) );

            return rcond->constraint;
        }

        for( DRC_RULE_CONDITION* condition : rcond->conditions )
        {
            bool result = condition->EvaluateFor( a, b, aLayer, aReporter );

            if( result )
            {
                REPORT( _( "Rule applied." ) );
                return rcond->constraint;
            }
            else
            {
                REPORT( _( "Condition not satisfied; rule not applied." ) );
                REPORT( "" );
            }
        }
    }

    // fixme: return optional<drc_constraint>, let the particular test decide what to do if no matching constraint
    // is found
    static DRC_CONSTRAINT nullConstraint;
    nullConstraint.m_DisallowFlags = 0;

    return nullConstraint;
}


void DRC_ENGINE::Report( std::shared_ptr<DRC_ITEM> aItem, MARKER_PCB *aMarker )
{
    m_drcReport->AddItem( aItem, aMarker );

    if( m_reporter )
    {
        wxString msg = wxString::Format( "Test '%s': %s (code %d)",
                                         aItem->GetViolatingTest()->GetName(),
                                         aItem->GetErrorMessage(),
                                         aItem->GetErrorCode() );

        auto rule = aItem->GetViolatingRule();

        if( rule )
            msg += wxString::Format( ", violating rule: '%s'", rule->m_Name );
        
        m_reporter->Report( msg );
        
        wxString violatingItemsStr = "Violating items: ";

        if( aMarker )
        {
           m_reporter->Report( wxString::Format( "  |- violating position (%d, %d)",
                                                 aMarker->GetPos().x,
                                                 aMarker->GetPos().y ) );
        }
    }
}

void DRC_ENGINE::ReportAux ( const wxString& aStr )
{
    if( !m_reporter )
        return;

    m_reporter->Report( aStr, RPT_SEVERITY_INFO );
}


void DRC_ENGINE::ReportProgress( double aProgress )
{
    if( !m_progressReporter )
        return;

    m_progressReporter->SetCurrentProgress( aProgress );
}


void DRC_ENGINE::ReportStage ( const wxString& aStageName, int index, int total )
{
    if( !m_progressReporter )
        return;

    m_progressReporter->SetNumPhases( total );
    m_progressReporter->BeginPhase( index ); // fixme: coalesce all stages/test providers
    m_progressReporter->Report( aStageName );
}


#if 0
DRC_CONSTRAINT DRC_ENGINE::GetWorstGlobalConstraint( DRC_CONSTRAINT_TYPE_T ruleID )
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


std::vector<DRC_CONSTRAINT> DRC_ENGINE::QueryConstraintsById( DRC_CONSTRAINT_TYPE_T constraintID )
{
    std::vector<DRC_CONSTRAINT> rv;
    for ( auto c : m_constraintMap[constraintID]->sortedConstraints )
        rv.push_back(c->constraint);
    return rv;
}


bool DRC_ENGINE::HasCorrectRulesForId( DRC_CONSTRAINT_TYPE_T constraintID )
{
    //drc_dbg(10,"hascorrect id %d size %d\n", ruleID,  m_ruleMap[ruleID]->sortedRules.size( ) );
    return m_constraintMap[constraintID]->sortedConstraints.size() != 0;
}


bool DRC_ENGINE::QueryWorstConstraint( DRC_CONSTRAINT_TYPE_T aConstraintId,
                                       DRC_CONSTRAINT& aConstraint,
                                       DRC_CONSTRAINT_QUERY_T aQueryType )
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
