/////////////////////////////////////////////////////////////////////////////

// Name:        zones_by_polygon.cpp
// Licence:     GPL License
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "zones.h"
#include "id.h"
#include "protos.h"

using namespace std;

bool verbose = false;       // false if zone outline diags must not be shown

// Outline creation:
static void Abort_Zone_Create_Outline( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_New_Edge_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

// Corner moving
static void Abort_Zone_Move_Corner_Or_Outlines( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Show_Zone_Corner_Or_Outline_While_Move_Mouse( WinEDA_DrawPanel* panel,
                                                          wxDC*             DC,
                                                          bool              erase );

/* Local variables */
static wxPoint         s_CornerInitialPosition;                 // Used to abort a move corner command
static bool            s_CornerIsNew;                           // Used to abort a move corner command (if it is a new corner, it must be deleted)
static bool            s_AddCutoutToCurrentZone;                // if true, the next outline will be addes to s_CurrentZone
static ZONE_CONTAINER* s_CurrentZone;                           // if != NULL, these ZONE_CONTAINER params will be used for the next zone
static wxPoint         s_CursorLastPosition;                    // in move zone outline, last cursor position. Used to calculate the move vector

#include "dialog_copper_zones.h"

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
    if ( zone_container == NULL )
        return;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = zone_container;

    /* set zones setup to the current zone */
    g_Zone_Default_Setting.ImportSetting( *zone_container );

    // Use the general event handle to set others params (like toolbar) */
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
    if ( zone_container == NULL )
        return;
    s_AddCutoutToCurrentZone = true;
    s_CurrentZone = zone_container;

    /* set zones setup to the current zone */
    g_Zone_Default_Setting.ImportSetting( *zone_container );

    // Use the general event handle to set others params (like toolbar) */
    wxCommandEvent evt;
    evt.SetId( ID_PCB_ZONES_BUTT );
    Process_Special_Functions( evt );
}


/**********************************************************************************/
void WinEDA_PcbFrame::Delete_Zone_Fill( wxDC* DC, SEGZONE* aZone, long aTimestamp )
/**********************************************************************************/

/** Function Delete_Zone_Fill
 * Remove the zone fillig which include the segment aZone, or the zone which have the given time stamp.
 *  A zone is a group of segments which have the same TimeStamp
 * @param DC = current Device Context (can be NULL)
 * @param aZone = zone segment within the zone to delete. Can be NULL
 * @param aTimestamp = Timestamp for the zone to delete, used if aZone == NULL
 */
{
    bool          modify  = false;
    unsigned long TimeStamp;

    if( aZone == NULL )
        TimeStamp = aTimestamp;
    else
        TimeStamp = aZone->m_TimeStamp; // Save reference time stamp (aZone will be deleted)

    SEGZONE* next;
    for( SEGZONE* zone = GetBoard()->m_Zone; zone != NULL; zone = next )
    {
        next = zone->Next();
        if( zone->m_TimeStamp == TimeStamp )
        {
            modify = TRUE;
            /* remove item from linked list and free memory */
            zone->DeleteStructure();
        }
    }

    // Now delete the outlines of the corresponding copper areas
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetBoard()->GetArea( ii );
        if( zone->m_TimeStamp == TimeStamp )
        {
            modify = TRUE;
            zone->m_FilledPolysList.clear();
        }
    }

    if( modify )
    {
        GetScreen()->SetModify();
        GetScreen()->SetRefreshReq();
    }
}


/*******************************************************/
int WinEDA_PcbFrame::Delete_LastCreatedCorner( wxDC* DC )
/*******************************************************/

/** Used **only** while creating a new zone outline
 * Remove and delete the current outline segment in progress
 * @return 0 if no corner in list, or corner number
 * if no corner in list, close the outline creation
 */
{
    ZONE_CONTAINER* zone = GetBoard()->m_CurrentZoneContour;

    if( zone == NULL )
        return 0;

    if( zone->GetNumCorners() == 0 )
        return 0;

    zone->DrawWhileCreateOutline( DrawPanel, DC, GR_XOR );

    if( zone->GetNumCorners() > 2 )
    {
        zone->m_Poly->DeleteCorner( zone->GetNumCorners() - 1 );
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
    }
    else
    {
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        SetCurItem( NULL );
        zone->RemoveAllContours();
        zone->m_Flags = 0;
    }
    return zone->GetNumCorners();
}


/*************************************************************************/
static void Abort_Zone_Create_Outline( WinEDA_DrawPanel* Panel, wxDC* DC )
/*************************************************************************/

/**
 * Function Abort_Zone_Create_Outline
 * cancels the Begin_Zone command if at least one EDGE_ZONE was created.
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
    ZONE_CONTAINER*  zone = pcbframe->GetBoard()->m_CurrentZoneContour;

    if( zone )
    {
        zone->DrawWhileCreateOutline( Panel, DC, GR_XOR );
        zone->m_Flags = 0;
        zone->RemoveAllContours();
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/*******************************************************************************************************/
void WinEDA_PcbFrame::Start_Move_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container,
                                              int corner_id, bool IsNewCorner )
/*******************************************************************************************************/

/**
 * Function Start_Move_Zone_Corner
 * Initialise parametres to move an existing corner of a zone.
 * if IsNewCorner is true, the Abort_Zone_Move_Corner_Or_Outlines will remove this corner, if called
 */
{
    if( zone_container->IsOnCopperLayer() ) /* Show the Net */
    {
        if( g_HightLigt_Status && DC )
        {
            Hight_Light( DC );  // Remove old hightlight selection
        }

        g_HightLigth_NetCode = g_Zone_Default_Setting.m_NetcodeSelection = zone_container->GetNet();
        if( DC )
            Hight_Light( DC );
    }

    zone_container->m_Flags  = IN_EDIT;
    DrawPanel->ManageCurseur = Show_Zone_Corner_Or_Outline_While_Move_Mouse;
    DrawPanel->ForceCloseManageCurseur = Abort_Zone_Move_Corner_Or_Outlines;
    s_CornerInitialPosition = zone_container->GetCornerPosition( corner_id );
    s_CornerIsNew = IsNewCorner;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/**************************************************************************************/
void WinEDA_PcbFrame::Start_Move_Zone_Drag_Outline_Edge( wxDC*           DC,
                                                         ZONE_CONTAINER* zone_container,
                                                         int             corner_id )
/**************************************************************************************/

/**
 * Function Start_Move_Zone_Corner
 * Prepares a drag edge for an existing zone outline,
 */
{
    zone_container->m_Flags = IS_DRAGGED;
    zone_container->m_CornerSelection = corner_id;
    DrawPanel->ManageCurseur = Show_Zone_Corner_Or_Outline_While_Move_Mouse;
    DrawPanel->ForceCloseManageCurseur = Abort_Zone_Move_Corner_Or_Outlines;
    s_CursorLastPosition     = s_CornerInitialPosition = GetScreen()->m_Curseur;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/*******************************************************************************************************/
void WinEDA_PcbFrame::Start_Move_Zone_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container )
/*******************************************************************************************************/

/**
 * Function Start_Move_Zone_Outlines
 * Initialise parametres to move an existing zone outlines.
 */
{
    /* Show the Net */
    if( zone_container->IsOnCopperLayer() ) /* Show the Net */
    {
        if( g_HightLigt_Status )
        {
            Hight_Light( DC );  // Remove old hightlight selection
        }

        g_HightLigth_NetCode = g_Zone_Default_Setting.m_NetcodeSelection = zone_container->GetNet();
        Hight_Light( DC );
    }

    zone_container->m_Flags  = IS_MOVED;
    DrawPanel->ManageCurseur = Show_Zone_Corner_Or_Outline_While_Move_Mouse;
    DrawPanel->ForceCloseManageCurseur = Abort_Zone_Move_Corner_Or_Outlines;
    s_CursorLastPosition = s_CornerInitialPosition = GetScreen()->m_Curseur;
    s_CornerIsNew = false;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/*************************************************************************************************/
void WinEDA_PcbFrame::End_Move_Zone_Corner_Or_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container )
/*************************************************************************************************/

/**
 * Function End_Move_Zone_Corner_Or_Outlines
 * Terminates a move corner in a zone outline, or a move zone outlines
 * @param DC = current Device Context (can be NULL)
 * @param zone_container: the given zone
 */
{
    zone_container->m_Flags  = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    if( DC )
        zone_container->Draw( DrawPanel, DC, GR_OR );
    GetScreen()->SetModify();
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    SetCurItem( NULL );       // This outline can be deleted when merging outlines

    /* Combine zones if possible */
    wxBusyCursor dummy;
    GetBoard()->AreaPolygonModified( zone_container, true, verbose );
    DrawPanel->Refresh();


    int ii = GetBoard()->GetAreaIndex( zone_container );     // test if zone_container exists
    if( ii < 0 )
        zone_container = NULL;                          // was removed by combining zones

    int error_count = GetBoard()->Test_Drc_Areas_Outlines_To_Areas_Outlines( zone_container, true );
    if( error_count )
    {
        DisplayError( this, _( "Area: DRC outline error" ) );
    }
}


/*************************************************************************************/
void WinEDA_PcbFrame::Remove_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container )
/*************************************************************************************/

/**
 * Function Remove_Zone_Corner
 * Remove the currently selected corner in a zone outline
 * the .m_CornerSelection is used as corner selection
 * @param DC = Current deice context (can be NULL )
 * @param zone_container = the zone that contains the selected corner
 *  the member .m_CornerSelection is used as selected corner
 */
{
    GetScreen()->SetModify();

    if( zone_container->m_Poly->GetNumCorners() <= 3 )
    {
        DrawPanel->PostDirtyRect( zone_container->GetBoundingBox() );
        if( DC )
        {  // Remove the full zone because this is no more an area
            Delete_Zone_Fill( DC, NULL, zone_container->m_TimeStamp );
            zone_container->DrawFilledArea( DrawPanel, DC, GR_XOR );
        }
        GetBoard()->Delete( zone_container );
        return;
    }

    int layer = zone_container->GetLayer();

    if( DC )
    {
        GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_XOR, layer );
        GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_XOR, layer );
    }

    zone_container->m_Poly->DeleteCorner( zone_container->m_CornerSelection );

    // modify zones outlines according to the new zone_container shape
    GetBoard()->AreaPolygonModified( zone_container, true, verbose );
    if( DC )
    {
        GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_OR, layer );
        GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_OR, layer );
    }

    int ii = GetBoard()->GetAreaIndex( zone_container );     // test if zone_container exists
    if( ii < 0 )
        zone_container = NULL;                          // zone_container does not exist anymaore, after combining zones
    int error_count = GetBoard()->Test_Drc_Areas_Outlines_To_Areas_Outlines( zone_container, true );
    if( error_count )
    {
        DisplayError( this, _( "Area: DRC outline error" ) );
    }
}


/**************************************************************************/
void Abort_Zone_Move_Corner_Or_Outlines( WinEDA_DrawPanel* Panel, wxDC* DC )
/**************************************************************************/

/**
 * Function Abort_Zone_Move_Corner_Or_Outlines
 * cancels the Begin_Zone state if at least one EDGE_ZONE has been created.
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
    ZONE_CONTAINER*  zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

//    zone_container->Draw( Panel, DC, GR_XOR );

    if( zone_container->m_Flags == IS_MOVED )
    {
        wxPoint offset;
        offset = s_CornerInitialPosition - s_CursorLastPosition;
        zone_container->Move( offset );
    }
    else if( zone_container->m_Flags == IS_DRAGGED )
    {
        wxPoint offset;
        offset = s_CornerInitialPosition - s_CursorLastPosition;
        zone_container->MoveEdge( offset );
    }
    else
    {
        if( s_CornerIsNew )
        {
            zone_container->m_Poly->DeleteCorner( zone_container->m_CornerSelection );
        }
        else
        {
            wxPoint pos = s_CornerInitialPosition;
            zone_container->m_Poly->MoveCorner( zone_container->m_CornerSelection, pos.x, pos.y );
        }
    }

//    zone_container->Draw( Panel, DC, GR_XOR );
    Panel->Refresh();


    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    pcbframe->SetCurItem( NULL );
    zone_container->m_Flags  = 0;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/*************************************************************************************************/
void Show_Zone_Corner_Or_Outline_While_Move_Mouse( WinEDA_DrawPanel* Panel, wxDC* DC, bool erase )
/*************************************************************************************************/

/* Redraws the zone outline when moving a corner according to the cursor position
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) Panel->m_Parent;
    ZONE_CONTAINER*  zone = (ZONE_CONTAINER*) pcbframe->GetCurItem();

    if( erase )    /* Undraw edge in old position*/
    {
        zone->Draw( Panel, DC, GR_XOR );
    }

    wxPoint          pos = pcbframe->GetScreen()->m_Curseur;
    if( zone->m_Flags == IS_MOVED )
    {
        wxPoint offset;
        offset = pos - s_CursorLastPosition;
        zone->Move( offset );
        s_CursorLastPosition = pos;
    }
    else if( zone->m_Flags == IS_DRAGGED )
    {
        wxPoint offset;
        offset = pos - s_CursorLastPosition;
        zone->MoveEdge( offset );
        s_CursorLastPosition = pos;
    }
    else
        zone->m_Poly->MoveCorner( zone->m_CornerSelection, pos.x, pos.y );

    zone->Draw( Panel, DC, GR_XOR );
}


/*************************************************/
int WinEDA_PcbFrame::Begin_Zone( wxDC* DC )
/*************************************************/

/**
 * Function Begin_Zone
 * either initializes the first segment of a new zone, or adds an
 * intermediate segment.
 * A new zone can be:
 * created from scratch: the user will be prompted to define parameters (layer, clearence ...)
 * created from a similar zone (s_CurrentZone is used): parameters are copied from s_CurrentZone
 * created as a cutout (an hole) inside s_CurrentZone
 */
{
    // verify if s_CurrentZone exists (could be deleted since last selection) :
    int ii;
    for( ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        if( s_CurrentZone == GetBoard()->GetArea( ii ) )
            break;
    }

    if( ii >= GetBoard()->GetAreaCount() ) // Not found: could be deleted since last selection
    {
        s_AddCutoutToCurrentZone = false;
        s_CurrentZone = NULL;
    }

    // If no zone contour in progress, a new zone is beeing created:
    if( GetBoard()->m_CurrentZoneContour == NULL )
        GetBoard()->m_CurrentZoneContour = new ZONE_CONTAINER( GetBoard() );

    ZONE_CONTAINER* zone = GetBoard()->m_CurrentZoneContour;
    if( zone->GetNumCorners() == 0 )    /* Start a new contour: init zone params (net, layer ...) */
    {
        if( s_CurrentZone == NULL )     // A new outline is created, from scratch
        {
            int diag;
            // Init zone params to reasonnable values
            zone->SetLayer( ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer );

            // Prompt user for parameters:
            DrawPanel->m_IgnoreMouseEvents = TRUE;
            if( zone->IsOnCopperLayer() )
            {   // Put a zone on a copper layer
                if ( g_HightLigth_NetCode )
                {
                    g_Zone_Default_Setting.m_NetcodeSelection = g_HightLigth_NetCode;
                    zone->SetNet( g_Zone_Default_Setting.m_NetcodeSelection );
                    zone->SetNetNameFromNetCode( );
                }

                wxGetApp().m_EDA_Config->Read( ZONE_THERMAL_RELIEF_GAP_STRING_KEY,
                    &g_Zone_Default_Setting.m_ThermalReliefGapValue );
                wxGetApp().m_EDA_Config->Read( ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY,
                    &g_Zone_Default_Setting.m_ThermalReliefCopperBridgeValue );

                dialog_copper_zone* frame = new dialog_copper_zone( this, &g_Zone_Default_Setting  );
                diag = frame->ShowModal();
                frame->Destroy();
            }
            else   // Put a zone on a non copper layer (technical layer)
            {
                diag = InstallDialogNonCopperZonesEditor( this, zone );
                g_Zone_Default_Setting.m_NetcodeSelection = 0;     // No net for non copper zones
            }
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_IgnoreMouseEvents = FALSE;

            if( diag ==  ZONE_ABORT )
                return 0;

            // Switch active layer to the selectec zonz layer
            ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer = g_Zone_Default_Setting.m_CurrentZone_Layer;
        }
        else  // Start a new contour: init zone params (net and layer) from an existing zone (add cutout or similar zone)
        {
            ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer = g_Zone_Default_Setting.m_CurrentZone_Layer =
                                                                s_CurrentZone->GetLayer();
            g_Zone_Default_Setting.ImportSetting( * s_CurrentZone);
        }

        /* Show the Net for zones on copper layers */
        if( g_Zone_Default_Setting.m_CurrentZone_Layer  < FIRST_NO_COPPER_LAYER )
        {
            if( s_CurrentZone )
                g_Zone_Default_Setting.m_NetcodeSelection = s_CurrentZone->GetNet();
            if( g_HightLigt_Status )
            {
                Hight_Light( DC ); // Remove old hightlight selection
            }

            g_HightLigth_NetCode = g_Zone_Default_Setting.m_NetcodeSelection;
            Hight_Light( DC );
        }
        if( !s_AddCutoutToCurrentZone )
            s_CurrentZone = NULL; // the zone is used only once ("add similar zone" command)
    }

    // if first segment
    if(  zone->GetNumCorners() == 0 )
    {
        zone->m_Flags = IS_NEW;
        zone->m_TimeStamp     = GetTimeStamp();
        g_Zone_Default_Setting.ExportSetting( *zone );
        zone->m_Poly->Start( g_Zone_Default_Setting.m_CurrentZone_Layer,
            GetScreen()->m_Curseur.x, GetScreen()->m_Curseur.y,
            zone->GetHatchStyle() );
        zone->AppendCorner( GetScreen()->m_Curseur );
        if( Drc_On && (m_drc->Drc( zone, 0 ) == BAD_DRC) && zone->IsOnCopperLayer() )
        {
            zone->m_Flags = 0;
            zone->RemoveAllContours();

            // use the form of SetCurItem() which does not write to the msg panel,
            // SCREEN::SetCurItem(), so the DRC error remains on screen.
            // WinEDA_PcbFrame::SetCurItem() calls DisplayInfo().
            GetScreen()->SetCurItem( NULL );
            DisplayError( this,
                _( "DRC error: this start point is inside or too close an other area" ) );
            return 0;
        }

        SetCurItem( zone );
        DrawPanel->ManageCurseur = Show_New_Edge_While_Move_Mouse;
        DrawPanel->ForceCloseManageCurseur = Abort_Zone_Create_Outline;
    }
    // edge in progress:
    else
    {
        ii = zone->GetNumCorners() - 1;

        /* edge in progress : the current corner coordinate was set by Show_New_Edge_While_Move_Mouse */
        if( zone->GetCornerPosition( ii - 1 ) != zone->GetCornerPosition( ii ) )
        {
            if( (Drc_On && m_drc->Drc( zone, ii - 1 ) == OK_DRC)
               || !zone->IsOnCopperLayer() ) // Ok, we can add a new corner
            {
                zone->AppendCorner( GetScreen()->m_Curseur );
                SetCurItem( zone );     // calls DisplayInfo().
            }
        }
    }

    return zone->GetNumCorners();
}


/*********************************************/
bool WinEDA_PcbFrame::End_Zone( wxDC* DC )
/*********************************************/

/** Function End_Zone
 * Terminates a zone outline creation
 * terminates (if no DRC error ) the zone edge creation process
 * @param DC = current Device Context
 * @return true if Ok, false if DRC error
 * if ok, put it in the main list GetBoard()->m_ZoneDescriptorList (a vector<ZONE_CONTAINER*>)
 */
{
    ZONE_CONTAINER* zone = GetBoard()->m_CurrentZoneContour;

    if( zone == NULL )
        return true;

    // Validate the curren outline:
    if( zone->GetNumCorners() <= 2 )   // An outline must have 3 corners or more
    {
        Abort_Zone_Create_Outline( DrawPanel, DC );
        return true;
    }

    // Validate the current edge:
    int icorner = zone->GetNumCorners() - 1;
    if( zone->IsOnCopperLayer() )
    {
        if( Drc_On && m_drc->Drc( zone, icorner - 1 ) == BAD_DRC )  // we can't validate last edge
            return false;
        if( Drc_On && m_drc->Drc( zone, icorner ) == BAD_DRC )      // we can't validate the closing edge
        {
            DisplayError( this,
                _( "DRC error: closing this area creates a drc error with an other area" ) );
            DrawPanel->MouseToCursorSchema();
            return false;
        }
    }

    zone->m_Flags = 0;

    zone->DrawWhileCreateOutline( DrawPanel, DC, GR_XOR );

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;

    // Undraw old drawings, because they can have important changes
    int layer = zone->GetLayer();
    GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_XOR, layer );
    GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_XOR, layer );

    /* Put edges in list */
    if( s_CurrentZone == NULL )
    {
        zone->m_Poly->Close(); // Close the current corner list
        GetBoard()->Add( zone );
        GetBoard()->m_CurrentZoneContour = NULL;
    }
    else    // Append this outline as a cutout to an existing zone
    {
        for( int ii = 0; ii < zone->GetNumCorners(); ii++ )
        {
            s_CurrentZone->AppendCorner( zone->GetCornerPosition( ii ) );
        }

        s_CurrentZone->m_Poly->Close(); // Close the current corner list
        zone->RemoveAllContours();      // All corners are copied in s_CurrentZone. Free corner list.
        zone = s_CurrentZone;
    }

    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    GetScreen()->SetCurItem( NULL );       // This outine can be deleted when merging outlines

    // Combine zones if possible :
    GetBoard()->AreaPolygonModified( zone, true, verbose );

    // Redraw the real edge zone :
    GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_OR, layer );
    GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_OR, layer );

    int ii = GetBoard()->GetAreaIndex( zone );   // test if zone_container exists
    if( ii < 0 )
        zone = NULL;                        // was removed by combining zones
    int error_count = GetBoard()->Test_Drc_Areas_Outlines_To_Areas_Outlines( zone, true );
    if( error_count )
    {
        DisplayError( this, _( "Area: DRC outline error" ) );
    }

    GetScreen()->SetModify();
    return true;
}


/******************************************************************************************/
static void Show_New_Edge_While_Move_Mouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/******************************************************************************************/

/* Redraws the edge zone when moving mouse
 */
{
    WinEDA_PcbFrame* pcbframe = (WinEDA_PcbFrame*) panel->m_Parent;
    wxPoint          c_pos    = pcbframe->GetScreen()->m_Curseur;
    ZONE_CONTAINER*  zone = pcbframe->GetBoard()->m_CurrentZoneContour;

    if( zone == NULL )
        return;

    int icorner = zone->GetNumCorners() - 1;
    if ( icorner < 1 )
        return;     // We must have 2 (or more) corners

    if( erase )    /* Undraw edge in old position*/
    {
        zone->DrawWhileCreateOutline( panel, DC );
    }

    /* Redraw the curent edge in its new position */
    if( g_Zone_45_Only )
    {
        // calculate the new position as allowed
        wxPoint StartPoint = zone->GetCornerPosition( icorner - 1 );
        Calcule_Coord_Extremite_45( StartPoint.x, StartPoint.y,
            &c_pos.x, &c_pos.y );
    }

    zone->SetCornerPosition( icorner, c_pos );

    zone->DrawWhileCreateOutline( panel, DC );
}


/***********************************************************************************/
void WinEDA_PcbFrame::Edit_Zone_Params( wxDC* DC, ZONE_CONTAINER* zone_container )
/***********************************************************************************/

/**
 * Function Edit_Zone_Params
 * Edit params (layer, clearance, ...) for a zone outline
 */
{
    int diag;
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    if( zone_container->GetLayer() < FIRST_NO_COPPER_LAYER )
    {   // edit a zone on a copper layer
        g_Zone_Default_Setting.ImportSetting(*zone_container);
        dialog_copper_zone* frame = new dialog_copper_zone( this, &g_Zone_Default_Setting );
        diag = frame->ShowModal();
        frame->Destroy();
    }
    else   // edit a zone on a non copper layer (technical layer)
        diag = InstallDialogNonCopperZonesEditor( this, zone_container );

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    if( diag == ZONE_ABORT )
        return;

    // Undraw old zone outlines
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = GetBoard()->GetArea( ii );
        edge_zone->Draw( DrawPanel, DC, GR_XOR );
    }

    g_Zone_Default_Setting.ExportSetting( *zone_container);
    NETINFO_ITEM* net = GetBoard()->FindNet( g_Zone_Default_Setting.m_NetcodeSelection );
    if( net )   // net === NULL should not occur
        zone_container->m_Netname = net->GetNetname();


    // Combine zones if possible :
    GetBoard()->AreaPolygonModified( zone_container, true, verbose );

    // Redraw the real new zone outlines:
    GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_OR, -1 );

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
    int      ncont = zone_container->m_Poly->GetContour( zone_container->m_CornerSelection );

    EDA_Rect dirty = zone_container->GetBoundingBox();

    Delete_Zone_Fill( DC, NULL, zone_container->m_TimeStamp );  // Remove fill segments

    if( ncont == 0 )    // This is the main outline: remove all
    {
        SaveCopyInUndoList( zone_container, UR_DELETED );
        GetBoard()->Remove( zone_container );
    }

    else
    {
        zone_container->m_Poly->RemoveContour( ncont );
    }

    DrawPanel->PostDirtyRect( dirty );

    GetScreen()->SetModify();
}


/***************************************************************************************/
int WinEDA_PcbFrame::Fill_Zone( wxDC* DC, ZONE_CONTAINER* zone_container, bool verbose )
/***************************************************************************************/

/** Function Fill_Zone()
 *  Calculate the zone filling for the outline zone_container
 *  The zone outline is a frontier, and can be complex (with holes)
 *  The filling starts from starting points like pads, tracks.
 * If exists, the old filling is removed
 * @param DC = current Device Context
 * @param zone_container = zone to fill
 * @param verbose = true to show error messages
 * @return error level (0 = no error)
 */
{
    wxString msg;

    MsgPanel->EraseMsgBox();
    if( GetBoard()->ComputeBoundaryBox() == FALSE )
    {
        if( verbose )
            DisplayError( this, wxT( "Board is empty!" ), 10 );
        return -1;
    }

    /* Shows the Net */
    g_Zone_Default_Setting.m_NetcodeSelection = zone_container->GetNet();
    if( g_HightLigt_Status && (g_HightLigth_NetCode != g_Zone_Default_Setting.m_NetcodeSelection)  && DC )
    {
        Hight_Light( DC );      // Remove old highlight selection
    }

    g_HightLigth_NetCode = g_Zone_Default_Setting.m_NetcodeSelection;
    if( DC )
        Hight_Light( DC );

    if( g_HightLigth_NetCode > 0 )
    {
        NETINFO_ITEM* net = GetBoard()->FindNet( g_HightLigth_NetCode );
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
            msg = net->GetNetname();
    }
    else
        msg = _( "No Net" );

    Affiche_1_Parametre( this, 22, _( "NetName" ), msg, RED );
    wxBusyCursor dummy;     // Shows an hourglass cursor (removed by its destructor)

    int          error_level = 0;
    zone_container->m_FilledPolysList.clear();
    Delete_Zone_Fill( NULL, NULL, zone_container->m_TimeStamp );
    zone_container->BuildFilledPolysListData( GetBoard() );
    if ( DC )
        DrawPanel->Refresh();

    GetScreen()->SetModify();

    return error_level;
}


/************************************************************/
int WinEDA_PcbFrame::Fill_All_Zones( bool verbose )
/************************************************************/

/** Function Fill_All_Zones()
 *  Fill all zones on the board
 * The old fillings are removed
 * @param verbose = true to show error messages
 * @return error level (0 = no error)
 */
{
    ZONE_CONTAINER* zone_container;
    int             error_level = 0;

    // Remove all zones :
    GetBoard()->m_Zone.DeleteAll();

    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        zone_container = GetBoard()->GetArea( ii );
        error_level    = Fill_Zone( NULL, zone_container, verbose );
        if( error_level && !verbose )
            break;
    }
    test_connexions( NULL );
    Tst_Ratsnest( NULL, 0 );    // Recalculate the active ratsnest, i.e. the unconnected links */
    DrawPanel->Refresh( true );
    return error_level;
}


