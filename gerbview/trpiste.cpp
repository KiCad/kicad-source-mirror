/*****************************************************************/
/* Routines de tracage des pistes ( Toutes, 1 piste, 1 segment ) */
/*****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

/* Definition des cas ou l'on force l'affichage en SKETCH (membre .flags) */
#define FORCE_SKETCH (DRAG | EDIT )

/* variables locales : */

/***************************************************************************************************/
void Draw_Track_Buffer( WinEDA_DrawPanel* panel, wxDC* DC, BOARD* Pcb, int draw_mode,
                        int printmasklayer )
/***************************************************************************************************/

/* Function to draw the tracks (i.e Spots or lines) in gerbview
 *  Polygons are not handled here (there are in Pcb->m_Zone)
 * @param DC = device context to draw
 * @param Pcb = Board to draw (only Pcb->m_Track is used)
 * @param draw_mode = draw mode for the device context (GR_COPY, GR_OR, GR_XOR ..)
 * @param printmasklayer = mask for allowed layer (=-1 to draw all layers)
 */
{
    int           layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    GERBER*       gerber = g_GERBER_List[layer];
    int           dcode_hightlight = 0;

    if( gerber )
        dcode_hightlight = gerber->m_Selected_Tool;

    for( TRACK* track = Pcb->m_Track;  track;  track = track->Next() )
    {
        if( !(track->ReturnMaskLayer() & printmasklayer) )
            continue;

        if( dcode_hightlight == track->GetNet() && track->GetLayer()==layer )
            Trace_Segment( panel, DC, track, draw_mode | GR_SURBRILL );
        else
            Trace_Segment( panel, DC, track, draw_mode );
    }
}


#if 1

/***********************************************************************************/
void Trace_Segment( WinEDA_DrawPanel* panel, wxDC* DC, TRACK* track, int draw_mode )
/***********************************************************************************/

/* routine de trace de 1 segment de piste.
 *  Parametres :
 *  track = adresse de la description de la piste en buflib
 *  draw_mode = mode ( GR_XOR, GR_OR..)
 */
{
    int         l_piste;
    int         color;
    int         zoom;
    int         rayon;
    int         fillopt;
    static bool show_err;

    if( track->m_Flags & DRAW_ERASED )   // draw in background color, used by classs TRACK in gerbview
    {
        color = g_DrawBgColor;
    }
    else
    {
        color = g_DesignSettings.m_LayerColor[track->GetLayer()];
        if( color & ITEM_NOT_SHOW )
            return;

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

    zoom = panel->GetZoom();

    rayon = l_piste = track->m_Width >> 1;

    fillopt = DisplayOpt.DisplayPcbTrackFill ? FILLED : SKETCH;

    switch( track->m_Shape )
    {
    case S_CIRCLE:
        rayon = (int) hypot( (double) (track->m_End.x - track->m_Start.x),
                            (double) (track->m_End.y - track->m_Start.y) );
        if( (l_piste / zoom) < L_MIN_DESSIN )
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon, 0, color );
        }

        if( fillopt == SKETCH )
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon - l_piste, 0, color );
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon + l_piste, 0, color );
        }
        else
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon, track->m_Width, color );
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
        fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;
        if( (rayon / zoom) < L_MIN_DESSIN )
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon, 0, color );
        }
        else if( fillopt == SKETCH )
        {
            GRCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                      rayon, 0, color );
        }
        else
        {
            GRFilledCircle( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                            rayon, 0, color, color );
        }
        break;

    case S_SPOT_RECT:
    case S_RECT:
        fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;
        if( (l_piste / zoom) < L_MIN_DESSIN )
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
        if( (l_piste / zoom) < L_MIN_DESSIN )
        {
            GRLine( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                    track->m_End.x, track->m_End.y, 0, color );
            break;
        }

        if( fillopt == SKETCH )
        {
            GRCSegm( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                     track->m_End.x, track->m_End.y,
                     track->m_Width, color );
        }
        else
        {
            GRFillCSegm( &panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
                         track->m_End.x, track->m_End.y,
                         track->m_Width, color );
        }
        break;

    default:
        if( !show_err )
        {
            DisplayError( panel, wxT( "Trace_Segment() type error" ) );
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

        DrawGraphicText( panel, DC,
                         pos, (EDA_Colors) g_DCodesColor, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER );
    }
}
