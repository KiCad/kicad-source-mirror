/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

// The DXF reader lib (libdxfrw) comes from dxflib project used in QCAD
// See http://www.ribbonsoft.com
// Each time a dxf entity is read, a "call back" function is called
// like void DXF_IMPORT_PLUGIN::addLine( const DL_LineData& data ) when a line is read.
// this function just add the BOARD entity from dxf parameters (start and end point ...)


#include "dxf_import_plugin.h"
#include <wx/arrstr.h>
#include <wx/regex.h>
#include <geometry/ellipse.h>
#include <bezier_curves.h>

#include <trigo.h>
#include <macros.h>
#include <cmath>    // isnan
#include <board.h>
#include "common.h"


/*
 * Important notes
 * 1. All output coordinates of this importer are in mm
 * 2. DXFs have a concept of world (WCS) and object coordinates (OCS)
   3. The following objects are world coordinates:
        - Line
        - Point
        - Polyline (3D)
        - Vertex (3D)
        - Polymesh
        - Polyface
        - Viewport
    4. The following entities are object coordinates
        - Circle
        - Arc
        - Solid
        - Trace
        - Attrib
        - Shape
        - Insert
        - Polyline (2D)
        - Vertex (2D)
        - LWPolyline
        - Hatch
        - Image
        - Text
 *   5. Object coordinates must be run through the arbitrary axis
 *      translation even though they are 2D drawings and most of the time
 *      the import is fine. Sometimes, against all logic, CAD tools like
 *      SolidWorks may randomly insert circles "mirror" that must be unflipped
 *      by following the object to world conversion
 *    6. Blocks are virtual groups, blocks must be placed by a INSERT entity
 *    7. Blocks may be repeated multiple times
 *    8. There is no sane way to make text look perfect like the original CAD.
 *       DXF simply does mpt specifying text/font enough to make it portable.
 *       We however make do try to get it somewhat close/visually appealing.
 *    9. We silently drop the z coordinate on 3d polylines
 */


// minimum bulge value before resorting to a line segment;
// the value 0.0218 is equivalent to about 5 degrees arc,
#define MIN_BULGE 0.0218

#define SCALE_FACTOR(x) (x)


DXF_IMPORT_PLUGIN::DXF_IMPORT_PLUGIN() : DL_CreationAdapter()
{
    m_xOffset          = 0.0;          // X coord offset for conversion (in mm)
    m_yOffset          = 0.0;          // Y coord offset for conversion (in mm)
    m_version          = 0;            // the dxf version, not yet used
    m_defaultThickness = 0.2;          // default thickness (in mm)
    m_brdLayer         = Dwgs_User;    // The default import layer
    m_importAsFPShapes = true;
    m_minX             = m_minY = std::numeric_limits<double>::max();
    m_maxX             = m_maxY = std::numeric_limits<double>::lowest();
    m_currentUnit      = DXF_IMPORT_UNITS::DEFAULT;

    m_importCoordinatePrecision = 4;    // initial value per dxf spec
    m_importAnglePrecision      = 0;    // initial value per dxf spec

    // placeholder layer so we can fallback to something later
    auto layer0 = std::make_unique<DXF_IMPORT_LAYER>( "", DXF_IMPORT_LINEWEIGHT_BY_LW_DEFAULT );
    m_layers.push_back( std::move( layer0 ) );

    m_currentBlock = nullptr;
}


DXF_IMPORT_PLUGIN::~DXF_IMPORT_PLUGIN()
{
}


bool DXF_IMPORT_PLUGIN::Load( const wxString& aFileName )
{
    try
    {
        return ImportDxfFile( aFileName );
    }
    catch( const std::bad_alloc& )
    {
        m_layers.clear();
        m_blocks.clear();
        m_styles.clear();

        m_internalImporter.ClearShapes();

        ReportMsg( _( "Memory was exhausted trying to load the DXF, it may be too large." ) );
        return false;
    }
}


bool DXF_IMPORT_PLUGIN::LoadFromMemory( const wxMemoryBuffer& aMemBuffer )
{
    try
    {
        return ImportDxfFile( aMemBuffer );
    }
    catch( const std::bad_alloc& )
    {
        m_layers.clear();
        m_blocks.clear();
        m_styles.clear();

        m_internalImporter.ClearShapes();

        ReportMsg( _( "Memory was exhausted trying to load the DXF, it may be too large." ) );
        return false;
    }
}


bool DXF_IMPORT_PLUGIN::Import()
{
    wxCHECK( m_importer, false );
    m_internalImporter.ImportTo( *m_importer );

    return true;
}


double DXF_IMPORT_PLUGIN::GetImageWidth() const
{
    return m_maxX - m_minX;
}


double DXF_IMPORT_PLUGIN::GetImageHeight() const
{
    return m_maxY - m_minY;
}


BOX2D DXF_IMPORT_PLUGIN::GetImageBBox() const
{
    BOX2D bbox;
    bbox.SetOrigin( m_minX, m_minY );
    bbox.SetEnd( m_maxX, m_maxY );

    return bbox;
}


void DXF_IMPORT_PLUGIN::SetImporter( GRAPHICS_IMPORTER* aImporter )
{
    GRAPHICS_IMPORT_PLUGIN::SetImporter( aImporter );

    if( m_importer )
        SetDefaultLineWidthMM( m_importer->GetLineWidthMM() );
}


double DXF_IMPORT_PLUGIN::mapX( double aDxfCoordX )
{
    return SCALE_FACTOR( m_xOffset + ( aDxfCoordX * getCurrentUnitScale() ) );
}


double DXF_IMPORT_PLUGIN::mapY( double aDxfCoordY )
{
    return SCALE_FACTOR( m_yOffset - ( aDxfCoordY * getCurrentUnitScale() ) );
}


double DXF_IMPORT_PLUGIN::mapDim( double aDxfValue )
{
    return SCALE_FACTOR( aDxfValue * getCurrentUnitScale() );
}


bool DXF_IMPORT_PLUGIN::ImportDxfFile( const wxString& aFile )
{
    DL_Dxf dxf_reader;

    // wxFopen takes care of unicode filenames across platforms
    FILE* fp = wxFopen( aFile, wxT( "rt" ) );

    if( fp == nullptr )
        return false;

    // Note the dxf reader takes care of switching to "C" locale before reading the file
    // and will close the file after reading
    bool success = dxf_reader.in( fp, this );

    return success;
}


bool DXF_IMPORT_PLUGIN::ImportDxfFile( const wxMemoryBuffer& aMemBuffer )
{
    DL_Dxf dxf_reader;

    std::string str( reinterpret_cast<char*>( aMemBuffer.GetData() ), aMemBuffer.GetDataLen() );

    // Note the dxf reader takes care of switching to "C" locale before reading the file
    // and will close the file after reading
    bool success = dxf_reader.in( str, this );

    return success;
}


void DXF_IMPORT_PLUGIN::ReportMsg( const wxString& aMessage )
{
    // Add message to keep trace of not handled dxf entities
    m_messages += aMessage;
    m_messages += '\n';
}


void DXF_IMPORT_PLUGIN::addSpline( const DL_SplineData& aData )
{
    // Called when starting reading a spline
    m_curr_entity.Clear();
    m_curr_entity.m_EntityParseStatus = 1;
    m_curr_entity.m_EntityFlag = aData.flags;
    m_curr_entity.m_EntityType = DL_ENTITY_SPLINE;
    m_curr_entity.m_SplineDegree = aData.degree;
    m_curr_entity.m_SplineTangentStartX = aData.tangentStartX;
    m_curr_entity.m_SplineTangentStartY = aData.tangentStartY;
    m_curr_entity.m_SplineTangentEndX = aData.tangentEndX;
    m_curr_entity.m_SplineTangentEndY = aData.tangentEndY;
    m_curr_entity.m_SplineKnotsCount = aData.nKnots;
    m_curr_entity.m_SplineControlCount = aData.nControl;
    m_curr_entity.m_SplineFitCount = aData.nFit;
}


void DXF_IMPORT_PLUGIN::addControlPoint( const DL_ControlPointData& aData )
{
    // Called for every spline control point, when reading a spline entity
    m_curr_entity.m_SplineControlPointList.emplace_back( aData.x , aData.y, aData.w );
}


void DXF_IMPORT_PLUGIN::addFitPoint( const DL_FitPointData& aData )
{
    // Called for every spline fit point, when reading a spline entity
    // we store only the X,Y coord values in a VECTOR2D
    m_curr_entity.m_SplineFitPointList.emplace_back( aData.x, aData.y );
}


void DXF_IMPORT_PLUGIN::addKnot( const DL_KnotData& aData)
{
    // Called for every spline knot value, when reading a spline entity
    m_curr_entity.m_SplineKnotsList.push_back( aData.k );
}


void DXF_IMPORT_PLUGIN::addLayer( const DL_LayerData& aData )
{
    wxString name = wxString::FromUTF8( aData.name.c_str() );

    int lw = attributes.getWidth();

    if( lw == DXF_IMPORT_LINEWEIGHT_BY_LAYER )
        lw = DXF_IMPORT_LINEWEIGHT_BY_LW_DEFAULT;

    std::unique_ptr<DXF_IMPORT_LAYER> layer = std::make_unique<DXF_IMPORT_LAYER>( name, lw );

    m_layers.push_back( std::move( layer ) );
}


void DXF_IMPORT_PLUGIN::addLinetype( const DL_LinetypeData& data )
{
#if 0
    wxString name = From_UTF8( data.name.c_str() );
    wxString description = From_UTF8( data.description.c_str() );
#endif
}


double DXF_IMPORT_PLUGIN::lineWeightToWidth( int lw, DXF_IMPORT_LAYER* aLayer )
{
    if( lw == DXF_IMPORT_LINEWEIGHT_BY_LAYER && aLayer != nullptr )
        lw = aLayer->m_lineWeight;

    // All lineweights >= 0 are always in 100ths of mm
    double mm = m_defaultThickness;

    if( lw >= 0 )
        mm = lw / 100.0;

    return SCALE_FACTOR( mm );
}


DXF_IMPORT_LAYER* DXF_IMPORT_PLUGIN::getImportLayer( const std::string& aLayerName )
{
    DXF_IMPORT_LAYER* layer     = m_layers.front().get();
    wxString          layerName = wxString::FromUTF8( aLayerName.c_str() );

    if( !layerName.IsEmpty() )
    {
        auto resultIt = std::find_if( m_layers.begin(), m_layers.end(),
                [layerName]( const auto& it )
                {
                    return it->m_layerName == layerName;
                } );

        if( resultIt != m_layers.end() )
            layer = resultIt->get();
    }

    return layer;
}


DXF_IMPORT_BLOCK* DXF_IMPORT_PLUGIN::getImportBlock( const std::string& aBlockName )
{
    DXF_IMPORT_BLOCK* block     = nullptr;
    wxString          blockName = wxString::FromUTF8( aBlockName.c_str() );

    if( !blockName.IsEmpty() )
    {
        auto resultIt = std::find_if( m_blocks.begin(), m_blocks.end(),
                [blockName]( const auto& it )
                {
                    return it->m_name == blockName;
                } );

        if( resultIt != m_blocks.end() )
            block = resultIt->get();
    }

    return block;
}


DXF_IMPORT_STYLE* DXF_IMPORT_PLUGIN::getImportStyle( const std::string& aStyleName )
{
    DXF_IMPORT_STYLE* style     = nullptr;
    wxString          styleName = wxString::FromUTF8( aStyleName.c_str() );

    if( !styleName.IsEmpty() )
    {
        auto resultIt = std::find_if( m_styles.begin(), m_styles.end(),
                [styleName]( const auto& it )
                {
                    return it->m_name == styleName;
                } );

        if( resultIt != m_styles.end() )
            style = resultIt->get();
    }

    return style;
}


void DXF_IMPORT_PLUGIN::addLine( const DL_LineData& aData )
{
    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    VECTOR2D start( mapX( aData.x1 ), mapY( aData.y1 ) );
    VECTOR2D end( mapX( aData.x2 ), mapY( aData.y2 ) );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddLine( start, end, lineWidth );

    updateImageLimits( start );
    updateImageLimits( end );
}


void DXF_IMPORT_PLUGIN::addPolyline(const DL_PolylineData& aData )
{
    // Convert DXF Polylines into a series of KiCad Lines and Arcs.
    // A Polyline (as opposed to a LWPolyline) may be a 3D line or
    // even a 3D Mesh. The only type of Polyline which is guaranteed
    // to import correctly is a 2D Polyline in X and Y, which is what
    // we assume of all Polylines. The width used is the width of the Polyline.
    // per-vertex line widths, if present, are ignored.
    m_curr_entity.Clear();
    m_curr_entity.m_EntityParseStatus = 1;
    m_curr_entity.m_EntityFlag = aData.flags;
    m_curr_entity.m_EntityType = DL_ENTITY_POLYLINE;
}


void DXF_IMPORT_PLUGIN::addVertex( const DL_VertexData& aData )
{
    if( m_curr_entity.m_EntityParseStatus == 0 )
        return;     // Error

    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    /* support for per-vertex-encoded linewidth (Cadence uses it) */
    /* linewidths are scaled by 100 in DXF */
    if( aData.startWidth > 0.0 )
        lineWidth = aData.startWidth / 100.0;
    else if ( aData.endWidth > 0.0 )
        lineWidth = aData.endWidth / 100.0;

    const DL_VertexData* vertex = &aData;

    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D vertexCoords = ocsToWcs( arbAxis, VECTOR3D( vertex->x, vertex->y, vertex->z ) );

    if( m_curr_entity.m_EntityParseStatus == 1 )    // This is the first vertex of an entity
    {
        m_curr_entity.m_LastCoordinate.x  = mapX( vertexCoords.x );
        m_curr_entity.m_LastCoordinate.y  = mapY( vertexCoords.y );
        m_curr_entity.m_PolylineStart = m_curr_entity.m_LastCoordinate;
        m_curr_entity.m_BulgeVertex = vertex->bulge;
        m_curr_entity.m_EntityParseStatus = 2;
        return;
    }

    VECTOR2D seg_end( mapX( vertexCoords.x ), mapY( vertexCoords.y ) );

    if( std::abs( m_curr_entity.m_BulgeVertex ) < MIN_BULGE )
        insertLine( m_curr_entity.m_LastCoordinate, seg_end, lineWidth );
    else
        insertArc( m_curr_entity.m_LastCoordinate, seg_end, m_curr_entity.m_BulgeVertex,
                   lineWidth );

    m_curr_entity.m_LastCoordinate = seg_end;
    m_curr_entity.m_BulgeVertex = vertex->bulge;
}


void DXF_IMPORT_PLUGIN::endEntity()
{
    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    if( m_curr_entity.m_EntityType == DL_ENTITY_POLYLINE ||
        m_curr_entity.m_EntityType == DL_ENTITY_LWPOLYLINE )
    {
        // Polyline flags bit 0 indicates closed (1) or open (0) polyline
        if( m_curr_entity.m_EntityFlag & 1 )
        {
            if( std::abs( m_curr_entity.m_BulgeVertex ) < MIN_BULGE )
            {
                insertLine( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart,
                            lineWidth );
            }
            else
            {
                insertArc( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart,
                           m_curr_entity.m_BulgeVertex, lineWidth );
            }
        }
    }

    if( m_curr_entity.m_EntityType == DL_ENTITY_SPLINE )
        insertSpline( lineWidth );

    m_curr_entity.Clear();
}


void DXF_IMPORT_PLUGIN::addBlock( const DL_BlockData& aData )
{
    wxString name = wxString::FromUTF8( aData.name.c_str() );

    std::unique_ptr<DXF_IMPORT_BLOCK> block = std::make_unique<DXF_IMPORT_BLOCK>( name, aData.bpx,
                                                                                  aData.bpy );

    m_blocks.push_back( std::move( block ) );

    m_currentBlock = m_blocks.back().get();
}


void DXF_IMPORT_PLUGIN::endBlock()
{
    m_currentBlock = nullptr;
}

void DXF_IMPORT_PLUGIN::addInsert( const DL_InsertData& aData )
{
    DXF_IMPORT_BLOCK* block = getImportBlock( aData.name );

    if( block == nullptr )
        return;

    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );

    MATRIX3x3D rot;
    rot.SetRotation( DEG2RAD( -aData.angle ) ); // DL_InsertData angle is in degrees

    MATRIX3x3D scale;
    scale.SetScale( VECTOR2D( aData.sx, aData.sy ) );

    MATRIX3x3D trans = ( arbAxis * rot ) * scale;
    VECTOR3D insertCoords = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );

    VECTOR2D translation( mapX( insertCoords.x ), mapY( insertCoords.y ) );
    translation -= VECTOR2D( mapX( block->m_baseX ), mapY( block->m_baseY ) );

    for( const std::unique_ptr<IMPORTED_SHAPE>& shape : block->m_buffer.GetShapes() )
    {
        std::unique_ptr<IMPORTED_SHAPE> newShape = shape->clone();

        newShape->Transform( trans, translation );

        m_internalImporter.AddShape( newShape );
    }
}


void DXF_IMPORT_PLUGIN::addCircle( const DL_CircleData& aData )
{
    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D   centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.cx, aData.cy, aData.cz ) );

    VECTOR2D          center( mapX( centerCoords.x ), mapY( centerCoords.y ) );
    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddCircle( center, mapDim( aData.radius ), lineWidth, false );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addArc( const DL_ArcData& aData )
{
    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D   centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.cx, aData.cy, aData.cz ) );

    // Init arc centre:
    VECTOR2D center( mapX( centerCoords.x ), mapY( centerCoords.y ) );

    // aData.anglex is in degrees.
    EDA_ANGLE  startangle( aData.angle1, DEGREES_T );
    EDA_ANGLE  endangle( aData.angle2, DEGREES_T );

    if( ( arbAxis.GetScale().x < 0 ) != ( arbAxis.GetScale().y < 0 ) )
    {
        startangle = ANGLE_180 - startangle;
        endangle = ANGLE_180 - endangle;
        std::swap( startangle, endangle );
    }

    // Init arc start point
    VECTOR2D startPoint( aData.radius, 0.0 );
    RotatePoint( startPoint, -startangle );
    VECTOR2D arcStart( mapX( startPoint.x + centerCoords.x ),
                       mapY( startPoint.y + centerCoords.y ) );

    // calculate arc angle (arcs are CCW, and should be < 0 in Pcbnew)
    EDA_ANGLE angle = -( endangle - startangle );

    if( angle > ANGLE_0 )
        angle -= ANGLE_360;

    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddArc( center, arcStart, angle, lineWidth );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addEllipse( const DL_EllipseData& aData )
{
    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D   centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.cx, aData.cy, aData.cz ) );
    VECTOR3D   majorCoords = ocsToWcs( arbAxis, VECTOR3D( aData.mx, aData.my, aData.mz ) );

    // DXF ellipses store the minor axis length as a ratio to the major axis.
    // The major coords are relative to the center point.
    // For now, we assume ellipses in the XY plane.

    VECTOR2D center( mapX( centerCoords.x ), mapY( centerCoords.y ) );
    VECTOR2D major( mapX( majorCoords.x ), mapY( majorCoords.y ) );

    // DXF elliptical arcs store their angles in radians (unlike circular arcs which use degrees)
    // The arcs wind CCW as in KiCad.  The end angle must be greater than the start angle, and if
    // the extrusion direction is negative, the arc winding is CW instead!  Note that this is a
    // simplification that assumes the DXF is representing a 2D drawing, and would need to be
    // revisited if we want to import true 3D drawings and "flatten" them to the 2D KiCad plane
    // internally.
    EDA_ANGLE startAngle( aData.angle1, RADIANS_T );
    EDA_ANGLE endAngle( aData.angle2, RADIANS_T );

    if( startAngle > endAngle )
        endAngle += ANGLE_360;

    if( aData.ratio == 1.0 )
    {
        double radius = major.EuclideanNorm();

        if( startAngle == endAngle )
        {
            DL_CircleData circle( aData.cx, aData.cy, aData.cz, radius );
            addCircle( circle );
            return;
        }
        else
        {
            // Angles are relative to major axis
            startAngle -= EDA_ANGLE( major );
            endAngle -= EDA_ANGLE( major );

            DL_ArcData arc( aData.cx, aData.cy, aData.cz, radius, startAngle.AsDegrees(),
                            endAngle.AsDegrees() );
            addArc( arc );
            return;
        }
    }

    // TODO: testcases for negative extrusion vector; handle it here

    std::vector<BEZIER<double>> splines;
    ELLIPSE<double> ellipse( center, major, aData.ratio, startAngle, endAngle );

    TransformEllipseToBeziers( ellipse, splines );

    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;

    for( const BEZIER<double>& b : splines )
        bufferToUse->AddSpline( b.Start, b.C1, b.C2, b.End, lineWidth );

    // Naive bounding
    updateImageLimits( center + major );
    updateImageLimits( center - major );
}


void DXF_IMPORT_PLUGIN::addText( const DL_TextData& aData )
{
    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D refPointCoords    = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );
    VECTOR3D   secPointCoords =
            ocsToWcs( arbAxis, VECTOR3D( std::isnan( aData.apx ) ? 0 : aData.apx,
                                         std::isnan( aData.apy ) ? 0 : aData.apy,
                                         std::isnan( aData.apz ) ? 0 : aData.apz ) );

    VECTOR2D refPoint( mapX( refPointCoords.x ), mapY( refPointCoords.y ) );
    VECTOR2D secPoint( mapX( secPointCoords.x ), mapY( secPointCoords.y ) );

    if( aData.vJustification != 0 || aData.hJustification != 0 || aData.hJustification == 4 )
    {
        if( aData.hJustification != 3 && aData.hJustification != 5 )
        {
            VECTOR2D tmp = secPoint;
            secPoint = refPoint;
            refPoint = tmp;
        }
    }

    wxString text = toNativeString( wxString::FromUTF8( aData.text.c_str() ) );

    DXF_IMPORT_STYLE* style = getImportStyle( aData.style.c_str() );

    double textHeight = mapDim( aData.height );

    // The 0.9 factor gives a better height/width base ratio with our font
    double charWidth = textHeight * 0.9;

    if( style != nullptr )
        charWidth *= style->m_widthFactor;

    double textWidth = charWidth * text.length();   // Rough approximation
    double textThickness = textHeight / 8.0;        // Use a reasonable line thickness for this text

    VECTOR2D bottomLeft( 0.0, 0.0 );
    VECTOR2D bottomRight( 0.0, 0.0 );
    VECTOR2D topLeft( 0.0, 0.0 );
    VECTOR2D topRight( 0.0, 0.0 );

    GR_TEXT_H_ALIGN_T hJustify = GR_TEXT_H_ALIGN_LEFT;
    GR_TEXT_V_ALIGN_T vJustify = GR_TEXT_V_ALIGN_BOTTOM;

    switch( aData.vJustification )
    {
    case 0: //DRW_Text::VBaseLine:
    case 1: //DRW_Text::VBottom:
        vJustify = GR_TEXT_V_ALIGN_BOTTOM;

        topLeft.y = textHeight;
        topRight.y = textHeight;
        break;

    case 2: //DRW_Text::VMiddle:
        vJustify = GR_TEXT_V_ALIGN_CENTER;

        bottomRight.y = -textHeight / 2.0;
        bottomLeft.y = -textHeight / 2.0;
        topLeft.y = textHeight / 2.0;
        topRight.y = textHeight / 2.0;
        break;

    case 3: //DRW_Text::VTop:
        vJustify = GR_TEXT_V_ALIGN_TOP;

        bottomLeft.y = -textHeight;
        bottomRight.y = -textHeight;
        break;
    }

    switch( aData.hJustification )
    {
    case 0: //DRW_Text::HLeft:
    case 3: //DRW_Text::HAligned:    // no equivalent options in text pcb.
    case 5: //DRW_Text::HFit:       // no equivalent options in text pcb.
        hJustify = GR_TEXT_H_ALIGN_LEFT;

        bottomRight.x = textWidth;
        topRight.x = textWidth;
        break;

    case 1: //DRW_Text::HCenter:
    case 4: //DRW_Text::HMiddle:     // no equivalent options in text pcb.
        hJustify = GR_TEXT_H_ALIGN_CENTER;

        bottomLeft.x = -textWidth / 2.0;
        topLeft.x = -textWidth / 2.0;
        bottomRight.x = textWidth / 2.0;
        topRight.x = textWidth / 2.0;
        break;

    case 2: //DRW_Text::HRight:
        hJustify = GR_TEXT_H_ALIGN_RIGHT;

        bottomLeft.x = -textWidth;
        topLeft.x = -textWidth;
        break;
    }

#if 0
    wxString sty = wxString::FromUTF8( aData.style.c_str() );
    sty = sty.ToLower();

    if( aData.textgen == 2 )
    {
        // Text dir = left to right;
    } else if( aData.textgen == 4 )
    {
        // Text dir = top to bottom;
    } else
    {
    }
#endif

    // dxf_lib imports text angle in radians (although there are no comment about that.
    // So, for the moment, convert this angle to degrees
    double angle_degree = aData.angle * 180 / M_PI;

    // We also need the angle in radians. so convert angle_degree to radians
    // regardless the aData.angle unit
    double angleInRads = angle_degree * M_PI / 180.0;
    double cosine = cos(angleInRads);
    double sine = sin(angleInRads);

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddText( refPoint, text, textHeight, charWidth, textThickness, angle_degree,
                          hJustify, vJustify );

    // Calculate the boundary box and update the image limits:
    bottomLeft.x = bottomLeft.x * cosine - bottomLeft.y * sine;
    bottomLeft.y = bottomLeft.x * sine + bottomLeft.y * cosine;

    bottomRight.x = bottomRight.x * cosine - bottomRight.y * sine;
    bottomRight.y = bottomRight.x * sine + bottomRight.y * cosine;

    topLeft.x = topLeft.x * cosine - topLeft.y * sine;
    topLeft.y = topLeft.x * sine + topLeft.y * cosine;

    topRight.x = topRight.x * cosine - topRight.y * sine;
    topRight.y = topRight.x * sine + topRight.y * cosine;

    bottomLeft += refPoint;
    bottomRight += refPoint;
    topLeft += refPoint;
    topRight += refPoint;

    updateImageLimits( bottomLeft );
    updateImageLimits( bottomRight );
    updateImageLimits( topLeft );
    updateImageLimits( topRight );
}


void DXF_IMPORT_PLUGIN::addMTextChunk( const std::string& text )
{
    // If the text string is greater than 250 characters, the string is divided into 250-character
    // chunks, which appear in one or more group 3 codes. If group 3 codes are used, the last group
    // is a group 1 and has fewer than 250 characters

    m_mtextContent.append( text );
}


void DXF_IMPORT_PLUGIN::addMText( const DL_MTextData& aData )
{
    m_mtextContent.append( aData.text );

    // TODO: determine control codes applied to the whole text?
    wxString text = toNativeString( wxString::FromUTF8( m_mtextContent.c_str() ) );

    DXF_IMPORT_STYLE* style = getImportStyle( aData.style.c_str() );
    double            textHeight = mapDim( aData.height );

    // The 0.9 factor gives a better height/width base ratio with our font
    double charWidth = textHeight * 0.9;

    if( style != nullptr )
        charWidth *= style->m_widthFactor;

    double textWidth = charWidth * text.length();   // Rough approximation
    double textThickness = textHeight/8.0;          // Use a reasonable line thickness for this text

    VECTOR2D bottomLeft(0.0, 0.0);
    VECTOR2D bottomRight(0.0, 0.0);
    VECTOR2D topLeft(0.0, 0.0);
    VECTOR2D topRight(0.0, 0.0);

    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D   textposCoords = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );
    VECTOR2D   textpos( mapX( textposCoords.x ), mapY( textposCoords.y ) );

    // Initialize text justifications:
    GR_TEXT_H_ALIGN_T hJustify = GR_TEXT_H_ALIGN_LEFT;
    GR_TEXT_V_ALIGN_T vJustify = GR_TEXT_V_ALIGN_BOTTOM;

    if( aData.attachmentPoint <= 3 )
    {
        vJustify = GR_TEXT_V_ALIGN_TOP;

        bottomLeft.y = -textHeight;
        bottomRight.y = -textHeight;
    }
    else if( aData.attachmentPoint <= 6 )
    {
        vJustify = GR_TEXT_V_ALIGN_CENTER;

        bottomRight.y = -textHeight / 2.0;
        bottomLeft.y = -textHeight / 2.0;
        topLeft.y = textHeight / 2.0;
        topRight.y = textHeight / 2.0;
    }
    else
    {
        vJustify = GR_TEXT_V_ALIGN_BOTTOM;

        topLeft.y = textHeight;
        topRight.y = textHeight;
    }

    if( aData.attachmentPoint % 3 == 1 )
    {
        hJustify = GR_TEXT_H_ALIGN_LEFT;

        bottomRight.x = textWidth;
        topRight.x = textWidth;
    }
    else if( aData.attachmentPoint % 3 == 2 )
    {
        hJustify = GR_TEXT_H_ALIGN_CENTER;

        bottomLeft.x = -textWidth / 2.0;
        topLeft.x = -textWidth / 2.0;
        bottomRight.x = textWidth / 2.0;
        topRight.x = textWidth / 2.0;
    }
    else
    {
        hJustify = GR_TEXT_H_ALIGN_RIGHT;

        bottomLeft.x = -textWidth;
        topLeft.x = -textWidth;
    }

#if 0   // These setting have no meaning in Pcbnew
    if( data.alignH == 1 )
    {
        // Text is left to right;
    }
    else if( data.alignH == 3 )
    {
        // Text is top to bottom;
    }
    else
    {
        // use ByStyle;
    }

    if( aData.alignV == 1 )
    {
        // use AtLeast;
    }
    else
    {
        // useExact;
    }
#endif

    // dxf_lib imports text angle in radians (although there are no comment about that.
    // So, for the moment, convert this angle to degrees
    double angle_degree = aData.angle * 180/M_PI;

    // We also need the angle in radians. so convert angle_degree to radians
    // regardless the aData.angle unit
    double angleInRads = angle_degree * M_PI / 180.0;
    double cosine = cos(angleInRads);
    double sine = sin(angleInRads);


    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddText( textpos, text, textHeight, charWidth, textThickness, angle_degree,
                          hJustify, vJustify );

    bottomLeft.x = bottomLeft.x * cosine - bottomLeft.y * sine;
    bottomLeft.y = bottomLeft.x * sine + bottomLeft.y * cosine;

    bottomRight.x = bottomRight.x * cosine - bottomRight.y * sine;
    bottomRight.y = bottomRight.x * sine + bottomRight.y * cosine;

    topLeft.x = topLeft.x * cosine - topLeft.y * sine;
    topLeft.y = topLeft.x * sine + topLeft.y * cosine;

    topRight.x = topRight.x * cosine - topRight.y * sine;
    topRight.y = topRight.x * sine + topRight.y * cosine;

    bottomLeft += textpos;
    bottomRight += textpos;
    topLeft += textpos;
    topRight += textpos;

    updateImageLimits( bottomLeft );
    updateImageLimits( bottomRight );
    updateImageLimits( topLeft );
    updateImageLimits( topRight );

    m_mtextContent.clear();
}


double DXF_IMPORT_PLUGIN::getCurrentUnitScale()
{
    double scale = 1.0;

    switch( m_currentUnit )
    {
    case DXF_IMPORT_UNITS::INCH:        scale = 25.4;    break;
    case DXF_IMPORT_UNITS::FEET:        scale = 304.8;   break;
    case DXF_IMPORT_UNITS::MM:          scale = 1.0;     break;
    case DXF_IMPORT_UNITS::CM:          scale = 10.0;    break;
    case DXF_IMPORT_UNITS::METERS:      scale = 1000.0;  break;
    case DXF_IMPORT_UNITS::MICROINCHES: scale = 2.54e-5; break;
    case DXF_IMPORT_UNITS::MILS:        scale = 0.0254;  break;
    case DXF_IMPORT_UNITS::YARDS:       scale = 914.4;   break;
    case DXF_IMPORT_UNITS::ANGSTROMS:   scale = 1.0e-7;  break;
    case DXF_IMPORT_UNITS::NANOMETERS:  scale = 1.0e-6;  break;
    case DXF_IMPORT_UNITS::MICRONS:     scale = 1.0e-3;  break;
    case DXF_IMPORT_UNITS::DECIMETERS:  scale = 100.0;   break;

    default:
        // use the default of 1.0 for:
        // 0: Unspecified Units
        // 3: miles
        // 7: kilometers
        // 15: decameters
        // 16: hectometers
        // 17: gigameters
        // 18: AU
        // 19: lightyears
        // 20: parsecs
        break;
    }

    return scale;
}


void DXF_IMPORT_PLUGIN::setVariableInt( const std::string& key, int value, int code )
{
    // Called for every int variable in the DXF file (e.g. "$INSUNITS").

    if( key == "$DWGCODEPAGE" )
    {
        m_codePage = value;
        return;
    }

    if( key == "$AUPREC" )
    {
        m_importAnglePrecision = value;
        return;
    }

    if( key == "$LUPREC" )
    {
        m_importCoordinatePrecision = value;
        return;
    }

    if( key == "$INSUNITS" )    // Drawing units
    {
        m_currentUnit = DXF_IMPORT_UNITS::DEFAULT;

        switch( value )
        {
        case 1:  m_currentUnit = DXF_IMPORT_UNITS::INCH;        break;
        case 2:  m_currentUnit = DXF_IMPORT_UNITS::FEET;        break;
        case 4:  m_currentUnit = DXF_IMPORT_UNITS::MM;          break;
        case 5:  m_currentUnit = DXF_IMPORT_UNITS::CM;          break;
        case 6:  m_currentUnit = DXF_IMPORT_UNITS::METERS;      break;
        case 8:  m_currentUnit = DXF_IMPORT_UNITS::MICROINCHES; break;
        case 9:  m_currentUnit = DXF_IMPORT_UNITS::MILS;        break;
        case 10: m_currentUnit = DXF_IMPORT_UNITS::YARDS;       break;
        case 11: m_currentUnit = DXF_IMPORT_UNITS::ANGSTROMS;   break;
        case 12: m_currentUnit = DXF_IMPORT_UNITS::NANOMETERS;  break;
        case 13: m_currentUnit = DXF_IMPORT_UNITS::MICRONS;     break;
        case 14: m_currentUnit = DXF_IMPORT_UNITS::DECIMETERS;  break;

        default:
            // use the default for:
            // 0: Unspecified Units
            // 3: miles
            // 7: kilometers
            // 15: decameters
            // 16: hectometers
            // 17: gigameters
            // 18: AU
            // 19: lightyears
            // 20: parsecs
            break;
        }

    return;
    }
}


void DXF_IMPORT_PLUGIN::setVariableString( const std::string& key, const std::string& value,
                                           int code )
{
    // Called for every string variable in the DXF file (e.g. "$ACADVER").
}


wxString DXF_IMPORT_PLUGIN::toDxfString( const wxString& aStr )
{
    wxString    res;
    int         j = 0;

    for( unsigned i = 0; i<aStr.length(); ++i )
    {
        int c = aStr[i];

        if( c > 175 || c < 11 )
        {
            res.append( aStr.Mid( j, i - j ) );
            j = i;

            switch( c )
            {
            case 0x0A:
                res += wxT( "\\P" );
                break;

                // diameter:
#ifdef _WIN32
            // windows, as always, is special.
            case 0x00D8:
#else
            case 0x2205:
#endif
                res += wxT( "%%C" );
                break;

            // degree:
            case 0x00B0:
                res += wxT( "%%D" );
                break;

            // plus/minus
            case 0x00B1:
                res += wxT( "%%P" );
                break;

            default:
                j--;
                break;
            }

            j++;
        }
    }

    res.append( aStr.Mid( j ) );
    return res;
}


wxString DXF_IMPORT_PLUGIN::toNativeString( const wxString& aData )
{
    wxString res;
    size_t   i = 0;
    int      braces = 0;
    int      overbarLevel = -1;

    // For description, see:
    // https://ezdxf.readthedocs.io/en/stable/dxfinternals/entities/mtext.html
    // https://www.cadforum.cz/en/text-formatting-codes-in-mtext-objects-tip8640

    for( i = 0; i < aData.length(); i++ )
    {
        switch( (wchar_t) aData[i] )
        {
        case '{': // Text area influenced by the code
            braces++;
            break;

        case '}':
            if( overbarLevel == braces )
            {
                res << '}';
                overbarLevel = -1;
            }
            braces--;
            break;

        case '^': // C0 control code
            if( ++i >= aData.length() )
                break;

            switch( (wchar_t) aData[i] )
            {
            case 'I': res << '\t'; break;
            case 'J': res << '\b'; break;
            case ' ': res << '^'; break;
            default: break;
            }
            break;

        case '\\':
        {
            if( ++i >= aData.length() )
                break;

            switch( (wchar_t) aData[i] )
            {
            case 'P': // New paragraph (new line)
            case 'X': // Paragraph wrap on the dimension line (only in dimensions)
                res << '\n';
                break;

            case '~': // Non-wrapping space, hard space
                res << L'\u00A0';
                break;

            case 'U': // Unicode character, e.g. \U+ff08
            {
                i += 2;
                wxString codeHex;

                for( ; codeHex.length() < 4 && i < aData.length(); i++ )
                    codeHex << aData[i];

                unsigned long codeVal = 0;

                if( codeHex.ToCULong( &codeVal, 16 ) && codeVal != 0 )
                    res << wxUniChar( codeVal );

                i--;
            }
            break;

            case 'S': // Stacking
            {
                i++;
                wxString stacked;

                for( ; i < aData.length(); i++ )
                {
                    if( aData[i] == ';' )
                        break;
                    else
                        stacked << aData[i];
                }

                if( stacked.Contains( wxS( "#" ) ) )
                {
                    res << '^' << '{';
                    res << stacked.BeforeFirst( '#' );
                    res << '}' << '/' << '_' << '{';
                    res << stacked.AfterFirst( '#' );
                    res << '}';
                }
                else
                {
                    stacked.Replace( wxS( "^ " ), wxS( "/" ) );
                    res << stacked;
                }
            }
            break;

            case 'O': // Start overstrike
                if( overbarLevel == -1 )
                {
                    res << '~' << '{';
                    overbarLevel = braces;
                }
                break;
            case 'o': // Stop overstrike
                if( overbarLevel == braces )
                {
                    res << '}';
                    overbarLevel = -1;
                }
                break;

            case 'L': // Start underline
            case 'l': // Stop underline
            case 'K': // Start strike-through
            case 'k': // Stop strike-through
            case 'N': // New column
                // Ignore
                break;

            case 'p': // Control codes for bullets, numbered paragraphs, tab stops and columns
            case 'Q': // Slanting (obliquing) text by angle
            case 'H': // Text height
            case 'W': // Text width
            case 'F': // Font selection
            case 'f': // Font selection (alternative)
            case 'A': // Alignment
            case 'C': // Color change (ACI colors)
            case 'c': // Color change (truecolor)
            case 'T': // Tracking, char.spacing
                // Skip to ;
                for( ; i < aData.length(); i++ )
                {
                    if( aData[i] == ';' )
                        break;
                }
                break;

            default: // Escaped character
                if( ++i >= aData.length() )
                    break;

                res << aData[i];
                break;
            }
        }
        break;

        default: res << aData[i];
        }
    }

    if( overbarLevel != -1 )
    {
        res << '}';
        overbarLevel = -1;
    }

#if 1
    wxRegEx regexp;

    // diameter:
    regexp.Compile( wxT( "%%[cC]" ) );
#ifdef __WINDOWS__
    // windows, as always, is special.
    regexp.Replace( &res, wxChar( 0xD8 ) );
#else
    // Empty_set, diameter is 0x2300
    regexp.Replace( &res, wxChar( 0x2205 ) );
#endif

    // degree:
    regexp.Compile( wxT( "%%[dD]" ) );
    regexp.Replace( &res, wxChar( 0x00B0 ) );

    // plus/minus
    regexp.Compile( wxT( "%%[pP]" ) );
    regexp.Replace( &res, wxChar( 0x00B1 ) );
#endif

    return res;
}


void DXF_IMPORT_PLUGIN::addTextStyle( const DL_StyleData& aData )
{
    wxString name = wxString::FromUTF8( aData.name.c_str() );

    auto style = std::make_unique<DXF_IMPORT_STYLE>( name, aData.fixedTextHeight, aData.widthFactor,
                                                     aData.bold, aData.italic );

    m_styles.push_back( std::move( style ) );
}


void DXF_IMPORT_PLUGIN::addPoint( const DL_PointData& aData )
{
    MATRIX3x3D arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D   centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.x, aData.y, aData.z ) );
    VECTOR2D   center( mapX( centerCoords.x ), mapY( centerCoords.y ) );

    // we emulate points with filled circles
    // set the linewidth to something that even small circles look good with
    // thickness is optional for dxf points
    // note: we had to modify the dxf library to grab the attribute for thickness
    double lineWidth = 0.0001;
    double thickness = mapDim( std::max( aData.thickness, 0.01 ) );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddCircle( center, thickness, lineWidth, true );

    VECTOR2D radiusDelta( SCALE_FACTOR( thickness ), SCALE_FACTOR( thickness ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::insertLine( const VECTOR2D& aSegStart,
                                    const VECTOR2D& aSegEnd, double aWidth )
{
    VECTOR2D origin( SCALE_FACTOR( aSegStart.x ), SCALE_FACTOR( aSegStart.y ) );
    VECTOR2D end( SCALE_FACTOR( aSegEnd.x ), SCALE_FACTOR( aSegEnd.y ) );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddLine( origin, end, aWidth );

    updateImageLimits( origin );
    updateImageLimits( end );
}


void DXF_IMPORT_PLUGIN::insertArc( const VECTOR2D& aSegStart, const VECTOR2D& aSegEnd,
                                   double aBulge, double aWidth )
{
    VECTOR2D segment_startpoint( SCALE_FACTOR( aSegStart.x ), SCALE_FACTOR( aSegStart.y ) );
    VECTOR2D segment_endpoint( SCALE_FACTOR( aSegEnd.x ), SCALE_FACTOR( aSegEnd.y ) );

    // ensure aBulge represents an angle from +/- ( 0 .. approx 359.8 deg )
    if( aBulge < -2000.0 )
        aBulge = -2000.0;
    else if( aBulge > 2000.0 )
        aBulge = 2000.0;

    double ang = 4.0 * atan( aBulge );

    // reflect the Y values to put everything in a RHCS
    VECTOR2D sp( aSegStart.x, -aSegStart.y );
    VECTOR2D ep( aSegEnd.x, -aSegEnd.y );

    // angle from end->start
    double offAng = atan2( ep.y - sp.y, ep.x - sp.x );

    // length of subtended segment = 1/2 distance between the 2 points
    double d = 0.5 * sqrt( ( sp.x - ep.x ) * ( sp.x - ep.x ) + ( sp.y - ep.y ) * ( sp.y - ep.y ) );

    // midpoint of the subtended segment
    double xm   = ( sp.x + ep.x ) * 0.5;
    double ym   = ( sp.y + ep.y ) * 0.5;
    double radius = d / sin( ang * 0.5 );

    if( radius < 0.0 )
        radius = -radius;

    // calculate the height of the triangle with base d and hypotenuse r
    double dh2 = radius * radius - d * d;

    // this should only ever happen due to rounding errors when r == d
    if( dh2 < 0.0 )
        dh2 = 0.0;

    double h = sqrt( dh2 );

    if( ang < 0.0 )
        offAng -= M_PI_2;
    else
        offAng += M_PI_2;

    // for angles greater than 180 deg we need to flip the
    // direction in which the arc center is found relative
    // to the midpoint of the subtended segment.
    if( ang < -M_PI )
        offAng += M_PI;
    else if( ang > M_PI )
        offAng -= M_PI;

    // center point
    double cx = h * cos( offAng ) + xm;
    double cy = h * sin( offAng ) + ym;
    VECTOR2D center( SCALE_FACTOR( cx ), SCALE_FACTOR( -cy ) );
    VECTOR2D arc_start;
    EDA_ANGLE angle( ang, RADIANS_T );

    if( ang < 0.0 )
    {
        arc_start = VECTOR2D( SCALE_FACTOR( ep.x ), SCALE_FACTOR( -ep.y ) );
    }
    else
    {
        arc_start = VECTOR2D( SCALE_FACTOR( sp.x ), SCALE_FACTOR( -sp.y ) );
        angle = -angle;
    }

    GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                           : &m_internalImporter;
    bufferToUse->AddArc( center, arc_start, angle, aWidth );

    VECTOR2D radiusDelta( SCALE_FACTOR( radius ), SCALE_FACTOR( radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


#include "tinysplinecxx.h"

void DXF_IMPORT_PLUGIN::insertSpline( double aWidth )
{
#if 0   // Debug only
    wxLogMessage( "spl deg %d kn %d ctr %d fit %d",
                  m_curr_entity.m_SplineDegree,
                  m_curr_entity.m_SplineKnotsList.size(),
                  m_curr_entity.m_SplineControlPointList.size(),
                  m_curr_entity.m_SplineFitPointList.size() );
#endif

    unsigned imax = m_curr_entity.m_SplineControlPointList.size();

    if( imax < 2 )  // malformed spline
        return;

#if 0   // set to 1 to approximate the spline by segments between 2 control points
    VECTOR2D startpoint( mapX( m_curr_entity.m_SplineControlPointList[0].m_x ),
                         mapY( m_curr_entity.m_SplineControlPointList[0].m_y ) );

    for( unsigned int ii = 1; ii < imax; ++ii )
    {
        VECTOR2D endpoint( mapX( m_curr_entity.m_SplineControlPointList[ii].m_x ),
                           mapY( m_curr_entity.m_SplineControlPointList[ii].m_y ) );

        if( startpoint != endpoint )
        {
            m_internalImporter.AddLine( startpoint, endpoint, aWidth );

            updateImageLimits( startpoint );
            updateImageLimits( endpoint );

            startpoint = endpoint;
        }
    }
#else   // Use bezier curves, supported by pcbnew, to approximate the spline
    std::vector<double> ctrlp;

    for( unsigned ii = 0; ii < imax; ++ii )
    {
        ctrlp.push_back( m_curr_entity.m_SplineControlPointList[ii].m_x );
        ctrlp.push_back( m_curr_entity.m_SplineControlPointList[ii].m_y );
    }

    tinyspline::BSpline beziers;
    std::vector<double> coords;

    try
    {
	    tinyspline::BSpline dxfspline( m_curr_entity.m_SplineControlPointList.size(),
                                       /* coord dim */ 2, m_curr_entity.m_SplineDegree );

	    dxfspline.setControlPoints( ctrlp );
	    dxfspline.setKnots( m_curr_entity.m_SplineKnotsList );

        if( dxfspline.degree() < 3 )
            dxfspline = dxfspline.elevateDegree( 3 - dxfspline.degree() );

        beziers = dxfspline.toBeziers();
        coords = beziers.controlPoints();
    }
    catch( const std::runtime_error& )  // tinyspline throws everything including data validation
                                        // as runtime errors
    {
        // invalid spline definition, drop this block
        ReportMsg( _( "Invalid spline definition encountered" ) );
        return;
    }

    size_t order = beziers.order();
    size_t dim = beziers.dimension();
    size_t numBeziers = ( coords.size() / dim ) / order;

	for( size_t i = 0; i < numBeziers; i++ )
    {
        size_t ii = i * dim * order;
        VECTOR2D start( mapX( coords[ ii ] ), mapY( coords[ ii + 1 ] ) );
        VECTOR2D bezierControl1( mapX( coords[ii + 2] ), mapY( coords[ii + 3] ) );

        // not sure why this happens, but it seems to sometimes slip degree on the final bezier
        VECTOR2D bezierControl2;

        if( ii + 5 >= coords.size() )
            bezierControl2 = bezierControl1;
        else
            bezierControl2 = VECTOR2D( mapX( coords[ii + 4] ), mapY( coords[ii + 5] ) );

        VECTOR2D end;

        if( ii + 7 >= coords.size() )
            end = bezierControl2;
        else
            end = VECTOR2D( mapX( coords[ii + 6] ), mapY( coords[ii + 7] ) );

        GRAPHICS_IMPORTER_BUFFER* bufferToUse = m_currentBlock ? &m_currentBlock->m_buffer
                                                               : &m_internalImporter;
        bufferToUse->AddSpline( start, bezierControl1, bezierControl2, end, aWidth );
    }
#endif
}


void DXF_IMPORT_PLUGIN::updateImageLimits( const VECTOR2D& aPoint )
{
    m_minX = std::min( aPoint.x, m_minX );
    m_maxX = std::max( aPoint.x, m_maxX );

    m_minY = std::min( aPoint.y, m_minY );
    m_maxY = std::max( aPoint.y, m_maxY );
}


MATRIX3x3D DXF_IMPORT_PLUGIN::getArbitraryAxis( DL_Extrusion* aData )
{
    VECTOR3D arbZ, arbX, arbY;

    double direction[3];
    aData->getDirection( direction );

    arbZ = VECTOR3D( direction[0], direction[1], direction[2] ).Normalize();

    if( ( abs( arbZ.x ) < ( 1.0 / 64.0 ) ) && ( abs( arbZ.y ) < ( 1.0 / 64.0 ) ) )
        arbX = VECTOR3D( 0, 1, 0 ).Cross( arbZ ).Normalize();
    else
        arbX = VECTOR3D( 0, 0, 1 ).Cross( arbZ ).Normalize();

    arbY = arbZ.Cross( arbX ).Normalize();

    return MATRIX3x3D{ arbX, arbY, arbZ };
}


VECTOR3D DXF_IMPORT_PLUGIN::wcsToOcs( const MATRIX3x3D& arbitraryAxis, VECTOR3D point )
{
    return arbitraryAxis * point;
}


VECTOR3D DXF_IMPORT_PLUGIN::ocsToWcs( const MATRIX3x3D& arbitraryAxis, VECTOR3D point )
{
    VECTOR3D worldX = wcsToOcs( arbitraryAxis, VECTOR3D( 1, 0, 0 ) );
    VECTOR3D worldY = wcsToOcs( arbitraryAxis, VECTOR3D( 0, 1, 0 ) );
    VECTOR3D worldZ = wcsToOcs( arbitraryAxis, VECTOR3D( 0, 0, 1 ) );

    MATRIX3x3 world( worldX, worldY, worldZ );

    return world * point;
}
