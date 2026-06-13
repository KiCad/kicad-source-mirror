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

#include "sch_differ.h"
#include "sch_diff_utils.h"

#include <diff_merge/doc_property_helpers.h>
#include <diff_merge/property_diff.h>

#include <schematic.h>
#include <schematic_settings.h>
#include <erc/erc_settings.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

#include <algorithm>


namespace KICAD_DIFF
{

SCH_DIFFER::SCH_DIFFER( const SCHEMATIC* aBefore, const SCHEMATIC* aAfter,
                        const wxString& aPath ) :
        m_before( aBefore ),
        m_after( aAfter ),
        m_path( aPath )
{}


SCH_DIFFER::~SCH_DIFFER() = default;


void SCH_DIFFER::SetScope( const KIID_PATH& aBeforeScope, const KIID_PATH& aAfterScope )
{
    m_scopeBefore = aBeforeScope;
    m_scopeAfter = aAfterScope;
}


wxString SCH_DIFFER::itemTypeName( const SCH_ITEM* aItem )
{
    if( !aItem )
        return wxEmptyString;

    return aItem->GetClass();
}


std::optional<wxString> SCH_DIFFER::itemRefdes( const SCH_ITEM*       aItem,
                                                const SCH_SHEET_PATH* aPath )
{
    if( !aPath )
        return std::nullopt;

    if( auto sym = dynamic_cast<const SCH_SYMBOL*>( aItem ) )
        return sym->GetRef( aPath );

    return std::nullopt;
}


void SCH_DIFFER::walk( const SCHEMATIC* aSchematic, std::vector<WalkedItem>& aOut,
                       std::vector<std::unique_ptr<SCH_SHEET_PATH>>& aStorage, const KIID_PATH& aScope ) const
{
    // Retain one owned SCH_SHEET_PATH per sheet and alias it across that
    // sheet's items. The per-sheet visitor materializes the copy once; the
    // item visitor reuses the stored pointer, matching the original loop.
    SCH_SHEET_PATH* storedPath = nullptr;

    WalkSchematic(
            aSchematic,
            [&]( const SCH_SHEET_PATH& aPath )
            {
                aStorage.push_back( std::make_unique<SCH_SHEET_PATH>( aPath ) );
                storedPath = aStorage.back().get();
            },
            [&]( SCH_ITEM* aItem, const SCH_SHEET_PATH&, const KIID_PATH& aKiidPath )
            {
                WalkedItem w;
                w.id        = aKiidPath;
                w.item      = aItem;
                w.sheetPath = storedPath;
                aOut.push_back( std::move( w ) );
            },
            aScope );
}


namespace
{

/// Compute a bounding box with the schematic's CurrentSheet temporarily set
/// to the item's sheet path, so SCH_FIELD/SCH_SYMBOL text widths resolve
/// against the correct instance.
BOX2I bboxOnSheet( const SCHEMATIC* aSch, const SCH_ITEM* aItem,
                   const SCH_SHEET_PATH* aPath )
{
    SHEET_SCOPE scope( aSch, aPath );
    return aItem->GetBoundingBox();
}

} // anonymous namespace


std::vector<PROPERTY_DELTA> SCH_DIFFER::diffProperties( const SCH_ITEM* aBefore,
                                                        const SCH_ITEM* aAfter,
                                                        const SCH_SHEET_PATH* aBeforePath,
                                                        const SCH_SHEET_PATH* aAfterPath ) const
{
    if( !aBefore || !aAfter || typeid( *aBefore ) != typeid( *aAfter ) )
        return {};

    SHEET_SCOPE beforeScope( m_before, aBeforePath );
    SHEET_SCOPE afterScope( m_after, aAfterPath );

    auto deltas = DiffItemProperties( aBefore, aAfter );

    // In scoped mode drop properties whose value comes from a per-instance
    // override. Comparing them across shared sheet instances flags every
    // symbol as Modified even when the stored content is identical.
    const bool scoped = !m_scopeBefore.empty() || !m_scopeAfter.empty();

    if( scoped )
    {
        auto perInstance = []( const wxString& aName )
        {
            return aName == wxS( "Reference" ) || aName == wxS( "Unit" );
        };

        deltas.erase( std::remove_if( deltas.begin(), deltas.end(),
                                      [&]( const PROPERTY_DELTA& d )
                                      {
                                          return perInstance( d.name );
                                      } ),
                      deltas.end() );
    }

    return deltas;
}


void SCH_DIFFER::sortChanges( std::vector<ITEM_CHANGE>& aChanges )
{
    std::sort( aChanges.begin(), aChanges.end(),
               []( const ITEM_CHANGE& aL, const ITEM_CHANGE& aR )
               {
                   if( aL.id < aR.id )            return true;
                   if( aR.id < aL.id )            return false;

                   if( aL.typeName != aR.typeName )
                       return aL.typeName < aR.typeName;

                   return static_cast<int>( aL.kind ) < static_cast<int>( aR.kind );
               } );
}


DOCUMENT_DIFF SCH_DIFFER::Diff()
{
    DOCUMENT_DIFF result;
    result.path    = m_path;
    result.docType = wxS( "kicad_sch" );

    if( !m_before || !m_after )
        return result;

    // Walk both schematics through their entire sheet hierarchy.
    std::vector<WalkedItem>                       beforeWalk;
    std::vector<WalkedItem>                       afterWalk;
    std::vector<std::unique_ptr<SCH_SHEET_PATH>>  beforePaths;
    std::vector<std::unique_ptr<SCH_SHEET_PATH>>  afterPaths;

    walk( m_before, beforeWalk, beforePaths, m_scopeBefore );
    walk( m_after, afterWalk, afterPaths, m_scopeAfter );

    if( m_options.progress )
        m_options.progress( 0.2 );

    // Build reconciler descriptors and a lookup back to the live items.
    std::vector<ITEM_DESCRIPTOR>             beforeDesc;
    std::vector<ITEM_DESCRIPTOR>             afterDesc;
    std::map<KIID_PATH, const WalkedItem*>   beforeMap;
    std::map<KIID_PATH, const WalkedItem*>   afterMap;

    auto makeDescriptor = []( const WalkedItem& aW, const SCHEMATIC* aSch ) -> ITEM_DESCRIPTOR
    {
        ITEM_DESCRIPTOR d;
        d.id       = aW.id;
        d.type     = itemTypeName( aW.item );
        d.position = aW.item->GetPosition();
        d.bbox     = bboxOnSheet( aSch, aW.item, aW.sheetPath );

        if( auto sym = dynamic_cast<const SCH_SYMBOL*>( aW.item ) )
        {
            d.keyProps.emplace_back( wxS( "lib_id" ),
                                     std::string( sym->GetLibId().Format().c_str() ) );
            d.keyProps.emplace_back( wxS( "reference" ),
                                     sym->GetRef( aW.sheetPath ).ToStdString() );
        }

        return d;
    };

    for( const WalkedItem& w : beforeWalk )
    {
        beforeDesc.push_back( makeDescriptor( w, m_before ) );
        beforeMap[w.id] = &w;
    }

    for( const WalkedItem& w : afterWalk )
    {
        afterDesc.push_back( makeDescriptor( w, m_after ) );
        afterMap[w.id] = &w;
    }

    IDENTITY_RECONCILER reconciler( m_options.identity );
    RECONCILIATION      recon = reconciler.Reconcile( beforeDesc, afterDesc );

    if( m_options.progress )
        m_options.progress( 0.5 );

    // Duplicate KIID_PATH entries inside either schematic.
    for( const KIID_PATH& dup : recon.duplicatesA )
    {
        ITEM_CHANGE c;
        c.id       = dup;
        c.typeName = wxS( "SCH_ITEM" );
        c.kind     = CHANGE_KIND::DUPLICATE_UUID;
        result.changes.push_back( std::move( c ) );
    }

    for( const KIID_PATH& dup : recon.duplicatesB )
    {
        if( std::find_if( result.changes.begin(), result.changes.end(),
                          [&]( const ITEM_CHANGE& aC )
                          { return aC.id == dup && aC.kind == CHANGE_KIND::DUPLICATE_UUID; } )
            != result.changes.end() )
        {
            continue;
        }

        ITEM_CHANGE c;
        c.id       = dup;
        c.typeName = wxS( "SCH_ITEM" );
        c.kind     = CHANGE_KIND::DUPLICATE_UUID;
        result.changes.push_back( std::move( c ) );
    }

    // Matched pairs: emit MODIFIED records when any delta surfaces.
    for( const auto& [idA, idB] : recon.aToB )
    {
        auto itA = beforeMap.find( idA );
        auto itB = afterMap.find( idB );
        const WalkedItem* a = itA == beforeMap.end() ? nullptr : itA->second;
        const WalkedItem* b = itB == afterMap.end()  ? nullptr : itB->second;

        if( !a || !b )
            continue;

        std::vector<PROPERTY_DELTA> propDeltas;

        if( m_options.deepCompare )
            propDeltas = diffProperties( a->item, b->item, a->sheetPath, b->sheetPath );

        bool semanticallyEqual = ( *a->item == *b->item );
        const bool scoped = !m_scopeBefore.empty() || !m_scopeAfter.empty();

        if( propDeltas.empty() && ( semanticallyEqual || scoped ) )
            continue;

        ITEM_CHANGE c;
        c.id         = idA;
        c.typeName   = itemTypeName( a->item );
        c.kind       = CHANGE_KIND::MODIFIED;
        c.bbox       = bboxOnSheet( m_after, b->item, b->sheetPath );
        c.refdes     = itemRefdes( b->item, b->sheetPath );
        c.properties = std::move( propDeltas );
        result.changes.push_back( std::move( c ) );
    }

    for( const KIID_PATH& idA : recon.aOnly )
    {
        auto it = beforeMap.find( idA );

        if( it == beforeMap.end() || !it->second )
            continue;

        const WalkedItem* a = it->second;
        ITEM_CHANGE c;
        c.id       = idA;
        c.typeName = itemTypeName( a->item );
        c.kind     = CHANGE_KIND::REMOVED;
        c.bbox     = bboxOnSheet( m_before, a->item, a->sheetPath );
        c.refdes   = itemRefdes( a->item, a->sheetPath );

        {
            SHEET_SCOPE scope( m_before, a->sheetPath );
            c.properties = ItemProperties( a->item, /*aAsAfter=*/false );
        }

        result.changes.push_back( std::move( c ) );
    }

    for( const KIID_PATH& idB : recon.bOnly )
    {
        auto it = afterMap.find( idB );

        if( it == afterMap.end() || !it->second )
            continue;

        const WalkedItem* b = it->second;

        ITEM_CHANGE c;
        c.id       = idB;
        c.typeName = itemTypeName( b->item );
        c.kind     = CHANGE_KIND::ADDED;
        c.bbox     = bboxOnSheet( m_after, b->item, b->sheetPath );
        c.refdes   = itemRefdes( b->item, b->sheetPath );

        {
            SHEET_SCOPE scope( m_after, b->sheetPath );
            c.properties = ItemProperties( b->item, /*aAsAfter=*/true );
        }

        result.changes.push_back( std::move( c ) );
    }

    // Page settings live on the root SCH_SCREEN, not on any walked item, so
    // the per-item loop above can't see a change to them. Emit a synthetic
    // ITEM_CHANGE with empty KIID_PATH so the planner can resolve it.
    std::vector<PROPERTY_DELTA> docDeltas;

    const SCH_SCREEN* beforeRoot = m_before ? m_before->RootScreen() : nullptr;
    const SCH_SCREEN* afterRoot  = m_after  ? m_after->RootScreen()  : nullptr;

    if( beforeRoot && afterRoot )
        AppendPaperDeltas( docDeltas, beforeRoot->GetPageSettings(),
                           afterRoot->GetPageSettings() );

    // ERC severity overrides live in the project file. Diff only fires when
    // sibling .kicad_pro files were loaded — for plain .kicad_sch temp blobs
    // both sides see defaults and we never get here. Be defensive about
    // ErcSettings() — it wxASSERTs on a null project; under QA the asserter
    // throws.
    auto ercSeverities =
            []( const SCHEMATIC* aSch ) -> const std::map<int, SEVERITY>*
            {
                if( !aSch || !aSch->IsValid() )
                    return nullptr;

                return &aSch->ErcSettings().m_ERCSeverities;
            };

    const std::map<int, SEVERITY>* beforeERC = ercSeverities( m_before );
    const std::map<int, SEVERITY>* afterERC  = ercSeverities( m_after );

    if( beforeERC && afterERC && *beforeERC != *afterERC )
    {
        PROPERTY_DELTA d;
        d.name   = DOC_PROP_ERC_SEVERITIES;
        d.before = DIFF_VALUE::FromString( SummarizeSeverities( *beforeERC ) );
        d.after  = DIFF_VALUE::FromString( SummarizeSeverities( *afterERC ) );
        docDeltas.push_back( std::move( d ) );
    }

    // Drawing sheet file path. Lives on SCHEMATIC_SETTINGS::m_SchDrawingSheet
    // FileName (which lives inside the project file). Settings() wxASSERTs
    // on a null project; only access when both sides loaded a sibling
    // .kicad_pro.
    auto schDrawingSheet = []( const SCHEMATIC* aSch ) -> wxString
    {
        if( !aSch || !aSch->IsValid() )
            return wxEmptyString;

        return aSch->Settings().m_SchDrawingSheetFileName;
    };

    const wxString beforeSheet = schDrawingSheet( m_before );
    const wxString afterSheet  = schDrawingSheet( m_after );

    if( beforeSheet != afterSheet )
    {
        PROPERTY_DELTA d;
        d.name   = DOC_PROP_DRAWING_SHEET;
        d.before = DIFF_VALUE::FromString( beforeSheet );
        d.after  = DIFF_VALUE::FromString( afterSheet );
        docDeltas.push_back( std::move( d ) );
    }

    if( !docDeltas.empty() )
    {
        ITEM_CHANGE c;
        c.id         = KIID_PATH();
        c.typeName   = wxS( "SCHEMATIC" );
        c.kind       = CHANGE_KIND::MODIFIED;
        c.bbox       = BOX2I();
        c.properties = std::move( docDeltas );
        result.changes.push_back( std::move( c ) );
    }

    // Per-sheet paper settings: sub-sheets in a hierarchy can carry their
    // own paper size / orientation distinct from the root. Dedup by screen
    // pointer so re-instantiated sub-sheets (multiple SCH_SHEET_PATHs hit
    // the same underlying SCH_SCREEN) produce one delta, not N — otherwise
    // the user would see duplicate rows and the applier could resolve them
    // inconsistently.
    auto sheetPaperByScreen =
            []( const SCHEMATIC* aSch )
                    -> std::map<const SCH_SCREEN*, std::pair<KIID_PATH, const PAGE_INFO*>>
            {
                std::map<const SCH_SCREEN*, std::pair<KIID_PATH, const PAGE_INFO*>> out;

                if( !aSch || !aSch->IsValid() )
                    return out;

                for( const SCH_SHEET_PATH& path : aSch->BuildSheetListSortedByPageNumbers() )
                {
                    if( const SCH_SCREEN* screen = path.LastScreen() )
                        out.try_emplace( screen, path.Path(), &screen->GetPageSettings() );
                }

                return out;
            };

    const SCH_SCREEN* const beforeRootScreen = beforeRoot;

    // Build the after-side path->screen map once. We CANNOT store a pointer
    // into a temporary SCH_SHEET_LIST from BuildSheetListSortedByPageNumbers
    // — the list goes out of scope before we'd dereference. Storing the raw
    // SCH_SCREEN* is safe because the screens are owned by the SCHEMATIC.
    std::map<KIID_PATH, const SCH_SCREEN*> afterScreensByPath;

    if( m_after && m_after->IsValid() )
    {
        for( const SCH_SHEET_PATH& p : m_after->BuildSheetListSortedByPageNumbers() )
        {
            if( const SCH_SCREEN* screen = p.LastScreen() )
                afterScreensByPath.try_emplace( p.Path(), screen );
        }
    }

    for( const auto& [beforeScreen, entry] : sheetPaperByScreen( m_before ) )
    {
        // Root paper is covered by the empty-KIID_PATH delta above; skip
        // the root screen here so we don't double-report.
        if( beforeScreen == beforeRootScreen )
            continue;

        const KIID_PATH& sheetPath    = entry.first;
        const PAGE_INFO& beforePaper  = *entry.second;

        // Match by KIID_PATH. Topology changes (renamed / re-parented
        // sheets) where the path no longer matches are out of scope; the
        // differ doesn't model sheet structure changes, only content on
        // common paths.
        auto afterIt = afterScreensByPath.find( sheetPath );

        if( afterIt == afterScreensByPath.end() || !afterIt->second )
            continue;

        std::vector<PROPERTY_DELTA> sheetDeltas;
        AppendPaperDeltas( sheetDeltas, beforePaper,
                           afterIt->second->GetPageSettings() );

        if( !sheetDeltas.empty() )
        {
            ITEM_CHANGE c;
            c.id         = sheetPath;
            // Append the sentinel KIID so this ID doesn't collide with the
            // SCH_SHEET symbol that lives at the same `sheetPath`. The
            // applier checks the last KIID to route to the screen branch.
            c.id.push_back( SchScreenSentinelKiid() );
            c.typeName   = wxS( "SCH_SCREEN" );
            c.kind       = CHANGE_KIND::MODIFIED;
            c.bbox       = BOX2I();
            c.properties = std::move( sheetDeltas );
            result.changes.push_back( std::move( c ) );
        }
    }

    sortChanges( result.changes );

    if( m_options.progress )
        m_options.progress( 1.0 );

    return result;
}

} // namespace KICAD_DIFF
