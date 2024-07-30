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
#include <pcb_group.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/topo_match.h>
#include <optional>
#include <algorithm>
#include <random>
#include <core/profile.h>
#include <wx/log.h>
#include <pgm_base.h>


#define MULTICHANNEL_EXTRA_DEBUG

static const wxString traceMultichannelTool = wxT( "MULTICHANNEL_TOOL" );


MULTICHANNEL_TOOL::MULTICHANNEL_TOOL() : PCB_TOOL_BASE( "pcbnew.Multichannel" )
{
    m_reporter.reset( new NULL_REPORTER );
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
        m_reporter->Report( _( "ERROR:" ) + wxS( " " ) + aMessage );
    };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );
    //compiler.SetDebugReporter( m_reporter );

    m_reporter->Report(
            wxString::Format( wxT( "Process rule area '%s'" ), aRuleArea->GetZoneName() ) );

    auto ok = compiler.Compile( aRuleArea->GetRuleAreaExpression(), &ucode, &preflightCtx );


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
            m_reporter->Report( wxString::Format( wxT( "- %s [sheet %s]" ), fp->GetReference(),
                                                  fp->GetSheetname() ) );
            aComponents.insert( fp );
        }
    }

    return true;
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInSheet( wxString aSheetName )
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


const SHAPE_LINE_CHAIN MULTICHANNEL_TOOL::buildRAOutline( std::set<FOOTPRINT*>& aFootprints,
                                                          int                   aMargin )
{
    std::vector<VECTOR2I> bbCorners, hullVertices;

    for( auto fp : aFootprints )
    {
        auto bb = fp->GetBoundingBox( false, false );
        bb.Inflate( aMargin );

        bbCorners.push_back( VECTOR2I( bb.GetX(), bb.GetY() ) );
        bbCorners.push_back( VECTOR2I( bb.GetX() + bb.GetWidth(), bb.GetY() ) );
        bbCorners.push_back( VECTOR2I( bb.GetX() + bb.GetWidth(), bb.GetY() + bb.GetHeight() ) );
        bbCorners.push_back( VECTOR2I( bb.GetX(), bb.GetY() + bb.GetHeight() ) );
    }

    BuildConvexHull( hullVertices, bbCorners );

    SHAPE_LINE_CHAIN hull( hullVertices );
    SHAPE_LINE_CHAIN raOutline;

    // make the newly computed convex hull use only 90 degree segments
    hull.SetClosed( true );
    for( int i = 0; i < hull.SegmentCount(); i++ )
    {
        const auto&    seg = hull.CSegment( i );
        const VECTOR2I p0( seg.A.x, seg.B.y );
        const VECTOR2I p1( seg.B.x, seg.A.y );

        raOutline.Append( seg.A );
        if( hull.PointInside( p0 ) )
            raOutline.Append( p0 );
        else
            raOutline.Append( p1 );
    }

    raOutline.SetClosed( true );
    raOutline.Simplify();

    return raOutline;
}


void MULTICHANNEL_TOOL::QuerySheets()
{
    using PathAndName = std::pair<wxString, wxString>;
    std::set<PathAndName> uniqueSheets;

    m_areas.m_areas.clear();

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        uniqueSheets.insert( PathAndName( fp->GetSheetname(), fp->GetSheetfile() ) );
    }

    for( const PathAndName& sheet : uniqueSheets )
    {
        RULE_AREA ent;

        ent.m_generateEnabled = false;
        ent.m_sheetPath = sheet.first;
        ent.m_sheetName = sheet.second;
        ent.m_sheetComponents = queryComponentsInSheet( ent.m_sheetPath );
        m_areas.m_areas.push_back( ent );

        wxLogTrace( traceMultichannelTool, wxT("found sheet '%s' @ '%s' s %d\n"),
            ent.m_sheetName, ent.m_sheetPath, (int)m_areas.m_areas.size() );
    }
}


void MULTICHANNEL_TOOL::FindExistingRuleAreas()
{
    m_areas.m_areas.clear();

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;
        if( zone->GetRuleAreaType() != RULE_AREA_TYPE::PLACEMENT )
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


int MULTICHANNEL_TOOL::repeatLayout( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE*> refRAs;

    for( EDA_ITEM* item : selection() )
    {
        if( item->Type() == PCB_ZONE_T )
        {
            ZONE* zone = static_cast<ZONE*>( item );
            if( !zone->GetIsRuleArea() )
                continue;
            if( zone->GetRuleAreaType() != RULE_AREA_TYPE::PLACEMENT )
                continue;

            refRAs.push_back( zone );
        }
    }

    if( refRAs.size() != 1 )
    {
        frame()->ShowInfoBarError( _( "Please select a single reference Rule Area to copy from" ),
                                       true );
        return 0;
    }

    FindExistingRuleAreas();

    int status = CheckRACompatibility( refRAs.front() );

    if( status < 0 )
        return status;

    DIALOG_MULTICHANNEL_REPEAT_LAYOUT dialog( frame(), this );
    int                               ret = dialog.ShowModal();

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
    //KI_TEST::CONSOLE_LOG          consoleLog;
    //m_reporter.reset( new KI_TEST::CONSOLE_MSG_REPORTER ( &consoleLog ) );

    
    BOARD_COMMIT commit( GetManager(), true );

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

        if( !copyRuleAreaContents( targetArea.second.m_matchingComponents, &commit, m_areas.m_refRA,
                                   targetArea.first, m_areas.m_options ) )
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

        totalCopied++;
    }

    commit.Push( _( "Repeat layout" ) );


    if( Pgm().IsGUI() )
    {
        frame()->ShowInfoBarMsg( wxString::Format( _( "Copied to %d Rule Areas." ), totalCopied ),
                             true );
    }
    return 0;
}


wxString MULTICHANNEL_TOOL::stripComponentIndex( wxString aRef ) const
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
                pad, { PCB_PAD_T, PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T }, true );

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


bool MULTICHANNEL_TOOL::copyRuleAreaContents( TMATCH::COMPONENT_MATCHES& aMatches, BOARD_COMMIT* aCommit,
                                              RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                              REPEAT_LAYOUT_OPTIONS aOpts )
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

        printf("ref-ls: %s\n", aRefArea->m_area->GetLayerSet().FmtHex().c_str() );
        printf("target-ls: %s\n", aTargetArea->m_area->GetLayerSet().FmtHex().c_str() );


        for( BOARD_ITEM* item : targetRouting )
        {
            if( item->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            // item already removed
            if( aCommit->GetStatus( item ) != 0 )
                continue;

            if( aTargetArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                aCommit->Remove( item );
        }

        for( BOARD_ITEM* item : refRouting )
        {
            if( !aRefArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;
            if( !aTargetArea->m_area->GetLayerSet().Contains( item->GetLayer() ) )
                continue;

            BOARD_ITEM* copied = static_cast<BOARD_ITEM*>( item->Clone() );

            copied->Move( disp );
            aCommit->Add( copied );
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

            printf("ref-fp-l: %s\n", refFP->GetLayerSet().FmtHex().c_str() );

#if 1
        //fixme: respect layers assigned to RAs
        printf("ref-ls: %s\n", aRefArea->m_area->GetLayerSet().FmtHex().c_str() );
        printf("target-ls: %s\n", aRefArea->m_area->GetLayerSet().FmtHex().c_str() );

        if( ! aRefArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) )
        {
            wxLogTrace( traceMultichannelTool, wxT("discard ref:%s (ref layer)\n"), refFP->GetReference() );
            continue;
        }
        if( ! aTargetArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) )
        {
            wxLogTrace( traceMultichannelTool, wxT("discard ref:%s (target layer)\n"), refFP->GetReference() );
            continue;
        }
#endif

            if( targetFP->IsLocked() && !aOpts.m_includeLockedItems )
                continue;

            aCommit->Modify( targetFP );

            targetFP->SetLayerAndFlip( refFP->GetLayer() );
            targetFP->SetOrientation( refFP->GetOrientation() );
            VECTOR2I targetPos = refFP->GetPosition() + disp;
            targetFP->SetPosition( targetPos );
            targetFP->Reference().SetTextAngle( refFP->Reference().GetTextAngle() );
            targetFP->Reference().SetPosition( refFP->Reference().GetPosition() + disp );
            targetFP->Value().SetTextAngle( refFP->Value().GetTextAngle() );
            targetFP->Value().SetPosition( refFP->Value().GetPosition() + disp );
        }
    }

    return true;
}


bool MULTICHANNEL_TOOL::resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea, RULE_AREA_COMPAT_DATA& aMatches )
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

    return (status == TMATCH::CONNECTION_GRAPH::ST_OK ); 
}


int MULTICHANNEL_TOOL::AutogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    //KI_TEST::CONSOLE_LOG          consoleLog;
    //m_reporter.reset( new KI_TEST::CONSOLE_MSG_REPORTER ( &consoleLog ) );


    if( Pgm().IsGUI() )
    {
        QuerySheets();

        if( m_areas.m_areas.size() <= 1 )
        {
            frame()->ShowInfoBarError( _( "Cannot auto-generate any placement areas because the "
                                        "schematic has only one or no hierarchical sheet(s)." ),
                                    true );
            return 0;
        }


        DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS dialog( frame(), this );
        int                                     ret = dialog.ShowModal();

        if( ret != wxID_OK )
            return 0;
    }

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;
        if( zone->GetRuleAreaType() != RULE_AREA_TYPE::PLACEMENT )
            continue;

        std::set<FOOTPRINT*> components;
        identifyComponentsInRuleArea( zone, components );

        if( components.empty() )
            continue;

        for( auto& ra : m_areas.m_areas )
        {
            if( components == ra.m_sheetComponents )
            {
                m_reporter->Report( wxString::Format(
                        wxT( "Placement rule area for sheet '%s' already exists as '%s'\n" ),
                        ra.m_sheetPath, zone->GetZoneName() ) );

                ra.m_area = zone;
                ra.m_existsAlready = true;
            }
        }
    }

    m_reporter->Report(
            wxString::Format( wxT( "%d placement areas found\n" ), (int) m_areas.m_areas.size() ) );

    BOARD_COMMIT commit( GetManager(), true );

    for( RULE_AREA& ra : m_areas.m_areas )
    {
        if( !ra.m_generateEnabled )
            continue;
        if( ra.m_existsAlready && !m_areas.m_replaceExisting )
            continue;

        auto raOutline = buildRAOutline( ra.m_sheetComponents, 100000 );

        std::unique_ptr<ZONE> newZone( new ZONE( board() ) );

        newZone->SetZoneName( wxString::Format( wxT( "auto-placement-area-%s" ), ra.m_sheetPath ) );
        m_reporter->Report( wxString::Format( wxT( "Generated rule area '%s' (%d components)\n" ),
                                              newZone->GetZoneName(),
                                              (int) ra.m_sheetComponents.size() ) );

        newZone->SetIsRuleArea( true );
        newZone->SetLayerSet( LSET::AllCuMask() );
        newZone->SetRuleAreaType( RULE_AREA_TYPE::PLACEMENT );
        newZone->SetRuleAreaExpression(
                wxString::Format( wxT( "A.memberOfSheet('%s')" ), ra.m_sheetPath ) );
        newZone->AddPolygon( raOutline );
        newZone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
        //aBoard->Add( newZone.release() );
        commit.Add( newZone.get() );
        commit.Push( _( "Auto-generate placement rule areas" ) );

        if( m_areas.m_groupItems )
        {
            // fixme: sth gets weird when creating new zones & grouping them within a single COMMIT
            BOARD_COMMIT grpCommit( GetManager(), true );

            PCB_GROUP* grp = new PCB_GROUP( board() );

            grpCommit.Add( grp );

            grpCommit.Stage( newZone.get(), CHT_GROUP );
            grp->AddItem( newZone.get() );

            for( auto fp : ra.m_sheetComponents )
            {
                grpCommit.Stage( fp, CHT_GROUP );
                grp->AddItem( fp );
            }
            grpCommit.Push( _( "Group components with their placement rule areas" ) );
        }

        newZone.release();
    }

    return true;
}
