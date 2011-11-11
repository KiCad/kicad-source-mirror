/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_pad_draw_functions.cpp
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "class_pcb_screen.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "layers_id_colors_and_visibility.h"
#include "wxBasePcbFrame.h"
#include "pcbcommon.h"
#include "macros.h"

#include "pcbnew_id.h"             // ID_TRACK_BUTT
#include "pcbnew.h"
#include "colors_selection.h"

#include "class_board.h"


/* uncomment this line to show this pad with its specfic size and color
 * when it is not on copper layers, and only one solder mask layer or solder paste layer
 * is displayed for this pad
 * After testing this feature,I am not sure this is a good idea
 * but the code is left here.
 */

//#define SHOW_PADMASK_REAL_SIZE_AND_COLOR


// Helper class to store parameters used to draw a pad
PAD_DRAWINFO::PAD_DRAWINFO()
{
    m_DrawPanel       = NULL;
    m_DrawMode        = 0;
    m_Color           = BLACK;
    m_HoleColor       = BLACK; // could be DARKGRAY;
    m_NPHoleColor     = YELLOW;
    m_PadClearance    = 0;
    m_Display_padnum  = true;
    m_Display_netname = true;
    m_ShowPadFilled   = true;
    m_ShowNCMark      = true;
    m_ShowNotPlatedHole = false;
    m_IsPrinting = false;
}


void D_PAD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDraw_mode, const wxPoint& aOffset )
{
    int    color = 0;
    wxSize mask_margin;   // margin (clearance) used for some non copper layers

#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
    int    showActualMaskSize = 0;  /* Layer number if the actual pad size on mask layer can
                                     * be displayed i.e. if only one layer is shown for this pad
                                     * and this layer is a mask (solder mask or sloder paste
                                     */
#endif

    if( m_Flags & DO_NOT_DRAW )
        return;

    PAD_DRAWINFO drawInfo;

    drawInfo.m_Offset = aOffset;

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
    (LAYER_BACK | ADHESIVE_LAYER_BACK | SOLDERPASTE_LAYER_BACK \
     | SILKSCREEN_LAYER_BACK | SOLDERMASK_LAYER_BACK)

    // Mask layers for Front side of board
    #define FRONT_SIDE_LAYERS \
    (LAYER_FRONT | ADHESIVE_LAYER_FRONT | SOLDERPASTE_LAYER_FRONT \
     | SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_FRONT)

    BOARD* brd = GetBoard();
    bool   frontVisible = brd->IsElementVisible( PCB_VISIBLE( PAD_FR_VISIBLE ) );
    bool   backVisible  = brd->IsElementVisible( PCB_VISIBLE( PAD_BK_VISIBLE ) );

    if( !frontVisible && !backVisible )
        return;

    /* If pad are only on front side (no layer on back side)
     * and if hide front side pads is enabled, do not draw
     */
    if( !frontVisible && ( (m_layerMask & BACK_SIDE_LAYERS) == 0 ) )
        return;

    /* If pad are only on back side (no layer on front side)
     * and if hide back side pads is enabled, do not draw
     */
    if( !backVisible && ( (m_layerMask & FRONT_SIDE_LAYERS) == 0 ) )
        return;


    PCB_BASE_FRAME* frame  = (PCB_BASE_FRAME*) aPanel->GetParent();
    PCB_SCREEN*     screen = frame->GetScreen();

    if( frame->m_DisplayPadFill == FILLED )
        drawInfo.m_ShowPadFilled = true;
    else
        drawInfo.m_ShowPadFilled = false;

    if( m_layerMask & LAYER_FRONT )
    {
        color = brd->GetVisibleElementColor( PAD_FR_VISIBLE );
    }

    if( m_layerMask & LAYER_BACK )
    {
        color |= brd->GetVisibleElementColor( PAD_BK_VISIBLE );
    }

    if( color == 0 ) /* Not on copper layer */
    {
        // If the pad in on only one tech layer, use the layer color else use DARKGRAY
        int mask_non_copper_layers = m_layerMask & ~ALL_CU_LAYERS;
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
        mask_non_copper_layers &= brd->GetVisibleLayers();
#endif
        switch( mask_non_copper_layers )
        {
        case 0:
            break;

        case ADHESIVE_LAYER_BACK:
            color = brd->GetLayerColor( ADHESIVE_N_BACK );
            break;

        case ADHESIVE_LAYER_FRONT:
            color = brd->GetLayerColor( ADHESIVE_N_FRONT );
            break;

        case SOLDERPASTE_LAYER_BACK:
            color = brd->GetLayerColor( SOLDERPASTE_N_BACK );
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
            showActualMaskSize = SOLDERPASTE_N_BACK;
#endif
            break;

        case SOLDERPASTE_LAYER_FRONT:
            color = brd->GetLayerColor( SOLDERPASTE_N_FRONT );
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
            showActualMaskSize = SOLDERPASTE_N_FRONT;
#endif
            break;

        case SILKSCREEN_LAYER_BACK:
            color = brd->GetLayerColor( SILKSCREEN_N_BACK );
            break;

        case SILKSCREEN_LAYER_FRONT:
            color = brd->GetLayerColor( SILKSCREEN_N_FRONT );
            break;

        case SOLDERMASK_LAYER_BACK:
            color = brd->GetLayerColor( SOLDERMASK_N_BACK );
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
            showActualMaskSize = SOLDERMASK_N_BACK;
#endif
            break;

        case SOLDERMASK_LAYER_FRONT:
            color = brd->GetLayerColor( SOLDERMASK_N_FRONT );
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
            showActualMaskSize = SOLDERMASK_N_FRONT;
#endif
            break;

        case DRAW_LAYER:
            color = brd->GetLayerColor( DRAW_N );
            break;

        case COMMENT_LAYER:
            color = brd->GetLayerColor( COMMENT_N );
            break;

        case ECO1_LAYER:
            color = brd->GetLayerColor( ECO1_N );
            break;

        case ECO2_LAYER:
            color = brd->GetLayerColor( ECO2_N );
            break;

        case EDGE_LAYER:
            color = brd->GetLayerColor( EDGE_N );
            break;

        default:
            color = DARKGRAY;
            break;
        }
    }

    // if PAD_SMD pad and high contrast mode
    if( ( m_Attribut == PAD_SMD || m_Attribut == PAD_CONN ) && DisplayOpt.ContrastModeDisplay )
    {
        // when routing tracks
        if( frame && frame->GetToolId() == ID_TRACK_BUTT )
        {
            int routeTop = screen->m_Route_Layer_TOP;
            int routeBot = screen->m_Route_Layer_BOTTOM;

            // if routing between copper and component layers,
            // or the current layer is one of said 2 external copper layers,
            // then highlight only the current layer.
            if( ( ( 1 << routeTop ) | ( 1 << routeBot ) ) == ( LAYER_BACK | LAYER_FRONT )
               || ( ( 1 << screen->m_Active_Layer ) & ( LAYER_BACK | LAYER_FRONT ) ) )
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
            color = brd->GetLayerColor( screen->m_Active_Layer );

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


    if( aDraw_mode & GR_HIGHLIGHT )
    {
        if( aDraw_mode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }

    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    bool DisplayIsol = DisplayOpt.DisplayPadIsol;

    if( ( m_layerMask & ALL_CU_LAYERS ) == 0 )
        DisplayIsol = false;

    if( m_Attribut == PAD_HOLE_NOT_PLATED )
        drawInfo.m_ShowNotPlatedHole = true;

    drawInfo.m_DrawMode    = aDraw_mode;
    drawInfo.m_Color       = color;
    drawInfo.m_DrawPanel   = aPanel;
    drawInfo.m_Mask_margin = mask_margin;
    drawInfo.m_ShowNCMark  = brd->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) );
    drawInfo.m_IsPrinting  = screen->m_IsPrinting;
    SetAlpha( &color, 170 );

    /* Get the pad clearance. This has a meaning only for Pcbnew.
     *  for CvPcb (and GerbView) GetClearance() creates debug errors because
     *  there is no net classes so a call to GetClearance() is made only when
     *   needed (never needed in CvPcb nor in GerbView)
     */
    drawInfo.m_PadClearance = DisplayIsol ? GetClearance() : 0;

    /* Draw the pad number */
    if( frame && !frame->m_DisplayPadNum )
        drawInfo.m_Display_padnum = false;

    if( ( DisplayOpt.DisplayNetNamesMode == 0 ) || ( DisplayOpt.DisplayNetNamesMode == 2 ) )
        drawInfo.m_Display_netname = false;

    // Display net names is restricted to pads that are on the active layer
    // in hight contrast mode display
    if( !IsOnLayer( screen->m_Active_Layer ) && DisplayOpt.ContrastModeDisplay )
        drawInfo.m_Display_netname = false;

    DrawShape( &aPanel->m_ClipBox, aDC, drawInfo );
}


void D_PAD::DrawShape( EDA_RECT* aClipBox, wxDC* aDC, PAD_DRAWINFO& aDrawInfo )
{
    wxPoint coord[4];
    int     delta_cx, delta_cy;
    int     angle = m_Orient;
    int     seg_width;

    GRSetDrawMode( aDC, aDrawInfo.m_DrawMode );

    // calculate pad shape position :
    wxPoint shape_pos = ReturnShapePos() - aDrawInfo.m_Offset;

    wxSize  halfsize = TO_LEGACY_LU_WXS( m_Size );
    halfsize.x /= 2;
    halfsize.y /= 2;

    switch( GetShape() )
    {
    case PAD_CIRCLE:
        if( aDrawInfo.m_ShowPadFilled )
            GRFilledCircle( aClipBox, aDC, shape_pos.x, shape_pos.y,
                            halfsize.x + aDrawInfo.m_Mask_margin.x, 0,
                            aDrawInfo.m_Color, aDrawInfo.m_Color );
        else
            GRCircle( aClipBox, aDC, shape_pos.x, shape_pos.y,
                      halfsize.x + aDrawInfo.m_Mask_margin.x,
                      m_PadSketchModePenSize, aDrawInfo.m_Color );

        if( aDrawInfo.m_PadClearance )
        {
            GRCircle( aClipBox,
                      aDC, shape_pos.x, shape_pos.y,
                      halfsize.x + aDrawInfo.m_PadClearance,
                      0,
                      aDrawInfo.m_Color );
        }

        break;

    case PAD_OVAL:
    {
        wxPoint segStart, segEnd;
        seg_width = BuildSegmentFromOvalShape(segStart, segEnd, angle);
        segStart += shape_pos;
        segEnd += shape_pos;

        if( aDrawInfo.m_ShowPadFilled )
        {
            GRFillCSegm( aClipBox, aDC, segStart.x, segStart.y, segEnd.x, segEnd.y,
                         seg_width, aDrawInfo.m_Color );
        }
        else
        {
            GRCSegm( aClipBox, aDC, segStart.x, segStart.y, segEnd.x, segEnd.y,
                     seg_width, m_PadSketchModePenSize, aDrawInfo.m_Color );
        }

        /* Draw the isolation line. */
        if( aDrawInfo.m_PadClearance )
        {
            seg_width += 2 * aDrawInfo.m_PadClearance;
            GRCSegm( aClipBox, aDC, segStart.x, segStart.y, segEnd.x, segEnd.y,
                     seg_width, aDrawInfo.m_Color );
        }
    }
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
        BuildPadPolygon( coord, aDrawInfo.m_Mask_margin, angle );

        for( int ii = 0; ii < 4; ii++ )
            coord[ii] += shape_pos;

        GRClosedPoly( aClipBox, aDC, 4, coord, aDrawInfo.m_ShowPadFilled,
                      aDrawInfo.m_ShowPadFilled ? 0 : m_PadSketchModePenSize,
                      aDrawInfo.m_Color, aDrawInfo.m_Color );

        if( aDrawInfo.m_PadClearance )
        {
            BuildPadPolygon( coord, wxSize( aDrawInfo.m_PadClearance,
                                            aDrawInfo.m_PadClearance ), angle );
            for( int ii = 0; ii < 4; ii++ )
                coord[ii] += shape_pos;

            GRClosedPoly( aClipBox, aDC, 4, coord, 0, aDrawInfo.m_Color, aDrawInfo.m_Color );
        }
        break;


    default:
        break;
    }

    /* Draw the pad hole */
    wxPoint holepos = m_Pos - aDrawInfo.m_Offset;
    int     hole    = TO_LEGACY_LU( m_Drill.x ) / 2;

    bool drawhole = hole > 0;

    if( !aDrawInfo.m_ShowPadFilled && !aDrawInfo. m_ShowNotPlatedHole )
        drawhole = false;

    if( drawhole )
    {
        bool blackpenstate = false;

        if( aDrawInfo.m_IsPrinting )
        {
            blackpenstate = GetGRForceBlackPenState();
            GRForceBlackPen( false );
            aDrawInfo.m_HoleColor = g_DrawBgColor;
        }

        if( aDrawInfo.m_DrawMode != GR_XOR )
            GRSetDrawMode( aDC, GR_COPY );
        else
            GRSetDrawMode( aDC, GR_XOR );

        int hole_color = aDrawInfo.m_HoleColor;

        if( aDrawInfo. m_ShowNotPlatedHole )    // Draw a specific hole color
            hole_color = aDrawInfo.m_NPHoleColor;

        switch( m_DrillShape )
        {
        case PAD_CIRCLE:
            if( aDC->LogicalToDeviceXRel( hole ) > 1 )
                GRFilledCircle( aClipBox, aDC, holepos.x, holepos.y, hole, 0,
                                aDrawInfo.m_Color, hole_color );
            break;

        case PAD_OVAL:
            halfsize.x = TO_LEGACY_LU( m_Drill.x ) / 2;
            halfsize.y = TO_LEGACY_LU( m_Drill.y ) / 2;

            if( m_Drill.x > m_Drill.y )  /* horizontal */
            {
                delta_cx = halfsize.x - halfsize.y;
                delta_cy = 0;
                seg_width    = TO_LEGACY_LU( m_Drill.y );
            }
            else                         /* vertical */
            {
                delta_cx = 0;
                delta_cy = halfsize.y - halfsize.x;
                seg_width    = TO_LEGACY_LU( m_Drill.x );
            }

            RotatePoint( &delta_cx, &delta_cy, angle );

            GRFillCSegm( aClipBox, aDC, holepos.x + delta_cx, holepos.y + delta_cy,
                         holepos.x - delta_cx, holepos.y - delta_cy, seg_width,
                         hole_color );
            break;

        default:
            break;
        }

        if( aDrawInfo.m_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }

    GRSetDrawMode( aDC, aDrawInfo.m_DrawMode );

    /* Draw "No connect" ( / or \ or cross X ) if necessary. : */
    if( m_Netname.IsEmpty() && aDrawInfo.m_ShowNCMark )
    {
        int dx0 = MIN( halfsize.x, halfsize.y );
        int nc_color = BLUE;

        if( m_layerMask & LAYER_FRONT )    /* Draw \ */
            GRLine( aClipBox, aDC, holepos.x - dx0, holepos.y - dx0,
                    holepos.x + dx0, holepos.y + dx0, 0, nc_color );

        if( m_layerMask & LAYER_BACK ) /* Draw / */
            GRLine( aClipBox, aDC, holepos.x + dx0, holepos.y - dx0,
                    holepos.x - dx0, holepos.y + dx0, 0, nc_color );
    }

    /* Draw the pad number */
    if( !aDrawInfo.m_Display_padnum && !aDrawInfo.m_Display_netname )
        return;

    wxPoint tpos0 = shape_pos;     // Position of the centre of text
    wxPoint tpos  = tpos0;
    wxSize  AreaSize;              // size of text area, normalized to AreaSize.y < AreaSize.x
    int     shortname_len = m_ShortNetname.Len();

    if( !aDrawInfo.m_Display_netname )
        shortname_len = 0;

    if( GetShape() == PAD_CIRCLE )
        angle = 0;

    AreaSize = TO_LEGACY_LU_WXS( m_Size );

    if( m_Size.y > m_Size.x )
    {
        angle += 900;
        AreaSize.x = TO_LEGACY_LU( m_Size.y );
        AreaSize.y = TO_LEGACY_LU( m_Size.x );
    }

    if( shortname_len > 0 )       // if there is a netname, provides room to display this netname
    {
        AreaSize.y /= 2;          // Text used only the upper area of the
                                  // pad. The lower area displays the net name
        tpos.y -= AreaSize.y / 2;
    }

    // Calculate the position of text, that is the middle point of the upper
    // area of the pad
    RotatePoint( &tpos, shape_pos, angle );

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

    if( aDrawInfo.m_Display_padnum )
    {
        ReturnStringPadName( buffer );
        int numpad_len = buffer.Len();
        numpad_len = MAX( numpad_len, MIN_CHAR_COUNT );

        tsize = min( AreaSize.y, AreaSize.x / numpad_len );
        #define CHAR_SIZE_MIN 5

        if( aDC->LogicalToDeviceXRel( tsize ) >= CHAR_SIZE_MIN ) // Not drawable when size too small.
        {
            // tsize reserve room for marges and segments thickness
            tsize = (int) ( tsize * 0.8 );
            DrawGraphicText( aDrawInfo.m_DrawPanel, aDC, tpos, WHITE, buffer, t_angle,
                             wxSize( tsize, tsize ), GR_TEXT_HJUSTIFY_CENTER,
                             GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false, false );
        }
    }

    // display the short netname, if exists
    if( shortname_len == 0 )
        return;

    shortname_len = MAX( shortname_len, MIN_CHAR_COUNT );
    tsize = min( AreaSize.y, AreaSize.x / shortname_len );

    if( aDC->LogicalToDeviceXRel( tsize ) >= CHAR_SIZE_MIN )  // Not drawable in size too small.
    {
        tpos = tpos0;

        if( aDrawInfo.m_Display_padnum )
            tpos.y += AreaSize.y / 2;

        RotatePoint( &tpos, shape_pos, angle );

        // tsize reserve room for marges and segments thickness
        tsize = (int) ( tsize * 0.8 );
        DrawGraphicText( aDrawInfo.m_DrawPanel, aDC, tpos, WHITE, m_ShortNetname, t_angle,
                         wxSize( tsize, tsize ), GR_TEXT_HJUSTIFY_CENTER,
                         GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false, false );
    }
}


/**
 * Function BuildSegmentFromOvalShape
 * Has meaning only for OVAL (and ROUND) pads.
 * Build an equivalent segment having the same shape as the OVAL shape,
 * aSegStart and aSegEnd are the ending points of the equivalent segment of the shape
 * aRotation is the asked rotation of the segment (usually m_Orient)
 */
int D_PAD::BuildSegmentFromOvalShape(wxPoint& aSegStart, wxPoint& aSegEnd, int aRotation) const
{
    int width;

    if( m_Size.y < m_Size.x )     // Build an horizontal equiv segment
    {
        int delta   = TO_LEGACY_LU( ( m_Size.x - m_Size.y ) / 2 );
        aSegStart.x = -delta;
        aSegStart.y = 0;
        aSegEnd.x = delta;
        aSegEnd.y = 0;
        width = TO_LEGACY_LU( m_Size.y );
    }
    else        // Vertical oval: build a vertical equiv segment
    {
        int delta   = TO_LEGACY_LU( ( m_Size.y -m_Size.x ) / 2 );
        aSegStart.x = 0;
        aSegStart.y = -delta;
        aSegEnd.x = 0;
        aSegEnd.y = delta;
        width = TO_LEGACY_LU( m_Size.x );
    }

    if( aRotation )
    {
        RotatePoint( &aSegStart, aRotation);
        RotatePoint( &aSegEnd, aRotation);
    }

    return width;
}


void D_PAD::BuildPadPolygon( wxPoint aCoord[4], wxSize aInflateValue, int aRotation ) const
{
    if( (GetShape() != PAD_RECT) && (GetShape() != PAD_TRAPEZOID) )
        return;

    wxSize delta;
    wxSize halfsize;

    halfsize.x = TO_LEGACY_LU( m_Size.x / 2 );
    halfsize.y = TO_LEGACY_LU( m_Size.y / 2 );

    /* For rectangular shapes, inflate is easy
     */
    if( GetShape() == PAD_RECT )
    {
        halfsize += aInflateValue;

        // Verify if do not deflate more than than size
        // Only possible for inflate negative values.
        if( halfsize.x < 0 )
            halfsize.x = 0;

        if( halfsize.y < 0 )
            halfsize.y = 0;
    }
    else
    {
        // Trapezoidal pad: verify delta values
        delta.x = TO_LEGACY_LU( m_DeltaSize.x / 2 );
        delta.y = TO_LEGACY_LU( m_DeltaSize.y / 2 );

        // be sure delta values are not to large
        if( (delta.x < 0) && (delta.x <= -halfsize.y) )
            delta.x = -halfsize.y + 1;

        if( (delta.x > 0) && (delta.x >= halfsize.y) )
            delta.x = halfsize.y - 1;

        if( (delta.y < 0) && (delta.y <= -halfsize.x) )
            delta.y = -halfsize.x + 1;

        if( (delta.y > 0) && (delta.y >= halfsize.x) )
            delta.y = halfsize.x - 1;
    }

    // Build the basic rectangular or trapezoid shape
    // delta is null for rectangular shapes
    aCoord[0].x = -halfsize.x - delta.y;     // lower left
    aCoord[0].y = +halfsize.y + delta.x;

    aCoord[1].x = -halfsize.x + delta.y;     // upper left
    aCoord[1].y = -halfsize.y - delta.x;

    aCoord[2].x = +halfsize.x - delta.y;     // upper right
    aCoord[2].y = -halfsize.y + delta.x;

    aCoord[3].x = +halfsize.x + delta.y;     // lower right
    aCoord[3].y = +halfsize.y - delta.x;

    // Offsetting the trapezoid shape id needed
    // It is assumed delta.x or/and delta.y == 0
    if( GetShape() == PAD_TRAPEZOID && (aInflateValue.x != 0 || aInflateValue.y != 0) )
    {
        double angle;
        wxSize corr;

        if( delta.y )    // lower and upper segment is horizontal
        {
            // Calculate angle of left (or right) segment with vertical axis
            angle = atan2( m_DeltaSize.y, m_Size.y ); /// TODO: make atan2 available to LENGTH

            // left and right sides are moved by aInflateValue.x in their perpendicular direction
            // We must calculate the corresponding displacement on the horizontal axis
            // that is delta.x +- corr.x depending on the corner
            corr.x  = wxRound( tan( angle ) * aInflateValue.x );
            delta.x = wxRound( aInflateValue.x / cos( angle ) );

            // Horizontal sides are moved up and down by aInflateValue.y
            delta.y = aInflateValue.y;

            // corr.y = 0 by the constructor
        }
        else if( delta.x )          // left and right segment is vertical
        {
            // Calculate angle of lower (or upper) segment with horizontal axis
            angle = atan2( m_DeltaSize.x, m_Size.x );

            // lower and upper sides are moved by aInflateValue.x in their perpendicular direction
            // We must calculate the corresponding displacement on the vertical axis
            // that is delta.y +- corr.y depending on the corner
            corr.y  = wxRound( tan( angle ) * aInflateValue.y );
            delta.y = wxRound( aInflateValue.y / cos( angle ) );

            // Vertical sides are moved left and right by aInflateValue.x
            delta.x = aInflateValue.x;

            // corr.x = 0 by the constructor
        }
        else                                    // the trapezoid is a rectangle
        {
            delta = aInflateValue;              // this pad is rectangular (delta null).
        }

        aCoord[0].x += -delta.x - corr.x;       // lower left
        aCoord[0].y += delta.y + corr.y;

        aCoord[1].x += -delta.x + corr.x;     // upper left
        aCoord[1].y += -delta.y - corr.y;

        aCoord[2].x += delta.x - corr.x;     // upper right
        aCoord[2].y += -delta.y + corr.y;

        aCoord[3].x += delta.x + corr.x;     // lower right
        aCoord[3].y += delta.y - corr.y;

        /* test coordinates and clamp them if the offset correction is too large:
         * Note: if a coordinate is bad, the other "symmetric" coordinate is bad
         * So when a bad coordinate is found, the 2 symmetric coordinates
         * are set to the minimun value (0)
         */

        if( aCoord[0].x > 0 )       // lower left x coordinate must be <= 0
            aCoord[0].x = aCoord[3].x = 0;

        if( aCoord[1].x > 0 )       // upper left x coordinate must be <= 0
            aCoord[1].x = aCoord[2].x = 0;

        if( aCoord[0].y < 0 )       // lower left y coordinate must be >= 0
            aCoord[0].y = aCoord[1].y = 0;

        if( aCoord[3].y < 0 )       // lower right y coordinate must be >= 0
            aCoord[3].y = aCoord[2].y = 0;
    }

    if( aRotation )
    {
        for( int ii = 0; ii < 4; ii++ )
            RotatePoint( &aCoord[ii], aRotation );
    }
}
