/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015-2020 KiCad Developers, see change_log.txt for contributors.
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
#include <common.h>
#include "wx/html/m_templ.h"
#include "wx/html/styleparams.h"
#include <drc/drc_item.h>
#include <class_board.h>


// These, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

DRC_ITEM DRC_ITEM::unconnectedItems( DRCE_UNCONNECTED_ITEMS,
        _( "Unconnected items" ),
        wxT( "unconnected_items" ) );

DRC_ITEM DRC_ITEM::shortingItems( DRCE_SHORTING_ITEMS,
        _( "Items shorting two nets" ),
        wxT( "shorting_items" ) );

DRC_ITEM DRC_ITEM::itemsNotAllowed( DRCE_ALLOWED_ITEMS,
        _( "Items not allowed" ),
        wxT( "items_not_allowed" ) );

DRC_ITEM DRC_ITEM::clearance( DRCE_CLEARANCE,
        _( "Clearance violation" ),
        wxT( "clearance" ) );

DRC_ITEM DRC_ITEM::tracksCrossing( DRCE_TRACKS_CROSSING,
        _( "Tracks crossing" ),
        wxT( "tracks_crossing" ) );

DRC_ITEM DRC_ITEM::copperEdgeClearance( DRCE_COPPER_EDGE_CLEARANCE,
        _( "Board edge clearance violation" ),
        wxT( "copper_edge_clearance" ) );

DRC_ITEM DRC_ITEM::zonesIntersect( DRCE_ZONES_INTERSECT,
       _( "Copper areas intersect" ),
       wxT( "zones_intersect" ) );

DRC_ITEM DRC_ITEM::zoneHasEmptyNet( DRCE_ZONE_HAS_EMPTY_NET,
        _( "Copper zone net has no pads" ),
        wxT( "zone_has_empty_net" ) );

DRC_ITEM DRC_ITEM::viaDangling( DRCE_DANGLING_VIA,
        _( "Via is not connected" ),
        wxT( "via_dangling" ) );

DRC_ITEM DRC_ITEM::trackDangling( DRCE_DANGLING_TRACK,
        _( "Track has unconnected end" ),
        wxT( "track_dangling" ) );

DRC_ITEM DRC_ITEM::holeClearance( DRCE_HOLE_CLEARANCE,
        _( "Hole clearance" ),
        wxT( "hole_clearance" ) );

DRC_ITEM DRC_ITEM::holeNearHole( DRCE_DRILLED_HOLES_TOO_CLOSE,
        _( "Drilled holes too close together" ),
        wxT( "hole_near_hole" ) );

DRC_ITEM DRC_ITEM::trackWidth( DRCE_TRACK_WIDTH,
        _( "Track width" ),
        wxT( "track_width" ) );

DRC_ITEM DRC_ITEM::annularWidth( DRCE_ANNULAR_WIDTH,
        _( "Annular width" ),
        wxT( "annular_width" ) );

DRC_ITEM DRC_ITEM::drillTooSmall( DRCE_TOO_SMALL_DRILL,
        _( "Drill too small" ),
        wxT( "drill_too_small" ) );

DRC_ITEM DRC_ITEM::viaHoleLargerThanPad( DRCE_VIA_HOLE_BIGGER,
        _( "Via hole larger than diameter" ),
        wxT( "via_hole_larger_than_pad" ) );

DRC_ITEM DRC_ITEM::viaDiameter( DRCE_VIA_DIAMETER,
        _( "Via diameter" ),
        wxT( "via_diameter" ) );

DRC_ITEM DRC_ITEM::padstack( DRCE_PADSTACK,
        _( "Padstack is not valid" ),
        wxT( "padstack" ) );

DRC_ITEM DRC_ITEM::microviaDrillTooSmall( DRCE_TOO_SMALL_MICROVIA_DRILL,
        _( "Micro via drill too small" ),
        wxT( "microvia_drill_too_small" ) );

DRC_ITEM DRC_ITEM::keepout( DRCE_KEEPOUT,
        _( "Keepout violation" ),
        wxT( "keepout" ) );

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
        _( "Item on a disabled layer" ),
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

DRC_ITEM DRC_ITEM::unresolvedVariable( DRCE_UNRESOLVED_VARIABLE,
        _( "Unresolved text variable" ),
        wxT( "unresolved_variable" ) );

DRC_ITEM DRC_ITEM::silkOverPad( DRCE_SILK_OVER_PAD,
        _( "Silkscreen overlapping pad" ),
        wxT( "silk_over_pad" ) );

DRC_ITEM DRC_ITEM::silkClearance( DRCE_SILK_CLEARANCE,
        _( "Silkscreen clearance" ),
        wxT( "silk_clearance" ) );

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

std::vector<std::reference_wrapper<RC_ITEM>> DRC_ITEM::allItemTypes( {
            DRC_ITEM::unconnectedItems,
            DRC_ITEM::shortingItems,
            DRC_ITEM::itemsNotAllowed,
            DRC_ITEM::clearance,
            DRC_ITEM::tracksCrossing,
            DRC_ITEM::copperEdgeClearance,
            DRC_ITEM::zonesIntersect,
            DRC_ITEM::zoneHasEmptyNet,
            DRC_ITEM::viaDangling,
            DRC_ITEM::trackDangling,
            DRC_ITEM::holeNearHole,
            DRC_ITEM::holeClearance,
            DRC_ITEM::trackWidth,
            DRC_ITEM::annularWidth,
            DRC_ITEM::drillTooSmall,
            DRC_ITEM::viaHoleLargerThanPad,
            DRC_ITEM::padstack,
            DRC_ITEM::microviaDrillTooSmall,
            DRC_ITEM::keepout,
            DRC_ITEM::courtyardsOverlap,
            DRC_ITEM::missingCourtyard,
            DRC_ITEM::malformedCourtyard,
            DRC_ITEM::pthInsideCourtyard,
            DRC_ITEM::npthInsideCourtyard,
            DRC_ITEM::itemOnDisabledLayer,
            DRC_ITEM::invalidOutline,
            DRC_ITEM::duplicateFootprints,
            DRC_ITEM::missingFootprint,
            DRC_ITEM::extraFootprint,
            DRC_ITEM::netConflict,
            DRC_ITEM::unresolvedVariable,
            DRC_ITEM::silkClearance,
            DRC_ITEM::silkOverPad,
            DRC_ITEM::lengthOutOfRange,
            DRC_ITEM::skewOutOfRange,
            DRC_ITEM::tooManyVias,
            DRC_ITEM::diffPairGapOutOfRange,
            DRC_ITEM::diffPairUncoupledLengthTooLong
        } );


std::shared_ptr<DRC_ITEM> DRC_ITEM::Create( int aErrorCode )
{
    switch( aErrorCode )
    {
    case DRCE_UNCONNECTED_ITEMS:        return std::make_shared<DRC_ITEM>( unconnectedItems );
    case DRCE_SHORTING_ITEMS:           return std::make_shared<DRC_ITEM>( shortingItems );
    case DRCE_ALLOWED_ITEMS:            return std::make_shared<DRC_ITEM>( itemsNotAllowed );
    case DRCE_CLEARANCE:                return std::make_shared<DRC_ITEM>( clearance );
    case DRCE_TRACKS_CROSSING:          return std::make_shared<DRC_ITEM>( tracksCrossing );
    case DRCE_COPPER_EDGE_CLEARANCE:    return std::make_shared<DRC_ITEM>( copperEdgeClearance );
    case DRCE_ZONES_INTERSECT:          return std::make_shared<DRC_ITEM>( zonesIntersect );
    case DRCE_ZONE_HAS_EMPTY_NET:       return std::make_shared<DRC_ITEM>( zoneHasEmptyNet );
    case DRCE_DANGLING_VIA:             return std::make_shared<DRC_ITEM>( viaDangling );
    case DRCE_DANGLING_TRACK:           return std::make_shared<DRC_ITEM>( trackDangling );
    case DRCE_DRILLED_HOLES_TOO_CLOSE:  return std::make_shared<DRC_ITEM>( holeNearHole );
    case DRCE_HOLE_CLEARANCE:           return std::make_shared<DRC_ITEM>( holeClearance );
    case DRCE_TRACK_WIDTH:              return std::make_shared<DRC_ITEM>( trackWidth );
    case DRCE_ANNULAR_WIDTH:                  return std::make_shared<DRC_ITEM>( annularWidth );
    case DRCE_TOO_SMALL_DRILL:          return std::make_shared<DRC_ITEM>( drillTooSmall );
    case DRCE_VIA_HOLE_BIGGER:          return std::make_shared<DRC_ITEM>( viaHoleLargerThanPad );
    case DRCE_VIA_DIAMETER:             return std::make_shared<DRC_ITEM>( viaDiameter );
    case DRCE_PADSTACK:                 return std::make_shared<DRC_ITEM>( padstack );
    case DRCE_TOO_SMALL_MICROVIA_DRILL: return std::make_shared<DRC_ITEM>( microviaDrillTooSmall );
    case DRCE_KEEPOUT:                  return std::make_shared<DRC_ITEM>( keepout );
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
    case DRCE_UNRESOLVED_VARIABLE:      return std::make_shared<DRC_ITEM>( unresolvedVariable );
    case DRCE_SILK_OVER_PAD:            return std::make_shared<DRC_ITEM>( silkOverPad );
    case DRCE_SILK_CLEARANCE:           return std::make_shared<DRC_ITEM>( silkClearance );
    case DRCE_LENGTH_OUT_OF_RANGE:      return std::make_shared<DRC_ITEM>( lengthOutOfRange );
    case DRCE_SKEW_OUT_OF_RANGE:        return std::make_shared<DRC_ITEM>( skewOutOfRange );
    case DRCE_TOO_MANY_VIAS:            return std::make_shared<DRC_ITEM>( tooManyVias );
    case DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE:          return std::make_shared<DRC_ITEM>( diffPairGapOutOfRange );
    case DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG: return std::make_shared<DRC_ITEM>( diffPairUncoupledLengthTooLong );
    default:
        wxFAIL_MSG( "Unknown DRC error code" );
        return nullptr;
    }

    return nullptr;
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


wxString escapeHtml( wxString aString )
{
    aString.Replace( wxT("<"), wxT("&lt;") );
    aString.Replace( wxT(">"), wxT("&gt;") );
    return aString;
}


