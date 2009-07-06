/******************************************************/
/* Routines de localisation d'un element d'un schema. */
/******************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "trigo.h"
#include "macros.h"
#include "class_drawpickedstruct.h"

#include "protos.h"

/* Routines Locales */
static SCH_ITEM* LastSnappedStruct = NULL;
static int       PickedBoxMinX, PickedBoxMinY, PickedBoxMaxX, PickedBoxMaxY;
static bool IsBox1InBox2( int StartX1, int StartY1, int EndX1, int EndY1,
                          int StartX2, int StartY2, int EndX2, int EndY2 );
static bool SnapPoint2( const wxPoint& aPosRef, int SearchMask,
                        SCH_ITEM* DrawList, DrawPickedStruct* DontSnapList, double aScaleFactor );


/*********************************************************************/
SCH_COMPONENT* LocateSmallestComponent( SCH_SCREEN* Screen )
/*********************************************************************/

/* Search the smaller (considering its area) component under the mouse cursor or the pcb cursor
 *  If more than 1 component is found, a pointer to the smaller component is returned
 */
{
    SCH_COMPONENT* component = NULL, * lastcomponent = NULL;
    SCH_ITEM*      DrawList;
    EDA_Rect       BoundaryBox;
    float          sizeref = 0, sizecurr;

    DrawList = Screen->EEDrawList;

    while( DrawList )
    {
        if( ( SnapPoint2( Screen->m_MousePosition, LIBITEM,
                         DrawList, NULL, Screen->GetZoom() ) ) == FALSE )
        {
            if( ( SnapPoint2( Screen->m_Curseur, LIBITEM,
                             DrawList, NULL, Screen->GetScalingFactor() ) ) == FALSE )
                break;
        }
        component = (SCH_COMPONENT*) LastSnappedStruct;
        DrawList  = component->Next();
        if( lastcomponent == NULL )  // First time a component is located
        {
            lastcomponent = component;
            BoundaryBox   = lastcomponent->GetBoundaryBox();
            sizeref = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );
        }
        else
        {
            BoundaryBox = component->GetBoundaryBox();
            sizecurr    = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );
            if( sizeref > sizecurr )   // a smallest component is found
            {
                sizeref = sizecurr;
                lastcomponent = component;
            }
        }
    }

    return lastcomponent;
}


/********************************************************************************/
SCH_ITEM* PickStruct( const wxPoint& refpos, BASE_SCREEN* screen, int SearchMask )
/******************************************************************************/

/* Search an item at pos refpos
 *  SearchMask = (bitwise OR):
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
 *          pointer on item found or NULL
 *
 */
{
    bool Snapped;

    if( screen == NULL || screen->EEDrawList == NULL )
        return NULL;

    if( ( Snapped = SnapPoint2( refpos, SearchMask,
                               screen->EEDrawList, NULL, screen->GetScalingFactor() ) ) != FALSE )
    {
        return LastSnappedStruct;
    }
    return NULL;
}


/***********************************************************************/
SCH_ITEM* PickStruct( EDA_Rect& block, BASE_SCREEN* screen, int SearchMask )
/************************************************************************/

/* Search items in block
 *      Return:
 *          pointeur sur liste de pointeurs de structures si Plusieurs
 *                  structures selectionnees.
 *          pointeur sur la structure si 1 seule
 *
 */
{
    int x, y, OrigX, OrigY;
    DrawPickedStruct* PickedList = NULL, * PickedItem;
    SCH_ITEM*         DrawStruct;

    OrigX = block.GetX();
    OrigY = block.GetY();
    x     = block.GetRight();
    y     = block.GetBottom();

    if( x < OrigX )
        EXCHG( x, OrigX );
    if( y < OrigY )
        EXCHG( y, OrigY );

    SCH_ITEM* DrawList = screen->EEDrawList;
    if( screen==NULL || DrawList == NULL )
        return NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( DrawStructInBox( OrigX, OrigY, x, y, DrawStruct ) )
        {
            /* Put this structure in the picked list: */
            PickedItem = new DrawPickedStruct( DrawStruct );

            PickedItem->SetNext( PickedList );
            PickedList = PickedItem;
        }
    }

    if( PickedList && PickedList->Next() == NULL )
    {
        /* Only one item was picked - convert to scalar form (no list): */
        PickedItem = PickedList;
        PickedList = (DrawPickedStruct*) PickedList->m_PickedStruct;
        SAFE_DELETE( PickedItem );
    }

    if( PickedList != NULL )
    {
        PickedBoxMinX = OrigX;  PickedBoxMinY = OrigY;
        PickedBoxMaxX = x; PickedBoxMaxY = y;
    }

    return (SCH_ITEM*) PickedList;
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
bool SnapPoint2( const wxPoint& aPosRef, int SearchMask,
                 SCH_ITEM* DrawList, DrawPickedStruct* DontSnapList, double aScaleFactor )
{
    DrawPickedStruct* DontSnap;

    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        /* Make sure this structure is NOT in the dont snap list: */
        DontSnap = DontSnapList;
        for( ; DontSnap != NULL; DontSnap = DontSnap->Next() )
            if( DontSnap->m_PickedStruct == DrawList )
                break;

        if( DontSnap )
            if( DontSnap->m_PickedStruct == DrawList )
                continue;

        int hitminDist = MAX( g_DrawDefaultLineThickness, 3 ) ;
        switch( DrawList->Type() )
        {
        case DRAW_POLYLINE_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (DrawPolylineStruct*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;

            for( unsigned i = 0; i < STRUCT->GetCornerCount() - 1; i++ )
            {
                if( TestSegmentHit( aPosRef, STRUCT->m_PolyPoints[i],
                                      STRUCT->m_PolyPoints[i + 1], hitminDist ) )
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

            if( TestSegmentHit( aPosRef, STRUCT->m_Start, STRUCT->m_End, 0 ) )
            {
                if( ( (SearchMask & DRAWITEM) && (STRUCT->GetLayer() == LAYER_NOTES) )
                   || ( (SearchMask & WIREITEM) && (STRUCT->GetLayer() == LAYER_WIRE) )
                   || ( (SearchMask & BUSITEM) && (STRUCT->GetLayer() == LAYER_BUS) )
                    )
                {
                    if( SearchMask & EXCLUDE_WIRE_BUS_ENDPOINTS )
                    {
                        if( aPosRef == STRUCT->m_Start || aPosRef == STRUCT->m_End )
                            break;
                    }

                    if( SearchMask & WIRE_BUS_ENDPOINTS_ONLY )
                    {
                        if( !STRUCT->IsOneEndPointAt( aPosRef ) )
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

            if( TestSegmentHit( aPosRef, STRUCT->m_Pos, STRUCT->m_End(), hitminDist ) )
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
            if( STRUCT->HitTest( aPosRef ) )
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
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case DRAW_MARKER_STRUCT_TYPE:
        {
            #undef  STRUCT
            #define STRUCT ( (MARKER_SCH*) DrawList )
            if( !(SearchMask & MARKERITEM) )
                break;
            if( STRUCT->HitTest(aPosRef) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;
        }

        case TYPE_SCH_TEXT:
            #undef  STRUCT
            #define STRUCT ( (SCH_TEXT*) DrawList )
            if( !( SearchMask & TEXTITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;


        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            #undef  STRUCT
            #define STRUCT ( (SCH_TEXT*) DrawList )     // SCH_TEXT is the base class of these labels
            if( !(SearchMask & LABELITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return TRUE;
            }
            break;

        case TYPE_SCH_COMPONENT:
            if( !( SearchMask & (LIBITEM | FIELDCMPITEM) ) )
                break;

            if( SearchMask & FIELDCMPITEM )
            {
                SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) DrawList;
                for( int i = REFERENCE; i < DrawLibItem->GetFieldCount(); i++ )
                {
                    SCH_CMP_FIELD* field = DrawLibItem->GetField( i );

                    if( field->m_Attributs & TEXT_NO_VISIBLE )
                        continue;

                    if( field->IsVoid() )
                        continue;

                    EDA_Rect BoundaryBox = field->GetBoundaryBox();
                    if( BoundaryBox.Inside( aPosRef ) )
                    {
                        LastSnappedStruct = field;
                        return TRUE;
                    }
                }
            }
            else
            {
                #undef  STRUCT
                #define STRUCT ( (SCH_COMPONENT*) DrawList )
                EDA_Rect BoundaryBox = STRUCT->GetBoundaryBox();
                if( BoundaryBox.Inside( aPosRef ) )
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
            if( STRUCT->HitTest( aPosRef ) )
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
            msg << DrawList->GetClass() << wxT( ")" );
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
bool DrawStructInBox( int x1, int y1, int x2, int y2, SCH_ITEM* DrawStruct )
{
    int xt1, yt1, xt2, yt2;
    int dx, dy;
    wxString msg;

    switch( DrawStruct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        #undef  STRUCT
        #define STRUCT ( (DrawPolylineStruct*) DrawStruct )
        for( unsigned i = 0; i < STRUCT->GetCornerCount(); i++ )
        {
            if( STRUCT->m_PolyPoints[i].x >= x1 && STRUCT->m_PolyPoints[i].x <= x2
                && STRUCT->m_PolyPoints[i].y >= y1 && STRUCT->m_PolyPoints[i].y <=y2 )
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
        #define STRUCT ( (MARKER_SCH*) DrawStruct )
        if( (STRUCT->m_Pos.x >= x1) && (STRUCT->m_Pos.x <= x2)
           && (STRUCT->m_Pos.y >= y1) && (STRUCT->m_Pos.y <= y2) )
            return TRUE;
        break;

    case TYPE_SCH_LABEL:
    case TYPE_SCH_TEXT:
        #undef STRUCT
        #define STRUCT ( (SCH_TEXT*) DrawStruct )
        dx  = STRUCT->m_Size.x * STRUCT->GetLength();
        dy  = STRUCT->m_Size.y;
        xt1 = xt2 = STRUCT->m_Pos.x;
        yt1 = yt2 = STRUCT->m_Pos.y;

        switch( STRUCT->m_Orient )
        {
        case 0:             /* HORIZONTAL  Left justified */
            xt2 += dx; yt2 -= dy;
            break;

        case 1:             /* VERTICAL UP */
            xt2 -= dy; yt2 -= dx;
            break;

        case 2:             /* horizontal  Right justified  */
            xt2 -= dx; yt2 -= dy;
            break;

        case 3:             /* vertical DOWN */
            xt2 -= dy; yt2 += dx;
            break;
        }

        if( IsBox1InBox2( xt1, yt1, xt2, yt2, x1, y1, x2, y2 ) )
            return TRUE;
        break;

    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:
        #undef STRUCT
        #define STRUCT ( (SCH_LABEL*) DrawStruct )
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

    case TYPE_SCH_COMPONENT:
    {
        #undef STRUCT
        #define STRUCT ( (SCH_COMPONENT*) DrawStruct )
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

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        break;

    case DRAW_PICK_ITEM_STRUCT_TYPE:
        break;

    default:
        msg.Printf(
            wxT( "DrawStructInBox() Err: unexpected StructType %d (" ),
            DrawStruct->Type() );
        msg << DrawStruct->GetClass() << wxT( ")" );
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



/*********************************************************************************/
LibEDA_BaseStruct* LocateDrawItem( SCH_SCREEN*             Screen,
                                   const wxPoint&          aRefPoint,
                                   EDA_LibComponentStruct* LibEntry,
                                   int                     Unit,
                                   int                     Convert,
                                   int                     masque )
/*********************************************************************************/

/* Locates a body item( not pins )
 *  Unit = part number (if Unit = 0, all parts are considered)
 *  Convert = convert value for shape (si Convert = 0, all shapes are considered)
 *  remember the Y axis is from bottom to top in library entries for graphic items.
 */
{
    LibEDA_BaseStruct* DrawItem;

    if( LibEntry == NULL )
        return NULL;

    if( LibEntry->Type != ROOT )
    {
        DisplayError( NULL, wxT( "Error in LocateDrawItem: Entry is ALIAS" ) );
        return NULL;
    }

    DrawItem = LibEntry->m_Drawings;

    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        if( Unit && DrawItem->m_Unit && (Unit != DrawItem->m_Unit) )
            continue;
        if( Convert && DrawItem->m_Convert && (Convert != DrawItem->m_Convert) )
            continue;

        switch( DrawItem->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_ARC_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_CIRCLE_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        case COMPONENT_RECT_DRAW_TYPE:  // Locate a rect if the mouse cursor is on a side of this rectangle
            if( (masque & LOCATE_COMPONENT_RECT_DRAW_TYPE) == 0 )
                break;
             if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
         break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_POLYLINE_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        case COMPONENT_BEZIER_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_POLYLINE_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        case COMPONENT_LINE_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_LINE_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            if( (masque & LOCATE_COMPONENT_GRAPHIC_TEXT_DRAW_TYPE) == 0 )
                break;
            if( DrawItem->HitTest( aRefPoint ) )
                return DrawItem;
        break;

        default:
            ;
        }
    }

    return NULL;
}


/*******************************************************************/
LibDrawPin* LocatePinByNumber( const wxString& ePin_Number,
                               SCH_COMPONENT*  eComponent )
/*******************************************************************/

/** Find a PIN in a component
 * @param pin_number = pin number (string)
 * @param pin_number = pin number (string)
 * @return a pointer on the pin, or NULL if not found
 */
{
    LibEDA_BaseStruct* DrawItem;
    EDA_LibComponentStruct* Entry;
    LibDrawPin* Pin;
    int Unit, Convert;

    Entry = FindLibPart( eComponent->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return NULL;

    if( Entry->Type != ROOT )
    {
        DisplayError( NULL, wxT( "LocatePinByNumber() error: Entry is ALIAS" ) );
        return NULL;
    }

    Unit    = eComponent->m_Multi;
    Convert = eComponent->m_Convert;

    DrawItem = Entry->m_Drawings;
    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        if( DrawItem->Type() == COMPONENT_PIN_DRAW_TYPE ) /* Pin Trouvee */
        {
            Pin = (LibDrawPin*) DrawItem;

            if( Unit && DrawItem->m_Unit && (DrawItem->m_Unit != Unit) )
                continue;

            if( Convert && DrawItem->m_Convert && (DrawItem->m_Convert != Convert) )
                continue;
            wxString pNumber;
            Pin->ReturnPinStringNum( pNumber );
            if( ePin_Number == pNumber )
                return Pin;
        }
    }

    return NULL;
}


/*******************************************************************/
LibEDA_BaseStruct* LocatePin( const wxPoint& RefPos,
                              EDA_LibComponentStruct* Entry,
                              int Unit, int convert, SCH_COMPONENT* DrawLibItem )
/*******************************************************************/

/* Routine de localisation d'une PIN de la PartLib pointee par Entry
 *  retourne un pointeur sur la pin, ou NULL si pas trouve
 *  Si Unit = 0, le numero d'unite n'est pas teste
 *  Si convert = 0, le numero convert n'est pas teste
 */
{
    if( Entry == NULL )
        return NULL;

    if( Entry->Type != ROOT )
    {
        DisplayError( NULL, wxT( "LocatePin() error: Entry is ALIAS" ) );
        return NULL;
    }

    LibEDA_BaseStruct* DrawItem = Entry->m_Drawings;
    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        if( DrawItem->Type() == COMPONENT_PIN_DRAW_TYPE ) /* Pin Trouvee */
        {
            LibDrawPin* Pin = (LibDrawPin*) DrawItem;

            if( Unit && DrawItem->m_Unit && (DrawItem->m_Unit != Unit) )
                continue;

            if( convert && DrawItem->m_Convert && (DrawItem->m_Convert != convert) )
                continue;

            if( DrawLibItem == NULL )
            {
                if ( Pin->HitTest( RefPos ) )
                    return DrawItem;
            }

            else
            {
                int mindist = Pin->m_Width ? Pin->m_Width / 2 : g_DrawDefaultLineThickness / 2;

                // Have a minimal tolerance for hit test
                if( mindist < 3 )
                    mindist = 3;        // = 3 mils
                if ( Pin->HitTest( RefPos - DrawLibItem->m_Pos, mindist, DrawLibItem->m_Transform ) )
                    return DrawItem;
            }
        }
    }

    return NULL;
}


/***********************************************************************************/
Hierarchical_PIN_Sheet_Struct* LocateSheetLabel( DrawSheetStruct* Sheet, const wxPoint& pos )
/***********************************************************************************/
{
    int size, dy, minx, maxx;
    Hierarchical_PIN_Sheet_Struct* SheetLabel;

    SheetLabel = Sheet->m_Label;
    while( (SheetLabel) && (SheetLabel->Type()==DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE) )
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
        SheetLabel = SheetLabel->Next();
    }

    return NULL;
}


/**************************************************************************/
LibDrawPin* LocateAnyPin( SCH_ITEM* DrawList, const wxPoint& RefPos,
                          SCH_COMPONENT** libpart )
/**************************************************************************/
{
    SCH_ITEM* DrawStruct;
    EDA_LibComponentStruct* Entry;
    SCH_COMPONENT* LibItem = NULL;
    LibDrawPin* Pin = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != TYPE_SCH_COMPONENT )
            continue;
        LibItem = (SCH_COMPONENT*) DrawStruct;
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
Hierarchical_PIN_Sheet_Struct* LocateAnyPinSheet( const wxPoint& RefPos,
                                                  SCH_ITEM*      DrawList )
/***************************************************************/
{
    SCH_ITEM* DrawStruct;
    Hierarchical_PIN_Sheet_Struct* PinSheet = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
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
