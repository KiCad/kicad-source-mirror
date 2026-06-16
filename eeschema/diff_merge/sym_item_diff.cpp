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

#include <diff_merge/sym_item_diff.h>
#include <diff_merge/property_diff.h>

#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_item.h>
#include <sch_pin.h>

#include <map>
#include <set>


namespace KICAD_DIFF
{

namespace
{
    wxString graphicTypeName( const SCH_ITEM* aItem )
    {
        switch( aItem->Type() )
        {
        case SCH_SHAPE_T: return _( "Graphic" );
        case SCH_TEXT_T: return _( "Text" );
        case SCH_TEXTBOX_T: return _( "Text Box" );
        default: return _( "Item" );
        }
    }


    void diffKeyed( std::vector<SYM_ELEMENT>& aOut, const wxString& aTypeName,
                    const std::map<wxString, const EDA_ITEM*>& aBefore,
                    const std::map<wxString, const EDA_ITEM*>& aAfter )
    {
        std::set<wxString> keys;

        for( const auto& [key, item] : aBefore )
            keys.insert( key );

        for( const auto& [key, item] : aAfter )
            keys.insert( key );

        for( const wxString& key : keys )
        {
            auto            beforeIt = aBefore.find( key );
            auto            afterIt = aAfter.find( key );
            const EDA_ITEM* before = beforeIt != aBefore.end() ? beforeIt->second : nullptr;
            const EDA_ITEM* after = afterIt != aAfter.end() ? afterIt->second : nullptr;

            if( before && after )
            {
                std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( before, after );

                if( !deltas.empty() )
                    aOut.push_back( { after, aTypeName, key, CHANGE_KIND::MODIFIED, std::move( deltas ) } );
            }
            else if( before )
            {
                aOut.push_back( { before, aTypeName, key, CHANGE_KIND::REMOVED, ItemProperties( before, false ) } );
            }
            else
            {
                aOut.push_back( { after, aTypeName, key, CHANGE_KIND::ADDED, ItemProperties( after, true ) } );
            }
        }
    }


    std::map<wxString, const EDA_ITEM*> pinsByNumber( const LIB_SYMBOL* aSym )
    {
        std::map<wxString, const EDA_ITEM*> out;

        if( aSym )
        {
            for( SCH_PIN* pin : aSym->GetPins() )
            {
                if( pin )
                    out[pin->GetNumber()] = pin;
            }
        }

        return out;
    }


    std::map<wxString, const EDA_ITEM*> fieldsByName( const LIB_SYMBOL* aSym )
    {
        std::map<wxString, const EDA_ITEM*> out;

        if( aSym )
        {
            std::vector<SCH_FIELD*> fields;
            aSym->GetFields( fields );

            for( SCH_FIELD* field : fields )
            {
                if( field )
                    out[field->GetName()] = field;
            }
        }

        return out;
    }


    std::vector<const SCH_ITEM*> graphics( const LIB_SYMBOL* aSym )
    {
        std::vector<const SCH_ITEM*> out;

        if( aSym )
        {
            for( const SCH_ITEM& item : aSym->GetDrawItems() )
            {
                if( item.Type() != SCH_PIN_T && item.Type() != SCH_FIELD_T )
                    out.push_back( &item );
            }
        }

        return out;
    }
} // namespace


std::vector<SYM_ELEMENT> DiffSymbolElements( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter )
{
    std::vector<SYM_ELEMENT> out;

    diffKeyed( out, _( "Pin" ), pinsByNumber( aBefore ), pinsByNumber( aAfter ) );
    diffKeyed( out, _( "Field" ), fieldsByName( aBefore ), fieldsByName( aAfter ) );

    std::vector<const SCH_ITEM*> beforeGraphics = graphics( aBefore );
    std::vector<const SCH_ITEM*> afterGraphics = graphics( aAfter );
    std::set<const SCH_ITEM*>    matchedBefore;
    std::set<const SCH_ITEM*>    matchedAfter;

    // Pass 1: consume graphics the property diff sees as identical so unchanged
    // geometry never shows up.
    for( const SCH_ITEM* after : afterGraphics )
    {
        for( const SCH_ITEM* before : beforeGraphics )
        {
            if( matchedBefore.count( before ) || before->Type() != after->Type() )
                continue;

            if( DiffItemProperties( before, after ).empty() )
            {
                matchedBefore.insert( before );
                matchedAfter.insert( after );
                break;
            }
        }
    }

    // Pass 2: pair each leftover after-graphic with its nearest same-type
    // leftover before-graphic (a modified shape), else it is newly added.
    for( const SCH_ITEM* after : afterGraphics )
    {
        if( matchedAfter.count( after ) )
            continue;

        const SCH_ITEM* best = nullptr;
        double          bestDist = 0.0;

        for( const SCH_ITEM* before : beforeGraphics )
        {
            if( matchedBefore.count( before ) || before->Type() != after->Type() )
                continue;

            VECTOR2I delta = before->GetPosition() - after->GetPosition();
            double   dist = (double) delta.x * delta.x + (double) delta.y * delta.y;

            if( !best || dist < bestDist )
            {
                best = before;
                bestDist = dist;
            }
        }

        if( best )
        {
            matchedBefore.insert( best );

            std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( best, after );

            if( !deltas.empty() )
                out.push_back( { after, graphicTypeName( after ), wxEmptyString, CHANGE_KIND::MODIFIED,
                                 std::move( deltas ) } );
        }
        else
        {
            out.push_back( { after, graphicTypeName( after ), wxEmptyString, CHANGE_KIND::ADDED,
                             ItemProperties( after, true ) } );
        }
    }

    for( const SCH_ITEM* before : beforeGraphics )
    {
        if( !matchedBefore.count( before ) )
            out.push_back( { before, graphicTypeName( before ), wxEmptyString, CHANGE_KIND::REMOVED,
                             ItemProperties( before, false ) } );
    }

    return out;
}

} // namespace KICAD_DIFF
