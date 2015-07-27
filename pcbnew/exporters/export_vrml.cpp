/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2014  Cirilo Bernado
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <drawtxt.h>
#include <trigo.h>
#include <pgm_base.h>
#include <3d_struct.h>
#include <macros.h>
#include <exception>
#include <fstream>
#include <iomanip>

#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <convert_from_iu.h>

#include "../3d-viewer/modelparsers.h"

#include <vector>
#include <cmath>
#include <vrml_layer.h>

// minimum width (mm) of a VRML line
#define MIN_VRML_LINEWIDTH 0.12

// offset for art layers, mm (silk, paste, etc)
#define  ART_OFFSET 0.025


struct VRML_COLOR
{
    float diffuse_red;
    float diffuse_grn;
    float diffuse_blu;

    float spec_red;
    float spec_grn;
    float spec_blu;

    float emit_red;
    float emit_grn;
    float emit_blu;

    float ambient;
    float transp;
    float shiny;

    VRML_COLOR()
    {
        // default green
        diffuse_red = 0.13;
        diffuse_grn = 0.81;
        diffuse_blu = 0.22;
        spec_red = 0.13;
        spec_grn = 0.81;
        spec_blu = 0.22;
        emit_red = 0.0;
        emit_grn = 0.0;
        emit_blu = 0.0;

        ambient = 1.0;
        transp  = 0;
        shiny   = 0.2;
    }

    VRML_COLOR( float dr, float dg, float db,
                float sr, float sg, float sb,
                float er, float eg, float eb,
                float am, float tr, float sh )
    {
        diffuse_red = dr;
        diffuse_grn = dg;
        diffuse_blu = db;
        spec_red = sr;
        spec_grn = sg;
        spec_blu = sb;
        emit_red = er;
        emit_grn = eg;
        emit_blu = eb;

        ambient = am;
        transp  = tr;
        shiny   = sh;
    }
};


enum VRML_COLOR_INDEX
{
    VRML_COLOR_PCB = 0,
    VRML_COLOR_TRACK,
    VRML_COLOR_SILK,
    VRML_COLOR_TIN,
    VRML_COLOR_LAST
};


class MODEL_VRML
{
private:
    double      layer_z[LAYER_ID_COUNT];
    VRML_COLOR  colors[VRML_COLOR_LAST];

    int         iMaxSeg;                    // max. sides to a small circle
    double      arcMinLen, arcMaxLen;       // min and max lengths of an arc chord

public:
    VRML_LAYER  holes;
    VRML_LAYER  board;
    VRML_LAYER  top_copper;
    VRML_LAYER  bot_copper;
    VRML_LAYER  top_silk;
    VRML_LAYER  bot_silk;
    VRML_LAYER  top_tin;
    VRML_LAYER  bot_tin;
    VRML_LAYER  plated_holes;

    bool plainPCB;

    double scale;           // board internal units to output scaling
    double minLineWidth;    // minimum width of a VRML line segment
    int    precision;       // precision of output units

    double  tx;             // global translation along X
    double  ty;             // global translation along Y

    double board_thickness; // depth of the PCB

    LAYER_NUM s_text_layer;
    int s_text_width;

    MODEL_VRML()
    {
        for( unsigned i = 0; i < DIM( layer_z );  ++i )
            layer_z[i] = 0;

        holes.GetArcParams( iMaxSeg, arcMinLen, arcMaxLen );

        // this default only makes sense if the output is in mm
        board_thickness = 1.6;

        // pcb green
        colors[ VRML_COLOR_PCB ]    = VRML_COLOR( .07, .3, .12, .07, .3, .12,
                                                  0, 0, 0, 1, 0, 0.2 );
        // track green
        colors[ VRML_COLOR_TRACK ]  = VRML_COLOR( .08, .5, .1, .08, .5, .1,
                                                  0, 0, 0, 1, 0, 0.2 );
        // silkscreen white
        colors[ VRML_COLOR_SILK ]   = VRML_COLOR( .9, .9, .9, .9, .9, .9,
                                                  0, 0, 0, 1, 0, 0.2 );
        // pad silver
        colors[ VRML_COLOR_TIN ]    = VRML_COLOR( .749, .756, .761, .749, .756, .761,
                                                  0, 0, 0, 0.8, 0, 0.8 );

        plainPCB = false;
        SetScale( 1.0 );
        SetOffset( 0.0, 0.0 );
        s_text_layer = F_Cu;
        s_text_width = 1;
    }

    VRML_COLOR& GetColor( VRML_COLOR_INDEX aIndex )
    {
        return colors[aIndex];
    }

    void SetOffset( double aXoff, double aYoff )
    {
        tx = aXoff;
        ty = -aYoff;

        holes.SetVertexOffsets( aXoff, aYoff );
        board.SetVertexOffsets( aXoff, aYoff );
        top_copper.SetVertexOffsets( aXoff, aYoff );
        bot_copper.SetVertexOffsets( aXoff, aYoff );
        top_silk.SetVertexOffsets( aXoff, aYoff );
        bot_silk.SetVertexOffsets( aXoff, aYoff );
        top_tin.SetVertexOffsets( aXoff, aYoff );
        bot_tin.SetVertexOffsets( aXoff, aYoff );
        plated_holes.SetVertexOffsets( aXoff, aYoff );
    }

    double GetLayerZ( LAYER_NUM aLayer )
    {
        if( unsigned( aLayer ) >= DIM( layer_z ) )
            return 0;

        return layer_z[ aLayer ];
    }

    void SetLayerZ( LAYER_NUM aLayer, double aValue )
    {
        layer_z[aLayer] = aValue;
    }

    // set the scaling of the VRML world
    bool SetScale( double aWorldScale )
    {
        if( aWorldScale < 0.001 || aWorldScale > 10.0 )
            throw( std::runtime_error( "WorldScale out of range (valid range is 0.001 to 10.0)" ) );

        scale = aWorldScale * MM_PER_IU;
        minLineWidth = aWorldScale * MIN_VRML_LINEWIDTH;

        // set the precision of the VRML coordinates
        if( aWorldScale < 0.01 )
            precision = 8;
        else if( aWorldScale < 0.1 )
            precision = 7;
        else if( aWorldScale< 1.0 )
            precision = 6;
        else if( aWorldScale < 10.0 )
            precision = 5;
        else
            precision = 4;

        double smin = arcMinLen * aWorldScale;
        double smax = arcMaxLen * aWorldScale;

        holes.SetArcParams( iMaxSeg, smin, smax );
        board.SetArcParams( iMaxSeg, smin, smax );
        top_copper.SetArcParams( iMaxSeg, smin, smax);
        bot_copper.SetArcParams( iMaxSeg, smin, smax);
        top_silk.SetArcParams( iMaxSeg, smin, smax );
        bot_silk.SetArcParams( iMaxSeg, smin, smax );
        top_tin.SetArcParams( iMaxSeg, smin, smax );
        bot_tin.SetArcParams( iMaxSeg, smin, smax );
        plated_holes.SetArcParams( iMaxSeg, smin, smax );

        return true;
    }

};


// static var. for dealing with text
static MODEL_VRML* model_vrml;


// select the VRML layer object to draw on; return true if
// a layer has been selected.
static bool GetLayer( MODEL_VRML& aModel, LAYER_NUM layer, VRML_LAYER** vlayer )
{
    switch( layer )
    {
    case B_Cu:
        *vlayer = &aModel.bot_copper;
        break;

    case F_Cu:
        *vlayer = &aModel.top_copper;
        break;

    case B_SilkS:
        *vlayer = &aModel.bot_silk;
        break;

    case F_SilkS:
        *vlayer = &aModel.top_silk;
        break;

    default:
        return false;
    }

    return true;
}


static void write_triangle_bag( std::ofstream& output_file, VRML_COLOR& color,
                                VRML_LAYER* layer, bool plane, bool top,
                                double top_z, double bottom_z, int aPrecision )
{
    /* A lot of nodes are not required, but blender sometimes chokes
     * without them */
    static const char* shape_boiler[] =
    {
        "Transform {\n",
        "  children [\n",
        "    Group {\n",
        "      children [\n",
        "        Shape {\n",
        "          appearance Appearance {\n",
        "            material Material {\n",
        0,                                      // Material marker
        "            }\n",
        "          }\n",
        "          geometry IndexedFaceSet {\n",
        "            solid TRUE\n",
        "            coord Coordinate {\n",
        "              point [\n",
        0,                                      // Coordinates marker
        "              ]\n",
        "            }\n",
        "            coordIndex [\n",
        0,                                      // Index marker
        "            ]\n",
        "          }\n",
        "        }\n",
        "      ]\n",
        "    }\n",
        "  ]\n",
        "}\n",
        0    // End marker
    };

    int marker_found = 0, lineno = 0;

    while( marker_found < 4 )
    {
        if( shape_boiler[lineno] )
            output_file << shape_boiler[lineno];
        else
        {
            marker_found++;

            switch( marker_found )
            {
            case 1:    // Material marker
                output_file << "              diffuseColor " << std::setprecision(3);
                output_file << color.diffuse_red << " ";
                output_file << color.diffuse_grn << " ";
                output_file << color.diffuse_blu << "\n";

                output_file << "              specularColor ";
                output_file << color.spec_red << " ";
                output_file << color.spec_grn << " ";
                output_file << color.spec_blu << "\n";

                output_file << "              emissiveColor ";
                output_file << color.emit_red << " ";
                output_file << color.emit_grn << " ";
                output_file << color.emit_blu << "\n";

                output_file << "              ambientIntensity " << color.ambient << "\n";
                output_file << "              transparency " << color.transp << "\n";
                output_file << "              shininess " << color.shiny << "\n";
                break;

            case 2:

                if( plane )
                    layer->WriteVertices( top_z, output_file, aPrecision );
                else
                    layer->Write3DVertices( top_z, bottom_z, output_file, aPrecision );

                output_file << "\n";
                break;

            case 3:

                if( plane )
                    layer->WriteIndices( top, output_file );
                else
                    layer->Write3DIndices( output_file );

                output_file << "\n";
                break;

            default:
                break;
            }
        }

        lineno++;
    }
}


static void write_layers( MODEL_VRML& aModel, std::ofstream& output_file, BOARD* aPcb )
{
    // VRML_LAYER board;
    aModel.board.Tesselate( &aModel.holes );
    double brdz = aModel.board_thickness / 2.0
                  - ( Millimeter2iu( ART_OFFSET / 2.0 ) ) * aModel.scale;
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_PCB ),
                        &aModel.board, false, false, brdz, -brdz, aModel.precision );

    if( aModel.plainPCB )
        return;

    // VRML_LAYER top_copper;
    aModel.top_copper.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_TRACK ),
                        &aModel.top_copper, true, true,
                        aModel.GetLayerZ( F_Cu ), 0, aModel.precision );

    // VRML_LAYER top_tin;
    aModel.top_tin.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_TIN ),
                        &aModel.top_tin, true, true,
                        aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * aModel.scale,
                        0, aModel.precision );

    // VRML_LAYER bot_copper;
    aModel.bot_copper.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_TRACK ),
                        &aModel.bot_copper, true, false,
                        aModel.GetLayerZ( B_Cu ), 0, aModel.precision );

    // VRML_LAYER bot_tin;
    aModel.bot_tin.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_TIN ),
                        &aModel.bot_tin, true, false,
                        aModel.GetLayerZ( B_Cu )
                        - Millimeter2iu( ART_OFFSET / 2.0 ) * aModel.scale,
                        0, aModel.precision );

    // VRML_LAYER PTH;
    aModel.plated_holes.Tesselate( NULL, true );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_TIN ),
                        &aModel.plated_holes, false, false,
                        aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * aModel.scale,
                        aModel.GetLayerZ( B_Cu ) - Millimeter2iu( ART_OFFSET / 2.0 ) * aModel.scale,
                        aModel.precision );

    // VRML_LAYER top_silk;
    aModel.top_silk.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_SILK ), &aModel.top_silk,
                        true, true, aModel.GetLayerZ( F_SilkS ), 0, aModel.precision );

    // VRML_LAYER bot_silk;
    aModel.bot_silk.Tesselate( &aModel.holes );
    write_triangle_bag( output_file, aModel.GetColor( VRML_COLOR_SILK ), &aModel.bot_silk,
                        true, false, aModel.GetLayerZ( B_SilkS ), 0, aModel.precision );
}


static void compute_layer_Zs( MODEL_VRML& aModel, BOARD* pcb )
{
    int copper_layers = pcb->GetCopperLayerCount();

    // We call it 'layer' thickness, but it's the whole board thickness!
    aModel.board_thickness = pcb->GetDesignSettings().GetBoardThickness() * aModel.scale;
    double half_thickness = aModel.board_thickness / 2;

    // Compute each layer's Z value, more or less like the 3d view
    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        LAYER_ID i = *seq;

        if( i < copper_layers )
            aModel.SetLayerZ( i,  half_thickness - aModel.board_thickness * i / (copper_layers - 1) );
        else
            aModel.SetLayerZ( i, - half_thickness );  // bottom layer
    }

    /* To avoid rounding interference, we apply an epsilon to each
     * successive layer */
    double epsilon_z = Millimeter2iu( ART_OFFSET ) * aModel.scale;
    aModel.SetLayerZ( B_Paste, -half_thickness - epsilon_z * 4 );
    aModel.SetLayerZ( B_Adhes, -half_thickness - epsilon_z * 3 );
    aModel.SetLayerZ( B_SilkS, -half_thickness - epsilon_z * 2 );
    aModel.SetLayerZ( B_Mask, -half_thickness - epsilon_z );
    aModel.SetLayerZ( F_Mask, half_thickness + epsilon_z );
    aModel.SetLayerZ( F_SilkS, half_thickness + epsilon_z * 2 );
    aModel.SetLayerZ( F_Adhes, half_thickness + epsilon_z * 3 );
    aModel.SetLayerZ( F_Paste, half_thickness + epsilon_z * 4 );
    aModel.SetLayerZ( Dwgs_User, half_thickness + epsilon_z * 5 );
    aModel.SetLayerZ( Cmts_User, half_thickness + epsilon_z * 6 );
    aModel.SetLayerZ( Eco1_User, half_thickness + epsilon_z * 7 );
    aModel.SetLayerZ( Eco2_User, half_thickness + epsilon_z * 8 );
    aModel.SetLayerZ( Edge_Cuts, 0 );
}


static void export_vrml_line( MODEL_VRML& aModel, LAYER_NUM layer,
                              double startx, double starty,
                              double endx, double endy, double width )
{
    VRML_LAYER* vlayer;

    if( !GetLayer( aModel, layer, &vlayer ) )
        return;

    if( width < aModel.minLineWidth)
        width = aModel.minLineWidth;

    starty = -starty;
    endy = -endy;

    double  angle   = atan2( endy - starty, endx - startx ) * 180.0 / M_PI;
    double  length  = Distance( startx, starty, endx, endy ) + width;
    double  cx  = ( startx + endx ) / 2.0;
    double  cy  = ( starty + endy ) / 2.0;

    if( !vlayer->AddSlot( cx, cy, length, width, angle, false ) )
        throw( std::runtime_error( vlayer->GetError() ) );
}


static void export_vrml_circle( MODEL_VRML& aModel, LAYER_NUM layer,
                                double startx, double starty,
                                double endx, double endy, double width )
{
    VRML_LAYER* vlayer;

    if( !GetLayer( aModel, layer, &vlayer ) )
        return;

    if( width < aModel.minLineWidth )
        width = aModel.minLineWidth;

    starty = -starty;
    endy = -endy;

    double hole, radius;

    radius = Distance( startx, starty, endx, endy ) + ( width / 2);
    hole = radius - width;

    if( !vlayer->AddCircle( startx, starty, radius, false ) )
        throw( std::runtime_error( vlayer->GetError() ) );

    if( hole > 0.0001 )
    {
        if( !vlayer->AddCircle( startx, starty, hole, true ) )
            throw( std::runtime_error( vlayer->GetError() ) );
    }
}


static void export_vrml_arc( MODEL_VRML& aModel, LAYER_NUM layer,
                             double centerx, double centery,
                             double arc_startx, double arc_starty,
                             double width, double arc_angle )
{
    VRML_LAYER* vlayer;

    if( !GetLayer( aModel, layer, &vlayer ) )
        return;

    if( width < aModel.minLineWidth )
        width = aModel.minLineWidth;

    centery = -centery;
    arc_starty = -arc_starty;

    if( !vlayer->AddArc( centerx, centery, arc_startx, arc_starty, width, -arc_angle, false ) )
        throw( std::runtime_error( vlayer->GetError() ) );

}


static void export_vrml_drawsegment( MODEL_VRML& aModel, DRAWSEGMENT* drawseg )
{
    LAYER_NUM layer = drawseg->GetLayer();
    double  w   = drawseg->GetWidth() * aModel.scale;
    double  x   = drawseg->GetStart().x * aModel.scale;
    double  y   = drawseg->GetStart().y * aModel.scale;
    double  xf  = drawseg->GetEnd().x * aModel.scale;
    double  yf  = drawseg->GetEnd().y * aModel.scale;

    // Items on the edge layer are handled elsewhere; just return
    if( layer == Edge_Cuts )
        return;

    switch( drawseg->GetShape() )
    {
    case S_ARC:
        export_vrml_arc( aModel, layer,
                         (double) drawseg->GetCenter().x * aModel.scale,
                         (double) drawseg->GetCenter().y * aModel.scale,
                         (double) drawseg->GetArcStart().x * aModel.scale,
                         (double) drawseg->GetArcStart().y * aModel.scale,
                         w, drawseg->GetAngle() / 10 );
        break;

    case S_CIRCLE:
        export_vrml_circle( aModel, layer, x, y, xf, yf, w );
        break;

    default:
        export_vrml_line( aModel, layer, x, y, xf, yf, w );
        break;
    }
}


/* C++ doesn't have closures and neither continuation forms... this is
 * for coupling the vrml_text_callback with the common parameters */
static void vrml_text_callback( int x0, int y0, int xf, int yf )
{
    LAYER_NUM s_text_layer = model_vrml->s_text_layer;
    int s_text_width = model_vrml->s_text_width;
    double  scale = model_vrml->scale;

    export_vrml_line( *model_vrml, s_text_layer,
                      x0 * scale, y0 * scale,
                      xf * scale, yf * scale,
                      s_text_width * scale );
}


static void export_vrml_pcbtext( MODEL_VRML& aModel, TEXTE_PCB* text )
{
    model_vrml->s_text_layer    = text->GetLayer();
    model_vrml->s_text_width    = text->GetThickness();

    wxSize size = text->GetSize();

    if( text->IsMirrored() )
        size.x = -size.x;

    EDA_COLOR_T color = BLACK;  // not actually used, but needed by DrawGraphicText

    if( text->IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( text->GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        text->GetPositionsOfLinesOfMultilineText( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString& txt = strings_list.Item( ii );
            DrawGraphicText( NULL, NULL, positions[ii], color,
                             txt, text->GetOrientation(), size,
                             text->GetHorizJustify(), text->GetVertJustify(),
                             text->GetThickness(), text->IsItalic(),
                             true,
                             vrml_text_callback );
        }
    }
    else
    {
        DrawGraphicText( NULL, NULL, text->GetTextPosition(), color,
                         text->GetShownText(), text->GetOrientation(), size,
                         text->GetHorizJustify(), text->GetVertJustify(),
                         text->GetThickness(), text->IsItalic(),
                         true,
                         vrml_text_callback );
    }
}


static void export_vrml_drawings( MODEL_VRML& aModel, BOARD* pcb )
{
    // draw graphic items
    for( EDA_ITEM* drawing = pcb->m_Drawings; drawing != 0; drawing = drawing->Next() )
    {
        LAYER_ID layer = ( (DRAWSEGMENT*) drawing )->GetLayer();

        if( layer != F_Cu && layer != B_Cu && layer != B_SilkS && layer != F_SilkS )
            continue;

        switch( drawing->Type() )
        {
        case PCB_LINE_T:
            export_vrml_drawsegment( aModel, (DRAWSEGMENT*) drawing );
            break;

        case PCB_TEXT_T:
            export_vrml_pcbtext( aModel, (TEXTE_PCB*) drawing );
            break;

        default:
            break;
        }
    }
}


// board edges and cutouts
static void export_vrml_board( MODEL_VRML& aModel, BOARD* pcb )
{
    SHAPE_POLY_SET  bufferPcbOutlines;      // stores the board main outlines
    SHAPE_POLY_SET  allLayerHoles;          // Contains through holes, calculated only once
    // Build a polygon from edge cut items
    wxString msg;

    if( !pcb->GetBoardPolygonOutlines( bufferPcbOutlines, allLayerHoles, &msg ) )
    {
        msg << wxT( "\n\n" ) <<
        _( "Unable to calculate the board outlines;\n"
           "fall back to using the board boundary box." );
        wxMessageBox( msg );
    }

    double  scale = aModel.scale;
    int seg;

    for( int i = 0; i < bufferPcbOutlines.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = bufferPcbOutlines.COutline( i );

        seg = aModel.board.NewContour();

        for( int j = 0; j < outline.PointCount(); j++ )
        {
            aModel.board.AddVertex( seg, (double)outline.CPoint(j).x * scale,
                                        -((double)outline.CPoint(j).y * scale ) );

        }

        aModel.board.EnsureWinding( seg, false );
    }

    for( int i = 0; i < allLayerHoles.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = allLayerHoles.COutline( i );

        seg = aModel.holes.NewContour();

        if( seg < 0 )
        {
            msg << wxT( "\n\n" ) <<
            _( "VRML Export Failed:\nCould not add holes to contours." );
            wxMessageBox( msg );

            return;
        }

        for( int j = 0; j < outline.PointCount(); j++ )
        {
            aModel.holes.AddVertex( seg, (double)outline.CPoint(j).x * scale,
                                        -((double)outline.CPoint(j).y * scale ) );

        }

        aModel.holes.EnsureWinding( seg, true );
    }
}


static void export_round_padstack( MODEL_VRML& aModel, BOARD* pcb,
                                   double x, double y, double r,
                                   LAYER_NUM bottom_layer, LAYER_NUM top_layer,
                                   double hole )
{
    LAYER_NUM   layer = top_layer;
    bool        thru = true;

    // if not a thru hole do not put a hole in the board
    if( top_layer != F_Cu || bottom_layer != B_Cu )
        thru = false;

    if( thru && hole > 0 )
        aModel.holes.AddCircle( x, -y, hole, true );

    if( aModel.plainPCB )
        return;

    while( 1 )
    {
        if( layer == B_Cu )
        {
            aModel.bot_copper.AddCircle( x, -y, r );

            if( hole > 0 && !thru )
                aModel.bot_copper.AddCircle( x, -y, hole, true );

        }
        else if( layer == F_Cu )
        {
            aModel.top_copper.AddCircle( x, -y, r );

            if( hole > 0 && !thru )
                aModel.top_copper.AddCircle( x, -y, hole, true );

        }

        if( layer == bottom_layer )
            break;

        layer = bottom_layer;
    }
}


static void export_vrml_via( MODEL_VRML& aModel, BOARD* pcb, const VIA* via )
{
    double      x, y, r, hole;
    LAYER_ID    top_layer, bottom_layer;

    hole = via->GetDrillValue() * aModel.scale / 2.0;
    r   = via->GetWidth() * aModel.scale / 2.0;
    x   = via->GetStart().x * aModel.scale;
    y   = via->GetStart().y * aModel.scale;
    via->LayerPair( &top_layer, &bottom_layer );

    // do not render a buried via
    if( top_layer != F_Cu && bottom_layer != B_Cu )
        return;

    // Export the via padstack
    export_round_padstack( aModel, pcb, x, y, r, bottom_layer, top_layer, hole );
}


static void export_vrml_tracks( MODEL_VRML& aModel, BOARD* pcb )
{
    for( TRACK* track = pcb->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            export_vrml_via( aModel, pcb, (const VIA*) track );
        }
        else if( ( track->GetLayer() == B_Cu || track->GetLayer() == F_Cu )
                   && !aModel.plainPCB )
            export_vrml_line( aModel, track->GetLayer(),
                              track->GetStart().x * aModel.scale,
                              track->GetStart().y * aModel.scale,
                              track->GetEnd().x * aModel.scale,
                              track->GetEnd().y * aModel.scale,
                              track->GetWidth() * aModel.scale );
    }
}


static void export_vrml_zones( MODEL_VRML& aModel, BOARD* aPcb )
{
    double scale = aModel.scale;

    for( int ii = 0; ii < aPcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = aPcb->GetArea( ii );

        VRML_LAYER* vl;

        if( !GetLayer( aModel, zone->GetLayer(), &vl ) )
            continue;

        if( !zone->IsFilled() )
        {
            zone->SetFillMode( 0 ); // use filled polygons
            zone->BuildFilledSolidAreasPolygons( aPcb );
        }

        const SHAPE_POLY_SET& poly = zone->GetFilledPolysList();

        for( int i = 0; i < poly.OutlineCount(); i++ )
        {
            const SHAPE_LINE_CHAIN& outline = poly.COutline( i );

            int seg = vl->NewContour();

            for( int j = 0; j < outline.PointCount(); j++ )
            {
                if( !vl->AddVertex( seg, (double)outline.CPoint( j ).x * scale,
                                         -((double)outline.CPoint( j ).y * scale ) ) )
                    throw( std::runtime_error( vl->GetError() ) );

            }

            vl->EnsureWinding( seg, false );
        }
    }
}


static void export_vrml_text_module( TEXTE_MODULE* module )
{
    if( module->IsVisible() )
    {
        wxSize size = module->GetSize();

        if( module->IsMirrored() )
            size.x = -size.x;  // Text is mirrored

        model_vrml->s_text_layer    = module->GetLayer();
        model_vrml->s_text_width    = module->GetThickness();

        DrawGraphicText( NULL, NULL, module->GetTextPosition(), BLACK,
                         module->GetShownText(), module->GetDrawRotation(), size,
                         module->GetHorizJustify(), module->GetVertJustify(),
                         module->GetThickness(), module->IsItalic(),
                         true,
                         vrml_text_callback );
    }
}


static void export_vrml_edge_module( MODEL_VRML& aModel, EDGE_MODULE* aOutline,
                                     double aOrientation )
{
    LAYER_NUM layer = aOutline->GetLayer();
    double  x   = aOutline->GetStart().x * aModel.scale;
    double  y   = aOutline->GetStart().y * aModel.scale;
    double  xf  = aOutline->GetEnd().x * aModel.scale;
    double  yf  = aOutline->GetEnd().y * aModel.scale;
    double  w   = aOutline->GetWidth() * aModel.scale;

    switch( aOutline->GetShape() )
    {
    case S_SEGMENT:
        export_vrml_line( aModel, layer, x, y, xf, yf, w );
        break;

    case S_ARC:
        export_vrml_arc( aModel, layer, x, y, xf, yf, w, aOutline->GetAngle() / 10 );
        break;

    case S_CIRCLE:
        export_vrml_circle( aModel, layer, x, y, xf, yf, w );
        break;

    case S_POLYGON:
        {
            VRML_LAYER* vl;

            if( !GetLayer( aModel, layer, &vl ) )
                break;

            int nvert = aOutline->GetPolyPoints().size() - 1;
            int i = 0;

            if( nvert < 3 ) break;

            int seg = vl->NewContour();

            if( seg < 0 )
                break;

            while( i < nvert )
            {
                CPolyPt corner( aOutline->GetPolyPoints()[i] );
                RotatePoint( &corner.x, &corner.y, aOrientation );
                corner.x += aOutline->GetPosition().x;
                corner.y += aOutline->GetPosition().y;

                x = corner.x * aModel.scale;
                y = - ( corner.y * aModel.scale );

                if( !vl->AddVertex( seg, x, y ) )
                    throw( std::runtime_error( vl->GetError() ) );

                ++i;
            }
            vl->EnsureWinding( seg, false );
        }
        break;

    default:
        break;
    }
}


static void export_vrml_padshape( MODEL_VRML& aModel, VRML_LAYER* aTinLayer, D_PAD* aPad )
{
    // The (maybe offset) pad position
    wxPoint pad_pos = aPad->ShapePos();
    double  pad_x   = pad_pos.x * aModel.scale;
    double  pad_y   = pad_pos.y * aModel.scale;
    wxSize  pad_delta = aPad->GetDelta();

    double  pad_dx  = pad_delta.x * aModel.scale / 2.0;
    double  pad_dy  = pad_delta.y * aModel.scale / 2.0;

    double  pad_w   = aPad->GetSize().x * aModel.scale / 2.0;
    double  pad_h   = aPad->GetSize().y * aModel.scale / 2.0;

    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:

        if( !aTinLayer->AddCircle( pad_x, -pad_y, pad_w, false ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;

    case PAD_OVAL:

        if( !aTinLayer->AddSlot( pad_x, -pad_y, pad_w * 2.0, pad_h * 2.0,
                                 aPad->GetOrientation()/10.0, false ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;

    case PAD_RECT:
        // Just to be sure :D
        pad_dx  = 0;
        pad_dy  = 0;

    case PAD_TRAPEZOID:
    {
        double coord[8] =
        {
            -pad_w + pad_dy, -pad_h - pad_dx,
            -pad_w - pad_dy, pad_h + pad_dx,
            +pad_w - pad_dy, -pad_h + pad_dx,
            +pad_w + pad_dy, pad_h - pad_dx
        };

        for( int i = 0; i < 4; i++ )
        {
            RotatePoint( &coord[i * 2], &coord[i * 2 + 1], aPad->GetOrientation() );
            coord[i * 2] += pad_x;
            coord[i * 2 + 1] += pad_y;
        }

        int lines;

        lines = aTinLayer->NewContour();

        if( lines < 0 )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        if( !aTinLayer->AddVertex( lines, coord[0], -coord[1] ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        if( !aTinLayer->AddVertex( lines, coord[4], -coord[5] ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        if( !aTinLayer->AddVertex( lines, coord[6], -coord[7] ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        if( !aTinLayer->AddVertex( lines, coord[2], -coord[3] ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        if( !aTinLayer->EnsureWinding( lines, false ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;
    }

    default:
        break;
    }
}


static void export_vrml_pad( MODEL_VRML& aModel, BOARD* pcb, D_PAD* aPad )
{
    double  hole_drill_w    = (double) aPad->GetDrillSize().x * aModel.scale / 2.0;
    double  hole_drill_h    = (double) aPad->GetDrillSize().y * aModel.scale / 2.0;
    double  hole_drill      = std::min( hole_drill_w, hole_drill_h );
    double  hole_x          = aPad->GetPosition().x * aModel.scale;
    double  hole_y          = aPad->GetPosition().y * aModel.scale;

    // Export the hole on the edge layer
    if( hole_drill > 0 )
    {
        bool pth = false;

        if( ( aPad->GetAttribute() != PAD_HOLE_NOT_PLATED )
            && !aModel.plainPCB )
            pth = true;

        if( aPad->GetDrillShape() == PAD_DRILL_OBLONG )
        {
            // Oblong hole (slot)
            aModel.holes.AddSlot( hole_x, -hole_y, hole_drill_w * 2.0, hole_drill_h * 2.0,
                                  aPad->GetOrientation()/10.0, true, pth );

            if( pth )
                aModel.plated_holes.AddSlot( hole_x, -hole_y,
                                             hole_drill_w * 2.0, hole_drill_h * 2.0,
                                             aPad->GetOrientation()/10.0, true, false );
        }
        else
        {
            // Drill a round hole
            aModel.holes.AddCircle( hole_x, -hole_y, hole_drill, true, pth );

            if( pth )
                aModel.plated_holes.AddCircle( hole_x, -hole_y, hole_drill, true, false );

        }
    }

    if( aModel.plainPCB )
        return;

    // The pad proper, on the selected layers
    LSET layer_mask = aPad->GetLayerSet();

    if( layer_mask[B_Cu] )
    {
        export_vrml_padshape( aModel, &aModel.bot_tin, aPad );
    }

    if( layer_mask[F_Cu] )
    {
        export_vrml_padshape( aModel, &aModel.top_tin, aPad );
    }
}


// From axis/rot to quaternion
static void build_quat( double x, double y, double z, double a, double q[4] )
{
    double sina = sin( a / 2 );

    q[0] = x * sina;
    q[1] = y * sina;
    q[2] = z * sina;
    q[3] = cos( a / 2 );
}


// From quaternion to axis/rot
static void from_quat( double q[4], double rot[4] )
{
    rot[3] = acos( q[3] ) * 2;

    for( int i = 0; i < 3; i++ )
    {
        rot[i] = q[i] / sin( rot[3] / 2 );
    }
}


// Quaternion composition
static void compose_quat( double q1[4], double q2[4], double qr[4] )
{
    double tmp[4];

    tmp[0] = q2[3] * q1[0] + q2[0] * q1[3] + q2[1] * q1[2] - q2[2] * q1[1];
    tmp[1] = q2[3] * q1[1] + q2[1] * q1[3] + q2[2] * q1[0] - q2[0] * q1[2];
    tmp[2] = q2[3] * q1[2] + q2[2] * q1[3] + q2[0] * q1[1] - q2[1] * q1[0];
    tmp[3] = q2[3] * q1[3] - q2[0] * q1[0] - q2[1] * q1[1] - q2[2] * q1[2];

    qr[0] = tmp[0];
    qr[1] = tmp[1];
    qr[2] = tmp[2];
    qr[3] = tmp[3];
}


static void export_vrml_module( MODEL_VRML& aModel, BOARD* aPcb, MODULE* aModule,
                                std::ofstream& aOutputFile, double aVRMLModelsToBiu,
                                bool aExport3DFiles, bool aUseRelativePaths,
                                const wxString& a3D_Subdir )
{
    if( !aModel.plainPCB )
    {
        // Reference and value
        if( aModule->Reference().IsVisible() )
            export_vrml_text_module( &aModule->Reference() );

        if( aModule->Value().IsVisible() )
            export_vrml_text_module( &aModule->Value() );

        // Export module edges
        for( EDA_ITEM* item = aModule->GraphicalItems(); item; item = item->Next() )
        {
            switch( item->Type() )
            {
                case PCB_MODULE_TEXT_T:
                    export_vrml_text_module( static_cast<TEXTE_MODULE*>( item ) );
                    break;

                case PCB_MODULE_EDGE_T:
                    export_vrml_edge_module( aModel, static_cast<EDGE_MODULE*>( item ),
                                             aModule->GetOrientation() );
                    break;

                default:
                    break;
            }
        }
    }

    // Export pads
    for( D_PAD* pad = aModule->Pads(); pad; pad = pad->Next() )
        export_vrml_pad( aModel, aPcb, pad );

    bool isFlipped = aModule->GetLayer() == B_Cu;

    // Export the object VRML model(s)
    for( S3D_MASTER* vrmlm = aModule->Models();  vrmlm;  vrmlm = vrmlm->Next() )
    {
        if( !vrmlm->Is3DType( S3D_MASTER::FILE3D_VRML ) )
            continue;

        wxFileName modelFileName = vrmlm->GetShape3DFullFilename();
        wxFileName destFileName( a3D_Subdir, modelFileName.GetName(), modelFileName.GetExt() );

        // Only copy VRML files.
        if( modelFileName.FileExists() && modelFileName.GetExt() == wxT( "wrl" ) )
        {
            if( aExport3DFiles )
            {
                wxDateTime srcModTime = modelFileName.GetModificationTime();
                wxDateTime destModTime = srcModTime;

                destModTime.SetToCurrent();

                if( destFileName.FileExists() )
                    destModTime = destFileName.GetModificationTime();

                // Only copy the file if it doesn't exist or has been modified.  This eliminates
                // the redundant file copies.
                if( srcModTime != destModTime )
                {
                    wxLogDebug( wxT( "Copying 3D model %s to %s." ),
                                GetChars( modelFileName.GetFullPath() ),
                                GetChars( destFileName.GetFullPath() ) );

                    if( !wxCopyFile( modelFileName.GetFullPath(), destFileName.GetFullPath() ) )
                        continue;
                }
            }

            /* Calculate 3D shape rotation:
             * this is the rotation parameters, with an additional 180 deg rotation
             * for footprints that are flipped
             * When flipped, axis rotation is the horizontal axis (X axis)
             */
            double rotx = -vrmlm->m_MatRotation.x;
            double roty = -vrmlm->m_MatRotation.y;
            double rotz = -vrmlm->m_MatRotation.z;

            if( isFlipped )
            {
                rotx += 180.0;
                roty = -roty;
                rotz = -rotz;
            }

            // Do some quaternion munching
            double q1[4], q2[4], rot[4];
            build_quat( 1, 0, 0, DEG2RAD( rotx ), q1 );
            build_quat( 0, 1, 0, DEG2RAD( roty ), q2 );
            compose_quat( q1, q2, q1 );
            build_quat( 0, 0, 1, DEG2RAD( rotz ), q2 );
            compose_quat( q1, q2, q1 );

            // Note here aModule->GetOrientation() is in 0.1 degrees,
            // so module rotation has to be converted to radians
            build_quat( 0, 0, 1, DECIDEG2RAD( aModule->GetOrientation() ), q2 );
            compose_quat( q1, q2, q1 );
            from_quat( q1, rot );

            aOutputFile << "Transform {\n";

            // A null rotation would fail the acos!
            if( rot[3] != 0.0 )
            {
                aOutputFile << "  rotation " << std::setprecision( 3 );
                aOutputFile << rot[0] << " " << rot[1] << " " << rot[2] << " " << rot[3] << "\n";
            }

            // adjust 3D shape local offset position
            // they are given in inch, so they are converted in board IU.
            double offsetx = vrmlm->m_MatPosition.x * IU_PER_MILS * 1000.0;
            double offsety = vrmlm->m_MatPosition.y * IU_PER_MILS * 1000.0;
            double offsetz = vrmlm->m_MatPosition.z * IU_PER_MILS * 1000.0;

            if( isFlipped )
                offsetz = -offsetz;
            else // In normal mode, Y axis is reversed in Pcbnew.
                offsety = -offsety;

            RotatePoint( &offsetx, &offsety, aModule->GetOrientation() );

            aOutputFile << "  translation " << std::setprecision( aModel.precision );
            aOutputFile << ( ( offsetx + aModule->GetPosition().x ) *
                             aModel.scale + aModel.tx ) << " ";
            aOutputFile << ( -(offsety + aModule->GetPosition().y) *
                             aModel.scale - aModel.ty ) << " ";
            aOutputFile << ( (offsetz * aModel.scale ) +
                             aModel.GetLayerZ( aModule->GetLayer() ) ) << "\n";
            aOutputFile << "  scale ";
            aOutputFile << ( vrmlm->m_MatScale.x * aVRMLModelsToBiu ) << " ";
            aOutputFile << ( vrmlm->m_MatScale.y * aVRMLModelsToBiu ) << " ";
            aOutputFile << ( vrmlm->m_MatScale.z * aVRMLModelsToBiu ) << "\n";
            aOutputFile << "  children [\n    Inline {\n      url \"";

            if( aUseRelativePaths )
            {
                wxFileName tmp = destFileName;
                tmp.SetExt( wxT( "" ) );
                tmp.SetName( wxT( "" ) );
                tmp.RemoveLastDir();
                destFileName.MakeRelativeTo( tmp.GetPath() );
            }

            wxString fn = destFileName.GetFullPath();
            fn.Replace( wxT( "\\" ), wxT( "/" ) );
            aOutputFile << TO_UTF8( fn ) << "\"\n    } ]\n";
            aOutputFile << "  }\n";
        }
    }
}


bool PCB_EDIT_FRAME::ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                                      bool aExport3DFiles, bool aUseRelativePaths,
                                      bool aUsePlainPCB, const wxString& a3D_Subdir )
{
    wxString        msg;
    BOARD*          pcb = GetBoard();
    bool            ok  = true;

    MODEL_VRML model3d;
    model3d.plainPCB = aUsePlainPCB;

    model_vrml = &model3d;
    std::ofstream output_file;

    try
    {
        output_file.exceptions( std::ofstream::failbit );
        output_file.open( TO_UTF8( aFullFileName ), std::ios_base::out );

        // Switch the locale to standard C (needed to print floating point numbers like 1.3)
        SetLocaleTo_C_standard();

        // Begin with the usual VRML boilerplate
        wxString fn = aFullFileName;
        fn.Replace( wxT( "\\" ), wxT( "/" ) );
        output_file << "#VRML V2.0 utf8\n";
        output_file << "WorldInfo {\n";
        output_file << "  title \"" << TO_UTF8( fn ) << " - Generated by Pcbnew\"\n";
        output_file << "}\n";

        // Set the VRML world scale factor
        model3d.SetScale( aMMtoWRMLunit );

        output_file << "Transform {\n";

        // compute the offset to center the board on (0, 0, 0)
        // XXX - NOTE: we should allow the user a GUI option to specify the offset
        EDA_RECT bbbox = pcb->ComputeBoundingBox();

        model3d.SetOffset( -model3d.scale * bbbox.Centre().x, model3d.scale * bbbox.Centre().y );

        output_file << "  children [\n";

        // Preliminary computation: the z value for each layer
        compute_layer_Zs( model3d, pcb );

        // board edges and cutouts
        export_vrml_board( model3d, pcb );

        // Drawing and text on the board
        if( !aUsePlainPCB )
            export_vrml_drawings( model3d, pcb );

        // Export vias and trackage
        export_vrml_tracks( model3d, pcb );

        // Export zone fills
        if( !aUsePlainPCB )
            export_vrml_zones( model3d, pcb);

        /* scaling factor to convert 3D models to board units (decimils)
         * Usually we use Wings3D to create thems.
         * One can consider the 3D units is 0.1 inch (2.54 mm)
         * So the scaling factor from 0.1 inch to board units
         * is 2.54 * aMMtoWRMLunit
         */
        double wrml_3D_models_scaling_factor = 2.54 * aMMtoWRMLunit;

        // Export footprints
        for( MODULE* module = pcb->m_Modules; module != 0; module = module->Next() )
            export_vrml_module( model3d, pcb, module, output_file, wrml_3D_models_scaling_factor,
                                aExport3DFiles, aUseRelativePaths, a3D_Subdir );

            // write out the board and all layers
            write_layers( model3d, output_file, pcb );

        // Close the outer 'transform' node
        output_file << "]\n}\n";
    }
    catch( const std::exception& e )
    {
        wxString msg;
        msg << _( "IDF Export Failed:\n" ) << FROM_UTF8( e.what() );
        wxMessageBox( msg );

        ok = false;
    }

    // End of work
    output_file.exceptions( std::ios_base::goodbit );
    output_file.close();
    SetLocaleTo_Default();       // revert to the current  locale

    return ok;
}
