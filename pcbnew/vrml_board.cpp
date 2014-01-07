/*
 * file: vrml_board.cpp
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013  Cirilo Bernardo
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

/*
 * NOTES ON OUTPUT PRECISION:
 *
 * If we use %.6f then we have no need for special unit dependent formatting:
 *
 *  inch: .0254 microns
 *  mm:   0.001 microns
 *  m:    1 micron
 *
 */


#include <sstream>
#include <string>
#include <iomanip>
#include <cmath>
#include <fctsys.h>
#include <vrml_board.h>

#ifndef CALLBACK
#define CALLBACK
#endif

#define GLCALLBACK(x) (( void (CALLBACK*)() )&(x))

void FormatDoublet( double x, double y, int precision, std::string& strx, std::string& stry )
{
    std::ostringstream ostr;

    ostr << std::fixed << std::setprecision( precision );

    ostr << x;
    strx = ostr.str();

    ostr.str( "" );
    ostr << y;
    stry = ostr.str();

    while( *strx.rbegin() == '0' )
        strx.erase( strx.size() - 1 );

    while( *stry.rbegin() == '0' )
        stry.erase( stry.size() - 1 );
}


void FormatSinglet( double x, int precision, std::string& strx )
{
    std::ostringstream ostr;

    ostr << std::fixed << std::setprecision( precision );

    ostr << x;
    strx = ostr.str();

    while( *strx.rbegin() == '0' )
        strx.erase( strx.size() - 1 );
}


int CalcNSides( double rad, double dev )
{
    if( dev <= 0 || rad <= 0 )
        return 6;

    int csides;
    double n = dev / rad;

    // note: in the following, the first comparison and csides is chosen to
    // yield a maximum of 360 segments; in practice we probably want a smaller limit.
    if( n < 0.0001523048 )
        csides = 360;
    else if( n >= 0.5 ) // 0.5 yields an angle >= 60 deg. (6 or fewer sides)
        csides = 6;
    else
        csides = M_PI * 2.0 / acos( 1.0 - n ) + 1;

    if( csides < 6 )
        csides = 6;

    return csides;
}


static void CALLBACK vrml_tess_begin( GLenum cmd, void* user_data )
{
    VRML_LAYER* lp = (VRML_LAYER*) user_data;

    lp->glStart( cmd );
}


static void CALLBACK vrml_tess_end( void* user_data )
{
    VRML_LAYER* lp = (VRML_LAYER*) user_data;

    lp->glEnd();
}


static void CALLBACK vrml_tess_vertex( void* vertex_data, void* user_data )
{
    VRML_LAYER* lp = (VRML_LAYER*) user_data;

    lp->glPushVertex( (VERTEX_3D*) vertex_data );
}


static void CALLBACK vrml_tess_err( GLenum errorID, void* user_data )
{
    VRML_LAYER* lp = (VRML_LAYER*) user_data;

    lp->Fault = true;
    lp->SetGLError( errorID );
}


static void CALLBACK vrml_tess_combine( GLdouble coords[3], void* vertex_data[4],
        GLfloat weight[4], void** outData, void* user_data )
{
    VRML_LAYER* lp = (VRML_LAYER*) user_data;

    *outData = lp->AddExtraVertex( coords[0], coords[1] );
}


VRML_LAYER::VRML_LAYER()
{
    fix = false;
    Fault = false;
    idx = 0;
    ord = 0;
    glcmd   = 0;
    pholes  = NULL;
    maxdev  = 0.02;

    tess = gluNewTess();

    if( !tess )
        return;

    // set up the tesselator callbacks
    gluTessCallback( tess, GLU_TESS_BEGIN_DATA, GLCALLBACK( vrml_tess_begin ) );

    gluTessCallback( tess, GLU_TESS_VERTEX_DATA, GLCALLBACK( vrml_tess_vertex ) );

    gluTessCallback( tess, GLU_TESS_END_DATA, GLCALLBACK( vrml_tess_end ) );

    gluTessCallback( tess, GLU_TESS_ERROR_DATA, GLCALLBACK( vrml_tess_err ) );

    gluTessCallback( tess, GLU_TESS_COMBINE_DATA, GLCALLBACK( vrml_tess_combine ) );

    gluTessProperty( tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE );

    gluTessNormal( tess, 0, 0, 1 );
}


VRML_LAYER::~VRML_LAYER()
{
    Clear();

    if( tess )
    {
        gluDeleteTess( tess );
        tess = NULL;
    }
}


// clear all data
void VRML_LAYER::Clear( void )
{
    int i;

    fix = false;
    idx = 0;

    for( i = contours.size(); i > 0; --i )
    {
        delete contours.back();
        contours.pop_back();
    }

    while( !areas.empty() )
        areas.pop_back();

    for( i = vertices.size(); i > 0; --i )
    {
        delete vertices.back();
        vertices.pop_back();
    }

    clearTmp();
}


// set the max. deviation of an arc segment
bool VRML_LAYER::SetMaxDev( double max )
{
    // assure max. dev > 2 microns regardless of the
    // prevailing units ( inch, mm, m, 0.1 inch )
    if( max < 0.000002 )
    {
        error = "SetMaxDev(): specified value is < 0.000002";
        return false;
    }

    maxdev = max;

    return true;
}


// clear ephemeral data in between invocations of the tesselation routine
void VRML_LAYER::clearTmp( void )
{
    unsigned int i;

    Fault   = false;
    hidx    = 0;
    eidx    = 0;
    ord = 0;
    glcmd = 0;

    while( !triplets.empty() )
        triplets.pop_back();

    for( i = outline.size(); i > 0; --i )
    {
        delete outline.back();
        outline.pop_back();
    }

    for( i = ordmap.size(); i > 0; --i )
        ordmap.pop_back();

    for( i = extra_verts.size(); i > 0; --i )
    {
        delete extra_verts.back();
        extra_verts.pop_back();
    }

    // note: unlike outline and extra_verts,
    // vlist is not responsible for memory management
    for( i = vlist.size(); i > 0; --i )
        vlist.pop_back();

    // go through the vertex list and reset ephemeral parameters
    for( i = 0; i < vertices.size(); ++i )
    {
        vertices[i]->o = -1;
    }
}


// create a new contour to be populated; returns an index
// into the contour list or -1 if there are problems
int VRML_LAYER::NewContour( void )
{
    if( fix )
        return -1;

    std::list<int>* contour = new std::list<int>;

    if( !contour )
        return -1;

    contours.push_back( contour );
    areas.push_back( 0.0 );

    return contours.size() - 1;
}


// adds a vertex to the existing list and places its index in
// an existing contour; returns true if OK,
// false otherwise (indexed contour does not exist)
bool VRML_LAYER::AddVertex( int aContour, double x, double y )
{
    if( fix )
    {
        error = "AddVertex(): no more vertices may be added (Tesselate was previously executed)";
        return false;
    }

    if( aContour < 0 || (unsigned int) aContour >= contours.size() )
    {
        error = "AddVertex(): aContour is not within a valid range";
        return false;
    }

    VERTEX_3D* vertex = new VERTEX_3D;

    if( !vertex )
    {
        error = "AddVertex(): a new vertex could not be allocated";
        return false;
    }

    vertex->x   = x;
    vertex->y   = y;
    vertex->i   = idx++;
    vertex->o   = -1;

    VERTEX_3D* v2 = NULL;

    if( contours[aContour]->size() > 0 )
        v2 = vertices[ contours[aContour]->back() ];

    vertices.push_back( vertex );
    contours[aContour]->push_back( vertex->i );

    if( v2 )
        areas[aContour] += ( x - v2->x ) * ( y + v2->y );

    return true;
}


// ensure the winding of a contour with respect to the normal (0, 0, 1);
// set 'hole' to true to ensure a hole (clockwise winding)
bool VRML_LAYER::EnsureWinding( int aContour, bool hole )
{
    if( aContour < 0 || (unsigned int) aContour >= contours.size() )
    {
        error = "EnsureWinding(): aContour is outside the valid range";
        return false;
    }

    std::list<int>* cp = contours[aContour];

    if( cp->size() < 3 )
    {
        error = "EnsureWinding(): there are fewer than 3 vertices";
        return false;
    }

    double dir = areas[aContour];

    VERTEX_3D* vp0 = vertices[ cp->back() ];
    VERTEX_3D* vp1 = vertices[ cp->front() ];

    dir += ( vp1->x - vp0->x ) * ( vp1->y + vp0->y );

    // if dir is positive, winding is CW
    if( ( hole && dir < 0 ) || ( !hole && dir > 0 ) )
    {
        cp->reverse();
        areas[aContour] = -areas[aContour];
    }

    return true;
}


// adds a circle the existing list; if 'hole' is true the contour is
// a hole. Returns true if OK.
bool VRML_LAYER::AddCircle( double x, double y, double rad, int csides, bool hole )
{
    int pad = NewContour();

    if( pad < 0 )
    {
        error = "AddCircle(): failed to add a contour";
        return false;
    }

    if( csides < 6 )
        csides = CalcNSides( rad, maxdev );

    // even numbers give prettier results
    if( csides & 1 )
        csides += 1;

    double da = M_PI * 2.0 / csides;

    bool fail = false;

    if( hole )
    {
        for( double angle = 0; angle < M_PI * 2; angle += da )
            fail |= !AddVertex( pad, x + rad * cos( angle ), y - rad * sin( angle ) );
    }
    else
    {
        for( double angle = 0; angle < M_PI * 2; angle += da )
            fail |= !AddVertex( pad, x + rad * cos( angle ), y + rad * sin( angle ) );
    }

    return !fail;
}


// adds a slotted pad with orientation given by angle; if 'hole' is true the
// contour is a hole. Returns true if OK.
bool VRML_LAYER::AddSlot( double cx, double cy, double length, double width,
        double angle, int csides, bool hole )
{
    if( width > length )
    {
        angle += M_PI2;
        std::swap( length, width );
    }

    width   /= 2.0;
    length  = length / 2.0 - width;

    if( csides < 6 )
        csides = CalcNSides( width, maxdev );

    if( csides & 1 )
        csides += 1;

    csides /= 2;

    double capx, capy;

    capx    = cx + cos( angle ) * length;
    capy    = cy + sin( angle ) * length;

    double ang, da;
    int i;
    int pad = NewContour();

    if( pad < 0 )
    {
        error = "AddCircle(): failed to add a contour";
        return false;
    }

    da = M_PI / csides;
    bool fail = false;

    if( hole )
    {
        for( ang = angle + M_PI2, i = 0; i < csides; ang -= da, ++i )
            fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        ang = angle - M_PI2;
        fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        capx    = cx - cos( angle ) * length;
        capy    = cy - sin( angle ) * length;

        for( ang = angle - M_PI2, i = 0; i < csides; ang -= da, ++i )
            fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        ang = angle + M_PI2;
        fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );
    }
    else
    {
        for( ang = angle - M_PI2, i = 0; i < csides; ang += da, ++i )
            fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        ang = angle + M_PI2;
        fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        capx    = cx - cos( angle ) * length;
        capy    = cy - sin( angle ) * length;

        for( ang = angle + M_PI2, i = 0; i < csides; ang += da, ++i )
            fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );

        ang = angle - M_PI2;
        fail |= !AddVertex( pad, capx + width * cos( ang ), capy + width * sin( ang ) );
    }

    return !fail;
}


// adds an arc with the given center, start point, pen width, and angle.
bool VRML_LAYER::AddArc( double cx, double cy, double startx, double starty,
        double width, double angle, int csides, bool hole )
{
    // we don't accept small angles; in fact, 1 degree ( 0.01745 ) is already
    // way too small but we must set a limit somewhere
    if( angle < 0.01745 && angle > -0.01745 )
    {
        error = "AddArc(): angle is too small: abs( angle ) < 0.01745";
        return false;
    }

    double rad = sqrt( (startx - cx) * (startx - cx) + (starty - cy) * (starty - cy) );

    width /= 2.0;    // this is the radius of the caps

    // we will not accept an arc with an inner radius close to zero so we
    // set a limit here. the end result will vary somewhat depending on
    // the output units
    if( width >= ( rad * 1.01 ) )
    {
        error = "AddArc(): width/2 exceeds radius*1.01";
        return false;
    }

    // calculate the radii of the outer and inner arcs
    double  orad    = rad + width;
    double  irad    = rad - width;

    int osides  = csides * angle / ( M_PI * 2.0 );
    int isides  = csides * angle / ( M_PI * 2.0 );

    if( osides < 0 )
        osides = -osides;

    if( osides < 3 )
    {
        osides = CalcNSides( orad, maxdev ) * angle / ( M_PI * 2.0 );

        if( osides < 0 )
            osides = -osides;

        if( osides < 3 )
            osides = 3;
    }

    if( isides < 0 )
        isides = -isides;

    if( isides < 3 )
    {
        isides = CalcNSides( irad, maxdev ) * angle / ( M_PI * 2.0 );

        if( isides < 0 )
            isides = -isides;

        if( isides < 3 )
            isides = 3;
    }

    if( csides < 6 )
        csides = CalcNSides( width, maxdev );

    if( csides & 1 )
        csides += 1;

    csides /= 2;

    double  stAngle     = atan2( starty - cy, startx - cx );
    double  endAngle    = stAngle + angle;

    // calculate ends of inner and outer arc
    double  oendx   = cx + orad* cos( endAngle );
    double  oendy   = cy + orad* sin( endAngle );
    double  ostx    = cx + orad* cos( stAngle );
    double  osty    = cy + orad* sin( stAngle );

    double  iendx   = cx + irad* cos( endAngle );
    double  iendy   = cy + irad* sin( endAngle );
    double  istx    = cx + irad* cos( stAngle );
    double  isty    = cy + irad* sin( stAngle );

    if( ( angle < 0 && !hole ) || ( angle > 0 && hole ) )
    {
        angle = -angle;
        std::swap( stAngle, endAngle );
        std::swap( oendx, ostx );
        std::swap( oendy, osty );
        std::swap( iendx, istx );
        std::swap( iendy, isty );
    }

    int arc = NewContour();

    if( arc < 0 )
    {
        error = "AddArc(): could not create a contour";
        return false;
    }

    // trace the outer arc:
    int i;
    double  ang;
    double  da = angle / osides;

    for( ang = stAngle, i = 0; i < osides; ang += da, ++i )
        AddVertex( arc, cx + orad * cos( ang ), cy + orad * sin( ang ) );

    // trace the first cap
    double  capx    = ( iendx + oendx ) / 2.0;
    double  capy    = ( iendy + oendy ) / 2.0;

    if( hole )
        da = -M_PI / csides;
    else
        da = M_PI / csides;

    for( ang = endAngle + da, i = 2; i < csides; ang += da, ++i )
        AddVertex( arc, capx + width * cos( ang ), capy + width * sin( ang ) );

    // trace the inner arc:
    da = -angle / isides;

    for( ang = endAngle, i = 0; i < isides; ang += da, ++i )
        AddVertex( arc, cx + irad * cos( ang ), cy + irad * sin( ang ) );

    // trace the final cap
    capx    = ( istx + ostx ) / 2.0;
    capy    = ( isty + osty ) / 2.0;

    if( hole )
        da = -M_PI / csides;
    else
        da = M_PI / csides;

    for( ang = stAngle + M_PI + da, i = 2; i < csides; ang += da, ++i )
        AddVertex( arc, capx + width * cos( ang ), capy + width * sin( ang ) );

    return true;
}


// tesselates the contours in preparation for a 3D output;
// returns true if all was fine, false otherwise
bool VRML_LAYER::Tesselate( VRML_LAYER* holes )
{
    if( !tess )
    {
        error = "Tesselate(): GLU tesselator was not initialized";
        return false;
    }

    pholes  = holes;
    Fault   = false;

    if( contours.size() < 1 || vertices.size() < 3 )
    {
        error = "Tesselate(): not enough vertices";
        return false;
    }

    // finish the winding calculation on all vertices prior to setting 'fix'
    if( !fix )
    {
        for( unsigned int i = 0; i < contours.size(); ++i )
        {
            if( contours[i]->size() < 3 )
                continue;

            VERTEX_3D* vp0 = vertices[ contours[i]->back() ];
            VERTEX_3D* vp1 = vertices[ contours[i]->front() ];
            areas[i] += ( vp1->x - vp0->x ) * ( vp1->y + vp0->y );
        }
    }

    // prevent the addition of any further contours and contour vertices
    fix = true;

    // clear temporary internals which may have been used in a previous run
    clearTmp();

    // request an outline
    gluTessProperty( tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE );

    // adjust internal indices for extra points and holes
    if( holes )
        hidx = holes->GetSize();
    else
        hidx = 0;

    eidx = idx + hidx;

    // open the polygon
    gluTessBeginPolygon( tess, this );

    pushVertices( false );

    // close the polygon
    gluTessEndPolygon( tess );

    if( Fault )
        return false;

    // push the (solid) outline to the tesselator
    if( !pushOutline( holes ) )
        return false;

    // add the holes contained by this object
    pushVertices( true );

    // import external holes (if any)
    if( hidx && ( holes->Import( idx, tess ) < 0 ) )
    {
        std::ostringstream ostr;
        ostr << "Tesselate():FAILED: " << holes->GetError();
        error = ostr.str();
        return NULL;
    }

    if( Fault )
        return false;

    // erase the previous outline data and vertex order
    // but preserve the extra vertices
    for( int i = outline.size(); i > 0; --i )
    {
        delete outline.back();
        outline.pop_back();
    }

    for( unsigned int i = ordmap.size(); i > 0; --i )
        ordmap.pop_back();

    // go through the vertex lists and reset ephemeral parameters
    for( unsigned int i = 0; i < vertices.size(); ++i )
    {
        vertices[i]->o = -1;
    }

    for( unsigned int i = 0; i < extra_verts.size(); ++i )
    {
        extra_verts[i]->o = -1;
    }

    ord = 0;

    // close the polygon; we now have all the data necessary for the tesselation
    gluTessEndPolygon( tess );

    // request a tesselated surface
    gluTessProperty( tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE );

    if( !pushOutline( holes ) )
        return false;

    gluTessEndPolygon( tess );

    if( Fault )
        return false;

    return true;
}


bool VRML_LAYER::pushOutline( VRML_LAYER* holes )
{
    // traverse the outline list to push all used vertices
    if( outline.size() < 1 )
    {
        error = "pushOutline() failed: no vertices to push";
        return false;
    }

    gluTessBeginPolygon( tess, this );

    std::list<std::list<int>*>::const_iterator obeg = outline.begin();
    std::list<std::list<int>*>::const_iterator oend = outline.end();

    int pi;
    std::list<int>::const_iterator  begin;
    std::list<int>::const_iterator  end;
    GLdouble pt[3];
    VERTEX_3D* vp;

    while( obeg != oend )
    {
        if( (*obeg)->size() < 3 )
        {
            ++obeg;
            continue;
        }

        gluTessBeginContour( tess );

        begin = (*obeg)->begin();
        end = (*obeg)->end();

        while( begin != end )
        {
            pi = *begin;

            if( pi < 0 || (unsigned int) pi > ordmap.size() )
            {
                error = "pushOutline():BUG: *outline.begin() is not a valid index to ordmap";
                return false;
            }

            // retrieve the actual index
            pi = ordmap[pi];

            vp = getVertexByIndex( pi, holes );

            if( !vp )
            {
                error = "pushOutline():: BUG: ordmap[n] is not a valid index to vertices[]";
                return false;
            }

            pt[0]   = vp->x;
            pt[1]   = vp->y;
            pt[2]   = 0.0;
            gluTessVertex( tess, pt, vp );
            ++begin;
        }

        gluTessEndContour( tess );
        ++obeg;
    }

    return true;
}


// writes out the vertex list;
// 'z' is the Z coordinate of every point
bool VRML_LAYER::WriteVertices( double z, FILE* fp )
{
    if( !fp )
    {
        error = "WriteVertices(): invalid file pointer";
        return false;
    }

    if( ordmap.size() < 3 )
    {
        error = "WriteVertices(): not enough vertices";
        return false;
    }

    int i, j;

    VERTEX_3D* vp = getVertexByIndex( ordmap[0], pholes );

    if( !vp )
        return false;

    std::string strx, stry, strz;
    FormatDoublet( vp->x, vp->y, 6, strx, stry );
    FormatSinglet( z, 6, strz );

    fprintf( fp, "%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );

    for( i = 1, j = ordmap.size(); i < j; ++i )
    {
        vp = getVertexByIndex( ordmap[i], pholes );

        if( !vp )
            return false;

        FormatDoublet( vp->x, vp->y, 6, strx, stry );

        if( i & 1 )
            fprintf( fp, ", %s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
        else
            fprintf( fp, ",\n%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
    }

    return true;
}


// writes out the vertex list for a 3D feature; top and bottom are the
// Z values for the top and bottom; top must be > bottom
bool VRML_LAYER::Write3DVertices( double top, double bottom, FILE* fp )
{
    if( !fp )
    {
        error = "Write3DVertices(): NULL file pointer";
        return false;
    }

    if( ordmap.size() < 3 )
    {
        error = "Write3DVertices(): insufficient vertices";
        return false;
    }

    if( top <= bottom )
    {
        error = "Write3DVertices(): top <= bottom";
        return false;
    }

    int i, j;

    VERTEX_3D* vp = getVertexByIndex( ordmap[0], pholes );

    if( !vp )
        return false;

    std::string strx, stry, strz;
    FormatDoublet( vp->x, vp->y, 6, strx, stry );
    FormatSinglet( top, 6, strz );

    fprintf( fp, "%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );

    for( i = 1, j = ordmap.size(); i < j; ++i )
    {
        vp = getVertexByIndex( ordmap[i], pholes );

        if( !vp )
            return false;

        FormatDoublet( vp->x, vp->y, 6, strx, stry );

        if( i & 1 )
            fprintf( fp, ", %s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
        else
            fprintf( fp, ",\n%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
    }

    // repeat for the bottom layer
    vp = getVertexByIndex( ordmap[0], pholes );
    FormatDoublet( vp->x, vp->y, 6, strx, stry );
    FormatSinglet( bottom, 6, strz );

    bool endl;

    if( i & 1 )
    {
        fprintf( fp, ", %s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
        endl = false;
    }
    else
    {
        fprintf( fp, ",\n%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
        endl = true;
    }

    for( i = 1, j = ordmap.size(); i < j; ++i )
    {
        vp = getVertexByIndex( ordmap[i], pholes );
        FormatDoublet( vp->x, vp->y, 6, strx, stry );

        if( endl )
        {
            fprintf( fp, ", %s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
            endl = false;
        }
        else
        {
            fprintf( fp, ",\n%s %s %s", strx.c_str(), stry.c_str(), strz.c_str() );
            endl = true;
        }
    }

    return true;
}


// writes out the index list;
// 'top' indicates the vertex ordering and should be
// true for a polygon visible from above the PCB
bool VRML_LAYER::WriteIndices( bool top, FILE* fp )
{
    if( triplets.empty() )
    {
        error = "WriteIndices(): no triplets (triangular facets) to write";
        return false;
    }

    // go through the triplet list and write out the indices based on order
    std::list<TRIPLET_3D>::const_iterator   tbeg    = triplets.begin();
    std::list<TRIPLET_3D>::const_iterator   tend    = triplets.end();

    int i = 1;

    if( top )
        fprintf( fp, "%d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
    else
        fprintf( fp, "%d, %d, %d, -1", tbeg->i2, tbeg->i1, tbeg->i3 );

    ++tbeg;

    while( tbeg != tend )
    {
        if( (i++ & 7) == 4 )
        {
            i = 1;

            if( top )
                fprintf( fp, ",\n%d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
            else
                fprintf( fp, ",\n%d, %d, %d, -1", tbeg->i2, tbeg->i1, tbeg->i3 );
        }
        else
        {
            if( top )
                fprintf( fp, ", %d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
            else
                fprintf( fp, ", %d, %d, %d, -1", tbeg->i2, tbeg->i1, tbeg->i3 );
        }

        ++tbeg;
    }

    return true;
}


// writes out the index list for a 3D feature
bool VRML_LAYER::Write3DIndices( FILE* fp )
{
    if( triplets.empty() )
    {
        error = "Write3DIndices(): no triplets (triangular facets) to write";
        return false;
    }

    if( outline.empty() )
    {
        error = "WriteIndices(): no outline available";
        return false;
    }

    // go through the triplet list and write out the indices based on order
    std::list<TRIPLET_3D>::const_iterator   tbeg    = triplets.begin();
    std::list<TRIPLET_3D>::const_iterator   tend    = triplets.end();

    int i = 1;
    int idx2 = ordmap.size();    // index to the bottom vertices

    // print out the top vertices
    fprintf( fp, "%d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
    ++tbeg;

    while( tbeg != tend )
    {
        if( (i++ & 7) == 4 )
        {
            i = 1;
            fprintf( fp, ",\n%d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
        }
        else
        {
            fprintf( fp, ", %d, %d, %d, -1", tbeg->i1, tbeg->i2, tbeg->i3 );
        }

        ++tbeg;
    }

    // print out the bottom vertices
    tbeg = triplets.begin();

    while( tbeg != tend )
    {
        if( (i++ & 7) == 4 )
        {
            i = 1;
            fprintf( fp, ",\n%d, %d, %d, -1", tbeg->i2 + idx2, tbeg->i1 + idx2, tbeg->i3 + idx2 );
        }
        else
        {
            fprintf( fp, ", %d, %d, %d, -1", tbeg->i2 + idx2, tbeg->i1 + idx2, tbeg->i3 + idx2 );
        }

        ++tbeg;
    }

    int firstPoint;
    int lastPoint;
    int curPoint;

    std::list<std::list<int>*>::const_iterator  obeg    = outline.begin();
    std::list<std::list<int>*>::const_iterator  oend    = outline.end();
    std::list<int>* cp;
    std::list<int>::const_iterator  cbeg;
    std::list<int>::const_iterator  cend;

    while( obeg != oend )
    {
        cp = *obeg;

        if( cp->size() < 3 )
        {
            ++obeg;
            continue;
        }

        cbeg    = cp->begin();
        cend    = cp->end();

        firstPoint  = *(cbeg++);
        lastPoint   = firstPoint;

        while( cbeg != cend )
        {
            curPoint = *(cbeg++);
            fprintf( fp, ",\n %d, %d, %d, -1, %d, %d, %d, -1",
                    curPoint, lastPoint, curPoint + idx2,
                    curPoint + idx2, lastPoint, lastPoint + idx2 );
            lastPoint = curPoint;
        }

        fprintf( fp, ",\n %d, %d, %d, -1, %d, %d, %d, -1",
                firstPoint, lastPoint, firstPoint + idx2,
                firstPoint + idx2, lastPoint, lastPoint + idx2 );

        ++obeg;
    }

    return true;
}


// add a triangular facet (triplet) to the ouptut index list
bool VRML_LAYER::addTriplet( VERTEX_3D* p0, VERTEX_3D* p1, VERTEX_3D* p2 )
{
    double  dx0 = p1->x - p0->x;
    double  dx1 = p2->x - p0->x;

    double  dy0 = p1->y - p0->y;
    double  dy1 = p2->y - p0->y;

    // this number is chosen because we shall only write 6 decimal places
    // on the VRML output
    double err = 0.000001;

    // test if the triangles are degenerate (parallel sides)

    if( dx0 < err && dx0 > -err && dx1 < err && dx1 > -err )
        return false;

    if( dy0 < err && dy0 > -err && dy1 < err && dy1 > -err )
        return false;

    double  sl0 = dy0 / dx0;
    double  sl1 = dy1 / dx1;

    double dsl = sl1 - sl0;

    if( dsl < err && dsl > -err )
        return false;

    triplets.push_back( TRIPLET_3D( p0->o, p1->o, p2->o ) );

    return true;
}


// add an extra vertex (to be called only by the COMBINE callback)
VERTEX_3D* VRML_LAYER::AddExtraVertex( double x, double y )
{
    VERTEX_3D* vertex = new VERTEX_3D;

    if( !vertex )
    {
        error = "AddExtraVertex(): could not allocate a new vertex";
        return NULL;
    }

    if( eidx == 0 )
        eidx = idx + hidx;

    vertex->x   = x;
    vertex->y   = y;
    vertex->i   = eidx++;
    vertex->o   = -1;

    extra_verts.push_back( vertex );

    return vertex;
}


// start a GL command list
void VRML_LAYER::glStart( GLenum cmd )
{
    glcmd = cmd;

    while( !vlist.empty() )
        vlist.pop_back();
}


// process a vertex
void VRML_LAYER::glPushVertex( VERTEX_3D* vertex )
{
    if( vertex->o < 0 )
    {
        vertex->o = ord++;
        ordmap.push_back( vertex->i );
    }

    vlist.push_back( vertex );
}


// end a GL command list
void VRML_LAYER::glEnd( void )
{
    switch( glcmd )
    {
    case GL_LINE_LOOP:
        {
            // add the loop to the list of outlines
            std::list<int>* loop = new std::list<int>;

            if( !loop )
                break;

            for( unsigned int i = 0; i < vlist.size(); ++i )
            {
                loop->push_back( vlist[i]->o );
            }

            outline.push_back( loop );
        }
        break;

    case GL_TRIANGLE_FAN:
        processFan();
        break;

    case GL_TRIANGLE_STRIP:
        processStrip();
        break;

    case GL_TRIANGLES:
        processTri();
        break;

    default:
        break;
    }

    while( !vlist.empty() )
        vlist.pop_back();

    glcmd = 0;
}


// set the error message
void VRML_LAYER::SetGLError( GLenum errorID )
{
    error = "";
    error = (const char*)gluGetString( errorID );

    if( error.empty() )
    {
        std::ostringstream ostr;
        ostr << "Unknown OpenGL error: " << errorID;
        error = ostr.str();
    }
}


// process a GL_TRIANGLE_FAN list
void VRML_LAYER::processFan( void )
{
    if( vlist.size() < 3 )
        return;

    VERTEX_3D* p0 = vlist[0];

    int i;
    int end = vlist.size();

    for( i = 2; i < end; ++i )
    {
        addTriplet( p0, vlist[i - 1], vlist[i] );
    }
}


// process a GL_TRIANGLE_STRIP list
void VRML_LAYER::processStrip( void )
{
    // note: (source: http://www.opengl.org/wiki/Primitive)
    // GL_TRIANGLE_STRIP​: Every group of 3 adjacent vertices forms a triangle.
    // The face direction of the strip is determined by the winding of the
    // first triangle. Each successive triangle will have its effective face
    // order reverse, so the system compensates for that by testing it in the
    // opposite way. A vertex stream of n length will generate n-2 triangles.

    if( vlist.size() < 3 )
        return;

    int i;
    int end = vlist.size();
    bool flip = false;

    for( i = 2; i < end; ++i )
    {
        if( flip )
        {
            addTriplet( vlist[i - 1], vlist[i - 2], vlist[i] );
            flip = false;
        }
        else
        {
            addTriplet( vlist[i - 2], vlist[i - 1], vlist[i] );
            flip = true;
        }
    }
}


// process a GL_TRIANGLES list
void VRML_LAYER::processTri( void )
{
    // notes:
    // 1. each successive group of 3 vertices is a triangle
    // 2. as per OpenGL specification, any incomplete triangles are to be ignored

    if( vlist.size() < 3 )
        return;

    int i;
    int end = vlist.size();

    for( i = 2; i < end; i += 3 )
        addTriplet( vlist[i - 2], vlist[i - 1], vlist[i] );
}


// push the internally held vertices
void VRML_LAYER::pushVertices( bool holes )
{
    // push the internally held vertices
    unsigned int i;

    std::list<int>::const_iterator  begin;
    std::list<int>::const_iterator  end;
    GLdouble pt[3];
    VERTEX_3D* vp;

    for( i = 0; i < contours.size(); ++i )
    {
        if( contours[i]->size() < 3 )
            continue;

        if( ( holes && areas[i] <= 0.0 ) || ( !holes && areas[i] > 0.0 ) )
            continue;

        gluTessBeginContour( tess );

        begin = contours[i]->begin();
        end = contours[i]->end();

        while( begin != end )
        {
            vp = vertices[ *begin ];
            pt[0]   = vp->x;
            pt[1]   = vp->y;
            pt[2]   = 0.0;
            gluTessVertex( tess, pt, vp );
            ++begin;
        }

        gluTessEndContour( tess );
    }
}


VERTEX_3D* VRML_LAYER::getVertexByIndex( int index, VRML_LAYER* holes )
{
    if( index < 0 || (unsigned int) index >= ( idx + hidx + extra_verts.size() ) )
    {
        error = "getVertexByIndex():BUG: invalid index";
        return NULL;
    }

    if( index < idx )
    {
        // vertex is in the vertices[] list
        return vertices[ index ];
    }
    else if( index >= idx + hidx )
    {
        // vertex is in the extra_verts[] list
        return extra_verts[index - idx - hidx];
    }

    // vertex is in the holes object
    if( !holes )
    {
        error = "getVertexByIndex():BUG: invalid index";
        return NULL;
    }

    VERTEX_3D* vp = holes->GetVertexByIndex( index );

    if( !vp )
    {
        std::ostringstream ostr;
        ostr << "getVertexByIndex():FAILED: " << holes->GetError();
        error = ostr.str();
        return NULL;
    }

    return vp;
}


// retrieve the total number of vertices
int VRML_LAYER::GetSize( void )
{
    return vertices.size();
}


// Inserts all contours into the given tesselator; this results in the
// renumbering of all vertices from 'start'. Returns the end number.
// Take care when using this call since tesselators cannot work on
// the internal data concurrently
int VRML_LAYER::Import( int start, GLUtesselator* tess )
{
    if( start < 0 )
    {
        error = "Import(): invalid index ( start < 0 )";
        return -1;
    }

    if( !tess )
    {
        error = "Import(): NULL tesselator pointer";
        return -1;
    }

    unsigned int i, j;

    // renumber from 'start'
    for( i = 0, j = vertices.size(); i < j; ++i )
    {
        vertices[i]->i = start++;
        vertices[i]->o = -1;
    }

    // push each contour to the tesselator
    VERTEX_3D* vp;
    GLdouble pt[3];

    std::list<int>::const_iterator cbeg;
    std::list<int>::const_iterator cend;

    for( i = 0; i < contours.size(); ++i )
    {
        if( contours[i]->size() < 3 )
            continue;

        cbeg = contours[i]->begin();
        cend = contours[i]->end();

        gluTessBeginContour( tess );

        while( cbeg != cend )
        {
            vp = vertices[ *cbeg++ ];
            pt[0] = vp->x;
            pt[1] = vp->y;
            pt[2] = 0.0;
            gluTessVertex( tess, pt, vp );
        }

        gluTessEndContour( tess );
    }

    return start;
}


// return the vertex identified by index
VERTEX_3D* VRML_LAYER::GetVertexByIndex( int index )
{
    int i0 = vertices[0]->i;

    if( index < i0 || index >= ( i0 + (int) vertices.size() ) )
    {
        error = "GetVertexByIndex(): invalid index";
        return NULL;
    }

    return vertices[index - i0];
}


// return the error string
const std::string& VRML_LAYER::GetError( void )
{
    return error;
}
