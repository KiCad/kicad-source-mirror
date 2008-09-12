/* filling_zone_algorithm:
 * Algos used to fill a zone defined by a polygon and a filling starting point
 */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "autorout.h"
#include "cell.h"
#include "trigo.h"
#include "protos.h"

/* Local functions */
static void Genere_Segments_Zone( WinEDA_PcbFrame* frame, wxDC* DC, int net_code, int layer );

/* Local variables */
static bool          Zone_Debug = false;
static unsigned long s_TimeStamp; /* Time stamp common to all segments relative to the new created zone */

/*****************************************************************************/
int ZONE_CONTAINER::Fill_Zone( WinEDA_PcbFrame* frame, wxDC* DC, bool verbose )
/*****************************************************************************/

/** Function Fill_Zone()
 *  Calculate the zone filling
 *  The zone outline is a frontier, and can be complex (with holes)
 *  The filling starts from starting points like pads, tracks.
 * @param frame = reference to the main frame
 * @param DC = current Device Context (can be NULL)
 * @param verbose = true to show error messages
 * @return error level (0 = no error)
 */
{
    int         ii, jj;
    int         error_level = 0;
    int         lp_tmp, lay_tmp_TOP, lay_tmp_BOTTOM;
    int         save_isol = g_DesignSettings.m_TrackClearence;
    wxPoint     ZoneStartFill;
    wxString    msg;
    BOARD*      Pcb    = frame->m_Pcb;

	// Set the g_DesignSettings.m_TrackClearence (used to fill board map) to the max of m_TrackClearence and m_ZoneClearence
    g_DesignSettings.m_TrackClearence = max ( g_DesignSettings.m_TrackClearence, g_DesignSettings.m_ZoneClearence);

	// In order to avoid ends of segments used to fill the zone, and to the clearence the radius of ends
	// which is g_GridRoutingSize/2
	g_DesignSettings.m_TrackClearence += g_GridRoutingSize/2;

    g_HightLigth_NetCode = m_NetCode;

    s_TimeStamp = m_TimeStamp;

    // Delete the old filling, if any :
    frame->Delete_Zone_Fill( DC, NULL, m_TimeStamp );

    // calculate the fixed step of the routing matrix as 25 mils or more
    E_scale = g_GridRoutingSize / 25;

    if( g_GridRoutingSize < 1 )
        g_GridRoutingSize = 1;

    // calculate the Ncols and Nrows, size of the routing matrix
    ComputeMatriceSize( frame, g_GridRoutingSize );

    // create the routing matrix in autorout.h's eda_global BOARDHEAD Board
    Nb_Sides = ONE_SIDE;
    if( Board.InitBoard() < 0 )
    {
        if( verbose )
            DisplayError( frame, wxT( "Mo memory for creating zones" ) );
        error_level = 1;
        return error_level;
    }

    msg.Printf( wxT( "%d" ), Ncols );
    Affiche_1_Parametre( frame, 1, wxT( "Cols" ), msg, GREEN );

    msg.Printf( wxT( "%d" ), Nrows );
    Affiche_1_Parametre( frame, 7, wxT( "Lines" ), msg, GREEN );

    msg.Printf( wxT( "%d" ), Board.m_MemSize / 1024 );
    Affiche_1_Parametre( frame, 14, wxT( "Mem(Ko)" ), msg, CYAN );

    lay_tmp_BOTTOM = Route_Layer_BOTTOM;
    lay_tmp_TOP    = Route_Layer_TOP;

    Route_Layer_BOTTOM = Route_Layer_TOP = m_Layer;
    lp_tmp = g_DesignSettings.m_CurrentTrackWidth;
    g_DesignSettings.m_CurrentTrackWidth = g_GridRoutingSize;


    // trace the pcb edges (pcb contour) into the routing matrix
    Route_Layer_BOTTOM = Route_Layer_TOP = EDGE_N;
    PlaceCells( Pcb, -1, 0 );
    Route_Layer_BOTTOM = Route_Layer_TOP = m_Layer;

    /* Create the starting point for the zone:
     * The starting point and all the tracks are suitable "starting points" */
    TRACK* pt_segm = Pcb->m_Track;
    for( ; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( g_HightLigth_NetCode != pt_segm->GetNet() )
            continue;

        if( ! pt_segm->IsOnLayer( m_Layer ) )
            continue;

//        if( pt_segm->Type() != TYPETRACK )
//            continue;

        TraceSegmentPcb( Pcb, pt_segm, CELL_is_FRIEND, 0, WRITE_CELL );
    }
    // trace the zone edges into the routing matrix
    int        i_start_contour = 0;
    for( unsigned ic = 0; ic < m_Poly->corner.size(); ic++ )
    {
        int xi = m_Poly->corner[ic].x - Pcb->m_BoundaryBox.m_Pos.x;
        int yi = m_Poly->corner[ic].y - Pcb->m_BoundaryBox.m_Pos.y;
        int xf, yf;
        if( m_Poly->corner[ic].end_contour == FALSE && ic < m_Poly->corner.size() - 1 )
        {
            xf = m_Poly->corner[ic + 1].x - Pcb->m_BoundaryBox.m_Pos.x;
            yf = m_Poly->corner[ic + 1].y - Pcb->m_BoundaryBox.m_Pos.y;
        }
        else
        {
            xf = m_Poly->corner[i_start_contour].x - Pcb->m_BoundaryBox.m_Pos.x;
            yf = m_Poly->corner[i_start_contour].y - Pcb->m_BoundaryBox.m_Pos.y;
            i_start_contour = ic + 1;
        }
        TraceLignePcb( xi, yi, xf, yf, -1, HOLE | CELL_is_EDGE, WRITE_CELL );
    }

    /* Create a starting point to create the zone filling, from pads */
    LISTE_PAD* pad;
    int        cells_count = 0;
    for( ii = 0, pad = frame->m_Pcb->m_Pads; ii < frame->m_Pcb->m_NbPads; ii++, pad++ )
    {
		if ( ! (*pad)->IsOnLayer( GetLayer() ) ) continue;
		if ( (*pad)->GetNet() != GetNet() ) continue;
        wxPoint pos = (*pad)->m_Pos;
        if( m_Poly->TestPointInside( pos.x, pos.y ) )
        {
			pos -= Pcb->m_BoundaryBox.m_Pos;
			ZoneStartFill.x = pos.x / g_GridRoutingSize;

            ZoneStartFill.y = pos.y / g_GridRoutingSize;
			BoardCell cell = GetCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM );
			if ( (cell & CELL_is_EDGE) == 0 )
			{
				OrCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM, CELL_is_ZONE );
				cells_count++;
			}
        }
    }

    /* Create a starting point to create the zone filling, from vias and tracks */
    TRACK* track;
    for( track = frame->m_Pcb->m_Track; track != NULL; track = track->Next() )
    {
		if ( ! track->IsOnLayer( GetLayer() ) ) continue;
		if ( track->GetNet() != GetNet() ) continue;
        wxPoint pos = track->m_Start;
        if( m_Poly->TestPointInside( pos.x, pos.y ) )
        {
			pos -= Pcb->m_BoundaryBox.m_Pos;
			ZoneStartFill.x = pos.x / g_GridRoutingSize;

            ZoneStartFill.y = pos.y / g_GridRoutingSize;
			BoardCell cell = GetCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM );
			if ( (cell & CELL_is_EDGE) == 0 )
			{
				OrCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM, CELL_is_ZONE );
				cells_count++;
			}
        }
        pos = track->m_End;
        if( m_Poly->TestPointInside( pos.x, pos.y ) )
        {
			pos -= Pcb->m_BoundaryBox.m_Pos;
			ZoneStartFill.x = pos.x / g_GridRoutingSize;

            ZoneStartFill.y = pos.y / g_GridRoutingSize;
			BoardCell cell = GetCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM );
			if ( (cell & CELL_is_EDGE) == 0 )
			{
				OrCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM, CELL_is_ZONE );
				cells_count++;
			}
        }
    }

    if( cells_count == 0 )
    {
        if( verbose )
		{
			msg = _( "No pads or starting point found to fill this zone outline" );
			msg << wxT("\n");
			msg << MenuText( frame->m_Pcb );
            DisplayError( frame, msg );
		}
        error_level = 2;
        goto end_of_zone_fill;
    }

    // mark the cells forming part of the zone
    ii = 1; jj = 1;
    while( ii )
    {
        msg.Printf( wxT( "%d" ), jj++ );
        Affiche_1_Parametre( frame, 50, wxT( "Iter." ), msg, CYAN );
        ii = Propagation( frame );
    }

    // selection of the suitable cells for the points of anchoring of the zone
    for( ii = 0; ii < Nrows; ii++ )
    {
        for( jj = 0; jj < Ncols; jj++ )
        {
            long cell = GetCell( ii, jj, BOTTOM );
            if( (cell & CELL_is_ZONE) )
            {
                if( (cell & CELL_is_FRIEND) == 0 )
                    AndCell( ii, jj, BOTTOM, (BoardCell) ~(CELL_is_FRIEND | CELL_is_ZONE) );
            }
        }
    }

    if( Zone_Debug && DC )
    {
        DisplayBoard( frame->DrawPanel, DC );
    }
    // now, all the cell candidates are marked

    // place all the obstacles into the matrix, such as (pads, tracks, vias,
    // pcb edges or segments)
    ii = 0;
    if( m_PadOption == PAD_NOT_IN_ZONE )
        ii = FORCE_PADS;

    Affiche_1_Parametre( frame, 42, wxT( "GenZone" ), wxEmptyString, RED );
    PlaceCells( Pcb, g_HightLigth_NetCode, ii );
    Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Ok" ), RED );

    /* Recreate zone limits on the routing matrix
     *  (could be deleted by PlaceCells()) : */
    i_start_contour = 0;
    for( unsigned ic = 0; ic < m_Poly->corner.size(); ic++ )
    {
        int xi = m_Poly->corner[ic].x - Pcb->m_BoundaryBox.m_Pos.x;
        int yi = m_Poly->corner[ic].y - Pcb->m_BoundaryBox.m_Pos.y;
        int xf, yf;
        if( m_Poly->corner[ic].end_contour == FALSE && ic < m_Poly->corner.size() - 1 )
        {
            xf = m_Poly->corner[ic + 1].x - Pcb->m_BoundaryBox.m_Pos.x;
            yf = m_Poly->corner[ic + 1].y - Pcb->m_BoundaryBox.m_Pos.y;
        }
        else
        {
            xf = m_Poly->corner[i_start_contour].x - Pcb->m_BoundaryBox.m_Pos.x;
            yf = m_Poly->corner[i_start_contour].y - Pcb->m_BoundaryBox.m_Pos.y;
            i_start_contour = ic + 1;
        }
        TraceLignePcb( xi, yi, xf, yf, -1, HOLE | CELL_is_EDGE, WRITE_CELL );
    }

    /* Init the starting point for zone filling : this is the mouse position
     *  (could be deleted by PlaceCells()) : */
    for( ii = 0, pad = frame->m_Pcb->m_Pads; ii < frame->m_Pcb->m_NbPads; ii++, pad++ )
    {
		if ( ! (*pad)->IsOnLayer( GetLayer() ) ) continue;
		if ( (*pad)->GetNet() != GetNet() ) continue;
        wxPoint pos = (*pad)->m_Pos;
        if( m_Poly->TestPointInside( pos.x, pos.y ) )
        {
			pos -= Pcb->m_BoundaryBox.m_Pos;
            ZoneStartFill.x = pos.x / g_GridRoutingSize;

            ZoneStartFill.y = pos.y / g_GridRoutingSize;
			BoardCell cell = GetCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM );
			if ( (cell & CELL_is_EDGE) == 0 )
				OrCell( ZoneStartFill.y, ZoneStartFill.x, BOTTOM, CELL_is_ZONE );
        }
    }


    /* Filling the cells of the matrix (this is the zone building)*/
    ii = 1; jj = 1;
    while( ii )
    {
        msg.Printf( wxT( "%d" ), jj++ );
        Affiche_1_Parametre( frame, 50, wxT( "Iter." ), msg, CYAN );
        ii = Propagation( frame );
    }

    if( Zone_Debug && DC )
	{
        DisplayBoard( frame->DrawPanel, DC );
	}

    // replace obstacles into the matrix(pads)
    if( m_PadOption == THERMAL_PAD )
        PlaceCells( Pcb, g_HightLigth_NetCode, FORCE_PADS );

    if( Zone_Debug && DC )
        DisplayBoard( frame->DrawPanel, DC );

    /* Convert the matrix information (cells) to segments which are actually the zone */
    if( g_HightLigth_NetCode < 0 )
        Genere_Segments_Zone( frame, DC, 0, m_Layer );
    else
        Genere_Segments_Zone( frame, DC, g_HightLigth_NetCode, m_Layer );

    /* Create the thermal reliefs */
    g_DesignSettings.m_CurrentTrackWidth = lp_tmp;
    if( m_PadOption == THERMAL_PAD )
        frame->Genere_Pad_Connexion( DC, m_Layer );

end_of_zone_fill:
    g_DesignSettings.m_TrackClearence = save_isol;

    // free the memory
    Board.UnInitBoard();

    // restore original values unchanged
    Route_Layer_TOP    = lay_tmp_TOP;
    Route_Layer_BOTTOM = lay_tmp_BOTTOM;

    return error_level;
}


/*******************************************************************************************/
static void Genere_Segments_Zone( WinEDA_PcbFrame* frame, wxDC* DC, int net_code, int layer )
/*******************************************************************************************/

/** Function Genere_Segments_Zone()
 * Create the zone segments from the routing matrix structure
 *  Algorithm:
 * Search for consecutive cells (flagged "zone") , and create segments
 *  from the first cell to the last cell in the matrix
 *      2 searchs are made
 *          1 - From left to right and create horizontal zone segments
 *          2 - From top to bottom, and create vertical zone segmùents
 * @param net_code = net_code common to all segment zone created
 * @param DC = current device context ( can be NULL )
 * @param frame = current WinEDA_PcbFrame
 * global: parameter TimeStamp: time stamp common to all segment zone created
 */
{
    int      row, col;
    long     current_cell, old_cell;
    int      ux0  = 0, uy0 = 0, ux1 = 0, uy1 = 0;
    int      Xmin = frame->m_Pcb->m_BoundaryBox.m_Pos.x;
    int      Ymin = frame->m_Pcb->m_BoundaryBox.m_Pos.y;
    SEGZONE* pt_track;
    int      nbsegm = 0;
    wxString msg;

    /* Create horizontal segments */
    Affiche_1_Parametre( frame, 64, wxT( "Segm H" ), wxT( "0" ), BROWN );
    for( row = 0; row < Nrows; row++ )
    {
        old_cell = 0;
        uy0 = uy1 = (row * g_GridRoutingSize) + Ymin;
        for( col = 0; col < Ncols; col++ )
        {
            current_cell = GetCell( row, col, BOTTOM ) & CELL_is_ZONE;
            if( current_cell ) /* ce point doit faire partie d'un segment */
            {
                ux1 = (col * g_GridRoutingSize) + Xmin;
                if( old_cell == 0 )
                    ux0 = ux1;
            }

            if( !current_cell || (col == Ncols - 1) ) /* peut etre fin d'un segment */
            {
                if( (old_cell) && (ux0 != ux1) )
                {
                    /* un segment avait debute de longueur > 0 */
                    pt_track = new SEGZONE( frame->m_Pcb );
                    pt_track->SetLayer( layer );
                    pt_track->SetNet( net_code );

                    pt_track->m_Width = g_GridRoutingSize;

                    pt_track->m_Start.x = ux0;
                    pt_track->m_Start.y = uy0;

                    pt_track->m_End.x = ux1;
                    pt_track->m_End.y = uy1;

                    pt_track->m_TimeStamp = s_TimeStamp;

                    pt_track->Insert( frame->m_Pcb, NULL );
                    if ( DC )
						pt_track->Draw( frame->DrawPanel, DC, GR_OR );
                    nbsegm++;
                }
            }
            old_cell = current_cell;
        }

        msg.Printf( wxT( "%d" ), nbsegm );
        Affiche_1_Parametre( frame, -1, wxEmptyString, msg, BROWN );
    }

    /* Create vertical segments */
    Affiche_1_Parametre( frame, 72, wxT( "Segm V" ), wxT( "0" ), BROWN );
    for( col = 0; col < Ncols; col++ )
    {
        old_cell = 0;
        ux0 = ux1 = (col * g_GridRoutingSize) + Xmin;
        for( row = 0; row < Nrows; row++ )
        {
            current_cell = GetCell( row, col, BOTTOM ) & CELL_is_ZONE;
            if( current_cell ) /* ce point doit faire partie d'un segment */
            {
                uy1 = (row * g_GridRoutingSize) + Ymin;
                if( old_cell == 0 )
                    uy0 = uy1;
            }
            if( !current_cell || (row == Nrows - 1) )    /* peut etre fin d'un segment */
            {
                if( (old_cell) && (uy0 != uy1) )
                {
                    /* un segment avait debute de longueur > 0 */
                    pt_track = new SEGZONE( frame->m_Pcb );
                    pt_track->SetLayer( layer );
                    pt_track->m_Width = g_GridRoutingSize;
                    pt_track->SetNet( net_code );

                    pt_track->m_Start.x = ux0;
                    pt_track->m_Start.y = uy0;

                    pt_track->m_End.x = ux1;
                    pt_track->m_End.y = uy1;

                    pt_track->m_TimeStamp = s_TimeStamp;
                    pt_track->Insert( frame->m_Pcb, NULL );
                    if( DC )
                        pt_track->Draw( frame->DrawPanel, DC, GR_OR );
                    nbsegm++;
                }
            }
            old_cell = current_cell;
        }

        msg.Printf( wxT( "%d" ), nbsegm );
        Affiche_1_Parametre( frame, -1, wxEmptyString, msg, BROWN );
    }
}


/********************************************/
int Propagation( WinEDA_PcbFrame* frame )
/********************************************/

/** Function Propagation()
 * An important function to calculate zones
 * Uses the routing matrix to fill the cells within the zone
 * Search and mark cells within the zone, and agree with DRC options.
 * Requirements:
 * Start from an initial point, to fill zone
 * The zone must have no "copper island"
 *  Algorithm:
 *  If the current cell has a neightbour flagged as "cell in the zone", it
 *  become a cell in the zone
 *  The first point in the zone is the starting point
 *  4 searches within the matrix are made:
 *          1 - Left to right and top to bottom
 *          2 - Right to left and top to bottom
 *          3 - bottom to top and Right to left
 *          4 - bottom to top and Left to right
 *  Given the current cell, for each search, we consider the 2 neightbour cells
 *  the previous cell on the same line and the previous cell on the same column.
 *
 *  This funtion can request some iterations
 *  Iterations are made until no cell is added to the zone.
 *  @return: added cells count (i.e. which the attribute CELL_is_ZONE is set)
 */
{
    int       row, col, nn;
    long      current_cell, old_cell_H;
    int long* pt_cell_V;
    int       nbpoints = 0;

#define NO_CELL_ZONE (HOLE | CELL_is_EDGE | CELL_is_ZONE)
    wxString  msg;

    Affiche_1_Parametre( frame, 57, wxT( "Detect" ), msg, CYAN );
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "1" ), CYAN );

    // Alloc memory to handle 1 line or 1 colunmn on the routing matrix
    nn = MAX( Nrows, Ncols ) * sizeof(*pt_cell_V);
    pt_cell_V = (long*) MyMalloc( nn );

    /* search 1 : from left to right and top to bottom */
    memset( pt_cell_V, 0, nn );
    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;
        for( col = 0; col < Ncols; col++ )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    /* search 2 : from right to left and top to bottom */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "2" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;
        for( col = Ncols - 1; col >= 0; col-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    /* search 3 : from bottom to top and right to left balayage */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "3" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( col = Ncols - 1; col >= 0; col-- )
    {
        old_cell_H = 0;
        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    /* search 4 : from bottom to top and left to right */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "4" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( col = 0; col < Ncols; col++ )
    {
        old_cell_H = 0;
        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    MyFree( pt_cell_V );

    return nbpoints;
}


/*****************************************************************************/
bool WinEDA_PcbFrame::Genere_Pad_Connexion( wxDC* DC, int layer )
/*****************************************************************************/

/* Create the thermal relief for each pad in the zone:
 *  this is 4 small segments from the pad to the zone
 */
{
    int        ii, jj, Npads;
    D_PAD*     pt_pad;
    LISTE_PAD* pt_liste_pad;
    TRACK*     pt_track, * loctrack;
    int        angle;
    int        cX, cY, dx, dy;
    int        sommet[4][2];
    wxString   msg;

    if( m_Pcb->m_Zone == NULL )
        return FALSE;                                   /* error: no zone */

    if( m_Pcb->m_Zone->m_TimeStamp != s_TimeStamp )     /* error: this is not the new zone */
        return FALSE;

    /* Count the pads, i.e. the thermal relief to create count, and displays it */
    Affiche_1_Parametre( this, 50, wxT( "NPads" ), wxT( "    " ), CYAN );
    pt_liste_pad = (LISTE_PAD*) m_Pcb->m_Pads;
    for( ii = 0, Npads = 0; ii < m_Pcb->m_NbPads; ii++, pt_liste_pad++ )
    {
        pt_pad = *pt_liste_pad;

        /* Search pads relative to the selected net code */
        if( pt_pad->GetNet() != g_HightLigth_NetCode )
            continue;

        /* Is the pad on the active layer ? */
        if( (pt_pad->m_Masque_Layer & g_TabOneLayerMask[layer]) == 0 )
            continue;
        Npads++;
    }

    msg.Printf( wxT( "%d" ), Npads );
    Affiche_1_Parametre( this, -1, wxEmptyString, msg, CYAN );

    /* Create the thermal reliefs */
    Affiche_1_Parametre( this, 57, wxT( "Pads" ), wxT( "     " ), CYAN );
    pt_liste_pad = (LISTE_PAD*) m_Pcb->m_Pads;
    for( ii = 0, Npads = 0; ii < m_Pcb->m_NbPads; ii++, pt_liste_pad++ )
    {
        pt_pad = *pt_liste_pad;

        /* Search pads relative to the selected net code */
        if( pt_pad->GetNet() != g_HightLigth_NetCode )
            continue;
        /* Is the pad on the active layer ? */
        if( (pt_pad->m_Masque_Layer & g_TabOneLayerMask[layer]) == 0 )
            continue;

        /* Create the theram relief for the current pad */
        Npads++;

        msg.Printf( wxT( "%d" ), Npads );
        Affiche_1_Parametre( this, -1, wxEmptyString, msg, CYAN );

        cX = pt_pad->GetPosition().x;
        cY = pt_pad->GetPosition().y;

        dx = pt_pad->m_Size.x / 2;
        dy = pt_pad->m_Size.y / 2;

        dx += g_DesignSettings.m_TrackClearence + g_GridRoutingSize;
        dy += g_DesignSettings.m_TrackClearence + g_GridRoutingSize;

        if( pt_pad->m_PadShape == PAD_TRAPEZOID )
        {
            dx += abs( pt_pad->m_DeltaSize.y ) / 2;
            dy += abs( pt_pad->m_DeltaSize.x ) / 2;
        }

        /* calculate the 4 segment coordinates (starting from the pad centre cX,cY) */
        sommet[0][0] = 0; sommet[0][1] = -dy;
        sommet[1][0] = -dx; sommet[1][1] = 0;
        sommet[2][0] = 0; sommet[2][1] = dy;
        sommet[3][0] = dx; sommet[3][1] = 0;

        angle = pt_pad->m_Orient;
        for( jj = 0; jj < 4; jj++ )
        {
            RotatePoint( &sommet[jj][0], &sommet[jj][1], angle );

            pt_track = new SEGZONE( m_Pcb );

            pt_track->SetLayer( layer );
            pt_track->m_Width = g_DesignSettings.m_CurrentTrackWidth;
            pt_track->SetNet( g_HightLigth_NetCode );
            pt_track->start       = pt_pad;
            pt_track->m_Start.x   = cX; pt_track->m_Start.y = cY;
            pt_track->m_End.x     = cX + sommet[jj][0];
            pt_track->m_End.y     = cY + sommet[jj][1];
            pt_track->m_TimeStamp = s_TimeStamp;

            /* Test if the segment is allowed */
            if( BAD_DRC==m_drc->DrcBlind( pt_track, m_Pcb->m_Track ) )
            {
                // Drc error, retry with a smaller width
                // because some drc errors are due to a track width > filling zone size.
                pt_track->m_Width = g_GridRoutingSize;
                if( BAD_DRC==m_drc->DrcBlind( pt_track, m_Pcb->m_Track ) )
                {
                    delete pt_track;
                    continue;
                }
            }

            /* Search for a zone segment */
            loctrack = Locate_Zone( m_Pcb->m_Zone, pt_track->m_End, layer );
            if( (loctrack == NULL) || (loctrack->m_TimeStamp != s_TimeStamp) )
            {
                delete pt_track;
                continue;
            }

            pt_track->Insert( m_Pcb, NULL );
            if( DC )
                pt_track->Draw( DrawPanel, DC, GR_OR );
        }
    }

    return TRUE;
}
