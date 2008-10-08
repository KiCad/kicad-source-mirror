/*****************************************************************************
*	Program to draw EE diagrams.											 *
* This module redraw/draw all structs.										 *
*****************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


char marq_bitmap[] =
{
    12, 12, 0, 0,                           /* Dimensions x et y, offsets x et y du bitmap de marqueurs*/
    YELLOW,                                 /* Couleur */
    1,  1,  1, 1, 1, 1, 1, 1, 0, 0, 0, 0,   /* bitmap: >= 1 : color, 0 = notrace */
    1,  1,  1, 0, 1, 0, 1, 1, 0, 0, 0, 0,
    1,  1,  1, 1, 0, 0, 0, 1, 0, 0, 0, 0,
    1,  0,  1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    1,  1,  0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1,  1,  0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    1,  1,  1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0,  0,  0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0,  0,  0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
    0,  0,  0, 0, 0, 0, 0, 0, 1, 1, 1, 0,
    0,  0,  0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 1, 0
};

char marqERC_bitmap[] =
{
    8, 8, 0, 0, /* Dimensions x et y , offsets x et y du bitmap de marqueurs*/
    -1,         /* Color: -1 = couleur non pr�cis�e */
    1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 0, 1, 0, 0, 0,
    1, 1, 1, 1, 0, 0, 0, 0,
    1, 0, 1, 1, 1, 0, 0, 0,
    1, 1, 0, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 0,
};


static EDA_BaseStruct* HighLightStruct = NULL;

/************************************************************/
void DrawDanglingSymbol( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& pos, int Color )
/************************************************************/
{
    if( !g_IsPrinting )   // Draw but do not print the Dangling Symbol */
    {
        GRRect( &panel->m_ClipBox, DC,
            pos.x - DANGLING_SYMBOL_SIZE, pos.y - DANGLING_SYMBOL_SIZE,
            pos.x + DANGLING_SYMBOL_SIZE, pos.y + DANGLING_SYMBOL_SIZE,
            0, Color );
    }
}


/*************************************************/
void SetHighLightStruct( EDA_BaseStruct* HighLight )
/*************************************************/
{
    HighLightStruct = HighLight;
}


/**********************************************************************/
void WinEDA_SchematicFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/**********************************************************************/

/*
  * Redraws only the active window which is assumed to be whole visible.
 */
{
    wxString title;

    if( GetScreen() == NULL )
        return;

    ActiveScreen = GetScreen();

    /* Reinit draw and pen parameters */
    GRResetPenAndBrush( DC );
    DC->SetBackground( *wxBLACK_BRUSH );
    DC->SetBackgroundMode( wxTRANSPARENT );

    DrawPanel->CursorOff( DC ); // effacement curseur

    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    RedrawStructList( DrawPanel, DC, GetScreen()->EEDrawList, GR_DEFAULT_DRAWMODE );

    TraceWorkSheet( DC, GetScreen(), g_DrawMinimunLineWidth );

    DrawPanel->CursorOn( DC );          // reaffichage curseur
    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    Affiche_Status_Box();
    GetScreen()->ClrRefreshReq();

    // Display the sheet filename, and the sheet path, for non root sheets
    if( GetScreen()->m_FileName == g_DefaultSchematicFileName ) // This is the root sheet
    {
        wxString msg = g_Main_Title + wxT( " " ) + GetBuildVersion();
        title.Printf( wxT( "%s [%s]" ), msg.GetData(), GetScreen()->m_FileName.GetData() );
        SetTitle( title );
    }
    else
    {
        title = wxT( "[" );
        title << GetScreen()->m_FileName << wxT( "]  " ) << _("Sheet") ;
        title << wxT( " " ) << m_CurrentSheet->PathHumanReadable();
        SetTitle( title );
    }
}


/*******************************************************************************/
void WinEDA_DrawPanel::PrintPage( wxDC* DC, bool Print_Sheet_Ref, int PrintMask )
/*******************************************************************************/
/** PrintPage
 * used to print a page.
 * Print the page pointed by ActiveScreen, set by the calling print function
 * @param DC = wxDC given by the calling print function
 * @param Print_Sheet_Ref = true to print page references
 * @param PrintMask = not used here
 */
{
    wxBeginBusyCursor();

    RedrawStructList( this, DC, ActiveScreen->EEDrawList, GR_COPY );

    if( Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( DC, ActiveScreen, g_DrawMinimunLineWidth );

    wxEndBusyCursor();
}


/*****************************************************************************
* Routine to redraw list of structs.										 *
* If the list is of DrawPickStruct types then the picked item are drawn.	 *
*****************************************************************************/
void RedrawStructList( WinEDA_DrawPanel* panel, wxDC* DC,
                       SCH_ITEM* Structs, int DrawMode, int Color )
{
    while( Structs )
    {
        if( Structs->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            SCH_ITEM* item = ( (DrawPickedStruct*) Structs )->m_PickedStruct;

// uncomment line below when there is a virtual EDA_BaseStruct::GetBoundingBox()
       //   if( panel->m_ClipBox.Intersects( item->GetBoundingBox() ) )
            {
                RedrawOneStruct( panel, DC, item, DrawMode, Color );
            }
        }
        else
        {
            if( !(Structs->m_Flags & IS_MOVED) )
            {
// uncomment line below when there is a virtual EDA_BaseStruct::GetBoundingBox()
        //      if( panel->m_ClipBox.Intersects( Structs->GetBoundingBox() ) )
                    RedrawOneStruct( panel, DC, Structs, DrawMode, Color );
            }
        }

        Structs = Structs->Next();
    }
}


/*****************************************************************************
* Routine to redraw list of structs.										 *
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


/*****************************************************************************************/
void EDA_DrawLineStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                               int DrawMode, int Color )
/*****************************************************************************************/
/* Draw wires, Bus, and dashed liges.. */
{
    int color;
    int zoom  = panel->GetZoom();
    int width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );
    if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
        width *= 3;

    if( m_Layer == LAYER_NOTES )
        GRDashedLine( &panel->m_ClipBox, DC, m_Start.x + offset.x, m_Start.y + offset.y,
            m_End.x + offset.x, m_End.y + offset.y, width, color );
    else
        GRLine( &panel->m_ClipBox, DC, m_Start.x + offset.x, m_Start.y + offset.y,
            m_End.x + offset.x, m_End.y + offset.y, width, color );

    if( m_StartIsDangling )
        DrawDanglingSymbol( panel, DC, m_Start + offset, color );

    if( m_EndIsDangling )
        DrawDanglingSymbol( panel, DC, m_End + offset, color );
}


/****************************************************************************************/
void DrawMarkerStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                             int DrawMode, int Color )
/****************************************************************************************/
{
#define WAR 1   // utilis� aussi dans erc.cpp

    if( m_Type == MARQ_ERC )
    {
        int color = Color;
        if( Color <= 0 )
        {
            color = (m_MarkFlags == WAR ) ?
                    g_LayerDescr.LayerColor[LAYER_ERC_WARN] :
                    g_LayerDescr.LayerColor[LAYER_ERC_ERR];
        }
        Draw_Marqueur( panel, DC, m_Pos + offset, marqERC_bitmap, DrawMode, color );
    }
    else
        Draw_Marqueur( panel, DC, m_Pos + offset, marq_bitmap, DrawMode, Color );
}


/*************************************************************************/
void DrawNoConnectStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                int DrawMode, int Color )
/*************************************************************************/
/* DRaw the "No Connect" symbol.. */
{
    const int DELTA = (DRAWNOCONNECT_SIZE / 2);
    int pX, pY, color;
    int width = g_DrawMinimunLineWidth;

    pX = m_Pos.x + offset.x; pY = m_Pos.y + offset.y;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( LAYER_NOCONNECT );
    GRSetDrawMode( DC, DrawMode );

    GRLine( &panel->m_ClipBox, DC, pX - DELTA, pY - DELTA, pX + DELTA, pY + DELTA, width, color );
    GRLine( &panel->m_ClipBox, DC, pX + DELTA, pY - DELTA, pX - DELTA, pY + DELTA, width, color );
}


EDA_Rect DrawNoConnectStruct::GetBoundingBox()
{
    const int DELTA = (DRAWNOCONNECT_SIZE / 2);
    EDA_Rect box( wxPoint(m_Pos.x-DELTA,m_Pos.y-DELTA), wxSize(2*DELTA,2*DELTA) );
    box.Normalize();
    return box;
}


/**************************************************************/
void DrawBusEntryStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                               int DrawMode, int Color )
/***************************************************************/

/* Draw the bus entries  .. */

{
    int color;
    int zoom  = panel->GetZoom();
    int width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
        width *= 3;

    GRLine( &panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
        m_End().x + offset.x, m_End().y + offset.y, width, color );
}


EDA_Rect DrawBusEntryStruct::GetBoundingBox()
{
    int dx = m_Pos.x - m_End().x;
    int dy = m_Pos.y - m_End().y;
    EDA_Rect box( wxPoint(m_Pos.x,m_Pos.y), wxSize(dx,dy) );
    box.Normalize();
    return box;
}

/*****************************************************************************
* Routine to redraw polyline struct.										 *
*****************************************************************************/
void DrawPolylineStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                               int DrawMode, int Color )
{
    int i, color;
    int zoom  = panel->GetZoom();
    int width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    if( (m_Layer == LAYER_BUS) && (zoom <= 16) )
    {
        width *= 3;
    }

    GRMoveTo( m_Points[0], m_Points[1] );
    if( m_Layer == LAYER_NOTES )
    {
        for( i = 1; i < m_NumOfPoints; i++ )
            GRDashedLineTo( &panel->m_ClipBox, DC, m_Points[i * 2] + offset.x,
                m_Points[i * 2 + 1] + offset.y, width, color );
    }
    else
    {
        for( i = 1; i < m_NumOfPoints; i++ )
            GRLineTo( &panel->m_ClipBox,
                DC,
                m_Points[i * 2] + offset.x,
                m_Points[i * 2 + 1] + offset.y,
                width,
                color );
    }
}


/*****************************************************************************
* Routine to redraw connection struct.										 *
*****************************************************************************/
void DrawJunctionStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                               int DrawMode, int Color )
{
    int color;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRFilledCircle( &panel->m_ClipBox, DC, m_Pos.x + offset.x, m_Pos.y + offset.y,
       DRAWJUNCTION_SIZE, 0, color, color );
}


/**********************************************************/
void DrawStructsInGhost( WinEDA_DrawPanel* panel, wxDC* DC,
                         SCH_ITEM* DrawStruct, int dx, int dy )
/**********************************************************/

/* Routine de redessin en mode fantome (Dessin simplifie en g_XorMode et
  * g_GhostColor
  * de structures.
  * Utilisee dans les deplacements de blocs
 */
{
    int Width, ii;
    int DrawMode = g_XorMode;
    int width    = g_DrawMinimunLineWidth;


    GRSetDrawMode( DC, DrawMode );

    switch( DrawStruct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
    {
        DrawPolylineStruct* Struct;
        Struct = (DrawPolylineStruct*) DrawStruct;
        GRMoveTo( Struct->m_Points[0] + dx, Struct->m_Points[1] + dy );
        for( ii = 1; ii < Struct->m_NumOfPoints; ii++ )
            GRLineTo( &panel->m_ClipBox, DC, Struct->m_Points[ii * 2] + dx,
                Struct->m_Points[ii * 2 + 1] + dy, width, g_GhostColor );

        break;
    }

    case DRAW_SEGMENT_STRUCT_TYPE:
    {
        EDA_DrawLineStruct* Struct;
        Struct = (EDA_DrawLineStruct*) DrawStruct;
        if( (Struct->m_Flags & STARTPOINT) == 0 )
        {
            GRMoveTo( Struct->m_Start.x + dx, Struct->m_Start.y + dy );
        }
        else
        {
            GRMoveTo( Struct->m_Start.x, Struct->m_Start.y );
        }
        if( (Struct->m_Flags & ENDPOINT) == 0 )
        {
            GRLineTo( &panel->m_ClipBox,
                DC,
                Struct->m_End.x + dx,
                Struct->m_End.y + dy,
                width,
                g_GhostColor );
        }
        else
        {
            GRLineTo( &panel->m_ClipBox, DC, Struct->m_End.x, Struct->m_End.y, width, g_GhostColor );
        }
        break;
    }

    case DRAW_BUSENTRY_STRUCT_TYPE:
    {
        DrawBusEntryStruct* Struct = (DrawBusEntryStruct*) DrawStruct;
        int xx = Struct->m_Pos.x + dx, yy = Struct->m_Pos.y + dy;
        GRMoveTo( xx, yy );
        GRLineTo( &panel->m_ClipBox,
            DC,
            Struct->m_Size.x + xx,
            Struct->m_Size.y + yy,
            width,
            g_GhostColor );
        break;
    }

    case DRAW_JUNCTION_STRUCT_TYPE:
    {
        DrawJunctionStruct* Struct;
        Struct = (DrawJunctionStruct*) DrawStruct;
        Width  = DRAWJUNCTION_SIZE;
        GRFilledRect( &panel->m_ClipBox,
            DC,
            Struct->m_Pos.x - Width + dx,
            Struct->m_Pos.y - Width + dy,
            Struct->m_Pos.x + Width + dx,
            Struct->m_Pos.y + Width + dy,
            g_GhostColor,
            g_GhostColor );
        break;
    }

    case TYPE_SCH_TEXT:
    {
        SCH_TEXT* Struct;
        Struct = (SCH_TEXT*) DrawStruct;
        Struct->Draw( panel, DC, wxPoint( dx, dy ), DrawMode, g_GhostColor );
        break;
    }

    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    {
        SCH_LABEL* Struct;
        Struct = (SCH_LABEL*) DrawStruct;
        Struct->Draw( panel, DC, wxPoint( dx, dy ), DrawMode, g_GhostColor );
        break;
    }

    case DRAW_NOCONNECT_STRUCT_TYPE:
    {
        DrawNoConnectStruct* Struct;
        Struct = (DrawNoConnectStruct*) DrawStruct;
        Struct->Draw( panel, DC, wxPoint( dx, dy ), DrawMode, g_GhostColor );
        break;
    }

    case TYPE_SCH_COMPONENT:
    {
        EDA_LibComponentStruct* LibEntry;
        SCH_COMPONENT* Struct;
        Struct   = (SCH_COMPONENT*) DrawStruct;
        LibEntry = FindLibPart( Struct->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( LibEntry == NULL )
            break;
        DrawingLibInGhost( panel, DC, LibEntry, Struct, Struct->m_Pos.x + dx,
            Struct->m_Pos.y + dy,
            Struct->m_Multi, Struct->m_Convert,
            g_GhostColor, FALSE );
        break;
    }

    case DRAW_SHEET_STRUCT_TYPE:
    {
        DrawSheetStruct* Struct = (DrawSheetStruct*) DrawStruct;
        GRRect( &panel->m_ClipBox, DC, Struct->m_Pos.x + dx, Struct->m_Pos.y + dy,
            Struct->m_Pos.x + Struct->m_Size.x + dx,
            Struct->m_Pos.y + Struct->m_Size.y + dy, width, g_GhostColor );
        break;
    }

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    case DRAW_MARKER_STRUCT_TYPE:
        break;

    default:
        break;
    }
}


/************************************************************/
void Draw_Marqueur( WinEDA_DrawPanel* panel, wxDC* DC,
                    wxPoint pos, char* pt_bitmap, int DrawMode, int Color )
/************************************************************/

/*
  * Place un repere sur l'ecran au point de coordonnees PCB pos_X, pos_Y
  * Le marqueur est defini par un tableau de 2 + (lig*col) elements:
  *  1er element: dim nbre ligne
  *  2er element: dim nbre col
  *  suite: lig * col elements a 0 ou 1 : si 1 mise a color du pixel
 *
  * copie la description du marqueur en current_marqueur (global)
 */
{
    int  px, py, color;
    char ii, ii_max, jj, jj_max;

    if( pt_bitmap == NULL )
        pt_bitmap = marq_bitmap;

    px = GRMapX( pos.x ); py = GRMapY( pos.y );

    /* Lecture des dimensions */
    ii_max = *(pt_bitmap++); jj_max = *(pt_bitmap++);

    /* lecture des offsets */
    px += *(pt_bitmap++); py += *(pt_bitmap++);

    color = *(pt_bitmap++);
    if( (Color > 0) )
        color = Color;
    if( color < 0 )
        color = 0;
    GRSetDrawMode( DC, DrawMode );

    /* Trace du bitmap */
    for( ii = 0; ii < ii_max; ii++ )
    {
        for( jj = 0; jj < jj_max; jj++, pt_bitmap++ )
        {
            if( *pt_bitmap )
                GRSPutPixel( &panel->m_ClipBox, DC, px + ii, py + jj, color );
        }
    }
}
