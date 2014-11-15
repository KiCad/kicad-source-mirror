/*
 * file: vrml_layer.h
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
 *  @file vrml_layer.h
 */

/*
 *  Classes and structures to support the tesselation of a
 *  PCB for VRML output.
 */

#ifndef VRML_LAYER_H
#define VRML_LAYER_H


#include <wx/glcanvas.h>    // CALLBACK definition, needed on Windows
                            // alse needed on OSX to define __DARWIN__

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#include <fstream>
#include <vector>
#include <list>
#include <utility>

#ifndef M_PI2
#define M_PI2 ( M_PI / 2.0 )
#endif

#ifndef M_PI4
#define M_PI4 ( M_PI / 4.0 )
#endif


struct VERTEX_3D
{
    double  x;
    double  y;
    int i;          // vertex index
    int o;          // vertex order
    bool pth;       // true for plate-through hole
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
    // Arc parameters
    int    maxArcSeg;                       // maximum number of arc segments in a small circle
    double minSegLength;                    // min. segment length
    double maxSegLength;                    // max. segment length

    // Vertex offsets to work around a suspected GLU tesselator bug
    double offsetX;
    double offsetY;

    bool    fix;                            // when true, no more vertices may be added by the user
    int     idx;                            // vertex index (number of contained vertices)
    int     ord;                            // vertex order (number of ordered vertices)
    unsigned int idxout;                    // outline index to first point in 3D outline
    std::vector<VERTEX_3D*> vertices;       // vertices of all contours
    std::vector<std::list<int>*> contours;  // lists of vertices for each contour
    std::vector<bool>pth;                   // indicates whether a 'contour' is a PTH or not
    std::vector<bool>solid;                 // indicates whether a 'contour' is a solid or a hole
    std::vector< double > areas;            // area of the contours (positive if winding is CCW)
    std::list<TRIPLET_3D> triplets;         // output facet triplet list (triplet of ORDER values)
    std::list<std::list<int>*> outline;     // indices for outline outputs (index by ORDER values)
    std::vector<int> ordmap;                // mapping of ORDER to INDEX

    std::string error;                      // error message

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
    VERTEX_3D* getVertexByIndex( int aPointIndex, VRML_LAYER* holes );

    void    processFan( void );                 // process a GL_TRIANGLE_FAN list
    void    processStrip( void );               // process a GL_TRIANGLE_STRIP list
    void    processTri( void );                 // process a GL_TRIANGLES list

    void pushVertices( bool holes );            // push the internal vertices
    bool pushOutline( VRML_LAYER* holes );      // push the outline vertices

    // calculate number of sides on an arc (angle is in radians)
    int calcNSides( double aRadius, double aAngle );

    // returns the number of solid or hole contours
    int checkNContours( bool holes );

public:
    /// set to true when a fault is encountered during tesselation
    bool Fault;

    VRML_LAYER();
    virtual ~VRML_LAYER();

    /**
     * Function GetArcParams
     * retieves the parameters used in calculating the number of vertices in an arc
     *
     * @param aMaxSeg is the maximum number of segments for an arc with cords of length aMinLength
     * @param aMinLength is the minimum length of cords in an arc
     * @param aMaxLength is the maximum length of cords in an arc
     */
    void GetArcParams( int& aMaxSeg, double& aMinLength, double& aMaxLength );

    /**
     * Function SetArcParams
     * sets the parameters used in calculating the number of vertices in an arc.
     * The default settings are reasonable for rendering for unit lengths of 1mm
     *
     * @param aMaxSeg is the maximum number of segments for an arc with cords of length aMinLength
     * @param aMinLength is the minimum length of cords in an arc
     * @param aMaxLength is the maximum length of cords in an arc
     *
     * @return bool: true if the parameters were accepted
     */
    bool SetArcParams( int aMaxSeg, double aMinLength, double aMaxLength );

    /**
     * Function Clear
     * erases all data except for arc parameters.
     */
    void Clear( void );

    /**
     * Function GetSize
     * returns the total number of vertices indexed
     */
    int GetSize( void );

    /**
     * Function GetNConours
     * returns the number of stored contours
     */
    int GetNContours( void )
    {
        return contours.size();
    }

    /**
     * Function NewContour
     * creates a new list of vertices and returns an index to the list
     *
     * @param aPlatedHole is true if the new contour will represent a plated hole
     *
     * @return int: index to the list or -1 if the operation failed
     */
    int NewContour( bool aPlatedHole = false );

    /**
     * Function AddVertex
     * adds a point to the requested contour
     *
     * @param aContour is an index previously returned by a call to NewContour()
     * @param aXpos is the X coordinate of the vertex
     * @param aYpos is the Y coordinate of the vertex
     *
     * @return bool: true if the vertex was added
     */
    bool AddVertex( int aContourID, double aXpos, double aYpos );

    /**
     * Function EnsureWinding
     * checks the winding of a contour and ensures that it is a hole or
     * a solid depending on the value of @param hole
     *
     * @param aContour is an index to a contour as returned by NewContour()
     * @param aHoleFlag determines if the contour must be a hole
     *
     * @return bool: true if the operation suceeded
     */
    bool EnsureWinding( int aContourID, bool aHoleFlag );

    /**
     * Function AppendCircle
     * adds a circular contour to the specified (empty) contour
     *
     * @param aXpos is the X coordinate of the hole center
     * @param aYpos is the Y coordinate of the hole center
     * @param aRadius is the radius of the hole
     * @param aContourID is the contour index
     * @param aHoleFlag determines if the contour to be created is a cutout
     *
     * @return bool: true if the new contour was successfully created
     */
    bool AppendCircle( double aXpos, double aYpos, double aRadius,
                       int aContourID, bool aHoleFlag = false );

    /**
     * Function AddCircle
     * creates a circular contour and adds it to the internal list
     *
     * @param aXpos  is the X coordinate of the hole center
     * @param aYpos is the Y coordinate of the hole center
     * @param aRadius is the radius of the hole
     * @param aHoleFlag determines if the contour to be created is a cutout
     * @param aPlatedHole is true if this is a plated hole
     *
     * @return bool: true if the new contour was successfully created
     */
    bool AddCircle( double aXpos, double aYpos, double aRadius,
                    bool aHoleFlag = false, bool aPlatedHole = false );

    /**
     * Function AddSlot
     * creates and adds a slot feature to the list of contours
     *
     * @param aCenterX is the X coordinate of the slot's center
     * @param aCenterY is the Y coordinate of the slot's center
     * @param aSlotLength is the length of the slot along the major axis
     * @param aSlotWidth is the width of the slot along the minor axis
     * @param aAngle (degrees) is the orientation of the slot
     * @param aHoleFlag determines whether the slot is a hole or a solid
     * @param aPlatedHole is true if this is a plated slot
     *
     * @return bool: true if the slot was successfully created
     */
    bool AddSlot( double aCenterX, double aCenterY, double aSlotLength, double aSlotWidth,
                  double aAngle, bool aHoleFlag = false, bool aPlatedHole = false );

    /**
     * Function AppendArc
     * adds an arc to the specified contour
     *
     * @param aCenterX is the X coordinate of the arc's center
     * @param aCenterY is the Y coordinate of the arc's center
     * @param aRadius is the radius of the arc
     * @param aStartAngle (degrees) is the starting angle of the arc
     * @param aAngle (degrees) is the measure of the arc
     * @param aContourID is the contour's index
     *
     * @return bool: true if the slot was successfully created
     */
    bool AppendArc( double aCenterX, double aCenterY, double aRadius,
                    double aStartAngle, double aAngle, int aContourID );

    /**
     * Function AddArc
     * creates a slotted arc and adds it to the internal list of contours
     *
     * @param aCenterX is the X coordinate of the arc's center
     * @param aCenterY is the Y coordinate of the arc's center
     * @param aStartX is the X coordinate of the starting point
     * @param aStartY is the Y coordinate of the starting point
     * @param aArcWidth is the width of the arc
     * @param aAngle is the included angle (degrees)
     * @param aHoleFlag determines whether the arc is to be a hole or a solid
     * @param aPlatedHole is true if this is a plated slotted arc
     *
     * @return bool: true if the feature was successfully created
     */
    bool AddArc( double aCenterX, double aCenterY,
                 double aStartX, double aStartY,
                 double aArcWidth, double aAngle,
                 bool aHoleFlag = false, bool aPlatedHole = false );

    /**
     * Function Tesselate
     * creates a list of outline vertices as well as the
     * vertex sets required to render the surface.
     *
     * @param holes  is an optional pointer to cutouts to be imposed on the
     *               surface.
     * @param aHolesOnly is true if the outline contains only holes
     *
     * @return bool: true if the operation succeeded
     */
    bool Tesselate( VRML_LAYER* holes = NULL, bool aHolesOnly = false );

    /**
     * Function WriteVertices
     * writes out the list of vertices required to render a
     * planar surface.
     *
     * @param aZcoord  is the Z coordinate of the plane
     * @param aOutFile is the file to write to
     * @param aPrecision is the precision of the output coordinates
     *
     * @return bool: true if the operation succeeded
     */
    bool WriteVertices( double aZcoord, std::ofstream& aOutFile, int aPrecision );

    /**
     * Function Write3DVertices
     * writes out the list of vertices required to render an extruded solid
     *
     * @param aTopZ is the Z coordinate of the top plane
     * @param aBottomZ is the Z coordinate of the bottom plane
     * @param aOutFile is the file to write to
     * @param aPrecision is the precision of the output coordinates
     *
     * @return bool: true if the operation succeeded
     */
    bool Write3DVertices( double aTopZ, double aBottomZ, std::ofstream& aOutFile, int aPrecision );

    /**
     * Function WriteIndices
     * writes out the vertex sets required to render a planar
     * surface.
     *
     * @param aTopFlag is true if the surface is to be visible from above;
     * if false the surface will be visible from below.
     * @param aOutFile is the file to write to
     *
     * @return bool: true if the operation succeeded
     */
    bool WriteIndices( bool aTopFlag, std::ofstream& aOutFile );

    /**
     * Function Write3DIndices
     * writes out the vertex sets required to render an extruded solid
     *
     * @param aOutFile is the file to write to
     * @param aIncludePlatedHoles is true if holes marked as plated should
     *        be rendered. Default is false since the user will usually
     *        render these holes in a different color
     *
     * @return bool: true if the operation succeeded
     */
    bool Write3DIndices( std::ofstream& aOutFile, bool aIncludePlatedHoles = false );

    /**
     * Function AddExtraVertex
     * adds an extra vertex as required by the GLU tesselator.
     *
     * @param aXpos is the X coordinate of the newly created point
     * @param aYpos is the Y coordinate of the newly created point
     * @param aPlatedHole is true if this point is part of a plated hole
     *
     * @return VERTEX_3D*: is the new vertex or NULL if a vertex
     * could not be created.
     */
    VERTEX_3D* AddExtraVertex( double aXpos, double aYpos, bool aPlatedHole );

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
     * @param aPointIndex is a vertex index
     *
     * @return VERTEX_3D*: the requested vertex or NULL
     */
    VERTEX_3D* GetVertexByIndex( int aPointIndex );

    /*
     * Function GetError
     * Returns the error message related to the last failed operation
     */
    const std::string& GetError( void );

    void SetVertexOffsets( double aXoffset, double aYoffset );
};

#endif    // VRML_LAYER_H
