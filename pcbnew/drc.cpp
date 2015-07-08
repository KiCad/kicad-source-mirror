
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file drc.cpp
 */

#include <fctsys.h>
#include <wxPcbStruct.h>
#include <trigo.h>
#include <base_units.h>
#include <class_board_design_settings.h>

#include <class_module.h>
#include <class_track.h>
#include <class_pad.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <geometry/seg.h>

#include <pcbnew.h>
#include <drc_stuff.h>

#include <dialog_drc.h>
#include <wx/progdlg.h>


void DRC::ShowDialog()
{
    if( !m_ui )
    {
        m_ui = new DIALOG_DRC_CONTROL( this, m_mainWindow );
        updatePointers();

        // copy data retained in this DRC object into the m_ui DrcPanel:

        PutValueInLocalUnits( *m_ui->m_SetTrackMinWidthCtrl,
                              m_pcb->GetDesignSettings().m_TrackMinWidth );
        PutValueInLocalUnits( *m_ui->m_SetViaMinSizeCtrl,
                              m_pcb->GetDesignSettings().m_ViasMinSize );
        PutValueInLocalUnits( *m_ui->m_SetMicroViakMinSizeCtrl,
                              m_pcb->GetDesignSettings().m_MicroViasMinSize );

        m_ui->m_CreateRptCtrl->SetValue( m_doCreateRptFile );
        m_ui->m_RptFilenameCtrl->SetValue( m_rptFilename );
    }
    else
        updatePointers();

    m_ui->Show( true );
}


void DRC::DestroyDialog( int aReason )
{
    if( m_ui )
    {
        if( aReason == wxID_OK )
        {
            // if user clicked OK, save his choices in this DRC object.
            m_doCreateRptFile = m_ui->m_CreateRptCtrl->GetValue();
            m_rptFilename     = m_ui->m_RptFilenameCtrl->GetValue();
        }

        m_ui->Destroy();
        m_ui = 0;
    }
}


DRC::DRC( PCB_EDIT_FRAME* aPcbWindow )
{
    m_mainWindow = aPcbWindow;
    m_pcb = aPcbWindow->GetBoard();
    m_ui  = 0;

    // establish initial values for everything:
    m_doPad2PadTest     = true;     // enable pad to pad clearance tests
    m_doUnconnectedTest = true;     // enable unconnected tests
    m_doZonesTest = true;           // enable zone to items clearance tests
    m_doKeepoutTest = true;         // enable keepout areas to items clearance tests
    m_abortDRC = false;
    m_drcInProgress = false;

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
    // maybe someday look at pointainer.h  <- google for "pointainer.h"
    for( unsigned i = 0; i<m_unconnected.size();  ++i )
        delete m_unconnected[i];
}


int DRC::Drc( TRACK* aRefSegm, TRACK* aList )
{
    updatePointers();

    if( !doTrackDrc( aRefSegm, aList, true ) )
    {
        wxASSERT( m_currentMarker );

        m_mainWindow->SetMsgPanel( m_currentMarker );
        return BAD_DRC;
    }

    if( !doTrackKeepoutDrc( aRefSegm ) )
    {
        wxASSERT( m_currentMarker );

        m_mainWindow->SetMsgPanel( m_currentMarker );
        return BAD_DRC;
    }

    return OK_DRC;
}


int DRC::Drc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    updatePointers();

    if( !doEdgeZoneDrc( aArea, aCornerIndex ) )
    {
        wxASSERT( m_currentMarker );
        m_mainWindow->SetMsgPanel( m_currentMarker );
        return BAD_DRC;
    }

    return OK_DRC;
}


void DRC::RunTests( wxTextCtrl* aMessages )
{
    // Ensure ratsnest is up to date:
    if( (m_pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
    {
        if( aMessages )
        {
            aMessages->AppendText( _( "Compile ratsnest...\n" ) );
            wxSafeYield();
        }

        m_mainWindow->Compile_Ratsnest( NULL, true );
    }

    // someone should have cleared the two lists before calling this.

    if( !testNetClasses() )
    {
        // testing the netclasses is a special case because if the netclasses
        // do not pass the BOARD_DESIGN_SETTINGS checks, then every member of a net
        // class (a NET) will cause its items such as tracks, vias, and pads
        // to also fail.  So quit after *all* netclass errors have been reported.
        if( aMessages )
            aMessages->AppendText( _( "Aborting\n" ) );

        // update the m_ui listboxes
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

    // test track and via clearances to other tracks, pads, and vias
    if( aMessages )
    {
        aMessages->AppendText( _( "Track clearances...\n" ) );
        wxSafeYield();
    }

    testTracks( aMessages ? aMessages->GetParent() : m_mainWindow, true );

    // Before testing segments and unconnected, refill all zones:
    // this is a good caution, because filled areas can be outdated.
    if( aMessages )
    {
        aMessages->AppendText( _( "Fill zones...\n" ) );
        wxSafeYield();
    }

    m_mainWindow->Fill_All_Zones( aMessages ? aMessages->GetParent() : m_mainWindow,
                                  false );

    // test zone clearances to other zones
    if( aMessages )
    {
        aMessages->AppendText( _( "Test zones...\n" ) );
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
        aMessages->AppendText( _( "Test texts...\n" ) );
        wxSafeYield();
    }

    testTexts();

    // update the m_ui listboxes
    updatePointers();

    if( aMessages )
    {
        // no newline on this one because it is last, don't want the window
        // to unnecessarily scroll.
        aMessages->AppendText( _( "Finished" ) );
    }
}


void DRC::ListUnconnectedPads()
{
    testUnconnected();

    // update the m_ui listboxes
    updatePointers();
}


void DRC::updatePointers()
{
    // update my pointers, m_mainWindow is the only unchangeable one
    m_pcb = m_mainWindow->GetBoard();

    if( m_ui )  // Use diag list boxes only in DRC dialog
    {
        m_ui->m_ClearanceListBox->SetList( new DRC_LIST_MARKERS( m_pcb ) );
        m_ui->m_UnconnectedListBox->SetList( new DRC_LIST_UNCONNECTED( &m_unconnected ) );
    }
}


bool DRC::doNetClass( NETCLASSPTR nc, wxString& msg )
{
    bool ret = true;

    const BOARD_DESIGN_SETTINGS& g = m_pcb->GetDesignSettings();

#define FmtVal( x ) GetChars( StringFromValue( g_UserUnit, x ) )

#if 0   // set to 1 when (if...) BOARD_DESIGN_SETTINGS has a m_MinClearance value
    if( nc->GetClearance() < g.m_MinClearance )
    {
        msg.Printf( _( "NETCLASS: '%s' has Clearance:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetClearance() ),
                    FmtVal( g.m_TrackClearance )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_CLEARANCE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }
#endif

    if( nc->GetTrackWidth() < g.m_TrackMinWidth )
    {
        msg.Printf( _( "NETCLASS: '%s' has TrackWidth:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetTrackWidth() ),
                    FmtVal( g.m_TrackMinWidth )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_TRACKWIDTH, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetViaDiameter() < g.m_ViasMinSize )
    {
        msg.Printf( _( "NETCLASS: '%s' has Via Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDiameter() ),
                    FmtVal( g.m_ViasMinSize )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_VIASIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetViaDrill() < g.m_ViasMinDrill )
    {
        msg.Printf( _( "NETCLASS: '%s' has Via Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDrill() ),
                    FmtVal( g.m_ViasMinDrill )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_VIADRILLSIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetuViaDiameter() < g.m_MicroViasMinSize )
    {
        msg.Printf( _( "NETCLASS: '%s' has uVia Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDiameter() ),
                    FmtVal( g.m_MicroViasMinSize )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_uVIASIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetuViaDrill() < g.m_MicroViasMinDrill )
    {
        msg.Printf( _( "NETCLASS: '%s' has uVia Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDrill() ),
                    FmtVal( g.m_MicroViasMinDrill )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_uVIADRILLSIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
        m_currentMarker = 0;
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

    // find the max size of the pads (used to stop the test)
    int max_size = 0;

    for( unsigned i = 0; i < sortedPads.size(); ++i )
    {
        D_PAD* pad = sortedPads[i];

        // GetBoundingRadius() is the radius of the minimum sized circle fully containing the pad
        int radius = pad->GetBoundingRadius();
        if( radius > max_size )
            max_size = radius;
    }

    // Test the pads
    D_PAD** listEnd = &sortedPads[ sortedPads.size() ];

    for( unsigned i = 0; i< sortedPads.size(); ++i )
    {
        D_PAD* pad = sortedPads[i];

        int    x_limit = max_size + pad->GetClearance() +
                         pad->GetBoundingRadius() + pad->GetPosition().x;

        if( !doPadToPadsDrc( pad, &sortedPads[i], listEnd, x_limit ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }
}


void DRC::testTracks( wxWindow *aActiveWindow, bool aShowProgressBar )
{
    wxProgressDialog * progressDialog = NULL;
    const int delta = 500;  // This is the number of tests between 2 calls to the
                            // progress bar
    int count = 0;
    for( TRACK* segm = m_pcb->m_Track; segm && segm->Next(); segm = segm->Next() )
        count++;

    int deltamax = count/delta;

    if( aShowProgressBar && deltamax > 3 )
    {
        progressDialog = new wxProgressDialog( _( "Track clearances" ), wxEmptyString,
                                               deltamax, aActiveWindow,
                                               wxPD_AUTO_HIDE | wxPD_CAN_ABORT |
                                               wxPD_APP_MODAL | wxPD_ELAPSED_TIME );
        progressDialog->Update( 0, wxEmptyString );
    }

    int ii = 0;
    count = 0;

    for( TRACK* segm = m_pcb->m_Track; segm && segm->Next(); segm = segm->Next() )
    {
        if ( ii++ > delta )
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

        if( !doTrackDrc( segm, segm->Next(), true ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }

    if( progressDialog )
        progressDialog->Destroy();
}


void DRC::testUnconnected()
{
    if( (m_pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
    {
        wxClientDC dc( m_mainWindow->GetCanvas() );
        m_mainWindow->Compile_Ratsnest( &dc, true );
    }

    if( m_pcb->GetRatsnestsCount() == 0 )
        return;

    wxString msg;

    for( unsigned ii = 0; ii < m_pcb->GetRatsnestsCount();  ++ii )
    {
        RATSNEST_ITEM& rat = m_pcb->m_FullRatsnest[ii];

        if( (rat.m_Status & CH_ACTIF) == 0 )
            continue;

        D_PAD*    padStart = rat.m_PadStart;
        D_PAD*    padEnd   = rat.m_PadEnd;

        msg = padStart->GetSelectMenuText() + wxT( " net " ) + padStart->GetNetname();

        DRC_ITEM* uncItem = new DRC_ITEM( DRCE_UNCONNECTED_PADS,
                                          msg,
                                          padEnd->GetSelectMenuText(),
                                          padStart->GetPosition(), padEnd->GetPosition() );

        m_unconnected.push_back( uncItem );
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
    // is stored, and initalized from the file or the zone properpies editor.
    // if it differs from the net name from net code, there is a DRC issue
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* test_area = m_pcb->GetArea( ii );

        if( !test_area->IsOnCopperLayer() )
            continue;

        int netcode = test_area->GetNetCode();

        // a netcode < 0 or > 0 and no pad in net  is a error or strange
        // perhaps a "dead" net, which happens when all pads in this net were removed
        // Remark: a netcode < 0 should not happen (this is more a bug somewhere)
        int pads_in_net = (test_area->GetNetCode() > 0) ?
                            test_area->GetNet()->GetNodesCount() : 1;

        if( ( netcode < 0 ) || pads_in_net == 0 )
        {
            m_currentMarker = fillMarker( test_area,
                                          DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE, m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
            m_currentMarker = NULL;
        }
    }

    // Test copper areas outlines, and create markers when needed
    m_pcb->Test_Drc_Areas_Outlines_To_Areas_Outlines( NULL, true );
}


void DRC::testKeepoutAreas()
{
    // Test keepout areas for vias, tracks and pads inside keepout areas
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* area = m_pcb->GetArea( ii );

        if( !area->GetIsKeepout() )
            continue;

        for( TRACK* segm = m_pcb->m_Track; segm != NULL; segm = segm->Next() )
        {
            if( segm->Type() == PCB_TRACE_T )
            {
                if( ! area->GetDoNotAllowTracks()  )
                    continue;

                if( segm->GetLayer() != area->GetLayer() )
                    continue;

                if( area->Outline()->Distance( segm->GetStart(), segm->GetEnd(),
                                               segm->GetWidth() ) == 0 )
                {
                    m_currentMarker = fillMarker( segm, NULL,
                                                  DRCE_TRACK_INSIDE_KEEPOUT, m_currentMarker );
                    m_pcb->Add( m_currentMarker );
                    m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
                    m_currentMarker = 0;
                }
            }
            else if( segm->Type() == PCB_VIA_T )
            {
                if( ! area->GetDoNotAllowVias()  )
                    continue;

                if( ! ((VIA*)segm)->IsOnLayer( area->GetLayer() ) )
                    continue;

                if( area->Outline()->Distance( segm->GetPosition() ) < segm->GetWidth()/2 )
                {
                    m_currentMarker = fillMarker( segm, NULL,
                                                  DRCE_VIA_INSIDE_KEEPOUT, m_currentMarker );
                    m_pcb->Add( m_currentMarker );
                    m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
                    m_currentMarker = 0;
                }
            }
        }
        // Test pads: TODO
    }
}


void DRC::testTexts()
{
    std::vector<wxPoint> textShape;      // a buffer to store the text shape (set of segments)
    std::vector<D_PAD*> padList = m_pcb->GetPads();

    // Test text areas for vias, tracks and pads inside text areas
    for( BOARD_ITEM* item = m_pcb->m_Drawings; item; item = item->Next() )
    {
        // Drc test only items on copper layers
        if( ! IsCopperLayer( item->GetLayer() ) )
            continue;

        // only texts on copper layers are tested
        if( item->Type() !=  PCB_TEXT_T )
            continue;

        textShape.clear();

        // So far the bounding box makes up the text-area
        TEXTE_PCB* text = (TEXTE_PCB*) item;
        text->TransformTextShapeToSegmentList( textShape );

        if( textShape.size() == 0 )     // Should not happen (empty text?)
            continue;

        for( TRACK* track = m_pcb->m_Track; track != NULL; track = track->Next() )
        {
            if( ! track->IsOnLayer( item->GetLayer() ) )
                    continue;

            // Test the distance between each segment and the current track/via
            int min_dist = ( track->GetWidth() + text->GetThickness() ) /2 +
                           track->GetClearance(NULL);

            if( track->Type() == PCB_TRACE_T )
            {
                SEG segref( track->GetStart(), track->GetEnd() );

                // Error condition: Distance between text segment and track segment is
                // smaller than the clearance of the segment
                for( unsigned jj = 0; jj < textShape.size(); jj += 2 )
                {
                    SEG segtest( textShape[jj], textShape[jj+1] );
                    int dist = segref.Distance( segtest );

                    if( dist < min_dist )
                    {
                        m_currentMarker = fillMarker( track, text,
                                                      DRCE_TRACK_INSIDE_TEXT,
                                                      m_currentMarker );
                        m_pcb->Add( m_currentMarker );
                        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
                        m_currentMarker = NULL;
                        break;
                    }
                }
            }
            else if( track->Type() == PCB_VIA_T )
            {
                // Error condition: Distance between text segment and via is
                // smaller than the clearance of the via
                for( unsigned jj = 0; jj < textShape.size(); jj += 2 )
                {
                    SEG segtest( textShape[jj], textShape[jj+1] );

                    if( segtest.PointCloserThan( track->GetPosition(), min_dist ) )
                    {
                        m_currentMarker = fillMarker( track, text,
                                                      DRCE_VIA_INSIDE_TEXT, m_currentMarker );
                        m_pcb->Add( m_currentMarker );
                        m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
                        m_currentMarker = NULL;
                        break;
                    }
                }
            }
        }

        // Test pads
        for( unsigned ii = 0; ii < padList.size(); ii++ )
        {
            D_PAD* pad = padList[ii];

            if( ! pad->IsOnLayer( item->GetLayer() ) )
                    continue;

            wxPoint shape_pos = pad->ShapePos();

            for( unsigned jj = 0; jj < textShape.size(); jj += 2 )
            {
                SEG segtest( textShape[jj], textShape[jj+1] );
                /* In order to make some calculations more easier or faster,
                 * pads and tracks coordinates will be made relative
                 * to the segment origin
                 */
                wxPoint origin = textShape[jj];  // origin will be the origin of other coordinates
                m_segmEnd = textShape[jj+1] - origin;
                wxPoint delta = m_segmEnd;
                m_segmAngle = 0;

                // for a non horizontal or vertical segment Compute the segment angle
                // in tenths of degrees and its length
                if( delta.x || delta.y )    // delta.x == delta.y == 0 for vias
                {
                    // Compute the segment angle in 0,1 degrees
                    m_segmAngle = ArcTangente( delta.y, delta.x );

                    // Compute the segment length: we build an equivalent rotated segment,
                    // this segment is horizontal, therefore dx = length
                    RotatePoint( &delta, m_segmAngle );    // delta.x = length, delta.y = 0
                }

                m_segmLength = delta.x;
                m_padToTestPos = shape_pos - origin;

                if( !checkClearanceSegmToPad( pad, text->GetThickness(),
                                              pad->GetClearance(NULL) ) )
                {
                    m_currentMarker = fillMarker( pad, text,
                                                  DRCE_PAD_INSIDE_TEXT, m_currentMarker );
                    m_pcb->Add( m_currentMarker );
                    m_mainWindow->GetGalCanvas()->GetView()->Add( m_currentMarker );
                    m_currentMarker = NULL;
                    break;
                }
            }
        }
    }
}


bool DRC::doTrackKeepoutDrc( TRACK* aRefSeg )
{
    // Test keepout areas for vias, tracks and pads inside keepout areas
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* area = m_pcb->GetArea( ii );

        if( !area->GetIsKeepout() )
            continue;

        if( aRefSeg->Type() == PCB_TRACE_T )
        {
            if( ! area->GetDoNotAllowTracks()  )
                continue;

            if( aRefSeg->GetLayer() != area->GetLayer() )
                continue;

            if( area->Outline()->Distance( aRefSeg->GetStart(), aRefSeg->GetEnd(),
                                           aRefSeg->GetWidth() ) == 0 )
            {
                m_currentMarker = fillMarker( aRefSeg, NULL,
                                              DRCE_TRACK_INSIDE_KEEPOUT, m_currentMarker );
                return false;
            }
        }
        else if( aRefSeg->Type() == PCB_VIA_T )
        {
            if( ! area->GetDoNotAllowVias()  )
                continue;

            if( ! ((VIA*)aRefSeg)->IsOnLayer( area->GetLayer() ) )
                continue;

            if( area->Outline()->Distance( aRefSeg->GetPosition() ) < aRefSeg->GetWidth()/2 )
            {
                m_currentMarker = fillMarker( aRefSeg, NULL,
                                              DRCE_VIA_INSIDE_KEEPOUT, m_currentMarker );
                return false;
            }
        }
    }

    return true;
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

        // No problem if pads are on different copper layers,
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->GetLayerSet() & layerMask ) == 0 )
        {
            // if holes are in the same location and have the same size and shape,
            // this can be accepted
            if( pad->GetPosition() == aRefPad->GetPosition()
                && pad->GetDrillSize() == aRefPad->GetDrillSize()
                && pad->GetDrillShape() == aRefPad->GetDrillShape() )
            {
                if( aRefPad->GetDrillShape() == PAD_DRILL_CIRCLE )
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
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_OBLONG ?
                                                           PAD_OVAL : PAD_CIRCLE );
                dummypad.SetOrientation( pad->GetOrientation() );

                if( !checkClearancePadToPad( aRefPad, &dummypad ) )
                {
                    // here we have a drc error on pad!
                    m_currentMarker = fillMarker( pad, aRefPad,
                                                  DRCE_HOLE_NEAR_PAD, m_currentMarker );
                    return false;
                }
            }

            if( aRefPad->GetDrillSize().x ) // pad reference has a hole
            {
                dummypad.SetPosition( aRefPad->GetPosition() );
                dummypad.SetSize( aRefPad->GetDrillSize() );
                dummypad.SetShape( aRefPad->GetDrillShape() == PAD_DRILL_OBLONG ?
                                                               PAD_OVAL : PAD_CIRCLE );
                dummypad.SetOrientation( aRefPad->GetOrientation() );

                if( !checkClearancePadToPad( pad, &dummypad ) )
                {
                    // here we have a drc error on aRefPad!
                    m_currentMarker = fillMarker( aRefPad, pad,
                                                  DRCE_HOLE_NEAR_PAD, m_currentMarker );
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

        if( !checkClearancePadToPad( aRefPad, pad ) )
        {
            // here we have a drc error!
            m_currentMarker = fillMarker( aRefPad, pad, DRCE_PAD_NEAR_PAD1, m_currentMarker );
            return false;
        }
    }

    return true;
}
