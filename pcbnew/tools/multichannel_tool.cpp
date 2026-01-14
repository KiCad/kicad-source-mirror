/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <pcb_text.h>
#include <component_classes/component_class.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/topo_match.h>
#include <optional>
#include <algorithm>
#include <pcbnew_scripting_helpers.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/pcb_picker_tool.h>
#include <random>
#include <core/profile.h>
#include <wx/log.h>
#include <wx/richmsgdlg.h>
#include <pgm_base.h>


#define MULTICHANNEL_EXTRA_DEBUG

static const wxString traceMultichannelTool = wxT( "MULTICHANNEL_TOOL" );


static void ShowTopologyMismatchReasons( wxWindow* aParent, const wxString& aSummary,
                                         const std::vector<wxString>& aReasons )
{
    if( !aParent || aReasons.empty() )
        return;

    wxString reasonText;

    for( size_t idx = 0; idx < aReasons.size(); ++idx )
    {
        if( idx > 0 )
            reasonText += wxT( "\n" );

        reasonText += aReasons[idx];
    }

    wxRichMessageDialog dlg( aParent, aSummary, _( "Topology mismatch" ), wxICON_ERROR | wxOK );
    dlg.ShowDetailedText( reasonText );
    dlg.ShowModal();
}


MULTICHANNEL_TOOL::MULTICHANNEL_TOOL() : PCB_TOOL_BASE( "pcbnew.Multichannel" )
{
}


MULTICHANNEL_TOOL::~MULTICHANNEL_TOOL()
{

}

void MULTICHANNEL_TOOL::ShowMismatchDetails( wxWindow* aParent, const wxString& aSummary,
                                             const std::vector<wxString>& aReasons ) const
{
    wxWindow* parent = aParent ? aParent : frame();
    ShowTopologyMismatchReasons( parent, aSummary, aReasons );
}


void MULTICHANNEL_TOOL::setTransitions()
{
    Go( &MULTICHANNEL_TOOL::AutogenerateRuleAreas, PCB_ACTIONS::generatePlacementRuleAreas.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::repeatLayout, PCB_ACTIONS::repeatLayout.MakeEvent() );
}


bool MULTICHANNEL_TOOL::findComponentsInRuleArea( RULE_AREA*            aRuleArea,
                                                  std::set<FOOTPRINT*>& aComponents )
{
    if( !aRuleArea || !aRuleArea->m_zone )
        return false;

    // When we're copying the layout of a design block, we are provided an exact list of items
    // rather than querying the board for items that are inside the area.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK )
    {
        // Get all board connected items that are from the design bloc
        for( EDA_ITEM* item : aRuleArea->m_designBlockItems )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
                aComponents.insert( static_cast<FOOTPRINT*>( item ) );
        }

        return (int) aComponents.size();
    }


    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  ctx, preflightCtx;

    auto reportError =
            [&]( const wxString& aMessage, int aOffset )
            {
                wxLogTrace( traceMultichannelTool, wxT( "ERROR: %s"), aMessage );
            };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );
    //compiler.SetDebugReporter( m_reporter );

    wxLogTrace( traceMultichannelTool, wxT( "rule area '%s'" ), aRuleArea->m_zone->GetZoneName() );

    wxString ruleText;

    switch( aRuleArea->m_zone->GetPlacementAreaSourceType() )
    {
    case PLACEMENT_SOURCE_T::SHEETNAME:
        ruleText = wxT( "A.memberOfSheetOrChildren('" ) + aRuleArea->m_zone->GetPlacementAreaSource() + wxT( "')" );
        break;
    case PLACEMENT_SOURCE_T::COMPONENT_CLASS:
        ruleText = wxT( "A.hasComponentClass('" ) + aRuleArea->m_zone->GetPlacementAreaSource() + wxT( "')" );
        break;
    case PLACEMENT_SOURCE_T::GROUP_PLACEMENT:
        ruleText = wxT( "A.memberOfGroup('" ) + aRuleArea->m_zone->GetPlacementAreaSource() + wxT( "')" );
        break;
    case PLACEMENT_SOURCE_T::DESIGN_BLOCK:
        // For design blocks, handled above outside the rules system
        break;
    }

    auto ok = compiler.Compile( ruleText, &ucode, &preflightCtx );

    if( !ok )
        return false;

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        ctx.SetItems( fp, fp );
        LIBEVAL::VALUE* val = ucode.Run( &ctx );

        if( val->AsDouble() != 0.0 )
        {
            wxLogTrace( traceMultichannelTool, wxT( " - %s [sheet %s]" ),
                        fp->GetReference(),
                        fp->GetSheetname() );

            aComponents.insert( fp );
        }
    }

    return true;
}


bool MULTICHANNEL_TOOL::findOtherItemsInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_ITEM*>& aItems )
{
    if( !aRuleArea || !aRuleArea->m_zone )
        return false;

    // When we're copying the layout of a design block, we are provided an exact list of items
    // rather than querying the board for items that are inside the area.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK )
    {
        // Get all board items that aren't footprints or connected items,
        // since they'll be handled in the the other findXInRuleArea routines
        for( EDA_ITEM* item : aRuleArea->m_designBlockItems )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
                continue;

            if( BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( item ) )
            {
                if( !boardItem->IsConnected() )
                    aItems.insert( boardItem );
            }
        }

        return aItems.size() > 0;
    }

    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  ctx, preflightCtx;

    auto reportError =
            [&]( const wxString& aMessage, int aOffset )
            {
                wxLogTrace( traceMultichannelTool, wxT( "ERROR: %s"), aMessage );
            };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );

    // Use the zone's UUID to identify it uniquely. Using the zone name could match other zones
    // with the same name (e.g., a copper fill zone with the same name as a rule area).
    wxString ruleText = wxString::Format( wxT( "A.enclosedByArea('%s')" ),
                                          aRuleArea->m_zone->m_Uuid.AsString() );

    if( !compiler.Compile( ruleText, &ucode, &preflightCtx ) )
        return false;

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
        if( zone == aRuleArea->m_zone )
            continue;

        testAndAdd( zone );
    }

    for( BOARD_ITEM* drawing : board()->Drawings() )
    {
        if( !drawing->IsConnected() )
            testAndAdd( drawing );
    }

    return true;
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInSheet( wxString aSheetName ) const
{
    std::set<FOOTPRINT*> rv;

    if( aSheetName.EndsWith( wxT( "/" ) ) )
        aSheetName.RemoveLast();

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        auto sn = fp->GetSheetname();
        if( sn.EndsWith( wxT( "/" ) ) )
            sn.RemoveLast();

        if( sn == aSheetName )
            rv.insert( fp );
    }

    return rv;
}


std::set<FOOTPRINT*>
MULTICHANNEL_TOOL::queryComponentsInComponentClass( const wxString& aComponentClassName ) const
{
    std::set<FOOTPRINT*> rv;

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        if( fp->GetComponentClass()->ContainsClassName( aComponentClassName ) )
            rv.insert( fp );
    }

    return rv;
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInGroup( const wxString& aGroupName ) const
{
    std::set<FOOTPRINT*> rv;

    for( PCB_GROUP* group : board()->Groups() )
    {
        if( group->GetName() == aGroupName )
        {
            for( EDA_ITEM* item : group->GetItems() )
            {
                if( item->Type() == PCB_FOOTPRINT_T )
                    rv.insert( static_cast<FOOTPRINT*>( item ) );
            }
        }
    }

    return rv;
}


const SHAPE_LINE_CHAIN MULTICHANNEL_TOOL::buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin )
{
    std::vector<VECTOR2I> bbCorners;
    bbCorners.reserve( aFootprints.size() * 4 );

    for( FOOTPRINT* fp : aFootprints )
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


void MULTICHANNEL_TOOL::GeneratePotentialRuleAreas()
{
    using PathAndName = std::pair<wxString, wxString>;
    std::set<PathAndName> uniqueSheets;
    std::set<wxString>    uniqueComponentClasses;
    std::set<wxString>    uniqueGroups;

    m_areas.m_areas.clear();

    for( const FOOTPRINT* fp : board()->Footprints() )
    {
        uniqueSheets.insert( PathAndName( fp->GetSheetname(), fp->GetSheetfile() ) );

        const COMPONENT_CLASS* compClass = fp->GetComponentClass();

        for( const COMPONENT_CLASS* singleClass : compClass->GetConstituentClasses() )
            uniqueComponentClasses.insert( singleClass->GetName() );

        if( fp->GetParentGroup() && !fp->GetParentGroup()->GetName().IsEmpty() )
            uniqueGroups.insert( fp->GetParentGroup()->GetName() );
    }

    for( const PathAndName& sheet : uniqueSheets )
    {
        RULE_AREA ent;

        ent.m_sourceType = PLACEMENT_SOURCE_T::SHEETNAME;
        ent.m_generateEnabled = false;
        ent.m_sheetPath = sheet.first;
        ent.m_sheetName = sheet.second;
        ent.m_components = queryComponentsInSheet( ent.m_sheetPath );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT("found sheet '%s' @ '%s' s %d\n"),
                    ent.m_sheetName,
                    ent.m_sheetPath,
                    (int) m_areas.m_areas.size() );
    }

    for( const wxString& compClass : uniqueComponentClasses )
    {
        RULE_AREA ent;

        ent.m_sourceType = PLACEMENT_SOURCE_T::COMPONENT_CLASS;
        ent.m_generateEnabled = false;
        ent.m_componentClass = compClass;
        ent.m_components = queryComponentsInComponentClass( ent.m_componentClass );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT( "found component class '%s' s %d\n" ),
                    ent.m_componentClass,
                    static_cast<int>( m_areas.m_areas.size() ) );
    }

    for( const wxString& groupName : uniqueGroups )
    {
        RULE_AREA ent;

        ent.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
        ent.m_generateEnabled = false;
        ent.m_groupName = groupName;
        ent.m_components = queryComponentsInGroup( ent.m_groupName );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT( "found group '%s' s %d\n" ),
                    ent.m_componentClass,
                    static_cast<int>( m_areas.m_areas.size() ) );
    }
}


void MULTICHANNEL_TOOL::FindExistingRuleAreas()
{
    m_areas.m_areas.clear();

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;

        if( !zone->GetPlacementAreaEnabled() )
            continue;

        RULE_AREA area;

        area.m_existsAlready = true;
        area.m_zone = zone;
        area.m_ruleName = zone->GetZoneName();
        area.m_center = zone->Outline()->COutline( 0 ).Centre();

        findComponentsInRuleArea( &area, area.m_components );

        m_areas.m_areas.push_back( area );

        wxLogTrace( traceMultichannelTool, wxT( "RA '%s', %d footprints\n" ), area.m_ruleName,
                    (int) area.m_components.size() );
    }

    wxLogTrace( traceMultichannelTool, wxT( "Total RAs found: %d\n" ), (int) m_areas.m_areas.size() );
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

    auto isSelectedItemAnRA =
            []( EDA_ITEM* aItem ) -> ZONE*
            {
                if( !aItem || aItem->Type() != PCB_ZONE_T )
                    return nullptr;

                ZONE* zone = static_cast<ZONE*>( aItem );

                if( !zone->GetIsRuleArea() )
                    return nullptr;

                if( !zone->GetPlacementAreaEnabled() )
                    return nullptr;

                return zone;
            };

    for( EDA_ITEM* item : selection() )
    {
        if( ZONE* zone = isSelectedItemAnRA( item ) )
        {
            refRAs.push_back( zone );
        }
        else if( item->Type() == PCB_GROUP_T )
        {
            PCB_GROUP *group = static_cast<PCB_GROUP*>( item );

            for( EDA_ITEM* grpItem : group->GetItems() )
            {
                if( ZONE* grpZone = isSelectedItemAnRA( grpItem ) )
                    refRAs.push_back( grpZone );
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
        frame()->ShowInfoBarError( _( "No Rule Areas to repeat layout to have been found." ), true );
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
        if( ra.m_zone == aRefZone )
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
        if( ra.m_zone == m_areas.m_refRA->m_zone )
            continue;

        m_areas.m_compatMap[&ra] = RULE_AREA_COMPAT_DATA();

        resolveConnectionTopology( m_areas.m_refRA, &ra, m_areas.m_compatMap[&ra] );
    }

    return 0;
}


int MULTICHANNEL_TOOL::RepeatLayout( const TOOL_EVENT& aEvent, RULE_AREA& aRefArea, RULE_AREA& aTargetArea )
{
    RULE_AREA_COMPAT_DATA compat;

    if( !resolveConnectionTopology( &aRefArea, &aTargetArea, compat ) )
    {
        if( Pgm().IsGUI() )
        {
            wxString summary = wxString::Format( _( "Rule Area topologies do not match: %s" ), compat.m_errorMsg );
            ShowTopologyMismatchReasons( frame(), summary, compat.m_mismatchReasons );
        }

        return -1;
    }

    BOARD_COMMIT commit( GetManager(), true, false );

    if( !copyRuleAreaContents( &aRefArea, &aTargetArea, &commit, m_areas.m_options, compat ) )
    {
        auto errMsg = wxString::Format( _( "Copy Rule Area contents failed between rule areas '%s' and '%s'." ),
                                        m_areas.m_refRA->m_zone->GetZoneName(),
                                        aTargetArea.m_zone->GetZoneName() );

        commit.Revert();

        if( Pgm().IsGUI() )
            frame()->ShowInfoBarError( errMsg, true );

        return -1;
    }

    if( aTargetArea.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
    {
        if( aTargetArea.m_components.size() == 0 || !( *aTargetArea.m_components.begin() )->GetParentGroup() )
        {
            commit.Revert();

            if( Pgm().IsGUI() )
                frame()->ShowInfoBarError( _( "Target group does not have a group." ), true );

            return -1;
        }

        EDA_GROUP* group = ( *aTargetArea.m_components.begin() )->GetParentGroup();

        commit.Add( group->AsEdaItem() );

        for( BOARD_ITEM* item : compat.m_groupableItems )
        {
            commit.Modify( item );
            group->AddItem( item );
        }
    }

    commit.Push( _( "Repeat layout" ) );

    return 0;
}


int MULTICHANNEL_TOOL::RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone )
{
    int totalCopied = 0;

    BOARD_COMMIT commit( GetManager(), true, false );

    for( auto& [targetArea, compatData] : m_areas.m_compatMap )
    {
        if( !compatData.m_doCopy )
        {
            wxLogTrace( traceMultichannelTool, wxT( "skipping copy to RA '%s' (disabled in dialog)\n" ),
                        targetArea->m_ruleName );
            continue;
        }

        if( !compatData.m_isOk )
            continue;

        if( !copyRuleAreaContents( m_areas.m_refRA, targetArea, &commit, m_areas.m_options, compatData ) )
        {
            auto errMsg = wxString::Format( _( "Copy Rule Area contents failed between rule areas '%s' and '%s'." ),
                                            m_areas.m_refRA->m_zone->GetZoneName(),
                                            targetArea->m_zone->GetZoneName() );

            commit.Revert();

            if( Pgm().IsGUI() )
                frame()->ShowInfoBarError( errMsg, true );

            return -1;
        }

        totalCopied++;
        wxSafeYield();
    }

    if( m_areas.m_options.m_groupItems )
    {
        for( const auto& [targetArea, compatData] : m_areas.m_compatMap )
        {
            pruneExistingGroups( commit, compatData.m_affectedItems );

            PCB_GROUP* group = new PCB_GROUP( board() );

            commit.Add( group );

            for( BOARD_ITEM* item : compatData.m_groupableItems )
            {
                commit.Modify( item );
                group->AddItem( item );
            }
        }
    }

    commit.Push( _( "Repeat layout" ) );

    if( Pgm().IsGUI() )
        frame()->ShowInfoBarMsg( wxString::Format( _( "Copied to %d Rule Areas." ), totalCopied ), true );

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


int MULTICHANNEL_TOOL::findRoutingInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_CONNECTED_ITEM*>& aOutput,
                                              std::shared_ptr<CONNECTIVITY_DATA> aConnectivity,
                                              const SHAPE_POLY_SET& aRAPoly, const REPEAT_LAYOUT_OPTIONS& aOpts ) const
{
    if( !aRuleArea || !aRuleArea->m_zone )
        return 0;

    // The user also will consider tracks and vias that are inside the source area but
    // not connected to any of the source pads to count as "routing" (e.g. stitching vias)

    int count = 0;

    // When we're copying the layout of a design block, we are provided an exact list of items
    // rather than querying the board for items that are inside the area.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK )
    {
        // Get all board connected items that are from the design block, except pads,
        // which shouldn't be copied
        for( EDA_ITEM* item : aRuleArea->m_designBlockItems )
        {
            // Include any connected items except pads.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            {
                if( bci->IsConnected() )
                    aOutput.insert( bci );
            }
        }

        return (int) aOutput.size();
    }

    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  ctx, preflightCtx;

    auto reportError =
            [&]( const wxString& aMessage, int aOffset )
            {
                wxLogTrace( traceMultichannelTool, wxT( "ERROR: %s" ), aMessage );
            };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );

    // Use the zone's UUID to identify it uniquely. Using the zone name could match other zones
    // with the same name (e.g., a copper fill zone with the same name as a rule area).
    wxString ruleText = wxString::Format( wxT( "A.enclosedByArea('%s')" ),
                                          aRuleArea->m_zone->m_Uuid.AsString() );

    auto testAndAdd =
            [&]( BOARD_CONNECTED_ITEM* aItem )
            {
                if( aOutput.contains( aItem ) )
                    return;

                ctx.SetItems( aItem, aItem );
                LIBEVAL::VALUE* val = ucode.Run( &ctx );

                if( val->AsDouble() != 0.0 )
                {
                    aOutput.insert( aItem );
                    count++;
                }
            };

    if( compiler.Compile( ruleText, &ucode, &preflightCtx ) )
    {
        for( PCB_TRACK* track : board()->Tracks() )
            testAndAdd( track );

        for( BOARD_ITEM* drawing : board()->Drawings() )
        {
            if( drawing->IsConnected() )
                testAndAdd( static_cast<BOARD_CONNECTED_ITEM*>( drawing ) );
        }
    }

    return count;
}


bool MULTICHANNEL_TOOL::copyRuleAreaContents( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                              BOARD_COMMIT* aCommit, REPEAT_LAYOUT_OPTIONS aOpts,
                                              RULE_AREA_COMPAT_DATA& aCompatData )
{
    // copy RA shapes first
    SHAPE_LINE_CHAIN refOutline = aRefArea->m_zone->Outline()->COutline( 0 );
    SHAPE_LINE_CHAIN targetOutline = aTargetArea->m_zone->Outline()->COutline( 0 );

    FOOTPRINT* targetAnchorFp = nullptr;
    VECTOR2I disp = aTargetArea->m_center - aRefArea->m_center;
    EDA_ANGLE rot = EDA_ANGLE( 0 );

    if( aOpts.m_anchorFp )
    {
        for( const auto& [refFP, targetFP] : aCompatData.m_matchingComponents )
        {
            if( refFP->GetReference() == aOpts.m_anchorFp->GetReference() )
                targetAnchorFp = targetFP;
        }

        if( targetAnchorFp )
        {
            VECTOR2I oldpos = aOpts.m_anchorFp->GetPosition();
            rot = EDA_ANGLE( targetAnchorFp->GetOrientationDegrees() - aOpts.m_anchorFp->GetOrientationDegrees() );
            aOpts.m_anchorFp->Rotate( VECTOR2( 0, 0 ), EDA_ANGLE( rot ) );
            oldpos = aOpts.m_anchorFp->GetPosition();
            VECTOR2I newpos = targetAnchorFp->GetPosition();
            disp = newpos - oldpos;
            aOpts.m_anchorFp->Rotate( VECTOR2( 0, 0 ), EDA_ANGLE( -rot ) );
        }
    }

    SHAPE_POLY_SET refPoly;
    refPoly.AddOutline( refOutline );
    refPoly.CacheTriangulation( false );

    SHAPE_POLY_SET targetPoly;

    SHAPE_LINE_CHAIN newTargetOutline( refOutline );
    newTargetOutline.Rotate( rot, VECTOR2( 0, 0 ) );
    newTargetOutline.Move( disp );
    targetPoly.AddOutline( newTargetOutline );
    targetPoly.CacheTriangulation( false );

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
    std::map<EDA_GROUP*, EDA_GROUP*>   groupMap;

    auto fixupParentGroup =
            [&]( BOARD_ITEM* sourceItem, BOARD_ITEM* destItem )
            {
                if( EDA_GROUP* parentGroup = sourceItem->GetParentGroup() )
                {
                    if( !groupMap.contains( parentGroup ) )
                    {
                        BOARD_ITEM* newGroup = static_cast<PCB_GROUP*>( parentGroup->AsEdaItem() )->Duplicate( false );
                        groupMap[parentGroup] = static_cast<PCB_GROUP*>( newGroup );
                        aCommit->Add( newGroup );
                    }

                    destItem->SetParentGroup( groupMap[parentGroup] );
                }
            };

    // Only stage changes for a target Rule Area zone if it actually belongs to the board.
    // In some workflows (e.g. ApplyDesignBlockLayout), the target area is a temporary zone
    // and is not added to the BOARD.
    bool targetZoneOnBoard = false;

    if( aTargetArea->m_zone )
    {
        for( ZONE* z : board()->Zones() )
        {
            if( z == aTargetArea->m_zone )
            {
                targetZoneOnBoard = true;
                break;
            }
        }
    }

    if( targetZoneOnBoard )
    {
        aCommit->Modify( aTargetArea->m_zone );
        aCompatData.m_affectedItems.insert( aTargetArea->m_zone );
        aCompatData.m_groupableItems.insert( aTargetArea->m_zone );
    }

    if( aOpts.m_copyRouting )
    {
        std::set<BOARD_CONNECTED_ITEM*> refRouting;
        std::set<BOARD_CONNECTED_ITEM*> targetRouting;

        wxLogTrace( traceMultichannelTool, wxT( "copying routing: %d fps\n" ),
                    (int) aCompatData.m_matchingComponents.size() );

        std::set<int> refc;
        std::set<int> targc;

        for( const auto& [refFP, targetFP] : aCompatData.m_matchingComponents )
        {
            for( PAD* pad : refFP->Pads() )
                refc.insert( pad->GetNetCode() );

            for( PAD* pad : targetFP->Pads() )
                targc.insert( pad->GetNetCode() );
        }

        findRoutingInRuleArea( aTargetArea, targetRouting, connectivity, targetPoly, aOpts );
        findRoutingInRuleArea( aRefArea, refRouting, connectivity, refPoly, aOpts );

        for( BOARD_CONNECTED_ITEM* item : targetRouting )
        {
            // Never remove pads as part of routing copy.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            if( aOpts.m_connectedRoutingOnly && !targc.contains( item->GetNetCode() ) )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
            {
                aCompatData.m_affectedItems.insert( item );
                aCommit->Remove( item );
            }
        }

        for( BOARD_CONNECTED_ITEM* item : refRouting )
        {
            // Never copy pads as part of routing copy.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            if( aOpts.m_connectedRoutingOnly && !refc.contains( item->GetNetCode() ) )
                continue;

            if( !aRefArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            if( !aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            BOARD_CONNECTED_ITEM* copied = static_cast<BOARD_CONNECTED_ITEM*>( item->Duplicate( false ) );

            fixupNet( item, copied, aCompatData.m_matchingComponents );
            fixupParentGroup( item, copied );

            copied->Rotate( VECTOR2( 0, 0 ), rot );
            copied->Move( disp );
            aCompatData.m_groupableItems.insert( copied );
            aCommit->Add( copied );
        }
    }

    if( aOpts.m_copyOtherItems )
    {
        std::set<BOARD_ITEM*> sourceItems;
        std::set<BOARD_ITEM*> targetItems;

        findOtherItemsInRuleArea( aRefArea, sourceItems );
        findOtherItemsInRuleArea( aTargetArea, targetItems );

        for( BOARD_ITEM* item : targetItems )
        {
            if( item->Type() == PCB_TEXT_T && item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( item->Type() == PCB_ZONE_T )
            {
                ZONE* zone = static_cast<ZONE*>( item );

                // Check all zone layers are included in the rule area
                bool layerMismatch = false;
                LSET zoneLayers = zone->GetLayerSet();

                for( const PCB_LAYER_ID& layer : zoneLayers )
                {
                    if( !aTargetArea->m_zone->GetLayerSet().Contains( layer ) )
                        layerMismatch = true;
                }

                if( !layerMismatch )
                {
                    aCompatData.m_affectedItems.insert( zone );
                    aCommit->Remove( zone );
                }
            }
            else
            {
                if( aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                {
                    aCompatData.m_affectedItems.insert( item );
                    aCommit->Remove( item );
                }
            }
        }

        for( BOARD_ITEM* item : sourceItems )
        {
            if( item->Type() == PCB_TEXT_T && item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            BOARD_ITEM* copied = nullptr;

            if( item->Type() == PCB_ZONE_T )
            {
                ZONE* zone = static_cast<ZONE*>( item );

                // Check all zone layers are included in the rule area
                bool layerMismatch = false;
                LSET zoneLayers = zone->GetLayerSet();

                for( const PCB_LAYER_ID& layer : zoneLayers )
                {
                    if( !aRefArea->m_zone->GetLayerSet().Contains( layer )
                        || !aTargetArea->m_zone->GetLayerSet().Contains( layer ) )
                    {
                        layerMismatch = true;
                    }
                }

                if( layerMismatch )
                    continue;

                ZONE* targetZone = static_cast<ZONE*>( item->Duplicate( false ) );
                fixupNet( zone, targetZone, aCompatData.m_matchingComponents );

                copied = targetZone;
            }
            else
            {
                if( !aRefArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                    continue;

                if( !aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                    continue;

                copied = static_cast<BOARD_ITEM*>( item->Clone() );
            }

            if( copied )
            {
                fixupParentGroup( item, copied );

                copied->ClearFlags();
                copied->Rotate( VECTOR2( 0, 0 ), rot );
                copied->Move( disp );
                aCompatData.m_groupableItems.insert( copied );
                aCommit->Add( copied );
            }
        }
    }

    if( aOpts.m_copyPlacement )
    {
        for( const auto& [refFP, targetFP] : aCompatData.m_matchingComponents )
        {
            if( !aRefArea->m_zone->GetLayerSet().Contains( refFP->GetLayer() ) )
            {
                wxLogTrace( traceMultichannelTool, wxT( "discard ref:%s (ref layer)\n" ),
                            refFP->GetReference() );
                continue;
            }
            if( !aTargetArea->m_zone->GetLayerSet().Contains( refFP->GetLayer() ) )
            {
                wxLogTrace( traceMultichannelTool, wxT( "discard ref:%s (target layer)\n" ),
                            refFP->GetReference() );
                continue;
            }

            // Ignore footprints outside of the rule area
            if( !refFP->GetEffectiveShape( refFP->GetLayer() )->Collide( &refPoly, 0 ) )
                continue;

            if( targetFP->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            aCommit->Modify( targetFP );

            targetFP->SetLayerAndFlip( refFP->GetLayer() );
            targetFP->SetOrientation( refFP->GetOrientation() );
            targetFP->SetPosition( refFP->GetPosition() );
            targetFP->Rotate( VECTOR2( 0, 0 ), rot );
            targetFP->Move( disp );

            for( PCB_FIELD* refField : refFP->GetFields() )
            {
                wxCHECK2( refField, continue );

                PCB_FIELD* targetField = targetFP->GetField( refField->GetName() );
                wxCHECK2( targetField, continue );

                targetField->SetLayerSet( refField->GetLayerSet() );
                targetField->SetVisible( refField->IsVisible() );
                targetField->SetAttributes( refField->GetAttributes() );
                targetField->SetPosition( refField->GetPosition() );
                targetField->Rotate( VECTOR2( 0, 0 ), rot );
                targetField->Move( disp );
                targetField->SetIsKnockout( refField->IsKnockout() );
            }

            // Copy non-field text items (user-added text on the footprint)
            for( BOARD_ITEM* refItem : refFP->GraphicalItems() )
            {
                if( refItem->Type() != PCB_TEXT_T )
                    continue;

                PCB_TEXT* refText = static_cast<PCB_TEXT*>( refItem );

                for( BOARD_ITEM* targetItem : targetFP->GraphicalItems() )
                {
                    if( targetItem->Type() != PCB_TEXT_T )
                        continue;

                    PCB_TEXT* targetText = static_cast<PCB_TEXT*>( targetItem );

                    // Match text items by their text content
                    if( targetText->GetText() == refText->GetText() )
                    {
                        targetText->SetLayer( refText->GetLayer() );
                        targetText->SetVisible( refText->IsVisible() );
                        targetText->SetAttributes( refText->GetAttributes() );
                        targetText->SetPosition( refText->GetPosition() );
                        targetText->Rotate( VECTOR2( 0, 0 ), rot );
                        targetText->Move( disp );
                        targetText->SetIsKnockout( refText->IsKnockout() );
                        break;
                    }
                }
            }

            // Copy 3D model settings
            targetFP->Models() = refFP->Models();

            aCompatData.m_affectedItems.insert( targetFP );
            aCompatData.m_groupableItems.insert( targetFP );
        }
    }

    aTargetArea->m_zone->RemoveAllContours();
    aTargetArea->m_zone->AddPolygon( newTargetOutline );
    aTargetArea->m_zone->UnHatchBorder();
    aTargetArea->m_zone->HatchBorder();

    return true;
}

/**
 * @brief Attempts to make sure copied items are assigned the right net
 *
 */
void MULTICHANNEL_TOOL::fixupNet( BOARD_CONNECTED_ITEM* aRef, BOARD_CONNECTED_ITEM* aTarget,
                                  TMATCH::COMPONENT_MATCHES& aComponentMatches )
{
    auto                                     connectivity = board()->GetConnectivity();
    const std::vector<BOARD_CONNECTED_ITEM*> refConnectedPads = connectivity->GetNetItems( aRef->GetNetCode(),
                                                                                           { PCB_PAD_T } );

    for( const BOARD_CONNECTED_ITEM* refConItem : refConnectedPads )
    {
        if( refConItem->Type() != PCB_PAD_T )
            continue;

        const PAD* refPad = static_cast<const PAD*>( refConItem );
        FOOTPRINT* sourceFootprint = refPad->GetParentFootprint();

        if( aComponentMatches.contains( sourceFootprint ) )
        {
            const FOOTPRINT*        targetFootprint = aComponentMatches[sourceFootprint];
            std::vector<const PAD*> targetFpPads = targetFootprint->GetPads( refPad->GetNumber() );

            if( !targetFpPads.empty() )
            {
                int targetNetCode = targetFpPads[0]->GetNet()->GetNetCode();
                aTarget->SetNetCode( targetNetCode );

                break;
            }
        }
    }
}


bool MULTICHANNEL_TOOL::resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                                   RULE_AREA_COMPAT_DATA& aMatches )
{
    using namespace TMATCH;

    std::unique_ptr<CONNECTION_GRAPH> cgRef ( CONNECTION_GRAPH::BuildFromFootprintSet( aRefArea->m_components ) );
    std::unique_ptr<CONNECTION_GRAPH> cgTarget ( CONNECTION_GRAPH::BuildFromFootprintSet( aTargetArea->m_components ) );

    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> mismatchReasons;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), aMatches.m_matchingComponents, mismatchReasons );

    aMatches.m_isOk = status;

    if( status )
    {
        aMatches.m_errorMsg = _( "OK" );
        aMatches.m_mismatchReasons.clear();
        return true;
    }

    aMatches.m_mismatchReasons.clear();

    for( const auto& reason : mismatchReasons )
    {
        if( reason.m_reason.IsEmpty() )
            continue;

        if( !reason.m_reference.IsEmpty() && !reason.m_candidate.IsEmpty() )
        {
            aMatches.m_mismatchReasons.push_back( wxString::Format( wxT( "%s -> %s: %s" ),
                                                                    reason.m_reference,
                                                                    reason.m_candidate,
                                                                    reason.m_reason ) );
        }
        else if( !reason.m_reference.IsEmpty() )
        {
            aMatches.m_mismatchReasons.push_back( wxString::Format( wxT( "%s: %s" ),
                                                                    reason.m_reference,
                                                                    reason.m_reason ) );
        }
        else
            aMatches.m_mismatchReasons.push_back( reason.m_reason );
    }

    if( aMatches.m_mismatchReasons.empty() )
        aMatches.m_mismatchReasons.push_back( _( "Topology mismatch" ) );

    aMatches.m_errorMsg = aMatches.m_mismatchReasons.front();

    return status;
}


bool MULTICHANNEL_TOOL::pruneExistingGroups( COMMIT& aCommit,
                                             const std::unordered_set<BOARD_ITEM*>& aItemsToRemove )
{
    // Note: groups are only collections, not "real" hierarchy.  A group's members are still parented
    // by the board (and therefore nested groups are still in the board's list of groups).
    for( PCB_GROUP* group : board()->Groups() )
    {
        std::vector<EDA_ITEM*> pruneList;

        for( EDA_ITEM* refItem : group->GetItems() )
        {
            for( BOARD_ITEM* testItem : aItemsToRemove )
            {
                if( refItem->m_Uuid == testItem->m_Uuid )
                    pruneList.push_back( refItem );
            }
        }

        if( !pruneList.empty() )
        {
            aCommit.Modify( group );

            for( EDA_ITEM* item : pruneList )
                group->RemoveItem( item );

            if( group->GetItems().empty() )
                aCommit.Remove( group );
        }
    }

    return false;
}


int MULTICHANNEL_TOOL::AutogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    if( Pgm().IsGUI() )
    {
        GeneratePotentialRuleAreas();

        if( m_areas.m_areas.size() <= 1 )
        {
            frame()->ShowInfoBarError( _( "Cannot auto-generate any placement areas because the "
                                          "schematic has only one or no hierarchical sheets, "
                                          "groups, or component classes." ),
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

        if( !zone->GetPlacementAreaEnabled() )
            continue;

        std::set<FOOTPRINT*> components;
        RULE_AREA            zoneRA;
        zoneRA.m_zone = zone;
        zoneRA.m_sourceType = zone->GetPlacementAreaSourceType();
        findComponentsInRuleArea( &zoneRA, components );

        if( components.empty() )
            continue;

        for( RULE_AREA& ra : m_areas.m_areas )
        {
            if( components == ra.m_components )
            {
                if( zone->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::SHEETNAME )
                {
                    wxLogTrace( traceMultichannelTool,
                                wxT( "Placement rule area for sheet '%s' already exists as '%s'\n" ),
                                ra.m_sheetPath, zone->GetZoneName() );
                }
                else if( zone->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
                {
                    wxLogTrace( traceMultichannelTool,
                                wxT( "Placement rule area for component class '%s' already exists as '%s'\n" ),
                                ra.m_componentClass, zone->GetZoneName() );
                }
                else
                {
                    wxLogTrace( traceMultichannelTool,
                                wxT( "Placement rule area for group '%s' already exists as '%s'\n" ),
                                ra.m_groupName, zone->GetZoneName() );
                }

                ra.m_oldZone = zone;
                ra.m_existsAlready = true;
            }
        }
    }

    wxLogTrace( traceMultichannelTool, wxT( "%d placement areas found\n" ), (int) m_areas.m_areas.size() );

    BOARD_COMMIT commit( GetManager(), true, false );

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( !ra.m_generateEnabled )
            continue;

        if( ra.m_existsAlready && !m_areas.m_replaceExisting )
            continue;

        if( ra.m_components.empty() )
            continue;

        SHAPE_LINE_CHAIN raOutline = buildRAOutline( ra.m_components, 100000 );

        std::unique_ptr<ZONE> newZone( new ZONE( board() ) );

        if( ra.m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
            newZone->SetZoneName( wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_sheetPath ) );
        else if( ra.m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
            newZone->SetZoneName( wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_componentClass ) );
        else
            newZone->SetZoneName( wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_groupName ) );

        wxLogTrace( traceMultichannelTool, wxT( "Generated rule area '%s' (%d components)\n" ),
                    newZone->GetZoneName(),
                    (int) ra.m_components.size() );

        newZone->SetIsRuleArea( true );
        newZone->SetLayerSet( LSET::AllCuMask() );
        newZone->SetPlacementAreaEnabled( true );
        newZone->SetDoNotAllowZoneFills( false );
        newZone->SetDoNotAllowVias( false );
        newZone->SetDoNotAllowTracks( false );
        newZone->SetDoNotAllowPads( false );
        newZone->SetDoNotAllowFootprints( false );

        if( ra.m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
        {
            newZone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::SHEETNAME );
            newZone->SetPlacementAreaSource( ra.m_sheetPath );
        }
        else if( ra.m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
        {
            newZone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::COMPONENT_CLASS );
            newZone->SetPlacementAreaSource( ra.m_componentClass );
        }
        else
        {
            newZone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
            newZone->SetPlacementAreaSource( ra.m_groupName );
        }

        newZone->AddPolygon( raOutline );
        newZone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );

        if( ra.m_existsAlready )
        {
            commit.Remove( ra.m_oldZone );
        }

        ra.m_zone = newZone.release();
        commit.Add( ra.m_zone );
    }

    // fixme: handle corner cases where the items belonging to a Rule Area already
    // belong to other groups.

    if( m_areas.m_options.m_groupItems )
    {
        for( RULE_AREA& ra : m_areas.m_areas )
        {
            if( !ra.m_generateEnabled )
                continue;

            if( ra.m_existsAlready && !m_areas.m_replaceExisting )
                continue;

            std::unordered_set<BOARD_ITEM*> toPrune;

            std::copy( ra.m_components.begin(), ra.m_components.end(), std::inserter( toPrune, toPrune.begin() ) );

            if( ra.m_existsAlready )
                toPrune.insert( ra.m_zone );

            pruneExistingGroups( commit, toPrune );

            PCB_GROUP* group = new PCB_GROUP( board() );

            commit.Add( group );

            commit.Modify( ra.m_zone );
            group->AddItem( ra.m_zone );

            for( FOOTPRINT* fp : ra.m_components )
            {
                commit.Modify( fp );
                group->AddItem( fp );
            }
        }
    }

    commit.Push( _( "Auto-generate placement rule areas" ) );

    return true;
}
