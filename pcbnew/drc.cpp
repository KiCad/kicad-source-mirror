
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2007 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

/****************************/
/* DRC control              */
/****************************/

#include "fctsys.h"
#include "wxPcbStruct.h"
#include "trigo.h"
#include "class_board_design_settings.h"

#include "class_module.h"
#include "class_track.h"
#include "class_pad.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "protos.h"
#include "drc_stuff.h"

#include "dialog_drc.h"


void DRC::ShowDialog()
{
    if( !m_ui )
    {
        m_ui = new DIALOG_DRC_CONTROL( this, m_mainWindow );
        updatePointers();

        // copy data retained in this DRC object into the m_ui DrcPanel:

#ifdef KICAD_NANOMETRE
        LengthToTextCtrl( *m_ui->m_SetTrackMinWidthCtrl,
                          m_pcb->GetBoardDesignSettings()->m_TrackMinWidth );
        LengthToTextCtrl( *m_ui->m_SetViaMinSizeCtrl,
                          m_pcb->GetBoardDesignSettings()->m_ViasMinSize );
        LengthToTextCtrl( *m_ui->m_SetMicroViakMinSizeCtrl,
                          m_pcb->GetBoardDesignSettings()->m_MicroViasMinSize );
#else
        PutValueInLocalUnits( *m_ui->m_SetTrackMinWidthCtrl,
                              TO_LEGACY_LU( m_pcb->GetBoardDesignSettings()->m_TrackMinWidth ),
                              m_mainWindow->m_InternalUnits );
        PutValueInLocalUnits( *m_ui->m_SetViaMinSizeCtrl,
                              TO_LEGACY_LU( m_pcb->GetBoardDesignSettings()->m_ViasMinSize ),
                              m_mainWindow->m_InternalUnits );
        PutValueInLocalUnits( *m_ui->m_SetMicroViakMinSizeCtrl,
                              TO_LEGACY_LU( m_pcb->GetBoardDesignSettings()->m_MicroViasMinSize ),
                              m_mainWindow->m_InternalUnits );
#endif

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

        m_currentMarker->DisplayInfo( m_mainWindow );
        return BAD_DRC;
    }

    return OK_DRC;
}


/**
 * Function Drc
 * tests the outline segment starting at CornerIndex and returns the result and displays the error
 * in the status panel only if one exists.
 *      Test Edge inside other areas
 *      Test Edge too close other areas
 * @param aArea The areaparent which contains the corner.
 * @param aCornerIndex The starting point of the segment to test.
 * @return int - BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
 */
int DRC::Drc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    updatePointers();

    if( !doEdgeZoneDrc( aArea, aCornerIndex ) )
    {
        wxASSERT( m_currentMarker );
        m_currentMarker->DisplayInfo( m_mainWindow );
        return BAD_DRC;
    }

    return OK_DRC;
}


/**
 * Function RunTests
 * will actually run all the tests specified with a previous call to
 * SetSettings()
 */
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
        // do not pass the g_DesignSettings checks, then every member of a net
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
    testTracks();

    // Before testing segments and unconnected, refill all zones:
    // this is a good caution, because filled areas can be outdated.
    if( aMessages )
    {
        aMessages->AppendText( _( "Fill zones...\n" ) );
        wxSafeYield();
    }

    m_mainWindow->Fill_All_Zones( false );
    wxSafeYield();

    // test zone clearances to other zones, pads, tracks, and vias
    if( aMessages && m_doZonesTest )
    {
        aMessages->AppendText( _( "Test zones...\n" ) );
        wxSafeYield();
    }

    testZones( m_doZonesTest );

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
    // update my pointers, m_mainWindow is the only unchangable one
    m_pcb = m_mainWindow->GetBoard();

    if( m_ui )  // Use diag list boxes only in DRC dialog
    {
        m_ui->m_ClearanceListBox->SetList( new DRC_LIST_MARKERS( m_pcb ) );
        m_ui->m_UnconnectedListBox->SetList( new DRC_LIST_UNCONNECTED( &m_unconnected ) );
    }
}


bool DRC::doNetClass( NETCLASS* nc, wxString& msg )
{
    bool ret = true;

    const BOARD_DESIGN_SETTINGS& g = *m_pcb->GetBoardDesignSettings();

#define FmtVal( x ) GetChars( ReturnStringFromValue( g_UserUnit, x, PCB_INTERNAL_UNIT ) )

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
        m_currentMarker = 0;
        ret = false;
    }
#endif

    if( nc->GetTrackWidth() < TO_LEGACY_LU( g.m_TrackMinWidth ) )
    {
        msg.Printf( _( "NETCLASS: '%s' has TrackWidth:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetTrackWidth() ),
                    FmtVal( TO_LEGACY_LU( g.m_TrackMinWidth ) )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_TRACKWIDTH, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetViaDiameter() < TO_LEGACY_LU( g.m_ViasMinSize ) )
    {
        msg.Printf( _( "NETCLASS: '%s' has Via Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDiameter() ),
                    FmtVal( TO_LEGACY_LU( g.m_ViasMinSize ) )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_VIASIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetViaDrill() < TO_LEGACY_LU( g.m_ViasMinDrill ) )
    {
        msg.Printf( _( "NETCLASS: '%s' has Via Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetViaDrill() ),
                    FmtVal( TO_LEGACY_LU( g.m_ViasMinDrill ) )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_VIADRILLSIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetuViaDiameter() < TO_LEGACY_LU( g.m_MicroViasMinSize ) )
    {
        msg.Printf( _( "NETCLASS: '%s' has uVia Dia:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDiameter() ),
                    FmtVal( TO_LEGACY_LU( g.m_MicroViasMinSize ) )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_uVIASIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    if( nc->GetuViaDrill() < TO_LEGACY_LU( g.m_MicroViasMinDrill ) )
    {
        msg.Printf( _( "NETCLASS: '%s' has uVia Drill:%s which is less than global:%s" ),
                    GetChars( nc->GetName() ),
                    FmtVal( nc->GetuViaDrill() ),
                    FmtVal( TO_LEGACY_LU( g.m_MicroViasMinDrill ) )
                    );

        m_currentMarker = fillMarker( DRCE_NETCLASS_uVIADRILLSIZE, msg, m_currentMarker );
        m_pcb->Add( m_currentMarker );
        m_currentMarker = 0;
        ret = false;
    }

    return ret;
}


bool DRC::testNetClasses()
{
    bool        ret = true;

    NETCLASSES& netclasses = m_pcb->m_NetClasses;

    wxString    msg;   // construct this only once here, not in a loop, since somewhat expensive.

    if( !doNetClass( netclasses.GetDefault(), msg ) )
        ret = false;

    for( NETCLASSES::const_iterator i = netclasses.begin();  i != netclasses.end();  ++i )
    {
        NETCLASS* nc = i->second;

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

        // m_ShapeMaxRadius is the radius value of the circle containing the pad
        if( pad->m_ShapeMaxRadius > max_size )
            max_size = pad->m_ShapeMaxRadius;
    }

    // Test the pads
    D_PAD** listEnd = &sortedPads[ sortedPads.size() ];

    for( unsigned i = 0; i< sortedPads.size(); ++i )
    {
        D_PAD* pad = sortedPads[i];

        int    x_limit = max_size + pad->GetClearance() +
                         pad->m_ShapeMaxRadius + pad->GetPosition().x;

        if( !doPadToPadsDrc( pad, &sortedPads[i], listEnd, x_limit ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }
}


void DRC::testTracks()
{
    for( TRACK* segm = m_pcb->m_Track;  segm && segm->Next();  segm = segm->Next() )
    {
        if( !doTrackDrc( segm, segm->Next(), true ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }
}


void DRC::testUnconnected()
{
    if( (m_pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
    {
        wxClientDC dc( m_mainWindow->DrawPanel );
        m_mainWindow->Compile_Ratsnest( &dc, true );
    }

    if( m_pcb->GetRatsnestsCount() == 0 )
        return;

    for( unsigned ii = 0; ii < m_pcb->GetRatsnestsCount();  ++ii )
    {
        RATSNEST_ITEM* rat = &m_pcb->m_FullRatsnest[ii];

        if( (rat->m_Status & CH_ACTIF) == 0 )
            continue;

        D_PAD*    padStart = rat->m_PadStart;
        D_PAD*    padEnd   = rat->m_PadEnd;

        DRC_ITEM* uncItem = new DRC_ITEM( DRCE_UNCONNECTED_PADS,
                                          padStart->GetSelectMenuText(),
                                          padEnd->GetSelectMenuText(),
                                          padStart->GetPosition(), padEnd->GetPosition() );

        m_unconnected.push_back( uncItem );
    }
}


void DRC::testZones( bool adoTestFillSegments )
{
    // Test copper areas for valid netcodes
    // if a netcode is < 0 the netname was not found when reading a netlist
    // if a netcode is == 0 the netname is void, and the zone is not connected.
    // This is allowed, but i am not sure this is a good idea
    for( int ii = 0; ii < m_pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* Area_To_Test = m_pcb->GetArea( ii );

        if( !Area_To_Test->IsOnCopperLayer() )
            continue;

        if( Area_To_Test->GetNet() < 0 )
        {
            m_currentMarker = fillMarker( Area_To_Test,
                                          DRCE_NON_EXISTANT_NET_FOR_ZONE_OUTLINE, m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }

    // Test copper areas outlines, and create markers when needed
    m_pcb->Test_Drc_Areas_Outlines_To_Areas_Outlines( NULL, true );

    TRACK* zoneSeg;

    if( !adoTestFillSegments )
        return;

    // m_pcb->m_Zone is fully obsolete. Keep this test for compatibility
    // with old designs. Will be removed on day
    for( zoneSeg = m_pcb->m_Zone;  zoneSeg && zoneSeg->Next(); zoneSeg = zoneSeg->Next() )
    {
        // Test zoneSeg with other zone segments and with all pads
        if( !doTrackDrc( zoneSeg, zoneSeg->Next(), true ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;
        }

        // Pads already tested: disable pad test

        bool rc = doTrackDrc( zoneSeg, m_pcb->m_Track, false );

        if( !rc )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;
        }
    }
}


bool DRC::doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit )
{
    int layerMask = aRefPad->m_layerMask & ALL_CU_LAYERS;

    /* used to test DRC pad to holes: this dummy pad has the size and shape of the hole
     * to test pad to pad hole DRC, using the pad to pad DRC test function.
     * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE dummymodule( m_pcb );    // Creates a dummy parent
    D_PAD dummypad( &dummymodule );
    dummypad.m_layerMask   |= ALL_CU_LAYERS;  // Ensure the hole is on all copper layers
    dummypad.m_LocalClearance = 1;   /* Use the minimal local clearance value for the dummy pad
                                      *  the clearance of the active pad will be used
                                      *  as minimum distance to a hole
                                      *  (a value = 0 means use netclass value)
                                      */

    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;

        // We can stop the test when pad->m_Pos.x > x_limit
        // because the list is sorted by X values
        if( pad->m_Pos.x > x_limit )
            break;

        // No problem if pads are on different copper layers,
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->m_layerMask & layerMask ) == 0 )
        {
            // if holes are in the same location and have the same size and shape,
            // this can be accepted
            if( pad->GetPosition() == aRefPad->GetPosition()
                && pad->m_Drill == aRefPad->m_Drill
                && pad->m_DrillShape == aRefPad->m_DrillShape )
            {
                if( aRefPad->m_DrillShape == PAD_CIRCLE )
                    continue;

                // for oval holes: must also have the same orientation
                if( pad->m_Orient == aRefPad->m_Orient )
                    continue;
            }

            /* Here, we must test clearance between holes and pads
             * dummy pad size and shape is adjusted to pad drill size and shape
             */
            if( pad->m_Drill.x )
            {
                // pad under testing has a hole, test this hole against pad reference
                dummypad.SetPosition( pad->GetPosition() );
                dummypad.m_Size     = pad->m_Drill;
                dummypad.m_PadShape = (pad->m_DrillShape == PAD_OVAL) ? PAD_OVAL : PAD_CIRCLE;
                dummypad.m_Orient   = pad->m_Orient;

                // compute the radius of the circle containing this pad
                dummypad.ComputeShapeMaxRadius();

                if( !checkClearancePadToPad( aRefPad, &dummypad ) )
                {
                    // here we have a drc error on pad!
                    m_currentMarker = fillMarker( pad, aRefPad,
                                                  DRCE_HOLE_NEAR_PAD, m_currentMarker );
                    return false;
                }
            }

            if( aRefPad->m_Drill.x ) // pad reference has a hole
            {
                dummypad.SetPosition( aRefPad->GetPosition() );
                dummypad.m_Size     = aRefPad->m_Drill;
                dummypad.m_PadShape = (aRefPad->m_DrillShape == PAD_OVAL) ? PAD_OVAL : PAD_CIRCLE;
                dummypad.m_Orient   = aRefPad->m_Orient;

                // compute the radius of the circle containing this pad
                dummypad.ComputeShapeMaxRadius();

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
        if( pad->GetNet() && ( aRefPad->GetNet() == pad->GetNet() ) )
            continue;

        // if pads are from the same footprint
        if( pad->GetParent() == aRefPad->GetParent() )
        {
            // and have the same pad number ( equivalent pads  )

            // one can argue that this 2nd test is not necessary, that any
            // two pads from a single module are acceptable.  This 2nd test
            // should eventually be a configuration option.
            if( pad->m_NumPadName == aRefPad->m_NumPadName )
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
