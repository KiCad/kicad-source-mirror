/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <board_commit.h>
#include <tools/pcb_actions.h>

#include <dialogs/dialog_multichannel_generate_rule_areas.h>
#include <dialogs/dialog_multichannel_repeat_layout.h>

#include "multichannel_tool.h"

#include <pcbexpr_evaluator.h>

#include <zone.h>
#include <geometry/convex_hull.h>
#include <geometry/shape_utils.h>
#include <pcb_group.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/topo_match.h>
#include <optional>
#include <algorithm>
#include <pcbnew_scripting_helpers.h>
#include <tool/tool_manager.h>
#include <tools/pcb_picker_tool.h>
#include <random>
#include <core/profile.h>
#include <wx/log.h>
#include <pgm_base.h>


#define MULTICHANNEL_EXTRA_DEBUG

static const wxString traceMultichannelTool = wxT( "MULTICHANNEL_TOOL" );


MULTICHANNEL_TOOL::MULTICHANNEL_TOOL() : PCB_TOOL_BASE( "pcbnew.Multichannel" )
{
}


MULTICHANNEL_TOOL::~MULTICHANNEL_TOOL()
{

}


void MULTICHANNEL_TOOL::setTransitions()
{
    Go( &MULTICHANNEL_TOOL::AutogenerateRuleAreas, PCB_ACTIONS::generatePlacementRuleAreas.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::repeatLayout, PCB_ACTIONS::repeatLayout.MakeEvent() );
}


bool MULTICHANNEL_TOOL::identifyComponentsInRuleArea( ZONE*                 aRuleArea,
                                                      std::set<FOOTPRINT*>& aComponents )
{
    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  ctx, preflightCtx;

    auto reportError = [&]( const wxString& aMessage, int aOffset )
    {
        wxLogTrace( traceMultichannelTool, wxT( "ERROR: %s"), aMessage );
    };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );
    //compiler.SetDebugReporter( m_reporter );

    wxLogTrace( traceMultichannelTool, wxT( "rule area '%s'"), aRuleArea->GetZoneName() );

    wxString ruleText;

    switch( aRuleArea->GetRuleAreaPlacementSourceType() )
    {
    case RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME:
    {
        ruleText =
                wxT( "A.memberOfSheet('" ) + aRuleArea->GetRuleAreaPlacementSource() + wxT( "')" );
        break;
    }
    case RULE_AREA_PLACEMENT_SOURCE_TYPE::COMPONENT_CLASS:
        ruleText = wxT( "A.hasComponentClass('" ) + aRuleArea->GetRuleAreaPlacementSource()
                   + wxT( "')" );
        break;
    }

    auto ok = compiler.Compile( ruleText, &ucode, &preflightCtx );

    if( !ok )
    {
        return false;
    }

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        ctx.SetItems( fp, fp );
        auto val = ucode.Run( &ctx );
        if( val->AsDouble() != 0.0 )
        {
            wxLogTrace( traceMultichannelTool, wxT( " - %s [sheet %s]" ), fp->GetReference(),
                                                  fp->GetSheetname() );

            aComponents.insert( fp );
        }
    }

    return true;
}


bool MULTICHANNEL_TOOL::findOtherItemsInRuleArea( ZONE* aRuleArea, std::set<BOARD_ITEM*>& aItems )
{
    std::vector<BOARD_ITEM*> result;

    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  ctx, preflightCtx;

    auto reportError = [&]( const wxString& aMessage, int aOffset )
    {
        wxLogTrace( traceMultichannelTool, wxT( "ERROR: %s"), aMessage );
    };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );

    bool restoreBlankName = false;

    if( aRuleArea->GetZoneName().IsEmpty() )
    {
        restoreBlankName = true;
        aRuleArea->SetZoneName( aRuleArea->m_Uuid.AsString() );
    }

    wxString ruleText = wxString::Format( wxT( "A.enclosedByArea('%s')" ), aRuleArea->GetZoneName() );

    if( !compiler.Compile( ruleText, &ucode, &preflightCtx ) )
    {
        if( restoreBlankName )
            aRuleArea->SetZoneName( wxEmptyString );

        return false;
    }

    auto testAndAdd =
        [&]( BOARD_ITEM* aItem )
        {
            ctx.SetItems( aItem, aItem );
            auto val = ucode.Run( &ctx );

            if( val->AsDouble() != 0.0 )
                aItems.insert( aItem );
        };

    for( ZONE* zone : board()->Zones() )
    {
        if( zone == aRuleArea )
            continue;

        testAndAdd( zone );
    }

    for( BOARD_ITEM* drawing : board()->Drawings() )
        testAndAdd( drawing );

    for( PCB_GROUP* group : board()->Groups() )
    {
        // A group is cloned in its entirety if *all* children are contained
        bool addGroup = true;

        group->RunOnDescendants(
                [&]( BOARD_ITEM* aItem )
                {
                    if( aItem->IsType( { PCB_ZONE_T, PCB_SHAPE_T, PCB_DIMENSION_T } ) )
                    {
                        ctx.SetItems( aItem, aItem );
                        auto val = ucode.Run( &ctx );

                        if( val->AsDouble() == 0.0 )
                            addGroup = false;
                    }
                } );

        if( addGroup )
            aItems.insert( group );
    }

    if( restoreBlankName )
        aRuleArea->SetZoneName( wxEmptyString );

    return true;
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInSheet( wxString aSheetName ) const
{
    std::set<FOOTPRINT*> rv;
    if( aSheetName.EndsWith( wxT( "/" ) ) )
        aSheetName.RemoveLast();

    for( auto& fp : board()->Footprints() )
    {
        auto sn = fp->GetSheetname();
        if( sn.EndsWith( wxT( "/" ) ) )
            sn.RemoveLast();

        if( sn == aSheetName )
        {
            rv.insert( fp );
        }
    }

    return rv;
}


std::set<FOOTPRINT*>
MULTICHANNEL_TOOL::queryComponentsInComponentClass( const wxString& aComponentClassName ) const
{
    std::set<FOOTPRINT*> rv;

    for( auto& fp : board()->Footprints() )
    {
        if( fp->GetComponentClass()->ContainsClassName( aComponentClassName ) )
            rv.insert( fp );
    }

    return rv;
}


const SHAPE_LINE_CHAIN MULTICHANNEL_TOOL::buildRAOutline( std::set<FOOTPRINT*>& aFootprints,
                                                          int                   aMargin )
{
    std::vector<VECTOR2I> bbCorners;
    bbCorners.reserve( aFootprints.size() * 4 );

    for( auto fp : aFootprints )
    {
        const BOX2I bb = fp->GetBoundingBox( false ).GetInflated( aMargin );
        KIGEOM::CollectBoxCorners( bb, bbCorners );
    }

    std::vector<VECTOR2I> hullVertices;
    BuildConvexHull( hullVertices, bbCorners );

    SHAPE_LINE_CHAIN hull( hullVertices );

    // Make the newly computed convex hull use only 90 degree segments
    return KIGEOM::RectifyPolygon( hull );
}


void MULTICHANNEL_TOOL::QuerySheetsAndComponentClasses()
{
    using PathAndName = std::pair<wxString, wxString>;
    std::set<PathAndName> uniqueSheets;
    std::set<wxString>    uniqueComponentClasses;

    m_areas.m_areas.clear();

    for( const FOOTPRINT* fp : board()->Footprints() )
    {
        uniqueSheets.insert( PathAndName( fp->GetSheetname(), fp->GetSheetfile() ) );

        const COMPONENT_CLASS* compClass = fp->GetComponentClass();

        for( const COMPONENT_CLASS* singleClass : compClass->GetConstituentClasses() )
            uniqueComponentClasses.insert( singleClass->GetName() );
    }

    for( const PathAndName& sheet : uniqueSheets )
    {
        RULE_AREA ent;

        ent.m_sourceType = RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME;
        ent.m_generateEnabled = false;
        ent.m_sheetPath = sheet.first;
        ent.m_sheetName = sheet.second;
        ent.m_components = queryComponentsInSheet( ent.m_sheetPath );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT("found sheet '%s' @ '%s' s %d\n"),
            ent.m_sheetName, ent.m_sheetPath, (int)m_areas.m_areas.size() );
    }

    for( const wxString& compClass : uniqueComponentClasses )
    {
        RULE_AREA ent;

        ent.m_sourceType = RULE_AREA_PLACEMENT_SOURCE_TYPE::COMPONENT_CLASS;
        ent.m_generateEnabled = false;
        ent.m_componentClass = compClass;
        ent.m_components = queryComponentsInComponentClass( ent.m_componentClass );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT( "found component class '%s' s %d\n" ),
                    ent.m_componentClass, static_cast<int>( m_areas.m_areas.size() ) );
    }
}


void MULTICHANNEL_TOOL::FindExistingRuleAreas()
{
    m_areas.m_areas.clear();

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;
        if( !zone->GetRuleAreaPlacementEnabled() )
            continue;

        RULE_AREA area;

        area.m_existsAlready = true;
        area.m_area = zone;

        identifyComponentsInRuleArea( zone, area.m_raFootprints );

        area.m_ruleName = zone->GetZoneName();
        area.m_center = zone->Outline()->COutline( 0 ).Centre();
        m_areas.m_areas.push_back( area );

        wxLogTrace( traceMultichannelTool, wxT("RA '%s', %d footprints\n"), area.m_ruleName, (int) area.m_raFootprints.size() );
    }

    wxLogTrace( traceMultichannelTool, wxT("Total RAs found: %d\n"), (int) m_areas.m_areas.size() );
}


RULE_AREA* MULTICHANNEL_TOOL::findRAByName( const wxString& aName )
{
    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( ra.m_ruleName == aName )
            return &ra;
    }

    return nullptr;
}


void MULTICHANNEL_TOOL::UpdatePickedItem( const EDA_ITEM* aItem )
{
    m_toolMgr->RunAction( PCB_ACTIONS::repeatLayout );
}


int MULTICHANNEL_TOOL::repeatLayout( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE*> refRAs;

    auto isSelectedItemAnRA = []( EDA_ITEM* aItem ) -> ZONE*
        {
            if( !aItem || aItem->Type() != PCB_ZONE_T )
                return nullptr;

            ZONE* zone = static_cast<ZONE*>( aItem );

            if( !zone->GetIsRuleArea() )
                return nullptr;

            if( !zone->GetRuleAreaPlacementEnabled() )
                return nullptr;

            return zone;
        };

    for( EDA_ITEM* item : selection() )
    {
        if( auto zone = isSelectedItemAnRA( item ) )
        {
            refRAs.push_back(zone);
        }
        else if ( item->Type() == PCB_GROUP_T )
        {
            PCB_GROUP *group = static_cast<PCB_GROUP*>( item );

            for( BOARD_ITEM* grpItem : group->GetItems() )
            {
                if( auto grpZone = isSelectedItemAnRA( grpItem ) )
                {
                    refRAs.push_back( grpZone );
                }
            }
        }
    }

    if( refRAs.size() != 1 )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectItemInteractively,
                              PCB_PICKER_TOOL::INTERACTIVE_PARAMS {
                                  this,
                                  _( "Select a reference Rule Area to copy from..." ),
                                  [&]( EDA_ITEM* aItem )
                                  {
                                      return isSelectedItemAnRA( aItem ) != nullptr;
                                  }
                              } );

        return 0;
    }

    FindExistingRuleAreas();

    int status = CheckRACompatibility( refRAs.front() );

    if( status < 0 )
        return status;

    if( m_areas.m_areas.size() <= 1 )
    {
        frame()->ShowInfoBarError( _( "No Rule Areas to repeat layout to have been found." ),
                                    true );
        return 0;
    }

    DIALOG_MULTICHANNEL_REPEAT_LAYOUT dialog( frame(), this );
    int ret = dialog.ShowModal();

    if( ret != wxID_OK )
        return 0;

    return RepeatLayout( aEvent, refRAs.front() );
}


int MULTICHANNEL_TOOL::CheckRACompatibility( ZONE *aRefZone )
{
    m_areas.m_refRA = nullptr;

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( ra.m_area == aRefZone )
        {
            m_areas.m_refRA = &ra;
            break;
        }
    }

    if( !m_areas.m_refRA )
        return -1;

    m_areas.m_compatMap.clear();

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( ra.m_area == m_areas.m_refRA->m_area )
            continue;

        m_areas.m_compatMap[&ra] = RULE_AREA_COMPAT_DATA();

        resolveConnectionTopology( m_areas.m_refRA, &ra, m_areas.m_compatMap[&ra] );
    }

    return 0;
}


int MULTICHANNEL_TOOL::RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone )
{
    int totalCopied = 0;

    for( auto& targetArea : m_areas.m_compatMap )
    {
        if( !targetArea.second.m_doCopy )
        {
            wxLogTrace( traceMultichannelTool, wxT("skipping copy to RA '%s' (disabled in dialog)\n"),
                 targetArea.first->m_ruleName );
            continue;
        }

        if( !targetArea.second.m_isOk )
            continue;

        BOARD_COMMIT commit( GetManager(), true );

        std::unordered_set<BOARD_ITEM*> affectedItems;
        std::unordered_set<BOARD_ITEM*> groupableItems;

        if( !copyRuleAreaContents( targetArea.second.m_matchingComponents, &commit, m_areas.m_refRA,
                                   targetArea.first, m_areas.m_options, affectedItems,
                                   groupableItems ) )
        {
            auto errMsg = wxString::Format(
                    _( "Copy Rule Area contents failed between rule areas '%s' and '%s'." ),
                    m_areas.m_refRA->m_area->GetZoneName(),
                    targetArea.first->m_area->GetZoneName() );

            commit.Revert();
            if( Pgm().IsGUI() )
            {
                frame()->ShowInfoBarError( errMsg, true );
            }

            return -1;
        }

        commit.Push( _( "Repeat layout" ) );

        if( m_areas.m_options.m_groupItems )
        {
            // fixme: sth gets weird when creating new zones & grouping them within a single COMMIT
            BOARD_COMMIT grpCommit( GetManager(), true );

            pruneExistingGroups( grpCommit, affectedItems );

            PCB_GROUP* grp = new PCB_GROUP( board() );

            grpCommit.Add( grp );

            for( BOARD_ITEM* item : groupableItems )
            {
                grpCommit.Stage( item, CHT_GROUP );
            }

            grpCommit.Push( _( "Group components with their placement rule areas" ) );
        }

        totalCopied++;
    }

    if( Pgm().IsGUI() )
    {
        frame()->ShowInfoBarMsg( wxString::Format( _( "Copied to %d Rule Areas." ), totalCopied ),
                             true );
    }
    return 0;
}


wxString MULTICHANNEL_TOOL::stripComponentIndex( const wxString& aRef ) const
{
    wxString rv;

    // fixme: i'm pretty sure this can be written in a simpler way, but I really suck at figuring
    // out which wx's built in functions would do it for me. And I hate regexps :-)
    for( auto k : aRef )
    {
        if( !k.IsAscii() )
            break;
        char c;
        k.GetAsChar( &c );

        if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c == '_' ) )
            rv.Append( k );
        else
            break;
    }

    return rv;
}


int MULTICHANNEL_TOOL::findRoutedConnections( std::set<BOARD_ITEM*>&             aOutput,
                                              std::shared_ptr<CONNECTIVITY_DATA> aConnectivity,
                                              const SHAPE_POLY_SET& aRAPoly, RULE_AREA* aRA,
                                              FOOTPRINT*                   aFp,
                                              const REPEAT_LAYOUT_OPTIONS& aOpts ) const
{
    std::set<BOARD_ITEM*> conns;

    for( PAD* pad : aFp->Pads() )
    {
        const std::vector<BOARD_CONNECTED_ITEM*> connItems = aConnectivity->GetConnectedItems(
                pad, { PCB_PAD_T, PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_SHAPE_T }, true );

        for( BOARD_CONNECTED_ITEM* item : connItems )
            conns.insert( item );
    }

    int count = 0;

    for( BOARD_ITEM* item : conns )
    {
        // fixme: respect layer sets assigned to each RA

        if( item->Type() == PCB_PAD_T )
            continue;

        std::shared_ptr<SHAPE> effShape = item->GetEffectiveShape( item->GetLayer() );

        if( effShape->Collide( &aRAPoly, 0 ) )
        {
            aOutput.insert( item );
            count++;
        }
    }

    return count;
}


bool MULTICHANNEL_TOOL::copyRuleAreaContents( TMATCH::COMPONENT_MATCHES& aMatches,
                                              BOARD_COMMIT* aCommit,
                                              RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                              REPEAT_LAYOUT_OPTIONS aOpts,
                                              std::unordered_set<BOARD_ITEM*>& aAffectedItems,
                                              std::unordered_set<BOARD_ITEM*>& aGroupableItems )
{
    // copy RA shapes first
    SHAPE_LINE_CHAIN refOutline = aRefArea->m_area->Outline()->COutline( 0 );
    SHAPE_LINE_CHAIN targetOutline = aTargetArea->m_area->Outline()->COutline( 0 );

    VECTOR2I disp = aTargetArea->m_center - aRefArea->m_center;

    SHAPE_POLY_SET refPoly;
    refPoly.AddOutline( refOutline );
    refPoly.CacheTriangulation( false );

    SHAPE_POLY_SET targetPoly;

    SHAPE_LINE_CHAIN newTargetOutline( refOutline );
    newTargetOutline.Move( disp );
    targetPoly.AddOutline( newTargetOutline );
    targetPoly.CacheTriangulation( false );

    auto connectivity = board()->GetConnectivity();

    aCommit->Modify( aTargetArea->m_area );

    aAffectedItems.insert( aTargetArea->m_area );
    aGroupableItems.insert( aTargetArea->m_area );

    if( aOpts.m_copyRouting )
    {
        std::set<BOARD_ITEM*> refRouting;
        std::set<BOARD_ITEM*> targetRouting;

        wxLogTrace( traceMultichannelTool, wxT("copying routing: %d fps\n"), (int) aMatches.size() );

        for( auto& fpPair : aMatches )
        {
            findRoutedConnections( targetRouting, connectivity, targetPoly, aTargetArea,
                                   fpPair.second, aOpts );
            findRoutedConnections( refRouting, connectivity, refPoly, aRefArea, fpPair.first,
                                   aOpts );

            wxLogTrace( traceMultichannelTool, wxT("target-routes %d\n"), (int) targetRouting.size() );
        }

        for( BOARD_ITEM* item : targetRouting )
        {
            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( aTargetArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
            {
                aAffectedItems.insert( item );
                aCommit->Remove( item );
            }
        }

        for( BOARD_ITEM* item : refRouting )
        {
            if( !aRefArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            if( !aTargetArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            BOARD_ITEM* copied = static_cast<BOARD_ITEM*>( item->Clone() );

            copied->Move( disp );
            copied->SetParentGroup( nullptr );
            aGroupableItems.insert( copied );
            aCommit->Add( copied );
        }
    }

    if( aOpts.m_copyOtherItems )
    {
        std::set<BOARD_ITEM*> sourceItems;

        findOtherItemsInRuleArea( aRefArea->m_area, sourceItems );

        for( BOARD_ITEM* item : sourceItems )
        {
            if( !aRefArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            if( !aTargetArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            // Groups that are fully-contained within the area are added themselves; copy their
            // items as part of DeepClone rather than explicitly
            if( item->GetParentGroup() && sourceItems.contains( item->GetParentGroup() )  )
                continue;

            BOARD_ITEM* copied;

            if( item->Type() == PCB_GROUP_T )
            {
                copied = static_cast<PCB_GROUP*>( item )->DeepClone();
            }
            else
            {
                copied = static_cast<BOARD_ITEM*>( item->Clone() );
            }

            copied->ClearFlags();
            copied->SetParentGroup( nullptr );
            copied->Move( disp );
            aGroupableItems.insert( copied );
            aCommit->Add( copied );

            getView()->Query( copied->GetBoundingBox(),
                [&]( KIGFX::VIEW_ITEM* viewItem ) -> bool
                {
                    BOARD_ITEM* existingItem = static_cast<BOARD_ITEM*>( viewItem );

                    if( existingItem && existingItem->Similarity( *copied ) == 1.0 )
                        aCommit->Remove( existingItem );

                    return true;
                } );
        }
    }

    aTargetArea->m_area->RemoveAllContours();
    aTargetArea->m_area->AddPolygon( newTargetOutline );
    aTargetArea->m_area->UnHatchBorder();
    aTargetArea->m_area->HatchBorder();

    if( aOpts.m_copyPlacement )
    {
        for( auto& fpPair : aMatches )
        {
            FOOTPRINT* refFP = fpPair.first;
            FOOTPRINT* targetFP = fpPair.second;

            if( !aRefArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) )
            {
                wxLogTrace( traceMultichannelTool, wxT( "discard ref:%s (ref layer)\n" ),
                            refFP->GetReference() );
                continue;
            }
            if( !aTargetArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) )
            {
                wxLogTrace( traceMultichannelTool, wxT( "discard ref:%s (target layer)\n" ),
                            refFP->GetReference() );
                continue;
            }

            if( targetFP->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            aCommit->Modify( targetFP );

            targetFP->SetLayerAndFlip( refFP->GetLayer() );
            targetFP->SetOrientation( refFP->GetOrientation() );
            VECTOR2I targetPos = refFP->GetPosition() + disp;
            targetFP->SetPosition( targetPos );

            for( PCB_FIELD* refField : refFP->Fields() )
            {
                if( !refField->IsVisible() )
                    continue;

                PCB_FIELD* targetField = targetFP->GetFieldById( refField->GetId() );
                wxCHECK2( targetField, continue );

                targetField->SetAttributes( refField->GetAttributes() );
                targetField->SetPosition( refField->GetPosition() + disp );
                targetField->SetIsKnockout( refField->IsKnockout() );
            }

            aAffectedItems.insert( targetFP );
            aGroupableItems.insert( targetFP );
        }
    }

    return true;
}


bool MULTICHANNEL_TOOL::resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                                   RULE_AREA_COMPAT_DATA& aMatches )
{
    using namespace TMATCH;

    std::unique_ptr<CONNECTION_GRAPH> cgRef ( CONNECTION_GRAPH::BuildFromFootprintSet( aRefArea->m_raFootprints ) );
    std::unique_ptr<CONNECTION_GRAPH> cgTarget ( CONNECTION_GRAPH::BuildFromFootprintSet( aTargetArea->m_raFootprints ) );

    auto status = cgRef->FindIsomorphism( cgTarget.get(), aMatches.m_matchingComponents );

    switch( status )
    {
        case CONNECTION_GRAPH::ST_OK:
            aMatches.m_isOk = true;
            aMatches.m_errorMsg = _("OK");
            break;
        case CONNECTION_GRAPH::ST_EMPTY:
            aMatches.m_isOk = false;
            aMatches.m_errorMsg = _("One or both of the areas has no components assigned.");
            break;
        case CONNECTION_GRAPH::ST_COMPONENT_COUNT_MISMATCH:
            aMatches.m_isOk = false;
            aMatches.m_errorMsg = _("Component count mismatch");
            break;
        case CONNECTION_GRAPH::ST_ITERATION_COUNT_EXCEEDED:
            aMatches.m_isOk = false;
            aMatches.m_errorMsg = _("Iteration count exceeded (timeout)");
            break;
        case CONNECTION_GRAPH::ST_TOPOLOGY_MISMATCH:
            aMatches.m_isOk = false;
            aMatches.m_errorMsg = _("Topology mismatch");
            break;
        default:
            break;
    }

    return ( status == TMATCH::CONNECTION_GRAPH::ST_OK );
}


bool MULTICHANNEL_TOOL::pruneExistingGroups( COMMIT& aCommit,
                                             const std::unordered_set<BOARD_ITEM*>& aItemsToRemove )
{
    for( PCB_GROUP* grp : board()->Groups() )
    {
        std::unordered_set<BOARD_ITEM*>& grpItems = grp->GetItems();
        size_t n_erased = 0;

        for( BOARD_ITEM* refItem : grpItems )
        {
            //printf("check ref %p [%s]\n", refItem, refItem->GetTypeDesc().c_str().AsChar() );
            for( BOARD_ITEM* testItem : aItemsToRemove )
            {
                if( refItem->m_Uuid == testItem->m_Uuid )
                {
                    aCommit.Stage( refItem, CHT_UNGROUP );
                    n_erased++;
                }
            }
        }

        if( n_erased == grpItems.size() )
        {
            aCommit.Stage( grp, CHT_REMOVE );
        }

        //printf("Grp %p items %d pruned %d air %d\n", grp,grpItems.size(), (int) n_erased, (int) aItemsToRemove.size() );
    }

    return false;
}


int MULTICHANNEL_TOOL::AutogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    if( Pgm().IsGUI() )
    {
        QuerySheetsAndComponentClasses();

        if( m_areas.m_areas.size() <= 1 )
        {
            frame()->ShowInfoBarError( _( "Cannot auto-generate any placement areas because the "
                                          "schematic has only one or no hierarchical sheet(s) or "
                                          "component classes." ),
                                       true );
            return 0;
        }

        DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS dialog( frame(), this );
        int ret = dialog.ShowModal();

        if( ret != wxID_OK )
            return 0;
    }

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;
        if( !zone->GetRuleAreaPlacementEnabled() )
            continue;

        std::set<FOOTPRINT*> components;
        identifyComponentsInRuleArea( zone, components );

        if( components.empty() )
            continue;

        for( RULE_AREA& ra : m_areas.m_areas )
        {
            if( components == ra.m_components )
            {
                if( zone->GetRuleAreaPlacementSourceType()
                    == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
                {
                    wxLogTrace(
                            traceMultichannelTool,
                            wxT( "Placement rule area for sheet '%s' already exists as '%s'\n" ),
                            ra.m_sheetPath, zone->GetZoneName() );
                }
                else
                {
                    wxLogTrace( traceMultichannelTool,
                                wxT( "Placement rule area for component class '%s' already exists "
                                     "as '%s'\n" ),
                                ra.m_componentClass, zone->GetZoneName() );
                }

                ra.m_oldArea = zone;
                ra.m_existsAlready = true;
            }
        }
    }

    wxLogTrace( traceMultichannelTool,
             wxT( "%d placement areas found\n" ), (int) m_areas.m_areas.size() );

    BOARD_COMMIT commit( GetManager(), true );

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( !ra.m_generateEnabled )
            continue;

        if( ra.m_existsAlready && !m_areas.m_replaceExisting )
            continue;

        SHAPE_LINE_CHAIN raOutline = buildRAOutline( ra.m_components, 100000 );

        std::unique_ptr<ZONE> newZone( new ZONE( board() ) );

        if( ra.m_sourceType == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
        {
            newZone->SetZoneName(
                    wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_sheetPath ) );
        }
        else
        {
            newZone->SetZoneName(
                    wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_componentClass ) );
        }

        wxLogTrace( traceMultichannelTool, wxT( "Generated rule area '%s' (%d components)\n" ),
                    newZone->GetZoneName(), (int) ra.m_components.size() );

        newZone->SetIsRuleArea( true );
        newZone->SetLayerSet( LSET::AllCuMask() );
        newZone->SetRuleAreaPlacementEnabled( true );
        newZone->SetDoNotAllowCopperPour( false );
        newZone->SetDoNotAllowVias( false );
        newZone->SetDoNotAllowTracks( false );
        newZone->SetDoNotAllowPads( false );
        newZone->SetDoNotAllowFootprints( false );

        if( ra.m_sourceType == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
        {
            newZone->SetRuleAreaPlacementSourceType( RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME );
            newZone->SetRuleAreaPlacementSource( ra.m_sheetPath );
        }
        else
        {
            newZone->SetRuleAreaPlacementSourceType(
                    RULE_AREA_PLACEMENT_SOURCE_TYPE::COMPONENT_CLASS );
            newZone->SetRuleAreaPlacementSource( ra.m_componentClass );
        }

        newZone->AddPolygon( raOutline );
        newZone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );

        if( ra.m_existsAlready )
        {
            commit.Remove( ra.m_oldArea );
        }

        ra.m_area = newZone.get();
        commit.Add( newZone.release() );

    }

    commit.Push( _( "Auto-generate placement rule areas" ) );

    // fixme: handle corner cases where the items belonging to a Rule Area already
    // belong to other groups.

    if( m_areas.m_options.m_groupItems )
    {
        // fixme: sth gets weird when creating new zones & grouping them within a single COMMIT
        BOARD_COMMIT grpCommit( GetManager(), true );

        for( RULE_AREA& ra : m_areas.m_areas )
        {
            if( !ra.m_generateEnabled )
                continue;

            if( ra.m_existsAlready && !m_areas.m_replaceExisting )
                continue;

            std::unordered_set<BOARD_ITEM*> toPrune;

            std::copy( ra.m_components.begin(), ra.m_components.end(),
                       std::inserter( toPrune, toPrune.begin() ) );

            if( ra.m_existsAlready )
                toPrune.insert( ra.m_area );

            pruneExistingGroups( grpCommit, toPrune );

            PCB_GROUP* grp = new PCB_GROUP( board() );

            grpCommit.Add( grp );
            grpCommit.Stage( ra.m_area, CHT_GROUP );

            for( FOOTPRINT* fp : ra.m_components )
            {
                grpCommit.Stage( fp, CHT_GROUP );
            }
        }
        grpCommit.Push( _( "Group components with their placement rule areas" ) );
    }

    return true;
}
