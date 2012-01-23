/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file 3d_aux.cpp
 */

#include <fctsys.h>

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include <common.h>
#include <trigo.h>
#include <wxBasePcbFrame.h>

#include <class_board_design_settings.h>
#include <class_zone.h>
#include <class_text_mod.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>

#include <3d_viewer.h>
#include <trackball.h>


void S3D_MASTER::Set_Object_Coords( std::vector< S3D_Vertex >& aVertices )
{
    unsigned ii;

    /* adjust object scale, rotation and offset position */
    for( ii = 0; ii < aVertices.size(); ii++ )
    {
        aVertices[ii].x *= m_MatScale.x;
        aVertices[ii].y *= m_MatScale.y;
        aVertices[ii].z *= m_MatScale.z;

        /* adjust rotation */
        if( m_MatRotation.x )
            RotatePoint( &aVertices[ii].y, &aVertices[ii].z, (int) (m_MatRotation.x * 10) );

        if( m_MatRotation.y )
            RotatePoint( &aVertices[ii].z, &aVertices[ii].x, (int) (m_MatRotation.y * 10) );

        if( m_MatRotation.z )
            RotatePoint( &aVertices[ii].x, &aVertices[ii].y, (int) (m_MatRotation.z * 10) );

        /* adjust offset position (offset is given in UNIT 3D (0.1 inch) */
#define SCALE_3D_CONV (PCB_INTERNAL_UNIT / UNITS3D_TO_UNITSPCB)
        aVertices[ii].x += m_MatPosition.x * SCALE_3D_CONV;
        aVertices[ii].y += m_MatPosition.y * SCALE_3D_CONV;
        aVertices[ii].z += m_MatPosition.z * SCALE_3D_CONV;
    }
}


void Set_Object_Data( std::vector< S3D_Vertex >& aVertices )
{
    unsigned ii;
    GLfloat ax, ay, az, bx, by, bz, nx, ny, nz, r;

    /* ignore faces with less than 3 points */
    if( aVertices.size() < 3 )
        return;

    /* calculate normal direction */
    ax = aVertices[1].x - aVertices[0].x;
    ay = aVertices[1].y - aVertices[0].y;
    az = aVertices[1].z - aVertices[0].z;

    bx = aVertices[aVertices.size() - 1].x - aVertices[0].x;
    by = aVertices[aVertices.size() - 1].y - aVertices[0].y;
    bz = aVertices[aVertices.size() - 1].z - aVertices[0].z;

    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;

    r = sqrt( nx * nx + ny * ny + nz * nz );

    if( r >= 0.000001 ) /* avoid division by zero */
    {
        nx /= r;
        ny /= r;
        nz /= r;
        glNormal3f( nx, ny, nz );
    }

    /* glBegin/glEnd */
    switch( aVertices.size() )
    {
    case 3:
        glBegin( GL_TRIANGLES );
        break;

    case 4:
        glBegin( GL_QUADS );
        break;

    default:
        glBegin( GL_POLYGON );
        break;
    }

    /* draw polygon/triangle/quad */
    for( ii = 0; ii < aVertices.size(); ii++ )
    {
        glVertex3f( aVertices[ii].x * DataScale3D,
                    aVertices[ii].y * DataScale3D,
                    aVertices[ii].z * DataScale3D );
    }

    glEnd();
}


GLuint EDA_3D_CANVAS::DisplayCubeforTest()
{
    GLuint gllist = glGenLists( 1 );

    glNewList( gllist, GL_COMPILE_AND_EXECUTE );
    /* draw six faces of a cube */
    glBegin( GL_QUADS );
    glNormal3f( 0.0F, 0.0F, 1.0F );
    glVertex3f( 0.5F, 0.5F, 0.5F ); glVertex3f( -0.5F, 0.5F, 0.5F );
    glVertex3f( -0.5F, -0.5F, 0.5F ); glVertex3f( 0.5F, -0.5F, 0.5F );

    glNormal3f( 0.0F, 0.0F, -1.0F );
    glVertex3f( -0.5F, -0.5F, -0.5F ); glVertex3f( -0.5F, 0.5F, -0.5F );
    glVertex3f( 0.5F, 0.5F, -0.5F ); glVertex3f( 0.5F, -0.5F, -0.5F );

    glNormal3f( 0.0F, 1.0F, 0.0F );
    glVertex3f( 0.5F, 0.5F, 0.5F ); glVertex3f( 0.5F, 0.5F, -0.5F );
    glVertex3f( -0.5F, 0.5F, -0.5F ); glVertex3f( -0.5F, 0.5F, 0.5F );

    glNormal3f( 0.0F, -1.0F, 0.0F );
    glVertex3f( -0.5F, -0.5F, -0.5F ); glVertex3f( 0.5F, -0.5F, -0.5F );
    glVertex3f( 0.5F, -0.5F, 0.5F ); glVertex3f( -0.5F, -0.5F, 0.5F );

    glNormal3f( 1.0F, 0.0F, 0.0F );
    glVertex3f( 0.5F, 0.5F, 0.5F ); glVertex3f( 0.5F, -0.5F, 0.5F );
    glVertex3f( 0.5F, -0.5F, -0.5F ); glVertex3f( 0.5F, 0.5F, -0.5F );

    glNormal3f( -1.0F, 0.0F, 0.0F );
    glVertex3f( -0.5F, -0.5F, -0.5F ); glVertex3f( -0.5F, -0.5F, 0.5F );
    glVertex3f( -0.5F, 0.5F, 0.5F ); glVertex3f( -0.5F, 0.5F, -0.5F );
    glEnd();

    glEndList();

    return gllist;
}


Info_3D_Visu::Info_3D_Visu()
{
    int ii;

    m_Beginx = m_Beginy = 0.0;  /* position of mouse */
    m_Zoom   = 1.0;             /* field of view in degrees */
    trackball( m_Quat, 0.0, 0.0, 0.0, 0.0 );

    for( ii = 0; ii < 4; ii++ )
        m_Rot[ii] = 0.0;

    m_Layers = 1;
    m_BoardSettings  = NULL;
    m_Draw3DAxis     = true;
    m_Draw3DModule   = true;
    m_Draw3DZone     = true;
    m_Draw3DComments = true;
    m_Draw3DDrawings = true;
    m_Draw3DEco1     = true;
    m_Draw3DEco2     = true;
}


Info_3D_Visu::~Info_3D_Visu()
{
}


WinEDA_VertexCtrl::WinEDA_VertexCtrl( wxWindow* parent, const wxString& title,
                                      wxBoxSizer* BoxSizer,
                                      EDA_UNITS_T units, int internal_unit )
{
    wxString      text;
    wxStaticText* msgtitle;

    m_Units = units;
    m_Internal_Unit = internal_unit;

    if( title.IsEmpty() )
        text = _( "Vertex " );
    else
        text = title;

    text += ReturnUnitSymbol( units );

    msgtitle = new wxStaticText( parent, -1, text, wxDefaultPosition, wxSize( -1, -1 ), 0 );

    BoxSizer->Add( msgtitle, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM | wxADJUST_MINSIZE );

    wxFlexGridSizer* GridSizer = new wxFlexGridSizer( 3, 2, 0, 0 );

    BoxSizer->Add( GridSizer, 0, wxGROW | wxALL, 5 );

    msgtitle = new wxStaticText( parent, -1, wxT( "X:" ) );

    GridSizer->Add( msgtitle, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT | wxADJUST_MINSIZE,
                    5 );
    m_XValueCtrl = new wxTextCtrl( parent, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize( -1, -1 ), 0 );

    GridSizer->Add( m_XValueCtrl,
                    0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT,
                    5 );

    msgtitle = new wxStaticText( parent, -1, wxT( "Y:" ), wxDefaultPosition,
                                 wxSize( -1, -1 ), 0 );

    GridSizer->Add( msgtitle,
                    0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT | wxADJUST_MINSIZE,
                    5 );
    m_YValueCtrl = new wxTextCtrl( parent, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize( -1, -1 ), 0 );

    GridSizer->Add( m_YValueCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT,
                    5 );

    msgtitle = new wxStaticText( parent, -1, wxT( "Z:" ), wxDefaultPosition,
                                 wxSize( -1, -1 ), 0 );

    GridSizer->Add( msgtitle, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT | wxADJUST_MINSIZE,
                    5 );
    m_ZValueCtrl = new wxTextCtrl( parent, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize( -1, -1 ), 0 );

    GridSizer->Add( m_ZValueCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL |
                    wxLEFT | wxRIGHT,
                    5 );
}


WinEDA_VertexCtrl::~WinEDA_VertexCtrl()
{
}


S3D_Vertex WinEDA_VertexCtrl::GetValue()
{
    S3D_Vertex value;
    double     dtmp;

    m_XValueCtrl->GetValue().ToDouble( &dtmp );
    value.x = dtmp;
    m_YValueCtrl->GetValue().ToDouble( &dtmp );
    value.y = dtmp;
    m_ZValueCtrl->GetValue().ToDouble( &dtmp );
    value.z = dtmp;
    return value;
}


void WinEDA_VertexCtrl::SetValue( S3D_Vertex vertex )
{
    wxString text;

    text.Printf( wxT( "%f" ), vertex.x );
    m_XValueCtrl->Clear();
    m_XValueCtrl->AppendText( text );

    text.Printf( wxT( "%f" ), vertex.y );
    m_YValueCtrl->Clear();
    m_YValueCtrl->AppendText( text );

    text.Printf( wxT( "%f" ), vertex.z );
    m_ZValueCtrl->Clear();
    m_ZValueCtrl->AppendText( text );
}


void WinEDA_VertexCtrl::Enable( bool onoff )
{
    m_XValueCtrl->Enable( onoff );
    m_YValueCtrl->Enable( onoff );
    m_ZValueCtrl->Enable( onoff );
}
