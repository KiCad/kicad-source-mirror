/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015-2022 KiCad Developers, see change_log.txt for contributors.
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


#include "wx/html/m_templ.h"
#include "wx/html/styleparams.h"
#include "core/kicad_algo.h"
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <board.h>
#include <pcb_marker.h>


// These, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

// NOTE: Avoid changing the settings key for a DRC item after it has been created


DRC_ITEM DRC_ITEM::heading_electrical( 0, _( "Electrical" ), "" );
DRC_ITEM DRC_ITEM::heading_DFM( 0, _( "Design for Manufacturing" ), "" );
DRC_ITEM DRC_ITEM::heading_schematic_parity( 0, _( "Schematic Parity" ), "" );
DRC_ITEM DRC_ITEM::heading_signal_integrity( 0, _( "Signal Integrity" ), "" );
DRC_ITEM DRC_ITEM::heading_readability( 0, _( "Readability" ), "" );
DRC_ITEM DRC_ITEM::heading_misc( 0, _( "Miscellaneous" ), "" );

DRC_ITEM DRC_ITEM::unconnectedItems( DRCE_UNCONNECTED_ITEMS,
        _( "Missing connection between items" ),
        wxT( "unconnected_items" ) );

DRC_ITEM DRC_ITEM::shortingItems( DRCE_SHORTING_ITEMS,
        _( "Items shorting two nets" ),
        wxT( "shorting_items" ) );

DRC_ITEM DRC_ITEM::itemsNotAllowed( DRCE_ALLOWED_ITEMS,
        _( "Items not allowed" ),
        wxT( "items_not_allowed" ) );

DRC_ITEM DRC_ITEM::textOnEdgeCuts( DRCE_TEXT_ON_EDGECUTS,
        _( "Text (or dimension) on Edge.Cuts layer" ),
        wxT( "text_on_edge_cuts" ) );

DRC_ITEM DRC_ITEM::clearance( DRCE_CLEARANCE,
        _( "Clearance violation" ),
        wxT( "clearance" ) );

DRC_ITEM DRC_ITEM::tracksCrossing( DRCE_TRACKS_CROSSING,
        _( "Tracks crossing" ),
        wxT( "tracks_crossing" ) );

DRC_ITEM DRC_ITEM::edgeClearance( DRCE_EDGE_CLEARANCE,
        _( "Board edge clearance violation" ),
        wxT( "copper_edge_clearance" ) );

DRC_ITEM DRC_ITEM::zonesIntersect( DRCE_ZONES_INTERSECT,
        _( "Copper zones intersect" ),
        wxT( "zones_intersect" ) );

DRC_ITEM DRC_ITEM::isolatedCopper( DRCE_ISOLATED_COPPER,
        _( "Isolated copper fill" ),
        wxT( "isolated_copper" ) );

DRC_ITEM DRC_ITEM::starvedThermal( DRCE_STARVED_THERMAL,
        _( "Thermal relief connection to zone incomplete" ),
        wxT( "starved_thermal" ) );

DRC_ITEM DRC_ITEM::viaDangling( DRCE_DANGLING_VIA,
        _( "Via is not connected or connected on only one layer" ),
        wxT( "via_dangling" ) );

DRC_ITEM DRC_ITEM::trackDangling( DRCE_DANGLING_TRACK,
        _( "Track has unconnected end" ),
        wxT( "track_dangling" ) );

DRC_ITEM DRC_ITEM::holeClearance( DRCE_HOLE_CLEARANCE,
        _( "Hole clearance violation" ),
        wxT( "hole_clearance" ) );

DRC_ITEM DRC_ITEM::holeNearHole( DRCE_DRILLED_HOLES_TOO_CLOSE,
        _( "Drilled holes too close together" ),
        wxT( "hole_near_hole" ) );

DRC_ITEM DRC_ITEM::holesCoLocated( DRCE_DRILLED_HOLES_COLOCATED,
        _( "Drilled holes co-located" ),
        wxT( "holes_co_located" ) );

DRC_ITEM DRC_ITEM::connectionWidth( DRCE_CONNECTION_WIDTH,
        _( "Copper connection too narrow" ),
        wxT( "connection_width" ) );

DRC_ITEM DRC_ITEM::trackWidth( DRCE_TRACK_WIDTH,
        _( "Track width" ),
        wxT( "track_width" ) );

DRC_ITEM DRC_ITEM::annularWidth( DRCE_ANNULAR_WIDTH,
        _( "Annular width" ),
        wxT( "annular_width" ) );

DRC_ITEM DRC_ITEM::drillTooSmall( DRCE_DRILL_OUT_OF_RANGE,
        _( "Drill out of range" ),
        wxT( "drill_out_of_range" ) );

DRC_ITEM DRC_ITEM::viaDiameter( DRCE_VIA_DIAMETER,
        _( "Via diameter" ),
        wxT( "via_diameter" ) );

DRC_ITEM DRC_ITEM::padstack( DRCE_PADSTACK,
        _( "Padstack is not valid" ),
        wxT( "padstack" ) );

DRC_ITEM DRC_ITEM::microviaDrillTooSmall( DRCE_MICROVIA_DRILL_OUT_OF_RANGE,
        _( "Micro via drill out of range" ),
        wxT( "microvia_drill_out_of_range" ) );

DRC_ITEM DRC_ITEM::courtyardsOverlap( DRCE_OVERLAPPING_FOOTPRINTS,
        _( "Courtyards overlap" ),
        wxT( "courtyards_overlap" ) );

DRC_ITEM DRC_ITEM::missingCourtyard( DRCE_MISSING_COURTYARD,
        _( "Footprint has no courtyard defined" ),
        wxT( "missing_courtyard" ) );

DRC_ITEM DRC_ITEM::malformedCourtyard( DRCE_MALFORMED_COURTYARD,
        _( "Footprint has malformed courtyard" ),
        wxT( "malformed_courtyard" ) );

DRC_ITEM DRC_ITEM::pthInsideCourtyard( DRCE_PTH_IN_COURTYARD,
        _( "PTH inside courtyard" ),
        wxT( "pth_inside_courtyard" ) );

DRC_ITEM DRC_ITEM::npthInsideCourtyard( DRCE_NPTH_IN_COURTYARD,
        _( "NPTH inside courtyard" ),
        wxT( "npth_inside_courtyard" ) );

DRC_ITEM DRC_ITEM::itemOnDisabledLayer( DRCE_DISABLED_LAYER_ITEM,
        _( "Item on a disabled copper layer" ),
        wxT( "item_on_disabled_layer" ) );

DRC_ITEM DRC_ITEM::invalidOutline( DRCE_INVALID_OUTLINE,
        _( "Board has malformed outline" ),
        wxT( "invalid_outline" ) );

DRC_ITEM DRC_ITEM::duplicateFootprints( DRCE_DUPLICATE_FOOTPRINT,
        _( "Duplicate footprints" ),
        wxT( "duplicate_footprints" ) );

DRC_ITEM DRC_ITEM::missingFootprint( DRCE_MISSING_FOOTPRINT,
        _( "Missing footprint" ),
        wxT( "missing_footprint" ) );

DRC_ITEM DRC_ITEM::extraFootprint( DRCE_EXTRA_FOOTPRINT,
        _( "Extra footprint" ),
        wxT( "extra_footprint" ) );

DRC_ITEM DRC_ITEM::netConflict( DRCE_NET_CONFLICT,
        _( "Pad net doesn't match schematic" ),
        wxT( "net_conflict" ) );

DRC_ITEM DRC_ITEM::libFootprintIssues( DRCE_LIB_FOOTPRINT_ISSUES,
        _( "Footprint not found in libraries" ),
        wxT( "lib_footprint_issues" ) );

DRC_ITEM DRC_ITEM::libFootprintMismatch( DRCE_LIB_FOOTPRINT_MISMATCH,
        _( "Footprint doesn't match copy in library" ),
        wxT( "lib_footprint_mismatch" ) );

DRC_ITEM DRC_ITEM::unresolvedVariable( DRCE_UNRESOLVED_VARIABLE,
        _( "Unresolved text variable" ),
        wxT( "unresolved_variable" ) );

DRC_ITEM DRC_ITEM::assertionFailure( DRCE_ASSERTION_FAILURE,
        _( "Assertion failure" ),
        wxT( "assertion_failure" ) );

DRC_ITEM DRC_ITEM::copperSliver( DRCE_COPPER_SLIVER,
        _( "Copper sliver" ),
        wxT( "copper_sliver" ) );

DRC_ITEM DRC_ITEM::solderMaskBridge( DRCE_SOLDERMASK_BRIDGE,
        _( "Solder mask aperture bridges items with different nets" ),
        wxT( "solder_mask_bridge" ) );

DRC_ITEM DRC_ITEM::silkClearance( DRCE_SILK_CLEARANCE,
        _( "Silkscreen clipped by solder mask" ),
        wxT( "silk_over_copper" ) );

DRC_ITEM DRC_ITEM::silkEdgeClearance( DRCE_SILK_EDGE_CLEARANCE,
        _( "Silkscreen clipped by board edge" ),
        wxT( "silk_edge_clearance" ) );

DRC_ITEM DRC_ITEM::silkOverlaps( DRCE_OVERLAPPING_SILK,
        _( "Silkscreen overlap" ),
        wxT( "silk_overlap" ) );

DRC_ITEM DRC_ITEM::textHeightOutOfRange( DRCE_TEXT_HEIGHT,
        _( "Text height out of range" ),
        wxT( "text_height" ) );

DRC_ITEM DRC_ITEM::textThicknessOutOfRange( DRCE_TEXT_THICKNESS,
        _( "Text thickness out of range" ),
        wxT( "text_thickness" ) );

DRC_ITEM DRC_ITEM::lengthOutOfRange( DRCE_LENGTH_OUT_OF_RANGE,
        _( "Trace length out of range" ),
        wxT( "length_out_of_range" ) );

DRC_ITEM DRC_ITEM::skewOutOfRange( DRCE_SKEW_OUT_OF_RANGE,
        _( "Skew between traces out of range" ),
        wxT( "skew_out_of_range" ) );

DRC_ITEM DRC_ITEM::tooManyVias( DRCE_TOO_MANY_VIAS,
        _( "Too many vias on a connection" ),
        wxT( "too_many_vias" ) );

DRC_ITEM DRC_ITEM::diffPairGapOutOfRange( DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE,
        _( "Differential pair gap out of range" ),
        wxT( "diff_pair_gap_out_of_range" ) );

DRC_ITEM DRC_ITEM::diffPairUncoupledLengthTooLong( DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG,
        _( "Differential uncoupled length too long" ),
        wxT( "diff_pair_uncoupled_length_too_long" ) );

DRC_ITEM DRC_ITEM::footprint( DRCE_FOOTPRINT,
        _( "Footprint is not valid" ),
        wxT( "footprint" ) );

DRC_ITEM DRC_ITEM::footprintTypeMismatch( DRCE_FOOTPRINT_TYPE_MISMATCH,
        _( "Footprint component type doesn't match footprint pads" ),
        wxT( "footprint_type_mismatch" ) );

DRC_ITEM DRC_ITEM::footprintTHPadhasNoHole( DRCE_PAD_TH_WITH_NO_HOLE,
        _( "Through hole pad has no hole" ),
        wxT( "through_hole_pad_without_hole" ) );


std::vector<std::reference_wrapper<RC_ITEM>> DRC_ITEM::allItemTypes( {
            DRC_ITEM::heading_electrical,
            DRC_ITEM::shortingItems,
            DRC_ITEM::tracksCrossing,
            DRC_ITEM::clearance,
            DRC_ITEM::viaDangling,
            DRC_ITEM::trackDangling,
            DRC_ITEM::starvedThermal,

            DRC_ITEM::heading_DFM,
            DRC_ITEM::edgeClearance,
            DRC_ITEM::holeClearance,
            DRC_ITEM::holeNearHole,
            DRC_ITEM::trackWidth,
            DRC_ITEM::annularWidth,
            DRC_ITEM::drillTooSmall,
            DRC_ITEM::microviaDrillTooSmall,
            DRC_ITEM::courtyardsOverlap,
            DRC_ITEM::missingCourtyard,
            DRC_ITEM::malformedCourtyard,
            DRC_ITEM::invalidOutline,
            DRC_ITEM::copperSliver,
            DRC_ITEM::solderMaskBridge,
            DRC_ITEM::connectionWidth,

            DRC_ITEM::heading_schematic_parity,
            DRC_ITEM::duplicateFootprints,
            DRC_ITEM::missingFootprint,
            DRC_ITEM::extraFootprint,
            DRC_ITEM::netConflict,
            DRC_ITEM::unconnectedItems,

            DRC_ITEM::heading_signal_integrity,
            DRC_ITEM::lengthOutOfRange,
            DRC_ITEM::skewOutOfRange,
            DRC_ITEM::tooManyVias,
            DRC_ITEM::diffPairGapOutOfRange,
            DRC_ITEM::diffPairUncoupledLengthTooLong,

            DRC_ITEM::heading_readability,
            DRC_ITEM::silkOverlaps,
            DRC_ITEM::silkClearance,
            DRC_ITEM::silkEdgeClearance,
            DRC_ITEM::textHeightOutOfRange,
            DRC_ITEM::textThicknessOutOfRange,

            DRC_ITEM::heading_misc,
            DRC_ITEM::itemsNotAllowed,
            DRC_ITEM::zonesIntersect,
            DRC_ITEM::isolatedCopper,
            DRC_ITEM::footprint,
            DRC_ITEM::padstack,
            DRC_ITEM::pthInsideCourtyard,
            DRC_ITEM::npthInsideCourtyard,
            DRC_ITEM::itemOnDisabledLayer,
            DRC_ITEM::unresolvedVariable,

            DRC_ITEM::footprintTypeMismatch,
            DRC_ITEM::libFootprintIssues,
            DRC_ITEM::libFootprintMismatch,
            DRC_ITEM::footprintTHPadhasNoHole
        } );


std::shared_ptr<DRC_ITEM> DRC_ITEM::Create( int aErrorCode )
{
    switch( aErrorCode )
    {
    case DRCE_UNCONNECTED_ITEMS:        return std::make_shared<DRC_ITEM>( unconnectedItems );
    case DRCE_SHORTING_ITEMS:           return std::make_shared<DRC_ITEM>( shortingItems );
    case DRCE_ALLOWED_ITEMS:            return std::make_shared<DRC_ITEM>( itemsNotAllowed );
    case DRCE_TEXT_ON_EDGECUTS:         return std::make_shared<DRC_ITEM>( textOnEdgeCuts );
    case DRCE_CLEARANCE:                return std::make_shared<DRC_ITEM>( clearance );
    case DRCE_TRACKS_CROSSING:          return std::make_shared<DRC_ITEM>( tracksCrossing );
    case DRCE_EDGE_CLEARANCE:           return std::make_shared<DRC_ITEM>( edgeClearance );
    case DRCE_ZONES_INTERSECT:          return std::make_shared<DRC_ITEM>( zonesIntersect );
    case DRCE_ISOLATED_COPPER:          return std::make_shared<DRC_ITEM>( isolatedCopper );
    case DRCE_STARVED_THERMAL:          return std::make_shared<DRC_ITEM>( starvedThermal );
    case DRCE_DANGLING_VIA:             return std::make_shared<DRC_ITEM>( viaDangling );
    case DRCE_DANGLING_TRACK:           return std::make_shared<DRC_ITEM>( trackDangling );
    case DRCE_DRILLED_HOLES_TOO_CLOSE:  return std::make_shared<DRC_ITEM>( holeNearHole );
    case DRCE_DRILLED_HOLES_COLOCATED:  return std::make_shared<DRC_ITEM>( holesCoLocated );
    case DRCE_HOLE_CLEARANCE:           return std::make_shared<DRC_ITEM>( holeClearance );
    case DRCE_CONNECTION_WIDTH:         return std::make_shared<DRC_ITEM>( connectionWidth );
    case DRCE_TRACK_WIDTH:              return std::make_shared<DRC_ITEM>( trackWidth );
    case DRCE_ANNULAR_WIDTH:            return std::make_shared<DRC_ITEM>( annularWidth );
    case DRCE_DRILL_OUT_OF_RANGE:       return std::make_shared<DRC_ITEM>( drillTooSmall );
    case DRCE_VIA_DIAMETER:             return std::make_shared<DRC_ITEM>( viaDiameter );
    case DRCE_PADSTACK:                 return std::make_shared<DRC_ITEM>( padstack );
    case DRCE_MICROVIA_DRILL_OUT_OF_RANGE: return std::make_shared<DRC_ITEM>( microviaDrillTooSmall );
    case DRCE_OVERLAPPING_FOOTPRINTS:   return std::make_shared<DRC_ITEM>( courtyardsOverlap );
    case DRCE_MISSING_COURTYARD:        return std::make_shared<DRC_ITEM>( missingCourtyard );
    case DRCE_MALFORMED_COURTYARD:      return std::make_shared<DRC_ITEM>( malformedCourtyard );
    case DRCE_PTH_IN_COURTYARD:         return std::make_shared<DRC_ITEM>( pthInsideCourtyard );
    case DRCE_NPTH_IN_COURTYARD:        return std::make_shared<DRC_ITEM>( npthInsideCourtyard );
    case DRCE_DISABLED_LAYER_ITEM:      return std::make_shared<DRC_ITEM>( itemOnDisabledLayer );
    case DRCE_INVALID_OUTLINE:          return std::make_shared<DRC_ITEM>( invalidOutline );
    case DRCE_MISSING_FOOTPRINT:        return std::make_shared<DRC_ITEM>( missingFootprint );
    case DRCE_DUPLICATE_FOOTPRINT:      return std::make_shared<DRC_ITEM>( duplicateFootprints );
    case DRCE_NET_CONFLICT:             return std::make_shared<DRC_ITEM>( netConflict );
    case DRCE_EXTRA_FOOTPRINT:          return std::make_shared<DRC_ITEM>( extraFootprint );
    case DRCE_LIB_FOOTPRINT_ISSUES:     return std::make_shared<DRC_ITEM>( libFootprintIssues );
    case DRCE_LIB_FOOTPRINT_MISMATCH:   return std::make_shared<DRC_ITEM>( libFootprintMismatch );
    case DRCE_UNRESOLVED_VARIABLE:      return std::make_shared<DRC_ITEM>( unresolvedVariable );
    case DRCE_ASSERTION_FAILURE:        return std::make_shared<DRC_ITEM>( assertionFailure );
    case DRCE_COPPER_SLIVER:            return std::make_shared<DRC_ITEM>( copperSliver );
    case DRCE_OVERLAPPING_SILK:         return std::make_shared<DRC_ITEM>( silkOverlaps );
    case DRCE_SILK_CLEARANCE:           return std::make_shared<DRC_ITEM>( silkClearance );
    case DRCE_SILK_EDGE_CLEARANCE:      return std::make_shared<DRC_ITEM>( silkEdgeClearance );
    case DRCE_SOLDERMASK_BRIDGE:        return std::make_shared<DRC_ITEM>( solderMaskBridge );
    case DRCE_TEXT_HEIGHT:              return std::make_shared<DRC_ITEM>( textHeightOutOfRange );
    case DRCE_TEXT_THICKNESS:           return std::make_shared<DRC_ITEM>( textThicknessOutOfRange );
    case DRCE_LENGTH_OUT_OF_RANGE:      return std::make_shared<DRC_ITEM>( lengthOutOfRange );
    case DRCE_SKEW_OUT_OF_RANGE:        return std::make_shared<DRC_ITEM>( skewOutOfRange );
    case DRCE_TOO_MANY_VIAS:            return std::make_shared<DRC_ITEM>( tooManyVias );
    case DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE:          return std::make_shared<DRC_ITEM>( diffPairGapOutOfRange );
    case DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG: return std::make_shared<DRC_ITEM>( diffPairUncoupledLengthTooLong );
    case DRCE_FOOTPRINT:                return std::make_shared<DRC_ITEM>( footprint );
    case DRCE_FOOTPRINT_TYPE_MISMATCH:  return std::make_shared<DRC_ITEM>( footprintTypeMismatch );
    case DRCE_PAD_TH_WITH_NO_HOLE:      return std::make_shared<DRC_ITEM>( footprintTHPadhasNoHole );
    default:
        wxFAIL_MSG( wxT( "Unknown DRC error code" ) );
        return nullptr;
    }
}


std::shared_ptr<DRC_ITEM> DRC_ITEM::Create( const wxString& aErrorKey )
{
    for( const RC_ITEM& item : allItemTypes )
    {
        if( aErrorKey == item.GetSettingsKey() )
            return std::make_shared<DRC_ITEM>( static_cast<const DRC_ITEM&>( item ) );
    }

    // This can happen if a project has old-format exclusions.  Just drop these items.
    return nullptr;
}


KIID DRC_ITEM::GetAuxItem2ID() const
{
    if( m_errorCode == DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG )
    {
        // we have lots of segments, but it's enough to show the first P and the first N
        return niluuid;
    }

    return m_ids.size() > 2 ? m_ids[2] : niluuid;
}


KIID DRC_ITEM::GetAuxItem3ID() const
{
    if( m_errorCode == DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG )
    {
        // we have lots of segments, but it's enough to show the first P and the first N
        return niluuid;
    }

    return m_ids.size() > 3 ? m_ids[3] : niluuid;
}


wxString DRC_ITEM::GetViolatingRuleDesc() const
{
    if( m_violatingRule )
        return wxString::Format( _( "Rule: %s" ), m_violatingRule->m_Name );
    else
        return _( "Local override" );
}


void DRC_ITEMS_PROVIDER::SetSeverities( int aSeverities )
{
    m_severities = aSeverities;

    m_filteredMarkers.clear();

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        if( alg::contains( m_markerTypes, marker->GetMarkerType() )
                && ( marker->GetSeverity() & m_severities ) > 0 )
        {
            m_filteredMarkers.push_back( marker );
        }
    }
}


int DRC_ITEMS_PROVIDER::GetCount( int aSeverity ) const
{
    if( aSeverity < 0 )
        return m_filteredMarkers.size();

    int count = 0;

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        if( alg::contains( m_markerTypes, marker->GetMarkerType() )
                && ( marker->GetSeverity() & aSeverity ) > 0 )
        {
            count++;
        }
    }

    return count;
}


std::shared_ptr<RC_ITEM> DRC_ITEMS_PROVIDER::GetItem( int aIndex ) const
{
    PCB_MARKER* marker = m_filteredMarkers[ aIndex ];

    return marker ? marker->GetRCItem() : nullptr;
}


void DRC_ITEMS_PROVIDER::DeleteItem( int aIndex, bool aDeep )
{
    PCB_MARKER* marker = m_filteredMarkers[ aIndex ];
    m_filteredMarkers.erase( m_filteredMarkers.begin() + aIndex );

    if( aDeep )
        m_board->Delete( marker );
}

