/*******************************/
/* class_pad_draw_function.cpp */
/*******************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "pcbnew_id.h"             // ID_TRACK_BUTT
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

/* uncomment this line to show this pad with its specfic size and color
 * when it is not on copper layers, and only one solder mask layer or solder paste layer
 * is displayed for this pad
 * After testing this feature,I am not sure this is a good idea
 * but the code is left here.
 */
//#define SHOW_PADMASK_REAL_SIZE_AND_COLOR

/** Draw a pad:
 *  @param  DC = device context
 *  @param offset = draw offset
 *  @param draw_mode = mode: GR_OR, GR_XOR, GR_AND...
 */
void D_PAD::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode,
                  const wxPoint& offset )
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
    wxSize  mask_margin;  // margin (clearance) used for some non copper layers
    int showActualMaskSize = 0;     /* == layer number if the actual pad size on mask layer can be displayed
                                      * i.e. if only one layer is shown for this pad
                                      * and this layer is a mask (solder mask or sloder paste
                                      */

    if( m_Flags & DO_NOT_DRAW )
        return;

    /* We can show/hide pads from the layer manager.
    * options are show/hide pads on front and/or back side of the board
    * For through pads, we hide them only if both sides are hidden.
    * smd pads on back are hidden for all layers (copper and technical layers)
    * on back side of the board
    * smd pads on front are hidden for all layers (copper and technical layers)
    * on front side of the board
    * ECO, edge and Draw layers and not considered
    */

    // Mask layers for Back side of board
    #define BACK_SIDE_LAYERS \
    (LAYER_BACK | ADHESIVE_LAYER_BACK | SOLDERPASTE_LAYER_BACK\
    | SILKSCREEN_LAYER_BACK | SOLDERMASK_LAYER_BACK)

    // Mask layers for Front side of board
    #define FRONT_SIDE_LAYERS \
    (LAYER_FRONT | ADHESIVE_LAYER_FRONT | SOLDERPASTE_LAYER_FRONT\
    | SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_FRONT)

    BOARD * brd =  GetBoard( );
    bool frontVisible = brd->IsElementVisible( PCB_VISIBLE(PAD_FR_VISIBLE) );
    bool backVisible = brd->IsElementVisible( PCB_VISIBLE(PAD_BK_VISIBLE) );

    if( !frontVisible && !backVisible )
        return;

    /* If pad are only on front side (no layer on back side)
     * and if hide front side pads is enabled, do not draw
    */
    if( !frontVisible && ( (m_Masque_Layer & BACK_SIDE_LAYERS) == 0 ) )
        return;

    /* If pad are only on back side (no layer on front side)
     * and if hide back side pads is enabled, do not draw
    */
    if( !backVisible && ( (m_Masque_Layer & FRONT_SIDE_LAYERS) == 0 ) )
        return;


    WinEDA_BasePcbFrame* frame  = (WinEDA_BasePcbFrame*) panel->GetParent();
    PCB_SCREEN*          screen = frame->GetScreen();
    if( frame->m_DisplayPadFill == FILLED )
        fillpad = 1;

#if defined(PCBNEW) || defined(__WXMAC__)
    if( m_Flags & IS_MOVED || !DisplayOpt.DisplayPadFill )
        fillpad = 0;
#endif

    if( m_Masque_Layer & LAYER_FRONT )
    {
            color = brd->GetVisibleElementColor(PAD_FR_VISIBLE);
    }

    if( m_Masque_Layer & LAYER_BACK )
    {
        color |= brd->GetVisibleElementColor(PAD_BK_VISIBLE);
    }

    if( color == 0 ) /* Not on copper layer */
    {
        // If the pad in on only one tech layer, use the layer color
        // else use DARKGRAY
        int mask_non_copper_layers = m_Masque_Layer & ~ALL_CU_LAYERS;
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
        mask_non_copper_layers &= brd->GetVisibleLayers();
#endif
        switch( mask_non_copper_layers )
        {
        case 0:
            break;

        case ADHESIVE_LAYER_BACK:
            color = brd->GetLayerColor(ADHESIVE_N_BACK);
            break;

        case ADHESIVE_LAYER_FRONT:
            color = brd->GetLayerColor(ADHESIVE_N_FRONT);
            break;

        case SOLDERPASTE_LAYER_BACK:
            color = brd->GetLayerColor(SOLDERPASTE_N_BACK);
            showActualMaskSize = SOLDERPASTE_N_BACK;
            break;

        case SOLDERPASTE_LAYER_FRONT:
            color = brd->GetLayerColor(SOLDERPASTE_N_FRONT);
            showActualMaskSize = SOLDERPASTE_N_FRONT;
            break;

        case SILKSCREEN_LAYER_BACK:
            color = brd->GetLayerColor(SILKSCREEN_N_BACK);
            break;

        case SILKSCREEN_LAYER_FRONT:
            color = brd->GetLayerColor(SILKSCREEN_N_FRONT);
            break;

        case SOLDERMASK_LAYER_BACK:
            color = brd->GetLayerColor(SOLDERMASK_N_BACK);
            showActualMaskSize = SOLDERMASK_N_BACK;
            break;

        case SOLDERMASK_LAYER_FRONT:
            color = brd->GetLayerColor(SOLDERMASK_N_FRONT);
            showActualMaskSize = SOLDERMASK_N_FRONT;
            break;

        case DRAW_LAYER:
            color = brd->GetLayerColor(DRAW_N);
            break;

        case COMMENT_LAYER:
            color = brd->GetLayerColor(COMMENT_N);
            break;

        case ECO1_LAYER:
            color = brd->GetLayerColor(ECO1_N);
            break;

        case ECO2_LAYER:
            color = brd->GetLayerColor(ECO2_N);
            break;

        case EDGE_LAYER:
            color = brd->GetLayerColor(EDGE_N);
            break;

        default:
            color = DARKGRAY;
            break;
        }
    }

    // if PAD_SMD pad and high contrast mode
    if( ( m_Attribut == PAD_SMD || m_Attribut == PAD_CONN )
       && DisplayOpt.ContrastModeDisplay )
    {
        // when routing tracks
        if( frame && frame->m_ID_current_state == ID_TRACK_BUTT )
        {
            int routeTop = screen->m_Route_Layer_TOP;
            int routeBot = screen->m_Route_Layer_BOTTOM;

            // if routing between copper and component layers,
            // or the current layer is one of said 2 external copper layers,
            // then highlight only the current layer.
            if( ( ( 1 << routeTop ) | ( 1 << routeBot ) )
               == ( LAYER_BACK | LAYER_FRONT )
               || ( ( 1 << screen->m_Active_Layer )
                   & ( LAYER_BACK | LAYER_FRONT ) ) )
            {
                if( !IsOnLayer( screen->m_Active_Layer ) )
                {
                    color &= ~MASKCOLOR;
                    color |= DARKDARKGRAY;
                }
            }
            // else routing between an internal signal layer and some other
            // layer.  Grey out all PAD_SMD pads not on current or the single
            // selected external layer.
            else if( !IsOnLayer( screen->m_Active_Layer )
                    && !IsOnLayer( routeTop )
                    && !IsOnLayer( routeBot ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }
        // when not edting tracks, show PAD_SMD components not on active layer
        // as greyed out
        else
        {
            if( !IsOnLayer( screen->m_Active_Layer ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }
    }

#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
    if( showActualMaskSize )
    {
        switch( showActualMaskSize )
        {
        case SOLDERMASK_N_BACK:
        case SOLDERMASK_N_FRONT:
            mask_margin.x = mask_margin.y = GetSolderMaskMargin();
            break;

        case SOLDERPASTE_N_BACK:
        case SOLDERPASTE_N_FRONT:
            mask_margin = GetSolderPasteMargin();
            break;

        default:
            break;
        }
    }
#endif

    // if Contrast mode is ON and a technical layer active, show pads on this
    // layer so we can see pads on paste or solder layer and the size of the
    // mask
    if( DisplayOpt.ContrastModeDisplay && screen->m_Active_Layer > LAST_COPPER_LAYER )
    {
        if( IsOnLayer( screen->m_Active_Layer ) )
        {
            color = brd->GetLayerColor(screen->m_Active_Layer);

            // In hight contrast mode, and if the active layer is the mask
            // layer shows the pad size with the mask clearance
            switch( screen->m_Active_Layer )
            {
            case SOLDERMASK_N_BACK:
            case SOLDERMASK_N_FRONT:
                mask_margin.x = mask_margin.y = GetSolderMaskMargin();
                break;

            case SOLDERPASTE_N_BACK:
            case SOLDERPASTE_N_FRONT:
                mask_margin = GetSolderPasteMargin();
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
     *  for Cvpcb (and Gerbview) GetClearance() creates debug errors because
     *  there is no net classes so a call to GetClearance() is made only when
     *   needed (never needed in Cvpcb nor in Gerbview)
     */
    int padClearance = DisplayIsol ? GetClearance() : 0;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
        if( fillpad )
            GRFilledCircle( &panel->m_ClipBox, DC, xc, yc,
                            dx + mask_margin.x, 0, color, color );
        else
            GRCircle( &panel->m_ClipBox, DC, xc, yc, dx + mask_margin.x,
                      m_PadSketchModePenSize, color );

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
        if( dx > dy )       /* horizontal */
        {
            delta_cx = dx - dy;
            delta_cy = 0;
            rotdx    = m_Size.y + ( mask_margin.y * 2 );
        }
        else                /* vertical */
        {
            delta_cx = 0;
            delta_cy = dy - dx;
            rotdx    = m_Size.x + ( mask_margin.x * 2 );
        }
        RotatePoint( &delta_cx, &delta_cy, angle );

        if( fillpad )
        {
            GRFillCSegm( &panel->m_ClipBox, DC,
                         ux0 + delta_cx, uy0 + delta_cy,
                         ux0 - delta_cx, uy0 - delta_cy,
                         rotdx, color );
        }
        else
        {
            GRCSegm( &panel->m_ClipBox, DC,
                     ux0 + delta_cx, uy0 + delta_cy,
                     ux0 - delta_cx, uy0 - delta_cy,
                     rotdx, m_PadSketchModePenSize, color );
        }

        /* Draw the isolation line. */
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
        ddx = ( m_DeltaSize.x >> 1 );
        ddy = ( m_DeltaSize.y >> 1 );

        coord[0].x = -dx - ddy - mask_margin.x;     // lower left
        coord[0].y = +dy + ddx + mask_margin.y;

        coord[1].x = -dx + ddy - mask_margin.x;     // upper left
        coord[1].y = -dy - ddx - mask_margin.y;

        coord[2].x = +dx - ddy + mask_margin.x;     // upper right
        coord[2].y = -dy + ddx - mask_margin.y;

        coord[3].x = +dx + ddy + mask_margin.x;     // lower right
        coord[3].y = +dy - ddx + mask_margin.y;

        for( ii = 0; ii < 4; ii++ )
        {
            RotatePoint( &coord[ii].x, &coord[ii].y, angle );
            coord[ii].x = coord[ii].x + ux0;
            coord[ii].y = coord[ii].y + uy0;
        }

        GRClosedPoly( &panel->m_ClipBox, DC, 4, coord, fillpad,
            fillpad ? 0 : m_PadSketchModePenSize, color, color );

        if( DisplayIsol )
        {
            dx += padClearance;
            dy += padClearance;

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
            color = BLACK;  // or DARKGRAY;

        if( draw_mode != GR_XOR )
            GRSetDrawMode( DC, GR_COPY );
        else
            GRSetDrawMode( DC, GR_XOR );

        switch( m_DrillShape )
        {
        case PAD_CIRCLE:

#ifdef USE_WX_ZOOM
            if( DC->LogicalToDeviceXRel( hole ) > 1 )
#else
            if( screen->Scale( hole ) > 1 ) /* draw hole if its size is enough */
#endif
                GRFilledCircle( &panel->m_ClipBox, DC, cx0, cy0, hole, 0,
                                color, color );
            break;

        case PAD_OVAL:
            dx = m_Drill.x >> 1;
            dy = m_Drill.y >> 1;

            if( m_Drill.x > m_Drill.y )  /* horizontal */
            {
                delta_cx = dx - dy;
                delta_cy = 0;
                rotdx    = m_Drill.y;
            }
            else                         /* vertical */
            {
                delta_cx = 0;
                delta_cy = dy - dx;
                rotdx    = m_Drill.x;
            }
            RotatePoint( &delta_cx, &delta_cy, angle );

            GRFillCSegm( &panel->m_ClipBox, DC, cx0 + delta_cx, cy0 + delta_cy,
                         cx0 - delta_cx, cy0 - delta_cy, rotdx, color );
            break;

        default:
            break;
        }

        if( screen->m_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }

    GRSetDrawMode( DC, draw_mode );

    /* Draw "No connect" ( / or \ or cross X ) if necessary. : */
    if( m_Netname.IsEmpty() && brd->IsElementVisible( PCB_VISIBLE(NO_CONNECTS_VISIBLE) ) )
    {
        dx0 = MIN( dx0, dy0 );
        int nc_color = BLUE;

        if( m_Masque_Layer & LAYER_FRONT )    /* Draw \ */
            GRLine( &panel->m_ClipBox, DC, cx0 - dx0, cy0 - dx0,
                    cx0 + dx0, cy0 + dx0, 0, nc_color );

        if( m_Masque_Layer & LAYER_BACK ) /* Draw / */
            GRLine( &panel->m_ClipBox, DC, cx0 + dx0, cy0 - dx0,
                    cx0 - dx0, cy0 + dx0, 0, nc_color );
    }

    /* Draw the pad number */
    bool display_padnum = true;
    if( frame && !frame->m_DisplayPadNum )
        display_padnum = false;

    bool display_netname = true;
    if( ( DisplayOpt.DisplayNetNamesMode == 0 )
       || ( DisplayOpt.DisplayNetNamesMode == 2 ) )
        display_netname = false;

    if( !display_padnum && !display_netname )
        return;

    wxPoint tpos0 = wxPoint( ux0, uy0 );    // Position of the centre of text
    wxPoint tpos  = tpos0;
    wxSize  AreaSize;                       // size of text area, normalized to
                                            // AreaSize.y < AreaSize.x
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

    if( shortname_len > 0 )             // if there is a netname, provides room
                                        // to display this netname
    {
        AreaSize.y /= 2;                // Text used only the upper area of the
                                        // pad. The lower area displays the net
                                        // name
        tpos.y     -= AreaSize.y / 2;
    }

    // Calculate the position of text, that is the middle point of the upper
    // area of the pad
    RotatePoint( &tpos, wxPoint( ux0, uy0 ), angle );

    /* Draw text with an angle between -90 deg and + 90 deg */
    int t_angle = angle;
    NORMALIZE_ANGLE_90( t_angle );

    /* Note: in next calculations, texte size is calculated for 3 or more
     * chars.  Of course, pads numbers and nets names can have less than 3
     * chars. but after some tries, i found this is gives the best look
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

#ifdef USE_WX_ZOOM
        if( DC->LogicalToDeviceXRel( tsize ) >= CHAR_SIZE_MIN ) // Not drawable when size too small.
#else
        if( screen->Scale( tsize ) >= CHAR_SIZE_MIN )   // Not drawable when size too small.
#endif
        {
            tsize = (int) ( tsize * 0.8 );              // reserve room for
                                                        // marges and segments
                                                        // thickness

            DrawGraphicText( panel, DC, tpos, WHITE, buffer, t_angle,
                             wxSize( tsize, tsize ), GR_TEXT_HJUSTIFY_CENTER,
                             GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false, false,
                             false );
        }
    }

    // display the short netname, if exists
    if( shortname_len == 0 )
        return;

    shortname_len = MAX( shortname_len, MIN_CHAR_COUNT );
    tsize = min( AreaSize.y, AreaSize.x / shortname_len );

#ifdef USE_WX_ZOOM
    if( DC->LogicalToDeviceXRel( tsize ) >= CHAR_SIZE_MIN )  // Not drawable in size too small.
#else
    if( screen->Scale( tsize ) >= CHAR_SIZE_MIN )   // Not drawable in size too small.
#endif
    {
        if( !( !IsOnLayer( screen->m_Active_Layer )
               && DisplayOpt.ContrastModeDisplay ) )
        {
            tpos = tpos0;
            if( display_padnum )
                tpos.y += AreaSize.y / 2;
            RotatePoint( &tpos, wxPoint( ux0, uy0 ), angle );

            tsize = (int) ( tsize * 0.8 );   // reserve room for marges and
                                             // segments thickness
            DrawGraphicText( panel, DC, tpos, WHITE, m_ShortNetname, t_angle,
                             wxSize( tsize, tsize ), GR_TEXT_HJUSTIFY_CENTER,
                             GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false, false );
        }
    }
}
