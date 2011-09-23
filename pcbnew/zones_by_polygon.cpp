/**
 * @file zones_by_polygon.cpp
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxPcbStruct.h"

#include "class_board.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "zones.h"
#include "pcbnew_id.h"
#include "protos.h"
#include "zones_functions_for_undo_redo.h"
#include "drc_stuff.h"

bool s_Verbose = false;       // false if zone outline diags must not be shown

// Outline creation:
static void Abort_Zone_Create_Outline( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void Show_New_Edge_While_Move_Mouse( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                            const wxPoint& aPosition, bool aErase );

// Corner moving
static void Abort_Zone_Move_Corner_Or_Outlines( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void Show_Zone_Corner_Or_Outline_While_Move_Mouse( EDA_DRAW_PANEL* aPanel,
                                                          wxDC*           aDC,
                                                          const wxPoint&  aPosition,
                                                          bool            aErase );

/* Local variables */
static wxPoint         s_CornerInitialPosition;     // Used to abort a move corner command
static bool            s_CornerIsNew;               // Used to abort a move corner command (if it is a new corner, it must be deleted)
static bool            s_AddCutoutToCurrentZone;    // if true, the next outline will be addes to s_CurrentZone
static ZONE_CONTAINER* s_CurrentZone;               // if != NULL, these ZONE_CONTAINER params will be used for the next zone
static wxPoint         s_CursorLastPosition;        // in move zone outline, last cursor position. Used to calculate the move vector
static PICKED_ITEMS_LIST s_PickedList;              // a picked list to save zones for undo/redo command
static PICKED_ITEMS_LIST _AuxiliaryList;            // a picked list to store zones that are deleted or added when combined

#include "dialog_copper_zones.h"


/**
 * Function Add_Similar_Zone
 * Add a zone to a given zone outline.
 * if the zones are overlappeing they will be merged
 * @param DC = current Device Context
 * @param zone_container = parent zone outline
 */
void PCB_EDIT_FRAME::Add_Similar_Zone( wxDC* DC, ZONE_CONTAINER* zone_container )
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
    OnSelectTool( evt );
}


/**
 * Function Add_Zone_Cutout
 * Add a cutout zone to a given zone outline
 * @param DC = current Device Context
 * @param zone_container = parent zone outline
 */
void PCB_EDIT_FRAME::Add_Zone_Cutout( wxDC* DC, ZONE_CONTAINER* zone_container )
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
    OnSelectTool( evt );
}


/** Used **only** while creating a new zone outline
 * Remove and delete the current outline segment in progress
 * @return 0 if no corner in list, or corner number
 * if no corner in list, close the outline creation
 */
int PCB_EDIT_FRAME::Delete_LastCreatedCorner( wxDC* DC )
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

        if( DrawPanel->IsMouseCaptured() )
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
    }
    else
    {
        DrawPanel->SetMouseCapture( NULL, NULL );
        SetCurItem( NULL );
        zone->RemoveAllContours();
        zone->m_Flags = 0;
    }

    return zone->GetNumCorners();
}


/**
 * Function Abort_Zone_Create_Outline
 * cancels the Begin_Zone command if at least one EDGE_ZONE was created.
 */
static void Abort_Zone_Create_Outline( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Panel->GetParent();
    ZONE_CONTAINER* zone = pcbframe->GetBoard()->m_CurrentZoneContour;

    if( zone )
    {
        zone->DrawWhileCreateOutline( Panel, DC, GR_XOR );
        zone->m_Flags = 0;
        zone->RemoveAllContours();
    }

    pcbframe->SetCurItem( NULL );
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
    Panel->SetMouseCapture( NULL, NULL );
}


/**
 * Function Start_Move_Zone_Corner
 * Initialise parametres to move an existing corner of a zone.
 * if IsNewCorner is true, the Abort_Zone_Move_Corner_Or_Outlines will remove this corner, if called
 */
void PCB_EDIT_FRAME::Start_Move_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container,
                                             int corner_id, bool IsNewCorner )
{
    if( zone_container->IsOnCopperLayer() ) /* Show the Net */
    {
        if( GetBoard()->IsHighLightNetON() && DC )
        {
            High_Light( DC );  // Remove old hightlight selection
        }

        g_Zone_Default_Setting.m_NetcodeSelection = zone_container->GetNet();
        GetBoard()->SetHighLightNet( zone_container->GetNet() );

        if( DC )
            High_Light( DC );
    }


    // Prepare copy of old zones, for undo/redo.
    // if the corner is new, remove it from list, save and insert it in list
    int cx = zone_container->m_Poly->GetX( corner_id );
    int cy = zone_container->m_Poly->GetY( corner_id );

    if ( IsNewCorner )
        zone_container->m_Poly->DeleteCorner( corner_id );

    _AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList.ClearListAndDeleteItems();

    SaveCopyOfZones( s_PickedList, GetBoard(), zone_container->GetNet(),
                     zone_container->GetLayer() );

    if ( IsNewCorner )
        zone_container->m_Poly->InsertCorner(corner_id-1, cx, cy );

    zone_container->m_Flags  = IN_EDIT;
    DrawPanel->SetMouseCapture( Show_Zone_Corner_Or_Outline_While_Move_Mouse,
                                  Abort_Zone_Move_Corner_Or_Outlines );
    s_CornerInitialPosition = zone_container->GetCornerPosition( corner_id );
    s_CornerIsNew = IsNewCorner;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/**
 * Function Start_Move_Zone_Drag_Outline_Edge
 * Prepares a drag edge for an existing zone outline,
 */
void PCB_EDIT_FRAME::Start_Move_Zone_Drag_Outline_Edge( wxDC*           DC,
                                                        ZONE_CONTAINER* zone_container,
                                                        int             corner_id )
{
    zone_container->m_Flags = IS_DRAGGED;
    zone_container->m_CornerSelection = corner_id;
    DrawPanel->SetMouseCapture( Show_Zone_Corner_Or_Outline_While_Move_Mouse,
                                Abort_Zone_Move_Corner_Or_Outlines );
    s_CursorLastPosition     = s_CornerInitialPosition = GetScreen()->GetCrossHairPosition();
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    s_PickedList.ClearListAndDeleteItems();
    _AuxiliaryList.ClearListAndDeleteItems();
    SaveCopyOfZones( s_PickedList, GetBoard(), zone_container->GetNet(),
                     zone_container->GetLayer() );
}


/**
 * Function Start_Move_Zone_Outlines
 * Initialise parametres to move an existing zone outlines.
 */
void PCB_EDIT_FRAME::Start_Move_Zone_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container )
{
    /* Show the Net */
    if( zone_container->IsOnCopperLayer() ) /* Show the Net */
    {
        if( GetBoard()->IsHighLightNetON() )
        {
            High_Light( DC );  // Remove old hightlight selection
        }

        g_Zone_Default_Setting.m_NetcodeSelection = zone_container->GetNet();
        GetBoard()->SetHighLightNet( zone_container->GetNet() );
        High_Light( DC );
    }

    s_PickedList.ClearListAndDeleteItems();
    _AuxiliaryList.ClearListAndDeleteItems();
    SaveCopyOfZones( s_PickedList, GetBoard(), zone_container->GetNet(),
                     zone_container->GetLayer() );

    zone_container->m_Flags  = IS_MOVED;
    DrawPanel->SetMouseCapture( Show_Zone_Corner_Or_Outline_While_Move_Mouse,
                                Abort_Zone_Move_Corner_Or_Outlines );
    s_CursorLastPosition = s_CornerInitialPosition = GetScreen()->GetCrossHairPosition();
    s_CornerIsNew = false;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/**
 * Function End_Move_Zone_Corner_Or_Outlines
 * Terminates a move corner in a zone outline, or a move zone outlines
 * @param DC = current Device Context (can be NULL)
 * @param zone_container: the given zone
 */
void PCB_EDIT_FRAME::End_Move_Zone_Corner_Or_Outlines( wxDC* DC, ZONE_CONTAINER* zone_container )
{
    zone_container->m_Flags  = 0;
    DrawPanel->SetMouseCapture( NULL, NULL );

    if( DC )
        zone_container->Draw( DrawPanel, DC, GR_OR );

    OnModify();
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;

    SetCurItem( NULL );       // This outline can be deleted when merging outlines

    /* Combine zones if possible */
    wxBusyCursor dummy;
    GetBoard()->AreaPolygonModified( &_AuxiliaryList, zone_container, true, s_Verbose );
    DrawPanel->Refresh();


    int ii = GetBoard()->GetAreaIndex( zone_container );     // test if zone_container exists

    if( ii < 0 )
        zone_container = NULL;                          // was removed by combining zones

    UpdateCopyOfZonesList( s_PickedList, _AuxiliaryList, GetBoard() );
    SaveCopyInUndoList(s_PickedList, UR_UNSPECIFIED);
    s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    int error_count = GetBoard()->Test_Drc_Areas_Outlines_To_Areas_Outlines( zone_container, true );

    if( error_count )
    {
        DisplayError( this, _( "Area: DRC outline error" ) );
    }
}


/**
 * Function Remove_Zone_Corner
 * Remove the currently selected corner in a zone outline
 * the .m_CornerSelection is used as corner selection
 * @param DC = Current device context (can be NULL )
 * @param zone_container = the zone that contains the selected corner
 *  the member .m_CornerSelection is used as selected corner
 */
void PCB_EDIT_FRAME::Remove_Zone_Corner( wxDC* DC, ZONE_CONTAINER* zone_container )
{
    OnModify();

    if( zone_container->m_Poly->GetNumCorners() <= 3 )
    {
        DrawPanel->RefreshDrawingRect( zone_container->GetBoundingBox() );

        if( DC )
        {  // Remove the full zone because this is no more an area
            zone_container->UnFill();
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

    _AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList. ClearListAndDeleteItems();
    SaveCopyOfZones( s_PickedList, GetBoard(), zone_container->GetNet(),
                     zone_container->GetLayer() );
    zone_container->m_Poly->DeleteCorner( zone_container->m_CornerSelection );

    // modify zones outlines according to the new zone_container shape
    GetBoard()->AreaPolygonModified( &_AuxiliaryList, zone_container, true, s_Verbose );

    if( DC )
    {
        GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_OR, layer );
        GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_OR, layer );
    }

    UpdateCopyOfZonesList( s_PickedList, _AuxiliaryList, GetBoard() );
    SaveCopyInUndoList(s_PickedList, UR_UNSPECIFIED);
    s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    int ii = GetBoard()->GetAreaIndex( zone_container );     // test if zone_container exists

    if( ii < 0 )
        zone_container = NULL;   // zone_container does not exist anymaore, after combining zones

    int error_count = GetBoard()->Test_Drc_Areas_Outlines_To_Areas_Outlines( zone_container, true );

    if( error_count )
    {
        DisplayError( this, _( "Area: DRC outline error" ) );
    }
}


/**
 * Function Abort_Zone_Move_Corner_Or_Outlines
 * cancels the Begin_Zone state if at least one EDGE_ZONE has been created.
 */
void Abort_Zone_Move_Corner_Or_Outlines( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Panel->GetParent();
    ZONE_CONTAINER*  zone_container = (ZONE_CONTAINER*) pcbframe->GetCurItem();

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

    Panel->SetMouseCapture( NULL, NULL );
    _AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList. ClearListAndDeleteItems();
    Panel->Refresh();

    pcbframe->SetCurItem( NULL );
    zone_container->m_Flags  = 0;
    s_AddCutoutToCurrentZone = false;
    s_CurrentZone = NULL;
}


/* Redraws the zone outline when moving a corner according to the cursor position
 */
void Show_Zone_Corner_Or_Outline_While_Move_Mouse( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                                   const wxPoint& aPosition, bool aErase )
{
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) aPanel->GetParent();
    ZONE_CONTAINER* zone = (ZONE_CONTAINER*) pcbframe->GetCurItem();

    if( aErase )    /* Undraw edge in old position*/
    {
        zone->Draw( aPanel, aDC, GR_XOR );
    }

    wxPoint          pos = pcbframe->GetScreen()->GetCrossHairPosition();

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
    {
        zone->m_Poly->MoveCorner( zone->m_CornerSelection, pos.x, pos.y );
    }

    zone->Draw( aPanel, aDC, GR_XOR );
}


/**
 * Function Begin_Zone
 * either initializes the first segment of a new zone, or adds an
 * intermediate segment.
 * A new zone can be:
 * created from scratch: the user will be prompted to define parameters (layer, clearence ...)
 * created from a similar zone (s_CurrentZone is used): parameters are copied from s_CurrentZone
 * created as a cutout (an hole) inside s_CurrentZone
 */
int PCB_EDIT_FRAME::Begin_Zone( wxDC* DC )
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

    if( zone->GetNumCorners() == 0 )    // Start a new contour: init zone params (net, layer ...)
    {
        if( s_CurrentZone == NULL )     // A new outline is created, from scratch
        {
            int diag;
            // Init zone params to reasonnable values
            zone->SetLayer( getActiveLayer() );

            // Prompt user for parameters:
            DrawPanel->m_IgnoreMouseEvents = true;

            if( zone->IsOnCopperLayer() )
            {   // Put a zone on a copper layer
                if ( GetBoard()->GetHighLightNetCode() > 0 )
                {
                    g_Zone_Default_Setting.m_NetcodeSelection = GetBoard()->GetHighLightNetCode();

                    zone->SetNet( g_Zone_Default_Setting.m_NetcodeSelection );
                    zone->SetNetNameFromNetCode( );
                }

                wxGetApp().m_EDA_Config->Read( ZONE_THERMAL_RELIEF_GAP_STRING_KEY,
                                               &g_Zone_Default_Setting.m_ThermalReliefGapValue );
                wxGetApp().m_EDA_Config->Read( ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY,
                                               &g_Zone_Default_Setting.m_ThermalReliefCopperBridgeValue );

                g_Zone_Default_Setting.m_CurrentZone_Layer = zone->GetLayer();
                DIALOG_COPPER_ZONE* frame = new DIALOG_COPPER_ZONE( this, &g_Zone_Default_Setting  );
                diag = frame->ShowModal();
                frame->Destroy();
            }
            else   // Put a zone on a non copper layer (technical layer)
            {
                diag = InstallDialogNonCopperZonesEditor( zone );
                g_Zone_Default_Setting.m_NetcodeSelection = 0;     // No net for non copper zones
            }

            DrawPanel->MoveCursorToCrossHair();
            DrawPanel->m_IgnoreMouseEvents = false;

            if( diag ==  ZONE_ABORT )
                return 0;

            // Switch active layer to the selectec zonz layer
            setActiveLayer( g_Zone_Default_Setting.m_CurrentZone_Layer );
        }
        else  // Start a new contour: init zone params (net and layer) from an existing
        {     // zone (add cutout or similar zone)
            g_Zone_Default_Setting.m_CurrentZone_Layer = s_CurrentZone->GetLayer();
            setActiveLayer( s_CurrentZone->GetLayer() );
            g_Zone_Default_Setting.ImportSetting( * s_CurrentZone);
        }

        /* Show the Net for zones on copper layers */
        if( g_Zone_Default_Setting.m_CurrentZone_Layer  < FIRST_NO_COPPER_LAYER )
        {
            if( s_CurrentZone )
                g_Zone_Default_Setting.m_NetcodeSelection = s_CurrentZone->GetNet();

            if( GetBoard()->IsHighLightNetON() )
            {
                High_Light( DC ); // Remove old hightlight selection
            }

            GetBoard()->SetHighLightNet( g_Zone_Default_Setting.m_NetcodeSelection );
            High_Light( DC );
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
                             GetScreen()->GetCrossHairPosition().x,
                             GetScreen()->GetCrossHairPosition().y,
                             zone->GetHatchStyle() );
        zone->AppendCorner( GetScreen()->GetCrossHairPosition() );

        if( Drc_On && (m_drc->Drc( zone, 0 ) == BAD_DRC) && zone->IsOnCopperLayer() )
        {
            zone->m_Flags = 0;
            zone->RemoveAllContours();

            // use the form of SetCurItem() which does not write to the msg panel,
            // SCREEN::SetCurItem(), so the DRC error remains on screen.
            // PCB_EDIT_FRAME::SetCurItem() calls DisplayInfo().
            GetScreen()->SetCurItem( NULL );
            DisplayError( this,
                          _( "DRC error: this start point is inside or too close an other area" ) );
            return 0;
        }

        SetCurItem( zone );
        DrawPanel->SetMouseCapture( Show_New_Edge_While_Move_Mouse, Abort_Zone_Create_Outline );
    }
    else    // edge in progress:
    {
        ii = zone->GetNumCorners() - 1;

        /* edge in progress : the current corner coordinate was set by Show_New_Edge_While_Move_Mouse */
        if( zone->GetCornerPosition( ii - 1 ) != zone->GetCornerPosition( ii ) )
        {
            if( !Drc_On || !zone->IsOnCopperLayer() || ( m_drc->Drc( zone, ii - 1 ) == OK_DRC ) )
            {   // Ok, we can add a new corner
                zone->AppendCorner( GetScreen()->GetCrossHairPosition() );
                SetCurItem( zone );     // calls DisplayInfo().
            }
        }
    }

    return zone->GetNumCorners();
}


/**
 * Function End_Zone
 * Terminates a zone outline creation
 * terminates (if no DRC error ) the zone edge creation process
 * @param DC = current Device Context
 * @return true if Ok, false if DRC error
 * if ok, put it in the main list GetBoard()->m_ZoneDescriptorList (a vector<ZONE_CONTAINER*>)
 */
bool PCB_EDIT_FRAME::End_Zone( wxDC* DC )
{
    ZONE_CONTAINER* zone = GetBoard()->m_CurrentZoneContour;

    if( zone == NULL )
        return true;

    // Validate the current outline:
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
            DrawPanel->MoveCursorToCrossHair();
            return false;
        }
    }

    zone->m_Flags = 0;

    zone->DrawWhileCreateOutline( DrawPanel, DC, GR_XOR );

    DrawPanel->SetMouseCapture( NULL, NULL );

    // Undraw old drawings, because they can have important changes
    int layer = zone->GetLayer();
    GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_XOR, layer );
    GetBoard()->RedrawFilledAreas( DrawPanel, DC, GR_XOR, layer );

    // Save initial zones configuration, for undo/redo, before adding new zone
    _AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList.ClearListAndDeleteItems();
    SaveCopyOfZones(s_PickedList, GetBoard(), zone->GetNet(), zone->GetLayer() );

    /* Put new zone in list */
    if( s_CurrentZone == NULL )
    {
        zone->m_Poly->Close(); // Close the current corner list
        GetBoard()->Add( zone );
        GetBoard()->m_CurrentZoneContour = NULL;
        // Add this zone in picked list, as new item
        ITEM_PICKER picker( zone, UR_NEW );
        s_PickedList.PushItem( picker );
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
    GetBoard()->AreaPolygonModified( &_AuxiliaryList, zone, true, s_Verbose );

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

    UpdateCopyOfZonesList( s_PickedList, _AuxiliaryList, GetBoard() );
    SaveCopyInUndoList(s_PickedList, UR_UNSPECIFIED);
    s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    OnModify();
    return true;
}


/* Redraws the zone outlines when moving mouse
 */
static void Show_New_Edge_While_Move_Mouse( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                            const wxPoint& aPosition, bool aErase )
{
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) aPanel->GetParent();
    wxPoint         c_pos    = pcbframe->GetScreen()->GetCrossHairPosition();
    ZONE_CONTAINER* zone = pcbframe->GetBoard()->m_CurrentZoneContour;

    if( zone == NULL )
        return;

    int icorner = zone->GetNumCorners() - 1;

    if ( icorner < 1 )
        return;     // We must have 2 (or more) corners

    if( aErase )    /* Undraw edge in old position*/
    {
        zone->DrawWhileCreateOutline( aPanel, aDC );
    }

    /* Redraw the curent edge in its new position */
    if( g_Zone_45_Only )
    {
        // calculate the new position as allowed
        wxPoint StartPoint = zone->GetCornerPosition( icorner - 1 );
        CalculateSegmentEndPoint( c_pos, StartPoint.x, StartPoint.y, &c_pos.x, &c_pos.y );
    }

    zone->SetCornerPosition( icorner, c_pos );

    zone->DrawWhileCreateOutline( aPanel, aDC );
}


/**
 * Function Edit_Zone_Params
 * Edit params (layer, clearance, ...) for a zone outline
 */
void PCB_EDIT_FRAME::Edit_Zone_Params( wxDC* DC, ZONE_CONTAINER* zone_container )
{
    int diag;
    DrawPanel->m_IgnoreMouseEvents = true;

    /* Save initial zones configuration, for undo/redo, before adding new zone
     * note the net name and the layer can be changed, so we must save all zones
     */
    _AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList.ClearListAndDeleteItems();
    SaveCopyOfZones(s_PickedList, GetBoard(), -1, -1 );

    if( zone_container->GetLayer() < FIRST_NO_COPPER_LAYER )
    {   // edit a zone on a copper layer
        g_Zone_Default_Setting.ImportSetting(*zone_container);
        DIALOG_COPPER_ZONE* frame = new DIALOG_COPPER_ZONE( this, &g_Zone_Default_Setting );
        diag = frame->ShowModal();
        frame->Destroy();
    }
    else   // edit a zone on a non copper layer (technical layer)
    {
        diag = InstallDialogNonCopperZonesEditor( zone_container );
    }

    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = false;

    if( diag == ZONE_ABORT )
    {
        _AuxiliaryList.ClearListAndDeleteItems();
        s_PickedList.ClearListAndDeleteItems();
        return;
    }

    if( diag == ZONE_EXPORT_VALUES )
    {
        UpdateCopyOfZonesList( s_PickedList, _AuxiliaryList, GetBoard() );
        SaveCopyInUndoList(s_PickedList, UR_UNSPECIFIED);
        s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items
        return;
    }

    // Undraw old zone outlines
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = GetBoard()->GetArea( ii );
        edge_zone->Draw( DrawPanel, DC, GR_XOR );
    }

    g_Zone_Default_Setting.ExportSetting( *zone_container);
    NETINFO_ITEM* net = GetBoard()->FindNet( g_Zone_Default_Setting.m_NetcodeSelection );

    if( net )   // net == NULL should not occur
        zone_container->m_Netname = net->GetNetname();

    // Combine zones if possible :
    GetBoard()->AreaPolygonModified( &_AuxiliaryList, zone_container, true, s_Verbose );

    // Redraw the real new zone outlines:
    GetBoard()->RedrawAreasOutlines( DrawPanel, DC, GR_OR, -1 );

    UpdateCopyOfZonesList( s_PickedList, _AuxiliaryList, GetBoard() );
    SaveCopyInUndoList(s_PickedList, UR_UNSPECIFIED);
    s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    OnModify();
}


/**
 * Function Delete_Zone_Contour
 * Remove the zone which include the segment aZone, or the zone which have the given time stamp.
 *  A zone is a group of segments which have the same TimeStamp
 * @param DC = current Device Context (can be NULL)
 * @param zone_container = zone to modify
 *  the member .m_CornerSelection is used to find the outline to remove.
 * if the outline is the main outline, all the zone_container is removed (deleted)
 * otherwise, the hole is deleted
 */
void PCB_EDIT_FRAME::Delete_Zone_Contour( wxDC* DC, ZONE_CONTAINER* zone_container )
{
    int      ncont = zone_container->m_Poly->GetContour( zone_container->m_CornerSelection );

    EDA_RECT dirty = zone_container->GetBoundingBox();

    // For compatibility with old boards: remove old SEGZONE fill segments
    Delete_OldZone_Fill( NULL, zone_container->m_TimeStamp );

    // Remove current filling:
    zone_container->UnFill();

    if( ncont == 0 )    // This is the main outline: remove all
    {
        SaveCopyInUndoList( zone_container, UR_DELETED );
        GetBoard()->Remove( zone_container );
    }

    else
    {
        zone_container->m_Poly->RemoveContour( ncont );
    }

    DrawPanel->RefreshDrawingRect( dirty );

    OnModify();
}
