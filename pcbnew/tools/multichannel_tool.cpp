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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <board_commit.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection.h>
#include <tools/pcb_selection_tool.h>
#include <tool/action_menu.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_menu.h>
#include <bitmaps.h>
#include <eda_group.h>

#include <dialogs/dialog_multichannel_generate_rule_areas.h>
#include <dialogs/dialog_multichannel_repeat_layout.h>

#include "multichannel_tool.h"

#include <pcbexpr_evaluator.h>

#include <zone.h>
#include <board.h>
#include <netinfo.h>
#include <board_design_settings.h>
#include <geometry/convex_hull.h>
#include <geometry/shape_utils.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_text.h>
#include <component_classes/component_class.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/topo_match.h>
#include <algorithm>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/pcb_picker_tool.h>
#include <tools/generator_tool.h>
#include <router/router_tool.h>
#include <router/pns_router.h>
#include <geometry/shape_rect.h>
#include <scoped_set_reset.h>
#include <trigo.h>
#include <chrono>
#include <future>
#include <core/profile.h>
#include <core/raii.h>
#include <widgets/wx_progress_reporters.h>
#include <string_utils.h>
#include <wx/log.h>
#include <wx/richmsgdlg.h>
#include <pgm_base.h>


#define MULTICHANNEL_EXTRA_DEBUG

static const wxString traceMultichannelTool = wxT( "MULTICHANNEL_TOOL" );


static wxString FormatComponentList( const std::set<FOOTPRINT*>& aComponents )
{
    std::vector<wxString> refs;

    for( FOOTPRINT* fp : aComponents )
    {
        if( !fp )
            continue;

        refs.push_back( fp->GetReferenceAsString() );
    }

    std::sort( refs.begin(), refs.end(),
               []( const wxString& aLhs, const wxString& aRhs )
               {
                   return aLhs.CmpNoCase( aRhs ) < 0;
               } );

    if( refs.empty() )
        return _( "(none)" );

    wxString result;
    wxString line;
    size_t   componentsOnLine = 0;

    for( const wxString& ref : refs )
    {
        if( componentsOnLine == 10 )
        {
            if( !result.IsEmpty() )
                result += wxT( "\n" );

            result += line;
            line.clear();
            componentsOnLine = 0;
        }

        AccumulateDescription( line, ref );
        componentsOnLine++;
    }

    if( !line.IsEmpty() )
    {
        if( !result.IsEmpty() )
            result += wxT( "\n" );

        result += line;
    }

    return result;
}


static wxString JoinMismatchReasons( const std::vector<wxString>& aReasons )
{
    wxString text;

    for( const wxString& reason : aReasons )
    {
        if( !text.IsEmpty() )
            text += wxT( "\n" );

        text += reason;
    }

    return text;
}


static void ShowTopologyMismatchReasons( wxWindow* aParent, const wxString& aSummary,
                                         const std::vector<wxString>& aReasons )
{
    if( !aParent || aReasons.empty() )
        return;

    wxRichMessageDialog dlg( aParent, aSummary, _( "Topology mismatch" ), wxICON_ERROR | wxOK );
    dlg.ShowDetailedText( JoinMismatchReasons( aReasons ) );
    dlg.ShowModal();
}


static void collectGroupFootprints( EDA_GROUP* aGroup, std::set<FOOTPRINT*>& aOut )
{
    for( EDA_ITEM* item : aGroup->GetItems() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            aOut.insert( static_cast<FOOTPRINT*>( item ) );
        else if( item->Type() == PCB_GROUP_T )
            collectGroupFootprints( static_cast<PCB_GROUP*>( item ), aOut );
    }
}


static void collectGroupBoardItems( EDA_GROUP* aGroup, std::set<BOARD_ITEM*>& aOut )
{
    for( EDA_ITEM* item : aGroup->GetItems() )
    {
        // A generator's own bounding box already covers its children (meander arcs).
        if( item->Type() == PCB_GROUP_T )
            collectGroupBoardItems( static_cast<PCB_GROUP*>( item ), aOut );
        else if( item->IsBOARD_ITEM() )
            aOut.insert( static_cast<BOARD_ITEM*>( item ) );
    }
}


// A linked channel made with its items grouped is exactly its group, so an update repeats what the first copy did
static EDA_GROUP* linkedChannelGroup( const BOARD* aBoard, const RULE_AREA* aArea )
{
    ZONE* zone = aArea->m_zone;

    if( !zone || aArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK || zone->GetLinkSet() == niluuid
        || !zone->GetRepeatOptions().m_groupItems )
    {
        return nullptr;
    }

    // Scratch zones carry the board pointer too, so membership is by the board's own index
    if( aBoard->ResolveItem( zone->m_Uuid, true ) != zone )
        return nullptr;

    return zone->GetParentGroup();
}


MULTICHANNEL_TOOL::MULTICHANNEL_TOOL() : PCB_TOOL_BASE( "pcbnew.Multichannel" )
{
}


MULTICHANNEL_TOOL::~MULTICHANNEL_TOOL()
{

}


void MULTICHANNEL_TOOL::Reset( RESET_REASON aReason )
{
    PCB_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD || aReason == SUPERMODEL_RELOAD || aReason == SHUTDOWN )
        closeRepeatDialog();
}

static void makePlacementArea( ZONE* aZone, const SHAPE_LINE_CHAIN& aOutline )
{
    aZone->SetIsRuleArea( true );
    aZone->SetLayerSet( LSET::AllCuMask() );
    aZone->SetPlacementAreaEnabled( true );
    aZone->SetDoNotAllowZoneFills( false );
    aZone->SetDoNotAllowVias( false );
    aZone->SetDoNotAllowTracks( false );
    aZone->SetDoNotAllowPads( false );
    aZone->SetDoNotAllowFootprints( false );
    aZone->AddPolygon( aOutline );
    aZone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
}


// Touching edges do not count, so channels may share a border
static bool outlinesOverlap( const SHAPE_POLY_SET& aA, const SHAPE_POLY_SET& aB )
{
    if( !aA.BBox().Intersects( aB.BBox() ) )
        return false;

    SHAPE_POLY_SET overlap = aA;
    overlap.BooleanIntersection( aB );
    return !overlap.IsEmpty();
}


// The matched pair holding the anchor footprint. An anchor outside the reference area falls back to the
// first pair, so the group is not shifted to center alignment
static const std::pair<FOOTPRINT* const, FOOTPRINT*>& anchorPair( const TMATCH::COMPONENT_MATCHES& aMatches,
                                                                  const KIID&                      aAnchor )
{
    auto it = std::ranges::find_if( aMatches, [&]( const auto& aPair ) { return aPair.first->m_Uuid == aAnchor; } );
    return it != aMatches.end() ? *it : *aMatches.begin();
}


// The outline aTarget takes on when aRef is repeated onto it, and the transform that produces it
static SHAPE_LINE_CHAIN repeatedOutline( const RULE_AREA* aRef, const RULE_AREA* aTarget, const KIID& aAnchor,
                                         const TMATCH::COMPONENT_MATCHES& aMatches, VECTOR2I& aDisp, EDA_ANGLE& aRot )
{
    aDisp = aTarget->m_center - aRef->m_center;
    aRot = ANGLE_0;

    if( aAnchor != niluuid && !aMatches.empty() )
    {
        const auto& [refAnchorFp, targetAnchorFp] = anchorPair( aMatches, aAnchor );
        VECTOR2I    oldpos = refAnchorFp->GetPosition();

        aRot = targetAnchorFp->GetOrientation() - refAnchorFp->GetOrientation();
        RotatePoint( oldpos, aRot );
        aDisp = targetAnchorFp->GetPosition() - oldpos;
    }

    SHAPE_LINE_CHAIN outline( aRef->m_zone->Outline()->COutline( 0 ) );
    outline.Rotate( aRot, VECTOR2I( 0, 0 ) );
    outline.Move( aDisp );
    return outline;
}


// The linked rule areas the selection stands for, whether as the area, a group holding it or a part of its channel
static std::set<ZONE*> linkedAreasIn( const SELECTION& aSelection )
{
    std::set<ZONE*> zones;

    auto asLinked = []( EDA_ITEM* aItem ) -> ZONE*
                    {
                        if( aItem->Type() != PCB_ZONE_T )
                            return nullptr;

                        ZONE* zone = static_cast<ZONE*>( aItem );
                        return zone->GetIsRuleArea() && zone->GetLinkSet() != niluuid ? zone : nullptr;
                    };

    // True when the group directly holds a linked area
    auto addFrom = [&]( EDA_GROUP* aGroup )
                   {
                       bool found = false;

                       for( EDA_ITEM* child : aGroup->GetItems() )
                       {
                           if( ZONE* zone = asLinked( child ) )
                           {
                               zones.insert( zone );
                               found = true;
                           }
                       }

                       return found;
                   };

    for( EDA_ITEM* item : aSelection )
    {
        // What is selected by name wins over the channels around it
        if( ZONE* zone = asLinked( item ) )
        {
            zones.insert( zone );
            continue;
        }

        EDA_GROUP* group = item->Type() == PCB_GROUP_T ? static_cast<PCB_GROUP*>( item ) : item->GetParentGroup();

        while( group && !addFrom( group ) )
            group = group->AsEdaItem()->GetParentGroup();
    }

    return zones;
}


ZONE* MULTICHANNEL_TOOL::ChannelZone( const PCB_GROUP* aGroup )
{
    for( EDA_ITEM* child : aGroup->GetItems() )
    {
        if( child->Type() == PCB_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( child );

            if( zone->GetIsRuleArea() && zone->GetPlacementAreaEnabled() )
                return zone;
        }
    }

    return nullptr;
}


class MULTICHANNEL_CONTEXT_MENU : public CONDITIONAL_MENU
{
public:
    MULTICHANNEL_CONTEXT_MENU( TOOL_INTERACTIVE* aTool ) : CONDITIONAL_MENU( aTool )
    {
        SetIcon( BITMAPS::mode_module );
        SetTitle( _( "Multi-Channel" ) );

        // A channel is repeated again through its Properties
        auto offChannel =
                []( const SELECTION& aSelection )
                {
                    for( EDA_ITEM* item : aSelection )
                    {
                        if( item->Type() != PCB_GROUP_T )
                            continue;

                        if( MULTICHANNEL_TOOL::ChannelZone( static_cast<PCB_GROUP*>( item ) ) )
                            return false;
                    }

                    return true;
                };

        auto oneLinked = []( const SELECTION& aSelection ) { return linkedAreasIn( aSelection ).size() == 1; };
        auto anyLinked = []( const SELECTION& aSelection ) { return !linkedAreasIn( aSelection ).empty(); };

        AddItem( PCB_ACTIONS::expandToMatchingTopology, SELECTION_CONDITIONS::HasType( PCB_FOOTPRINT_T ) );
        AddItem( PCB_ACTIONS::expandToLargestTopology, SELECTION_CONDITIONS::HasType( PCB_FOOTPRINT_T ) );
        AddItem( PCB_ACTIONS::repeatLayout, offChannel );
        AddItem( PCB_ACTIONS::updateLinkedLayouts, oneLinked );
        AddItem( PCB_ACTIONS::unlinkLayout, anyLinked );
    }

    ACTION_MENU* create() const override { return new MULTICHANNEL_CONTEXT_MENU( m_tool ); }
};


void MULTICHANNEL_TOOL::ShowMismatchDetails( wxWindow* aParent, const wxString& aSummary,
                                             const std::vector<wxString>& aReasons ) const
{
    wxWindow* parent = aParent ? aParent : frame();
    ShowTopologyMismatchReasons( parent, aSummary, aReasons );
}


bool MULTICHANNEL_TOOL::Init()
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    wxCHECK( selTool, false );

    TOOL_MENU& selToolMenu = selTool->GetToolMenu();

    std::shared_ptr<MULTICHANNEL_CONTEXT_MENU> menu = std::make_shared<MULTICHANNEL_CONTEXT_MENU>( this );
    selToolMenu.RegisterSubMenu( menu );

    selToolMenu.GetMenu().AddMenu( menu.get(),
                                   SELECTION_CONDITIONS::HasTypes( { PCB_FOOTPRINT_T, PCB_GROUP_T, PCB_ZONE_T } ),
                                   100 );

    return true;
}


void MULTICHANNEL_TOOL::setTransitions()
{
    Go( &MULTICHANNEL_TOOL::AutogenerateRuleAreas, PCB_ACTIONS::generatePlacementRuleAreas.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::repeatLayout, PCB_ACTIONS::repeatLayout.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::updateLinkedLayouts, PCB_ACTIONS::updateLinkedLayouts.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::unlinkLayout, PCB_ACTIONS::unlinkLayout.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::expandToMatchingTopology, PCB_ACTIONS::expandToMatchingTopology.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::expandToLargestTopology, PCB_ACTIONS::expandToLargestTopology.MakeEvent() );
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

    wxString sourceName = aRuleArea->m_zone->GetPlacementAreaSource();
    sourceName.Replace( wxT( "'" ), wxT( "\\'" ) );

    wxString ruleText;

    switch( aRuleArea->m_zone->GetPlacementAreaSourceType() )
    {
    case PLACEMENT_SOURCE_T::SHEETNAME:
        ruleText = wxT( "A.memberOfSheetOrChildren('" ) + sourceName + wxT( "')" );
        break;
    case PLACEMENT_SOURCE_T::COMPONENT_CLASS:
        ruleText = wxT( "A.hasComponentClass('" ) + sourceName + wxT( "')" );
        break;
    case PLACEMENT_SOURCE_T::GROUP_PLACEMENT:
        ruleText = wxT( "A.memberOfGroup('" ) + sourceName + wxT( "')" );
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
        // Get all board items that aren't footprints.  Connected items are usually handled by the
        // routing path, except zones which are copied via "other items".
        for( EDA_ITEM* item : aRuleArea->m_designBlockItems )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
                continue;

            // Generators are copied whole by the routing pass. Nested groups are not yet
            // preserved (TODO) so they are skipped here.
            if( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T )
                continue;

            // Cells are copied with their owning PCB_TABLE, not as standalone items.
            if( item->Type() == PCB_TABLECELL_T )
                continue;

            if( BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( item ) )
            {
                if( !boardItem->IsConnected() || boardItem->Type() == PCB_ZONE_T )
                    aItems.insert( boardItem );
            }
        }

        if( aRuleArea->m_fromSelection )
        {
            const BOX2I areaBox = aRuleArea->m_zone->Outline()->BBox();

            auto inside =
                    [&]( BOARD_ITEM* aItem )
                    {
                        return ( aRuleArea->m_zone->GetLayerSet() & aItem->GetLayerSet() ).any()
                               && areaBox.Contains( aItem->GetBoundingBox() );
                    };

            for( ZONE* zone : board()->Zones() )
            {
                if( !zone->GetIsRuleArea() && inside( zone ) )
                    aItems.insert( zone );
            }

            for( BOARD_ITEM* drawing : board()->Drawings() )
            {
                if( !drawing->IsConnected() && !drawing->GetParentGroup() && inside( drawing ) )
                    aItems.insert( drawing );
            }
        }

        return aItems.size() > 0;
    }

    if( EDA_GROUP* group = linkedChannelGroup( board(), aRuleArea ) )
    {
        std::set<BOARD_ITEM*> members;
        collectGroupBoardItems( group, members );

        for( BOARD_ITEM* boardItem : members )
        {
            if( boardItem == aRuleArea->m_zone || boardItem->Type() == PCB_FOOTPRINT_T
                || boardItem->Type() == PCB_GENERATOR_T || boardItem->Type() == PCB_TABLECELL_T )
            {
                continue;
            }

            if( !boardItem->IsConnected() || boardItem->Type() == PCB_ZONE_T )
                aItems.insert( boardItem );
        }

        return true;
    }

    // The design-block apply target uses a scratch zone not on the board, so enclosedByArea()
    // below finds nothing. Resolve the group's items directly from the group.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT
        && ( !aRuleArea->m_components.empty() || aRuleArea->m_group ) )
    {
        bool zoneOnBoard = false;

        for( ZONE* zone : board()->Zones() )
        {
            if( zone == aRuleArea->m_zone )
            {
                zoneOnBoard = true;
                break;
            }
        }

        if( !zoneOnBoard )
        {
            EDA_GROUP* group = aRuleArea->m_group;

            if( !group && !aRuleArea->m_components.empty() )
                group = ( *aRuleArea->m_components.begin() )->GetParentGroup();

            if( group )
            {
                std::set<BOARD_ITEM*> members;
                collectGroupBoardItems( group, members );

                for( BOARD_ITEM* boardItem : members )
                {
                    if( boardItem->Type() == PCB_FOOTPRINT_T || boardItem->Type() == PCB_GENERATOR_T
                        || boardItem->Type() == PCB_TABLECELL_T )
                        continue;

                    if( !boardItem->IsConnected() || boardItem->Type() == PCB_ZONE_T )
                        aItems.insert( boardItem );
                }
            }

            return aItems.size() > 0;
        }
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

    wxString childPrefix = aSheetName + wxT( "/" );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        auto sn = fp->GetSheetname();

        if( sn.EndsWith( wxT( "/" ) ) )
            sn.RemoveLast();

        if( sn == aSheetName || sn.StartsWith( childPrefix ) )
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
        if( group->GetName().Matches( aGroupName ) )
            collectGroupFootprints( group, rv );
    }

    return rv;
}


PCB_GROUP* MULTICHANNEL_TOOL::findPlacementGroup( const wxString& aGroupName ) const
{
    PCB_GROUP* found = nullptr;

    for( PCB_GROUP* group : board()->Groups() )
    {
        if( !group->GetName().Matches( aGroupName ) )
            continue;

        if( found )
            return nullptr;

        found = group;
    }

    return found;
}


std::set<BOARD_ITEM*> MULTICHANNEL_TOOL::queryBoardItemsInGroup( const wxString& aGroupName ) const
{
    std::set<BOARD_ITEM*> rv;

    for( PCB_GROUP* group : board()->Groups() )
    {
        if( !group->GetName().Matches( aGroupName ) )
            continue;

        for( EDA_ITEM* item : group->GetItems() )
        {
            if( item->IsBOARD_ITEM() )
                rv.insert( static_cast<BOARD_ITEM*>( item ) );
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

const SHAPE_LINE_CHAIN MULTICHANNEL_TOOL::buildRAOutline( const std::set<BOARD_ITEM*>& aItems, int aMargin )
{
    std::vector<VECTOR2I> bbCorners;
    bbCorners.reserve( aItems.size() * 4 );

    for( BOARD_ITEM* item : aItems )
    {
        BOX2I bb = item->GetBoundingBox();

        if( item->Type() == PCB_FOOTPRINT_T )
            bb = static_cast<FOOTPRINT*>( item )->GetBoundingBox( false );

        KIGEOM::CollectBoxCorners( bb.GetInflated( aMargin ), bbCorners );
    }

    std::vector<VECTOR2I> hullVertices;
    BuildConvexHull( hullVertices, bbCorners );

    SHAPE_LINE_CHAIN hull( hullVertices );

    // Make the newly computed convex hull use only 90 degree segments
    return KIGEOM::RectifyPolygon( hull );
}


// Returns each parent sheet path above aSheetName, e.g. "/A/B/C/" -> { "/A/", "/A/B/" }.
// The root and the sheet itself are left out.
static std::vector<wxString> getParentSheetPaths( const wxString& aSheetName )
{
    std::vector<wxString> segments;
    wxString              cur;

    for( wxUniChar ch : aSheetName )
    {
        if( ch == '/' )
        {
            if( !cur.IsEmpty() )
                segments.push_back( cur );

            cur.clear();
        }
        else
        {
            cur += ch;
        }
    }

    if( !cur.IsEmpty() )
        segments.push_back( cur );

    std::vector<wxString> rv;
    wxString              prefix = wxT( "/" );

    for( size_t i = 0; i + 1 < segments.size(); ++i )
    {
        prefix += segments[i] + wxT( "/" );
        rv.push_back( prefix );
    }

    return rv;
}


void MULTICHANNEL_TOOL::GeneratePotentialRuleAreas()
{
    // Sheet path -> sheet file. Container sheets that only hold subsheets have no file of
    // their own, so they map to an empty string.
    std::map<wxString, wxString> uniqueSheets;
    std::set<wxString>           uniqueComponentClasses;
    std::set<wxString>           uniqueGroups;

    m_areas.m_areas.clear();

    for( const FOOTPRINT* fp : board()->Footprints() )
    {
        uniqueSheets[fp->GetSheetname()] = fp->GetSheetfile();

        // Offer the parent sheets as channels too, not just the deepest one.
        for( const wxString& parent : getParentSheetPaths( fp->GetSheetname() ) )
            uniqueSheets.emplace( parent, wxString() );

        const COMPONENT_CLASS* compClass = fp->GetComponentClass();

        for( const COMPONENT_CLASS* singleClass : compClass->GetConstituentClasses() )
            uniqueComponentClasses.insert( singleClass->GetName() );

        // Offer every named group up the chain, not just the immediate parent, so a
        // channel group wrapping several sub-groups can be picked too.
        for( EDA_GROUP* grp = fp->GetParentGroup(); grp; grp = grp->AsEdaItem()->GetParentGroup() )
        {
            if( !grp->GetName().IsEmpty() )
                uniqueGroups.insert( grp->GetName() );
        }
    }

    for( const auto& [sheetPath, sheetFile] : uniqueSheets )
    {
        RULE_AREA ent;

        ent.m_sourceType = PLACEMENT_SOURCE_T::SHEETNAME;
        ent.m_generateEnabled = false;
        ent.m_sheetPath = sheetPath;
        ent.m_sheetName = sheetFile;
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

        area.m_zone = zone;
        area.m_sourceType = zone->GetPlacementAreaSourceType();
        area.m_ruleName = zone->GetZoneName();
        area.m_center = zone->Outline()->COutline( 0 ).Centre();

        switch( area.m_sourceType )
        {
        case PLACEMENT_SOURCE_T::SHEETNAME: area.m_sheetPath = zone->GetPlacementAreaSource(); break;

        case PLACEMENT_SOURCE_T::COMPONENT_CLASS: area.m_componentClass = zone->GetPlacementAreaSource(); break;

        case PLACEMENT_SOURCE_T::GROUP_PLACEMENT:
            area.m_groupName = zone->GetPlacementAreaSource();
            area.m_group = findPlacementGroup( area.m_groupName );
            break;

        case PLACEMENT_SOURCE_T::DESIGN_BLOCK: break;
        }

        findComponentsInRuleArea( &area, area.m_components );

        m_areas.m_areas.push_back( area );

        wxLogTrace( traceMultichannelTool, wxT( "RA '%s', %d footprints\n" ), area.m_ruleName,
                    (int) area.m_components.size() );
    }

    wxLogTrace( traceMultichannelTool, wxT( "Total RAs found: %d\n" ), (int) m_areas.m_areas.size() );
}


void MULTICHANNEL_TOOL::UpdatePickedItem( const EDA_ITEM* aItem )
{
    if( aItem )
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

    closeRepeatDialog();

    // Plain parts with no rule area are the reference, and the channels repeating them are discovered
    if( refRAs.empty() && ( selection().HasType( PCB_FOOTPRINT_T ) || selection().HasType( PCB_GROUP_T ) ) )
    {
        StartRepeatFromSelection( std::vector<EDA_ITEM*>( selection().begin(), selection().end() ) );

        if( !m_areas.m_refRA )
        {
            frame()->ShowInfoBarMsg( _( "Select the footprints of the circuit to repeat." ), true );
            return 0;
        }
    }
    else if( refRAs.size() != 1 )
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
    else
    {
        FindExistingRuleAreas();

        int status = CheckRACompatibility( refRAs.front() );

        if( status < 0 )
            return status;

        if( m_areas.m_areas.size() <= 1 )
        {
            frame()->ShowInfoBarError( _( "No Rule Areas to repeat layout to have been found." ), true );
            return 0;
        }

        const KIID& linkSet = refRAs.front()->GetLinkSet();
        m_areas.m_options = refRAs.front()->GetRepeatOptions();
        m_areas.m_linkAreas = linkSet != niluuid;

        // A linked source's set is the user's previous choice
        for( auto& [area, compat] : m_areas.m_compatMap )
            compat.m_doCopy = compat.m_isOk && ( linkSet == niluuid || area->m_zone->GetLinkSet() == linkSet );
    }

    // The view's preview group can be gone since the board loaded, so it is set up again as the router does
    getView()->InitPreview();

    // Not modal, so the board can be panned and zoomed while the preview shows the result
    m_dialog = new DIALOG_MULTICHANNEL_REPEAT_LAYOUT( frame(), this );
    m_dialog->Bind( wxEVT_CLOSE_WINDOW, &MULTICHANNEL_TOOL::onRepeatDialogClosed, this );
    m_dialog->Show( true );
    UpdateRepeatPreview();
    return 0;
}


// A net that belongs to a channel reaches at most one footprint outside it (a connector or the neighbouring
// block). Rails and buses reach many, so they never tie a channel together
static bool isChannelNet( const std::vector<FOOTPRINT*>& aOwners, const std::set<FOOTPRINT*>& aChannel )
{
    return std::ranges::count_if( aOwners, [&]( FOOTPRINT* aFp ) { return !aChannel.contains( aFp ); } ) <= 1;
}


static std::set<int> channelNets( const std::map<int, std::vector<FOOTPRINT*>>& aNetOwners,
                                  const std::set<FOOTPRINT*>&                    aChannel )
{
    std::set<int> nets;

    for( const auto& [net, owners] : aNetOwners )
    {
        if( owners.size() > 1 && isChannelNet( owners, aChannel )
            && std::ranges::any_of( owners, [&]( FOOTPRINT* aFp ) { return aChannel.contains( aFp ); } ) )
        {
            nets.insert( net );
        }
    }

    return nets;
}


// Moves aItem into aGroup, staging the group it leaves so undo restores that membership too
static void stageAddToGroup( BOARD_COMMIT& aCommit, EDA_GROUP* aGroup, BOARD_ITEM* aItem )
{
    if( EDA_GROUP* old = aItem->GetParentGroup() )
    {
        if( old == aGroup || old->AsEdaItem()->Type() == PCB_GENERATOR_T )
            return;

        aCommit.Modify( old->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );
    }

    aCommit.Modify( aItem );
    aGroup->AddItem( aItem );
}


static std::set<int> padNets( const FOOTPRINT* aFp, const std::set<int>& aAmong )
{
    std::set<int> nets;

    for( const PAD* pad : aFp->Pads() )
    {
        if( aAmong.contains( pad->GetNetCode() ) )
            nets.insert( pad->GetNetCode() );
    }

    return nets;
}


// Repeated sheets give every instance the same symbol UUIDs under its own instance path
static std::vector<TMATCH::COMPONENT_MATCHES> matchSheetInstances( BOARD* aBoard, const std::set<FOOTPRINT*>& aRef )
{
    using PATH = std::vector<KIID>;

    std::optional<PATH> refPrefix;

    for( FOOTPRINT* fp : aRef )
    {
        const KIID_PATH& path = fp->GetPath();

        if( path.size() < 2 )
            return {};

        if( !refPrefix )
            refPrefix = PATH( path.begin(), path.end() - 1 );

        size_t depth = std::min( refPrefix->size(), path.size() - 1 );
        auto   diff = std::mismatch( refPrefix->begin(), refPrefix->begin() + depth, path.begin() );

        refPrefix->resize( diff.first - refPrefix->begin() );
    }

    if( !refPrefix || refPrefix->empty() )
        return {};

    const size_t depth = refPrefix->size();

    auto rest =
            [&]( const KIID_PATH& aPath )
            {
                return PATH( aPath.begin() + depth, aPath.end() );
            };

    std::map<PATH, FOOTPRINT*> refByRest;

    for( FOOTPRINT* fp : aRef )
        refByRest[rest( fp->GetPath() )] = fp;

    std::map<PATH, TMATCH::COMPONENT_MATCHES> instances;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        const KIID_PATH& path = fp->GetPath();

        if( aRef.contains( fp ) || path.size() <= depth )
            continue;

        PATH prefix( path.begin(), path.begin() + depth );

        if( prefix == *refPrefix )
            continue;

        if( auto it = refByRest.find( rest( path ) ); it != refByRest.end() && it->second->GetFPID() == fp->GetFPID() )
            instances[prefix][it->second] = fp;
    }

    std::vector<TMATCH::COMPONENT_MATCHES> result;

    for( auto& [prefix, matches] : instances )
        result.push_back( std::move( matches ) );

    return result;
}


// Pairs the reference footprints, in growth order from the seed, with board footprints that sit on the same
// mirrored nets, backtracking on dead ends. With aLoose a leaf may stay unmatched
struct FLAT_MATCHER
{
    const std::map<int, std::vector<FOOTPRINT*>>& m_netOwners;
    const std::set<int>&                          m_internal;
    const std::set<FOOTPRINT*>&                   m_leaves;
    const std::set<FOOTPRINT*>&                   m_claimed;
    const std::vector<FOOTPRINT*>&                m_order;
    bool                                          m_loose;

    TMATCH::COMPONENT_MATCHES m_map = {};
    std::set<FOOTPRINT*>      m_used = {};
    std::map<int, int>        m_netMap = {};
    std::set<int>             m_netsTaken = {};
    int                       m_budget = 10000;

    bool bind( FOOTPRINT* aRef, FOOTPRINT* aTarget, std::vector<int>& aAdded )
    {
        for( PAD* pad : aRef->Pads() )
        {
            int net = pad->GetNetCode();

            if( !m_internal.contains( net ) )
                continue;

            const PAD* peer = aTarget->FindPadByNumber( pad->GetNumber() );

            if( !peer )
                return false;

            int peerNet = peer->GetNetCode();

            if( auto it = m_netMap.find( net ); it != m_netMap.end() )
            {
                if( it->second != peerNet )
                    return false;
            }
            else
            {
                if( peerNet <= 0 || m_netsTaken.contains( peerNet ) )
                    return false;

                m_netMap[net] = peerNet;
                m_netsTaken.insert( peerNet );
                aAdded.push_back( net );
            }
        }

        return true;
    }

    // Pairs m_order[aIndex] with aChoice and matches the rest, undoing the pairing on a dead end
    bool pair( size_t aIndex, FOOTPRINT* aChoice )
    {
        FOOTPRINT*       ref = m_order[aIndex];
        std::vector<int> added;

        if( bind( ref, aChoice, added ) )
        {
            m_map[ref] = aChoice;
            m_used.insert( aChoice );

            if( extend( aIndex + 1 ) )
                return true;

            m_map.erase( ref );
            m_used.erase( aChoice );
        }

        for( int net : added )
        {
            m_netsTaken.erase( m_netMap[net] );
            m_netMap.erase( net );
        }

        return false;
    }

    bool extend( size_t aIndex )
    {
        if( aIndex == m_order.size() )
            return true;

        if( --m_budget < 0 )
            return false;

        FOOTPRINT* ref = m_order[aIndex];

        // One mapped net is enough to bound the choices; bind() checks the rest
        auto mapped = std::ranges::find_if( ref->Pads(),
                                            [&]( PAD* aPad ) { return m_netMap.contains( aPad->GetNetCode() ); } );

        if( mapped != ref->Pads().end() )
        {
            for( FOOTPRINT* fp : m_netOwners.at( m_netMap.at( ( *mapped )->GetNetCode() ) ) )
            {
                if( !m_claimed.contains( fp ) && !m_used.contains( fp ) && fp->GetFPID() == ref->GetFPID()
                    && pair( aIndex, fp ) )
                {
                    return true;
                }

                if( m_budget < 0 )
                    return false;
            }
        }

        // A leaf may stay unmatched; the caller works out which parts were left out
        return m_loose && m_leaves.contains( ref ) && extend( aIndex + 1 );
    }
};


void MULTICHANNEL_TOOL::StartRepeatFromSelection( const std::vector<EDA_ITEM*>& aItems )
{
    m_areas = RULE_AREAS_DATA();

    // The channels become groups, so copies join them
    m_areas.m_options.m_groupItems = true;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() == PCB_GROUP_T )
            collectGroupBoardItems( static_cast<PCB_GROUP*>( item ), m_areas.m_selectedItems );
        else if( item->IsBOARD_ITEM() && !static_cast<BOARD_ITEM*>( item )->GetParentFootprint() )
            m_areas.m_selectedItems.insert( static_cast<BOARD_ITEM*>( item ) );
    }

    for( BOARD_ITEM* item : m_areas.m_selectedItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            m_areas.m_selectedFootprints.insert( static_cast<FOOTPRINT*>( item ) );
    }

    DiscoverRepeatCandidates( false );

    // A session without a reference must not divert later rule area repeats into the selection flow
    if( !m_areas.m_refRA )
        m_areas = RULE_AREAS_DATA();
}


// The footprints on each net, in board order
static std::map<int, std::vector<FOOTPRINT*>> buildNetOwners( const BOARD* aBoard )
{
    std::map<int, std::vector<FOOTPRINT*>> netOwners;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            std::vector<FOOTPRINT*>& owners = netOwners[pad->GetNetCode()];

            if( pad->GetNetCode() > 0 && ( owners.empty() || owners.back() != fp ) )
                owners.push_back( fp );
        }
    }

    return netOwners;
}


std::vector<RULE_AREA_COMPAT_DATA> MULTICHANNEL_TOOL::findMatches( const std::set<FOOTPRINT*>& selected,
                                                                  bool                        aLoose )
{
    std::map<int, std::vector<FOOTPRINT*>> netOwners = buildNetOwners( board() );

    std::set<int>        internal = channelNets( netOwners, selected );
    std::set<FOOTPRINT*> leaves;

    for( FOOTPRINT* fp : selected )
    {
        if( padNets( fp, internal ).size() <= 1 )
            leaves.insert( fp );
    }

    std::vector<RULE_AREA_COMPAT_DATA> found;

    // Kept when it lacks no selected part, or only leaves under loose matching
    auto classify =
            [&]( RULE_AREA_COMPAT_DATA& aCandidate )
            {
                for( FOOTPRINT* fp : selected )
                {
                    if( !aCandidate.m_discovered.contains( fp ) )
                        aCandidate.m_unmatched.insert( fp );
                }

                aCandidate.m_doCopy = true;

                return aCandidate.m_unmatched.empty()
                       || ( aLoose && std::ranges::all_of( aCandidate.m_unmatched,
                                                           [&]( FOOTPRINT* aFp ) { return leaves.contains( aFp ); } ) );
            };

    for( TMATCH::COMPONENT_MATCHES& matches : matchSheetInstances( board(), selected ) )
    {
        RULE_AREA_COMPAT_DATA candidate;
        candidate.m_discovered = std::move( matches );

        if( classify( candidate ) )
            found.push_back( std::move( candidate ) );
    }

    if( found.empty() )
    {
        std::map<LIB_ID, int> fpidCount;

        for( FOOTPRINT* fp : board()->Footprints() )
            fpidCount[fp->GetFPID()]++;

        // The rarest part keeps the number of seeds low; a leaf may be the part a channel lacks
        FOOTPRINT* seed = *std::ranges::min_element( selected,
                [&]( FOOTPRINT* aA, FOOTPRINT* aB )
                {
                    return std::make_pair( leaves.contains( aA ), fpidCount[aA->GetFPID()] )
                           < std::make_pair( leaves.contains( aB ), fpidCount[aB->GetFPID()] );
                } );

        std::vector<FOOTPRINT*> order = { seed };

        for( size_t i = 0; i < order.size(); ++i )
        {
            for( int net : padNets( order[i], internal ) )
            {
                for( FOOTPRINT* fp : netOwners[net] )
                {
                    if( selected.contains( fp ) && std::ranges::find( order, fp ) == order.end() )
                        order.push_back( fp );
                }
            }
        }

        std::set<FOOTPRINT*> claimed = selected;

        for( FOOTPRINT* twin : board()->Footprints() )
        {
            if( claimed.contains( twin ) || twin->GetFPID() != seed->GetFPID() )
                continue;

            FLAT_MATCHER          matcher{ netOwners, internal, leaves, claimed, order, aLoose };
            RULE_AREA_COMPAT_DATA candidate;
            bool                  ok = matcher.pair( 0, twin );

            if( matcher.m_budget < 0 )
            {
                candidate.m_discovered = { { seed, twin } };
                candidate.m_searchLimit = true;
                found.push_back( std::move( candidate ) );
                continue;
            }

            candidate.m_discovered = matcher.m_map;

            if( !ok || !classify( candidate ) )
                continue;

            // Rails are judged against both channels together, whichever channels were found first
            RULE_AREA               refArea;
            RULE_AREA               targetArea;
            std::set<FOOTPRINT*>    both;
            std::unordered_set<int> rails;

            for( const auto& [ref, target] : candidate.m_discovered )
            {
                refArea.m_components.insert( ref );
                targetArea.m_components.insert( target );
                both.insert( { ref, target } );
            }

            for( FOOTPRINT* fp : both )
            {
                for( PAD* pad : fp->Pads() )
                {
                    if( pad->GetNetCode() > 0 && !isChannelNet( netOwners[pad->GetNetCode()], both ) )
                        rails.insert( pad->GetNetCode() );
                }
            }

            // A flat match is only a guess until the topology agrees, so it claims nothing before that
            RULE_AREA_COMPAT_DATA check;

            if( !resolveConnectionTopology( &refArea, &targetArea, check, {}, rails ) )
                continue;

            claimed.insert( targetArea.m_components.begin(), targetArea.m_components.end() );
            found.push_back( std::move( candidate ) );
        }
    }

    return found;
}


void MULTICHANNEL_TOOL::DiscoverRepeatCandidates( bool aLoose )
{
    const std::set<FOOTPRINT*>& selected = m_areas.m_selectedFootprints;
    PROF_TIMER                  timer;

    m_areas.m_refRA = nullptr;
    m_focusArea = nullptr;
    m_areas.m_compatMap.clear();
    m_areas.m_areas.clear();
    m_areas.m_scratchItems.clear();

    if( selected.empty() )
        return;

    std::vector<RULE_AREA_COMPAT_DATA> found = findMatches( selected, aLoose );

    // Filled before any pointer to its elements is taken
    m_areas.m_areas.reserve( found.size() + 1 );

    auto addArea =
            [&]( const SHAPE_LINE_CHAIN& aOutline, const std::set<FOOTPRINT*>& aFootprints,
                 PLACEMENT_SOURCE_T aSourceType ) -> RULE_AREA&
            {
                ZONE*      zone = new ZONE( board() );
                PCB_GROUP* group = new PCB_GROUP( board() );

                m_areas.m_scratchItems.emplace_back( zone );
                m_areas.m_scratchItems.emplace_back( group );
                makePlacementArea( zone, aOutline );

                RULE_AREA& area = m_areas.m_areas.emplace_back();
                area.m_zone = zone;
                area.m_group = group;
                area.m_components = aFootprints;
                area.m_center = aOutline.Centre();
                area.m_sourceType = aSourceType;
                return area;
            };

    // The selection is copied as given, like a design block, rather than by what its area encloses
    addArea( buildRAOutline( m_areas.m_selectedItems, 100000 ), selected, PLACEMENT_SOURCE_T::DESIGN_BLOCK )
            .m_fromSelection = true;

    for( const RULE_AREA_COMPAT_DATA& candidate : found )
    {
        std::set<FOOTPRINT*> targets;

        for( const auto& [ref, target] : candidate.m_discovered )
            targets.insert( target );

        addArea( buildRAOutline( targets, 100000 ), targets, PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
    }

    m_areas.m_refRA = &m_areas.m_areas.front();

    for( size_t i = 0; i < found.size(); ++i )
        m_areas.m_compatMap[&m_areas.m_areas[i + 1]] = std::move( found[i] );

    // A candidate that fails on its own starts unticked, so its missing parts do not shrink the others
    bool resolved = updateSelectionCore();
    bool unticked = false;

    for( auto& [area, compat] : m_areas.m_compatMap )
    {
        unticked |= compat.m_doCopy && !compat.m_isOk;
        compat.m_doCopy &= compat.m_isOk;
    }

    // Cancelled candidates are unticked too, and resolving the rest again would restart the search
    if( unticked && resolved )
        updateSelectionCore();

    wxLogTrace( traceMultichannelTool, wxT( "Discovery: %s, %d candidates" ), timer.to_string(),
                (int) m_areas.m_compatMap.size() );
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::ExpandToMatchingTopology( const std::set<FOOTPRINT*>& aSeed, bool aLargest )
{
    // Each trial runs a full discovery, so a large board cannot be searched without limit
    int budget = 400;

    auto hasTwin =
            [&]( const std::set<FOOTPRINT*>& aSet )
            {
                budget--;

                return std::ranges::any_of( findMatches( aSet, false ),
                                            []( const RULE_AREA_COMPAT_DATA& aMatch )
                                            {
                                                return !aMatch.m_searchLimit;
                                            } );
            };

    std::set<FOOTPRINT*> result = aSeed;

    if( result.empty() || !hasTwin( result ) )
        return result;

    // A sheet instance is the largest set a hierarchical design repeats
    const KIID_PATH& path = ( *aSeed.begin() )->GetPath();

    if( aLargest && path.size() >= 2 )
    {
        std::set<FOOTPRINT*> sheet;

        for( FOOTPRINT* fp : board()->Footprints() )
        {
            const KIID_PATH& other = fp->GetPath();

            if( other.size() == path.size() && std::equal( path.begin(), path.end() - 1, other.begin() ) )
                sheet.insert( fp );
        }

        bool holdsSeed = std::ranges::all_of( aSeed, [&]( FOOTPRINT* aFp ) { return sheet.contains( aFp ); } );

        if( sheet.size() > result.size() && holdsSeed && hasTwin( sheet ) )
        {
            return sheet;
        }
    }

    std::map<int, std::vector<FOOTPRINT*>> netOwners = buildNetOwners( board() );

    // Rails and buses reach too many parts to tie a channel together
    constexpr size_t maxOwners = 12;

    std::set<FOOTPRINT*> rejected;
    bool                 grew = true;

    while( grew && budget > 0 )
    {
        grew = false;

        // A step only reaches the parts next to what the pass started with
        const std::set<FOOTPRINT*> frontier = result;

        for( const auto& [net, owners] : netOwners )
        {
            if( owners.size() > maxOwners || !std::ranges::any_of( owners, [&]( FOOTPRINT* aFp )
                                                                   { return frontier.contains( aFp ); } ) )
            {
                continue;
            }

            for( FOOTPRINT* fp : owners )
            {
                if( budget <= 0 )
                    break;

                if( result.contains( fp ) || rejected.contains( fp ) )
                    continue;

                std::set<FOOTPRINT*> trial = result;
                trial.insert( fp );

                if( hasTwin( trial ) )
                {
                    result = std::move( trial );
                    grew = true;
                }
                else
                {
                    rejected.insert( fp );
                }
            }
        }

        // A part refused alone may fit once its neighbours are in
        if( grew )
            rejected.clear();

        if( !aLargest )
            break;
    }

    return result;
}


int MULTICHANNEL_TOOL::expandToMatchingTopology( const TOOL_EVENT& aEvent )
{
    return expandSelection( false );
}


int MULTICHANNEL_TOOL::expandToLargestTopology( const TOOL_EVENT& aEvent )
{
    return expandSelection( true );
}


int MULTICHANNEL_TOOL::expandSelection( bool aLargest )
{
    std::set<FOOTPRINT*> seed;

    for( EDA_ITEM* item : selection() )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            seed.insert( static_cast<FOOTPRINT*>( item ) );
        else if( item->Type() == PCB_GROUP_T )
            collectGroupFootprints( static_cast<PCB_GROUP*>( item ), seed );
    }

    if( seed.empty() )
    {
        frame()->ShowInfoBarMsg( _( "Select a footprint to expand from." ), true );
        return 0;
    }

    std::set<FOOTPRINT*> expanded = ExpandToMatchingTopology( seed, aLargest );

    if( expanded.size() == seed.size() )
    {
        frame()->ShowInfoBarMsg( _( "No larger topology repeats elsewhere on the board." ), true );
        return 0;
    }

    EDA_ITEMS items( expanded.begin(), expanded.end() );

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &items );

    frame()->ShowInfoBarMsg( wxString::Format( _( "Selected %d parts that repeat elsewhere." ), (int) expanded.size() ),
                             true );
    return 0;
}


bool MULTICHANNEL_TOOL::updateSelectionCore()
{
    RULE_AREA*           ref = m_areas.m_refRA;
    std::set<FOOTPRINT*> pruned;

    if( !ref )
        return true;

    // Every channel keeps only the parts all ticked channels share
    for( const auto& [area, compat] : m_areas.m_compatMap )
    {
        if( compat.m_doCopy )
            pruned.insert( compat.m_unmatched.begin(), compat.m_unmatched.end() );
    }

    ref->m_components.clear();
    ref->m_designBlockItems.clear();

    for( BOARD_ITEM* item : m_areas.m_selectedItems )
    {
        if( item->Type() == PCB_FOOTPRINT_T && pruned.contains( static_cast<FOOTPRINT*>( item ) ) )
            continue;

        ref->m_designBlockItems.insert( item );

        if( item->Type() == PCB_FOOTPRINT_T )
            ref->m_components.insert( static_cast<FOOTPRINT*>( item ) );
    }

    if( ref->m_components.empty() )
    {
        for( auto& [area, compat] : m_areas.m_compatMap )
        {
            compat.m_isOk = false;
            compat.m_errorMsg = _( "The ticked channels share no selected part" );
        }

        return true;
    }

    // The anchor must be shared by all channels; the biggest part is the one placed first
    if( !std::ranges::any_of( ref->m_components,
                              [&]( FOOTPRINT* aFp ) { return aFp->m_Uuid == m_areas.m_options.m_anchor; } ) )
    {
        m_areas.m_options.m_anchor = niluuid;
        int64_t biggest = -1;

        for( FOOTPRINT* fp : ref->m_components )
        {
            if( int64_t area = fp->GetBoundingBox( false ).GetArea(); area > biggest )
            {
                biggest = area;
                m_areas.m_options.m_anchor = fp->m_Uuid;
            }
        }
    }

    std::vector<TOPOLOGY_JOB> jobs;

    for( auto& [area, compat] : m_areas.m_compatMap )
    {
        area->m_components.clear();

        for( FOOTPRINT* fp : ref->m_components )
        {
            if( auto it = compat.m_discovered.find( fp ); it != compat.m_discovered.end() )
                area->m_components.insert( it->second );
        }

        if( compat.m_searchLimit )
        {
            compat.m_isOk = false;
            compat.m_errorMsg = _( "Search limit reached" );
            continue;
        }

        jobs.push_back( { ref, area, &compat } );
    }

    bool resolved = resolveTopologies( _( "Matching channels..." ), jobs );

    for( const TOPOLOGY_JOB& job : jobs )
    {
        RULE_AREA*             area = job.m_target;
        RULE_AREA_COMPAT_DATA& compat = *job.m_compat;

        if( !compat.m_isOk )
            continue;

        if( !compat.m_unmatched.empty() )
            compat.m_errorMsg = wxString::Format( _( "Loose: prunes %s" ), JoinReferences( compat.m_unmatched ) );

        // Copying clears the target by its outline, so it has to be where the copy will land
        VECTOR2I  disp;
        EDA_ANGLE rot;

        area->m_zone->RemoveAllContours();
        area->m_zone->AddPolygon( repeatedOutline( ref, area, m_areas.m_options.m_anchor, compat.m_matchingComponents,
                                                   disp, rot ) );
    }

    return resolved;
}


int MULTICHANNEL_TOOL::applySelectionRepeat()
{
    RULE_AREA*              ref = m_areas.m_refRA;
    std::vector<RULE_AREA*> channels = { ref };
    BOARD_COMMIT            commit( GetManager(), true, false );

    for( auto& [area, compat] : m_areas.m_compatMap )
    {
        if( compat.m_doCopy && compat.m_isOk )
            channels.push_back( area );
    }

    if( channels.size() < 2 )
        return 0;

    auto addToBoard =
            [&]( BOARD_ITEM* aItem )
            {
                auto it = std::ranges::find_if( m_areas.m_scratchItems,
                                                [&]( const auto& aScratch ) { return aScratch.get() == aItem; } );
                commit.Add( it->release() );
            };

    std::set<wxString> names;

    for( PCB_GROUP* group : board()->Groups() )
        names.insert( group->GetName() );

    for( RULE_AREA* area : channels )
    {
        // Each channel is named after its counterpart of the anchor
        RULE_AREA* matched = area == ref ? channels[1] : area;
        const auto& [refAnchor, targetAnchor] =
                anchorPair( m_areas.m_compatMap[matched].m_matchingComponents, m_areas.m_options.m_anchor );
        FOOTPRINT* anchor = area == ref ? refAnchor : targetAnchor;

        wxString name = BOARD::MakeUniqueZoneName( wxString::Format( _( "%s channel" ), anchor->GetReference() ),
                                                   [&]( const wxString& aName ) { return names.contains( aName ); } );
        names.insert( name );

        area->m_group->SetName( name );
        area->m_zone->SetZoneName( name );

        // A channel that is exactly one sheet instance keeps following the schematic
        wxString sheet = anchor->GetSheetname();
        bool     wholeSheet = queryComponentsInSheet( sheet ) == area->m_components;

        area->m_zone->SetPlacementAreaSourceType( wholeSheet ? PLACEMENT_SOURCE_T::SHEETNAME
                                                             : PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
        area->m_zone->SetPlacementAreaSource( wholeSheet ? sheet : area->m_group->GetName() );

        addToBoard( area->m_zone );
        addToBoard( area->m_group );
        stageAddToGroup( commit, area->m_group, area->m_zone );

        for( FOOTPRINT* fp : area->m_components )
            stageAddToGroup( commit, area->m_group, fp );
    }

    for( EDA_ITEM* item : ref->m_designBlockItems )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            stageAddToGroup( commit, ref->m_group, static_cast<BOARD_ITEM*>( item ) );
    }

    // What the area of the parts brings along is part of the channel, so linked updates repeat it too
    std::set<BOARD_ITEM*> found;

    if( m_areas.m_options.m_copyRouting )
    {
        std::set<int> nets;

        for( FOOTPRINT* fp : ref->m_components )
        {
            for( PAD* pad : fp->Pads() )
                nets.insert( pad->GetNetCode() );
        }

        for( BOARD_CONNECTED_ITEM* item : findRepeatRouting( ref, nets, m_areas.m_options ) )
            found.insert( item );
    }

    if( m_areas.m_options.m_copyOtherItems )
        findOtherItemsInRuleArea( ref, found );

    for( BOARD_ITEM* item : found )
    {
        if( !item->GetParentGroup() && !ref->m_designBlockItems.contains( item ) )
            stageAddToGroup( commit, ref->m_group, item );
    }

    if( m_areas.m_linkAreas )
    {
        std::vector<RULE_AREA*> targets;

        for( RULE_AREA* area : channels )
        {
            if( area != ref )
                targets.push_back( area );
        }

        linkAreas( commit, ref->m_zone, targets );
    }

    applyRepeat( commit );

    // Pushing delivers the model change, which ends the session and frees ref
    EDA_ITEM* refGroup = ref->m_group->AsEdaItem();
    int       created = (int) channels.size();

    commit.Push( _( "Repeat Layout" ) );

    // Editing the old loose selection would leave the reference area behind
    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction( ACTIONS::selectItem, refGroup );

    if( Pgm().IsGUI() )
    {
        frame()->ShowInfoBarMsg( wxString::Format( m_areas.m_linkAreas ? _( "Created %d linked channels." )
                                                                       : _( "Created %d channels." ),
                                                   created ),
                                 true );
    }

    return 0;
}


wxString MULTICHANNEL_TOOL::JoinReferences( const std::set<FOOTPRINT*>& aFootprints )
{
    wxArrayString refs;

    for( FOOTPRINT* fp : aFootprints )
        refs.push_back( fp->GetReference() );

    refs.Sort();
    return wxJoin( refs, ',' );
}


void MULTICHANNEL_TOOL::RefreshRepeat()
{
    if( m_areas.FromSelection() )
        updateSelectionCore();

    UpdateRepeatPreview();
}


int MULTICHANNEL_TOOL::ApplyRepeat()
{
    clearRepeatPreview();

    if( m_areas.FromSelection() )
        return applySelectionRepeat();

    return RepeatLayout( TOOL_EVENT(), m_areas.m_refRA->m_zone );
}


void MULTICHANNEL_TOOL::UpdateRepeatPreview()
{
    KIGFX::VIEW* view = getView();

    clearRepeatPreview();

    if( !view || !m_areas.m_refRA )
        return;

    PROF_TIMER timer;

    // The copy is staged and reverted; only clones of its result stay on screen
    auto                  savedCompat = m_areas.m_compatMap;
    REPEAT_LAYOUT_OPTIONS savedOptions = m_areas.m_options;
    BOARD_COMMIT          commit( GetManager(), true, false );

    // Grouping changes nothing visible and would stage edits to unrelated groups
    m_areas.m_options.m_groupItems = false;
    applyRepeat( commit );

    auto previewClone =
            [&]( BOARD_ITEM* aItem ) -> BOARD_ITEM*
            {
                BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( aItem->Clone() );

                // The revert below deletes groups the copy staged, and drawing asks the parent group for its lock
                clone->SetParentGroup( nullptr );
                view->AddToPreview( clone, true );
                return clone;
            };

    auto isContainer =
            []( const BOARD_ITEM* aItem )
            {
                return aItem->Type() == PCB_GROUP_T || aItem->Type() == PCB_GENERATOR_T;
            };

    auto show =
            [&]( BOARD_ITEM* aItem )
            {
                // Group and generator clones share members, so those are shown one member at a time
                if( isContainer( aItem ) )
                {
                    aItem->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                          {
                                              if( !isContainer( aChild ) )
                                                  previewClone( aChild );
                                          },
                                          RECURSE_MODE::RECURSE );
                    return;
                }

                BOARD_ITEM* clone = previewClone( aItem );
                clone->SetBrightened();
                clone->RunOnChildren( [&]( BOARD_ITEM* aChild ) { view->AddToPreview( aChild, false ); },
                                      RECURSE_MODE::RECURSE );
            };

    for( const auto& [area, compat] : m_areas.m_compatMap )
    {
        for( BOARD_ITEM* item : compat.m_affectedItems )
            m_previewHidden.push_back( item->m_Uuid );

        for( BOARD_ITEM* item : compat.m_groupableItems )
            show( item );
    }

    // Marks and ratsnest follow the parts to where the copy puts them
    for( const auto& [area, compat] : m_areas.m_compatMap )
    {
        for( BOARD_ITEM* item : compat.m_affectedItems )
        {
            if( item->Type() == PCB_FOOTPRINT_T )
            {
                FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );

                m_previewBoxes[fp] = fp->GetBoundingBox( false );
            }
        }
    }

    if( !m_previewBoxes.empty() )
    {
        std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
        std::vector<BOARD_ITEM*>           moved;

        for( const auto& [fp, box] : m_previewBoxes )
            moved.push_back( fp );

        for( BOARD_ITEM* fp : moved )
            connectivity->Update( fp );

        connectivity->RecalculateRatsnest();

        auto movedPad =
                [&]( const std::shared_ptr<const CN_ANCHOR>& aNode )
                {
                    BOARD_CONNECTED_ITEM* parent = aNode->Parent();

                    return parent && parent->Type() == PCB_PAD_T
                           && m_previewBoxes.contains( parent->GetParentFootprint() );
                };

        for( const CN_EDGE& edge : connectivity->GetRatsnestForItems( moved ) )
        {
            const std::shared_ptr<const CN_ANCHOR>& a = edge.GetSourceNode();
            const std::shared_ptr<const CN_ANCHOR>& b = edge.GetTargetNode();

            if( a && b && !a->Dirty() && !b->Dirty() && ( movedPad( a ) || movedPad( b ) ) )
                m_previewRatsnest.emplace_back( a->Pos(), b->Pos() );
        }
    }

    // Revert swaps in fresh footprint children, so the originals are hidden only once it is done
    commit.Revert();
    setPreviewHidden( true );
    hideMovedRatsnest();
    m_areas.m_compatMap = std::move( savedCompat );
    m_areas.m_options = savedOptions;
    canvas()->Refresh();
    HighlightRepeatAreas( m_focusArea );
    wxLogTrace( traceMultichannelTool, wxT( "Preview: %s" ), timer.to_string() );
}


void MULTICHANNEL_TOOL::hideMovedRatsnest()
{
    if( m_previewBoxes.empty() )
        return;

    // Lines at the old positions would otherwise sit beside the previewed ones
    std::vector<BOARD_ITEM*> moved;

    for( const auto& [fp, box] : m_previewBoxes )
        moved.push_back( fp );

    board()->GetConnectivity()->BlockRatsnestItems( moved );
    canvas()->RedrawRatsnest();
}


std::set<BOARD_CONNECTED_ITEM*> MULTICHANNEL_TOOL::findRepeatRouting( RULE_AREA* aArea, const std::set<int>& aNets,
                                                                      const REPEAT_LAYOUT_OPTIONS& aOpts )
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();
    std::set<BOARD_CONNECTED_ITEM*>    scanned;
    std::set<BOARD_CONNECTED_ITEM*>    routing;

    findRoutingInRuleArea( aArea, scanned );

    for( BOARD_CONNECTED_ITEM* item : scanned )
    {
        if( aNets.contains( item->GetNetCode() ) )
            routing.insert( item );
    }

    // A linked channel's group already holds everything it carries
    if( aOpts.m_connectedRoutingOnly || linkedChannelGroup( board(), aArea ) )
        return routing;

    const std::set<BOARD_CONNECTED_ITEM*> inside = routing;

    for( BOARD_CONNECTED_ITEM* item : inside )
    {
        if( item->Type() == PCB_PAD_T )
            continue;

        for( CN_ITEM* cnItem : connectivity->GetConnectivityAlgo()->ItemEntry( item ).GetItems() )
        {
            for( CN_ITEM* touching : cnItem->ConnectedItems() )
            {
                BOARD_CONNECTED_ITEM* next = touching->Valid() ? touching->Parent() : nullptr;

                if( !next || ( next->Type() != PCB_TRACE_T && next->Type() != PCB_ARC_T && next->Type() != PCB_VIA_T ) )
                    continue;

                // Tracks of a meander travel with their generator
                EDA_GROUP* parent = next->GetParentGroup();

                if( parent && parent->AsEdaItem()->Type() == PCB_GENERATOR_T )
                    continue;

                if( next->GetNetCode() == item->GetNetCode() && aNets.contains( next->GetNetCode() ) )
                    routing.insert( next );
            }
        }
    }

    return routing;
}


void MULTICHANNEL_TOOL::HighlightRepeatAreas( const RULE_AREA* aArea )
{
    KIGFX::VIEW* view = getView();

    if( !view )
        return;

    m_focusArea = aArea;

    if( !m_highlight )
        m_highlight = view->MakeOverlay();

    m_highlight->Clear();

    auto mark =
            [&]( const RULE_AREA* aMarked, const KIGFX::COLOR4D& aColor )
            {
                m_highlight->SetIsStroke( true );
                m_highlight->SetIsFill( true );
                m_highlight->SetStrokeColor( aColor.WithAlpha( 0.9 ) );
                m_highlight->SetFillColor( aColor.WithAlpha( 0.08 ) );
                m_highlight->SetLineWidth( pcbIUScale.mmToIU( 0.2 ) );

                for( FOOTPRINT* fp : aMarked->m_components )
                {
                    auto  moved = m_previewBoxes.find( fp );
                    BOX2I box = moved != m_previewBoxes.end() ? moved->second : fp->GetBoundingBox( false );

                    m_highlight->Rectangle( box.GetOrigin(), box.GetEnd() );
                }
            };

    if( m_areas.m_refRA )
        mark( m_areas.m_refRA, KIGFX::COLOR4D( 0.1, 0.8, 0.3, 1.0 ) );

    if( aArea )
        mark( aArea, KIGFX::COLOR4D( 1.0, 0.6, 0.1, 1.0 ) );

    m_highlight->SetIsFill( false );
    m_highlight->SetStrokeColor( KIGFX::COLOR4D( 0.6, 0.8, 1.0, 0.8 ) );
    m_highlight->SetLineWidth( pcbIUScale.mmToIU( 0.05 ) );

    for( const SEG& line : m_previewRatsnest )
        m_highlight->Line( line.A, line.B );

    view->Update( m_highlight.get() );
    canvas()->Refresh();
}


void MULTICHANNEL_TOOL::clearRepeatPreview()
{
    KIGFX::VIEW* view = getView();

    if( !view )
        return;

    view->ClearPreview();
    setPreviewHidden( false );
    m_previewHidden.clear();

    if( !m_previewBoxes.empty() )
    {
        board()->GetConnectivity()->ClearLocalRatsnest();
        canvas()->RedrawRatsnest();
    }

    m_previewBoxes.clear();
    m_previewRatsnest.clear();

    if( m_highlight )
    {
        view->Remove( m_highlight.get() );
        m_highlight.reset();
    }

    canvas()->Refresh();
}


void MULTICHANNEL_TOOL::setPreviewHidden( bool aHide )
{
    KIGFX::VIEW* view = getView();

    // Hidden items may have been deleted since, so they are found again by UUID
    for( const KIID& id : m_previewHidden )
    {
        BOARD_ITEM* item = board()->ResolveItem( id, true );

        if( !item || !view->HasItem( item ) )
            continue;

        view->Hide( item, aHide );
        item->RunOnChildren( [&]( BOARD_ITEM* aChild ) { view->Hide( aChild, aHide ); }, RECURSE_MODE::NO_RECURSE );
    }
}


void MULTICHANNEL_TOOL::closeRepeatDialog()
{
    if( m_dialog )
        m_dialog->Close( true );
}


void MULTICHANNEL_TOOL::onRepeatDialogClosed( wxCloseEvent& aEvent )
{
    clearRepeatPreview();
    m_focusArea = nullptr;

    m_dialog->Unbind( wxEVT_CLOSE_WINDOW, &MULTICHANNEL_TOOL::onRepeatDialogClosed, this );
    m_dialog->Destroy();
    m_dialog = nullptr;

    m_areas = RULE_AREAS_DATA();
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

    std::vector<TOPOLOGY_JOB> jobs;

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( ra.m_zone == m_areas.m_refRA->m_zone )
            continue;

        jobs.push_back( { m_areas.m_refRA, &ra, &m_areas.m_compatMap[&ra] } );
    }

    if( !resolveTopologies( _( "Checking Rule Area compatibility..." ), jobs ) )
    {
        m_areas.m_compatMap.clear();
        return -1;
    }

    return 0;
}


bool MULTICHANNEL_TOOL::resolveTopologies( const wxString& aTitle, const std::vector<TOPOLOGY_JOB>& aJobs )
{
    if( aJobs.empty() )
        return true;

    int               total = static_cast<int>( aJobs.size() );
    std::atomic<int>  completed( 0 );
    std::atomic<bool> cancelled( false );
    std::atomic<int>  matchedComponents( 0 );
    std::atomic<int>  totalComponents( 0 );

    TMATCH::ISOMORPHISM_PARAMS isoParams;
    isoParams.m_cancelled = &cancelled;
    isoParams.m_matchedComponents = &matchedComponents;
    isoParams.m_totalComponents = &totalComponents;

    // Process RA resolutions sequentially on a single background thread.
    // Each resolveConnectionTopology call internally parallelizes its MRV scan
    // across the thread pool, creating many short-lived tasks that fully utilize
    // all available cores.  Running the outer loop sequentially avoids thread
    // pool starvation from nested parallelism.
    // The loop waits on those tasks, so it runs outside the pool lest a one-thread pool deadlock
    auto future = std::async(
            std::launch::async,
            [this, &aJobs, &completed, &cancelled, &matchedComponents, &isoParams]()
            {
                for( const TOPOLOGY_JOB& job : aJobs )
                {
                    if( cancelled.load( std::memory_order_relaxed ) )
                        break;

                    matchedComponents.store( 0, std::memory_order_relaxed );
                    resolveConnectionTopology( job.m_ref, job.m_target, *job.m_compat, isoParams );

                    if( cancelled.load( std::memory_order_relaxed ) )
                        break;

                    completed.fetch_add( 1, std::memory_order_relaxed );
                }
            } );

    if( Pgm().IsGUI() )
    {
        // Declared first so the windows are enabled again only after the progress dialog is gone, the
        // Repeat dialog last so it is raised over the board
        std::unique_ptr<WINDOW_DISABLER>      dialogDisabler;
        std::unique_ptr<WINDOW_DISABLER>      frameDisabler;
        std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
        auto startTime = std::chrono::steady_clock::now();
        double highWaterMark = 0.0;

        while( future.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready )
        {
            if( !reporter )
            {
                // Short enough to feel immediate, long enough not to flash for a small board
                if( std::chrono::steady_clock::now() - startTime > std::chrono::milliseconds( 250 ) )
                {
                    // The progress dialog pumps events from its first Show, and the board and a non-modal
                    // Repeat dialog must not change while the worker reads them
                    dialogDisabler = std::make_unique<WINDOW_DISABLER>( m_dialog );
                    frameDisabler = std::make_unique<WINDOW_DISABLER>( frame() );
                    reporter = std::make_unique<WX_PROGRESS_REPORTER>( frame(), aTitle, 1, PR_CAN_ABORT );
                }
                else
                {
                    // Flush background-thread log messages so timing traces appear promptly
                    wxLog::FlushActive();
                }
            }

            if( reporter )
            {
                int done = completed.load( std::memory_order_relaxed );
                int matched = matchedComponents.load( std::memory_order_relaxed );
                int compTotal = totalComponents.load( std::memory_order_relaxed );

                double fraction = ( compTotal > 0 )
                                          ? static_cast<double>( matched ) / compTotal
                                          : 0.0;
                double progress = static_cast<double>( done + fraction ) / total;

                if( progress > highWaterMark )
                    highWaterMark = progress;

                reporter->SetCurrentProgress( highWaterMark );
                reporter->Report( wxString::Format(
                        _( "Resolving topology %d of %d (%d/%d components)" ),
                        std::min( done + 1, total ), total, matched, compTotal ) );

                if( !reporter->KeepRefreshing() )
                    cancelled.store( true, std::memory_order_relaxed );
            }
        }
    }
    else
    {
        future.wait();
    }

    if( !cancelled.load( std::memory_order_relaxed ) )
        return true;

    // A job stopped part way says no more about its topology than one never started
    for( size_t i = static_cast<size_t>( completed.load() ); i < aJobs.size(); ++i )
    {
        aJobs[i].m_compat->m_isOk = false;
        aJobs[i].m_compat->m_errorMsg = _( "Cancelled" );
        aJobs[i].m_compat->m_mismatchReasons.clear();
    }

    return false;
}


int MULTICHANNEL_TOOL::RepeatLayout( const TOOL_EVENT& aEvent, RULE_AREA& aRefArea, RULE_AREA& aTargetArea,
                                     REPEAT_LAYOUT_OPTIONS& aOptions, BOARD_COMMIT* aExternalCommit,
                                     wxString* aErrorOut )
{
    wxCHECK_MSG( aRefArea.m_zone, -1, wxT( "Reference Rule Area has no zone." ) );
    wxCHECK_MSG( aTargetArea.m_zone, -1, wxT( "Target Rule Area has no zone." ) );

    const bool silent = aErrorOut != nullptr;

    auto reportError = [&]( const wxString& aMsg )
    {
        if( aErrorOut )
            *aErrorOut = aMsg;
        else if( Pgm().IsGUI() )
            frame()->ShowInfoBarError( aMsg, true );
    };

    RULE_AREA_COMPAT_DATA compat;

    if( !resolveConnectionTopology( &aRefArea, &aTargetArea, compat ) )
    {
        if( silent )
        {
            *aErrorOut = compat.m_mismatchReasons.empty() ? compat.m_errorMsg
                                                          : JoinMismatchReasons( compat.m_mismatchReasons );
        }
        else if( Pgm().IsGUI() )
        {
            wxString summary = wxString::Format( _( "Rule Area topologies do not match: %s" ), compat.m_errorMsg );
            ShowTopologyMismatchReasons( frame(), summary, compat.m_mismatchReasons );
        }

        return -1;
    }

    std::optional<BOARD_COMMIT> localCommit;

    if( !aExternalCommit )
        localCommit.emplace( GetManager(), true, false );

    BOARD_COMMIT& commit = aExternalCommit ? *aExternalCommit : *localCommit;

    // If no anchor is provided, pick the first matched pair to avoid center-alignment shifting
    // the whole group. This keeps Apply Design Block Layout from moving the group to wherever
    // the source design block happened to be placed.
    if( aRefArea.m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK
        && aTargetArea.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT && aOptions.m_anchor == niluuid )
    {
        if( !compat.m_matchingComponents.empty() )
            aOptions.m_anchor = compat.m_matchingComponents.begin()->first->m_Uuid;
    }

    copyRuleAreaContents( &aRefArea, &aTargetArea, &commit, aOptions, compat );

    if( aTargetArea.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
    {
        EDA_GROUP* group = aTargetArea.m_group;

        if( !group && !aTargetArea.m_components.empty() )
            group = ( *aTargetArea.m_components.begin() )->GetParentGroup();

        if( !group )
        {
            if( !aExternalCommit )
                commit.Revert();

            reportError( _( "Target group does not have a group." ) );

            return -1;
        }

        commit.Modify( group->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );

        for( BOARD_ITEM* item : compat.m_groupableItems )
        {
            // Items already in a nested group stay there so the nesting survives
            if( group->ContainsItem( item ) )
                continue;

            commit.Modify( item );
            group->AddItem( item );
        }
    }

    if( !aExternalCommit )
        commit.Push( _( "Repeat Layout" ) );

    return 0;
}


int MULTICHANNEL_TOOL::applyRepeat( BOARD_COMMIT& aCommit )
{
    int totalCopied = 0;

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

        copyRuleAreaContents( m_areas.m_refRA, targetArea, &aCommit, m_areas.m_options, compatData );
        totalCopied++;
    }

    if( m_areas.m_options.m_groupItems )
    {
        for( const auto& [targetArea, compatData] : m_areas.m_compatMap )
        {
            if( compatData.m_groupableItems.size() < 2 )
                continue;

            // A group-driven area already has a group.
            EDA_GROUP* group =
                    targetArea->m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT ? targetArea->m_group : nullptr;

            if( group )
            {
                aCommit.Modify( group->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );
            }
            else
            {
                pruneExistingGroups( aCommit, compatData.m_affectedItems );

                group = new PCB_GROUP( board() );
                aCommit.Add( group->AsEdaItem() );
            }

            for( BOARD_ITEM* item : compatData.m_groupableItems )
                stageAddToGroup( aCommit, group, item );
        }
    }

    return totalCopied;
}


int MULTICHANNEL_TOOL::RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone )
{
    BOARD_COMMIT commit( GetManager(), true, false );
    int          totalCopied = applyRepeat( commit );

    if( m_areas.m_linkAreas )
    {
        std::vector<RULE_AREA*> targets;

        for( const auto& [targetArea, compatData] : m_areas.m_compatMap )
        {
            if( compatData.m_doCopy && compatData.m_isOk )
                targets.push_back( targetArea );
        }

        linkAreas( commit, aRefZone, targets );
    }

    commit.Push( _( "Repeat Layout" ) );

    if( Pgm().IsGUI() )
        frame()->ShowInfoBarMsg( wxString::Format( _( "Copied to %d Rule Areas." ), totalCopied ), true );

    return 0;
}


void MULTICHANNEL_TOOL::detachArea( BOARD_COMMIT& aCommit, ZONE* aZone )
{
    if( aZone->GetBoard() )
        aCommit.Modify( aZone );

    aZone->SetLinkSet( niluuid );
    aZone->SetRepeatOptions( REPEAT_LAYOUT_OPTIONS() );
}


void MULTICHANNEL_TOOL::linkAreas( BOARD_COMMIT& aCommit, ZONE* aRefZone, const std::vector<RULE_AREA*>& aTargets )
{
    const REPEAT_LAYOUT_OPTIONS& opts = m_areas.m_options;
    const KIID                   linkSet;
    std::set<KIID>               oldSets;

    // New zones are not on the board yet, so the commit has nothing to snapshot
    auto join = [&]( ZONE* aZone, const KIID& aAnchor )
                {
                    if( aZone->GetBoard() )
                        aCommit.Modify( aZone );

                    REPEAT_LAYOUT_OPTIONS own = opts;
                    own.m_anchor = aAnchor;

                    if( aZone->GetLinkSet() != niluuid )
                        oldSets.insert( aZone->GetLinkSet() );

                    aZone->SetLinkSet( linkSet );
                    aZone->SetRepeatOptions( own );
                };

    join( aRefZone, opts.m_anchor );

    for( RULE_AREA* target : aTargets )
    {
        const TMATCH::COMPONENT_MATCHES& matches = m_areas.m_compatMap[target].m_matchingComponents;
        KIID                             anchor = niluuid;

        // Each area keeps its own counterpart of the anchor, so any of them can be the source later
        if( opts.m_anchor != niluuid && !matches.empty() )
            anchor = anchorPair( matches, opts.m_anchor ).second->m_Uuid;

        join( target->m_zone, anchor );
    }

    // Peers that were left out stay linked to each other unless they are alone
    dropLoneLinks( aCommit, oldSets );
}


void MULTICHANNEL_TOOL::dropLoneLinks( BOARD_COMMIT& aCommit, const std::set<KIID>& aSets )
{
    std::map<KIID, std::vector<ZONE*>> members;

    for( ZONE* zone : board()->Zones() )
    {
        if( aSets.contains( zone->GetLinkSet() ) )
            members[zone->GetLinkSet()].push_back( zone );
    }

    for( const auto& [linkSet, zones] : members )
    {
        if( zones.size() < 2 )
            detachArea( aCommit, zones.front() );
    }
}


bool MULTICHANNEL_TOOL::routerBusy() const
{
    // A router keeps its own copy of the board while routing; editing under it leaves it stale
    ROUTER_TOOL*    router = m_toolMgr->GetTool<ROUTER_TOOL>();
    GENERATOR_TOOL* generators = m_toolMgr->GetTool<GENERATOR_TOOL>();

    return ( router && router->RoutingInProgress() )
           || ( generators && generators->Router() && generators->Router()->RoutingInProgress() );
}


int MULTICHANNEL_TOOL::updateLinkedLayouts( const TOOL_EVENT& aEvent )
{
    std::set<ZONE*> sources = linkedAreasIn( selection() );

    if( sources.size() != 1 )
    {
        frame()->ShowInfoBarMsg( sources.empty() ? _( "Select a linked Rule Area, or a part of one, to update the "
                                                      "others from." )
                                                 : _( "Select items from a single linked channel." ),
                                 true );
        return 0;
    }

    if( routerBusy() )
    {
        frame()->ShowInfoBarWarning( _( "Finish routing before updating linked layouts." ), true );
        return 0;
    }

    UpdateLinkedLayouts( *sources.begin() );
    return 0;
}


int MULTICHANNEL_TOOL::unlinkLayout( const TOOL_EVENT& aEvent )
{
    std::set<ZONE*> members = linkedAreasIn( selection() );

    if( members.empty() )
        frame()->ShowInfoBarMsg( _( "Select a linked Rule Area, or a part of one, to unlink." ), true );
    else
        UnlinkAreas( members );

    return 0;
}


void MULTICHANNEL_TOOL::UnlinkAreas( const std::set<ZONE*>& aZones )
{
    BOARD_COMMIT   commit( GetManager(), true, false );
    std::set<KIID> oldSets;

    for( ZONE* zone : aZones )
    {
        if( zone->GetLinkSet() != niluuid )
            oldSets.insert( zone->GetLinkSet() );

        detachArea( commit, zone );
    }

    dropLoneLinks( commit, oldSets );
    commit.Push( _( "Unlink Layout" ) );
}


bool MULTICHANNEL_TOOL::UpdateLinkedLayouts( ZONE* aSource )
{
    // The dialog's session lives in the same data this rebuilds
    closeRepeatDialog();

    FindExistingRuleAreas();

    RULE_AREA*                ref = nullptr;
    std::vector<RULE_AREA*>   linked;
    std::map<FOOTPRINT*, int> footprintUse;

    for( RULE_AREA& area : m_areas.m_areas )
    {
        if( area.m_zone->GetLinkSet() == niluuid )
            continue;

        linked.push_back( &area );

        if( area.m_zone == aSource )
            ref = &area;

        for( FOOTPRINT* fp : area.m_components )
            footprintUse[fp]++;
    }

    if( !ref )
        return false;

    // A target sharing parts with another linked area would overwrite that area's layout
    auto sharesParts =
            [&]( RULE_AREA* aArea )
            {
                return std::ranges::any_of( aArea->m_components,
                                            [&]( FOOTPRINT* aFp ) { return footprintUse[aFp] > 1; } );
            };

    // Copper is cleared and copied by area, unless both ends hold exactly their own group's items
    auto overlapsOtherArea =
            [&]( RULE_AREA* aArea, const SHAPE_POLY_SET& aOutline )
            {
                return std::ranges::any_of( linked,
                                            [&]( RULE_AREA* aOther )
                                            {
                                                return aOther != aArea
                                                       && outlinesOverlap( aOutline, *aOther->m_zone->Outline() );
                                            } );
            };

    const bool refGrouped = linkedChannelGroup( board(), ref ) != nullptr;

    // A source lying over another linked area would copy that area's contents too
    if( sharesParts( ref ) || ( !refGrouped && overlapsOtherArea( ref, *ref->m_zone->Outline() ) ) )
    {
        if( Pgm().IsGUI() )
        {
            frame()->ShowInfoBarWarning(
                    wxString::Format( _( "'%s' overlaps another linked Rule Area." ), ref->m_ruleName ), true );
        }

        return false;
    }

    BOARD_COMMIT                commit( GetManager(), true, false );
    std::vector<wxString>       problems;
    std::vector<SHAPE_POLY_SET> acceptedOutlines;

    m_areas.m_refRA = ref;
    m_areas.m_options = aSource->GetRepeatOptions();
    m_areas.m_compatMap.clear();

    std::vector<RULE_AREA*>   targets;
    std::vector<TOPOLOGY_JOB> jobs;

    for( RULE_AREA* target : linked )
    {
        if( target == ref || target->m_zone->GetLinkSet() != aSource->GetLinkSet() )
            continue;

        targets.push_back( target );
        jobs.push_back( { ref, target, &m_areas.m_compatMap[target] } );
    }

    if( !resolveTopologies( _( "Updating linked layouts..." ), jobs ) )
    {
        m_areas.m_compatMap.clear();
        return false;
    }

    for( RULE_AREA* target : targets )
    {
        RULE_AREA_COMPAT_DATA& compat = m_areas.m_compatMap[target];

        if( !compat.m_isOk )
        {
            problems.push_back( wxString::Format( wxT( "'%s': %s" ), target->m_ruleName, compat.m_errorMsg ) );
            continue;
        }

        // Unless both ends are grouped, copying clears the target by area, so both its outline now and the
        // one the copy gives it must stay clear of every other linked area
        VECTOR2I       disp;
        EDA_ANGLE      rot;
        SHAPE_POLY_SET repeated( repeatedOutline( ref, target, m_areas.m_options.m_anchor,
                                                  compat.m_matchingComponents, disp, rot ) );

        bool overlapsAccepted = std::ranges::any_of( acceptedOutlines,
                                                     [&]( const SHAPE_POLY_SET& aOther )
                                                     {
                                                         return outlinesOverlap( repeated, aOther );
                                                     } );

        bool scoped = refGrouped && linkedChannelGroup( board(), target ) != nullptr;

        if( sharesParts( target )
            || ( !scoped && ( overlapsAccepted || overlapsOtherArea( target, *target->m_zone->Outline() )
                              || overlapsOtherArea( target, repeated ) ) ) )
        {
            problems.push_back(
                    wxString::Format( _( "'%s' overlaps another linked Rule Area." ), target->m_ruleName ) );
            continue;
        }

        compat.m_doCopy = true;
        acceptedOutlines.push_back( std::move( repeated ) );
    }

    int copied = applyRepeat( commit );

    if( !problems.empty() && Pgm().IsGUI() )
    {
        frame()->ShowInfoBarWarning(
                wxString::Format( _( "Some linked Rule Areas were not updated. %s" ), JoinMismatchReasons( problems ) ),
                true );
    }

    if( !copied )
        return false;

    commit.Push( _( "Update Linked Layouts" ), SKIP_ENTERED_GROUP );
    return true;
}


int MULTICHANNEL_TOOL::findRoutingInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_CONNECTED_ITEM*>& aOutput ) const
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
                // Zones are handled by the "copy other items" path, we need this check here
                // because design blocks explicitly include them as part of the block contents,
                // but other RA types grab them by querying the board for items enclosed by the RA polygon
                if( bci->Type() == PCB_ZONE_T )
                    continue;

                // Tracks inside a generator (meander) are copied with the generator.
                if( EDA_GROUP* parent = bci->GetParentGroup() )
                {
                    if( parent->AsEdaItem()->Type() == PCB_GENERATOR_T )
                        continue;
                }

                if( bci->IsConnected() )
                    aOutput.insert( bci );
            }
        }

        if( !aRuleArea->m_fromSelection )
            return (int) aOutput.size();

        count = (int) aOutput.size();
    }

    if( EDA_GROUP* group = linkedChannelGroup( board(), aRuleArea ) )
    {
        std::set<BOARD_ITEM*> members;
        collectGroupBoardItems( group, members );

        for( BOARD_ITEM* member : members )
        {
            BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( member );

            if( !bci || bci->Type() == PCB_PAD_T || bci->Type() == PCB_ZONE_T || !bci->IsConnected() )
                continue;

            if( EDA_GROUP* parent = bci->GetParentGroup() )
            {
                if( parent->AsEdaItem()->Type() == PCB_GENERATOR_T )
                    continue;
            }

            aOutput.insert( bci );
            count++;
        }

        return count;
    }

    // The design-block apply target uses a scratch zone not on the board, so enclosedByArea()
    // below finds nothing. Match routing against the zone outline directly.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT || aRuleArea->m_fromSelection )
    {
        bool zoneOnBoard = false;

        for( ZONE* zone : board()->Zones() )
        {
            if( zone == aRuleArea->m_zone )
            {
                zoneOnBoard = true;
                break;
            }
        }

        if( !zoneOnBoard )
        {
            const SHAPE_POLY_SET& areaOutline = *aRuleArea->m_zone->Outline();
            const BOX2I           areaBox = areaOutline.BBox();
            int                   maxError = board()->GetDesignSettings().m_MaxError;

            auto enclosedByZone = [&]( BOARD_CONNECTED_ITEM* aItem )
            {
                if( aOutput.contains( aItem ) )
                    return;

                // Tracks inside a generator (meander) are removed with the generator.
                if( EDA_GROUP* parent = aItem->GetParentGroup() )
                {
                    if( parent->AsEdaItem()->Type() == PCB_GENERATOR_T )
                        return;
                }

                if( !( aRuleArea->m_zone->GetLayerSet() & aItem->GetLayerSet() ).any()
                    || !areaBox.Contains( aItem->GetBoundingBox() ) )
                {
                    return;
                }

                SHAPE_POLY_SET itemShape;
                aItem->TransformShapeToPolygon( itemShape, aItem->GetLayer(), 0, maxError, ERROR_OUTSIDE );

                if( itemShape.IsEmpty() )
                    return;

                itemShape.BooleanSubtract( areaOutline );

                if( itemShape.IsEmpty() )
                {
                    aOutput.insert( aItem );
                    count++;
                }
            };

            for( PCB_TRACK* track : board()->Tracks() )
                enclosedByZone( track );

            for( BOARD_ITEM* drawing : board()->Drawings() )
            {
                if( drawing->IsConnected() )
                    enclosedByZone( static_cast<BOARD_CONNECTED_ITEM*>( drawing ) );
            }

            return count;
        }
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

    const BOX2I zoneBox = aRuleArea->m_zone->Outline()->BBox();

    auto testAndAdd =
            [&]( BOARD_CONNECTED_ITEM* aItem )
            {
                if( aOutput.contains( aItem ) )
                    return;

                // Tracks inside a generator (meander) are copied with the generator.
                if( EDA_GROUP* parent = aItem->GetParentGroup() )
                {
                    if( parent->AsEdaItem()->Type() == PCB_GENERATOR_T )
                        return;
                }

                // Items outside the area's box cannot be enclosed, so skip the costly evaluation
                if( !( aRuleArea->m_zone->GetLayerSet() & aItem->GetLayerSet() ).any()
                    || !zoneBox.Intersects( aItem->GetBoundingBox() ) )
                {
                    return;
                }

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


void MULTICHANNEL_TOOL::copyRuleAreaContents( RULE_AREA* aRefArea, RULE_AREA* aTargetArea, BOARD_COMMIT* aCommit,
                                              REPEAT_LAYOUT_OPTIONS aOpts, RULE_AREA_COMPAT_DATA& aCompatData )
{
    VECTOR2I         disp;
    EDA_ANGLE        rot;
    SHAPE_LINE_CHAIN newTargetOutline =
            repeatedOutline( aRefArea, aTargetArea, aOpts.m_anchor, aCompatData.m_matchingComponents, disp, rot );

    // Reference net to the net of the first matching target pad, built once for all copied items
    const bool         hasMatches = !aCompatData.m_matchingComponents.empty();
    std::map<int, int> netMap;

    for( const auto& [refFP, targetFP] : aCompatData.m_matchingComponents )
    {
        for( const PAD* refPad : refFP->Pads() )
        {
            if( refPad->GetNetCode() <= 0 )
                continue;

            std::vector<const PAD*> targetPads = targetFP->GetPads( refPad->GetNumber() );

            if( !targetPads.empty() )
                netMap.emplace( refPad->GetNetCode(), targetPads[0]->GetNetCode() );
        }
    }

    SHAPE_POLY_SET refPoly;
    refPoly.AddOutline( aRefArea->m_zone->Outline()->COutline( 0 ) );
    refPoly.CacheTriangulation();

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();

    // Group placement targets let RepeatLayout() reuse the existing target group, and m_groupItems
    // flat-groups every copy into one rule-area group. Reconstructing source groups here in either
    // case strands their members and leaves empty phantom clones behind (issue 22316).
    const bool preserveGroups = aTargetArea->m_sourceType != PLACEMENT_SOURCE_T::GROUP_PLACEMENT
                                && !aOpts.m_groupItems;

    // Defer reconstruction until every copy is made. Cloning a source group the moment one member
    // is copied would duplicate user groups that merely overlap the source area (issue 22316); a
    // group is rebuilt only once all of its members have been reproduced.
    std::vector<std::pair<BOARD_ITEM*, BOARD_ITEM*>> groupFixupPairs;
    std::set<BOARD_ITEM*>                            reproducedSourceItems;

    auto fixupParentGroup =
            [&]( BOARD_ITEM* sourceItem, BOARD_ITEM* destItem )
            {
                // The copy inherits the source's parent-group pointer but is not a member of that
                // group; clear the dangling reference.
                destItem->SetParentGroup( nullptr );

                if( !preserveGroups )
                    return;

                if( sourceItem->GetParentGroup() )
                    groupFixupPairs.emplace_back( sourceItem, destItem );

                reproducedSourceItems.insert( sourceItem );
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

        // The source rule-area zone maps to the target zone; treat it as reproduced so a group
        // containing it can still be rebuilt.
        if( preserveGroups )
        {
            if( aRefArea->m_zone->GetParentGroup() )
                groupFixupPairs.emplace_back( aRefArea->m_zone, aTargetArea->m_zone );

            reproducedSourceItems.insert( aRefArea->m_zone );
        }
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

        // What is removed from the target spans the same run as what is copied from the reference
        if( aRefArea->ScansContents() )
        {
            targetRouting = findRepeatRouting( aTargetArea, targc, aOpts );
            refRouting = findRepeatRouting( aRefArea, refc, aOpts );
        }
        else
        {
            findRoutingInRuleArea( aTargetArea, targetRouting );
            findRoutingInRuleArea( aRefArea, refRouting );
        }

        // Nets used by the target group's own items, footprint pads included. Taken from the area's
        // footprints too, since a channel created from a selection fills its group only when applied
        std::set<int> targetGroupNets;

        for( FOOTPRINT* fp : aTargetArea->m_components )
        {
            for( PAD* pad : fp->Pads() )
                targetGroupNets.insert( pad->GetNetCode() );
        }

        if( aTargetArea->m_group )
        {
            std::set<BOARD_ITEM*> members;
            collectGroupBoardItems( aTargetArea->m_group, members );

            for( BOARD_ITEM* member : members )
            {
                if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( member ) )
                    targetGroupNets.insert( bci->GetNetCode() );
            }
        }

        // A channel made from a selection or a link replaces loose routing only on its own nets; buses
        // and rails it shares stay. A design block replaces it on every net of the group
        const bool    channelScope = aRefArea->m_fromSelection || linkedChannelGroup( board(), aTargetArea );
        std::set<int> targetChannelNets;

        for( int net : targetGroupNets )
        {
            if( !channelScope )
            {
                targetChannelNets.insert( net );
                continue;
            }

            std::vector<FOOTPRINT*> owners;

            for( BOARD_CONNECTED_ITEM* pad : connectivity->GetNetItems( net, { PCB_PAD_T } ) )
            {
                if( std::ranges::find( owners, pad->GetParentFootprint() ) == owners.end() )
                    owners.push_back( pad->GetParentFootprint() );
            }

            if( isChannelNet( owners, aTargetArea->m_components ) )
                targetChannelNets.insert( net );
        }

        for( BOARD_CONNECTED_ITEM* item : targetRouting )
        {
            // Never remove pads as part of routing copy.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( aRefArea->m_designBlockItems.count( item ) )
                continue;

            // Design block apply: replace only the group's own routing, nested groups included, and
            // loose routing on the group's nets. Other groups' routing belongs to stacked instances (issue 24767).
            // Everything else is unrelated and just sits inside the block's area (issue 24944).
            if( aTargetArea->m_group && !aTargetArea->m_group->ContainsItem( item ) )
            {
                if( item->GetParentGroup() )
                    continue;

                if( item->IsLocked() )
                    continue;

                if( item->GetNetCode() <= 0 || !targetChannelNets.contains( item->GetNetCode() ) )
                    continue;
            }

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            if( ( aOpts.m_connectedRoutingOnly || aRefArea->ScansContents() ) && !targc.contains( item->GetNetCode() ) )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( !aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
            {
                continue;
            }

            aCompatData.m_affectedItems.insert( item );
            aCommit->Remove( item );
        }

        for( BOARD_CONNECTED_ITEM* item : refRouting )
        {
            // Never copy pads as part of routing copy.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            if( ( aOpts.m_connectedRoutingOnly || aRefArea->ScansContents() ) && !refc.contains( item->GetNetCode() ) )
                continue;

            if( !aRefArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            if( !aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            BOARD_CONNECTED_ITEM* copied = static_cast<BOARD_CONNECTED_ITEM*>( item->Duplicate( false ) );

            fixupNet( item, copied, hasMatches, netMap );
            fixupParentGroup( item, copied );

            copied->Rotate( VECTOR2( 0, 0 ), rot );
            copied->Move( disp );
            aCompatData.m_groupableItems.insert( copied );
            aCommit->Add( copied );
        }

        // Copy generators (meanders) whole so they are not flattened to loose tracks. Design
        // block apply has an exact item list, other rule areas resolve them by area.
        std::vector<PCB_GENERATOR*> refGenerators;
        std::vector<PCB_GENERATOR*> targetGenerators;

        auto appendGenerators =
                [&]( EDA_GROUP* aGroup, std::vector<PCB_GENERATOR*>& aOut )
                {
                    std::set<BOARD_ITEM*> members;
                    collectGroupBoardItems( aGroup, members );

                    for( BOARD_ITEM* member : members )
                    {
                        if( member->Type() == PCB_GENERATOR_T )
                            aOut.push_back( static_cast<PCB_GENERATOR*>( member ) );
                    }
                };

        if( aRefArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK )
        {
            for( EDA_ITEM* item : aRefArea->m_designBlockItems )
            {
                if( item->Type() == PCB_GENERATOR_T )
                    refGenerators.push_back( static_cast<PCB_GENERATOR*>( item ) );
            }

            EDA_GROUP* targetGroup = aTargetArea->m_group;

            if( !targetGroup && !aTargetArea->m_components.empty() )
                targetGroup = ( *aTargetArea->m_components.begin() )->GetParentGroup();

            if( targetGroup )
                appendGenerators( targetGroup, targetGenerators );
        }
        else
        {
            // Each side takes its generators from its group when it is a linked channel, else from its outline
            EDA_GROUP* refGroup = linkedChannelGroup( board(), aRefArea );
            EDA_GROUP* targetGroup = linkedChannelGroup( board(), aTargetArea );

            if( refGroup )
                appendGenerators( refGroup, refGenerators );

            if( targetGroup )
                appendGenerators( targetGroup, targetGenerators );

            if( !refGroup || !targetGroup )
            {
                const SHAPE_LINE_CHAIN& refOut = aRefArea->m_zone->Outline()->COutline( 0 );
                const SHAPE_LINE_CHAIN& targetOut = aTargetArea->m_zone->Outline()->COutline( 0 );

                for( PCB_GENERATOR* gen : board()->Generators() )
                {
                    if( gen->GetGeneratorType() != wxT( "tuning_pattern" ) )
                        continue;

                    if( !refGroup && gen->HitTest( refOut, false ) )
                        refGenerators.push_back( gen );
                    else if( !targetGroup && !( refGroup && gen->HitTest( refOut, false ) )
                             && gen->HitTest( targetOut, false ) )
                        targetGenerators.push_back( gen );
                }
            }
        }

        // Remove the target's existing generators so the copy replaces them.
        for( PCB_GENERATOR* gen : targetGenerators )
        {
            if( gen->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            gen->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        aCommit->Remove( child );
                    },
                    RECURSE_MODE::RECURSE );
            aCommit->Remove( gen );
            aCompatData.m_affectedItems.insert( gen );
        }

        for( PCB_GENERATOR* gen : refGenerators )
        {
            if( gen->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            PCB_GENERATOR* clone = gen->DeepClone();

            clone->ResetUuid();
            clone->RunOnChildren(
                    []( BOARD_ITEM* child )
                    {
                        child->ResetUuidDirect();
                    },
                    RECURSE_MODE::RECURSE );

            clone->ClearFlags();
            clone->Rotate( VECTOR2( 0, 0 ), rot );
            clone->Move( disp );
            aCommit->Add( clone );

            clone->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->ClearFlags();

                        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( child ) )
                            fixupNet( bci, bci, hasMatches, netMap );

                        aCommit->Add( child );
                    },
                    RECURSE_MODE::RECURSE );

            aCompatData.m_groupableItems.insert( clone );
        }
    }

    if( aOpts.m_copyOtherItems )
    {
        std::set<BOARD_ITEM*> sourceItems;
        std::set<BOARD_ITEM*> targetItems;

        findOtherItemsInRuleArea( aRefArea, sourceItems );
        findOtherItemsInRuleArea( aTargetArea, targetItems );

        // Apply Design Block Layout uses synthetic copper-only rule area zones that don't
        // reflect the layers the user actually drew on. The source items were collected by
        // explicit enumeration (m_designBlockItems) and the destination is a group bounding
        // box, so the per-item layer filter would incorrectly reject silkscreen, fab and
        // user drawings. Skip the layer filter only when both halves are the synthetic
        // design-block-to-group flow; regular GROUP_PLACEMENT rule areas have user-authored
        // layer sets that must still be honored.
        const bool skipLayerFilter = aRefArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK
                                     && aTargetArea->m_sourceType
                                                == PLACEMENT_SOURCE_T::GROUP_PLACEMENT;

        for( BOARD_ITEM* item : targetItems )
        {
            if( item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                continue;

            // Don't remove the appended source items: this geometric query can pick them up, but
            // they're deleted when the temporary append is reverted, leaving dangling pointers.
            if( aRefArea->m_designBlockItems.count( item ) )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( item->Type() == PCB_ZONE_T )
            {
                ZONE* zone = static_cast<ZONE*>( item );

                // Check all zone layers are included in the target rule area.
                if( skipLayerFilter
                    || aTargetArea->m_zone->GetLayerSet().ContainsAll( zone->GetLayerSet() ) )
                {
                    aCompatData.m_affectedItems.insert( zone );
                    aCommit->Remove( zone );
                }
            }
            else
            {
                if( skipLayerFilter
                    || aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                {
                    aCompatData.m_affectedItems.insert( item );
                    aCommit->Remove( item );
                }
            }
        }

        for( BOARD_ITEM* item : sourceItems )
        {
            if( item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
                continue;

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            BOARD_ITEM* copied = nullptr;

            if( item->Type() == PCB_ZONE_T )
            {
                ZONE* zone = static_cast<ZONE*>( item );

                if( !skipLayerFilter )
                {
                    LSET allowedLayers =
                            aRefArea->m_zone->GetLayerSet() & aTargetArea->m_zone->GetLayerSet();

                    // Check all zone layers are included in both source and target rule areas.
                    if( !allowedLayers.ContainsAll( zone->GetLayerSet() ) )
                        continue;
                }

                ZONE* targetZone = static_cast<ZONE*>( item->Duplicate( false ) );
                fixupNet( zone, targetZone, hasMatches, netMap );

                copied = targetZone;
            }
            else
            {
                if( !skipLayerFilter )
                {
                    if( !aRefArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                        continue;

                    if( !aTargetArea->m_zone->GetLayerSet().Contains( item->GetLayer() ) )
                        continue;
                }

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

            // For regular Rule Area repeat, ignore source footprints outside the reference area.
            // For Design Block apply, use the exact source item set collected from the block.
            if( aRefArea->m_sourceType != PLACEMENT_SOURCE_T::DESIGN_BLOCK
                && !linkedChannelGroup( board(), aRefArea )
                && !refFP->GetEffectiveShape( refFP->GetLayer() )->Collide( &refPoly, 0 ) )
            {
                continue;
            }

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

                if( !targetField )
                    continue;

                targetField->SetLayerSet( refField->GetLayerSet() );
                targetField->SetVisible( refField->IsVisible() );
                targetField->SetAttributes( refField->GetAttributes() );
                targetField->SetPosition( refField->GetPosition() );
                targetField->SetTextAngle( refField->GetTextAngle() );
                targetField->Rotate( VECTOR2( 0, 0 ), rot );
                targetField->Move( disp );
                targetField->SetIsKnockout( refField->IsKnockout() );
            }

            // Copy non-field text items. Texts can share content (e.g. "${REFERENCE}" on both
            // F.SilkS and B.SilkS), so match one-to-one and prefer the same layer. Otherwise both
            // collapse onto one target item and the other side's text is lost.
            std::set<PCB_TEXT*> consumedTargets;

            for( BOARD_ITEM* refItem : refFP->GraphicalItems() )
            {
                if( refItem->Type() != PCB_TEXT_T )
                    continue;

                PCB_TEXT* refText = static_cast<PCB_TEXT*>( refItem );
                PCB_TEXT* targetText = nullptr;

                for( BOARD_ITEM* targetItem : targetFP->GraphicalItems() )
                {
                    if( targetItem->Type() != PCB_TEXT_T )
                        continue;

                    PCB_TEXT* candidate = static_cast<PCB_TEXT*>( targetItem );

                    if( consumedTargets.contains( candidate ) || candidate->GetText() != refText->GetText() )
                    {
                        continue;
                    }

                    targetText = candidate;

                    if( candidate->GetLayer() == refText->GetLayer() )
                        break;
                }

                if( !targetText )
                    continue;

                consumedTargets.insert( targetText );

                targetText->SetLayer( refText->GetLayer() );
                targetText->SetVisible( refText->IsVisible() );
                targetText->SetAttributes( refText->GetAttributes() );
                targetText->SetPosition( refText->GetPosition() );
                targetText->SetTextAngle( refText->GetTextAngle() );
                targetText->Rotate( VECTOR2( 0, 0 ), rot );
                targetText->Move( disp );
                targetText->SetIsKnockout( refText->IsKnockout() );
            }

            // Copy 3D model settings
            targetFP->Models() = refFP->Models();

            std::set<PAD*> consumedPads;

            for( PAD* refPad : refFP->Pads() )
            {
                for( PAD* targetPad : targetFP->Pads() )
                {
                    if( consumedPads.contains( targetPad ) || targetPad->GetNumber() != refPad->GetNumber() )
                    {
                        continue;
                    }

                    consumedPads.insert( targetPad );
                    targetPad->ImportSettingsFrom( *refPad );
                    break;
                }
            }

            aCompatData.m_affectedItems.insert( targetFP );
            aCompatData.m_groupableItems.insert( targetFP );

            // The matched footprint maps to its target; treat it as reproduced so a group
            // containing it can be rebuilt.
            if( preserveGroups && refFP->GetParentGroup() )
                groupFixupPairs.emplace_back( refFP, targetFP );

            if( preserveGroups )
                reproducedSourceItems.insert( refFP );
        }
    }

    // Rebuild a source group only when all of its members were reproduced. A group that merely
    // overlaps the source area keeps uncopied members, so it is left untouched rather than
    // partially duplicated (issue 22316).
    if( preserveGroups && !groupFixupPairs.empty() )
    {
        std::map<EDA_GROUP*, EDA_GROUP*> groupMap;
        std::map<EDA_GROUP*, bool>       fullyReproducedCache;

        auto groupFullyReproduced =
                [&]( EDA_GROUP* aGroup )
                {
                    if( auto it = fullyReproducedCache.find( aGroup ); it != fullyReproducedCache.end() )
                        return it->second;

                    bool reproduced = true;

                    for( EDA_ITEM* member : aGroup->GetItems() )
                    {
                        // Nested groups are not reproduced, so a parent containing one can never
                        // be fully reproduced.
                        if( !member->IsBOARD_ITEM()
                            || !reproducedSourceItems.contains( static_cast<BOARD_ITEM*>( member ) ) )
                        {
                            reproduced = false;
                            break;
                        }
                    }

                    fullyReproducedCache[aGroup] = reproduced;
                    return reproduced;
                };

        for( const auto& [sourceItem, destItem] : groupFixupPairs )
        {
            EDA_GROUP* parentGroup = sourceItem->GetParentGroup();

            if( !parentGroup || !groupFullyReproduced( parentGroup ) )
                continue;

            if( !groupMap.contains( parentGroup ) )
            {
                PCB_GROUP* newGroup = static_cast<PCB_GROUP*>(
                        static_cast<PCB_GROUP*>( parentGroup->AsEdaItem() )->Duplicate( false ) );
                newGroup->GetItems().clear();
                newGroup->SetParentGroup( nullptr );

                if( newGroup->Type() == PCB_GENERATOR_T )
                {
                    newGroup->Rotate( VECTOR2( 0, 0 ), rot );
                    newGroup->Move( disp );
                }

                groupMap[parentGroup] = newGroup;
                aCommit->Add( newGroup );
            }

            // AddItem reparents the footprint out of any group it already belongs to; stage that
            // group so the membership change is captured for undo.
            if( EDA_GROUP* oldGroup = destItem->GetParentGroup() )
            {
                if( oldGroup != groupMap[parentGroup] )
                    aCommit->Modify( oldGroup->AsEdaItem() );
            }

            groupMap[parentGroup]->AddItem( destItem );
        }
    }

    aTargetArea->m_zone->RemoveAllContours();
    aTargetArea->m_zone->AddPolygon( newTargetOutline );
    aTargetArea->m_zone->UnHatchBorder();
    aTargetArea->m_zone->HatchBorder();
}

/**
 * @brief Attempts to make sure copied items are assigned the right net
 *
 */
void MULTICHANNEL_TOOL::fixupNet( BOARD_CONNECTED_ITEM* aRef, BOARD_CONNECTED_ITEM* aTarget,
                                  bool aHasMatches,
                                  const std::map<int, int>& aNetMap )
{
    // Copy as no-net.
    if( !aHasMatches )
    {
        aTarget->SetNetCode( 0 );
        return;
    }

    auto it = aNetMap.find( aRef->GetNetCode() );

    if( it != aNetMap.end() )
        aTarget->SetNetCode( it->second );
}


std::vector<NETINFO_ITEM*> MULTICHANNEL_TOOL::IsolateDesignBlockAutoNets( BOARD*                      aBoard,
                                                                          const std::set<FOOTPRINT*>& aFootprints,
                                                                          const std::unordered_set<EDA_ITEM*>& aItems )
{
    std::vector<NETINFO_ITEM*>   created;
    std::map<int, NETINFO_ITEM*> remap;
    int                          counter = 0;

    // Auto-generated names are tied to a reference designator, so a block's Net-(D3-A) collides
    // with a different part's Net-(D3-A) on the board. Named/power nets are intentional and left
    // alone so the topology matcher keeps excluding real global rails.
    auto isAutoName = []( const wxString& aName )
    {
        return aName.StartsWith( wxT( "Net-(" ) ) || aName.StartsWith( wxT( "unconnected-" ) );
    };

    auto remapItem = [&]( BOARD_CONNECTED_ITEM* aItem )
    {
        int code = aItem->GetNetCode();

        if( code <= 0 )
            return;

        NETINFO_ITEM* oldNet = aBoard->FindNet( code );

        if( !oldNet || !isAutoName( oldNet->GetNetname() ) )
            return;

        auto it = remap.find( code );

        if( it == remap.end() )
        {
            wxString name;

            do
            {
                name = wxString::Format( wxT( "__dbapply_%d_%d" ), code, counter++ );
            } while( aBoard->FindNet( name ) );

            NETINFO_ITEM* newNet = new NETINFO_ITEM( aBoard, name );
            aBoard->Add( newNet );
            created.push_back( newNet );
            it = remap.emplace( code, newNet ).first;
        }

        aItem->SetNet( it->second );
    };

    for( FOOTPRINT* fp : aFootprints )
    {
        for( PAD* pad : fp->Pads() )
            remapItem( pad );
    }

    for( EDA_ITEM* item : aItems )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            remapItem( bci );
    }

    return created;
}


// Pairs footprints by their unique closest symbol instance path, independent of net topology
// Leaves aResult untouched unless that gives a full pad compatible bijection
static bool matchBySymbolInstancePath( const std::set<FOOTPRINT*>& aRef, const std::set<FOOTPRINT*>& aTarget,
                                       TMATCH::COMPONENT_MATCHES& aResult )
{
    if( aRef.empty() || aRef.size() != aTarget.size() )
        return false;

    TMATCH::COMPONENT_MATCHES result;
    std::set<FOOTPRINT*>      used;

    for( FOOTPRINT* refFp : aRef )
    {
        FOOTPRINT* targetFp = nullptr;
        size_t     bestLength = 0;
        bool       tied = false;

        for( FOOTPRINT* candidate : aTarget )
        {
            size_t length = TMATCH::CommonPathTailLength( refFp->GetPath(), candidate->GetPath() );

            if( length > bestLength )
            {
                targetFp = candidate;
                bestLength = length;
                tied = false;
            }
            else if( length == bestLength && length > 0 )
            {
                tied = true;
            }
        }

        // A missing or ambiguous path (copy paste, hand built group) is not a clean instance link
        if( !targetFp || tied )
            return false;

        // Routing and placement copy only makes sense between pad compatible footprints
        if( refFp->GetFPID() != targetFp->GetFPID() || refFp->Pads().size() != targetFp->Pads().size() )
            return false;

        if( !used.insert( targetFp ).second )
            return false;

        result[refFp] = targetFp;
    }

    aResult = std::move( result );
    return true;
}


bool MULTICHANNEL_TOOL::resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                                   RULE_AREA_COMPAT_DATA& aMatches,
                                                   const TMATCH::ISOMORPHISM_PARAMS& aParams,
                                                   const std::unordered_set<int>& aExtraGlobalNets )
{
    using namespace TMATCH;

    // Footprint-free design block has nothing to match to
    if( aRefArea->m_sourceType == PLACEMENT_SOURCE_T::DESIGN_BLOCK && aRefArea->m_components.empty()
        && aTargetArea->m_components.empty() )
    {
        aMatches.m_matchingComponents.clear();
        aMatches.m_isOk = true;
        aMatches.m_errorMsg = _( "OK" );
        aMatches.m_mismatchReasons.clear();
        return true;
    }

    // Placement areas resolve their components from their source, so two areas sharing a sheet,
    // component class or group resolve to identical footprints. Repeating into such a target would
    // move and delete the reference's own items, corrupting the placement rather than copying it.
    if( !aRefArea->m_components.empty() )
    {
        std::set<FOOTPRINT*> shared;
        std::set_intersection( aRefArea->m_components.begin(), aRefArea->m_components.end(),
                               aTargetArea->m_components.begin(), aTargetArea->m_components.end(),
                               std::inserter( shared, shared.begin() ) );

        if( !shared.empty() )
        {
            aMatches.m_matchingComponents.clear();
            aMatches.m_isOk = false;
            aMatches.m_errorMsg = _( "Target Rule Area shares components with the reference area" );
            aMatches.m_mismatchReasons.clear();
            aMatches.m_mismatchReasons.push_back(
                    _( "This target Rule Area selects the same components as the reference area. "
                       "Repeat layout cannot copy a Rule Area onto itself. Give each placement Rule "
                       "Area a distinct sheet, component class or group." ) );
            aMatches.m_mismatchReasons.push_back( wxString::Format( _( "Shared components:\n%s" ),
                                                                    FormatComponentList( shared ) ) );
            return false;
        }
    }

    // A global rail connects >=2 pads in more than one channel; find them across all areas so the
    // exclusion is the same for every target, not just the one being matched against.
    std::unordered_map<int, std::vector<const RULE_AREA*>> netInternalAreas;

    for( const RULE_AREA& area : m_areas.m_areas )
    {
        std::unordered_map<int, int> areaNetPadCounts;

        for( const FOOTPRINT* fp : area.m_components )
        {
            for( const PAD* pad : fp->Pads() )
            {
                if( pad->GetNetCode() > 0 )
                    areaNetPadCounts[pad->GetNetCode()]++;
            }
        }

        for( const auto& [netCode, padCount] : areaNetPadCounts )
        {
            if( padCount >= 2 )
                netInternalAreas[netCode].push_back( &area );
        }
    }

    // Require two component-disjoint areas so overlapping rule areas can't make a per-channel net
    // look global.
    auto disjoint =
            []( const RULE_AREA* aA, const RULE_AREA* aB )
            {
                for( FOOTPRINT* fp : aA->m_components )
                {
                    if( aB->m_components.count( fp ) )
                        return false;
                }

                return true;
            };

    std::unordered_set<int> globalNets = aExtraGlobalNets;

    for( const auto& [netCode, areas] : netInternalAreas )
    {
        for( size_t i = 0; i < areas.size() && !globalNets.count( netCode ); i++ )
        {
            for( size_t j = i + 1; j < areas.size(); j++ )
            {
                if( disjoint( areas[i], areas[j] ) )
                {
                    globalNets.insert( netCode );
                    break;
                }
            }
        }
    }

    PROF_TIMER timerBuild;
    std::unique_ptr<CONNECTION_GRAPH> cgRef( CONNECTION_GRAPH::BuildFromFootprintSet( aRefArea->m_components,
                                                                                       aTargetArea->m_components,
                                                                                       globalNets ) );
    std::unique_ptr<CONNECTION_GRAPH> cgTarget( CONNECTION_GRAPH::BuildFromFootprintSet( aTargetArea->m_components,
                                                                                         aRefArea->m_components,
                                                                                         globalNets ) );
    timerBuild.Stop();

    wxLogTrace( traceMultichannelTool, wxT( "Graph construction: %s (%d + %d components)" ),
                timerBuild.to_string(),
                (int) aRefArea->m_components.size(),
                (int) aTargetArea->m_components.size() );

    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> mismatchReasons;

    PROF_TIMER timerIso;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), aMatches.m_matchingComponents,
                                          mismatchReasons, aParams );
    timerIso.Stop();

    wxLogTrace( traceMultichannelTool, wxT( "FindIsomorphism: %s, result=%d" ),
                timerIso.to_string(), status ? 1 : 0 );

    // Net topology can legitimately differ between a design block and its placed instance once the
    // user edits connectivity on the board (a wire tying two block pads onto one net, etc.). Fall
    // back to the symbol instance linkage, which still gives the correct mapping in that case. Skip
    // it on a cancelled scan so cancellation is not mistaken for a topology miss.
    const bool cancelled = aParams.m_cancelled && aParams.m_cancelled->load( std::memory_order_relaxed );

    if( !status && !cancelled
        && matchBySymbolInstancePath( aRefArea->m_components, aTargetArea->m_components,
                                      aMatches.m_matchingComponents ) )
    {
        status = true;
    }

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
            aMatches.m_mismatchReasons.push_back( reason.m_reason );
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
        aMatches.m_mismatchReasons.push_back( _( "The components in the two areas could not be paired up." ) );

    // The reason above already gives both totals. Only the lists add anything, and only when
    // both sides have parts to compare.
    if( aRefArea->m_components.size() != aTargetArea->m_components.size() && !aRefArea->m_components.empty()
        && !aTargetArea->m_components.empty() )
    {
        aMatches.m_mismatchReasons.push_back( wxString::Format( _( "Reference area components:\n%s" ),
                                                                FormatComponentList( aRefArea->m_components ) ) );
        aMatches.m_mismatchReasons.push_back( wxString::Format( _( "Target area components:\n%s" ),
                                                                FormatComponentList( aTargetArea->m_components ) ) );
    }

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

            if( group->GetItems().size() < 2 )
                aCommit.Remove( group );
        }
    }

    return false;
}


int MULTICHANNEL_TOOL::AutogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    closeRepeatDialog();

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

    wxLogTrace( traceMultichannelTool, wxT( "%d placement areas found\n" ), (int) m_areas.m_areas.size() );

    BOARD_COMMIT commit( GetManager(), true, false );

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        // Clear the result so skipped candidates are not grouped below.
        ra.m_zone = nullptr;

        if( !ra.m_generateEnabled )
            continue;

        if( ra.m_components.empty() )
            continue;

        wxString source;
        wxString existingAreaTraceFormat;

        if( ra.m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
        {
            source = ra.m_sheetPath;
            existingAreaTraceFormat = wxT( "Placement rule area for sheet '%s' already exists as '%s'\n" );
        }
        else if( ra.m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
        {
            source = ra.m_componentClass;
            existingAreaTraceFormat = wxT( "Placement rule area for component class '%s' already exists as '%s'\n" );
        }
        else
        {
            source = ra.m_groupName;
            existingAreaTraceFormat = wxT( "Placement rule area for group '%s' already exists as '%s'\n" );
        }

        std::unique_ptr<ZONE> newZone( new ZONE( board() ) );
        newZone->SetZoneName( wxString::Format( wxT( "auto-placement-area-%s" ), source ) );
        newZone->SetPlacementAreaSourceType( ra.m_sourceType );
        newZone->SetPlacementAreaSource( source );

        std::vector<ZONE*> oldZones;

        for( ZONE* zone : board()->Zones() )
        {
            if( !zone->GetIsRuleArea() || !zone->GetPlacementAreaEnabled() )
                continue;

            if( zone->GetPlacementAreaSourceType() != newZone->GetPlacementAreaSourceType()
                || zone->GetPlacementAreaSource() != newZone->GetPlacementAreaSource() )
            {
                continue;
            }

            wxLogTrace( traceMultichannelTool, existingAreaTraceFormat, source, zone->GetZoneName() );

            oldZones.push_back( zone );
        }

        if( !oldZones.empty() && !m_areas.m_replaceExisting )
            continue;

        SHAPE_LINE_CHAIN raOutline;

        // Groups are a way for the user to more explicitly provide a list of items to include in
        // the multichannel tool, as opposed to inferring them based on sheet structure or component classes.
        // So for group-based RAs, we build the RA outline based everything in the group, not just components.
        if( ra.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
        {
            std::set<BOARD_ITEM*> groupItems = queryBoardItemsInGroup( ra.m_groupName );

            if( groupItems.empty() )
            {
                wxLogTrace( traceMultichannelTool,
                            wxT( "Skipping placement rule area generation for source group '%s': group has no board items." ),
                            ra.m_groupName );
                continue;
            }

            raOutline = buildRAOutline( groupItems, 100000 );
        }
        else
        {
            // Start from the footprints, then also take everything in the design-block groups
            // they belong to (recursively) so routing and meanders that extend past the
            // footprints land inside the outline. Group membership keeps it bounded to this channel.
            std::set<BOARD_ITEM*> outlineItems;
            std::set<EDA_GROUP*>  groups;
            std::set<int>         channelNets;

            for( FOOTPRINT* fp : ra.m_components )
            {
                outlineItems.insert( fp );

                for( PAD* pad : fp->Pads() )
                    channelNets.insert( pad->GetNetCode() );

                for( EDA_GROUP* g = fp->GetParentGroup(); g; g = g->AsEdaItem()->GetParentGroup() )
                    groups.insert( g );
            }

            for( EDA_GROUP* g : groups )
                collectGroupBoardItems( g, outlineItems );

            // Also include tracks and vias on nets local to this channel (all pads on the net
            // belong to the channel), so loose connections between blocks land in the outline.
            std::set<int> foreignNets;

            for( FOOTPRINT* fp : board()->Footprints() )
            {
                if( ra.m_components.count( fp ) )
                    continue;

                for( PAD* pad : fp->Pads() )
                    foreignNets.insert( pad->GetNetCode() );
            }

            for( PCB_TRACK* track : board()->Tracks() )
            {
                int net = track->GetNetCode();

                if( net > 0 && channelNets.count( net ) && !foreignNets.count( net ) )
                    outlineItems.insert( track );
            }

            raOutline = buildRAOutline( outlineItems, 100000 );
        }

        wxLogTrace( traceMultichannelTool, wxT( "Generated rule area '%s' (%d components)\n" ),
                    newZone->GetZoneName(),
                    (int) ra.m_components.size() );

        makePlacementArea( newZone.get(), raOutline );

        for( ZONE* oldZone : oldZones )
            commit.Remove( oldZone );

        ra.m_zone = newZone.release();
        commit.Add( ra.m_zone );
    }

    // fixme: handle corner cases where the items belonging to a Rule Area already
    // belong to other groups.

    if( m_areas.m_options.m_groupItems )
    {
        for( RULE_AREA& ra : m_areas.m_areas )
        {
            if( !ra.m_zone )
                continue;

            std::unordered_set<BOARD_ITEM*> toPrune;

            std::copy( ra.m_components.begin(), ra.m_components.end(), std::inserter( toPrune, toPrune.begin() ) );

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

    commit.Push( _( "Auto-generate Placement Rule Areas" ) );

    return true;
}
