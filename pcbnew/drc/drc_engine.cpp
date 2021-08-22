/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <progress_reporter.h>
#include <string_utils.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_test_provider.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <geometry/shape.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_null.h>
#include <convert_basic_shapes_to_polygon.h>

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
    m_drawingSheet( nullptr ),
    m_schematicNetlist( nullptr ),
    m_rulesValid( false ),
    m_userUnits( EDA_UNITS::MILLIMETRES ),
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
    for( DRC_RULE* rule : m_rules )
        delete rule;

    for( std::pair<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>*> pair : m_constraintMap )
    {
        for( DRC_ENGINE_CONSTRAINT* constraint : *pair.second )
            delete constraint;

        delete pair.second;
    }
}


static bool isKeepoutZone( const BOARD_ITEM* aItem, bool aCheckFlags )
{
    if( !aItem )
        return false;

    if( aItem->Type() != PCB_ZONE_T && aItem->Type() != PCB_FP_ZONE_T )
        return false;

    const ZONE* zone = static_cast<const ZONE*>( aItem );

    if( !zone->GetIsRuleArea() )
        return false;

    if( aCheckFlags )
    {
        if(    !zone->GetDoNotAllowTracks()
            && !zone->GetDoNotAllowVias()
            && !zone->GetDoNotAllowPads()
            && !zone->GetDoNotAllowCopperPour()
            && !zone->GetDoNotAllowFootprints() )
        {
            return false;
        }
    }

    return true;
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

    DRC_RULE* rule = createImplicitRule( _( "board setup constraints" ) );

    DRC_CONSTRAINT clearanceConstraint( CLEARANCE_CONSTRAINT );
    clearanceConstraint.Value().SetMin( bds.m_MinClearance );
    rule->AddConstraint( clearanceConstraint );

    DRC_CONSTRAINT widthConstraint( TRACK_WIDTH_CONSTRAINT );
    widthConstraint.Value().SetMin( bds.m_TrackMinWidth );
    rule->AddConstraint( widthConstraint );

    DRC_CONSTRAINT drillConstraint( HOLE_SIZE_CONSTRAINT );
    drillConstraint.Value().SetMin( bds.m_MinThroughDrill );
    rule->AddConstraint( drillConstraint );

    DRC_CONSTRAINT annulusConstraint( ANNULAR_WIDTH_CONSTRAINT );
    annulusConstraint.Value().SetMin( bds.m_ViasMinAnnularWidth );
    rule->AddConstraint( annulusConstraint );

    DRC_CONSTRAINT diameterConstraint( VIA_DIAMETER_CONSTRAINT );
    diameterConstraint.Value().SetMin( bds.m_ViasMinSize );
    rule->AddConstraint( diameterConstraint );

    DRC_CONSTRAINT holeToHoleConstraint( HOLE_TO_HOLE_CONSTRAINT );
    holeToHoleConstraint.Value().SetMin( bds.m_HoleToHoleMin );
    rule->AddConstraint( holeToHoleConstraint );

    DRC_CONSTRAINT diffPairGapConstraint( DIFF_PAIR_GAP_CONSTRAINT );
    diffPairGapConstraint.Value().SetMin( bds.m_MinClearance );
    rule->AddConstraint( diffPairGapConstraint );

    rule = createImplicitRule( _( "board setup constraints silk" ) );
    rule->m_LayerCondition = LSET( 2, F_SilkS, B_SilkS );
    DRC_CONSTRAINT silkClearanceConstraint( SILK_CLEARANCE_CONSTRAINT );
    silkClearanceConstraint.Value().SetMin( bds.m_SilkClearance );
    rule->AddConstraint( silkClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints hole" ) );
    DRC_CONSTRAINT holeClearanceConstraint( HOLE_CLEARANCE_CONSTRAINT );
    holeClearanceConstraint.Value().SetMin( bds.m_HoleClearance );
    rule->AddConstraint( holeClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints edge" ) );
    DRC_CONSTRAINT edgeClearanceConstraint( EDGE_CLEARANCE_CONSTRAINT );
    edgeClearanceConstraint.Value().SetMin( bds.m_CopperEdgeClearance );
    rule->AddConstraint( edgeClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints courtyard" ) );
    DRC_CONSTRAINT courtyardClearanceConstraint( COURTYARD_CLEARANCE_CONSTRAINT );
    holeToHoleConstraint.Value().SetMin( 0 );
    rule->AddConstraint( courtyardClearanceConstraint );

    // 2) micro-via specific defaults (new DRC doesn't treat microvias in any special way)

    DRC_RULE* uViaRule = createImplicitRule( _( "board setup micro-via constraints" ) );

    uViaRule->m_Condition = new DRC_RULE_CONDITION( "A.Via_Type == 'Micro'" );

    DRC_CONSTRAINT uViaDrillConstraint( HOLE_SIZE_CONSTRAINT );
    uViaDrillConstraint.Value().SetMin( bds.m_MicroViasMinDrill );
    uViaRule->AddConstraint( uViaDrillConstraint );

    DRC_CONSTRAINT uViaDiameterConstraint( VIA_DIAMETER_CONSTRAINT );
    uViaDiameterConstraint.Value().SetMin( bds.m_MicroViasMinSize );
    uViaRule->AddConstraint( uViaDiameterConstraint );

    if( !bds.m_MicroViasAllowed )
    {
        DRC_CONSTRAINT disallowConstraint( DISALLOW_CONSTRAINT );
        disallowConstraint.m_DisallowFlags = DRC_DISALLOW_MICRO_VIAS;
        uViaRule->AddConstraint( disallowConstraint );
    }

    if( !bds.m_BlindBuriedViaAllowed )
    {
        DRC_RULE* bbViaRule = createImplicitRule( _( "board setup constraints" ) );

        bbViaRule->m_Condition = new DRC_RULE_CONDITION( "A.Via_Type == 'Blind/buried'" );

        DRC_CONSTRAINT disallowConstraint( DISALLOW_CONSTRAINT );
        disallowConstraint.m_DisallowFlags = DRC_DISALLOW_BB_VIAS;
        bbViaRule->AddConstraint( disallowConstraint );
    }

    // 3) per-netclass rules

    std::vector<DRC_RULE*> netclassClearanceRules;
    std::vector<DRC_RULE*> netclassItemSpecificRules;

    auto makeNetclassRules =
            [&]( const NETCLASSPTR& nc, bool isDefault )
            {
                wxString ncName = nc->GetName();

                DRC_RULE* netclassRule;
                wxString  expr;

                if( nc->GetClearance() || nc->GetTrackWidth() )
                {
                    netclassRule = new DRC_RULE;
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s'", ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassClearanceRules.push_back( netclassRule );

                    if( nc->GetClearance() )
                    {
                        DRC_CONSTRAINT constraint( CLEARANCE_CONSTRAINT );
                        constraint.Value().SetMin( std::max( bds.m_MinClearance,
                                                             nc->GetClearance() ) );
                        netclassRule->AddConstraint( constraint );
                    }

                    if( nc->GetTrackWidth() )
                    {
                        DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                        constraint.Value().SetMin( bds.m_TrackMinWidth );
                        constraint.Value().SetOpt( nc->GetTrackWidth() );
                        netclassRule->AddConstraint( constraint );
                    }
                }

                if( nc->GetDiffPairWidth() )
                {
                    netclassRule = new DRC_RULE;
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.inDiffPair('*')", ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_TrackMinWidth );
                    constraint.Value().SetOpt( nc->GetDiffPairWidth() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->GetDiffPairGap() )
                {
                    netclassRule = new DRC_RULE;
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s'", ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( DIFF_PAIR_GAP_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_MinClearance );
                    constraint.Value().SetOpt( nc->GetDiffPairGap() );
                    netclassRule->AddConstraint( constraint );

                    // A narrower diffpair gap overrides the netclass min clearance (but is still
                    // trimmed to the board min clearance, which is absolute).
                    if( nc->GetDiffPairGap() < nc->GetClearance() )
                    {
                        netclassRule = new DRC_RULE;
                        netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                                 ncName );
                        netclassRule->m_Implicit = true;

                        expr = wxString::Format( "A.NetClass == '%s' && AB.isCoupledDiffPair()",
                                                 ncName );
                        netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                        netclassItemSpecificRules.push_back( netclassRule );

                        DRC_CONSTRAINT min_clearanceConstraint( CLEARANCE_CONSTRAINT );
                        min_clearanceConstraint.Value().SetMin( std::max( bds.m_MinClearance,
                                                                          nc->GetDiffPairGap() ) );
                        netclassRule->AddConstraint( min_clearanceConstraint );
                    }
                }

                if( nc->GetViaDiameter() || nc->GetViaDrill() )
                {
                    netclassRule = new DRC_RULE;
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.Via_Type != 'Micro'",
                                             ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    if( nc->GetViaDiameter() )
                    {
                        DRC_CONSTRAINT constraint( VIA_DIAMETER_CONSTRAINT );
                        constraint.Value().SetMin( bds.m_ViasMinSize );
                        constraint.Value().SetOpt( nc->GetViaDiameter() );
                        netclassRule->AddConstraint( constraint );
                    }

                    if( nc->GetViaDrill() )
                    {
                        DRC_CONSTRAINT constraint( HOLE_SIZE_CONSTRAINT );
                        constraint.Value().SetMin( bds.m_MinThroughDrill );
                        constraint.Value().SetOpt( nc->GetViaDrill() );
                        netclassRule->AddConstraint( constraint );
                    }
                }

                if( nc->GetuViaDiameter() || nc->GetuViaDrill() )
                {
                    netclassRule = new DRC_RULE;
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( "A.NetClass == '%s' && A.Via_Type == 'Micro'",
                                             ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    if( nc->GetuViaDiameter() )
                    {
                        DRC_CONSTRAINT constraint( VIA_DIAMETER_CONSTRAINT );
                        constraint.Value().SetMin( bds.m_MicroViasMinSize );
                        constraint.Value().SetMin( nc->GetuViaDiameter() );
                        netclassRule->AddConstraint( constraint );
                    }

                    if( nc->GetuViaDrill() )
                    {
                        DRC_CONSTRAINT constraint( HOLE_SIZE_CONSTRAINT );
                        constraint.Value().SetMin( bds.m_MicroViasMinDrill );
                        constraint.Value().SetOpt( nc->GetuViaDrill() );
                        netclassRule->AddConstraint( constraint );
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

    // 3) keepout area rules

    std::vector<ZONE*> keepoutZones;

    for( ZONE* zone : m_board->Zones() )
    {
        if( isKeepoutZone( zone, true ) )
            keepoutZones.push_back( zone );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
        {
            if( isKeepoutZone( zone, true ) )
                keepoutZones.push_back( zone );
        }
    }

    for( ZONE* zone : keepoutZones )
    {
        wxString name = zone->GetZoneName();

        if( name.IsEmpty() )
        {
            rule = createImplicitRule( _( "keepout area" ) );
            name = zone->m_Uuid.AsString();
        }
        else
        {
            rule = createImplicitRule( wxString::Format( _( "keepout area '%s'" ), name ) );
        }

        rule->m_Condition = new DRC_RULE_CONDITION( wxString::Format( "A.insideArea('%s')",
                                                                      name ) );

        rule->m_LayerCondition = zone->GetLayerSet();

        int disallowFlags = 0;

        if( zone->GetDoNotAllowTracks() )
            disallowFlags |= DRC_DISALLOW_TRACKS;

        if( zone->GetDoNotAllowVias() )
            disallowFlags |= DRC_DISALLOW_VIAS;

        if( zone->GetDoNotAllowPads() )
            disallowFlags |= DRC_DISALLOW_PADS;

        if( zone->GetDoNotAllowCopperPour() )
            disallowFlags |= DRC_DISALLOW_ZONES;

        if( zone->GetDoNotAllowFootprints() )
            disallowFlags |= DRC_DISALLOW_FOOTPRINTS;

        DRC_CONSTRAINT disallowConstraint( DISALLOW_CONSTRAINT );
        disallowConstraint.m_DisallowFlags = disallowFlags;
        rule->AddConstraint( disallowConstraint );
    }

    ReportAux( wxString::Format( "Building %d implicit netclass rules",
                                 (int) netclassClearanceRules.size() ) );
}


static wxString formatConstraint( const DRC_CONSTRAINT& constraint )
{
    struct FORMATTER
    {
        DRC_CONSTRAINT_T type;
        wxString         name;
        std::function<wxString(const DRC_CONSTRAINT&)> formatter;
    };

    auto formatMinMax =
            []( const DRC_CONSTRAINT& c ) -> wxString
            {
                wxString str;
                const auto value = c.GetValue();

                if ( value.HasMin() )
                    str += wxString::Format( " min: %d", value.Min() );

                if ( value.HasOpt() )
                    str += wxString::Format( " opt: %d", value.Opt() );

                if ( value.HasMax() )
                    str += wxString::Format( " max: %d", value.Max() );

                return str;
            };

    std::vector<FORMATTER> formats =
    {
        { CLEARANCE_CONSTRAINT,           "clearance",           formatMinMax },
        { HOLE_CLEARANCE_CONSTRAINT,      "hole_clearance",      formatMinMax },
        { HOLE_TO_HOLE_CONSTRAINT,        "hole_to_hole",        formatMinMax },
        { EDGE_CLEARANCE_CONSTRAINT,      "edge_clearance",      formatMinMax },
        { HOLE_SIZE_CONSTRAINT,           "hole_size",           formatMinMax },
        { COURTYARD_CLEARANCE_CONSTRAINT, "courtyard_clearance", formatMinMax },
        { SILK_CLEARANCE_CONSTRAINT,      "silk_clearance",      formatMinMax },
        { TRACK_WIDTH_CONSTRAINT,         "track_width",         formatMinMax },
        { ANNULAR_WIDTH_CONSTRAINT,       "annular_width",       formatMinMax },
        { DISALLOW_CONSTRAINT,            "disallow",            nullptr },
        { VIA_DIAMETER_CONSTRAINT,        "via_diameter",        formatMinMax },
        { LENGTH_CONSTRAINT,              "length",              formatMinMax },
        { SKEW_CONSTRAINT,                "skew",                formatMinMax },
        { VIA_COUNT_CONSTRAINT,           "via_count",           formatMinMax }
    };

    for( FORMATTER& fmt : formats )
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


void DRC_ENGINE::compileRules()
{
    ReportAux( wxString::Format( "Compiling Rules (%d rules): ",
                                 (int) m_rules.size() ) );

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "- Provider: '%s': ", provider->GetName() ) );
        drc_dbg( 7, "do prov %s", provider->GetName() );

        for( DRC_CONSTRAINT_T id : provider->GetConstraintTypes() )
        {
            drc_dbg( 7, "do id %d", id );

            if( m_constraintMap.find( id ) == m_constraintMap.end() )
                m_constraintMap[ id ] = new std::vector<DRC_ENGINE_CONSTRAINT*>();

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

                    DRC_ENGINE_CONSTRAINT* rcons = new DRC_ENGINE_CONSTRAINT;

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
}


void DRC_ENGINE::InitEngine( const wxFileName& aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( "Create DRC provider: '%s'", provider->GetName() ) );
        provider->SetDRCEngine( this );
    }

    for( DRC_RULE* rule : m_rules )
        delete rule;

    m_rules.clear();
    m_rulesValid = false;

    for( std::pair<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>*> pair : m_constraintMap )
    {
        for( DRC_ENGINE_CONSTRAINT* constraint : *pair.second )
            delete constraint;

        delete pair.second;
    }

    m_constraintMap.clear();

    m_board->IncrementTimeStamp();  // Clear board-level caches

    try         // attempt to load full set of rules (implicit + user rules)
    {
        loadImplicitRules();
        loadRules( aRulePath );
        compileRules();
    }
    catch( PARSE_ERROR& original_parse_error )
    {
        try     // try again with just our implicit rules
        {
            loadImplicitRules();
            compileRules();
        }
        catch( PARSE_ERROR& )
        {
            wxFAIL_MSG( "Compiling implicit rules failed." );
        }

        throw original_parse_error;
    }

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = INT_MAX;

    m_rulesValid = true;
}


void DRC_ENGINE::RunTests( EDA_UNITS aUnits, bool aReportAllTrackErrors, bool aTestFootprints )
{
    m_userUnits = aUnits;

    m_reportAllTrackErrors = aReportAllTrackErrors;
    m_testFootprints = aTestFootprints;

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( m_designSettings->Ignore( ii ) )
            m_errorLimits[ ii ] = 0;
        else
            m_errorLimits[ ii ] = INT_MAX;
    }

    m_board->IncrementTimeStamp();      // Invalidate all caches

    if( !ReportPhase( _( "Tessellating copper zones..." ) ) )
        return;

    // Number of zones between progress bar updates
    int                delta = 5;
    std::vector<ZONE*> copperZones;

    for( ZONE* zone : m_board->Zones() )
    {
        zone->CacheBoundingBox();
        zone->CacheTriangulation();

        if( !zone->GetIsRuleArea() )
            copperZones.push_back( zone );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
        {
            zone->CacheBoundingBox();
            zone->CacheTriangulation();

            if( !zone->GetIsRuleArea() )
                copperZones.push_back( zone );
        }

        footprint->BuildPolyCourtyards();
    }

    int zoneCount = copperZones.size();

    for( int ii = 0; ii < zoneCount; ++ii )
    {
        ZONE* zone = copperZones[ ii ];

        if( ( ii % delta ) == 0 || ii == zoneCount -  1 )
        {
            if( !ReportProgress( (double) ii / (double) zoneCount ) )
                return;
        }

        m_board->m_CopperZoneRTrees[ zone ] = std::make_unique<DRC_RTREE>();

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( IsCopperLayer( layer ) )
                m_board->m_CopperZoneRTrees[ zone ]->Insert( zone, layer );
        }
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


DRC_CONSTRAINT DRC_ENGINE::EvalRules( DRC_CONSTRAINT_T aConstraintType, const BOARD_ITEM* a,
                                      const BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                                      REPORTER* aReporter )
{
#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }
#define UNITS aReporter ? aReporter->GetUnits() : EDA_UNITS::MILLIMETRES
#define REPORT_VALUE( v ) MessageTextFromValue( UNITS, v )
    /*
     * NOTE: all string manipulation MUST BE KEPT INSIDE the REPORT macro.  It absolutely
     * kills performance when running bulk DRC tests (where aReporter is nullptr).
     */

    const BOARD_CONNECTED_ITEM* ac = a && a->IsConnected() ?
                                         static_cast<const BOARD_CONNECTED_ITEM*>( a ) : nullptr;
    const BOARD_CONNECTED_ITEM* bc = b && b->IsConnected() ?
                                         static_cast<const BOARD_CONNECTED_ITEM*>( b ) : nullptr;

    bool a_is_non_copper = a && ( !a->IsOnCopperLayer() || isKeepoutZone( a, false ) );
    bool b_is_non_copper = b && ( !b->IsOnCopperLayer() || isKeepoutZone( b, false ) );

    DRC_CONSTRAINT constraint;
    constraint.m_Type = aConstraintType;

    // Local overrides take precedence over everything *except* board min clearance
    if( aConstraintType == CLEARANCE_CONSTRAINT || aConstraintType == HOLE_CLEARANCE_CONSTRAINT )
    {
        int override = 0;

        if( ac && !b_is_non_copper )
        {
            int overrideA = ac->GetLocalClearanceOverrides( nullptr );

            if( overrideA > 0 )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( a->GetSelectMenuText( UNITS ) ),
                                          REPORT_VALUE( overrideA ) ) )

                override = ac->GetLocalClearanceOverrides( &m_msg );
            }
        }

        if( bc && !a_is_non_copper )
        {
            int overrideB = bc->GetLocalClearanceOverrides( nullptr );

            if( overrideB > 0 )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( b->GetSelectMenuText( UNITS ) ),
                                          EscapeHTML( REPORT_VALUE( overrideB ) ) ) )

                if( overrideB > override )
                    override = bc->GetLocalClearanceOverrides( &m_msg );
            }
        }

        if( override )
        {
            if( aConstraintType == CLEARANCE_CONSTRAINT )
            {
                if( override < m_designSettings->m_MinClearance )
                {
                    override = m_designSettings->m_MinClearance;
                    m_msg = _( "board minimum" );

                    REPORT( "" )
                    REPORT( wxString::Format( _( "Board minimum clearance: %s." ),
                                              REPORT_VALUE( override ) ) )
                }
            }
            else
            {
                if( override < m_designSettings->m_HoleClearance )
                {
                    override = m_designSettings->m_HoleClearance;
                    m_msg = _( "board minimum hole" );

                    REPORT( "" )
                    REPORT( wxString::Format( _( "Board minimum hole clearance: %s." ),
                                              REPORT_VALUE( override ) ) )
                }
            }

            constraint.SetName( m_msg );
            constraint.m_Value.SetMin( override );
            return constraint;
        }
    }

    auto processConstraint =
            [&]( const DRC_ENGINE_CONSTRAINT* c ) -> bool
            {
                bool implicit = c->parentRule && c->parentRule->m_Implicit;

                REPORT( "" )

                switch( c->constraint.m_Type )
                {
                case CLEARANCE_CONSTRAINT:
                case COURTYARD_CLEARANCE_CONSTRAINT:
                case SILK_CLEARANCE_CONSTRAINT:
                case HOLE_CLEARANCE_CONSTRAINT:
                case EDGE_CLEARANCE_CONSTRAINT:
                {
                    int val = c->constraint.m_Value.Min();
                    REPORT( wxString::Format( _( "Checking %s clearance: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              REPORT_VALUE( val ) ) )
                    break;
                }

                case TRACK_WIDTH_CONSTRAINT:
                case ANNULAR_WIDTH_CONSTRAINT:
                case VIA_DIAMETER_CONSTRAINT:
                {
                    if( aReporter )
                    {
                        wxString min = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
                        wxString opt = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
                        wxString max = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
                        wxString msg;

                        if( implicit )
                        {
                            opt = StringFromValue( UNITS, c->constraint.m_Value.Opt(), true );

                            switch( c->constraint.m_Type )
                            {
                            case TRACK_WIDTH_CONSTRAINT:   msg = "track width";   break;
                            case ANNULAR_WIDTH_CONSTRAINT: msg = "annular width"; break;
                            case VIA_DIAMETER_CONSTRAINT:  msg = "via diameter";  break;
                            default:                       msg = "constraint";    break;
                            }

                            REPORT( wxString::Format( _( "Checking %s %s: %s." ),
                                                      EscapeHTML( c->constraint.GetName() ),
                                                      EscapeHTML( msg ),
                                                      opt ) )
                        }
                        else
                        {
                            if( c->constraint.m_Value.HasMin() )
                                min = StringFromValue( UNITS, c->constraint.m_Value.Min(), true );

                            if( c->constraint.m_Value.HasOpt() )
                                opt = StringFromValue( UNITS, c->constraint.m_Value.Opt(), true );

                            if( c->constraint.m_Value.HasMax() )
                                max = StringFromValue( UNITS, c->constraint.m_Value.Max(), true );

                            REPORT( wxString::Format( _( "Checking %s: min %s; opt %s; max %s." ),
                                                      EscapeHTML( c->constraint.GetName() ),
                                                      min,
                                                      opt,
                                                      max ) )
                        }
                    }
                    break;
                }

                default:
                    REPORT( wxString::Format( _( "Checking %s." ),
                                              EscapeHTML( c->constraint.GetName() ) ) )
                }

                if( c->constraint.m_Type == CLEARANCE_CONSTRAINT )
                {
                    if( implicit && ( a_is_non_copper || b_is_non_copper ) )
                    {
                        REPORT( _( "Board and netclass clearances apply only between copper "
                                   "items." ) );
                        return true;
                    }
                }
                else if( c->constraint.m_Type == DISALLOW_CONSTRAINT )
                {
                    int mask;

                    if( a->GetFlags() & HOLE_PROXY )
                    {
                        mask = DRC_DISALLOW_HOLES;
                    }
                    else if( a->Type() == PCB_VIA_T )
                    {
                        mask = DRC_DISALLOW_VIAS;

                        switch( static_cast<const PCB_VIA*>( a )->GetViaType() )
                        {
                        case VIATYPE::BLIND_BURIED: mask |= DRC_DISALLOW_BB_VIAS;    break;
                        case VIATYPE::MICROVIA:     mask |= DRC_DISALLOW_MICRO_VIAS; break;
                        default:                                                     break;
                        }
                    }
                    else
                    {
                        switch( a->Type() )
                        {
                        case PCB_TRACE_T:        mask = DRC_DISALLOW_TRACKS;     break;
                        case PCB_ARC_T:          mask = DRC_DISALLOW_TRACKS;     break;
                        case PCB_PAD_T:          mask = DRC_DISALLOW_PADS;       break;
                        case PCB_FOOTPRINT_T:    mask = DRC_DISALLOW_FOOTPRINTS; break;
                        case PCB_SHAPE_T:        mask = DRC_DISALLOW_GRAPHICS;   break;
                        case PCB_FP_SHAPE_T:     mask = DRC_DISALLOW_GRAPHICS;   break;
                        case PCB_TEXT_T:         mask = DRC_DISALLOW_TEXTS;      break;
                        case PCB_FP_TEXT_T:      mask = DRC_DISALLOW_TEXTS;      break;
                        case PCB_ZONE_T:         mask = DRC_DISALLOW_ZONES;      break;
                        case PCB_FP_ZONE_T:      mask = DRC_DISALLOW_ZONES;      break;
                        case PCB_LOCATE_HOLE_T:  mask = DRC_DISALLOW_HOLES;      break;
                        default:                 mask = 0;                       break;
                        }
                    }

                    if( ( c->constraint.m_DisallowFlags & mask ) == 0 )
                    {
                        if( implicit )
                            REPORT( _( "Keepout constraint not met." ) )
                        else
                            REPORT( _( "Disallow constraint not met." ) )

                        return false;
                    }

                    LSET itemLayers = a->GetLayerSet();

                    if( a->Type() == PCB_FOOTPRINT_T )
                    {
                        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( a );

                        if( !footprint->GetPolyCourtyard( F_CrtYd ).IsEmpty() )
                            itemLayers |= LSET::FrontMask();

                        if( !footprint->GetPolyCourtyard( B_CrtYd ).IsEmpty() )
                            itemLayers |= LSET::BackMask();
                    }

                    if( !( c->layerTest & itemLayers ).any() )
                    {
                        if( implicit )
                        {
                            REPORT( _( "Keepout layer(s) not matched." ) )
                        }
                        else if( c->parentRule )
                        {
                            REPORT( wxString::Format( _( "Rule layer '%s' not matched; rule "
                                                         "ignored." ),
                                                      EscapeHTML( c->parentRule->m_LayerSource ) ) )
                        }
                        else
                        {
                            REPORT( _( "Rule layer not matched; rule ignored." ) )
                        }

                        return false;
                    }
                }

                if( aLayer != UNDEFINED_LAYER && !c->layerTest.test( aLayer ) )
                {
                    if( implicit )
                    {
                        REPORT( "Constraint layer not matched." )
                    }
                    else if( c->parentRule )
                    {
                        REPORT( wxString::Format( _( "Rule layer '%s' not matched; rule ignored." ),
                                                  EscapeHTML( c->parentRule->m_LayerSource ) ) )
                    }
                    else
                    {
                        REPORT( _( "Rule layer not matched; rule ignored." ) )
                    }

                    return false;
                }

                if( !c->condition || c->condition->GetExpression().IsEmpty() )
                {
                    REPORT( implicit ? _( "Unconditional constraint applied." )
                                     : _( "Unconditional rule applied." ) );

                    constraint = c->constraint;
                    return true;
                }
                else
                {
                    if( implicit )
                    {
                        // Don't report on implicit rule conditions; they're synthetic.
                    }
                    else
                    {
                        REPORT( wxString::Format( _( "Checking rule condition '%s'." ),
                                                  EscapeHTML( c->condition->GetExpression() ) ) )
                    }

                    if( c->condition->EvaluateFor( a, b, aLayer, aReporter ) )
                    {
                        REPORT( implicit ? _( "Constraint applied." )
                                         : _( "Rule applied; overrides previous constraints." ) )

                        if( c->constraint.m_Value.HasMin() )
                            constraint.m_Value.SetMin( c->constraint.m_Value.Min() );

                        if( c->constraint.m_Value.HasOpt() )
                            constraint.m_Value.SetOpt( c->constraint.m_Value.Opt() );

                        if( c->constraint.m_Value.HasMax() )
                            constraint .m_Value.SetMax( c->constraint.m_Value.Max() );

                        // While the expectation would be to OR the disallow flags, we've already
                        // masked them down to aItem's type -- so we're really only looking for a
                        // boolean here.
                        constraint.m_DisallowFlags = c->constraint.m_DisallowFlags;

                        constraint.SetParentRule( c->constraint.GetParentRule() );

                        return true;
                    }
                    else
                    {
                        REPORT( implicit ? _( "Membership not satisfied; constraint ignored." )
                                         : _( "Condition not satisfied; rule ignored." ) )

                        return false;
                    }
                }
            };

    if( m_constraintMap.count( aConstraintType ) )
    {
        std::vector<DRC_ENGINE_CONSTRAINT*>* ruleset = m_constraintMap[ aConstraintType ];

        for( int ii = 0; ii < (int) ruleset->size(); ++ii )
            processConstraint( ruleset->at( ii ) );
    }

    if( constraint.GetParentRule() && !constraint.GetParentRule()->m_Implicit )
        return constraint;

    // Unfortunately implicit rules don't work for local clearances (such as zones) because
    // they have to be max'ed with netclass values (which are already implicit rules), and our
    // rule selection paradigm is "winner takes all".
    if( aConstraintType == CLEARANCE_CONSTRAINT )
    {
        int global = constraint.m_Value.Min();
        int localA = ac ? ac->GetLocalClearance( nullptr ) : 0;
        int localB = bc ? bc->GetLocalClearance( nullptr ) : 0;
        int clearance = global;

        if( localA > 0 )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local clearance on %s; clearance: %s." ),
                                      EscapeHTML( a->GetSelectMenuText( UNITS ) ),
                                      REPORT_VALUE( localA ) ) )

            if( localA > clearance )
                clearance = ac->GetLocalClearance( &m_msg );
        }

        if( localB > 0 )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local clearance on %s; clearance: %s." ),
                                      EscapeHTML( b->GetSelectMenuText( UNITS ) ),
                                      REPORT_VALUE( localB ) ) )

            if( localB > clearance )
                clearance = bc->GetLocalClearance( &m_msg );
        }

        if( localA > global || localB > global )
        {
            constraint.SetName( m_msg );
            constraint.m_Value.SetMin( clearance );
            return constraint;
        }
    }

    if( !constraint.GetParentRule() )
    {
        constraint.m_Type = NULL_CONSTRAINT;
        constraint.m_DisallowFlags = 0;
    }

    return constraint;

#undef REPORT
#undef UNITS
#undef REPORT_VALUE
}


bool DRC_ENGINE::IsErrorLimitExceeded( int error_code )
{
    assert( error_code >= 0 && error_code <= DRCE_LAST );
    return m_errorLimits[ error_code ] <= 0;
}


void DRC_ENGINE::ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, const wxPoint& aPos )
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


bool DRC_ENGINE::HasRulesForConstraintType( DRC_CONSTRAINT_T constraintID )
{
    //drc_dbg(10,"hascorrect id %d size %d\n", ruleID,  m_ruleMap[ruleID]->sortedRules.size( ) );
    if( m_constraintMap.count( constraintID ) )
        return m_constraintMap[ constraintID ]->size() > 0;

    return false;
}


bool DRC_ENGINE::QueryWorstConstraint( DRC_CONSTRAINT_T aConstraintId, DRC_CONSTRAINT& aConstraint )
{
    int worst = 0;

    if( m_constraintMap.count( aConstraintId ) )
    {
        for( DRC_ENGINE_CONSTRAINT* c : *m_constraintMap[aConstraintId] )
        {
            int current = c->constraint.GetValue().Min();

            if( current > worst )
            {
                worst = current;
                aConstraint = c->constraint;
            }
        }
    }

    return worst > 0;
}


// fixme: move two functions below to pcbcommon?
int DRC_ENGINE::MatchDpSuffix( const wxString& aNetName, wxString& aComplementNet,
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


bool DRC_ENGINE::IsNetADiffPair( BOARD* aBoard, NETINFO_ITEM* aNet, int& aNetP, int& aNetN )
{
    wxString refName = aNet->GetNetname();
    wxString dummy, coupledNetName;

    if( int polarity = MatchDpSuffix( refName, coupledNetName, dummy ) )
    {
        NETINFO_ITEM* net = aBoard->FindNet( coupledNetName );

        if( !net )
            return false;

        if( polarity > 0 )
        {
            aNetP = aNet->GetNetCode();
            aNetN = net->GetNetCode();
        }
        else
        {
            aNetP = net->GetNetCode();
            aNetN = aNet->GetNetCode();
        }

        return true;
    }

    return false;
}


std::shared_ptr<SHAPE> DRC_ENGINE::GetShape( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer )
{
    if( aItem->Type() == PCB_PAD_T && !static_cast<PAD*>( aItem )->FlashLayer( aLayer ) )
    {
        PAD* aPad = static_cast<PAD*>( aItem );

        if( aPad->GetAttribute() == PAD_ATTRIB::PTH )
        {
            BOARD_DESIGN_SETTINGS& bds = aPad->GetBoard()->GetDesignSettings();

            // Note: drill size represents finish size, which means the actual holes size is the
            // plating thickness larger.
            auto hole = static_cast<SHAPE_SEGMENT*>( aPad->GetEffectiveHoleShape()->Clone() );
            hole->SetWidth( hole->GetWidth() + bds.GetHolePlatingThickness() );
            return std::make_shared<SHAPE_SEGMENT>( *hole );
        }

        return std::make_shared<SHAPE_NULL>();
    }

    return aItem->GetEffectiveShape( aLayer );
}


bool DRC_ENGINE::IsNetTie( BOARD_ITEM* aItem )
{
    if( aItem->GetParent() && aItem->GetParent()->Type() == PCB_FOOTPRINT_T )
        return static_cast<FOOTPRINT*>( aItem->GetParent() )->IsNetTie();

    return false;
}


DRC_TEST_PROVIDER* DRC_ENGINE::GetTestProvider( const wxString& name ) const
{
    for( auto prov : m_testProviders )
    {
        if( name == prov->GetName() )
            return prov;
    }

    return nullptr;
}
