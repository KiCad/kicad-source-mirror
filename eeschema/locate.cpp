/******************************************************/
/* Routines de localisation d'un element d'un schema. */
/******************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "trigo.h"
#include "macros.h"

#include "protos.h"

/* Routines exportees */
int         distance( int dx, int dy, int spot_cX, int spot_cY, int seuil );

/* Routines Locales */
static EDA_BaseStruct* LastSnappedStruct = NULL;
static int             PickedBoxMinX, PickedBoxMinY, PickedBoxMaxX, PickedBoxMaxY;
static bool IsBox1InBox2( int StartX1, int StartY1, int EndX1, int EndY1,
                          int StartX2, int StartY2, int EndX2, int EndY2 );
static bool IsPointInBox( int pX, int pY,
                          int BoxX1, int BoxY1, int BoxX2, int BoxY2 );
static bool IsPointOnSegment( int pX, int pY,
                              int SegmX1, int SegmY1, int SegmX2, int SegmY2, int seuil = 0 );
static bool SnapPoint2( const wxPoint& PosRef, int SearchMask,
                        EDA_BaseStruct* DrawList, DrawPickedStruct* DontSnapList, int zoom_value );


/*********************************************************************/
EDA_SchComponentStruct* LocateSmallestComponent( SCH_SCREEN* Screen )
/*********************************************************************/

/* Search the smaller (considering its area) component under the mouse cursor or the pcb cursor
 *  If more than 1 component is found, a pointer to the smaller component is returned
 */
{
    EDA_SchComponentStruct* DrawLibItem = NULL, * LastDrawLibItem = NULL;
    EDA_BaseStruct*         DrawList;
    EDA_Rect BoundaryBox;
    float    sizeref = 0, sizecurr;

    DrawList = Screen->EEDrawList;

    while( DrawList )
    {
        if( ( SnapPoint2( Screen->m_MousePosition, LIBITEM,
                         DrawList, NULL, Screen->GetZoom() ) ) == FALSE )
        {
            if( ( SnapPoint2( Screen->m_Curseur, LIBITEM,
                             DrawList, NULL, Screen->GetZoom() ) ) == FALSE )
                break;
        }
        DrawLibItem = (EDA_SchComponentStruct*) LastSnappedStruct;
        DrawList    = DrawLibItem->Pnext;
        if( LastDrawLibItem == NULL )  // First time a component is located
        {
            LastDrawLibItem = DrawLibItem;
            BoundaryBox = LastDrawLibItem->GetBoundaryBox();
            sizeref = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );
        }
        else
        {
            BoundaryBox = DrawLibItem->GetBoundaryBox();
            sizecurr    = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );
            if( sizeref > sizecurr )   // a smallest component is found
            {
                sizeref = sizecurr;
                LastDrawLibItem = DrawLibItem;
            }
        }
    }

    return LastDrawLibItem;
}


/* 	SearchMask = (bitwise OR):
 *  LIBITEM
 *  WIREITEM
 *  BUSITEM
 *  RACCORDITEM
 *  JUNCTIONITEM
 *  DRAWITEM
 *  TEXTITEM
 *  LABELITEM
 *  SHEETITEM
 *  MARKERITEM
 *  NOCONNECTITEM
 *  SEARCH_PINITEM
 *  SHEETLABELITEM
 *  FIELDCMPITEM
 * 
 *  if EXCLUDE_WIRE_BUS_ENDPOINTS is set, in wire ou bus search and locate,
 *  start and end points are not included in search
 *  if WIRE_BUS_ENDPOINTS_ONLY is set, in wire ou bus search and locate,
 *  only start and end points are included in search
 * 
 * 
 *  Return:
 *      -Bloc search:
 *          pointeur sur liste de pointeurs de structures si Plusieurs
 *                  structures selectionnees.
 *          pointeur sur la structure si 1 seule
 * 
 *      Positon serach:
 *          pointeur sur la structure.
 *      Si pas de structures selectionnees: retourne NULL
 * 
 */
/***********************************************************************/
EDA_BaseStruct* PickStruct( const wxPoint& refpos,
                            EDA_BaseStruct* DrawList, int SearchMask )
/************************************************************************/

/* Search an item at pos pos
 */
{
    bool Snapped;
    int  zoom = ActiveScreen->GetZoom();

    if( DrawList == NULL )
        return NULL;

    if( ( Snapped = SnapPoint2( refpos, SearchMask,
                                DrawList, NULL, zoom ) ) != FALSE )
    {
        return LastSnappedStruct;
    }
    return NULL;
}


/***********************************************************************/
EDA_BaseStruct* PickStruct( EDA_Rect& block,
                            EDA_BaseStruct* DrawList, int SearchMask )
/************************************************************************/

/* Search items in block
 */
{
    int               x, y, OrigX, OrigY;
    DrawPickedStruct* PickedList = NULL, * PickedItem;
    EDA_BaseStruct*   DrawStruct;

    OrigX = block.GetX();
    OrigY = block.GetY();
    x = block.GetRight();
    y = block.GetBottom();

    if( x < OrigX )
        EXCHG( x, OrigX );
    if( y < OrigY )
        EXCHG( y, OrigY );

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
    {
        if( DrawStructInBox( OrigX, OrigY, x, y, DrawStruct ) )
        {
            /* Put this structure in the picked list: */
            PickedItem = new DrawPickedStruct( DrawStruct );

            PickedItem->Pnext = PickedList;
            PickedList = PickedItem;
        }
    }

    if( PickedList && PickedList->Pnext == NULL )
    {
        /* Only one item was picked - convert to scalar form (no list): */
        PickedItem = PickedList;
        PickedList = (DrawPickedStruct*) PickedList->m_PickedStruct;
        delete PickedItem;
    }

    if( PickedList != NULL )
    {
        PickedBoxMinX = OrigX;  PickedBoxMinY = OrigY;
        PickedBoxMaxX = x; PickedBoxMaxY = y;
    }

    return PickedList;
}


/*****************************************************************************
* Routine to search all objects for the closest point to a given point, in	 *
* drawing space, and snap it to that points if closer than SnapDistance.	 *
* Note we use L1 norm as distance measure, as it is the fastest.			 *
* This routine updates LastSnappedStruct to the last object used in to snap  *
* a point. This variable is global to this module only (see above).			 *
* If DontSnapList is not NULL, structes in this list are skipped.			 *
* The routine returns TRUE if point was snapped.							 *
*****************************************************************************/
bool SnapPoint2( const wxPoint& PosRef, int SearchMask,
                 EDA_BaseStruct* DrawList, DrawPickedStruct* DontSnapList, int zoom_value )
{
    int i, * Points, x = PosRef.x, y = PosRef.y;
    int x1, y1, x2, y2, NumOfPoints2;
    DrawPickedStruct* DontSnap;
    int dx, dy;

    for( ; DrawList != NULL; DrawList = DrawList->Pnext )
    {
        /* Make sure this structure is NOT in the dont snap list: */
        DontSnap = DontSnapList;
        for( ; DontSnap != NULL; DontSnap = (DrawPickedStruct*) DontSnap->Pnext )
            if( DontSnap->m_PickedStruct == DrawList )
                break;

        if( DontSnap )
            if( DontSnap->m_PickedStruct == DrawList )
                continue;

        switch( DrawList->Type() )
        {
        case DRAW_POLYLINE_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawPolylineStruct*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;
            
            Points = STRUCT->m_Points;
            NumOfPoints2 = STRUCT->m_NumOfPoints * 2;
            for( i = 0; i < NumOfPoints2 - 2; i += 2 )
            {
                x1 = Points[i]; y1 = Points[i + 1];
                x2 = Points[i + 2]; y2 = Points[i + 3];
                if( IsPointOnSegment( x, y, x1, y1, x2, y2 ) )
                {
                    LastSnappedStruct = DrawList;
                    return TRUE;
                }
            }
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (EDA_DrawLineStruct*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;

            if( IsPointOnSegment( x, y, STRUCT->m_Start.x, STRUCT->m_Start.y,
                                  STRUCT->m_End.x, STRUCT->m_End.y ) )
            {
                if( ( (SearchMask & DRAWITEM) && (STRUCT->m_Layer == LAYER_NOTES) )
                   || ( (SearchMask & WIREITEM) && (STRUCT->m_Layer == LAYER_WIRE) )
                   || ( (SearchMask & BUSITEM) && (STRUCT->m_Layer == LAYER_BUS) )
                    )
                {
                    if( SearchMask & EXCLUDE_WIRE_BUS_ENDPOINTS )
                    {
                        if( x == STRUCT->m_Start.x && y == STRUCT->m_Start.y )
                            break;
                        if( x == STRUCT->m_End.x && y == STRUCT->m_End.y )
                            break;
                    }

                    if( SearchMask & WIRE_BUS_ENDPOINTS_ONLY )
                    {
                        if( !STRUCT->IsOneEndPointAt( wxPoint( x, y ) ) )
                            break;
                    }

                    LastSnappedStruct = DrawList;
                    return TRUE;
                }
            }
            break;


        case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawBusEntryStruct*) DrawList )
            if( !( SearchMask & (RACCORDITEM) ) )
                break;

            if( IsPointOnSegment( x, y, STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                                  STRUCT->m_End().x, STRUCT->m_End().y ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawJunctionStruct*) DrawList )
            if( !(SearchMask & JUNCTIONITEM) )
                break;
            dx = DRAWJUNCTION_SIZE / 2;
            x1 = STRUCT->m_Pos.x - dx;
            y1 = STRUCT->m_Pos.y - dx;
            x2 = STRUCT->m_Pos.x + dx;
            y2 = STRUCT->m_Pos.y + dx;
            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;


        case DRAW_NOCONNECT_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawNoConnectStruct*) DrawList )
            if( !(SearchMask & NOCONNECTITEM) )
                break;
            dx = (DRAWNOCONNECT_SIZE * zoom_value) / 2;
            x1 = STRUCT->m_Pos.x - dx;
            y1 = STRUCT->m_Pos.y - dx;
            x2 = STRUCT->m_Pos.x + dx;
            y2 = STRUCT->m_Pos.y + dx;
            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_MARKER_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawMarkerStruct*) DrawList )
            if( !(SearchMask & MARKERITEM) )
                break;
            dx = (DRAWMARKER_SIZE * zoom_value) / 2;
            x1 = STRUCT->m_Pos.x - dx;
            y1 = STRUCT->m_Pos.y - dx;
            x2 = STRUCT->m_Pos.x + dx;
            y2 = STRUCT->m_Pos.y + dx;
            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_LABEL_STRUCT_TYPE:
        case DRAW_TEXT_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawTextStruct*) DrawList )
            if( !( SearchMask & (TEXTITEM | LABELITEM) ) )
                break;
            dx = STRUCT->m_Size.x * STRUCT->GetLength();
            dy = STRUCT->m_Size.y;
            x1 = x2 = STRUCT->m_Pos.x;
            y1 = y2 = STRUCT->m_Pos.y;

            switch( STRUCT->m_Orient )
            {
            case 0:             /* HORIZONTAL */
                x2 += dx; y2 -= dy;
                break;

            case 1:             /* VERTICAL UP */
                x2 -= dy; y2 -= dx;
                break;

            case 2:             /* horizontal inverse */
                x2 -= dx; y2 += dy;
                break;

            case 3:             /* vertical DOWN */
                x2 += dy; y2 += dx;
                break;
            }

            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;


        case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawGlobalLabelStruct*) DrawList )
            if( !(SearchMask & LABELITEM) )
                break;
            dx = STRUCT->m_Size.x * ( STRUCT->GetLength() + 1 );        /* longueur */
            dy = STRUCT->m_Size.y / 2;                                  /* Demi hauteur */
            x1 = x2 = STRUCT->m_Pos.x;
            y1 = y2 = STRUCT->m_Pos.y;

            switch( STRUCT->m_Orient )
            {
            case 0:             /* HORIZONTAL */
                x2 -= dx; y2 += dy; y1 -= dy;
                break;

            case 1:             /* VERTICAL UP */
                x1 -= dy; x2 += dy; y2 += dx;
                break;

            case 2:             /* horizontal inverse */
                x2 += dx; y2 += dy; y1 -= dy;
                break;

            case 3:             /* vertical DOWN */
                x1 -= dy; x2 += dy; y2 -= dx;
                break;
            }

            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_LIB_ITEM_STRUCT_TYPE:
            if( !( SearchMask & (LIBITEM | FIELDCMPITEM) ) )
                break;

            if( SearchMask & FIELDCMPITEM )
            {
                PartTextStruct*         Field;
                EDA_SchComponentStruct* DrawLibItem = (EDA_SchComponentStruct*) DrawList;
                for( i = REFERENCE; i < NUMBER_OF_FIELDS; i++ )
                {
                    Field = &DrawLibItem->m_Field[i];
                    if( (Field->m_Attributs & TEXT_NO_VISIBLE) )
                        continue;
                    if( Field->IsVoid() )
                        continue;
                    EDA_Rect BoundaryBox = Field->GetBoundaryBox();
                    if( BoundaryBox.Inside( x, y ) )
                    {
                        LastSnappedStruct = Field;
                        return TRUE;
                    }
                }
            }
            else
            {
                #undef  STRUCT
                #define STRUCT ( (EDA_SchComponentStruct*) DrawList )
                EDA_Rect BoundaryBox = STRUCT->GetBoundaryBox();
                if( BoundaryBox.Inside( x, y ) )
                {
                    LastSnappedStruct = DrawList;
                    return TRUE;
                }
            }
            break;

        case DRAW_SHEET_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawSheetStruct*) DrawList )
            if( !(SearchMask & SHEETITEM) )
                break;
            /* Recalcul des coordonnees de l'encadrement du composant */
            x1 = STRUCT->m_Pos.x;
            y1 = STRUCT->m_Pos.y;
            x2 = STRUCT->m_Pos.x + STRUCT->m_Size.x;
            y2 = STRUCT->m_Pos.y + STRUCT->m_Size.y;

            if( IsPointInBox( x, y, x1, y1, x2, y2 ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_PICK_ITEM_STRUCT_TYPE:
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "SnapPoint2() error: unexpected strct type %d (" ), DrawList->Type() );
            msg << DrawList->ReturnClassName() << wxT( ")" );
            DisplayError( NULL, msg );
            break;
        }
        }
    }

    return FALSE;
}


/*****************************************************************************
* Routine to test if an object has non empty intersection with the box		 *
* defined by x1/y1 and x2/y2 (x1 < x2, y1 < y2), and return TRUE if so. This *
* routine is used to pick all points in a given box.						 *
*****************************************************************************/
bool DrawStructInBox( int x1, int y1, int x2, int y2,
                      EDA_BaseStruct* DrawStruct )
{
    int i, * Points, xt1, yt1, xt2, yt2, NumOfPoints2;
    int dx, dy;
    wxString msg;

    switch( DrawStruct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        #undef  STRUCT
        #define STRUCT ( (DrawPolylineStruct*) DrawStruct )
        Points = STRUCT->m_Points;
        NumOfPoints2 = STRUCT->m_NumOfPoints * 2;
        for( i = 0; i < NumOfPoints2; i += 2 )
        {
            if( Points[i] >= x1 && Points[i] <= x2
                && Points[i + 1] >= y1 && Points[i + 1] <=y2 )
                return TRUE;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (EDA_DrawLineStruct*) DrawStruct )
        if( STRUCT->m_Start.x >= x1 && STRUCT->m_Start.x <= x2
            && STRUCT->m_Start.y >= y1 && STRUCT->m_Start.y <=y2 )
            return TRUE;
        if( (STRUCT->m_End.x >= x1) && (STRUCT->m_End.x <= x2)
           && (STRUCT->m_End.y >= y1) && (STRUCT->m_End.y <=y2) )
            return TRUE;
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawBusEntryStruct*) DrawStruct )
        if( STRUCT->m_Pos.x >= x1 && STRUCT->m_Pos.x <= x2
            && STRUCT->m_Pos.y >= y1 && STRUCT->m_Pos.y <=y2 )
            return TRUE;
        if( (STRUCT->m_End().x >= x1) && ( STRUCT->m_End().x <= x2)
           && ( STRUCT->m_End().y >= y1) && ( STRUCT->m_End().y <=y2) )
            return TRUE;
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawJunctionStruct*) DrawStruct )
        if( (STRUCT->m_Pos.x >= x1) && (STRUCT->m_Pos.x <= x2)
           && (STRUCT->m_Pos.y >= y1) && (STRUCT->m_Pos.y <= y2) )
            return TRUE;
        break;


    case DRAW_NOCONNECT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawNoConnectStruct*) DrawStruct )
        if( (STRUCT->m_Pos.x >= x1) && (STRUCT->m_Pos.x <= x2)
           && (STRUCT->m_Pos.y >= y1) && (STRUCT->m_Pos.y <= y2) )
            return TRUE;
        break;


    case DRAW_MARKER_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawMarkerStruct*) DrawStruct )
        if( (STRUCT->m_Pos.x >= x1) && (STRUCT->m_Pos.x <= x2)
           && (STRUCT->m_Pos.y >= y1) && (STRUCT->m_Pos.y <= y2) )
            return TRUE;
        break;

    case DRAW_LABEL_STRUCT_TYPE:
    case DRAW_TEXT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawTextStruct*) DrawStruct )
        dx  = STRUCT->m_Size.x * STRUCT->GetLength();
        dy  = STRUCT->m_Size.y;
        xt1 = xt2 = STRUCT->m_Pos.x;
        yt1 = yt2 = STRUCT->m_Pos.y;

        switch( STRUCT->m_Orient )
        {
        case 0:             /* HORIZONTAL */
            xt2 += dx; yt2 -= dy;
            break;

        case 1:             /* VERTICAL UP */
            xt2 -= dy; yt2 -= dx;
            break;

        case 2:             /* horizontal inverse */
            xt2 -= dx; yt2 += dy;
            break;

        case 3:             /* vertical DOWN */
            xt2 += dy; yt2 += dx;
            break;
        }

        if( IsBox1InBox2( xt1, yt1, xt2, yt2, x1, y1, x2, y2 ) )
            return TRUE;
        break;

    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawGlobalLabelStruct*) DrawStruct )
        dx  = STRUCT->m_Size.x * ( STRUCT->GetLength() + 1);    /* longueur totale */
        dy  = STRUCT->m_Size.y / 2;                             /* Demi hauteur */
        xt1 = xt2 = STRUCT->m_Pos.x;
        yt1 = yt2 = STRUCT->m_Pos.y;

        switch( STRUCT->m_Orient )
        {
        case 0:             /* HORIZONTAL */
            xt2 -= dx; yt2 += dy; yt1 -= dy;
            break;

        case 1:             /* VERTICAL UP */
            xt1 -= dy; xt2 += dy; yt2 += dx;
            break;

        case 2:             /* horizontal inverse */
            xt2 += dx; yt2 += dy; yt1 -= dy;
            break;

        case 3:             /* vertical DOWN */
            xt1 -= dy; xt2 += dy; yt2 -= dx;
            break;
        }

        if( IsBox1InBox2( xt1, yt1, xt2, yt2, x1, y1, x2, y2 ) )
            return TRUE;
        break;

    case DRAW_LIB_ITEM_STRUCT_TYPE:
    {
        #undef STRUCT
        #define STRUCT ( (EDA_SchComponentStruct*) DrawStruct )
        EDA_Rect BoundaryBox = STRUCT->GetBoundaryBox();
        xt1 = BoundaryBox.GetX();
        yt1 = BoundaryBox.GetY();
        xt2 = BoundaryBox.GetRight();
        yt2 = BoundaryBox.GetBottom();
        if( IsBox1InBox2( xt1, yt1, xt2, yt2, x1, y1, x2, y2 ) )
            return TRUE;
        break;
    }

    case DRAW_SHEET_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (DrawSheetStruct*) DrawStruct )
        /* Recalcul des coordonnees de l'encadrement du composant */
        xt1 = STRUCT->m_Pos.x;
        yt1 = STRUCT->m_Pos.y;
        xt2 = STRUCT->m_Pos.x + STRUCT->m_Size.x;
        yt2 = STRUCT->m_Pos.y + STRUCT->m_Size.y;

        if( IsBox1InBox2( xt1, yt1, xt2, yt2, x1, y1, x2, y2 ) )
            return TRUE;
        break;

    case DRAW_SHEETLABEL_STRUCT_TYPE:
        break;

    case DRAW_PICK_ITEM_STRUCT_TYPE:
        break;

    default:
        msg.Printf(
            wxT( "DrawStructInBox() Err: unexpected StructType %d (" ),
            DrawStruct->Type() );
        msg << DrawStruct->ReturnClassName() << wxT( ")" );
        DisplayError( NULL, msg );
        break;
    }

    return FALSE;
}


/****************************************************************************/
static bool IsBox1InBox2( int StartX1, int StartY1, int EndX1, int EndY1,
                          int StartX2, int StartY2, int EndX2, int EndY2 )
/****************************************************************************/

/* Routine detectant que le rectangle 1 (Box1) et le rectangle 2 (Box2) se
 *  recouvrent.
 *  Retourne TRUE ou FALSE.
 * 
 *  On Considere ici qu'il y a recouvrement si l'un au moins des coins
 *  d'un 'Box' est compris dans l'autre
 */
{
    int cX, cY;

    if( StartX1 > EndX1 )
        EXCHG( StartX1, EndX1 );
    if( StartX2 > EndX2 )
        EXCHG( StartX2, EndX2 );
    if( StartY1 > EndY1 )
        EXCHG( StartY1, EndY1 );
    if( StartY2 > EndY2 )
        EXCHG( StartY2, EndY2 );

    /* Tst des 4 coins du rectangle 1 */
    cX = StartX1; cY = StartY1; /* 1er coin */
    if( (cX >= StartX2) && (cX <= EndX2) && (cY >= StartY2) && (cY <= EndY2) )
        return TRUE;

    cX = EndX1; cY = StartY1;   /* 2er coin */
    if( (cX >= StartX2) && (cX <= EndX2) && (cY >= StartY2) && (cY <= EndY2) )
        return TRUE;

    cX = EndX1; cY = EndY1;   /* 3eme coin */
    if( (cX >= StartX2) && (cX <= EndX2) && (cY >= StartY2) && (cY <= EndY2) )
        return TRUE;

    cX = StartX1; cY = EndY1;   /* 4eme coin */
    if( (cX >= StartX2) && (cX <= EndX2) && (cY >= StartY2) && (cY <= EndY2) )
        return TRUE;

    /* Tst des 4 coins du rectangle 2 */
    cX = StartX2; cY = StartY2;   /* 1er coin */
    if( (cX >= StartX1) && (cX <= EndX1) && (cY >= StartY1) && (cY <= EndY1) )
        return TRUE;

    cX = EndX2; cY = StartY2;   /* 2er coin */
    if( (cX >= StartX1) && (cX <= EndX1) && (cY >= StartY1) && (cY <= EndY1) )
        return TRUE;

    cX = EndX2; cY = EndY2;   /* 3er coin */
    if( (cX >= StartX1) && (cX <= EndX1) && (cY >= StartY1) && (cY <= EndY1) )
        return TRUE;

    cX = StartX2; cY = EndY2;   /* 4er coin */
    if( (cX >= StartX1) && (cX <= EndX1) && (cY >= StartY1) && (cY <= EndY1) )
        return TRUE;


    return FALSE;
}


/**********************************************************************/
static bool IsPointInBox( int pX, int pY,
                          int BoxX1, int BoxY1, int BoxX2, int BoxY2 )
/**********************************************************************/

/* Routine detectant que le point pX,pY est dans le rectangle (Box)
 *  Retourne TRUE ou FALSE.
 * 
 */
{
    if( BoxX1 > BoxX2 )
        EXCHG( BoxX1, BoxX2 );
    if( BoxY1 > BoxY2 )
        EXCHG( BoxY1, BoxY2 );

    if( (pX >= BoxX1) && (pX <= BoxX2) && (pY >= BoxY1) && (pY <= BoxY2) )
        return TRUE;

    else
        return FALSE;
}


/********************************************************************************/
static bool IsPointOnSegment( int pX, int pY,
                              int SegmX1, int SegmY1, int SegmX2, int SegmY2, int seuil )
/********************************************************************************/

/* Routine detectant que le point pX,pY est sur le Segment X1,Y1 a X2,Y2
 *  Retourne TRUE ou FALSE.
 */
{
    /* Recalcul des coord avec SegmX1, SegmX2 comme origine */
    pX     -= SegmX1; pY -= SegmY1;
    SegmX2 -= SegmX1; SegmY2 -= SegmY1;

    if( distance( SegmX2, SegmY2, pX, pY, seuil ) )
        return TRUE;

    else
        return FALSE;
}


/*********************************************************************************/
LibEDA_BaseStruct* LocateDrawItem( SCH_SCREEN* Screen,
                                   const wxPoint& refpoint,
                                   EDA_LibComponentStruct* LibEntry,
                                   int Unit,
                                   int Convert,
                                   int masque )
/*********************************************************************************/

/* Routine de localisation d'un element de dessin de symbole( sauf pins )
 *  Unit = Unite d'appartenance (si Unit = 0, recherche sur toutes unites)
 *  Convert = Conversion d'appartenance (si Convert = 0, recherche sur
 *  toutes variantes)
 */
{
    int x, y, dx, dy, ii, * ptpoly;
    int px, py;
    LibEDA_BaseStruct* DrawItem;
    int seuil;

    if( LibEntry == NULL )
        return NULL;

    if( LibEntry->Type != ROOT )
    {
        DisplayError( NULL, wxT( "Error in LocateDrawItem: Entry is ALIAS" ) );
        return NULL;
    }

    DrawItem = LibEntry->m_Drawings;

    seuil = 3;     /* Tolerance: 1/2 pas de petite grille */
    px    = refpoint.x;
    py    = refpoint.y;

    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        if( Unit && DrawItem->m_Unit && (Unit != DrawItem->m_Unit) )
            continue;
        if( Convert && DrawItem->m_Convert && (Convert != DrawItem->m_Convert) )
            continue;

        switch( DrawItem->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
        {
            LibDrawArc* Arc = (LibDrawArc*) DrawItem;
            if( (masque & LOCATE_COMPONENT_ARC_DRAW_TYPE) == 0 )
                break;
            dx = px - Arc->m_Pos.x;
            dy = py + Arc->m_Pos.y;
            ii = (int) sqrt( dx * dx + dy * dy );
            if( abs( ii - Arc->m_Rayon ) <= seuil )
                return DrawItem;
        }
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
        {
            LibDrawCircle* Circle = (LibDrawCircle*) DrawItem;
            if( (masque & LOCATE_COMPONENT_CIRCLE_DRAW_TYPE) == 0 )
                break;
            dx = px - Circle->m_Pos.x;
            dy = py + Circle->m_Pos.y;
            ii = (int) sqrt( dx * dx + dy * dy );
            if( abs( ii - Circle->m_Rayon ) <= seuil )
                return DrawItem;
        }
            break;

        case COMPONENT_RECT_DRAW_TYPE:
        {           // Locate a rect if the mouse cursor is on a segment
            LibDrawSquare* Square = (LibDrawSquare*) DrawItem;
            if( (masque & LOCATE_COMPONENT_RECT_DRAW_TYPE) == 0 )
                break;
            if( IsPointOnSegment( px, py,   // locate lower segment
                                  Square->m_Pos.x, -Square->m_Pos.y,
                                  Square->m_End.x, -Square->m_Pos.y, seuil ) )
                return DrawItem;
            if( IsPointOnSegment( px, py,   // locate right segment
                                  Square->m_End.x, -Square->m_Pos.y,
                                  Square->m_End.x, -Square->m_End.y, seuil ) )
                return DrawItem;
            if( IsPointOnSegment( px, py,   // locate upper segment
                                  Square->m_End.x, -Square->m_End.y,
                                  Square->m_Pos.x, -Square->m_End.y, seuil ) )
                return DrawItem;
            if( IsPointOnSegment( px, py,   // locate left segment
                                  Square->m_Pos.x, -Square->m_End.y,
                                  Square->m_Pos.x, -Square->m_Pos.y, seuil ) )
                return DrawItem;
        }
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            LibDrawPolyline* polyline = (LibDrawPolyline*) DrawItem;
            if( (masque & LOCATE_COMPONENT_POLYLINE_DRAW_TYPE) == 0 )
                break;
            ptpoly = polyline->PolyList;
            for( ii = polyline->n - 1; ii > 0; ii--, ptpoly += 2 )
            {
                if( IsPointOnSegment( px, py,
                                      ptpoly[0], -ptpoly[1], ptpoly[2], -ptpoly[3], seuil ) )
                    return DrawItem;
            }
        }
            break;

        case COMPONENT_LINE_DRAW_TYPE:
        {
            LibDrawSegment* Segment = (LibDrawSegment*) DrawItem;
            if( (masque & LOCATE_COMPONENT_LINE_DRAW_TYPE) == 0 )
                break;
            if( IsPointOnSegment( px, py,
                                  Segment->m_Pos.x, -Segment->m_Pos.y,
                                  Segment->m_End.x, -Segment->m_End.y, seuil ) )
                return DrawItem;
        }
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        {
            LibDrawText* Text = (LibDrawText*) DrawItem;
            if( (masque & LOCATE_COMPONENT_GRAPHIC_TEXT_DRAW_TYPE) == 0 )
                break;
            ii = Text->m_Text.Len(); if( ii < 2 )
                ii = 2;
            dx = (Text->m_Size.x * ii) / 2;
            dy = Text->m_Size.y / 2;
            if( Text->m_Horiz == TEXT_ORIENT_VERT )
            {
                EXCHG( dx, dy );
            }
            x = px - Text->m_Pos.x;
            y = py + Text->m_Pos.y;
            if( (abs( x ) <= dx) && (abs( y ) <= dy) )
                return DrawItem;            /* Texte trouve */
        }
            break;

        default:
            ;
        }
    }

    return NULL;
}


/****************************************************************/
int distance( int dx, int dy, int spot_cX, int spot_cY, int seuil )
/****************************************************************/

/*
 *  Calcul de la distance du point spot_cx,spot_cy a un segment de droite,
 *  d'origine 0,0 et d'extremite dx, dy;
 *  retourne:
 *      0 si distance > seuil
 *      1 si distance <= seuil
 *  Variables utilisees ( sont ramenees au repere centre sur l'origine du segment)
 *      dx, dy = coord de l'extremite segment.
 *      spot_cX,spot_cY = coord du curseur souris
 *  la recherche se fait selon 4 cas:
 *      segment horizontal
 *      segment vertical
 *      segment quelconque
 */
{
    int cXrot, cYrot,   /* coord du point (souris) dans le repere tourne */
        segX, segY;     /* coord extremite segment tj >= 0 */
    int pointX, pointY;/* coord point a tester dans repere modifie dans lequel
                        *  segX et segY sont >=0 */

    segX = dx; segY = dy; pointX = spot_cX; pointY = spot_cY;

    /*Recalcul coord pour que le segment soit dans 1er quadrant (coord >= 0)*/
    if( segX < 0 )   /* mise en0 par symetrie par rapport a l'axe Y */
    {
        segX = -segX; pointX = -pointX;
    }
    if( segY < 0 )   /* mise en > 0 par symymetrie par rapport a l'axe X */
    {
        segY = -segY; pointY = -pointY;
    }


    if( segY == 0 ) /* piste Horizontale */
    {
        if( abs( pointY ) <= seuil )
        {
            if( (pointX >= 0) && (pointX <= segX) )
                return 1;
            /* Etude des extremites : cercle de rayon seuil */
            if( (pointX < 0) && (pointX >= -seuil) )
            {
                if( ( (pointX * pointX) + (pointY * pointY) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (pointX > segX) && ( pointX <= (segX + seuil) ) )
            {
                if( ( ( (pointX - segX) * (pointX - segX) ) + (pointY * pointY) ) <=
                   (seuil * seuil) )
                    return 1;
            }
        }
    }
    else if( segX == 0 ) /* piste verticale */
    {
        if( abs( pointX ) <= seuil )
        {
            if( (pointY >= 0 ) && (pointY <= segY) )
                return 1;
            if( (pointY < 0) && (pointY >= -seuil) )
            {
                if( ( (pointY * pointY) + (pointX * pointX) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (pointY > segY) && ( pointY <= (segY + seuil) ) )
            {
                if( ( ( (pointY - segY) * (pointY - segY) ) + (pointX * pointX) ) <=
                   (seuil * seuil) )
                    return 1;
            }
        }
    }
    else    /* orientation quelconque */
    {
        /* On fait un changement d'axe (rotation) de facon a ce que le segment
         *  de piste soit horizontal dans le nouveau repere */
        int angle;

        angle = (int) ( atan2( (float) segY, (float) segX ) * 1800 / M_PI);
        cXrot = pointX; cYrot = pointY;
        RotatePoint( &cXrot, &cYrot, angle );   /* Rotation du point a tester */
        RotatePoint( &segX, &segY, angle );     /* Rotation du segment */

        /*la piste est Horizontale , par suite des modifs de coordonnes
         *  et d'axe, donc segX = longueur du segment */

        if( abs( cYrot ) <= seuil ) /* ok sur axe vertical) */
        {
            if( (cXrot >= 0) && (cXrot <= segX) )
                return 1;
            /* Etude des extremites : cercle de rayon seuil */
            if( (cXrot < 0) && (cXrot >= -seuil) )
            {
                if( ( (cXrot * cXrot) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (cXrot > segX) && ( cXrot <= (segX + seuil) ) )
            {
                if( ( ( (cXrot - segX) * (cXrot - segX) ) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
        }
    }
    return 0;
}


/*******************************************************************/
LibEDA_BaseStruct* LocatePin( const wxPoint& RefPos,
                              EDA_LibComponentStruct* Entry,
                              int Unit, int convert, EDA_SchComponentStruct* DrawLibItem )
/*******************************************************************/

/* Routine de localisation d'une PIN de la PartLib pointee par Entry
 *  retourne un pointeur sur la pin, ou NULL si pas trouve
 *  Si Unit = 0, le numero d'unite n'est pas teste
 *  Si convert = 0, le numero convert n'est pas teste
 * 
 *  m_Transform = matrice de transformation.
 *  Si NULL: 	matrice de transformation " normale" [1 , 0 , 0 , -1]
 *  (la matrice de transformation " normale" etant [1 , 0 , 0 , -1]
 *  la coord dy doit etre inversee).
 *  PartX, PartY: coordonnees de positionnement du composant
 */
{
    LibEDA_BaseStruct* DrawItem;
    LibDrawPin* Pin;
    int x1, y1, x2, y2;

    if( Entry == NULL )
        return NULL;

    if( Entry->Type != ROOT )
    {
        DisplayError( NULL, wxT( "LocatePin() error: Entry is ALIAS" ) );
        return NULL;
    }

    DrawItem = Entry->m_Drawings;
    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        if( DrawItem->Type() == COMPONENT_PIN_DRAW_TYPE ) /* Pin Trouvee */
        {
            Pin = (LibDrawPin*) DrawItem;

            if( Unit && DrawItem->m_Unit && (DrawItem->m_Unit != Unit) )
                continue;

            if( convert && DrawItem->m_Convert && (DrawItem->m_Convert != convert) )
                continue;

            x2 = Pin->m_Pos.x;
            y2 = Pin->m_Pos.y;
            x1 = Pin->ReturnPinEndPoint().x;
            y1 = Pin->ReturnPinEndPoint().y;

            if( DrawLibItem == NULL )
            {
                y1 = -y1; y2 = -y2;
            }
            else
            {
                int x = x1, y = y1;
                x1 = DrawLibItem->m_Pos.x + DrawLibItem->m_Transform[0][0] * x
                     + DrawLibItem->m_Transform[0][1] * y;
                y1 = DrawLibItem->m_Pos.y + DrawLibItem->m_Transform[1][0] * x
                     + DrawLibItem->m_Transform[1][1] * y;
                x  = x2; y = y2;
                x2 = DrawLibItem->m_Pos.x + DrawLibItem->m_Transform[0][0] * x
                     + DrawLibItem->m_Transform[0][1] * y;
                y2 = DrawLibItem->m_Pos.y + DrawLibItem->m_Transform[1][0] * x
                     + DrawLibItem->m_Transform[1][1] * y;
            }

            if( x1 > x2 )
                EXCHG( x1, x2 );if( y1 > y2 )
                EXCHG( y1, y2 );

            if( (RefPos.x >= x1) && (RefPos.x <= x2)
               && (RefPos.y >= y1) && (RefPos.y <= y2) )
                return DrawItem;
        }
    }

    return NULL;
}


/***********************************************************************************/
DrawSheetLabelStruct* LocateSheetLabel( DrawSheetStruct* Sheet, const wxPoint& pos )
/***********************************************************************************/
{
    int size, dy, minx, maxx;
    DrawSheetLabelStruct* SheetLabel;

    SheetLabel = Sheet->m_Label;
    while( (SheetLabel) && (SheetLabel->Type()==DRAW_SHEETLABEL_STRUCT_TYPE) )
    {
        size = ( SheetLabel->GetLength() + 1 ) * SheetLabel->m_Size.x;
        if( SheetLabel->m_Edge )
            size = -size;
        minx = SheetLabel->m_Pos.x; maxx = SheetLabel->m_Pos.x + size;
        if( maxx < minx )
            EXCHG( maxx, minx );
        dy = SheetLabel->m_Size.x / 2;
        if( (ABS( pos.y - SheetLabel->m_Pos.y ) <= dy )
           && (pos.x <= maxx)
           && (pos.x >= minx) )
            return SheetLabel;
        SheetLabel = (DrawSheetLabelStruct*) SheetLabel->Pnext;
    }

    return NULL;
}


/**************************************************************************/
LibDrawPin* LocateAnyPin( EDA_BaseStruct* DrawList, const wxPoint& RefPos,
                          EDA_SchComponentStruct** libpart )
/**************************************************************************/
{
    EDA_BaseStruct* DrawStruct;
    EDA_LibComponentStruct* Entry;
    EDA_SchComponentStruct* LibItem = NULL;
    LibDrawPin* Pin = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
    {
        if( DrawStruct->Type() != DRAW_LIB_ITEM_STRUCT_TYPE )
            continue;
        LibItem = (EDA_SchComponentStruct*) DrawStruct;
        Entry   = FindLibPart( LibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry == NULL )
            continue;
        Pin = (LibDrawPin*) LocatePin( RefPos, Entry, LibItem->m_Multi,
                                       LibItem->m_Convert, LibItem );
        if( Pin )
            break;
    }

    if( libpart )
        *libpart = LibItem;
    return Pin;
}


/***************************************************************/
DrawSheetLabelStruct* LocateAnyPinSheet( const wxPoint&  RefPos,
                                         EDA_BaseStruct* DrawList )
/***************************************************************/
{
    EDA_BaseStruct* DrawStruct;
    DrawSheetLabelStruct* PinSheet = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
    {
        if( DrawStruct->Type() != DRAW_SHEET_STRUCT_TYPE )
            continue;
        PinSheet = LocateSheetLabel( (DrawSheetStruct*) DrawStruct,
                                    RefPos );
        if( PinSheet )
            break;
    }

    return PinSheet;
}
