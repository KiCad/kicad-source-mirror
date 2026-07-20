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
#include <tools/pcb_selection.h>

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
#include <connectivity/connectivity_data.h>
#include <connectivity/topo_match.h>
#include <algorithm>
#include <pcbnew_scripting_helpers.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/pcb_picker_tool.h>
#include <chrono>
#include <core/profile.h>
#include <thread_pool.h>
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

        return aItems.size() > 0;
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
                for( EDA_ITEM* member : group->GetItems() )
                {
                    if( member->Type() == PCB_FOOTPRINT_T || member->Type() == PCB_GROUP_T
                        || member->Type() == PCB_GENERATOR_T || member->Type() == PCB_TABLECELL_T )
                        continue;

                    if( BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( member ) )
                    {
                        if( !boardItem->IsConnected() || boardItem->Type() == PCB_ZONE_T )
                            aItems.insert( boardItem );
                    }
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


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInGroup( const wxString& aGroupName ) const
{
    std::set<FOOTPRINT*> rv;

    for( PCB_GROUP* group : board()->Groups() )
    {
        if( group->GetName() == aGroupName )
            collectGroupFootprints( group, rv );
    }

    return rv;
}


std::set<BOARD_ITEM*> MULTICHANNEL_TOOL::queryBoardItemsInGroup( const wxString& aGroupName ) const
{
    std::set<BOARD_ITEM*> rv;

    for( PCB_GROUP* group : board()->Groups() )
    {
        if( group->GetName() != aGroupName )
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

    std::vector<RULE_AREA*> targets;

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( ra.m_zone == m_areas.m_refRA->m_zone )
            continue;

        targets.push_back( &ra );
        m_areas.m_compatMap[&ra] = RULE_AREA_COMPAT_DATA();
    }

    if( targets.empty() )
        return 0;

    int              total = static_cast<int>( targets.size() );
    std::atomic<int> completed( 0 );
    std::atomic<bool> cancelled( false );
    std::atomic<int>  matchedComponents( 0 );
    std::atomic<int>  totalComponents( 0 );
    RULE_AREA*        refRA = m_areas.m_refRA;

    TMATCH::ISOMORPHISM_PARAMS isoParams;
    isoParams.m_cancelled = &cancelled;
    isoParams.m_matchedComponents = &matchedComponents;
    isoParams.m_totalComponents = &totalComponents;

    // Process RA resolutions sequentially on a single background thread.
    // Each resolveConnectionTopology call internally parallelizes its MRV scan
    // across the thread pool, creating many short-lived tasks that fully utilize
    // all available cores.  Running the outer loop sequentially avoids thread
    // pool starvation from nested parallelism.
    thread_pool& tp = GetKiCadThreadPool();

    auto future = tp.submit_task(
            [this, refRA, &targets, &completed, &cancelled, &matchedComponents, &isoParams]()
            {
                for( RULE_AREA* target : targets )
                {
                    if( cancelled.load( std::memory_order_relaxed ) )
                        break;

                    matchedComponents.store( 0, std::memory_order_relaxed );

                    RULE_AREA_COMPAT_DATA& compatData = m_areas.m_compatMap[target];
                    resolveConnectionTopology( refRA, target, compatData, isoParams );
                    completed.fetch_add( 1, std::memory_order_relaxed );
                }
            } );

    if( Pgm().IsGUI() )
    {
        std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
        auto startTime = std::chrono::steady_clock::now();
        double highWaterMark = 0.0;

        while( future.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready )
        {
            if( !reporter )
            {
                auto elapsed = std::chrono::steady_clock::now() - startTime;

                if( elapsed > std::chrono::seconds( 1 ) )
                {
                    reporter = std::make_unique<WX_PROGRESS_REPORTER>(
                            frame(), _( "Checking Rule Area compatibility..." ), 1, PR_CAN_ABORT );
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
                        done + 1, total, matched, compTotal ) );

                if( !reporter->KeepRefreshing() )
                    cancelled.store( true, std::memory_order_relaxed );
            }
        }
    }
    else
    {
        future.wait();
    }

    if( cancelled.load( std::memory_order_relaxed ) )
    {
        m_areas.m_compatMap.clear();
        return -1;
    }

    return 0;
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
            *aErrorOut = compat.m_errorMsg;
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
    if( aTargetArea.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT && !aOptions.m_anchorFp )
    {
        if( !compat.m_matchingComponents.empty() )
            aOptions.m_anchorFp = compat.m_matchingComponents.begin()->first;
    }

    if( !copyRuleAreaContents( &aRefArea, &aTargetArea, &commit, aOptions, compat ) )
    {
        auto errMsg = wxString::Format( _( "Copy Rule Area contents failed between rule areas '%s' and '%s'." ),
                                        aRefArea.m_zone->GetZoneName(), aTargetArea.m_zone->GetZoneName() );

        if( !aExternalCommit )
            commit.Revert();

        reportError( errMsg );

        return -1;
    }

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
            commit.Modify( item );
            group->AddItem( item );
        }
    }

    if( !aExternalCommit )
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
            if( compatData.m_groupableItems.size() < 2 )
                continue;

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

        return (int) aOutput.size();
    }

    // The design-block apply target uses a scratch zone not on the board, so enclosedByArea()
    // below finds nothing. Match routing against the zone outline directly.
    if( aRuleArea->m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
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

                if( !( aRuleArea->m_zone->GetLayerSet() & aItem->GetLayerSet() ).any() )
                    return;

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

        // If the dialog-selected anchor reference doesn't exist in the target area (e.g. refs don't match),
        // fall back to the first matched pair to avoid center-alignment shifting the whole group.
        if( !targetAnchorFp && !aCompatData.m_matchingComponents.empty() )
            targetAnchorFp = aCompatData.m_matchingComponents.begin()->second;

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

        findRoutingInRuleArea( aTargetArea, targetRouting, connectivity, targetPoly, aOpts );
        findRoutingInRuleArea( aRefArea, refRouting, connectivity, refPoly, aOpts );

        // Nets used by the target group's own items, footprint pads included.
        std::set<int> targetGroupNets;

        if( aTargetArea->m_group )
        {
            for( EDA_ITEM* member : aTargetArea->m_group->GetItems() )
            {
                if( member->Type() == PCB_FOOTPRINT_T )
                {
                    for( PAD* pad : static_cast<FOOTPRINT*>( member )->Pads() )
                        targetGroupNets.insert( pad->GetNetCode() );
                }
                else if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( member ) )
                {
                    targetGroupNets.insert( bci->GetNetCode() );
                }
            }
        }

        for( BOARD_CONNECTED_ITEM* item : targetRouting )
        {
            // Never remove pads as part of routing copy.
            if( item->Type() == PCB_PAD_T )
                continue;

            if( aRefArea->m_designBlockItems.count( item ) )
                continue;

            // Design block apply: replace only the group's own routing and loose routing on the
            // group's nets. Other groups' routing belongs to stacked instances (issue 24767).
            // Everything else is unrelated and just sits inside the block's area (issue 24944).
            if( aTargetArea->m_group && item->GetParentGroup() != aTargetArea->m_group )
            {
                if( item->GetParentGroup() )
                    continue;

                if( item->IsLocked() )
                    continue;

                if( item->GetNetCode() <= 0 || !targetGroupNets.contains( item->GetNetCode() ) )
                    continue;
            }

            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            if( aOpts.m_connectedRoutingOnly && !targc.contains( item->GetNetCode() ) )
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

        // Copy generators (meanders) whole so they are not flattened to loose tracks. Design
        // block apply has an exact item list, other rule areas resolve them by area.
        std::vector<PCB_GENERATOR*> refGenerators;
        std::vector<PCB_GENERATOR*> targetGenerators;

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
            {
                for( EDA_ITEM* member : targetGroup->GetItems() )
                {
                    if( member->Type() == PCB_GENERATOR_T )
                        targetGenerators.push_back( static_cast<PCB_GENERATOR*>( member ) );
                }
            }
        }
        else
        {
            const SHAPE_LINE_CHAIN& refOut = aRefArea->m_zone->Outline()->COutline( 0 );
            const SHAPE_LINE_CHAIN& targetOut = aTargetArea->m_zone->Outline()->COutline( 0 );

            for( PCB_GENERATOR* gen : board()->Generators() )
            {
                if( gen->GetGeneratorType() != wxT( "tuning_pattern" ) )
                    continue;

                if( gen->HitTest( refOut, false ) )
                    refGenerators.push_back( gen );
                else if( gen->HitTest( targetOut, false ) )
                    targetGenerators.push_back( gen );
            }
        }

        // Remove the target's existing generators so the copy replaces them.
        for( PCB_GENERATOR* gen : targetGenerators )
        {
            gen->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        aCommit->Remove( child );
                    },
                    RECURSE_MODE::RECURSE );
            aCommit->Remove( gen );
        }

        for( PCB_GENERATOR* gen : refGenerators )
        {
            PCB_GENERATOR* clone = gen->DeepClone();

            clone->ClearFlags();
            clone->Rotate( VECTOR2( 0, 0 ), rot );
            clone->Move( disp );
            aCommit->Add( clone );

            clone->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->ClearFlags();

                        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( child ) )
                            fixupNet( bci, bci, aCompatData.m_matchingComponents );

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
                fixupNet( zone, targetZone, aCompatData.m_matchingComponents );

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
                targetText->Rotate( VECTOR2( 0, 0 ), rot );
                targetText->Move( disp );
                targetText->SetIsKnockout( refText->IsKnockout() );
            }

            // Copy 3D model settings
            targetFP->Models() = refFP->Models();

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

    return true;
}

/**
 * @brief Attempts to make sure copied items are assigned the right net
 *
 */
void MULTICHANNEL_TOOL::fixupNet( BOARD_CONNECTED_ITEM* aRef, BOARD_CONNECTED_ITEM* aTarget,
                                  TMATCH::COMPONENT_MATCHES& aComponentMatches )
{
    // Copy as no-net.
    if( aComponentMatches.empty() )
    {
        aTarget->SetNetCode( 0 );
        return;
    }

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


// A placed design block or repeated sheet stamps the originating symbol instance UUID into each
// footprint's path. When that linkage is complete and unique it is an authoritative one to one
// mapping, independent of net topology. Returns false (and leaves aResult untouched) unless it
// yields a full pad compatible bijection, so callers can fall back to topology matching.
static bool matchBySymbolInstancePath( const std::set<FOOTPRINT*>& aRef, const std::set<FOOTPRINT*>& aTarget,
                                       TMATCH::COMPONENT_MATCHES& aResult )
{
    if( aRef.empty() || aRef.size() != aTarget.size() )
        return false;

    auto symbolUuid = []( const FOOTPRINT* aFp ) -> KIID
    {
        const KIID_PATH& path = aFp->GetPath();
        return path.empty() ? niluuid : path.back();
    };

    std::map<KIID, FOOTPRINT*> targetByUuid;

    for( FOOTPRINT* fp : aTarget )
    {
        KIID uuid = symbolUuid( fp );

        // A missing or duplicated UUID (copy paste, hand built group) is not a clean instance link
        if( uuid == niluuid || !targetByUuid.emplace( uuid, fp ).second )
            return false;
    }

    TMATCH::COMPONENT_MATCHES result;
    std::set<FOOTPRINT*>      used;

    for( FOOTPRINT* refFp : aRef )
    {
        KIID uuid = symbolUuid( refFp );

        if( uuid == niluuid )
            return false;

        auto it = targetByUuid.find( uuid );

        if( it == targetByUuid.end() )
            return false;

        FOOTPRINT* targetFp = it->second;

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
                                                   const TMATCH::ISOMORPHISM_PARAMS& aParams )
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

    std::unordered_set<int> globalNets;

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

    // Component count mismatch
    if( aRefArea->m_components.size() != aTargetArea->m_components.size() )
    {
        aMatches.m_mismatchReasons.push_back(
                wxString::Format( _( "Reference area total components: %d" ), (int) aRefArea->m_components.size() ) );
        aMatches.m_mismatchReasons.push_back( wxString::Format( _( "Reference area components:\n%s" ),
                                                                FormatComponentList( aRefArea->m_components ) ) );
        aMatches.m_mismatchReasons.push_back(
                wxString::Format( _( "Target area total components: %d" ), (int) aTargetArea->m_components.size() ) );
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

            // A group needs at least 2 items (zone + at least 1 component)
            if( ra.m_components.empty() )
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
