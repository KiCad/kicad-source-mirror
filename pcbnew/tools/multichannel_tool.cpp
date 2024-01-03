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

#include <legacy/pcbexpr_evaluator.h>

#include <zone.h>
#include <geometry/convex_hull.h>
#include <pcb_group.h>
#include <connectivity/connectivity_data.h>
//#include <../qa/tools/pns/pns_log_viewer_frame.h>


MULTICHANNEL_TOOL::MULTICHANNEL_TOOL() : PCB_TOOL_BASE( "pcbnew.Multichannel" )
{
    m_reporter.reset( new NULL_REPORTER );
}


MULTICHANNEL_TOOL::~MULTICHANNEL_TOOL()
{

}


void MULTICHANNEL_TOOL::setTransitions()
{
    Go( &MULTICHANNEL_TOOL::autogenerateRuleAreas, PCB_ACTIONS::generatePlacementRuleAreas.MakeEvent() );
    Go( &MULTICHANNEL_TOOL::repeatLayout, PCB_ACTIONS::repeatLayout.MakeEvent() );
}


#if 0
int MULTICHANNEL_TOOL::autogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    using DData = DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_DATA;
    DData ddata;

    using PathAndName = std::pair<wxString, wxString>;
    std::set<PathAndName> uniqueSheets;

    for ( auto fp : board()->Footprints() )
    {
        uniqueSheets.insert( PathAndName( fp->GetSheetname(), fp->GetSheetfile() ) );
    }

    for( auto& sheet : uniqueSheets )
    {
        DData::ENTRY ent;

        ent.m_generateEnabled = true;
        ent.m_sheetPath = sheet.first;
        ent.m_sheetName = sheet.second;
        ddata.m_sheets.push_back( ent );
    }

    ddata.m_replaceExisting = true;

    DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS dialog( frame(), ddata );
    int ret = dialog.ShowModal();
    return 0;
}

#endif
    
bool MULTICHANNEL_TOOL::identifyComponentsInRuleArea( ZONE* aRuleArea, std::set<FOOTPRINT*>& aComponents )
{
    LEGACY::PCBEXPR_COMPILER compiler( new LEGACY::PCBEXPR_UNIT_RESOLVER );
    LEGACY::PCBEXPR_UCODE ucode;
    LEGACY::PCBEXPR_CONTEXT ctx, preflightCtx;

    auto reportError =  [&]( const wxString& aMessage, int aOffset )
                {
                    m_reporter->Report( _( "ERROR:" ) + wxS( " " ) + aMessage );
                };

    ctx.SetErrorCallback( reportError );
    preflightCtx.SetErrorCallback( reportError );
    compiler.SetErrorCallback( reportError );
    //compiler.SetDebugReporter( m_reporter );

    m_reporter->Report( wxString::Format( wxT("Process rule area '%s'"), aRuleArea->GetZoneName() ) );

    auto ok = compiler.Compile( aRuleArea->GetRuleAreaExpression(), &ucode, &preflightCtx );
    
    
    if(!ok)
        return false;
    for( auto& fp : board()->Footprints() )
    {
        ctx.SetItems( fp, fp );
        //printf("Test fp %x\n", fp);
        auto val = ucode.Run( &ctx );
        if( val->AsDouble() != 0.0 )
        {
            m_reporter->Report( wxString::Format( wxT("- %s [sheet %s]"), fp->GetReference(),
            fp->GetSheetname() ) );
            aComponents.insert( fp );
        }
    }

    return true;
}


std::set<FOOTPRINT*> MULTICHANNEL_TOOL::queryComponentsInSheet( wxString aSheetName )
{
    std::set<FOOTPRINT*> rv;
    if( aSheetName.EndsWith(wxT("/")))
        aSheetName.RemoveLast();
        
    for( auto& fp : board()->Footprints() )
    {
        auto sn = fp->GetSheetname();
        if( sn.EndsWith(wxT("/")))
            sn.RemoveLast();

        if ( sn == aSheetName )
        {
            rv.insert(fp);
        }
    }

//    printf("sheet %s : %d components\n", aSheetName.c_str().AsChar(), rv.size() );

    return rv;
}


const SHAPE_LINE_CHAIN MULTICHANNEL_TOOL::buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin )
{
    std::vector<VECTOR2I> bbCorners, hullVertices;

    for (auto fp : aFootprints)
    {
        auto bb = fp->GetBoundingBox( false, false );
        bb.Inflate( aMargin );

        bbCorners.push_back( { bb.GetX(), bb.GetY() } );
        bbCorners.push_back( { bb.GetX() + bb.GetWidth(), bb.GetY() } );
        bbCorners.push_back( { bb.GetX() + bb.GetWidth(), bb.GetY() + bb.GetHeight() } );
        bbCorners.push_back( { bb.GetX(), bb.GetY() + bb.GetHeight() } );

        //printf("- bb %s %d %d %d %d\n", fp->GetReference().c_str().AsChar(), bb.GetX(), bb.GetY(), bb.GetWidth(), bb.GetHeight() );
    }

    BuildConvexHull( hullVertices, bbCorners );

    SHAPE_LINE_CHAIN hull (hullVertices);
    SHAPE_LINE_CHAIN raOutline;

    hull.SetClosed(true);
    for( int i = 0; i < hull.SegmentCount(); i++ )
    {
        const auto& seg = hull.CSegment(i);
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

void MULTICHANNEL_TOOL::querySheets()
{
    using PathAndName = std::pair<wxString, wxString>;
    std::set<PathAndName> uniqueSheets;

    m_areas.m_areas.clear();

    for ( auto fp : board()->Footprints() )
    {
        uniqueSheets.insert( PathAndName( fp->GetSheetname(), fp->GetSheetfile() ) );
    }

    for( auto& sheet : uniqueSheets )
    {
        RULE_AREA ent;
        ent.m_generateEnabled = false;
        ent.m_sheetPath = sheet.first;
        ent.m_sheetName = sheet.second;
        ent.m_sheetComponents = queryComponentsInSheet(  ent.m_sheetPath );
        m_areas.m_areas.push_back( ent );
    }
}

void MULTICHANNEL_TOOL::findExistingRuleAreas()
{
    m_areas.m_areas.clear();

    for( auto zone : board()->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;
        if( zone->GetRuleAreaType() != RULE_AREA_TYPE::PLACEMENT )
            continue;

        std::set<FOOTPRINT*> components;
        identifyComponentsInRuleArea( zone, components );

        RULE_AREA area;

        area.m_existsAlready = true;
        area.m_area = zone;

        for( auto fp : components )
        {
            FP_WITH_CONNECTIONS fpc;
            fpc.fp = fp;
            area.m_raFootprints.push_back( fpc );
        }
        area.m_ruleName = zone->GetZoneName();
        area.m_center = zone->Outline()->COutline(0).Centre();

        m_areas.m_areas.push_back( area );
    }
}


RULE_AREA* MULTICHANNEL_TOOL::findRAByName( const wxString& aName )
{
    for( auto& ra : m_areas.m_areas )
    {
        if( ra.m_ruleName == aName )
            return &ra;
    }

    return nullptr;
}

int MULTICHANNEL_TOOL::repeatLayout( const TOOL_EVENT& aEvent )
{
    //KI_TEST::CONSOLE_LOG          consoleLog;
    //m_reporter.reset( new KI_TEST::CONSOLE_MSG_REPORTER ( &consoleLog ) );

    std::vector<ZONE*> refRAs;

    for( EDA_ITEM* item : selection() )
    {
        if( item->Type() == PCB_ZONE_T )
        {
            auto zone = static_cast<ZONE*>( item );
            if( !zone->GetIsRuleArea() )
                continue;
            if( zone->GetRuleAreaType() != RULE_AREA_TYPE::PLACEMENT )
                continue;

            refRAs.push_back( zone );
        }
    }

    printf("refRAs: %d\n", refRAs.size() );

    if( refRAs.size() != 1 )
    {
        frame()->ShowInfoBarError( 
            _("Please select a single reference Rule Area to copy from"),
            true );
        return 0;
    }

    findExistingRuleAreas();

    DIALOG_MULTICHANNEL_REPEAT_LAYOUT dialog( frame(), this );
    int ret = dialog.ShowModal();

    if( ret != wxID_OK )
        return 0;

    auto refArea = findRAByName( wxT( "r1" ) );
    auto targetArea = findRAByName( wxT( "r2" ) );
    //auto targetArea = findRAByName( wxT( "auto-placement-area-/io_drivers_fp/bank2/io56/" ) );


    MULTICHANNEL_TOOL::FP_PAIRS pairs;

    BOARD_COMMIT  commit( frame()->GetToolManager(), true ); //<PNS_LOG_VIEWER_FRAME>()->GetToolManager(), true );

    REPEAT_LAYOUT_OPTIONS opts;

    if( ! resolveConnectionTopology( refArea, targetArea, pairs ) )
    {
        m_reporter->Report( wxString::Format( wxT( "Can't fit connection topologies between %s and %s"), 
        refArea->m_area->GetZoneName(),
        targetArea->m_area->GetZoneName() ) );
    }

    if( copyRuleAreaContents( pairs, &commit, refArea, targetArea, opts ) )
    {
        printf("Committing\n");
        commit.Push( wxT("Repeat layout"));
    }

    return 0;
}

wxString MULTICHANNEL_TOOL::stripComponentIndex( wxString aRef ) const
{
    wxString rv;

    for (auto k : aRef )
    {
        if( !k.IsAscii() )
            break;
        char c;
        k.GetAsChar(&c);

        if( (c >= 'a' && c <= 'z') || ( c>= 'A' && c<= 'Z' ) || ( c == '_' ) )
            rv.Append(k);
        else
            break;
    }

    return rv;
}

bool MULTICHANNEL_TOOL::copyRuleAreaContents( FP_PAIRS& aMatches, BOARD_COMMIT* aCommit, RULE_AREA* aRefArea, RULE_AREA* aTargetArea, REPEAT_LAYOUT_OPTIONS aOpts )
{
    // copy shapes

    printf("ref : %p z: %p\n", aRefArea, aRefArea->m_area );
    printf("tgt : %p z: %p\n", aTargetArea, aTargetArea->m_area );

    SHAPE_LINE_CHAIN refOutline = aRefArea->m_area->Outline()->COutline(0);

    printf("RefOutlVtx: %d closed %d\n", refOutline.PointCount(), refOutline.IsClosed() ? 1: 0);
    VECTOR2I disp = aTargetArea->m_center - aRefArea->m_center;
    
    SHAPE_POLY_SET refPoly;
    refPoly.AddOutline( refOutline );
    refPoly.CacheTriangulation( false );

    SHAPE_LINE_CHAIN targetOutline( refOutline );

    targetOutline.Move( disp );

    auto connectivity = board()->GetConnectivity();

    if( aOpts.m_copyRouting )
    {

    for( auto& fpPair : aMatches )
    {
        auto refFP = fpPair.first;

        for( auto pad : refFP->Pads() )
        {
            auto connItems = connectivity->GetConnectedItems( pad, { PCB_PAD_T, PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T }, true );

            printf("pad [%p] %s-%s: %d items\n", pad, refFP->GetReference().c_str().AsChar(), pad->GetNumber().c_str().AsChar(), connItems.size() );

            for( auto item : connItems )
            {


                //if( ! ( item->GetLayerSet(). aRefArea->m_area->GetLayerSet() ) )
                  //  continue;
                
                printf("process conn: type %\n", item->Type() );

                auto effShape = item->GetEffectiveShape( item->GetLayer() );
                auto bb =effShape->BBox();

                printf("Effshape: %s bbox %d %d %d %d\n",
                    effShape->TypeName().c_str().AsChar(),
                    bb.GetLeft(), bb.GetTop(),
                    bb.GetRight(), bb.GetBottom()
                );


                //overlay->SetIsFill( true );
                //overlay->SetFillColor( GREEN );
                //overlay->Rectangle( bb.GetOrigin(), bb.GetEnd() );
                
                auto rbb = refPoly.BBox();
                printf("ref: bbox %d %d %d %d tris %d\n",
                    rbb.GetLeft(), rbb.GetTop(),
                    rbb.GetRight(), rbb.GetBottom()
                    );

                //overlay->SetIsFill( true );
                //overlay->SetFillColor( RED );
                //overlay->Rectangle( rbb.GetOrigin(), rbb.GetEnd() );


                if( rbb.Intersects( bb ) )
                    printf("Possible collision\n");

                if( effShape->Collide( &refPoly, 0 ) )
                {
                    //printf("Coll : %p\n", item );

                    auto ni = static_cast<BOARD_ITEM*>( item->Clone() );
                    ni->Move( disp );
                    aCommit->Add( ni );
                }
            }
        }
    }
    }

    aCommit->Modify( aTargetArea->m_area );

    aTargetArea->m_area->RemoveAllContours();
    aTargetArea->m_area->AddPolygon( targetOutline );

    for( auto& fpPair : aMatches )
    {
        auto refFP = fpPair.first;
        auto targetFP = fpPair.second;

        printf("ref-ls: %s\n", aRefArea->m_area->GetLayerSet().FmtHex().c_str() );
        printf("target-ls: %s\n", aRefArea->m_area->GetLayerSet().FmtHex().c_str() );

#if 0
        if( ! aRefArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) );
        {
            printf("discard ref:%s (ref layer)\n", refFP->GetReference().c_str().AsChar() );
            continue;
        }
        if( ! aTargetArea->m_area->GetLayerSet().Contains( refFP->GetLayer() ) );
        {
            printf("discard ref:%s (target layer)\n", refFP->GetReference().c_str().AsChar() );
            continue;
        }
#endif
        aCommit->Modify( targetFP );

        targetFP->SetOrientation( refFP->GetOrientation() );
        VECTOR2I targetPos = refFP->GetPosition() + disp;
        targetFP->SetPosition( targetPos );
        targetFP->SetLayerAndFlip( refFP->GetLayer() );
        targetFP->Reference().SetTextAngle( refFP->Reference().GetTextAngle() );
        targetFP->Reference().SetPosition( refFP->Reference().GetPosition() );
        targetFP->Value().SetTextAngle( refFP->Value().GetTextAngle() );
        targetFP->Value().SetPosition( refFP->Value().GetPosition() );
    }

    return true;
}


bool MULTICHANNEL_TOOL::resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea, MULTICHANNEL_TOOL::FP_PAIRS& aMatches )
{
    std::map<NETINFO_ITEM*, std::vector<PAD* > > allPads;

    for( auto fp : board()->Footprints() )
        for( auto pad : fp->Pads() )
        {
            auto iter = allPads.find( pad->GetNet() );

            if( iter == allPads.end() )
                allPads[ pad->GetNet() ] = { pad };
            else
                allPads[ pad->GetNet() ].push_back( pad );
        }

    auto belongsToRAFootprint = [] ( RULE_AREA* ra, PAD *aPad ) -> FOOTPRINT*
    {
        for( auto fp : ra->m_raFootprints )
            for ( auto pad : fp.fp->Pads() )
            {
                if (aPad == pad)
                    return fp.fp;
            }

        return nullptr;
    };

    auto findPadConnectionsWithinRA = [&] ( RULE_AREA* ra, PAD *aPad ) -> std::vector<PAD_PREFIX_ENTRY>
    {
        std::vector<PAD_PREFIX_ENTRY> rv;

        for( auto cpad : allPads[aPad->GetNet()] )
        {
            if( belongsToRAFootprint( ra, cpad) )
            {
                rv.push_back( PAD_PREFIX_ENTRY( cpad, stripComponentIndex( cpad->GetParentFootprint()->GetReference() ) ) );
            }
        }

        return rv;
    };

    auto buildPadConnectionsWithinRA = [&] ( RULE_AREA* ra ) -> bool
    {
        for( auto &fp : ra->m_raFootprints )
        {
            for( auto pad : fp.fp->Pads() )
            {
                fp.connsWithinRA[ pad ] = findPadConnectionsWithinRA ( ra, pad );

                #if 0
                printf("p %s-%s ->", pad->GetParentAsString().c_str().AsChar(), pad->GetNumber().c_str().AsChar() );
                for( auto p : fp.connsWithinRA[ pad ] )
                {
                    printf(" %s-%s", p.pad->GetParentAsString().c_str().AsChar(), p.pad->GetNumber().c_str().AsChar() );

                }

                printf("\n");
                #endif
            }
        }

        return true;
    };

    auto matchConnections = [&] ( FP_WITH_CONNECTIONS*aRef, FP_WITH_CONNECTIONS* aTarget ) -> bool
    {
        #if 0
        for( auto ref : aRef->connsWithinRA )
            for( auto conn : ref.second )
                printf("ref: %s-%s\n", ref.first->GetNumber().c_str().AsChar(),
                conn.format().c_str().AsChar() );

        for( auto tgt : aTarget->connsWithinRA )
            for( auto conn : tgt.second )
                printf("tgt: %s-%s\n", tgt.first->GetNumber().c_str().AsChar(),
                conn.format().c_str().AsChar() );
        #endif

        for( auto ref : aRef->connsWithinRA )
        {
            bool matchFound = false;

            for( auto tgt : aTarget->connsWithinRA)
            {
                bool padsMatch = true;
                if( ref.second.size() != tgt.second.size() )
                    continue;

                for( int i = 0; i < ref.second.size(); i++ )
                {
                    PAD_PREFIX_ENTRY&eref = ref.second[i];
                    PAD_PREFIX_ENTRY&etgt = tgt.second[i];

                    //printf("test %s vs %s\n", eref.format().c_str().AsChar(), 
                    //etgt.format().c_str().AsChar() );

                    if( ! eref.matchesPadNumberAndPrefix( etgt ))
                    {
                        padsMatch = false;
                        break;
                        //printf("i %d match found\n", i );

                        //printf("match ref %s tgt %s", eref.Format().c_str(), etgt.Format().c_str() );
                    }
                }
                if( padsMatch )
                    matchFound = true;
            }

            if( !matchFound )
                return false;
        }

        return true;
    };

    if( aRefArea->m_raFootprints.size() != aTargetArea->m_raFootprints.size() )
    {
        m_reporter->Report( wxString::Format( wxT("component count mismatch, ref=%d, target=%d\n"), 
                    (int) aRefArea->m_raFootprints.size(),
                    (int) aTargetArea->m_raFootprints.size() ) );
    }

    buildPadConnectionsWithinRA( aRefArea );
    buildPadConnectionsWithinRA( aTargetArea );

    for( auto& refFP : aRefArea->m_raFootprints )
    {
        refFP.sortByPadNumbers();
        refFP.processed = false;
    }

    for( auto& targetFP : aTargetArea->m_raFootprints )
    {
        targetFP.sortByPadNumbers();
        targetFP.processed = false;
    }

    int targetsRemaining = aTargetArea->m_raFootprints.size();

    for( auto& refFP : aRefArea->m_raFootprints )
    {
        //int tid = random() % targetsRemaining;
        //FP_WITH_CONNECTIONS* targetFP = nullptr;

        bool anyMatchesFound = false;

        for( auto &targetFP : aTargetArea->m_raFootprints )
        {
            if( stripComponentIndex( refFP.fp->GetReference() ) != stripComponentIndex( targetFP.fp->GetReference() ) )
                continue;

            if( !targetFP.processed )
            {
                bool matches = matchConnections( &refFP, &targetFP );

                //printf("testFP %s vs %s\n", refFP.fp->GetReference().c_str().AsChar(), 
                  //  targetFP.fp->GetReference().c_str().AsChar() );


                if( matches )
                {
                    printf("%s: matches %s\n", refFP.fp->GetReference().c_str().AsChar(), targetFP.fp->GetReference().c_str().AsChar() );
                    //refFP.processed = true;
                    //targetFP.processed = true;
                    anyMatchesFound = true;
                    targetFP.processed = true;
                    aMatches.push_back( { refFP.fp, targetFP.fp } );
                    break;
                }
                //else
                
                /*if( tid == 0 )
                {
                    targetFP = &fp;
                    break;
                }
                else
                    tid--;*/
            }
        }

        if( !anyMatchesFound )
        {
                    printf("%s: no match\n", refFP.fp->GetReference().c_str().AsChar() );
                    return false;
        }



    }

        /*for( auto refPad : refFP->Pads() )
        {
            auto refNet = refPad->GetNet();
            auto refPads = allPads[ refNet ];

            
        }*/
    //}

    return true;
}


int MULTICHANNEL_TOOL::autogenerateRuleAreas( const TOOL_EVENT& aEvent )
{
    //KI_TEST::CONSOLE_LOG          consoleLog;
    //m_reporter.reset( new KI_TEST::CONSOLE_MSG_REPORTER ( &consoleLog ) );

    querySheets();

    if( m_areas.m_areas.size() <= 1 )
    {
        frame()->ShowInfoBarError( 
            _("Cannot auto-generate any placement areas because the schematic has only one or no hierarchical sheet(s)."),
            true );
        return 0;
    }

    m_areas.m_replaceExisting = true;
    m_areas.m_groupItems = false;

    DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS dialog( frame(), this );
    int ret = dialog.ShowModal();

    if( ret != wxID_OK )
        return 0;

    for( auto zone : board()->Zones() )
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
            //printf("cmps: %d raf: %d\n", components.size(), ra.m_sheetComponents.size() );
            if( components  == ra.m_sheetComponents )
            {
                m_reporter->Report( wxString::Format( wxT("Placement rule area for sheet '%s' already exists as '%s'\n"), 
                    ra.m_sheetPath,
                    zone->GetZoneName()
                ) );

                ra.m_area = zone;
                ra.m_existsAlready = true;
            }
        }
    }

    m_reporter->Report( wxString::Format( wxT("%d placement areas found\n"), (int)m_areas.m_areas.size() ) );

    BOARD_COMMIT  commit( frame()->GetToolManager(), true );

    for( auto& ra : m_areas.m_areas )
    {
        if ( !ra.m_generateEnabled )
            continue;
        if ( ra.m_existsAlready && !m_areas.m_replaceExisting )
            continue;

        auto raOutline = buildRAOutline( ra.m_sheetComponents, 100000 );

        //printf("RA %s : %d vts\n", ra.m_sheetName.c_str().AsChar(), raOutline.PointCount() );

        std::unique_ptr<ZONE> newZone ( new ZONE( board() ) );

        newZone->SetZoneName( wxString::Format( wxT("auto-placement-area-%s"), ra.m_sheetPath ) );
        m_reporter->Report( wxString::Format( wxT("Generated rule area '%s' (%d components)\n"),
            newZone->GetZoneName(),
            (int) ra.m_sheetComponents.size() ) );

        newZone->SetIsRuleArea( true );
        newZone->SetLayerSet( LSET::AllCuMask() );
        newZone->SetRuleAreaType( RULE_AREA_TYPE::PLACEMENT );
        newZone->SetRuleAreaExpression( wxString::Format( wxT("A.memberOfSheet('%s')"), ra.m_sheetPath ));
        newZone->AddPolygon( raOutline );
        newZone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );
        //aBoard->Add( newZone.release() );
        commit.Add( newZone.get() );

        if( m_areas.m_groupItems )
        {
            PCB_GROUP *grp = new PCB_GROUP( board() );

            grp->AddItem( newZone.get() );

            for( auto fp : ra.m_sheetComponents )
                grp->AddItem( fp );

            commit.Add( grp );
        }

        newZone.release();
    }

    commit.Push( wxT("Auto-generate placement rule areas") );

    return true;
}
