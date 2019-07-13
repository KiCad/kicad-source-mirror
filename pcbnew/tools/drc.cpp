/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <board_design_settings.h>
#include <class_edge_mod.h>
#include <class_drawsegment.h>
#include <class_module.h>
#include <class_track.h>
#include <class_pad.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <geometry/seg.h>
#include <math_for_graphics.h>
#include <geometry/geometry_utils.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>
#include <kiface_i.h>
#include <pcbnew.h>
#include <tools/drc.h>
#include <pcb_netlist.h>

#include <dialog_drc.h>
#include <wx/progdlg.h>
#include <board_commit.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_arc.h>

#include <drc/courtyard_overlap.h>
#include "zone_filler_tool.h"

DRC::DRC() :
        PCB_TOOL_BASE( "pcbnew.DRCTool" )
{
    m_drcDialog  = NULL;

    // establish initial values for everything:
    m_doPad2PadTest     = true;         // enable pad to pad clearance tests
    m_doUnconnectedTest = true;         // enable unconnected tests
    m_doZonesTest = false;              // disable zone to items clearance tests
    m_doKeepoutTest = true;             // enable keepout areas to items clearance tests
    m_refillZones = false;              // Only fill zones if requested by user.
    m_reportAllTrackErrors = false;
    m_testFootprints = false;

    m_drcRun = false;
    m_footprintsTested = false;

    m_doCreateRptFile = false;
    // m_rptFilename set to empty by its constructor

    m_currentMarker = NULL;

    m_segmAngle  = 0;
    m_segmLength = 0;

    m_xcliplo = 0;
    m_ycliplo = 0;
    m_xcliphi = 0;
    m_ycliphi = 0;
}


DRC::~DRC()
{
    for( DRC_ITEM* unconnectedItem : m_unconnected )
        delete unconnectedItem;

    for( DRC_ITEM* footprintItem : m_footprints )
        delete footprintItem;
}


void DRC::Reset( RESET_REASON aReason )
{
    m_pcbEditorFrame = getEditFrame<PCB_EDIT_FRAME>();

    if( aReason == MODEL_RELOAD )
    {
        if( m_drcDialog )
            DestroyDRCDialog( wxID_OK );

        m_pcb = m_pcbEditorFrame->GetBoard();

        m_markerFactory.SetUnitsProvider( [=]() { return m_pcbEditorFrame->GetUserUnits(); } );
    }
}


void DRC::ShowDRCDialog( wxWindow* aParent )
{
    bool show_dlg_modal = true;

    // the dialog needs a parent frame. if it is not specified, this is
    // the PCB editor frame specified in DRC class.
    if( !aParent )
    {
        // if any parent is specified, the dialog is modal.
        // if this is the default PCB editor frame, it is not modal
        show_dlg_modal = false;
        aParent = m_pcbEditorFrame;
    }

    Activate();
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( !m_drcDialog )
    {
        m_drcDialog = new DIALOG_DRC_CONTROL( this, m_pcbEditorFrame, aParent );
        updatePointers();

        m_drcDialog->SetRptSettings( m_doCreateRptFile, m_rptFilename );

        if( show_dlg_modal )
            m_drcDialog->ShowModal();
        else
            m_drcDialog->Show( true );
    }
    else    // The dialog is just not visible (because the user has double clicked on an error item)
    {
        updatePointers();
        m_drcDialog->Show( true );
    }
}


int DRC::ShowDRCDialog( const TOOL_EVENT& aEvent )
{
    ShowDRCDialog( nullptr );
    return 0;
}


void DRC::addMarkerToPcb( MARKER_PCB* aMarker )
{
    BOARD_COMMIT commit( m_pcbEditorFrame );
    commit.Add( aMarker );
    commit.Push( wxEmptyString, false, false );
}


void DRC::DestroyDRCDialog( int aReason )
{
    if( m_drcDialog )
    {
        m_drcDialog->GetRptSettings( &m_doCreateRptFile, m_rptFilename);

        m_drcDialog->Destroy();
        m_drcDialog = nullptr;
    }
}


int DRC::TestZoneToZoneOutline( ZONE_CONTAINER* aZone, bool aCreateMarkers )
{
    BOARD* board = m_pcbEditorFrame->GetBoard();
    BOARD_COMMIT commit( m_pcbEditorFrame );
    int nerrors = 0;

    std::vector<SHAPE_POLY_SET> smoothed_polys;
    smoothed_polys.resize( board->GetAreaCount() );

    for( int ia = 0; ia < board->GetAreaCount(); ia++ )
    {
        ZONE_CONTAINER*    zoneRef = board->GetArea( ia );
        std::set<VECTOR2I> colinearCorners;
        zoneRef->GetColinearCorners( board, colinearCorners );

        zoneRef->BuildSmoothedPoly( smoothed_polys[ia], &colinearCorners );
    }

    // iterate through all areas
    for( int ia = 0; ia < board->GetAreaCount(); ia++ )
    {
        ZONE_CONTAINER* zoneRef = board->GetArea( ia );

        if( !zoneRef->IsOnCopperLayer() )
            continue;

        // When testing only a single area, skip all others
        if( aZone && ( aZone != zoneRef) )
            continue;

        // If we are testing a single zone, then iterate through all other zones
        // Otherwise, we have already tested the zone combination
        for( int ia2 = ( aZone ? 0 : ia + 1 ); ia2 < board->GetAreaCount(); ia2++ )
        {
            ZONE_CONTAINER* zoneToTest = board->GetArea( ia2 );

            if( zoneRef == zoneToTest )
                continue;

            // test for same layer
            if( zoneRef->GetLayer() != zoneToTest->GetLayer() )
                continue;

            // Test for same net
            if( zoneRef->GetNetCode() == zoneToTest->GetNetCode() && zoneRef->GetNetCode() >= 0 )
                continue;

            // test for different priorities
            if( zoneRef->GetPriority() != zoneToTest->GetPriority() )
                continue;

            // test for different types
            if( zoneRef->GetIsKeepout() != zoneToTest->GetIsKeepout() )
                continue;

            // Examine a candidate zone: compare zoneToTest to zoneRef

            // Get clearance used in zone to zone test.  The policy used to
            // obtain that value is now part of the zone object itself by way of
            // ZONE_CONTAINER::GetClearance().
            int zone2zoneClearance = zoneRef->GetClearance( zoneToTest );

            // Keepout areas have no clearance, so set zone2zoneClearance to 1
            // ( zone2zoneClearance = 0  can create problems in test functions)
            if( zoneRef->GetIsKeepout() )
                zone2zoneClearance = 1;

            // test for some corners of zoneRef inside zoneToTest
            for( auto iterator = smoothed_polys[ia].IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;
                wxPoint pt( currentVertex.x, currentVertex.y );

                if( smoothed_polys[ia2].Contains( currentVertex ) )
                {
                    if( aCreateMarkers )
                        commit.Add( m_markerFactory.NewMarker( pt, zoneRef, zoneToTest,
                                                               DRCE_ZONES_INTERSECT ) );

                    nerrors++;
                }
            }

            // test for some corners of zoneToTest inside zoneRef
            for( auto iterator = smoothed_polys[ia2].IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;
                wxPoint pt( currentVertex.x, currentVertex.y );

                if( smoothed_polys[ia].Contains( currentVertex ) )
                {
                    if( aCreateMarkers )
                        commit.Add( m_markerFactory.NewMarker( pt, zoneToTest, zoneRef,
                                                               DRCE_ZONES_INTERSECT ) );

                    nerrors++;
                }
            }

            // Iterate through all the segments of refSmoothedPoly
            std::set<wxPoint> conflictPoints;

            for( auto refIt = smoothed_polys[ia].IterateSegmentsWithHoles(); refIt; refIt++ )
            {
                // Build ref segment
                SEG refSegment = *refIt;

                // Iterate through all the segments in smoothed_polys[ia2]
                for( auto testIt = smoothed_polys[ia2].IterateSegmentsWithHoles(); testIt; testIt++ )
                {
                    // Build test segment
                    SEG testSegment = *testIt;
                    wxPoint pt;

                    int ax1, ay1, ax2, ay2;
                    ax1 = refSegment.A.x;
                    ay1 = refSegment.A.y;
                    ax2 = refSegment.B.x;
                    ay2 = refSegment.B.y;

                    int bx1, by1, bx2, by2;
                    bx1 = testSegment.A.x;
                    by1 = testSegment.A.y;
                    bx2 = testSegment.B.x;
                    by2 = testSegment.B.y;

                    int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                         0,
                                                         ax1, ay1, ax2, ay2,
                                                         0,
                                                         zone2zoneClearance,
                                                         &pt.x, &pt.y );

                    if( d < zone2zoneClearance )
                        conflictPoints.insert( pt );
                }
            }

            for( wxPoint pt : conflictPoints )
            {
                if( aCreateMarkers )
                    commit.Add( m_markerFactory.NewMarker( pt, zoneRef, zoneToTest,
                                                           DRCE_ZONES_TOO_CLOSE ) );

                nerrors++;
            }
        }
    }

    if( aCreateMarkers )
        commit.Push( wxEmptyString, false, false );

    return nerrors;
}


void DRC::RunTests( wxTextCtrl* aMessages )
{
    // be sure m_pcb is the current board, not a old one
    // ( the board can be reloaded )
    m_pcb = m_pcbEditorFrame->GetBoard();

    if( aMessages )
    {
        aMessages->AppendText( _( "Board Outline...\n" ) );
        wxSafeYield();
    }

    testOutline();

    // someone should have cleared the two lists before calling this.
    if( !testNetClasses() )
    {
        // testing the netclasses is a special case because if the netclasses
        // do not pass the BOARD_DESIGN_SETTINGS checks, then every member of a net
        // class (a NET) will cause its items such as tracks, vias, and pads
        // to also fail.  So quit after *all* netclass errors have been reported.
        if( aMessages )
            aMessages->AppendText( _( "Aborting\n" ) );

        // update the m_drcDialog listboxes
        updatePointers();

        return;
    }

    // test pad to pad clearances, nothing to do with tracks, vias or zones.
    if( m_doPad2PadTest )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Pad clearances...\n" ) );
            wxSafeYield();
        }

        testPad2Pad();
    }

    // test clearances between drilled holes
    if( aMessages )
    {
        aMessages->AppendText( _( "Drill clearances...\n" ) );
        wxSafeYield();
    }

    testDrilledHoles();

    // caller (a wxTopLevelFrame) is the wxDialog or the Pcb Editor frame that call DRC:
    wxWindow* caller = aMessages ? aMessages->GetParent() : m_pcbEditorFrame;

    if( m_refillZones )
    {
        if( aMessages )
            aMessages->AppendText( _( "Refilling all zones...\n" ) );

        m_toolMgr->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( caller );
    }
    else
    {
        if( aMessages )
            aMessages->AppendText( _( "Checking zone fills...\n" ) );

        m_toolMgr->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( caller );
    }

    // test track and via clearances to other tracks, pads, and vias
    if( aMessages )
    {
        aMessages->AppendText( _( "Track clearances...\n" ) );
        wxSafeYield();
    }

    testTracks( aMessages ? aMessages->GetParent() : m_pcbEditorFrame, true );

    // test zone clearances to other zones
    if( aMessages )
    {
        aMessages->AppendText( _( "Zone to zone clearances...\n" ) );
        wxSafeYield();
    }

    testZones();

    // find and gather unconnected pads.
    if( m_doUnconnectedTest )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Unconnected pads...\n" ) );
            aMessages->Refresh();
        }

        testUnconnected();
    }

    // find and gather vias, tracks, pads inside keepout areas.
    if( m_doKeepoutTest )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Keepout areas ...\n" ) );
            aMessages->Refresh();
        }

        testKeepoutAreas();
    }

    // find and gather vias, tracks, pads inside text boxes.
    if( aMessages )
    {
        aMessages->AppendText( _( "Text and graphic clearances...\n" ) );
        wxSafeYield();
    }

    testCopperTextAndGraphics();

    // find overlapping courtyard ares.
    if( m_pcb->GetDesignSettings().m_ProhibitOverlappingCourtyards
        || m_pcb->GetDesignSettings().m_RequireCourtyards )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Courtyard areas...\n" ) );
            aMessages->Refresh();
        }

        doFootprintOverlappingDrc();
    }

    for( DRC_ITEM* footprintItem : m_footprints )
        delete footprintItem;

    m_footprints.clear();
    m_footprintsTested = false;

    if( m_testFootprints && !Kiface().IsSingle() )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Checking footprints against schematic...\n" ) );
            aMessages->Refresh();
        }

        NETLIST netlist;
        m_pcbEditorFrame->FetchNetlistFromSchematic( netlist, PCB_EDIT_FRAME::ANNOTATION_DIALOG );

        if( m_drcDialog )
            m_drcDialog->Raise();

        TestFootprints( netlist, m_pcb, m_drcDialog->GetUserUnits(), m_footprints );
        m_footprintsTested = true;
    }

    // Check if there are items on disabled layers
    testDisabledLayers();

    if( aMessages )
    {
        aMessages->AppendText( _( "Items on disabled layers...\n" ) );
        aMessages->Refresh();
    }

    m_drcRun = true;

    // update the m_drcDialog listboxes
    updatePointers();

    if( aMessages )
    {
        // no newline on this one because it is last, don't want the window
        // to unnecessarily scroll.
        aMessages->AppendText( _( "Finished" ) );
    }
}


void DRC::updatePointers()
{
    // update my pointers, m_pcbEditorFrame is the only unchangeable one
    m_pcb = m_pcbEditorFrame->GetBoard();

    if( m_drcDialog )  // Use diag list boxes only in DRC dialog
    {
        m_drcDialog->m_ClearanceListBox->SetList(
                m_pcbEditorFrame->GetUserUnits(), new DRC_LIST_MARKERS( m_pcb ) );
        m_drcDialog->m_UnconnectedListBox->SetList(
                m_pcbEditorFrame->GetUserUnits(), new DRC_LIST_GENERIC( &m_unconnected ) );
        m_drcDialog->m_FootprintsListBox->SetList(
                m_pcbEditorFrame->GetUserUnits(), new DRC_LIST_GENERIC( &m_footprints ) );

        m_drcDialog->UpdateDisplayedCounts();
    }
}


bool DRC::doNetClass( const NETCLASSPTR& nc, wxString& msg )
{
    bool ret = true;

    const BOARD_DESIGN_SETTINGS& g = m_pcb->GetDesignSettings();

#define FmtVal( x ) GetChars( StringFromValue( m_pcbEditorFrame->GetUserUnits(), x ) )

#if 0   // set to 1 when (if...) BOARD_DESIGN_SETTINGS has a m_MinClearance value
    if( nc->GetClearance() < g.m_MinClearance )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has Clearance:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetClearance() ),
                    FmtVal( g.m_TrackClearance )
                    );

        addMarkerToPcb( fillMarker( DRCE_NETCLASS_CLEARANCE, msg, m_currentMarker ) );
        m_currentMarker = nullptr;
        ret = false;
    }
#endif

    if( nc->GetTrackWidth() < g.m_TrackMinWidth )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has TrackWidth:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetTrackWidth() ),
                    FmtVal( g.m_TrackMinWidth )
                    );

        addMarkerToPcb( m_markerFactory.NewMarker( DRCE_NETCLASS_TRACKWIDTH, msg ) );
        ret = false;
    }

    if( nc->GetViaDiameter() < g.m_ViasMinSize )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has Via Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDiameter() ),
                    FmtVal( g.m_ViasMinSize )
                    );

        addMarkerToPcb( m_markerFactory.NewMarker( DRCE_NETCLASS_VIASIZE, msg ) );
        ret = false;
    }

    if( nc->GetViaDrill() < g.m_ViasMinDrill )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has Via Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDrill() ),
                    FmtVal( g.m_ViasMinDrill )
                    );

        addMarkerToPcb( m_markerFactory.NewMarker( DRCE_NETCLASS_VIADRILLSIZE, msg ) );
        ret = false;
    }

    if( nc->GetuViaDiameter() < g.m_MicroViasMinSize )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has uVia Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDiameter() ),
                    FmtVal( g.m_MicroViasMinSize ) );

        addMarkerToPcb( m_markerFactory.NewMarker( DRCE_NETCLASS_uVIASIZE, msg ) );
        ret = false;
    }

    if( nc->GetuViaDrill() < g.m_MicroViasMinDrill )
    {
        msg.Printf( _( "NETCLASS: \"%s\" has uVia Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDrill() ),
                    FmtVal( g.m_MicroViasMinDrill ) );

        addMarkerToPcb( m_markerFactory.NewMarker( DRCE_NETCLASS_uVIADRILLSIZE, msg ) );
        ret = false;
    }

    return ret;
}


bool DRC::testNetClasses()
{
    bool        ret = true;

    NETCLASSES& netclasses = m_pcb->GetDesignSettings().m_NetClasses;

    wxString    msg;   // construct this only once here, not in a loop, since somewhat expensive.

    if( !doNetClass( netclasses.GetDefault(), msg ) )
        ret = false;

    for( NETCLASSES::const_iterator i = netclasses.begin();  i != netclasses.end();  ++i )
    {
        NETCLASSPTR nc = i->second;

        if( !doNetClass( nc, msg ) )
            ret = false;
    }

    return ret;
}


void DRC::testPad2Pad()
{
    std::vector<D_PAD*> sortedPads;

    m_pcb->GetSortedPadListByXthenYCoord( sortedPads );

    if( sortedPads.empty() )
        return;

    // find the max size of the pads (used to stop the test)
    int max_size = 0;

    for( D_PAD* pad : sortedPads )
    {
        // GetBoundingRadius() is the radius of the minimum sized circle fully containing the pad
        int radius = pad->GetBoundingRadius();

        if( radius > max_size )
            max_size = radius;
    }

    // Upper limit of pad list (limit not included)
    D_PAD** listEnd = &sortedPads[0] + sortedPads.size();

    // Test the pads
    for( auto& pad : sortedPads )
    {
        int x_limit = pad->GetClearance() + pad->GetBoundingRadius() + pad->GetPosition().x;

        if( !doPadToPadsDrc( pad, &pad, listEnd, max_size + x_limit ) )
        {
            wxASSERT( m_currentMarker );
            addMarkerToPcb ( m_currentMarker );
            m_currentMarker = nullptr;
        }
    }
}


void DRC::testDrilledHoles()
{
    int holeToHoleMin = m_pcb->GetDesignSettings().m_HoleToHoleMin;

    if( holeToHoleMin == 0 )    // No min setting turns testing off.
        return;

    // Test drilled hole clearances to minimize drill bit breakage.
    //
    // Notes: slots are milled, so we're only concerned with circular holes
    //        microvias are laser-drilled, so we're only concerned with standard vias

    struct DRILLED_HOLE
    {
        wxPoint     m_location;
        int         m_drillRadius;
        BOARD_ITEM* m_owner;
    };

    std::vector<DRILLED_HOLE> holes;
    DRILLED_HOLE              hole;

    for( MODULE* mod : m_pcb->Modules() )
    {
        for( D_PAD* pad : mod->Pads( ) )
        {
            if( pad->GetDrillSize().x && pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
            {
                hole.m_location = pad->GetPosition();
                hole.m_drillRadius = pad->GetDrillSize().x / 2;
                hole.m_owner = pad;
                holes.push_back( hole );
            }
        }
    }

    for( TRACK* track : m_pcb->Tracks() )
    {
        VIA* via = dynamic_cast<VIA*>( track );
        if( via && via->GetViaType() == VIA_THROUGH )
        {
            hole.m_location = via->GetPosition();
            hole.m_drillRadius = via->GetDrillValue() / 2;
            hole.m_owner = via;
            holes.push_back( hole );
        }
    }

    for( size_t ii = 0; ii < holes.size(); ++ii )
    {
        const DRILLED_HOLE& refHole = holes[ ii ];

        for( size_t jj = ii + 1; jj < holes.size(); ++jj )
        {
            const DRILLED_HOLE& checkHole = holes[ jj ];

            // Holes with identical locations are allowable
            if( checkHole.m_location == refHole.m_location )
                continue;

            if( KiROUND( GetLineLength( checkHole.m_location, refHole.m_location ) )
                    <  checkHole.m_drillRadius + refHole.m_drillRadius + holeToHoleMin )
            {
                addMarkerToPcb( new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(),
                                                DRCE_DRILLED_HOLES_TOO_CLOSE, refHole.m_location,
                                                refHole.m_owner, refHole.m_location,
                                                checkHole.m_owner, checkHole.m_location ) );
            }
        }
    }
}


void DRC::testTracks( wxWindow *aActiveWindow, bool aShowProgressBar )
{
    wxProgressDialog * progressDialog = NULL;
    const int delta = 500;  // This is the number of tests between 2 calls to the
                            // progress bar
    int count = m_pcb->Tracks().size();

    int deltamax = count/delta;

    if( aShowProgressBar && deltamax > 3 )
    {
        // Do not use wxPD_APP_MODAL style here: it is not necessary and create issues
        // on OSX
        progressDialog = new wxProgressDialog( _( "Track clearances" ), wxEmptyString,
                                               deltamax, aActiveWindow,
                                               wxPD_AUTO_HIDE | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME );
        progressDialog->Update( 0, wxEmptyString );
    }

    int ii = 0;
    count = 0;

    for( auto seg_it = m_pcb->Tracks().begin(); seg_it != m_pcb->Tracks().end(); seg_it++ )
    {
        if( ii++ > delta )
        {
            ii = 0;
            count++;

            if( progressDialog )
            {
                if( !progressDialog->Update( count, wxEmptyString ) )
                    break;  // Aborted by user
#ifdef __WXMAC__
                // Work around a dialog z-order issue on OS X
                if( count == deltamax )
                    aActiveWindow->Raise();
#endif
            }
        }

        // Test new segment against tracks and pads, optionally against copper zones
        if( !doTrackDrc( *seg_it, seg_it + 1, m_pcb->Tracks().end(), m_doZonesTest ) )
        {
            if( m_currentMarker )
            {
                addMarkerToPcb ( m_currentMarker );
                m_currentMarker = nullptr;
            }
        }
    }

    if( progressDialog )
        progressDialog->Destroy();
}


void DRC::testUnconnected()
{
    for( DRC_ITEM* unconnectedItem : m_unconnected )
        delete unconnectedItem;

    m_unconnected.clear();

    auto connectivity = m_pcb->GetConnectivity();

    connectivity->Clear();
    connectivity->Build( m_pcb ); // just in case. This really needs to be reliable.
    connectivity->RecalculateRatsnest();

    std::vector<CN_EDGE> edges;
    connectivity->GetUnconnectedEdges( edges );

    for( const auto& edge : edges )
    {
        auto src = edge.GetSourcePos();
        auto dst = edge.GetTargetPos();

        m_unconnected.emplace_back( new DRC_ITEM( m_pcbEditorFrame->GetUserUnits(),
                                                  DRCE_UNCONNECTED_ITEMS,
                                                  edge.GetSourceNode()->Parent(),
                                                  wxPoint( src.x, src.y ),
                                                  edge.GetTargetNode()->Parent(),
                                                  wxPoint( dst.x, dst.y ) ) );
    }
}


void DRC::testZones()
{
    // Test copper areas for valid netcodes
    // if a netcode is < 0 the netname was not found when reading a netlist
    // if a netcode is == 0 the netname is void, and the zone is not connected.
    // This is allowed, but i am not sure this is a good idea
    //
    // In recent Pcbnew versions, the netcode is always >= 0, but an internal net name
    // is stored, and initialized from the file or the zone properties editor.
    // if it differs from the net name from net code, there is a DRC issue
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = m_pcb->GetArea( ii );

        if( !zone->IsOnCopperLayer() )
            continue;

        int netcode = zone->GetNetCode();
        // a netcode < 0 or > 0 and no pad in net  is a error or strange
        // perhaps a "dead" net, which happens when all pads in this net were removed
        // Remark: a netcode < 0 should not happen (this is more a bug somewhere)
        int pads_in_net = ( netcode > 0 ) ? m_pcb->GetConnectivity()->GetPadCount( netcode ) : 1;

        if( ( netcode < 0 ) || pads_in_net == 0 )
        {
            wxPoint markerPos = zone->GetPosition();
            addMarkerToPcb( m_markerFactory.NewMarker( markerPos, zone,
                                                       DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE ) );
        }
    }

    // Test copper areas outlines, and create markers when needed
    TestZoneToZoneOutline( NULL, true );
}


void DRC::testKeepoutAreas()
{
    // Test keepout areas for vias, tracks and pads inside keepout areas
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* area = m_pcb->GetArea( ii );

        if( !area->GetIsKeepout() )
            continue;

        for( auto segm : m_pcb->Tracks() )
        {
            if( segm->Type() == PCB_TRACE_T )
            {
                if( !area->GetDoNotAllowTracks()  )
                    continue;

                // Ignore if the keepout zone is not on the same layer
                if( !area->IsOnLayer( segm->GetLayer() ) )
                    continue;

                SEG trackSeg( segm->GetStart(), segm->GetEnd() );

                if( area->Outline()->Distance( trackSeg, segm->GetWidth() ) == 0 )
                    addMarkerToPcb(
                            m_markerFactory.NewMarker( segm, area, DRCE_TRACK_INSIDE_KEEPOUT ) );
            }
            else if( segm->Type() == PCB_VIA_T )
            {
                if( ! area->GetDoNotAllowVias()  )
                    continue;

                auto viaLayers = segm->GetLayerSet();

                if( !area->CommonLayerExists( viaLayers ) )
                    continue;

                if( area->Outline()->Distance( segm->GetPosition() ) < segm->GetWidth()/2 )
                    addMarkerToPcb(
                            m_markerFactory.NewMarker( segm, area, DRCE_VIA_INSIDE_KEEPOUT ) );
            }
        }
        // Test pads: TODO
    }
}


void DRC::testCopperTextAndGraphics()
{
    // Test copper items for clearance violations with vias, tracks and pads

    for( BOARD_ITEM* brdItem : m_pcb->Drawings() )
    {
        if( IsCopperLayer( brdItem->GetLayer() ) )
        {
            if( brdItem->Type() == PCB_TEXT_T )
                testCopperTextItem( brdItem );
            else if( brdItem->Type() == PCB_LINE_T )
                testCopperDrawItem( static_cast<DRAWSEGMENT*>( brdItem ));
        }
    }

    for( MODULE* module : m_pcb->Modules() )
    {
        TEXTE_MODULE& ref = module->Reference();
        TEXTE_MODULE& val = module->Value();

        if( ref.IsVisible() && IsCopperLayer( ref.GetLayer() ) )
            testCopperTextItem( &ref );

        if( val.IsVisible() && IsCopperLayer( val.GetLayer() ) )
            testCopperTextItem( &val );

        if( module->IsNetTie() )
            continue;

        for( auto item : module->GraphicalItems() )
        {
            if( IsCopperLayer( item->GetLayer() ) )
            {
                if( item->Type() == PCB_MODULE_TEXT_T && ( (TEXTE_MODULE*) item )->IsVisible() )
                    testCopperTextItem( item );
                else if( item->Type() == PCB_MODULE_EDGE_T )
                    testCopperDrawItem( static_cast<DRAWSEGMENT*>( item ));
            }
        }
    }
}


void DRC::testCopperDrawItem( DRAWSEGMENT* aItem )
{
    std::vector<SEG> itemShape;
    int itemWidth = aItem->GetWidth();

    switch( aItem->GetShape() )
    {
    case S_ARC:
    {
        SHAPE_ARC arc( aItem->GetCenter(), aItem->GetArcStart(), (double) aItem->GetAngle() / 10.0 );

        auto l = arc.ConvertToPolyline();

        for( int i = 0; i < l.SegmentCount(); i++ )
            itemShape.push_back( l.CSegment(i) );

        break;
    }

    case S_SEGMENT:
        itemShape.emplace_back( SEG( aItem->GetStart(), aItem->GetEnd() ) );
        break;

    case S_CIRCLE:
    {
        // SHAPE_CIRCLE has no ConvertToPolyline() method, so use a 360.0 SHAPE_ARC
        SHAPE_ARC circle( aItem->GetCenter(), aItem->GetEnd(), 360.0 );

        auto l = circle.ConvertToPolyline();

        for( int i = 0; i < l.SegmentCount(); i++ )
            itemShape.push_back( l.CSegment(i) );

        break;
    }

    case S_CURVE:
    {
        aItem->RebuildBezierToSegmentsPointsList( aItem->GetWidth() );
        wxPoint start_pt = aItem->GetBezierPoints()[0];

        for( unsigned int jj = 1; jj < aItem->GetBezierPoints().size(); jj++ )
        {
            wxPoint end_pt = aItem->GetBezierPoints()[jj];
            itemShape.emplace_back( SEG( start_pt, end_pt ) );
            start_pt = end_pt;
        }

        break;
    }

    default:
        break;
    }

    // Test tracks and vias
    for( auto track : m_pcb->Tracks() )
    {
        if( !track->IsOnLayer( aItem->GetLayer() ) )
            continue;

        int minDist = ( track->GetWidth() + itemWidth ) / 2 + track->GetClearance( NULL );
        SEG trackAsSeg( track->GetStart(), track->GetEnd() );

        for( const auto& itemSeg : itemShape )
        {
            if( trackAsSeg.Distance( itemSeg ) < minDist )
            {
                if( track->Type() == PCB_VIA_T )
                    addMarkerToPcb( m_markerFactory.NewMarker(
                            track, aItem, itemSeg, DRCE_VIA_NEAR_COPPER ) );
                else
                    addMarkerToPcb( m_markerFactory.NewMarker(
                            track, aItem, itemSeg, DRCE_TRACK_NEAR_COPPER ) );
                break;
            }
        }
    }

    // Test pads
    for( auto pad : m_pcb->GetPads() )
    {
        if( !pad->IsOnLayer( aItem->GetLayer() ) )
            continue;

        // Graphic items are allowed to act as net-ties within their own footprint
        if( pad->GetParent() == aItem->GetParent() )
            continue;

        SHAPE_POLY_SET padOutline;
        pad->TransformShapeWithClearanceToPolygon( padOutline, pad->GetClearance( NULL ) );

        for( const auto& itemSeg : itemShape )
        {
            if( padOutline.Distance( itemSeg, itemWidth ) == 0 )
            {
                addMarkerToPcb( m_markerFactory.NewMarker( pad, aItem, DRCE_PAD_NEAR_COPPER ) );
                break;
            }
        }
    }
}


void DRC::testCopperTextItem( BOARD_ITEM* aTextItem )
{
    EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aTextItem );

    if( text == nullptr )
        return;

    std::vector<wxPoint> textShape;      // a buffer to store the text shape (set of segments)
    int textWidth = text->GetThickness();

    // So far the bounding box makes up the text-area
    text->TransformTextShapeToSegmentList( textShape );

    if( textShape.size() == 0 )     // Should not happen (empty text?)
        return;

    EDA_RECT bbox = text->GetTextBox();
    SHAPE_RECT rect_area( bbox.GetX(), bbox.GetY(), bbox.GetWidth(), bbox.GetHeight() );

    // Test tracks and vias
    for( auto track : m_pcb->Tracks() )
    {
        if( !track->IsOnLayer( aTextItem->GetLayer() ) )
            continue;

        int minDist = ( track->GetWidth() + textWidth ) / 2 + track->GetClearance( NULL );
        SEG trackAsSeg( track->GetStart(), track->GetEnd() );

        // Fast test to detect a trach segment candidate inside the text bounding box
        if( !rect_area.Collide( trackAsSeg, minDist ) )
            continue;

        for( unsigned jj = 0; jj < textShape.size(); jj += 2 )
        {
            SEG textSeg( textShape[jj], textShape[jj+1] );

            if( trackAsSeg.Distance( textSeg ) < minDist )
            {
                if( track->Type() == PCB_VIA_T )
                    addMarkerToPcb( m_markerFactory.NewMarker(
                            track, aTextItem, textSeg, DRCE_VIA_NEAR_COPPER ) );
                else
                    addMarkerToPcb( m_markerFactory.NewMarker(
                            track, aTextItem, textSeg, DRCE_TRACK_NEAR_COPPER ) );
                break;
            }
        }
    }

    // Test pads
    for( auto pad : m_pcb->GetPads() )
    {
        if( !pad->IsOnLayer( aTextItem->GetLayer() ) )
            continue;

        // Fast test to detect a pad candidate inside the text bounding box
        // Finer test (time consumming) is made only for pads near the text.
        int bb_radius = pad->GetBoundingRadius() + pad->GetClearance( NULL );
        VECTOR2I shape_pos( pad->ShapePos() );

        if( !rect_area.Collide( SEG( shape_pos, shape_pos ), bb_radius ) )
            continue;

        SHAPE_POLY_SET padOutline;

        int minDist = textWidth/2 + pad->GetClearance( NULL );
        pad->TransformShapeWithClearanceToPolygon( padOutline, 0 );

        for( unsigned jj = 0; jj < textShape.size(); jj += 2 )
        {
            SEG textSeg( textShape[jj], textShape[jj+1] );

            if( padOutline.Distance( textSeg, 0 ) <= minDist )
            {
                addMarkerToPcb( m_markerFactory.NewMarker( pad, aTextItem, DRCE_PAD_NEAR_COPPER ) );
                break;
            }
        }
    }
}


void DRC::testOutline()
{
    wxPoint error_loc( m_pcb->GetBoardEdgesBoundingBox().GetPosition() );

    m_board_outlines.RemoveAllContours();

    if( !m_pcb->GetBoardPolygonOutlines( m_board_outlines, nullptr, &error_loc ) )
    {
        addMarkerToPcb( m_markerFactory.NewMarker( error_loc, m_pcb, DRCE_INVALID_OUTLINE ) );
        return;
    }
}


void DRC::testDisabledLayers()
{
    BOARD* board = m_pcbEditorFrame->GetBoard();
    wxCHECK( board, /*void*/ );
    LSET disabledLayers = board->GetEnabledLayers().flip();

    // Perform the test only for copper layers
    disabledLayers &= LSET::AllCuMask();

    auto createMarker = [&]( BOARD_ITEM* aItem ) {
        addMarkerToPcb( m_markerFactory.NewMarker(
                aItem->GetPosition(), aItem, DRCE_DISABLED_LAYER_ITEM ) );
    };

    for( auto track : board->Tracks() )
    {
        if( disabledLayers.test( track->GetLayer() ) )
            createMarker( track );
    }

    for( auto module : board->Modules() )
    {
        module->RunOnChildren( [&]( BOARD_ITEM* aItem )
            {
                if( disabledLayers.test( aItem->GetLayer() ) )
                    createMarker( aItem );
            } );
    }

    for( auto zone : board->Zones() )
    {
        if( disabledLayers.test( zone->GetLayer() ) )
            createMarker( zone );
    }
}


bool DRC::doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    LSET layerMask = aRefPad->GetLayerSet() & all_cu;

    /* used to test DRC pad to holes: this dummy pad has the size and shape of the hole
     * to test pad to pad hole DRC, using the pad to pad DRC test function.
     * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE  dummymodule( m_pcb );    // Creates a dummy parent
    D_PAD   dummypad( &dummymodule );

    // Ensure the hole is on all copper layers
    dummypad.SetLayerSet( all_cu | dummypad.GetLayerSet() );

    // Use the minimal local clearance value for the dummy pad.
    // The clearance of the active pad will be used as minimum distance to a hole
    // (a value = 0 means use netclass value)
    dummypad.SetLocalClearance( 1 );

    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;

        // We can stop the test when pad->GetPosition().x > x_limit
        // because the list is sorted by X values
        if( pad->GetPosition().x > x_limit )
            break;

        // No problem if pads which are on copper layers are on different copper layers,
        // (pads can be only on a technical layer, to build complex pads)
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->GetLayerSet() & layerMask ) == 0 &&
            ( pad->GetLayerSet() & all_cu ) != 0 &&
            ( aRefPad->GetLayerSet() & all_cu ) != 0 )
        {
            // if holes are in the same location and have the same size and shape,
            // this can be accepted
            if( pad->GetPosition() == aRefPad->GetPosition()
                && pad->GetDrillSize() == aRefPad->GetDrillSize()
                && pad->GetDrillShape() == aRefPad->GetDrillShape() )
            {
                if( aRefPad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                    continue;

                // for oval holes: must also have the same orientation
                if( pad->GetOrientation() == aRefPad->GetOrientation() )
                    continue;
            }

            /* Here, we must test clearance between holes and pads
             * dummy pad size and shape is adjusted to pad drill size and shape
             */
            if( pad->GetDrillSize().x )
            {
                // pad under testing has a hole, test this hole against pad reference
                dummypad.SetPosition( pad->GetPosition() );
                dummypad.SetSize( pad->GetDrillSize() );
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                                           PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( pad->GetOrientation() );

                if( !checkClearancePadToPad( aRefPad, &dummypad ) )
                {
                    // here we have a drc error on pad!
                    m_currentMarker = m_markerFactory.NewMarker( pad, aRefPad, DRCE_HOLE_NEAR_PAD );
                    return false;
                }
            }

            if( aRefPad->GetDrillSize().x ) // pad reference has a hole
            {
                dummypad.SetPosition( aRefPad->GetPosition() );
                dummypad.SetSize( aRefPad->GetDrillSize() );
                dummypad.SetShape( aRefPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                                               PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( aRefPad->GetOrientation() );

                if( !checkClearancePadToPad( pad, &dummypad ) )
                {
                    // here we have a drc error on aRefPad!
                    m_currentMarker = m_markerFactory.NewMarker( aRefPad, pad, DRCE_HOLE_NEAR_PAD );
                    return false;
                }
            }

            continue;
        }

        // The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
        // But no problem if pads have the same netcode (same net)
        if( pad->GetNetCode() && ( aRefPad->GetNetCode() == pad->GetNetCode() ) )
            continue;

        // if pads are from the same footprint
        if( pad->GetParent() == aRefPad->GetParent() )
        {
            // and have the same pad number ( equivalent pads  )

            // one can argue that this 2nd test is not necessary, that any
            // two pads from a single module are acceptable.  This 2nd test
            // should eventually be a configuration option.
            if( pad->PadNameEqual( aRefPad ) )
                continue;
        }

        // if either pad has no drill and is only on technical layers, not a clearance violation
        if( ( ( pad->GetLayerSet() & layerMask ) == 0 && !pad->GetDrillSize().x ) ||
            ( ( aRefPad->GetLayerSet() & layerMask ) == 0 && !aRefPad->GetDrillSize().x ) )
        {
            continue;
        }

        if( !checkClearancePadToPad( aRefPad, pad ) )
        {
            // here we have a drc error!
            m_currentMarker = m_markerFactory.NewMarker( aRefPad, pad, DRCE_PAD_NEAR_PAD1 );
            return false;
        }
    }

    return true;
}


void DRC::doFootprintOverlappingDrc()
{
    DRC_COURTYARD_OVERLAP drc_overlap(
            m_markerFactory, [&]( MARKER_PCB* aMarker ) { addMarkerToPcb( aMarker ); } );

    drc_overlap.RunDRC( *m_pcb );
}


void DRC::TestFootprints( NETLIST& aNetlist, BOARD* aPCB, EDA_UNITS_T aUnits,
                          DRC_LIST& aDRCList )
{

    // Search for duplicate footprints on the board
    auto comp = []( const MODULE* x, const MODULE* y ) {
        return x->GetReference().CmpNoCase( y->GetReference() ) < 0;
    };
    auto mods = std::set<MODULE*, decltype( comp )>( comp );

    for( auto mod : aPCB->Modules() )
    {
        auto ins = mods.insert( mod );
        if( !ins.second )
        {
            aDRCList.emplace_back( new DRC_ITEM( aUnits, DRCE_DUPLICATE_FOOTPRINT, mod,
                    mod->GetPosition(), *ins.first, ( *ins.first )->GetPosition() ) );
        }
    }

    // Search for component footprints in the netlist but not on the board.
    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        COMPONENT* component = aNetlist.GetComponent( ii );

        auto module = aPCB->FindModuleByReference( component->GetReference() );

        if( module == NULL )
        {
            wxString msg = wxString::Format( wxT( "%s (%s)" ),
                                             component->GetReference(),
                                             component->GetValue() );

            aDRCList.emplace_back( new DRC_ITEM( DRCE_MISSING_FOOTPRINT, msg ) );
        }
    }

    // Search for component footprints found on board but not in netlist.
    for( auto module : mods )
    {
        COMPONENT* component = aNetlist.GetComponentByReference( module->GetReference() );

        if( component == NULL )
        {
            aDRCList.emplace_back( new DRC_ITEM( aUnits, DRCE_EXTRA_FOOTPRINT,
                                                 module, module->GetPosition(),
                                                 nullptr, wxPoint() ) );
        }
    }
}


void DRC::setTransitions()
{
    Go( &DRC::ShowDRCDialog,              PCB_ACTIONS::runDRC.MakeEvent() );
}



