/*********************************/
/* Module de nettoyage du schema */
/*********************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "trigo.h"
#include "confirm.h"
#include "macros.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "netlist.h"


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
    int       flag;
    bool      Modify = FALSE;

    WinEDA_SchematicFrame* frame;

    frame = (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

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

    frame->TestDanglingEnds( EEDrawList, DC );
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
        DisplayError( NULL,
                     wxT( "BreakSegmentOnJunction() error: NULL screen" ) );
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
        case DRAW_POLYLINE_STRUCT_TYPE:
        case TYPE_MARKER_SCH:
        case TYPE_SCH_TEXT:
        case DRAW_SHEET_STRUCT_TYPE:
        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        default:
            break;
        }
        DrawList = DrawList->Next();
    }
}


/* Break a segment ( BUS, WIRE ) int 2 segments at location aBreakpoint,
 * if aBreakpoint in on segment segment
 * ( excluding ends)
 * fill aPicklist with modified items if non null
 */
void BreakSegment(SCH_SCREEN * aScreen, wxPoint aBreakpoint )
{
    EDA_DrawLineStruct* segment, * NewSegment;
    for( SCH_ITEM* DrawList = aScreen->EEDrawList;DrawList; DrawList = DrawList->Next() )
    {
    if( DrawList->Type() != DRAW_SEGMENT_STRUCT_TYPE )
        continue;

        segment = (EDA_DrawLineStruct*) DrawList;

        if( !TestSegmentHit( aBreakpoint, segment->m_Start, segment->m_End, 0 ) )
            continue;

        /* Segment connecte: doit etre coupe en 2 si px,py n'est
         *  pas une extremite */
        if( (segment->m_Start == aBreakpoint) || (segment->m_End == aBreakpoint ) )
            continue;
        /* Ici il faut couper le segment en 2 */
        NewSegment = segment->GenCopy();
        NewSegment->m_Start = aBreakpoint;
        segment->m_End = NewSegment->m_Start;
        NewSegment->SetNext( segment->Next() );
        segment->SetNext( NewSegment );
        DrawList = NewSegment;
    }
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
        if( atan2( (double) ( RefSegm->m_Start.x - RefSegm->m_End.x ),
                  (double) ( RefSegm->m_Start.y - RefSegm->m_End.y ) ) ==
           atan2( (double) ( TstSegm->m_Start.x - TstSegm->m_End.x ),
                 (double) ( TstSegm->m_Start.y - TstSegm->m_End.y ) ) )
        {
            RefSegm->m_End = TstSegm->m_End;
            return 1;
        }
    }

    return 0;
}
