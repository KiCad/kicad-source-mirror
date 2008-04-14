/*********************************/
/* Module de nettoyage du schema */
/*********************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "macros.h"
#include "protos.h"


/* Routines locales */
static int TstAlignSegment( EDA_DrawLineStruct* RefSegm, EDA_DrawLineStruct* TstSegm );

/* Variable locales */


/*******************************************/
bool SCH_SCREEN::SchematicCleanUp( wxDC* DC )
/*******************************************/

/* Routine de nettoyage:
 *  - regroupe les segments de fils (ou de bus) alignes en 1 seul segment
 *  - Detecte les objets identiques superposes
 */
{
    SCH_ITEM* DrawList, * TstDrawList;
    int             flag;
    bool            Modify = FALSE;

    DrawList = EEDrawList;
    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        if( DrawList->Type() == DRAW_SEGMENT_STRUCT_TYPE )
        {
            TstDrawList = DrawList->Next();
            while( TstDrawList )
            {
                if( TstDrawList->Type() == DRAW_SEGMENT_STRUCT_TYPE )
                {
                    flag = TstAlignSegment( (EDA_DrawLineStruct*) DrawList,
                                           (EDA_DrawLineStruct*) TstDrawList );
                    if( flag )  /* Suppression de TstSegm */
                    {
                        /* keep the bits set in .m_Flags, because the deleted segment can be flagged */
                        DrawList->m_Flags |= TstDrawList->m_Flags;
                        EraseStruct( TstDrawList, this );
                        SetRefreshReq();
                        TstDrawList = EEDrawList;
                        Modify = TRUE;
                    }
                    else
                        TstDrawList = TstDrawList->Next();
                }
                else
                    TstDrawList = TstDrawList->Next();
            }
        }
    }

    g_EDA_Appl->m_SchematicFrame->TestDanglingEnds( EEDrawList, DC );
    return Modify;
}


/***********************************************/
void BreakSegmentOnJunction( SCH_SCREEN* Screen )
/************************************************/

/* Routine creant des debuts / fin de segment (BUS ou WIRES) sur les jonctions
 *  et les raccords
 */
{
    SCH_ITEM* DrawList;

    if( Screen == NULL )
    {
        DisplayError( NULL, wxT( "BreakSegmentOnJunction() error: NULL screen" ) );
        return;
    }

    DrawList = Screen->EEDrawList;
    while( DrawList )
    {
        switch( DrawList->Type() )
        {
        case DRAW_JUNCTION_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawJunctionStruct*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            break;

        case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawBusEntryStruct*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            BreakSegment( Screen, STRUCT->m_End() );
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
        case DRAW_NOCONNECT_STRUCT_TYPE:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_COMPONENT:
        case DRAW_PICK_ITEM_STRUCT_TYPE:
        case DRAW_POLYLINE_STRUCT_TYPE:
        case DRAW_MARKER_STRUCT_TYPE:
        case TYPE_SCH_TEXT:
        case DRAW_SHEET_STRUCT_TYPE:
        case DRAW_SHEETLABEL_STRUCT_TYPE:
            break;

        default:
            break;
        }
        DrawList = DrawList->Next();
    }
}


/*********************************************************/
DrawPickedStruct* BreakSegment( SCH_SCREEN* screen,
                                wxPoint breakpoint, bool PutInUndoList )
/*********************************************************/

/* Coupe un segment ( BUS, WIRE ) en 2 au point breakpoint,
 *  - si ce point est sur le segment
 *  - extremites non comprises
 *  If PutInUndoList == TRUE, create a list of modifictions, for undo command
 */
{
    EDA_BaseStruct* DrawList;
    EDA_DrawLineStruct* segment, * NewSegment;
    int ox, oy, fx, fy;
    DrawPickedStruct* List = NULL;

    DrawList = screen->EEDrawList;
    while( DrawList )
    {
        switch( DrawList->Type() )
        {
        case DRAW_SEGMENT_STRUCT_TYPE:
            segment = (EDA_DrawLineStruct*) DrawList;
            ox = segment->m_Start.x; oy = segment->m_Start.y;
            fx = segment->m_End.x; fy = segment->m_End.y;
            if( distance( fx - ox, fy - oy, breakpoint.x - ox, breakpoint.y - oy, 0 ) == 0 )
                break;

            /* Segment connecte: doit etre coupe en 2 si px,py n'est
             *  pas une extremite */
            if( (ox == breakpoint.x) && (oy == breakpoint.y ) )
                break;
            if( (fx == breakpoint.x) && (fy == breakpoint.y ) )
                break;
            /* Ici il faut couper le segment en 2 */
            if( PutInUndoList )         // First: put copy of the old segment in undo list
            {
                DrawPickedStruct* wrapper = new DrawPickedStruct();

                wrapper->m_Flags = IS_CHANGED;
                wrapper->m_PickedStruct = segment->GenCopy();
                wrapper->m_Image = segment;
                wrapper->m_PickedStruct->m_Image = segment;
                wrapper->Pnext = List;
                List = wrapper;
            }
            NewSegment = segment->GenCopy();
            NewSegment->m_Start = breakpoint;
            segment->m_End    = NewSegment->m_Start;
            NewSegment->Pnext = segment->Pnext;
            segment->Pnext    = NewSegment;
            DrawList = NewSegment;
            if( PutInUndoList )
            {
                DrawPickedStruct* wrapper = new DrawPickedStruct();

                wrapper->m_Flags = IS_NEW;
                wrapper->m_Image = NewSegment;
                wrapper->Pnext   = List;
                List = wrapper;
            }
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
        case DRAW_BUSENTRY_STRUCT_TYPE:
        case DRAW_POLYLINE_STRUCT_TYPE:
            break;

        default:
            break;
        }

        DrawList = DrawList->Pnext;
    }

    return List;
}


/***********************************************************/
static int TstAlignSegment( EDA_DrawLineStruct* RefSegm,
                            EDA_DrawLineStruct* TstSegm )
/***********************************************************/

/* Search if the 2 segments RefSegm and TstSegm are on a line.
 *  Retourn 0 if no
 *      1 if yes, and RefSegm is modified to be the equivalent segment
 */
{
    if( RefSegm == TstSegm )
        return 0;
    if( RefSegm->GetLayer() != TstSegm->GetLayer() )
        return 0;

    // search for a common end, and modify coordinates to ensure RefSegm->m_End == TstSegm->m_Start
    if( RefSegm->m_Start == TstSegm->m_Start )
    {
        if( RefSegm->m_End == TstSegm->m_End )          // trivial case: RefSegm and TstSegm are identical
            return 1;
        EXCHG( RefSegm->m_Start, RefSegm->m_End );      // at this point, RefSegm->m_End == TstSegm->m_Start
    }
    else if( RefSegm->m_Start == TstSegm->m_End )
    {
        EXCHG( RefSegm->m_Start, RefSegm->m_End );
        EXCHG( TstSegm->m_Start, TstSegm->m_End );    // at this point, RefSegm->m_End == TstSegm->m_Start
    }
    else if( RefSegm->m_End == TstSegm->m_End )
    {
        EXCHG( TstSegm->m_Start, TstSegm->m_End );      // at this point, RefSegm->m_End == TstSegm->m_Start
    }
    else if( RefSegm->m_End != TstSegm->m_Start )       // No common end point, segments cannot be merged
        return 0;

    /* Test alignment: */
    if( RefSegm->m_Start.y == RefSegm->m_End.y )       // Horizontal segment
    {
        if( TstSegm->m_Start.y == TstSegm->m_End.y )
        {
            RefSegm->m_End = TstSegm->m_End;
            return 1;
        }
    }
    else if( RefSegm->m_Start.x == RefSegm->m_End.x )  // Vertical segment
    {
        if( TstSegm->m_Start.x == TstSegm->m_End.x )
        {
            RefSegm->m_End = TstSegm->m_End;
            return 1;
        }
    }
    else
    {
        if( atan2( RefSegm->m_Start.x - RefSegm->m_End.x, RefSegm->m_Start.y -
                   RefSegm->m_End.y ) ==
           atan2( TstSegm->m_Start.x - TstSegm->m_End.x, TstSegm->m_Start.y - TstSegm->m_End.y ) )
        {
            RefSegm->m_End = TstSegm->m_End;
            return 1;
        }
    }

    return 0;
}
