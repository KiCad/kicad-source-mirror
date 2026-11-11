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

#include "sym_lib_differ.h"

#include <diff_merge/property_diff.h>
#include <diff_merge/sym_item_diff.h>
#include <lib_symbol.h>
#include <sch_item.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>

#include <wx/arrstr.h>
#include <wx/filename.h>

#include <algorithm>


namespace KICAD_DIFF
{

SYM_LIB_DIFFER::SYM_LIB_DIFFER( const SYMBOL_MAP& aBefore, const SYMBOL_MAP& aAfter, const wxString& aPath ) :
        m_before( aBefore ),
        m_after( aAfter ),
        m_path( aPath )
{
}


SYM_LIB_DIFFER::~SYM_LIB_DIFFER() = default;


std::pair<std::vector<std::unique_ptr<LIB_SYMBOL>>, SYM_LIB_DIFFER::SYMBOL_MAP>
SYM_LIB_DIFFER::LoadLibrary( const wxString& aPath )
{
    std::vector<std::unique_ptr<LIB_SYMBOL>> owners;
    SYMBOL_MAP                               map;

    // The s-expression library plugin asserts on relative paths; absolute the
    // input so CLI callers can pass `./` paths without crashing.
    wxFileName absPath( aPath );
    absPath.MakeAbsolute();
    wxString absStr = absPath.GetFullPath();

    SCH_IO_KICAD_SEXPR       io;
    std::vector<LIB_SYMBOL*> rawList;

    // EnumerateSymbolLib can throw IO_ERROR / std::exception on a corrupt or
    // unreadable file. Let the exception escape so callers can distinguish a
    // load failure from a deliberately-empty library; swallowing here was
    // misreported as a valid empty diff downstream.
    io.EnumerateSymbolLib( rawList, absStr, nullptr );

    // EnumerateSymbolLib returns pointers owned by the IO plugin's cache,
    // which is destroyed when the SCH_IO_KICAD_SEXPR goes out of scope. Clone
    // each symbol so we own a stable copy.
    for( LIB_SYMBOL* sym : rawList )
    {
        if( !sym )
            continue;

        std::unique_ptr<LIB_SYMBOL> owner( new LIB_SYMBOL( *sym ) );
        map[owner->GetName()] = owner.get();
        owners.push_back( std::move( owner ) );
    }

    // The clones still point their parent at the IO cache, which dies with the
    // plugin below. Re-link derived symbols to the copies we own so Flatten can
    // still pull in the inherited pins and fields.
    std::map<wxString, LIB_SYMBOL*> byName;

    for( const std::unique_ptr<LIB_SYMBOL>& owner : owners )
        byName[owner->GetName()] = owner.get();

    for( const std::unique_ptr<LIB_SYMBOL>& owner : owners )
    {
        if( owner->GetParentName().IsEmpty() )
            continue;

        auto it = byName.find( owner->GetParentName() );

        if( it != byName.end() )
            owner->SetParent( it->second );
    }

    return { std::move( owners ), std::move( map ) };
}


namespace
{
    // Compare also sees symbol state that no property or element carries
    constexpr int SYMBOL_COMPARE_FLAGS = ~( SCH_ITEM::COMPARE_FLAGS::UUID | SCH_ITEM::COMPARE_FLAGS::IDENTITY );


    std::vector<PROPERTY_DELTA> symbolDeltas( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter )
    {
        std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( aBefore, aAfter );
        wxString                    before = wxJoin( aBefore->GetFPFilters(), ' ' );
        wxString                    after = wxJoin( aAfter->GetFPFilters(), ' ' );

        if( before != after )
        {
            deltas.push_back(
                    { wxS( "Footprint Filters" ), DIFF_VALUE::FromString( before ), DIFF_VALUE::FromString( after ) } );
        }

        // Unit and body style names are not properties, so a merge could drop one unnoticed
        auto unitNames = []( const LIB_SYMBOL* aSymbol )
        {
            wxArrayString names;

            for( const auto& [unit, name] : aSymbol->GetUnitDisplayNames() )
                names.Add( wxString::Format( wxS( "%d: %s" ), unit, name ) );

            return wxJoin( names, ',' );
        };

        before = unitNames( aBefore );
        after = unitNames( aAfter );

        if( before != after )
        {
            deltas.push_back(
                    { wxS( "Unit Names" ), DIFF_VALUE::FromString( before ), DIFF_VALUE::FromString( after ) } );
        }

        auto bodyStyleNames = []( const LIB_SYMBOL* aSymbol )
        {
            wxArrayString names;

            for( const wxString& name : aSymbol->GetBodyStyleNames() )
                names.Add( name );

            return wxJoin( names, ',' );
        };

        before = bodyStyleNames( aBefore );
        after = bodyStyleNames( aAfter );

        if( before != after )
        {
            deltas.push_back( { wxS( "Body Style Names" ), DIFF_VALUE::FromString( before ),
                                DIFF_VALUE::FromString( after ) } );
        }

        return deltas;
    }


    bool hasSymbolDifferences( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter )
    {
        std::unique_ptr<LIB_SYMBOL> before = aBefore->Flatten();
        std::unique_ptr<LIB_SYMBOL> after = aAfter->Flatten();

        return before->Compare( *after, SYMBOL_COMPARE_FLAGS ) != 0
               || !symbolDeltas( before.get(), after.get() ).empty()
               || !DiffSymbolElements( before.get(), after.get() ).empty();
    }
} // namespace


DOCUMENT_DIFF SYM_LIB_DIFFER::Diff()
{
    DOCUMENT_DIFF result = DiffLibraryByName(
            m_before, m_after, m_path, wxS( "kicad_sym" ), wxS( "LIB_SYMBOL" ), m_options,
            []( const LIB_SYMBOL* aSym )
            {
                return aSym->GetUnitBoundingBox( 0, 0 );
            },
            []( const LIB_SYMBOL* aA, const LIB_SYMBOL* aB )
            {
                return hasSymbolDifferences( aA, aB );
            } );

    for( ITEM_CHANGE& change : result.changes )
    {
        if( change.kind != CHANGE_KIND::MODIFIED || !change.refdes )
            continue;

        auto beforeIt = m_before.find( *change.refdes );
        auto afterIt = m_after.find( *change.refdes );

        if( beforeIt == m_before.end() || afterIt == m_after.end() )
            continue;

        std::unique_ptr<LIB_SYMBOL> before = beforeIt->second->Flatten();
        std::unique_ptr<LIB_SYMBOL> after = afterIt->second->Flatten();

        change.properties = symbolDeltas( before.get(), after.get() );
        AppendElementChanges( change, DiffSymbolElements( before.get(), after.get() ) );
    }

    return result;
}


bool SYM_LIB_DIFFER::MergeKeepsBothSides( const SYMBOL_MAP& aAncestor, const SYMBOL_MAP& aOurs,
                                          const SYMBOL_MAP& aTheirs, const SYMBOL_MAP& aMerged )
{
    auto carries = [&]( const SYMBOL_MAP& aSide, const SYMBOL_MAP& aOther )
    {
        std::map<KIID_PATH, ITEM_CHANGE> applied;
        std::map<KIID_PATH, ITEM_CHANGE> wanted;

        for( ITEM_CHANGE& change : SYM_LIB_DIFFER( aSide, aMerged ).Diff().changes )
            applied.emplace( change.id, std::move( change ) );

        for( ITEM_CHANGE& change : SYM_LIB_DIFFER( aAncestor, aOther ).Diff().changes )
            wanted.emplace( change.id, std::move( change ) );

        // The bounding box only places a change on screen, and comes from a different symbol on each side
        return std::equal( applied.begin(), applied.end(), wanted.begin(), wanted.end(),
                           []( const auto& aA, const auto& aB )
                           {
                               return aA.first == aB.first && aA.second.kind == aB.second.kind
                                      && aA.second.properties == aB.second.properties
                                      && aA.second.children == aB.second.children;
                           } );
    };

    return carries( aOurs, aTheirs ) && carries( aTheirs, aOurs );
}

} // namespace KICAD_DIFF
