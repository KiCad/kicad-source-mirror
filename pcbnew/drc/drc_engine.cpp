/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/log.h>
#include <reporter.h>
#include <common.h>
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
#include <pcb_shape.h>
#include <core/profile.h>
#include <thread_pool.h>
#include <zone.h>
#include <project/project_file.h>
#include <project/tuning_profiles.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>


// wxListBox's performance degrades horrifically with very large datasets.  It's not clear
// they're useful to the user anyway.
#define ERROR_LIMIT 199
#define EXTENDED_ERROR_LIMIT 499


/**
 * Flag to enable DRC profile timing logging.
 *
 * Use "KICAD_DRC_PROFILE" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* traceDrcProfile = wxT( "KICAD_DRC_PROFILE" );


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
        UNITS_PROVIDER( pcbIUScale, EDA_UNITS::MM ),
        m_designSettings ( aSettings ),
        m_board( aBoard ),
        m_drawingSheet( nullptr ),
        m_schematicNetlist( nullptr ),
        m_rulesValid( false ),
        m_reportAllTrackErrors( false ),
        m_testFootprints( false ),
        m_logReporter( nullptr ),
        m_progressReporter( nullptr )
{
    m_errorLimits.resize( DRCE_LAST + 1 );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        m_errorLimits[ii] = ERROR_LIMIT;
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

    if( !zone->HasKeepoutParametersSet() )
        return false;

    if( aCheckFlags )
    {
        if(    !zone->GetDoNotAllowTracks()
            && !zone->GetDoNotAllowVias()
            && !zone->GetDoNotAllowPads()
            && !zone->GetDoNotAllowZoneFills()
            && !zone->GetDoNotAllowFootprints() )
        {
            return false;
        }
    }

    return true;
}


std::shared_ptr<DRC_RULE> DRC_ENGINE::createImplicitRule( const wxString&           name,
                                                          const DRC_IMPLICIT_SOURCE aImplicitSource )
{
    std::shared_ptr<DRC_RULE> rule = std::make_shared<DRC_RULE>();

    rule->m_Name = name;
    rule->SetImplicitSource( aImplicitSource );

    addRule( rule );

    return rule;
}


void DRC_ENGINE::loadImplicitRules()
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    wxString               expr, expr2, ncName;

    // 1) global defaults

    std::shared_ptr<DRC_RULE> rule =
            createImplicitRule( _( "board setup constraints" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );

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

    rule = createImplicitRule( _( "board setup constraints zone fill strategy" ),
                               DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    DRC_CONSTRAINT thermalSpokeCountConstraint( MIN_RESOLVED_SPOKES_CONSTRAINT );
    thermalSpokeCountConstraint.Value().SetMin( bds.m_MinResolvedSpokes );
    rule->AddConstraint( thermalSpokeCountConstraint );

    rule = createImplicitRule( _( "board setup constraints silk" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    rule->m_LayerCondition = LSET( { F_SilkS, B_SilkS } );
    DRC_CONSTRAINT silkClearanceConstraint( SILK_CLEARANCE_CONSTRAINT );
    silkClearanceConstraint.Value().SetMin( bds.m_SilkClearance );
    rule->AddConstraint( silkClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints silk text height" ),
                               DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    rule->m_LayerCondition = LSET( { F_SilkS, B_SilkS } );
    DRC_CONSTRAINT silkTextHeightConstraint( TEXT_HEIGHT_CONSTRAINT );
    silkTextHeightConstraint.Value().SetMin( bds.m_MinSilkTextHeight );
    rule->AddConstraint( silkTextHeightConstraint );

    rule = createImplicitRule( _( "board setup constraints silk text thickness" ),
                               DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    rule->m_LayerCondition = LSET( { F_SilkS, B_SilkS } );
    DRC_CONSTRAINT silkTextThicknessConstraint( TEXT_THICKNESS_CONSTRAINT );
    silkTextThicknessConstraint.Value().SetMin( bds.m_MinSilkTextThickness );
    rule->AddConstraint( silkTextThicknessConstraint );

    rule = createImplicitRule( _( "board setup constraints hole" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    DRC_CONSTRAINT holeClearanceConstraint( HOLE_CLEARANCE_CONSTRAINT );
    holeClearanceConstraint.Value().SetMin( bds.m_HoleClearance );
    rule->AddConstraint( holeClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints edge" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    DRC_CONSTRAINT edgeClearanceConstraint( EDGE_CLEARANCE_CONSTRAINT );
    edgeClearanceConstraint.Value().SetMin( bds.m_CopperEdgeClearance );
    rule->AddConstraint( edgeClearanceConstraint );

    rule = createImplicitRule( _( "board setup constraints courtyard" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );
    DRC_CONSTRAINT courtyardClearanceConstraint( COURTYARD_CLEARANCE_CONSTRAINT );
    holeToHoleConstraint.Value().SetMin( 0 );
    rule->AddConstraint( courtyardClearanceConstraint );

    // 2a) micro-via specific defaults (new DRC doesn't treat microvias in any special way)

    std::shared_ptr<DRC_RULE> uViaRule =
            createImplicitRule( _( "board setup micro-via constraints" ), DRC_IMPLICIT_SOURCE::BOARD_SETUP_CONSTRAINT );

    uViaRule->m_Condition = new DRC_RULE_CONDITION( wxT( "A.Via_Type == 'Micro'" ) );

    DRC_CONSTRAINT uViaDrillConstraint( HOLE_SIZE_CONSTRAINT );
    uViaDrillConstraint.Value().SetMin( bds.m_MicroViasMinDrill );
    uViaRule->AddConstraint( uViaDrillConstraint );

    DRC_CONSTRAINT uViaDiameterConstraint( VIA_DIAMETER_CONSTRAINT );
    uViaDiameterConstraint.Value().SetMin( bds.m_MicroViasMinSize );
    uViaRule->AddConstraint( uViaDiameterConstraint );

    // 2b) barcode-specific defaults

    std::shared_ptr<DRC_RULE> barcodeRule =
            createImplicitRule( _( "barcode visual separation default" ), DRC_IMPLICIT_SOURCE::BARCODE_DEFAULTS );
    DRC_CONSTRAINT barcodeSeparationConstraint( PHYSICAL_CLEARANCE_CONSTRAINT );
    barcodeSeparationConstraint.Value().SetMin( GetIuScale().mmToIU( 1.0 ) );
    barcodeRule->AddConstraint( barcodeSeparationConstraint );
    barcodeRule->m_Condition = new DRC_RULE_CONDITION( wxT( "A.Type == 'Barcode'" ) );

    // 3) per-netclass rules

    std::vector<std::shared_ptr<DRC_RULE>> netclassClearanceRules;
    std::vector<std::shared_ptr<DRC_RULE>> netclassItemSpecificRules;

    auto makeNetclassRules =
            [&]( const std::shared_ptr<NETCLASS>& nc, bool isDefault )
            {
                ncName = nc->GetName();
                ncName.Replace( "'", "\\'" );

                if( nc->HasClearance() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ),
                                                             nc->GetClearanceParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s')" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassClearanceRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( CLEARANCE_CONSTRAINT );
                    constraint.Value().SetMin( nc->GetClearance() );
                    netclassRule->AddConstraint( constraint );

                    {
                        std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );
                        m_netclassClearances[nc->GetName()] = nc->GetClearance();
                    }
                }

                if( nc->HasTrackWidth() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ),
                                                             nc->GetTrackWidthParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s')" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassClearanceRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_TrackMinWidth );
                    constraint.Value().SetOpt( nc->GetTrackWidth() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->HasDiffPairWidth() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             nc->GetDiffPairWidthParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.inDiffPair('*')" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_TrackMinWidth );
                    constraint.Value().SetOpt( nc->GetDiffPairWidth() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->HasDiffPairGap() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (diff pair)" ),
                                                             nc->GetDiffPairGapParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s')" ), ncName );
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
                                                                 nc->GetDiffPairGapParent()->GetHumanReadableName() );
                        netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                        expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && AB.isCoupledDiffPair()" ), ncName );
                        netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                        netclassItemSpecificRules.push_back( netclassRule );

                        DRC_CONSTRAINT min_clearanceConstraint( CLEARANCE_CONSTRAINT );
                        min_clearanceConstraint.Value().SetMin( nc->GetDiffPairGap() );
                        netclassRule->AddConstraint( min_clearanceConstraint );

                        m_hasDiffPairClearanceOverrides = true;
                    }
                }

                if( nc->HasViaDiameter() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ),
                                                             nc->GetViaDiameterParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Via_Type != 'Micro'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( VIA_DIAMETER_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_ViasMinSize );
                    constraint.Value().SetOpt( nc->GetViaDiameter() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->HasViaDrill() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s'" ),
                                                             nc->GetViaDrillParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Via_Type != 'Micro'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( HOLE_SIZE_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_MinThroughDrill );
                    constraint.Value().SetOpt( nc->GetViaDrill() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->HasuViaDiameter() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (uvia)" ),
                                                             nc->GetuViaDiameterParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Via_Type == 'Micro'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( VIA_DIAMETER_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_MicroViasMinSize );
                    constraint.Value().SetMin( nc->GetuViaDiameter() );
                    netclassRule->AddConstraint( constraint );
                }

                if( nc->HasuViaDrill() )
                {
                    std::shared_ptr<DRC_RULE> netclassRule = std::make_shared<DRC_RULE>();
                    netclassRule->m_Name = wxString::Format( _( "netclass '%s' (uvia)" ),
                                                             nc->GetuViaDrillParent()->GetHumanReadableName() );
                    netclassRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::NET_CLASS );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Via_Type == 'Micro'" ), ncName );
                    netclassRule->m_Condition = new DRC_RULE_CONDITION( expr );
                    netclassItemSpecificRules.push_back( netclassRule );

                    DRC_CONSTRAINT constraint( HOLE_SIZE_CONSTRAINT );
                    constraint.Value().SetMin( bds.m_MicroViasMinDrill );
                    constraint.Value().SetOpt( nc->GetuViaDrill() );
                    netclassRule->AddConstraint( constraint );
                }
            };

    m_board->SynchronizeNetsAndNetClasses( false );
    makeNetclassRules( bds.m_NetSettings->GetDefaultNetclass(), true );

    for( const auto& [name, netclass] : bds.m_NetSettings->GetNetclasses() )
        makeNetclassRules( netclass, false );

    for( const auto& [name, netclass] : bds.m_NetSettings->GetCompositeNetclasses() )
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

    // 4) tuning profile rules
    auto addTuningSingleRule =
            [&]( const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& aLayerEntry, const wxString& aProfileName,
                 const wxString& aNetclassName )
            {
                if( aLayerEntry.GetWidth() <= 0 )
                    return;

                std::shared_ptr<DRC_RULE> tuningRule = std::make_shared<DRC_RULE>();
                tuningRule->m_Severity = bds.m_DRCSeverities[DRCE_TUNING_PROFILE_IMPLICIT_RULES];
                tuningRule->m_Name = wxString::Format( _( "tuning profile '%s'" ), aProfileName );
                tuningRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::TUNING_PROFILE );

                expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Layer == '%s'" ),
                                         aNetclassName,
                                         LSET::Name( aLayerEntry.GetSignalLayer() ) );
                tuningRule->m_Condition = new DRC_RULE_CONDITION( expr );

                DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                constraint.Value().SetMin( std::max( bds.m_TrackMinWidth, aLayerEntry.GetWidth() ) );
                constraint.Value().SetOpt( aLayerEntry.GetWidth() );
                constraint.Value().SetMax( aLayerEntry.GetWidth() );
                tuningRule->AddConstraint( constraint );

                addRule( tuningRule );
            };

    auto addTuningDifferentialRules =
            [&]( const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& aLayerEntry, const wxString& aProfileName,
                 const NETCLASS* aNetclass )
            {
                if( aLayerEntry.GetWidth() <= 0 || aLayerEntry.GetDiffPairGap() <= 0 )
                    return;

                std::shared_ptr<DRC_RULE> tuningRule = std::make_shared<DRC_RULE>();
                tuningRule->m_Severity = bds.m_DRCSeverities[DRCE_TUNING_PROFILE_IMPLICIT_RULES];
                tuningRule->m_Name = wxString::Format( _( "tuning profile '%s'" ), aProfileName );
                tuningRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::TUNING_PROFILE );

                expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Layer == '%s' && A.inDiffPair('*')" ),
                                         aNetclass->GetName(),
                                         LSET::Name( aLayerEntry.GetSignalLayer() ) );
                tuningRule->m_Condition = new DRC_RULE_CONDITION( expr );

                DRC_CONSTRAINT constraint( TRACK_WIDTH_CONSTRAINT );
                constraint.Value().SetMin( std::max( bds.m_TrackMinWidth, aLayerEntry.GetWidth() ) );
                constraint.Value().SetOpt( aLayerEntry.GetWidth() );
                constraint.Value().SetMax( aLayerEntry.GetWidth() );
                tuningRule->AddConstraint( constraint );

                addRule( tuningRule );

                std::shared_ptr<DRC_RULE> tuningRule2 = std::make_shared<DRC_RULE>();
                tuningRule2->m_Severity = bds.m_DRCSeverities[DRCE_TUNING_PROFILE_IMPLICIT_RULES];
                tuningRule2->m_Name = wxString::Format( _( "tuning profile '%s'" ), aProfileName );
                tuningRule2->SetImplicitSource( DRC_IMPLICIT_SOURCE::TUNING_PROFILE );

                expr2 = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Layer == '%s' && A.inDiffPair('*')" ),
                                          aNetclass->GetName(),
                                          LSET::Name( aLayerEntry.GetSignalLayer() ) );
                tuningRule2->m_Condition = new DRC_RULE_CONDITION( expr2 );

                DRC_CONSTRAINT constraint2( DIFF_PAIR_GAP_CONSTRAINT );
                constraint2.Value().SetMin( std::max( bds.m_MinClearance, aLayerEntry.GetDiffPairGap() ) );
                constraint2.Value().SetOpt( aLayerEntry.GetDiffPairGap() );
                constraint2.Value().SetMax( aLayerEntry.GetDiffPairGap() );
                tuningRule2->AddConstraint( constraint2 );

                addRule( tuningRule2 );

                // A narrower diffpair gap overrides the netclass min clearance
                if( aLayerEntry.GetDiffPairGap() < aNetclass->GetClearance() )
                {
                    std::shared_ptr<DRC_RULE> diffPairClearanceRule = std::make_shared<DRC_RULE>();
                    diffPairClearanceRule->m_Severity = bds.m_DRCSeverities[DRCE_TUNING_PROFILE_IMPLICIT_RULES];
                    diffPairClearanceRule->m_Name = wxString::Format( _( "tuning profile '%s'" ), aProfileName );
                    diffPairClearanceRule->SetImplicitSource( DRC_IMPLICIT_SOURCE::TUNING_PROFILE );

                    expr = wxString::Format( wxT( "A.hasExactNetclass('%s') && A.Layer == '%s' && A.inDiffPair('*')" ),
                                             aNetclass->GetName(),
                                             LSET::Name( aLayerEntry.GetSignalLayer() ) );
                    diffPairClearanceRule->m_Condition = new DRC_RULE_CONDITION( expr );

                    DRC_CONSTRAINT min_clearanceConstraint( CLEARANCE_CONSTRAINT );
                    min_clearanceConstraint.Value().SetMin( aLayerEntry.GetDiffPairGap() );
                    diffPairClearanceRule->AddConstraint( min_clearanceConstraint );

                    addRule( diffPairClearanceRule );
                }
            };

    if( PROJECT* project = m_board->GetProject() )
    {
        std::shared_ptr<TUNING_PROFILES> tuningParams = project->GetProjectFile().TuningProfileParameters();

        auto addNetclassTuningProfileRules =
                [&tuningParams, &addTuningSingleRule, &addTuningDifferentialRules]( NETCLASS* aNetclass )
                {
                    if( aNetclass->HasTuningProfile() )
                    {
                        const wxString        delayProfileName = aNetclass->GetTuningProfile();
                        const TUNING_PROFILE& profile = tuningParams->GetTuningProfile( delayProfileName );

                        for( const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& entry : profile.m_TrackPropagationEntries )
                        {
                            if( entry.GetWidth() <= 0 )
                                continue;

                            if( profile.m_Type == TUNING_PROFILE::PROFILE_TYPE::SINGLE )
                                addTuningSingleRule( entry, delayProfileName, aNetclass->GetName() );
                            else
                                addTuningDifferentialRules( entry, delayProfileName, aNetclass );
                        }
                    }
                };

        addNetclassTuningProfileRules( bds.m_NetSettings->GetDefaultNetclass().get() );

        for( const auto& [netclassName, netclass] : bds.m_NetSettings->GetNetclasses() )
            addNetclassTuningProfileRules( netclass.get() );

        for( const auto& [netclassName, netclass] : bds.m_NetSettings->GetCompositeNetclasses() )
            addNetclassTuningProfileRules( netclass.get() );
    }

    // 5) keepout area rules
    auto addKeepoutZoneRule =
            [&]( ZONE* zone, FOOTPRINT* parentFP )
            {
                const wxString& name = zone->GetZoneName();

                if( name.IsEmpty() )
                {
                    if( parentFP )
                    {
                        rule = createImplicitRule(
                                wxString::Format( _( "keepout area of %s" ), DescribeRef( parentFP->GetReference() ) ),
                                DRC_IMPLICIT_SOURCE::KEEPOUT );
                    }
                    else
                    {
                        rule = createImplicitRule( _( "keepout area" ), DRC_IMPLICIT_SOURCE::KEEPOUT );
                    }

                }
                else
                {
                    if( parentFP )
                    {
                        rule = createImplicitRule( wxString::Format( _( "keepout area '%s' of %s" ), name,
                                                                     DescribeRef( parentFP->GetReference() ) ),
                                                   DRC_IMPLICIT_SOURCE::KEEPOUT );
                    }
                    else
                    {
                        rule = createImplicitRule( wxString::Format( _( "keepout area '%s'" ), name ),
                                                   DRC_IMPLICIT_SOURCE::KEEPOUT );
                    }
                }

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

                if( zone->GetDoNotAllowZoneFills() )
                    disallowFlags |= DRC_DISALLOW_ZONES;

                if( zone->GetDoNotAllowFootprints() )
                    disallowFlags |= DRC_DISALLOW_FOOTPRINTS;

                DRC_CONSTRAINT disallowConstraint( DISALLOW_CONSTRAINT );
                disallowConstraint.m_DisallowFlags = disallowFlags;
                rule->AddConstraint( disallowConstraint );
            };

    for( ZONE* zone : m_board->Zones() )
    {
        if( isKeepoutZone( zone, true ) )
            addKeepoutZoneRule( zone, nullptr );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
        {
            if( isKeepoutZone( zone, true ) )
                addKeepoutZoneRule( zone, footprint );
        }
    }
}


void DRC_ENGINE::loadRules( const wxFileName& aPath )
{
    if( m_board && aPath.FileExists() )
    {
        std::vector<std::shared_ptr<DRC_RULE>> rules;

        if( FILE* fp = wxFopen( aPath.GetFullPath(), wxT( "rt" ) ) )
        {
            FILE_LINE_READER lineReader( fp, aPath.GetFullPath() );  // Will close rules file
            wxString         rulesText;

            std::function<bool( wxString* )> resolver =
                    [&]( wxString* token ) -> bool
                    {
                        return m_board->ResolveTextVar( token, 0 );
                    };

            while( char* line = lineReader.ReadLine() )
            {
                wxString str( line );
                str = m_board->ConvertCrossReferencesToKIIDs( str );
                str = ExpandTextVars( str, &resolver );

                rulesText << str << '\n';
            }

            DRC_RULES_PARSER parser( rulesText, aPath.GetFullPath() );
            parser.Parse( rules, m_logReporter );
        }

        // Copy the rules into the member variable afterwards so that if Parse() throws then
        // the possibly malformed rules won't contaminate the current ruleset.

        for( std::shared_ptr<DRC_RULE>& rule : rules )
            m_rules.push_back( rule );
    }
}


void DRC_ENGINE::compileRules()
{
    if( m_logReporter )
        m_logReporter->Report( wxT( "Compiling Rules" ) );

    REPORTER error_semaphore;

    for( std::shared_ptr<DRC_RULE>& rule : m_rules )
    {
        DRC_RULE_CONDITION* condition = nullptr;

        if( rule->m_Condition && !rule->m_Condition->GetExpression().IsEmpty() )
        {
            condition = rule->m_Condition;
            condition->Compile( &error_semaphore );
        }

        if( error_semaphore.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
            THROW_PARSE_ERROR( wxT( "Parse error" ), rule->m_Name,
                               TO_UTF8( rule->m_Condition->GetExpression() ), 0, 0 );

        for( const DRC_CONSTRAINT& constraint : rule->m_Constraints )
        {
            auto& ruleVec = m_constraintMap[ constraint.m_Type ];

            if( !ruleVec )
                ruleVec = new std::vector<DRC_ENGINE_CONSTRAINT*>();

            DRC_ENGINE_CONSTRAINT* engineConstraint = new DRC_ENGINE_CONSTRAINT;

            engineConstraint->layerTest = rule->m_LayerCondition;
            engineConstraint->condition = condition;
            engineConstraint->constraint = constraint;
            engineConstraint->parentRule = rule;
            ruleVec->push_back( engineConstraint );
        }
    }

    m_hasExplicitClearanceRules = false;
    m_explicitConstraints.clear();

    for( auto& [constraintType, ruleList] : m_constraintMap )
    {
        for( DRC_ENGINE_CONSTRAINT* c : *ruleList )
        {
            if( c->parentRule && !c->parentRule->IsImplicit() )
            {
                m_explicitConstraints[constraintType].push_back( c );

                if( constraintType == CLEARANCE_CONSTRAINT )
                    m_hasExplicitClearanceRules = true;
            }
        }
    }
}


void DRC_ENGINE::InitEngine( const std::shared_ptr<DRC_RULE>& rule )
{
    m_testProviders = DRC_SHOWMATCHES_PROVIDER_REGISTRY::Instance().GetShowMatchesProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        if( m_logReporter )
            m_logReporter->Report( wxString::Format( wxT( "Create DRC provider: '%s'" ), provider->GetName() ) );

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

    m_board->IncrementTimeStamp(); // Clear board-level caches

    try
    {
        m_rules.push_back( rule );
        compileRules();
    }
    catch( PARSE_ERROR& original_parse_error )
    {
        throw original_parse_error;
    }

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
        m_errorLimits[ii] = ERROR_LIMIT;

    m_rulesValid = true;
}


void DRC_ENGINE::InitEngine( const wxFileName& aRulePath )
{
    m_testProviders = DRC_TEST_PROVIDER_REGISTRY::Instance().GetTestProviders();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        if( m_logReporter )
            m_logReporter->Report( wxString::Format( wxT( "Create DRC provider: '%s'" ), provider->GetName() ) );

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

    {
        std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );
        m_ownClearanceCache.clear();
        m_netclassClearances.clear();
    }

    m_hasExplicitClearanceRules = false;
    m_hasDiffPairClearanceOverrides = false;
    m_explicitConstraints.clear();

    m_board->IncrementTimeStamp();  // Clear board-level caches

    try         // attempt to load full set of rules (implicit + user rules)
    {
        loadImplicitRules();
        loadRules( aRulePath );
        compileRules();
    }
    catch( PARSE_ERROR& original_parse_error )
    {
        m_rules.clear();

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

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        m_errorLimits[ ii ] = ERROR_LIMIT;

    m_rulesValid = true;
}


void DRC_ENGINE::RunTests( EDA_UNITS aUnits, bool aReportAllTrackErrors, bool aTestFootprints,
                           BOARD_COMMIT* aCommit )
{
    PROF_TIMER timer;

    SetUserUnits( aUnits );

    m_reportAllTrackErrors = aReportAllTrackErrors;
    m_testFootprints = aTestFootprints;

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
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

    // Recompute component classes
    m_board->GetComponentClassManager().ForceComponentClassRecalculation();

    int timestamp = m_board->GetTimeStamp();

    for( DRC_TEST_PROVIDER* provider : m_testProviders )
    {
        if( m_logReporter )
            m_logReporter->Report( wxString::Format( wxT( "Run DRC provider: '%s'" ), provider->GetName() ) );

        if( !provider->RunTests( aUnits ) )
            break;
    }

    timer.Stop();
    wxLogTrace( traceDrcProfile, "DRC took %0.3f ms", timer.msecs() );

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
                              PrintZoneConnection( constraint.m_ZoneConnection ) ) )

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
                                      PrintZoneConnection( ZONE_CONNECTION::FULL ) ) )
            constraint.m_ZoneConnection = ZONE_CONNECTION::FULL;
        }
    }

    return constraint;
}


DRC_CONSTRAINT DRC_ENGINE::EvalRules( DRC_CONSTRAINT_T aConstraintType, const BOARD_ITEM* a,
                                      const BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                                      REPORTER* aReporter )
{
    /*
     * NOTE: all string manipulation MUST BE KEPT INSIDE the REPORT macro.  It absolutely
     * kills performance when running bulk DRC tests (where aReporter is nullptr).
     */

    const BOARD_CONNECTED_ITEM* ac = a && a->IsConnected() ? static_cast<const BOARD_CONNECTED_ITEM*>( a ) : nullptr;
    const BOARD_CONNECTED_ITEM* bc = b && b->IsConnected() ? static_cast<const BOARD_CONNECTED_ITEM*>( b ) : nullptr;

    bool a_is_non_copper = a && ( !a->IsOnCopperLayer() || isKeepoutZone( a, false ) );
    bool b_is_non_copper = b && ( !b->IsOnCopperLayer() || isKeepoutZone( b, false ) );

    const PAD*       pad  = nullptr;
    const ZONE*      zone = nullptr;
    const FOOTPRINT* parentFootprint = nullptr;

    if(    aConstraintType == ZONE_CONNECTION_CONSTRAINT
        || aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT
        || aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT
        || aConstraintType == SOLDER_MASK_EXPANSION_CONSTRAINT
        || aConstraintType == SOLDER_PASTE_ABS_MARGIN_CONSTRAINT
        || aConstraintType == SOLDER_PASTE_REL_MARGIN_CONSTRAINT )
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
                {
                    if( c->parentRule && c->parentRule->IsImplicit() )
                        constraint.m_ImplicitMin = true;

                    constraint.m_Value.SetMin( c->constraint.m_Value.Min() );
                }

                if( c->constraint.m_Value.HasOpt() )
                    constraint.m_Value.SetOpt( c->constraint.m_Value.Opt() );

                if( c->constraint.m_Value.HasMax() )
                    constraint .m_Value.SetMax( c->constraint.m_Value.Max() );

                switch( c->constraint.m_Type )
                {
                case CLEARANCE_CONSTRAINT:
                case COURTYARD_CLEARANCE_CONSTRAINT:
                case SILK_CLEARANCE_CONSTRAINT:
                case HOLE_CLEARANCE_CONSTRAINT:
                case EDGE_CLEARANCE_CONSTRAINT:
                case PHYSICAL_CLEARANCE_CONSTRAINT:
                case PHYSICAL_HOLE_CLEARANCE_CONSTRAINT:
                    if( constraint.m_Value.Min() > MAXIMUM_CLEARANCE )
                        constraint.m_Value.SetMin( MAXIMUM_CLEARANCE );

                    break;

                default:
                    break;
                }

                // While the expectation would be to OR the disallow flags, we've already
                // masked them down to aItem's type -- so we're really only looking for a
                // boolean here.
                constraint.m_DisallowFlags = c->constraint.m_DisallowFlags;

                constraint.m_ZoneConnection = c->constraint.m_ZoneConnection;

                constraint.SetParentRule( c->constraint.GetParentRule() );

                constraint.SetOptionsFromOther( c->constraint );
            };

    const FOOTPRINT* footprints[2] = { a ? a->GetParentFootprint() : nullptr,
                                       b ? b->GetParentFootprint() : nullptr };

    // Handle Footprint net ties, which will zero out the clearance for footprint objects
    if( aConstraintType == CLEARANCE_CONSTRAINT // Only zero clearance, other constraints still apply
        && ( ( ( !ac ) ^ ( !bc ) )// Only apply to cases where we are comparing a connected item to a non-connected item
                                  // and not both connected.  Connected items of different nets still need to be checked
                                  // for their standard clearance value
            || ( ( footprints[0] == footprints[1] )  // Unless both items are in the same footprint
                   && footprints[0] ) )              // And that footprint exists
        && !a_is_non_copper       // Also, both elements need to be on copper layers
        && !b_is_non_copper )
    {
        const BOARD_ITEM* child_items[2] = {a, b};

        // These are the items being compared against, so the order is reversed
        const BOARD_CONNECTED_ITEM* alt_items[2] = {bc, ac};

        for( int ii = 0; ii < 2; ++ii )
        {
            // We need both a footprint item and a connected item to check for a net tie
            if( !footprints[ii] || !alt_items[ii] )
                continue;

            const std::set<int>& netcodes = footprints[ii]->GetNetTieCache( child_items[ii] );

            auto it = netcodes.find( alt_items[ii]->GetNetCode() );

            if( it != netcodes.end() )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Net tie on %s; clearance: 0." ),
                                          EscapeHTML( footprints[ii]->GetItemDescription( this, true ) ) ) )

                constraint.SetName( _( "net tie" ) );
                constraint.m_Value.SetMin( 0 );
                return constraint;
            }
        }
    }

    // Local overrides take precedence over everything *except* board min clearance
    if( aConstraintType == CLEARANCE_CONSTRAINT || aConstraintType == HOLE_CLEARANCE_CONSTRAINT )
    {
        int                override_val = 0;
        std::optional<int> overrideA;
        std::optional<int> overrideB;

        if( ac && !b_is_non_copper )
            overrideA = ac->GetClearanceOverrides( nullptr );

        if( bc && !a_is_non_copper )
            overrideB = bc->GetClearanceOverrides( nullptr );

        if( overrideA.has_value() || overrideB.has_value() )
        {
            wxString msg;

            if( overrideA.has_value() )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( a->GetItemDescription( this, true ) ),
                                          MessageTextFromValue( overrideA.value() ) ) )

                override_val = ac->GetClearanceOverrides( &msg ).value();
            }

            if( overrideB.has_value() )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "Local override on %s; clearance: %s." ),
                                          EscapeHTML( b->GetItemDescription( this, true ) ),
                                          MessageTextFromValue( overrideB.value() ) ) )

                if( overrideB > override_val )
                    override_val = bc->GetClearanceOverrides( &msg ).value();
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
        if( pad && pad->GetLocalZoneConnection() != ZONE_CONNECTION::INHERITED )
        {
            wxString msg;
            ZONE_CONNECTION override = pad->GetZoneConnectionOverrides( &msg );

            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; zone connection: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      PrintZoneConnection( override ) ) )

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
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( gap_override ) ) )

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
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( spoke_override ) ) )

            if( zone && zone->GetMinThickness() > spoke_override )
            {
                spoke_override = zone->GetMinThickness();

                REPORT( "" )
                REPORT( wxString::Format( _( "%s min thickness: %s." ),
                                          EscapeHTML( zone->GetItemDescription( this, true ) ),
                                          MessageTextFromValue( spoke_override ) ) )
            }

            constraint.SetName( msg );
            constraint.m_Value.SetMin( spoke_override );
            return constraint;
        }
    }
    else if( aConstraintType == SOLDER_MASK_EXPANSION_CONSTRAINT )
    {
        std::optional<int> override;

        if( pad )
            override = pad->GetLocalSolderMaskMargin();
        else if( a->Type() == PCB_SHAPE_T )
            override = static_cast<const PCB_SHAPE*>( a )->GetLocalSolderMaskMargin();
        else if( const PCB_TRACK* track = dynamic_cast<const PCB_TRACK*>( a ) )
            override = track->GetLocalSolderMaskMargin();

        if( override )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; solder mask expansion: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( override.value() ) ) )

            constraint.m_Value.SetOpt( override.value() );
            return constraint;
        }
    }
    else if( aConstraintType == SOLDER_PASTE_ABS_MARGIN_CONSTRAINT )
    {
        std::optional<int> override;

        if( pad )
            override = pad->GetLocalSolderPasteMargin();

        if( override )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; solder paste absolute clearance: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( override.value() ) ) )

            constraint.m_Value.SetOpt( override.value_or( 0 ) );
            return constraint;
        }
    }
    else if( aConstraintType == SOLDER_PASTE_REL_MARGIN_CONSTRAINT )
    {
        std::optional<double> overrideRatio;

        if( pad )
            overrideRatio = pad->GetLocalSolderPasteMarginRatio();

        if( overrideRatio )
        {
            REPORT( "" )
            REPORT( wxString::Format( _( "Local override on %s; solder paste relative clearance: %s." ),
                                      EscapeHTML( pad->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( overrideRatio.value() * 100.0 ) ) )

            constraint.m_Value.SetOpt( KiROUND( overrideRatio.value_or( 0 ) * 1000 ) );
            return constraint;
        }
    }

    auto testAssertion =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
                REPORT( wxString::Format( _( "Checking assertion '%s'." ),
                                          EscapeHTML( c->constraint.m_Test->GetExpression() ) ) )

                if( c->constraint.m_Test->EvaluateFor( a, b, c->constraint.m_Type, aLayer, aReporter ) )
                    REPORT( _( "Assertion passed." ) )
                else
                    REPORT( EscapeHTML( _( "--> Assertion failed. <--" ) ) )
            };

    auto processConstraint =
            [&]( const DRC_ENGINE_CONSTRAINT* c )
            {
                bool implicit = c->parentRule && c->parentRule->IsImplicit();

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
                case CREEPAGE_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s creepage: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                    break;
                case MAX_UNCOUPLED_CONSTRAINT:
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

                case SOLDER_MASK_EXPANSION_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s solder mask expansion: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                    break;

                case SOLDER_PASTE_ABS_MARGIN_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s solder paste absolute clearance: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                    break;

                case SOLDER_PASTE_REL_MARGIN_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s solder paste relative clearance: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                    break;

                case MIN_RESOLVED_SPOKES_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s min spoke count: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              MessageTextFromUnscaledValue( c->constraint.m_Value.Min() ) ) )
                    break;

                case ZONE_CONNECTION_CONSTRAINT:
                    REPORT( wxString::Format( _( "Checking %s zone connection: %s." ),
                                              EscapeHTML( c->constraint.GetName() ),
                                              PrintZoneConnection( c->constraint.m_ZoneConnection ) ) )
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
                    if( implicit )
                    {
                        switch( c->constraint.m_Type )
                        {
                        case TRACK_WIDTH_CONSTRAINT:
                            if( c->constraint.m_Value.HasOpt() )
                            {
                                REPORT( wxString::Format( _( "Checking %s track width: opt %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                            }
                            else if( c->constraint.m_Value.HasMin() )
                            {
                                REPORT( wxString::Format( _( "Checking %s track width: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            }

                            break;

                        case ANNULAR_WIDTH_CONSTRAINT:
                            REPORT( wxString::Format( _( "Checking %s annular width: min %s." ),
                                                      EscapeHTML( c->constraint.GetName() ),
                                                      MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                            break;

                        case VIA_DIAMETER_CONSTRAINT:
                            if( c->constraint.m_Value.HasOpt() )
                            {
                                REPORT( wxString::Format( _( "Checking %s via diameter: opt %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                            }
                            else if( c->constraint.m_Value.HasMin() )
                            {
                                REPORT( wxString::Format( _( "Checking %s via diameter: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            }
                            break;

                        case HOLE_SIZE_CONSTRAINT:
                            if( c->constraint.m_Value.HasOpt() )
                            {
                                REPORT( wxString::Format( _( "Checking %s hole size: opt %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                            }
                            else if( c->constraint.m_Value.HasMin() )
                            {
                                REPORT( wxString::Format( _( "Checking %s hole size: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            }

                            break;

                        case TEXT_HEIGHT_CONSTRAINT:
                        case TEXT_THICKNESS_CONSTRAINT:
                        case CONNECTION_WIDTH_CONSTRAINT:
                            REPORT( wxString::Format( _( "Checking %s: min %s." ),
                                                      EscapeHTML( c->constraint.GetName() ),
                                                      MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            break;

                        case DIFF_PAIR_GAP_CONSTRAINT:
                            if( c->constraint.m_Value.HasOpt() )
                            {
                                REPORT( wxString::Format( _( "Checking %s diff pair gap: opt %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Opt() ) ) )
                            }
                            else if( c->constraint.m_Value.HasMin() )
                            {
                                REPORT( wxString::Format( _( "Checking %s clearance: min %s." ),
                                                          EscapeHTML( c->constraint.GetName() ),
                                                          MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            }

                            break;

                        case HOLE_TO_HOLE_CONSTRAINT:
                            REPORT( wxString::Format( _( "Checking %s hole to hole: min %s." ),
                                                      EscapeHTML( c->constraint.GetName() ),
                                                      MessageTextFromValue( c->constraint.m_Value.Min() ) ) )
                            break;

                        default:
                            REPORT( wxString::Format( _( "Checking %s." ),
                                                      EscapeHTML( c->constraint.GetName() ) ) )
                        }
                    }
                    else
                    {
                        REPORT( wxString::Format( _( "Checking %s: min %s; opt %s; max %s." ),
                                                  EscapeHTML( c->constraint.GetName() ),
                                                  c->constraint.m_Value.HasMin()
                                                                ? MessageTextFromValue( c->constraint.m_Value.Min() )
                                                                : wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" ),
                                                  c->constraint.m_Value.HasOpt()
                                                                ? MessageTextFromValue( c->constraint.m_Value.Opt() )
                                                                : wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" ),
                                                  c->constraint.m_Value.HasMax()
                                                                ? MessageTextFromValue( c->constraint.m_Value.Max() )
                                                                : wxT( "<i>" ) + _( "undefined" ) + wxT( "</i>" ) ) )
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
                                                      EscapeHTML( a->GetItemDescription( this, true ) ) ) )
                        }
                        else if( b_is_non_copper )
                        {
                            REPORT( wxString::Format( _( "%s contains no copper.  Rule ignored." ),
                                                      EscapeHTML( b->GetItemDescription( this, true ) ) ) )
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
                        const PCB_VIA* via = static_cast<const PCB_VIA*>( a );

                        if( via->IsMicroVia() )
                            mask = DRC_DISALLOW_MICRO_VIAS;
                        else if( via->IsBlindVia() )
                            mask = DRC_DISALLOW_BLIND_VIAS;
                        else if( via->IsBuriedVia() )
                            mask = DRC_DISALLOW_BURIED_VIAS;
                        else
                            mask = DRC_DISALLOW_THROUGH_VIAS;
                    }
                    else
                    {
                        switch( a->Type() )
                        {
                        case PCB_TRACE_T: mask = DRC_DISALLOW_TRACKS; break;
                        case PCB_ARC_T: mask = DRC_DISALLOW_TRACKS; break;
                        case PCB_PAD_T: mask = DRC_DISALLOW_PADS; break;
                        case PCB_FOOTPRINT_T: mask = DRC_DISALLOW_FOOTPRINTS; break;
                        case PCB_SHAPE_T: mask = DRC_DISALLOW_GRAPHICS; break;
                        case PCB_BARCODE_T: mask = DRC_DISALLOW_GRAPHICS; break;
                        case PCB_FIELD_T: mask = DRC_DISALLOW_TEXTS; break;
                        case PCB_TEXT_T: mask = DRC_DISALLOW_TEXTS; break;
                        case PCB_TEXTBOX_T: mask = DRC_DISALLOW_TEXTS; break;
                        case PCB_TABLE_T: mask = DRC_DISALLOW_TEXTS; break;

                        case PCB_ZONE_T:
                            // Treat teardrop areas as tracks for DRC purposes
                            if( static_cast<const ZONE*>( a )->IsTeardropArea() )
                                mask = DRC_DISALLOW_TRACKS;
                            else
                                mask = DRC_DISALLOW_ZONES;

                            break;

                        case PCB_LOCATE_HOLE_T: mask = DRC_DISALLOW_HOLES; break;
                        default: mask = 0; break;
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
                        && ( !a->HasDrilledHole() && !b->HasDrilledHole() ) )
                {
                    // Report non-drilled-holes as an implicit condition
                    REPORT( wxString::Format( _( "%s is not a drilled hole; rule ignored." ),
                                              a->GetItemDescription( this, true ) ) )
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
                        REPORT( wxString::Format( _( "Checking rule condition '%s'." ),
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

    // Fast-path for netclass clearance when no explicit or diff pair override rules exist
    if( aConstraintType == CLEARANCE_CONSTRAINT
        && !m_hasExplicitClearanceRules
        && !m_hasDiffPairClearanceOverrides
        && !aReporter
        && !a_is_non_copper
        && ( !b || !b_is_non_copper ) )
    {
        int clearance = 0;

        // Get netclass names outside of the lock to minimize critical section
        wxString ncNameA;
        wxString ncNameB;

        if( ac )
        {
            NETCLASS* ncA = ac->GetEffectiveNetClass();

            if( ncA )
                ncNameA = ncA->GetName();
        }

        if( bc )
        {
            NETCLASS* ncB = bc->GetEffectiveNetClass();

            if( ncB )
                ncNameB = ncB->GetName();
        }

        // Look up clearances with shared lock protection
        if( !ncNameA.empty() || !ncNameB.empty() )
        {
            std::shared_lock<std::shared_mutex> readLock( m_clearanceCacheMutex );

            if( !ncNameA.empty() )
            {
                auto it = m_netclassClearances.find( ncNameA );

                if( it != m_netclassClearances.end() )
                    clearance = it->second;
            }

            if( !ncNameB.empty() )
            {
                auto it = m_netclassClearances.find( ncNameB );

                if( it != m_netclassClearances.end() )
                    clearance = std::max( clearance, it->second );
            }
        }

        if( clearance > 0 )
        {
            constraint.m_Value.SetMin( clearance );
            constraint.m_ImplicitMin = true;
        }
    }
    else
    {
        auto it = m_constraintMap.find( aConstraintType );

        if( it != m_constraintMap.end() )
        {
            for( DRC_ENGINE_CONSTRAINT* rule : *it->second )
                processConstraint( rule );
        }
    }

    if( constraint.GetParentRule() && !constraint.GetParentRule()->IsImplicit() )
        return constraint;

    // Special case for properties which can be inherited from parent footprints.  We've already
    // checked for local overrides, and there were no rules targetting the item itself, so we know
    // we're inheriting and need to see if there are any rules targetting the parent footprint.
    if( pad && parentFootprint && (   aConstraintType == ZONE_CONNECTION_CONSTRAINT
                                   || aConstraintType == THERMAL_RELIEF_GAP_CONSTRAINT
                                   || aConstraintType == THERMAL_SPOKE_WIDTH_CONSTRAINT
                                   || aConstraintType == SOLDER_MASK_EXPANSION_CONSTRAINT
                                   || aConstraintType == SOLDER_PASTE_ABS_MARGIN_CONSTRAINT
                                   || aConstraintType == SOLDER_PASTE_REL_MARGIN_CONSTRAINT ) )
    {
        REPORT( "" )
        REPORT( wxString::Format( _( "Inheriting from parent: %s." ),
                                  EscapeHTML( parentFootprint->GetItemDescription( this, true ) ) ) )

        if( a == pad )
            a = parentFootprint;
        else
            b = parentFootprint;

        auto it = m_constraintMap.find( aConstraintType );

        if( it != m_constraintMap.end() )
        {
            for( DRC_ENGINE_CONSTRAINT* rule : *it->second )
                processConstraint( rule );

            if( constraint.GetParentRule() && !constraint.GetParentRule()->IsImplicit() )
                return constraint;
        }

        // Found nothing again?  Return the defaults.
        if( aConstraintType == SOLDER_MASK_EXPANSION_CONSTRAINT )
        {
            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "board setup" ) );
            constraint.m_Value.SetOpt( m_designSettings->m_SolderMaskExpansion );
            return constraint;
        }
        else if( aConstraintType == SOLDER_PASTE_ABS_MARGIN_CONSTRAINT )
        {
            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "board setup" ) );
            constraint.m_Value.SetOpt( m_designSettings->m_SolderPasteMargin );
            return constraint;
        }
        else if( aConstraintType == SOLDER_PASTE_REL_MARGIN_CONSTRAINT )
        {
            constraint.SetParentRule( nullptr );
            constraint.SetName( _( "board setup" ) );
            constraint.m_Value.SetOpt( KiROUND( m_designSettings->m_SolderPasteMarginRatio * 1000 ) );
            return constraint;
        }
    }

    // Unfortunately implicit rules don't work for local clearances (such as zones) because
    // they have to be max'ed with netclass values (which are already implicit rules), and our
    // rule selection paradigm is "winner takes all".
    if( aConstraintType == CLEARANCE_CONSTRAINT )
    {
        int  global = constraint.m_Value.Min();
        int  clearance = global;
        bool needBlankLine = true;

        if( ac && ac->GetLocalClearance().has_value() )
        {
            int localA = ac->GetLocalClearance().value();

            if( needBlankLine )
            {
                REPORT( "" )
                needBlankLine = false;
            }

            REPORT( wxString::Format( _( "Local clearance on %s: %s." ),
                                      EscapeHTML( a->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( localA ) ) )

            if( localA > clearance )
            {
                wxString msg;
                clearance = ac->GetLocalClearance( &msg ).value();
                constraint.SetParentRule( nullptr );
                constraint.SetName( msg );
                constraint.m_Value.SetMin( clearance );
            }
        }

        if( bc && bc->GetLocalClearance().has_value() )
        {
            int localB = bc->GetLocalClearance().value();

            if( needBlankLine )
            {
                REPORT( "" )
                needBlankLine = false;
            }

            REPORT( wxString::Format( _( "Local clearance on %s: %s." ),
                                      EscapeHTML( b->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( localB ) ) )

            if( localB > clearance )
            {
                wxString msg;
                clearance = bc->GetLocalClearance( &msg ).value();
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
            ZONE_CONNECTION local = parentFootprint->GetLocalZoneConnection();

            if( local != ZONE_CONNECTION::INHERITED )
            {
                REPORT( "" )
                REPORT( wxString::Format( _( "%s zone connection: %s." ),
                                          EscapeHTML( parentFootprint->GetItemDescription( this, true ) ),
                                          PrintZoneConnection( local ) ) )

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
                                      EscapeHTML( zone->GetItemDescription( this, true ) ),
                                      PrintZoneConnection( local ) ) )

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
                                      EscapeHTML( zone->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( local ) ) )

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
                                      EscapeHTML( zone->GetItemDescription( this, true ) ),
                                      MessageTextFromValue( local ) ) )

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


DRC_CLEARANCE_BATCH DRC_ENGINE::EvalClearanceBatch( const BOARD_ITEM* a, const BOARD_ITEM* b,
                                                     PCB_LAYER_ID aLayer )
{
    DRC_CLEARANCE_BATCH result;
    DRC_CONSTRAINT c;

    c = EvalRules( CLEARANCE_CONSTRAINT, a, b, aLayer );

    if( c.m_Value.HasMin() )
        result.clearance = c.m_Value.Min();

    c = EvalRules( HOLE_CLEARANCE_CONSTRAINT, a, b, aLayer );

    if( c.m_Value.HasMin() )
        result.holeClearance = c.m_Value.Min();

    c = EvalRules( HOLE_TO_HOLE_CONSTRAINT, a, b, aLayer );

    if( c.m_Value.HasMin() )
        result.holeToHole = c.m_Value.Min();

    c = EvalRules( EDGE_CLEARANCE_CONSTRAINT, a, b, aLayer );

    if( c.m_Value.HasMin() )
        result.edgeClearance = c.m_Value.Min();

    c = EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, a, b, aLayer );

    if( c.m_Value.HasMin() )
        result.physicalClearance = c.m_Value.Min();

    return result;
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
                REPORT( wxString::Format( _( "Checking rule assertion '%s'." ),
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
                    REPORT( wxString::Format( _( "Checking rule condition '%s'." ),
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

    auto it = m_constraintMap.find( ASSERTION_CONSTRAINT );

    if( it != m_constraintMap.end() )
    {
        for( int ii = 0; ii < (int) it->second->size(); ++ii )
            processConstraint( it->second->at( ii ) );
    }
}


#undef REPORT


bool DRC_ENGINE::IsErrorLimitExceeded( int error_code )
{
    assert( error_code >= 0 && error_code <= DRCE_LAST );
    return m_errorLimits[ error_code ] <= 0;
}


void DRC_ENGINE::ReportViolation( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                                  int aMarkerLayer, const std::function<void( PCB_MARKER* )>& aPathGenerator )
{
    static std::mutex globalLock;

    m_errorLimits[ aItem->GetErrorCode() ] -= 1;

    if( m_violationHandler )
    {
        std::lock_guard<std::mutex> guard( globalLock );
        m_violationHandler( aItem, aPos, aMarkerLayer, aPathGenerator );
    }

    if( m_logReporter )
    {
        wxString msg = wxString::Format( wxT( "Test '%s': %s (code %d)" ),
                                         aItem->GetViolatingTest()->GetName(),
                                         aItem->GetErrorMessage( false ),
                                         aItem->GetErrorCode() );

        DRC_RULE* rule = aItem->GetViolatingRule();

        if( rule )
            msg += wxString::Format( wxT( ", violating rule: '%s'" ), rule->m_Name );

        m_logReporter->Report( msg );

        wxString violatingItemsStr = wxT( "Violating items: " );

        m_logReporter->Report( wxString::Format( wxT( "  |- violating position (%d, %d)" ),
                                              aPos.x,
                                              aPos.y ) );
    }
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
    bool retval = m_progressReporter->KeepRefreshing( false );
    wxSafeYield( nullptr, true ); // Force an update for the message
    return retval;
}


bool DRC_ENGINE::IsCancelled() const
{
    return m_progressReporter && m_progressReporter->IsCancelled();
}


bool DRC_ENGINE::HasRulesForConstraintType( DRC_CONSTRAINT_T constraintID )
{
    auto it = m_constraintMap.find( constraintID );
    return it != m_constraintMap.end() && !it->second->empty();
}


bool DRC_ENGINE::QueryWorstConstraint( DRC_CONSTRAINT_T aConstraintId, DRC_CONSTRAINT& aConstraint )
{
    int  worst = 0;
    auto it = m_constraintMap.find( aConstraintId );

    if( it != m_constraintMap.end() )
    {
        for( DRC_ENGINE_CONSTRAINT* c : *it->second )
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
    auto          it = m_constraintMap.find( aConstraintId );

    if( it != m_constraintMap.end() )
    {
        for( DRC_ENGINE_CONSTRAINT* c : *it->second )
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


std::vector<BOARD_ITEM*> DRC_ENGINE::GetItemsMatchingCondition( const wxString& aExpression,
                                                                DRC_CONSTRAINT_T aConstraint,
                                                                REPORTER* aReporter )
{
    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "[ShowMatches] engine enter: expr='%s', constraint=%d" ), aExpression, (int) aConstraint );
    std::vector<BOARD_ITEM*> matches;

    if( !m_board )
        return matches;

    DRC_RULE_CONDITION condition( aExpression );

    if( !condition.Compile( aReporter ? aReporter : m_logReporter ) )
    {
        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "[ShowMatches] engine: compile failed" ) );
        return matches;
    }

    // Rebuild the from-to cache so that fromTo() expressions can be evaluated.
    // This cache requires explicit rebuilding before use since it depends on the full
    // connectivity graph being available.
    if( auto connectivity = m_board->GetConnectivity() )
    {
        if( auto ftCache = connectivity->GetFromToCache() )
            ftCache->Rebuild( m_board );
    }

    BOARD_ITEM_SET items = m_board->GetItemSet();
    size_t totalItems = 0;
    size_t skippedItems = 0;
    size_t noLayerItems = 0;
    size_t checkedItems = 0;

    for( auto& [kiid, item] : m_board->GetItemByIdCache() )
    {
        totalItems++;

        // Skip items that don't have visible geometry or can't be meaningfully matched
        switch( item->Type() )
        {
        case PCB_NETINFO_T:
        case PCB_GENERATOR_T:
        case PCB_GROUP_T:
            skippedItems++;
            continue;

        default:
            break;
        }

        LSET itemLayers = item->GetLayerSet();

        if( itemLayers.none() )
        {
            noLayerItems++;
            continue;
        }

        checkedItems++;
        bool matched = false;

        for( PCB_LAYER_ID layer : itemLayers )
        {
            if( condition.EvaluateFor( item, nullptr, static_cast<int>( aConstraint ), layer,
                                    aReporter ? aReporter : m_logReporter ) )
            {
                matches.push_back( item );
                wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                            wxS( "[ShowMatches] engine: match type=%d kiid=%s layer=%d" ),
                            (int) item->Type(), kiid.AsString(), (int) layer );
                matched = true;
                break; // No need to check other layers
            }
        }

        // Log a few non-matching items to help debug condition issues
        if( !matched && matches.size() == 0 && checkedItems <= 5 )
        {
            wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                        wxS( "[ShowMatches] engine: no-match sample type=%d kiid=%s layers=%s" ),
                        (int) item->Type(), kiid.AsString(), itemLayers.FmtHex() );
        }
    }

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "[ShowMatches] engine stats: total=%zu skipped=%zu noLayer=%zu checked=%zu" ),
                totalItems, skippedItems, noLayerItems, checkedItems );

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "[ShowMatches] engine exit: total=%zu" ), matches.size() );
    return matches;
}


int DRC_ENGINE::GetCachedOwnClearance( const BOARD_ITEM* aItem, PCB_LAYER_ID aLayer,
                                       wxString* aSource )
{
    DRC_OWN_CLEARANCE_CACHE_KEY key{ aItem->m_Uuid, aLayer };

    // Fast path: check cache with shared (read) lock
    {
        std::shared_lock<std::shared_mutex> readLock( m_clearanceCacheMutex );

        auto it = m_ownClearanceCache.find( key );

        if( it != m_ownClearanceCache.end() )
        {
            // Cache hit. We don't cache the source string since it's rarely requested
            // and caching it would add complexity.
            return it->second;
        }
    }

    // Cache miss - evaluate the constraint (outside lock to avoid blocking other threads)
    DRC_CONSTRAINT_T constraintType = CLEARANCE_CONSTRAINT;

    if( aItem->Type() == PCB_PAD_T )
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
            constraintType = HOLE_CLEARANCE_CONSTRAINT;
    }

    DRC_CONSTRAINT constraint = EvalRules( constraintType, aItem, nullptr, aLayer );

    int clearance = 0;

    if( constraint.Value().HasMin() )
    {
        clearance = constraint.Value().Min();

        if( aSource )
            *aSource = constraint.GetName();
    }

    // Store in cache with exclusive (write) lock using double-checked locking
    {
        std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );

        auto it = m_ownClearanceCache.find( key );

        if( it == m_ownClearanceCache.end() )
            m_ownClearanceCache[key] = clearance;
    }

    return clearance;
}


void DRC_ENGINE::InvalidateClearanceCache( const KIID& aUuid )
{
    std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );

    // Remove all entries for this item (across all layers)
    auto it = m_ownClearanceCache.begin();

    while( it != m_ownClearanceCache.end() )
    {
        if( it->first.m_uuid == aUuid )
            it = m_ownClearanceCache.erase( it );
        else
            ++it;
    }
}


void DRC_ENGINE::ClearClearanceCache()
{
    std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );
    m_ownClearanceCache.clear();
}


void DRC_ENGINE::InitializeClearanceCache()
{
    if( !m_board )
        return;

    // Pre-populate the cache for all connected items to avoid delays during first render.
    // We only need to cache copper layers since clearance outlines are only drawn on copper.

    LSET copperLayers = m_board->GetEnabledLayers() & LSET::AllCuMask();

    using CLEARANCE_MAP = std::unordered_map<DRC_OWN_CLEARANCE_CACHE_KEY, int>;

    // Build flat list of (item, layer) pairs to process.
    // Estimate size based on tracks + pads * average layers (2 for typical TH pads).
    std::vector<std::pair<const BOARD_ITEM*, PCB_LAYER_ID>> itemsToProcess;
    size_t estimatedPads = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
        estimatedPads += footprint->Pads().size();

    itemsToProcess.reserve( m_board->Tracks().size() + estimatedPads * 2 );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            for( PCB_LAYER_ID layer : LSET( track->GetLayerSet() & copperLayers ).Seq() )
                itemsToProcess.emplace_back( track, layer );
        }
        else
        {
            itemsToProcess.emplace_back( track, track->GetLayer() );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            for( PCB_LAYER_ID layer : LSET( pad->GetLayerSet() & copperLayers ).Seq() )
                itemsToProcess.emplace_back( pad, layer );
        }
    }

    if( itemsToProcess.empty() )
        return;

    {
        std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );
        m_ownClearanceCache.reserve( itemsToProcess.size() );
    }

    thread_pool& tp = GetKiCadThreadPool();

    auto processItems = [this]( size_t aStart, size_t aEnd,
                                const std::vector<std::pair<const BOARD_ITEM*, PCB_LAYER_ID>>& aItems )
                                -> CLEARANCE_MAP
    {
        CLEARANCE_MAP localCache;

        for( size_t i = aStart; i < aEnd; ++i )
        {
            const BOARD_ITEM* item = aItems[i].first;
            PCB_LAYER_ID layer = aItems[i].second;

            DRC_CONSTRAINT_T constraintType = CLEARANCE_CONSTRAINT;

            if( item->Type() == PCB_PAD_T )
            {
                const PAD* pad = static_cast<const PAD*>( item );

                if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                    constraintType = HOLE_CLEARANCE_CONSTRAINT;
            }

            DRC_CONSTRAINT constraint = EvalRules( constraintType, item, nullptr, layer );

            int clearance = 0;

            if( constraint.Value().HasMin() )
                clearance = constraint.Value().Min();

            localCache[{ item->m_Uuid, layer }] = clearance;
        }

        return localCache;
    };

    auto results = tp.submit_blocks( 0, itemsToProcess.size(),
            [&]( size_t aStart, size_t aEnd ) -> CLEARANCE_MAP
            {
                return processItems( aStart, aEnd, itemsToProcess );
            } );

    // Collect all results first WITHOUT holding the lock to avoid deadlock.
    // Worker threads call EvalRules() which needs a read lock on m_clearanceCacheMutex.
    // If we held a write lock while calling .get(), workers would block on the read lock
    // while we block waiting for them to complete.
    std::vector<CLEARANCE_MAP> collectedResults;
    collectedResults.reserve( results.size() );

    for( size_t i = 0; i < results.size(); ++i )
    {
        if( results[i].valid() )
            collectedResults.push_back( results[i].get() );
    }

    // Now merge with write lock held, but no blocking on futures
    {
        std::unique_lock<std::shared_mutex> writeLock( m_clearanceCacheMutex );

        for( const auto& localCache : collectedResults )
            m_ownClearanceCache.insert( localCache.begin(), localCache.end() );
    }
}
