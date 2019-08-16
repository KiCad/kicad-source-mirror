/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  create_graphic_brd_items.cpp
 * @brief This file implements the creation of 2D graphic primitives of pcb items:
 *  pads, tracks, drawsegments, texts....
 * It is based on the function found in the files:
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

#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_pcb_text.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_text_mod.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <gr_text.h>
#include <utility>
#include <vector>



// These variables are parameters used in addTextSegmToContainer.
// But addTextSegmToContainer is a call-back function,
// so we cannot send them as arguments.
static int s_textWidth;
static CGENERICCONTAINER2D *s_dstcontainer = NULL;
static float s_biuTo3Dunits;
static const BOARD_ITEM *s_boardItem = NULL;

// This is a call back function, used by GRText to draw the 3D text shape:
void addTextSegmToContainer( int x0, int y0, int xf, int yf, void* aData )
{
    wxASSERT( s_dstcontainer != NULL );

    const SFVEC2F start3DU( x0 * s_biuTo3Dunits, -y0 * s_biuTo3Dunits );
    const SFVEC2F end3DU  ( xf * s_biuTo3Dunits, -yf * s_biuTo3Dunits );

    if( Is_segment_a_circle( start3DU, end3DU ) )
        s_dstcontainer->Add( new CFILLEDCIRCLE2D( start3DU,
                                                  ( s_textWidth / 2 ) * s_biuTo3Dunits,
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
void CINFO3D_VISU::AddShapeWithClearanceToContainer( const TEXTE_PCB* aText,
                                                     CGENERICCONTAINER2D *aDstContainer,
                                                     PCB_LAYER_ID aLayerId,
                                                     int aClearanceValue )
{
    wxSize size = aText->GetTextSize();

    if( aText->IsMirrored() )
        size.x = -size.x;

    s_boardItem    = (const BOARD_ITEM *) &aText;
    s_dstcontainer = aDstContainer;
    s_textWidth    = aText->GetThickness() + ( 2 * aClearanceValue );
    s_biuTo3Dunits = m_biuTo3Dunits;

    // not actually used, but needed by GRText
    const COLOR4D dummy_color = COLOR4D::BLACK;

    if( aText->IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( aText->GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        aText->GetPositionsOfLinesOfMultilineText( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ++ii )
        {
            wxString txt = strings_list.Item( ii );

            GRText( NULL, positions[ii], dummy_color, txt, aText->GetTextAngle(), size,
                    aText->GetHorizJustify(), aText->GetVertJustify(), aText->GetThickness(),
                    aText->IsItalic(), true, addTextSegmToContainer );
        }
    }
    else
    {
        GRText( NULL, aText->GetTextPos(), dummy_color, aText->GetShownText(),
                aText->GetTextAngle(), size, aText->GetHorizJustify(), aText->GetVertJustify(),
                aText->GetThickness(), aText->IsItalic(), true, addTextSegmToContainer );
    }
}


void CINFO3D_VISU::AddShapeWithClearanceToContainer( const DIMENSION* aDimension,
                                                     CGENERICCONTAINER2D *aDstContainer,
                                                     PCB_LAYER_ID aLayerId,
                                                     int aClearanceValue )
{
    AddShapeWithClearanceToContainer(&aDimension->Text(), aDstContainer, aLayerId, aClearanceValue);

    const int linewidth = aDimension->GetWidth() + (2 * aClearanceValue);

    std::pair<wxPoint const *, wxPoint const *> segs[] = {
        {&aDimension->m_crossBarO,     &aDimension->m_crossBarF},
        {&aDimension->m_featureLineGO, &aDimension->m_featureLineGF},
        {&aDimension->m_featureLineDO, &aDimension->m_featureLineDF},
        {&aDimension->m_crossBarF,     &aDimension->m_arrowD1F},
        {&aDimension->m_crossBarF,     &aDimension->m_arrowD2F},
        {&aDimension->m_crossBarO,     &aDimension->m_arrowG1F},
        {&aDimension->m_crossBarO,     &aDimension->m_arrowG2F}};

    for( auto const & ii : segs )
    {
        const SFVEC2F start3DU(  ii.first->x * m_biuTo3Dunits,
                                -ii.first->y * m_biuTo3Dunits );

        const SFVEC2F end3DU  (  ii.second->x * m_biuTo3Dunits,
                                -ii.second->y * m_biuTo3Dunits );

        aDstContainer->Add( new CROUNDSEGMENT2D( start3DU,
                                                 end3DU,
                                                 linewidth * m_biuTo3Dunits,
                                                 *aDimension ) );
    }
}


// Based on
// void MODULE::TransformGraphicShapesWithClearanceToPolygonSet
// board_items_to_polygon_shape_transform.cpp#L204
void CINFO3D_VISU::AddGraphicsShapesWithClearanceToContainer( const MODULE* aModule,
                                                              CGENERICCONTAINER2D *aDstContainer,
                                                              PCB_LAYER_ID aLayerId,
                                                              int aInflateValue )
{
    std::vector<TEXTE_MODULE *> texts;  // List of TEXTE_MODULE to convert
    EDGE_MODULE* outline;

    for( auto item : aModule->GraphicalItems() )
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

    for( TEXTE_MODULE* text : texts )
    {
        s_textWidth = text->GetThickness() + ( 2 * aInflateValue );
        wxSize size = text->GetTextSize();

        if( text->IsMirrored() )
            size.x = -size.x;

        GRText( NULL, text->GetTextPos(), BLACK, text->GetShownText(), text->GetDrawRotation(),
                size, text->GetHorizJustify(), text->GetVertJustify(), text->GetThickness(),
                text->IsItalic(), true, addTextSegmToContainer );
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
                                              wxSize aClearanceValue ) const
{
    // note: for most of shapes, aClearanceValue.x = aClearanceValue.y
    // only rectangular and oval shapes can have different values
    // when drawn on the solder paste layer, because we can have a margin that is a
    // percent of pad size
    const int dx = (aPad->GetSize().x / 2) + aClearanceValue.x;
    const int dy = (aPad->GetSize().y / 2) + aClearanceValue.y;

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
        if( dx == dy )
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
            else    //if( dy < dx )
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
                aDstContainer->Add( new CROUNDSEGMENT2D( start3DU, end3DU,
                                                         iwidth * m_biuTo3Dunits,
                                                         *aPad ) );
            }
        }
    }
    break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        // see pcbnew/board_items_to_polygon_shape_transform.cpp

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
        // Round segments cannot have 0-length elements, so we approximate them
        // as a small circle
        for( int i = 1; i <= 4; i++ )
        {
            if( Is_segment_a_circle( corners3DU[i - 1], corners3DU[i & 3] ) )
            {
                aDstContainer->Add( new CFILLEDCIRCLE2D( corners3DU[i - 1],
                                        aClearanceValue.x * m_biuTo3Dunits,
                                        *aPad ) );
            }
            else
            {
                aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[i - 1],
                                                         corners3DU[i & 3],
                                                         aClearanceValue.x * 2.0f * m_biuTo3Dunits,
                                                         *aPad ) );
            }
        }
    }
    break;

    case PAD_SHAPE_ROUNDRECT:
    {
        wxSize shapesize( aPad->GetSize() );
        shapesize.x += aClearanceValue.x * 2;
        shapesize.y += aClearanceValue.y * 2;

        int rounding_radius = aPad->GetRoundRectCornerRadius( shapesize );

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
        // Round segments cannot have 0-length elements, so we approximate them
        // as a small circle
        for( int i = 1; i <= 4; i++ )
        {
            if( Is_segment_a_circle( corners3DU[i - 1], corners3DU[i & 3] ) )
            {
                aDstContainer->Add( new CFILLEDCIRCLE2D( corners3DU[i - 1],
                                        rounding_radius * m_biuTo3Dunits,
                                        *aPad ) );
            }
            else
            {
                aDstContainer->Add( new CROUNDSEGMENT2D( corners3DU[i - 1],
                                        corners3DU[i & 3],
                                        rounding_radius * 2.0f * m_biuTo3Dunits,
                                        *aPad ) );
            }
        }
    }
    break;

    case PAD_SHAPE_CHAMFERED_RECT:
    {
        wxSize shapesize( aPad->GetSize() );
        shapesize.x += aClearanceValue.x * 2;
        shapesize.y += aClearanceValue.y * 2;

        SHAPE_POLY_SET polyList;     // Will contain the pad outlines in board coordinates

        int corner_radius = aPad->GetRoundRectCornerRadius( shapesize );
        TransformRoundChamferedRectToPolygon( polyList, PadShapePos, shapesize, aPad->GetOrientation(),
                                         corner_radius, aPad->GetChamferRectRatio(),
                                         aPad->GetChamferPositions(), ARC_HIGH_DEF );

        // Add the PAD polygon
        Convert_shape_line_polygon_to_triangles( polyList, *aDstContainer, m_biuTo3Dunits, *aPad );

    }
        break;

    case PAD_SHAPE_CUSTOM:
    {
        SHAPE_POLY_SET polyList;     // Will contain the pad outlines in board coordinates
        polyList.Append( aPad->GetCustomShapeAsPolygon() );
        aPad->CustomShapeAsPolygonToBoardPosition( &polyList, aPad->ShapePos(), aPad->GetOrientation() );

        if( aClearanceValue.x )
            polyList.Inflate( aClearanceValue.x, 32 );

        // Add the PAD polygon
        Convert_shape_line_polygon_to_triangles( polyList, *aDstContainer, m_biuTo3Dunits, *aPad );

    }
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


void CINFO3D_VISU::AddPadsShapesWithClearanceToContainer( const MODULE* aModule,
                                                          CGENERICCONTAINER2D *aDstContainer,
                                                          PCB_LAYER_ID aLayerId,
                                                          int aInflateValue,
                                                          bool aSkipNPTHPadsWihNoCopper )
{
    wxSize margin;

    for( auto pad : aModule->Pads() )
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

        createNewPadWithClearance( pad, aDstContainer, margin );
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
                                                     PCB_LAYER_ID aLayerId,
                                                     int aClearanceValue )
{
    // The full width of the lines to create
    // The extra 1 protects the inner/outer radius values from degeneracy
    const int linewidth = aDrawSegment->GetWidth() + (2 * aClearanceValue) + 1;

    switch( aDrawSegment->GetShape() )
    {
    case S_CIRCLE:
    {
        const SFVEC2F center3DU(  aDrawSegment->GetCenter().x * m_biuTo3Dunits,
                                 -aDrawSegment->GetCenter().y * m_biuTo3Dunits );

        const float inner_radius  =
                std::max<float>( (aDrawSegment->GetRadius() - linewidth / 2) * m_biuTo3Dunits, 0.0 );
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

    case S_CURVE:
    case S_POLYGON:
    {
        SHAPE_POLY_SET polyList;

        aDrawSegment->TransformShapeWithClearanceToPolygon( polyList, aClearanceValue );

        polyList.Simplify( SHAPE_POLY_SET::PM_FAST );

        if( polyList.IsEmpty() ) // Just for caution
            break;

        Convert_shape_line_polygon_to_triangles( polyList, *aDstContainer,
                                                 m_biuTo3Dunits, *aDrawSegment );
    }
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
                                                   PCB_LAYER_ID aLayerId )
{
    // Copy the polys list because we have to simplify it
    SHAPE_POLY_SET polyList = SHAPE_POLY_SET( aZoneContainer->GetFilledPolysList(), true );

    // This convert the poly in outline and holes
    Convert_shape_line_polygon_to_triangles( polyList, *aDstContainer, m_biuTo3Dunits,
                                             *aZoneContainer );

    // add filled areas outlines, which are drawn with thick lines segments
    // but only if filled polygons outlines have thickness
    if( !aZoneContainer->GetFilledPolysUseThickness() )
        return;

    float line_thickness = aZoneContainer->GetMinThickness() * m_biuTo3Dunits;

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
                float radius = line_thickness/2;

                if( radius > 0.0 )  // degenerated circles crash 3D viewer
                    aDstContainer->Add( new CFILLEDCIRCLE2D( start3DU, radius,
                                                             *aZoneContainer ) );
            }
            else
            {
                aDstContainer->Add( new CROUNDSEGMENT2D( start3DU, end3DU, line_thickness,
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
                    float radius = line_thickness/2;

                    if( radius > 0.0 )  // degenerated circles crash 3D viewer
                        aDstContainer->Add(
                                    new CFILLEDCIRCLE2D( start3DU, radius,
                                                         *aZoneContainer ) );
                }
                else
                {
                    aDstContainer->Add(
                                new CROUNDSEGMENT2D( start3DU, end3DU, line_thickness,
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
    aPad->BuildPadShapePolygon( corners, wxSize( 0, 0 ) );


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
            aDstContainer->Add(
                    new CFILLEDCIRCLE2D( start3DU, ( aWidth / 2 ) * m_biuTo3Dunits, *aPad ) );
        }
        else
        {
            aDstContainer->Add(
                    new CROUNDSEGMENT2D( start3DU, end3DU, aWidth * m_biuTo3Dunits, *aPad ) );
        }
    }
}
