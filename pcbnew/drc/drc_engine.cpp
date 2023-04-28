/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <atomic>
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
#include <drc/drc_item.h>
#include <drc/drc_cache_generator.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <thread_pool.h>
#include <zone.h>


// wxListBox's performance degrades horrifically with very large datasets.  It's not clear
// they're useful to the user anyway.
#define ERROR_LIMIT 199
#define EXTENDED_ERROR_LIMIT 499


void drcPrintDebugMessage( int level, const wxString& msg, const char *function, int line )
{
    wxString valueStr;

    if( wxGetEnv( wxT( "DRC_DEBUG" ), &valueStr ) )
    {
        int setLevel = wxAtoi( valueStr );

        if( level <= setLevel )
            printf( "%-30s:%d | %s\n", function, line, (const char *) msg.c_str() );
    }
}


DRC_ENGINE::DRC_ENGINE( BOARD* aBoard, BOARD_DESIGN_SETTINGS *aSettings ) :
    UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MILLIMETRES ),
    m_designSettings ( aSettings ),
    m_board( aBoard ),
    m_drawingSheet( nullptr ),
    m_schematicNetlist( nullptr ),
    m_rulesValid( false ),
    m_reportAllTrackErrors( false ),
    m_testFootprints( false ),
    m_reporter( nullptr ),
    m_progressReporter( nullptr )
{
    m_errorLimits.resize( DRCE_LAST + 1 );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = ERROR_LIMIT;
}


DRC_ENGINE::~DRC_ENGINE()
{
    m_rules.clear();

    for( std::pair<DRC_CONSTRAINT_T, std::vector<DRC_ENGINE_CONSTRAINT*>*> pair : m_constraintMap )
    {
        for( DRC_ENGINE_CONSTRAINT* constraint : *pair.second )
            delete constraint;

        delete pair.second;
    }
}


static bool isKeepoutZone( const BOARD_ITEM* aItem, bool aCheckFlags )
{
    if( !aItem || aItem->Type() != PCB_ZONE_T )
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


std::shared_ptr<DRC_RULE> DRC_ENGINE::createImplicitRule( const wxString& name )
{
    std::shared_ptr<DRC_RULE> rule = std::make_shared<DRC_RULE>();

    rule->m_Name = name;
    rule->m_Implicit = true;

    addRule( rule );

    return rule;
}


void DRC_ENGINE::loadImplicitRules()
{
    ReportAux( wxString::Format( wxT( "Building implicit rules (per-item/class overrides, etc...)" ) ) );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // 1) global defaults

    std::shared_ptr<DRC_RULE> rule = createImplicitRule( _( "board setup constraints" ) );

    DRC_CONSTRAINT widthConstraint( TRACK_WIDTH_CONSTRAINT );
    widthConstraint.Value().SetMin( bds.m_TrackMinWidth );
    rule->AddConstraint( widthConstraint );

    DRC_CONSTRAINT connectionConstraint( CONNECTION_WIDTH_CONSTRAINT );
    connectionConstraint.Value().SetMin( bds.m_MinConn );
    rule->AddConstraint( connectionConstraint );

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

    rule = createImplicitRule( _( "board setup constraints zone fill strategy" ) );
    DRC_CONSTRAINT thermalSpokeCountConstraint( MIN_RESOLVED_SPOKES_CONSTRAINT );
    thermalSpokeCountConstraint.Value().SetMin( bds.m_MinResolvedSpokes );
    rule->AddConstraint( thermalSpokeCountConstraint );

    rule = createImplicitRule( _( "board setup constraints silk" ) );
    rule->m_LayerCondition = LSET( 2, F_SilkS, B_SilkS );
    DRC_CONSTRAINT silkClearanceConstraint( SILK_CLEARANCE_CONSTRAINT );
    silkClearanceConstraint.Value().SetMin( bds.m_SilkClearance );
    rule->AddConstraint( silkClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints silk text height" ) );
    rule->m_LayerCondition = LSET( 2, F_SilkS, B_SilkS );
    DRC_CONSTRAINT silkTextHeightConstraint( TEXT_HEIGHT_CONSTRAINT );
    silkTextHeightConstraint.Value().SetMin( bds.m_MinSilkTextHeight );
    rule->AddConstraint( silkTextHeightConstraint );

    rule = createImplicitRule( _( "board setup constraints silk text thickness" ) );
    rule->m_LayerCondition = LSET( 2, F_SilkS, B_SilkS );
    DRC_CONSTRAINT silkTextThicknessConstraint( TEXT_THICKNESS_CONSTRAINT );
    silkTextThicknessConstraint.Value().SetMin( bds.m_MinSilkTextThickness );
    rule->AddConstraint( silkTextThicknessConstraint );

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

    std::shared_ptr<DRC_RULE> uViaRule = createImplicitRule( _( "board setup micro-via constraints" ) );

    uViaRule->m_Condition = new DRC_RULE_CONDITION( wxT( "A.Via_Type == 'Micro'" ) );

    DRC_CONSTRAINT uViaDrillConstraint( HOLE_SIZE_CONSTRAINT );
    uViaDrillConstraint.Value().SetMin( bds.m_MicroViasMinDrill );
    uViaRule->AddConstraint( uViaDrillConstraint );

    DRC_CONSTRAINT uViaDiameterConstraint( VIA_DIAMETER_CONSTRAINT );
    uViaDiameterConstraint.Value().SetMin( bds.m_MicroViasMinSize );
    uViaRule->AddConstraint( uViaDiameterConstraint );

    // 3) per-netclass rules

    std::vector<std::shared_ptr<DRC_RULE>> netclassClearanceRules;
    std::vector<std::shared_ptr<DRC_RULE>> netclassItemSpecificRules;

    auto makeNetclassRules =
            [&]( const std::shared_ptr<NETCLASS>& nc, bool isDefault )
            {
                wxString ncName = nc->GetName();
                wxString expr;

                if( nc->GetClearance() || nc->GetTrackWidth() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( wxT( "A.NetClass == '%s'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassClearanceRules.push_back( netclassRule );

                    if( nc->GetClearance() )
                    {
                        DRC_CONSTRAINT constraint( CLEARANCE_CONSTRAINT );
                        constraint.Value().SetMin( nc->GetClearance() );
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
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( wxT( "A.NetClass == '%s' && A.inDiffPair('*')" ),
                                             ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_TrackMinWidth );
                    constraint.Value().SetOpt( nc->GetDiffPairWidth() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->GetDiffPairGap() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( wxT( "A.NetClass == '%s'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( DIFF_PAIR_GAP_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_MinClearance );
                    constraint.Value().SetOpt( nc->GetDiffPairGap() );
                    netclassRule->AddConstraint( constraint );

                    // A narrower diffpair gap overrides the netclass min clearance
                    if( nc->GetDiffPairGap() < nc->GetClearance() )
                    {
                        netclassRule = std::make_shared<DRC_RULE>();
                        netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                                 ncName );
                        netclassRule->m_Implicit = true;

                        expr = wxString::Format( wxT( "A.NetClass == '%s' && AB.isCoupledDiffPair()" ),
                                                 ncName );
                        netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                        netclassItemSpecificRules.push_back( netclassRule );

                        DRC_CONSTRAINT min_clearanceConstraint( CLEARANCE_CONSTRAINT );
                        min_clearanceConstraint.Value().SetMin( nc->GetDiffPairGap() );
                        netclassRule->AddConstraint( min_clearanceConstraint );
                    }
                }

                if( nc->GetViaDiameter() || nc->GetViaDrill() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( wxT( "A.NetClass == '%s' && A.Via_Type != 'Micro'" ),
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
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (uvia)" ), ncName );
                    netclassRule->m_Implicit = true;

                    expr = wxString::Format( wxT( "A.NetClass == '%s' && A.Via_Type == 'Micro'" ),
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

    m_board->SynchronizeNetsAndNetClasses( false );
    makeNetclassRules( bds.m_NetSettings->m_DefaultNetClass, true );

    for( const auto& [ name, netclass ] : bds.m_NetSettings->m_NetClasses )
        makeNetclassRules( netclass, false );

    // The netclass clearance rules have to be sorted by min clearance so the right one fires
    // if 'A' and 'B' belong to two different netclasses.
    //
    // The item-specific netclass rules are all unary, so there's no 'A' vs 'B' issue.

    std::sort( netclassClearanceRules.begin(), netclassClearanceRules.end(),
               []( const std::shared_ptr<DRC_RULE>& lhs, const std::shared_ptr<DRC_RULE>& rhs )
               {
                   return lhs->m_Constraints[0].m_Value.Min()
                                < rhs->m_Constraints[0].m_Value.Min();
               } );

    for( std::shared_ptr<DRC_RULE>& ncRule : netclassClearanceRules )
        addRule( ncRule );

    for( std::shared_ptr<DRC_RULE>& ncRule : netclassItemSpecificRules )
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
            rule = createImplicitRule( _( "keepout area" ) );
        else
            rule = createImplicitRule( wxString::Format( _( "keepout area '%s'" ), name ) );

        rule->m_ImplicitItemId = zone->m_Uuid;

        rule->m_Condition = new DRC_RULE_CONDITION( wxString::Format( wxT( "A.intersectsArea('%s')" ),
                                                                      zone->m_Uuid.AsString() ) );

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

    ReportAux( wxString::Format( wxT( "Building %d implicit netclass rules" ),
                                 (int) netclassClearanceRules.size() ) );
}


void DRC_ENGINE::loadRules( const wxFileName& aPath )
{
    if( aPath.FileExists() )
    {
        std::vector<std::shared_ptr<DRC_RULE>> rules;

        FILE* fp = wxFopen( aPath.GetFullPath(), wxT( "rt" ) );

        if( fp )
        {
            DRC_RULES_PARSER parser( fp, aPath.GetFullPath() );
            parser.Parse( rules, m_reporter );
        }

        // Copy the rules into the member variable afterwards so that if Parse() throws then
        // the possibly malformed rules won't contaminate the current ruleset.

        for( std::shared_ptr<DRC_RULE>& rule : rules )
            m_rules.push_back( rule );
    }
}


void DRC_ENGINE::compileRules()
{
    ReportAux( wxString::Format( wxT( "Compiling Rules (%d rules): " ), (int) m_rules.size() ) );

    for( std::shared_ptr<DRC_RULE>& rule : m_rules )
    {
        DRC_RULE_CONDITION* condition = nullptr;

        if( rule->m_Condition && !rule->m_Condition->GetExpression().IsEmpty() )
        {
            condition = rule->m_Condition;
            condition->Compile( nullptr );
        }

        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
        {
            if( !m_constraintMap.count( constraint.m_Type ) )
                m_constraintMap[ constraint.m_Type ] = new std::vector<DRC_ENGINE_CONSTRAINT*>();

            DRC_ENGINE_CONSTRAINT* engineConstraint = new DRC_ENGINE_CONSTRAINT;

            engineConstraint->layerTest = rule->m_LayerCondition;
            engineConstraint->condition = condition;
            engineConstraint->constraint = constraint;
            engineConstraint->parentRule = rule;
            m_constraintMap[ constraint.m_Type ]->push_back( engineConstraint );
        }
    }
}


void DRC_ENGINE::InitEngine( const wxFileName& aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( wxT( "Create DRC provider: '%s'" ), provider->GetName() ) );
        provider->SetDRCEngine( this );
    }

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
            wxFAIL_MSG( wxT( "Compiling implicit rules failed." ) );
        }

        throw original_parse_error;
    }

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = ERROR_LIMIT;

    m_rulesValid = true;
}


void DRC_ENGINE::RunTests( EDA_UNITS aUnits, bool aReportAllTrackErrors, bool aTestFootprints )
{
    SetUserUnits( aUnits );

    m_reportAllTrackErrors = aReportAllTrackErrors;
    m_testFootprints = aTestFootprints;

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( m_designSettings->Ignore( ii ) )
            m_errorLimits[ ii ] = 0;
        else if( ii == DRCE_CLEARANCE || ii == DRCE_UNCONNECTED_ITEMS )
            m_errorLimits[ ii ] = EXTENDED_ERROR_LIMIT;
        else
            m_errorLimits[ ii ] = ERROR_LIMIT;
    }

    DRC_TEST_PROVIDER::Init();

    m_board->IncrementTimeStamp();      // Invalidate all caches...

    DRC_CACHE_GENERATOR cacheGenerator;
    cacheGenerator.SetDRCEngine( this );

    if( !cacheGenerator.Run() )         // ... and regenerate them.
        return;

    int timestamp = m_board->GetTimeStamp();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        ReportAux( wxString::Format( wxT( "Run DRC provider: '%s'" ), provider->GetName() ) );

        if( !provider->RunTests( aUnits ) )
            break;
    }

    // DRC tests are multi-threaded; anything that causes us to attempt to re-generate the
    // caches while DRC is running is problematic.
    wxASSERT( timestamp == m_board->GetTimeStamp() );
}


#define REPORT( s ) { if( aReporter ) { aReporter->Report( s ); } }

DRC_CONSTRAINT DRC_ENGINE::EvalZoneConnection( const BOARD_ITEM* a, const BOARD_ITEM* b,
                                               PCB_LAYER_ID aLayer, REPORTER* aReporter )
{
    DRC_CONSTRAINT constraint = EvalRules( ZONE_CONNECTION_CONSTRAINT, a, b, aLayer, aReporter );

    REPORT( "" )
    REPORT( wxString::Format( _( "Resolved zone connection type: %s." ),
                              EscapeHTML( PrintZoneConnection( constraint.m_ZoneConnection ) ) ) )

    if( constraint.m_ZoneConnection == ZONE_CONNECTION::THT_THERMAL )
    {
        const PAD* pad = nullptr;

        if( a->Type() == PCB_PAD_T )
            pad = static_cast<const PAD*>( a );
        else if( b->Type() == PCB_PAD_T )
            pad = static_cast<const PAD*>( b );

        if( pad && pad->GetAttribute() == PAD_ATTRIB::PTH )
        {
            constraint.m_ZoneConnection = ZONE_CONNECTION::THERMAL;
        }
        else
        {
            REPORT( wxString::Format( _( "Pad is not a through hole pad; connection will be: %s." ),
                                      EscapeHTML( PrintZoneConnection( ZONE_CONNECTION::FULL ) ) ) )
            constraint.m_ZoneConnection = ZONE_CONNECTION::FULL;
        }
    }

    return constraint;
}


bool hasDrilledHole( const BOARD_ITEM* aItem )
{
    if( !aItem->HasHole() )
        return false;

    switch( aItem->Type() )
    {
    case PCB_VIA_T:
        return true;

    case PCB_PAD_T:
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        return pad->GetDrillSizeX() == pad->GetDrillSizeY();
    }

    default:
        return false;
    }
}


DRC_CONSTRAINT DRC_ENGINE::EvalRules( DRC_CONSTRAINT_T aConstraintType, const BOARD_ITEM* a,
                                      const BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                                      REPORTER* aReporter )
{
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

    const PAD*       pad  = nullptr;
    const ZONE*      zone = nullptr;
    const FOOTPRINT* parentFootprint = nullptr;

    if( aConstraintType == ZONE_CONNECTION_CONSTRAINT
     || aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT
     || aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT )
    {
        if( a && a->Type() == PCB_PAD_T )
            pad = static_cast<const PAD*>( a );
        else if( a && a->Type() == PCB_ZONE_T )
            zone = static_cast<const ZONE*>( a );

        if( b && b->Type() == PCB_PAD_T )
            pad = static_cast<const PAD*>( b );
        else if( b && b->Type() == PCB_ZONE_T )
            zone = static_cast<const ZONE*>( b );

        if( pad )
            parentFootprint = pad->GetParentFootprint();
    }

    DRC_CONSTRAINT constraint;
    constraint.m_Type = aConstraintType;

    auto applyConstraint =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
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

                constraint.m_ZoneConnection = c->constraint.m_ZoneConnection;

                constraint.SetParentRule( c->constraint.GetParentRule() );
            };

    // Local overrides take precedence over everything *except* board min clearance
    if( aConstraintType == CLEARANCE_CONSTRAINT || aConstraintType == HOLE_CLEARANCE_CONSTRAINT )
    {
        int override_val = 0;
        int overrideA = 0;
        int overrideB = 0;

        if( ac && !b_is_non_copper )
            overrideA = ac->GetLocalClearanceOverrides( nullptr );

        if( bc && !a_is_non_copper )
            overrideB = bc->GetLocalClearanceOverrides( nullptr );

        if( overrideA > 0 || overrideB > 0 )
        {
            wxString msg;

            if( overrideA > 0 )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( a->GetItemDescription( this ) ),
                                          MessageTextFromValue( overrideA ) ) )

                override_val = ac->GetLocalClearanceOverrides( &msg );
            }

            if( overrideB > 0 )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( b->GetItemDescription( this ) ),
                                          EscapeHTML( MessageTextFromValue( overrideB ) ) ) )

                if( overrideB > override_val )
                    override_val = bc->GetLocalClearanceOverrides( &msg );
            }

            if( override_val )
            {
                if( aConstraintType == CLEARANCE_CONSTRAINT )
                {
                    if( override_val < m_designSettings->m_MinClearance )
                    {
                        override_val = m_designSettings->m_MinClearance;
                        msg = _( "board minimum" );

                        REPORT( "" )
                        REPORT( wxString::Format( _( "Board minimum clearance: %s." ),
                                                  MessageTextFromValue( override_val ) ) )
                    }
                }
                else
                {
                    if( override_val < m_designSettings->m_HoleClearance )
                    {
                        override_val = m_designSettings->m_HoleClearance;
                        msg = _( "board minimum hole" );

                        REPORT( "" )
                        REPORT( wxString::Format( _( "Board minimum hole clearance: %s." ),
                                                  MessageTextFromValue( override_val ) ) )
                    }
                }

                constraint.SetName( msg );
                constraint.m_Value.SetMin( override_val );
                return constraint;
            }
        }
    }
    else if( aConstraintType == ZONE_CONNECTION_CONSTRAINT )
    {
        if( pad && pad->GetLocalZoneConnectionOverride( nullptr ) != ZONE_CONNECTION::INHERITED )
        {
            wxString msg;
            ZONE_CONNECTION override = pad->GetLocalZoneConnectionOverride( &msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; zone connection: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this ) ),
                                      EscapeHTML( PrintZoneConnection( override ) ) ) )

            constraint.SetName( msg );
            constraint.m_ZoneConnection = override;
            return constraint;
        }
    }
    else if( aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT )
    {
        if( pad && pad->GetLocalThermalGapOverride( nullptr ) > 0 )
        {
            wxString msg;
            int gap_override = pad->GetLocalThermalGapOverride( &msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; thermal relief gap: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this ) ),
                                      EscapeHTML( MessageTextFromValue( gap_override ) ) ) )

            constraint.SetName( msg );
            constraint.m_Value.SetMin( gap_override );
            return constraint;
        }
    }
    else if( aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT )
    {
        if( pad && pad->GetLocalSpokeWidthOverride( nullptr ) > 0 )
        {
            wxString msg;
            int spoke_override = pad->GetLocalSpokeWidthOverride( &msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; thermal spoke width: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this ) ),
                                      EscapeHTML( MessageTextFromValue( spoke_override ) ) ) )

            if( zone && zone->GetMinThickness() > spoke_override )
            {
                spoke_override = zone->GetMinThickness();

                REPORT( "" )
                REPORT( wxString::Format( _( "%s min thickness: %s." ),
                                          EscapeHTML( zone->GetItemDescription( this ) ),
                                          EscapeHTML( MessageTextFromValue( spoke_override ) ) ) )
            }

            constraint.SetName( msg );
            constraint.m_Value.SetMin( spoke_override );
            return constraint;
        }
    }

    auto testAssertion =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
                REPORT( wxString::Format( _( "Checking assertion \"%s\"." ),
                                          EscapeHTML( c->constraint.m_Test->GetExpression() ) ) )

                if( c->constraint.m_Test->EvaluateFor( a, b, c->constraint.m_Type, aLayer,
                                                       aReporter ) )
                {
                    REPORT( _( "Assertion passed." ) )
                }
                else
                {
                    REPORT( EscapeHTML( _( "--> Assertion failed. <--" ) ) )
                }
            };

    auto processConstraint =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
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
                case PHYSICAL_CLEARANCE_CONSTRAINT:
                case PHYSICAL_HOLE_CLEARANCE_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s clearance: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                    break;

                case DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s max uncoupled length: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Max() ) ) )
                    break;

                case SKEW_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s max skew: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Max() ) ) )
                    break;

                case THERMAL_RELIEF_GAP_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s gap: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                    break;

                case THERMAL_SPOKE_WIDTH_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s thermal spoke width: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                    break;

                case MIN_RESOLVED_SPOKES_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s min spoke count: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              EDA_UNIT_UTILS::UI::MessageTextFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                                                        c->constraint.m_Value.Min() ) ) )
                    break;

                case ZONE_CONNECTION_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s zone connection: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              EscapeHTML( PrintZoneConnection( c->constraint.m_ZoneConnection ) ) ) )
                    break;

                case TRACK_WIDTH_CONSTRAINT:
                case ANNULAR_WIDTH_CONSTRAINT:
                case VIA_DIAMETER_CONSTRAINT:
                case HOLE_SIZE_CONSTRAINT:
                case TEXT_HEIGHT_CONSTRAINT:
                case TEXT_THICKNESS_CONSTRAINT:
                case DIFF_PAIR_GAP_CONSTRAINT:
                case LENGTH_CONSTRAINT:
                case CONNECTION_WIDTH_CONSTRAINT:
                case HOLE_TO_HOLE_CONSTRAINT:
                {
                    if( aReporter )
                    {
                        wxString min = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
                        wxString opt = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );
                        wxString max = wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" );

                        if( implicit )
                        {
                            min = MessageTextFromValue( c->constraint.m_Value.Min() );
                            opt = MessageTextFromValue( c->constraint.m_Value.Opt() );

                            switch( c->constraint.m_Type )
                            {
                            case TRACK_WIDTH_CONSTRAINT:
                                if( c->constraint.m_Value.HasOpt() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s track width: opt %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              opt ) )
                                }
                                else if( c->constraint.m_Value.HasMin() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s track width: min %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              min ) )
                                }

                                break;

                            case ANNULAR_WIDTH_CONSTRAINT:
                                REPORT( wxString::Format( _( "Checking %s annular width: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          opt ) )
                                break;

                            case VIA_DIAMETER_CONSTRAINT:
                                if( c->constraint.m_Value.HasOpt() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s via diameter: opt %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              opt ) )
                                }
                                else if( c->constraint.m_Value.HasMin() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s via diameter: min %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              min ) )
                                }
                                break;

                            case HOLE_SIZE_CONSTRAINT:
                                if( c->constraint.m_Value.HasOpt() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s hole size: opt %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              opt ) )
                                }
                                else if( c->constraint.m_Value.HasMin() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s hole size: min %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              min ) )
                                }

                                break;

                            case TEXT_HEIGHT_CONSTRAINT:
                            case TEXT_THICKNESS_CONSTRAINT:
                            case CONNECTION_WIDTH_CONSTRAINT:
                                REPORT( wxString::Format( _( "Checking %s: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          min ) )
                                break;

                            case DIFF_PAIR_GAP_CONSTRAINT:
                                if( c->constraint.m_Value.HasOpt() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s diff pair gap: opt %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              opt ) )
                                }
                                else if( c->constraint.m_Value.HasMin() )
                                {
                                    REPORT( wxString::Format( _( "Checking %s clearance: min %s." ),
                                                              EscapeHTML( c->constraint.GetName() ),
                                                              min ) )
                                }

                                break;

                            case HOLE_TO_HOLE_CONSTRAINT:
                                REPORT( wxString::Format( _( "Checking %s hole to hole: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          min ) )
                                break;

                            default:
                                REPORT( wxString::Format( _( "Checking %s." ),
                                                          EscapeHTML( c->constraint.GetName() ) ) )
                            }
                        }
                        else
                        {
                            if( c->constraint.m_Value.HasMin() )
                                min = MessageTextFromValue( c->constraint.m_Value.Min() );

                            if( c->constraint.m_Value.HasOpt() )
                                opt = MessageTextFromValue( c->constraint.m_Value.Opt() );

                            if( c->constraint.m_Value.HasMax() )
                                max = MessageTextFromValue( c->constraint.m_Value.Max() );

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
                    if( a_is_non_copper || b_is_non_copper )
                    {
                        if( implicit )
                        {
                            REPORT( _( "Netclass clearances apply only between copper items." ) )
                        }
                        else if( a_is_non_copper )
                        {
                            REPORT( wxString::Format( _( "%s contains no copper.  Rule ignored." ),
                                                      EscapeHTML( a->GetItemDescription( this ) ) ) )
                        }
                        else if( b_is_non_copper )
                        {
                            REPORT( wxString::Format( _( "%s contains no copper.  Rule ignored." ),
                                                      EscapeHTML( b->GetItemDescription( this ) ) ) )
                        }

                        return;
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
                        case PCB_TEXT_T:         mask = DRC_DISALLOW_TEXTS;      break;
                        case PCB_TEXTBOX_T:      mask = DRC_DISALLOW_TEXTS;      break;

                        case PCB_ZONE_T:
                            // Treat teardrop areas as tracks for DRC purposes
                            if( static_cast<const ZONE*>( a )->IsTeardropArea() )
                                mask = DRC_DISALLOW_TRACKS;
                            else
                                mask = DRC_DISALLOW_ZONES;

                            break;

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

                        return;
                    }

                    LSET itemLayers = a->GetLayerSet();

                    if( a->Type() == PCB_FOOTPRINT_T )
                    {
                        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( a );

                        if( !footprint->GetCourtyard( F_CrtYd ).IsEmpty() )
                            itemLayers |= LSET::FrontMask();

                        if( !footprint->GetCourtyard( B_CrtYd ).IsEmpty() )
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
                            REPORT( wxString::Format( _( "Rule layer '%s' not matched; rule ignored." ),
                                                      EscapeHTML( c->parentRule->m_LayerSource ) ) )
                        }
                        else
                        {
                            REPORT( _( "Rule layer not matched; rule ignored." ) )
                        }

                        return;
                    }
                }

                if( ( aLayer != UNDEFINED_LAYER && !c->layerTest.test( aLayer ) )
                        || ( m_board->GetEnabledLayers() & c->layerTest ).count() == 0 )
                {
                    if( implicit )
                    {
                        REPORT( _( "Constraint layer not matched." ) )
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
                }
                else if( c->constraint.m_Type == HOLE_TO_HOLE_CONSTRAINT
                        && ( !hasDrilledHole( a ) || !hasDrilledHole( b ) ) )
                {
                    // Report non-drilled-holes as an implicit condition
                    if( aReporter )
                    {
                        const BOARD_ITEM* x = !hasDrilledHole( a ) ? a : b;

                        REPORT( wxString::Format( _( "%s is not a drilled hole; rule ignored." ),
                                                  x->GetItemDescription( this ) ) )
                    }
                }
                else if( !c->condition || c->condition->GetExpression().IsEmpty() )
                {
                    if( aReporter )
                    {
                        if( implicit )
                        {
                            REPORT( _( "Unconditional constraint applied." ) )
                        }
                        else if( constraint.m_Type == ASSERTION_CONSTRAINT )
                        {
                            REPORT( _( "Unconditional rule applied." ) )
                            testAssertion( c );
                        }
                        else
                        {
                            REPORT( _( "Unconditional rule applied; overrides previous constraints." ) )
                        }
                    }

                    applyConstraint( c );
                }
                else
                {
                    if( implicit )
                    {
                        // Don't report on implicit rule conditions; they're synthetic.
                    }
                    else
                    {
                        REPORT( wxString::Format( _( "Checking rule condition \"%s\"." ),
                                                  EscapeHTML( c->condition->GetExpression() ) ) )
                    }

                    if( c->condition->EvaluateFor( a, b, c->constraint.m_Type, aLayer, aReporter ) )
                    {
                        if( aReporter )
                        {
                            if( implicit )
                            {
                                REPORT( _( "Constraint applied." ) )
                            }
                            else if( constraint.m_Type == ASSERTION_CONSTRAINT )
                            {
                                REPORT( _( "Rule applied." ) )
                                testAssertion( c );
                            }
                            else
                            {
                                REPORT( _( "Rule applied; overrides previous constraints." ) )
                            }
                        }

                        applyConstraint( c );
                    }
                    else
                    {
                        REPORT( implicit ? _( "Membership not satisfied; constraint ignored." )
                                         : _( "Condition not satisfied; rule ignored." ) )
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

    // Special case for pad zone connections which can iherit from their parent footprints.
    // We've already checked for local overrides, and there were no rules targetting the pad
    // itself, so we know we're inheriting and need to see if there are any rules targetting
    // the parent footprint.
    if( pad && parentFootprint && (   aConstraintType == ZONE_CONNECTION_CONSTRAINT
                                   || aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT
                                   || aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT ) )
    {
        if( a == pad )
            a = parentFootprint;
        else
            b = parentFootprint;

        if( m_constraintMap.count( aConstraintType ) )
        {
            std::vector<DRC_ENGINE_CONSTRAINT*>* ruleset = m_constraintMap[ aConstraintType ];

            for( int ii = 0; ii < (int) ruleset->size(); ++ii )
                processConstraint( ruleset->at( ii ) );

            if( constraint.GetParentRule() && !constraint.GetParentRule()->m_Implicit )
                return constraint;
        }
    }

    // Unfortunately implicit rules don't work for local clearances (such as zones) because
    // they have to be max'ed with netclass values (which are already implicit rules), and our
    // rule selection paradigm is "winner takes all".
    if( aConstraintType == CLEARANCE_CONSTRAINT )
    {
        int  global = constraint.m_Value.Min();
        int  localA = ac ? ac->GetLocalClearance( nullptr ) : 0;
        int  localB = bc ? bc->GetLocalClearance( nullptr ) : 0;
        int  clearance = global;
        bool needBlankLine = true;

        if( localA > 0 )
        {
            if( needBlankLine )
            {
                REPORT( "" )
                needBlankLine = false;
            }

            REPORT( wxString::Format( _( "Local clearance on %s: %s." ),
                                      EscapeHTML( a->GetItemDescription( this ) ),
                                      MessageTextFromValue( localA ) ) )

            if( localA > clearance )
            {
                wxString msg;
                clearance = ac->GetLocalClearance( &msg );
                constraint.SetParentRule( nullptr );
                constraint.SetName( msg );
                constraint.m_Value.SetMin( clearance );
            }
        }

        if( localB > 0 )
        {
            if( needBlankLine )
            {
                REPORT( "" )
                needBlankLine = false;
            }

            REPORT( wxString::Format( _( "Local clearance on %s: %s." ),
                                      EscapeHTML( b->GetItemDescription( this ) ),
                                      MessageTextFromValue( localB ) ) )

            if( localB > clearance )
            {
                wxString msg;
                clearance = bc->GetLocalClearance( &msg );
                constraint.SetParentRule( nullptr );
                constraint.SetName( msg );
                constraint.m_Value.SetMin( clearance );
            }
        }

        if( !a_is_non_copper && !b_is_non_copper )
        {
            if( needBlankLine )
            {
                REPORT( "" )
                needBlankLine = false;
            }

            REPORT( wxString::Format( _( "Board minimum clearance: %s." ),
                                      MessageTextFromValue( m_designSettings->m_MinClearance ) ) )

            if( clearance < m_designSettings->m_MinClearance )
            {
                constraint.SetParentRule( nullptr );
                constraint.SetName( _( "board minimum" ) );
                constraint.m_Value.SetMin( m_designSettings->m_MinClearance );
            }
        }

        return constraint;
    }
    else if( aConstraintType == DIFF_PAIR_GAP_CONSTRAINT )
    {
        REPORT( "" )
        REPORT( wxString::Format( _( "Board minimum clearance: %s." ),
                                  MessageTextFromValue( m_designSettings->m_MinClearance ) ) )

        if( constraint.m_Value.Min() < m_designSettings->m_MinClearance )
        {
            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "board minimum" ) );
            constraint.m_Value.SetMin( m_designSettings->m_MinClearance );
        }

        return constraint;
    }
    else if( aConstraintType == ZONE_CONNECTION_CONSTRAINT )
    {
        if( pad && parentFootprint )
        {
            ZONE_CONNECTION local = parentFootprint->GetZoneConnection();

            if( local != ZONE_CONNECTION::INHERITED )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "%s zone connection: %s." ),
                                          EscapeHTML( parentFootprint->GetItemDescription( this ) ),
                                          EscapeHTML( PrintZoneConnection( local ) ) ) )

                constraint.SetParentRule( nullptr );
                constraint.SetName( _( "footprint" ) );
                constraint.m_ZoneConnection = local;
                return constraint;
            }
        }

        if( zone )
        {
            ZONE_CONNECTION local = zone->GetPadConnection();

            REPORT( "" )
            REPORT( wxString::Format( _( "%s pad connection: %s." ),
                                      EscapeHTML( zone->GetItemDescription( this ) ),
                                      EscapeHTML( PrintZoneConnection( local ) ) ) )

            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "zone" ) );
            constraint.m_ZoneConnection = local;
            return constraint;
        }
    }
    else if( aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT )
    {
        if( zone )
        {
            int local = zone->GetThermalReliefGap();

            REPORT( "" )
            REPORT( wxString::Format( _( "%s thermal relief gap: %s." ),
                                      EscapeHTML( zone->GetItemDescription( this ) ),
                                      EscapeHTML( MessageTextFromValue( local ) ) ) )

            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "zone" ) );
            constraint.m_Value.SetMin( local );
            return constraint;
        }
    }
    else if( aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT )
    {
        if( zone )
        {
            int local = zone->GetThermalReliefSpokeWidth();

            REPORT( "" )
            REPORT( wxString::Format( _( "%s thermal spoke width: %s." ),
                                      EscapeHTML( zone->GetItemDescription( this ) ),
                                      EscapeHTML( MessageTextFromValue( local ) ) ) )

            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "zone" ) );
            constraint.m_Value.SetMin( local );
            return constraint;
        }
    }

    if( !constraint.GetParentRule() )
    {
        constraint.m_Type = NULL_CONSTRAINT;
        constraint.m_DisallowFlags = 0;
    }

    return constraint;
}


void DRC_ENGINE::ProcessAssertions( const BOARD_ITEM* a,
                                    std::function<void( const DRC_CONSTRAINT* )> aFailureHandler,
                                    REPORTER* aReporter )
{
    /*
     * NOTE: all string manipulation MUST BE KEPT INSIDE the REPORT macro.  It absolutely
     * kills performance when running bulk DRC tests (where aReporter is nullptr).
     */

    auto testAssertion =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
                REPORT( wxString::Format( _( "Checking rule assertion \"%s\"." ),
                                          EscapeHTML( c->constraint.m_Test->GetExpression() ) ) )

                if( c->constraint.m_Test->EvaluateFor( a, nullptr, c->constraint.m_Type,
                                                       a->GetLayer(), aReporter ) )
                {
                    REPORT( _( "Assertion passed." ) )
                }
                else
                {
                    REPORT( EscapeHTML( _( "--> Assertion failed. <--" ) ) )
                    aFailureHandler( &c->constraint );
                }
            };

    auto processConstraint =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Checking %s." ), c->constraint.GetName() ) )

                if( !( a->GetLayerSet() & c->layerTest ).any() )
                {
                    REPORT( wxString::Format( _( "Rule layer '%s' not matched; rule ignored." ),
                                              EscapeHTML( c->parentRule->m_LayerSource ) ) )
                }

                if( !c->condition || c->condition->GetExpression().IsEmpty() )
                {
                    REPORT( _( "Unconditional rule applied." ) )
                    testAssertion( c );
                }
                else
                {
                    REPORT( wxString::Format( _( "Checking rule condition \"%s\"." ),
                                              EscapeHTML( c->condition->GetExpression() ) ) )

                    if( c->condition->EvaluateFor( a, nullptr, c->constraint.m_Type,
                                                   a->GetLayer(), aReporter ) )
                    {
                        REPORT( _( "Rule applied." ) )
                        testAssertion( c );
                    }
                    else
                    {
                        REPORT( _( "Condition not satisfied; rule ignored." ) )
                    }
                }
            };

    if( m_constraintMap.count( ASSERTION_CONSTRAINT ) )
    {
        std::vector<DRC_ENGINE_CONSTRAINT*>* ruleset = m_constraintMap[ ASSERTION_CONSTRAINT ];

        for( int ii = 0; ii < (int) ruleset->size(); ++ii )
            processConstraint( ruleset->at( ii ) );
    }
}


#undef REPORT


bool DRC_ENGINE::IsErrorLimitExceeded( int error_code )
{
    assert( error_code >= 0 && error_code <= DRCE_LAST );
    return m_errorLimits[ error_code ] <= 0;
}


void DRC_ENGINE::ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                                  int aMarkerLayer )
{
    static std::mutex globalLock;

    m_errorLimits[ aItem->GetErrorCode() ] -= 1;

    if( m_violationHandler )
    {
        std::lock_guard<std::mutex> guard( globalLock );
        m_violationHandler( aItem, aPos, aMarkerLayer );
    }

    if( m_reporter )
    {
        wxString msg = wxString::Format( wxT( "Test '%s': %s (code %d)" ),
                                         aItem->GetViolatingTest()->GetName(),
                                         aItem->GetErrorMessage(),
                                         aItem->GetErrorCode() );

        DRC_RULE* rule = aItem->GetViolatingRule();

        if( rule )
            msg += wxString::Format( wxT( ", violating rule: '%s'" ), rule->m_Name );

        m_reporter->Report( msg );

        wxString violatingItemsStr = wxT( "Violating items: " );

        m_reporter->Report( wxString::Format( wxT( "  |- violating position (%d, %d)" ),
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


bool DRC_ENGINE::KeepRefreshing( bool aWait )
{
    if( !m_progressReporter )
        return true;

    return m_progressReporter->KeepRefreshing( aWait );
}


void DRC_ENGINE::AdvanceProgress()
{
    if( m_progressReporter )
        m_progressReporter->AdvanceProgress();
}


void DRC_ENGINE::SetMaxProgress( int aSize )
{
    if( m_progressReporter )
        m_progressReporter->SetMaxProgress( aSize );
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


bool DRC_ENGINE::IsCancelled() const
{
    return m_progressReporter && m_progressReporter->IsCancelled();
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


std::set<int> DRC_ENGINE::QueryDistinctConstraints( DRC_CONSTRAINT_T aConstraintId )
{
    std::set<int> distinctMinimums;

    if( m_constraintMap.count( aConstraintId ) )
    {
        for( DRC_ENGINE_CONSTRAINT* c : *m_constraintMap[aConstraintId] )
            distinctMinimums.emplace( c->constraint.GetValue().Min() );
    }

    return distinctMinimums;
}


// fixme: move two functions below to pcbcommon?
int DRC_ENGINE::MatchDpSuffix( const wxString& aNetName, wxString& aComplementNet,
                               wxString& aBaseDpName )
{
    int rv = 0;
    int count = 0;

    for( auto it = aNetName.rbegin(); it != aNetName.rend() && rv == 0; ++it, ++count )
    {
        int ch = *it;

        if( ( ch >= '0' && ch <= '9' ) || ch == '_' )
        {
            continue;
        }
        else if( ch == '+' )
        {
            aComplementNet = wxT( "-" );
            rv = 1;
        }
        else if( ch == '-' )
        {
            aComplementNet = wxT( "+" );
            rv = -1;
        }
        else if( ch == 'N' )
        {
            aComplementNet = wxT( "P" );
            rv = -1;
        }
        else if ( ch == 'P' )
        {
            aComplementNet = wxT( "N" );
            rv = 1;
        }
        else
        {
            break;
        }
    }

    if( rv != 0 && count >= 1 )
    {
        aBaseDpName = aNetName.Left( aNetName.Length() - count );
        aComplementNet = wxString( aBaseDpName ) << aComplementNet << aNetName.Right( count - 1 );
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


/**
 * Check if the given collision between a track and another item occurs during the track's entry
 * into a net-tie pad.
 */
bool DRC_ENGINE::IsNetTieExclusion( int aTrackNetCode, PCB_LAYER_ID aTrackLayer,
                                    const VECTOR2I& aCollisionPos, BOARD_ITEM* aCollidingItem )
{
    FOOTPRINT* parentFootprint = aCollidingItem->GetParentFootprint();

    if( parentFootprint && parentFootprint->IsNetTie() )
    {
        int                     epsilon = GetDesignSettings()->GetDRCEpsilon();
        std::map<wxString, int> padToNetTieGroupMap = parentFootprint->MapPadNumbersToNetTieGroups();

        for( PAD* pad : parentFootprint->Pads() )
        {
            if( padToNetTieGroupMap[ pad->GetNumber() ] >= 0 && aTrackNetCode == pad->GetNetCode() )
            {
                if( pad->GetEffectiveShape( aTrackLayer )->Collide( aCollisionPos, epsilon ) )
                    return true;
            }
        }
    }

    return false;
}


DRC_TEST_PROVIDER* DRC_ENGINE::GetTestProvider( const wxString& name ) const
{
    for( DRC_TEST_PROVIDER* prov : m_testProviders )
    {
        if( name == prov->GetName() )
            return prov;
    }

    return nullptr;
}
