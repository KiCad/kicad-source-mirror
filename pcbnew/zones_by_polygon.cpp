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

#include "id.h"

#include "protos.h"

/* Local functions */

// Outile creation:
static void Abort_Zone_Create_Outline( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_New_Zone_Edge_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

// Corner moving
static void Abort_Zone_Move_Corner( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_Zone_Corner_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Local variables */
static bool                        Zone_45_Only = FALSE;
static ZONE_CONTAINER::m_PadInZone s_Zone_Pad_Options = ZONE_CONTAINER::THERMAL_PAD;
static int                         s_Zone_Layer;                // Layer used to create the current zone
static int                         s_Zone_Hatching;             // Option to show the zone area (outlines only, short hatches or full hatches
static int                         s_NetcodeSelection;          // Net code selection for the current zone
static wxPoint                     s_CornerInitialPosition;     // Used to abort a move corner command
static bool                        s_CornerIsNew;               // Used to abort a move corner command (if it is a new corner, it must be deleted)
static bool                        s_AddCutoutToCurrentZone;    // if true, the next outline will be addes to s_CurrentZone
static ZONE_CONTAINER*             s_CurrentZone;               // if != NULL, these ZONE_CONTAINER params will be used for the next zone

// key used to store net sort option in config file :
#define ZONE_NET_SORT_OPTION_KEY wxT( "Zone_NetSort_Opt" )

enum zone_cmd {
    ZONE_ABORT,
    ZONE_OK
};

#include "dialog_zones_by_polygon.cpp"

/**********************************************************************************/
void WinEDA_PcbFrame::Add_Similar_Zone( wxDC* DC, ZONE_CONTAINER* zone_container )
/**********************************************************************************/

/**
 * Function Add_Similar_Zone
 * Add a zone to a given zone outline.
 * if the zones are overlappeing they will be merged
 * @param DC = current Device Context
 * @param zone_container = parent zone outline
 */
{
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = zone_container;
    wxCommandEvent evt;
    evt.SetId( ID_PCB_ZONES_BUTT );
    Process_Special_Functions( evt );
}


/**********************************************************************************/
void WinEDA_PcbFrame::Add_Zone_Cutout( wxDC* DC, ZONE_CONTAINER* zone_container )
/**********************************************************************************/

/**
 * Function Add_Zone_Cutout
 * Add a cutout zone to a given zone outline
 * @param DC = current Device Context
 * @param zone_container = parent zone outline
 */
{
    s_AddCutoutToCurrentZone = true;
    s_CurrentZone = zone_container;
    wxCommandEvent evt;
    evt.SetId( ID_PCB_ZONES_BUTT );
    Process_Special_Functions( evt );
}


/*****************************************************************************/
void WinEDA_PcbFrame::Delete_Zone_Fill( wxDC* DC, SEGZONE* aZone, long aTimestamp )
/******************************************************************************/

/** Function Delete_Zone_Fill
 * Remove the zone fillig which include the segment aZone, or the zone which have the given time stamp.
 *  A zone is a group of segments which have the same TimeStamp
 * @param DC = current Device Context (can be NULL)
 * @param aZone = zone segment within the zone to delete. Can be NULL
 * @param aTimestamp = Timestamp for the zone to delete, used if aZone == NULL
 */
{
    int           nb_segm = 0;
    bool          modify  = false;
    unsigned long TimeStamp;

    if( aZone == NULL )
        TimeStamp = aTimestamp;
    else
        TimeStamp = aZone->m_TimeStamp; // Save reference time stamp (aZone will be deleted)

    SEGZONE* next;
    for( SEGZONE* zone = m_Pcb->m_Zone; zone != NULL; zone = next )
    {
        next = zone->Next();
        if( zone->m_TimeStamp == TimeStamp )
        {
            modify = TRUE;

            /* Erase segment from screen */
            if( DC )
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

/* Used only while creating a new zone outline
 * Remove and delete the current outline segment in progress
 */
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
 * cancels the Begin_Zone command if at least one EDGE_ZONE has been created.
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
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/**************************************************************/
void WinEDA_BasePcbFrame::DelLimitesZone( wxDC* DC, bool Redraw )
/**************************************************************/
{
    EDGE_ZONE* segment;
    EDGE_ZONE* next;

    if( m_Pcb->m_CurrentLimitZone == NULL )
        return;

    // erase the old zone outline, one segment at a time
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
void WinEDA_PcbFrame::Start_Move_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container,
                                              int corner_id, bool IsNewCorner )
/*******************************************************************************************************/

/**
 * Function Start_Move_Zone_Corner
 * Initialise parametres to move an existing corner of a zone.
 * if IsNewCorner is true, the Abort_Zone_Move_Corner will remove this corner, if called
 */
{
    /* Show the Net */
    if( g_HightLigt_Status )
    {
        Hight_Light( DC );  // Remove old hightlight selection
    }

    g_HightLigth_NetCode = s_NetcodeSelection = zone_container->GetNet();
    Hight_Light( DC );

    zone_container->m_Flags  = IN_EDIT;
    DrawPanel->ManageCurseur = Show_Zone_Corner_While_Move_Mouse;
    DrawPanel->ForceCloseManageCurseur = Abort_Zone_Move_Corner;
    s_CornerInitialPosition.x = zone_container->m_Poly->GetX( corner_id );
    s_CornerInitialPosition.y = zone_container->m_Poly->GetY( corner_id );
    s_CornerIsNew = IsNewCorner;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/***************************************************************************************/
void WinEDA_PcbFrame::End_Move_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container )
/****************************************************************************************/

/**
 * Function End_Move_Zone_Corner
 * Terminates a move corner in a zone outline
 */
{
    zone_container->m_Flags  = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    zone_container->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    GetScreen()->SetModify();
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    SetCurItem( NULL );       // This outine can be deleted when merging outlines

    /* Combine zones if possible */
    int layer = zone_container->GetLayer();

    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        if( layer == edge_zone->GetLayer() )
            edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    m_Pcb->AreaPolygonModified( zone_container, true, false );
    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        if( layer == edge_zone->GetLayer() )
            edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    }
}


/*************************************************************************************/
void WinEDA_PcbFrame::Remove_Zone_Corner( wxDC* DC, ZONE_CONTAINER * zone_container )
/*************************************************************************************/
/**
 * Function End_Move_Zone_Corner
 * Remove the currently selected corner in a zone outline
 * the .m_CornerSelection is used as corner selection
 */
{
	if ( zone_container->m_Poly->GetNumCorners() <= 3 )
	{
		Delete_Zone_Fill( DC, NULL, zone_container->m_TimeStamp );
		m_Pcb->Delete( zone_container );
		return;
	}

    int layer = zone_container->GetLayer();

    if ( DC )
	{
		for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
		{
			ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
			if( layer == edge_zone->GetLayer() )
				edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
		}
	}

	zone_container->m_Poly->DeleteCorner(zone_container->m_CornerSelection);
	
	// modify zones outlines accordiing to the new zone_container shape
    m_Pcb->AreaPolygonModified( zone_container, true, false );
    if ( DC )
	{
		for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
		{
			ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
			if( layer == edge_zone->GetLayer() )
				edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
		}
	}
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
    ZONE_CONTAINER*  zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

    zone_container->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );

    if( s_CornerIsNew )
    {
        zone_container->m_Poly->DeleteCorner( zone_container->m_CornerSelection );
    }
    else
    {
        wxPoint pos = s_CornerInitialPosition;
        zone_container->m_Poly->MoveCorner( zone_container->m_CornerSelection, pos.x, pos.y );
    }
    zone_container->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
    zone_container->m_Flags  = 0;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/**************************************************************************************/
void Show_Zone_Corner_While_Move_Mouse( WinEDA_DrawPanel* Panel, wxDC* DC, bool erase )
/**************************************************************************************/

/* Redraws the zone outline when moving a corner according to the cursor position
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
    ZONE_CONTAINER*  zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

//    if( erase )    /* Undraw edge in old position*/
    {
        zone_container->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    wxPoint          pos = pcbframe->GetScreen()->m_Curseur;
    zone_container->m_Poly->MoveCorner( zone_container->m_CornerSelection, pos.x, pos.y );
    zone_container->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );
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

    // verify if s_CurrentZone exists:
    unsigned   ii;

    for( ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        if( s_CurrentZone == m_Pcb->m_ZoneDescriptorList[ii] )
            break;
    }

    if( ii == m_Pcb->m_ZoneDescriptorList.size() ) // Not found: coul be deleted since last selection
    {
        s_AddCutoutToCurrentZone = false;
        s_CurrentZone = NULL;
    }

    oldedge = m_Pcb->m_CurrentLimitZone;

    if( m_Pcb->m_CurrentLimitZone == NULL )    /* Start a new contour: init zone params (net and layer) */
    {
        if( s_CurrentZone == NULL )
        {
            DrawPanel->m_IgnoreMouseEvents = TRUE;
            WinEDA_ZoneFrame* frame = new WinEDA_ZoneFrame( this );

            int diag = frame->ShowModal();
            frame->Destroy();
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_IgnoreMouseEvents = FALSE;

            if( diag ==  ZONE_ABORT )
                return NULL;
        }
        else /* Start a new contour: init zone params (net and layer) from an existing zone */
        {
            GetScreen()->m_Active_Layer = s_Zone_Layer = s_CurrentZone->GetLayer();
            s_Zone_Hatching = s_CurrentZone->m_Poly->GetHatchStyle();
        }
        /* Show the Net */
        if( g_HightLigt_Status && (g_HightLigth_NetCode != s_NetcodeSelection) )
        {
            Hight_Light( DC );  // Remove old hightlight selection
        }

        if( s_CurrentZone )
            s_NetcodeSelection = s_CurrentZone->GetNet();
        g_HightLigth_NetCode = s_NetcodeSelection;
        Hight_Light( DC );

        if( !s_AddCutoutToCurrentZone )
            s_CurrentZone = NULL; // the zone is used only once
    }

    // if first segment
    if( (m_Pcb->m_CurrentLimitZone == NULL )    /* Initial start of a new outline */
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
    {
        /* edge in progress : the ending point coordinate was set by Show_New_Zone_Edge_While_Move_Mouse */
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

/** Function End_Zone
 * Terminates a zone outline creation
 * Close the current zone outline considered as a polygon
 * put it in the main list m_Pcb->m_ZoneDescriptorList (a vector<ZONE_CONTAINER*>)
 */
{
    EDGE_ZONE* edge;
    int        layer = GetScreen()->m_Active_Layer;

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

    // Undraw old drawings, because they can have important changes
    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        if( layer == edge_zone->GetLayer() )
            edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    /* Put edges in list */
    ZONE_CONTAINER* new_zone_container;
    if( s_CurrentZone == NULL )
    {
        new_zone_container = new ZONE_CONTAINER( m_Pcb );
        new_zone_container->SetLayer( layer );
        new_zone_container->SetNet( g_HightLigth_NetCode );
        new_zone_container->m_TimeStamp = GetTimeStamp();

        edge = m_Pcb->m_CurrentLimitZone;
        new_zone_container->m_Poly->Start( layer, 0, 0,
                                           edge->m_Start.x, edge->m_Start.y,
                                           s_Zone_Hatching );
        edge = edge->Next();
        while( edge )
        {
            new_zone_container->m_Poly->AppendCorner( edge->m_Start.x, edge->m_Start.y );
            edge = edge->Next();
        }

        new_zone_container->m_Poly->Close(); // Close the current corner list
        new_zone_container->m_Poly->SetHatch( s_Zone_Hatching );
        new_zone_container->m_PadOption     = s_Zone_Pad_Options;
        new_zone_container->m_ZoneClearance = g_DesignSettings.m_ZoneClearence;
        new_zone_container->m_GridFillValue = g_GridRoutingSize;

        m_Pcb->m_ZoneDescriptorList.push_back( new_zone_container );
    }
    else    // Append this outline as a cutout to an existing zone
    {
        new_zone_container = s_CurrentZone;
        edge    = m_Pcb->m_CurrentLimitZone;
        while( edge )
        {
            new_zone_container->m_Poly->AppendCorner( edge->m_Start.x, edge->m_Start.y );
            edge = edge->Next();
        }

        new_zone_container->m_Poly->Close(); // Close the current corner list
    }

    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    /* Remove the current temporary list */
    DelLimitesZone( DC, TRUE );

    new_zone_container->m_Flags = 0;
    SetCurItem( NULL );       // This outine can be deleted when merging outlines

    // Combine zones if possible :
    m_Pcb->AreaPolygonModified( new_zone_container, true, false );

    // Redraw the real edge zone :
    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        if( layer == edge_zone->GetLayer() )
            edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    }

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
void WinEDA_PcbFrame::Edit_Zone_Params( wxDC* DC, ZONE_CONTAINER* zone_container )
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

	// Undraw old zone outlines
    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    zone_container->SetLayer( s_Zone_Layer );
    zone_container->SetNet( s_NetcodeSelection );
    EQUIPOT* net = m_Pcb->FindNet( s_NetcodeSelection );
    if( net )
        zone_container->m_Netname = net->m_Netname;
    zone_container->m_Poly->SetHatch( s_Zone_Hatching );
    zone_container->m_PadOption     = s_Zone_Pad_Options;
    zone_container->m_ZoneClearance = g_DesignSettings.m_ZoneClearence;
    zone_container->m_GridFillValue = g_GridRoutingSize;

    // Combine zones if possible :
    m_Pcb->AreaPolygonModified( zone_container, true, false );

    // Redraw the real new zone outlines:
    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->m_ZoneDescriptorList[ii];
        edge_zone->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    }

    GetScreen()->SetModify();
}

/************************************************************************************/
void WinEDA_PcbFrame::Delete_Zone_Contour( wxDC* DC, ZONE_CONTAINER* zone_container )
/************************************************************************************/

/** Function Delete_Zone_Contour
 * Remove the zone which include the segment aZone, or the zone which have the given time stamp.
 *  A zone is a group of segments which have the same TimeStamp
 * @param DC = current Device Context (can be NULL)
 * @param zone_container = zone to modify
 *  the member .m_CornerSelection is used to find the outline to remove.
 * if the outline is the main outline, all the zone_container is removed (deleted)
 * otherwise, the hole is deleted
 */
{
	int ncont = zone_container->m_Poly->GetContour(zone_container->m_CornerSelection);

	if ( DC )
		zone_container->Draw(DrawPanel, DC, wxPoint(0,0), GR_XOR);

	Delete_Zone_Fill( DC, NULL, zone_container->m_TimeStamp );	// Remove fill segments
	
	if ( ncont == 0 )	// This is the main outline: remove all
		m_Pcb->Delete( zone_container );

	else
	{
		zone_container->m_Poly->RemoveContour( ncont );
		if ( DC )
			zone_container->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);
	}
    GetScreen()->SetModify();
}


/***************************************************************************************/
int WinEDA_PcbFrame::Fill_Zone( wxDC* DC, ZONE_CONTAINER* zone_container, bool verbose )
/***************************************************************************************/

/** Function Fill_Zone()
 *  Calculate the zone filling for the outline zone_container
 *  The zone outline is a frontier, and can be complex (with holes)
 *  The filling starts from starting points like pads, tracks.
 * If exists the old filling is removed
 * @param DC = current Device Context
 * @param zone_container = zone to fill
 * @param verbose = true to show error messages
 * @return error level (0 = no error)
 */
{
    wxString msg;

    MsgPanel->EraseMsgBox();
    if( m_Pcb->ComputeBoundaryBox() == FALSE )
    {
        if( verbose )
            DisplayError( this, wxT( "Board is empty!" ), 10 );
        return -1;
    }

    /* Show the Net */
    s_NetcodeSelection = zone_container->GetNet();
    if( g_HightLigt_Status && (g_HightLigth_NetCode != s_NetcodeSelection)  && DC )
    {
        Hight_Light( DC );      // Remove old hightlight selection
    }

    g_HightLigth_NetCode = s_NetcodeSelection;
    if( DC )
        Hight_Light( DC );

    if( g_HightLigth_NetCode > 0 )
    {
        EQUIPOT* net = m_Pcb->FindNet( g_HightLigth_NetCode );
        if( net == NULL )
        {
            if( g_HightLigth_NetCode > 0 )
            {
                if( verbose )
                    DisplayError( this, wxT( "Unable to find Net name" ) );
                return -2;
            }
        }
        else
            msg = net->m_Netname;
    }
    else
        msg = _( "No Net" );

    Affiche_1_Parametre( this, 22, _( "NetName" ), msg, RED );
    wxBusyCursor dummy;     // Shows an hourglass cursor (removed by its destructor)
    zone_container->m_PadOption     = s_Zone_Pad_Options;
    zone_container->m_ZoneClearance = g_DesignSettings.m_ZoneClearence;
    zone_container->m_GridFillValue = g_GridRoutingSize;
    int error_level = zone_container->Fill_Zone( this, DC, verbose );

    GetScreen()->SetModify();

    return error_level;
}


/************************************************************/
int WinEDA_PcbFrame::Fill_All_Zones( wxDC* DC, bool verbose )
/************************************************************/

/** Function Fill_All_Zones()
 *  Fill all zones on the board
 * The old fillings are removed
 * @param frame = reference to the main frame
 * @param DC = current Device Context
 * @param verbose = true to show error messages
 * @return error level (0 = no error)
 */
{
    ZONE_CONTAINER* zone_container;
    int             error_level = 0;

    // Remove all zones :
    if( m_Pcb->m_Zone )
    {
        m_Pcb->m_Zone->DeleteStructList();
        m_Pcb->m_Zone = NULL;
        m_Pcb->m_NbSegmZone = 0;
    }

    for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
    {
        zone_container = m_Pcb->m_ZoneDescriptorList[ii];
        error_level    = Fill_Zone( NULL, zone_container, verbose );
        if( error_level && !verbose )
            break;
    }

    DrawPanel->Refresh( true );
    return error_level;
}
