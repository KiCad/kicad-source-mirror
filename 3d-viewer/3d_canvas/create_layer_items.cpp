/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  create_layer_items.cpp
 * @brief This file implements the creation of the pcb board.
 * It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "cinfo3d_visu.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cring2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/croundsegment2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cpolygon4pts2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cpolygon2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/ctriangle2d.h"
#include "../3d_rendering/3d_render_raytracing/accelerators/ccontainer2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/ccylinder.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/clayeritem.h"
#include <openmp_mutex.h>
#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_pcb_text.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_text_mod.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <drawtxt.h>
#include <vector>



// These variables are parameters used in addTextSegmToContainer.
// But addTextSegmToContainer is a call-back function,
// so we cannot send them as arguments.
static int s_textWidth;
static CGENERICCONTAINER2D *s_dstcontainer = NULL;
static float s_biuTo3Dunits;
static const CBBOX2D *s_boardBBox3DU = NULL;
static const BOARD_ITEM *s_boardItem = NULL;

// This is a call back function, used by DrawGraphicText to draw the 3D text shape:
void addTextSegmToContainer( int x0, int y0, int xf, int yf )
{
    wxASSERT( s_boardBBox3DU != NULL );
    wxASSERT( s_dstcontainer != NULL );

    const SFVEC2F start3DU( x0 * s_biuTo3Dunits, -y0 * s_biuTo3Dunits );
    const SFVEC2F end3DU  ( xf * s_biuTo3Dunits, -yf * s_biuTo3Dunits );

    if( Is_segment_a_circle( start3DU, end3DU ) )
        s_dstcontainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                  s_textWidth * s_biuTo3Dunits,
                                                  *s_boardItem) );
    else
        s_dstcontainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                  end3DU,
                                                  s_textWidth * s_biuTo3Dunits,
                                                  *s_boardItem ) );
}


// Based on
// void TEXTE_PCB::TransformShapeWithClearanceToPolygonSet
// board_items_to_polygon_shape_transform.cpp
void CINFO3D_VISU::AddShapeWithClearanceToContainer( const TEXTE_PCB* aTextPCB,
                                                     CGENERICCONTAINER2D *aDstContainer,
                                                     LAYER_ID aLayerId,
                                                     int aClearanceValue )
{
    wxSize size = aTextPCB->GetSize();

    if( aTextPCB->IsMirrored() )
        size.x = -size.x;

    s_boardItem    = (const BOARD_ITEM *)&aTextPCB;
    s_dstcontainer = aDstContainer;
    s_textWidth    = aTextPCB->GetThickness() + ( 2 * aClearanceValue );
    s_biuTo3Dunits = m_biuTo3Dunits;
    s_boardBBox3DU = &m_board2dBBox3DU;

    // not actually used, but needed by DrawGraphicText
    const EDA_COLOR_T dummy_color = BLACK;

    if( aTextPCB->IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( aTextPCB->GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        aTextPCB->GetPositionsOfLinesOfMultilineText( positions,
                                                      strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ++ii )
        {
            wxString txt = strings_list.Item( ii );

            DrawGraphicText( NULL, NULL, positions[ii], dummy_color,
                             txt, aTextPCB->GetOrientation(), size,
                             aTextPCB->GetHorizJustify(), aTextPCB->GetVertJustify(),
                             aTextPCB->GetThickness(), aTextPCB->IsItalic(),
                             true, addTextSegmToContainer );
        }
    }
    else
    {
        DrawGraphicText( NULL, NULL, aTextPCB->GetTextPosition(), dummy_color,
                         aTextPCB->GetShownText(), aTextPCB->GetOrientation(), size,
                         aTextPCB->GetHorizJustify(), aTextPCB->GetVertJustify(),
                         aTextPCB->GetThickness(), aTextPCB->IsItalic(),
                         true, addTextSegmToContainer );
    }
}


// Based on
// void MODULE::TransformGraphicShapesWithClearanceToPolygonSet
// board_items_to_polygon_shape_transform.cpp#L204
void CINFO3D_VISU::AddGraphicsShapesWithClearanceToContainer( const MODULE* aModule,
                                                              CGENERICCONTAINER2D *aDstContainer,
                                                              LAYER_ID aLayerId,
                                                              int aInflateValue )
{
    std::vector<TEXTE_MODULE *> texts;  // List of TEXTE_MODULE to convert
    EDGE_MODULE* outline;

    for( EDA_ITEM* item = aModule->GraphicalItems();
         item != NULL;
         item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

            if( text->GetLayer() == aLayerId && text->IsVisible() )
                texts.push_back( text );
        }
        break;


        case PCB_MODULE_EDGE_T:
        {
            outline = (EDGE_MODULE*) item;

            if( outline->GetLayer() != aLayerId )
                break;

            AddShapeWithClearanceToContainer( (const DRAWSEGMENT *)outline,
                                              aDstContainer,
                                              aLayerId, 0 );
        }
        break;

        default:
            break;
        }
    }

    // Convert texts sur modules
    if( aModule->Reference().GetLayer() == aLayerId && aModule->Reference().IsVisible() )
        texts.push_back( &aModule->Reference() );

    if( aModule->Value().GetLayer() == aLayerId && aModule->Value().IsVisible() )
        texts.push_back( &aModule->Value() );

    s_boardItem    = (const BOARD_ITEM *)&aModule->Value();
    s_dstcontainer = aDstContainer;
    s_biuTo3Dunits = m_biuTo3Dunits;
    s_boardBBox3DU = &m_board2dBBox3DU;

    for( unsigned ii = 0; ii < texts.size(); ++ii )
    {
        TEXTE_MODULE *textmod = texts[ii];
        s_textWidth = textmod->GetThickness() + ( 2 * aInflateValue );
        wxSize size = textmod->GetSize();

        if( textmod->IsMirrored() )
            size.x = -size.x;

        DrawGraphicText( NULL, NULL, textmod->GetTextPosition(), BLACK,
                         textmod->GetShownText(), textmod->GetDrawRotation(), size,
                         textmod->GetHorizJustify(), textmod->GetVertJustify(),
                         textmod->GetThickness(), textmod->IsItalic(),
                         true, addTextSegmToContainer );
    }
}


COBJECT2D *CINFO3D_VISU::createNewTrack( const TRACK* aTrack,
                                         int aClearanceValue ) const
{
    SFVEC2F start3DU(  aTrack->GetStart().x * m_biuTo3Dunits,
                      -aTrack->GetStart().y * m_biuTo3Dunits ); // y coord is inverted

    switch( aTrack->Type() )
    {
    case PCB_VIA_T:
    {
        const float radius = ( ( aTrack->GetWidth() / 2 ) + aClearanceValue ) * m_biuTo3Dunits;

        return new CFILLEDCIRCLE2D( start3DU, radius, *aTrack );
    }
        break;

    default:
    {
        wxASSERT( aTrack->Type() == PCB_TRACE_T );

        SFVEC2F end3DU (  aTrack->GetEnd().x * m_biuTo3Dunits,
                         -aTrack->GetEnd().y * m_biuTo3Dunits );

        // Cannot add segments that have the same start and end point
        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            const float radius = ((aTrack->GetWidth() / 2) + aClearanceValue) * m_biuTo3Dunits;

            return new CFILLEDCIRCLE2D( start3DU, radius, *aTrack );
        }
        else
        {
            const float width = (aTrack->GetWidth() + 2 * aClearanceValue ) * m_biuTo3Dunits;

            return new CROUNDSEGMENT2D( start3DU, end3DU, width, *aTrack );
        }
    }
        break;
    }

    return NULL;
}


// Based on:
// void D_PAD:: TransformShapeWithClearanceToPolygon(
// board_items_to_polygon_shape_transform.cpp
void CINFO3D_VISU::createNewPadWithClearance( const D_PAD* aPad,
                                                    CGENERICCONTAINER2D *aDstContainer,
                                                    int aClearanceValue ) const
{
    const int dx = (aPad->GetSize().x / 2) + aClearanceValue;
    const int dy = (aPad->GetSize().y / 2) + aClearanceValue;

    if( !dx || !dy )
    {
        wxLogTrace( m_logTrace,
                    wxT( "CINFO3D_VISU::createNewPadWithClearance - found an invalid pad" ) );

        return;
    }

    wxPoint PadShapePos = aPad->ShapePos(); // Note: for pad having a shape offset,
                                            // the pad position is NOT the shape position

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
    {
        const float radius = dx * m_biuTo3Dunits;

        const SFVEC2F center(  PadShapePos.x * m_biuTo3Dunits,
                              -PadShapePos.y * m_biuTo3Dunits );

        aDstContainer->Add( new CFILLEDCIRCLE2D( center, radius, *aPad ) );
    }
    break;

    case PAD_SHAPE_OVAL:
    {
        if( abs( dx - dy ) == 0 )
        {
            // The segment object cannot store start and end the same position,
            // so add a circle instead
            const float radius = dx * m_biuTo3Dunits;

            const SFVEC2F center(  PadShapePos.x * m_biuTo3Dunits,
                                  -PadShapePos.y * m_biuTo3Dunits );

            aDstContainer->Add( new CFILLEDCIRCLE2D( center, radius, *aPad ) );
        }
        else
        {
            // An oval pad has the same shape as a segment with rounded ends

            int iwidth;
            wxPoint shape_offset = wxPoint( 0, 0 );

            if( dy > dx )   // Oval pad X/Y ratio for choosing translation axis
            {
                shape_offset.y = dy - dx;
                iwidth = dx * 2;
            }
            else    //if( dy <= dx )
            {
                shape_offset.x = dy - dx;
                iwidth = dy * 2;
            }

            RotatePoint( &shape_offset, aPad->GetOrientation() );

            const wxPoint start = PadShapePos - shape_offset;
            const wxPoint end   = PadShapePos + shape_offset;

            const SFVEC2F start3DU(  start.x * m_biuTo3Dunits, -start.y * m_biuTo3Dunits );
            const SFVEC2F end3DU  (    end.x * m_biuTo3Dunits,   -end.y * m_biuTo3Dunits );

             // Cannot add segments that have the same start and end point
            if( Is_segment_a_circle( start3DU, end3DU ) )
            {
                aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                         (iwidth / 2) * m_biuTo3Dunits,
                                                         *aPad ) );
            }
            else
            {
                aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                         end3DU,
                                                         iwidth * m_biuTo3Dunits,
                                                         *aPad ) );
            }
        }
    }
    break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        // https://github.com/KiCad/kicad-source-mirror/blob/0cab3e47ad8097db7b898b3cef2cf9b235318ca3/pcbnew/board_items_to_polygon_shape_transform.cpp#L613

        wxPoint corners[4];
        aPad->BuildPadPolygon( corners, wxSize( 0, 0), aPad->GetOrientation() );

        SFVEC2F corners3DU[4];

        // Note: for pad having a shape offset,
        // the pad position is NOT the shape position
        for( unsigned int ii = 0; ii < 4; ++ii )
        {
            corners[ii] += aPad->ShapePos();          // Shift origin to position

            corners3DU[ii] = SFVEC2F( corners[ii].x * m_biuTo3Dunits,
                                      -corners[ii].y * m_biuTo3Dunits );
        }


        // Learn more at:
        // https://lists.launchpad.net/kicad-developers/msg18729.html

        // Add the PAD polygon
        aDstContainer->Add( new CPOLYGON4PTS2D( corners3DU[0],
                                                corners3DU[1],
                                                corners3DU[2],
                                                corners3DU[3],
                                                *aPad ) );

        // Add the PAD contours
        // !TODO: check the corners because it cannot add
        // roundsegments that are in the same start and end position
        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[0],
                                                 corners3DU[1],
                                                 aClearanceValue * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[1],
                                                 corners3DU[2],
                                                 aClearanceValue * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[2],
                                                 corners3DU[3],
                                                 aClearanceValue * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[3],
                                                 corners3DU[0],
                                                 aClearanceValue * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );
    }
    break;

    case PAD_SHAPE_ROUNDRECT:
    {
        const int pad_radius = aPad->GetRoundRectCornerRadius();
        const int rounding_radius = pad_radius + aClearanceValue;

        wxSize shapesize( aPad->GetSize() );
        shapesize.x += aClearanceValue * 2;
        shapesize.y += aClearanceValue * 2;

        wxPoint corners[4];

        GetRoundRectCornerCenters( corners,
                                   rounding_radius,
                                   PadShapePos,
                                   shapesize,
                                   aPad->GetOrientation() );

        SFVEC2F corners3DU[4];

        for( unsigned int ii = 0; ii < 4; ++ii )
            corners3DU[ii] = SFVEC2F( corners[ii].x * m_biuTo3Dunits,
                                     -corners[ii].y * m_biuTo3Dunits );

        // Add the PAD polygon (For some reason the corners need
        // to be inverted to display with the correctly orientation)
        aDstContainer->Add( new CPOLYGON4PTS2D( corners3DU[0],
                                                corners3DU[3],
                                                corners3DU[2],
                                                corners3DU[1],
                                                *aPad ) );

        // Add the PAD contours
        // !TODO: check the corners because it cannot add
        // roundsegments that are in the same start and end position
        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[0],
                            corners3DU[1],
                            rounding_radius * 2.0f * m_biuTo3Dunits,
                            *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[1],
                                                 corners3DU[2],
                                                 rounding_radius * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[2],
                                                 corners3DU[3],
                                                 rounding_radius * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );

        aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[3],
                                                 corners3DU[0],
                                                 rounding_radius * 2.0f * m_biuTo3Dunits,
                                                 *aPad ) );
    }
    break;

    default:
        wxFAIL_MSG( "CINFO3D_VISU::createNewPadWithClearance - a pad shape type is not implemented" );
        break;
    }
}


// Based on:
// BuildPadDrillShapePolygon
// board_items_to_polygon_shape_transform.cpp
COBJECT2D *CINFO3D_VISU::createNewPadDrill( const D_PAD* aPad, int aInflateValue )
{
    wxSize drillSize = aPad->GetDrillSize();

    if( !drillSize.x || !drillSize.y )
    {
        wxLogTrace( m_logTrace, wxT( "CINFO3D_VISU::createNewPadDrill - found an invalid pad" ) );
        return NULL;
    }

    if( drillSize.x == drillSize.y )    // usual round hole
    {
        const int radius = (drillSize.x / 2) + aInflateValue;

        const SFVEC2F center(  aPad->GetPosition().x * m_biuTo3Dunits,
                              -aPad->GetPosition().y * m_biuTo3Dunits );

        return new CFILLEDCIRCLE2D( center, radius * m_biuTo3Dunits, *aPad );

    }
    else                                // Oblong hole
    {
        wxPoint start, end;
        int width;

        aPad->GetOblongDrillGeometry( start, end, width );

        width += aInflateValue * 2;
        start += aPad->GetPosition();
        end   += aPad->GetPosition();

        SFVEC2F start3DU(  start.x * m_biuTo3Dunits,
                          -start.y * m_biuTo3Dunits );

        SFVEC2F end3DU (  end.x * m_biuTo3Dunits,
                         -end.y * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            return new CFILLEDCIRCLE2D( start3DU, (width / 2) * m_biuTo3Dunits, *aPad );
        }
        else
        {
            return new CROUNDSEGMENT2D( start3DU, end3DU, width * m_biuTo3Dunits, *aPad );
        }
    }

    return NULL;
}


// This function pretends to be like the
// void D_PAD::BuildPadShapePolygon(
// board_items_to_polygon_shape_transform.cpp
void CINFO3D_VISU::createNewPad( const D_PAD* aPad,
                                       CGENERICCONTAINER2D *aDstContainer,
                                       const wxSize &aInflateValue ) const
{
    switch( aPad->GetShape() )
    {
    default:
        wxFAIL_MSG( wxT( "CINFO3D_VISU::createNewPad: found a not implemented pad shape (new shape?)" ) );
        break;

    case PAD_SHAPE_CIRCLE:
    case PAD_SHAPE_OVAL:
    case PAD_SHAPE_ROUNDRECT:
        createNewPadWithClearance( aPad, aDstContainer, aInflateValue.x );
        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
        wxPoint corners[4];
        aPad->BuildPadPolygon( corners, aInflateValue, aPad->GetOrientation() );

        // Note: for pad having a shape offset,
        // the pad position is NOT the shape position
        for( unsigned int ii = 0; ii < 4; ++ii )
            corners[ii] += aPad->ShapePos(); // Shift origin to position

        aDstContainer->Add( new CPOLYGON4PTS2D(
                                SFVEC2F( corners[0].x * m_biuTo3Dunits,
                                        -corners[0].y * m_biuTo3Dunits ),
                                SFVEC2F( corners[1].x * m_biuTo3Dunits,
                                        -corners[1].y * m_biuTo3Dunits ),
                                SFVEC2F( corners[2].x * m_biuTo3Dunits,
                                        -corners[2].y * m_biuTo3Dunits ),
                                SFVEC2F( corners[3].x * m_biuTo3Dunits,
                                        -corners[3].y * m_biuTo3Dunits ),
                                *aPad ) );

        break;
    }
}


void CINFO3D_VISU::AddPadsShapesWithClearanceToContainer( const MODULE* aModule,
                                                          CGENERICCONTAINER2D *aDstContainer,
                                                          LAYER_ID aLayerId,
                                                          int aInflateValue,
                                                          bool aSkipNPTHPadsWihNoCopper )
{
    const D_PAD* pad = aModule->Pads();

    wxSize margin;

    for( ; pad != NULL; pad = pad->Next() )
    {
        if( !pad->IsOnLayer( aLayerId ) )
            continue;

        // NPTH pads are not drawn on layers if the
        // shape size and pos is the same as their hole:
        if( aSkipNPTHPadsWihNoCopper && (pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED) )
        {
            if( (pad->GetDrillSize() == pad->GetSize()) &&
                (pad->GetOffset() == wxPoint( 0, 0 )) )
            {
                switch( pad->GetShape() )
                {
                case PAD_SHAPE_CIRCLE:
                    if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                case PAD_SHAPE_OVAL:
                    if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                default:
                    break;
                }
            }
        }

        switch( aLayerId )
        {
        case F_Mask:
        case B_Mask:
            margin.x = margin.y = pad->GetSolderMaskMargin() + aInflateValue;
            break;

        case F_Paste:
        case B_Paste:
            margin = pad->GetSolderPasteMargin();
            margin.x += aInflateValue;
            margin.y += aInflateValue;
            break;

        default:
            margin.x = margin.y = aInflateValue;
            break;
        }

        createNewPad( pad, aDstContainer, margin );
    }
}

// based on TransformArcToPolygon function from
// common/convert_basic_shapes_to_polygon.cpp
void CINFO3D_VISU::TransformArcToSegments( const wxPoint &aCentre,
                                           const wxPoint &aStart,
                                           double aArcAngle,
                                           int aCircleToSegmentsCount,
                                           int aWidth,
                                           CGENERICCONTAINER2D *aDstContainer,
                                           const BOARD_ITEM &aBoardItem )
{
    wxPoint arc_start, arc_end;
    int     delta = 3600 / aCircleToSegmentsCount;   // rotate angle in 0.1 degree

    arc_end = arc_start = aStart;

    if( aArcAngle != 3600 )
    {
        RotatePoint( &arc_end, aCentre, -aArcAngle );
    }

    if( aArcAngle < 0 )
    {
        std::swap( arc_start, arc_end );
        aArcAngle = -aArcAngle;
    }

    // Compute the ends of segments and creates poly
    wxPoint curr_end    = arc_start;
    wxPoint curr_start  = arc_start;

    for( int ii = delta; ii < aArcAngle; ii += delta )
    {
        curr_end = arc_start;
        RotatePoint( &curr_end, aCentre, -ii );

        const SFVEC2F start3DU( curr_start.x * m_biuTo3Dunits, -curr_start.y * m_biuTo3Dunits );
        const SFVEC2F end3DU  ( curr_end.x   * m_biuTo3Dunits, -curr_end.y   * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                     ( aWidth / 2 ) * m_biuTo3Dunits,
                                                     aBoardItem ) );
        }
        else
        {
            aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                     end3DU,
                                                     aWidth * m_biuTo3Dunits,
                                                     aBoardItem ) );
        }

        curr_start = curr_end;
    }

    if( curr_end != arc_end )
    {
        const SFVEC2F start3DU( curr_end.x * m_biuTo3Dunits, -curr_end.y * m_biuTo3Dunits );
        const SFVEC2F end3DU  ( arc_end.x  * m_biuTo3Dunits, -arc_end.y  * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                     ( aWidth / 2 ) * m_biuTo3Dunits,
                                                     aBoardItem ) );
        }
        else
        {
            aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                     end3DU,
                                                     aWidth * m_biuTo3Dunits,
                                                     aBoardItem ) );
        }
    }
}

// Based on
// TransformShapeWithClearanceToPolygon
// board_items_to_polygon_shape_transform.cpp#L431
void CINFO3D_VISU::AddShapeWithClearanceToContainer( const DRAWSEGMENT* aDrawSegment,
                                                     CGENERICCONTAINER2D *aDstContainer,
                                                     LAYER_ID aLayerId,
                                                     int aClearanceValue )
{
    // The full width of the lines to create:
    const int linewidth = aDrawSegment->GetWidth() + (2 * aClearanceValue);

    switch( aDrawSegment->GetShape() )
    {
    case S_CIRCLE:
    {
        const SFVEC2F center3DU(  aDrawSegment->GetCenter().x * m_biuTo3Dunits,
                                 -aDrawSegment->GetCenter().y * m_biuTo3Dunits );

        const float inner_radius  = (aDrawSegment->GetRadius() - linewidth / 2) * m_biuTo3Dunits;
        const float outter_radius = (aDrawSegment->GetRadius() + linewidth / 2) * m_biuTo3Dunits;

        aDstContainer->Add( new CRING2D( center3DU,
                                         inner_radius,
                                         outter_radius,
                                         *aDrawSegment ) );
    }
    break;

    case S_ARC:
    {
        const unsigned int nr_segments =
                GetNrSegmentsCircle( aDrawSegment->GetBoundingBox().GetSizeMax() );

        TransformArcToSegments( aDrawSegment->GetCenter(),
                                aDrawSegment->GetArcStart(),
                                aDrawSegment->GetAngle(),
                                nr_segments,
                                aDrawSegment->GetWidth(),
                                aDstContainer,
                                *aDrawSegment );
    }
    break;

    case S_SEGMENT:
    {
        const SFVEC2F start3DU(  aDrawSegment->GetStart().x  * m_biuTo3Dunits,
                                -aDrawSegment->GetStart().y * m_biuTo3Dunits );

        const SFVEC2F end3DU  (  aDrawSegment->GetEnd().x    * m_biuTo3Dunits,
                                -aDrawSegment->GetEnd().y   * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                     ( linewidth / 2 ) * m_biuTo3Dunits,
                                                     *aDrawSegment ) );
        }
        else
        {
            aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                     end3DU,
                                                     linewidth * m_biuTo3Dunits,
                                                     *aDrawSegment ) );
        }
    }
    break;

    case S_POLYGON:
    {
         // Check for malformed polygon.
        if( aDrawSegment->GetPolyPoints().size() > 2 )
        {
            // The polygon is expected to be a simple polygon
            // not self intersecting, no hole.
            MODULE* module = aDrawSegment->GetParentModule(); // NULL for items not in footprints
            const double orientation = module ? module->GetOrientation() : 0.0;

            // Build the polygon with the actual position and orientation:
            std::vector< wxPoint> poly;
            poly = aDrawSegment->GetPolyPoints();

            for( unsigned ii = 0; ii < poly.size(); ++ii )
            {
                RotatePoint( &poly[ii], orientation );
                poly[ii] += aDrawSegment->GetPosition();
            }

            // Generate polygons for the outline + clearance

            if( linewidth ) // Add thick outlines
            {
                CPolyPt corner1( poly[poly.size()-1] );

                for( unsigned ii = 0; ii < poly.size(); ++ii )
                {
                    CPolyPt corner2( poly[ii] );

                    if( corner2 != corner1 )
                    {
                        const SFVEC2F start3DU(  corner1.x * m_biuTo3Dunits,
                                                -corner1.y * m_biuTo3Dunits );

                        const SFVEC2F end3DU(    corner2.x * m_biuTo3Dunits,
                                                -corner2.y * m_biuTo3Dunits );

                        if( Is_segment_a_circle( start3DU, end3DU ) )
                        {
                            aDstContainer->Add(
                                        new CFILLEDCIRCLE2D( start3DU,
                                                             (linewidth / 2) * m_biuTo3Dunits,
                                                             *aDrawSegment ) );
                        }
                        else
                        {
                            aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                                     end3DU,
                                                                     linewidth * m_biuTo3Dunits,
                                                                     *aDrawSegment ) );
                        }
                    }

                    corner1 = corner2;
                }
            }

            // Polygon for the inside
            SHAPE_LINE_CHAIN path;

            for( unsigned ii = 0; ii < poly.size(); ++ii )
            {
                wxPoint corner = poly[ii];
                path.Append( corner.x, corner.y );
            }

            path.SetClosed( true );

            SHAPE_POLY_SET polyList;

            polyList.AddOutline( path );

            // This convert the poly in outline and holes
            polyList.Simplify( SHAPE_POLY_SET::PM_FAST );
            polyList.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            if( polyList.IsEmpty() ) // Just for caution
                break;

            Convert_shape_line_polygon_to_triangles( polyList,
                                                     *aDstContainer,
                                                     m_biuTo3Dunits,
                                                     *aDrawSegment );
        }
    }
    break;

    case S_CURVE:       // Bezier curve (not yet in use in KiCad)
        break;

    default:
        break;
    }
}


// Based on
// TransformSolidAreasShapesToPolygonSet
// board_items_to_polygon_shape_transform.cpp
void CINFO3D_VISU::AddSolidAreasShapesToContainer( const ZONE_CONTAINER* aZoneContainer,
                                                   CGENERICCONTAINER2D *aDstContainer,
                                                   LAYER_ID aLayerId )
{
    // Copy the polys list because we have to simplify it
    SHAPE_POLY_SET polyList = SHAPE_POLY_SET(aZoneContainer->GetFilledPolysList());

    // This convert the poly in outline and holes

    // Note: This two sequencial calls are need in order to get
    // the triangulation function to work properly.
    polyList.Simplify( SHAPE_POLY_SET::PM_FAST );
    polyList.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    if( polyList.IsEmpty() )
        return;

    Convert_shape_line_polygon_to_triangles( polyList,
                                             *aDstContainer,
                                             m_biuTo3Dunits,
                                             *aZoneContainer );


    // add filled areas outlines, which are drawn with thick lines segments
    // /////////////////////////////////////////////////////////////////////////
    for( int i = 0; i < polyList.OutlineCount(); ++i )
    {
        // Add outline
        const SHAPE_LINE_CHAIN& pathOutline = polyList.COutline( i );

        for( int j = 0; j < pathOutline.PointCount(); ++j )
        {
            const VECTOR2I& a = pathOutline.CPoint( j );
            const VECTOR2I& b = pathOutline.CPoint( j + 1 );

            SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
            SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

            if( Is_segment_a_circle( start3DU, end3DU ) )
            {
                aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                         (aZoneContainer->GetMinThickness() / 2) *
                                                         m_biuTo3Dunits,
                                                         *aZoneContainer ) );
            }
            else
            {
                aDstContainer->Add( new CROUNDSEGMENT2D( start3DU, end3DU,
                                                         aZoneContainer->GetMinThickness() *
                                                         m_biuTo3Dunits,
                                                         *aZoneContainer ) );
            }
        }

        // Add holes (of the poly, ie: the open parts) for this outline
        for( int h = 0; h < polyList.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN& pathHole = polyList.CHole( i, h );

            for( int j = 0; j < pathHole.PointCount(); j++ )
            {
                const VECTOR2I& a = pathHole.CPoint( j );
                const VECTOR2I& b = pathHole.CPoint( j + 1 );

                SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
                SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

                if( Is_segment_a_circle( start3DU, end3DU ) )
                {
                    aDstContainer->Add(
                                new CFILLEDCIRCLE2D( start3DU,
                                                     (aZoneContainer->GetMinThickness() / 2) *
                                                     m_biuTo3Dunits,
                                                     *aZoneContainer ) );
                }
                else
                {
                    aDstContainer->Add(
                                new CROUNDSEGMENT2D( start3DU, end3DU,
                                                     aZoneContainer->GetMinThickness() *
                                                     m_biuTo3Dunits,
                                                     *aZoneContainer ) );
                }
            }
        }
    }
}



void CINFO3D_VISU::buildPadShapeThickOutlineAsSegments( const D_PAD*  aPad,
                                                        CGENERICCONTAINER2D *aDstContainer,
                                                        int aWidth )
{
    if( aPad->GetShape() == PAD_SHAPE_CIRCLE )    // Draw a ring
    {
        const SFVEC2F center3DU(  aPad->ShapePos().x * m_biuTo3Dunits,
                                 -aPad->ShapePos().y * m_biuTo3Dunits );

        const int radius = aPad->GetSize().x / 2;
        const float inner_radius  = (radius - aWidth / 2) * m_biuTo3Dunits;
        const float outter_radius = (radius + aWidth / 2) * m_biuTo3Dunits;

        aDstContainer->Add( new CRING2D( center3DU,
                                         inner_radius,
                                         outter_radius,
                                         *aPad ) );

        return;
    }

    // For other shapes, draw polygon outlines
    SHAPE_POLY_SET corners;

    const int segcountforcircle = GetNrSegmentsCircle( glm::min( aPad->GetSize().x,
                                                                 aPad->GetSize().y) );

    const double correctionFactor = GetCircleCorrectionFactor( segcountforcircle );

    aPad->BuildPadShapePolygon( corners, wxSize( 0, 0 ),
                                // This two factors are only expected to be used if render an oval
                                segcountforcircle, correctionFactor );


    // Add outlines as thick segments in polygon buffer

    const SHAPE_LINE_CHAIN& path = corners.COutline( 0 );

    for( int j = 0; j < path.PointCount(); j++ )
    {
        const VECTOR2I& a = path.CPoint( j );
        const VECTOR2I& b = path.CPoint( j + 1 );

        SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
        SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                     (aWidth / 2) * m_biuTo3Dunits,
                                                     *aPad ) );
        }
        else
        {
            aDstContainer->Add( new CROUNDSEGMENT2D( start3DU, end3DU,
                                                     aWidth * m_biuTo3Dunits,
                                                     *aPad ) );
        }
    }
}


void CINFO3D_VISU::destroyLayers()
{
    if( !m_layers_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_poly.begin();
             ii != m_layers_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_poly.clear();
    }

    if( !m_layers_inner_holes_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_inner_holes_poly.begin();
             ii != m_layers_inner_holes_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_inner_holes_poly.clear();
    }

    if( !m_layers_outer_holes_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_outer_holes_poly.begin();
             ii != m_layers_outer_holes_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_outer_holes_poly.clear();
    }

    if( !m_layers_container2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_container2D.begin();
             ii != m_layers_container2D.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_container2D.clear();
    }

    if( !m_layers_holes2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_holes2D.begin();
             ii != m_layers_holes2D.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_holes2D.clear();
    }

    m_through_holes_inner.Clear();
    m_through_holes_outer.Clear();
    m_through_holes_vias_outer.Clear();
    m_through_holes_vias_inner.Clear();
    m_through_outer_holes_poly_NPTH.RemoveAllContours();
    m_through_outer_holes_poly.RemoveAllContours();
    //m_through_inner_holes_poly.RemoveAllContours();

    m_through_outer_holes_vias_poly.RemoveAllContours();
    m_through_inner_holes_vias_poly.RemoveAllContours();
}


void CINFO3D_VISU::createLayers( REPORTER *aStatusTextReporter )
{
    // Number of segments to draw a circle using segments (used on countour zones
    // and text copper elements )
    const int    segcountforcircle = 12;
    const double correctionFactor  = GetCircleCorrectionFactor( segcountforcircle );

    // segments to draw a circle to build texts. Is is used only to build
    // the shape of each segment of the stroke font, therefore no need to have
    // many segments per circle.
    const int segcountInStrokeFont  = 12;
    const double correctionFactorStroke = GetCircleCorrectionFactor( segcountInStrokeFont );

    destroyLayers();

    // Build Copper layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L692
    // /////////////////////////////////////////////////////////////////////////

    #ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCopperLayersTime = GetRunningMicroSecs();

    unsigned start_Time = stats_startCopperLayersTime;
#endif

    LAYER_ID cu_seq[MAX_CU_LAYERS];
    LSET     cu_set = LSET::AllCuMask( m_copperLayersCount );

    m_stats_nr_tracks               = 0;
    m_stats_track_med_width         = 0;
    m_stats_nr_vias                 = 0;
    m_stats_via_med_hole_diameter   = 0;
    m_stats_nr_holes                = 0;
    m_stats_hole_med_diameter       = 0;

    // Prepare track list, convert in a vector. Calc statistic for the holes
    // /////////////////////////////////////////////////////////////////////////
    std::vector< const TRACK *> trackList;
    trackList.clear();
    trackList.reserve( m_board->m_Track.GetCount() );

    for( const TRACK* track = m_board->m_Track; track; track = track->Next() )
    {
        if( !Is3DLayerEnabled( track->GetLayer() ) ) // Skip non enabled layers
            continue;

        // Note: a TRACK holds normal segment tracks and
        // also vias circles (that have also drill values)
        trackList.push_back( track );

        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast< const VIA*>( track );
            m_stats_nr_vias++;
            m_stats_via_med_hole_diameter += via->GetDrillValue() * m_biuTo3Dunits;
        }
        else
        {
            m_stats_nr_tracks++;
        }

        m_stats_track_med_width += track->GetWidth() * m_biuTo3Dunits;
    }

    if( m_stats_nr_tracks )
        m_stats_track_med_width /= (float)m_stats_nr_tracks;

    if( m_stats_nr_vias )
        m_stats_via_med_hole_diameter /= (float)m_stats_nr_vias;

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T01: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Prepare copper layers index and containers
    // /////////////////////////////////////////////////////////////////////////
    std::vector< LAYER_ID > layer_id;
    layer_id.clear();
    layer_id.reserve( m_copperLayersCount );

    for( unsigned i = 0; i < DIM( cu_seq ); ++i )
        cu_seq[i] = ToLAYER_ID( B_Cu - i );

    for( LSEQ cu = cu_set.Seq( cu_seq, DIM( cu_seq ) ); cu; ++cu )
    {
        const LAYER_ID curr_layer_id = *cu;

        if( !Is3DLayerEnabled( curr_layer_id ) ) // Skip non enabled layers
            continue;

        layer_id.push_back( curr_layer_id );

        CBVHCONTAINER2D *layerContainer = new CBVHCONTAINER2D;
        m_layers_container2D[curr_layer_id] = layerContainer;

        if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
            (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
        {
            SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
            m_layers_poly[curr_layer_id] = layerPoly;
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T02: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Create tracks and vias" ) );

    // Create tracks as objects and add it to container
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD TRACKS
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            // NOTE: Vias can be on multiple layers
            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // Add object item to layer container
            layerContainer->Add( createNewTrack( track, 0.0f ) );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T03: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        // ADD TRACKS
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast< const VIA*>( track );
                const VIATYPE_T viatype = via->GetViaType();
                const float holediameter = via->GetDrillValue() * BiuTo3Dunits();
                const float thickness = GetCopperThickness3DU();
                const float hole_inner_radius = ( holediameter / 2.0f );

                const SFVEC2F via_center(  via->GetStart().x * m_biuTo3Dunits,
                                          -via->GetStart().y * m_biuTo3Dunits );

                if( viatype != VIA_THROUGH )
                {

                    // Add hole objects
                    // /////////////////////////////////////////////////////////

                    CBVHCONTAINER2D *layerHoleContainer = NULL;

                    // Check if the layer is already created
                    if( m_layers_holes2D.find( curr_layer_id ) == m_layers_holes2D.end() )
                    {
                        // not found, create a new container
                        layerHoleContainer = new CBVHCONTAINER2D;
                        m_layers_holes2D[curr_layer_id] = layerHoleContainer;
                    }
                    else
                    {
                        // found
                        layerHoleContainer = m_layers_holes2D[curr_layer_id];
                    }

                    // Add a hole for this layer
                    layerHoleContainer->Add( new CFILLEDCIRCLE2D( via_center,
                                                                  hole_inner_radius + thickness,
                                                                  *track ) );
                }
                else if( lIdx == 0 ) // it only adds once the THT holes
                {
                    // Add through hole object
                    // /////////////////////////////////////////////////////////
                    m_through_holes_outer.Add( new CFILLEDCIRCLE2D( via_center,
                                                                    hole_inner_radius + thickness,
                                                                    *track ) );

                    m_through_holes_vias_outer.Add(
                                new CFILLEDCIRCLE2D( via_center,
                                                     hole_inner_radius + thickness,
                                                     *track ) );

                    m_through_holes_inner.Add( new CFILLEDCIRCLE2D( via_center,
                                                                    hole_inner_radius,
                                                                    *track ) );

                    //m_through_holes_vias_inner.Add( new CFILLEDCIRCLE2D( via_center,
                    //                                                     hole_inner_radius,
                    //                                                     *track ) );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T04: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        // ADD TRACKS
        const unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast< const VIA*>( track );
                const VIATYPE_T viatype = via->GetViaType();

                if( viatype != VIA_THROUGH )
                {

                    // Add VIA hole contourns
                    // /////////////////////////////////////////////////////////

                    // Add outter holes of VIAs
                    SHAPE_POLY_SET *layerOuterHolesPoly = NULL;
                    SHAPE_POLY_SET *layerInnerHolesPoly = NULL;

                    // Check if the layer is already created
                    if( m_layers_outer_holes_poly.find( curr_layer_id ) ==
                        m_layers_outer_holes_poly.end() )
                    {
                        // not found, create a new container
                        layerOuterHolesPoly = new SHAPE_POLY_SET;
                        m_layers_outer_holes_poly[curr_layer_id] = layerOuterHolesPoly;

                        wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) ==
                                  m_layers_inner_holes_poly.end() );

                        layerInnerHolesPoly = new SHAPE_POLY_SET;
                        m_layers_inner_holes_poly[curr_layer_id] = layerInnerHolesPoly;
                    }
                    else
                    {
                        // found
                        layerOuterHolesPoly = m_layers_outer_holes_poly[curr_layer_id];

                        wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) !=
                                  m_layers_inner_holes_poly.end() );

                        layerInnerHolesPoly = m_layers_inner_holes_poly[curr_layer_id];
                    }

                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2) + GetCopperThicknessBIU();

                    TransformCircleToPolygon( *layerOuterHolesPoly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    TransformCircleToPolygon( *layerInnerHolesPoly,
                                              via->GetStart(),
                                              holediameter / 2,
                                              GetNrSegmentsCircle( holediameter ) );
                }
                else if( lIdx == 0 ) // it only adds once the THT holes
                {
                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2)+ GetCopperThicknessBIU();

                    // Add through hole contourns
                    // /////////////////////////////////////////////////////////
                    TransformCircleToPolygon( m_through_outer_holes_poly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    TransformCircleToPolygon( m_through_inner_holes_poly,
                                              via->GetStart(),
                                              holediameter / 2,
                                              GetNrSegmentsCircle( holediameter ) );

                    // Add samething for vias only

                    TransformCircleToPolygon( m_through_outer_holes_vias_poly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    //TransformCircleToPolygon( m_through_inner_holes_vias_poly,
                    //                          via->GetStart(),
                    //                          holediameter / 2,
                    //                          GetNrSegmentsCircle( holediameter ) );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T05: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Creates outline contours of the tracks and add it to the poly of the layer
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD TRACKS
            unsigned int nTracks = trackList.size();

            for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
            {
                const TRACK *track = trackList[trackIdx];

                if( !track->IsOnLayer( curr_layer_id ) )
                    continue;

                // Add the track contour
                int nrSegments = GetNrSegmentsCircle( track->GetWidth() );

                track->TransformShapeWithClearanceToPolygon(
                            *layerPoly,
                            0,
                            nrSegments,
                            GetCircleCorrectionFactor( nrSegments ) );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T06: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add holes of modules
    // /////////////////////////////////////////////////////////////////////////
    for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        const D_PAD* pad = module->Pads();

        for( ; pad; pad = pad->Next() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x )    // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness,
            // if not plated, no copper
            const int inflate = (pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED) ?
                                GetCopperThicknessBIU() : 0;

            m_stats_nr_holes++;
            m_stats_hole_med_diameter += ( ( pad->GetDrillSize().x +
                                             pad->GetDrillSize().y ) / 2.0f ) * m_biuTo3Dunits;

            m_through_holes_outer.Add( createNewPadDrill( pad, inflate ) );
            m_through_holes_inner.Add( createNewPadDrill( pad,       0 ) );
        }
    }
    if( m_stats_nr_holes )
        m_stats_hole_med_diameter /= (float)m_stats_nr_holes;

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T07: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add contours of the pad holes (pads can be Circle or Segment holes)
    // /////////////////////////////////////////////////////////////////////////
    for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        const D_PAD* pad = module->Pads();

        for( ; pad; pad = pad->Next() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x ) // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness.
            const int inflate = GetCopperThicknessBIU();

            // we use the hole diameter to calculate the seg count.
            // for round holes, padHole.x == padHole.y
            // for oblong holes, the diameter is the smaller of (padHole.x, padHole.y)
            const int diam = std::min( padHole.x, padHole.y );


            if( pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                pad->BuildPadDrillShapePolygon( m_through_outer_holes_poly,
                                                inflate,
                                                GetNrSegmentsCircle( diam ) );

                pad->BuildPadDrillShapePolygon( m_through_inner_holes_poly,
                                                0,
                                                GetNrSegmentsCircle( diam ) );
            }
            else
            {
                // If not plated, no copper.
                pad->BuildPadDrillShapePolygon( m_through_outer_holes_poly_NPTH,
                                                inflate,
                                                GetNrSegmentsCircle( diam ) );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T08: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add modules PADs objects to containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD PADS
        for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            // Note: NPTH pads are not drawn on copper layers when the pad
            // has same shape as its hole
            AddPadsShapesWithClearanceToContainer( module,
                                                   layerContainer,
                                                   curr_layer_id,
                                                   0,
                                                   true );

            // Micro-wave modules may have items on copper layers
            AddGraphicsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0 );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T09: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add modules PADs poly contourns
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD PADS
            for( const MODULE* module = m_board->m_Modules;
                 module;
                 module = module->Next() )
            {
                // Construct polys
                // /////////////////////////////////////////////////////////////

                // Note: NPTH pads are not drawn on copper layers when the pad
                // has same shape as its hole
                transformPadsShapesWithClearanceToPolygon( module->Pads(),
                                                           curr_layer_id,
                                                           *layerPoly,
                                                           0,
                                                           true );

                // Micro-wave modules may have items on copper layers
                module->TransformGraphicTextWithClearanceToPolygonSet( curr_layer_id,
                                                                        *layerPoly,
                                                                        0,
                                                                        segcountforcircle,
                                                                        correctionFactor );

                transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T10: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add graphic item on copper layers to object containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD GRAPHIC ITEMS ON COPPER LAYERS (texts)
        for( const BOARD_ITEM* item = m_board->m_Drawings;
             item;
             item = item->Next() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:  // should not exist on copper layers
            {
                AddShapeWithClearanceToContainer( (DRAWSEGMENT*)item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
            }
            break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (TEXTE_PCB*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
            break;

            default:
                wxLogTrace( m_logTrace,
                            wxT( "createLayers: item type: %d not implemented" ),
                            item->Type() );
            break;
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T11: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add graphic item on copper layers to poly contourns
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD GRAPHIC ITEMS ON COPPER LAYERS (texts)
            for( const BOARD_ITEM* item = m_board->m_Drawings;
                 item;
                 item = item->Next() )
            {
                if( !item->IsOnLayer( curr_layer_id ) )
                    continue;

                switch( item->Type() )
                {
                case PCB_LINE_T: // should not exist on copper layers
                {
                    const int nrSegments =
                            GetNrSegmentsCircle( item->GetBoundingBox().GetSizeMax() );

                    ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                                *layerPoly,
                                0,
                                nrSegments,
                                GetCircleCorrectionFactor( nrSegments ) );
                }
                break;

                case PCB_TEXT_T:
                    ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                                *layerPoly,
                                0,
                                segcountforcircle,
                                correctionFactor );
                break;

                default:
                    wxLogTrace( m_logTrace,
                                wxT( "createLayers: item type: %d not implemented" ),
                                item->Type() );
                break;
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T12: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( GetFlag( FL_ZONE ) )
    {
        if( aStatusTextReporter )
            aStatusTextReporter->Report( _( "Create zones" ) );

        // Add zones objects
        // /////////////////////////////////////////////////////////////////////
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            if( aStatusTextReporter )
                aStatusTextReporter->Report( wxString::Format( _( "Create zones of layer %s" ),
                                                               LSET::Name( curr_layer_id ) ) );

            wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

            CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

            // ADD COPPER ZONES
            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                const ZONE_CONTAINER* zone = m_board->GetArea( ii );
                const LAYER_ID zonelayer = zone->GetLayer();

                if( zonelayer == curr_layer_id )
                {
                    AddSolidAreasShapesToContainer( zone,
                                                    layerContainer,
                                                    curr_layer_id );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T13: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( GetFlag( FL_ZONE ) &&
        GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        // Add zones poly contourns
        // /////////////////////////////////////////////////////////////////////
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD COPPER ZONES
            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                const ZONE_CONTAINER* zone = m_board->GetArea( ii );
                const LAYER_NUM zonelayer = zone->GetLayer();

                if( zonelayer == curr_layer_id )
                {
                    zone->TransformSolidAreasShapesToPolygonSet( *layerPoly,
                                                                 segcountforcircle,
                                                                 correctionFactor );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T14: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Simplify layer polygons
    // /////////////////////////////////////////////////////////////////////////

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Simplifying polygons" ) );

    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        const int nLayers = layer_id.size();

        #pragma omp parallel for
        for( signed int lIdx = 0; lIdx < nLayers; ++lIdx )
        {
            const LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            wxASSERT( layerPoly != NULL );

            // This will make a union of all added contourns
            layerPoly->Simplify( SHAPE_POLY_SET::PM_FAST );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T15: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Simplify holes polygon contours
    // /////////////////////////////////////////////////////////////////////////
    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Simplify holes contours" ) );

    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const LAYER_ID curr_layer_id = layer_id[lIdx];

        if( m_layers_outer_holes_poly.find( curr_layer_id ) !=
            m_layers_outer_holes_poly.end() )
        {
            // found
            SHAPE_POLY_SET *polyLayer = m_layers_outer_holes_poly[curr_layer_id];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );

            wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) !=
                      m_layers_inner_holes_poly.end() );

            polyLayer = m_layers_inner_holes_poly[curr_layer_id];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T16: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
#endif
    // End Build Copper layers


    // This will make a union of all added contourns
    m_through_inner_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_poly_NPTH.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_vias_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    //m_through_inner_holes_vias_poly.Simplify( SHAPE_POLY_SET::PM_FAST ); // Not in use

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endCopperLayersTime = GetRunningMicroSecs();
#endif


    // Build Tech layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L1059
    // /////////////////////////////////////////////////////////////////////////
#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startTechLayersTime = GetRunningMicroSecs();
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Build Tech layers" ) );

    // draw graphic items, on technical layers
    static const LAYER_ID teckLayerList[] = {
            B_Adhes,
            F_Adhes,
            B_Paste,
            F_Paste,
            B_SilkS,
            F_SilkS,
            B_Mask,
            F_Mask,

            // Aux Layers
            Dwgs_User,
            Cmts_User,
            Eco1_User,
            Eco2_User,
            Edge_Cuts,
            Margin
        };

    // User layers are not drawn here, only technical layers
    for( LSEQ seq = LSET::AllNonCuMask().Seq( teckLayerList, DIM( teckLayerList ) );
         seq;
         ++seq )
    {
        const LAYER_ID curr_layer_id = *seq;

        if( !Is3DLayerEnabled( curr_layer_id ) )
                    continue;

        CBVHCONTAINER2D *layerContainer = new CBVHCONTAINER2D;
        m_layers_container2D[curr_layer_id] = layerContainer;

        SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
        m_layers_poly[curr_layer_id] = layerPoly;

        // Add drawing objects
        // /////////////////////////////////////////////////////////////////////
        for( BOARD_ITEM* item = m_board->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                AddShapeWithClearanceToContainer( (DRAWSEGMENT*)item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (TEXTE_PCB*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            default:
                break;
            }
        }


        // Add drawing contours
        // /////////////////////////////////////////////////////////////////////
        for( BOARD_ITEM* item = m_board->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
            {
                const unsigned int nr_segments =
                        GetNrSegmentsCircle( item->GetBoundingBox().GetSizeMax() );

                ((DRAWSEGMENT*) item)->TransformShapeWithClearanceToPolygon( *layerPoly,
                                                                             0,
                                                                             nr_segments,
                                                                             0.0 );
            }
                break;

            case PCB_TEXT_T:
                ((TEXTE_PCB*) item)->TransformShapeWithClearanceToPolygonSet( *layerPoly,
                                                                              0,
                                                                              segcountInStrokeFont,
                                                                              1.0 );
                break;

            default:
                break;
            }
        }


        // Add modules tech layers - objects
        // /////////////////////////////////////////////////////////////////////
        for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                D_PAD*  pad = module->Pads();
                int     linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsSegments( pad,
                                                         layerContainer,
                                                         linewidth );
                }
            }
            else
            {
                AddPadsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0,
                                                       false );
            }

            AddGraphicsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0 );
        }


        // Add modules tech layers - contours
        // /////////////////////////////////////////////////////////////////////
        for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                D_PAD*  pad = module->Pads();
                const int linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsPolygon( pad, *layerPoly, linewidth );
                }
            }
            else
            {
                transformPadsShapesWithClearanceToPolygon( module->Pads(),
                                                           curr_layer_id,
                                                           *layerPoly,
                                                           0,
                                                           false );
            }

            // On tech layers, use a poor circle approximation, only for texts (stroke font)
            module->TransformGraphicTextWithClearanceToPolygonSet( curr_layer_id,
                                                                   *layerPoly,
                                                                   0,
                                                                   segcountInStrokeFont,
                                                                   correctionFactorStroke,
                                                                   segcountInStrokeFont );

            // Add the remaining things with dynamic seg count for circles
            transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
        }


        // Draw non copper zones
        // /////////////////////////////////////////////////////////////////////
        if( GetFlag( FL_ZONE ) )
        {
            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                ZONE_CONTAINER* zone = m_board->GetArea( ii );

                if( !zone->IsOnLayer( curr_layer_id ) )
                    continue;

                AddSolidAreasShapesToContainer( zone,
                                                layerContainer,
                                                curr_layer_id );
            }

            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                ZONE_CONTAINER* zone = m_board->GetArea( ii );

                if( !zone->IsOnLayer( curr_layer_id ) )
                    continue;

                zone->TransformSolidAreasShapesToPolygonSet( *layerPoly,
                                                             // Use the same segcount as stroke font
                                                             segcountInStrokeFont,
                                                             correctionFactorStroke );
            }
        }

        // This will make a union of all added contourns
        layerPoly->Simplify( SHAPE_POLY_SET::PM_FAST );
    }
    // End Build Tech layers

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endTechLayersTime = GetRunningMicroSecs();
#endif


    // Build BVH for holes and vias
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startHolesBVHTime = GetRunningMicroSecs();
#endif

    m_through_holes_inner.BuildBVH();
    m_through_holes_outer.BuildBVH();

    if( !m_layers_holes2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_holes2D.begin();
             ii != m_layers_holes2D.end();
             ++ii )
        {
            ((CBVHCONTAINER2D *)(ii->second))->BuildBVH();
        }
    }

    // We only need the Solder mask to initialize the BVH
    // because..?
    if( (CBVHCONTAINER2D *)m_layers_container2D[B_Mask] )
        ((CBVHCONTAINER2D *)m_layers_container2D[B_Mask])->BuildBVH();

    if( (CBVHCONTAINER2D *)m_layers_container2D[F_Mask] )
        ((CBVHCONTAINER2D *)m_layers_container2D[F_Mask])->BuildBVH();

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endHolesBVHTime = GetRunningMicroSecs();

    printf( "CINFO3D_VISU::createLayers times\n" );
    printf( "  Copper Layers:          %.3f ms\n",
            (float)( stats_endCopperLayersTime  - stats_startCopperLayersTime  ) / 1e3 );
    printf( "  Holes BVH creation:     %.3f ms\n",
            (float)( stats_endHolesBVHTime      - stats_startHolesBVHTime      ) / 1e3 );
    printf( "  Tech Layers:            %.3f ms\n",
            (float)( stats_endTechLayersTime    - stats_startTechLayersTime    ) / 1e3 );
    printf( "Statistics:\n" );
    printf( "  m_stats_nr_tracks                   %u\n", m_stats_nr_tracks );
    printf( "  m_stats_nr_vias                     %u\n", m_stats_nr_vias );
    printf( "  m_stats_nr_holes                    %u\n", m_stats_nr_holes );
    printf( "  m_stats_via_med_hole_diameter (3DU) %f\n", m_stats_via_med_hole_diameter );
    printf( "  m_stats_hole_med_diameter     (3DU) %f\n", m_stats_hole_med_diameter );
    printf( "  m_calc_seg_min_factor3DU      (3DU) %f\n", m_calc_seg_min_factor3DU );
    printf( "  m_calc_seg_max_factor3DU      (3DU) %f\n", m_calc_seg_max_factor3DU );
#endif
}
