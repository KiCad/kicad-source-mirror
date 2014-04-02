/*
 * file: vrml_board.h
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

/**
 *  @file vrml_board.h
 */

/*
 *  Classes and structures to support the tesselation of a
 *  PCB for VRML output.
 */

#ifndef VRML_BOARD_H
#define VRML_BOARD_H

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#include <cstdio>
#include <vector>
#include <list>
#include <utility>

#ifndef M_PI2
#define M_PI2 ( M_PI / 2.0 )
#endif

#ifndef M_PI4
#define M_PI4 ( M_PI / 4.0 )
#endif

class GLUtesselator;

struct VERTEX_3D
{
    double  x;
    double  y;
    int i;          // vertex index
    int o;          // vertex order
};

struct TRIPLET_3D
{
    int i1, i2, i3;

    TRIPLET_3D( int p1, int p2, int p3 )
    {
        i1  = p1;
        i2  = p2;
        i3  = p3;
    }
};


class VRML_LAYER
{
private:
    bool    fix;                            // when true, no more vertices may be added by the user
    int     idx;                            // vertex index (number of contained vertices)
    int     ord;                            // vertex order (number of ordered vertices)
    std::vector<VERTEX_3D*> vertices;       // vertices of all contours
    std::vector<std::list<int>*> contours;  // lists of vertices for each contour
    std::vector< double > areas;            // area of the contours (positive if winding is CCW)
    std::list<TRIPLET_3D> triplets;         // output facet triplet list (triplet of ORDER values)
    std::list<std::list<int>*> outline;     // indices for outline outputs (index by ORDER values)
    std::vector<int> ordmap;                // mapping of ORDER to INDEX

    std::string error;                      // error message

    double maxdev;                          // max. deviation from circle when calculating N sides

    int hidx;                               // number of vertices in the holes
    int eidx;                               // index for extra vertices
    std::vector<VERTEX_3D*> extra_verts;    // extra vertices added for outlines and facets
    std::vector<VERTEX_3D*> vlist;          // vertex list for the GL command in progress
    VRML_LAYER* pholes;                     // pointer to another layer object used for tesselation;
                                            // this object is normally expected to hold only holes

    GLUtesselator* tess;                    // local instance of the GLU tesselator

    GLenum glcmd;                           // current GL command type ( fan, triangle, tri-strip, loop )

    void clearTmp( void );                  // clear ephemeral data used by the tesselation routine

    // add a triangular facet (triplet) to the output index list
    bool addTriplet( VERTEX_3D* p0, VERTEX_3D* p1, VERTEX_3D* p2 );

    // retrieve a vertex given its index; the vertex may be contained in the
    // vertices vector, extra_verts vector, or foreign VRML_LAYER object
    VERTEX_3D* getVertexByIndex( int index, VRML_LAYER* holes );

    void    processFan( void );                 // process a GL_TRIANGLE_FAN list
    void    processStrip( void );               // process a GL_TRIANGLE_STRIP list
    void    processTri( void );                 // process a GL_TRIANGLES list

    void pushVertices( bool holes );            // push the internal vertices
    bool pushOutline( VRML_LAYER* holes );      // push the outline vertices

public:
    /// set to true when a fault is encountered during tesselation
    bool Fault;

    VRML_LAYER();
    virtual ~VRML_LAYER();

    /**
     * Function Clear
     * erases all data.
     */
    void Clear( void );

    /**
     * Function GetSize
     * returns the total number of vertices indexed
     */
    int GetSize( void );

    /**
     * Function SetMaxDev
     * sets the maximum deviation from a circle; this parameter is
     * used for the automatic calculation of segments within a
     * circle or an arc.
     *
     * @param max is the maximum deviation from a perfect circle or arc;
     * minimum value is 0.000002 units
     *
     * @return bool: true if the value was accepted
     */
    bool SetMaxDev( double max );

    /**
     * Function NewContour
     * creates a new list of vertices and returns an index to the list
     *
     * @return int: index to the list or -1 if the operation failed
     */
    int NewContour( void );

    /**
     * Function AddVertex
     * adds a point to the requested contour
     *
     * @param aContour is an index previously returned by a call to NewContour()
     * @param x is the X coordinate of the vertex
     * @param y is the Y coordinate of the vertex
     *
     * @return bool: true if the vertex was added
     */
    bool AddVertex( int aContour, double x, double y );

    /**
     * Function EnsureWinding
     * checks the winding of a contour and ensures that it is a hole or
     * a solid depending on the value of @param hole
     *
     * @param aContour is an index to a contour as returned by NewContour()
     * @param hole determines if the contour must be a hole
     *
     * @return bool: true if the operation suceeded
     */
    bool EnsureWinding( int aContour, bool hole );

    /**
     * Function AddCircle
     * creates a circular contour and adds it to the internal list
     *
     * @param x is the X coordinate of the hole center
     * @param y is the Y coordinate of the hole center
     * @param rad is the radius of the hole
     * @param csides is the number of sides (segments) in a circle;
     *      use a value of 1 to automatically calculate a suitable number.
     * @param hole determines if the contour to be created is a cutout
     *
     * @return bool: true if the new contour was successfully created
     */
    bool AddCircle( double x, double y, double rad, int csides, bool hole = false );

    /**
     * Function AddSlot
     * creates and adds a slot feature to the list of contours
     *
     * @param cx is the X coordinate of the slot
     * @param cy is the Y coordinate of the slot
     * @param length is the length of the slot along the major axis
     * @param width is the width of the slot along the minor axis
     * @param angle (radians) is the orientation of the slot
     * @param csides is the number of sides to a circle; use 1 to
     *  take advantage of automatic calculations.
     * @param hole determines whether the slot is a hole or a solid
     *
     * @return bool: true if the slot was successfully created
     */
    bool AddSlot( double cx, double cy, double length, double width,
            double angle, int csides, bool hole = false );

    /**
     * Function AddArc
     * creates an arc and adds it to the internal list of contours
     *
     * @param cx is the X coordinate of the arc's center
     * @param cy is the Y coordinate of the arc's center
     * @param startx is the X coordinate of the starting point
     * @param starty is the Y coordinate of the starting point
     * @param width is the width of the arc
     * @param angle is the included angle
     * @param csides is the number of segments in a circle; use 1
     *  to take advantage of automatic calculations of this number
     * @param hole determined whether the arc is to be a hole or a solid
     *
     * @return bool: true if the feature was successfully created
     */
    bool AddArc( double cx, double cy, double startx, double starty,
            double width, double angle, int csides, bool hole = false );


    /**
     * Function Tesselate
     * creates a list of outline vertices as well as the
     * vertex sets required to render the surface.
     *
     * @param holes  is a pointer to cutouts to be imposed on the
     * surface.
     *
     * @return bool: true if the operation succeeded
     */
    bool Tesselate( VRML_LAYER* holes );

    /**
     * Function WriteVertices
     * writes out the list of vertices required to render a
     * planar surface.
     *
     * @param z is the Z coordinate of the plane
     * @param fp is the file to write to
     *
     * @return bool: true if the operation succeeded
     */
    bool WriteVertices( double z, FILE* fp );

    /**
     * Function Write3DVertices
     * writes out the list of vertices required to render an extruded solid
     *
     * @param top is the Z coordinate of the top plane
     * @param bottom is the Z coordinate of the bottom plane
     * @param fp is the file to write to
     *
     * @return bool: true if the operation succeeded
     */
    bool Write3DVertices( double top, double bottom, FILE* fp );

    /**
     * Function WriteIndices
     * writes out the vertex sets required to render a planar
     * surface.
     *
     * @param top is true if the surface is to be visible from above;
     * if false the surface will be visible from below.
     * @param fp is the file to write to
     *
     * @return bool: true if the operation succeeded
     */
    bool WriteIndices( bool top, FILE* fp );

    /**
     * Function Write3DIndices
     * writes out the vertex sets required to render an extruded solid
     *
     * @param fp is the file to write to
     *
     * @return bool: true if the operation succeeded
     */
    bool Write3DIndices( FILE* fp );

    /**
     * Function AddExtraVertex
     * adds an extra vertex as required by the GLU tesselator
     *
     * @return VERTEX_3D*: is the new vertex or NULL if a vertex
     * could not be created.
     */
    VERTEX_3D* AddExtraVertex( double x, double y );

    /**
     * Function glStart
     * is invoked by the GLU tesselator callback to notify this object
     * of the type of GL command which is applicable to the upcoming
     * vertex list.
     *
     * @param cmd is the GL command
     */
    void glStart( GLenum cmd );

    /**
     * Function glPushVertex
     * is invoked by the GLU tesselator callback; the supplied vertex is
     * added to the internal list of vertices awaiting processing upon
     * execution of glEnd()
     *
     * @param vertex is a vertex forming part of the GL command as previously
     * set by glStart
     */
    void glPushVertex( VERTEX_3D* vertex );

    /**
     * Function glEnd
     * is invoked by the GLU tesselator callback to notify this object
     * that the vertex list is complete and ready for processing
     */
    void glEnd( void );

    /**
     * Function SetGLError
     * sets the error message according to the specified OpenGL error
     */
    void SetGLError( GLenum error_id );

    /**
     * Function Import
     * inserts all contours into the given tesselator; this
     * results in the renumbering of all vertices from @param start.
     * Take care when using this call since tesselators cannot work on
     * the internal data concurrently.
     *
     * @param start is the starting number for vertex indices
     * @param tess is a pointer to a GLU Tesselator object
     *
     * @return int: the number of vertices exported
     */
    int Import( int start, GLUtesselator* tess );

    /**
     * Function GetVertexByIndex
     * returns a pointer to the requested vertex or
     * NULL if no such vertex exists.
     *
     * @param ptindex is a vertex index
     *
     * @return VERTEX_3D*: the requested vertex or NULL
     */
    VERTEX_3D* GetVertexByIndex( int ptindex );

    /*
     * Function GetError
     * Returns the error message related to the last failed operation
     */
    const std::string& GetError( void );
};

#endif    // VRML_BOARD_H
