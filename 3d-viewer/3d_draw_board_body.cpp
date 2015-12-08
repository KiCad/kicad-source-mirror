/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
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
 * @file 3d_draw_board_body.cpp
 *
 */

#include <fctsys.h>
#include <common.h>
#include <trigo.h>
#include <pcbstruct.h>
#include <drawtxt.h>
#include <layers_id_colors_and_visibility.h>

#include <wxBasePcbFrame.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <colors_selection.h>
#include <convert_basic_shapes_to_polygon.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <gal/opengl/opengl_compositor.h>
#ifdef __WINDOWS__
#include <GL/glew.h>        // must be included before gl.h
#endif

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>
#include <3d_draw_basic_functions.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_file_io.h>


#include <CImage.h>
#include <reporter.h>

// An option for all operations on polygons:
// when useFastModeForPolygons = true, calculations can be *a lot* faster.
// but created polygons can be not stricty simple (can share edges)
// Although stricty simple are better for glu tesselation functions, I do not see
// any issue when allowing not stricty simple polygons.
// But I see *very* long calculations when setting useFastMode to false.
// So, be careful if changing thie option
bool useFastModeForPolygons = true;

/* returns the Z orientation parameter 1.0 or -1.0 for aLayer
 * Z orientation is 1.0 for all layers but "back" layers:
 *  B_Cu , B_Adhes, B_Paste ), B_SilkS
 * used to calculate the Z orientation parameter for glNormal3f
 */
GLfloat  Get3DLayer_Z_Orientation( LAYER_NUM aLayer );

void EDA_3D_CANVAS::buildBoardThroughHolesPolygonList( SHAPE_POLY_SET& allBoardHoles,
                                                int aSegCountPerCircle, bool aOptimizeLargeCircles )
{
    // hole diameter value to change seg count by circle:
    int small_hole_limit = Millimeter2iu( 1.0 );
    int copper_thickness = GetPrm3DVisu().GetCopperThicknessBIU();

    BOARD* pcb = GetBoard();

    // Build holes of through vias:
    for( TRACK* track = pcb->m_Track;  track;  track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        VIA *via = static_cast<VIA*>( track );

        if( via->GetViaType() != VIA_THROUGH )
            continue;

        int holediameter = via->GetDrillValue();
        int hole_outer_radius = (holediameter + copper_thickness) / 2;

        TransformCircleToPolygon( allBoardHoles,
                                  via->GetStart(), hole_outer_radius,
                                  aSegCountPerCircle );
    }

    // Build holes of through pads:
    for( MODULE* footprint = pcb->m_Modules; footprint; footprint = footprint->Next() )
    {
        for( D_PAD* pad = footprint->Pads(); pad; pad = pad->Next() )
        {
            // Calculate a factor to apply to segcount for large holes ( > 1 mm)
            // (bigger pad drill size -> more segments) because holes in pads can have
            // very different sizes and optimizing this segcount gives a better look
            // Mainly mounting holes have a size bigger than small_hole_limit
            wxSize padHole = pad->GetDrillSize();

            if( ! padHole.x )       // Not drilled pad like SMD pad
                continue;

            // we use the hole diameter to calculate the seg count.
            // for round holes, padHole.x == padHole.y
            // for oblong holes, the diameter is the smaller of (padHole.x, padHole.y)
            int diam = std::min( padHole.x, padHole.y );
            int segcount = aSegCountPerCircle;

            if( diam > small_hole_limit )
            {
                double segFactor = (double)diam / small_hole_limit;
                segcount = (int)(aSegCountPerCircle * segFactor);

                // limit segcount to 48. For a circle this is a very good approx.
                if( segcount > 48 )
                    segcount = 48;
            }

            // The hole in the body is inflated by copper thickness.
            int inflate = copper_thickness;

            // If not plated, no copper.
            if( pad->GetAttribute () == PAD_ATTRIB_HOLE_NOT_PLATED )
                inflate = 0;

            pad->BuildPadDrillShapePolygon( allBoardHoles, inflate, segcount );
        }
    }

    allBoardHoles.Simplify( useFastModeForPolygons );
}


void EDA_3D_CANVAS::buildBoard3DView( GLuint aBoardList, GLuint aBodyOnlyList,
                                      REPORTER* aErrorMessages, REPORTER* aActivity  )
{
    BOARD* pcb = GetBoard();

    // If FL_RENDER_SHOW_HOLES_IN_ZONES is true, holes are correctly removed from copper zones areas.
    // If FL_RENDER_SHOW_HOLES_IN_ZONES is false, holes are not removed from copper zones areas,
    // but the calculation time is twice shorter.
    bool remove_Holes = isEnabled( FL_RENDER_SHOW_HOLES_IN_ZONES );

    bool realistic_mode = isRealisticMode();
    bool useTextures = isRealisticMode() && isEnabled( FL_RENDER_TEXTURES );

    // Number of segments to convert a circle to polygon
    // We use 2 values: the first gives a good shape (for instanes rond pads)
    // the second is used to speed up calculations, when a poor approximation is acceptable (holes)
    const int       segcountforcircle   = 18;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2.0) );
    const int       segcountLowQuality  = 12;   // segments to draw a circle with low quality
                                                // to reduce time calculations
                                                // for holes and items which do not need
                                                // a fine representation
    double          correctionFactorLQ  = 1.0 / cos( M_PI / (segcountLowQuality * 2.0) );

    SHAPE_POLY_SET  bufferPolys;        // copper areas: tracks, pads and filled zones areas
                                        // when holes are removed from zones
    SHAPE_POLY_SET  bufferPcbOutlines;  // stores the board main outlines
    SHAPE_POLY_SET  bufferZonesPolys;   // copper filled zones areas
                                        // when holes are not removed from zones
    SHAPE_POLY_SET  currLayerHoles;     // Contains holes for the current layer
    SHAPE_POLY_SET  allLayerHoles;      // Contains holes for all layers

    // Build a polygon from edge cut items
    wxString msg;

    if( !pcb->GetBoardPolygonOutlines( bufferPcbOutlines, allLayerHoles, &msg ) )
    {
        if( aErrorMessages )
        {
            msg << wxT("\n") << _("Unable to calculate the board outlines.\n"
                                  "Therefore use the board boundary box.") << wxT("\n\n");

            aErrorMessages->Report( msg, REPORTER::RPT_WARNING );
        }
    }

    // Build board holes, with optimization of large holes shape.
    buildBoardThroughHolesPolygonList( allLayerHoles, segcountLowQuality, true );

    LSET            cu_set = LSET::AllCuMask( GetPrm3DVisu().m_CopperLayersCount );

    glNewList( aBoardList, GL_COMPILE );

    for( LSEQ cu = cu_set.CuStack();  cu;  ++cu )
    {
        LAYER_ID layer = *cu;

        // Skip non enabled layers in normal mode,
        // and internal layers in realistic mode
        if( !is3DLayerEnabled( layer ) )
            continue;

        if( aActivity )
            aActivity->Report( wxString::Format( _( "Build layer %s" ), LSET::Name( layer ) ) );

        bufferPolys.RemoveAllContours();
        bufferZonesPolys.RemoveAllContours();
        currLayerHoles.RemoveAllContours();

        // Draw track shapes:
        for( TRACK* track = pcb->m_Track;  track;  track = track->Next() )
        {
            if( !track->IsOnLayer( layer ) )
                continue;

            track->TransformShapeWithClearanceToPolygon( bufferPolys,
                                                         0, segcountforcircle,
                                                         correctionFactor );

            // Add blind/buried via holes
            if( track->Type() == PCB_VIA_T )
            {
                VIA *via = static_cast<VIA*>( track );

                if( via->GetViaType() == VIA_THROUGH )
                    continue;   // already done

                int holediameter = via->GetDrillValue();
                int thickness = GetPrm3DVisu().GetCopperThicknessBIU();
                int hole_outer_radius = (holediameter + thickness) / 2;

                TransformCircleToPolygon( currLayerHoles,
                                          via->GetStart(), hole_outer_radius,
                                          segcountLowQuality );
            }
        }

        // draw pad shapes
        for( MODULE* module = pcb->m_Modules;  module;  module = module->Next() )
        {
            // Note: NPTH pads are not drawn on copper layers when the pad
            // has same shape as its hole
            module->TransformPadsShapesWithClearanceToPolygon( layer,
                                                               bufferPolys,
                                                               0,
                                                               segcountforcircle,
                                                               correctionFactor, true );

            // Micro-wave modules may have items on copper layers
            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                                                                     bufferPolys,
                                                                     0,
                                                                     segcountforcircle,
                                                                     correctionFactor );

            // pad holes are already in list.
        }

        // Draw copper zones. Note:
        // * if the holes are removed from copper zones
        // the polygons are stored in bufferPolys (which contains all other polygons)
        // * if the holes are NOT removed from copper zones
        // the polygons are stored in bufferZonesPolys
        if( isEnabled( FL_ZONE ) )
        {
            for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
            {
                ZONE_CONTAINER* zone = pcb->GetArea( ii );
                LAYER_NUM       zonelayer = zone->GetLayer();

                if( zonelayer == layer )
                {
                    zone->TransformSolidAreasShapesToPolygonSet(
                        remove_Holes ? bufferPolys : bufferZonesPolys,
                        segcountLowQuality, correctionFactorLQ );
                }
            }
        }

        // draw graphic items on copper layers (texts)
        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:    // should not exist on copper layers
                ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                    bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            case PCB_TEXT_T:
                ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                    bufferPolys, 0, segcountLowQuality, correctionFactor );
                break;

            default:
                break;
            }
        }

        // bufferPolys contains polygons to merge. Many overlaps .
        // Calculate merged polygons
        if( bufferPolys.IsEmpty() )
            continue;

        // Use Clipper lib to subtract holes to copper areas
        if( currLayerHoles.OutlineCount() )
        {
            currLayerHoles.Append(allLayerHoles);
            currLayerHoles.Simplify( useFastModeForPolygons );
            bufferPolys.BooleanSubtract( currLayerHoles, useFastModeForPolygons );
        }
        else
            bufferPolys.BooleanSubtract( allLayerHoles, useFastModeForPolygons );

        int thickness = GetPrm3DVisu().GetLayerObjectThicknessBIU( layer );
        int zpos = GetPrm3DVisu().GetLayerZcoordBIU( layer );

        float zNormal = 1.0f; // When using thickness it will draw first the top and then botton (with z inverted)

        // If we are not using thickness, then the z-normal has to match the layer direction
        // because just one plane will be drawn
        if( !thickness )
            zNormal = Get3DLayer_Z_Orientation( layer );

        if( realistic_mode )
        {
            setGLCopperColor();
        }
        else
        {
            EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( layer );
            SetGLColor( color );
        }

        // If holes are removed from copper zones, bufferPolys contains all polygons
        // to draw (tracks+zones+texts).
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos, thickness,
                                            GetPrm3DVisu().m_BiuTo3Dunits, useTextures,
                                            zNormal );

        // If holes are not removed from copper zones (for calculation time reasons,
        // the zone polygons are stored in bufferZonesPolys and have to be drawn now:
        if( !bufferZonesPolys.IsEmpty() )
        {
            Draw3D_SolidHorizontalPolyPolygons( bufferZonesPolys, zpos, thickness,
                                    GetPrm3DVisu().m_BiuTo3Dunits, useTextures,
                                    zNormal );
        }
    }

    if( aActivity )
        aActivity->Report( _( "Build board body" ) );

    // Draw plated vertical holes inside the board, but not always. They are drawn:
    // - if the board body is not shown, to show the holes.
    // - or if the copper thickness is shown
    if( !isEnabled( FL_SHOW_BOARD_BODY ) || isEnabled( FL_USE_COPPER_THICKNESS ) )
    {
        // Draw vias holes (vertical cylinders)
        for( const TRACK* track = pcb->m_Track;  track;  track = track->Next() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast<const VIA*>(track);
                draw3DViaHole( via );
            }
        }

        // Draw pads holes (vertical cylinders)
        for( const MODULE* module = pcb->m_Modules;  module;  module = module->Next() )
        {
            for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
                if( pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED )
                    draw3DPadHole( pad );
        }
    }

    glEndList();

    // Build the body board:
    glNewList( aBodyOnlyList, GL_COMPILE );

    if( isRealisticMode() )
    {
        setGLEpoxyColor( 1.00 );
    }
    else
    {
        EDA_COLOR_T color = g_ColorsSettings.GetLayerColor( Edge_Cuts );
        SetGLColor( color, 0.7 );
    }

    float copper_thickness = GetPrm3DVisu().GetCopperThicknessBIU();

    // a small offset between substrate and external copper layer to avoid artifacts
    // when drawing copper items on board
    float epsilon = Millimeter2iu( 0.01 );
    float zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );
    float board_thickness = GetPrm3DVisu().GetLayerZcoordBIU( F_Cu )
                        - GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );

    // items on copper layers and having a thickness = copper_thickness
    // are drawn from zpos - copper_thickness/2 to zpos + copper_thickness
    // therefore substrate position is copper_thickness/2 to
    // substrate_height - copper_thickness/2
    zpos += (copper_thickness + epsilon) / 2.0f;
    board_thickness -= copper_thickness + epsilon;

    bufferPcbOutlines.BooleanSubtract( allLayerHoles, useFastModeForPolygons );

    if( !bufferPcbOutlines.IsEmpty() )
    {
        Draw3D_SolidHorizontalPolyPolygons( bufferPcbOutlines, zpos + board_thickness / 2.0,
                                            board_thickness, GetPrm3DVisu().m_BiuTo3Dunits, useTextures,
                                            1.0f );
    }

    glEndList();
}


void EDA_3D_CANVAS::buildTechLayers3DView( REPORTER* aErrorMessages, REPORTER* aActivity )
{
    BOARD* pcb = GetBoard();
    bool useTextures = isRealisticMode() && isEnabled( FL_RENDER_TEXTURES );

    // Number of segments to draw a circle using segments
    const int       segcountforcircle   = 18;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );
    const int       segcountLowQuality  = 12;   // segments to draw a circle with low quality
                                                // to reduce time calculations
                                                // for holes and items which do not need
                                                // a fine representation

    double          correctionFactorLQ = 1.0 / cos( M_PI / (segcountLowQuality * 2) );

    // segments to draw a circle to build texts. Is is used only to build
    // the shape of each segment of the stroke font, therefore no need to have
    // many segments per circle.
    const int       segcountInStrokeFont = 8;

    SHAPE_POLY_SET bufferPolys;
    SHAPE_POLY_SET allLayerHoles;              // Contains through holes, calculated only once
    SHAPE_POLY_SET bufferPcbOutlines;          // stores the board main outlines

    // Build a polygon from edge cut items
    wxString msg;

    if( !pcb->GetBoardPolygonOutlines( bufferPcbOutlines, allLayerHoles, &msg ) )
    {
        if( aErrorMessages )
        {
            msg << wxT("\n") <<
                _("Unable to calculate the board outlines.\n"
                  "Therefore use the board boundary box.") << wxT("\n\n");
            aErrorMessages->Report( msg, REPORTER::RPT_WARNING );
        }
    }

    // Build board holes, with no optimization of large holes shape.
    buildBoardThroughHolesPolygonList( allLayerHoles, segcountLowQuality, false );

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
    };

    // User layers are not drawn here, only technical layers
    for( LSEQ seq = LSET::AllTechMask().Seq( teckLayerList, DIM( teckLayerList ) );  seq;  ++seq )
    {
        LAYER_ID layer = *seq;

        if( !is3DLayerEnabled( layer ) )
            continue;

        if( layer == Edge_Cuts && isEnabled( FL_SHOW_BOARD_BODY )  )
            continue;

        if( aActivity )
            aActivity->Report( wxString::Format( _( "Build layer %s" ), LSET::Name( layer ) ) );

        bufferPolys.RemoveAllContours();

        for( BOARD_ITEM* item = pcb->m_Drawings; item; item = item->Next() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                        bufferPolys, 0, segcountforcircle, correctionFactor );
                break;

            case PCB_TEXT_T:
                ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                        bufferPolys, 0, segcountLowQuality, 1.0 );
                break;

            default:
                break;
            }
        }

        for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
        {
            if( layer == F_SilkS || layer == B_SilkS )
            {
                // On silk screen layers, the pad shape is only the pad outline
                // never a filled shape
                D_PAD*  pad = module->Pads();
                int     linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( layer ) )
                        continue;

                    buildPadShapeThickOutlineAsPolygon( pad, bufferPolys,
                            linewidth, segcountforcircle, correctionFactor );
                }
            }
            else
                module->TransformPadsShapesWithClearanceToPolygon( layer,
                        bufferPolys, 0, segcountforcircle, correctionFactor );

            // On tech layers, use a poor circle approximation, only for texts (stroke font)
            module->TransformGraphicShapesWithClearanceToPolygonSet( layer,
                    bufferPolys, 0, segcountforcircle, correctionFactor, segcountInStrokeFont );
        }

        // Draw non copper zones
        if( isEnabled( FL_ZONE ) )
        {
            for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
            {
                ZONE_CONTAINER* zone = pcb->GetArea( ii );

                if( !zone->IsOnLayer( layer ) )
                    continue;

                zone->TransformSolidAreasShapesToPolygonSet(
                        bufferPolys, segcountLowQuality, correctionFactorLQ );
            }
        }

        // bufferPolys contains polygons to merge. Many overlaps .
        // Calculate merged polygons and remove pads and vias holes
        if( bufferPolys.IsEmpty() )
            continue;

        // Solder mask layers are "negative" layers.
        // Shapes should be removed from the full board area.
        if( layer == B_Mask || layer == F_Mask )
        {
            SHAPE_POLY_SET cuts = bufferPolys;
            bufferPolys = bufferPcbOutlines;

            cuts.Append(allLayerHoles);
            cuts.Simplify( useFastModeForPolygons );

            bufferPolys.BooleanSubtract( cuts, useFastModeForPolygons );
        }
        // Remove holes from Solder paste layers and silkscreen
        else if( layer == B_Paste || layer == F_Paste
                 || layer == B_SilkS || layer == F_SilkS  )
        {
            bufferPolys.BooleanSubtract( allLayerHoles, useFastModeForPolygons );
        }

        int thickness = 0;

        if( layer != B_Mask && layer != F_Mask )
            thickness = GetPrm3DVisu().GetLayerObjectThicknessBIU( layer );

        int zpos = GetPrm3DVisu().GetLayerZcoordBIU( layer );

        if( layer == Edge_Cuts )
        {
            thickness = GetPrm3DVisu().GetLayerZcoordBIU( F_Cu )
                        - GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );
            zpos = GetPrm3DVisu().GetLayerZcoordBIU( B_Cu )
                   + (thickness / 2);
        }
        else
        {
            // for Draw3D_SolidHorizontalPolyPolygons, zpos it the middle between bottom and top
            // sides.
            // However for top layers, zpos should be the bottom layer pos,
            // and for bottom layers, zpos should be the top layer pos.
            if( Get3DLayer_Z_Orientation( layer ) > 0 )
                zpos += thickness/2;
            else
                zpos -= thickness/2 ;
        }


        float zNormal = 1.0f; // When using thickness it will draw first the top and then botton (with z inverted)

        // If we are not using thickness, then the znormal must face the layer direction
        // because it will draw just one plane
        if( !thickness )
            zNormal = Get3DLayer_Z_Orientation( layer );


        setGLTechLayersColor( layer );
        Draw3D_SolidHorizontalPolyPolygons( bufferPolys, zpos,
                thickness, GetPrm3DVisu().m_BiuTo3Dunits, useTextures,
                zNormal );
    }
}
