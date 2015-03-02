/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <class_pcb_screen.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <layers_id_colors_and_visibility.h>
#include <wxBasePcbFrame.h>
#include <pcbnew_id.h>             // ID_TRACK_BUTT
#include <pcbnew.h>
#include <class_board.h>


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
    m_DrawMode        = GR_COPY;
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


void D_PAD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDraw_mode,
                  const wxPoint& aOffset )
{
    wxSize mask_margin;   // margin (clearance) used for some non copper layers

#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
    int    showActualMaskSize = 0;  /* Layer number if the actual pad size on mask layer can
                                     * be displayed i.e. if only one layer is shown for this pad
                                     * and this layer is a mask (solder mask or solder paste
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

    BOARD* brd = GetBoard();
    bool   frontVisible = brd->IsElementVisible( PCB_VISIBLE( PAD_FR_VISIBLE ) );
    bool   backVisible  = brd->IsElementVisible( PCB_VISIBLE( PAD_BK_VISIBLE ) );

    if( !frontVisible && !backVisible )
        return;

    // If pad is only on front side (no layer on back side)
    // and if hide front side pads is enabled, do not draw
    if( !frontVisible && !( m_layerMask & LSET::BackMask() ).any() )
        return;

    // If pad is only on back side (no layer on front side)
    // and if hide back side pads is enabled, do not draw
    if( !backVisible && !( m_layerMask & LSET::FrontMask() ).any() )
        return;

    PCB_BASE_FRAME* frame  = (PCB_BASE_FRAME*) aPanel->GetParent();

    wxCHECK_RET( frame != NULL, wxT( "Panel has no parent frame window." ) );

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)frame->GetDisplayOptions();
    PCB_SCREEN*     screen = frame->GetScreen();

    if( displ_opts && displ_opts->m_DisplayPadFill == SKETCH )
        drawInfo.m_ShowPadFilled = false;
    else
        drawInfo.m_ShowPadFilled = true;

    EDA_COLOR_T color = BLACK;

    if( m_layerMask[F_Cu] )
    {
        color = brd->GetVisibleElementColor( PAD_FR_VISIBLE );
    }

    if( m_layerMask[B_Cu] )
    {
        color = ColorMix( color, brd->GetVisibleElementColor( PAD_BK_VISIBLE ) );
    }

    if( color == BLACK ) // Not on a visible copper layer (i.e. still nothing to show)
    {
        // If the pad is on only one tech layer, use the layer color else use DARKGRAY
        LSET mask_non_copper_layers = m_layerMask & ~LSET::AllCuMask();

#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
        mask_non_copper_layers &= brd->GetVisibleLayers();
#endif
        LAYER_ID pad_layer = mask_non_copper_layers.ExtractLayer();

        switch( (int) pad_layer )
        {
        case UNDEFINED_LAYER:   // More than one layer
            color = DARKGRAY;
            break;

        case UNSELECTED_LAYER:  // Shouldn't really happen...
            break;

        default:
            color = brd->GetLayerColor( pad_layer );
#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
            showActualMaskSize = pad_layer;
#endif
        }
    }

    // if SMD or connector pad and high contrast mode
    if( ( aDraw_mode & GR_ALLOW_HIGHCONTRAST ) &&
        ( GetAttribute() == PAD_SMD || GetAttribute() == PAD_CONN ) &&
        displ_opts && displ_opts->m_ContrastModeDisplay )
    {
        // when routing tracks
        if( frame->GetToolId() == ID_TRACK_BUTT )
        {
            LAYER_ID routeTop = screen->m_Route_Layer_TOP;
            LAYER_ID routeBot = screen->m_Route_Layer_BOTTOM;

            // if routing between copper and component layers,
            // or the current layer is one of said 2 external copper layers,
            // then highlight only the current layer.
            if( ( screen->m_Active_Layer == F_Cu || screen->m_Active_Layer == B_Cu ) ||
                ( routeTop==F_Cu && routeBot==B_Cu ) ||
                ( routeTop==B_Cu && routeBot==F_Cu )
                )
            {
                if( !IsOnLayer( screen->m_Active_Layer ) )
                    ColorTurnToDarkDarkGray( &color );
            }
            // else routing between an internal signal layer and some other
            // layer.  Grey out all PAD_SMD pads not on current or the single
            // selected external layer.
            else if( !IsOnLayer( screen->m_Active_Layer )
                    && !IsOnLayer( routeTop )
                    && !IsOnLayer( routeBot ) )
            {
                ColorTurnToDarkDarkGray( &color );
            }
        }
        // when not edting tracks, show PAD_SMD components not on active layer
        // as greyed out
        else
        {
            if( !IsOnLayer( screen->m_Active_Layer ) )
                ColorTurnToDarkDarkGray( &color );
        }
    }

#ifdef SHOW_PADMASK_REAL_SIZE_AND_COLOR
    if( showActualMaskSize )
    {
        switch( showActualMaskSize )
        {
        case B_Mask:
        case F_Mask:
            mask_margin.x = mask_margin.y = GetSolderMaskMargin();
            break;

        case B_Paste:
        case F_Paste:
            mask_margin = GetSolderPasteMargin();
            break;

        default:
            // Another layer which has no margin to handle
            break;
        }
    }
#endif

    // if Contrast mode is ON and a technical layer active, show pads on this
    // layer so we can see pads on paste or solder layer and the size of the
    // mask
    if( ( aDraw_mode & GR_ALLOW_HIGHCONTRAST ) &&
        displ_opts && displ_opts->m_ContrastModeDisplay && !IsCopperLayer( screen->m_Active_Layer ) )
    {
        if( IsOnLayer( screen->m_Active_Layer ) )
        {
            color = brd->GetLayerColor( screen->m_Active_Layer );

            // In high contrast mode, and if the active layer is the mask
            // layer shows the pad size with the mask clearance
            switch( screen->m_Active_Layer )
            {
            case B_Mask:
            case F_Mask:
                mask_margin.x = mask_margin.y = GetSolderMaskMargin();
                break;

            case B_Paste:
            case F_Paste:
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
        ColorChangeHighlightFlag( &color, !(aDraw_mode & GR_AND) );

    ColorApplyHighlightFlag( &color );

    bool DisplayIsol = displ_opts && displ_opts->m_DisplayPadIsol;

    if( !( m_layerMask & LSET::AllCuMask() ).any() )
        DisplayIsol = false;

    if( ( GetAttribute() == PAD_HOLE_NOT_PLATED ) &&
        brd->IsElementVisible( NON_PLATED_VISIBLE ) )
    {
        drawInfo.m_ShowNotPlatedHole = true;
        drawInfo.m_NPHoleColor = brd->GetVisibleElementColor( NON_PLATED_VISIBLE );
    }

    drawInfo.m_DrawMode    = aDraw_mode;
    drawInfo.m_Color       = color;
    drawInfo.m_DrawPanel   = aPanel;
    drawInfo.m_Mask_margin = mask_margin;
    drawInfo.m_ShowNCMark  = brd->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) );
    drawInfo.m_IsPrinting  = screen->m_IsPrinting;
    SetAlpha( &color, 170 );

    /* Get the pad clearance. This has a meaning only for Pcbnew.
     *  for CvPcb GetClearance() creates debug errors because
     *  there is no net classes so a call to GetClearance() is made only when
     *   needed (never needed in CvPcb)
     */
    drawInfo.m_PadClearance = DisplayIsol ? GetClearance() : 0;

    // Draw the pad number
    if( displ_opts && !displ_opts->m_DisplayPadNum )
        drawInfo.m_Display_padnum = false;

    if( displ_opts &&
        (( displ_opts ->m_DisplayNetNamesMode == 0 ) || ( displ_opts->m_DisplayNetNamesMode == 2 )) )
        drawInfo.m_Display_netname = false;

    // Display net names is restricted to pads that are on the active layer
    // in high contrast mode display
    if( ( aDraw_mode & GR_ALLOW_HIGHCONTRAST ) &&
        !IsOnLayer( screen->m_Active_Layer ) && displ_opts && displ_opts->m_ContrastModeDisplay )
        drawInfo.m_Display_netname = false;

    DrawShape( aPanel->GetClipBox(), aDC, drawInfo );
}


void D_PAD::DrawShape( EDA_RECT* aClipBox, wxDC* aDC, PAD_DRAWINFO& aDrawInfo )
{
    wxPoint coord[4];
    double  angle = m_Orient;
    int     seg_width;

    GRSetDrawMode( aDC, aDrawInfo.m_DrawMode );

    // calculate pad shape position :
    wxPoint shape_pos = ShapePos() - aDrawInfo.m_Offset;

    wxSize  halfsize = m_Size;
    halfsize.x >>= 1;
    halfsize.y >>= 1;

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
                      0, aDrawInfo.m_Color );
        }

        break;

    case PAD_OVAL:
    {
        wxPoint segStart, segEnd;
        seg_width = BuildSegmentFromOvalShape(segStart, segEnd, angle,
                                              aDrawInfo.m_Mask_margin);
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

        // Draw the clearance line
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

    // Draw the pad hole
    wxPoint holepos = m_Pos - aDrawInfo.m_Offset;
    int     hole    = m_Drill.x >> 1;

    bool drawhole = hole > 0;

    if( !aDrawInfo.m_ShowPadFilled && !aDrawInfo.m_ShowNotPlatedHole )
        drawhole = false;

    if( drawhole )
    {
        bool blackpenstate = false;

        if( aDrawInfo.m_IsPrinting )
        {
            blackpenstate = GetGRForceBlackPenState();
            GRForceBlackPen( false );
            aDrawInfo.m_HoleColor = WHITE;
        }

        if( aDrawInfo.m_DrawMode != GR_XOR )
            GRSetDrawMode( aDC, GR_COPY );
        else
            GRSetDrawMode( aDC, GR_XOR );

        EDA_COLOR_T hole_color = aDrawInfo.m_HoleColor;

        if( aDrawInfo. m_ShowNotPlatedHole )    // Draw a specific hole color
            hole_color = aDrawInfo.m_NPHoleColor;

        switch( GetDrillShape() )
        {
        case PAD_DRILL_CIRCLE:
            if( aDC->LogicalToDeviceXRel( hole ) > MIN_DRAW_WIDTH )
                GRFilledCircle( aClipBox, aDC, holepos.x, holepos.y, hole, 0,
                                hole_color, hole_color );
            break;

        case PAD_DRILL_OBLONG:
        {
            wxPoint drl_start, drl_end;
            GetOblongDrillGeometry( drl_start, drl_end, seg_width );
            GRFilledSegment( aClipBox, aDC, holepos + drl_start,
                             holepos + drl_end, seg_width, hole_color );
        }
            break;

        default:
            break;
        }

        if( aDrawInfo.m_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }

    GRSetDrawMode( aDC, aDrawInfo.m_DrawMode );

    // Draw "No connect" ( / or \ or cross X ) if necessary
    if( GetNetCode() == 0 && aDrawInfo.m_ShowNCMark )
    {
        int dx0 = std::min( halfsize.x, halfsize.y );
        EDA_COLOR_T nc_color = BLUE;

        if( m_layerMask[F_Cu] )    /* Draw \ */
            GRLine( aClipBox, aDC, holepos.x - dx0, holepos.y - dx0,
                    holepos.x + dx0, holepos.y + dx0, 0, nc_color );

        if( m_layerMask[B_Cu] )     // Draw /
            GRLine( aClipBox, aDC, holepos.x + dx0, holepos.y - dx0,
                    holepos.x - dx0, holepos.y + dx0, 0, nc_color );
    }

    if( aDrawInfo.m_DrawMode != GR_XOR )
        GRSetDrawMode( aDC, GR_COPY );
    else
        GRSetDrawMode( aDC, GR_XOR );

    // Draw the pad number
    if( !aDrawInfo.m_Display_padnum && !aDrawInfo.m_Display_netname )
        return;

    wxPoint tpos0 = shape_pos;     // Position of the centre of text
    wxPoint tpos  = tpos0;
    wxSize  AreaSize;              // size of text area, normalized to AreaSize.y < AreaSize.x
    int     shortname_len = 0;

    if( aDrawInfo.m_Display_netname )
        shortname_len = GetShortNetname().Len();

    if( GetShape() == PAD_CIRCLE )
        angle = 0;

    AreaSize = m_Size;

    if( m_Size.y > m_Size.x )
    {
        angle += 900;
        AreaSize.x = m_Size.y;
        AreaSize.y = m_Size.x;
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

    // Draw text with an angle between -90 deg and + 90 deg
    double t_angle = angle;
    NORMALIZE_ANGLE_90( t_angle );

    /* Note: in next calculations, texte size is calculated for 3 or more
     * chars.  Of course, pads numbers and nets names can have less than 3
     * chars. but after some tries, i found this is gives the best look
     */
    #define MIN_CHAR_COUNT 3
    wxString buffer;

    int      tsize;
    EDA_RECT* clipBox = aDrawInfo.m_DrawPanel?
                        aDrawInfo.m_DrawPanel->GetClipBox() : NULL;

    if( aDrawInfo.m_Display_padnum )
    {
        StringPadName( buffer );
        int numpad_len = buffer.Len();
        numpad_len = std::max( numpad_len, MIN_CHAR_COUNT );

        tsize = std::min( AreaSize.y, AreaSize.x / numpad_len );

        if( aDC->LogicalToDeviceXRel( tsize ) >= MIN_TEXT_SIZE ) // Not drawable when size too small.
        {
            // tsize reserve room for marges and segments thickness
            tsize = ( tsize * 7 ) / 10;
            DrawGraphicHaloText( clipBox, aDC, tpos,
                                 aDrawInfo.m_Color, BLACK, WHITE,
                                 buffer, t_angle,
                                 wxSize( tsize , tsize ), GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_CENTER, tsize / 7, false, false );

        }
    }

    // display the short netname, if exists
    if( shortname_len == 0 )
        return;

    shortname_len = std::max( shortname_len, MIN_CHAR_COUNT );
    tsize = std::min( AreaSize.y, AreaSize.x / shortname_len );

    if( aDC->LogicalToDeviceXRel( tsize ) >= MIN_TEXT_SIZE )  // Not drawable in size too small.
    {
        tpos = tpos0;

        if( aDrawInfo.m_Display_padnum )
            tpos.y += AreaSize.y / 2;

        RotatePoint( &tpos, shape_pos, angle );

        // tsize reserve room for marges and segments thickness
        tsize = ( tsize * 7 ) / 10;
        DrawGraphicHaloText( clipBox, aDC, tpos,
                             aDrawInfo.m_Color, BLACK, WHITE,
                             GetShortNetname(), t_angle,
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
int D_PAD::BuildSegmentFromOvalShape(wxPoint& aSegStart, wxPoint& aSegEnd,
                                     double aRotation, const wxSize& aMargin) const
{
    int width;

    if( m_Size.y < m_Size.x )     // Build an horizontal equiv segment
    {
        int delta   = ( m_Size.x - m_Size.y ) / 2;
        aSegStart.x = -delta - aMargin.x;
        aSegStart.y = 0;
        aSegEnd.x = delta + aMargin.x;
        aSegEnd.y = 0;
        width = m_Size.y + ( aMargin.y * 2 );
    }
    else        // Vertical oval: build a vertical equiv segment
    {
        int delta   = ( m_Size.y -m_Size.x ) / 2;
        aSegStart.x = 0;
        aSegStart.y = -delta - aMargin.y;
        aSegEnd.x = 0;
        aSegEnd.y = delta + aMargin.y;
        width = m_Size.x + ( aMargin.x * 2 );
    }

    if( aRotation )
    {
        RotatePoint( &aSegStart, aRotation);
        RotatePoint( &aSegEnd, aRotation);
    }

    return width;
}


void D_PAD::BuildPadPolygon( wxPoint aCoord[4], wxSize aInflateValue,
                             double aRotation ) const
{
    wxSize delta;
    wxSize halfsize;

    halfsize.x = m_Size.x >> 1;
    halfsize.y = m_Size.y >> 1;

    switch( GetShape() )
    {
        case PAD_RECT:
            // For rectangular shapes, inflate is easy
            halfsize += aInflateValue;

            // Verify if do not deflate more than than size
            // Only possible for inflate negative values.
            if( halfsize.x < 0 )
                halfsize.x = 0;

            if( halfsize.y < 0 )
                halfsize.y = 0;
            break;

        case PAD_TRAPEZOID:
            // Trapezoidal pad: verify delta values
            delta.x = ( m_DeltaSize.x >> 1 );
            delta.y = ( m_DeltaSize.y >> 1 );

            // be sure delta values are not to large
            if( (delta.x < 0) && (delta.x <= -halfsize.y) )
                delta.x = -halfsize.y + 1;

            if( (delta.x > 0) && (delta.x >= halfsize.y) )
                delta.x = halfsize.y - 1;

            if( (delta.y < 0) && (delta.y <= -halfsize.x) )
                delta.y = -halfsize.x + 1;

            if( (delta.y > 0) && (delta.y >= halfsize.x) )
                delta.y = halfsize.x - 1;
        break;

        default:    // is used only for rect and trap. pads
            return;
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
            angle = atan2( m_DeltaSize.y, m_Size.y );

            // left and right sides are moved by aInflateValue.x in their perpendicular direction
            // We must calculate the corresponding displacement on the horizontal axis
            // that is delta.x +- corr.x depending on the corner
            corr.x  = KiROUND( tan( angle ) * aInflateValue.x );
            delta.x = KiROUND( aInflateValue.x / cos( angle ) );

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
            corr.y  = KiROUND( tan( angle ) * aInflateValue.y );
            delta.y = KiROUND( aInflateValue.y / cos( angle ) );

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
