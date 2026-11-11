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

#include <diff_merge/lib_element_diff.h>
#include <diff_merge/property_diff.h>

#include <eda_item.h>

#include <set>


namespace KICAD_DIFF
{

void DiffKeyedElements( std::vector<LIB_ELEMENT>& aOut, const wxString& aTypeName,
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


void DiffUnkeyedElements( std::vector<LIB_ELEMENT>& aOut, const std::vector<const EDA_ITEM*>& aBefore,
                          const std::vector<const EDA_ITEM*>&                            aAfter,
                          const std::function<wxString( const EDA_ITEM* )>&              aTypeName,
                          const std::function<bool( const EDA_ITEM*, const EDA_ITEM* )>& aSameGeometry )
{
    auto sameGeometry = [&]( const EDA_ITEM* aBeforeItem, const EDA_ITEM* aAfterItem )
    {
        return !aSameGeometry || aSameGeometry( aBeforeItem, aAfterItem );
    };

    std::set<const EDA_ITEM*> matchedBefore;
    std::set<const EDA_ITEM*> matchedAfter;

    // Consume identical elements first so unchanged geometry never shows up
    for( const EDA_ITEM* after : aAfter )
    {
        for( const EDA_ITEM* before : aBefore )
        {
            if( matchedBefore.count( before ) || before->Type() != after->Type() )
                continue;

            if( DiffItemProperties( before, after ).empty() && sameGeometry( before, after ) )
            {
                matchedBefore.insert( before );
                matchedAfter.insert( after );
                break;
            }
        }
    }

    // Pair each leftover with the nearest leftover of its type, else it is new
    for( const EDA_ITEM* after : aAfter )
    {
        if( matchedAfter.count( after ) )
            continue;

        const EDA_ITEM* best = nullptr;
        double          bestDist = 0.0;

        for( const EDA_ITEM* before : aBefore )
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

            if( !deltas.empty() || !sameGeometry( best, after ) )
            {
                aOut.push_back(
                        { after, aTypeName( after ), wxEmptyString, CHANGE_KIND::MODIFIED, std::move( deltas ) } );
            }
        }
        else
        {
            aOut.push_back(
                    { after, aTypeName( after ), wxEmptyString, CHANGE_KIND::ADDED, ItemProperties( after, true ) } );
        }
    }

    for( const EDA_ITEM* before : aBefore )
    {
        if( !matchedBefore.count( before ) )
        {
            aOut.push_back( { before, aTypeName( before ), wxEmptyString, CHANGE_KIND::REMOVED,
                              ItemProperties( before, false ) } );
        }
    }
}

} // namespace KICAD_DIFF
