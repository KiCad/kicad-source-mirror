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

#include <wx/filename.h>


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
    bool hasSymbolDifferences( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter )
    {
        std::unique_ptr<LIB_SYMBOL> before = aBefore->Flatten();
        std::unique_ptr<LIB_SYMBOL> after = aAfter->Flatten();

        return !DiffItemProperties( before.get(), after.get() ).empty()
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

        change.properties = DiffItemProperties( before.get(), after.get() );

        for( const SYM_ELEMENT& element : DiffSymbolElements( before.get(), after.get() ) )
        {
            ITEM_CHANGE child;
            child.typeName = element.typeName;
            child.kind = element.kind;
            child.properties = element.deltas;

            if( !element.key.IsEmpty() )
                child.refdes = element.key;

            change.children.push_back( std::move( child ) );
        }
    }

    return result;
}

} // namespace KICAD_DIFF
