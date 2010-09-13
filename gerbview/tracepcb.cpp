/****************/
/* tracepcb.cpp */
/****************/

/*
 *  Redraw the screen.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

/***************/
/* tracepcb.cpp */
/***************/

static void Affiche_DCodes_Pistes( WinEDA_DrawPanel* panel, wxDC* DC,
                            BOARD* Pcb, int drawmode );

/** virtual Function PrintPage
 * Used to print the board (on printer, or when creating SVF files).
 * Print the board, but only layers allowed by aPrintMaskLayer
 * @param aDC = the print device context
 * @param aPrint_Sheet_Ref = true to print frame references
 * @param aPrint_Sheet_Ref = a 32 bits mask: bit n = 1 -> layer n is printed
 * @param aPrintMirrorMode = true to plot mirrored
 * @param aData = a pointer to an optional data (not used here: can be NULL)
 */
void WinEDA_GerberFrame::PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref, int aPrintMasklayer,
                                bool aPrintMirrorMode, void * aData )
{
    DISPLAY_OPTIONS save_opt;
    int DisplayPolygonsModeImg;

    save_opt = DisplayOpt;

    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode = 0;

    DisplayPolygonsModeImg = g_DisplayPolygonsModeSketch;
    g_DisplayPolygonsModeSketch = 0;

    DrawPanel->m_PrintIsMirrored = aPrintMirrorMode;

    Trace_Gerber( aDC, GR_COPY, aPrintMasklayer );

    if( aPrint_Sheet_Ref )
        TraceWorkSheet( aDC, GetScreen(), 0 );

    DrawPanel->m_PrintIsMirrored = false;

    DisplayOpt = save_opt;
    g_DisplayPolygonsModeSketch = DisplayPolygonsModeImg;
}


/*******************************************************************/
void WinEDA_GerberFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/
/* Redraws the full screen, including axis and grid
 */
{
    PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

    if( !GetBoard() )
        return;
    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    DrawPanel->DrawBackGround( DC );

    //buid mask layer :
    int masklayer = 0;
    for( int layer = 0; layer < 32; layer++ )
        if( GetBoard()->IsLayerVisible( layer ) )
            masklayer |= 1 << layer;

    Trace_Gerber( DC, GR_COPY, masklayer );
    TraceWorkSheet( DC, screen, 0 );

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    DrawPanel->DrawCursor( DC );
}

/********************************************************************/
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset )
/********************************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
// @todo: replace WinEDA_GerberFrame::Trace_Gerber() by this function
{
}


/***********************************************************************************/
void WinEDA_GerberFrame::Trace_Gerber( wxDC* aDC, int aDraw_mode, int aPrintMasklayer )
/***********************************************************************************/
/* Trace all elements of PCBs (i.e Spots, filled polygons or lines) on the active screen
* @param aDC = current device context
* @param aDraw_mode = draw mode for the device context (GR_COPY, GR_OR, GR_XOR ..)
* @param aPrintMasklayer = mask for allowed layer (=-1 to draw all layers)
*/
{
    if( !GetBoard() )
        return;

    bool    erase = false;
    int     color;
    bool    filled;
    int     layer = GetScreen()->m_Active_Layer;

    // Draw filled polygons
    std::vector<wxPoint>    points;

    // minimize reallocations of the vector's internal array by starting with a good sized one.
    points.reserve(10000);

    TRACK* next_track;
    for( TRACK* track = GetBoard()->m_Zone; track; track = next_track )
    {
        next_track = track->Next();

        if( !(track->ReturnMaskLayer() & aPrintMasklayer) )
            continue;

        if( (points.size() == 0) && (track->GetNet() == 0) )     // first point of a new polygon
        {
            erase = ( track->m_Flags & DRAW_ERASED );
            points.push_back( track->m_Start );
        }

        points.push_back( track->m_End );

        if( (next_track == NULL ) || (next_track->GetNet() == 0) )  // EndPoint of the current polygon
        {
            color = g_ColorsSettings.GetLayerColor( track->GetLayer() );
            filled = (g_DisplayPolygonsModeSketch == 0);
            if( erase )
            {
                color = g_DrawBgColor;
                filled = true;
            }

            GRClosedPoly( &DrawPanel->m_ClipBox, aDC, points.size(), &points[0],
                          filled, color, color );
            points.clear();
        }
    }

    // Draw tracks and flashes down here.
    // This will probably not be a final solution to drawing order issues
    GERBER*       gerber = g_GERBER_List[layer];
    int           dcode_hightlight = 0;

    if( gerber )
        dcode_hightlight = gerber->m_Selected_Tool;

    for( TRACK* track = GetBoard()->m_Track; track; track = track->Next() )
    {
        if( !(track->ReturnMaskLayer() & aPrintMasklayer) )
            continue;
        if( dcode_hightlight == track->GetNet() && track->GetLayer()==layer )
            Trace_Segment( GetBoard(), DrawPanel, aDC, track, aDraw_mode | GR_SURBRILL );
        else
            Trace_Segment( GetBoard(), DrawPanel, aDC, track, aDraw_mode );
    }

    if( IsElementVisible( DCODES_VISIBLE) )
        Affiche_DCodes_Pistes( DrawPanel, aDC, GetBoard(), GR_COPY );

    GetScreen()->ClrRefreshReq();
}

#if 1

/***********************************************************************************/
void Trace_Segment( BOARD* aBrd, WinEDA_DrawPanel* panel, wxDC* DC, TRACK* track, int draw_mode )
/***********************************************************************************/

/* Trace 1 segment of track (segment, spot...).
 *  Parameters :
 *  Track = a pointer on of the description of the track
 *  draw_mode = mode ( GR_XOR, GR_OR..)
 */
{
    int         l_piste;
    int         color;
    int         fillopt;
    int         radius;
    int         halfPenWidth;
    static bool show_err;

    if( track->m_Flags & DRAW_ERASED )   // draw in background color, used by classs TRACK in gerbview
    {
        color = g_DrawBgColor;
    }
    else
    {
        if( aBrd->IsLayerVisible( track->GetLayer() ) == false )
            return;

        color = aBrd->GetLayerColor( track->GetLayer() );

        if( draw_mode & GR_SURBRILL )
        {
            if( draw_mode & GR_AND )
                color &= ~HIGHT_LIGHT_FLAG;
            else
                color |= HIGHT_LIGHT_FLAG;
        }
        if( color & HIGHT_LIGHT_FLAG )
            color = ColorRefs[color & MASKCOLOR].m_LightColor;
    }

    GRSetDrawMode( DC, draw_mode );


    fillopt = DisplayOpt.DisplayPcbTrackFill ? FILLED : SKETCH;

    switch( track->m_Shape )
    {
    case S_CIRCLE:
        radius = (int) hypot( (double) (track->m_End.x - track->m_Start.x),
                              (double) (track->m_End.y - track->m_Start.y) );

        halfPenWidth = track->m_Width >> 1;

#ifdef USE_WX_ZOOM
        if( DC->LogicalToDeviceXRel( halfPenWidth ) < L_MIN_DESSIN )
#else
        if( panel->GetScreen()->Scale( halfPenWidth ) < L_MIN_DESSIN )
#endif
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x,
                      track->m_Start.y, radius, 0, color );
        }

        if( fillopt == SKETCH )
        {
            // draw the border of the pen's path using two circles, each as narrow as possible
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      radius - halfPenWidth, 0, color );
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      radius + halfPenWidth, 0, color );
        }
        else
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      radius, track->m_Width, color );
        }
        break;

    case S_ARC:
        if( fillopt == SKETCH )
        {
            GRArc1( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                    track->m_End.x, track->m_End.y,
                    track->m_Param, track->GetSubNet(), 0, color );
        }
        else
        {
            GRArc1( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                    track->m_End.x, track->m_End.y,
                    track->m_Param, track->GetSubNet(),
                    track->m_Width, color );
        }
        break;

    case S_SPOT_CIRCLE:
        radius = track->m_Width >> 1;

        fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;

#ifdef USE_WX_ZOOM
        if( DC->LogicalToDeviceXRel( radius ) < L_MIN_DESSIN )
#else
        if( panel->GetScreen()->Scale( radius ) < L_MIN_DESSIN )
#endif
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x,
                      track->m_Start.y, radius, 0, color );
        }
        else if( fillopt == SKETCH )
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x,
                      track->m_Start.y, radius, 0, color );
        }
        else
        {
            GRFilledCircle( &panel->m_ClipBox, DC, track->m_Start.x,
                            track->m_Start.y, radius, 0, color, color );
        }
        break;

    case S_SPOT_RECT:
    case S_RECT:

        l_piste = track->m_Width >> 1;

        fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;

#ifdef USE_WX_ZOOM
        if( DC->LogicalToDeviceXRel( l_piste ) < L_MIN_DESSIN )
#else
        if( panel->GetScreen()->Scale( l_piste ) < L_MIN_DESSIN )
#endif
        {
            GRLine( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                    track->m_End.x, track->m_End.y, 0, color );
        }
        else if( fillopt == SKETCH )
        {
            GRRect( &panel->m_ClipBox, DC,
                    track->m_Start.x - l_piste,
                    track->m_Start.y - l_piste,
                    track->m_End.x + l_piste,
                    track->m_End.y + l_piste,
                    0, color );
        }
        else
        {
            GRFilledRect( &panel->m_ClipBox, DC,
                          track->m_Start.x - l_piste,
                          track->m_Start.y - l_piste,
                          track->m_End.x + l_piste,
                          track->m_End.y + l_piste,
                          0, color, color );
        }
        break;

    case S_SPOT_OVALE:
        fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;

    case S_SEGMENT:
        l_piste = track->m_Width >> 1;

#ifdef USE_WX_ZOOM
        if( DC->LogicalToDeviceXRel( l_piste ) < L_MIN_DESSIN )
#else
        if( panel->GetScreen()->Scale( l_piste ) < L_MIN_DESSIN )
#endif
        {
            GRLine( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                    track->m_End.x, track->m_End.y, 0, color );
            break;
        }

        if( fillopt == SKETCH )
        {
            GRCSegm( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                     track->m_End.x, track->m_End.y, track->m_Width, color );
        }
        else
        {
            GRFillCSegm( &panel->m_ClipBox, DC, track->m_Start.x,
                         track->m_Start.y, track->m_End.x, track->m_End.y,
                         track->m_Width, color );
        }
        break;

    default:
        if( !show_err )
        {
            wxMessageBox( wxT( "Trace_Segment() type error" ) );
            show_err = TRUE;
        }
        break;
    }
}

#endif


/*****************************************************************************************/
void Affiche_DCodes_Pistes( WinEDA_DrawPanel* panel, wxDC* DC, BOARD* Pcb, int drawmode )
/*****************************************************************************************/
{
    TRACK*   track;
    wxPoint  pos;
    int      width, orient;
    wxString Line;

    GRSetDrawMode( DC, drawmode );
    track = Pcb->m_Track;
    for( ; track != NULL; track = track->Next() )
    {
        if( Pcb->IsLayerVisible( track->GetLayer() ) == false )
            continue;

        if( (track->m_Shape == S_ARC)
         || (track->m_Shape == S_CIRCLE)
         || (track->m_Shape == S_ARC_RECT) )
        {
            pos.x = track->m_Start.x;
            pos.y = track->m_Start.y;
        }
        else
        {
            pos.x = (track->m_Start.x + track->m_End.x) / 2;
            pos.y = (track->m_Start.y + track->m_End.y) / 2;
        }

        Line.Printf( wxT( "D%d" ), track->GetNet() );

        width  = track->m_Width;
        orient = TEXT_ORIENT_HORIZ;
        if( track->m_Shape >= S_SPOT_CIRCLE )   // forme flash
        {
            width /= 3;
        }
        else        // lines
        {
            int dx, dy;
            dx = track->m_Start.x - track->m_End.x;
            dy = track->m_Start.y - track->m_End.y;
            if( abs( dx ) < abs( dy ) )
                orient = TEXT_ORIENT_VERT;
            width /= 2;
        }

        int color = g_ColorsSettings.GetItemColor(DCODES_VISIBLE);

        DrawGraphicText( panel, DC,
                         pos, (EDA_Colors) color, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
			  0, false, false, false);
    }
}


/* Virtual fonction needed by the PCB_SCREEN class derived from BASE_SCREEN
* this is a virtual pure function in BASE_SCREEN
* do nothing in gerbview
* could be removed later
*/
void PCB_SCREEN::ClearUndoORRedoList(UNDO_REDO_CONTAINER&, int )
{
}
