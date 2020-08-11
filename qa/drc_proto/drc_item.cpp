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

#include <vector>
#include <fctsys.h>
#include <common.h>
#include <drc_proto/drc_item.h>
#include <class_board.h>


// These, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

test::DRC_ITEM test::DRC_ITEM::unconnectedItems( DRCE_UNCONNECTED_ITEMS,
        _( "Unconnected items" ),
        wxT( "unconnected_items" ) );

test::DRC_ITEM test::DRC_ITEM::shortingItems( DRCE_SHORTING_ITEMS,
        _( "Items shorting two nets" ),
        wxT( "shorting_items" ) );

test::DRC_ITEM test::DRC_ITEM::itemsNotAllowed( DRCE_ALLOWED_ITEMS,
        _( "Items not allowed" ),
        wxT( "items_not_allowed" ) );

test::DRC_ITEM test::DRC_ITEM::clearance( DRCE_CLEARANCE,
        _( "Clearance violation" ),
        wxT( "clearance" ) );

test::DRC_ITEM test::DRC_ITEM::tracksCrossing( DRCE_TRACKS_CROSSING,
        _( "Tracks crossing" ),
        wxT( "tracks_crossing" ) );

test::DRC_ITEM test::DRC_ITEM::copperEdgeClearance( DRCE_COPPER_EDGE_CLEARANCE,
        _( "Board edge clearance violation" ),
        wxT( "copper_edge_clearance" ) );

test::DRC_ITEM test::DRC_ITEM::zonesIntersect( DRCE_ZONES_INTERSECT,
       _( "Copper areas intersect" ),
       wxT( "zones_intersect" ) );

test::DRC_ITEM test::DRC_ITEM::zoneHasEmptyNet( DRCE_ZONE_HAS_EMPTY_NET,
        _( "Copper zone net has no pads" ),
        wxT( "zone_has_empty_net" ) );

test::DRC_ITEM test::DRC_ITEM::viaDangling( DRCE_DANGLING_VIA,
        _( "Via is not connected" ),
        wxT( "via_dangling" ) );

test::DRC_ITEM test::DRC_ITEM::trackDangling( DRCE_DANGLING_TRACK,
        _( "Track has unconnected end" ),
        wxT( "track_dangling" ) );

test::DRC_ITEM test::DRC_ITEM::holeClearance( DRCE_HOLE_CLEARANCE,
        _( "Drilled hole clearance violation" ),
        wxT( "hole_clearance" ) );

test::DRC_ITEM test::DRC_ITEM::trackWidth( DRCE_TRACK_WIDTH,
        _( "Track width outside allowed limits" ),
        wxT( "track_width" ) );

test::DRC_ITEM test::DRC_ITEM::viaTooSmall( DRCE_TOO_SMALL_VIA,
        _( "Via size too small" ),
        wxT( "via_too_small" ) );

test::DRC_ITEM test::DRC_ITEM::viaAnnulus( DRCE_VIA_ANNULUS,
        _( "Via annulus" ),
        wxT( "via_annulus" ) );

test::DRC_ITEM test::DRC_ITEM::drillTooSmall( DRCE_TOO_SMALL_DRILL,
        _( "Drill too small" ),
        wxT( "drill_too_small" ) );

test::DRC_ITEM test::DRC_ITEM::viaHoleLargerThanPad( DRCE_VIA_HOLE_BIGGER,
        _( "Via hole larger than diameter" ),
        wxT( "via_hole_larger_than_pad" ) );

test::DRC_ITEM test::DRC_ITEM::padstack( DRCE_PADSTACK,
        _( "Padstack is not valid" ),
        wxT( "padstack" ) );

test::DRC_ITEM test::DRC_ITEM::microviaTooSmall( DRCE_TOO_SMALL_MICROVIA,
        _( "Micro via size too small" ),
        wxT( "microvia_too_small" ) );

test::DRC_ITEM test::DRC_ITEM::microviaDrillTooSmall( DRCE_TOO_SMALL_MICROVIA_DRILL,
        _( "Micro via drill too small" ),
        wxT( "microvia_drill_too_small" ) );

test::DRC_ITEM test::DRC_ITEM::keepout( DRCE_KEEPOUT,
        _( "Keepout violation" ),
        wxT( "keepout" ) );

test::DRC_ITEM test::DRC_ITEM::courtyardsOverlap( DRCE_OVERLAPPING_FOOTPRINTS,
        _( "Courtyards overlap" ),
        wxT( "courtyards_overlap" ) );

test::DRC_ITEM test::DRC_ITEM::missingCourtyard( DRCE_MISSING_COURTYARD,
        _( "Footprint has no courtyard defined" ),
        wxT( "missing_courtyard" ) );

test::DRC_ITEM test::DRC_ITEM::malformedCourtyard( DRCE_MALFORMED_COURTYARD,
        _( "Footprint has malformed courtyard" ),
        wxT( "malformed_courtyard" ) );

test::DRC_ITEM test::DRC_ITEM::pthInsideCourtyard( DRCE_PTH_IN_COURTYARD,
        _( "PTH inside courtyard" ),
        wxT( "pth_inside_courtyard" ) );

test::DRC_ITEM test::DRC_ITEM::npthInsideCourtyard( DRCE_NPTH_IN_COURTYARD,
        _( "NPTH inside courtyard" ),
        wxT( "npth_inside_courtyard" ) );

test::DRC_ITEM test::DRC_ITEM::itemOnDisabledLayer( DRCE_DISABLED_LAYER_ITEM,
        _( "Item on a disabled layer" ),
        wxT( "item_on_disabled_layer" ) );

test::DRC_ITEM test::DRC_ITEM::invalidOutline( DRCE_INVALID_OUTLINE,
        _( "Board has malformed outline" ),
        wxT( "invalid_outline" ) );

test::DRC_ITEM test::DRC_ITEM::duplicateFootprints( DRCE_DUPLICATE_FOOTPRINT,
        _( "Duplicate footprints" ),
        wxT( "duplicate_footprints" ) );

test::DRC_ITEM test::DRC_ITEM::missingFootprint( DRCE_MISSING_FOOTPRINT,
        _( "Missing footprint" ),
        wxT( "missing_footprint" ) );

test::DRC_ITEM test::DRC_ITEM::extraFootprint( DRCE_EXTRA_FOOTPRINT,
        _( "Extra footprint" ),
        wxT( "extra_footprint" ) );

test::DRC_ITEM test::DRC_ITEM::unresolvedVariable( DRCE_UNRESOLVED_VARIABLE,
        _( "Unresolved text variable" ),
        wxT( "unresolved_variable" ) );


std::vector<std::reference_wrapper<RC_ITEM>> test::DRC_ITEM::allItemTypes( {
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
            DRC_ITEM::holeClearance,
            DRC_ITEM::trackWidth,
            DRC_ITEM::viaTooSmall,
            DRC_ITEM::viaAnnulus,
            DRC_ITEM::drillTooSmall,
            DRC_ITEM::viaHoleLargerThanPad,
            DRC_ITEM::padstack,
            DRC_ITEM::microviaTooSmall,
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
            DRC_ITEM::unresolvedVariable
        } );


std::shared_ptr<test::DRC_ITEM> test::DRC_ITEM::Create( int aErrorCode )
{
    DRC_ITEM *item;

    switch( aErrorCode )
    {
    case DRCE_UNCONNECTED_ITEMS:        item = new DRC_ITEM( unconnectedItems ); break;
    case DRCE_SHORTING_ITEMS:           item = new DRC_ITEM( shortingItems ); break;
    case DRCE_ALLOWED_ITEMS:            item = new DRC_ITEM( itemsNotAllowed ); break;
    case DRCE_CLEARANCE:                item = new DRC_ITEM( clearance ); break;
    case DRCE_TRACKS_CROSSING:          item = new DRC_ITEM( tracksCrossing ); break;
    case DRCE_COPPER_EDGE_CLEARANCE:    item = new DRC_ITEM( copperEdgeClearance ); break;
    case DRCE_ZONES_INTERSECT:          item = new DRC_ITEM( zonesIntersect ); break;
    case DRCE_ZONE_HAS_EMPTY_NET:       item = new DRC_ITEM( zoneHasEmptyNet ); break;
    case DRCE_DANGLING_VIA:             item = new DRC_ITEM( viaDangling ); break;
    case DRCE_DANGLING_TRACK:           item = new DRC_ITEM( trackDangling ); break;
    case DRCE_HOLE_CLEARANCE:           item = new DRC_ITEM( holeClearance ); break;
    case DRCE_TRACK_WIDTH:              item = new DRC_ITEM( trackWidth ); break;
    case DRCE_TOO_SMALL_VIA:            item = new DRC_ITEM( viaTooSmall ); break;
    case DRCE_VIA_ANNULUS:              item = new DRC_ITEM( viaAnnulus ); break;
    case DRCE_TOO_SMALL_DRILL:          item = new DRC_ITEM( drillTooSmall ); break;
    case DRCE_VIA_HOLE_BIGGER:          item = new DRC_ITEM( viaHoleLargerThanPad ); break;
    case DRCE_PADSTACK:                 item = new DRC_ITEM( padstack ); break;
    case DRCE_TOO_SMALL_MICROVIA:       item = new DRC_ITEM( microviaTooSmall ); break;
    case DRCE_TOO_SMALL_MICROVIA_DRILL: item = new DRC_ITEM( microviaDrillTooSmall ); break;
    case DRCE_KEEPOUT:                  item = new DRC_ITEM( keepout ); break;
    case DRCE_OVERLAPPING_FOOTPRINTS:   item = new DRC_ITEM( courtyardsOverlap ); break;
    case DRCE_MISSING_COURTYARD:        item = new DRC_ITEM( missingCourtyard ); break;
    case DRCE_MALFORMED_COURTYARD:      item = new DRC_ITEM( malformedCourtyard ); break;
    case DRCE_PTH_IN_COURTYARD:         item = new DRC_ITEM( pthInsideCourtyard ); break;
    case DRCE_NPTH_IN_COURTYARD:        item = new DRC_ITEM( npthInsideCourtyard ); break;
    case DRCE_DISABLED_LAYER_ITEM:      item = new DRC_ITEM( itemOnDisabledLayer ); break;
    case DRCE_INVALID_OUTLINE:          item = new DRC_ITEM( invalidOutline ); break;
    case DRCE_MISSING_FOOTPRINT:        item = new DRC_ITEM( duplicateFootprints ); break;
    case DRCE_DUPLICATE_FOOTPRINT:      item = new DRC_ITEM( missingFootprint ); break;
    case DRCE_EXTRA_FOOTPRINT:          item = new DRC_ITEM( extraFootprint ); break;
    case DRCE_UNRESOLVED_VARIABLE:      item = new DRC_ITEM( unresolvedVariable ); break;

    default:
        wxFAIL_MSG( wxString::Format( "Unknown DRC error code %d", aErrorCode ) );
        return nullptr;
    }
    return std::shared_ptr<test::DRC_ITEM>( item );
}


std::shared_ptr<test::DRC_ITEM> test::DRC_ITEM::Create( const wxString& aErrorKey )
{
    for( const RC_ITEM& item : allItemTypes )
    {
        if( aErrorKey == item.GetSettingsKey() )
            return std::shared_ptr<DRC_ITEM>( new DRC_ITEM( static_cast<const DRC_ITEM&>( item ) ) );
    }

    // This can happen if a project has old-format exclusions.  Just drop these items.
    return nullptr;
}
