/****************************************************/
/*	BLOCK.CPP										*/
/* Gestion des Operations sur Blocks et Effacements */
/****************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "class_marker_sch.h"

#include "protos.h"

/* Variables Locales */

/* Fonctions exportees */
void               MoveItemsInList( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList, const wxPoint aMoveVector );
void               MirrorListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& aMirrorPoint );
void               MirrorOneStruct( SCH_ITEM* DrawStruct, wxPoint&  aMirrorPoint );


/*
* Small function to mirror (relative to a vertiacl axis at aMirrorPoint.x position)
* a given point
*/
static void MirrorYPoint( wxPoint& point, wxPoint& aMirrorPoint )
{
    point.x -= aMirrorPoint.x;
    NEGATE( point.x );
    point.x += aMirrorPoint.x;
}


/**************************************************************/
void MirrorOneStruct( SCH_ITEM* DrawStruct, wxPoint& aMirrorPoint )
/**************************************************************/

/* Given a structure rotate it to 90 degrees refer to the aMirrorPoint point.
 */
{
    int dx;
    DrawPolylineStruct*            DrawPoly;
    DrawJunctionStruct*            DrawConnect;
    EDA_DrawLineStruct*            DrawSegment;
    DrawBusEntryStruct*            DrawRaccord;
    SCH_COMPONENT*                 DrawLibItem;
    DrawSheetStruct*               DrawSheet;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    MARKER_SCH* DrawMarker;
    DrawNoConnectStruct*           DrawNoConnect;
    SCH_TEXT* DrawText;
    wxPoint px;

    if( !DrawStruct )
        return;

    switch( DrawStruct->Type() )
    {
    case TYPE_NOT_INIT:
        break;

    case DRAW_POLYLINE_STRUCT_TYPE:
        DrawPoly = (DrawPolylineStruct*) DrawStruct;
        for( unsigned ii = 0; ii < DrawPoly->GetCornerCount(); ii++ )
        {
            wxPoint point;
            point = DrawPoly->m_PolyPoints[ii];
            MirrorYPoint( point, aMirrorPoint );
            DrawPoly->m_PolyPoints[ii] = point;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        DrawSegment = (EDA_DrawLineStruct*) DrawStruct;
        if( (DrawSegment->m_Flags & STARTPOINT) == 0 )
        {
            MirrorYPoint( DrawSegment->m_Start, aMirrorPoint );
        }
        if( (DrawSegment->m_Flags & ENDPOINT) == 0 )
        {
            MirrorYPoint( DrawSegment->m_End, aMirrorPoint );
        }
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        DrawRaccord = (DrawBusEntryStruct*) DrawStruct;
        MirrorYPoint( DrawRaccord->m_Pos, aMirrorPoint );
        NEGATE( DrawRaccord->m_Size.x );
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        DrawConnect = (DrawJunctionStruct*) DrawStruct;
        MirrorYPoint( DrawConnect->m_Pos, aMirrorPoint );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        DrawMarker = (MARKER_SCH*) DrawStruct;
        MirrorYPoint( DrawMarker->m_Pos, aMirrorPoint );
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        DrawNoConnect = (DrawNoConnectStruct*) DrawStruct;
        MirrorYPoint( DrawNoConnect->m_Pos, aMirrorPoint );
        break;

    case TYPE_SCH_TEXT:
    case TYPE_SCH_LABEL:

        // Text is NOT really mirrored; it is moved to a suitable position
        // which is the closest position for a true mirrored text
        // The center position is mirrored and the text is moved for half horizontal len
        DrawText = (SCH_TEXT*) DrawStruct;
        px = DrawText->m_Pos;
        if( DrawText->m_Orient == 0 )       /* horizontal text */
            dx = DrawText->LenSize( DrawText->m_Text ) / 2;
        else if( DrawText->m_Orient == 2 )  /* invert horizontal text*/
            dx = -DrawText->LenSize( DrawText->m_Text ) / 2;
        else
            dx = 0;
        px.x += dx;
        MirrorYPoint( px, aMirrorPoint );
        px.x -= dx;
        DrawText->m_Pos.x = px.x;
        break;

    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:

        // Text is not really mirrored: Orientation is changed
        DrawText = (SCH_LABEL*) DrawStruct;
        if( DrawText->m_Orient == 0 )       /* horizontal text */
            DrawText->m_Orient = 2;
        else if( DrawText->m_Orient == 2 )  /* invert horizontal text*/
            DrawText->m_Orient = 0;

        px = DrawText->m_Pos;
        MirrorYPoint( px, aMirrorPoint );
        DrawText->m_Pos.x = px.x;
        break;

    case TYPE_SCH_COMPONENT:
    {
        DrawLibItem = (SCH_COMPONENT*) DrawStruct;
        dx = DrawLibItem->m_Pos.x;
        WinEDA_SchematicFrame* frame = (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
        frame->CmpRotationMiroir( DrawLibItem, NULL, CMP_MIROIR_Y );
        MirrorYPoint( DrawLibItem->m_Pos, aMirrorPoint );
        dx -= DrawLibItem->m_Pos.x;

        for( int ii = 0; ii < DrawLibItem->GetFieldCount(); ii++ )
        {
            /* move the fields to the new position because the component itself has moved */
            DrawLibItem->GetField( ii )->m_Pos.x -= dx;
        }
    }
        break;

    case DRAW_SHEET_STRUCT_TYPE:
        DrawSheet = (DrawSheetStruct*) DrawStruct;
        MirrorYPoint( DrawSheet->m_Pos, aMirrorPoint );
        DrawSheet->m_Pos.x -= DrawSheet->m_Size.x;

        DrawSheetLabel = DrawSheet->m_Label;
        while( DrawSheetLabel != NULL )
        {
            MirrorYPoint( DrawSheetLabel->m_Pos, aMirrorPoint );
            DrawSheetLabel->m_Edge = DrawSheetLabel->m_Edge ? 0 : 1;
            DrawSheetLabel =
                (Hierarchical_PIN_Sheet_Struct*) DrawSheetLabel->Next();
        }

        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        DrawSheetLabel = (Hierarchical_PIN_Sheet_Struct*) DrawStruct;
        MirrorYPoint( DrawSheetLabel->m_Pos, aMirrorPoint );
        break;

    default:
        break;
    }
}


/*****************************************************************************
* Routine to Mirror objects.							 *
*****************************************************************************/
void MirrorListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& aMirrorPoint )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetItemData( ii );
        MirrorOneStruct( item, aMirrorPoint );      // Place it in its new position.
        item->m_Flags = 0;
    }
}



/*****************************************************************************
* Routine to place a given object.											 *
*****************************************************************************/
void MoveItemsInList( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList, const wxPoint aMoveVector )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetItemData( ii );
        MoveOneStruct( item, aMoveVector );
    }
}


/**************************************************************************/
void MoveOneStruct( SCH_ITEM* aItem, const wxPoint& aMoveVector )
/*************************************************************************/
/* Given a structure move it by aMoveVector.
 */
{
    DrawPolylineStruct*            DrawPoly;
    DrawJunctionStruct*            DrawConnect;
    EDA_DrawLineStruct*            DrawSegment;
    DrawBusEntryStruct*            DrawRaccord;
    SCH_COMPONENT*                 DrawLibItem;
    DrawSheetStruct*               DrawSheet;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    MARKER_SCH* DrawMarker;
    DrawNoConnectStruct*           DrawNoConnect;

    if( !aItem )
        return;

    switch( aItem->Type() )
    {
    case TYPE_NOT_INIT:
        break;

    case DRAW_POLYLINE_STRUCT_TYPE:
        DrawPoly = (DrawPolylineStruct*) aItem;
        for( unsigned ii = 0; ii < DrawPoly->GetCornerCount(); ii++ )
        {
            DrawPoly->m_PolyPoints[ii] += aMoveVector;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        DrawSegment = (EDA_DrawLineStruct*) aItem;
        if( (DrawSegment->m_Flags & STARTPOINT) == 0 )
        {
            DrawSegment->m_Start += aMoveVector;
        }
        if( (DrawSegment->m_Flags & ENDPOINT) == 0 )
        {
            DrawSegment->m_End += aMoveVector;
        }
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        DrawRaccord = (DrawBusEntryStruct*) aItem;
        DrawRaccord->m_Pos += aMoveVector;
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        DrawConnect = (DrawJunctionStruct*) aItem;
        DrawConnect->m_Pos += aMoveVector;
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        DrawMarker = (MARKER_SCH*) aItem;
        DrawMarker->m_Pos += aMoveVector;
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        DrawNoConnect = (DrawNoConnectStruct*) aItem;
        DrawNoConnect->m_Pos += aMoveVector;
        break;

    case TYPE_SCH_TEXT:
             #define DrawText ( (SCH_TEXT*) aItem )
        DrawText->m_Pos += aMoveVector;
        break;

    case TYPE_SCH_LABEL:
             #define DrawLabel ( (SCH_LABEL*) aItem )
        DrawLabel->m_Pos += aMoveVector;
        break;

    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:
             #define DrawGHLabel ( (SCH_LABEL*) aItem )
        DrawGHLabel->m_Pos += aMoveVector;
        break;

    case TYPE_SCH_COMPONENT:
        DrawLibItem = (SCH_COMPONENT*) aItem;
        DrawLibItem->m_Pos += aMoveVector;
        for( int ii = 0; ii < DrawLibItem->GetFieldCount(); ii++ )
        {
            DrawLibItem->GetField( ii )->m_Pos += aMoveVector;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        DrawSheet = (DrawSheetStruct*) aItem;
        DrawSheet->m_Pos += aMoveVector;
        DrawSheetLabel    = DrawSheet->m_Label;
        while( DrawSheetLabel != NULL )
        {
            DrawSheetLabel->m_Pos += aMoveVector;
            DrawSheetLabel = DrawSheetLabel->Next();
        }

        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        DrawSheetLabel = (Hierarchical_PIN_Sheet_Struct*) aItem;
        DrawSheetLabel->m_Pos += aMoveVector;
        break;

    default:
        break;
    }
}
