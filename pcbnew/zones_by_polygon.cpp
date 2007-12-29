/////////////////////////////////////////////////////////////////////////////

// Name:        zones_by_polygon.cpp
// Licence:     GNU License
/////////////////////////////////////////////////////////////////////////////

#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma implementation "dialog_zones_by_polygon.h"
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

using namespace std;

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "autorout.h"
#include "cell.h"
#include "trigo.h"

#include "protos.h"

/* Imported functions */
void        Build_Zone( WinEDA_PcbFrame* frame, wxDC* DC, int net_code,
                        bool Zone_Exclude_Pads, bool Zone_Create_Thermal_Relief );

/* Local functions */
// Outile creation:
static void Abort_Zone_Create_Outline( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_New_Zone_Edge_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
// Corner moving
static void Abort_Zone_Move_Corner( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_Zone_Corner_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Local variables */
static bool Zone_45_Only      = FALSE;
static bool Zone_Exclude_Pads = TRUE;
static bool s_Zone_Create_Thermal_Relief = TRUE;
static int s_Zone_Layer;			// Layer used to create the current zone
static int s_Zone_Hatching;			// Option to show the zone area (outlines only, short hatches or full hatches
static int s_NetcodeSelection;		// Net code selection for the current zone
static wxPoint s_CornerInitialPosition;		// Used to abort a move corner command
static bool s_CornerIsNew;			// Used to abort a move corner command (if it is a new corner, it must be deleted)

// key used to store net sort option in config file :
#define ZONE_NET_SORT_OPTION_KEY wxT("Zone_NetSort_Opt")

enum zone_cmd {
	ZONE_ABORT,
	ZONE_OK
};

#include "dialog_zones_by_polygon.cpp"



/**********************************************************/
void WinEDA_PcbFrame::Delete_Zone( wxDC* DC, SEGZONE* aZone )
/**********************************************************/

/* Remove the zone which include the segment aZone.
 *  A zone is a group of segments which have the same TimeStamp
 */
{
    if( aZone == NULL )
        return;

    int           nb_segm   = 0;
    bool          modify    = FALSE;
    unsigned long TimeStamp = aZone->m_TimeStamp;   // Save reference time stamp (aZone will be deleted)

    SEGZONE*      next;
    for( SEGZONE* zone = m_Pcb->m_Zone; zone != NULL; zone = next )
    {
        next = zone->Next();
        if( zone->m_TimeStamp == TimeStamp )
        {
            modify = TRUE;

            /* Erase segment from screen */
            Trace_Une_Piste( DrawPanel, DC, zone, nb_segm, GR_XOR );
            /* remove item from linked list and free memory */
            zone->DeleteStructure();
        }
    }

    if( modify )
    {
        GetScreen()->SetModify();
        GetScreen()->SetRefreshReq();
    }
}


/*****************************************************************************/
EDGE_ZONE* WinEDA_PcbFrame::Del_SegmEdgeZone( wxDC* DC, EDGE_ZONE* edge_zone )
/*****************************************************************************/
/* Routine d'effacement du segment de limite zone en cours de trace */
{
    EDGE_ZONE* segm;

    if( m_Pcb->m_CurrentLimitZone )
        segm = m_Pcb->m_CurrentLimitZone;
    else
        segm = edge_zone;

    if( segm == NULL )
        return NULL;

    Trace_DrawSegmentPcb( DrawPanel, DC, segm, GR_XOR );

    m_Pcb->m_CurrentLimitZone = segm->Next();
    delete segm;

    segm = m_Pcb->m_CurrentLimitZone;
    SetCurItem( segm );

    if( segm )
    {
        segm->Pback = NULL;
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
    }
    else
    {
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        SetCurItem( NULL );
    }
    return segm;
}


/*************************************************************************/
static void Abort_Zone_Create_Outline( WinEDA_DrawPanel* Panel, wxDC* DC )
/*************************************************************************/

/**
 * Function Abort_Zone_Create_Outline
 * cancels the Begin_Zone state if at least one EDGE_ZONE has been created.
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;

    if( pcbframe->m_Pcb->m_CurrentLimitZone )
    {
        if( Panel->ManageCurseur )  // trace in progress
        {
            Panel->ManageCurseur( Panel, DC, 0 );
        }
        pcbframe->DelLimitesZone( DC, TRUE );
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
}


/**************************************************************/
void WinEDA_BasePcbFrame::DelLimitesZone( wxDC* DC, bool Redraw )
/**************************************************************/
{
    EDGE_ZONE* segment;
    EDGE_ZONE* next;

    if( m_Pcb->m_CurrentLimitZone == NULL )
        return;

    // erase the old zone border, one segment at a time
    for( segment = m_Pcb->m_CurrentLimitZone; segment; segment = next )
    {
        next = segment->Next();

        if( Redraw && DC )
            Trace_DrawSegmentPcb( DrawPanel, DC, segment, GR_XOR );

        delete segment;
    }

    m_Pcb->m_CurrentLimitZone = NULL;

    SetCurItem( NULL );
}

/*******************************************************************************************************/
void WinEDA_PcbFrame::Start_Move_Zone_Corner( wxDC* DC , ZONE_CONTAINER * zone_container,
	int corner_id, bool IsNewCorner )
/*******************************************************************************************************/
/**
 * Function Start_Move_Zone_Corner
 * Initialise parametres to move an existing corner of a zone.
 * if IsNewCorner is true, the Abort_Zone_Move_Corner will remove this corner, if called
 */
{
	/* Show the Net */
	if( (g_HightLigth_NetCode > 0) && (g_HightLigth_NetCode != s_NetcodeSelection) )
	{
		Hight_Light( DC );	// Remove old hightlight selection
	}
	
	g_HightLigth_NetCode = s_NetcodeSelection;
	if ( ! g_HightLigt_Status )
		Hight_Light( DC );

	zone_container->m_Flags = IN_EDIT;
	DrawPanel->ManageCurseur = Show_Zone_Corner_While_Move_Mouse;
	DrawPanel->ForceCloseManageCurseur = Abort_Zone_Move_Corner;
	s_CornerInitialPosition.x = zone_container->GetX(corner_id);
	s_CornerInitialPosition.y = zone_container->GetY(corner_id);
	s_CornerIsNew = IsNewCorner;
}

/***************************************************************************************/
void WinEDA_PcbFrame::End_Move_Zone_Corner( wxDC* DC , ZONE_CONTAINER * zone_container )
/****************************************************************************************/
/**
 * Function End_Move_Zone_Corner
 * Terminates a move corner in a zone outline
 */
{
	zone_container->m_Flags = 0;
	DrawPanel->ManageCurseur = NULL;
	DrawPanel->ForceCloseManageCurseur = NULL;
	zone_container->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);
}


/**************************************************************/
void Abort_Zone_Move_Corner( WinEDA_DrawPanel* Panel, wxDC* DC )
/**************************************************************/
/**
 * Function Abort_Zone_Move_Corner
 * cancels the Begin_Zone state if at least one EDGE_ZONE has been created.
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
	ZONE_CONTAINER* zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

	zone_container->Draw(Panel, DC, wxPoint(0,0), GR_XOR);
 
	if ( s_CornerIsNew )
	{
		zone_container->DeleteCorner( zone_container->m_CornerSelection );
	}
	else
	{
		wxPoint pos = s_CornerInitialPosition;
		zone_container->MoveCorner( zone_container->m_CornerSelection, pos.x, pos.y );
	}
 	zone_container->Draw(Panel, DC, wxPoint(0,0), GR_XOR);

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
	zone_container->m_Flags = 0;
}


/**************************************************************************************/
void Show_Zone_Corner_While_Move_Mouse( WinEDA_DrawPanel* Panel, wxDC* DC, bool erase )
/**************************************************************************************/
/* Redraws the zone outline when moving a corner according to the cursor position
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
	ZONE_CONTAINER* zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

//    if( erase )    /* Undraw edge in old position*/
    {
		zone_container->Draw(Panel, DC, wxPoint(0,0), GR_XOR);
    }

	wxPoint pos = pcbframe->GetScreen()->m_Curseur;
    zone_container->MoveCorner( zone_container->m_CornerSelection, pos.x, pos.y );
 	zone_container->Draw(Panel, DC, wxPoint(0,0), GR_XOR);
}



/*************************************************/
EDGE_ZONE* WinEDA_PcbFrame::Begin_Zone( wxDC* DC )
/*************************************************/
/**
 * Function Begin_Zone
 * either initializes the first segment of a new zone, or adds an
 * intermediate segment.
 */
{
    EDGE_ZONE* oldedge;
    EDGE_ZONE* newedge = NULL;

    oldedge = m_Pcb->m_CurrentLimitZone;
	
    if( m_Pcb->m_CurrentLimitZone == NULL )    /* Start a new contour: init zone params (net and layer) */
	{
		DrawPanel->m_IgnoreMouseEvents = TRUE;
		WinEDA_ZoneFrame* frame = new WinEDA_ZoneFrame( this );

		int diag = frame->ShowModal();
		frame->Destroy();
		DrawPanel->MouseToCursorSchema();
		DrawPanel->m_IgnoreMouseEvents = FALSE;

		if( diag ==  ZONE_ABORT )
			return NULL;

		GetScreen()->m_Active_Layer = s_Zone_Layer;

		/* Show the Net */
		if( (g_HightLigth_NetCode > 0) && (g_HightLigth_NetCode != s_NetcodeSelection) )
		{
			Hight_Light( DC );	// Remove old hightlight selection
	    }
		
		g_HightLigth_NetCode = s_NetcodeSelection;
		if ( ! g_HightLigt_Status )
			Hight_Light( DC );
	}

    // if first segment
    if( (m_Pcb->m_CurrentLimitZone == NULL )    /* debut reel du trace */
       || (DrawPanel->ManageCurseur == NULL) )  /* reprise d'un trace complementaire */
    {
        newedge = new EDGE_ZONE( m_Pcb );
        newedge->m_Flags = IS_NEW | STARTPOINT | IS_MOVED;
        newedge->m_Start = newedge->m_End = GetScreen()->m_Curseur;
        newedge->SetLayer( GetScreen()->m_Active_Layer );

        // link into list:
        newedge->Pnext = oldedge;

        if( oldedge )
            oldedge->Pback = newedge;

        m_Pcb->m_CurrentLimitZone = newedge;

        DrawPanel->ManageCurseur = Show_New_Zone_Edge_While_Move_Mouse;
        DrawPanel->ForceCloseManageCurseur = Abort_Zone_Create_Outline;
    }
    // edge in progress:
    else
    {    /* edge in progress : the ending point coordinate was set by Show_New_Zone_Edge_While_Move_Mouse */
        if( oldedge->m_Start != oldedge->m_End )
        {
            oldedge->m_Flags &= ~(IS_NEW | IS_MOVED);

            newedge = new EDGE_ZONE( oldedge );
            newedge->m_Flags = IS_NEW | IS_MOVED;
            newedge->m_Start = newedge->m_End = oldedge->m_End;
            newedge->SetLayer( GetScreen()->m_Active_Layer );

            // link into list:
            newedge->Pnext = oldedge;
            oldedge->Pback = newedge;
            m_Pcb->m_CurrentLimitZone = newedge;
        }
    }

    return newedge;
}


/*********************************************/
void WinEDA_PcbFrame::End_Zone( wxDC* DC )
/*********************************************/

/*
 *  Terminates an edge zone creation
 * Close the current edge zone considered as a polygon
 * put it in the main list m_Pcb->m_ZoneDescriptorList (a vector<ZONE_CONTAINER*>)
 */
{
    EDGE_ZONE* edge;

    if( m_Pcb->m_CurrentLimitZone )
    {
        Begin_Zone( DC );

        /* The last segment is a stub: its lenght is 0.
         * Use it to close the polygon by setting its ending point coordinate = start point of first segment
		 */
        edge = m_Pcb->m_CurrentLimitZone;
        edge->m_Flags &= ~(IS_NEW | IS_MOVED);

        while( edge && edge->Next() )
        {
            edge = edge->Next();
            if( edge->m_Flags & STARTPOINT )
                break;

            edge->m_Flags &= ~(IS_NEW | IS_MOVED);
        }

        if( edge )
        {
            edge->m_Flags &= ~(IS_NEW | IS_MOVED);
            m_Pcb->m_CurrentLimitZone->m_End = edge->m_Start;
        }
        Trace_DrawSegmentPcb( DrawPanel, DC, m_Pcb->m_CurrentLimitZone, GR_XOR );
    }

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
	
	/* Put edges in list */
	ZONE_CONTAINER * polygon = new ZONE_CONTAINER( m_Pcb );
	polygon->SetLayer( GetScreen()->m_Active_Layer ); 
	polygon->SetNet( g_HightLigth_NetCode );
	polygon->m_TimeStamp = GetTimeStamp();
	
	EQUIPOT* net = m_Pcb->FindNet( g_HightLigth_NetCode );
	if ( net ) polygon->m_Netname = net->m_Netname; 
    edge = m_Pcb->m_CurrentLimitZone;
	polygon->Start( GetScreen()->m_Active_Layer, 0, NULL,
					edge->m_Start.x,  edge->m_Start.y, 
					s_Zone_Hatching );
    edge = edge->Next();
	while( edge )
	{
		polygon->AppendCorner( edge->m_Start.x,  edge->m_Start.y );
        edge = edge->Next();
	}
	polygon->Close();	// Close the current corner list
	polygon->Hatch();
	
	m_Pcb->m_ZoneDescriptorList.push_back(polygon);
	
	/* Remove the current temporary list */
    DelLimitesZone( DC, TRUE );
	
	/* Redraw the real edge zone */
	polygon->CPolyLine::Draw( );	// Build the line list
	polygon->Draw( DrawPanel, DC, wxPoint(0,0), GR_OR );
	
	GetScreen()->SetModify();
}


/******************************************************************************************/
static void Show_New_Zone_Edge_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/******************************************************************************************/

/* Redraws the edge zone when moving mouse
 */
{
    EDGE_ZONE*       edge;
    EDGE_ZONE*       currentEdge;
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) panel->m_Parent;

    if( pcbframe->m_Pcb->m_CurrentLimitZone == NULL )
        return;

    if( erase )    /* Undraw edge in old position*/
    {
        edge = pcbframe->m_Pcb->m_CurrentLimitZone;

        // for( ;  edge; edge = edge->Next() )
        {
            Trace_DrawSegmentPcb( panel, DC, edge, GR_XOR );
        }
    }

    /* Reinit layer (which can be changed) */
    for( edge = pcbframe->m_Pcb->m_CurrentLimitZone; edge; edge = edge->Next() )
    {
        edge->SetLayer( pcbframe->GetScreen()->m_Active_Layer );
    }

    /* Redraw the curent edge in its new position */
    currentEdge = pcbframe->m_Pcb->m_CurrentLimitZone;
    if( Zone_45_Only )
    {
        // calculate the new position as allowed
        currentEdge->m_End = pcbframe->GetScreen()->m_Curseur;
        Calcule_Coord_Extremite_45( currentEdge->m_Start.x, currentEdge->m_Start.y,
                                    &currentEdge->m_End.x, &currentEdge->m_End.y );
    }
    else    /* all orientations are allowed */
    {
        currentEdge->m_End = pcbframe->GetScreen()->m_Curseur;
    }

    // for( ; currentEdge;  currentEdge = currentEdge->Next() )
    {
        Trace_DrawSegmentPcb( panel, DC, currentEdge, GR_XOR );
    }
}


/***********************************************************************************/
void WinEDA_PcbFrame::Edit_Zone_Params( wxDC* DC , ZONE_CONTAINER * zone_container )
/***********************************************************************************/
/**
 * Function Edit_Zone_Params
 * Edit params (layer, clearance, ...) for a zone outline
 */
{
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_ZoneFrame* frame = new WinEDA_ZoneFrame( this, zone_container );

    int diag = frame->ShowModal();
    frame->Destroy();
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    if( diag == ZONE_ABORT )
        return;

	zone_container->Draw( DrawPanel, DC, wxPoint(0,0), GR_XOR );

	zone_container->SetLayer( s_Zone_Layer ); 
	zone_container->SetNet( s_NetcodeSelection );
	EQUIPOT* net = m_Pcb->FindNet( s_NetcodeSelection );
	if ( net ) zone_container->m_Netname = net->m_Netname; 
	zone_container->SetHatch(s_Zone_Hatching);

	zone_container->Draw( DrawPanel, DC, wxPoint(0,0), GR_OR );
}

/***************************************************************************/
void WinEDA_PcbFrame::Fill_Zone( wxDC* DC, ZONE_CONTAINER * zone_container )
/***************************************************************************/

/** Function Fill_Zone()
 *  Fillst the zone defined in zone_container
 */
{
    wxPoint  ZoneStartFill;
    wxString msg;

    MsgPanel->EraseMsgBox();
    if( m_Pcb->ComputeBoundaryBox() == FALSE )
    {
        DisplayError( this, wxT( "Board is empty!" ), 10 );
        return;
    }

    /* Show the Net */
    if( (g_HightLigth_NetCode > 0) && (g_HightLigth_NetCode != s_NetcodeSelection) )
    {
        Hight_Light( DC );	// Remove old hightlight selection
   }
	
    g_HightLigth_NetCode = s_NetcodeSelection;
	if ( ! g_HightLigt_Status )
        Hight_Light( DC );

    if( g_HightLigth_NetCode > 0 )
    {
		EQUIPOT* net = m_Pcb->FindNet( g_HightLigth_NetCode );
        if( net == NULL )
        {
            if( g_HightLigth_NetCode > 0 )
			{
                DisplayError( this, wxT( "Unable to find Net name" ) );
				return;
			}
        }
        else
            msg = net->m_Netname;
    }
    else
        msg = _( "No Net" );

    Affiche_1_Parametre( this, 22, _( "NetName" ), msg, RED );

	Build_Zone( this, DC, g_HightLigth_NetCode, Zone_Exclude_Pads, s_Zone_Create_Thermal_Relief );
	GetScreen()->SetModify();
}
