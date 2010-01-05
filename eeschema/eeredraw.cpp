/*****************************************************************************
*   Program to draw EE diagrams.                                             *
* This module redraw/draw all structs.                                       *
*****************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include "build_version.h"

static EDA_BaseStruct* HighLightStruct = NULL;


void DrawDanglingSymbol( WinEDA_DrawPanel* panel, wxDC* DC,
                         const wxPoint& pos, int Color )
{
    BASE_SCREEN* screen = panel->GetScreen();

    if( !screen->m_IsPrinting ) /* Draw but do not print the Dangling Symbol */
    {
        GRRect( &panel->m_ClipBox, DC,
                pos.x - DANGLING_SYMBOL_SIZE, pos.y - DANGLING_SYMBOL_SIZE,
                pos.x + DANGLING_SYMBOL_SIZE, pos.y + DANGLING_SYMBOL_SIZE,
                0, Color );
    }
}


void SetHighLightStruct( EDA_BaseStruct* HighLight )
{
    HighLightStruct = HighLight;
}


/*
 * Redraws only the active window which is assumed to be whole visible.
 */
void WinEDA_SchematicFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    wxString title;

    if( GetScreen() == NULL )
        return;

    ActiveScreen = GetScreen();

    /* Reinit draw and pen parameters */
    GRResetPenAndBrush( DC );
    DC->SetBackground( *wxBLACK_BRUSH );
    DC->SetBackgroundMode( wxTRANSPARENT );

    DrawPanel->CursorOff( DC );

    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    RedrawStructList( DrawPanel, DC, GetScreen()->EEDrawList,
                      GR_DEFAULT_DRAWMODE );

    TraceWorkSheet( DC, GetScreen(), g_DrawDefaultLineThickness );

    DrawPanel->CursorOn( DC );
    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    GetScreen()->ClrRefreshReq();

    // Display the sheet filename, and the sheet path, for non root sheets
    if( GetScreen()->m_FileName == m_DefaultSchematicFileName )
    {
        wxString msg = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();
        title.Printf( wxT( "%s [%s]" ), msg.GetData(),
                     GetScreen()->m_FileName.GetData() );
        SetTitle( title );
    }
    else
    {
        title = wxT( "[" );
        title << GetScreen()->m_FileName << wxT( "]  " ) << _( "Sheet" );
        title << wxT( " " ) << m_CurrentSheet->PathHumanReadable();
        SetTitle( title );
    }
}


/**
 * PrintPage
 * used to print a page.
 * Print the page pointed by ActiveScreen, set by the calling print function
 * @param DC = wxDC given by the calling print function
 * @param Print_Sheet_Ref = true to print page references
 * @param PrintMask = not used here
 * @param aPrintMirrorMode = not used here (Set when printing in mirror mode)
 */
void WinEDA_DrawPanel::PrintPage( wxDC* DC, bool Print_Sheet_Ref,
                                  int PrintMask, bool aPrintMirrorMode )
{
    wxBeginBusyCursor();

    RedrawStructList( this, DC, ActiveScreen->EEDrawList, GR_COPY );

    if( Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( DC, ActiveScreen, g_DrawDefaultLineThickness );

    wxEndBusyCursor();
}


/*****************************************************************************
* Routine to redraw list of structs.                                         *
* If the list is of DrawPickStruct types then the picked item are drawn.     *
*****************************************************************************/
void RedrawStructList( WinEDA_DrawPanel* panel, wxDC* DC,
                       SCH_ITEM* Structlist, int DrawMode, int Color )
{
    while( Structlist )
    {
        if( !(Structlist->m_Flags & IS_MOVED) )
        {
// uncomment line below when there is a virtual
// EDA_BaseStruct::GetBoundingBox()
            //      if( panel->m_ClipBox.Intersects( Structs->GetBoundingBox()
            // ) )
            RedrawOneStruct( panel, DC, Structlist, DrawMode, Color );
        }

        Structlist = Structlist->Next();
    }
}


/*****************************************************************************
* Routine to redraw list of structs.                                         *
*****************************************************************************/
void RedrawOneStruct( WinEDA_DrawPanel* panel, wxDC* DC,
                      SCH_ITEM* Struct, int DrawMode, int Color )
{
    if( Struct == NULL )
        return;

    if( HighLightStruct == Struct )
        Color = HIGHLIGHT_COLOR;

    Struct->Draw( panel, DC, wxPoint( 0, 0 ), DrawMode, Color );
}


/* Routine for repainting item in ghost mode. Used in the block moves. */
void DrawStructsInGhost( WinEDA_DrawPanel* aPanel,
                         wxDC*             aDC,
                         SCH_ITEM*         aItem,
                         const wxPoint&    aOffset )
{
    int DrawMode = g_XorMode;
    int width    = g_DrawDefaultLineThickness;

    GRSetDrawMode( aDC, DrawMode );

    switch( aItem->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
    {
        SCH_POLYLINE* Struct = (SCH_POLYLINE*) aItem;
        GRMoveTo( Struct->m_PolyPoints[0].x + aOffset.x,
                  Struct->m_PolyPoints[0].y + aOffset.y );
        for( unsigned ii = 1; ii < Struct->GetCornerCount(); ii++ )
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      Struct->m_PolyPoints[ii].x + aOffset.x,
                      Struct->m_PolyPoints[ii].y + aOffset.y,
                      width,
                      g_GhostColor );

        break;
    }

    case DRAW_SEGMENT_STRUCT_TYPE:
    {
        SCH_LINE* Struct;
        Struct = (SCH_LINE*) aItem;
        if( (Struct->m_Flags & STARTPOINT) == 0 )
        {
            GRMoveTo( Struct->m_Start.x + aOffset.x,
                      Struct->m_Start.y + aOffset.y );
        }
        else
        {
            GRMoveTo( Struct->m_Start.x, Struct->m_Start.y );
        }
        if( (Struct->m_Flags & ENDPOINT) == 0 )
        {
            GRLineTo( &aPanel->m_ClipBox, aDC, Struct->m_End.x + aOffset.x,
                      Struct->m_End.y + aOffset.y, width, g_GhostColor );
        }
        else
        {
            GRLineTo( &aPanel->m_ClipBox, aDC, Struct->m_End.x,
                      Struct->m_End.y, width, g_GhostColor );
        }
        break;
    }

    case DRAW_BUSENTRY_STRUCT_TYPE:
    {
        SCH_BUS_ENTRY* Struct = (SCH_BUS_ENTRY*) aItem;
        wxPoint        start  = Struct->m_Pos + aOffset;
        GRMoveTo( start.x, start.y );
        GRLineTo( &aPanel->m_ClipBox, aDC, Struct->m_Size.x + start.x,
                  Struct->m_Size.y + start.y, width, g_GhostColor );
        break;
    }

    case DRAW_JUNCTION_STRUCT_TYPE:
    {
        SCH_JUNCTION* Struct;
        Struct = (SCH_JUNCTION*) aItem;
        Struct->Draw( aPanel, aDC, aOffset, DrawMode, g_GhostColor );
        break;
    }

    case TYPE_SCH_TEXT:
    {
        SCH_TEXT* Struct;
        Struct = (SCH_TEXT*) aItem;
        Struct->Draw( aPanel, aDC, aOffset, DrawMode, g_GhostColor );
        break;
    }

    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    {
        SCH_LABEL* Struct;
        Struct = (SCH_LABEL*) aItem;
        Struct->Draw( aPanel, aDC, aOffset, DrawMode, g_GhostColor );
        break;
    }

    case DRAW_NOCONNECT_STRUCT_TYPE:
    {
        SCH_NO_CONNECT* Struct;
        Struct = (SCH_NO_CONNECT*) aItem;
        Struct->Draw( aPanel, aDC, aOffset, DrawMode, g_GhostColor );
        break;
    }

    case TYPE_SCH_COMPONENT:
    {
        SCH_COMPONENT* Component = (SCH_COMPONENT*) aItem;

        if( Component == NULL )
            break;

        Component->Draw( aPanel, aDC, aOffset, g_XorMode, g_GhostColor, false );
        break;
    }

    case DRAW_SHEET_STRUCT_TYPE:
    {
        SCH_SHEET* Struct = (SCH_SHEET*) aItem;
        GRRect( &aPanel->m_ClipBox,
                aDC,
                Struct->m_Pos.x + aOffset.x,
                Struct->m_Pos.y + aOffset.y,
                Struct->m_Pos.x + Struct->m_Size.x + aOffset.x,
                Struct->m_Pos.y + Struct->m_Size.y + aOffset.y,
                width,
                g_GhostColor );
        break;
    }

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    case TYPE_SCH_MARKER:
        break;

    default:
        break;
    }
}
