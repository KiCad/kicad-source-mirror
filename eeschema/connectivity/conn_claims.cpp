/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conn_claims.h"

#include <algorithm>
#include <tuple>

namespace SCH_CONNECTIVITY
{
namespace
{
    void SortNames( std::vector<NAME_ID>& aNames, const SESSION_KEYS& aKeys )
    {
        std::ranges::sort( aNames, NAME_LESS{ &aKeys } );
        aNames.erase( std::ranges::unique( aNames ).begin(), aNames.end() );
    }

    PRIORITY LabelPriority( KICAD_T aType )
    {
        switch( aType )
        {
        case SCH_LABEL_T: return PRIORITY::LOCAL_LABEL;
        case SCH_GLOBAL_LABEL_T: return PRIORITY::GLOBAL;
        case SCH_HIER_LABEL_T: return PRIORITY::HIER_LABEL;
        case SCH_SHEET_PIN_T: return PRIORITY::SHEET_PIN;
        default: return PRIORITY::NONE;
        }
    }
} // namespace

bool CLAIM::operator==( const CLAIM& aOther ) const
{
    return std::tie( priority, depth, path, name, ncName, fullName, source, globalPowerParent, localPowerParent,
                     outputShape, hasPad, portInstance )
                   == std::tie( aOther.priority, aOther.depth, aOther.path, aOther.name, aOther.ncName, aOther.fullName,
                                aOther.source, aOther.globalPowerParent, aOther.localPowerParent, aOther.outputShape,
                                aOther.hasPad, aOther.portInstance )
           && ( schema == aOther.schema || ( schema && aOther.schema && *schema == *aOther.schema ) );
}

bool CLAIM_LESS::operator()( const CLAIM& aLeft, const CLAIM& aRight ) const
{
    const auto rank = []( const CLAIM& aClaim )
    {
        return std::tuple( aClaim.priority, -static_cast<int>( aClaim.depth ), aClaim.globalPowerParent,
                           aClaim.localPowerParent, aClaim.outputShape, !aClaim.hasPad );
    };

    if( const auto leftRank = rank( aLeft ), rightRank = rank( aRight ); leftRank != rightRank )
        return leftRank < rightRank;

    if( aLeft.fullName != aRight.fullName )
        return keys.NameLess( aRight.fullName, aLeft.fullName );

    return keys.Less( aRight.source, aLeft.source );
}

INSTANCE_CLAIMS PrepareInstanceClaims( const SCREEN_FACTS& aFacts, const INSTANCE_FACTS& aText,
                                       const INSTANCE_SCOPE& aScope, SESSION_KEYS& aKeys, BUS_PARSE_CACHE& aParses )
{
    INSTANCE_CLAIMS result;
    result.instance = aScope.instance;
    std::map<KIID, const ITEM_TEXT_FACT*> text;

    for( const ITEM_TEXT_FACT& item : aText.items )
        text.emplace( item.id, &item );

    const auto prepare = [&]( const ITEM_FACT& fact, const PIN_FACT* pin, const KIID& id )
    {
        const auto found = text.find( id );

        if( pin && found == text.end() )
            return;

        SOURCE_CLAIMS& input = result.items[id];
        input.type = fact.type;
        PRIORITY priority = LabelPriority( fact.type );

        if( pin )
        {
            priority = pin->globalPower  ? PRIORITY::GLOBAL_POWER_PIN
                       : pin->localPower ? PRIORITY::LOCAL_POWER_PIN
                                         : PRIORITY::PIN;
            input.pinGroup = fact.id;
            input.pinWitness = !pin->globalPowerParent && !pin->localPowerParent;
            input.invisiblePower = pin->globalPower && pin->invisible && pin->type == ELECTRICAL_PINTYPE::PT_POWER_IN
                                   && !pin->globalPowerParent && !pin->localPowerParent;
        }

        if( fact.type == SCH_SHEET_PIN_T )
        {
            if( auto child = aScope.children.find( fact.owner ); child != aScope.children.end() )
                input.portInstance = child->second;
        }
        else if( fact.type == SCH_HIER_LABEL_T )
            input.portInstance = aScope.instance;

        if( found == text.end() )
            return;

        const ITEM_TEXT_FACT& item = *found->second;

        for( const wxString& name : item.netclasses )
            input.netclasses.push_back( aKeys.InternName( name ) );

        if( !item.canDrive || priority == PRIORITY::NONE )
            return;

        CLAIM claim;
        claim.priority = priority;
        claim.source = { id, aScope.instance };
        const bool unscoped =
                priority == PRIORITY::GLOBAL || priority == PRIORITY::GLOBAL_POWER_PIN || priority == PRIORITY::PIN;
        claim.depth = unscoped ? 0 : aScope.depth;
        claim.path = aKeys.InternName( unscoped ? wxString() : aScope.path );
        claim.name = aKeys.InternName( item.name );
        claim.ncName = aKeys.InternName( item.ncName );
        claim.fullName = aKeys.InternName( aKeys.Name( claim.path ) + item.name );
        claim.globalPowerParent = pin && pin->globalPowerParent;
        claim.localPowerParent = pin && pin->localPowerParent;
        claim.outputShape = fact.type == SCH_SHEET_PIN_T && fact.outputShape;
        claim.hasPad = item.name.Contains( wxS( "-Pad" ) );
        claim.schema = aParses.Parse( item.name ).value.schema;

        if( fact.type == SCH_SHEET_PIN_T )
            claim.portInstance = input.portInstance;

        input.claim = std::move( claim );
    };

    for( const ITEM_FACT& fact : aFacts.items )
    {
        if( fact.type == SCH_PIN_T )
        {
            for( const PIN_FACT& pin : fact.pins )
                prepare( fact, &pin, pin.id );
        }
        else
        {
            prepare( fact, nullptr, fact.id );
        }
    }

    for( const INSTANCE_RULE_AREA_FACT& area : aText.ruleAreas )
    {
        std::vector<NAME_ID> classes;

        for( const wxString& name : area.netclasses )
            classes.push_back( aKeys.InternName( name ) );

        for( const KIID& id : area.containedItems )
        {
            if( auto found = result.items.find( id ); found != result.items.end() )
                found->second.netclasses.insert( found->second.netclasses.end(), classes.begin(), classes.end() );
        }
    }

    for( auto& [id, input] : result.items )
        SortNames( input.netclasses, aKeys );

    return result;
}

ISLAND_RECORD BuildIslandRecord( const RECORD_GEOMETRY& aIsland, const INSTANCE_CLAIMS& aInputs,
                                 const SESSION_KEYS& aKeys )
{
    ISLAND_RECORD result;
    result.items.assign( aIsland.items.begin(), aIsland.items.end() );
    std::set<KIID>    pinGroups;
    std::vector<KIID> witnesses;
    bool              hasVector = false;
    bool              hasGroup = false;
    bool              hasSignalLabel = false;
    bool              hasBusLabel = false;

    for( const KIID& id : aIsland.items )
    {
        const SOURCE_CLAIMS& input = aInputs.items.at( id );

        if( input.claim )
        {
            if( input.type == SCH_LABEL_T || input.type == SCH_GLOBAL_LABEL_T || input.type == SCH_HIER_LABEL_T
                || input.type == SCH_SHEET_PIN_T )
            {
                hasBusLabel |= bool( input.claim->schema );
                hasSignalLabel |= !input.claim->schema;
            }

            result.claims.push_back( *input.claim );

            if( input.claim->Strong() )
                result.atoms.strongNames.push_back( input.claim->name );

            if( input.claim->schema )
            {
                hasVector |= input.claim->schema->shape == BUS_SCHEMA::SHAPE::VECTOR;
                hasGroup |= input.claim->schema->shape == BUS_SCHEMA::SHAPE::GROUP;
            }
        }

        if( input.type == SCH_PIN_T )
        {
            result.atoms.hasSymbolPin = true;

            // Naming counts every pin object while ERC counts merged stacked pins once
            if( input.pinWitness )
            {
                pinGroups.insert( input.pinGroup );
                witnesses.push_back( id );
            }
        }
        else if( input.type == SCH_NO_CONNECT_T )
        {
            if( !result.atoms.noConnect || id < *result.atoms.noConnect )
                result.atoms.noConnect = id;
        }

        result.atoms.invisiblePowerWired |= input.invisiblePower && aIsland.hasWire;
        result.netclasses.insert( result.netclasses.end(), input.netclasses.begin(), input.netclasses.end() );
    }

    std::sort( result.claims.rbegin(), result.claims.rend(), CLAIM_LESS{ aKeys } );
    result.kind = ( result.claims.empty() ? aIsland.hasBusLine : bool( result.claims.front().schema ) ) ? KIND::BUNDLE
                                                                                                        : KIND::SIGNAL;
    result.atoms.kindConflict = ( aIsland.hasWire && ( aIsland.hasBusLine || result.kind == KIND::BUNDLE ) )
                                || ( aIsland.hasBusLine && result.kind == KIND::SIGNAL )
                                || ( ( aIsland.hasWire || hasSignalLabel ) && hasBusLabel )
                                || ( aIsland.hasBusLine && hasSignalLabel );
    result.atoms.mixedBusShapes = hasVector && hasGroup;
    result.atoms.busNoConnect = result.kind == KIND::BUNDLE && result.atoms.noConnect.has_value();
    result.atoms.pinCount = static_cast<uint32_t>( pinGroups.size() );
    std::ranges::sort( witnesses, std::less<KIID>() );
    std::copy_n( witnesses.begin(), std::min( witnesses.size(), result.atoms.pinWitnesses.size() ),
                 result.atoms.pinWitnesses.begin() );

    for( const CLAIM& claim : result.claims )
    {
        // Edges are sorted below, so emission order within a claim is irrelevant
        switch( claim.priority )
        {
        case PRIORITY::GLOBAL:
        case PRIORITY::GLOBAL_POWER_PIN:
            result.edges.push_back( { SCOPE::GLOBAL, 0, claim.name } );
            [[fallthrough]];
        case PRIORITY::LOCAL_LABEL:
        case PRIORITY::LOCAL_POWER_PIN:
            result.edges.push_back( { SCOPE::SHEET, aInputs.instance, claim.name } );
            break;
        case PRIORITY::HIER_LABEL:
            result.edges.push_back( { SCOPE::SHEET, aInputs.instance, claim.name } );
            [[fallthrough]];
        case PRIORITY::SHEET_PIN:
            if( const auto& instance = aInputs.items.at( claim.source.item ).portInstance )
                result.edges.push_back( { SCOPE::PORT, *instance, claim.name, result.kind } );

            break;
        default: break;
        }
    }

    std::sort( result.edges.begin(), result.edges.end(), KEY_LESS{ aKeys } );
    result.edges.erase( std::unique( result.edges.begin(), result.edges.end() ), result.edges.end() );
    SortNames( result.atoms.strongNames, aKeys );

    if( result.claims.empty() )
        result.netclasses.clear();
    else
        SortNames( result.netclasses, aKeys );

    return result;
}
} // namespace SCH_CONNECTIVITY
