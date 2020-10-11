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
    m_errorLimits.resize( DRCE_LAST + 1 );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
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

    DRC_CONSTRAINT annulusConstraint( DRC_CONSTRAINT_TYPE_ANNULAR_WIDTH );
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

    DRC_CONSTRAINT silkToPadClearanceConstraint( DRC_CONSTRAINT_TYPE_SILK_CLEARANCE );
    silkToPadClearanceConstraint.Value().SetMin( 0 );
    rule->AddConstraint( silkToPadClearanceConstraint );

    DRC_CONSTRAINT diffPairGapConstraint( DRC_CONSTRAINT_TYPE_DIFF_PAIR_GAP );
    diffPairGapConstraint.Value().SetMin( bds.GetDefault()->GetClearance() );
    rule->AddConstraint( diffPairGapConstraint );


    // 2) micro-via specific defaults (new DRC doesn't treat microvias in any special way)

    DRC_RULE* uViaRule = createImplicitRule( _( "board setup micro-via constraints" ));

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

    std::vector<DRC_RULE*> netclassClearanceRules;
    std::vector<DRC_RULE*> netclassItemSpecificRules;

    auto makeNetclassRules =
            [&]( const NETCLASSPTR& nc, bool isDefault )
            {
                // Note: only add constraints for netclass values which are larger than board
                // minimums. (This ensures that the board minimums will still enforce a global
                // lower bound.)

                wxString ncName = nc->GetName();

                DRC_RULE* rule;
                wxString  expr;

                if( nc->GetClearance() > bds.m_MinClearance
                        || nc->GetTrackWidth() > bds.m_TrackMinWidth )
                {
                    rule = new DRC_RULE;
                    rule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    rule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s'",
                                             ncName );
                    rule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassClearanceRules.push_back( rule );

                    if( nc->GetClearance() > bds.m_MinClearance )
                    {
                        DRC_CONSTRAINT ncClearanceConstraint( DRC_CONSTRAINT_TYPE_CLEARANCE );
                        ncClearanceConstraint.Value().SetMin( nc->GetClearance() );
                        rule->AddConstraint( ncClearanceConstraint );
                    }

                    if( nc->GetTrackWidth() > bds.m_TrackMinWidth )
                    {
                        DRC_CONSTRAINT ncWidthConstraint( DRC_CONSTRAINT_TYPE_TRACK_WIDTH );
                        ncWidthConstraint.Value().SetMin( nc->GetTrackWidth() );
                        rule->AddConstraint( ncWidthConstraint );
                    }
                }

                if( nc->GetDiffPairWidth() || nc->GetDiffPairGap() )
                {
                    rule = new DRC_RULE;
                    rule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    rule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.isDiffPair()",
                                             ncName );
                    rule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( rule );

                    if( nc->GetDiffPairWidth() )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_TRACK_WIDTH );
                        constraint.Value().SetMin( nc->GetDiffPairWidth() );
                        rule->AddConstraint( constraint );
                    }

                    if( nc->GetDiffPairGap() )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_DIFF_PAIR_GAP );
                        constraint.Value().SetMin( nc->GetDiffPairGap() );
                        rule->AddConstraint( constraint );
                    }
                }

                if( nc->GetViaDiameter() > bds.m_ViasMinSize
                        || nc->GetViaDrill() > bds.m_MinThroughDrill )
                {
                    rule = new DRC_RULE;
                    rule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    rule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.Via_Type != 'micro_via'",
                                             ncName );
                    rule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( rule );

                    if( nc->GetViaDiameter() > bds.m_ViasMinSize )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
                        constraint.Value().SetMin( nc->GetViaDiameter() );
                        rule->AddConstraint( constraint );
                    }

                    if( nc->GetViaDrill() > bds.m_MinThroughDrill )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_HOLE_SIZE );
                        constraint.Value().SetMin( nc->GetViaDrill() );
                        rule->AddConstraint( constraint );
                    }
                }

                if( nc->GetuViaDiameter() > bds.m_MicroViasMinSize
                        || nc->GetuViaDrill() > bds.m_MicroViasMinDrill )
                {
                    rule = new DRC_RULE;
                    rule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    rule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.Via_Type == 'micro_via'",
                                             ncName );
                    rule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( rule );

                    if( nc->GetuViaDiameter() > bds.m_MicroViasMinSize )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_VIA_DIAMETER );
                        constraint.Value().SetMin( nc->GetuViaDiameter() );
                        rule->AddConstraint( constraint );
                    }

                    if( nc->GetuViaDrill() > bds.m_MicroViasMinDrill )
                    {
                        DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_HOLE_SIZE );
                        constraint.Value().SetMin( nc->GetuViaDrill() );
                        rule->AddConstraint( constraint );
                    }
                }
            };

    m_board->SynchronizeNetsAndNetClasses();
    makeNetclassRules( bds.GetNetClasses().GetDefault(), true );

    for( const std::pair<const wxString, NETCLASSPTR>& netclass : bds.GetNetClasses() )
        makeNetclassRules( netclass.second, false );

    // The netclass clearance rules have to be sorted by min clearance so the right one fires
    // if 'A' and 'B' belong to two different netclasses.
    //
    // The item-specific netclass rules are all unary, so there's no 'A' vs 'B' issue.

    std::sort( netclassClearanceRules.begin(), netclassClearanceRules.end(),
               []( DRC_RULE* lhs, DRC_RULE* rhs )
               {
                   return lhs->m_Constraints[0].m_Value.Min()
                                < rhs->m_Constraints[0].m_Value.Min();
               } );

    for( DRC_RULE* ncRule : netclassClearanceRules )
        addRule( ncRule );

    for( DRC_RULE* ncRule : netclassItemSpecificRules )
        addRule( ncRule );

    ReportAux( wxString::Format( "Building %d implicit netclass rules",
                                 (int) netclassClearanceRules.size() ) );
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
        { DRC_CONSTRAINT_TYPE_UNKNOWN,             "unknown",             nullptr },
        { DRC_CONSTRAINT_TYPE_CLEARANCE,           "clearance",           formatMinMax },
        { DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE,      "hole_clearance",      formatMinMax },
        { DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,      "edge_clearance",      formatMinMax },
        { DRC_CONSTRAINT_TYPE_HOLE_SIZE,           "hole_size",           formatMinMax },
        { DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE, "courtyard_clearance", formatMinMax },
        { DRC_CONSTRAINT_TYPE_SILK_CLEARANCE,      "silk_clearance",      formatMinMax },
        { DRC_CONSTRAINT_TYPE_TRACK_WIDTH,         "track_width",         formatMinMax },
        { DRC_CONSTRAINT_TYPE_ANNULAR_WIDTH,       "annular_width",       formatMinMax },
        { DRC_CONSTRAINT_TYPE_DISALLOW,            "disallow",            nullptr },
        { DRC_CONSTRAINT_TYPE_VIA_DIAMETER,        "via_diameter",        formatMinMax },
        { DRC_CONSTRAINT_TYPE_LENGTH,              "length",              formatMinMax },
        { DRC_CONSTRAINT_TYPE_SKEW,                "skew",                formatMinMax },
        { DRC_CONSTRAINT_TYPE_VIA_COUNT,           "via_count",           formatMinMax }
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


/**
 * @throws PARSE_ERROR
 */
void DRC_ENGINE::loadRules( const wxFileName& aPath )
{
    if( aPath.FileExists() )
    {
        std::vector<DRC_RULE*> rules;

        FILE* fp = wxFopen( aPath.GetFullPath(), wxT( "rt" ) );

        if( fp )
        {
            DRC_RULES_PARSER parser( fp, aPath.GetFullPath() );
            parser.Parse( rules, m_reporter );
        }

        // Copy the rules into the member variable afterwards so that if Parse() throws then
        // the possibly malformed rules won't contaminate the current ruleset.

        for( DRC_RULE* rule : rules )
            m_rules.push_back( rule );
    }
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


/**
 * @throws PARSE_ERROR
 */
void DRC_ENGINE::InitEngine( const wxFileName& aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "Create DRC provider: '%s'", provider->GetName() ) );
        provider->SetDRCEngine( this );
    }

    m_ruleConditions.clear();
    m_rules.clear();

    loadImplicitRules();
    loadRules( aRulePath );

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

    if( m_progressReporter )
    {
        int phases = 0;

        for( DRC_TEST_PROVIDER* provider : m_testProviders )
        {
            if( provider->IsEnabled() )
                phases += provider->GetNumPhases();
        }

        m_progressReporter->AddPhases( phases );
    }

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( m_designSettings->Ignore( ii ) )
            m_errorLimits[ ii ] = 0;
        else
            m_errorLimits[ ii ] = INT_MAX;
    }

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        if( !provider->IsEnabled() )
            continue;

        drc_dbg( 0, "Running test provider: '%s'\n", provider->GetName() );

        ReportAux( wxString::Format( "Run DRC provider: '%s'", provider->GetName() ) );

        if( !provider->Run() )
            break;
    }
}


DRC_CONSTRAINT DRC_ENGINE::EvalRulesForItems( DRC_CONSTRAINT_TYPE_T aConstraintId,
                                              const BOARD_ITEM* a, const BOARD_ITEM* b,
                                              PCB_LAYER_ID aLayer, REPORTER* aReporter )
{
#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }
#define UNITS aReporter ? aReporter->GetUnits() : EDA_UNITS::MILLIMETRES
    /*
     * NOTE: all string manipulation MUST be kept inside the REPORT macro.  It absolutely
     * kills performance when running bulk DRC tests (where aReporter is nullptr).
     */

    const BOARD_CONNECTED_ITEM* connectedA = dynamic_cast<const BOARD_CONNECTED_ITEM*>( a );
    const BOARD_CONNECTED_ITEM* connectedB = dynamic_cast<const BOARD_CONNECTED_ITEM*>( b );
    const DRC_CONSTRAINT*       constraintRef = nullptr;
    bool                        implicit = false;

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
                                      MessageTextFromValue( UNITS, overrideA ) ) )
        }

        if( connectedB && connectedB->GetLocalClearanceOverrides( nullptr ) > 0 )
        {
            overrideB = connectedB->GetLocalClearanceOverrides( &m_msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                      b->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, overrideB ) ) )
        }

        if( overrideA || overrideB )
        {
            DRC_CONSTRAINT constraint( DRC_CONSTRAINT_TYPE_CLEARANCE, m_msg );
            constraint.m_Value.SetMin( std::max( overrideA, overrideB ) );
            return constraint;
        }
    }

    if( m_constraintMap.count( aConstraintId ) )
    {
        std::vector<CONSTRAINT_WITH_CONDITIONS*>* ruleset = m_constraintMap[ aConstraintId ];

        // Last matching rule wins, so process in reverse order
        for( int ii = (int) ruleset->size() - 1; ii >= 0; --ii )
        {
            const CONSTRAINT_WITH_CONDITIONS* rcons = ruleset->at( ii );
            implicit = rcons->parentRule && rcons->parentRule->m_Implicit;

            REPORT( "" )

            if( aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE
                    || aConstraintId == DRC_CONSTRAINT_TYPE_SILK_CLEARANCE
                    || aConstraintId == DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE
                    || aConstraintId == DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE )
            {
                int clearance = rcons->constraint.m_Value.Min();
                REPORT( wxString::Format( _( "Checking %s; clearance: %s." ),
                                          rcons->constraint.GetName(),
                                          MessageTextFromValue( UNITS, clearance ) ) )
            }
            else
            {
                REPORT( wxString::Format( _( "Checking %s." ),
                                          rcons->constraint.GetName() ) )
            }

            if( aLayer != UNDEFINED_LAYER && !rcons->layerTest.test( aLayer ) )
            {
                if( rcons->parentRule )
                {
                    REPORT( wxString::Format( _( "Rule layer \"%s\" not matched." ),
                                              rcons->parentRule->m_LayerSource ) )
                    REPORT( "Rule not applied." )
                }

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
                                     : _( "Rule applied.  (No further rules will be checked.)" ) )

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
    }

    // Unfortunately implicit rules don't work for local clearances (such as zones) because
    // they have to be max'ed with netclass values (which are already implicit rules), and our
    // rule selection paradigm is "winner takes all".
    if( constraintRef && aConstraintId == DRC_CONSTRAINT_TYPE_CLEARANCE && implicit )
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
                                      MessageTextFromValue( UNITS, localA ) ) )

            if( localA > clearance )
                clearance = connectedA->GetLocalClearance( &m_msg );
        }

        if( localB > 0 )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local clearance on %s; clearance: %s." ),
                                      b->GetSelectMenuText( UNITS ),
                                      MessageTextFromValue( UNITS, localB ) ) )

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
    static DRC_CONSTRAINT nullConstraint( DRC_CONSTRAINT_TYPE_NULL );
    nullConstraint.m_DisallowFlags = 0;

    return constraintRef ? *constraintRef : nullConstraint;

#undef REPORT
#undef UNITS
}


bool DRC_ENGINE::IsErrorLimitExceeded( int error_code )
{
    assert( error_code >= 0 && error_code <= DRCE_LAST );
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


bool DRC_ENGINE::ReportProgress( double aProgress )
{
    if( !m_progressReporter )
        return true;

    m_progressReporter->SetCurrentProgress( aProgress );
    return m_progressReporter->KeepRefreshing( false );
}


bool DRC_ENGINE::ReportPhase( const wxString& aMessage )
{
    if( !m_progressReporter )
        return true;

    m_progressReporter->AdvancePhase( aMessage );
    return m_progressReporter->KeepRefreshing( false );
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

    if( m_constraintMap.count( constraintID ) )
    {
    for ( CONSTRAINT_WITH_CONDITIONS* c : *m_constraintMap[constraintID] )
        rv.push_back( c->constraint );
    }

    return rv;
}


bool DRC_ENGINE::HasRulesForConstraintType( DRC_CONSTRAINT_TYPE_T constraintID )
{
    //drc_dbg(10,"hascorrect id %d size %d\n", ruleID,  m_ruleMap[ruleID]->sortedRules.size( ) );
    if( m_constraintMap.count( constraintID ) )
        return m_constraintMap[ constraintID ]->size() > 0;

    return false;
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


// fixme: move two functions below to pcbcommon?
static int matchDpSuffix( const wxString& aNetName, wxString& aComplementNet,
                          wxString& aBaseDpName )
{
    int rv = 0;

    if( aNetName.EndsWith( "+" ) )
    {
        aComplementNet = "-";
        rv = 1;
    }
    else if( aNetName.EndsWith( "P" ) )
    {
        aComplementNet = "N";
        rv = 1;
    }
    else if( aNetName.EndsWith( "-" ) )
    {
        aComplementNet = "+";
        rv = -1;
    }
    else if( aNetName.EndsWith( "N" ) )
    {
        aComplementNet = "P";
        rv = -1;
    }
    // Match P followed by 2 digits
    else if( aNetName.Right( 2 ).IsNumber() && aNetName.Right( 3 ).Left( 1 ) == "P" )
    {
        aComplementNet = "N" + aNetName.Right( 2 );
        rv = 1;
    }
    // Match P followed by 1 digit
    else if( aNetName.Right( 1 ).IsNumber() && aNetName.Right( 2 ).Left( 1 ) == "P" )
    {
        aComplementNet = "N" + aNetName.Right( 1 );
        rv = 1;
    }
    // Match N followed by 2 digits
    else if( aNetName.Right( 2 ).IsNumber() && aNetName.Right( 3 ).Left( 1 ) == "N" )
    {
        aComplementNet = "P" + aNetName.Right( 2 );
        rv = -1;
    }
    // Match N followed by 1 digit
    else if( aNetName.Right( 1 ).IsNumber() && aNetName.Right( 2 ).Left( 1 ) == "N" )
    {
        aComplementNet = "P" + aNetName.Right( 1 );
        rv = -1;
    }
    if( rv != 0 )
    {
        aBaseDpName = aNetName.Left( aNetName.Length() - aComplementNet.Length() );
        aComplementNet = aBaseDpName + aComplementNet;
    }

    return rv;
}


int DRC_ENGINE::IsNetADiffPair( BOARD* aBoard, NETINFO_ITEM* aNet, int& aNetP, int& aNetN )
{
    wxString refName = aNet->GetNetname();
    wxString dummy, coupledNetName;

    if( int polarity = matchDpSuffix( refName, coupledNetName, dummy ) )
    {
        NETINFO_ITEM* net = aBoard->FindNet( coupledNetName );

        if( !net )
            return false;

        if( polarity > 0 )
        {
            aNetP = aNet->GetNet();
            aNetN = net->GetNet();
        }
        else
        {
            aNetP = net->GetNet();
            aNetN = aNet->GetNet();
        }

        return true;
    }

    return false;
}


DRC_TEST_PROVIDER* DRC_ENGINE::GetTestProvider( const wxString& name ) const
{
    for( auto prov : m_testProviders )
        if( name == prov->GetName() )
            return prov;

    return nullptr;
}
