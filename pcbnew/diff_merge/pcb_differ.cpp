/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include "pcb_differ.h"

#include <diff_merge/doc_property_helpers.h>
#include <diff_merge/property_diff.h>
#include <hashtables.h>

#include <board.h>
#include <board_design_settings.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <netclass.h>
#include <wildcards_and_files_ext.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_field.h>
#include <zone.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

#include <algorithm>


namespace KICAD_DIFF
{

PCB_DIFFER::PCB_DIFFER( const BOARD* aBefore, const BOARD* aAfter, const wxString& aPath ) :
        m_before( aBefore ),
        m_after( aAfter ),
        m_path( aPath )
{
}


PCB_DIFFER::~PCB_DIFFER() = default;


wxString PCB_DIFFER::itemTypeName( const BOARD_ITEM* aItem )
{
    if( !aItem )
        return wxEmptyString;

    return aItem->GetClass();
}


std::optional<wxString> PCB_DIFFER::itemRefdes( const BOARD_ITEM* aItem )
{
    if( auto fp = dynamic_cast<const FOOTPRINT*>( aItem ) )
        return fp->GetReference();

    if( auto track = dynamic_cast<const PCB_TRACK*>( aItem ) )
    {
        if( track->GetNetCode() > 0 && !track->GetNetname().IsEmpty() )
            return track->GetNetname();
    }

    if( auto fp = dynamic_cast<const FOOTPRINT*>( aItem ? aItem->GetParent() : nullptr ) )
    {
        // Pad / footprint-child: borrow the parent's refdes for cross-probing.
        return fp->GetReference();
    }

    return std::nullopt;
}


ITEM_DESCRIPTOR PCB_DIFFER::makeDescriptor( const BOARD_ITEM* aItem ) const
{
    ITEM_DESCRIPTOR d;
    d.id = KIID_PATH();
    d.id.push_back( aItem->m_Uuid );
    d.type = itemTypeName( aItem );
    d.position = aItem->GetPosition();
    d.bbox = aItem->GetBoundingBox();

    // keyProps for similarity fallback: identifying fields specific to the
    // item type. Footprints carry their library id (very stable); tracks
    // and pads carry net code.
    if( auto fp = dynamic_cast<const FOOTPRINT*>( aItem ) )
    {
        d.keyProps.emplace_back( wxS( "lib_id" ), fp->GetFPIDAsString().ToStdString() );
        d.keyProps.emplace_back( wxS( "reference" ), fp->GetReference().ToStdString() );
    }
    else if( auto track = dynamic_cast<const PCB_TRACK*>( aItem ) )
    {
        d.keyProps.emplace_back( wxS( "net" ), std::to_string( track->GetNetCode() ) );
        d.keyProps.emplace_back( wxS( "layer" ), std::to_string( track->GetLayer() ) );
    }
    else if( auto pad = dynamic_cast<const PAD*>( aItem ) )
    {
        d.keyProps.emplace_back( wxS( "number" ), pad->GetNumber().ToStdString() );
        d.keyProps.emplace_back( wxS( "net" ), std::to_string( pad->GetNetCode() ) );
    }
    else if( auto zone = dynamic_cast<const ZONE*>( aItem ) )
    {
        d.keyProps.emplace_back( wxS( "name" ), zone->GetZoneName().ToStdString() );
        d.keyProps.emplace_back( wxS( "net" ), std::to_string( zone->GetNetCode() ) );
    }

    return d;
}


// Library metadata that is never worth showing, for any change kind.
static bool pcbLibraryMetadataNoise( const wxString& aName )
{
    return aName == wxS( "Library Link" ) || aName == wxS( "Library Description" ) || aName == wxS( "Keywords" );
}


// Project-default propagation and library-reorganization noise. Inside a
// footprint, per-child layout shifts follow the parent's move, so they are
// noise too.
static bool pcbDiffPropertyIsNoise( const wxString& aName, bool aInsideFootprint )
{
    const bool globalNoise = pcbLibraryMetadataNoise( aName ) || aName == wxS( "Auto Thickness" )
                             || aName == wxS( "Keep Upright" ) || aName == wxS( "Thickness" )
                             || aName == wxS( "Enable Teardrops" ) || aName == wxS( "Thermal Relief Spoke Angle" )
                             || aName == wxS( "Pin Name" ) || aName == wxS( "Net" ) || aName == wxS( "Pad Shape" );

    if( globalNoise )
        return true;

    const bool childLayoutNoise = aName == wxS( "Position X" ) || aName == wxS( "Position Y" )
                                  || aName == wxS( "Start X" ) || aName == wxS( "Start Y" ) || aName == wxS( "End X" )
                                  || aName == wxS( "End Y" ) || aName == wxS( "Center X" ) || aName == wxS( "Center Y" )
                                  || aName == wxS( "Orientation" ) || aName == wxS( "Height" )
                                  || aName == wxS( "Width" ) || aName == wxS( "Layer" ) || aName == wxS( "Line Width" )
                                  || aName == wxS( "Hole Size X" ) || aName == wxS( "Hole Size Y" );

    return aInsideFootprint && childLayoutNoise;
}


// Full property list for an added or removed item. Only library metadata is
// dropped: net, pad shape, geometry and the like describe what was added or
// removed and are worth showing, unlike in the modified case.
static std::vector<PROPERTY_DELTA> pcbAddedRemovedProperties( const BOARD_ITEM* aItem, bool aAsAfter )
{
    std::vector<PROPERTY_DELTA> deltas = ItemProperties( aItem, aAsAfter );

    deltas.erase( std::remove_if( deltas.begin(), deltas.end(),
                                  [&]( const PROPERTY_DELTA& d )
                                  {
                                      return pcbLibraryMetadataNoise( d.name );
                                  } ),
                  deltas.end() );

    return deltas;
}


std::vector<PROPERTY_DELTA> PCB_DIFFER::diffProperties( const BOARD_ITEM* aBefore, const BOARD_ITEM* aAfter ) const
{
    auto deltas = DiffItemProperties( aBefore, aAfter );

    const bool insideFootprint = aBefore && aBefore->GetParent() && aBefore->GetParent()->Type() == PCB_FOOTPRINT_T;

    deltas.erase( std::remove_if( deltas.begin(), deltas.end(),
                                  [&]( const PROPERTY_DELTA& d )
                                  {
                                      return pcbDiffPropertyIsNoise( d.name, insideFootprint );
                                  } ),
                  deltas.end() );

    if( auto zoneA = dynamic_cast<const ZONE*>( aBefore ) )
    {
        if( auto zoneB = dynamic_cast<const ZONE*>( aAfter ) )
        {
            auto toPolygonSet = []( const SHAPE_POLY_SET* aPoly ) -> DIFF_VALUE::PolygonSet
            {
                DIFF_VALUE::PolygonSet out;

                if( !aPoly )
                    return out;

                for( int o = 0; o < aPoly->OutlineCount(); ++o )
                {
                    const auto&                        polyA = aPoly->CPolygon( o );
                    std::vector<std::vector<VECTOR2I>> contours;

                    for( const auto& contour : polyA )
                    {
                        std::vector<VECTOR2I> pts;
                        pts.reserve( contour.PointCount() );

                        for( int p = 0; p < contour.PointCount(); ++p )
                            pts.push_back( contour.CPoint( p ) );

                        contours.push_back( std::move( pts ) );
                    }

                    out.push_back( std::move( contours ) );
                }

                return out;
            };

            DIFF_VALUE::PolygonSet outlineA = toPolygonSet( zoneA->Outline() );
            DIFF_VALUE::PolygonSet outlineB = toPolygonSet( zoneB->Outline() );

            if( outlineA != outlineB )
            {
                PROPERTY_DELTA d;
                d.name = wxS( "Outline" );
                d.before = DIFF_VALUE::FromPolygonSet( std::move( outlineA ) );
                d.after = DIFF_VALUE::FromPolygonSet( std::move( outlineB ) );
                deltas.push_back( std::move( d ) );
            }

            for( PCB_LAYER_ID layer : zoneB->GetLayerSet().Seq() )
            {
                if( !zoneA->GetLayerSet().Contains( layer ) )
                    continue;

                DIFF_VALUE::PolygonSet fillA = toPolygonSet( zoneA->GetFilledPolysList( layer ).get() );
                DIFF_VALUE::PolygonSet fillB = toPolygonSet( zoneB->GetFilledPolysList( layer ).get() );

                if( fillA == fillB )
                    continue;

                PROPERTY_DELTA d;
                d.name = wxString::Format( wxS( "Filled Area (%s)" ), LayerName( layer ) );
                d.before = DIFF_VALUE::FromPolygonSet( std::move( fillA ) );
                d.after = DIFF_VALUE::FromPolygonSet( std::move( fillB ) );
                deltas.push_back( std::move( d ) );
            }
        }
    }

    return deltas;
}


std::vector<ITEM_CHANGE> PCB_DIFFER::diffFootprintChildren( const FOOTPRINT* aBefore, const FOOTPRINT* aAfter ) const
{
    std::vector<ITEM_CHANGE> children;

    if( !aBefore || !aAfter )
        return children;

    // Collect child items keyed by (parent_footprint_uuid, child_uuid) so the
    // identifier is globally meaningful — child UUIDs alone are not
    // sufficiently unique to be used as merge-engine keys outside their parent.
    std::vector<ITEM_DESCRIPTOR>           beforeDesc;
    std::vector<ITEM_DESCRIPTOR>           afterDesc;
    std::map<KIID_PATH, const BOARD_ITEM*> beforeMap;
    std::map<KIID_PATH, const BOARD_ITEM*> afterMap;

    auto childDescriptor = [&]( const FOOTPRINT* aFp, const BOARD_ITEM* aChild ) -> ITEM_DESCRIPTOR
    {
        ITEM_DESCRIPTOR d = makeDescriptor( aChild );
        d.id = KIID_PATH();
        d.id.push_back( aFp->m_Uuid );
        d.id.push_back( aChild->m_Uuid );
        return d;
    };

    auto collect = [&]( const FOOTPRINT* aFp, std::vector<ITEM_DESCRIPTOR>& aOut,
                        std::map<KIID_PATH, const BOARD_ITEM*>& aMap )
    {
        for( const PAD* pad : aFp->Pads() )
        {
            ITEM_DESCRIPTOR d = childDescriptor( aFp, pad );
            aOut.push_back( d );
            aMap[d.id] = pad;
        }

        for( const BOARD_ITEM* item : aFp->GraphicalItems() )
        {
            ITEM_DESCRIPTOR d = childDescriptor( aFp, item );
            aOut.push_back( d );
            aMap[d.id] = item;
        }

        for( const ZONE* zone : aFp->Zones() )
        {
            ITEM_DESCRIPTOR d = childDescriptor( aFp, zone );
            aOut.push_back( d );
            aMap[d.id] = zone;
        }

        for( const PCB_FIELD* field : aFp->GetFields() )
        {
            ITEM_DESCRIPTOR d = childDescriptor( aFp, field );
            aOut.push_back( d );
            aMap[d.id] = field;
        }
    };

    collect( aBefore, beforeDesc, beforeMap );
    collect( aAfter, afterDesc, afterMap );

    IDENTITY_RECONCILER reconciler( m_options.identity );
    RECONCILIATION      recon = reconciler.Reconcile( beforeDesc, afterDesc );

    // Matched pairs: compute property deltas. Items count as "changed" if any
    // property surfaced a delta OR if their semantic operator== rejects equality
    // (which covers fields that aren't exposed through PROPERTY_MANAGER).
    for( const auto& [idA, idB] : recon.aToB )
    {
        auto              itA = beforeMap.find( idA );
        auto              itB = afterMap.find( idB );
        const BOARD_ITEM* a = itA == beforeMap.end() ? nullptr : itA->second;
        const BOARD_ITEM* b = itB == afterMap.end() ? nullptr : itB->second;

        if( !a || !b )
            continue;

        std::vector<PROPERTY_DELTA> propDeltas;

        if( m_options.deepCompare )
            propDeltas = diffProperties( a, b );

        if( propDeltas.empty() )
            continue;

        ITEM_CHANGE c;
        c.id = idA;
        c.typeName = itemTypeName( a );
        c.kind = CHANGE_KIND::MODIFIED;
        c.bbox = b->GetBoundingBox();
        c.refdes = itemRefdes( b );
        c.properties = std::move( propDeltas );
        children.push_back( std::move( c ) );
    }

    // Footprint-internal PCB_SHAPE, PCB_TEXT, and PCB_FIELD add/remove are
    // dropped here: when a library link is updated, the footprint's body
    // outlines, silk text, and metadata fields get fresh UUIDs even though
    // the footprint itself is the same one. The reconciler then reports each
    // of them as removed-and-readded.
    auto isLibraryUuidNoise = []( const BOARD_ITEM* aItem )
    {
        return aItem && ( aItem->Type() == PCB_SHAPE_T || aItem->Type() == PCB_TEXT_T || aItem->Type() == PCB_FIELD_T );
    };

    for( const KIID_PATH& idA : recon.aOnly )
    {
        auto it = beforeMap.find( idA );

        if( it == beforeMap.end() || !it->second )
            continue;

        const BOARD_ITEM* a = it->second;

        if( isLibraryUuidNoise( a ) )
            continue;

        ITEM_CHANGE       c;
        c.id = idA;
        c.typeName = itemTypeName( a );
        c.kind = CHANGE_KIND::REMOVED;
        c.bbox = a->GetBoundingBox();
        c.refdes = itemRefdes( a );
        c.properties = pcbAddedRemovedProperties( a, /*aAsAfter=*/false );
        children.push_back( std::move( c ) );
    }

    for( const KIID_PATH& idB : recon.bOnly )
    {
        auto it = afterMap.find( idB );

        if( it == afterMap.end() || !it->second )
            continue;

        const BOARD_ITEM* b = it->second;

        if( isLibraryUuidNoise( b ) )
            continue;

        ITEM_CHANGE       c;
        c.id = idB;
        c.typeName = itemTypeName( b );
        c.kind = CHANGE_KIND::ADDED;
        c.bbox = b->GetBoundingBox();
        c.refdes = itemRefdes( b );
        c.properties = pcbAddedRemovedProperties( b, /*aAsAfter=*/true );
        children.push_back( std::move( c ) );
    }

    sortChanges( children );
    return children;
}


void PCB_DIFFER::sortChanges( std::vector<ITEM_CHANGE>& aChanges )
{
    std::sort( aChanges.begin(), aChanges.end(),
               []( const ITEM_CHANGE& aL, const ITEM_CHANGE& aR )
               {
                   if( aL.id < aR.id )
                       return true;
                   if( aR.id < aL.id )
                       return false;

                   if( aL.typeName != aR.typeName )
                       return aL.typeName < aR.typeName;

                   return static_cast<int>( aL.kind ) < static_cast<int>( aR.kind );
               } );
}


DOCUMENT_DIFF PCB_DIFFER::Diff()
{
    DOCUMENT_DIFF result;
    result.path = m_path;
    result.docType = wxS( "kicad_pcb" );

    if( !m_before || !m_after )
        return result;

    // Build descriptors for the top-level item set.
    const BOARD_ITEM_SET beforeSet = m_before->GetItemSet();
    const BOARD_ITEM_SET afterSet = m_after->GetItemSet();

    std::vector<ITEM_DESCRIPTOR>      beforeDesc;
    std::vector<ITEM_DESCRIPTOR>      afterDesc;
    std::map<KIID, const BOARD_ITEM*> beforeMap;
    std::map<KIID, const BOARD_ITEM*> afterMap;

    beforeDesc.reserve( beforeSet.size() );
    afterDesc.reserve( afterSet.size() );

    for( const BOARD_ITEM* item : beforeSet )
    {
        if( !item )
            continue;

        beforeDesc.push_back( makeDescriptor( item ) );
        beforeMap[item->m_Uuid] = item;
    }

    for( const BOARD_ITEM* item : afterSet )
    {
        if( !item )
            continue;

        afterDesc.push_back( makeDescriptor( item ) );
        afterMap[item->m_Uuid] = item;
    }

    if( m_options.progress )
        m_options.progress( 0.2 );

    IDENTITY_RECONCILER reconciler( m_options.identity );
    RECONCILIATION      recon = reconciler.Reconcile( beforeDesc, afterDesc );

    if( m_options.progress )
        m_options.progress( 0.5 );

    // Duplicate-UUID records (within either side).
    for( const KIID_PATH& dup : recon.duplicatesA )
    {
        ITEM_CHANGE c;
        c.id = dup;
        c.typeName = wxS( "BOARD_ITEM" );
        c.kind = CHANGE_KIND::DUPLICATE_UUID;
        result.changes.push_back( std::move( c ) );
    }

    for( const KIID_PATH& dup : recon.duplicatesB )
    {
        if( std::find_if( result.changes.begin(), result.changes.end(),
                          [&]( const ITEM_CHANGE& aC )
                          {
                              return aC.id == dup && aC.kind == CHANGE_KIND::DUPLICATE_UUID;
                          } )
            != result.changes.end() )
        {
            continue;
        }

        ITEM_CHANGE c;
        c.id = dup;
        c.typeName = wxS( "BOARD_ITEM" );
        c.kind = CHANGE_KIND::DUPLICATE_UUID;
        result.changes.push_back( std::move( c ) );
    }

    // Matched pairs.
    for( const auto& [idA, idB] : recon.aToB )
    {
        const KIID&       uuidA = idA.back();
        const KIID&       uuidB = idB.back();
        auto              itA = beforeMap.find( uuidA );
        auto              itB = afterMap.find( uuidB );
        const BOARD_ITEM* a = itA == beforeMap.end() ? nullptr : itA->second;
        const BOARD_ITEM* b = itB == afterMap.end() ? nullptr : itB->second;

        if( !a || !b )
            continue;

        std::vector<PROPERTY_DELTA> propDeltas;

        if( m_options.deepCompare )
            propDeltas = diffProperties( a, b );

        // For footprints, also walk children. A footprint may compare unequal
        // because a child changed, in which case the parent's properties
        // themselves haven't moved — we still need to record children.
        std::vector<ITEM_CHANGE> childChanges;

        if( auto fpA = dynamic_cast<const FOOTPRINT*>( a ) )
        {
            if( auto fpB = dynamic_cast<const FOOTPRINT*>( b ) )
                childChanges = diffFootprintChildren( fpA, fpB );
        }

        if( propDeltas.empty() && childChanges.empty() )
            continue;

        ITEM_CHANGE c;
        c.id = idA;
        c.typeName = itemTypeName( a );
        c.kind = CHANGE_KIND::MODIFIED;
        c.bbox = b->GetBoundingBox();
        c.refdes = itemRefdes( b );
        c.properties = std::move( propDeltas );
        c.children = std::move( childChanges );
        result.changes.push_back( std::move( c ) );
    }

    // Auto-generated items (teardrops, etc.) flip in lockstep with a global
    // setting rather than being user-authored. Their presence on one side and
    // not the other is reported as the Enable Teardrops setting change, not
    // as N separate add/remove records. Teardrops historically render as
    // ZONEs flagged with IsTeardropArea, hence the second branch.
    auto isAutoGenerated = []( const BOARD_ITEM* aItem )
    {
        if( !aItem )
            return false;

        if( aItem->Type() == PCB_GENERATOR_T )
            return true;

        if( auto zone = dynamic_cast<const ZONE*>( aItem ) )
            return zone->IsTeardropArea();

        return false;
    };

    // Items present only in ancestor: REMOVED.
    for( const KIID_PATH& idA : recon.aOnly )
    {
        auto it = beforeMap.find( idA.back() );

        if( it == beforeMap.end() || !it->second )
            continue;

        const BOARD_ITEM* a = it->second;

        if( isAutoGenerated( a ) )
            continue;

        ITEM_CHANGE c;
        c.id = idA;
        c.typeName = itemTypeName( a );
        c.kind = CHANGE_KIND::REMOVED;
        c.bbox = a->GetBoundingBox();
        c.refdes = itemRefdes( a );
        c.properties = pcbAddedRemovedProperties( a, /*aAsAfter=*/false );

        // For footprints, snapshot child items so the consumer can show what's
        // being removed without re-walking.
        if( auto fp = dynamic_cast<const FOOTPRINT*>( a ) )
        {
            std::vector<ITEM_CHANGE> dummyAfter;
            FOOTPRINT                empty( nullptr );
            // diffFootprintChildren needs two footprints; producing an empty
            // "after" gives us a REMOVED record per child.
            c.children = diffFootprintChildren( fp, &empty );
        }

        result.changes.push_back( std::move( c ) );
    }

    // Items present only in after: ADDED.
    for( const KIID_PATH& idB : recon.bOnly )
    {
        auto it = afterMap.find( idB.back() );

        if( it == afterMap.end() || !it->second )
            continue;

        const BOARD_ITEM* b = it->second;

        if( isAutoGenerated( b ) )
            continue;

        ITEM_CHANGE c;
        c.id = idB;
        c.typeName = itemTypeName( b );
        c.kind = CHANGE_KIND::ADDED;
        c.bbox = b->GetBoundingBox();
        c.refdes = itemRefdes( b );
        c.properties = pcbAddedRemovedProperties( b, /*aAsAfter=*/true );

        if( auto fp = dynamic_cast<const FOOTPRINT*>( b ) )
        {
            FOOTPRINT empty( nullptr );
            c.children = diffFootprintChildren( &empty, fp );
        }

        result.changes.push_back( std::move( c ) );
    }

    // Document-level settings — board thickness, paper format. These aren't
    // walked items so the per-item loop above can't catch a change to them;
    // emit a single synthetic ITEM_CHANGE with an empty KIID_PATH so the
    // merge engine can plan a resolution on it and the applier knows which
    // side's settings to carry over. Without this the applier defaults to
    // the new (empty) result BOARD's settings and silently reverts both
    // sides' divergent changes to defaults.
    std::vector<PROPERTY_DELTA> docDeltas;

    AppendPaperDeltas( docDeltas, m_before->GetPageSettings(), m_after->GetPageSettings() );

    const BOARD_DESIGN_SETTINGS& beforeDS = m_before->GetDesignSettings();
    const BOARD_DESIGN_SETTINGS& afterDS = m_after->GetDesignSettings();

    const int beforeThickness = beforeDS.GetBoardThickness();
    const int afterThickness = afterDS.GetBoardThickness();

    if( beforeThickness != afterThickness )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_BOARD_THICKNESS;
        d.before = DIFF_VALUE::FromInt( beforeThickness );
        d.after = DIFF_VALUE::FromInt( afterThickness );
        docDeltas.push_back( std::move( d ) );
    }

    // Stackup is structural (layer count, dielectric materials, copper
    // weights) so a per-field walk would explode the change list. Detect
    // any change with BOARD_STACKUP::operator== and emit a single delta
    // carrying a human-readable summary; the applier copies whole
    // BOARD_DESIGN_SETTINGS so a TAKE_OURS / TAKE_THEIRS resolution preserves
    // the chosen side's full stackup.
    const BOARD_STACKUP& beforeStackup = beforeDS.GetStackupDescriptor();
    const BOARD_STACKUP& afterStackup = afterDS.GetStackupDescriptor();

    auto summarizeStackup = []( const BOARD_STACKUP& aStackup ) -> std::string
    {
        int copper = 0;
        int dielectric = 0;

        for( const BOARD_STACKUP_ITEM* item : aStackup.GetList() )
        {
            if( !item )
                continue;

            if( item->GetType() == BS_ITEM_TYPE_COPPER )
                ++copper;
            else if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
                ++dielectric;
        }

        // Content hash matching BOARD_STACKUP::operator== fields so
        // before/after render differently when any compared field
        // changed and the copper/dielectric counts didn't.
        std::size_t h = std::hash<std::string>{}( aStackup.m_FinishType.ToStdString() );
        h = KiHashCombine( h, std::hash<bool>{}( aStackup.m_HasDielectricConstrains ) );
        h = KiHashCombine( h, std::hash<bool>{}( aStackup.m_HasThicknessConstrains ) );
        h = KiHashCombine( h, std::hash<bool>{}( aStackup.m_EdgePlating ) );
        h = KiHashCombine( h, std::hash<int>{}( static_cast<int>( aStackup.m_EdgeConnectorConstraints ) ) );

        for( const BOARD_STACKUP_ITEM* item : aStackup.GetList() )
        {
            if( !item )
                continue;

            h = KiHashCombine( h, std::hash<int>{}( static_cast<int>( item->GetType() ) ) );
            h = KiHashCombine( h, std::hash<int>{}( static_cast<int>( item->GetBrdLayerId() ) ) );
            h = KiHashCombine( h, std::hash<std::string>{}( item->GetLayerName().ToStdString() ) );
            h = KiHashCombine( h, std::hash<bool>{}( item->IsEnabled() ) );

            for( int sub = 0; sub < item->GetSublayersCount(); ++sub )
            {
                h = KiHashCombine( h, std::hash<int>{}( item->GetThickness( sub ) ) );
                h = KiHashCombine( h, std::hash<std::string>{}( item->GetMaterial( sub ).ToStdString() ) );
                h = KiHashCombine( h, std::hash<std::string>{}( item->GetColor( sub ).ToStdString() ) );

                if( item->HasEpsilonRValue() )
                    h = KiHashCombine( h, std::hash<double>{}( item->GetEpsilonR( sub ) ) );

                if( item->HasLossTangentValue() )
                    h = KiHashCombine( h, std::hash<double>{}( item->GetLossTangent( sub ) ) );
            }
        }

        return wxString::Format( wxS( "%d copper / %d dielectric layers (hash %zx)" ), copper, dielectric, h )
                .ToStdString();
    };

    if( beforeStackup != afterStackup )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_LAYER_STACKUP;
        d.before = DIFF_VALUE::FromString( summarizeStackup( beforeStackup ) );
        d.after = DIFF_VALUE::FromString( summarizeStackup( afterStackup ) );
        docDeltas.push_back( std::move( d ) );
    }

    // DRC severity overrides live in the project file. Diff only fires when
    // sibling .kicad_pro files were loaded — for plain .kicad_pcb temp blobs
    // (git mergetool case) both sides see defaults and we never get here.
    const std::map<int, SEVERITY>& beforeDRC = beforeDS.m_DRCSeverities;
    const std::map<int, SEVERITY>& afterDRC = afterDS.m_DRCSeverities;

    if( beforeDRC != afterDRC )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_DRC_SEVERITIES;
        d.before = DIFF_VALUE::FromString( SummarizeSeverities( beforeDRC ) );
        d.after = DIFF_VALUE::FromString( SummarizeSeverities( afterDRC ) );
        docDeltas.push_back( std::move( d ) );
    }

    // Net classes live in PROJECT_FILE via NET_SETTINGS.  Both NET_SETTINGS
    // instances are reached via BOARD_DESIGN_SETTINGS::m_NetSettings (a
    // shared_ptr).  After A1, NET_SETTINGS::operator== is content-aware
    // (covers default-netclass in-place edits + per-named-class parameter
    // edits + label / pattern / color / chain-class maps), so equality
    // alone is enough to gate the delta.  Render a count + content hash
    // summary so two configurations with the same class count but
    // different parameters produce distinct before / after strings.
    auto summarizeNetSettings = []( const NET_SETTINGS& aSettings ) -> std::string
    {
        std::size_t h = 0;
        auto        hashCombine = [&h]( std::size_t v )
        {
            h = KiHashCombine( h, v );
        };

        auto hashNetclass = [&]( const NETCLASS* nc )
        {
            if( !nc )
                return;

            hashCombine( std::hash<std::string>{}( nc->GetName().ToStdString() ) );
            hashCombine( static_cast<std::size_t>( nc->GetPriority() ) );
            hashCombine( static_cast<std::size_t>( nc->GetClearanceOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetTrackWidthOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetViaDiameterOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetViaDrillOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetuViaDiameterOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetuViaDrillOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetDiffPairWidthOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetDiffPairGapOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetDiffPairViaGapOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetWireWidthOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetBusWidthOpt().value_or( -1 ) ) );
            hashCombine( static_cast<std::size_t>( nc->GetLineStyleOpt().value_or( -1 ) ) );
            hashCombine( std::hash<std::string>{}( nc->GetTuningProfile().ToStdString() ) );

            // Per-netclass color overrides are part of NETCLASS::EqualsByPersistedFields;
            // include them so a color-only edit produces a distinct rendered summary.
            hashCombine( std::hash<std::string>{}( nc->GetSchematicColor( true ).ToCSSString().ToStdString() ) );
            hashCombine( std::hash<std::string>{}( nc->GetPcbColor( true ).ToCSSString().ToStdString() ) );
        };

        hashNetclass( aSettings.GetDefaultNetclass().get() );

        for( const auto& [name, nc] : aSettings.GetNetclasses() )
            hashNetclass( nc.get() );

        for( const auto& [netname, classes] : aSettings.GetNetclassLabelAssignments() )
        {
            hashCombine( std::hash<std::string>{}( netname.ToStdString() ) );

            for( const wxString& c : classes )
                hashCombine( std::hash<std::string>{}( c.ToStdString() ) );
        }

        for( const auto& [chain, className] : aSettings.GetNetChainClasses() )
        {
            hashCombine( std::hash<std::string>{}( chain.ToStdString() ) );
            hashCombine( std::hash<std::string>{}( className.ToStdString() ) );
        }

        for( const auto& [netname, color] : aSettings.GetNetColorAssignments() )
        {
            hashCombine( std::hash<std::string>{}( netname.ToStdString() ) );
            hashCombine( std::hash<std::string>{}( color.ToCSSString().ToStdString() ) );
        }

        const std::size_t classCount = aSettings.GetNetclasses().size() + ( aSettings.GetDefaultNetclass() ? 1u : 0u );

        return wxString::Format( wxS( "%zu netclass(es) (hash %zx)" ), classCount, h ).ToStdString();
    };

    const std::shared_ptr<NET_SETTINGS>& beforeNet = beforeDS.m_NetSettings;
    const std::shared_ptr<NET_SETTINGS>& afterNet = afterDS.m_NetSettings;

    // Diff fires only when both sides have NET_SETTINGS instances.  A null
    // pointer on either side means the BOARD wasn't loaded with a project file
    // (e.g. plain .kicad_pcb temp blob from git mergetool); skip silently —
    // matches DRC severities behaviour.
    if( beforeNet && afterNet && *beforeNet != *afterNet )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_NET_CLASSES;
        d.before = DIFF_VALUE::FromString( summarizeNetSettings( *beforeNet ) );
        d.after = DIFF_VALUE::FromString( summarizeNetSettings( *afterNet ) );
        docDeltas.push_back( std::move( d ) );
    }

    // Custom DRC rules live in a sibling .kicad_dru file next to the .kicad_pcb.
    // The file content isn't stored on BOARD; we read it from disk.  For
    // headless / temp-blob merges (git mergetool) the sibling file usually
    // isn't present and both sides return empty strings, so no delta fires.
    auto readSiblingDruContent = []( const BOARD* aBoard ) -> wxString
    {
        if( !aBoard )
            return wxEmptyString;

        wxString boardPath = aBoard->GetFileName();

        if( boardPath.IsEmpty() )
            return wxEmptyString;

        wxFileName fn( boardPath );
        fn.SetExt( FILEEXT::DesignRulesFileExtension );

        if( !fn.FileExists() )
            return wxEmptyString;

        wxFile file( fn.GetFullPath() );

        if( !file.IsOpened() )
            return wxEmptyString;

        wxString contents;
        file.ReadAll( &contents );
        return contents;
    };

    auto summarizeRules = []( const wxString& aContents ) -> std::string
    {
        if( aContents.IsEmpty() )
            return "(no custom rules)";

        std::size_t h = std::hash<std::string>{}( aContents.ToStdString() );
        return wxString::Format( wxS( "%zu byte(s) (hash %zx)" ), aContents.size(), h ).ToStdString();
    };

    const wxString beforeRules = readSiblingDruContent( m_before );
    const wxString afterRules = readSiblingDruContent( m_after );

    if( beforeRules != afterRules )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_CUSTOM_RULES;
        d.before = DIFF_VALUE::FromString( summarizeRules( beforeRules ) );
        d.after = DIFF_VALUE::FromString( summarizeRules( afterRules ) );
        docDeltas.push_back( std::move( d ) );
    }

    // Footprint library table (fp-lib-table) lives in the project directory
    // (no extension).  Read content directly; the same content-comparison
    // pattern as custom DRC rules applies.
    auto readProjectFileNamed = []( const BOARD* aBoard, const std::string& aFileName ) -> wxString
    {
        if( !aBoard )
            return wxEmptyString;

        wxString boardPath = aBoard->GetFileName();

        if( boardPath.IsEmpty() )
            return wxEmptyString;

        wxFileName fn( boardPath );
        fn.SetFullName( wxString::FromUTF8( aFileName ) );

        if( !fn.FileExists() )
            return wxEmptyString;

        wxFile file( fn.GetFullPath() );

        if( !file.IsOpened() )
            return wxEmptyString;

        wxString contents;
        file.ReadAll( &contents );
        return contents;
    };

    auto summarizeTable = []( const wxString& aContents, const wxString& aLabel ) -> std::string
    {
        if( aContents.IsEmpty() )
            return wxString::Format( wxS( "(no %s)" ), aLabel ).ToStdString();

        std::size_t h = std::hash<std::string>{}( aContents.ToStdString() );
        return wxString::Format( wxS( "%zu byte(s) (hash %zx)" ), aContents.size(), h ).ToStdString();
    };

    const wxString beforeFp = readProjectFileNamed( m_before, FILEEXT::FootprintLibraryTableFileName );
    const wxString afterFp = readProjectFileNamed( m_after, FILEEXT::FootprintLibraryTableFileName );

    if( beforeFp != afterFp )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_FP_LIB_TABLE;
        d.before = DIFF_VALUE::FromString( summarizeTable( beforeFp, wxS( "fp-lib-table" ) ) );
        d.after = DIFF_VALUE::FromString( summarizeTable( afterFp, wxS( "fp-lib-table" ) ) );
        docDeltas.push_back( std::move( d ) );
    }

    const wxString beforeSym = readProjectFileNamed( m_before, FILEEXT::SymbolLibraryTableFileName );
    const wxString afterSym = readProjectFileNamed( m_after, FILEEXT::SymbolLibraryTableFileName );

    if( beforeSym != afterSym )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_SYM_LIB_TABLE;
        d.before = DIFF_VALUE::FromString( summarizeTable( beforeSym, wxS( "sym-lib-table" ) ) );
        d.after = DIFF_VALUE::FromString( summarizeTable( afterSym, wxS( "sym-lib-table" ) ) );
        docDeltas.push_back( std::move( d ) );
    }

    // Drawing sheet file path lives in PROJECT_FILE::m_BoardDrawingSheetFile.
    // Resolved to absolute on load; for diff purposes the stored path is
    // what serializes back, so compare those strings directly.
    auto boardDrawingSheet = []( const BOARD* aBoard ) -> wxString
    {
        if( !aBoard || !aBoard->GetProject() )
            return wxEmptyString;

        return aBoard->GetProject()->GetProjectFile().m_BoardDrawingSheetFile;
    };

    const wxString beforeSheet = boardDrawingSheet( m_before );
    const wxString afterSheet = boardDrawingSheet( m_after );

    if( beforeSheet != afterSheet )
    {
        PROPERTY_DELTA d;
        d.name = DOC_PROP_DRAWING_SHEET;
        d.before = DIFF_VALUE::FromString( beforeSheet );
        d.after = DIFF_VALUE::FromString( afterSheet );
        docDeltas.push_back( std::move( d ) );
    }

    if( !docDeltas.empty() )
    {
        ITEM_CHANGE c;
        c.id = KIID_PATH(); // empty path = document-scope sentinel
        c.typeName = wxS( "BOARD" );
        c.kind = CHANGE_KIND::MODIFIED;
        c.bbox = BOX2I(); // document-scoped, no spatial location
        c.properties = std::move( docDeltas );
        result.changes.push_back( std::move( c ) );
    }

    sortChanges( result.changes );

    if( m_options.progress )
        m_options.progress( 1.0 );

    return result;
}

} // namespace KICAD_DIFF
