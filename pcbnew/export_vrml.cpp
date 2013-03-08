/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
#include <appl_wxstruct.h>
#include <3d_struct.h>
#include <macros.h>

#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <convert_from_iu.h>

#include <vector>
#include <cmath>

// Number of segments to approximate a circle per segments:
#define SEGM_COUNT_PER_360 32
// basic angle to approximate a circle per segments
static const double INC_ANGLE = M_PI*2 / SEGM_COUNT_PER_360;

/* helper function:
 * some characters cannot be used in names,
 * this function change them to "_"
 */
static void ChangeIllegalCharacters( wxString & aFileName, bool aDirSepIsIllegal );

// I use this a lot...
static const double PI2 = M_PI / 2;

struct POINT_3D
{
    double x, y, z;
};

struct POINT_2D
{
    POINT_2D( double _x = 0, double _y = 0 ) : x( _x ), y( _y )
    { }
    double x, y;
};

// Absolutely not optimized triangle bag :D
struct TRIANGLE
{
    TRIANGLE( double x1, double y1, double z1,
              double x2, double y2, double z2,
              double x3, double y3, double z3 )
    {
        p1.x = x1; p1.y = y1; p1.z = z1;
        p2.x = x2; p2.y = y2; p2.z = z2;
        p3.x = x3; p3.y = y3; p3.z = z3;
    }
    TRIANGLE() { }
    POINT_3D p1, p2, p3;
};
typedef std::vector<TRIANGLE> TRIANGLEBAG;

// A flat triangle fan
struct FLAT_FAN
{
    POINT_2D              c;
    std::vector<POINT_2D> pts;
    void                add( double x, double y )
    {
        pts.push_back( POINT_2D( x, y ) );
    }
    void bag( int layer, bool close = true );
};

// A flat quad ring
struct FLAT_RING
{
    std::vector<POINT_2D> inner;
    std::vector<POINT_2D> outer;
    void                add_inner( double x, double y )
    {
        inner.push_back( POINT_2D( x, y ) );
    }

    void add_outer( double x, double y )
    {
        outer.push_back( POINT_2D( x, y ) );
    }

    void bag( int layer, bool close = true );
};

// A vertical quad loop
struct VLoop
{
    std::vector<POINT_2D> pts;
    double              z_top, z_bottom;
    void                add( double x, double y )
    {
        pts.push_back( POINT_2D( x, y ) );
    }

    void bag( TRIANGLEBAG& triangles, bool close = true );
};

// The bags for all the layers
static TRIANGLEBAG layer_triangles[LAYER_COUNT];
static TRIANGLEBAG via_triangles[4];
static double      layer_z[LAYER_COUNT];

static void bag_flat_triangle( int layer, //{{{
                               double x1, double y1,
                               double x2, double y2,
                               double x3, double y3 )
{
    double z = layer_z[layer];

    layer_triangles[layer].push_back( TRIANGLE( x1, y1, z, x2, y2, z, x3, y3, z ) );
}


void FLAT_FAN::bag( int layer, bool close ) //{{{
{
    unsigned i;

    for( i = 0; i < pts.size() - 1; i++ )
        bag_flat_triangle( layer, c.x, c.y, pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y );

    if( close )
        bag_flat_triangle( layer, c.x, c.y, pts[i].x, pts[i].y, pts[0].x, pts[0].y );
}


static void bag_flat_quad( int layer, //{{{
                           double x1, double y1,
                           double x2, double y2,
                           double x3, double y3,
                           double x4, double y4 )
{
    bag_flat_triangle( layer, x1, y1, x3, y3, x2, y2 );
    bag_flat_triangle( layer, x2, y2, x3, y3, x4, y4 );
}


void FLAT_RING::bag( int layer, bool close ) //{{{
{
    unsigned i;

    for( i = 0; i < inner.size() - 1; i++ )
        bag_flat_quad( layer,
                       inner[i].x, inner[i].y,
                       outer[i].x, outer[i].y,
                       inner[i + 1].x, inner[i + 1].y,
                       outer[i + 1].x, outer[i + 1].y );

    if( close )
        bag_flat_quad( layer,
                       inner[i].x, inner[i].y,
                       outer[i].x, outer[i].y,
                       inner[0].x, inner[0].y,
                       outer[0].x, outer[0].y );
}


static void bag_vquad( TRIANGLEBAG& triangles, //{{{
                       double x1, double y1, double x2, double y2,
                       double z1, double z2 )
{
    triangles.push_back( TRIANGLE( x1, y1, z1,
                                   x2, y2, z1,
                                   x2, y2, z2 ) );
    triangles.push_back( TRIANGLE( x1, y1, z1,
                                   x2, y2, z2,
                                   x1, y1, z2 ) );
}


void VLoop::bag( TRIANGLEBAG& triangles, bool close ) //{{{
{
    unsigned i;

    for( i = 0; i < pts.size() - 1; i++ )
        bag_vquad( triangles, pts[i].x, pts[i].y,
                   pts[i + 1].x, pts[i + 1].y,
                   z_top, z_bottom );

    if( close )
        bag_vquad( triangles, pts[i].x, pts[i].y,
                   pts[0].x, pts[0].y,
                   z_top, z_bottom );
}


static void write_triangle_bag( FILE* output_file, int color_index, //{{{
                                const TRIANGLEBAG& triangles,
                                double boardIU2WRML )
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
        0,                                          // Material marker
        "              ambientIntensity 0.8\n",
        "              transparency 0.2\n",
        "              shininess 0.2\n",
        "            }\n",
        "          }\n",
        "          geometry IndexedFaceSet {\n",
        "            solid TRUE\n",
        "            coord Coordinate {\n",
        "              point [\n",
        0,                                          // Coordinates marker
        "              ]\n",
        "            }\n",
        "            coordIndex [\n",
        0,                                          // Index marker
        "            ]\n",
        "          }\n",
        "        }\n",
        "      ]\n",
        "    }\n",
        "  ]\n",
        "}\n",
        0 // End marker
    };
    int marker_found = 0, lineno = 0;

    while( marker_found < 4 )
    {
        if( shape_boiler[lineno] )
            fputs( shape_boiler[lineno], output_file );
        else
        {
            marker_found++;
            switch( marker_found )
            {
            case 1: // Material marker
                fprintf( output_file,
                         "              diffuseColor %g %g %g\n",
                         (double) ColorRefs[color_index].m_Red / 255.0,
                         (double) ColorRefs[color_index].m_Green / 255.0,
                         (double) ColorRefs[color_index].m_Blue / 255.0 );
                fprintf( output_file,
                         "              specularColor %g %g %g\n",
                         (double) ColorRefs[color_index].m_Red / 255.0,
                         (double) ColorRefs[color_index].m_Green / 255.0,
                         (double) ColorRefs[color_index].m_Blue / 255.0 );
                fprintf( output_file,
                         "              emissiveColor %g %g %g\n",
                         (double) ColorRefs[color_index].m_Red / 255.0,
                         (double) ColorRefs[color_index].m_Green / 255.0,
                         (double) ColorRefs[color_index].m_Blue / 255.0 );
                break;

            case 2:
            {
                // Coordinates marker
                for( TRIANGLEBAG::const_iterator i = triangles.begin();
                     i != triangles.end();
                     i++ )
                {
                    fprintf( output_file, "%.8g %.8g %.8g\n",
                             i->p1.x * boardIU2WRML, -i->p1.y * boardIU2WRML,
                             i->p1.z * boardIU2WRML );
                    fprintf( output_file, "%.8g %.8g %.8g\n",
                             i->p2.x * boardIU2WRML, -i->p2.y * boardIU2WRML,
                             i->p2.z * boardIU2WRML );
                    fprintf( output_file, "%.8g %.8g %.8g\n",
                             i->p3.x * boardIU2WRML, -i->p3.y * boardIU2WRML,
                             i->p3.z * boardIU2WRML );
                }
            }
            break;

            case 3:
            {
                // Index marker
                // OK, that's sick ...
                int j = 0;
                for( TRIANGLEBAG::const_iterator i = triangles.begin();
                     i != triangles.end();
                     i++ )
                {
                    fprintf( output_file, "%d %d %d -1\n", j, j + 1, j + 2 );
                    j += 3;
                }
            }
            break;

            default:
                break;
            }
        }
        lineno++;
    }
}


static void compute_layer_Zs( BOARD* pcb ) //{{{
{
    int    copper_layers = pcb->GetCopperLayerCount( );

    // We call it 'layer' thickness, but it's the whole board thickness!
    double board_thickness = pcb->GetDesignSettings().GetBoardThickness();
    double half_thickness  = board_thickness / 2;

    // Compute each layer's Z value, more or less like the 3d view
    for( int i = 0; i <= LAYER_N_FRONT; i++ )
    {
        if( i < copper_layers )
            layer_z[i] = board_thickness * i / (copper_layers - 1) - half_thickness;
        else
            layer_z[i] = half_thickness; // The component layer...
    }

    /* To avoid rounding interference, we apply an epsilon to each
     * successive layer */
    const double epsilon_z = 0.02 * IU_PER_MM; // That's 1/50 mm
    layer_z[SOLDERPASTE_N_BACK]  = -half_thickness - epsilon_z * 4;
    layer_z[ADHESIVE_N_BACK]     = -half_thickness - epsilon_z * 3;
    layer_z[SILKSCREEN_N_BACK]   = -half_thickness - epsilon_z * 2;
    layer_z[SOLDERMASK_N_BACK]   = -half_thickness - epsilon_z;
    layer_z[SOLDERMASK_N_FRONT]  = half_thickness + epsilon_z;
    layer_z[SILKSCREEN_N_FRONT]  = half_thickness + epsilon_z * 2;
    layer_z[ADHESIVE_N_FRONT]    = half_thickness + epsilon_z * 3;
    layer_z[SOLDERPASTE_N_FRONT] = half_thickness + epsilon_z * 4;
    layer_z[DRAW_N]    = half_thickness + epsilon_z * 5;
    layer_z[COMMENT_N] = half_thickness + epsilon_z * 6;
    layer_z[ECO1_N]    = half_thickness + epsilon_z * 7;
    layer_z[ECO2_N]    = half_thickness + epsilon_z * 8;
    layer_z[EDGE_N]    = 0;
}


static void export_vrml_line( int layer, double startx, double starty, //{{{
                              double endx, double endy, double width, int divisions )
{
    double  r     = width / 2;
    double  angle = atan2( endy - starty, endx - startx );
    double  alpha;
    FLAT_FAN fan;

    // Output the 'bone' as a triangle fan, this is the fan centre
    fan.c.x = (startx + endx) / 2;
    fan.c.y = (starty + endy) / 2;

    // The 'end' side cap
    for( alpha = angle - PI2; alpha < angle + PI2; alpha += PI2 / divisions )
        fan.add( endx + r * cos( alpha ), endy + r * sin( alpha ) );

    alpha = angle + PI2;
    fan.add( endx + r * cos( alpha ), endy + r * sin( alpha ) );
    // The 'start' side cap
    for( alpha = angle + PI2; alpha < angle + 3 * PI2; alpha += PI2 / divisions )
        fan.add( startx + r * cos( alpha ), starty + r * sin( alpha ) );

    alpha = angle + 3 * PI2;
    fan.add( startx + r * cos( alpha ), starty + r * sin( alpha ) );
    // Export the fan
    fan.bag( layer );
}


static void export_vrml_circle( int layer, double startx, double starty,
                                double endx, double endy, double width )
{
    double   hole, radius;
    FLAT_RING ring;

    radius = hypot( startx - endx, starty - endy ) + ( width / 2);
    hole  = radius - width;

    for( double alpha = 0; alpha < M_PI * 2; alpha += INC_ANGLE )
    {
        ring.add_inner( startx + hole * cos( alpha ), starty + hole * sin( alpha ) );
        ring.add_outer( startx + radius * cos( alpha ), starty + radius * sin( alpha ) );
    }

    ring.bag( layer );
}


static void export_vrml_slot( TRIANGLEBAG& triangles, //{{{
                              int top_layer, int bottom_layer, double xc, double yc,
                              double dx, double dy, int orient )
{
    double capx, capy; // Cap center
    VLoop  loop;
    int divisions = SEGM_COUNT_PER_360 / 2;

    loop.z_top    = layer_z[top_layer];
    loop.z_bottom = layer_z[bottom_layer];
    double angle = orient / 1800.0 * M_PI;

    if( dy > dx )
    {
        EXCHG( dx, dy );
        angle += PI2;
    }

    // The exchange above means that cutter radius is alvays dy/2
    double r = dy / 2;
    double alpha;
    // The first side cap
    capx = xc + cos( angle ) * dx / 2;
    capy = yc + sin( angle ) * dx / 2;

    for( alpha = angle - PI2; alpha < angle + PI2; alpha += PI2 / divisions )
        loop.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );

    alpha = angle + PI2;
    loop.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );

    // The other side cap
    capx = xc - cos( angle ) * dx / 2;
    capy = yc - sin( angle ) * dx / 2;

    for( alpha = angle + PI2; alpha < angle + 3 * PI2; alpha += PI2 / divisions )
        loop.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );

    alpha = angle + 3 * PI2;
    loop.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );
    loop.bag( triangles );
}


static void export_vrml_hole( TRIANGLEBAG& triangles,
                              int top_layer, int bottom_layer,
                              double xc, double yc, double hole )
{
    VLoop loop;

    loop.z_top    = layer_z[top_layer];
    loop.z_bottom = layer_z[bottom_layer];

    for( double alpha = 0; alpha < M_PI * 2; alpha += INC_ANGLE )
        loop.add( xc + cos( alpha ) * hole, yc + sin( alpha ) * hole );

    loop.bag( triangles );
}


static void export_vrml_oval_pad( int layer, double xc, double yc,
                                  double dx, double dy, int orient )
{
    double  capx, capy; // Cap center
    FLAT_FAN fan;

    fan.c.x = xc;
    fan.c.y = yc;
    double angle = orient / 1800.0 * M_PI;
    int divisions = SEGM_COUNT_PER_360 / 2;

    if( dy > dx )
    {
        EXCHG( dx, dy );
        angle += PI2;
    }

    // The exchange above means that cutter radius is alvays dy/2
    double r = dy / 2;
    double alpha;

    // The first side cap
    capx = xc + cos( angle ) * dx / 2;
    capy = yc + sin( angle ) * dx / 2;

    for( alpha = angle - PI2; alpha < angle + PI2; alpha += PI2 / divisions )
        fan.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );

    alpha = angle + PI2;
    fan.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );
    // The other side cap
    capx = xc - cos( angle ) * dx / 2;
    capy = yc - sin( angle ) * dx / 2;

    for( alpha = angle + PI2; alpha < angle + 3 * PI2; alpha += PI2 / divisions )
        fan.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );

    alpha = angle + 3 * PI2;
    fan.add( capx + r * cos( alpha ), capy + r * sin( alpha ) );
    fan.bag( layer );
}


static void export_vrml_arc( int layer, double centerx, double centery,
                             double arc_startx, double arc_starty,
                             double width, double arc_angle )
{
    FLAT_RING ring;
    double   start_angle = atan2( arc_starty - centery, arc_startx - centerx );

    int count = KiROUND( arc_angle / 360.0 * SEGM_COUNT_PER_360 );
    if( count < 0 )
        count = -count;
    if( count == 0 )
        count = 1;
    double divisions = arc_angle*M_PI/180.0 / count;

    double outer_radius = hypot( arc_starty - centery, arc_startx - centerx )
                          + ( width / 2);
    double inner_radius  = outer_radius - width;

    double alpha = 0;
    for( int ii = 0; ii <= count; alpha += divisions, ii++ )
    {
        double angle_rot = start_angle + alpha;
        ring.add_inner( centerx + cos( angle_rot ) * inner_radius,
                        centery + sin( angle_rot ) * inner_radius );
        ring.add_outer( centerx + cos( angle_rot ) * outer_radius,
                        centery + sin( angle_rot ) * outer_radius );
    }

    ring.bag( layer, false );
}

static void export_vrml_varc( TRIANGLEBAG& triangles,
                              int top_layer, int bottom_layer,
                              double centerx, double centery,
                              double arc_startx, double arc_starty,
                              double arc_angle )
{
    VLoop loop;

    loop.z_top    = layer_z[top_layer];
    loop.z_bottom = layer_z[bottom_layer];

    double start_angle = atan2( arc_starty - centery, arc_startx - centerx );
    double radius = hypot( arc_starty - centery, arc_startx - centerx );

    int count = KiROUND( arc_angle / 360.0 * SEGM_COUNT_PER_360 );
    if( count < 0 )
        count = -count;
    if( count == 0 )
        count = 1;
    double divisions = arc_angle*M_PI/180.0 / count;

    double alpha = 0;
    for( int ii = 0; ii <= count; alpha += divisions, ii++ )
    {
        double angle_rot = start_angle + alpha;
        loop.add( centerx + cos( angle_rot ) * radius, centery + sin( angle_rot ) * radius );
    }

    loop.bag( triangles );
}


static void export_vrml_drawsegment( DRAWSEGMENT* drawseg ) //{{{
{
    int    layer = drawseg->GetLayer();
    double w     = drawseg->GetWidth();
    double x     = drawseg->GetStart().x;
    double y     = drawseg->GetStart().y;
    double xf    = drawseg->GetEnd().x;
    double yf    = drawseg->GetEnd().y;

    // Items on the edge layer are high, not thick
    if( layer == EDGE_N )
    {
        switch( drawseg->GetShape() )
        {
        // There is a special 'varc' primitive for this
        case S_ARC:
            export_vrml_varc( layer_triangles[layer],
                              FIRST_COPPER_LAYER, LAST_COPPER_LAYER,
                              x, y, xf, yf, drawseg->GetAngle()/10 );
            break;

        // Circles on edge are usually important holes
        case S_CIRCLE:
            export_vrml_hole( layer_triangles[layer],
                              FIRST_COPPER_LAYER, LAST_COPPER_LAYER, x, y,
                              hypot( xf - x, yf - y ) / 2 );
            break;

        default:
        {
            // Simply a quad
            double z_top    = layer_z[FIRST_COPPER_LAYER];
            double z_bottom = layer_z[LAST_COPPER_LAYER];
            bag_vquad( layer_triangles[layer], x, y, xf, yf, z_top, z_bottom );
            break;
        }
        }
    }
    else
    {
        switch( drawseg->GetShape() )
        {
        case S_ARC:
            export_vrml_arc( layer,
                             (double) drawseg->GetCenter().x,
                             (double) drawseg->GetCenter().y,
                             (double) drawseg->GetArcStart().x,
                             (double) drawseg->GetArcStart().y,
                             w, drawseg->GetAngle()/10 );
            break;

        case S_CIRCLE:
            export_vrml_circle( layer, x, y, xf, yf, w );
            break;

        default:
            export_vrml_line( layer, x, y, xf, yf, w, 1 );
            break;
        }
    }
}


/* C++ doesn't have closures and neither continuation forms... this is
 * for coupling the vrml_text_callback with the common parameters */

static int s_text_layer;
static int s_text_width;
static void vrml_text_callback( int x0, int y0, int xf, int yf )
{
    export_vrml_line( s_text_layer, x0, y0, xf, yf, s_text_width, 1 );
}


static void export_vrml_pcbtext( TEXTE_PCB* text )
{
    // Coupling by globals! Ewwww...
    s_text_layer = text->GetLayer();
    s_text_width = text->m_Thickness;

    wxSize size = text->m_Size;
    if( text->m_Mirror )
        NEGATE( size.x );

    if( text->m_MultilineAllowed )
    {
        wxPoint        pos  = text->m_Pos;
        wxArrayString* list = wxStringSplit( text->m_Text, '\n' );
        wxPoint        offset;

        offset.y = text->GetInterline();

        RotatePoint( &offset, text->GetOrientation() );
        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            DrawGraphicText( NULL, NULL, pos, BLACK,
                             txt, text->GetOrientation(), size,
                             text->m_HJustify, text->m_VJustify,
                             text->m_Thickness, text->m_Italic,
                             true,
                             vrml_text_callback );
            pos += offset;
        }

        delete (list);
    }
    else
    {
        DrawGraphicText( NULL, NULL, text->m_Pos, BLACK,
                         text->m_Text, text->GetOrientation(), size,
                         text->m_HJustify, text->m_VJustify,
                         text->m_Thickness, text->m_Italic,
                         true,
                         vrml_text_callback );
    }
}


static void export_vrml_drawings( BOARD* pcb ) //{{{
{
    // draw graphic items
    for( EDA_ITEM* drawing = pcb->m_Drawings;  drawing != 0;  drawing = drawing->Next() )
    {
        switch( drawing->Type() )
        {
        case PCB_LINE_T:
            export_vrml_drawsegment( (DRAWSEGMENT*) drawing );
            break;

        case PCB_TEXT_T:
            export_vrml_pcbtext( (TEXTE_PCB*) drawing );
            break;

        default:
            break;
        }
    }
}


static void export_round_padstack( BOARD* pcb, double x, double y,
                                   double r,
                                   int bottom_layer, int top_layer )
{
    int copper_layers = pcb->GetCopperLayerCount( );

    for( int layer = bottom_layer; layer < copper_layers; layer++ )
    {
        // The last layer is always the component one, unless it's single face
        if( (layer > FIRST_COPPER_LAYER) && (layer == copper_layers - 1) )
            layer = LAST_COPPER_LAYER;

        if( layer <= top_layer )
            export_vrml_circle( layer, x, y, x + r / 2, y, r );
    }
}


static void export_vrml_via( BOARD* pcb, SEGVIA* via ) //{{{
{
    double x, y, r, hole;
    int    top_layer, bottom_layer;

    r    = via->GetWidth() / 2;
    hole = via->GetDrillValue() / 2;
    x    = via->GetStart().x;
    y    = via->GetStart().y;
    via->ReturnLayerPair( &top_layer, &bottom_layer );

    // Export the via padstack
    export_round_padstack( pcb, x, y, r, bottom_layer, top_layer );

    // Drill a hole
    export_vrml_hole( via_triangles[via->GetShape()], top_layer, bottom_layer, x, y, hole );
}


static void export_vrml_tracks( BOARD* pcb ) //{{{
{
    for( TRACK* track = pcb->m_Track; track != NULL; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            export_vrml_via( pcb, (SEGVIA*) track );
        else
            export_vrml_line( track->GetLayer(), track->GetStart().x, track->GetStart().y,
                              track->GetEnd().x, track->GetEnd().y, track->GetWidth(), 4 );
    }
}


/* not used? @todo complete
static void export_vrml_zones( BOARD* pcb )
{
    // Export fill segments
    for( SEGZONE* segzone = pcb->m_Zone;
        segzone != 0;
        segzone = segzone->Next() )
    {
        // Fill tracks are exported with low subdivisions
        if( segzone->Type() == PCB_ZONE_T )
            export_vrml_line( segzone->GetLayer(), segzone->m_Start.x, segzone->m_Start.y,
                              segzone->m_End.x, segzone->m_End.y, segzone->m_Width, 1 );
    }

    // Export zone outlines
    for( int i = 0; i < pcb->GetAreaCount(); i++ )
    {
        ZONE_CONTAINER* zone = pcb->GetArea( i );

        if( ( zone->m_FilledPolysList.size() == 0 )
           ||( zone->m_ZoneMinThickness <= 1 ) )
            continue;

        int width = zone->m_ZoneMinThickness;
        if( width > 0 )
        {
            int      imax  = zone->m_FilledPolysList.size() - 1;
            int      layer = zone->GetLayer();
            CPolyPt* firstcorner = &zone->m_FilledPolysList[0];
            CPolyPt* begincorner = firstcorner;

            // I'm not really positive about what he's doing here...
            for( int ic = 1; ic <= imax; ic++ )
            {
                CPolyPt* endcorner = &zone->m_FilledPolysList[ic];
                if( begincorner->utility == 0 ) // Draw only basic outlines, not extra segments
                    export_vrml_line( layer, begincorner->x, begincorner->y,
                                      endcorner->x, endcorner->y, width, 1 );
                if( (endcorner->end_contour) || (ic == imax) )  // the last corner of a filled area is found: draw it
                {
                    if( endcorner->utility == 0 )               // Draw only basic outlines, not extra segments
                        export_vrml_line( layer, endcorner->x, endcorner->y,
                                          firstcorner->x, firstcorner->y, width, 1 );
                    ic++;

                    // A new contour?
                    if( ic < imax - 1 )
                        begincorner = firstcorner = &zone->m_FilledPolysList[ic];
                }
                else
                    begincorner = endcorner;
            }
        }
    }
}
*/

static void export_vrml_text_module( TEXTE_MODULE* module ) //{{{
{
    if( module->IsVisible() )
    {
        wxSize size = module->m_Size;

        if( module->m_Mirror )
            NEGATE( size.x ); // Text is mirrored

        s_text_layer = module->GetLayer();
        s_text_width = module->m_Thickness;
        DrawGraphicText( NULL, NULL, module->m_Pos, BLACK,
                         module->m_Text, module->GetDrawRotation(), size,
                         module->m_HJustify, module->m_VJustify,
                         module->m_Thickness, module->m_Italic,
                         true,
                         vrml_text_callback );
    }
}


static void export_vrml_edge_module( EDGE_MODULE* aOutline ) //{{{
{
    int    layer = aOutline->GetLayer();
    double x     = aOutline->GetStart().x;
    double y     = aOutline->GetStart().y;
    double xf    = aOutline->GetEnd().x;
    double yf    = aOutline->GetEnd().y;
    double w     = aOutline->GetWidth();

    switch( aOutline->GetShape() )
    {
    case S_ARC:
        export_vrml_arc( layer, x, y, xf, yf, w, aOutline->GetAngle()/10 );
        break;

    case S_CIRCLE:
        export_vrml_circle( layer, x, y, xf, yf, w );
        break;

    default:
        export_vrml_line( layer, x, y, xf, yf, w, 1 );
        break;
    }
}


static void export_vrml_pad( BOARD* pcb, D_PAD* aPad ) //{{{
{
    double hole_drill_w = (double) aPad->GetDrillSize().x / 2;
    double hole_drill_h = (double) aPad->GetDrillSize().y / 2;
    double hole_drill   = std::min( hole_drill_w, hole_drill_h );
    double hole_x = aPad->GetPosition().x;
    double hole_y = aPad->GetPosition().y;

    // Export the hole on the edge layer
    if( hole_drill > 0 )
    {
        if( aPad->GetDrillShape() == PAD_OVAL )
        {
            // Oblong hole (slot)
            export_vrml_slot( layer_triangles[EDGE_N],
                              FIRST_COPPER_LAYER, LAST_COPPER_LAYER,
                              hole_x, hole_y, hole_drill_w, hole_drill_h, aPad->GetOrientation() );
        }
        else
        {
            // Drill a round hole
            export_vrml_hole( layer_triangles[EDGE_N],
                              FIRST_COPPER_LAYER, LAST_COPPER_LAYER,
                              hole_x, hole_y, hole_drill );
        }
    }

    // The pad proper, on the selected layers
    int         layer_mask    = aPad->GetLayerMask();
    int         copper_layers = pcb->GetCopperLayerCount( );

    // The (maybe offseted) pad position
    wxPoint     pad_pos   = aPad->ReturnShapePos();
    double      pad_x     = pad_pos.x;
    double      pad_y     = pad_pos.y;
    wxSize      pad_delta = aPad->GetDelta();

    double      pad_dx    = pad_delta.x / 2;
    double      pad_dy    = pad_delta.y / 2;

    double      pad_w     = aPad->GetSize().x / 2;
    double      pad_h     = aPad->GetSize().y / 2;

    for( int layer = FIRST_COPPER_LAYER; layer < copper_layers; layer++ )
    {
        // The last layer is always the component one, unless it's single face
        if( (layer > FIRST_COPPER_LAYER) && (layer == copper_layers - 1) )
            layer = LAST_COPPER_LAYER;

        if( layer_mask & (1 << layer) )
        {
            // OK, the pad is on this layer, export it
            switch( aPad->GetShape() )
            {
            case PAD_CIRCLE:
                export_vrml_circle( layer, pad_x, pad_y,
                                    pad_x + pad_w / 2, pad_y, pad_w );
                break;

            case PAD_OVAL:
                export_vrml_oval_pad( layer, pad_x, pad_y,
                                      pad_w * 2, pad_h * 2, aPad->GetOrientation() );
                break;

            case PAD_RECT:
                // Just to be sure :D
                pad_dx = 0;
                pad_dy = 0;

            case PAD_TRAPEZOID:
                {
                    int coord[8] =
                    {
                        KiROUND(-pad_w - pad_dy), KiROUND(+pad_h + pad_dx),
                        KiROUND(-pad_w + pad_dy), KiROUND(-pad_h - pad_dx),
                        KiROUND(+pad_w - pad_dy), KiROUND(+pad_h - pad_dx),
                        KiROUND(+pad_w + pad_dy), KiROUND(-pad_h + pad_dx),
                    };

                    for( int i = 0; i < 4; i++ )
                    {
                        RotatePoint( &coord[i * 2], &coord[i * 2 + 1], aPad->GetOrientation() );
                        coord[i * 2]     += KiROUND( pad_x );
                        coord[i * 2 + 1] += KiROUND( pad_y );
                    }

                    bag_flat_quad( layer, coord[0], coord[1],
                                   coord[2], coord[3],
                                   coord[4], coord[5],
                                   coord[6], coord[7] );
                }
                break;

            default:
                ;
            }
        }
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

    tmp[0] = q2[3] *q1[0] + q2[0] *q1[3] + q2[1] *q1[2] - q2[2] *q1[1];
    tmp[1] = q2[3] *q1[1] + q2[1] *q1[3] + q2[2] *q1[0] - q2[0] *q1[2];
    tmp[2] = q2[3] *q1[2] + q2[2] *q1[3] + q2[0] *q1[1] - q2[1] *q1[0];
    tmp[3] = q2[3] *q1[3] - q2[0] *q1[0] - q2[1] *q1[1] - q2[2] *q1[2];
    qr[0]  = tmp[0]; qr[1] = tmp[1];
    qr[2]  = tmp[2]; qr[3] = tmp[3];
}


static void export_vrml_module( BOARD* aPcb, MODULE* aModule,
                                FILE* aOutputFile,
                                double aVRMLModelsToBiu,
                                bool aExport3DFiles, const wxString & a3D_Subdir,
                                double boardIU2WRML )
{
    // Reference and value
    export_vrml_text_module( aModule->m_Reference );
    export_vrml_text_module( aModule->m_Value );

    // Export module edges
    for( EDA_ITEM* item = aModule->m_Drawings;  item != NULL;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            export_vrml_text_module( dynamic_cast<TEXTE_MODULE*>(item) );
            break;

        case PCB_MODULE_EDGE_T:
            export_vrml_edge_module( dynamic_cast<EDGE_MODULE*>(item) );
            break;

        default:
            break;
        }
    }

    // Export pads
    for( D_PAD* pad = aModule->m_Pads;  pad; pad = pad->Next() )
        export_vrml_pad( aPcb, pad );

    bool isFlipped = aModule->GetLayer() == LAYER_N_BACK;

    // Export the object VRML model(s)
    for( S3D_MASTER* vrmlm = aModule->m_3D_Drawings; vrmlm != 0; vrmlm = vrmlm->Next() )
    {
        wxString fname = vrmlm->m_Shape3DName;

        if( fname.IsEmpty() )
            continue;

        if( ! wxFileName::FileExists( fname ) )
        {
            wxFileName fn = fname;
            fname = wxGetApp().FindLibraryPath( fn );

            if( fname.IsEmpty() )   // keep "short" name if full filemane not found
                fname = vrmlm->m_Shape3DName;
        }

        fname.Replace(wxT("\\"), wxT("/" ) );
        wxString source_fname = fname;

        if( aExport3DFiles )    // Change illegal characters in short filename
        {
            ChangeIllegalCharacters( fname, true );
            fname = a3D_Subdir + wxT("/") + fname;

            if( !wxFileExists( fname ) )
                wxCopyFile( source_fname, fname );
        }

        /* Calculate 3D shape rotation:
         * this is the rotation parameters, with an additional 180 deg rotation
         * for footprints that are flipped
         * When flipped, axis rotation is the horizontal axis (X axis)
         */
        double rotx = - vrmlm->m_MatRotation.x;
        double roty = - vrmlm->m_MatRotation.y;
        double rotz = - vrmlm->m_MatRotation.z;

        if ( isFlipped )
        {
            rotx += 180.0;
            NEGATE(roty);
            NEGATE(rotz);
        }

        // Do some quaternion munching
        double q1[4], q2[4], rot[4];
        build_quat( 1, 0, 0, rotx / 180.0 * M_PI, q1 );
        build_quat( 0, 1, 0, roty / 180.0 * M_PI, q2 );
        compose_quat( q1, q2, q1 );
        build_quat( 0, 0, 1, rotz / 180.0 * M_PI, q2 );
        compose_quat( q1, q2, q1 );
        // Note here aModule->GetOrientation() is in 0.1 degrees,
        // so module rotation is aModule->GetOrientation() / 1800.0
        build_quat( 0, 0, 1, aModule->GetOrientation() / 1800.0 * M_PI, q2 );
        compose_quat( q1, q2, q1 );
        from_quat( q1, rot );

        fprintf( aOutputFile, "Transform {\n" );
        // A null rotation would fail the acos!
        if( rot[3] != 0.0 )
        {
            fprintf( aOutputFile, "  rotation %g %g %g %g\n", rot[0], rot[1], rot[2], rot[3] );
        }

        // adjust 3D shape local offset position
        // they are given in inch, so they are converted in board IU.
        double offsetx = vrmlm->m_MatPosition.x * IU_PER_MILS * 1000.0;
        double offsety = vrmlm->m_MatPosition.y * IU_PER_MILS * 1000.0;
        double offsetz = vrmlm->m_MatPosition.z * IU_PER_MILS * 1000.0;

        if ( isFlipped )
            NEGATE(offsetz);
        else    // In normal mode, Y axis is reversed in Pcbnew.
            NEGATE(offsety);

        RotatePoint(&offsetx, &offsety, aModule->GetOrientation());

        fprintf( aOutputFile, "  translation %g %g %g\n",
                 (offsetx + aModule->m_Pos.x) * boardIU2WRML,
                 - (offsety + aModule->m_Pos.y) * boardIU2WRML,    // Y axis is reversed in Pcbnew
                 (offsetz + layer_z[aModule->GetLayer()]) * boardIU2WRML);

        fprintf( aOutputFile, "  scale %g %g %g\n",
                 vrmlm->m_MatScale.x * aVRMLModelsToBiu,
                 vrmlm->m_MatScale.y * aVRMLModelsToBiu,
                 vrmlm->m_MatScale.z * aVRMLModelsToBiu );

        fprintf( aOutputFile,
//                 "  children [\n    Inline {\n      url \"file://%s\"\n    } ]\n",
                 "  children [\n    Inline {\n      url \"%s\"\n    } ]\n",
                 TO_UTF8( fname ) );
        fprintf( aOutputFile, "  }\n" );
    }
}


static void write_and_empty_triangle_bag( FILE* output_file, TRIANGLEBAG& triangles,
                                          int color, double boardIU2WRML )
{
    if( !triangles.empty() )
    {
        write_triangle_bag( output_file, color, triangles, boardIU2WRML );
        triangles.clear( );
    }
}

/* ExportVRML_File
 * Creates the file(s) exporting current BOARD to a VRML file.
 * aFullFileName = the full filename of the file to create
 * aMMtoWRMLunit = the general WRML scaling factor. 1.0 to export in mm
 * @param aExport3DFiles = true to copy 3D shapes in the subdir a3D_Subdir
 * a3D_Subdir = sub directory where 3D shapes files are copied
 * used only when aExport3DFiles == true
 */
/* Note1:
 * When copying 3D shapes files, the new filename is build from
 * the full path name, changing the separators by underscore.
 * this is needed because files with the same shortname can exist in different directories
 * Note 2:
 * ExportVRML_File generates coordinates in board units (BIU) inside the file.
 * (TODO: use mm inside the file)
 * A general scale transform is applied to the whole file
 * (1.0 to have the actual WRML unit im mm, 0.001 to have the actual WRML unit im meter
 * Note 3:
 * For 3D models built by a 3D modeler, the unit is 0,1 inch
 * A specfic scale is applied to 3D models to convert them to BIU
 *
 */
bool PCB_EDIT_FRAME::ExportVRML_File( const wxString & aFullFileName,
                                      double aMMtoWRMLunit, bool aExport3DFiles,
                                      const wxString & a3D_Subdir )
{
    wxString   msg;
    FILE*      output_file;
    BOARD*     pcb = GetBoard();

    output_file = wxFopen( aFullFileName, wxT( "wt" ) );
    if( output_file == NULL )
        return false;

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard();

    // Begin with the usual VRML boilerplate
    wxString name = aFullFileName;

    name.Replace(wxT("\\"), wxT("/" ) );
    ChangeIllegalCharacters( name, false );
    fprintf( output_file, "#VRML V2.0 utf8\n"
                          "WorldInfo {\n"
                          "  title \"%s - Generated by Pcbnew\"\n"
                          "}\n", TO_UTF8( name ) );

    /* The would be in BIU and not in meters, as the standard wants.
     * It is trivial to embed everything in a transform node to
     * fix it. For example here we build the world in inches...
    */

    // Global VRML scale to export to a different scale.
    // (aMMtoWRMLScale = 1.0 to export in mm)
    double boardIU2WRML = aMMtoWRMLunit / MM_PER_IU;
    fprintf( output_file, "Transform {\n" );

    /* Define the translation to have the board centre to the 2D axis origin
     * more easy for rotations...
     */
    EDA_RECT bbbox = pcb->ComputeBoundingBox();

    double dx = boardIU2WRML * bbbox.Centre().x;
    double dy = boardIU2WRML * bbbox.Centre().y;

    fprintf( output_file, "  translation %g %g 0.0\n", -dx, dy );
    fprintf( output_file, "  children [\n" );

    // Preliminary computation: the z value for each layer
    compute_layer_Zs( pcb );

    // Drawing and text on the board, and edges which are special
    export_vrml_drawings( pcb );

    // Export vias and trackage
    export_vrml_tracks( pcb );

    // Export zone fills
/* TODO    export_vrml_zones(pcb);
*/

    /* scaling factor to convert 3D models to board units (decimils)
     * Usually we use Wings3D to create thems.
     * One can consider the 3D units is 0.1 inch (2.54 mm)
     * So the scaling factor from 0.1 inch to board units
     * is 2.54 * aMMtoWRMLunit
     */
    double wrml_3D_models_scaling_factor = 2.54 * aMMtoWRMLunit;
    // Export footprints
    for( MODULE* module = pcb->m_Modules; module != 0; module = module->Next() )
        export_vrml_module( pcb, module, output_file,
                            wrml_3D_models_scaling_factor,
                            aExport3DFiles, a3D_Subdir,
                            boardIU2WRML );

    /* Output the bagged triangles for each layer
     * Each layer will be a separate shape */
    for( int layer = 0; layer < LAYER_COUNT; layer++ )
        write_and_empty_triangle_bag( output_file,
                                      layer_triangles[layer],
                                      pcb->GetLayerColor(layer),
                                      boardIU2WRML );

    // Same thing for the via layers
    for( int i = 0; i < 4; i++ )
        write_and_empty_triangle_bag( output_file,
                                      via_triangles[i],
                                      pcb->GetVisibleElementColor( VIAS_VISIBLE + i ),
                                      boardIU2WRML );

    // Close the outer 'transform' node
    fputs( "]\n}\n", output_file );

    // End of work
    fclose( output_file );
    SetLocaleTo_Default();       // revert to the current  locale

    return true;
}

/*
 * some characters cannot be used in filenames,
 * this function change them to "_"
 */
static void ChangeIllegalCharacters( wxString & aFileName, bool aDirSepIsIllegal )
{
    if( aDirSepIsIllegal )
        aFileName.Replace(wxT("/"), wxT("_" ) );

    aFileName.Replace(wxT(" "), wxT("_" ) );
    aFileName.Replace(wxT(":"), wxT("_" ) );
}
