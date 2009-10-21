/*******************************************************/
/* class_pad_draw_function.cpp : functionsto draw pads */
/*******************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "pcbnew_id.h"             // ID_TRACK_BUTT
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "pcbnew.h"


/*******************************************************************************************/
void D_PAD::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode,
                    const wxPoint& offset )
/*******************************************************************************************/

/** Draw a pad:
 *  @param  DC = device context
 *  @param offset = draw offset
 *  @param draw_mode = mode: GR_OR, GR_XOR, GR_AND...
 */
{
    int ii;
    int color = 0;
    int ux0, uy0,
        dx, dx0, dy, dy0,
        rotdx,
        delta_cx, delta_cy,
        xc, yc;
    int     angle;
    wxPoint coord[4];
    int     fillpad = 0;
    wxPoint shape_pos;
    int     mask_margin = 0;    // margin (clearance) used for some non copper layers

    if( m_Flags & DO_NOT_DRAW )
        return;

    WinEDA_BasePcbFrame* frame  = (WinEDA_BasePcbFrame*) panel->m_Parent;
    PCB_SCREEN*          screen = frame->GetScreen();
    if( frame->m_DisplayPadFill == FILLED )
        fillpad = 1;

#if defined(PCBNEW) || defined(__WXMAC__)
    if( m_Flags & IS_MOVED || !DisplayOpt.DisplayPadFill )
        fillpad = 0;
#endif

    if( m_Masque_Layer & CMP_LAYER )
        color = g_PadCMPColor;

    if( m_Masque_Layer & CUIVRE_LAYER )
        color |= g_PadCUColor;

    if( color == 0 ) /* Not on copper layer */
    {
        // If the pad in on only one tech layer, use the layer color
        // else use DARKGRAY
        switch( m_Masque_Layer & ~ALL_CU_LAYERS )
        {
        case ADHESIVE_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[ADHESIVE_N_CU];
            break;

        case ADHESIVE_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[ADHESIVE_N_CMP];
            break;

        case SOLDERPASTE_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SOLDERPASTE_N_CU];
            break;

        case SOLDERPASTE_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SOLDERPASTE_N_CMP];
            break;

        case SILKSCREEN_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SILKSCREEN_N_CU];
            break;

        case SILKSCREEN_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SILKSCREEN_N_CMP];
            break;

        case SOLDERMASK_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SOLDERMASK_N_CU];
            break;

        case SOLDERMASK_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SOLDERMASK_N_CMP];
            break;

        case DRAW_LAYER:
            color = g_DesignSettings.m_LayerColor[DRAW_N];
            break;

        case COMMENT_LAYER:
            color = g_DesignSettings.m_LayerColor[COMMENT_N];
            break;

        case ECO1_LAYER:
            color = g_DesignSettings.m_LayerColor[ECO1_N];
            break;

        case ECO2_LAYER:
            color = g_DesignSettings.m_LayerColor[ECO2_N];
            break;

        case EDGE_LAYER:
            color = g_DesignSettings.m_LayerColor[EDGE_N];
            break;

        default:
            color = DARKGRAY;
            break;
        }
    }


    // if PAD_SMD pad and high contrast mode
    if( (m_Attribut==PAD_SMD || m_Attribut==PAD_CONN) && DisplayOpt.ContrastModeDisplay )
    {
        // when routing tracks
        if( frame && frame->m_ID_current_state == ID_TRACK_BUTT )
        {
            int routeTop = screen->m_Route_Layer_TOP;
            int routeBot = screen->m_Route_Layer_BOTTOM;

            // if routing between copper and component layers,
            // or the current layer is one of said 2 external copper layers,
            // then highlight only the current layer.
            if( ( (1 << routeTop) | (1 << routeBot) ) == (CUIVRE_LAYER | CMP_LAYER)
               || ( (1 << screen->m_Active_Layer) & (CUIVRE_LAYER | CMP_LAYER) ) )
            {
                if( !IsOnLayer( screen->m_Active_Layer ) )
                {
                    color &= ~MASKCOLOR;
                    color |= DARKDARKGRAY;
                }
            }
            // else routing between an internal signal layer and some other layer.
            // grey out all PAD_SMD pads not on current or the single selected
            // external layer.
            else if( !IsOnLayer( screen->m_Active_Layer )
                    && !IsOnLayer( routeTop )
                    && !IsOnLayer( routeBot ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }
        // when not edting tracks, show PAD_SMD components not on active layer as greyed out
        else
        {
            if( !IsOnLayer( screen->m_Active_Layer ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }
    }

    // if Contrast mode is ON and a technical layer active, show pads on this layer
    // so we can see pads on paste or solder layer
    if( DisplayOpt.ContrastModeDisplay && screen->m_Active_Layer > LAST_COPPER_LAYER )
    {
        if( IsOnLayer( screen->m_Active_Layer ) )
        {
            color = g_DesignSettings.m_LayerColor[screen->m_Active_Layer];
            // In hight contrast mode, and if the active layer is the mask layer
            // shows the pad size with the mask clearance
            switch( screen->m_Active_Layer )
            {
            case SOLDERMASK_N_CU:
            case SOLDERMASK_N_CMP:
                mask_margin = g_DesignSettings.m_MaskMargin;
                break;

            default:
                break;
            }
        }
        else
            color = DARKDARKGRAY;
    }


    if( draw_mode & GR_SURBRILL )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHT_LIGHT_FLAG;
        else
            color |= HIGHT_LIGHT_FLAG;
    }

    if( color & HIGHT_LIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    GRSetDrawMode( DC, draw_mode );  /* mode de trace */

    /* calcul du centre des pads en coordonnees Ecran : */
    shape_pos = ReturnShapePos();
    ux0 = shape_pos.x - offset.x;
    uy0 = shape_pos.y - offset.y;
    xc  = ux0;
    yc  = uy0;

    dx = dx0 = m_Size.x >> 1;
    dy = dy0 = m_Size.y >> 1; /* demi dim  dx et dy */

    angle = m_Orient;

    bool DisplayIsol = DisplayOpt.DisplayPadIsol;
    if( ( m_Masque_Layer & ALL_CU_LAYERS ) == 0 )
        DisplayIsol = FALSE;

    SetAlpha( &color, 170 );

    /* Get the pad clearance. This has a meaning only for Pcbnew.
     *  for Cvpcb (and Gerbview) GetClearance() creates debug errors because there is no
     *  net classes so a call to GetClearance() is made only when needed
     *  (never needed in Cvpcb nor in Gerbview)
     */
    int padClearance = DisplayIsol ? GetClearance() : 0;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
        if( fillpad )
            GRFilledCircle( &panel->m_ClipBox, DC, xc, yc, dx + mask_margin, 0, color, color );
        else
            GRCircle( &panel->m_ClipBox, DC, xc, yc, dx + mask_margin, 0, color );

        if( DisplayIsol )
        {
            GRCircle( &panel->m_ClipBox,
                      DC,
                      xc,
                      yc,
                      dx + padClearance,
                      0,
                      color );
        }
        break;

    case PAD_OVAL:
        /* calcul de l'entraxe de l'ellipse */
        if( dx > dy )       /* ellipse horizontale */
        {
            delta_cx = dx - dy;
            delta_cy = 0;
            rotdx    = m_Size.y;
        }
        else                /* ellipse verticale */
        {
            delta_cx = 0;
            delta_cy = dy - dx;
            rotdx    = m_Size.x;
        }
        RotatePoint( &delta_cx, &delta_cy, angle );

        if( fillpad )
        {
            GRFillCSegm( &panel->m_ClipBox, DC,
                         ux0 + delta_cx + mask_margin, uy0 + delta_cy + mask_margin,
                         ux0 - delta_cx - mask_margin, uy0 - delta_cy - mask_margin,
                         rotdx, color );
        }
        else
        {
            GRCSegm( &panel->m_ClipBox, DC,
                     ux0 + delta_cx + mask_margin, uy0 + delta_cy + mask_margin,
                     ux0 - delta_cx - mask_margin, uy0 - delta_cy - mask_margin,
                     rotdx, color );
        }

        /* Trace de la marge d'isolement */
        if( DisplayIsol )
        {
            rotdx = rotdx + 2 * padClearance;

            GRCSegm( &panel->m_ClipBox, DC, ux0 + delta_cx, uy0 + delta_cy,
                     ux0 - delta_cx, uy0 - delta_cy,
                     rotdx, color );
        }
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
    {
        int ddx, ddy;
        ddx = (m_DeltaSize.x >> 1) + mask_margin;
        ddy = (m_DeltaSize.y >> 1) + mask_margin;      /* demi dim  dx et dy */

        coord[0].x = -dx - ddy;
        coord[0].y = +dy + ddx;

        coord[1].x = -dx + ddy;
        coord[1].y = -dy - ddx;

        coord[2].x = +dx - ddy;
        coord[2].y = -dy + ddx;

        coord[3].x = +dx + ddy;
        coord[3].y = +dy - ddx;

        for( ii = 0; ii < 4; ii++ )
        {
            RotatePoint( &coord[ii].x, &coord[ii].y, angle );
            coord[ii].x = coord[ii].x + ux0;
            coord[ii].y = coord[ii].y + uy0;
        }

        GRClosedPoly( &panel->m_ClipBox, DC, 4, coord, fillpad, color, color );

        if( DisplayIsol )
        {
            dx += padClearance - mask_margin;
            dy += padClearance - mask_margin;

            coord[0].x = -dx - ddy;
            coord[0].y = dy + ddx;

            coord[1].x = -dx + ddy;
            coord[1].y = -dy - ddx;

            coord[2].x = dx - ddy;
            coord[2].y = -dy + ddx;

            coord[3].x = dx + ddy;
            coord[3].y = dy - ddx;

            for( ii = 0; ii < 4; ii++ )
            {
                RotatePoint( &coord[ii].x, &coord[ii].y, angle );
                coord[ii].x = coord[ii].x + ux0;
                coord[ii].y = coord[ii].y + uy0;
            }

            GRClosedPoly( &panel->m_ClipBox, DC, 4, coord, 0, color, color );
        }
    }
    break;


    default:
        break;
    }

    /* Draw the pad hole */
    int cx0  = m_Pos.x - offset.x;
    int cy0  = m_Pos.y - offset.y;
    int hole = m_Drill.x >> 1;

    if( fillpad && hole )
    {
        bool blackpenstate = false;
        if( screen->m_IsPrinting )
        {
            blackpenstate = GetGRForceBlackPenState();
            GRForceBlackPen( false );
            color = g_DrawBgColor;
        }
        else
            color = BLACK; // or DARKGRAY;

        if( draw_mode != GR_XOR )
            GRSetDrawMode( DC, GR_COPY );
        else
            GRSetDrawMode( DC, GR_XOR );

        switch( m_DrillShape )
        {
        case PAD_CIRCLE:
            if( screen->Scale( hole ) > 1 ) /* draw hole if its size is enought */
                GRFilledCircle( &panel->m_ClipBox, DC, cx0, cy0, hole, 0, color, color );
            break;

        case PAD_OVAL:
            dx = m_Drill.x >> 1;
            dy = m_Drill.y >> 1;            /* demi dim  dx et dy */

            /* calcul de l'entraxe de l'ellipse */
            if( m_Drill.x > m_Drill.y )     /* ellipse horizontale */
            {
                delta_cx = dx - dy; delta_cy = 0;
                rotdx    = m_Drill.y;
            }
            else                /* ellipse verticale */
            {
                delta_cx = 0; delta_cy = dy - dx;
                rotdx    = m_Drill.x;
            }
            RotatePoint( &delta_cx, &delta_cy, angle );

            GRFillCSegm( &panel->m_ClipBox, DC, cx0 + delta_cx, cy0 + delta_cy,
                         cx0 - delta_cx, cy0 - delta_cy,
                         rotdx, color );
            break;

        default:
            break;
        }

        if( screen->m_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }

    GRSetDrawMode( DC, draw_mode );

    /* Trace du symbole "No connect" ( / ou \ ou croix en X) si necessaire : */
    if( m_Netname.IsEmpty() && DisplayOpt.DisplayPadNoConn )
    {
        dx0 = MIN( dx0, dy0 );
        int nc_color = BLUE;

        if( m_Masque_Layer & CMP_LAYER ) /* Trace forme \ */
            GRLine( &panel->m_ClipBox, DC, cx0 - dx0, cy0 - dx0,
                    cx0 + dx0, cy0 + dx0, 0, nc_color );

        if( m_Masque_Layer & CUIVRE_LAYER ) /* Trace forme / */
            GRLine( &panel->m_ClipBox, DC, cx0 + dx0, cy0 - dx0,
                    cx0 - dx0, cy0 + dx0, 0, nc_color );
    }

    /* Draw the pad number */
    bool display_padnum = true;
    if( frame && !frame->m_DisplayPadNum )
        display_padnum = false;

    bool display_netname = true;
    if( (DisplayOpt.DisplayNetNamesMode == 0) || (DisplayOpt.DisplayNetNamesMode == 2) )
        display_netname = false;

    if( !display_padnum && !display_netname )
        return;

    wxPoint tpos0 = wxPoint( ux0, uy0 );        // Position of the centre of text
    wxPoint tpos  = tpos0;
    wxSize  AreaSize;                           // size of text area, normalized to AreaSize.y < AreaSize.x
    int     shortname_len = m_ShortNetname.Len();
    if( !display_netname )
        shortname_len = 0;
    if( GetShape() == PAD_CIRCLE )
        angle = 0;
    AreaSize = m_Size;
    if( m_Size.y > m_Size.x )
    {
        angle += 900;
        AreaSize.x = m_Size.y;
        AreaSize.y = m_Size.x;
    }

    if( shortname_len > 0 )             // if there is a netname, provides room to display this netname
    {
        AreaSize.y /= 2;                // Text used only the upper area of the pad. The lower area displays the net name
        tpos.y     -= AreaSize.y / 2;
    }

    // Calculate the position of text, that is the middle point of the upper area of the pad
    RotatePoint( &tpos, wxPoint( ux0, uy0 ), angle );

    /* Draw text with an angle between -90 deg and + 90 deg */
    int t_angle = angle;
    NORMALIZE_ANGLE_90( t_angle );

    /* Note: in next calculations, texte size is calculated for 3 or more chars.
     *  Of course, pads numbers and nets names can have less than 3 chars.
     *  but after some tries, i found this is gives the best look
     */
    #define MIN_CHAR_COUNT 3
    wxString buffer;

    int      tsize;
    if( display_padnum )
    {
        ReturnStringPadName( buffer );
        int numpad_len = buffer.Len();
        numpad_len = MAX( numpad_len, MIN_CHAR_COUNT );

        tsize = min( AreaSize.y, AreaSize.x / numpad_len );
        #define CHAR_SIZE_MIN 5
        if( screen->Scale( tsize ) >= CHAR_SIZE_MIN )       // Not drawable when size too small.
        {
            tsize = (int) ( tsize * 0.8 );                  // reserve room for marges and segments thickness

            DrawGraphicText( panel, DC, tpos,
                             WHITE, buffer, t_angle, wxSize( tsize,
                                                             tsize ),
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false,
                             false, false );
        }
    }

    // display the short netname, if exists
    if( shortname_len == 0 )
        return;

    shortname_len = MAX( shortname_len, MIN_CHAR_COUNT );
    tsize = min( AreaSize.y, AreaSize.x / shortname_len );

    if( screen->Scale( tsize ) >= CHAR_SIZE_MIN )   // Not drawable in size too small.
    {
        if( !(!IsOnLayer( screen->m_Active_Layer )&& DisplayOpt.ContrastModeDisplay) )
        {
            tpos = tpos0;
            if( display_padnum )
                tpos.y += AreaSize.y / 2;
            RotatePoint( &tpos, wxPoint( ux0, uy0 ), angle );

            tsize = (int) ( tsize * 0.8 );   // reserve room for marges and segments thickness
            DrawGraphicText( panel, DC, tpos,
                             WHITE, m_ShortNetname, t_angle, wxSize( tsize, tsize ),
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7,
                             false, false );
        }
    }
}
