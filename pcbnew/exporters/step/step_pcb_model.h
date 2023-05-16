/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef OCE_VIS_OCE_UTILS_H
#define OCE_VIS_OCE_UTILS_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <BRepBuilderAPI_MakeWire.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>

#include <math/vector2d.h>
#include <math/vector3.h>
#include <geometry/shape_poly_set.h>

/**
 * Default distance between points to treat them as separate ones (mm)
 * 0.001 mm or less is a reasonable value. A too large value creates issues by
 * merging points that should be different.
 * Remember we are a 3D space, so a thin shape can be broken if 2 points
 * are merged (in X, Y, Z coords) when they should not.
 * round shapes converted to polygon can also be not good with a to large value
 */
static constexpr double OCC_MAX_DISTANCE_TO_MERGE_POINTS = 0.001;

// default PCB thickness in mm
static constexpr double BOARD_THICKNESS_DEFAULT_MM = 1.6;

// minimum PCB thickness in mm (10 microns assumes a very thin polyimide film)
// must be > OCC_MAX_DISTANCE_TO_MERGE_POINTS
static constexpr double BOARD_THICKNESS_MIN_MM = 0.01;

// default copper thickness in mm
// must be > OCC_MAX_DISTANCE_TO_MERGE_POINTS
static constexpr double COPPER_THICKNESS_DEFAULT_MM = 0.035;

// Max error to approximate an arc by segments (in mm)
static constexpr double ARC_TO_SEGMENT_MAX_ERROR_MM = 0.005;

class PAD;

typedef std::pair< std::string, TDF_Label > MODEL_DATUM;
typedef std::map< std::string, TDF_Label > MODEL_MAP;

extern void ReportMessage( const wxString& aMessage );

class STEP_PCB_MODEL
{
public:
    STEP_PCB_MODEL( const wxString& aPcbName );
    virtual ~STEP_PCB_MODEL();

    // add a pad hole or slot (must be in final position)
    bool AddPadHole( const PAD* aPad, const VECTOR2D& aOrigin );

    // add a pad/via shape (must be in final position)
    bool AddPadShape( const PAD* aPad, const VECTOR2D& aOrigin );

    // add a set of polygons (must be in final position) on top or bottom of the board as copper
    bool AddCopperPolygonShapes( const SHAPE_POLY_SET* aPolyShapes, bool aOnTop, const VECTOR2D& aOrigin );

    // add a component at the given position and orientation
    bool AddComponent( const std::string& aFileName, const std::string& aRefDes, bool aBottom,
                       VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                       VECTOR3D aOrientation, VECTOR3D aScale, bool aSubstituteModels = true );

    void SetBoardColor( double r, double g, double b );
    void SetCopperColor( double r, double g, double b );

    // set the thickness of the PCB (mm); the top of the PCB shall be at Z = aThickness
    // aThickness < 0.0 == use default thickness
    // aThickness <= THICKNESS_MIN == use THICKNESS_MIN
    // aThickness > THICKNESS_MIN == use aThickness
    void SetPCBThickness( double aThickness );

    // Set the max distance (in mm) to consider 2 points have the same coordinates
    // and can be merged
    void OCCSetMergeMaxDistance( double aDistance = OCC_MAX_DISTANCE_TO_MERGE_POINTS );

    void SetMaxError( int aMaxError ) { m_maxError = aMaxError; }

    // create the PCB model using the current outlines and drill holes
    bool CreatePCB( SHAPE_POLY_SET& aOutline, VECTOR2D aOrigin );

    /**
     * Convert a SHAPE_LINE_CHAIN (closed) to a TopoDS_Shape (polygonal vertical prism)
     * @param aShape is the TopoDS_Shape to initialize (must be empty)
     * @param aChain is a closed SHAPE_LINE_CHAIN (a polygon)
     * @param aThickness is the height of the created prism
     * @param aOrigin is the origin of the coordinates
     * @return true if success
     */
    bool MakeShape( TopoDS_Shape& aShape, const SHAPE_LINE_CHAIN& aChain, double aThickness,
                    double aZposition, const VECTOR2D& aOrigin );

    /**
     * Convert a SHAPE_LINE_CHAIN containing only one 360 deg arc to a TopoDS_Shape
     * ( vertical cylinder)
     * it is a specialized version of MakeShape()
     * @param aShape is the TopoDS_Shape to initialize (must be empty)
     * @param aChain is a closed SHAPE_LINE_CHAIN, image of a circle: containing one 360 deg arc
     * @param aThickness is the height of the created cylinder
     * @param aOrigin is the origin of the coordinates
     * @return true if success
     */
    bool MakeShapeAsCylinder( TopoDS_Shape& aShape, const SHAPE_LINE_CHAIN& aChain,
                              double aThickness, double aZposition, const VECTOR2D& aOrigin );

#ifdef SUPPORTS_IGES
    // write the assembly model in IGES format
    bool WriteIGES( const wxString& aFileName );
#endif

    // write the assembly model in STEP format
    bool WriteSTEP( const wxString& aFileName );

private:
    /**
     * @return true if the board(s) outline is valid. False otherwise
     */
    bool isBoardOutlineValid();

    /** create one solid board using current outline and drill holes set
     * @param aIdx is the main outline index
     * @param aOutline is the set of outlines with holes
     * @param aOrigin is the coordinate origin for 3 view
     */
    bool createOneBoard( int aIdx, SHAPE_POLY_SET& aOutline, VECTOR2D aOrigin );

    /**
     * Load a 3D model data.
     *
     * @param aFileNameUTF8 is the filename encoded UTF8 (different formats allowed)
     * but for WRML files a model data can be loaded instead of the vrml data,
     * not suitable in a step file.
     * @param aScale is the X,Y,Z scaling factors.
     * @param aLabel is the TDF_Label to store the data.
     * @param aSubstituteModels = true to allows data substitution, false to disallow.
     * @param aErrorMessage (can be nullptr) is an error message to be displayed on error.
     * @return true if successfully loaded, false on error.
     */
    bool getModelLabel( const std::string& aFileNameUTF8, VECTOR3D aScale, TDF_Label& aLabel,
                        bool aSubstituteModels, wxString* aErrorMessage = nullptr );

    bool getModelLocation( bool aBottom, VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                           VECTOR3D aOrientation, TopLoc_Location& aLocation );

    bool readIGES( Handle( TDocStd_Document )& m_doc, const char* fname );
    bool readSTEP( Handle( TDocStd_Document )& m_doc, const char* fname );

    TDF_Label transferModel( Handle( TDocStd_Document )& source, Handle( TDocStd_Document ) & dest,
                             VECTOR3D aScale );

    Handle( XCAFApp_Application )   m_app;
    Handle( TDocStd_Document )      m_doc;
    Handle( XCAFDoc_ShapeTool )     m_assy;
    TDF_Label                       m_assy_label;
    bool                            m_hasPCB;           // set true if CreatePCB() has been invoked
    std::vector<TDF_Label>          m_pcb_labels;       // labels for the PCB model (one by main outline)
    MODEL_MAP                       m_models;           // map of file names to model labels
    int                             m_components;       // number of successfully loaded components;
    double                          m_precision;        // model (length unit) numeric precision
    double                          m_angleprec;        // angle numeric precision
    double                          m_boardColor[3];    // board body, RGB values
    double                          m_copperColor[3];   // copper, RGB values
    double                          m_boardThickness;   // PCB thickness, mm
    double                          m_copperThickness;  // copper thickness, mm

    double                          m_minx;             // leftmost curve point
    double                          m_mergeOCCMaxDist;  // minimum distance (mm) below which two
                                                        // points are considered coincident by OCC

    // Holes in main outlines (more than one board)
    std::vector<TopoDS_Shape>       m_cutouts;

    // Main outlines (more than one board)
    std::vector<TopoDS_Shape>       m_board_outlines;

    /// Name of the PCB, which will most likely be the file name of the path.
    wxString                        m_pcbName;

    int                             m_maxError;
};

#endif // OCE_VIS_OCE_UTILS_H
