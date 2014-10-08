/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_draw_helper_functions.cpp
 */

#include <fctsys.h>

#include <PolyLine.h>
#include <colors.h>
#include <colors_selection.h>
#include <class_pad.h>
#include <class_track.h>

#include <convert_basic_shapes_to_polygon.h>

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <3d_draw_basic_functions.h>

#define TEXTURE_PCB_SCALE 5.0

// -----------------
// helper function (from wxWidgets, opengl/cube.cpp sample
// -----------------
void CheckGLError(const char *aFileName, int aLineNumber)
{
    GLenum errLast = GL_NO_ERROR;

    for ( ; ; )
    {
        GLenum err = glGetError();
        if ( err == GL_NO_ERROR )
            return;

        // normally the error is reset by the call to glGetError() but if
        // glGetError() itself returns an error, we risk looping forever here
        // so check that we get a different error than the last time
        if ( err == errLast )
        {
            wxLogError(wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;

        wxLogError( wxT( "OpenGL error %d At: %s, line: %d" ), err,
                    GetChars( FROM_UTF8( aFileName ) ), aLineNumber );
    }
}


INFO3D_VISU& EDA_3D_CANVAS::GetPrm3DVisu() const
{
    return Parent()->GetPrm3DVisu();
}

wxSize EDA_3D_CANVAS::getBoardSize() const
{
    // return the size of the board in pcb units
    return GetPrm3DVisu().m_BoardSize;
}


wxPoint EDA_3D_CANVAS::getBoardCenter() const
{
    // return the position of the board center in pcb units
    return GetPrm3DVisu().m_BoardPos;
}

// return true if we are in realistic mode render
bool EDA_3D_CANVAS::isRealisticMode() const
{
    return GetPrm3DVisu().IsRealisticMode();
}

// return true if aItem should be displayed
bool EDA_3D_CANVAS::isEnabled( DISPLAY3D_FLG aItem ) const
{
    return GetPrm3DVisu().GetFlag( aItem );
}


// Helper function: initialize the copper color to draw the board
// in realistic mode.
void EDA_3D_CANVAS::setGLCopperColor()
{
    glDisable( GL_TEXTURE_2D );
    SetGLColor( GetPrm3DVisu().m_CopperColor, 1.0 );
}

// Helper function: initialize the color to draw the epoxy
// body board in realistic mode.
void EDA_3D_CANVAS::setGLEpoxyColor( double aTransparency )
{
    // Generates an epoxy color, near board color
    SetGLColor( GetPrm3DVisu().m_BoardBodyColor, aTransparency );

    if( isEnabled( FL_RENDER_TEXTURES ) )
    {
        SetGLTexture( m_text_pcb, TEXTURE_PCB_SCALE );
    }
}

// Helper function: initialize the color to draw the
// solder mask layers in realistic mode.
void EDA_3D_CANVAS::setGLSolderMaskColor( double aTransparency )
{
    // Generates a solder mask color
    SetGLColor( GetPrm3DVisu().m_SolderMaskColor, aTransparency );

    if( isEnabled( FL_RENDER_TEXTURES ) )
    {
        SetGLTexture( m_text_pcb, TEXTURE_PCB_SCALE );
    }
}

// Helper function: initialize the color to draw the non copper layers
// in realistic mode and normal mode.
void EDA_3D_CANVAS::setGLTechLayersColor( LAYER_NUM aLayer )
{
    EDA_COLOR_T color;

    if( isRealisticMode() )
    {
        switch( aLayer )
        {
        case B_Paste:
        case F_Paste:
            SetGLColor( DARKGRAY, 0.7 );
            break;

        case B_SilkS:
        case F_SilkS:
            SetGLColor( GetPrm3DVisu().m_SilkScreenColor, 0.96 );

            if( isEnabled( FL_RENDER_TEXTURES ) )
            {
                SetGLTexture( m_text_silk, 10.0f );
            }

            break;

        case B_Mask:
        case F_Mask:
            setGLSolderMaskColor( 0.90 );
            break;

        default:
            color = g_ColorsSettings.GetLayerColor( aLayer );
            SetGLColor( color, 0.7 );
            break;
        }
    }
    else
    {
        color = g_ColorsSettings.GetLayerColor( aLayer );
        SetGLColor( color, 0.7 );
    }
}

void EDA_3D_CANVAS::Draw3DAxis()
{
    if( ! m_glLists[GL_ID_AXIS] )
    {
        m_glLists[GL_ID_AXIS] = glGenLists( 1 );
        glNewList( m_glLists[GL_ID_AXIS], GL_COMPILE );

        glEnable( GL_COLOR_MATERIAL );
        SetGLColor( WHITE );
        glBegin( GL_LINES );
        glNormal3f( 0.0f, 0.0f, 1.0f );     // Normal is Z axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 1.0f, 0.0f, 0.0f );     // X axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 0.0f, -1.0f, 0.0f );    // Y axis
        glNormal3f( 1.0f, 0.0f, 0.0f );     // Normal is Y axis
        glVertex3f( 0.0f, 0.0f, 0.0f );
        glVertex3f( 0.0f, 0.0f, 0.3f );     // Z axis
        glEnd();

        glEndList();
    }
}

// draw a 3D grid: an horizontal grid (XY plane and Z = 0,
// and a vertical grid (XZ plane and Y = 0)
void EDA_3D_CANVAS::Draw3DGrid( double aGriSizeMM )
{
    double      zpos = 0.0;
    EDA_COLOR_T gridcolor = DARKGRAY;           // Color of grid lines
    EDA_COLOR_T gridcolor_marker = LIGHTGRAY;   // Color of grid lines every 5 lines
    const double scale = GetPrm3DVisu().m_BiuTo3Dunits;
    const double transparency = 0.3;

    glNormal3f( 0.0, 0.0, 1.0 );

    wxSize  brd_size = getBoardSize();
    wxPoint brd_center_pos = getBoardCenter();
    NEGATE( brd_center_pos.y );

    int     xsize   = std::max( brd_size.x, Millimeter2iu( 100 ) );
    int     ysize   = std::max( brd_size.y, Millimeter2iu( 100 ) );

    // Grid limits, in 3D units
    double  xmin    = (brd_center_pos.x - xsize / 2) * scale;
    double  xmax    = (brd_center_pos.x + xsize / 2) * scale;
    double  ymin    = (brd_center_pos.y - ysize / 2) * scale;
    double  ymax    = (brd_center_pos.y + ysize / 2) * scale;
    double  zmin    = Millimeter2iu( -50 ) * scale;
    double  zmax    = Millimeter2iu( 100 ) * scale;

    // Draw horizontal grid centered on 3D origin (center of the board)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor, transparency );
        else
            SetGLColor( gridcolor_marker, transparency );

        int delta = KiROUND( ii * aGriSizeMM * IU_PER_MM );

        if( delta <= xsize / 2 )    // Draw grid lines parallel to X axis
        {
            glBegin( GL_LINES );
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymin, zpos );
            glVertex3f( (brd_center_pos.x + delta) * scale, -ymax, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin( GL_LINES );
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymin, zpos );
                glVertex3f( (brd_center_pos.x - delta) * scale, -ymax, zpos );
                glEnd();
            }
        }

        if( delta <= ysize / 2 )    // Draw grid lines parallel to Y axis
        {
            glBegin( GL_LINES );
            glVertex3f( xmin, -(brd_center_pos.y + delta) * scale, zpos );
            glVertex3f( xmax, -(brd_center_pos.y + delta) * scale, zpos );
            glEnd();

            if( ii != 0 )
            {
                glBegin( GL_LINES );
                glVertex3f( xmin, -(brd_center_pos.y - delta) * scale, zpos );
                glVertex3f( xmax, -(brd_center_pos.y - delta) * scale, zpos );
                glEnd();
            }
        }

        if( ( delta > ysize / 2 ) && ( delta > xsize / 2 ) )
            break;
    }

    // Draw vertical grid n Z axis
    glNormal3f( 0.0, -1.0, 0.0 );

    // Draw vertical grid lines (parallel to Z axis)
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor, transparency );
        else
            SetGLColor( gridcolor_marker, transparency );

        double delta = ii * aGriSizeMM * IU_PER_MM;

        glBegin( GL_LINES );
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmin );
        glVertex3f( (brd_center_pos.x + delta) * scale, -brd_center_pos.y * scale, zmax );
        glEnd();

        if( ii != 0 )
        {
            glBegin( GL_LINES );
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmin );
            glVertex3f( (brd_center_pos.x - delta) * scale, -brd_center_pos.y * scale, zmax );
            glEnd();
        }

        if( delta > xsize / 2 )
            break;
    }

    // Draw horizontal grid lines on Z axis
    for( int ii = 0; ; ii++ )
    {
        if( (ii % 5) )
            SetGLColor( gridcolor, transparency);
        else
            SetGLColor( gridcolor_marker, transparency );

        double delta = ii * aGriSizeMM * IU_PER_MM * scale;

        if( delta <= zmax )
        {
            // Draw grid lines on Z axis (positive Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, -brd_center_pos.y * scale, delta );
            glVertex3f( xmax, -brd_center_pos.y * scale, delta );
            glEnd();
        }

        if( delta <= -zmin && ( ii != 0 ) )
        {
            // Draw grid lines on Z axis (negative Z axis coordinates)
            glBegin( GL_LINES );
            glVertex3f( xmin, -brd_center_pos.y * scale, -delta );
            glVertex3f( xmax, -brd_center_pos.y * scale, -delta );
            glEnd();
        }

        if( ( delta > zmax ) && ( delta > -zmin ) )
            break;
    }
}


// Draw 3D pads.
void EDA_3D_CANVAS::Draw3DPadHole( const D_PAD* aPad )
{
    // Draw the pad hole
    wxSize  drillsize   = aPad->GetDrillSize();
    bool    hasHole     = drillsize.x && drillsize.y;

    if( !hasHole )
        return;

    // Store here the points to approximate hole by segments
    CPOLYGONS_LIST  holecornersBuffer;
    int             thickness   = GetPrm3DVisu().GetCopperThicknessBIU();
    int             height      = GetPrm3DVisu().GetLayerZcoordBIU( F_Cu ) -
                                  GetPrm3DVisu().GetLayerZcoordBIU( B_Cu );

    if( isRealisticMode() )
        setGLCopperColor();
    else
        SetGLColor( DARKGRAY );

    int holeZpoz    = GetPrm3DVisu().GetLayerZcoordBIU( B_Cu ) + thickness / 2;
    int holeHeight  = height - thickness;

    if( drillsize.x == drillsize.y )    // usual round hole
    {
        Draw3D_ZaxisCylinder( aPad->GetPosition(),
                              (drillsize.x + thickness) / 2, holeHeight,
                              thickness, holeZpoz, GetPrm3DVisu().m_BiuTo3Dunits );
    }
    else    // Oblong hole
    {
        wxPoint ends_offset;
        int     width;

        if( drillsize.x > drillsize.y )    // Horizontal oval
        {
            ends_offset.x = ( drillsize.x - drillsize.y ) / 2;
            width = drillsize.y;
        }
        else    // Vertical oval
        {
            ends_offset.y = ( drillsize.y - drillsize.x ) / 2;
            width = drillsize.x;
        }

        RotatePoint( &ends_offset, aPad->GetOrientation() );

        wxPoint start   = aPad->GetPosition() + ends_offset;
        wxPoint end     = aPad->GetPosition() - ends_offset;
        int     hole_radius = ( width + thickness ) / 2;

        // Draw the hole
        Draw3D_ZaxisOblongCylinder( start, end, hole_radius, holeHeight,
                                    thickness, holeZpoz, GetPrm3DVisu().m_BiuTo3Dunits );
    }
}


void EDA_3D_CANVAS::Draw3DViaHole( const VIA* aVia )
{
    LAYER_ID    top_layer, bottom_layer;
    int         inner_radius    = aVia->GetDrillValue() / 2;
    int         thickness       = GetPrm3DVisu().GetCopperThicknessBIU();

    aVia->LayerPair( &top_layer, &bottom_layer );

    // Drawing via hole:
    if( isRealisticMode() )
        setGLCopperColor();
    else
    {
        EDA_COLOR_T color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + aVia->GetViaType() );
        SetGLColor( color );
    }

    int height = GetPrm3DVisu().GetLayerZcoordBIU( top_layer ) -
                 GetPrm3DVisu().GetLayerZcoordBIU( bottom_layer ) - thickness;
    int   zpos = GetPrm3DVisu().GetLayerZcoordBIU( bottom_layer ) + thickness / 2;

    Draw3D_ZaxisCylinder( aVia->GetStart(), inner_radius + thickness / 2, height,
                          thickness, zpos, GetPrm3DVisu().m_BiuTo3Dunits );
}

/* Build a pad outline as non filled polygon, to draw pads on silkscreen layer
 * Used only to draw pads outlines on silkscreen layers.
 */
void EDA_3D_CANVAS::BuildPadShapeThickOutlineAsPolygon( const D_PAD*  aPad,
                                                CPOLYGONS_LIST& aCornerBuffer,
                                                int             aWidth,
                                                int             aCircleToSegmentsCount,
                                                double          aCorrectionFactor )
{
    if( aPad->GetShape() == PAD_CIRCLE )    // Draw a ring
    {
        TransformRingToPolygon( aCornerBuffer, aPad->ShapePos(),
                                aPad->GetSize().x / 2, aCircleToSegmentsCount, aWidth );
        return;
    }

    // For other shapes, draw polygon outlines
    CPOLYGONS_LIST corners;
    aPad->BuildPadShapePolygon( corners, wxSize( 0, 0 ),
                                aCircleToSegmentsCount, aCorrectionFactor );

    // Add outlines as thick segments in polygon buffer
    for( unsigned ii = 0, jj = corners.GetCornersCount() - 1;
         ii < corners.GetCornersCount(); jj = ii, ii++ )
    {
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              corners.GetPos( jj ),
                                              corners.GetPos( ii ),
                                              aCircleToSegmentsCount, aWidth );
    }
}

