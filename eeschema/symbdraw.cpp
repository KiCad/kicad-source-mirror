/*********************************************************************/
/*          EESchema - symbdraw.cpp                                  */
/* Create, move .. graphic shapes used to build and draw a component */
/* (lines, arcs ..                                                   */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"
#include "dialog_lib_edit_draw_item.h"

#include <boost/foreach.hpp>

#define EraseItem( item ) item->Draw( Panel, DC, wxPoint( 0,\
                                                          0 ), -1, g_XorMode, NULL,\
                                      DefaultTransformMatrix )

static void    SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void    ComputeArc( LIB_ARC* DrawItem, wxPoint ArcCentre );
static void    ComputeArcRadiusAngles( LIB_ARC* arc );
static wxPoint ComputeCircumCenter( wxPoint A, wxPoint B, wxPoint C );
static void    RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                        wxDC*             DC,
                                        bool              erase );

static wxPoint InitPosition, StartCursor, ItemPreviousPos;

typedef enum {
    START_POINT, END_POINT, OUTLINE
} SelectedPoint;

// Attributes of the arc in edit
static struct
{
    wxPoint       startPoint, endPoint; //!< Start, end coordinates
    int           stateDrawArc;         //!< The actual drawing state
    double        distanceCenter;       //!< Distance from arc center to middle of start-/end-point
    SelectedPoint selectedPoint;        //!< The selected point while modifying
    int           direction;            //!< Determines the side of the center point relative to start/end point
} arcState;

// Structure for saving attributes before modifying graphics objects
static struct
{
    wxPoint startPoint;
    wxPoint endPoint;
    wxPoint centerPoint;
    int     radius;
} savedAttributes;

/*
 * Show the dialog box for editing a graphical item properties
 */
void WinEDA_LibeditFrame::EditGraphicSymbol( wxDC* DC, LIB_DRAW_ITEM* DrawItem )
{
    if( DrawItem == NULL )
        return;

    LIB_COMPONENT*            component = DrawItem->GetParent();

    DIALOG_LIB_EDIT_DRAW_ITEM dialog( this, DrawItem->m_typeName );

    dialog.SetWidthUnits( ReturnUnitSymbol( g_UnitMetric ) );

    wxString val = ReturnStringFromValue( g_UnitMetric, m_drawLineWidth,
                                          m_InternalUnits );
    dialog.SetWidth( val );
    dialog.SetApplyToAllUnits( !m_drawSpecificUnit );
    dialog.EnableApplyToAllUnits( component && component->GetPartCount() > 1 );
    dialog.SetApplyToAllConversions( !m_drawSpecificConvert );
    dialog.EnableApplyToAllConversions( component && component->HasConversion() );
//    dialog.SetFillStyle( m_drawFillStyle );   // could better to show the current setting
    dialog.SetFillStyle( DrawItem->m_Fill);
    dialog.EnableFillStyle( DrawItem->IsFillable() );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    val = dialog.GetWidth();
    m_drawLineWidth = ReturnValueFromString( g_UnitMetric, val,
                                             m_InternalUnits );
    m_drawSpecificConvert = !dialog.GetApplyToAllConversions();
    m_drawSpecificUnit    = !dialog.GetApplyToAllUnits();

#if 0
    /* TODO: see if m_drawFillStyle must retain the last fill option or not.
     * if the last is Filled, having next new graphic items created
     * with filled body is often bad.
     * currently m_drawFillStyle is left with the defualt value (not filled)
     */
    if( DrawItem->IsFillable() )
        m_drawFillStyle = (FILL_T) dialog.GetFillStyle();
#endif

    // Save copy for undo is done before place.
    if( !( DrawItem->m_Flags & IS_NEW ) )
        SaveCopyInUndoList( DrawItem->GetParent() );

    if( m_drawSpecificUnit )
        DrawItem->m_Unit = GetUnit();
    else
        DrawItem->m_Unit = 0;

    if( m_drawSpecificConvert )
        DrawItem->m_Convert = GetConvert();
    else
        DrawItem->m_Convert = 0;

    if( DrawItem->IsFillable() )
        DrawItem->m_Fill = (FILL_T) dialog.GetFillStyle();

    DrawItem->SetWidth( m_drawLineWidth );

    if( component )
        component->GetDrawItemList().sort();
    OnModify( );

    DrawItem->DisplayInfo( this );
    DrawPanel->Refresh();
}


static void AbortSymbolTraceOn( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    LIB_DRAW_ITEM*       item;
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) Panel->GetParent();

    item = parent->GetDrawItem();

    if( item == NULL )
        return;

    arcState.stateDrawArc = 0;
    Panel->ManageCurseur  = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    if( item->m_Flags & IS_NEW )
    {
        SAFE_DELETE( item );
        parent->SetDrawItem( NULL );
    }
    else
    {
        // Restore old attributes, when the item was modified
        if( item->m_Flags == IS_RESIZED )
        {
            EraseItem( item );

            switch( item->Type() )
            {
            case COMPONENT_CIRCLE_DRAW_TYPE:
                ( (LIB_CIRCLE*) item )->m_Radius = savedAttributes.radius;
                break;

            case COMPONENT_RECT_DRAW_TYPE:
                ( (LIB_RECTANGLE*) item )->m_Pos = savedAttributes.startPoint;
                ( (LIB_RECTANGLE*) item )->m_End = savedAttributes.endPoint;
                break;

            case COMPONENT_POLYLINE_DRAW_TYPE:
            {
                wxPoint point = savedAttributes.startPoint;
                int     index = ( (LIB_POLYLINE*) item )->m_ModifyIndex;
                ( ( (LIB_POLYLINE*) item )->m_PolyPoints )[index] = point;
            }
            break;

            case COMPONENT_ARC_DRAW_TYPE:
                ( (LIB_ARC*) item )->m_ArcStart = savedAttributes.startPoint;
                ( (LIB_ARC*) item )->m_ArcEnd   = savedAttributes.endPoint;
                ( (LIB_ARC*) item )->m_Pos = savedAttributes.centerPoint;

                ComputeArcRadiusAngles( (LIB_ARC*) item );

                break;

            default:
                break;
            }

            item->m_Flags = 0;
        }
        else
        {
            wxPoint             curpos    = Panel->GetScreen()->m_Curseur;
            Panel->GetScreen()->m_Curseur = StartCursor;
            RedrawWhileMovingCursor( Panel, DC, TRUE );
            Panel->GetScreen()->m_Curseur = curpos;
        }
        item->m_Flags = 0;
    }

    Panel->Refresh( );
}


LIB_DRAW_ITEM* WinEDA_LibeditFrame::CreateGraphicItem( LIB_COMPONENT* LibEntry,
                                                       wxDC*          DC )
{
    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;

    switch( m_ID_current_state )
    {
    case ID_LIBEDIT_BODY_ARC_BUTT:
    {
        LIB_ARC* Arc = new LIB_ARC( LibEntry );

        m_drawItem = Arc;
        arcState.startPoint.x = arcState.endPoint.x = GetScreen()->m_Curseur.x;
        arcState.startPoint.y = arcState.endPoint.y = -( GetScreen()->m_Curseur.y );
        arcState.stateDrawArc = 1;
        Arc->m_Fill  = m_drawFillStyle;
        Arc->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    {
        LIB_CIRCLE* Circle = new LIB_CIRCLE( LibEntry );

        m_drawItem    = Circle;
        Circle->m_Pos = GetScreen()->m_Curseur;
        NEGATE( Circle->m_Pos.y );
        Circle->m_Fill  = m_drawFillStyle;
        Circle->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_RECT_BUTT:
    {
        LIB_RECTANGLE* Square = new LIB_RECTANGLE( LibEntry );

        m_drawItem    = Square;
        Square->m_Pos = GetScreen()->m_Curseur;
        NEGATE( Square->m_Pos.y );
        Square->m_End   = Square->m_Pos;
        Square->m_Fill  = m_drawFillStyle;
        Square->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    {
        LIB_POLYLINE* polyline = new LIB_POLYLINE( LibEntry );
        m_drawItem = polyline;
        wxPoint       point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        polyline->AddPoint( point );    // Start point of the current segment
        polyline->AddPoint( point );    // End point of the current segment
        polyline->m_Fill  = m_drawFillStyle;
        polyline->m_Width = m_drawLineWidth;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
    {
        LIB_SEGMENT* Segment = new LIB_SEGMENT( LibEntry );

        m_drawItem     = Segment;
        Segment->m_Pos = GetScreen()->m_Curseur;
        NEGATE( Segment->m_Pos.y );
        Segment->m_End   = Segment->m_Pos;
        Segment->m_Width = m_drawLineWidth;
    }
    break;

    case ID_LIBEDIT_BODY_TEXT_BUTT:
    {
        LIB_TEXT* Text = new LIB_TEXT( LibEntry );

        m_drawItem     = Text;
        Text->m_Size.x = Text->m_Size.y = m_textSize;
        Text->m_Orient = m_textOrientation;
        Text->m_Pos    = GetScreen()->m_Curseur;
        NEGATE( Text->m_Pos.y );

        // Enter the graphic text info
        DrawPanel->m_IgnoreMouseEvents = true;
        EditSymbolText( NULL, Text );
        DrawPanel->MouseToCursorSchema();
        DrawPanel->m_IgnoreMouseEvents = false;

        if( Text->m_Text.IsEmpty() )
        {
            SAFE_DELETE( Text );
            m_drawItem = NULL;
            DrawPanel->ManageCurseur = NULL;
            DrawPanel->ForceCloseManageCurseur = NULL;
        }
        else
        {
            StartMoveDrawSymbol( DC );
            Text->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                        DefaultTransformMatrix );
        }
    }
    break;

    default:
        DisplayError( this, wxT( "WinEDA_LibeditFrame::CreateGraphicItem() \
error"                                                                              ) );
        return NULL;
    }

    if( m_drawItem )
    {
        m_drawItem->m_Flags |= IS_NEW;
        if( m_drawSpecificUnit )
            m_drawItem->m_Unit = m_unit;
        if( m_drawSpecificConvert )
            m_drawItem->m_Convert = m_convert;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    return m_drawItem;
}


/* Create new library component graphic object.
 */
void WinEDA_LibeditFrame::GraphicItemBeginDraw( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    switch( m_drawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        if( arcState.stateDrawArc == 1 )
        {
            SymbolDisplayDraw( DrawPanel, DC, FALSE );
            arcState.stateDrawArc = 2;
            SymbolDisplayDraw( DrawPanel, DC, FALSE );
            break;
        }
        if( arcState.stateDrawArc > 1 )
        {
            EndDrawGraphicItem( DC );
            return;
        }
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        EndDrawGraphicItem( DC );
        return;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        wxPoint pos = GetScreen()->m_Curseur;
        NEGATE( pos.y );
        ( (LIB_POLYLINE*) m_drawItem )->AddPoint( pos );
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        break;

    default:
        ;
    }
}


/*
 * Redraw the graphic shape while moving
 */
static void RedrawWhileMovingCursor( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase )
{
    LIB_DRAW_ITEM* item;

    item = ( (WinEDA_LibeditFrame*) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    BASE_SCREEN* Screen = panel->GetScreen();
    wxPoint      pos;

    /* Erase shape in the old position*/
    if( erase )
    {
        pos = ItemPreviousPos - StartCursor;
        item->Draw( panel, DC, pos, -1, g_XorMode, NULL,
                    DefaultTransformMatrix );
    }

    /* Redraw moved shape */
    pos = Screen->m_Curseur - StartCursor;
    item->Draw( panel, DC, pos, -1, g_XorMode, NULL, DefaultTransformMatrix );
    ItemPreviousPos = Screen->m_Curseur;
}


void WinEDA_LibeditFrame::StartMoveDrawSymbol( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    SetCursor( wxCURSOR_HAND );

    m_drawItem->m_Flags |= IS_MOVED;
    StartCursor     = GetScreen()->m_Curseur;
    InitPosition    = m_drawItem->GetPosition();
    ItemPreviousPos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = RedrawWhileMovingCursor;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


// @brief Modify a graphic symbol (drag edges etc.)
void WinEDA_LibeditFrame::StartModifyDrawSymbol( wxDC* DC )
{
    if( m_drawItem == NULL )
        return;

    // TODO Check if this is really required
    StartCursor     = GetScreen()->m_Curseur;
    InitPosition    = m_drawItem->GetPosition();
    ItemPreviousPos = GetScreen()->m_Curseur;

    switch( m_drawItem->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
    {
        wxPoint cursor = StartCursor;
        cursor.y = -cursor.y;

        // The arc center point has to be rotated with while adjusting the
        // start or end point, determine the side of this point and the distance
        // from the start / end point
        wxPoint startPoint  = ( (LIB_ARC*) m_drawItem )->m_ArcStart;
        wxPoint endPoint    = ( (LIB_ARC*) m_drawItem )->m_ArcEnd;
        wxPoint centerPoint = ( (LIB_ARC*) m_drawItem )->m_Pos;
        wxPoint middlePoint = wxPoint( (startPoint.x + endPoint.x) / 2,
                                      (startPoint.y + endPoint.y) / 2 );
        wxPoint centerVector   = centerPoint - middlePoint;
        wxPoint startEndVector = TwoPointVector( startPoint, endPoint );
        arcState.distanceCenter = EuclideanNorm( centerVector );

        // Determine on which side is the center point
        arcState.direction = CrossProduct( startEndVector, centerVector ) ? 1 : -1;

        arcState.startPoint = startPoint;
        arcState.endPoint   = endPoint;

        // Save the old values
        savedAttributes.startPoint  = startPoint;
        savedAttributes.endPoint    = endPoint;
        savedAttributes.centerPoint = centerPoint;

        // Drag either the start, end point or the outline
        if( HitTestPoints( startPoint, cursor, MINIMUM_SELECTION_DISTANCE ) )
        {
            arcState.selectedPoint = START_POINT;
        }
        else if( HitTestPoints( endPoint, cursor, MINIMUM_SELECTION_DISTANCE ) )
        {
            arcState.selectedPoint = END_POINT;
        }
        else
            arcState.selectedPoint = OUTLINE;

        arcState.stateDrawArc = 0;

        m_drawItem->m_Flags |= IS_RESIZED;
    }
    break;

    case COMPONENT_RECT_DRAW_TYPE:
    {
        // Save old attributes
        savedAttributes.startPoint = ( (LIB_RECTANGLE*) m_drawItem )->m_Pos;
        savedAttributes.endPoint   = ( (LIB_RECTANGLE*) m_drawItem )->m_End;

        // Resize rectangle
        m_drawItem->m_Flags |= IS_RESIZED;

        // If the cursor is not on the rectangle edge point,
        // lock the width or height

        wxPoint rectEnd   = ( (LIB_RECTANGLE*) m_drawItem )->m_End;
        wxPoint rectStart = ( (LIB_RECTANGLE*) m_drawItem )->m_Pos;
        wxPoint cursor    = StartCursor;
        cursor.y = -cursor.y;

        ( (LIB_RECTANGLE*) m_drawItem )->m_isStartPointSelected =
            abs( rectStart.x - cursor.x ) < MINIMUM_SELECTION_DISTANCE
            || abs( rectStart.y - cursor.y ) < MINIMUM_SELECTION_DISTANCE;

        if( ( (LIB_RECTANGLE*) m_drawItem )->m_isStartPointSelected )
        {
            ( (LIB_RECTANGLE*) m_drawItem )->m_isWidthLocked =
                abs( rectStart.x - cursor.x ) >= MINIMUM_SELECTION_DISTANCE;

            ( (LIB_RECTANGLE*) m_drawItem )->m_isHeightLocked =
                abs( rectStart.y - cursor.y ) >= MINIMUM_SELECTION_DISTANCE;
        }
        else
        {
            ( (LIB_RECTANGLE*) m_drawItem )->m_isWidthLocked =
                abs( rectEnd.x - cursor.x ) >= MINIMUM_SELECTION_DISTANCE;

            ( (LIB_RECTANGLE*) m_drawItem )->m_isHeightLocked =
                abs( rectEnd.y - cursor.y ) >= MINIMUM_SELECTION_DISTANCE;
        }
    }
    break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        savedAttributes.radius = ( (LIB_CIRCLE*) m_drawItem )->m_Radius;
        m_drawItem->m_Flags   |= IS_RESIZED;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        // Drag one edge point of the polyline

        m_drawItem->m_Flags |= IS_RESIZED;

        // Find the nearest edge point to be dragged
        wxPoint cursor     = StartCursor;
        wxPoint startPoint = ( ( (LIB_POLYLINE*) m_drawItem )->m_PolyPoints )[0];
        cursor.y = NEGATE( cursor.y );

        // Begin with the first list point as nearest point
        int index = 0;
        ( (LIB_POLYLINE*) m_drawItem )->m_ModifyIndex = 0;
        savedAttributes.startPoint = startPoint;

        // First distance is the current minimum distance
        int distanceMin = (cursor - startPoint).x * (cursor - startPoint).x
                          + (cursor - startPoint).y * (cursor - startPoint).y;

        // Find the right index of the point to be dragged
        BOOST_FOREACH( wxPoint point, ( ( (LIB_POLYLINE*) m_drawItem )->m_PolyPoints ) ) {
            int distancePoint = (cursor - point).x * (cursor - point).x +
                                (cursor - point).y * (cursor - point).y;

            if( distancePoint < distanceMin )
            {
                // Save point
                savedAttributes.startPoint = point;

                ( (LIB_POLYLINE*) m_drawItem )->m_ModifyIndex = index;
                distanceMin = distancePoint;
            }
            index++;
        }
    }
    break;

    default:
        break;
    }

    DrawPanel->ManageCurseur = SymbolDisplayDraw;
    DrawPanel->ForceCloseManageCurseur = AbortSymbolTraceOn;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


//! @brief Manage mouse events when creating new graphic object or modifying an graphic object.
static void SymbolDisplayDraw( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int            dx, dy;
    int            DrawMode = g_XorMode;
    BASE_SCREEN*   Screen   = panel->GetScreen();
    wxPoint        currentCursorPosition = Screen->m_Curseur;
    FILL_T         fillStyle = NO_FILL;
    LIB_DRAW_ITEM* item;

    item = ( (WinEDA_LibeditFrame*) panel->GetParent() )->GetDrawItem();

    if( item == NULL )
        return;

    fillStyle = ( (WinEDA_LibeditFrame*) panel->GetParent() )->GetFillStyle();

    NEGATE( currentCursorPosition.y );

    GRSetDrawMode( DC, DrawMode );

    if( erase )
    {
        if( arcState.stateDrawArc == 1 )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRLine( &panel->m_ClipBox, DC, arcState.startPoint.x, -arcState.startPoint.y,
                    arcState.endPoint.x, -arcState.endPoint.y, 0, Color );
        }
        else
        {
            item->Draw( panel, DC, wxPoint( 0, 0 ), -1, DrawMode, NULL,
                        DefaultTransformMatrix );

            if( item->Type() == COMPONENT_ARC_DRAW_TYPE  && item->m_Flags != IS_RESIZED )
            {
                int Color = ReturnLayerColor( LAYER_DEVICE );
                GRDashedLine( &panel->m_ClipBox, DC, arcState.startPoint.x, -arcState.startPoint.y,
                              ( (LIB_ARC*) item )->m_Pos.x,
                              -( (LIB_ARC*) item )->m_Pos.y,
                              0, Color );
                GRDashedLine( &panel->m_ClipBox, DC, arcState.endPoint.x, -arcState.endPoint.y,
                              ( (LIB_ARC*) item )->m_Pos.x,
                              -( (LIB_ARC*) item )->m_Pos.y,
                              0, Color );
            }
        }
    }

    switch( item->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
        if( item->m_Flags == IS_RESIZED )
        {
            wxPoint newCenterPoint;

            // Choose the point of the arc to be adjusted
            if( arcState.selectedPoint == START_POINT )
            {
                arcState.startPoint = currentCursorPosition;
                arcState.endPoint   = ( (LIB_ARC*) item )->m_ArcEnd;
            }
            else if( arcState.selectedPoint == END_POINT )
            {
                arcState.endPoint   = currentCursorPosition;
                arcState.startPoint = ( (LIB_ARC*) item )->m_ArcStart;
            }
            else
            {
                // Use the cursor for adjusting the arc curvature
                arcState.startPoint = ( (LIB_ARC*) item )->m_ArcStart;
                arcState.endPoint   = ( (LIB_ARC*) item )->m_ArcEnd;

                wxPoint middlePoint = wxPoint( (arcState.startPoint.x + arcState.endPoint.x) / 2,
                                              (arcState.startPoint.y + arcState.endPoint.y) / 2 );


                // If the distance is too small, use the old center point
                // else the new center point is calculated over the three points start/end/cursor
                if( DistanceLinePoint( arcState.startPoint, arcState.endPoint,
                                       currentCursorPosition )
                    > MINIMUM_SELECTION_DISTANCE )
                {
                    newCenterPoint = ComputeCircumCenter( arcState.startPoint,
                                                          currentCursorPosition, arcState.endPoint );
                }
                else
                {
                    newCenterPoint = ( (LIB_ARC*) item )->m_Pos;
                }

                // Determine if the arc angle is larger than 180 degrees -> this happens if both
                // points (cursor position, center point) lie on the same side of the vector start-end
                int  crossA = CrossProduct( TwoPointVector( arcState.startPoint, arcState.endPoint ),
                                           TwoPointVector( arcState.endPoint,
                                                           currentCursorPosition ) );
                int  crossB = CrossProduct( TwoPointVector( arcState.startPoint, arcState.endPoint ),
                                           TwoPointVector( arcState.endPoint, newCenterPoint ) );

                bool isLarger180degrees = (crossA < 0 && crossB < 0) || (crossA >=0 && crossB >=0);

                if( isLarger180degrees )
                    newCenterPoint = ( (LIB_ARC*) item )->m_Pos;
            }

            if( arcState.selectedPoint == START_POINT || arcState.selectedPoint == END_POINT )
            {
                // Compute the new center point when the start/end points are modified
                wxPoint middlePoint = wxPoint( (arcState.startPoint.x + arcState.endPoint.x) / 2,
                                              (arcState.startPoint.y + arcState.endPoint.y) / 2 );

                wxPoint startEndVector = TwoPointVector( arcState.startPoint, arcState.endPoint );
                wxPoint perpendicularVector = wxPoint( -startEndVector.y, startEndVector.x );

                double  lengthPerpendicularVector = EuclideanNorm( perpendicularVector );

                // prevent too large values, division / 0
                if( lengthPerpendicularVector < 1e-1 )
                    lengthPerpendicularVector = 1e-1;

                perpendicularVector.x = (int) ( (double) perpendicularVector.x *
                                               arcState.distanceCenter /
                                               lengthPerpendicularVector ) * arcState.direction;
                perpendicularVector.y = (int) ( (double) perpendicularVector.y *
                                               arcState.distanceCenter /
                                               lengthPerpendicularVector ) * arcState.direction;

                newCenterPoint = middlePoint + perpendicularVector;

                ( (LIB_ARC*) item )->m_ArcStart = arcState.startPoint;
                ( (LIB_ARC*) item )->m_ArcEnd   = arcState.endPoint;
            }

            ( (LIB_ARC*) item )->m_Pos = newCenterPoint;
            ComputeArcRadiusAngles( (LIB_ARC*) item );
        }
        else
        {
            if( arcState.stateDrawArc == 1 )
            {
                arcState.endPoint.x = currentCursorPosition.x;
                arcState.endPoint.y = currentCursorPosition.y;
            }

            if( arcState.stateDrawArc == 2 )
            {
                ComputeArc( (LIB_ARC*) item, Screen->m_Curseur );
            }
            item->m_Fill = fillStyle;
        }
        break;

    case COMPONENT_CIRCLE_DRAW_TYPE:
        dx = ( (LIB_CIRCLE*) item )->m_Pos.x - currentCursorPosition.x;
        dy = ( (LIB_CIRCLE*) item )->m_Pos.y - currentCursorPosition.y;
        ( (LIB_CIRCLE*) item )->m_Radius =
            (int) sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        item->m_Fill = fillStyle;
        break;

    case COMPONENT_RECT_DRAW_TYPE:
        if( ( (LIB_RECTANGLE*) item )->m_isHeightLocked )
        {
            if( ( (LIB_RECTANGLE*) item )->m_isStartPointSelected )
            {
                ( (LIB_RECTANGLE*) item )->m_Pos.x = currentCursorPosition.x;
            }
            else
            {
                ( (LIB_RECTANGLE*) item )->m_End.x = currentCursorPosition.x;
            }
        }
        else if( ( (LIB_RECTANGLE*) item )->m_isWidthLocked )
        {
            if( ( (LIB_RECTANGLE*) item )->m_isStartPointSelected )
            {
                ( (LIB_RECTANGLE*) item )->m_Pos.y = currentCursorPosition.y;
            }
            else
            {
                ( (LIB_RECTANGLE*) item )->m_End.y = currentCursorPosition.y;
            }
        }
        else
        {
            if( ( (LIB_RECTANGLE*) item )->m_isStartPointSelected )
            {
                ( (LIB_RECTANGLE*) item )->m_Pos = currentCursorPosition;
            }
            else
            {
                ( (LIB_RECTANGLE*) item )->m_End = currentCursorPosition;
            }
        }

        item->m_Fill = fillStyle;
        break;

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        unsigned idx;

        if( item->m_Flags == IS_RESIZED )
            idx = ( (LIB_POLYLINE*) item )->m_ModifyIndex;
        else
            idx = ( (LIB_POLYLINE*) item )->GetCornerCount() - 1;

        ( (LIB_POLYLINE*) item )->m_PolyPoints[idx] = currentCursorPosition;
        item->m_Fill = fillStyle;
    }
    break;

    case COMPONENT_LINE_DRAW_TYPE:
        ( (LIB_SEGMENT*) item )->m_End = currentCursorPosition;
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        break;

    default:
        ;
    }

    if( arcState.stateDrawArc == 1 )
    {
        int Color = ReturnLayerColor( LAYER_DEVICE );
        GRLine( &panel->m_ClipBox,
                DC,
                arcState.startPoint.x,
                -arcState.startPoint.y,
                arcState.endPoint.x,
                -arcState.endPoint.y,
                0,
                Color );
    }
    else
    {
        item->Draw( panel, DC, wxPoint( 0, 0 ), -1, DrawMode, NULL,
                    DefaultTransformMatrix );

        if( item->Type() == COMPONENT_ARC_DRAW_TYPE  && item->m_Flags != IS_RESIZED )
        {
            int Color = ReturnLayerColor( LAYER_DEVICE );
            GRDashedLine( &panel->m_ClipBox, DC, arcState.startPoint.x, -arcState.startPoint.y,
                          ( (LIB_ARC*) item )->m_Pos.x,
                          -( (LIB_ARC*) item )->m_Pos.y,
                          0, Color );
            GRDashedLine( &panel->m_ClipBox, DC, arcState.endPoint.x, -arcState.endPoint.y,
                          ( (LIB_ARC*) item )->m_Pos.x,
                          -( (LIB_ARC*) item )->m_Pos.y,
                          0, Color );
        }
    }
}


/*
 * Place the new graphic object in the list of component drawing objects.
 */
void WinEDA_LibeditFrame::EndDrawGraphicItem( wxDC* DC )
{
    if( m_component == NULL || m_drawItem == NULL )
        return;

    if( m_drawItem->Type() == COMPONENT_ARC_DRAW_TYPE )
    {
        if( arcState.stateDrawArc == 1 ) /* Trace arc under way must be completed. */
        {
            DisplayError( this, wxT( "Arc in progress.." ) );
            return;
        }
        else
        {
            if( ( m_drawItem->m_Flags & IS_MOVED ) == 0 )
                SymbolDisplayDraw( DrawPanel, DC, FALSE );
        }
    }

    arcState.stateDrawArc = 0;

    if( m_drawItem->m_Flags & IS_NEW )
    {
        SaveCopyInUndoList( m_component );
        m_component->AddDrawItem( m_drawItem );

        switch( m_drawItem->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            ( (LIB_ARC*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LIB_CIRCLE*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LIB_RECTANGLE*) m_drawItem )->m_Fill = m_drawFillStyle;
            ( (LIB_RECTANGLE*) m_drawItem )->m_isHeightLocked = false;
            ( (LIB_RECTANGLE*) m_drawItem )->m_isWidthLocked  = false;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            ( (LIB_POLYLINE*) m_drawItem )->m_Fill = m_drawFillStyle;
            break;

        case COMPONENT_PIN_DRAW_TYPE:
        case COMPONENT_LINE_DRAW_TYPE:
        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            break;

        default:
            ;
        }
    }

    if( m_ID_current_state )
        SetCursor( wxCURSOR_PENCIL );
    else
        SetCursor( wxCURSOR_ARROW );

    if( m_drawItem->m_Flags & IS_MOVED )
    {
        wxPoint pos;
        pos.x = GetScreen()->m_Curseur.x + InitPosition.x - StartCursor.x,
        pos.y = GetScreen()->m_Curseur.y - InitPosition.y - StartCursor.y;
        NEGATE( pos.y );
        m_drawItem->Move( pos );
    }

    m_component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), m_unit, m_convert,
                       GR_DEFAULT_DRAWMODE );

    m_drawItem->m_Flags = 0;
    m_drawItem = NULL;

    OnModify( );

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
}


//! @brief Compute the missing attributes (angles, radius),
// when the three points (start, end, center) given
//! @param arc Arc to be modified
static void ComputeArcRadiusAngles( LIB_ARC* arc )
{
    wxPoint centerStartVector = TwoPointVector( arc->m_Pos, arc->m_ArcStart );
    wxPoint centerEndVector   = TwoPointVector( arc->m_Pos, arc->m_ArcEnd );

    arc->m_Radius = wxRound( EuclideanNorm( centerStartVector ) );

    arc->m_t1 = (int) ( atan2( (double) centerStartVector.y,
                              (double) centerStartVector.x ) * 1800 / M_PI );

    arc->m_t2 = (int) ( atan2( (double) centerEndVector.y,
                              (double) centerEndVector.x ) * 1800 / M_PI );

    NORMALIZE_ANGLE( arc->m_t1 );
    NORMALIZE_ANGLE( arc->m_t2 );  // angles = 0 .. 3600

    // Restrict angle to less than 180 to avoid PBS display mirror
    // Trace because it is assumed that the arc is less than 180 deg to find
    // orientation after rotate or mirror.
    if( (arc->m_t2 - arc->m_t1) > 1800 )
        arc->m_t2 -= 3600;
    else if( (arc->m_t2 - arc->m_t1) <= -1800 )
        arc->m_t2 += 3600;

    wxString msg;
    int      angle = arc->m_t2 - arc->m_t1;
    msg.Printf( _( "Arc %.1f deg" ), (float) angle / 10 );
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame->PrintMsg( msg );

    while( (arc->m_t2 - arc->m_t1) >= 1800 )
    {
        arc->m_t2--;
        arc->m_t1++;
    }

    while( (arc->m_t1 - arc->m_t2) >= 1800 )
    {
        arc->m_t2++;
        arc->m_t1--;
    }

    NORMALIZE_ANGLE( arc->m_t1 );
}


/*
 * Routine for adjusting the parameters of the arc currently being drawn.
 * Calculates the center, radius, angles for the arc current
 * Passes through the points arcState.startPoint.x, arcState.endPoint.x Y and Y with the nearest center
 * of the mouse position.
 * Note: The center is obviously not on the grid
 */
static void ComputeArc( LIB_ARC* DrawItem, wxPoint ArcCentre )
{
    int dx, dy;
    int cX, cY;
    int angle;

    cX = ArcCentre.x;
    cY = ArcCentre.y;

    cY = -cY;   /* Attention to the orientation of the axis Y. */

    /* Calculating cX and cY for the arc passes through arcState.startPoint.x, arcState.endPoint.x,
     * X and Y */
    dx    = arcState.endPoint.x - arcState.startPoint.x;
    dy    = arcState.endPoint.y - arcState.startPoint.y;
    cX   -= arcState.startPoint.x;
    cY   -= arcState.startPoint.y;
    angle = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );
    RotatePoint( &dx, &dy, angle );     /* The segment dx, dy is horizontal
                                         * -> Length = dx, dy = 0 */
    RotatePoint( &cX, &cY, angle );
    cX = dx / 2;           /* cX, cY is on the median segment 0.0 a dx, 0 */

    RotatePoint( &cX, &cY, -angle );
    cX += arcState.startPoint.x;
    cY += arcState.startPoint.y;
    DrawItem->m_Pos.x = cX;
    DrawItem->m_Pos.y = cY;

    dx = arcState.startPoint.x - DrawItem->m_Pos.x;
    dy = arcState.startPoint.y - DrawItem->m_Pos.y;

    DrawItem->m_Radius = (int) sqrt( ( (double) dx * dx ) +
                                    ( (double) dy * dy ) );

    DrawItem->m_t1 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    dx = arcState.endPoint.x - DrawItem->m_Pos.x;
    dy = arcState.endPoint.y - DrawItem->m_Pos.y;

    DrawItem->m_t2 = (int) ( atan2( (double) dy, (double) dx ) * 1800 / M_PI );

    DrawItem->m_ArcStart.x = arcState.startPoint.x;
    DrawItem->m_ArcStart.y = arcState.startPoint.y;
    DrawItem->m_ArcEnd.x   = arcState.endPoint.x;
    DrawItem->m_ArcEnd.y   = arcState.endPoint.y;

    NORMALIZE_ANGLE( DrawItem->m_t1 );
    NORMALIZE_ANGLE( DrawItem->m_t2 );  // angles = 0 .. 3600

    // Restrict angle to less than 180 to avoid PBS display mirror
    // Trace because it is assumed that the arc is less than 180 deg to find
    // orientation after rotate or mirror.
    if( (DrawItem->m_t2 - DrawItem->m_t1) > 1800 )
        DrawItem->m_t2 -= 3600;
    else if( (DrawItem->m_t2 - DrawItem->m_t1) <= -1800 )
        DrawItem->m_t2 += 3600;

    wxString msg;
    angle = DrawItem->m_t2 - DrawItem->m_t1;
    msg.Printf( _( "Arc %.1f deg" ), (float) angle / 10 );
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();
    frame->m_LibeditFrame->PrintMsg( msg );

    while( (DrawItem->m_t2 - DrawItem->m_t1) >= 1800 )
    {
        DrawItem->m_t2--;
        DrawItem->m_t1++;
    }

    while( (DrawItem->m_t1 - DrawItem->m_t2) >= 1800 )
    {
        DrawItem->m_t2++;
        DrawItem->m_t1--;
    }

    NORMALIZE_ANGLE( DrawItem->m_t1 );
    NORMALIZE_ANGLE( DrawItem->m_t2 );
}


//! @brief Given three points A B C, compute the circumcenter of the resulting triangle
//! reference: http://en.wikipedia.org/wiki/Circumscribed_circle
//! Coordinates of circumcenter in Cartesian coordinates
static wxPoint ComputeCircumCenter( wxPoint A, wxPoint B, wxPoint C )
{
    double  circumCenterX, circumCenterY;
    double  Ax = (double) A.x;
    double  Ay = (double) A.y;
    double  Bx = (double) B.x;
    double  By = (double) B.y;
    double  Cx = (double) C.x;
    double  Cy = (double) C.y;

    wxPoint circumCenter;

    double  D = 2.0 * ( Ax * ( By - Cy ) + Bx * ( Cy - Ay ) + Cx * ( Ay - By ) );

    // prevent division / 0
    if( fabs( D ) < 1e-7 )
        D = 1e-7;

    circumCenterX = ( (Ay * Ay + Ax * Ax) * (By - Cy) +
                     (By * By + Bx * Bx) * (Cy - Ay) +
                     (Cy * Cy + Cx * Cx) * (Ay - By) ) / D;

    circumCenterY = ( (Ay * Ay + Ax * Ax) * (Cx - Bx) +
                     (By * By + Bx * Bx) * (Ax - Cx) +
                     (Cy * Cy + Cx * Cx) * (Bx - Ax) ) / D;

    circumCenter.x = (int) circumCenterX;
    circumCenter.y = (int) circumCenterY;

    return circumCenter;
}


/*
 * Used for deleting last entered segment while creating a Polyline
 */
void WinEDA_LibeditFrame::DeleteDrawPoly( wxDC* DC )
{
    if( m_drawItem == NULL
        || m_drawItem->Type() != COMPONENT_POLYLINE_DRAW_TYPE )
        return;

    LIB_POLYLINE* Poly = (LIB_POLYLINE*) m_drawItem;

    m_drawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                      DefaultTransformMatrix );

    // First segment is kept, only its end point is changed
    while( Poly->GetCornerCount() > 2 )
    {
        Poly->m_PolyPoints.pop_back();
        unsigned idx   = Poly->GetCornerCount() - 1;
        wxPoint  point = GetScreen()->m_Curseur;
        NEGATE( point.y );
        if( Poly->m_PolyPoints[idx] != point )
        {
            Poly->m_PolyPoints[idx] = point;
            break;
        }
    }

    m_drawItem->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                      DefaultTransformMatrix );
}
