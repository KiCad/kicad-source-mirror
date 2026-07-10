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

#include "pcb_merge_applier.h"
#include "applier_helpers.h"

#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <board_stackup_manager/board_stackup.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <wildcards_and_files_ext.h>
#include <diff_merge/merge_validation_pipeline.h>
#include <diff_merge/property_diff.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <board_item_container.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_field.h>
#include <zone.h>

#include <map>
#include <set>


namespace KICAD_DIFF
{

PCB_MERGE_APPLIER::PCB_MERGE_APPLIER( const BOARD* aAncestor, const BOARD* aOurs,
                                      const BOARD* aTheirs, MERGE_PLAN aPlan ) :
        m_ancestor( aAncestor ),
        m_ours( aOurs ),
        m_theirs( aTheirs ),
        m_plan( std::move( aPlan ) )
{}


const BOARD_ITEM* PCB_MERGE_APPLIER::findItem( const BOARD* aBoard, const KIID& aId ) const
{
    if( !aBoard )
        return nullptr;

    // The board maintains its own KIID->item cache (m_itemByIdCache) and
    // resolves top-level items plus footprint children (pads, fields,
    // graphics, zones), a superset of what this applier needs.  Request
    // nullptr-on-missing rather than the DELETED_BOARD_ITEM sentinel so the
    // lookup keeps its original "not found = nullptr" contract.
    return aBoard->ResolveItem( aId, /* aAllowNullptrReturn */ true );
}


BOARD_ITEM* PCB_MERGE_APPLIER::cloneInto( BOARD* aTarget, const BOARD_ITEM* aSource ) const
{
    if( !aSource || !aTarget )
        return nullptr;

    std::unique_ptr<EDA_ITEM> cloned( aSource->Clone() );

    if( !cloned )
        return nullptr;

    auto* boardClone = dynamic_cast<BOARD_ITEM*>( cloned.get() );

    if( !boardClone )
        return nullptr;

    // aTarget is the transient offline BOARD that Apply() builds and serializes to
    // disk; it is never the live editor document.  Adding directly rather than
    // through BOARD_COMMIT is deliberate: there is no editor frame, undo stack, or
    // VIEW to keep in sync, and routing through a commit would be wrong here.
    aTarget->Add( boardClone, ADD_MODE::APPEND );

    // Ownership transfers to aTarget only once Add() has adopted the clone.
    cloned.release();
    return boardClone;
}


std::size_t PCB_MERGE_APPLIER::applyPropertyResolutions(
        BOARD_ITEM*                             aTarget,
        const std::vector<PROPERTY_RESOLUTION>& aProps,
        const BOARD_ITEM*                       aOurs,
        const BOARD_ITEM*                       aTheirs,
        const BOARD_ITEM*                       aAncestor )
{
    PROPERTY_APPLY_COUNTS counts =
            ApplyPropertyResolutions( aTarget, aProps, aOurs, aTheirs, aAncestor );

    m_report.propertiesApplied += counts.applied;
    m_report.propertiesFailed  += counts.failed;
    return counts.applied;
}


std::unique_ptr<BOARD> PCB_MERGE_APPLIER::Apply()
{
    if( !m_ours && !m_theirs && !m_ancestor )
        return nullptr;

    m_report                              = {};
    m_report.requiresZoneRefill           = m_plan.requiresZoneRefill;
    m_report.requiresConnectivityRebuild  = m_plan.requiresConnectivityRebuild;

    auto result = std::make_unique<BOARD>();

    // Index plan actions by item id so we can decide per item what to do.
    std::map<KIID_PATH, const ITEM_RESOLUTION*> actionsById;

    for( const ITEM_RESOLUTION& r : m_plan.actions )
        actionsById[r.id] = &r;

    // Read a file in the BOARD's project directory.  @p aIsFullName is true
    // for files whose name (no extension) lives next to the .kicad_pcb;
    // false to derive a sibling by extension.  Shared between the whole-side
    // divergence-staging path and the per-property MERGE_PROPS branch.
    auto readProjectSiblingFile = []( const BOARD* aBoard, const wxString& aName,
                                       bool aIsFullName ) -> wxString
    {
        if( !aBoard )
            return wxEmptyString;

        wxString boardPath = aBoard->GetFileName();

        if( boardPath.IsEmpty() )
            return wxEmptyString;

        wxFileName fn( boardPath );

        if( aIsFullName )
            fn.SetFullName( aName );
        else
            fn.SetExt( aName );

        if( !fn.FileExists() )
            return wxEmptyString;

        wxFile file( fn.GetFullPath() );

        if( !file.IsOpened() )
            return wxEmptyString;

        wxString contents;
        file.ReadAll( &contents );
        return contents;
    };

    auto readSiblingRules = [&]( const BOARD* aBoard ) -> wxString
    {
        return readProjectSiblingFile( aBoard,
                wxString::FromUTF8( FILEEXT::DesignRulesFileExtension ), false );
    };

    auto readFpLibTable = [&]( const BOARD* aBoard ) -> wxString
    {
        return readProjectSiblingFile( aBoard,
                wxString::FromUTF8( FILEEXT::FootprintLibraryTableFileName ), true );
    };

    auto readSymLibTable = [&]( const BOARD* aBoard ) -> wxString
    {
        return readProjectSiblingFile( aBoard,
                wxString::FromUTF8( FILEEXT::SymbolLibraryTableFileName ), true );
    };

    // Document-level settings — paper format, board thickness, design
    // settings. PCB_DIFFER emits a synthetic ITEM_CHANGE with an empty
    // KIID_PATH to capture changes here. Default: copy from ancestor (or
    // ours if no ancestor); the engine's TAKE_OURS / TAKE_THEIRS / TAKE_
    // ANCESTOR resolution overrides that.
    {
        const KIID_PATH        docPath;   // empty path = document sentinel
        const ITEM_RESOLUTION* docRes     = nullptr;
        auto                   docIt      = actionsById.find( docPath );

        if( docIt != actionsById.end() )
            docRes = docIt->second;

        const BOARD* settingsSrc = m_ancestor ? m_ancestor : m_ours;

        if( docRes )
        {
            switch( docRes->kind )
            {
            case ITEM_RES::TAKE_OURS:     settingsSrc = m_ours;     break;
            case ITEM_RES::TAKE_THEIRS:   settingsSrc = m_theirs;   break;
            case ITEM_RES::TAKE_ANCESTOR: settingsSrc = m_ancestor; break;
            default:                                                break;
            }
        }

        // Break the shared_ptr<NET_SETTINGS> alias that
        // BOARD_DESIGN_SETTINGS::CopyFrom installs when SetDesignSettings
        // runs (m_NetSettings = aOther.m_NetSettings copies the pointer).
        // Subsequent CopyFrom calls into result's NET_SETTINGS would
        // otherwise mutate the chosen side's settings too.
        auto detachNetSettingsFor = []( BOARD* aBoard )
        {
            if( !aBoard )
                return;

            aBoard->GetDesignSettings().m_NetSettings =
                    std::make_shared<NET_SETTINGS>( nullptr, "" );
        };

        // Copy a chosen side's net settings into result without aliasing the
        // shared_ptr (which would couple the merged board's NET_SETTINGS to
        // the source's lifetime and -- because NESTED_SETTINGS has a parent
        // linkage -- to the source project file's m_nested_settings map).
        auto adoptNetSettings = [&]( const BOARD* aSource )
        {
            if( !aSource || !aSource->GetDesignSettings().m_NetSettings
                || !result->GetDesignSettings().m_NetSettings )
            {
                return;
            }

            result->GetDesignSettings().m_NetSettings->CopyFrom(
                    *aSource->GetDesignSettings().m_NetSettings );
        };

        if( settingsSrc )
        {
            result->SetPageSettings( settingsSrc->GetPageSettings() );
            result->SetDesignSettings( settingsSrc->GetDesignSettings() );
            detachNetSettingsFor( result.get() );
            adoptNetSettings( settingsSrc );

            // Flag the project-file-scoped fields (DRC severities) the
            // handler needs to mirror onto ancestor + persist via Save
            // ProjectCopy. Fire whenever any side diverged from ancestor
            // — including a TAKE_ANCESTOR resolution, since the merge
            // output still needs ancestor's severity map written to disk
            // (the output path may not pre-exist, or may contain ours/
            // theirs).
            // Bind to a shared empty map when there is no ancestor so the
            // common ancestor-present branch references the member directly
            // instead of materializing a full copy of the severity map (a
            // mixed value-category ternary would force one).
            static const std::map<int, SEVERITY> s_emptySeverities;
            const std::map<int, SEVERITY>&       ancDrc =
                    m_ancestor ? m_ancestor->GetDesignSettings().m_DRCSeverities : s_emptySeverities;

            const bool oursDrcChanged =
                    m_ours && m_ours->GetDesignSettings().m_DRCSeverities != ancDrc;
            const bool theirsDrcChanged =
                    m_theirs && m_theirs->GetDesignSettings().m_DRCSeverities != ancDrc;

            // Net settings divergence detection. Like DRC severities, this only
            // fires when sibling .kicad_pro files were loaded; plain temp blobs
            // (git mergetool) see defaults on every side.
            auto netSettingsEqual = []( const BOARD* aLhs, const BOARD* aRhs )
            {
                if( !aLhs || !aRhs )
                    return true;

                const auto& lhs = aLhs->GetDesignSettings().m_NetSettings;
                const auto& rhs = aRhs->GetDesignSettings().m_NetSettings;

                if( !lhs && !rhs )
                    return true;

                if( !lhs || !rhs )
                    return false;

                return *lhs == *rhs;
            };

            const bool oursNetChanged   = !netSettingsEqual( m_ours,   m_ancestor );
            const bool theirsNetChanged = !netSettingsEqual( m_theirs, m_ancestor );

            // Custom DRC rules live in a sibling .kicad_dru file (not in the
            // .kicad_pro), so divergence detection reads the file content.
            // Plain temp-blob merges typically see empty content on every
            // side — diff fires only when a real project tree is present.
            const wxString ancRules    = readSiblingRules( m_ancestor );
            const wxString oursRules   = readSiblingRules( m_ours );
            const wxString theirsRules = readSiblingRules( m_theirs );

            const bool oursRulesChanged   = m_ours   && oursRules   != ancRules;
            const bool theirsRulesChanged = m_theirs && theirsRules != ancRules;

            // Stage the chosen side's rules content for the handler to write
            // alongside the merged board.  Whole-side path: take the
            // settingsSrc choice (TAKE_OURS/THEIRS/ANCESTOR).  Per-property
            // MERGE_PROPS overrides this below.
            if( oursRulesChanged || theirsRulesChanged )
            {
                if( settingsSrc == m_ours )
                    m_report.customDrcRules = oursRules;
                else if( settingsSrc == m_theirs )
                    m_report.customDrcRules = theirsRules;
                else
                    m_report.customDrcRules = ancRules;

                m_report.customDrcRulesSet = true;
            }

            // Library tables (fp-lib-table, sym-lib-table) follow the same
            // shape as custom DRC rules.  Read each side's content from the
            // project directory and stage the chosen side's content on the
            // report for the handler to write into the merged project dir.
            const wxString ancFp    = readFpLibTable( m_ancestor );
            const wxString oursFp   = readFpLibTable( m_ours );
            const wxString theirsFp = readFpLibTable( m_theirs );

            const bool oursFpChanged   = m_ours   && oursFp   != ancFp;
            const bool theirsFpChanged = m_theirs && theirsFp != ancFp;

            if( oursFpChanged || theirsFpChanged )
            {
                if( settingsSrc == m_ours )
                    m_report.fpLibTable = oursFp;
                else if( settingsSrc == m_theirs )
                    m_report.fpLibTable = theirsFp;
                else
                    m_report.fpLibTable = ancFp;

                m_report.fpLibTableSet = true;
            }

            const wxString ancSym    = readSymLibTable( m_ancestor );
            const wxString oursSym   = readSymLibTable( m_ours );
            const wxString theirsSym = readSymLibTable( m_theirs );

            const bool oursSymChanged   = m_ours   && oursSym   != ancSym;
            const bool theirsSymChanged = m_theirs && theirsSym != ancSym;

            if( oursSymChanged || theirsSymChanged )
            {
                if( settingsSrc == m_ours )
                    m_report.symLibTable = oursSym;
                else if( settingsSrc == m_theirs )
                    m_report.symLibTable = theirsSym;
                else
                    m_report.symLibTable = ancSym;

                m_report.symLibTableSet = true;
            }

            // Drawing sheet file lives on PROJECT_FILE. Detect divergence so
            // TAKE_ANCESTOR also persists the chosen path.
            auto drawingSheet = []( const BOARD* aBoard ) -> wxString
            {
                if( !aBoard || !aBoard->GetProject() )
                    return wxEmptyString;

                return aBoard->GetProject()->GetProjectFile().m_BoardDrawingSheetFile;
            };

            const wxString ancSheet    = drawingSheet( m_ancestor );
            const bool sheetDiverged   =
                    ( m_ours && drawingSheet( m_ours ) != ancSheet )
                    || ( m_theirs && drawingSheet( m_theirs ) != ancSheet );

            // Whole-side path: SetDesignSettings copies the board+stackup
            // fields but the drawing sheet path lives on PROJECT_FILE. The
            // result BOARD has no project to mutate; stage the chosen
            // value on REPORT so the handler can mirror it onto ancestor's
            // project before SaveProjectCopy. Without this, sheetDiverged
            // would flip projectFileTouched but no path value would be
            // staged, and the handler's mirror block would skip,
            // persisting ancestor's old sheet to disk.
            if( sheetDiverged && settingsSrc && settingsSrc->GetProject() )
            {
                m_report.drawingSheetFile =
                        settingsSrc->GetProject()->GetProjectFile().m_BoardDrawingSheetFile;
                m_report.drawingSheetFileSet = true;
            }

            if( oursDrcChanged || theirsDrcChanged )
            {
                m_report.drcSeveritiesTouched = true;
                m_report.projectFileTouched = true;
            }

            if( oursNetChanged || theirsNetChanged )
            {
                m_report.netClassesTouched = true;
                m_report.projectFileTouched = true;
            }

            if( sheetDiverged || oursRulesChanged || theirsRulesChanged
                || oursFpChanged || theirsFpChanged || oursSymChanged || theirsSymChanged )
            {
                m_report.projectFileTouched = true;
            }
        }

        // MERGE_PROPS for doc-level: orthogonal edits (ours touches paper,
        // theirs touches thickness) should auto-merge instead of forcing the
        // user to pick a side. Apply per-property over the whole-side base
        // we just copied.
        if( docRes && docRes->kind == ITEM_RES::MERGE_PROPS )
        {
            auto pickBoard = [&]( PROP_RES aKind ) -> const BOARD*
            {
                if( aKind == PROP_RES::OURS )     return m_ours;
                if( aKind == PROP_RES::THEIRS )   return m_theirs;
                return m_ancestor;
            };

            PAGE_INFO merged      = result->GetPageSettings();
            bool      pageTouched = false;

            for( const PROPERTY_RESOLUTION& prop : docRes->props )
            {
                const BOARD* src = pickBoard( prop.kind );

                if( !src )
                    continue;

                if( prop.name == DOC_PROP_PAGE_FORMAT )
                {
                    merged.SetType( src->GetPageSettings().GetType(), merged.IsPortrait() );
                    pageTouched = true;
                }
                else if( prop.name == DOC_PROP_PAGE_ORIENTATION )
                {
                    merged.SetPortrait( src->GetPageSettings().IsPortrait() );
                    pageTouched = true;
                }
                else if( prop.name == DOC_PROP_BOARD_THICKNESS )
                {
                    result->GetDesignSettings().SetBoardThickness(
                            src->GetDesignSettings().GetBoardThickness() );
                }
                else if( prop.name == DOC_PROP_LAYER_STACKUP )
                {
                    // Stackup is structural; per-property doesn't decompose,
                    // copy the whole stackup descriptor.
                    result->GetDesignSettings().GetStackupDescriptor() =
                            src->GetDesignSettings().GetStackupDescriptor();
                }
                else if( prop.name == DOC_PROP_DRC_SEVERITIES )
                {
                    // Always copy the chosen side's severity map and flag
                    // projectFileTouched. The engine only emitted this
                    // property in MERGE_PROPS because at least one side
                    // diverged from ancestor; a PROP_RES::ANCESTOR
                    // resolution writes ancestor's map back to the output
                    // .kicad_pro (which may not pre-exist).
                    result->GetDesignSettings().m_DRCSeverities =
                            src->GetDesignSettings().m_DRCSeverities;
                    m_report.drcSeveritiesTouched = true;
                    m_report.projectFileTouched = true;
                }
                else if( prop.name == DOC_PROP_FP_LIB_TABLE )
                {
                    m_report.fpLibTable    = readFpLibTable( src );
                    m_report.fpLibTableSet = true;
                    m_report.projectFileTouched = true;
                }
                else if( prop.name == DOC_PROP_SYM_LIB_TABLE )
                {
                    m_report.symLibTable    = readSymLibTable( src );
                    m_report.symLibTableSet = true;
                    m_report.projectFileTouched = true;
                }
                else if( prop.name == DOC_PROP_CUSTOM_RULES )
                {
                    // Stage the chosen side's .kicad_dru content on the
                    // report.  The handler writes it next to the merged
                    // .kicad_pcb.  PROP_RES::ANCESTOR re-reads ancestor's
                    // rules so a TAKE_ANCESTOR resolution still persists
                    // ancestor's content to the output path (which may
                    // not pre-exist or may contain ours' rules from a
                    // previous merge attempt).
                    m_report.customDrcRules    = readSiblingRules( src );
                    m_report.customDrcRulesSet = true;
                    m_report.projectFileTouched = true;
                }
                else if( prop.name == DOC_PROP_NET_CLASSES )
                {
                    // Net classes don't decompose per-property; copy the
                    // chosen side's whole NET_SETTINGS into result via
                    // CopyFrom (which preserves m_parent / m_path on the
                    // result's NET_SETTINGS so SaveProjectCopy walks the
                    // right nested-settings entry). The whole-side branch
                    // already detached the alias and adopted settingsSrc;
                    // this overrides that choice for the per-property
                    // resolution.
                    if( src && src->GetDesignSettings().m_NetSettings
                        && result->GetDesignSettings().m_NetSettings )
                    {
                        result->GetDesignSettings().m_NetSettings->CopyFrom(
                                *src->GetDesignSettings().m_NetSettings );
                    }

                    m_report.netClassesTouched = true;
                    m_report.projectFileTouched = true;
                }
                else if( prop.name == DOC_PROP_DRAWING_SHEET )
                {
                    // Drawing sheet path lives on PROJECT_FILE. Stage the
                    // chosen value on the result BOARD's project (which
                    // PCB_MERGE_APPLIER doesn't own — store the choice in
                    // the report so the handler can mirror it onto
                    // ancestor's project before SaveProjectCopy). The
                    // handler reads m_drawingSheetFile and applies if
                    // non-empty marker.
                    if( src && src->GetProject() )
                    {
                        m_report.drawingSheetFile =
                                src->GetProject()->GetProjectFile().m_BoardDrawingSheetFile;
                        m_report.drawingSheetFileSet = true;
                        m_report.projectFileTouched = true;
                    }
                }
            }

            if( pageTouched )
                result->SetPageSettings( merged );
        }
    }

    // Collect every distinct KIID that appears in any of the three boards or
    // in the plan.  Walk top-level items only -- footprint children are
    // handled implicitly when their parent footprint is cloned.
    std::set<KIID> allIds;
    CollectTopLevelIds( m_ancestor, allIds );
    CollectTopLevelIds( m_ours,     allIds );
    CollectTopLevelIds( m_theirs,   allIds );

    auto resolutionFor = [&]( const KIID& aUuid ) -> const ITEM_RESOLUTION*
    {
        KIID_PATH path;
        path.push_back( aUuid );

        auto it = actionsById.find( path );

        if( it == actionsById.end() )
            return nullptr;

        return it->second;
    };

    // Track which actions were consumed at the top level so the child-
    // resolution post-pass only sees nested actions.
    std::set<KIID_PATH> consumedActions;

    for( const KIID& uuid : allIds )
    {
        KIID_PATH topPath;
        topPath.push_back( uuid );

        if( actionsById.count( topPath ) )
            consumedActions.insert( topPath );

        const ITEM_RESOLUTION* res = resolutionFor( uuid );

        // No resolution = item unchanged on both sides; take from ancestor
        // (or ours if ancestor missing — handles new boards without a base).
        if( !res )
        {
            const BOARD_ITEM* src = findItem( m_ancestor, uuid );

            if( !src )
                src = findItem( m_ours, uuid );

            if( !src )
                src = findItem( m_theirs, uuid );

            cloneInto( result.get(), src );
            continue;
        }

        switch( res->kind )
        {
        case ITEM_RES::TAKE_OURS:
        {
            const BOARD_ITEM* src = findItem( m_ours, uuid );

            if( src )
            {
                cloneInto( result.get(), src );
                ++m_report.itemsTakenOurs;
            }

            break;
        }

        case ITEM_RES::TAKE_THEIRS:
        {
            const BOARD_ITEM* src = findItem( m_theirs, uuid );

            if( src )
            {
                cloneInto( result.get(), src );
                ++m_report.itemsTakenTheirs;
            }

            break;
        }

        case ITEM_RES::TAKE_ANCESTOR:
        {
            const BOARD_ITEM* src = findItem( m_ancestor, uuid );

            if( src )
                cloneInto( result.get(), src );

            break;
        }

        case ITEM_RES::DELETE_ITEM:
            ++m_report.itemsDeleted;
            // Intentionally drop the item.
            break;

        case ITEM_RES::KEEP:
        {
            // Conservative conflict default: take ancestor if available,
            // otherwise ours, otherwise theirs.
            const BOARD_ITEM* src = findItem( m_ancestor, uuid );

            if( !src )
                src = findItem( m_ours, uuid );

            if( !src )
                src = findItem( m_theirs, uuid );

            if( src )
            {
                cloneInto( result.get(), src );
                ++m_report.itemsKept;
            }

            break;
        }

        case ITEM_RES::MERGE_PROPS:
        {
            const BOARD_ITEM* ours     = findItem( m_ours, uuid );
            const BOARD_ITEM* theirs   = findItem( m_theirs, uuid );
            const BOARD_ITEM* ancestor = findItem( m_ancestor, uuid );

            // Start from ours; apply property resolutions.
            const BOARD_ITEM* base = ours ? ours : ( ancestor ? ancestor : theirs );

            if( !base )
                break;

            BOARD_ITEM* placed = cloneInto( result.get(), base );

            if( !placed )
                break;

            applyPropertyResolutions( placed, res->props, ours, theirs, ancestor );
            ++m_report.itemsMergedProps;
            break;
        }
        }
    }

    // Child-level resolution post-pass. The merge engine emits actions for
    // footprint children (pads, fields, graphics, zones) with KIID_PATHs of
    // the form [parent_uuid, child_uuid]. The top-level loop above brings
    // children along when the parent footprint is cloned, but does NOT apply
    // per-child resolutions. This pass finds the cloned child on the result
    // board and adds/removes/merges it per its resolution.

    // Index the merged board's footprints once so each child action resolves
    // its parent in O(log n), instead of rebuilding result->GetItemSet() (a
    // full item-set copy) and linear-scanning it per child action.
    std::map<KIID, FOOTPRINT*> footprintsByUuid;

    for( FOOTPRINT* fp : result->Footprints() )
    {
        if( fp )
            footprintsByUuid[fp->m_Uuid] = fp;
    }

    for( const auto& [actionPath, action] : actionsById )
    {
        if( consumedActions.count( actionPath ) )
            continue;

        if( actionPath.size() < 2 )
            continue;   // not a child path

        const KIID& parentUuid = actionPath.at( 0 );
        const KIID& childUuid  = actionPath.at( 1 );

        // Find the cloned parent footprint on the result board.
        auto       fpIt    = footprintsByUuid.find( parentUuid );
        FOOTPRINT* parentFp = fpIt != footprintsByUuid.end() ? fpIt->second : nullptr;

        if( !parentFp )
            continue;

        BOARD_ITEM* targetChild = nullptr;

        for( PAD* pad : parentFp->Pads() )
        {
            if( pad->m_Uuid == childUuid )
            {
                targetChild = pad;
                break;
            }
        }

        if( !targetChild )
        {
            for( BOARD_ITEM* g : parentFp->GraphicalItems() )
            {
                if( g && g->m_Uuid == childUuid )
                {
                    targetChild = g;
                    break;
                }
            }
        }

        if( !targetChild )
        {
            for( ZONE* z : parentFp->Zones() )
            {
                if( z && z->m_Uuid == childUuid )
                {
                    targetChild = z;
                    break;
                }
            }
        }

        if( !targetChild )
        {
            for( PCB_FIELD* f : parentFp->GetFields() )
            {
                if( f && f->m_Uuid == childUuid )
                {
                    targetChild = f;
                    break;
                }
            }
        }

        // Replace the parent's current child (if any) with a clone of the
        // chosen side's child, or add it when the ours-based parent clone does
        // not carry it.  Used by the take-a-side child resolutions below.
        auto adoptChildFrom = [&]( const BOARD* aSide )
        {
            if( targetChild )
            {
                parentFp->Remove( targetChild );
                delete targetChild;
                targetChild = nullptr;
            }

            const BOARD_ITEM* src = findItem( aSide, childUuid );

            if( !src )
                return;

            std::unique_ptr<EDA_ITEM> cloned( src->Clone() );

            if( auto* childClone = dynamic_cast<BOARD_ITEM*>( cloned.get() ) )
            {
                parentFp->Add( childClone, ADD_MODE::APPEND );

                // Ownership transfers to parentFp only once Add() has adopted the clone.
                cloned.release();
            }
        };

        switch( action->kind )
        {
        case ITEM_RES::MERGE_PROPS:
        {
            if( !targetChild )
                break;

            // Footprint children carry globally-unique UUIDs, so the per-board
            // index keys them directly — no parent-scoped scan needed.
            const BOARD_ITEM* oursChild     = findItem( m_ours, childUuid );
            const BOARD_ITEM* theirsChild   = findItem( m_theirs, childUuid );
            const BOARD_ITEM* ancestorChild = findItem( m_ancestor, childUuid );

            applyPropertyResolutions( targetChild, action->props,
                                      oursChild, theirsChild, ancestorChild );
            break;
        }

        case ITEM_RES::TAKE_THEIRS:
            // Child added or modified on theirs.  The ours-based parent clone
            // does not carry a theirs-added child, so clone it in (or replace
            // the ours version); otherwise the addition/edit is silently lost.
            adoptChildFrom( m_theirs );
            break;

        case ITEM_RES::TAKE_OURS:
            // The parent may have been cloned from ancestor/theirs when the
            // parent itself had no resolution (or resolved to another side).
            // Replace the current child from ours so one-sided child edits do
            // not depend on the parent's chosen clone source.
            adoptChildFrom( m_ours );
            break;

        case ITEM_RES::TAKE_ANCESTOR:
            adoptChildFrom( m_ancestor );
            break;

        case ITEM_RES::DELETE_ITEM:
            // Child removed on a side.  The ours-based clone still has it, so
            // drop it; otherwise the deletion is silently reverted.
            if( targetChild )
            {
                parentFp->Remove( targetChild );
                delete targetChild;
            }

            break;

        case ITEM_RES::KEEP:
            // Conservative conflict default: preserve the child already present
            // on the parent clone.
            break;
        }
    }

    // Post-apply validators.  Collect refdes entries from the merged result,
    // schema versions from each side, and the connectivity-rebuild ack the
    // caller may have set.  Failures land on m_report.validation; the CLI merge
    // handlers surface them through the job reporter.
    {
        VALIDATION_INPUT vInput;

        for( const FOOTPRINT* fp : result->Footprints() )
        {
            if( !fp )
                continue;

            REFDES_ENTRY entry;
            entry.refdes = fp->GetReference();
            entry.id     = KIID_PATH();
            entry.id.push_back( fp->m_Uuid );
            vInput.refdesEntries.push_back( std::move( entry ) );
        }

        // The merged board is serialized and its connectivity is rebuilt by the
        // consumer when it loads the result (connectivity is not persisted), so
        // the applier has satisfied the plan's rebuild requirement for the
        // output file.  Acknowledge it here so the validator doesn't raise a
        // false "stale connectivity" error on every connectivity-affecting merge.
        m_report.connectivityRebuildPerformed = m_plan.requiresConnectivityRebuild;

        vInput.planRequiredRebuild    = m_plan.requiresConnectivityRebuild;
        vInput.applierReportedRebuild = m_report.connectivityRebuildPerformed;

        vInput.ancestorSchemaVersion = m_ancestor ? m_ancestor->GetFileFormatVersionAtLoad() : 0;
        vInput.oursSchemaVersion     = m_ours     ? m_ours    ->GetFileFormatVersionAtLoad() : 0;
        vInput.theirsSchemaVersion   = m_theirs   ? m_theirs  ->GetFileFormatVersionAtLoad() : 0;

        m_report.validation = RunPostApplyValidators( vInput );
    }

    return result;
}

} // namespace KICAD_DIFF
