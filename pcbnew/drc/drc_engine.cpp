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

#include <fctsys.h>
#include <reporter.h>
#include <widgets/progress_reporter.h>
#include <drc/drc_engine.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_test_provider.h>

void drcPrintDebugMessage( int level, const wxString& msg, const char *function, int line )
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
    m_userUnits( EDA_UNITS::MILLIMETRES ),
    m_testTracksAgainstZones( false ),
    m_reportAllTrackErrors( false ),
    m_testFootprints( false ),
    m_reporter( nullptr ),
    m_progressReporter( nullptr )
{
    m_errorLimits.resize( DRCE_LAST );

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = INT_MAX;
}


DRC_ENGINE::~DRC_ENGINE()
{
}


DRC_RULE* DRC_ENGINE::createImplicitRule( const wxString& name )
{
    DRC_RULE *rule = new DRC_RULE;

    rule->m_Name = name;
    rule->m_Implicit = true;

    addRule( rule );

    return rule;
}


void DRC_ENGINE::loadImplicitRules()
{
    ReportAux( wxString::Format( "Building implicit rules (per-item/class overrides, etc...)" ) );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // 1) global defaults

    DRC_RULE* rule = createImplicitRule( _( "board setup constraints" ));

    DRC_CONSTRAINT clearanceConstraint( DRC_CONSTRAINT_TYPE_CLEARANCE );
    clearanceConstraint.Value().SetMin( bds.m_MinClearance );
    rule->AddConstraint( clearanceConstraint );

    DRC_CONSTRAINT widthConstraint( DRC_CONSTRAINT_TYPE_TRACK_WIDTH );
    widthConstraint.Value().SetMin( bds.m_TrackMinWidth );
    rule->AddConstraint( widthConstraint );

    DRC_CONSTRAINT drillConstraint( DRC_CONSTRAINT_TYPE_HOLE_SIZE );
    drillConstraint.Value().SetMin( bds.m_MinThroughDrill );
    rule->AddConstraint( drillConstraint );

    DRC_CONSTRAINT annulusConstraint( DRC_CONSTRAINT_TYPE_ANNULUS_WIDTH );
    annulusConstraint.Value().SetMin( bds.m_ViasMinAnnulus );
    rule->AddConstraint( annulusConstraint );

    DRC_CONSTRAINT diameterConstraint( DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
    diameterConstraint.Value().SetMin( bds.m_ViasMinSize );
    rule->AddConstraint( diameterConstraint );

    DRC_CONSTRAINT edgeClearanceConstraint( DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE );
    edgeClearanceConstraint.Value().SetMin( bds.m_CopperEdgeClearance );
    rule->AddConstraint( edgeClearanceConstraint );

    DRC_CONSTRAINT holeClearanceConstraint( DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE );
    holeClearanceConstraint.Value().SetMin( bds.m_HoleToHoleMin );
    rule->AddConstraint( holeClearanceConstraint );

    DRC_CONSTRAINT courtyardClearanceConstraint( DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE );
    holeClearanceConstraint.Value().SetMin( 0 );
    rule->AddConstraint( courtyardClearanceConstraint );

    // 2) micro-via specific defaults (new DRC doesn't treat microvias in any special way)

    DRC_RULE* uViaRule = createImplicitRule( _( "board setup constraints" ));

    uViaRule->m_Condition = new DRC_RULE_CONDITION ( "A.Via_Type == 'micro_via'" );

    DRC_CONSTRAINT uViaDrillConstraint( DRC_CONSTRAINT_TYPE_HOLE_SIZE );
    uViaDrillConstraint.Value().SetMin( bds.m_MicroViasMinDrill );
    uViaRule->AddConstraint( uViaDrillConstraint );

    DRC_CONSTRAINT uViaDiameterConstraint( DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
    uViaDiameterConstraint.Value().SetMin( bds.m_MicroViasMinSize );
    uViaRule->AddConstraint( uViaDiameterConstraint );

    if( !bds.m_MicroViasAllowed )
    {
        DRC_CONSTRAINT disallowConstraint( DRC_CONSTRAINT_TYPE_DISALLOW );
        disallowConstraint.m_DisallowFlags = DRC_DISALLOW_MICRO_VIAS;
        uViaRule->AddConstraint( disallowConstraint );
    }

    if( !bds.m_BlindBuriedViaAllowed )
    {
        DRC_RULE* bbViaRule = createImplicitRule( _( "board setup constraints" ));

        bbViaRule->m_Condition = new DRC_RULE_CONDITION ( "A.Via_Type == 'buried_via'" );

        DRC_CONSTRAINT disallowConstraint( DRC_CONSTRAINT_TYPE_DISALLOW );
        disallowConstraint.m_DisallowFlags = DRC_DISALLOW_BB_VIAS;
        bbViaRule->AddConstraint( disallowConstraint );
    }

    // 3) per-netclass rules

    int ruleCount = 0;

    auto makeNetclassRule =
            [&]( const NETCLASSPTR& nc, bool isDefault )
            {
                wxString className = nc->GetName();
                wxString expr;

                if( !isDefault )
                {
                    expr = wxString::Format( "A.NetClass == '%s' || B.NetClass == '%s'",
                                             className,
                                             className );
                }

                DRC_RULE_CONDITION* inNetclassCondition = new DRC_RULE_CONDITION ( expr );
                DRC_RULE* rule = createImplicitRule( wxString::Format( _( "netclass '%s'" ),
                                                                       className ));

                rule->m_Condition = inNetclassCondition;

                // Only add netclass clearances if they're larger than board minimums.  That way
                // board minimums will still enforce a global minimum.

                if( nc->GetClearance() > bds.m_MinClearance )
                {
                    DRC_CONSTRAINT ncClearanceConstraint( DRC_CONSTRAINT_TYPE_CLEARANCE );
                    ncClearanceConstraint.Value().SetMin( nc->GetClearance() );
                    rule->AddConstraint( ncClearanceConstraint );
                }

                ruleCount++;
            };

    m_board->SynchronizeNetsAndNetClasses();
    makeNetclassRule( bds.GetNetClasses().GetDefault(), true );

    for( const std::pair<const wxString, NETCLASSPTR>& netclass : bds.GetNetClasses() )
        makeNetclassRule( netclass.second, false );

    ReportAux( wxString::Format( "Building %d implicit netclass rules", ruleCount ) );
}

static wxString formatConstraint( const DRC_CONSTRAINT& constraint )
{
    struct Formatter
    {
        DRC_CONSTRAINT_TYPE_T type;
        wxString              name;
        std::function<wxString(const DRC_CONSTRAINT&)> formatter;
    };

    auto formatMinMax =
            []( const DRC_CONSTRAINT& c ) -> wxString
            {
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

    std::vector<Formatter> formats =
    {
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

    for( auto& fmt : formats )
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


bool DRC_ENGINE::LoadRules( const wxFileName& aPath )
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


bool DRC_ENGINE::CompileRules()
{
    ReportAux( wxString::Format( "Compiling Rules (%d rules, %d conditions): ",
                                 (int) m_rules.size(),
                                 (int) m_ruleConditions.size() ) );

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "- Provider: '%s': ", provider->GetName() ) );
        drc_dbg( 7, "do prov %s", provider->GetName() );

        for( DRC_CONSTRAINT_TYPE_T id : provider->GetConstraintTypes() )
        {
            drc_dbg( 7, "do id %d", id );

            if( m_constraintMap.find( id ) == m_constraintMap.end() )
                m_constraintMap[ id ] = new std::vector<CONSTRAINT_WITH_CONDITIONS*>();

            for( DRC_RULE* rule : m_rules )
            {
                DRC_RULE_CONDITION* condition = nullptr;
                bool compileOk = false;
                std::vector<DRC_CONSTRAINT> matchingConstraints;
                drc_dbg( 7, "Scan provider %s, rule %s",  provider->GetName(), rule->m_Name );

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

                    rcons->layerTest = rule->m_LayerCondition;
                    rcons->condition = condition;

                    matchingConstraints.push_back( constraint );

                    rcons->constraint = constraint;
                    rcons->parentRule = rule;
                    m_constraintMap[ id ]->push_back( rcons );
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


void DRC_ENGINE::InitEngine( const wxFileName& aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "Create DRC provider: '%s'", provider->GetName() ) );
        provider->SetDRCEngine( this );
    }

    LoadRules( aRulePath );
    loadImplicitRules();

    CompileRules();

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = INT_MAX;
}


void DRC_ENGINE::RunTests( EDA_UNITS aUnits, bool aTestTracksAgainstZones,
                           bool aReportAllTrackErrors, bool aTestFootprints )
{
    m_userUnits = aUnits;

    // Note: set these first.  The phase counts may be dependent on some of them.
    m_testTracksAgainstZones = aTestTracksAgainstZones;
    m_reportAllTrackErrors = aReportAllTrackErrors;
    m_testFootprints = aTestFootprints;

    int phases = 0;

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
        phases += provider->GetNumPhases();

    if( m_progressReporter )
        m_progressReporter->AddPhases( phases );

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( m_designSettings->Ignore( ii ) )
            m_errorLimits[ ii ] = 0;
        else
            m_errorLimits[ ii ] = INT_MAX;
    }

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        drc_dbg( 0, "Running test provider: '%s'\n", provider->GetName() );

        ReportAux( wxString::Format( "Run DRC provider: '%s'", provider->GetName() ) );
        provider->Run();
    }
}


DRC_CONSTRAINT DRC_ENGINE::EvalRulesForItems( DRC_CONSTRAINT_TYPE_T aConstraintId,
                                              const BOARD_ITEM* a, const BOARD_ITEM* b,
                                              PCB_LAYER_ID aLayer, REPORTER* aReporter )
{
#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }
#define UNITS aReporter ? aReporter->GetUnits() : EDA_UNITS::MILLIMETRES

    const BOARD_CONNECTED_ITEM* connectedA = dynamic_cast<const BOARD_CONNECTED_ITEM*>( a );
    const BOARD_CONNECTED_ITEM* connectedB = dynamic_cast<const BOARD_CONNECTED_ITEM*>( b );

    // Local overrides take precedence
    if( aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE )
    {
        int overrideA = 0;
        int overrideB = 0;

        if( connectedA && connectedA->GetLocalClearanceOverrides( nullptr ) > 0 )
        {
            overrideA = connectedA->GetLocalClearanceOverrides( &m_msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                      a->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, overrideA, true ) ) )
        }

        if( connectedB && connectedB->GetLocalClearanceOverrides( nullptr ) > 0 )
        {
            overrideB = connectedB->GetLocalClearanceOverrides( &m_msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                      b->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, overrideB, true ) ) )
        }

        if( overrideA || overrideB )
        {
            DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_CLEARANCE, m_msg );
            constraint.m_Value.SetMin( std::max( overrideA, overrideB ) );
            return constraint;
        }
    }

    std::vector<CONSTRAINT_WITH_CONDITIONS*>* ruleset = m_constraintMap[ aConstraintId ];

    const DRC_CONSTRAINT* constraintRef = nullptr;
    bool                  implicit = false;

    // Last matching rule wins, so process in reverse order
    for( int ii = (int) ruleset->size() - 1; ii >= 0; --ii )
    {
        wxString msg;
        const CONSTRAINT_WITH_CONDITIONS* rcons = ruleset->at( ii );
        implicit = rcons->parentRule && rcons->parentRule->m_Implicit;

        REPORT( "" )

        if( implicit )
        {
            msg = wxString::Format( _( "Checking %s;" ),
                                    rcons->parentRule->m_Name );
        }
        else
        {
            msg = wxString::Format( _( "Checking rule %s;"),
                                    rcons->parentRule->m_Name );
        }

        if( aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE )
        {
            int clearance = rcons->constraint.m_Value.Min();

            msg += " ";
            msg += wxString::Format( _( "clearance: %s." ),
                                     MessageTextFromValue( UNITS, clearance, true ) );
        }

        REPORT( msg );

        if( aLayer != UNDEFINED_LAYER && !rcons->layerTest.test( aLayer ) )
        {
            REPORT( wxString::Format( _( "Rule layer \"%s\" not matched." ),
                                      rcons->parentRule->m_LayerSource ) )
            REPORT( "Rule not applied." )

            continue;
        }

        if( !rcons->condition || rcons->condition->GetExpression().IsEmpty() )
        {
            REPORT( implicit ? _( "Unconditional constraint applied." )
                             : _( "Unconditional rule applied." ) )

            constraintRef = &rcons->constraint;
            break;
        }
        else
        {
            // Don't report on implicit rule conditions; they're synthetic.
            if( !implicit )
            {
                REPORT( wxString::Format( _( "Checking rule condition \"%s\"." ),
                                          rcons->condition->GetExpression() ) )
            }

            if( rcons->condition->EvaluateFor( a, b, aLayer, aReporter ) )
            {
                REPORT( implicit ? _( "Constraint applicable." )
                                 : _( "Rule applied." ) )

                constraintRef = &rcons->constraint;
                break;
            }
            else
            {
                REPORT( implicit ? _( "Membership not satisfied; constraint not applicable." )
                                 : _( "Condition not satisfied; rule not applied." ) )
            }
        }
    }

    // Unfortunately implicit rules don't work for local clearances (such as zones) because
    // they have to be max'ed with netclass values (which are already implicit rules), and our
    // rule selection paradigm is "winner takes all".
    if( aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE && implicit )
    {
        int global = constraintRef->m_Value.Min();
        int localA = connectedA ? connectedA->GetLocalClearance( nullptr ) : 0;
        int localB = connectedB ? connectedB->GetLocalClearance( nullptr ) : 0;
        int clearance = global;

        if( localA > 0 )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local clearance on %s; clearance: %s." ),
                                      a->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, localA, true ) ) )

            if( localA > clearance )
                clearance = connectedA->GetLocalClearance( &m_msg );
        }

        if( localB > 0 )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local clearance on %s; clearance: %s." ),
                                      b->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, localB, true ) ) )

            if( localB > clearance )
                clearance = connectedB->GetLocalClearance( &m_msg );
        }

        if( localA > global || localB > global )
        {
            DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_CLEARANCE, m_msg );
            constraint.m_Value.SetMin( clearance );
            return constraint;
        }
    }

    // fixme: return optional<drc_constraint>, let the particular test decide what to do if no matching constraint
    // is found
    static DRC_CONSTRAINT nullConstraint;
    nullConstraint.m_DisallowFlags = 0;

    return constraintRef ? *constraintRef : nullConstraint;

#undef REPORT
#undef UNITS
}


bool DRC_ENGINE::IsErrorLimitExceeded( int error_code )
{
    return m_errorLimits[ error_code ] <= 0;
}


void DRC_ENGINE::ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
{
    m_errorLimits[ aItem->GetErrorCode() ] -= 1;

    if( m_violationHandler )
        m_violationHandler( aItem, aPos );

    if( m_reporter )
    {
        wxString msg = wxString::Format( "Test '%s': %s (code %d)",
                                         aItem->GetViolatingTest()->GetName(),
                                         aItem->GetErrorMessage(),
                                         aItem->GetErrorCode() );

        DRC_RULE* rule = aItem->GetViolatingRule();

        if( rule )
            msg += wxString::Format( ", violating rule: '%s'", rule->m_Name );

        m_reporter->Report( msg );

        wxString violatingItemsStr = "Violating items: ";

        m_reporter->Report( wxString::Format( "  |- violating position (%d, %d)",
                                              aPos.x,
                                              aPos.y ) );
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


void DRC_ENGINE::ReportPhase( const wxString& aMessage )
{
    if( !m_progressReporter )
        return;

    m_progressReporter->AdvancePhase( aMessage );
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

    for ( CONSTRAINT_WITH_CONDITIONS* c : *m_constraintMap[constraintID] )
        rv.push_back( c->constraint );

    return rv;
}


bool DRC_ENGINE::HasRulesForConstraintType( DRC_CONSTRAINT_TYPE_T constraintID )
{
    //drc_dbg(10,"hascorrect id %d size %d\n", ruleID,  m_ruleMap[ruleID]->sortedRules.size( ) );
    return m_constraintMap[constraintID]->size() != 0;
}


bool DRC_ENGINE::QueryWorstConstraint( DRC_CONSTRAINT_TYPE_T aConstraintId,
                                       DRC_CONSTRAINT& aConstraint,
                                       DRC_CONSTRAINT_QUERY_T aQueryType )
{
    if( aQueryType == DRCCQ_LARGEST_MINIMUM )
    {
        int worst = 0;

        for( const DRC_CONSTRAINT& constraint : QueryConstraintsById( aConstraintId ) )
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
