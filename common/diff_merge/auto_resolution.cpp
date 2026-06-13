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

#include <diff_merge/auto_resolution.h>

#include <nlohmann/json.hpp>

#include <wx/intl.h>

#include <set>


namespace KICAD_DIFF
{

AUTO_RESOLUTION_PARSE_RESULT ParseAutoResolutionJson( const std::string& aJsonContent )
{
    AUTO_RESOLUTION_PARSE_RESULT out;

    nlohmann::json j;

    try
    {
        j = nlohmann::json::parse( aJsonContent );
    }
    catch( const nlohmann::json::exception& e )
    {
        out.status       = AUTO_RESOLUTION_STATUS::INVALID_JSON;
        out.errorContext = wxString::FromUTF8( e.what() );
        return out;
    }

    if( !j.is_object() )
    {
        out.status = AUTO_RESOLUTION_STATUS::NOT_OBJECT;
        return out;
    }

    for( const auto& [key, val] : j.items() )
    {
        const wxString keyStr = wxString::FromUTF8( key );

        if( !val.is_string() )
        {
            out.status       = AUTO_RESOLUTION_STATUS::BAD_VALUE;
            out.errorContext = keyStr;
            out.resolutions.clear();   // contract: populated only on OK
            return out;
        }

        ITEM_RES kind = ITEM_RES::KEEP;

        try
        {
            kind = ItemResFromString( val.get<std::string>() );
        }
        catch( const std::invalid_argument& )
        {
            out.status       = AUTO_RESOLUTION_STATUS::UNKNOWN_KIND;
            out.errorContext = keyStr;
            out.resolutions.clear();
            return out;
        }

        // Engine-internal kinds (KEEP / DELETE / MERGE_PROPS) make no sense
        // as a scripted resolution — only the TAKE_* family represents a
        // user-equivalent choice of side.
        if( kind != ITEM_RES::TAKE_OURS && kind != ITEM_RES::TAKE_THEIRS
            && kind != ITEM_RES::TAKE_ANCESTOR )
        {
            out.status       = AUTO_RESOLUTION_STATUS::ENGINE_INTERNAL_KIND;
            out.errorContext = keyStr;
            out.resolutions.clear();
            return out;
        }

        out.resolutions.emplace( keyStr, kind );
    }

    out.status = AUTO_RESOLUTION_STATUS::OK;
    return out;
}


APPLY_AUTO_RESOLUTIONS_RESULT
ApplyAutoResolutions( MERGE_PLAN&                          aPlan,
                      const std::vector<std::size_t>&      aConflictActionIndices,
                      const std::map<wxString, ITEM_RES>&  aResolutions )
{
    APPLY_AUTO_RESOLUTIONS_RESULT out;

    if( aConflictActionIndices.empty() )
    {
        out.status = APPLY_AUTO_RESOLUTIONS_STATUS::NO_CONFLICTS;
        return out;
    }

    // Stage the writes so a partial-coverage failure leaves the plan
    // untouched — the dialog promises that on PARTIAL the caller can
    // still observe the original plan, e.g. for fallback reporting.
    std::vector<std::pair<std::size_t, ITEM_RES>> staged;
    staged.reserve( aConflictActionIndices.size() );

    for( std::size_t actionIdx : aConflictActionIndices )
    {
        if( actionIdx >= aPlan.actions.size() )
        {
            // Stale/bad index — treat as PARTIAL so the caller bails out
            // rather than silently dropping a conflict.
            out.status = APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL;
            return out;
        }

        const ITEM_RESOLUTION& action = aPlan.actions[actionIdx];

        auto it = aResolutions.find( action.id.AsString() );

        if( it == aResolutions.end() )
        {
            out.status         = APPLY_AUTO_RESOLUTIONS_STATUS::PARTIAL;
            out.firstMissingId = action.id;
            return out;
        }

        staged.emplace_back( actionIdx, it->second );
    }

    for( const auto& [actionIdx, kind] : staged )
        aPlan.actions[actionIdx].kind = kind;

    aPlan.unresolved.clear();
    out.status       = APPLY_AUTO_RESOLUTIONS_STATUS::COMPLETE;
    out.appliedCount = staged.size();
    return out;
}


std::vector<CONFLICT_LIST_ENTRY> BuildConflictList( const MERGE_PLAN& aPlan )
{
    std::vector<CONFLICT_LIST_ENTRY> out;

    std::set<KIID_PATH> unresolvedSet( aPlan.unresolved.begin(), aPlan.unresolved.end() );

    for( std::size_t i = 0; i < aPlan.actions.size(); ++i )
    {
        const ITEM_RESOLUTION& action = aPlan.actions[i];

        if( !unresolvedSet.count( action.id ) )
            continue;

        wxString label = action.id.AsString();

        // Long KIID_PATH strings (multi-segment, on-disk-style) overflow the
        // dialog's list control — keep the right side of the path because
        // that's where the disambiguating segment lives.
        if( label.Length() > 40 )
            label = wxS( "…" ) + label.Right( 39 );

        out.push_back( { i, label, action.id } );
    }

    return out;
}


std::vector<KIID_PATH>
CollectUnresolvedConflicts( const MERGE_PLAN&                aPlan,
                            const std::vector<std::size_t>&  aConflictActionIndices )
{
    std::vector<KIID_PATH> out;

    for( std::size_t actionIdx : aConflictActionIndices )
    {
        // Bounds-check the index — the function is public and callers can
        // pass arbitrary indices. The dialog's own conflict-action list is
        // built from plan.actions so this never fires for it, but a future
        // mergetool consumer could feed in something stale.
        if( actionIdx >= aPlan.actions.size() )
            continue;

        const ITEM_RESOLUTION& a = aPlan.actions[actionIdx];

        if( a.kind != ITEM_RES::TAKE_OURS && a.kind != ITEM_RES::TAKE_THEIRS
            && a.kind != ITEM_RES::TAKE_ANCESTOR )
        {
            out.push_back( a.id );
        }
    }

    return out;
}


std::optional<BOX2I>
ResolveConflictBBox( const KIID_PATH&                  aId,
                     const std::map<KIID_PATH, BOX2I>& aPrimary,
                     const std::map<KIID_PATH, BOX2I>& aOurs,
                     const std::map<KIID_PATH, BOX2I>& aTheirs,
                     const std::map<KIID_PATH, BOX2I>& aAncestor )
{
    auto find = []( const std::map<KIID_PATH, BOX2I>& aMap,
                    const KIID_PATH&                  aLookup ) -> std::optional<BOX2I>
    {
        auto it = aMap.find( aLookup );

        if( it == aMap.end() )
            return std::nullopt;

        // Treat a zero-size bbox (regardless of origin) as absence so a
        // side with a placeholder entry falls through to the next side.
        // A 1xN sliver is still usable — the highlight rectangle just
        // gets narrow.
        if( it->second.GetWidth() <= 0 && it->second.GetHeight() <= 0 )
            return std::nullopt;

        return it->second;
    };

    if( auto b = find( aPrimary,  aId ); b.has_value() ) return b;
    if( auto b = find( aOurs,     aId ); b.has_value() ) return b;
    if( auto b = find( aTheirs,   aId ); b.has_value() ) return b;
    if( auto b = find( aAncestor, aId ); b.has_value() ) return b;

    return std::nullopt;
}


wxString BuildConflictDetailText( const ITEM_RESOLUTION& aResolution )
{
    wxString text;
    text << _( "Item id: " ) << aResolution.id.AsString() << wxS( "\n" );
    text << _( "Current resolution: " );

    switch( aResolution.kind )
    {
    case ITEM_RES::TAKE_OURS:     text << _( "Take ours" );     break;
    case ITEM_RES::TAKE_THEIRS:   text << _( "Take theirs" );   break;
    case ITEM_RES::TAKE_ANCESTOR: text << _( "Take ancestor" ); break;
    case ITEM_RES::MERGE_PROPS:   text << _( "Property-level merge" ); break;
    case ITEM_RES::DELETE_ITEM: text << _( "Delete" ); break;
    case ITEM_RES::KEEP:          text << _( "Keep (default)" ); break;
    }

    text << wxS( "\n\n" );
    text << wxString::Format( _( "%zu property edit(s) in this resolution.\n" ),
                              aResolution.props.size() );

    return text;
}

} // namespace KICAD_DIFF
