/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DXF2BRD_ITEMS_H
#define DXF2BRD_ITEMS_H

#include "graphics_import_plugin.h"
#include "graphics_importer_buffer.h"

#include <dl_creationadapter.h>
#include <dl_dxf.h>
#include <wildcards_and_files_ext.h>
#include <wx/wx.h>

#include <list>

class BOARD;
class BOARD_ITEM;

/**
 * A helper class to store a spline control point (in X,Y plane only)
 */
struct SPLINE_CTRL_POINT
{
    double m_x;
    double m_y;
    double m_weight;

    SPLINE_CTRL_POINT( double a_x, double a_y, double a_weight )
                    : m_x( a_x ), m_y( a_y ), m_weight( a_weight )
    {}
};

/**
 * A helper class to parse a DXF entity (polyline and spline)
 */
class DXF2BRD_ENTITY_DATA
{
public:
    int m_EntityType;           // the DXF type of entity
    int m_EntityParseStatus;    // Inside a entity: status of parsing:
                                // 0 = no entity
                                // 1 = first item of entity
                                // 2 = entity in progress
    int m_EntityFlag;           // a info flag to parse entities

    VECTOR2D m_LastCoordinate;  // the last vertex coordinate read (unit = mm)
    VECTOR2D m_PolylineStart;   // The first point of the polyline entity, when reading a
                                // polyline (unit = mm)
    double m_BulgeVertex;       // the last vertex bulge value read

    // for spline parsing: parameters
    unsigned int m_SplineDegree;
    unsigned int m_SplineKnotsCount;
    unsigned int m_SplineControlCount;
    unsigned int m_SplineFitCount;
    double m_SplineTangentStartX;   // tangent dir X for the start point
    double m_SplineTangentStartY;   // tangent dir Y for the start point
    double m_SplineTangentEndX;     // tangent dir X for the end point
    double m_SplineTangentEndY;     // tangent dir Y for the end point

    // for spline parsing: buffers to store control points, fit points and knot
    std::vector<double> m_SplineKnotsList;          // knots list, code 40
    // control points list coordinates, code 10, 20 & 30 (only X and Y cood and Weight)
    std::vector<SPLINE_CTRL_POINT> m_SplineControlPointList;
    // fit points list, code 11, 21 & 31 (only X and Y cood)
    std::vector<VECTOR2D> m_SplineFitPointList;

    DXF2BRD_ENTITY_DATA() { Clear(); };

    // Reset the entity parameters
    void Clear()
    {
        m_EntityType = DL_UNKNOWN;
        m_EntityParseStatus = 0;
        m_EntityFlag = 0;
        m_SplineDegree = 1;
        m_SplineKnotsCount = 0;
        m_SplineControlCount = 0;
        m_SplineFitCount = 0;
        m_SplineTangentStartX = 0.0;
        m_SplineTangentStartY = 0.0;
        m_SplineTangentEndX = 0.0;
        m_SplineTangentEndY = 0.0;
        m_BulgeVertex = 0.0;
        m_SplineKnotsList.clear();
        m_SplineControlPointList.clear();
        m_SplineFitPointList.clear();
    }
};


/**
 * This class import DXF ASCII files and convert basic entities to board entities.
 * It depends on the dxflib library.
 */
class DXF_IMPORT_PLUGIN : public GRAPHICS_IMPORT_PLUGIN, public DL_CreationAdapter
{
private:
    double m_xOffset;           // X coord offset for conversion (in mm)
    double m_yOffset;           // Y coord offset for conversion (in mm)
    double m_defaultThickness;  // default line thickness for conversion (in mm)
    double m_DXF2mm;            // The scale factor to convert DXF units to mm
    int m_brdLayer;             // The board layer to place imported DXF items
    int m_version;              // the dxf version, not used here
    bool m_inBlock;             // Are we parsing a block
    std::string m_codePage;     // The code page, not used here
    bool m_importAsfootprintGraphicItems;  // Use module items instead of board items when true.
                                // true when the items are imported in the footprint editor
    std::string m_messages;     // messages generated during dxf file parsing.
                                // Each message ends by '\n'
    DXF2BRD_ENTITY_DATA m_curr_entity;  // the current entity parameters when parsing a DXF entity

    double m_minX, m_maxX;      // handles image size in mm
    double m_minY, m_maxY;      // handles image size in mm

    GRAPHICS_IMPORTER_BUFFER m_internalImporter;


public:
    DXF_IMPORT_PLUGIN();
    ~DXF_IMPORT_PLUGIN();

    const wxString GetName() const override
    {
        return "AutoCAD DXF";
    }

    const std::vector<std::string> GetFileExtensions() const override
    {
        static std::vector<std::string> exts = { "dxf" };
        return exts;
    }

    bool Load( const wxString& aFileName ) override;
    bool Import() override;

    double GetImageWidth() const override;
    double GetImageHeight() const override;

    void updateImageLimits( const VECTOR2D& aPoint );

    virtual void SetImporter( GRAPHICS_IMPORTER* aImporter ) override;

    /**
     * Allows the import DXF items converted to board graphic items or footprint
     * graphic items.
     * @param aImportAsFootprintGraphic = true to import in a footprint, false to import on a board
     */
    void ImportAsFootprintGraphic( bool aImportAsFootprintGraphic )
    {
        m_importAsfootprintGraphicItems = aImportAsFootprintGraphic;
    }


    /**
     * Set the default line width when importing dxf items like lines to Pcbnew.
     * because dxf files have no line width explicit parameter, it will be most
     * of time the line width of imported lines
     * @param aWidth = line width in mm
     */
    void SetDefaultLineWidthMM( double aWidth )
    {
        m_defaultThickness = aWidth;
    }

    void SetLineWidthMM( double aWidth ) override { SetDefaultLineWidthMM( aWidth ); }

    /**
     * Set the coordinate offset between the imported dxf items and Pcbnew.
     * because dxf files have the Y axis from bottom to top;
     * aOffsetX = 0, and aOffsetY = - vertical page size to import a full page
     * @param aOffsetX = the X offset in mm
     * @param aOffsetY = the Y offset in mm
     */
    void SetOffset( double aOffsetX, double aOffsetY )
    {
        m_xOffset = aOffsetX;
        m_yOffset = aOffsetY;
    }

    /**
     * Set the layer number to import dxf items.
     * the layer should be a technical layer, not a copper layer
     */
    void SetBrdLayer( int aBrdLayer ) { m_brdLayer = aBrdLayer; }

    /**
     * Implementation of the method used for communicate
     * with this filter.
     *
     * @param aFile = the full filename.
     */
    bool ImportDxfFile( const wxString& aFile );

    /**
     * @return the list of messages in one string. Each message ends by '\n'
     */
    wxString GetMessages() const override
    {
        return m_messages;
    }

private:
    // report message to keep trace of not supported dxf entities:
    void reportMsg( const wxString& aMessage );

    // coordinate conversions from dxf file to mm
    double mapX( double aDxfCoordX );
    double mapY( double aDxfCoordY );
    double mapDim( double aDxfValue );
    // mapWidth returns ( in mm) the aDxfValue if aDxfWidth > 0
    // or m_defaultThickness
    double mapWidth( double aDxfWidth );

    // Functions to aid in the creation of a Polyline
    void insertLine( const VECTOR2D& aSegStart, const VECTOR2D& aSegEnd, int aWidth );
    void insertArc( const VECTOR2D& aSegStart, const VECTOR2D& aSegEnd,
                    double aBulge, int aWidth );
    // Add a dxf spline (stored in m_curr_entity) to the board, after conversion to segments
    void insertSpline( int aWidth );

    // Methods from DL_CreationAdapter:
    // They are something like"call back" functions,
    // called when the corresponding object is read in dxf file

    /**
     * Called for every string variable in the DXF file (e.g. "$ACADVER").
     */
    virtual void setVariableString( const std::string& key, const std::string& value,
            int code ) override;

    /**
     * Called for every int variable in the DXF file (e.g. "$ACADMAINTVER").
     */
    virtual void setVariableInt( const std::string& key, int value, int code ) override;

    /**
     * Called for every double variable in the DXF file (e.g. "$DIMEXO").
     */
    virtual void setVariableDouble( const std::string& key, double value, int code ) override {}

    virtual void addLayer( const DL_LayerData& aData ) override;
    virtual void addLine( const DL_LineData& aData) override;

    /**
     * Called for each BLOCK in the DXF file
     * These are re-usable elements that may be placed into the model space.  The elements
     * are dereferenced to the model, so we just need to skip the re-parsing for the block
     * elements.
     */
    virtual void addBlock( const DL_BlockData& ) override;
    virtual void endBlock() override;

    virtual void addCircle( const DL_CircleData& aData ) override;
    virtual void addArc( const DL_ArcData& aData ) override;
    //virtual void addLWPolyline( const DRW_LWPolyline& aData ) override;
    virtual void addText( const DL_TextData& aData ) override;
    virtual void addPolyline( const DL_PolylineData& aData ) override;

    /** Called for every polyline vertex */
    virtual void addVertex( const DL_VertexData& aData ) override;
    virtual void addMText( const DL_MTextData& aData) override;
    virtual void addTextStyle( const DL_StyleData& aData ) override;

    virtual void endEntity() override;

    /** Called for every spline */
    virtual void addSpline( const DL_SplineData& aData ) override;

    /** Called for every spline control point */
    virtual void addControlPoint( const DL_ControlPointData& aData ) override;

    /** Called for every spline fit point */
    virtual void addFitPoint( const DL_FitPointData& aData ) override;

    /** Called for every spline knot value */
    virtual void addKnot( const DL_KnotData& aData ) override;

    // Not yet handled DXF entities:
    virtual void addDimAlign( const DL_DimensionData&,
            const DL_DimAlignedData& ) override {}
    virtual void addDimLinear( const DL_DimensionData&,
            const DL_DimLinearData& ) override {}
    virtual void addDimRadial( const DL_DimensionData&,
            const DL_DimRadialData& ) override {}
    virtual void addDimDiametric( const DL_DimensionData&,
            const DL_DimDiametricData& ) override {}
    virtual void addDimAngular( const DL_DimensionData&,
            const DL_DimAngularData& ) override {}
    virtual void addDimAngular3P( const DL_DimensionData&,
            const DL_DimAngular3PData& ) override {}
    virtual void addDimOrdinate( const DL_DimensionData&,
            const DL_DimOrdinateData& ) override {}
    virtual void addLeader( const DL_LeaderData& ) override {}

    virtual void addLeaderVertex( const DL_LeaderVertexData& ) override {}

    virtual void addHatch( const DL_HatchData& ) override {}

    virtual void addTrace( const DL_TraceData& ) override {}
    virtual void add3dFace( const DL_3dFaceData& ) override {}

    virtual void addSolid( const DL_SolidData& ) override {}

    virtual void addImage( const DL_ImageData& ) override {}
    virtual void linkImage( const DL_ImageDefData& ) override {}

    virtual void addHatchLoop( const DL_HatchLoopData& ) override {}

    virtual void addHatchEdge( const DL_HatchEdgeData& ) override {}

    /**
     * Convert a native unicode string into a DXF encoded string.
     *
     * DXF encoding includes the following special sequences:
     * - %%%c for a diameter sign
     * - %%%d for a degree sign
     * - %%%p for a plus/minus sign
     */
    static wxString toDxfString( const wxString& aStr );

    /**
     * Converts a DXF encoded string into a native Unicode string.
     */
    static wxString toNativeString( const wxString& aData );

    void writeLine();
    void writeMtext();
};

#endif  // DXF2BRD_ITEMS_H
