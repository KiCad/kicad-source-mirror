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

#include "sch_merge_applier.h"
#include "sch_diff_utils.h"

#include <diff_merge/merge_validation_pipeline.h>
#include <diff_merge/property_diff.h>

#include <schematic.h>
#include <schematic_settings.h>
#include <erc/erc_settings.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_reference_list.h>
#include <sch_symbol.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <trace_helpers.h>

#include <set>
#include <utility>

#include <wx/log.h>


namespace KICAD_DIFF
{

/// True iff @p aItem is a SCH_SHEET.  Sheet-level operations require deep
/// cloning the SCH_SHEET + SCH_SCREEN pair and rebuilding the path graph;
/// the current applier skips them.  Hoisted to file scope so the predicate
/// is reusable across the multiple lambdas in Apply() that need it.
static bool isSheetItem( const SCH_ITEM* aItem )
{
    return aItem && aItem->Type() == SCH_SHEET_T;
}


/// Return @p aSchematic's root-screen file-format version, or 0 when no root
/// screen is reachable.  Used by the post-apply validator pipeline to feed
/// CheckSchemaVersions; hoisted from Apply() so a follow-up validator
/// fixture can call it without re-implementing the null-guard chain.
static int rootFileFormatVersion( const SCHEMATIC* aSchematic )
{
    if( !aSchematic )
        return 0;

    SCH_SCREEN* root = aSchematic->RootScreen();
    return root ? root->GetFileFormatVersionAtLoad() : 0;
}


SCH_MERGE_APPLIER::SCH_MERGE_APPLIER( SCHEMATIC* aAncestor, const SCHEMATIC* aOurs,
                                      const SCHEMATIC* aTheirs, MERGE_PLAN aPlan ) :
        m_ancestor( aAncestor ),
        m_ours( aOurs ),
        m_theirs( aTheirs ),
        m_plan( std::move( aPlan ) )
{}


std::map<KIID_PATH, SCH_MERGE_APPLIER::PathedItem>
SCH_MERGE_APPLIER::indexSchematic(
        const SCHEMATIC* aSchematic,
        std::vector<std::unique_ptr<SCH_SHEET_PATH>>& aStorage ) const
{
    std::map<KIID_PATH, PathedItem> index;

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
                index[aKiidPath] = { aItem, storedPath };
            } );

    return index;
}


std::size_t SCH_MERGE_APPLIER::applyPropertyResolutions(
        SCH_ITEM*                               aTarget,
        const std::vector<PROPERTY_RESOLUTION>& aProps,
        const SCH_ITEM*                         aOurs,
        const SCH_ITEM*                         aTheirs,
        const SCH_ITEM*                         aAncestor )
{
    PROPERTY_APPLY_COUNTS counts =
            ApplyPropertyResolutions( aTarget, aProps, aOurs, aTheirs, aAncestor );

    m_report.propertiesApplied += counts.applied;
    m_report.propertiesFailed  += counts.failed;
    return counts.applied;
}


bool SCH_MERGE_APPLIER::Apply()
{
    if( !m_ancestor )
        return false;

    m_report                              = {};
    m_report.requiresConnectivityRebuild  = m_plan.requiresConnectivityRebuild;

    // Build indices for ours/theirs (read-only) and ancestor (mutable target).
    std::vector<std::unique_ptr<SCH_SHEET_PATH>> ancStorage;
    std::vector<std::unique_ptr<SCH_SHEET_PATH>> oursStorage;
    std::vector<std::unique_ptr<SCH_SHEET_PATH>> theirsStorage;

    auto ancestorIndex = indexSchematic( m_ancestor, ancStorage );
    auto oursIndex     = indexSchematic( m_ours,     oursStorage );
    auto theirsIndex   = indexSchematic( m_theirs,   theirsStorage );

    // A sub-sheet instantiated more than once shares a single SCH_SCREEN, and
    // WalkSchematic yields one resolution per instance — all aliasing the same
    // SCH_ITEM under distinct KIID_PATHs.  The shared object is cloned/placed
    // once; placedByItemUuid maps its UUID to that single clone so each *other*
    // instance's resolution still applies its per-instance fields (a symbol's
    // Reference/Value live in per-sheet instance data) to the same clone instead
    // of being dropped.  freedAncestorItems guards against double-freeing the
    // shared ancestor (e.g. a whole-item DELETE on one instance), and
    // placedScreenItems guards cloneIntoSheet against a duplicate same-UUID
    // append onto the shared screen.
    std::set<const SCH_ITEM*>              freedAncestorItems;
    std::set<std::pair<SCH_SCREEN*, KIID>> placedScreenItems;
    std::map<KIID, SCH_ITEM*>              placedByItemUuid;

    auto cloneIntoSheet = [&]( const SCH_ITEM* aSource, SCH_SCREEN* aTargetScreen ) -> SCH_ITEM*
    {
        if( !aSource || !aTargetScreen )
            return nullptr;

        // Place a given item on a given screen at most once.  Shared screens are
        // visited once per instance, so without this a duplicate same-UUID clone
        // would be appended for every extra instance.
        if( !placedScreenItems.insert( { aTargetScreen, aSource->m_Uuid } ).second )
            return nullptr;

        EDA_ITEM* cloned = aSource->Clone();
        auto*     schClone = dynamic_cast<SCH_ITEM*>( cloned );

        if( !schClone )
        {
            delete cloned;
            return nullptr;
        }

        // The target screen belongs to the transient offline schematic this applier
        // builds and serializes to disk, not the live editor document.  Appending
        // directly rather than through SCH_COMMIT is deliberate: there is no editor
        // frame or undo stack to keep in sync, so a commit would be wrong here.
        aTargetScreen->Append( schClone );
        return schClone;
    };

    // Find the sheet path on ancestor matching the item's KIID_PATH prefix.
    // Returns nullptr if the path is not present — silently falling back to
    // root would place items on the wrong sheet and corrupt the hierarchy.
    auto sheetPathOnAncestor = [&]( const KIID_PATH& aFullPath ) -> SCH_SHEET_PATH*
    {
        if( aFullPath.empty() )
            return nullptr;

        KIID_PATH sheetPathKey = aFullPath;
        sheetPathKey.pop_back();

        for( auto& storedPath : ancStorage )
        {
            if( storedPath->Path() == sheetPathKey )
                return storedPath.get();
        }

        return nullptr;
    };

    auto takeFrom = [&]( const std::map<KIID_PATH, PathedItem>& aIndex,
                         std::map<KIID_PATH, PathedItem>::const_iterator aIt,
                         const ITEM_RESOLUTION&                         aRes,
                         std::map<KIID_PATH, PathedItem>::iterator      aAncIt,
                         std::size_t&                                   aCounter ) -> bool
    {
        if( aIt == aIndex.end() )
            return false;

        if( isSheetItem( aIt->second.item ) )
        {
            ++m_report.sheetActionsSkipped;
            wxLogTrace( traceDiffMerge,
                        wxT( "applier: sheet-level resolution skipped for %s" ),
                        aRes.id.AsString() );
            return false;
        }

        SCH_ITEM*   ancestorItem   = nullptr;
        SCH_SCREEN* ancestorScreen = nullptr;

        if( aAncIt != ancestorIndex.end() )
        {
            ancestorItem   = aAncIt->second.item;
            ancestorScreen = aAncIt->second.sheetPath->LastScreen();
        }

        SCH_SHEET_PATH* targetSheet = sheetPathOnAncestor( aRes.id );

        if( !targetSheet )
        {
            wxLogTrace( traceDiffMerge,
                        wxT( "applier: target sheet path missing for %s" ),
                        aRes.id.AsString() );
            return false;
        }

        // Detach (don't free) the ancestor copy before cloning so a shared
        // target screen never holds two same-KIID items at once, and reattach it
        // if the clone fails so the item is never lost on the error path (the
        // ancestor used to be deleted before the target/clone were validated).
        if( ancestorItem && ancestorScreen )
        {
            ancestorScreen->SetContentModified();
            ancestorScreen->Remove( ancestorItem );
        }

        SCH_ITEM* placed = cloneIntoSheet( aIt->second.item, targetSheet->LastScreen() );

        if( !placed )
        {
            if( ancestorItem && ancestorScreen )
                ancestorScreen->Append( ancestorItem );

            return false;
        }

        if( !aRes.id.empty() )
            placedByItemUuid[aRes.id.back()] = placed;

        if( ancestorItem && ancestorScreen )
        {
            freedAncestorItems.insert( ancestorItem );
            delete ancestorItem;
            ancestorIndex.erase( aAncIt );
        }

        ++aCounter;
        return true;
    };

    // Copy a symbol's per-instance record (Reference/Value/unit) for one sheet
    // path from a source item onto a destination clone.  Only symbols carry
    // per-instance data; other shared items are identical across instances, so
    // the single placed clone is already correct.
    auto copyInstanceData = []( SCH_ITEM* aDst, SCH_ITEM* aSrc, const KIID_PATH& aPath )
    {
        SCH_SYMBOL* dstSym = dynamic_cast<SCH_SYMBOL*>( aDst );
        SCH_SYMBOL* srcSym = dynamic_cast<SCH_SYMBOL*>( aSrc );

        if( !dstSym || !srcSym )
            return;

        SCH_SYMBOL_INSTANCE inst;

        if( srcSym->GetInstance( inst, aPath ) )
            dstSym->AddHierarchicalReference( inst );   // RemoveInstance()s first, so it overwrites
    };

    // Apply a secondary instance's resolution to the already-placed shared
    // clone.  The object itself was decided by the first instance; here only the
    // per-instance fields for this instance's sheet are updated, so a second
    // instance that resolved differently keeps its own value.
    auto applyInstanceResolution =
            [&]( SCH_ITEM* aPlaced, const ITEM_RESOLUTION& aRes,
                 std::map<KIID_PATH, PathedItem>::iterator aOursIt,
                 std::map<KIID_PATH, PathedItem>::iterator aTheirsIt )
    {
        SCH_SHEET_PATH* targetSheet = sheetPathOnAncestor( aRes.id );

        if( !targetSheet )
            return;

        SCH_ITEM* ours   = aOursIt   != oursIndex.end()   ? aOursIt->second.item   : nullptr;
        SCH_ITEM* theirs = aTheirsIt != theirsIndex.end() ? aTheirsIt->second.item : nullptr;

        SHEET_SCOPE scope( m_ancestor, targetSheet );

        if( aRes.kind == ITEM_RES::MERGE_PROPS )
        {
            // The shared ancestor was freed when the object was first placed, so
            // PROP_RES::ANCESTOR resolutions here fall back to no change;
            // OURS/THEIRS/CUSTOM resolve fully.
            applyPropertyResolutions( aPlaced, aRes.props, ours, theirs, nullptr );
        }
        else if( aRes.kind == ITEM_RES::TAKE_THEIRS )
        {
            copyInstanceData( aPlaced, theirs, targetSheet->Path() );
        }
        else if( aRes.kind == ITEM_RES::TAKE_OURS )
        {
            copyInstanceData( aPlaced, ours, targetSheet->Path() );
        }
        // TAKE_ANCESTOR / KEEP / DELETE: the first instance's whole-object
        // decision stands for the shared object.

        targetSheet->LastScreen()->Update( aPlaced );
    };

    for( const ITEM_RESOLUTION& res : m_plan.actions )
    {
        // Document-level resolution (empty KIID_PATH): page settings live on
        // the root SCH_SCREEN, ERC severities live on the project. Handle
        // TAKE_OURS / TAKE_THEIRS as whole-side copies, and MERGE_PROPS as
        // per-property apply so orthogonal doc edits (ours touches paper,
        // theirs touches ERC severities) auto-merge instead of forcing the
        // user to pick a side. KEEP and TAKE_ANCESTOR are no-ops.
        if( res.id.empty() )
        {
            auto pickSch = [&]( PROP_RES aKind ) -> const SCHEMATIC*
            {
                if( aKind == PROP_RES::OURS )     return m_ours;
                if( aKind == PROP_RES::THEIRS )   return m_theirs;
                return m_ancestor;
            };

            auto markProjectFieldTouched = [&]( const wxString& aProp )
            {
                if( aProp == DOC_PROP_ERC_SEVERITIES )
                    m_report.ercSeveritiesTouched = true;
                else if( aProp == DOC_PROP_DRAWING_SHEET )
                    m_report.drawingSheetFileTouched = true;
                else
                    return;

                m_report.projectFileTouched = true;
            };

            // Whole-document resolutions can explicitly choose ancestor values
            // without mutating memory. Track the exact diverged project fields
            // so the handler can patch just those subtrees into the output.
            auto markDivergedProjectFields = [&]( const SCHEMATIC* aSide )
            {
                if( !aSide || !aSide->IsValid() || !m_ancestor->IsValid() )
                    return;

                if( aSide->ErcSettings().m_ERCSeverities
                    != m_ancestor->ErcSettings().m_ERCSeverities )
                {
                    markProjectFieldTouched( DOC_PROP_ERC_SEVERITIES );
                }

                if( aSide->Settings().m_SchDrawingSheetFileName
                    != m_ancestor->Settings().m_SchDrawingSheetFileName )
                {
                    markProjectFieldTouched( DOC_PROP_DRAWING_SHEET );
                }
            };

            if( res.kind != ITEM_RES::MERGE_PROPS )
            {
                markDivergedProjectFields( m_ours );
                markDivergedProjectFields( m_theirs );
            }

            auto applyWholeSide = [&]( const SCHEMATIC* aSrc )
            {
                if( !aSrc )
                    return;

                if( aSrc->RootScreen() && m_ancestor->RootScreen() )
                {
                    m_ancestor->RootScreen()->SetPageSettings(
                            aSrc->RootScreen()->GetPageSettings() );
                }

                if( aSrc->IsValid() && m_ancestor->IsValid()
                    && aSrc->ErcSettings().m_ERCSeverities
                       != m_ancestor->ErcSettings().m_ERCSeverities )
                {
                    m_ancestor->ErcSettings().m_ERCSeverities =
                            aSrc->ErcSettings().m_ERCSeverities;
                    markProjectFieldTouched( DOC_PROP_ERC_SEVERITIES );
                }

                // Drawing sheet path lives on SCHEMATIC_SETTINGS (which sits
                // inside the project file). Settings() wxASSERTs on a null
                // project; gate on IsValid() like the ERC path.
                if( aSrc->IsValid() && m_ancestor->IsValid()
                    && aSrc->Settings().m_SchDrawingSheetFileName
                       != m_ancestor->Settings().m_SchDrawingSheetFileName )
                {
                    m_ancestor->Settings().m_SchDrawingSheetFileName =
                            aSrc->Settings().m_SchDrawingSheetFileName;
                    markProjectFieldTouched( DOC_PROP_DRAWING_SHEET );
                }
            };

            if( res.kind == ITEM_RES::TAKE_OURS )
            {
                applyWholeSide( m_ours );
            }
            else if( res.kind == ITEM_RES::TAKE_THEIRS )
            {
                applyWholeSide( m_theirs );
            }
            else if( res.kind == ITEM_RES::MERGE_PROPS )
            {
                SCH_SCREEN* ancRoot = m_ancestor->RootScreen();
                PAGE_INFO   merged  = ancRoot ? ancRoot->GetPageSettings() : PAGE_INFO();
                bool        pageTouched = false;

                for( const PROPERTY_RESOLUTION& prop : res.props )
                {
                    const SCHEMATIC* src = pickSch( prop.kind );

                    if( !src || !src->RootScreen() || !ancRoot )
                        continue;

                    const PAGE_INFO& srcPage = src->RootScreen()->GetPageSettings();

                    if( prop.name == DOC_PROP_PAGE_FORMAT )
                    {
                        merged.SetType( srcPage.GetType(), merged.IsPortrait() );
                        pageTouched = true;
                    }
                    else if( prop.name == DOC_PROP_PAGE_ORIENTATION )
                    {
                        merged.SetPortrait( srcPage.IsPortrait() );
                        pageTouched = true;
                    }
                    else if( prop.name == DOC_PROP_ERC_SEVERITIES )
                    {
                        markProjectFieldTouched( DOC_PROP_ERC_SEVERITIES );

                        if( src->IsValid() && m_ancestor->IsValid()
                            && src->ErcSettings().m_ERCSeverities
                               != m_ancestor->ErcSettings().m_ERCSeverities )
                        {
                            m_ancestor->ErcSettings().m_ERCSeverities =
                                    src->ErcSettings().m_ERCSeverities;
                        }
                    }
                    else if( prop.name == DOC_PROP_DRAWING_SHEET )
                    {
                        markProjectFieldTouched( DOC_PROP_DRAWING_SHEET );

                        if( src->IsValid() && m_ancestor->IsValid()
                            && src->Settings().m_SchDrawingSheetFileName
                               != m_ancestor->Settings().m_SchDrawingSheetFileName )
                        {
                            m_ancestor->Settings().m_SchDrawingSheetFileName =
                                    src->Settings().m_SchDrawingSheetFileName;
                        }
                    }
                }

                if( pageTouched && ancRoot )
                    ancRoot->SetPageSettings( merged );
            }

            continue;
        }

        // Per-sheet paper-format resolution: SCH_DIFFER emits these with
        // `id == sheetPath + SCH_SCREEN sentinel KIID`. The sentinel
        // distinguishes the SCH_SCREEN (page-format) resolution from the
        // SCH_SHEET symbol that lives at the same sheetPath. Strip the
        // sentinel, look up the screen, copy paper settings from the chosen
        // side.
        if( !res.id.empty() && res.id.back() == SchScreenSentinelKiid() )
        {
            KIID_PATH sheetPath = res.id;
            sheetPath.pop_back();

            auto resolveScreen =
                    [&]( const std::vector<std::unique_ptr<SCH_SHEET_PATH>>& aPaths )
                            -> SCH_SCREEN*
                    {
                        for( const auto& p : aPaths )
                        {
                            if( p->Path() == sheetPath )
                                return p->LastScreen();
                        }

                        return nullptr;
                    };

            SCH_SCREEN* ancScreen = resolveScreen( ancStorage );

            if( !ancScreen )
                continue;

            if( res.kind == ITEM_RES::TAKE_OURS )
            {
                if( SCH_SCREEN* src = resolveScreen( oursStorage ) )
                    ancScreen->SetPageSettings( src->GetPageSettings() );
            }
            else if( res.kind == ITEM_RES::TAKE_THEIRS )
            {
                if( SCH_SCREEN* src = resolveScreen( theirsStorage ) )
                    ancScreen->SetPageSettings( src->GetPageSettings() );
            }
            else if( res.kind == ITEM_RES::MERGE_PROPS )
            {
                // Orthogonal edits: ours changes Page Format, theirs changes
                // Page Orientation. Build a new PAGE_INFO field-by-field from
                // whichever side the per-property resolution names. Without
                // this branch the screen would silently stay at ancestor.
                PAGE_INFO merged = ancScreen->GetPageSettings();

                auto pickSrcScreen = [&]( PROP_RES aKind ) -> SCH_SCREEN*
                {
                    if( aKind == PROP_RES::OURS )     return resolveScreen( oursStorage );
                    if( aKind == PROP_RES::THEIRS )   return resolveScreen( theirsStorage );
                    return ancScreen;
                };

                for( const PROPERTY_RESOLUTION& prop : res.props )
                {
                    SCH_SCREEN* src = pickSrcScreen( prop.kind );

                    if( !src )
                        continue;

                    const PAGE_INFO& srcPage = src->GetPageSettings();

                    if( prop.name == DOC_PROP_PAGE_FORMAT )
                        merged.SetType( srcPage.GetType(), merged.IsPortrait() );
                    else if( prop.name == DOC_PROP_PAGE_ORIENTATION )
                        merged.SetPortrait( srcPage.IsPortrait() );
                }

                ancScreen->SetPageSettings( merged );
            }

            continue;
        }

        auto ancIt    = ancestorIndex.find( res.id );
        auto oursIt   = oursIndex.find( res.id );
        auto theirsIt = theirsIndex.find( res.id );

        // Sheet-level operations are explicitly out of scope (P0 from review:
        // SCH_SHEET::Clone() is shallow and would graft a screen from the
        // source schematic into ancestor, corrupting ownership).
        auto resolutionTargetsSheet = [&]() -> bool
        {
            if( ancIt    != ancestorIndex.end() && isSheetItem( ancIt->second.item ) )    return true;
            if( oursIt   != oursIndex.end()     && isSheetItem( oursIt->second.item ) )   return true;
            if( theirsIt != theirsIndex.end()   && isSheetItem( theirsIt->second.item ) ) return true;
            return false;
        };

        if( resolutionTargetsSheet() )
        {
            ++m_report.sheetActionsSkipped;
            continue;
        }

        // A shared SCH_SCREEN surfaces the same item under several instance
        // paths.  If an earlier instance already placed the shared object, apply
        // this instance's per-instance resolution to that clone rather than
        // re-cloning it (which would drop this instance's Reference/Value).
        if( !res.id.empty() )
        {
            auto placedIt = placedByItemUuid.find( res.id.back() );

            if( placedIt != placedByItemUuid.end() )
            {
                applyInstanceResolution( placedIt->second, res, oursIt, theirsIt );
                continue;
            }
        }

        // Otherwise, if an earlier instance freed this ancestor item without
        // placing a replacement (e.g. a whole-item DELETE), skip so we don't
        // double-free it.
        if( ancIt != ancestorIndex.end() && freedAncestorItems.count( ancIt->second.item ) )
            continue;

        switch( res.kind )
        {
        case ITEM_RES::TAKE_OURS:
            takeFrom( oursIndex, oursIt, res, ancIt, m_report.itemsTakenOurs );
            break;

        case ITEM_RES::TAKE_THEIRS:
            takeFrom( theirsIndex, theirsIt, res, ancIt, m_report.itemsTakenTheirs );
            break;

        case ITEM_RES::TAKE_ANCESTOR:
            // Already in ancestor; nothing to do.
            break;

        case ITEM_RES::DELETE_ITEM:
        {
            if( ancIt != ancestorIndex.end() )
            {
                SCH_ITEM* victim = ancIt->second.item;
                freedAncestorItems.insert( victim );
                ancIt->second.sheetPath->LastScreen()->DeleteItem( victim );
                ancestorIndex.erase( ancIt );
                ++m_report.itemsDeleted;
            }

            break;
        }

        case ITEM_RES::KEEP:
            // Conservative-conflict default: leave whatever is in ancestor.
            ++m_report.itemsKept;
            break;

        case ITEM_RES::MERGE_PROPS:
        {
            // Align with PCB applier convention: clone OURS as the base, then
            // apply resolutions. This way property failures default to the
            // ours value, not the ancestor value.
            if( oursIt == oursIndex.end() )
                break;

            SCH_SHEET_PATH* targetSheet = sheetPathOnAncestor( res.id );

            if( !targetSheet )
                break;

            // applyPropertyResolutions reads from ancestor for
            // PROP_RES::ANCESTOR, so the ancestor item must outlive that
            // call. Capture pointers before any mutation.
            const SCH_ITEM* theirs        = theirsIt != theirsIndex.end()
                                                    ? theirsIt->second.item : nullptr;
            SCH_ITEM*       ancestorItem  = nullptr;
            SCH_SCREEN*     ancestorScreen = nullptr;

            if( ancIt != ancestorIndex.end() )
            {
                ancestorItem   = ancIt->second.item;
                ancestorScreen = ancIt->second.sheetPath->LastScreen();
            }

            // Detach ancestor from its screen RTree before the clone goes in,
            // so the screen never holds two same-KIID items at once. Append /
            // RTree don't enforce that invariant but property-listener
            // callbacks fired during Set can observe screen state.
            if( ancestorItem && ancestorScreen )
            {
                ancestorScreen->SetContentModified();
                ancestorScreen->Remove( ancestorItem );
            }

            SCH_SCREEN* targetScreen = targetSheet->LastScreen();
            SCH_ITEM*   placed = cloneIntoSheet( oursIt->second.item, targetScreen );

            if( !placed )
            {
                if( ancestorItem && ancestorScreen )
                    ancestorScreen->Append( ancestorItem );

                break;
            }

            if( !res.id.empty() )
                placedByItemUuid[res.id.back()] = placed;

            {
                // Per-instance Reference/Value writes route through
                // SCHEMATIC::CurrentSheet(); point it at the symbol's own sheet
                // so SetRefProp/SetValueProp update the correct instance (the
                // differ reads its values under the same scope).
                SHEET_SCOPE scope( m_ancestor, targetSheet );

                applyPropertyResolutions( placed, res.props,
                                          oursIt->second.item, theirs, ancestorItem );

                // Property writes may have moved placed; refresh its RTree entry.
                targetScreen->Update( placed );
            }

            // Only free the ancestor if it was actually detached from a
            // screen above. A null ancestorScreen means we never had it in
            // hand, which means we never indexed it — leaving it untouched
            // is correct.
            if( ancestorItem && ancestorScreen )
            {
                freedAncestorItems.insert( ancestorItem );
                delete ancestorItem;
                ancestorIndex.erase( ancIt );
            }

            ++m_report.itemsMergedProps;
            break;
        }
        }
    }

    // Post-apply validators.  Collect every symbol reference across the
    // hierarchy + schema versions + connectivity ack into VALIDATION_INPUT.
    {
        VALIDATION_INPUT vInput;

        if( m_ancestor )
        {
            SCH_SHEET_LIST hierarchy = m_ancestor->Hierarchy();

            for( const SCH_SHEET_PATH& sheetPath : hierarchy )
            {
                SCH_SCREEN* screen = sheetPath.LastScreen();

                if( !screen )
                    continue;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
                {
                    SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                    REFDES_ENTRY entry;
                    entry.refdes = sym->GetRef( &sheetPath );
                    entry.id     = sheetPath.Path();
                    entry.id.push_back( sym->m_Uuid );
                    vInput.refdesEntries.push_back( std::move( entry ) );
                }
            }
        }

        vInput.planRequiredRebuild    = m_plan.requiresConnectivityRebuild;
        vInput.applierReportedRebuild = m_report.connectivityRebuildPerformed;

        vInput.ancestorSchemaVersion = rootFileFormatVersion( m_ancestor );
        vInput.oursSchemaVersion     = rootFileFormatVersion( m_ours );
        vInput.theirsSchemaVersion   = rootFileFormatVersion( m_theirs );

        m_report.validation = RunPostApplyValidators( vInput );
    }

    return true;
}

} // namespace KICAD_DIFF
