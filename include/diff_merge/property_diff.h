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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_DIFF_PROPERTY_DIFF_H
#define KICAD_DIFF_PROPERTY_DIFF_H

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/kicad_merge_engine.h>
#include <diff_merge/property_value_converter.h>

#include <properties/property.h>
#include <properties/property_mgr.h>
#include <properties/wx_any_utils.h>
#include <inspectable.h>
#include <trace_helpers.h>

#include <wx/log.h>

#include <algorithm>
#include <cstddef>
#include <typeinfo>
#include <vector>


namespace KICAD_DIFF
{

/**
 * Enumerate the property deltas between two items of the same dynamic type.
 *
 * Walks the PROPERTY_MANAGER properties for the items' shared type, skipping
 * hidden and context-unavailable properties, and emits one PROPERTY_DELTA per
 * value that differs (compared via KiWxAnyEquals, converted via WxAnyToDiffValue).
 * Returns an empty vector when the items are null or of differing types. The
 * result is sorted alphabetically by property name for deterministic output.
 *
 * Header-only because it reaches PROPERTY_MANAGER / INSPECTABLE, which live in
 * the `common` static library rather than `kicommon`; instantiating it in the
 * caller's translation unit keeps the dependency where it belongs.
 *
 * Callers that need a property-resolution scope (e.g. a schematic sheet path)
 * must establish it around the call; this helper only reads the live items.
 */
inline std::vector<PROPERTY_DELTA> DiffItemProperties( const INSPECTABLE* aBefore,
                                                       const INSPECTABLE* aAfter )
{
    std::vector<PROPERTY_DELTA> deltas;

    if( !aBefore || !aAfter || typeid( *aBefore ) != typeid( *aAfter ) )
        return deltas;

    PROPERTY_MANAGER& pm    = PROPERTY_MANAGER::Instance();
    const auto&       props = pm.GetProperties( TYPE_HASH( *aBefore ) );

    // IsAvailableFor and Get take a non-const INSPECTABLE*; we treat both items
    // as read-only inspections.
    INSPECTABLE* mutBefore = const_cast<INSPECTABLE*>( aBefore );
    INSPECTABLE* mutAfter  = const_cast<INSPECTABLE*>( aAfter );

    for( PROPERTY_BASE* prop : props )
    {
        if( !prop || prop->IsHiddenFromPropertiesManager() )
            continue;

        // Skip properties that aren't applicable to this particular item state
        // (e.g. context-dependent visibility).
        if( !pm.IsAvailableFor( TYPE_HASH( *aBefore ), prop, mutBefore ) )
            continue;

        if( !pm.IsAvailableFor( TYPE_HASH( *aAfter ), prop, mutAfter ) )
            continue;

        wxAny beforeVal = mutBefore->Get( prop );
        wxAny afterVal  = mutAfter->Get( prop );

        if( KiWxAnyEquals( beforeVal, afterVal, prop ) )
            continue;

        DIFF_VALUE beforeDV = WxAnyToDiffValue( beforeVal, prop );
        DIFF_VALUE afterDV  = WxAnyToDiffValue( afterVal, prop );

        // Skip if conversion failed for both sides (unsupported type) — we can't
        // produce a meaningful delta. If only one side converted, we still emit
        // because the user benefits from knowing "something changed".
        if( beforeDV.GetType() == DIFF_VALUE::T::NONE && afterDV.GetType() == DIFF_VALUE::T::NONE )
            continue;

        PROPERTY_DELTA d;
        d.name   = prop->Name();
        d.before = beforeDV;
        d.after  = afterDV;
        deltas.push_back( std::move( d ) );
    }

    // Deterministic order: alphabetical by property name.
    std::sort( deltas.begin(), deltas.end(),
               []( const PROPERTY_DELTA& aL, const PROPERTY_DELTA& aR )
               {
                   return aL.name < aR.name;
               } );

    return deltas;
}


/**
 * List one item's properties as one-sided deltas for an added or removed item.
 * aAsAfter puts each value on the after side (added), else the before side
 * (removed). The empty side stays NONE and renders as "<none>".
 */
inline std::vector<PROPERTY_DELTA> ItemProperties( const INSPECTABLE* aItem, bool aAsAfter )
{
    std::vector<PROPERTY_DELTA> deltas;

    if( !aItem )
        return deltas;

    PROPERTY_MANAGER& pm = PROPERTY_MANAGER::Instance();
    const auto&       props = pm.GetProperties( TYPE_HASH( *aItem ) );

    INSPECTABLE* mutItem = const_cast<INSPECTABLE*>( aItem );

    for( PROPERTY_BASE* prop : props )
    {
        if( !prop || prop->IsHiddenFromPropertiesManager() )
            continue;

        if( !pm.IsAvailableFor( TYPE_HASH( *aItem ), prop, mutItem ) )
            continue;

        DIFF_VALUE dv = WxAnyToDiffValue( mutItem->Get( prop ), prop );

        if( dv.GetType() == DIFF_VALUE::T::NONE )
            continue;

        // An unset string property (e.g. an unassigned Component Class) carries
        // no information for an added or removed item, so skip it.
        if( dv.GetType() == DIFF_VALUE::T::STRING && dv.AsString().IsEmpty() )
            continue;

        PROPERTY_DELTA d;
        d.name = prop->Name();

        if( aAsAfter )
            d.after = dv;
        else
            d.before = dv;

        deltas.push_back( std::move( d ) );
    }

    std::sort( deltas.begin(), deltas.end(),
               []( const PROPERTY_DELTA& aL, const PROPERTY_DELTA& aR )
               {
                   return aL.name < aR.name;
               } );

    return deltas;
}


/// Applied/failed tallies from ApplyPropertyResolutions, folded into a caller's report.
struct PROPERTY_APPLY_COUNTS
{
    std::size_t applied = 0;
    std::size_t failed  = 0;
};


/**
 * Apply per-property merge resolutions to aTarget, sourcing OURS/THEIRS/ANCESTOR
 * values from the matching source item and CUSTOM values from the resolution
 * payload. Works on any INSPECTABLE so PCB and schematic appliers share it.
 *
 * Header-only for the same reason as DiffItemProperties: it reaches
 * PROPERTY_MANAGER / INSPECTABLE, which live in the `common` static library.
 *
 * A property is counted as failed when it is missing on the target type, is
 * read-only, names a source side that wasn't supplied, carries a CUSTOM value
 * of DIFF_VALUE::T::NONE, or its Set() call is rejected by the target.
 */
inline PROPERTY_APPLY_COUNTS ApplyPropertyResolutions(
        INSPECTABLE* aTarget, const std::vector<PROPERTY_RESOLUTION>& aProps,
        const INSPECTABLE* aOurs, const INSPECTABLE* aTheirs, const INSPECTABLE* aAncestor )
{
    PROPERTY_APPLY_COUNTS counts;

    if( !aTarget || aProps.empty() )
        return counts;

    PROPERTY_MANAGER& pm = PROPERTY_MANAGER::Instance();

    for( const PROPERTY_RESOLUTION& res : aProps )
    {
        PROPERTY_BASE* prop = pm.GetProperty( TYPE_HASH( *aTarget ), res.name );

        if( !prop )
        {
            wxLogTrace( traceDiffMerge, wxT( "applier: property '%s' not found on target type" ),
                        res.name );
            ++counts.failed;
            continue;
        }

        if( !prop->Writeable( aTarget ) )
        {
            wxLogTrace( traceDiffMerge, wxT( "applier: property '%s' is read-only on target" ),
                        res.name );
            ++counts.failed;
            continue;
        }

        wxAny value;

        if( res.kind == PROP_RES::CUSTOM )
        {
            // Custom values come from the resolution payload itself — the UI
            // flow stores the user-edited value in
            // PROPERTY_RESOLUTION.customValue. Convert DIFF_VALUE back into a
            // wxAny the property can consume; a T::NONE payload carries nothing
            // and counts as failed.
            if( !DiffValueToWxAny( res.customValue, value ) )
            {
                ++counts.failed;
                continue;
            }
        }
        else
        {
            const INSPECTABLE* source = nullptr;

            switch( res.kind )
            {
            case PROP_RES::OURS:     source = aOurs;     break;
            case PROP_RES::THEIRS:   source = aTheirs;   break;
            case PROP_RES::ANCESTOR: source = aAncestor; break;
            case PROP_RES::CUSTOM:                       break;
            }

            if( !source )
            {
                ++counts.failed;
                continue;
            }

            value = const_cast<INSPECTABLE*>( source )->Get( prop );
        }

        if( !aTarget->Set( prop, value ) )
        {
            wxLogTrace( traceDiffMerge, wxT( "applier: Set failed for property '%s'" ),
                        res.name );
            ++counts.failed;
            continue;
        }

        ++counts.applied;
    }

    return counts;
}

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_PROPERTY_DIFF_H
