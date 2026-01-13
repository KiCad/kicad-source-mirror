/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <Standard_Handle.hxx>
#include <TDF_Label.hxx>
#include <TopoDS_Shape.hxx>

#include <math/vector2d.h>
#include <math/vector3.h>
#include <geometry/shape_poly_set.h>
#include <board_stackup_manager/board_stackup.h>

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

class TDocStd_Document;
class XCAFApp_Application;
class XCAFDoc_ColorTool;
class XCAFDoc_ShapeTool;

typedef std::pair< std::string, TDF_Label > MODEL_DATUM;
typedef std::map< std::string, TDF_Label > MODEL_MAP;


enum class OUTPUT_FORMAT
{
    FMT_OUT_UNKNOWN = 0,
    FMT_OUT_STEP,
    FMT_OUT_STEPZ,
    FMT_OUT_IGES,
    FMT_OUT_BREP,
    FMT_OUT_XAO,
    FMT_OUT_GLTF,
    FMT_OUT_PLY,
    FMT_OUT_STL,
    FMT_OUT_U3D,
    FMT_OUT_PDF
};

class STEP_PCB_MODEL
{
public:
    STEP_PCB_MODEL( const wxString& aPcbName, REPORTER* aReporter );
    virtual ~STEP_PCB_MODEL();

    // Update m_outFmt to aVariant, giving the output format variant
    void SpecializeVariant( OUTPUT_FORMAT aVariant ) { m_outFmt = aVariant; }

    // add a pad shape (must be in final position)
    // if aClipPolygon is not nullptr, the pad shape will be clipped by aClipPolygon
    // (usually aClipPolygon is the board outlines and use for castelleted pads)
    bool AddPadShape( const PAD* aPad, const VECTOR2D& aOrigin, bool aVia,
                      SHAPE_POLY_SET* aClipPolygon = nullptr );

    // add a pad hole or slot (must be in final position)
    bool AddHole( const SHAPE_SEGMENT& aShape, int aPlatingThickness, PCB_LAYER_ID aLayerTop,
                  PCB_LAYER_ID aLayerBot, bool aVia, const VECTOR2D& aOrigin, bool aCutCopper,
                  bool aCutBody );

    // add a plated hole shape (without the hole)
    bool AddBarrel( const SHAPE_SEGMENT& aShape, PCB_LAYER_ID aLayerTop, PCB_LAYER_ID aLayerBot,
                    bool aVia, const VECTOR2D& aOrigin, const wxString& aNetname );

    /**
     * Add a backdrill hole shape to remove board material and copper plating.
     *
     * A backdrill removes board material between the specified layers (inclusive), removes
     * annular rings on copper layers within that span, and removes the copper barrel plating
     * through those layers.
     *
     * @param aShape The hole shape (position and diameter of the backdrill)
     * @param aLayerStart The starting copper layer (e.g., F_Cu for top backdrill)
     * @param aLayerEnd The ending copper layer (inclusive, the layer where backdrill stops)
     * @param aOrigin The origin offset for coordinate transformation
     * @return true if successfully added
     */
    bool AddBackdrill( const SHAPE_SEGMENT& aShape, PCB_LAYER_ID aLayerStart,
                       PCB_LAYER_ID aLayerEnd, const VECTOR2D& aOrigin );

    /**
     * Add a counterbore shape to remove board material from the top or bottom of a hole.
     *
     * A counterbore creates a cylindrical recess from the specified side of the board,
     * removing board material and copper down to the specified depth.
     *
     * @param aPosition The center position of the counterbore
     * @param aDiameter The diameter of the counterbore (in IU)
     * @param aDepth The depth of the counterbore from the board surface (in IU)
     * @param aFrontSide True if counterbore is on the front (top) side, false for back (bottom)
     * @param aOrigin The origin offset for coordinate transformation
     * @return true if successfully added
     */
    bool AddCounterbore( const VECTOR2I& aPosition, int aDiameter, int aDepth,
                         bool aFrontSide, const VECTOR2D& aOrigin );

    /**
     * Add a countersink shape to remove board material from the top or bottom of a hole.
     *
     * A countersink creates an inverted cone recess from the specified side of the board.
     * The angle parameter specifies the total cone angle (the angle between opposite sides
     * of the cone), so the angle between the board surface and the cone slope is half this value.
     *
     * @param aPosition The center position of the countersink
     * @param aDiameter The diameter of the countersink at the board surface (in IU)
     * @param aDepth The depth of the countersink from the board surface (in IU)
     * @param aAngle The total cone angle in decidegrees (e.g., 900 = 90�, 820 = 82�)
     * @param aFrontSide True if countersink is on the front (top) side, false for back (bottom)
     * @param aOrigin The origin offset for coordinate transformation
     * @return true if successfully added
     */
    bool AddCountersink( const VECTOR2I& aPosition, int aDiameter, int aDepth, int aAngle,
                         bool aFrontSide, const VECTOR2D& aOrigin );

    /**
     * Get the knockout diameters for copper layers that a counterbore or countersink crosses.
     *
     * For a counterbore, the diameter is constant for all layers within the depth.
     * For a countersink, the diameter varies based on the cone angle and the Z position
     * of each layer.
     *
     * @param aDiameter The diameter at the board surface (in IU)
     * @param aDepth The depth of the feature from the board surface (in IU)
     * @param aAngle The cone angle in decidegrees (0 for counterbore, >0 for countersink)
     * @param aFrontSide True if feature is on the front (top) side, false for back (bottom)
     * @return A map of PCB_LAYER_ID to knockout diameter (in IU) for each affected copper layer
     */
    std::map<PCB_LAYER_ID, int> GetCopperLayerKnockouts( int aDiameter, int aDepth,
                                                         int aAngle, bool aFrontSide );

    // add a set of polygons (must be in final position)
    bool AddPolygonShapes( const SHAPE_POLY_SET* aPolyShapes, PCB_LAYER_ID aLayer,
                           const VECTOR2D& aOrigin, const wxString& aNetname );

    // add a component at the given position and orientation
    bool AddComponent( const wxString& aBaseName, const wxString& aFileName,
                       const std::vector<wxString>& aAltFilenames, const wxString& aRefDes,
                       bool aBottom, VECTOR2D aPosition, double aRotation, VECTOR3D aOffset,
                       VECTOR3D aOrientation, VECTOR3D aScale, bool aSubstituteModels = true );

    void SetCopperColor( double r, double g, double b );
    void SetPadColor( double r, double g, double b );

    void SetEnabledLayers( const LSET& aLayers );
    void SetFuseShapes( bool aValue );
    void SetSimplifyShapes( bool aValue );
    void SetStackup( const BOARD_STACKUP& aStackup );
    void SetNetFilter( const wxString& aFilter );
    void SetExtraPadThickness( bool aValue );

    // Set the max distance (in mm) to consider 2 points have the same coordinates
    // and can be merged
    void OCCSetMergeMaxDistance( double aDistance = OCC_MAX_DISTANCE_TO_MERGE_POINTS );

    // create the PCB model using the current outlines and drill holes
    bool CreatePCB( SHAPE_POLY_SET& aOutline, const VECTOR2D& aOrigin, bool aPushBoardBody );

    /**
     * Convert a SHAPE_POLY_SET to TopoDS_Shape's (polygonal vertical prisms, or flat faces)
     * @param aShapes is the TopoDS_Shape list to append to
     * @param aPolySet is the polygon set
     * @param aConvertToArcs set to approximate with arcs
     * @param aThickness is the height of the created prism, or 0.0: flat face pointing up, -0.0: down.
     * @param aOrigin is the origin of the coordinates
     * @return true if success
     */
    bool MakeShapes( std::vector<TopoDS_Shape>& aShapes, const SHAPE_POLY_SET& aPolySet,
                     bool aConvertToArcs, double aThickness, double aZposition, const VECTOR2D& aOrigin );

    /**
     * Make a segment shape based on start and end point. If they're too close, make a cylinder.
     * It is a specialized version of MakeShape()
     * @param aShape is the TopoDS_Shape to initialize (must be empty)
     * @param aStartPoint is the start point of the segment
     * @param aEndPoint is the end point of the segment
     * @param aWidth is the width of the segment
     * @param aThickness is the height of the created segment, or 0.0: flat face pointing up, -0.0: down.
     * @param aOrigin is the origin of the coordinates
     * @return true if success
     */
    bool MakeShapeAsThickSegment( TopoDS_Shape& aShape, const VECTOR2D& aStartPoint,
                                  const VECTOR2D& aEndPoint, double aWidth, double aThickness,
                                  double aZposition, const VECTOR2D& aOrigin );

    /**
     * Make a polygonal shape to create a vertical wall.
     * It is a specialized version of MakeShape()
     * @param aShape is the TopoDS_Shape to initialize (must be empty)
     * @param aPolySet is the outline of the wall
     * @param aHeight is the height of the wall.
     * @param aZposition is the Z postion of the wall
     * @param aOrigin is the origin of the coordinates
     * @return true if success
     */
    bool MakePolygonAsWall( TopoDS_Shape& aShape, SHAPE_POLY_SET& aPolySet, double aHeight,
                            double aZposition, const VECTOR2D& aOrigin );

#ifdef SUPPORTS_IGES
    // write the assembly model in IGES format
    bool WriteIGES( const wxString& aFileName );
#endif

    // write the assembly model in STEP format
    bool WriteSTEP( const wxString& aFileName, bool aOptimize, bool compress );

    // write the assembly in BREP format
    bool WriteBREP( const wxString& aFileName );

    // write the assembly in XAO format with pad faces as groups
    bool WriteXAO( const wxString& aFileName );

    /**
     * Write the assembly in binary GLTF Format
     *
     * We only support binary GLTF because GLTF is weird
     * Officially, binary GLTF is actually json+binary in one file
     * If we elected non-binary output with opecascade, it will generate
     * that one file as two separate files, one containing json that references the binary
     * Which is actually more annoying to deal with (to do the temp file rename, since we dont
     * control the binary name) and silly when you can just have the one file.
     *
     * @param aFileName Output file path
     *
     * @return true if the write succeeded without error
     */
    bool WriteGLTF( const wxString& aFileName );

    // write the assembly in PLY format (mesh)
    bool WritePLY( const wxString& aFileName );

    // write the assembly in STL format (mesh)
    bool WriteSTL( const wxString& aFileName );

    // write the assembly in U3D format (mesh)
    bool WriteU3D( const wxString& aFileName );

    // write the assembly in U3D format (mesh)
    bool WritePDF( const wxString& aFileName );
private:
    /**
     * @return true if the board(s) outline is valid. False otherwise
     */
    bool isBoardOutlineValid();

    void getLayerZPlacement( PCB_LAYER_ID aLayer, double& aZPos, double& aThickness );

    void getCopperLayerZPlacement( PCB_LAYER_ID aLayer, double& aZPos, double& aThickness );

    void getBoardBodyZPlacement( double& aZPos, double& aThickness );

    /**
     * Load a 3D model data.
     *
     * @param aBaseName is the model name to set.
     * @param aFileName is the filename (different formats allowed)
     * but for WRML files a model data can be loaded instead of the vrml data,
     * not suitable in a step file.
     * @param aAltFilenames provides additional filenames for WRL substitution.
     * @param aScale is the X,Y,Z scaling factors.
     * @param aLabel is the TDF_Label to store the data.
     * @param aSubstituteModels = true to allows data substitution, false to disallow.
     * @param aErrorMessage (can be nullptr) is an error message to be displayed on error.
     * @return true if successfully loaded, false on error.
     */
    bool getModelLabel( const wxString& aBaseName, const wxString& aFileName,
                        const std::vector<wxString>& aAltFilenames, VECTOR3D aScale,
                        TDF_Label& aLabel, bool aSubstituteModels,
                        wxString* aErrorMessage = nullptr );

    bool getModelLocation( bool aBottom, const VECTOR2D& aPosition, double aRotation, const VECTOR3D& aOffset,
                           const VECTOR3D& aOrientation, TopLoc_Location& aLocation );

    bool readIGES( Handle( TDocStd_Document ) & aDoc, const char* aFname );
    bool readSTEP( Handle( TDocStd_Document ) & aDoc, const char* aFname );
    bool readVRML( Handle( TDocStd_Document ) & aDoc, const char* aFname );

    bool performMeshing( Handle( XCAFDoc_ShapeTool ) & aShapeTool );

    TDF_Label transferModel( Handle( TDocStd_Document )& source, Handle( TDocStd_Document ) & dest,
                             const VECTOR3D& aScale );

    /**
     * Transfer color information from source document to destination document.
     *
     * This is necessary because TDocStd_XLinkTool::Copy may not properly transfer color
     * associations which are stored separately in the ColorTool section of XDE documents.
     * Colors are matched by searching for equivalent shapes in the destination document.
     */
    void transferColors( Handle( XCAFDoc_ShapeTool )& aSrcShapeTool,
                         Handle( XCAFDoc_ColorTool )& aSrcColorTool,
                         Handle( XCAFDoc_ShapeTool )& aDstShapeTool,
                         Handle( XCAFDoc_ColorTool )& aDstColorTool );

    bool CompressSTEP( wxString& inputFile, wxString& outputFile );

    Handle( XCAFApp_Application )   m_app;
    Handle( TDocStd_Document )      m_doc;
    Handle( XCAFDoc_ShapeTool )     m_assy;
    TDF_Label                       m_assy_label;
    bool                            m_hasPCB;           // set true if CreatePCB() has been invoked
    bool                            m_simplifyShapes;   // convert parts of outlines to arcs where possible
    bool                            m_fuseShapes;       // fuse geometry together
    bool                            m_extraPadThickness; // add extra thickness to pads
    std::vector<TDF_Label>          m_pcb_labels;       // labels for the PCB model (one by main outline)
    MODEL_MAP                       m_models;           // map of file names to model labels
    int                             m_components;       // number of successfully loaded components;
    double                          m_precision;        // model (length unit) numeric precision
    double                          m_angleprec;        // angle numeric precision
    double                          m_copperColor[3];   // copper, RGB values
    double                          m_padColor[3];      // pads, RGB values
    BOARD_STACKUP                   m_stackup;          // board stackup
    LSET                            m_enabledLayers;    // a set of layers enabled for export
    wxString                        m_netFilter;        // remove nets not matching this wildcard

    double                          m_minx;             // leftmost curve point
    double                          m_mergeOCCMaxDist;  // minimum distance (mm) below which two
                                                        // points are considered coincident by OCC

    // Holes in copper and main outlines
    std::vector<TopoDS_Shape> m_copperCutouts;
    std::vector<TopoDS_Shape> m_boardCutouts;

    // Main outlines (more than one board)
    std::vector<TopoDS_Shape> m_board_outlines;

    // Copper items. Key is netname.
    std::map<wxString, std::vector<TopoDS_Shape>> m_board_copper;
    std::map<wxString, std::vector<TopoDS_Shape>> m_board_copper_pads;
    std::map<wxString, std::vector<TopoDS_Shape>> m_board_copper_vias;
    std::map<wxString, std::vector<TopoDS_Shape>> m_board_copper_fused;

    // Graphical items
    std::vector<TopoDS_Shape> m_board_front_silk;
    std::vector<TopoDS_Shape> m_board_back_silk;
    std::vector<TopoDS_Shape> m_board_front_mask;
    std::vector<TopoDS_Shape> m_board_back_mask;

    // Data for pads. Key example: Pad_F_U2_1_GND
    std::map<wxString, std::vector<std::pair<gp_Pnt, TopoDS_Shape>>> m_pad_points;

    /// Name of the PCB, which will most likely be the file name of the path.
    wxString m_pcbName;

    /// The current output format for created file
    OUTPUT_FORMAT m_outFmt;
    REPORTER*     m_reporter;
};

#endif // OCE_VIS_OCE_UTILS_H
