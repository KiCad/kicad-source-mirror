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

#include <diff_merge/kicad_merge_engine.h>

#include <json_conversions.h>

#include <nlohmann/json.hpp>

#include <map>
#include <set>
#include <stdexcept>


namespace KICAD_DIFF
{

const char* PropResToString( PROP_RES aRes )
{
    switch( aRes )
    {
    case PROP_RES::OURS:     return "ours";
    case PROP_RES::THEIRS:   return "theirs";
    case PROP_RES::ANCESTOR: return "ancestor";
    case PROP_RES::CUSTOM:   return "custom";
    }

    return "ours";
}


PROP_RES PropResFromString( const std::string& aStr )
{
    if( aStr == "ours" )     return PROP_RES::OURS;
    if( aStr == "theirs" )   return PROP_RES::THEIRS;
    if( aStr == "ancestor" ) return PROP_RES::ANCESTOR;
    if( aStr == "custom" )   return PROP_RES::CUSTOM;

    throw std::invalid_argument( "Unknown PROP_RES: " + aStr );
}


const char* ItemResToString( ITEM_RES aRes )
{
    switch( aRes )
    {
    case ITEM_RES::TAKE_OURS:     return "take_ours";
    case ITEM_RES::TAKE_THEIRS:   return "take_theirs";
    case ITEM_RES::TAKE_ANCESTOR: return "take_ancestor";
    case ITEM_RES::MERGE_PROPS:   return "merge_props";
    case ITEM_RES::DELETE_ITEM: return "delete";
    case ITEM_RES::KEEP:          return "keep";
    }

    return "keep";
}


ITEM_RES ItemResFromString( const std::string& aStr )
{
    if( aStr == "take_ours" )     return ITEM_RES::TAKE_OURS;
    if( aStr == "take_theirs" )   return ITEM_RES::TAKE_THEIRS;
    if( aStr == "take_ancestor" ) return ITEM_RES::TAKE_ANCESTOR;
    if( aStr == "merge_props" )   return ITEM_RES::MERGE_PROPS;
    if( aStr == "delete" )
        return ITEM_RES::DELETE_ITEM;
    if( aStr == "keep" )          return ITEM_RES::KEEP;

    throw std::invalid_argument( "Unknown ITEM_RES: " + aStr );
}


namespace
{

void indexChangeTree( const ITEM_CHANGE&                       aChange,
                      std::map<KIID_PATH, const ITEM_CHANGE*>& aIndex )
{
    aIndex[aChange.id] = &aChange;

    for( const ITEM_CHANGE& child : aChange.children )
        indexChangeTree( child, aIndex );
}

} // namespace


std::map<KIID_PATH, const ITEM_CHANGE*> IndexChangesByKiid( const DOCUMENT_DIFF& aDiff )
{
    std::map<KIID_PATH, const ITEM_CHANGE*> index;

    for( const ITEM_CHANGE& c : aDiff.changes )
        indexChangeTree( c, index );

    return index;
}


std::map<wxString, const PROPERTY_DELTA*> IndexPropertiesByName( const ITEM_CHANGE& aChange )
{
    std::map<wxString, const PROPERTY_DELTA*> index;

    for( const PROPERTY_DELTA& d : aChange.properties )
        index[d.name] = &d;

    return index;
}


PROPERTY_RESOLUTION_OUTCOME ResolvePropertyConflict( const PROPERTY_DELTA* aOurs,
                                                    const PROPERTY_DELTA* aTheirs,
                                                    const KICAD_MERGE_ENGINE::OPTIONS& aOptions )
{
    PROPERTY_RESOLUTION_OUTCOME out;

    if( aOurs && !aTheirs )
    {
        out.kind = PROP_RES::OURS;
        return out;
    }

    if( !aOurs && aTheirs )
    {
        out.kind = PROP_RES::THEIRS;
        return out;
    }

    // Both sides null is unreachable from PlanMerge (allNames is the union of
    // present keys), but defend the deref-of-aOurs/aTheirs below: assert in
    // debug builds so a broken caller fails QA via wxAssertThrower, and
    // return a deterministic OURS in release builds so production doesn't
    // crash on a contract violation.
    wxASSERT( aOurs || aTheirs );

    if( !aOurs && !aTheirs )
    {
        out.kind = PROP_RES::OURS;
        return out;
    }

    // Branch order is load-bearing. autoResolveEqualValues runs FIRST so
    // converging-edits (both sides reach the same end value) auto-merge under
    // either option flag, even when ours' delta is a no-op against a stale
    // baseline. The two no-op detectors that follow are gated on matching
    // before values so a stale-baseline no-op doesn't silently override a
    // real edit on the other side.

    if( aOptions.autoResolveEqualValues && aOurs->after == aTheirs->after )
    {
        out.kind = PROP_RES::OURS;   // same end value; pick either
        return out;
    }

    // No-op detection requires both sides to share the same `before` value
    // (the 3-way merge contract). A stale baseline on either side disables
    // the auto-merge and falls through to the conflict path.
    const bool baselinesMatch = aOurs->before == aTheirs->before;

    if( aOptions.preferAutoMerge && baselinesMatch && aOurs->after == aOurs->before )
    {
        out.kind = PROP_RES::THEIRS;   // ours didn't really change
        return out;
    }

    if( aOptions.preferAutoMerge && baselinesMatch && aTheirs->after == aTheirs->before )
    {
        out.kind = PROP_RES::OURS;   // theirs didn't really change
        return out;
    }

    // True conflict — default to OURS but flag for user.
    out.kind = PROP_RES::OURS;
    out.isUnresolved = true;
    return out;
}


bool PROPERTY_RESOLUTION::operator==( const PROPERTY_RESOLUTION& aOther ) const
{
    return name == aOther.name && kind == aOther.kind && customValue == aOther.customValue;
}


nlohmann::json PROPERTY_RESOLUTION::ToJson() const
{
    nlohmann::json j;
    j["name"]  = name;
    j["kind"]  = PropResToString( kind );

    if( kind == PROP_RES::CUSTOM )
        j["custom"] = customValue.ToJson();

    return j;
}


PROPERTY_RESOLUTION PROPERTY_RESOLUTION::FromJson( const nlohmann::json& aJson )
{
    PROPERTY_RESOLUTION r;
    r.name = aJson.at( "name" ).get<wxString>();
    r.kind = PropResFromString( aJson.at( "kind" ).get<std::string>() );

    if( aJson.contains( "custom" ) )
        r.customValue = DIFF_VALUE::FromJson( aJson.at( "custom" ) );

    return r;
}


bool ITEM_RESOLUTION::operator==( const ITEM_RESOLUTION& aOther ) const
{
    return id == aOther.id && kind == aOther.kind && props == aOther.props;
}


nlohmann::json ITEM_RESOLUTION::ToJson() const
{
    nlohmann::json j;
    j["id"]   = id.AsString();
    j["kind"] = ItemResToString( kind );

    nlohmann::json arr = nlohmann::json::array();

    for( const PROPERTY_RESOLUTION& p : props )
        arr.push_back( p.ToJson() );

    j["props"] = std::move( arr );
    return j;
}


ITEM_RESOLUTION ITEM_RESOLUTION::FromJson( const nlohmann::json& aJson )
{
    ITEM_RESOLUTION r;
    r.id   = KIID_PATH( aJson.at( "id" ).get<wxString>() );
    r.kind = ItemResFromString( aJson.at( "kind" ).get<std::string>() );

    for( const auto& p : aJson.at( "props" ) )
        r.props.push_back( PROPERTY_RESOLUTION::FromJson( p ) );

    return r;
}


nlohmann::json MERGE_PLAN::ToJson() const
{
    nlohmann::json j;

    nlohmann::json arr = nlohmann::json::array();

    for( const ITEM_RESOLUTION& a : actions )
        arr.push_back( a.ToJson() );

    j["actions"] = std::move( arr );

    nlohmann::json unr = nlohmann::json::array();

    for( const KIID_PATH& u : unresolved )
        unr.push_back( u.AsString() );

    j["unresolved"]                  = std::move( unr );
    j["requiresZoneRefill"]          = requiresZoneRefill;
    j["requiresConnectivityRebuild"] = requiresConnectivityRebuild;
    return j;
}


MERGE_PLAN MERGE_PLAN::FromJson( const nlohmann::json& aJson )
{
    MERGE_PLAN p;

    for( const auto& a : aJson.at( "actions" ) )
        p.actions.push_back( ITEM_RESOLUTION::FromJson( a ) );

    for( const auto& u : aJson.at( "unresolved" ) )
        p.unresolved.push_back( KIID_PATH( u.get<wxString>() ) );

    p.requiresZoneRefill          = aJson.value( "requiresZoneRefill", false );
    p.requiresConnectivityRebuild = aJson.value( "requiresConnectivityRebuild", false );
    return p;
}


MERGE_PLAN KICAD_MERGE_ENGINE::Plan( const DOCUMENT_DIFF& aAncestorOurs,
                                     const DOCUMENT_DIFF& aAncestorTheirs ) const
{
    MERGE_PLAN plan;

    auto ourIndex     = IndexChangesByKiid( aAncestorOurs );
    auto theirIndex   = IndexChangesByKiid( aAncestorTheirs );

    std::set<KIID_PATH> processed;

    auto noteSideEffects = [&]( const ITEM_CHANGE& aChange )
    {
        if( ChangeInvalidatesZone( aChange ) )
            plan.requiresZoneRefill = true;

        if( ChangeRequiresConnectivityRebuild( aChange ) )
            plan.requiresConnectivityRebuild = true;
    };

    // Pass 1: items touched on both sides.
    for( const auto& [id, ourChange] : ourIndex )
    {
        auto it = theirIndex.find( id );

        if( it == theirIndex.end() )
            continue;   // only ours touched it — Pass 2 handles those

        const ITEM_CHANGE* theirChange = it->second;
        processed.insert( id );
        noteSideEffects( *ourChange );
        noteSideEffects( *theirChange );

        // Both deleted: take the deletion.
        if( ourChange->kind == CHANGE_KIND::REMOVED
            && theirChange->kind == CHANGE_KIND::REMOVED )
        {
            ITEM_RESOLUTION r;
            r.id   = id;
            r.kind = ITEM_RES::DELETE_ITEM;
            plan.actions.push_back( std::move( r ) );
            continue;
        }

        // One side deleted, the other modified: conflict (data loss risk).
        if( ourChange->kind == CHANGE_KIND::REMOVED
            && theirChange->kind == CHANGE_KIND::MODIFIED )
        {
            plan.unresolved.push_back( id );
            ITEM_RESOLUTION r;
            r.id   = id;
            r.kind = ITEM_RES::KEEP;   // safe default; user must resolve
            plan.actions.push_back( std::move( r ) );
            continue;
        }

        if( ourChange->kind == CHANGE_KIND::MODIFIED
            && theirChange->kind == CHANGE_KIND::REMOVED )
        {
            plan.unresolved.push_back( id );
            ITEM_RESOLUTION r;
            r.id   = id;
            r.kind = ITEM_RES::KEEP;
            plan.actions.push_back( std::move( r ) );
            continue;
        }

        // Both added with the same id: explicit collision. Keep the default
        // as KEEP so a downstream `kind == TAKE_*` check still reflects an
        // explicit user choice; applier's KEEP path falls back to
        // ancestor-or-ours.
        if( ourChange->kind == CHANGE_KIND::ADDED
            && theirChange->kind == CHANGE_KIND::ADDED )
        {
            plan.unresolved.push_back( id );
            ITEM_RESOLUTION r;
            r.id   = id;
            r.kind = ITEM_RES::KEEP;
            plan.actions.push_back( std::move( r ) );
            continue;
        }

        // Both modified: property-level merge.
        if( ourChange->kind == CHANGE_KIND::MODIFIED
            && theirChange->kind == CHANGE_KIND::MODIFIED )
        {
            // preferAutoMerge=false: every item touched on both sides
            // conflicts, even if the property edits are orthogonal.
            if( !m_options.preferAutoMerge )
            {
                plan.unresolved.push_back( id );
                ITEM_RESOLUTION r;
                r.id   = id;
                r.kind = ITEM_RES::KEEP;
                plan.actions.push_back( std::move( r ) );
                continue;
            }

            // A coarse MODIFIED record with no property deltas and no child
            // edits means the change was caught by semantic operator== alone
            // — something PROPERTY_MANAGER cannot serialize (e.g., raw cache
            // state). Resolving as MERGE_PROPS with zero props would silently
            // lose that change; flag a conflict instead.
            if( ourChange->properties.empty() && theirChange->properties.empty()
                && ourChange->children.empty() && theirChange->children.empty() )
            {
                plan.unresolved.push_back( id );
                ITEM_RESOLUTION r;
                r.id   = id;
                r.kind = ITEM_RES::KEEP;
                plan.actions.push_back( std::move( r ) );
                continue;
            }

            ITEM_RESOLUTION r;
            r.id   = id;
            r.kind = ITEM_RES::MERGE_PROPS;

            auto ourProps   = IndexPropertiesByName( *ourChange );
            auto theirProps = IndexPropertiesByName( *theirChange );

            // Union of property names touched on either side.
            std::set<wxString> allNames;

            for( const auto& [n, _] : ourProps )   allNames.insert( n );
            for( const auto& [n, _] : theirProps ) allNames.insert( n );

            bool hasUnresolvedProp = false;

            for( const wxString& name : allNames )
            {
                auto ourIt   = ourProps.find( name );
                auto theirIt = theirProps.find( name );

                const PROPERTY_DELTA* ourDelta   = ourIt   != ourProps.end()   ? ourIt->second   : nullptr;
                const PROPERTY_DELTA* theirDelta = theirIt != theirProps.end() ? theirIt->second : nullptr;

                const auto outcome = ResolvePropertyConflict( ourDelta, theirDelta, m_options );

                PROPERTY_RESOLUTION p;
                p.name = name;
                p.kind = outcome.kind;
                r.props.push_back( std::move( p ) );

                if( outcome.isUnresolved )
                    hasUnresolvedProp = true;
            }

            if( hasUnresolvedProp )
                plan.unresolved.push_back( id );

            plan.actions.push_back( std::move( r ) );
            continue;
        }

        // Any remaining combination is conservative-conflict.
        plan.unresolved.push_back( id );
        ITEM_RESOLUTION r;
        r.id   = id;
        r.kind = ITEM_RES::KEEP;
        plan.actions.push_back( std::move( r ) );
    }

    // Pass 2: items touched only on one side — auto-take that side.
    for( const auto& [id, ourChange] : ourIndex )
    {
        if( processed.count( id ) )
            continue;

        noteSideEffects( *ourChange );

        ITEM_RESOLUTION r;
        r.id = id;

        switch( ourChange->kind )
        {
        case CHANGE_KIND::ADDED:
        case CHANGE_KIND::MODIFIED:
            r.kind = ITEM_RES::TAKE_OURS;
            break;

        case CHANGE_KIND::REMOVED: r.kind = ITEM_RES::DELETE_ITEM; break;

        case CHANGE_KIND::DUPLICATE_UUID:
        case CHANGE_KIND::COLLISION:
            r.kind = ITEM_RES::KEEP;
            plan.unresolved.push_back( id );
            break;
        }

        plan.actions.push_back( std::move( r ) );
    }

    for( const auto& [id, theirChange] : theirIndex )
    {
        if( processed.count( id ) || ourIndex.count( id ) )
            continue;

        noteSideEffects( *theirChange );

        ITEM_RESOLUTION r;
        r.id = id;

        switch( theirChange->kind )
        {
        case CHANGE_KIND::ADDED:
        case CHANGE_KIND::MODIFIED:
            r.kind = ITEM_RES::TAKE_THEIRS;
            break;

        case CHANGE_KIND::REMOVED: r.kind = ITEM_RES::DELETE_ITEM; break;

        case CHANGE_KIND::DUPLICATE_UUID:
        case CHANGE_KIND::COLLISION:
            r.kind = ITEM_RES::KEEP;
            plan.unresolved.push_back( id );
            break;
        }

        plan.actions.push_back( std::move( r ) );
    }

    return plan;
}

} // namespace KICAD_DIFF
