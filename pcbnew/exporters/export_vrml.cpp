/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2014-2017  Cirilo Bernardo
 * Copyright (C) 2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <exception>
#include <fstream>
#include <iomanip>
#include <vector>
#include <wx/dir.h>

#include "3d_cache/3d_cache.h"
#include "3d_cache/3d_info.h"
#include "board.h"
#include "fp_shape.h"
#include "footprint.h"
#include "pcb_text.h"
#include "track.h"
#include "zone.h"
#include "convert_to_biu.h"
#include <core/arraydim.h>
#include <filename_resolver.h>
#include "plugins/3dapi/ifsg_all.h"
#include "streamwrapper.h"
#include "vrml_layer.h"
#include "pcb_edit_frame.h"

#include <bezier_curves.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <zone_filler.h>

#include <exporter_vrml.h>


static bool USE_INLINES;            // true to use legacy inline{} behavior
static bool USE_DEFS;               // true to reuse component definitions
static bool USE_RELPATH;            // true to use relative paths in VRML inline{}
static double WORLD_SCALE = 1.0;    // scaling from 0.1 in to desired VRML unit
static double BOARD_SCALE;          // scaling from mm to desired VRML world scale
static const int PRECISION = 6;     // legacy precision factor (now set to 6)
static wxString SUBDIR_3D;          // legacy 3D subdirectory
static wxString PROJ_DIR;           // project directory


static S3D_CACHE* cache;
VRML_COLOR colors[VRML_COLOR_LAST];
static SGNODE* sgmaterial[VRML_COLOR_LAST] = { NULL };



MODEL_VRML::MODEL_VRML() :
        m_OutputPCB( (SGNODE*) NULL )
{
    for( unsigned i = 0; i < arrayDim( m_layer_z );  ++i )
        m_layer_z[i] = 0;

    m_holes.GetArcParams( m_iMaxSeg, m_arcMinLen, m_arcMaxLen );

    // this default only makes sense if the output is in mm
    m_brd_thickness = 1.6;

    // pcb green
    colors[VRML_COLOR_PCB] = VRML_COLOR(
            0.07f, 0.3f, 0.12f, 0.01f, 0.03f, 0.01f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.02f );
    // copper color
    colors[VRML_COLOR_COPPER] = VRML_COLOR(
            0.72f, 0.45f, 0.2f, 0.01f, 0.05f, 0.01f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.02f );
    // silkscreen white
    colors[VRML_COLOR_SILK] = VRML_COLOR(
            0.9f, 0.9f, 0.9f, 0.1f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.02f );
    // solder paste silver
    colors[VRML_COLOR_TIN] = VRML_COLOR( 0.749f, 0.756f, 0.761f, 0.749f, 0.756f, 0.761f, 0.0f,
            0.0f, 0.0f, 0.8f, 0.0f, 0.8f );

    m_plainPCB = false;
    SetOffset( 0.0, 0.0 );
    m_text_layer = F_Cu;
    m_text_width = 1;
    m_minLineWidth = MIN_VRML_LINEWIDTH;
}


MODEL_VRML::~MODEL_VRML()
{
    // destroy any unassociated material appearances
    for( int j = 0; j < VRML_COLOR_LAST; ++j )
    {
        if( sgmaterial[j] && NULL == S3D::GetSGNodeParent( sgmaterial[j] ) )
            S3D::DestroyNode( sgmaterial[j] );

        sgmaterial[j] = NULL;
    }

    if( !m_components.empty() )
    {
        IFSG_TRANSFORM tmp( false );

        for( auto i : m_components )
        {
            tmp.Attach( i );
            tmp.SetParent( NULL );
        }

        m_components.clear();
        m_OutputPCB.Destroy();
    }
}


bool MODEL_VRML::SetScale( double aWorldScale )
{
    // set the scaling of the VRML world
    if( aWorldScale < 0.001 || aWorldScale > 10.0 )
        throw( std::runtime_error( "WorldScale out of range (valid range is 0.001 to 10.0)" ) );

    m_OutputPCB.SetScale( aWorldScale * 2.54 );
    WORLD_SCALE = aWorldScale * 2.54;

    return true;
}


void MODEL_VRML::SetOffset( double aXoff, double aYoff )
{
    m_tx = aXoff;
    m_ty = -aYoff;

    m_holes.SetVertexOffsets( aXoff, aYoff );
    m_board.SetVertexOffsets( aXoff, aYoff );
    m_top_copper.SetVertexOffsets( aXoff, aYoff );
    m_bot_copper.SetVertexOffsets( aXoff, aYoff );
    m_top_silk.SetVertexOffsets( aXoff, aYoff );
    m_bot_silk.SetVertexOffsets( aXoff, aYoff );
    m_top_tin.SetVertexOffsets( aXoff, aYoff );
    m_bot_tin.SetVertexOffsets( aXoff, aYoff );
    m_plated_holes.SetVertexOffsets( aXoff, aYoff );
}


bool MODEL_VRML::GetLayer3D( LAYER_NUM layer, VRML_LAYER** vlayer )
{
    // select the VRML layer object to draw on; return true if
    // a layer has been selected.
    switch( layer )
    {
    case B_Cu:    *vlayer = &m_bot_copper; return true;
    case F_Cu:    *vlayer = &m_top_copper; return true;
    case B_SilkS: *vlayer = &m_bot_silk;   return true;
    case F_SilkS: *vlayer = &m_top_silk;   return true;
    default:      return false;
    }
}

void MODEL_VRML::ExportVrmlSolderMask( BOARD* aPcb )
{
}


// static var. for dealing with text
static MODEL_VRML* model_vrml;


static void create_vrml_shell( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                               VRML_LAYER* layer, double top_z, double bottom_z );

static void create_vrml_plane( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
                               VRML_LAYER* layer, double aHeight, bool aTopPlane );

static void write_triangle_bag( std::ostream& aOut_file, const VRML_COLOR& aColor,
                                VRML_LAYER* aLayer, bool aPlane, bool aTop,
                                double aTop_z, double aBottom_z )
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
            aOut_file << shape_boiler[lineno];
        else
        {
            marker_found++;

            switch( marker_found )
            {
            case 1:    // Material marker
            {
                std::streamsize lastPrecision = aOut_file.precision();
                aOut_file << "              diffuseColor " << std::setprecision(3);
                aOut_file << aColor.diffuse_red << " ";
                aOut_file << aColor.diffuse_grn << " ";
                aOut_file << aColor.diffuse_blu << "\n";

                aOut_file << "              specularColor ";
                aOut_file << aColor.spec_red << " ";
                aOut_file << aColor.spec_grn << " ";
                aOut_file << aColor.spec_blu << "\n";

                aOut_file << "              emissiveColor ";
                aOut_file << aColor.emit_red << " ";
                aOut_file << aColor.emit_grn << " ";
                aOut_file << aColor.emit_blu << "\n";

                aOut_file << "              ambientIntensity " << aColor.ambient << "\n";
                aOut_file << "              transparency " << aColor.transp << "\n";
                aOut_file << "              shininess " << aColor.shiny << "\n";
                aOut_file.precision( lastPrecision );
            }
                break;

            case 2:

                if( aPlane )
                    aLayer->WriteVertices( aTop_z, aOut_file, PRECISION );
                else
                    aLayer->Write3DVertices( aTop_z, aBottom_z, aOut_file, PRECISION );

                aOut_file << "\n";
                break;

            case 3:

                if( aPlane )
                    aLayer->WriteIndices( aTop, aOut_file );
                else
                    aLayer->Write3DIndices( aOut_file );

                aOut_file << "\n";
                break;

            default:
                break;
            }
        }

        lineno++;
    }
}


static void write_layers( MODEL_VRML& aModel, BOARD* aPcb, const char* aFileName,
                          OSTREAM* aOutputFile )
{
    // VRML_LAYER board;
    aModel.m_board.Tesselate( &aModel.m_holes );
    double brdz = aModel.m_brd_thickness / 2.0
                  - ( Millimeter2iu( ART_OFFSET / 2.0 ) ) * BOARD_SCALE;

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_PCB ),
                            &aModel.m_board, false, false, brdz, -brdz );
    }
    else
    {
        create_vrml_shell( aModel.m_OutputPCB, VRML_COLOR_PCB, &aModel.m_board, brdz, -brdz );
    }

    if( aModel.m_plainPCB )
    {
        if( !USE_INLINES )
            S3D::WriteVRML( aFileName, true, aModel.m_OutputPCB.GetRawPtr(), USE_DEFS, true );

        return;
    }

    // VRML_LAYER m_top_copper;
    aModel.m_top_copper.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_COPPER ),
                            &aModel.m_top_copper, true, true,
                            aModel.GetLayerZ( F_Cu ), 0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_COPPER, &aModel.m_top_copper,
                           aModel.GetLayerZ( F_Cu ), true );
    }

    // VRML_LAYER m_top_tin;
    aModel.m_top_tin.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_TIN ),
                            &aModel.m_top_tin, true, true,
                            aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                            0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_TIN, &aModel.m_top_tin,
                           aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                           true );
    }

    // VRML_LAYER m_bot_copper;
    aModel.m_bot_copper.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_COPPER ),
                            &aModel.m_bot_copper, true, false,
                            aModel.GetLayerZ( B_Cu ), 0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_COPPER, &aModel.m_bot_copper,
                           aModel.GetLayerZ( B_Cu ), false );
    }

    // VRML_LAYER m_bot_tin;
    aModel.m_bot_tin.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_TIN ),
                            &aModel.m_bot_tin, true, false,
                            aModel.GetLayerZ( B_Cu )
                            - Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                            0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_TIN, &aModel.m_bot_tin,
                           aModel.GetLayerZ( B_Cu ) - Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                           false );
    }

    // VRML_LAYER PTH;
    aModel.m_plated_holes.Tesselate( NULL, true );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_TIN ),
                            &aModel.m_plated_holes, false, false,
                            aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                            aModel.GetLayerZ( B_Cu ) - Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE );
    }
    else
    {
        create_vrml_shell( aModel.m_OutputPCB, VRML_COLOR_TIN, &aModel.m_plated_holes,
                           aModel.GetLayerZ( F_Cu ) + Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE,
                           aModel.GetLayerZ( B_Cu ) - Millimeter2iu( ART_OFFSET / 2.0 ) * BOARD_SCALE );
    }

    // VRML_LAYER m_top_silk;
    aModel.m_top_silk.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_SILK ), &aModel.m_top_silk,
                            true, true, aModel.GetLayerZ( F_SilkS ), 0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_SILK, &aModel.m_top_silk,
                           aModel.GetLayerZ( F_SilkS ), true );
    }

    // VRML_LAYER m_bot_silk;
    aModel.m_bot_silk.Tesselate( &aModel.m_holes );

    if( USE_INLINES )
    {
        write_triangle_bag( *aOutputFile, aModel.GetColor( VRML_COLOR_SILK ), &aModel.m_bot_silk,
                            true, false, aModel.GetLayerZ( B_SilkS ), 0 );
    }
    else
    {
        create_vrml_plane( aModel.m_OutputPCB, VRML_COLOR_SILK, &aModel.m_bot_silk,
                           aModel.GetLayerZ( B_SilkS ), false );
    }

    if( !USE_INLINES )
        S3D::WriteVRML( aFileName, true, aModel.m_OutputPCB.GetRawPtr(), true, true );
}


void MODEL_VRML::ComputeLayer3D_Zpos( BOARD* pcb )
{
    int copper_layers = pcb->GetCopperLayerCount();

    // We call it 'layer' thickness, but it's the whole board thickness!
    m_brd_thickness = pcb->GetDesignSettings().GetBoardThickness() * BOARD_SCALE;
    double half_thickness = m_brd_thickness / 2;

    // Compute each layer's Z value, more or less like the 3d view
    for( LSEQ seq = LSET::AllCuMask().Seq();  seq;  ++seq )
    {
        PCB_LAYER_ID i = *seq;

        if( i < copper_layers )
            SetLayerZ( i,  half_thickness - m_brd_thickness * i / (copper_layers - 1) );
        else
            SetLayerZ( i, - half_thickness );  // bottom layer
    }

    // To avoid rounding interference, we apply an epsilon to each successive layer
    double epsilon_z = Millimeter2iu( ART_OFFSET ) * BOARD_SCALE;
    SetLayerZ( B_Paste, -half_thickness - epsilon_z * 4 );
    SetLayerZ( B_Adhes, -half_thickness - epsilon_z * 3 );
    SetLayerZ( B_SilkS, -half_thickness - epsilon_z * 2 );
    SetLayerZ( B_Mask, -half_thickness - epsilon_z );
    SetLayerZ( F_Mask, half_thickness + epsilon_z );
    SetLayerZ( F_SilkS, half_thickness + epsilon_z * 2 );
    SetLayerZ( F_Adhes, half_thickness + epsilon_z * 3 );
    SetLayerZ( F_Paste, half_thickness + epsilon_z * 4 );
    SetLayerZ( Dwgs_User, half_thickness + epsilon_z * 5 );
    SetLayerZ( Cmts_User, half_thickness + epsilon_z * 6 );
    SetLayerZ( Eco1_User, half_thickness + epsilon_z * 7 );
    SetLayerZ( Eco2_User, half_thickness + epsilon_z * 8 );
    SetLayerZ( Edge_Cuts, 0 );
}


void MODEL_VRML::ExportVrmlLine( LAYER_NUM layer,
                                 double startx, double starty,
                                 double endx, double endy, double width )
{
    VRML_LAYER* vlayer;

    if( !GetLayer3D( layer, &vlayer ) )
        return;

    if( width < m_minLineWidth)
        width = m_minLineWidth;

    starty = -starty;
    endy = -endy;

    double  angle   = atan2( endy - starty, endx - startx ) * 180.0 / M_PI;
    double  length  = Distance( startx, starty, endx, endy ) + width;
    double  cx  = ( startx + endx ) / 2.0;
    double  cy  = ( starty + endy ) / 2.0;

    if( !vlayer->AddSlot( cx, cy, length, width, angle, false ) )
        throw( std::runtime_error( vlayer->GetError() ) );
}


void MODEL_VRML::ExportVrmlCircle( LAYER_NUM layer,
                                double startx, double starty,
                                double endx, double endy, double width )
{
    VRML_LAYER* vlayer;

    if( !GetLayer3D( layer, &vlayer ) )
        return;

    if( width < m_minLineWidth )
        width = m_minLineWidth;

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


void MODEL_VRML::ExportVrmlArc( LAYER_NUM layer,
                             double centerx, double centery,
                             double arc_startx, double arc_starty,
                             double width, double arc_angle )
{
    VRML_LAYER* vlayer;

    if( !GetLayer3D( layer, &vlayer ) )
        return;

    if( width < m_minLineWidth )
        width = m_minLineWidth;

    centery = -centery;
    arc_starty = -arc_starty;

    if( !vlayer->AddArc( centerx, centery, arc_startx, arc_starty, width, -arc_angle, false ) )
        throw( std::runtime_error( vlayer->GetError() ) );

}


void MODEL_VRML::ExportVrmlPolygon( LAYER_NUM layer, PCB_SHAPE *aOutline,
                                      double aOrientation, wxPoint aPos )
{
    if( aOutline->IsPolyShapeValid() )
    {
        SHAPE_POLY_SET shape = aOutline->GetPolyShape();
        VRML_LAYER* vlayer;

        if( !GetLayer3D( layer, &vlayer ) )
                return;

        if( aOutline->GetWidth() )
        {
            int numSegs = GetArcToSegmentCount( aOutline->GetWidth() / 2, ARC_HIGH_DEF, 360.0 );
            shape.Inflate( aOutline->GetWidth() / 2, numSegs );
            shape.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        }

        shape.Rotate( -aOrientation, VECTOR2I( 0, 0 ) );
        shape.Move( aPos );
        const SHAPE_LINE_CHAIN& outline = shape.COutline( 0 );

        int seg = vlayer->NewContour();

        for( int j = 0; j < outline.PointCount(); j++ )
        {
            if( !vlayer->AddVertex( seg, outline.CPoint( j ).x * BOARD_SCALE,
                                     -outline.CPoint( j ).y * BOARD_SCALE ) )
                throw( std::runtime_error( vlayer->GetError() ) );
        }

        vlayer->EnsureWinding( seg, false );
    }
}


void MODEL_VRML::ExportVrmlDrawsegment( PCB_SHAPE* drawseg )
{
    LAYER_NUM layer = drawseg->GetLayer();
    double  w   = drawseg->GetWidth() * BOARD_SCALE;
    double  x   = drawseg->GetStart().x * BOARD_SCALE;
    double  y   = drawseg->GetStart().y * BOARD_SCALE;
    double  xf  = drawseg->GetEnd().x * BOARD_SCALE;
    double  yf  = drawseg->GetEnd().y * BOARD_SCALE;
    double  r   = sqrt( pow( x - xf, 2 ) + pow( y - yf, 2 ) );

    // Items on the edge layer are handled elsewhere; just return
    if( layer == Edge_Cuts )
        return;

    switch( drawseg->GetShape() )
    {
    case S_ARC:
        ExportVrmlArc( layer,
                         (double) drawseg->GetCenter().x * BOARD_SCALE,
                         (double) drawseg->GetCenter().y * BOARD_SCALE,
                         (double) drawseg->GetArcStart().x * BOARD_SCALE,
                         (double) drawseg->GetArcStart().y * BOARD_SCALE,
                         w, drawseg->GetAngle() / 10 );
        break;

    case S_CIRCLE:
        // Break circles into two 180 arcs to prevent the vrml hole from obscuring objects
        // within the hole area of the circle.
        ExportVrmlArc( layer, x, y, x, y-r, w, 180.0 );
        ExportVrmlArc( layer, x, y, x, y+r, w, 180.0 );
        break;

    case S_POLYGON:
        ExportVrmlPolygon( layer, drawseg, 0.0, wxPoint( 0, 0 ) );
        break;

    case S_SEGMENT:
        ExportVrmlLine( layer, x, y, xf, yf, w );
        break;

    case S_CURVE:
    {
        std::vector<VECTOR2D> output;
        std::vector<VECTOR2D> pointCtrl;

        pointCtrl.emplace_back( x, y );
        pointCtrl.emplace_back( drawseg->GetBezControl1().x * BOARD_SCALE,
                drawseg->GetBezControl1().y * BOARD_SCALE );
        pointCtrl.emplace_back( drawseg->GetBezControl2().x * BOARD_SCALE,
                drawseg->GetBezControl2().y * BOARD_SCALE );
        pointCtrl.emplace_back( xf, yf );

        BEZIER_POLY converter( pointCtrl );
        converter.GetPoly( output, w );

        for( size_t i = 1; i < output.size(); ++i )
        {
            ExportVrmlLine( layer, output[i - 1].x, output[i - 1].y,
                                             output[i].x, output[i].y, w );
        }

        break;
    }

    case S_RECT:
        ExportVrmlLine( layer, x, y, xf, y, w );
        ExportVrmlLine( layer, xf, y, xf, yf, w );
        ExportVrmlLine( layer, xf, yf, x, yf, w );
        ExportVrmlLine( layer, x, yf, x, y, w );
        break;

    default:
        break;
    }
}


/* C++ doesn't have closures and neither continuation forms... this is
 * for coupling the vrml_text_callback with the common parameters */
static void vrml_text_callback( int x0, int y0, int xf, int yf, void* aData )
{
    LAYER_NUM m_text_layer = model_vrml->m_text_layer;
    int m_text_width = model_vrml->m_text_width;

    model_vrml->ExportVrmlLine( m_text_layer,
                      x0 * BOARD_SCALE, y0 * BOARD_SCALE,
                      xf * BOARD_SCALE, yf * BOARD_SCALE,
                      m_text_width * BOARD_SCALE );
}


void MODEL_VRML::ExportVrmlPcbtext( PCB_TEXT* text )
{
    wxSize size = text->GetTextSize();

    if( text->IsMirrored() )
        size.x = -size.x;

    bool    forceBold = true;
    int     penWidth = text->GetEffectiveTextPenWidth();
    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by GRText

    model_vrml->m_text_layer    = text->GetLayer();
    model_vrml->m_text_width    = penWidth;

    if( text->IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( text->GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        text->GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            GRText( nullptr, positions[ii], color, strings_list[ii], text->GetTextAngle(), size,
                    text->GetHorizJustify(), text->GetVertJustify(), penWidth, text->IsItalic(),
                    forceBold, vrml_text_callback );
        }
    }
    else
    {
        GRText( nullptr, text->GetTextPos(), color, text->GetShownText(), text->GetTextAngle(),
                size, text->GetHorizJustify(), text->GetVertJustify(), penWidth, text->IsItalic(),
                forceBold, vrml_text_callback );
    }
}


void MODEL_VRML::ExportVrmlDrawings( BOARD* pcb )
{
    // draw graphic items
    for( auto drawing : pcb->Drawings() )
    {
        PCB_LAYER_ID layer = drawing->GetLayer();

        if( layer != F_Cu && layer != B_Cu && layer != B_SilkS && layer != F_SilkS )
            continue;

        switch( drawing->Type() )
        {
        case PCB_SHAPE_T:
            ExportVrmlDrawsegment( (PCB_SHAPE*) drawing );
            break;

        case PCB_TEXT_T:
            ExportVrmlPcbtext( (PCB_TEXT*) drawing );
            break;

        default:
            break;
        }
    }
}


// board edges and cutouts
void MODEL_VRML::ExportVrmlBoard( BOARD* aPcb )
{
    if( !aPcb->GetBoardPolygonOutlines( m_pcbOutlines ) )
    {
        wxLogWarning( _( "Board outline is malformed. Run DRC for a full analysis." ) );
    }

    int seg;

    for( int cnt = 0; cnt < m_pcbOutlines.OutlineCount(); cnt++ )
    {
        const SHAPE_LINE_CHAIN& outline = m_pcbOutlines.COutline( cnt );

        seg = m_board.NewContour();

        for( int j = 0; j < outline.PointCount(); j++ )
        {
            m_board.AddVertex( seg, (double)outline.CPoint(j).x * BOARD_SCALE,
                                    -((double)outline.CPoint(j).y * BOARD_SCALE ) );

        }

        m_board.EnsureWinding( seg, false );

        // Generate board holes from outlines:
        for( int ii = 0; ii < m_pcbOutlines.HoleCount( cnt ); ii++ )
        {
            const SHAPE_LINE_CHAIN& hole = m_pcbOutlines.Hole( cnt, ii );

            seg = m_holes.NewContour();

            if( seg < 0 )
            {
                wxLogError( _( "VRML Export Failed: Could not add holes to contours." ) );
                return;
            }

            for( int j = 0; j < hole.PointCount(); j++ )
            {
                m_holes.AddVertex( seg, (double) hole.CPoint(j).x * BOARD_SCALE,
                                          -( (double) hole.CPoint(j).y * BOARD_SCALE ) );
            }

            m_holes.EnsureWinding( seg, true );
        }
    }
}


void MODEL_VRML::ExportRoundPadstack( BOARD* pcb,
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
        m_holes.AddCircle( x, -y, hole, true );

    if( m_plainPCB )
        return;

    while( true )
    {
        if( layer == B_Cu )
        {
            m_bot_copper.AddCircle( x, -y, r );

            if( hole > 0 && !thru )
                m_bot_copper.AddCircle( x, -y, hole, true );

        }
        else if( layer == F_Cu )
        {
            m_top_copper.AddCircle( x, -y, r );

            if( hole > 0 && !thru )
                m_top_copper.AddCircle( x, -y, hole, true );

        }

        if( layer == bottom_layer )
            break;

        layer = bottom_layer;
    }
}


void MODEL_VRML::ExportVrmlVia( BOARD* aPcb, const VIA* aVia )
{
    double       x, y, r, hole;
    PCB_LAYER_ID top_layer, bottom_layer;

    hole = aVia->GetDrillValue() * BOARD_SCALE / 2.0;
    r    = aVia->GetWidth() * BOARD_SCALE / 2.0;
    x    = aVia->GetStart().x * BOARD_SCALE;
    y    = aVia->GetStart().y * BOARD_SCALE;
    aVia->LayerPair( &top_layer, &bottom_layer );

    // do not render a buried via
    if( top_layer != F_Cu && bottom_layer != B_Cu )
        return;

    // Export the via padstack
    ExportRoundPadstack( aPcb, x, y, r, bottom_layer, top_layer, hole );
}


void MODEL_VRML::ExportVrmlTracks( BOARD* pcb )
{
    for( TRACK* track : pcb->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            ExportVrmlVia( pcb, (const VIA*) track );
        }
        else if( ( track->GetLayer() == B_Cu || track->GetLayer() == F_Cu ) && !m_plainPCB )
        {
            if( track->Type() == PCB_ARC_T )
            {
                ARC* arc = static_cast<ARC*>( track );
                VECTOR2D center( arc->GetCenter() );
                double arc_angle_degree = arc->GetAngle()/10;

                // Vrml exporter does not export arcs with angle < 1.0 degree
                // ( to avoid issues with vrml viewers).
                // The best way is to convert them to a small straight line
                if( arc_angle_degree < -1.0 || arc_angle_degree > 1.0 )
                {
                    ExportVrmlArc( track->GetLayer(),
                                     center.x * BOARD_SCALE, center.y * BOARD_SCALE,
                                     arc->GetStart().x * BOARD_SCALE,
                                     arc->GetStart().y * BOARD_SCALE,
                                     arc->GetWidth() * BOARD_SCALE, arc_angle_degree );
                }
                else
                {
                    ExportVrmlLine( arc->GetLayer(),
                                      arc->GetStart().x * BOARD_SCALE,
                                      arc->GetStart().y * BOARD_SCALE,
                                      arc->GetEnd().x * BOARD_SCALE,
                                      arc->GetEnd().y * BOARD_SCALE,
                                      arc->GetWidth() * BOARD_SCALE );
                }
            }
            else
            {
                ExportVrmlLine( track->GetLayer(),
                                  track->GetStart().x * BOARD_SCALE,
                                  track->GetStart().y * BOARD_SCALE,
                                  track->GetEnd().x * BOARD_SCALE,
                                  track->GetEnd().y * BOARD_SCALE,
                                  track->GetWidth() * BOARD_SCALE );
            }
        }
    }
}


void MODEL_VRML::ExportVrmlZones( BOARD* aPcb, COMMIT* aCommit )
{
    for( ZONE* zone : aPcb->Zones() )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            VRML_LAYER* vl;

            if( !GetLayer3D( layer, &vl ) )
                continue;

            if( !zone->IsFilled() )
            {
                ZONE_FILLER filler( aPcb, aCommit );
                zone->SetFillMode( ZONE_FILL_MODE::POLYGONS ); // use filled polygons

                // If the zone fill failed, don't try adding it to the export
                std::vector<ZONE*> toFill = { zone };

                if( !filler.Fill( toFill ) )
                    continue;
            }

            const SHAPE_POLY_SET& poly = zone->GetFilledPolysList( layer );

            for( int i = 0; i < poly.OutlineCount(); i++ )
            {
                const SHAPE_LINE_CHAIN& outline = poly.COutline( i );

                int seg = vl->NewContour();

                for( int j = 0; j < outline.PointCount(); j++ )
                {
                    if( !vl->AddVertex( seg, (double) outline.CPoint( j ).x * BOARD_SCALE,
                                -( (double) outline.CPoint( j ).y * BOARD_SCALE ) ) )
                    {
                        throw( std::runtime_error( vl->GetError() ) );
                    }
                }

                vl->EnsureWinding( seg, false );
            }
        }
    }
}


static void export_vrml_fp_text( FP_TEXT* item )
{
    if( item->IsVisible() )
    {
        wxSize size = item->GetTextSize();

        if( item->IsMirrored() )
            size.x = -size.x;  // Text is mirrored

        bool forceBold = true;
        int  penWidth = item->GetEffectiveTextPenWidth();

        model_vrml->m_text_layer = item->GetLayer();
        model_vrml->m_text_width = penWidth;

        GRText( NULL, item->GetTextPos(), BLACK, item->GetShownText(), item->GetDrawRotation(),
                size, item->GetHorizJustify(), item->GetVertJustify(), penWidth, item->IsItalic(),
                forceBold, vrml_text_callback );
    }
}


void MODEL_VRML::ExportVrmlFpShape( FP_SHAPE* aOutline, FOOTPRINT* aFootprint )
{
    LAYER_NUM layer = aOutline->GetLayer();
    double  x   = aOutline->GetStart().x * BOARD_SCALE;
    double  y   = aOutline->GetStart().y * BOARD_SCALE;
    double  xf  = aOutline->GetEnd().x * BOARD_SCALE;
    double  yf  = aOutline->GetEnd().y * BOARD_SCALE;
    double  w   = aOutline->GetWidth() * BOARD_SCALE;

    switch( aOutline->GetShape() )
    {
    case S_SEGMENT:
        ExportVrmlLine( layer, x, y, xf, yf, w );
        break;

    case S_ARC:
        ExportVrmlArc( layer, x, y, xf, yf, w, aOutline->GetAngle() / 10 );
        break;

    case S_CIRCLE:
        ExportVrmlCircle( layer, x, y, xf, yf, w );
        break;

    case S_POLYGON:
        ExportVrmlPolygon( layer, aOutline, aFootprint->GetOrientationRadians(),
                             aFootprint->GetPosition() );
        break;

    case S_CURVE:
    {
        std::vector<VECTOR2D> output;
        std::vector<VECTOR2D> pointCtrl;

        pointCtrl.emplace_back( x, y );
        pointCtrl.emplace_back( aOutline->GetBezControl1().x * BOARD_SCALE,
                aOutline->GetBezControl1().y * BOARD_SCALE );
        pointCtrl.emplace_back( aOutline->GetBezControl2().x * BOARD_SCALE,
                aOutline->GetBezControl2().y * BOARD_SCALE );
        pointCtrl.emplace_back( xf, yf );

        BEZIER_POLY converter( pointCtrl );
        converter.GetPoly( output, w );

        for( size_t i = 1; i < output.size(); ++i )
        {
            ExportVrmlLine( layer, output[i - 1].x, output[i - 1].y,
                                             output[i].x, output[i].y, w );
        }

        break;
    }

    case S_RECT:
        ExportVrmlLine( layer, x, y, xf, y, w );
        ExportVrmlLine( layer, xf, y, xf, yf, w );
        ExportVrmlLine( layer, xf, yf, x, yf, w );
        ExportVrmlLine( layer, x, yf, x, y, w );
        break;

    default:
        break;
    }
}


void MODEL_VRML::ExportVrmlPadshape( VRML_LAYER* aTinLayer, PAD* aPad )
{
    // The (maybe offset) pad position
    wxPoint pad_pos = aPad->ShapePos();
    double  pad_x   = pad_pos.x * BOARD_SCALE;
    double  pad_y   = pad_pos.y * BOARD_SCALE;
    wxSize  pad_delta = aPad->GetDelta();

    double  pad_dx  = pad_delta.x * BOARD_SCALE / 2.0;
    double  pad_dy  = pad_delta.y * BOARD_SCALE / 2.0;

    double  pad_w   = aPad->GetSize().x * BOARD_SCALE / 2.0;
    double  pad_h   = aPad->GetSize().y * BOARD_SCALE / 2.0;

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:

        if( !aTinLayer->AddCircle( pad_x, -pad_y, pad_w, false ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;

    case PAD_SHAPE_OVAL:

        if( !aTinLayer->AddSlot( pad_x, -pad_y, pad_w * 2.0, pad_h * 2.0,
                                 aPad->GetOrientation()/10.0, false ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;

    case PAD_SHAPE_ROUNDRECT:
    case PAD_SHAPE_CHAMFERED_RECT:
    {
        SHAPE_POLY_SET polySet;
        const int corner_radius = aPad->GetRoundRectCornerRadius();
        TransformRoundChamferedRectToPolygon( polySet, wxPoint( 0, 0 ), aPad->GetSize(), 0.0,
                                              corner_radius, 0.0, 0, ARC_HIGH_DEF, ERROR_INSIDE );
        std::vector< wxRealPoint > cornerList;
        // TransformRoundChamferedRectToPolygon creates only one convex polygon
        SHAPE_LINE_CHAIN poly( polySet.Outline( 0 ) );

        cornerList.reserve( poly.PointCount() );
        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back(
                    poly.CPoint( ii ).x * BOARD_SCALE, -poly.CPoint( ii ).y * BOARD_SCALE );

        // Close polygon
        cornerList.push_back( cornerList[0] );
        if( !aTinLayer->AddPolygon( cornerList, pad_x, -pad_y, aPad->GetOrientation() ) )
            throw( std::runtime_error( aTinLayer->GetError() ) );

        break;
    }

    case PAD_SHAPE_CUSTOM:
    {
        SHAPE_POLY_SET polySet;
        std::vector< wxRealPoint > cornerList;
        aPad->MergePrimitivesAsPolygon( &polySet, UNDEFINED_LAYER );

        for( int cnt = 0; cnt < polySet.OutlineCount(); ++cnt )
        {
            SHAPE_LINE_CHAIN& poly = polySet.Outline( cnt );
            cornerList.clear();

            for( int ii = 0; ii < poly.PointCount(); ++ii )
                cornerList.emplace_back(
                        poly.CPoint( ii ).x * BOARD_SCALE, -poly.CPoint( ii ).y * BOARD_SCALE );

            // Close polygon
            cornerList.push_back( cornerList[0] );

            if( !aTinLayer->AddPolygon( cornerList, pad_x, -pad_y, aPad->GetOrientation() ) )
                throw( std::runtime_error( aTinLayer->GetError() ) );
        }

        break;
    }

    case PAD_SHAPE_RECT:
        // Just to be sure :D
        pad_dx  = 0;
        pad_dy  = 0;

        // Intentionally fall through and treat a rectangle as a trapezoid with no sloped sides
        KI_FALLTHROUGH;

    case PAD_SHAPE_TRAPEZOID:
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
    }
}


void MODEL_VRML::ExportVrmlPad( BOARD* aPcb, PAD* aPad )
{
    double  hole_drill_w    = (double) aPad->GetDrillSize().x * BOARD_SCALE / 2.0;
    double  hole_drill_h    = (double) aPad->GetDrillSize().y * BOARD_SCALE / 2.0;
    double  hole_drill      = std::min( hole_drill_w, hole_drill_h );
    double  hole_x          = aPad->GetPosition().x * BOARD_SCALE;
    double  hole_y          = aPad->GetPosition().y * BOARD_SCALE;

    // Export the hole on the edge layer
    if( hole_drill > 0 )
    {
        bool pth = false;

        if( ( aPad->GetAttribute() != PAD_ATTRIB_NPTH ) && !m_plainPCB )
            pth = true;

        if( aPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG )
        {
            // Oblong hole (slot)

            if( pth )
            {
                m_holes.AddSlot( hole_x, -hole_y, hole_drill_w * 2.0 + PLATE_OFFSET,
                    hole_drill_h * 2.0 + PLATE_OFFSET,
                    aPad->GetOrientation()/10.0, true, true );

                m_plated_holes.AddSlot( hole_x, -hole_y,
                    hole_drill_w * 2.0, hole_drill_h * 2.0,
                    aPad->GetOrientation()/10.0, true, false );
            }
            else
            {
                m_holes.AddSlot( hole_x, -hole_y, hole_drill_w * 2.0, hole_drill_h * 2.0,
                    aPad->GetOrientation()/10.0, true, false );

            }
        }
        else
        {
            // Drill a round hole

            if( pth )
            {
                m_holes.AddCircle( hole_x, -hole_y, hole_drill + PLATE_OFFSET, true, true );
                m_plated_holes.AddCircle( hole_x, -hole_y, hole_drill, true, false );
            }
            else
            {
                m_holes.AddCircle( hole_x, -hole_y, hole_drill, true, false );
            }

        }
    }

    if( m_plainPCB )
        return;

    // The pad proper, on the selected layers
    LSET layer_mask = aPad->GetLayerSet();

    if( layer_mask[B_Cu] )
    {
        if( layer_mask[B_Paste] )
            ExportVrmlPadshape( &m_bot_tin, aPad );
        else
            ExportVrmlPadshape( &m_bot_copper, aPad );
    }
    if( layer_mask[F_Cu] )
    {
        if( layer_mask[F_Paste] )
            ExportVrmlPadshape( &m_top_tin, aPad );
        else
            ExportVrmlPadshape( &m_top_copper, aPad );
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
        rot[i] = q[i] / sin( rot[3] / 2 );
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


void MODEL_VRML::ExportVrmlFootprint( BOARD* aPcb, FOOTPRINT* aFootprint,
                                   std::ostream* aOutputFile )
{
    if( !m_plainPCB )
    {
        // Reference and value
        if( aFootprint->Reference().IsVisible() )
            export_vrml_fp_text( &aFootprint->Reference() );

        if( aFootprint->Value().IsVisible() )
            export_vrml_fp_text( &aFootprint->Value() );

        // Export footprint graphics

        for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
        {
            switch( item->Type() )
            {
            case PCB_FP_TEXT_T:
                export_vrml_fp_text( static_cast<FP_TEXT*>( item ) );
                break;

            case PCB_FP_SHAPE_T:
                ExportVrmlFpShape( static_cast<FP_SHAPE*>( item ), aFootprint );
                break;

            default:
                break;
            }
        }
    }

    // Export pads
    for( PAD* pad : aFootprint->Pads() )
        ExportVrmlPad( aPcb, pad );

    bool isFlipped = aFootprint->GetLayer() == B_Cu;

    // Export the object VRML model(s)
    auto sM = aFootprint->Models().begin();
    auto eM = aFootprint->Models().end();

    wxFileName subdir( SUBDIR_3D, "" );

    while( sM != eM )
    {
        SGNODE* mod3d = (SGNODE*) cache->Load( sM->m_Filename );

        if( NULL == mod3d )
        {
            ++sM;
            continue;
        }

        /* Calculate 3D shape rotation:
         * this is the rotation parameters, with an additional 180 deg rotation
         * for footprints that are flipped
         * When flipped, axis rotation is the horizontal axis (X axis)
         */
        double rotx = -sM->m_Rotation.x;
        double roty = -sM->m_Rotation.y;
        double rotz = -sM->m_Rotation.z;

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

        // Note here aFootprint->GetOrientation() is in 0.1 degrees, so footprint rotation
        // has to be converted to radians
        build_quat( 0, 0, 1, DECIDEG2RAD( aFootprint->GetOrientation() ), q2 );
        compose_quat( q1, q2, q1 );
        from_quat( q1, rot );

        double offsetFactor = 1000.0f * IU_PER_MILS / 25.4f;

        // adjust 3D shape local offset position
        // they are given in mm, so they are converted in board IU.
        double offsetx = sM->m_Offset.x * offsetFactor;
        double offsety = sM->m_Offset.y * offsetFactor;
        double offsetz = sM->m_Offset.z * offsetFactor;

        if( isFlipped )
            offsetz = -offsetz;
        else // In normal mode, Y axis is reversed in Pcbnew.
            offsety = -offsety;

        RotatePoint( &offsetx, &offsety, aFootprint->GetOrientation() );

        SGPOINT trans;
        trans.x = ( offsetx + aFootprint->GetPosition().x ) * BOARD_SCALE + m_tx;
        trans.y = -( offsety + aFootprint->GetPosition().y) * BOARD_SCALE - m_ty;
        trans.z = (offsetz * BOARD_SCALE ) + GetLayerZ( aFootprint->GetLayer() );

        if( USE_INLINES )
        {
            wxFileName srcFile = cache->GetResolver()->ResolvePath( sM->m_Filename );
            wxFileName dstFile;
            dstFile.SetPath( SUBDIR_3D );
            dstFile.SetName( srcFile.GetName() );
            dstFile.SetExt( "wrl"  );

            // copy the file if necessary
            wxDateTime srcModTime = srcFile.GetModificationTime();
            wxDateTime destModTime = srcModTime;

            destModTime.SetToCurrent();

            if( dstFile.FileExists() )
                destModTime = dstFile.GetModificationTime();

            if( srcModTime != destModTime )
            {
                wxString fileExt = srcFile.GetExt();
                fileExt.LowerCase();

                // copy VRML models and use the scenegraph library to
                // translate other model types
                if( fileExt == "wrl" )
                {
                    if( !wxCopyFile( srcFile.GetFullPath(), dstFile.GetFullPath() ) )
                        continue;
                }
                else
                {
                    if( !S3D::WriteVRML( dstFile.GetFullPath().ToUTF8(), true, mod3d, USE_DEFS, true ) )
                        continue;
                }
            }

            (*aOutputFile) << "Transform {\n";

            // only write a rotation if it is >= 0.1 deg
            if( std::abs( rot[3] ) > 0.0001745 )
            {
                (*aOutputFile) << "  rotation " << std::setprecision( 5 );
                (*aOutputFile) << rot[0] << " " << rot[1] << " " << rot[2] << " " << rot[3] << "\n";
            }

            (*aOutputFile) << "  translation " << std::setprecision( PRECISION );
            (*aOutputFile) << trans.x << " ";
            (*aOutputFile) << trans.y << " ";
            (*aOutputFile) << trans.z << "\n";

            (*aOutputFile) << "  scale ";
            (*aOutputFile) << sM->m_Scale.x << " ";
            (*aOutputFile) << sM->m_Scale.y << " ";
            (*aOutputFile) << sM->m_Scale.z << "\n";

            (*aOutputFile) << "  children [\n    Inline {\n      url \"";

            if( USE_RELPATH )
            {
                wxFileName tmp = dstFile;
                tmp.SetExt( "" );
                tmp.SetName( "" );
                tmp.RemoveLastDir();
                dstFile.MakeRelativeTo( tmp.GetPath() );
            }

            wxString fn = dstFile.GetFullPath();
            fn.Replace( "\\", "/" );
            (*aOutputFile) << TO_UTF8( fn ) << "\"\n    } ]\n";
            (*aOutputFile) << "  }\n";
        }
        else
        {
            IFSG_TRANSFORM* modelShape = new IFSG_TRANSFORM( m_OutputPCB.GetRawPtr() );

            // only write a rotation if it is >= 0.1 deg
            if( std::abs( rot[3] ) > 0.0001745 )
                modelShape->SetRotation( SGVECTOR( rot[0], rot[1], rot[2] ), rot[3] );

            modelShape->SetTranslation( trans );
            modelShape->SetScale( SGPOINT( sM->m_Scale.x, sM->m_Scale.y, sM->m_Scale.z ) );

            if( NULL == S3D::GetSGNodeParent( mod3d ) )
            {
                m_components.push_back( mod3d );
                modelShape->AddChildNode( mod3d );
            }
            else
            {
                modelShape->AddRefNode( mod3d );
            }

        }

        ++sM;
    }
}


bool PCB_EDIT_FRAME::ExportVRML_File( const wxString& aFullFileName, double aMMtoWRMLunit,
                                      bool aExport3DFiles, bool aUseRelativePaths,
                                      bool aUsePlainPCB, const wxString& a3D_Subdir,
                                      double aXRef, double aYRef )
{
    BOARD*          pcb = GetBoard();
    bool            ok  = true;
    BOARD_COMMIT    commit( this );     // We may need to modify the board (for instance to
                                        // fill zones), so make sure we can revert.

    USE_INLINES = aExport3DFiles;
    USE_DEFS = true;
    USE_RELPATH = aUseRelativePaths;

    cache = Prj().Get3DCacheManager();
    PROJ_DIR = Prj().GetProjectPath();
    SUBDIR_3D = a3D_Subdir;
    MODEL_VRML model3d;
    model_vrml = &model3d;
    model3d.SetScale( aMMtoWRMLunit );

    if( USE_INLINES )
    {
        BOARD_SCALE = MM_PER_IU / 2.54;
        model3d.SetOffset( -aXRef / 2.54, aYRef / 2.54 );
    }
    else
    {
        BOARD_SCALE = MM_PER_IU;
        model3d.SetOffset( -aXRef, aYRef );
    }

    // plain PCB or else PCB with copper and silkscreen
    model3d.m_plainPCB = aUsePlainPCB;

    try
    {
        // Preliminary computation: the z value for each layer
        model3d.ComputeLayer3D_Zpos( pcb );

        // board edges and cutouts
        model3d.ExportVrmlBoard( pcb );

        // Drawing and text on the board
        if( !aUsePlainPCB )
            model3d.ExportVrmlSolderMask( pcb );

        // Drawing and text on the board
        if( !aUsePlainPCB )
            model3d.ExportVrmlDrawings( pcb );

        // Export vias and trackage
        model3d.ExportVrmlTracks( pcb );

        // Export zone fills
        if( !aUsePlainPCB )
            model3d.ExportVrmlZones( pcb, &commit );

        if( USE_INLINES )
        {
            // check if the 3D Subdir exists - create if not
            wxFileName subdir( SUBDIR_3D, "" );

            if( ! subdir.DirExists() )
            {
                if( !wxDir::Make( subdir.GetFullPath() ) )
                    throw( std::runtime_error( "Could not create 3D model subdirectory" ) );
            }

            OPEN_OSTREAM( output_file, TO_UTF8( aFullFileName ) );

            if( output_file.fail() )
            {
                std::ostringstream ostr;
                ostr << "Could not open file '" << TO_UTF8( aFullFileName ) << "'";
                throw( std::runtime_error( ostr.str().c_str() ) );
            }

            output_file.imbue( std::locale::classic() );

            // Begin with the usual VRML boilerplate
            wxString fn = aFullFileName;
            fn.Replace( "\\" , "/" );
            output_file << "#VRML V2.0 utf8\n";
            output_file << "WorldInfo {\n";
            output_file << "  title \"" << TO_UTF8( fn ) << " - Generated by Pcbnew\"\n";
            output_file << "}\n";
            output_file << "Transform {\n";
            output_file << "  scale " << std::setprecision( PRECISION );
            output_file << WORLD_SCALE << " ";
            output_file << WORLD_SCALE << " ";
            output_file << WORLD_SCALE << "\n";
            output_file << "  children [\n";

            // Export footprints
            for( FOOTPRINT* footprint : pcb->Footprints() )
                model3d.ExportVrmlFootprint( pcb, footprint, &output_file );

            // write out the board and all layers
            write_layers( model3d, pcb, TO_UTF8( aFullFileName ), &output_file );

            // Close the outer 'transform' node
            output_file << "]\n}\n";

            CLOSE_STREAM( output_file );
        }
        else
        {
            // Export footprints
            for( FOOTPRINT* footprint : pcb->Footprints() )
                model3d.ExportVrmlFootprint( pcb, footprint, NULL );

            // write out the board and all layers
            write_layers( model3d, pcb, TO_UTF8( aFullFileName ), NULL );
        }
    }
    catch( const std::exception& e )
    {
        wxString msg;
        msg << _( "IDF Export Failed:\n" ) << FROM_UTF8( e.what() );
        wxMessageBox( msg );

        ok = false;
    }

    commit.Revert();
    return ok;
}


static SGNODE* getSGColor( VRML_COLOR_INDEX colorIdx )
{
    if( colorIdx == -1 )
        colorIdx = VRML_COLOR_PCB;
    else if( colorIdx == VRML_COLOR_LAST )
        return NULL;

    if( sgmaterial[colorIdx] )
        return sgmaterial[colorIdx];

    IFSG_APPEARANCE vcolor( (SGNODE*) NULL );
    VRML_COLOR* cp = &colors[colorIdx];

    vcolor.SetSpecular( cp->spec_red, cp->spec_grn, cp->spec_blu );
    vcolor.SetDiffuse( cp->diffuse_red, cp->diffuse_grn, cp->diffuse_blu );
    vcolor.SetShininess( cp->shiny );
    // NOTE: XXX - replace with a better equation; using this definition
    // of ambient will not yield the best results
    vcolor.SetAmbient( cp->ambient, cp->ambient, cp->ambient );
    vcolor.SetTransparency( cp->transp );

    sgmaterial[colorIdx] = vcolor.GetRawPtr();

    return sgmaterial[colorIdx];
}


static void create_vrml_plane( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
    VRML_LAYER* layer, double top_z, bool aTopPlane )
{
    std::vector< double > vertices;
    std::vector< int > idxPlane;

    if( !( *layer ).Get2DTriangles( vertices, idxPlane, top_z, aTopPlane ) )
    {
        return;
    }

    if( ( idxPlane.size() % 3 ) )
    {
        throw( std::runtime_error( "[BUG] index lists are not a multiple of 3 (not a triangle list)" ) );
    }

    std::vector< SGPOINT > vlist;
    size_t nvert = vertices.size() / 3;
    size_t j = 0;

    for( size_t i = 0; i < nvert; ++i, j+= 3 )
        vlist.emplace_back( vertices[j], vertices[j+1], vertices[j+2] );

    // create the intermediate scenegraph
    IFSG_TRANSFORM tx0( PcbOutput.GetRawPtr() );    // tx0 = Transform for this outline
    IFSG_SHAPE shape( tx0 );            // shape will hold (a) all vertices and (b) a local list of normals
    IFSG_FACESET face( shape );         // this face shall represent the top and bottom planes
    IFSG_COORDS cp( face );             // coordinates for all faces
    cp.SetCoordsList( nvert, &vlist[0] );
    IFSG_COORDINDEX coordIdx( face );   // coordinate indices for top and bottom planes only
    coordIdx.SetIndices( idxPlane.size(), &idxPlane[0] );
    IFSG_NORMALS norms( face );         // normals for the top and bottom planes

    // set the normals
    if( aTopPlane )
    {
        for( size_t i = 0; i < nvert; ++i )
            norms.AddNormal( 0.0, 0.0, 1.0 );
    }
    else
    {
        for( size_t i = 0; i < nvert; ++i )
            norms.AddNormal( 0.0, 0.0, -1.0 );
    }

    // assign a color from the palette
    SGNODE* modelColor = getSGColor( colorID );

    if( NULL != modelColor )
    {
        if( NULL == S3D::GetSGNodeParent( modelColor ) )
            shape.AddChildNode( modelColor );
        else
            shape.AddRefNode( modelColor );
    }
}


static void create_vrml_shell( IFSG_TRANSFORM& PcbOutput, VRML_COLOR_INDEX colorID,
    VRML_LAYER* layer, double top_z, double bottom_z )
{
    std::vector< double > vertices;
    std::vector< int > idxPlane;
    std::vector< int > idxSide;

    if( top_z < bottom_z )
    {
        double tmp = top_z;
        top_z = bottom_z;
        bottom_z = tmp;
    }

    if( !( *layer ).Get3DTriangles( vertices, idxPlane, idxSide, top_z, bottom_z )
            || idxPlane.empty() || idxSide.empty() )
    {
        return;
    }

    if( ( idxPlane.size() % 3 ) || ( idxSide.size() % 3 ) )
    {
        throw( std::runtime_error( "[BUG] index lists are not a multiple of 3 (not a "
                                   "triangle list)" ) );
    }

    std::vector< SGPOINT > vlist;
    size_t nvert = vertices.size() / 3;
    size_t j = 0;

    for( size_t i = 0; i < nvert; ++i, j+= 3 )
        vlist.emplace_back( vertices[j], vertices[j+1], vertices[j+2] );

    // create the intermediate scenegraph
    IFSG_TRANSFORM tx0( PcbOutput.GetRawPtr() );    // tx0 = Transform for this outline
    IFSG_SHAPE shape( tx0 );            // shape will hold (a) all vertices and (b) a local list of normals
    IFSG_FACESET face( shape );         // this face shall represent the top and bottom planes
    IFSG_COORDS cp( face );             // coordinates for all faces
    cp.SetCoordsList( nvert, &vlist[0] );
    IFSG_COORDINDEX coordIdx( face );   // coordinate indices for top and bottom planes only
    coordIdx.SetIndices( idxPlane.size(), &idxPlane[0] );
    IFSG_NORMALS norms( face );         // normals for the top and bottom planes

    // number of TOP (and bottom) vertices
    j = nvert / 2;

    // set the TOP normals
    for( size_t i = 0; i < j; ++i )
        norms.AddNormal( 0.0, 0.0, 1.0 );

    // set the BOTTOM normals
    for( size_t i = 0; i < j; ++i )
        norms.AddNormal( 0.0, 0.0, -1.0 );

    // assign a color from the palette
    SGNODE* modelColor = getSGColor( colorID );

    if( NULL != modelColor )
    {
        if( NULL == S3D::GetSGNodeParent( modelColor ) )
            shape.AddChildNode( modelColor );
        else
            shape.AddRefNode( modelColor );
    }

    // create a second shape describing the vertical walls of the extrusion
    // using per-vertex-per-face-normals
    shape.NewNode( tx0 );
    shape.AddRefNode( modelColor );    // set the color to be the same as the top/bottom
    face.NewNode( shape );
    cp.NewNode( face );               // new vertex list
    norms.NewNode( face );            // new normals list
    coordIdx.NewNode( face );         // new index list

    // populate the new per-face vertex list and its indices and normals
    std::vector< int >::iterator sI = idxSide.begin();
    std::vector< int >::iterator eI = idxSide.end();

    size_t sidx = 0;    // index to the new coord set
    SGPOINT p1, p2, p3;
    SGVECTOR vnorm;

    while( sI != eI )
    {
        p1 = vlist[*sI];
        cp.AddCoord( p1 );
        ++sI;

        p2 = vlist[*sI];
        cp.AddCoord( p2 );
        ++sI;

        p3 = vlist[*sI];
        cp.AddCoord( p3 );
        ++sI;

        vnorm.SetVector( S3D::CalcTriNorm( p1, p2, p3 ) );
        norms.AddNormal( vnorm );
        norms.AddNormal( vnorm );
        norms.AddNormal( vnorm );

        coordIdx.AddIndex( (int)sidx );
        ++sidx;
        coordIdx.AddIndex( (int)sidx );
        ++sidx;
        coordIdx.AddIndex( (int)sidx );
        ++sidx;
    }
}
