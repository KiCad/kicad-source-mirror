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

#include <diff_merge/diff_tree_grouping.h>

#include <wx/intl.h>

#include <map>


namespace KICAD_DIFF
{
wxString ChangeKindLabel( CHANGE_KIND aKind )
{
    switch( aKind )
    {
    case CHANGE_KIND::ADDED: return _( "Added" );
    case CHANGE_KIND::REMOVED: return _( "Removed" );
    case CHANGE_KIND::MODIFIED: return _( "Modified" );
    case CHANGE_KIND::COLLISION: return _( "Collision" );
    case CHANGE_KIND::DUPLICATE_UUID: return _( "Duplicate ID" );
    }

    return wxEmptyString;
}


bool ChangeMatchesSearchFilter( const ITEM_CHANGE& aChange, const wxString& aLowercaseFilter )
{
    if( aLowercaseFilter.IsEmpty() )
        return true;

    if( aChange.typeName.Lower().Contains( aLowercaseFilter ) )
        return true;

    if( aChange.refdes.has_value() && aChange.refdes->Lower().Contains( aLowercaseFilter ) )
    {
        return true;
    }

    return false;
}


std::vector<CHANGE_TREE_GROUP> BuildChangeTreeGroups( const DOCUMENT_DIFF& aDiff, const wxString& aSearchFilter,
                                                      const std::array<bool, CATEGORY_COUNT>& aVisibleCategories )
{
    std::vector<CHANGE_TREE_GROUP> out;

    // Bucket changes by kind. The recursive walk flattens nested children
    // so a footprint-pad edit shows up as its own bucket entry even when
    // the parent footprint is also being shown.
    std::map<CHANGE_KIND, std::vector<CHANGE_TREE_ENTRY>>          byKind;
    std::map<std::pair<CHANGE_KIND, wxString>, const ITEM_CHANGE*> netEntries;

    const bool listChildren = aDiff.docType != wxS( "kicad_sym" );

    auto collect = [&byKind, &netEntries, listChildren]( auto& aSelf, const ITEM_CHANGE& aC ) -> void
    {
        if( IsRoutingNetChange( aC ) )
        {
            netEntries.try_emplace( std::make_pair( aC.kind, *aC.refdes ), &aC );
        }
        else
        {
            byKind[aC.kind].push_back( { &aC, ChangeDisplayLabel( aC ) } );
        }

        if( listChildren )
        {
            for( const ITEM_CHANGE& child : aC.children )
                aSelf( aSelf, child );
        }
    };

    for( const ITEM_CHANGE& c : aDiff.changes )
        collect( collect, c );

    for( const auto& [key, change] : netEntries )
    {
        const auto& [kind, netName] = key;
        byKind[kind].push_back( { change, wxS( "NET [" ) + netName + wxS( "]" ) } );
    }

    const wxString lowerFilter = aSearchFilter.Lower();

    for( const auto& [kind, items] : byKind )
    {
        const CATEGORY cat = CategoryFor( kind );

        if( !aVisibleCategories[static_cast<std::size_t>( cat )] )
            continue;

        std::vector<CHANGE_TREE_ENTRY> visibleEntries;
        visibleEntries.reserve( items.size() );

        for( const CHANGE_TREE_ENTRY& item : items )
        {
            if( !ChangeMatchesSearchFilter( *item.change, lowerFilter )
                && !item.itemLabel.Lower().Contains( lowerFilter ) )
            {
                continue;
            }

            visibleEntries.push_back( item );
        }

        if( visibleEntries.empty() )
            continue;

        wxString groupLabel;

        if( lowerFilter.IsEmpty() || visibleEntries.size() == items.size() )
            groupLabel = wxString::Format( wxS( "%s (%zu)" ), ChangeKindLabel( kind ), visibleEntries.size() );
        else
            groupLabel = wxString::Format( wxS( "%s (%zu/%zu)" ), ChangeKindLabel( kind ), visibleEntries.size(),
                                           items.size() );

        out.push_back( CHANGE_TREE_GROUP{ kind, std::move( groupLabel ), std::move( visibleEntries ) } );
    }

    return out;
}

} // namespace KICAD_DIFF
