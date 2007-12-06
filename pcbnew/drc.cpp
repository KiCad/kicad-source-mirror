
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2004-2007 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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
/* DRC control				*/
/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "protos.h"

#include "drc_stuff.h"

#include "dialog_drc.cpp"


/******************************************************/
void WinEDA_PcbFrame::Install_Test_DRC_Frame( wxDC* DC )
/******************************************************/
{
    m_drc->ShowDialog();
}


void DRC::ShowDialog()
{
    if( !m_ui )
    {
        m_ui = new DrcDialog( this, m_mainWindow );
        updatePointers();
        
        // copy data retained in this DRC object into the m_ui DrcPanel:
        
        PutValueInLocalUnits( *m_ui->m_SetClearance, g_DesignSettings.m_TrackClearence,
                             m_mainWindow->m_InternalUnits );;

        m_ui->m_Pad2PadTestCtrl->SetValue( m_doPad2PadTest );
        m_ui->m_ZonesTestCtrl->SetValue( m_doZonesTest );
        m_ui->m_UnconnectedTestCtrl->SetValue( m_doUnconnectedTest );
        
        m_ui->m_CreateRptCtrl->SetValue( m_doCreateRptFile );
        m_ui->m_RptFilenameCtrl->SetValue( m_rptFilename );
    }
    else
        updatePointers();
        
    m_ui->Show(true);
}


void DRC::DestroyDialog( int aReason )
{
    if( m_ui )
    {
        if( aReason == wxID_OK )
        {
            // if user clicked OK, save his choices in this DRC object. 
            m_doCreateRptFile   = m_ui->m_CreateRptCtrl->GetValue();
            m_doPad2PadTest     = m_ui->m_Pad2PadTestCtrl->GetValue();
            m_doZonesTest       = m_ui->m_ZonesTestCtrl->GetValue();
            m_doUnconnectedTest = m_ui->m_UnconnectedTestCtrl->GetValue();
            m_rptFilename       = m_ui->m_RptFilenameCtrl->GetValue();
        }
        
        m_ui->Destroy();
        m_ui = 0;
    }
}


DRC::DRC( WinEDA_PcbFrame* aPcbWindow )
{
    m_mainWindow = aPcbWindow;
    m_drawPanel  = aPcbWindow->DrawPanel;
    m_pcb        = aPcbWindow->m_Pcb;
    m_ui         = 0;

    // establish initial values for everything:    
    m_doPad2PadTest     = true;
    m_doUnconnectedTest = true;
    m_doZonesTest       = false;
    
    m_doCreateRptFile   = false;

    // m_rptFilename set to empty by its constructor

    m_currentMarker = 0;
    
    m_spotcx = 0;
    m_spotcy = 0;
    m_finx = 0;
    m_finy = 0;
    
    m_segmAngle = 0;
    m_segmLength = 0;
    
    m_xcliplo = 0;
    m_ycliplo = 0;
    m_xcliphi = 0;
    m_ycliphi = 0;

    m_drawPanel = 0;
}

DRC::~DRC()
{
    // maybe someday look at pointainer.h  <- google for "pointainer.h" 
    for( unsigned i=0; i<m_unconnected.size();  ++i )
        delete m_unconnected[i];
}

/***********************************************************************/
int DRC::Drc( TRACK* aRefSegm, TRACK* aList )
/***********************************************************************/
{
    updatePointers();
    
    if( !doTrackDrc( aRefSegm, aList ) )
    {
        wxASSERT( m_currentMarker );
        
        m_currentMarker->Display_Infos( m_mainWindow );
        return BAD_DRC;
    }
    
    return OK_DRC;
}


void DRC::RunTests()
{
    // someone should have cleared the two lists before calling this.
    
    // test pad to pad clearances, nothing to do with tracks, vias or zones.
    if( m_doPad2PadTest )
        testPad2Pad();

    // test track and via clearances to other tracks, pads, and vias
    testTracks();

    // test zone clearances to other zones, pads, tracks, and vias    
    if( m_doZonesTest )
        testZones();

    // find and gather unconnected pads.    
    if( m_doUnconnectedTest )
        testUnconnected();
    
    // update the m_ui listboxes
    updatePointers();
}



/***************************************************************/
void DRC::ListUnconnectedPads()
/***************************************************************/
{
    testUnconnected();
    
    // update the m_ui listboxes
    updatePointers();
}


void DRC::updatePointers()
{
    // update my pointers, m_mainWindow is the only unchangable one
    m_drawPanel = m_mainWindow->DrawPanel;
    m_pcb = m_mainWindow->m_Pcb;
    
	if ( m_ui ) // Use diag list boxes only in DRC dialog
	{
		m_ui->m_ClearanceListBox->SetList( new DRC_LIST_MARKERS( m_pcb ) );

		m_ui->m_UnconnectedListBox->SetList( new DRC_LIST_UNCONNECTED( &m_unconnected ) );
	}
}
    

void DRC::testTracks()
{
    for( TRACK* segm = m_pcb->m_Track;  segm && segm->Next();  segm=segm->Next() )
    {
        if( !doTrackDrc( segm, segm->Next() ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;            
        }
    }
}


void DRC::testPad2Pad()
{
    LISTE_PAD* pad_list_start = CreateSortedPadListByXCoord( m_pcb );
    LISTE_PAD* pad_list_limit = &pad_list_start[m_pcb->m_NbPads];
    LISTE_PAD* ppad;

    // find the max size of the pads (used to stop the test)
    int  max_size = 0;
    for( ppad = pad_list_start;  ppad<pad_list_limit;  ppad++ )
    {
        D_PAD* pad = *ppad;
        if( pad->m_Rayon > max_size )
            max_size = pad->m_Rayon;
    }

    // Test the pads
    for( ppad = pad_list_start;  ppad<pad_list_limit;  ppad++ )
    {
        D_PAD* pad = *ppad;
       
        if( !doPadToPadsDrc( pad, ppad, pad_list_limit, max_size ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;            
        }
    }

    free( pad_list_start );
}


void DRC::testUnconnected()
{
    if( (m_pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
    {
        wxClientDC dc( m_mainWindow->DrawPanel );
        m_mainWindow->Compile_Ratsnest( &dc, TRUE );
    }
    
    if( m_pcb->m_Ratsnest == NULL )
        return;

    CHEVELU* rat = m_pcb->m_Ratsnest;
    for( int i=0;  i<m_pcb->GetNumRatsnests();  ++i, ++rat )
    {
        if( (rat->status & CH_ACTIF) == 0 )
            continue;

        D_PAD*  padStart = rat->pad_start;
        D_PAD*  padEnd   = rat->pad_end;

        DRC_ITEM* uncItem = new DRC_ITEM( DRCE_UNCONNECTED_PADS, padStart->GetPosition(), 
                             padStart->MenuText(m_pcb), padEnd->MenuText(m_pcb),
                             padStart->GetPosition(),   padEnd->GetPosition() );
        
        m_unconnected.push_back( uncItem );
    }
}


void DRC::testZones()
{
    TRACK* zoneSeg;

    /* this was for display purposes, don't know that we need it anymore    
    m_pcb->m_NbSegmZone = 0;
    for( zoneSeg = m_pcb->m_Zone;   zoneSeg;   zoneSeg = zoneSeg->Next() )
        ++m_pcb->m_NbSegmZone;
    */

    for( zoneSeg = m_pcb->m_Zone;  zoneSeg && zoneSeg->Next();  zoneSeg=zoneSeg->Next() )
    {
        // Test zoneSeg with other zone segments and with all pads
        if( !doTrackDrc( zoneSeg, zoneSeg->Next() ) )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;            
        }

        // Test zoneSeg with all track segments
        int tmp = m_pcb->m_NbPads;
        
        m_pcb->m_NbPads = 0;    // Pads already tested: disable pad test
        bool rc = doTrackDrc( zoneSeg, m_pcb->m_Track );
        m_pcb->m_NbPads = tmp;

        if( !rc )
        {
            wxASSERT( m_currentMarker );
            m_pcb->Add( m_currentMarker );
            m_currentMarker = 0;            
        }
    }
}


MARKER* DRC::fillMarker( TRACK* aTrack, BOARD_ITEM* aItem, int aErrorCode, MARKER* fillMe )
{
    wxString    textA = aTrack->MenuText( m_pcb );
    wxString    textB;

    wxPoint     position;
    wxPoint     posB;
    
    if( aItem )     // aItem might be NULL
    {
        textB = aItem->MenuText( m_pcb );
        posB  = aItem->GetPosition();
        
        if( aItem->Type() == TYPEPAD )
            position = aItem->GetPosition();
        
        else if( aItem->Type() == TYPEVIA )
            position = aItem->GetPosition();
        
        else if( aItem->Type() == TYPETRACK )
        {
            TRACK*  track  = (TRACK*) aItem;
            wxPoint endPos = track->m_End;
            
            // either of aItem's start or end will be used for the marker position
            // first assume start, then switch at end if needed.  decision made on
            // distance from end of aTrack.
            position = track->m_Start;
    
            double dToEnd   = hypot( endPos.x   - aTrack->m_End.x, 
                                     endPos.y   - aTrack->m_End.y );
            double dToStart = hypot( position.x - aTrack->m_End.x,
                                     position.y - aTrack->m_End.y );
    
            if( dToEnd < dToStart )
                position = endPos;
        }
    }
    else
        position = aTrack->GetPosition(); 

    
    if( fillMe )
        fillMe->SetData( aErrorCode, position, 
                            textA, aTrack->GetPosition(), 
                            textB, posB );
    else
        fillMe = new MARKER( aErrorCode, position, 
                            textA, aTrack->GetPosition(), 
                            textB, posB );
    
    return fillMe;
}


MARKER* DRC::fillMarker( D_PAD* aPad, D_PAD* bPad, int aErrorCode, MARKER* fillMe )
{
    wxString    textA = aPad->MenuText( m_pcb );
    wxString    textB = bPad->MenuText( m_pcb );

    wxPoint posA = aPad->GetPosition();
    wxPoint posB = bPad->GetPosition();

    if( fillMe )
        fillMe->SetData( aErrorCode, posA, textA, posA, textB, posB );
    else
        fillMe = new MARKER( aErrorCode, posA, textA, posA, textB, posB );
    
    return fillMe;
}


/***********************************************************************/
bool DRC::doTrackDrc( TRACK* aRefSeg, TRACK* aStart )
/***********************************************************************/
{
    TRACK*  track;
    int     dx, dy;         // utilise pour calcul des dim x et dim y des segments
    int     w_dist;
    int     layerMask;
    int     net_code_ref;
    int     org_X, org_Y;   // Origine sur le PCB des axes du repere centre sur
                            //	l'origine du segment de reference
    wxPoint shape_pos;

    org_X        = aRefSeg->m_Start.x;
    org_Y        = aRefSeg->m_Start.y;

    m_finx = dx = aRefSeg->m_End.x - org_X;
    m_finy = dy = aRefSeg->m_End.y - org_Y;

    layerMask    = aRefSeg->ReturnMaskLayer();
    net_code_ref = aRefSeg->GetNet();

    m_segmAngle = 0;


    // @todo: is this necessary?
    /**************************************************************/
    /* Phase 0 : test if via's hole is bigger than its diameter : */
    /**************************************************************/
    
    if( aRefSeg->Type() == TYPEVIA )
    {
        // This test seems necessary since the dialog box that displays the
        // desired via hole size and width does not enforce a hole size smaller
        // than the via's diameter.
        
        if( aRefSeg->m_Drill > aRefSeg->m_Width )
        {
            m_currentMarker = fillMarker( aRefSeg, NULL, 
                                DRCE_VIA_HOLE_BIGGER, m_currentMarker );
            return false;
        }
    }
    
	// for a non horizontal or vertical segment Compute the segment angle
	// in tenths of degrees and its length
    if( dx || dy )
    {
        // Compute the segment angle in 0,1 degrees
        m_segmAngle = ArcTangente( dy, dx );

        // Compute the segment length: we build an equivalent rotated segment,
		// this segment is horizontal, therefore dx = length
        RotatePoint( &dx, &dy, m_segmAngle );    // dx = length, dy = 0 
    }

    m_segmLength = dx;

    /******************************************/
    /* Phase 1 : test DRC track to pads :     */
    /******************************************/
    
    D_PAD pseudo_pad( (MODULE*) NULL );     // construct this once outside following loop
    
    // Compute the min distance to pads
    w_dist = aRefSeg->m_Width >> 1;
    for( int ii=0;  ii<m_pcb->m_NbPads;  ++ii )
    {
        D_PAD* pad = m_pcb->m_Pads[ii];

        /* No problem if pads are on an other layer,
         * But if a drill hole exists	(a pad on a single layer can have a hole!)
		 * we must test the hole
		 */
        if( (pad->m_Masque_Layer & layerMask ) == 0 )
        {
            /* We must test the pad hole. In order to use the function "checkClearanceSegmToPad",
             * a pseudo pad is used, with a shape and a size like the hole 
             */
            if( pad->m_Drill.x == 0 )
                continue;

            pseudo_pad.m_Size     = pad->m_Drill;
            pseudo_pad.SetPosition( pad->GetPosition() );
            pseudo_pad.m_PadShape = pad->m_DrillShape;
            pseudo_pad.m_Orient   = pad->m_Orient;
            pseudo_pad.ComputeRayon();      // compute the radius
            
            m_spotcx = pseudo_pad.GetPosition().x - org_X;
            m_spotcy = pseudo_pad.GetPosition().y - org_Y;
            
            if( !checkClearanceSegmToPad( &pseudo_pad, w_dist,
                                        g_DesignSettings.m_TrackClearence ) )
            {
                m_currentMarker = fillMarker( aRefSeg, pad, 
                                    DRCE_TRACK_NEAR_THROUGH_HOLE, m_currentMarker );
                return false;
            }
            continue;
        }

        /* The pad must be in a net (i.e pt_pad->GetNet() != 0 )
         * but no problem if the pad netcode is the current netcode (same net) 
         */
        if( pad->GetNet() &&	// the pad must be connected
			net_code_ref == pad->GetNet() )	// the pad net is the same as current net -> Ok
            continue;

        // DRC for the pad
        shape_pos = pad->ReturnShapePos();
        m_spotcx   = shape_pos.x - org_X;
        m_spotcy   = shape_pos.y - org_Y;
        if( !checkClearanceSegmToPad( pad, w_dist, g_DesignSettings.m_TrackClearence ) )
        {
            m_currentMarker = fillMarker( aRefSeg, pad, 
                                DRCE_TRACK_NEAR_PAD, m_currentMarker );
            return false;
        }
    }

    /***********************************************/
    /* Phase 2: test DRC with other track segments */
    /***********************************************/

    // At this point the reference segment is the X axis

    // Test the reference segment with other track segments
    for( track=aStart;  track;  track=track->Next() )
    {
        // coord des extremites du segment teste dans le repere modifie
        int x0;
        int y0;
        int xf;
        int yf; 
        
        // No problem if segments have the same net code:
        if( net_code_ref == track->GetNet() )
            continue;

        // No problem if segment are on different layers :
        if( ( layerMask & track->ReturnMaskLayer() ) == 0 )
            continue;

        // the minimum distance = clearance plus half the reference track 
        // width plus half the other track's width
        w_dist  = aRefSeg->m_Width >> 1;
        w_dist += track->m_Width >> 1;
        w_dist += g_DesignSettings.m_TrackClearence;

        // If the reference segment is a via, we test it here
        if( aRefSeg->Type() == TYPEVIA )
        {
            int orgx, orgy; // origine du repere d'axe X = segment a comparer
            int angle = 0;  // angle du segment a tester;
            
            orgx = track->m_Start.x; 
            orgy = track->m_Start.y;
            
            dx = track->m_End.x - orgx; 
            dy = track->m_End.y - orgy;
            
            x0 = aRefSeg->m_Start.x - orgx; 
            y0 = aRefSeg->m_Start.y - orgy;

            if( track->Type() == TYPEVIA )
            {
                // Test distance between two vias                
                if( (int) hypot( x0, y0 ) < w_dist )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_VIA_NEAR_VIA, m_currentMarker );
                    return false;
                }
            }
            else    // test via to segment
            {
                // Compute l'angle
                angle = ArcTangente( dy, dx );

                // Compute new coordinates ( the segment become horizontal)
                RotatePoint( &dx, &dy, angle );
                RotatePoint( &x0, &y0, angle );

                if( !checkMarginToCircle( x0, y0, w_dist, dx ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_VIA_NEAR_TRACK, m_currentMarker );
                    return false;
                }
            }
            continue;
        }

        /* We compute x0,y0, xf,yf = starting and ending point coordinates for the segment to test
         * in the new axis : the new X axis is the reference segment
		 * We must translate and rotate the segment to test
		*/
        x0 = track->m_Start.x - org_X;
        y0 = track->m_Start.y - org_Y;

        xf = track->m_End.x - org_X;
        yf = track->m_End.y - org_Y;

        RotatePoint( &x0, &y0, m_segmAngle );
        RotatePoint( &xf, &yf, m_segmAngle );

        if( track->Type() == TYPEVIA )
        {
            if( checkMarginToCircle( x0, y0, w_dist, m_segmLength ) )
                continue;
            
            m_currentMarker = fillMarker( aRefSeg, track, 
                                DRCE_TRACK_NEAR_VIA, m_currentMarker );
            return false;
        }


        /*	We have changed axis:
         *  the reference segment is Horizontal.
         *  3 cases : the segment to test can be parallel, perpendicular or have an other direction
         */
        if( y0 == yf ) // parallel segments
        {
            if( abs( y0 ) >= w_dist )
                continue;

            if( x0 > xf )
                EXCHG( x0, xf );                                /* pour que x0 <= xf */

            if( x0 > (-w_dist) && x0 < (m_segmLength + w_dist) )   /* possible error drc */
            {
                /* Fine test : we consider the rounded shape of the ends */
                if( x0 >= 0 && x0 <= m_segmLength )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_TRACK_ENDS1, m_currentMarker );
                    return false;
                }
                if( !checkMarginToCircle( x0, y0, w_dist, m_segmLength ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_TRACK_ENDS2, m_currentMarker );
                    return false;
                }
            }
            if( xf > (-w_dist) && xf < (m_segmLength + w_dist) )
            {
                /* Fine test : we consider the rounded shape of the ends */
                if( xf >= 0 && xf <= m_segmLength )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_TRACK_ENDS3, m_currentMarker );
                    return false;
                }
                if( !checkMarginToCircle( xf, yf, w_dist, m_segmLength ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_TRACK_ENDS4, m_currentMarker );
                    return false;
                }
            }

            if( x0 <=0 && xf >= 0 )
            {
                m_currentMarker = fillMarker( aRefSeg, track, 
                                    DRCE_TRACK_UNKNOWN1, m_currentMarker );
                return false;
            }
        }
        else if( x0 == xf ) // perpendicular segments
        {
            if( ( x0 <= (-w_dist) ) || ( x0 >= (m_segmLength + w_dist) ) )
                continue;

            // Test if segments are crossing
            if( y0 > yf )
                EXCHG( y0, yf );
            if( (y0 < 0) && (yf > 0) )
            {
                m_currentMarker = fillMarker( aRefSeg, track, 
                                    DRCE_TRACKS_CROSSING, m_currentMarker );
                return false;
            }

            // At this point the drc error is due to an end near a reference segm end
            if( !checkMarginToCircle( x0, y0, w_dist, m_segmLength ) )
            {
                m_currentMarker = fillMarker( aRefSeg, track, 
                                    DRCE_ENDS_PROBLEM1, m_currentMarker );
                return false;
            }
            if( !checkMarginToCircle( xf, yf, w_dist, m_segmLength ) )
            {
                m_currentMarker = fillMarker( aRefSeg, track, 
                                    DRCE_ENDS_PROBLEM2, m_currentMarker );
                return false;
            }
        }
        else // segments quelconques entre eux */
        {
            // calcul de la "surface de securite du segment de reference
            // First rought 'and fast) test : the track segment is like a rectangle
            
            m_xcliplo = m_ycliplo = -w_dist;
            m_xcliphi = m_segmLength + w_dist; 
            m_ycliphi = w_dist;

            // A fine test is needed because a serment is not exactly a 
            // rectangle, it has rounded ends
            if( !checkLine( x0, y0, xf, yf ) )
            {
                /* 2eme passe : the track has rounded ends.
                 * we must a fine test for each rounded end and the 
                 * rectangular zone 
                 */

                m_xcliplo = 0; 
                m_xcliphi = m_segmLength;
                
                if( !checkLine( x0, y0, xf, yf ) )
                {
                    m_currentMarker = fillMarker( aRefSeg, track, 
                                        DRCE_ENDS_PROBLEM3, m_currentMarker );
                    return false;
                }
                else    // The drc error is due to the starting or the ending point of the reference segment
                {
                    // Test the starting and the ending point
                    int angle, rx0, ry0, rxf, ryf;
                    x0 = track->m_Start.x;
                    y0 = track->m_Start.y;

                    xf = track->m_End.x;
                    yf = track->m_End.y;

                    dx = xf - x0;
                    dy = yf - y0;

                    /* Compute the segment orientation (angle) en 0,1 degre */
                    angle = ArcTangente( dy, dx );

                    /* Compute the segment lenght: dx = longueur */
                    RotatePoint( &dx, &dy, angle );

                    /* Comute the reference segment coordinates relatives to a
                     *  X axis = current tested segment 
                     */
                    rx0 = aRefSeg->m_Start.x - x0;
                    ry0 = aRefSeg->m_Start.y - y0;
                    rxf = aRefSeg->m_End.x - x0;
                    ryf = aRefSeg->m_End.y - y0;

                    RotatePoint( &rx0, &ry0, angle );
                    RotatePoint( &rxf, &ryf, angle );
                    if( !checkMarginToCircle( rx0, ry0, w_dist, dx ) )
                    {
                        m_currentMarker = fillMarker( aRefSeg, track, 
                                            DRCE_ENDS_PROBLEM4, m_currentMarker );
                        return false;
                    }
                    if( !checkMarginToCircle( rxf, ryf, w_dist, dx ) )
                    {
                        m_currentMarker = fillMarker( aRefSeg, track, 
                                            DRCE_ENDS_PROBLEM5, m_currentMarker );
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


/*****************************************************************************/
bool DRC::doPadToPadsDrc( D_PAD* aRefPad, LISTE_PAD* aStart, LISTE_PAD* aEnd,
                                  int max_size )
/*****************************************************************************/
{
    int        layerMask = aRefPad->m_Masque_Layer & ALL_CU_LAYERS;
    
    int        x_limite = max_size + g_DesignSettings.m_TrackClearence +
                           aRefPad->m_Rayon + aRefPad->GetPosition().x;

    for(  LISTE_PAD* pad_list=aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;
        if( pad == aRefPad )
            continue;

        /* We can stop the test when pad->m_Pos.x > x_limite
         *  because the list is sorted by X values */
        if( pad->m_Pos.x > x_limite )
            break;

        /* No probleme if pads are on different copper layers */
        if( (pad->m_Masque_Layer & layerMask ) == 0 )
            continue;

        /* The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
         *  But no problem if pads have the same netcode (same net)*/
        if( pad->GetNet() && (aRefPad->GetNet() == pad->GetNet()) )
            continue;

        /* No problem if pads are from the same footprint
         *  and have the same pad number ( equivalent pads  )  */
        if( (pad->m_Parent == aRefPad->m_Parent) && (pad->m_NumPadName == aRefPad->m_NumPadName) )
            continue;

        if( !checkClearancePadToPad( aRefPad, pad, g_DesignSettings.m_TrackClearence ) )
        {
            // here we have a drc error!
            m_currentMarker = fillMarker( aRefPad, pad, 
                                DRCE_PAD_NEAR_PAD1, m_currentMarker );
            return false;
        }
    }

    return true;
}


/**************************************************************************************/
bool DRC::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, const int dist_min )
/***************************************************************************************/
{
    wxPoint rel_pos;
    int     dist;;
    wxPoint shape_pos;
    int     pad_angle;

    rel_pos   = aPad->ReturnShapePos();
    shape_pos = aRefPad->ReturnShapePos();

    // rel_pos is pad position relative to the aRefPad position
    rel_pos.x -= shape_pos.x;
    rel_pos.y -= shape_pos.y;
    
    dist = (int) hypot( rel_pos.x, rel_pos.y );

    bool diag = true;

    /* tst rapide: si les cercles exinscrits sont distants de dist_min au moins,
     *  il n'y a pas de risque: */
    if( (dist - aRefPad->m_Rayon - aPad->m_Rayon) >= dist_min )
        goto exit;

    /* Ici les pads sont proches et les cercles exinxcrits sont trop proches
     *  Selon les formes relatives il peut y avoir ou non erreur */

    bool swap_pads;
    swap_pads = false;
    if( (aRefPad->m_PadShape != CIRCLE) && (aPad->m_PadShape == CIRCLE) )
        swap_pads = true;
    else if( (aRefPad->m_PadShape != OVALE) && (aPad->m_PadShape == OVALE) )
        swap_pads = true;

    if( swap_pads )
    {
        EXCHG( aRefPad, aPad );
        rel_pos.x = -rel_pos.x;
        rel_pos.y = -rel_pos.y;
    }

    switch( aRefPad->m_PadShape )
    {
    case CIRCLE:        // aRefPad is like a track segment with a null lenght
        m_segmLength  = 0;
        m_segmAngle = 0;
        
        m_finx  = m_finy = 0;
        
        m_spotcx = rel_pos.x;
        m_spotcy = rel_pos.y;
        
        diag = checkClearanceSegmToPad( aPad, aRefPad->m_Rayon, dist_min );
        break;

    case RECT:
        RotatePoint( &rel_pos.x, &rel_pos.y, aRefPad->m_Orient );
        pad_angle = aRefPad->m_Orient + aPad->m_Orient;      // pad_angle = pad orient relative to the aRefPad orient
        NORMALIZE_ANGLE_POS( pad_angle );
        if( aPad->m_PadShape == RECT )
        {
            wxSize size = aPad->m_Size;
            if( (pad_angle == 0) || (pad_angle == 900) || (pad_angle == 1800) ||
               (pad_angle == 2700) )
            {
                if( (pad_angle == 900) || (pad_angle == 2700) )
                {
                    EXCHG( size.x, size.y );
                }

                // Test DRC:
                diag      = false;

                rel_pos.x = ABS( rel_pos.x );
                rel_pos.y = ABS( rel_pos.y );

                if( ( rel_pos.x - ( (size.x + aRefPad->m_Size.x) / 2 ) ) >= dist_min )
                    diag = true;

                if( ( rel_pos.y - ( (size.y + aRefPad->m_Size.y) / 2 ) ) >= dist_min )
                    diag = true;
            }
            else        // Any other orient
            {
                        /* TODO : any orient ... */
            }
        }
        break;

    case OVALE:     /* an oval pad is like a track segment */
    {
        /* Create and test a track segment with same dimensions */
        int segm_width;
        m_segmAngle = aRefPad->m_Orient;                     // Segment orient.
        if( aRefPad->m_Size.y < aRefPad->m_Size.x )         /* We suppose the pad is an horizontal oval */
        {
            segm_width = aRefPad->m_Size.y;
            m_segmLength  = aRefPad->m_Size.x - aRefPad->m_Size.y;
        }
        else        // it was a vertical oval, change to a rotated horizontal one
        {
            segm_width  = aRefPad->m_Size.x;
            m_segmLength   = aRefPad->m_Size.y - aRefPad->m_Size.x;
            m_segmAngle += 900;
        }
        
        /* the start point must be 0,0 and currently rel_pos is relative the center of pad coordinate */
        int sx = -m_segmLength / 2, sy = 0;        // Start point coordinate of the horizontal equivalent segment
        
        RotatePoint( &sx, &sy, m_segmAngle );    // True start point coordinate of the equivalent segment
        
        m_spotcx = rel_pos.x + sx;
        m_spotcy = rel_pos.y + sy;               // pad position / segment origin
        
        m_finx    = -sx;
        m_finy    = -sy;                          // end of segment coordinate
        
        diag = checkClearanceSegmToPad( aPad, segm_width / 2, dist_min );
        break;
    }

    default:
        /* TODO...*/
        break;
    }

exit:       // the only way out (hopefully) for simpler debugging
    
    return diag;
}


bool DRC::checkClearanceSegmToPad( const D_PAD* pad_to_test, int w_segm, int dist_min )
{
    int p_dimx; 
    int p_dimy;         // half the dimension of the pad
    int orient;
    int x0, y0, xf, yf;
    int seuil;
    int deltay;

    seuil  = w_segm + dist_min;
    p_dimx = pad_to_test->m_Size.x >> 1;
    p_dimy = pad_to_test->m_Size.y >> 1;

    if( pad_to_test->m_PadShape == CIRCLE )
    {
        /* calcul des coord centre du pad dans le repere axe X confondu
         *  avec le segment en tst */
        RotatePoint( &m_spotcx, &m_spotcy, m_segmAngle );
        return checkMarginToCircle( m_spotcx, m_spotcy, seuil + p_dimx, m_segmLength );
    }
    else
    {
        /* calcul de la "surface de securite" du pad de reference */
        m_xcliplo = m_spotcx - seuil - p_dimx;
        m_ycliplo = m_spotcy - seuil - p_dimy;
        m_xcliphi = m_spotcx + seuil + p_dimx;
        m_ycliphi = m_spotcy + seuil + p_dimy;

        x0 = y0 = 0;

        xf = m_finx;
        yf = m_finy;

        orient = pad_to_test->m_Orient;

        RotatePoint( &x0, &y0, m_spotcx, m_spotcy, -orient );
        RotatePoint( &xf, &yf, m_spotcx, m_spotcy, -orient );

        if( checkLine( x0, y0, xf, yf ) )
            return true;

        /* Erreur DRC : analyse fine de la forme de la pastille */

        switch( pad_to_test->m_PadShape )
        {
        default:
            return false;

        case OVALE:
            /* test de la pastille ovale ramenee au type ovale vertical */
            if( p_dimx > p_dimy )
            {
                EXCHG( p_dimx, p_dimy ); 
                orient += 900;
                if( orient >= 3600 )
                    orient -= 3600;
            }
            deltay = p_dimy - p_dimx;

            /* ici: p_dimx = rayon,
             *      delta = dist centre cercles a centre pad */

            /* Test du rectangle separant les 2 demi cercles */
            m_xcliplo = m_spotcx - seuil - p_dimx;
            m_ycliplo = m_spotcy - w_segm - deltay;
            m_xcliphi = m_spotcx + seuil + p_dimx;
            m_ycliphi = m_spotcy + w_segm + deltay;

            if( !checkLine( x0, y0, xf, yf ) )
                return false;

            /* test des 2 cercles */
            x0 = m_spotcx;     /* x0,y0 = centre du cercle superieur du pad ovale */
            y0 = m_spotcy + deltay;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );

            if( !checkMarginToCircle( x0, y0, p_dimx + seuil, m_segmLength ) )
                return false;

            x0 = m_spotcx;     /* x0,y0 = centre du cercle inferieur du pad ovale */
            y0 = m_spotcy - deltay;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );

            if( !checkMarginToCircle( x0, y0, p_dimx + seuil, m_segmLength ) )
                return false;
            break;

        case RECT:      /* 2 rectangle + 4 1/4 cercles a tester */
            /* Test du rectangle dimx + seuil, dimy */
            m_xcliplo = m_spotcx - p_dimx - seuil;
            m_ycliplo = m_spotcy - p_dimy;
            m_xcliphi = m_spotcx + p_dimx + seuil;
            m_ycliphi = m_spotcy + p_dimy;

            if( !checkLine( x0, y0, xf, yf ) )
            {
                return false;
            }

            /* Test du rectangle dimx , dimy + seuil */
            m_xcliplo = m_spotcx - p_dimx;
            m_ycliplo = m_spotcy - p_dimy - seuil;
            m_xcliphi = m_spotcx + p_dimx;
            m_ycliphi = m_spotcy + p_dimy + seuil;

            if( !checkLine( x0, y0, xf, yf ) )
            {
                return false;
            }

            /* test des 4 cercles ( surface d'solation autour des sommets */
            /* test du coin sup. gauche du pad */
            x0 = m_spotcx - p_dimx;
            y0 = m_spotcy - p_dimy;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );
            if( !checkMarginToCircle( x0, y0, seuil, m_segmLength ) )
            {
                return false;
            }

            /* test du coin sup. droit du pad */
            x0 = m_spotcx + p_dimx;
            y0 = m_spotcy - p_dimy;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );
            if( !checkMarginToCircle( x0, y0, seuil, m_segmLength ) )
            {
                return false;
            }

            /* test du coin inf. gauche du pad */
            x0 = m_spotcx - p_dimx;
            y0 = m_spotcy + p_dimy;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );
            if( !checkMarginToCircle( x0, y0, seuil, m_segmLength ) )
            {
                return false;
            }

            /* test du coin inf. droit du pad */
            x0 = m_spotcx + p_dimx;
            y0 = m_spotcy + p_dimy;
            RotatePoint( &x0, &y0, m_spotcx, m_spotcy, orient );
            RotatePoint( &x0, &y0, m_segmAngle );
            if( !checkMarginToCircle( x0, y0, seuil, m_segmLength ) )
            {
                return false;
            }

            break;
        }
    }
    return true;
}

/**********************************************************************/
bool DRC::checkMarginToCircle( int cx, int cy, int radius, int longueur )
/**********************************************************************/
{
    if( abs( cy ) > radius )
        return true;

    if( (cx >= -radius ) && ( cx <= (longueur + radius) ) )
    {
        if( (cx >= 0) && (cx <= longueur) )
            return false;
        
        if( cx > longueur )
            cx -= longueur;
        
        if( hypot( cx, cy ) < radius )
            return false;
    }

    return true;
}



/**********************************************/
/* int Tst_Ligne(int x1,int y1,int x2,int y2) */
/**********************************************/

static inline int USCALE( unsigned arg, unsigned num, unsigned den )
{
    int ii;

    ii = (int) ( ( (double) arg * num ) / den );
    return ii;
}


#define WHEN_OUTSIDE return true
#define WHEN_INSIDE

bool DRC::checkLine( int x1, int y1, int x2, int y2 )
{
    int temp;

    if( x1 > x2 )
    {
        EXCHG( x1, x2 );
        EXCHG( y1, y2 );
    }
    if( (x2 < m_xcliplo) || (x1 > m_xcliphi) )
    {
        WHEN_OUTSIDE;
    }
    if( y1 < y2 )
    {
        if( (y2 < m_ycliplo) || (y1 > m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 < m_ycliplo )
        {
            temp = USCALE( (x2 - x1), (m_ycliplo - y1), (y2 - y1) );
            if( (x1 += temp) > m_xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = m_ycliplo;
            WHEN_INSIDE;
        }
        if( y2 > m_ycliphi )
        {
            temp = USCALE( (x2 - x1), (y2 - m_ycliphi), (y2 - y1) );
            if( (x2 -= temp) < m_xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = m_ycliphi;
            WHEN_INSIDE;
        }
        if( x1 < m_xcliplo )
        {
            temp = USCALE( (y2 - y1), (m_xcliplo - x1), (x2 - x1) );
            y1  += temp; 
            x1 = m_xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > m_xcliphi )
        {
            temp = USCALE( (y2 - y1), (x2 - m_xcliphi), (x2 - x1) );
            y2  -= temp; 
            x2 = m_xcliphi;
            WHEN_INSIDE;
        }
    }
    else
    {
        if( (y1 < m_ycliplo) || (y2 > m_ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 > m_ycliphi )
        {
            temp = USCALE( (x2 - x1), (y1 - m_ycliphi), (y1 - y2) );
            if( (x1 += temp) > m_xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = m_ycliphi;
            WHEN_INSIDE;
        }
        if( y2 < m_ycliplo )
        {
            temp = USCALE( (x2 - x1), (m_ycliplo - y2), (y1 - y2) );
            if( (x2 -= temp) < m_xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = m_ycliplo;
            WHEN_INSIDE;
        }
        if( x1 < m_xcliplo )
        {
            temp = USCALE( (y1 - y2), (m_xcliplo - x1), (x2 - x1) );
            y1  -= temp;
            x1 = m_xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > m_xcliphi )
        {
            temp = USCALE( (y1 - y2), (x2 - m_xcliphi), (x2 - x1) );
            y2  += temp;
            x2 = m_xcliphi;
            WHEN_INSIDE;
        }
    }

    if( ( (x2 + x1)/2 <= m_xcliphi ) && ( (x2 + x1)/2 >= m_xcliplo ) \
     && ( (y2 + y1)/2 <= m_ycliphi ) && ( (y2 + y1)/2 >= m_ycliplo ) )
    {
        return false;
    }
    else
        return true;
}



#if 0

//----< new stuff above this line, old stuff below >------------------------


/* saving drc options */
static bool      s_Pad2PadTestOpt     = true;
static bool      s_UnconnectedTestOpt = true;
static bool      s_ZonesTestOpt     = false;
static bool      s_CreateRptFileOpt = false;
static FILE*     s_RptFile = NULL;
static wxString  s_RptFilename;

static int       ErrorsDRC_Count;
static MARKER* current_marqueur; /* Pour gestion des marqueurs sur pcb */

static bool      AbortDrc, DrcInProgress = FALSE;
static int       spot_cX, spot_cY;                      /* position d'elements a tester */
static int       finx, finy;                            // coord relatives de l'extremite du segm de reference
static int       segm_angle;                            // angle d'inclinaison du segment de reference en 0,1 degre
static int       segm_long;                             // longueur du segment de reference
static int       xcliplo, ycliplo, xcliphi, ycliphi;    /* coord de la surface de securite du segment a comparer */


/************************************************************************/
int WinEDA_PcbFrame::Test_DRC( wxDC* DC, bool TestPad2Pad, bool TestZone )
/************************************************************************/

/* Test DRC :
 *  Run a drc control for each pad and track segment
 *  Put a marker on pad or track end which have a drc problem
 */
{
    int             ii, jj, old_net;
    int             flag_err_Drc;
    TRACK*          pt_segm;
    D_PAD*          pad;
    MARQUEUR*       Marqueur;
    EDA_BaseStruct* PtStruct;
    wxString        Line;

#define PRINT_NB_PAD_POS      42
#define PRINT_PAD_ERR_POS     48
#define PRINT_TST_POS         20
#define PRINT_NB_SEGM_POS     26
#define PRINT_TRACK_ERR_POS   32
#define PRINT_NB_ZONESEGM_POS 60
#define PRINT_ZONE_ERR_POS    70

    DrcInProgress   = TRUE;
    ErrorsDRC_Count = 0;
    Compile_Ratsnest( DC, TRUE );

    MsgPanel->EraseMsgBox();

    m_CurrentScreen->SetRefreshReq();

    /* Delete previous markers */
    Erase_Marqueurs();

    if( TestPad2Pad )  /* First test: Test DRC between pads (no track)*/
    {
        Line.Printf( wxT( "%d" ), m_Pcb->m_NbPads );
        Affiche_1_Parametre( this, PRINT_NB_PAD_POS, wxT( "NbPad" ), Line, RED );
        Affiche_1_Parametre( this, PRINT_PAD_ERR_POS, wxT( "Pad Err" ), wxT( "0" ), LIGHTRED );
        
        if( DrcFrame )
            DrcFrame->m_logWindow->AppendText( _( "Tst Pad to Pad\n" ) );
        
        LISTE_PAD* pad_list_start = CreateSortedPadListByXCoord( m_Pcb );
        LISTE_PAD* pad_list_limit = &pad_list_start[m_Pcb->m_NbPads];
        int        max_size = 0;
        LISTE_PAD* pad_list;
        
        /* Compute the max size of the pads ( used to stop the test) */
        for( pad_list = pad_list_start; pad_list < pad_list_limit; pad_list++ )
        {
            pad = *pad_list;
            if( pad->m_Rayon > max_size )
                max_size = pad->m_Rayon;
        }

        /* Test the pads */
        for( pad_list = pad_list_start; pad_list < pad_list_limit; pad_list++ )
        {
            pad = *pad_list;
            if( Test_Pad_to_Pads_Drc( this, DC, pad, pad_list, pad_list_limit, max_size,
                                      TRUE ) == BAD_DRC )
            {
                Marqueur = current_marqueur;
                current_marqueur = NULL;
                if( Marqueur == NULL )
                {
                    DisplayError( this, wxT( "Test_Drc(): internal err" ) );
                    return ErrorsDRC_Count;
                }
                Line.Printf( wxT( "%d" ), ErrorsDRC_Count );
                Affiche_1_Parametre( this, PRINT_PAD_ERR_POS, wxEmptyString, Line, LIGHTRED );
                Marqueur->Pnext = m_Pcb->m_Drawings;
                Marqueur->Pback = m_Pcb;

                PtStruct = m_Pcb->m_Drawings;
                if( PtStruct )
                    PtStruct->Pback = Marqueur;
                m_Pcb->m_Drawings = Marqueur;
            }
        }

        free( pad_list_start );
    }

    /* Test track segments */
    Line.Printf( wxT( "%d" ), m_Pcb->m_NbSegmTrack );
    Affiche_1_Parametre( this, PRINT_NB_SEGM_POS, _( "SegmNb" ), Line, RED );
    Affiche_1_Parametre( this, PRINT_TRACK_ERR_POS, _( "Track Err" ), wxT( "0" ), LIGHTRED );
    pt_segm = m_Pcb->m_Track;

    if( DrcFrame )
        DrcFrame->m_logWindow->AppendText( _( "Tst Tracks\n" ) );
    
    for( ii = 0, old_net = -1, jj = 0;
         pt_segm != NULL;
         pt_segm = (TRACK*) pt_segm->Pnext, ii++, jj-- )
    {
        if( pt_segm->Pnext == NULL )
            break;
        
        if( jj == 0 )
        {
            jj = 10;
            wxYield();
            if( AbortDrc )
            {
                AbortDrc = FALSE; break;
            }
            /* Print stats */
            Line.Printf( wxT( "%d" ), ii );
            Affiche_1_Parametre( this, PRINT_TST_POS, wxT( "Test" ), Line, CYAN );
        }

        if( old_net != pt_segm->GetNet() )
        {
            wxString msg;
            jj = 1;
            EQUIPOT* equipot = m_Pcb->FindNet( pt_segm->GetNet() );
            if( equipot )
                msg = equipot->m_Netname + wxT( "        " );
            else
                msg = wxT( "<noname>" );
            Affiche_1_Parametre( this, 0, _( "Netname" ), msg, YELLOW );
            old_net = pt_segm->GetNet();
        }

        g_HightLigth_NetCode = pt_segm->GetNet();
        flag_err_Drc = Drc( this, DC, pt_segm, (TRACK*) pt_segm->Pnext, 1 );
        if( flag_err_Drc == BAD_DRC )
        {
            Marqueur = current_marqueur;
            current_marqueur = NULL;
            if( Marqueur == NULL )
            {
                DisplayError( this, wxT( "Test_Drc(): internal err" ) );
                return ErrorsDRC_Count;
            }
            Marqueur->Pnext = m_Pcb->m_Drawings;
            Marqueur->Pback = m_Pcb;

            PtStruct = m_Pcb->m_Drawings;
            if( PtStruct )
                PtStruct->Pback = Marqueur;
            m_Pcb->m_Drawings = Marqueur;

            GRSetDrawMode( DC, GR_OR );
            pt_segm->Draw( DrawPanel, DC, RED ^ LIGHTRED );
            Line.Printf( wxT( "%d" ), ErrorsDRC_Count );
            Affiche_1_Parametre( this, PRINT_TRACK_ERR_POS, wxEmptyString, Line, LIGHTRED );
        }
    }

    /* Test zone segments */
    if( TestZone )
    {
        m_Pcb->m_NbSegmZone = 0;
        for( pt_segm = (TRACK*) m_Pcb->m_Zone;  pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
            m_Pcb->m_NbSegmZone++;

        Line.Printf( wxT( "%d" ), m_Pcb->m_NbSegmZone );
        Affiche_1_Parametre( this, PRINT_NB_ZONESEGM_POS, _( "SegmNb" ), Line, RED );
        Affiche_1_Parametre( this, PRINT_ZONE_ERR_POS, _( "Zone Err" ), wxT( "0" ), LIGHTRED );

        if( DrcFrame )
            DrcFrame->m_logWindow->AppendText( _( "Tst Zones\n" ) );

        pt_segm = (TRACK*) m_Pcb->m_Zone;
        
        for( ii = 0, old_net = -1, jj = 0;
             pt_segm != NULL;
             pt_segm = (TRACK*) pt_segm->Pnext, ii++, jj-- )
        {
            if( pt_segm->Pnext == NULL )
                break;
            
            if( jj == 0 )
            {
                jj = 100;
                wxYield();
                if( AbortDrc )
                {
                    AbortDrc = FALSE; 
                    break;
                }
                /* Print stats */
                Line.Printf( wxT( "%d" ), ii );
                Affiche_1_Parametre( this, PRINT_TST_POS, wxT( "Test" ), Line, CYAN );
            }

            if( old_net != pt_segm->GetNet() )
            {
                jj = 1;
                wxString msg;
                EQUIPOT* equipot = m_Pcb->FindNet( pt_segm->GetNet() );
                
                if( equipot )
                    msg = equipot->m_Netname + wxT( "        " );
                else
                    msg = wxT( "<noname>" );
                
                Affiche_1_Parametre( this, 0, _( "Netname" ), msg, YELLOW );
                old_net = pt_segm->GetNet();
            }
            g_HightLigth_NetCode = pt_segm->GetNet();
            
            /* Test drc with other zone segments, and pads */
            flag_err_Drc = Drc( this, DC, pt_segm, (TRACK*) pt_segm->Pnext, 1 );
            if( flag_err_Drc == BAD_DRC )
            {
                Marqueur = current_marqueur;
                current_marqueur = NULL;
                if( Marqueur == NULL )
                {
                    DisplayError( this, wxT( "Test_Drc(): internal err" ) );
                    return ErrorsDRC_Count;
                }
                Marqueur->Pnext = m_Pcb->m_Drawings;
                Marqueur->Pback = m_Pcb;

                PtStruct = m_Pcb->m_Drawings;
                if( PtStruct )
                    PtStruct->Pback = Marqueur;
                m_Pcb->m_Drawings = Marqueur;

                GRSetDrawMode( DC, GR_OR );
                pt_segm->Draw( DrawPanel, DC, RED ^ LIGHTRED );
                Line.Printf( wxT( "%d" ), ErrorsDRC_Count );
                Affiche_1_Parametre( this, PRINT_ZONE_ERR_POS, wxEmptyString, Line, LIGHTRED );
            }

            /* Test drc with track segments */
            int tmp = m_Pcb->m_NbPads; 
            m_Pcb->m_NbPads = 0;    // Pads already tested: disable pad test
            flag_err_Drc    = Drc( this, DC, pt_segm, m_Pcb->m_Track, 1 );
            
            m_Pcb->m_NbPads = tmp;
            
            if( flag_err_Drc == BAD_DRC )
            {
                Marqueur = current_marqueur;
                current_marqueur = NULL;
                if( Marqueur == NULL )
                {
                    DisplayError( this, wxT( "Test_Drc(): internal err" ) );
                    return ErrorsDRC_Count;
                }
                Marqueur->Pnext = m_Pcb->m_Drawings;
                Marqueur->Pback = m_Pcb;

                PtStruct = m_Pcb->m_Drawings;
                if( PtStruct )
                    PtStruct->Pback = Marqueur;
                
                m_Pcb->m_Drawings = Marqueur;

                GRSetDrawMode( DC, GR_OR );
                pt_segm->Draw( DrawPanel, DC, RED ^ LIGHTRED );
                Line.Printf( wxT( "%d" ), ErrorsDRC_Count );
                Affiche_1_Parametre( this, PRINT_ZONE_ERR_POS, wxEmptyString, Line, LIGHTRED );
            }
        }
    }

    AbortDrc      = FALSE;
    DrcInProgress = FALSE;
    return ErrorsDRC_Count;
}


/***************************************************************/
void DrcDialog::ListUnconnectedPads( wxCommandEvent& event )
/***************************************************************/
{
    if( (m_Parent->m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
    {
        m_Parent->Compile_Ratsnest( m_DC, TRUE );
    }
    if( m_Parent->m_Pcb->m_Ratsnest == NULL )
        return;

    CHEVELU*          Ratsnest  = m_Parent->m_Pcb->m_Ratsnest;
    int               draw_mode = GR_SURBRILL | GR_OR;
    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;
    int               ii;
    wxString          msg;
    double            convert = 0.0001;

    msg = _( "Look for active routes\n" );
//    m_logWindow->AppendText( msg );
    if( s_RptFile )
        fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );

    m_UnconnectedCount = 0;
    for( ii = m_Parent->m_Pcb->GetNumRatsnests(); ii > 0; Ratsnest++, ii-- )
    {
        if( (Ratsnest->status & CH_ACTIF) == 0 )
            continue;

        m_UnconnectedCount++;
        if( m_UnconnectedCount == 1 )
        {
//            m_logWindow->AppendText( _( "Unconnected found:\n" ) );
        }

        D_PAD*   pad = Ratsnest->pad_start;
        pad->Draw( panel, m_DC, wxPoint( 0, 0 ), draw_mode );

        wxString pad_name    = pad->ReturnStringPadName();
        wxString module_name = ( (MODULE*) (pad->m_Parent) )->m_Reference->m_Text;

        msg.Printf( _( "%d > Pad %s (%s) @ %.4f,%.4f and " ), m_UnconnectedCount,
                    pad_name.GetData(), module_name.GetData(),
                    pad->m_Pos.x * convert, pad->m_Pos.y * convert );

//        m_logWindow->AppendText( msg );
        if( s_RptFile )
            fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );

        pad = Ratsnest->pad_end;
        pad->Draw( panel, m_DC, wxPoint( 0, 0 ), draw_mode );

        pad_name    = pad->ReturnStringPadName();
        module_name = ( (MODULE*) (pad->m_Parent) )->m_Reference->m_Text;

        msg.Printf( _( "Pad %s (%s) @ %.4f,%.4f\n" ),
                    pad_name.GetData(), module_name.GetData(),
                    pad->m_Pos.x * convert, pad->m_Pos.y * convert );

//        m_logWindow->AppendText( msg );
        if( s_RptFile )
            fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );
    }

    if( m_UnconnectedCount )
        msg.Printf( _( "Active routes: %d\n" ), m_UnconnectedCount );
    else
        msg = _( "OK! (No active routes)\n" );

//    m_logWindow->AppendText( msg );
    if( s_RptFile )
        fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );
}


/****************************************************/
void DrcDialog::TestDrc( wxCommandEvent& event )
/****************************************************/
{
    int      errors;
    wxString msg;

    if( !DrcInProgress )
    {
        if( m_CreateRptCtrl->IsChecked() ) // Create a file rpt
        {
            s_RptFilename = m_RptFilenameCtrl->GetValue();
            
            if( s_RptFilename.IsEmpty() )
                OnButtonBrowseRptFileClick( event );
            
            if( !s_RptFilename.IsEmpty() )
                s_RptFile = wxFopen( s_RptFilename, wxT( "w" ) );
            else
                s_RptFile = NULL;
        }

        if( s_RptFile )
        {
            fprintf( s_RptFile, "Drc report for %s\n",
                    CONV_TO_UTF8( m_Parent->m_CurrentScreen->m_FileName ) );
            char line[256];
            fprintf( s_RptFile, "Created on %s\n", DateAndTime( line ) );
        }

        s_Pad2PadTestOpt     = m_Pad2PadTestCtrl->IsChecked();
        s_UnconnectedTestOpt = m_UnconnectedTestCtrl->IsChecked();
        
        s_ZonesTestOpt = m_ZonesTestCtrl->IsChecked();
        
        AbortDrc = FALSE;
        m_logWindow->Clear();
        g_DesignSettings.m_TrackClearence =
            ReturnValueFromTextCtrl( *m_SetClearance, m_Parent->m_InternalUnits );
            
        /* Test DRC errors (clearance errors, bad connections .. */
        errors = m_Parent->Test_DRC( m_DC, m_Pad2PadTestCtrl->IsChecked(
                                    ), m_ZonesTestCtrl->IsChecked() );
        
        /* Search for active routes (unconnected pads) */
        if( m_UnconnectedTestCtrl->IsChecked() )
            ListUnconnectedPads( event );
        else
            m_UnconnectedCount = 0;
        
        if( errors )
            msg.Printf( _( "** End Drc: %d errors **\n" ), errors );
        else if( m_UnconnectedCount == 0 )
            msg = _( "** End Drc: No Error **\n" );
        
        m_logWindow->AppendText( msg );

        if( s_RptFile )
            fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );

        if( s_RptFile )
        {
            msg.Printf( _( "Report file <%s> created\n" ), s_RptFilename.GetData() );
            m_logWindow->AppendText( msg );
            fclose( s_RptFile );
            s_RptFile = NULL;
        }
    }
    else
        wxBell();
}


/*********************************************************/
void DrcDialog::DelDRCMarkers( wxCommandEvent& event )
/*********************************************************/
{
    m_Parent->Erase_Marqueurs();
    m_Parent->DrawPanel->ReDraw( m_DC, TRUE );
}


/***********************************************************************/
int Drc( WinEDA_BasePcbFrame* frame, wxDC* DC,
         TRACK* pt_segment, TRACK* StartBuffer, int show_err )
/***********************************************************************/

/**
 *  Test the current segment:
 * @param pt_segment = current segment to test
 * @param StartBuffer = track buffer to test (usually m_Pcb->m_Track)
 * @param show_err (flag) si 0 pas d'affichage d'erreur sur ecran
 * @return :      BAD_DRC (1) if DRC error  or OK_DRC (0) if OK
 */
{
    int     ii;
    TRACK*  pttrack;
    int     x0, y0, xf, yf; // coord des extremites du segment teste dans le repere modifie
    int     dx, dy;         // utilise pour calcul des dim x et dim y des segments
    int     w_dist;
    int     MaskLayer;
    int     net_code_ref;
    int     org_X, org_Y; // Origine sur le PCB des axes du repere centre sur
                         //	l'origine du segment de reference
    wxPoint shape_pos;

    org_X        = pt_segment->m_Start.x;
    org_Y        = pt_segment->m_Start.y;

    finx         = dx = pt_segment->m_End.x - org_X;
    finy         = dy = pt_segment->m_End.y - org_Y;

    MaskLayer    = pt_segment->ReturnMaskLayer();
    net_code_ref = pt_segment->GetNet();

    segm_angle = 0;
	/* for a non horizontal or vertical segment Compute the segment angle
	in 0,1 degrees and its lenght */
    if( dx || dy )
    {
        /* Compute the segment angle in 0,1 degrees */
        segm_angle = ArcTangente( dy, dx );

        /* Compute the segment lenght: we build an equivalent rotated segment,
		this segment is horizontal, therefore dx = lenght */
       RotatePoint( &dx, &dy, segm_angle ); /* dx = lenght, dy = 0 */
    }

    segm_long = dx;

    /******************************************/
    /* Phase 1 : test DRC track to pads :*/
    /******************************************/

    /* Compute the min distance to pads : */
    w_dist = (unsigned) (pt_segment->m_Width >> 1 );
    for( ii = 0; ii < frame->m_Pcb->m_NbPads; ii++ )
    {
        D_PAD* pt_pad = frame->m_Pcb->m_Pads[ii];

        /* No problem if pads are on an other layer,
         *  But if a drill hole exists	(a pad on a single layer can have a hole!)
		 * we must test the hole
		*/
        if( (pt_pad->m_Masque_Layer & MaskLayer ) == 0 )
        {
            /* We must test the pad hole. In order to use the function "TestClearanceSegmToPad",
             *  a pseudo pad is used, with a shape and a size like the hole */
            if( pt_pad->m_Drill.x == 0 )
                continue;

            D_PAD pseudo_pad( (MODULE*) NULL );

            pseudo_pad.m_Size     = pt_pad->m_Drill;
            pseudo_pad.m_Pos      = pt_pad->m_Pos;
            pseudo_pad.m_PadShape = pt_pad->m_DrillShape;
            pseudo_pad.m_Orient   = pt_pad->m_Orient;
            pseudo_pad.ComputeRayon();
            spot_cX = pseudo_pad.m_Pos.x - org_X;
            spot_cY = pseudo_pad.m_Pos.y - org_Y;
            if( TestClearanceSegmToPad( &pseudo_pad, w_dist,
                                        g_DesignSettings.m_TrackClearence ) != OK_DRC )
            {
                ErrorsDRC_Count++;
                if( show_err )
                    Affiche_Erreur_DRC( frame->DrawPanel, DC,
                                        frame->m_Pcb, pt_segment, pt_pad, 0 );
                return BAD_DRC;
            }
            continue;
        }

        /* The pad must be in a net (i.e pt_pad->GetNet() != 0 )
         *  but no problem if the pad netcode is the current netcode (same net) */
        if( pt_pad->GetNet() &&	// the pad must be connected
			net_code_ref == pt_pad->GetNet() )	// the pad net is the same as current net -> Ok
            continue;

        /* Test DRC pour les pads */
        shape_pos = pt_pad->ReturnShapePos();
        spot_cX   = shape_pos.x - org_X;
        spot_cY   = shape_pos.y - org_Y;
        if( TestClearanceSegmToPad( pt_pad, w_dist, g_DesignSettings.m_TrackClearence ) == OK_DRC )
            continue;

        /* Drc error found! */
        else
        {
            ErrorsDRC_Count++;
            if( show_err )
                Affiche_Erreur_DRC( frame->DrawPanel, DC,
                                    frame->m_Pcb, pt_segment, pt_pad, 1 );
            return BAD_DRC;
        }
    }

    /**********************************************/
    /* Phase 2: test DRC with other track segments */
    /**********************************************/

    /* At this point the reference segment is the X axis */

    /* Test the reference segment with other track segments */
    pttrack = StartBuffer;
    for( ; pttrack != NULL; pttrack = (TRACK*) pttrack->Pnext )
    {
        //No problem if segments have the meme net code:
        if( net_code_ref == pttrack->GetNet() )
            continue;

        // No problem if segment are on different layers :
        if( ( MaskLayer & pttrack->ReturnMaskLayer() ) == 0 )
            continue;

        /* calcul de la Distance mini = Isol+ rayon ou demi largeur seg ref
         + rayon ou demi largeur seg a comparer */
        w_dist  = pt_segment->m_Width >> 1;
        w_dist += pttrack->m_Width >> 1;
        w_dist += g_DesignSettings.m_TrackClearence;

        /* If the reference segment is a via, we test it here */
        if( pt_segment->Type() == TYPEVIA )
        {
            int orgx, orgy; // origine du repere d'axe X = segment a comparer
            int angle = 0;  // angle du segment a tester;
            orgx = pttrack->m_Start.x; orgy = pttrack->m_Start.y;
            dx   = pttrack->m_End.x - orgx; dy = pttrack->m_End.y - orgy;
            x0   = pt_segment->m_Start.x - orgx; y0 = pt_segment->m_Start.y - orgy;

            if( pttrack->Type() == TYPEVIA )   /* Tst distance entre 2 vias */
            {
                if( (int) hypot( (float) x0, (float) y0 ) < w_dist )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            21 );
                    return BAD_DRC;
                }
            }
            else    /* Tst drc via / segment */
            {
                /* Compute l'angle */
                angle = ArcTangente( dy, dx );

                /* Compute new coordinates ( the segment become horizontal) */
                RotatePoint( &dx, &dy, angle );
                RotatePoint( &x0, &y0, angle );

                if( TestMarginToCircle( x0, y0, w_dist, dx ) == BAD_DRC )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            20 );
                    return BAD_DRC;
                }
            }
            continue;
        }

        /* We compute x0,y0, xf,yf = starting and ending point coordinates for the segment to test
         *  in the new axis : the new X axis is the reference segment
		 *  We must translate and rotate the segment to test
		*/
        x0 = pttrack->m_Start.x - org_X;
        y0 = pttrack->m_Start.y - org_Y;

        xf = pttrack->m_End.x - org_X;
        yf = pttrack->m_End.y - org_Y;

        RotatePoint( &x0, &y0, segm_angle );
        RotatePoint( &xf, &yf, segm_angle );

        if( pttrack->Type() == TYPEVIA )
        {
            if( TestMarginToCircle( x0, y0, w_dist, segm_long ) == OK_DRC )
                continue;
            ErrorsDRC_Count++;
            if( show_err )
                Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pt_segment, pttrack, 21 );
            return BAD_DRC;
        }


        /*	We have changed axis:
         *  the reference segment is Horizontal.
         *  3 cases : the segment to test can be parallel, perpendicular or have an other direction
         */
        if( y0 == yf ) // parallel segments
        {
            if( abs( y0 ) >= w_dist )
                continue;

            if( x0 > xf )
                EXCHG( x0, xf );                                /* pour que x0 <= xf */

            if( x0 > (-w_dist) && x0 < (segm_long + w_dist) )   /* possible error drc */
            {
                /* Fine test : we consider the rounded shape of the ends */
                if( x0 >= 0 && x0 <= segm_long )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            2 );
                    return BAD_DRC;
                }
                if( TestMarginToCircle( x0, y0, w_dist, segm_long ) == BAD_DRC )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            2 );
                    return BAD_DRC;
                }
            }
            if( xf > (-w_dist) && xf < (segm_long + w_dist) )
            {
                /* Fine test : we consider the rounded shape of the ends */
                if( xf >= 0 && xf <= segm_long )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            3 );
                    return BAD_DRC;
                }
                if( TestMarginToCircle( xf, yf, w_dist, segm_long ) == BAD_DRC )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            3 );
                    return BAD_DRC;
                }
            }

            if( x0 <=0 && xf >= 0 )
            {
                ErrorsDRC_Count++;
                if( show_err )
                    Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pt_segment, pttrack, 4 );
                return BAD_DRC;
            }
        }
        else if( x0 == xf ) // perpendicular segments
        {
            if( ( x0 <= (-w_dist) ) || ( x0 >= (segm_long + w_dist) ) )
                continue;

            /* Test is segments are crossing */
            if( y0 > yf )
                EXCHG( y0, yf );
            if( (y0 < 0) && (yf > 0) )
            {
                ErrorsDRC_Count++;
                if( show_err )
                    Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pt_segment, pttrack, 6 );
                return BAD_DRC;
            }

            /* At this point the drc error is due to an end near a reference segm end */
            if( TestMarginToCircle( x0, y0, w_dist, segm_long ) == BAD_DRC )
            {
                ErrorsDRC_Count++;
                if( show_err )
                    Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pt_segment, pttrack, 7 );
                return BAD_DRC;
            }
            if( TestMarginToCircle( xf, yf, w_dist, segm_long ) == BAD_DRC )
            {
                ErrorsDRC_Count++;
                if( show_err )
                    Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pt_segment, pttrack, 8 );
                return BAD_DRC;
            }
        }
        else // segments quelconques entre eux */
        {
            int bflag = OK_DRC;
            /* calcul de la "surface de securite du segment de reference */
            /* First rought 'and fast) test : the track segment is like a rectangle */

            xcliplo = ycliplo = -w_dist;
            xcliphi = segm_long + w_dist; ycliphi = w_dist;

            bflag = Tst_Ligne( x0, y0, xf, yf );
            if( bflag == BAD_DRC )	/* A fine test is needed because a serment is not exactly a rectangle
				it has rounded ends */
            {
                /* 2eme passe : the track has rounded ends.
                 * we must a fine test for each rounded end and the rectangular zone */

                xcliplo = 0; xcliphi = segm_long;
                bflag   = Tst_Ligne( x0, y0, xf, yf );

                if( bflag == BAD_DRC )
                {
                    ErrorsDRC_Count++;
                    if( show_err )
                        Affiche_Erreur_DRC( frame->DrawPanel,
                                            DC,
                                            frame->m_Pcb,
                                            pt_segment,
                                            pttrack,
                                            9 );
                    return BAD_DRC;
                }
                else    // The drc error is due to the starting or the ending point of the reference segment
                {
                    // Test the starting and the ending point
                    int angle, rx0, ry0, rxf, ryf;
                    x0 = pttrack->m_Start.x;
                    y0 = pttrack->m_Start.y;

                    xf = pttrack->m_End.x;
                    yf = pttrack->m_End.y;

                    dx = xf - x0;
                    dy = yf - y0;

                    /* Compute the segment orientation (angle) en 0,1 degre */
                    angle = ArcTangente( dy, dx );

                    /* Compute the segment lenght: dx = longueur */
                    RotatePoint( &dx, &dy, angle );

                    /* Comute the reference segment coordinates relatives to a
                     *  X axis = current tested segment */
                    rx0 = pt_segment->m_Start.x - x0;
                    ry0 = pt_segment->m_Start.y - y0;
                    rxf = pt_segment->m_End.x - x0;
                    ryf = pt_segment->m_End.y - y0;

                    RotatePoint( &rx0, &ry0, angle );
                    RotatePoint( &rxf, &ryf, angle );
                    if( TestMarginToCircle( rx0, ry0, w_dist, dx ) == BAD_DRC )
                    {
                        ErrorsDRC_Count++;
                        if( show_err )
                            Affiche_Erreur_DRC( frame->DrawPanel,
                                                DC,
                                                frame->m_Pcb,
                                                pt_segment,
                                                pttrack,
                                                10 );
                        return BAD_DRC;
                    }
                    if( TestMarginToCircle( rxf, ryf, w_dist, dx ) == BAD_DRC )
                    {
                        ErrorsDRC_Count++;
                        if( show_err )
                            Affiche_Erreur_DRC( frame->DrawPanel,
                                                DC,
                                                frame->m_Pcb,
                                                pt_segment,
                                                pttrack,
                                                11 );
                        return BAD_DRC;
                    }
                }
            }
        }
    }

    return OK_DRC;
}


/*****************************************************************************/
static bool Test_Pad_to_Pads_Drc( WinEDA_BasePcbFrame* frame,
                                  wxDC* DC,
                                  D_PAD* pad_ref,
                                  LISTE_PAD* start_buffer,
                                  LISTE_PAD* end_buffer,
                                  int max_size,
                                  bool show_err )
/*****************************************************************************/

/** Test the drc between pad_ref and other pads.
 * the pad list must be sorted by x coordinate
 * @param frame = current active frame
 * @param DC = current DC
 * @param pad_ref = pad to test
 * @param end_buffer = upper limit of the pad list.
 * @param max_size = size of the biggest pad (used to stop the test when the X distance is > max_size)
 * @param show_err if true, display a marker and amessage.
 */
{
    int        MaskLayer;
    D_PAD*     pad;
    LISTE_PAD* pad_list = start_buffer;

    MaskLayer = pad_ref->m_Masque_Layer & ALL_CU_LAYERS;
    int        x_limite = max_size + g_DesignSettings.m_TrackClearence +
                          pad_ref->m_Rayon + pad_ref->m_Pos.x;

    for( ; pad_list < end_buffer; pad_list++ )
    {
        pad = *pad_list;
        if( pad == pad_ref )
            continue;

        /* We can stop the test when pad->m_Pos.x > x_limite
         *  because the list is sorted by X values */
        if( pad->m_Pos.x > x_limite )
            break;

        /* No probleme if pads are on different copper layers */
        if( (pad->m_Masque_Layer & MaskLayer ) == 0 )
            continue;

        /* The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
         *  But no problem if pads have the same netcode (same net)*/
        if( pad->GetNet() && (pad_ref->GetNet() == pad->GetNet()) )
            continue;

        /* No problem if pads are from the same footprint
         *  and have the same pad number ( equivalent pads  )  */
        if( (pad->m_Parent == pad_ref->m_Parent) && (pad->m_NumPadName == pad_ref->m_NumPadName) )
            continue;

        if( Pad_to_Pad_Isol( pad_ref, pad, g_DesignSettings.m_TrackClearence ) == OK_DRC )
            continue;

        else    /* here we have a drc error! */
        {
            ErrorsDRC_Count++;
            if( show_err )
                Affiche_Erreur_DRC( frame->DrawPanel, DC, frame->m_Pcb, pad_ref, pad );
            return BAD_DRC;
        }
    }

    return OK_DRC;
}


/**************************************************************************************/
static int Pad_to_Pad_Isol( D_PAD* pad_ref, D_PAD* pad, const int dist_min )
/***************************************************************************************/

/* Return OK_DRC si clearance between pad_ref and pad is >= dist_min
 *  or BAD_DRC if not */
{
    wxPoint rel_pos;
    int     dist, diag;
    wxPoint shape_pos;
    int     pad_angle;

    rel_pos   = pad->ReturnShapePos();
    shape_pos = pad_ref->ReturnShapePos();

    // rel_pos is pad position relative to the pad_ref position
    rel_pos.x -= shape_pos.x;
    rel_pos.y -= shape_pos.y;
    dist = (int) hypot( (double) rel_pos.x, (double) rel_pos.y );

    diag = OK_DRC;

    /* tst rapide: si les cercles exinscrits sont distants de dist_min au moins,
     *  il n'y a pas de risque: */
    if( (dist - pad_ref->m_Rayon - pad->m_Rayon) >= dist_min )
        return OK_DRC;

    /* Ici les pads sont proches et les cercles exinxcrits sont trop proches
     *  Selon les formes relatives il peut y avoir ou non erreur */

    bool swap_pads = false;
    if( (pad_ref->m_PadShape != CIRCLE) && (pad->m_PadShape == CIRCLE) )
        swap_pads = true;
    else if( (pad_ref->m_PadShape != OVALE) && (pad->m_PadShape == OVALE) )
        swap_pads = true;

    if( swap_pads )
    {
        EXCHG( pad_ref, pad );
        rel_pos.x = -rel_pos.x;
        rel_pos.y = -rel_pos.y;
    }

    switch( pad_ref->m_PadShape )
    {
    case CIRCLE:        // pad_ref is like a track segment with a null lenght
        segm_long  = 0;
        segm_angle = 0;
        finx    = finy = 0;
        spot_cX = rel_pos.x;
        spot_cY = rel_pos.y;
        diag    = TestClearanceSegmToPad( pad, pad_ref->m_Rayon, dist_min );
        break;

    case RECT:
        RotatePoint( &rel_pos.x, &rel_pos.y, pad_ref->m_Orient );
        pad_angle = pad_ref->m_Orient + pad->m_Orient;      // pad_angle = pad orient relative to the pad_ref orient
        NORMALIZE_ANGLE_POS( pad_angle );
        if( pad->m_PadShape == RECT )
        {
            wxSize size = pad->m_Size;
            if( (pad_angle == 0) || (pad_angle == 900) || (pad_angle == 1800) ||
               (pad_angle == 2700) )
            {
                if( (pad_angle == 900) || (pad_angle == 2700) )
                {
                    EXCHG( size.x, size.y );
                }

                // Test DRC:
                diag      = BAD_DRC;

                rel_pos.x = ABS( rel_pos.x );
                rel_pos.y = ABS( rel_pos.y );

                if( ( rel_pos.x - ( (size.x + pad_ref->m_Size.x) / 2 ) ) >= dist_min )
                    diag = OK_DRC;

                if( ( rel_pos.y - ( (size.y + pad_ref->m_Size.y) / 2 ) ) >= dist_min )
                    diag = OK_DRC;
            }
            else        // Any other orient
            {
                        /* TODO : any orient ... */
            }
        }
        break;

    case OVALE:     /* an oval pad is like a track segment */
    {
        /* Create and test a track segment with same dimensions */
        int segm_width;
        segm_angle = pad_ref->m_Orient;                     // Segment orient.
        if( pad_ref->m_Size.y < pad_ref->m_Size.x )         /* We suppose the pad is an horizontal oval */
        {
            segm_width = pad_ref->m_Size.y;
            segm_long  = pad_ref->m_Size.x - pad_ref->m_Size.y;
        }
        else        // it was a vertical oval, change to a rotated horizontal one
        {
            segm_width  = pad_ref->m_Size.x;
            segm_long   = pad_ref->m_Size.y - pad_ref->m_Size.x;
            segm_angle += 900;
        }
        /* the start point must be 0,0 and currently rel_pos is relative the center of pad coordinate */
        int sx = -segm_long / 2, sy = 0;        // Start point coordinate of the horizontal equivalent segment
        RotatePoint( &sx, &sy, segm_angle );    // True start point coordinate of the equivalent segment
        spot_cX = rel_pos.x + sx;
        spot_cY = rel_pos.y + sy;               // pad position / segment origin
        finx    = -sx;
        finy    = -sy;                          // end of segment coordinate
        diag    = TestClearanceSegmToPad( pad, segm_width / 2, dist_min );
        break;
    }

    default:
        /* TODO...*/
        break;
    }

    return diag;
}


/***************************************************************************/
static int TestClearanceSegmToPad( const D_PAD* pad_to_test, int w_segm, int dist_min )
/****************************************************************************/

/*
 *  Routine adaptee de la "distance()" (LOCATE.CPP)
 *  teste la distance du pad au segment de droite en cours
 *
 *  retourne:
 *      0 si distance >= dist_min
 *      1 si distance < dist_min
 *  Parametres d'appel:
 *      pad_to_test	= pointeur sur le pad a tester
 *      w_segm = demi largeur du segment a tester
 *      dist_min = marge a respecter
 *
 *  en variables globales
 *      segm_long = longueur du segment en test
 *      segm_angle = angle d'inclinaison du segment;
 *      finx, finy = coord fin du segment / origine
 *      spot_cX, spot_cY = position du pad / origine du segment
 */
{
    int p_dimx, p_dimy; /* demi - dimensions X et Y du pad a controler */
    int bflag;
    int orient;
    int x0, y0, xf, yf;
    int seuil;
    int deltay;

    seuil  = w_segm + dist_min;
    p_dimx = pad_to_test->m_Size.x >> 1;
    p_dimy = pad_to_test->m_Size.y >> 1;

    if( pad_to_test->m_PadShape == CIRCLE )
    {
        /* calcul des coord centre du pad dans le repere axe X confondu
         *  avec le segment en tst */
        RotatePoint( &spot_cX, &spot_cY, segm_angle );
        return TestMarginToCircle( spot_cX, spot_cY, seuil + p_dimx, segm_long );
    }
    else
    {
        /* calcul de la "surface de securite" du pad de reference */
        xcliplo = spot_cX - seuil - p_dimx;
        ycliplo = spot_cY - seuil - p_dimy;
        xcliphi = spot_cX + seuil + p_dimx;
        ycliphi = spot_cY + seuil + p_dimy;

        x0 = y0 = 0;

        xf = finx;
        yf = finy;

        orient = pad_to_test->m_Orient;

        RotatePoint( &x0, &y0, spot_cX, spot_cY, -orient );
        RotatePoint( &xf, &yf, spot_cX, spot_cY, -orient );

        bflag = Tst_Ligne( x0, y0, xf, yf );

        if( bflag == OK_DRC )
            return OK_DRC;

        /* Erreur DRC : analyse fine de la forme de la pastille */

        switch( pad_to_test->m_PadShape )
        {
        default:
            return BAD_DRC;

        case OVALE:
            /* test de la pastille ovale ramenee au type ovale vertical */
            if( p_dimx > p_dimy )
            {
                EXCHG( p_dimx, p_dimy ); orient += 900;
                if( orient >= 3600 )
                    orient -= 3600;
            }
            deltay = p_dimy - p_dimx;

            /* ici: p_dimx = rayon,
             *      delta = dist centre cercles a centre pad */

            /* Test du rectangle separant les 2 demi cercles */
            xcliplo = spot_cX - seuil - p_dimx;
            ycliplo = spot_cY - w_segm - deltay;
            xcliphi = spot_cX + seuil + p_dimx;
            ycliphi = spot_cY + w_segm + deltay;

            bflag = Tst_Ligne( x0, y0, xf, yf );
            if( bflag == BAD_DRC )
                return BAD_DRC;

            /* test des 2 cercles */
            x0 = spot_cX;     /* x0,y0 = centre du cercle superieur du pad ovale */
            y0 = spot_cY + deltay;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );

            bflag = TestMarginToCircle( x0, y0, p_dimx + seuil, segm_long );
            if( bflag == BAD_DRC )
                return BAD_DRC;

            x0 = spot_cX;     /* x0,y0 = centre du cercle inferieur du pad ovale */
            y0 = spot_cY - deltay;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );

            bflag = TestMarginToCircle( x0, y0, p_dimx + seuil, segm_long );
            if( bflag == BAD_DRC )
                return BAD_DRC;
            break;

        case RECT:      /* 2 rectangle + 4 1/4 cercles a tester */
            /* Test du rectangle dimx + seuil, dimy */
            xcliplo = spot_cX - p_dimx - seuil;
            ycliplo = spot_cY - p_dimy;
            xcliphi = spot_cX + p_dimx + seuil;
            ycliphi = spot_cY + p_dimy;

            bflag = Tst_Ligne( x0, y0, xf, yf );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            /* Test du rectangle dimx , dimy + seuil */
            xcliplo = spot_cX - p_dimx;
            ycliplo = spot_cY - p_dimy - seuil;
            xcliphi = spot_cX + p_dimx;
            ycliphi = spot_cY + p_dimy + seuil;

            bflag = Tst_Ligne( x0, y0, xf, yf );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            /* test des 4 cercles ( surface d'solation autour des sommets */
            /* test du coin sup. gauche du pad */
            x0 = spot_cX - p_dimx;
            y0 = spot_cY - p_dimy;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );
            bflag = TestMarginToCircle( x0, y0, seuil, segm_long );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            /* test du coin sup. droit du pad */
            x0 = spot_cX + p_dimx;
            y0 = spot_cY - p_dimy;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );
            bflag = TestMarginToCircle( x0, y0, seuil, segm_long );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            /* test du coin inf. gauche du pad */
            x0 = spot_cX - p_dimx;
            y0 = spot_cY + p_dimy;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );
            bflag = TestMarginToCircle( x0, y0, seuil, segm_long );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            /* test du coin inf. droit du pad */
            x0 = spot_cX + p_dimx;
            y0 = spot_cY + p_dimy;
            RotatePoint( &x0, &y0, spot_cX, spot_cY, orient );
            RotatePoint( &x0, &y0, segm_angle );
            bflag = TestMarginToCircle( x0, y0, seuil, segm_long );
            if( bflag == BAD_DRC )
            {
                return BAD_DRC;
            }

            break;
        }
    }
    return OK_DRC;
}


/*******************************************************************/
static int TestMarginToCircle( int cx, int cy, int rayon, int longueur )
/*******************************************************************/

/*
 *  Routine analogue a TestClearanceSegmToPad.
 *  Calcul de la distance d'un cercle (via ronde, extremite de piste)
 *   au segment de droite en cours de controle (segment de reference dans
 *   son repere )
 *  parametres:
 *      cx, cy: centre du cercle (surface ronde) a tester, dans le repere
 *                          segment de reference
 *      rayon = rayon du cercle
 *      longueur = longueur du segment dans son repere (i.e. coord de fin)
 *  retourne:
 *      OK_DRC si distance >= rayon
 *      BAD_DRC si distance < rayon
 */
{
    if( abs( cy ) > rayon )
        return OK_DRC;

    if( (cx >= -rayon ) && ( cx <= (longueur + rayon) ) )
    {
        if( (cx >= 0) && (cx <= longueur) )
            return BAD_DRC;
        if( cx > longueur )
            cx -= longueur;
        if( hypot( (double) cx, (double) cy ) < rayon )
            return BAD_DRC;
    }

    return OK_DRC;
}


/******************************************************************************/
static void Affiche_Erreur_DRC( WinEDA_DrawPanel* panel, wxDC* DC, BOARD* Pcb,
                                TRACK* pt_ref, BOARD_ITEM* pt_item, int errnumber )
/******************************************************************************/

/* affiche les erreurs de DRC :
 *  Message d'erreur
 +
 *  Marqueur
 *  number = numero d'identification
 */
{
    wxPoint  erc_pos;
    TRACK*   pt_segm;
    wxString msg;
    wxString tracktype, netname1, netname2;
    EQUIPOT* equipot = Pcb->FindNet( pt_ref->GetNet() );

    if( equipot )
        netname1 = equipot->m_Netname;
    else
        netname1 = wxT( "<noname>" );

    netname2 = wxT( "<noname>" );

    if( pt_ref->Type() == TYPEVIA )
        tracktype = wxT( "Via" );

    else if( pt_ref->Type() == TYPEZONE )
        tracktype = wxT( "Zone" );

    else
        tracktype = wxT( "Track" );

    if( pt_item->Type() == TYPEPAD )
    {
        D_PAD* pad = (D_PAD*) pt_item;
        equipot = Pcb->FindNet( pad->GetNet() );
        if( equipot )
            netname2 = equipot->m_Netname;

        erc_pos = pad->m_Pos;
        wxString pad_name    = pad->ReturnStringPadName();

        wxString module_name = ( (MODULE*) (pad->m_Parent) )->m_Reference->m_Text;

        msg.Printf( _( "%d Drc Err %d %s (net %s) and PAD %s (%s) net %s @ %d,%d\n" ),
                    ErrorsDRC_Count, errnumber, tracktype.GetData(),
                    netname1.GetData(),
                    pad_name.GetData(), module_name.GetData(),
                    netname2.GetData(),
                    erc_pos.x, erc_pos.y );
    }

    else    /* erreur sur segment de piste */
    {
        pt_segm = (TRACK*) pt_item;
        equipot = Pcb->FindNet( pt_segm->GetNet() );
        if( equipot )
            netname2 = equipot->m_Netname;
        erc_pos = pt_segm->m_Start;
        if( pt_segm->Type() == TYPEVIA )
        {
            msg.Printf( _( "%d Err type %d: %s (net %s) and VIA (net %s) @ %d,%d\n" ),
                        ErrorsDRC_Count, errnumber, tracktype.GetData(),
                        netname1.GetData(), netname2.GetData(),
                        erc_pos.x, erc_pos.y );
        }
        else
        {
            wxPoint erc_pos_f = pt_segm->m_End;
            if( hypot( (double) (erc_pos_f.x - pt_ref->m_End.x),
                      (double) (erc_pos_f.y - pt_ref->m_End.y) )
               < hypot( (double) (erc_pos.x - pt_ref->m_End.x),
                       (double) (erc_pos.y - pt_ref->m_End.y) ) )
            {
                EXCHG( erc_pos_f.x, erc_pos.x );
                EXCHG( erc_pos_f.y, erc_pos.y );
            }
            msg.Printf( _( "%d Err type %d: %s (net %s) and track (net %s) @ %d,%d\n" ),
                        ErrorsDRC_Count, errnumber, tracktype.GetData(),
                        netname1.GetData(), netname2.GetData(),
                        erc_pos.x, erc_pos.y );
        }
    }

    if( DrcFrame )
    {
//        DrcFrame->m_logWindow->AppendText( msg );
    }
    else
        panel->m_Parent->Affiche_Message( msg );

    if( s_RptFile )
        fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );

    if( current_marqueur == NULL )
        current_marqueur = new MARKER( Pcb );

    current_marqueur->m_Pos   = wxPoint( erc_pos.x, erc_pos.y );
    current_marqueur->m_Color = WHITE;
    current_marqueur->SetMessage( msg );
    current_marqueur->Draw( panel, DC, GR_OR );
}


/******************************************************************************/
static void Affiche_Erreur_DRC( WinEDA_DrawPanel* panel, wxDC* DC, BOARD* Pcb,
                                D_PAD* pad1, D_PAD* pad2 )
/******************************************************************************/

/* affiche les erreurs de DRC :
 *  Message d'erreur
 +
 *  Marqueur
 *  number = numero d'identification
 */
{
    wxString msg;

    wxString pad_name1    = pad1->ReturnStringPadName();
    wxString module_name1 = ( (MODULE*) (pad1->m_Parent) )->m_Reference->m_Text;
    wxString pad_name2    = pad2->ReturnStringPadName();
    wxString module_name2 = ( (MODULE*) (pad2->m_Parent) )->m_Reference->m_Text;
    wxString netname1, netname2;

    EQUIPOT* equipot = Pcb->FindNet( pad1->GetNet() );

    if( equipot )
        netname1 = equipot->m_Netname;
    else
        netname1 = wxT( "<noname>" );

    equipot = Pcb->FindNet( pad2->GetNet() );
    if( equipot )
        netname2 = equipot->m_Netname;
    else
        netname2 = wxT( "<noname>" );

    msg.Printf( _( "%d Drc Err: PAD %s (%s) net %s @ %d,%d and PAD %s (%s) net %s @ %d,%d\n" ),
        ErrorsDRC_Count,
        pad_name1.GetData(), module_name1.GetData(), netname1.GetData(), pad1->m_Pos.x, pad1->m_Pos.y,
        pad_name2.GetData(), module_name2.GetData(), netname2.GetData(), pad2->m_Pos.x, pad2->m_Pos.y );

    if( DrcFrame )
    {
//        DrcFrame->m_logWindow->AppendText( msg );
    }
    else
        panel->m_Parent->Affiche_Message( msg );

    if( s_RptFile )
        fprintf( s_RptFile, "%s", CONV_TO_UTF8( msg ) );

    if( current_marqueur == NULL )
        current_marqueur = new MARKER( Pcb );

    current_marqueur->m_Pos   = pad1->m_Pos;
    current_marqueur->m_Color = WHITE;
    current_marqueur->SetMessage( msg );
    current_marqueur->Draw( panel, DC, GR_OR );
}


/**********************************************/
/* int Tst_Ligne(int x1,int y1,int x2,int y2) */
/**********************************************/

/* Routine utilisee pour tester si une piste est en contact avec une autre piste.
 *
 *  Cette routine controle si la ligne (x1,y1 x2,y2) a une partie s'inscrivant
 *  dans le cadre (xcliplo,ycliplo xcliphi,ycliphi) (variables globales,
 *  locales a ce fichier)
 *
 *  Retourne OK_DRC si aucune partie commune
 *  Retourne BAD_DRC si partie commune
 */
static inline int USCALE( unsigned arg, unsigned num, unsigned den )
{
    int ii;

    ii = (int) ( ( (double) arg * num ) / den );
    return ii;
}


#define WHEN_OUTSIDE return (OK_DRC)
#define WHEN_INSIDE

static int Tst_Ligne( int x1, int y1, int x2, int y2 )
{
    int temp;

    if( x1 > x2 )
    {
        EXCHG( x1, x2 );
        EXCHG( y1, y2 );
    }
    if( (x2 < xcliplo) || (x1 > xcliphi) )
    {
        WHEN_OUTSIDE;
    }
    if( y1 < y2 )
    {
        if( (y2 < ycliplo) || (y1 > ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 < ycliplo )
        {
            temp = USCALE( (x2 - x1), (ycliplo - y1), (y2 - y1) );
            if( (x1 += temp) > xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = ycliplo;
            WHEN_INSIDE;
        }
        if( y2 > ycliphi )
        {
            temp = USCALE( (x2 - x1), (y2 - ycliphi), (y2 - y1) );
            if( (x2 -= temp) < xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = ycliphi;
            WHEN_INSIDE;
        }
        if( x1 < xcliplo )
        {
            temp = USCALE( (y2 - y1), (xcliplo - x1), (x2 - x1) );
            y1  += temp; x1 = xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > xcliphi )
        {
            temp = USCALE( (y2 - y1), (x2 - xcliphi), (x2 - x1) );
            y2  -= temp; x2 = xcliphi;
            WHEN_INSIDE;
        }
    }
    else
    {
        if( (y1 < ycliplo) || (y2 > ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 > ycliphi )
        {
            temp = USCALE( (x2 - x1), (y1 - ycliphi), (y1 - y2) );
            if( (x1 += temp) > xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = ycliphi;
            WHEN_INSIDE;
        }
        if( y2 < ycliplo )
        {
            temp = USCALE( (x2 - x1), (ycliplo - y2), (y1 - y2) );
            if( (x2 -= temp) < xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = ycliplo;
            WHEN_INSIDE;
        }
        if( x1 < xcliplo )
        {
            temp = USCALE( (y1 - y2), (xcliplo - x1), (x2 - x1) );
            y1  -= temp;
            x1 = xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > xcliphi )
        {
            temp = USCALE( (y1 - y2), (x2 - xcliphi), (x2 - x1) );
            y2  += temp;
            x2 = xcliphi;
            WHEN_INSIDE;
        }
    }

    if( ( (x2 + x1)/2 <= xcliphi ) && ( (x2 + x1)/2 >= xcliplo ) \
     && ( (y2 + y1)/2 <= ycliphi ) && ( (y2 + y1)/2 >= ycliplo ) )
    {
        return BAD_DRC;
    }
    else
        return OK_DRC;
}

#endif
