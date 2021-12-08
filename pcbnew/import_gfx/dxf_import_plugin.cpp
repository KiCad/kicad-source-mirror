/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 *       DXF simply does mpt secifying text/font enough to make it portable.
 *       We however make do try to get it somewhat close/visually appealing.
 *    9. We silently drop the z coordinate on 3d polylines
 */


// minimum bulge value before resorting to a line segment;
// the value 0.0218 is equivalent to about 5 degrees arc,
#define MIN_BULGE 0.0218

//#define SCALE_FACTOR(x) millimeter2iu(x)  /* no longer used */
#define SCALE_FACTOR(x) (x)


DXF_IMPORT_PLUGIN::DXF_IMPORT_PLUGIN() : DL_CreationAdapter()
{
    m_xOffset   = 0.0;          // X coord offset for conversion (in mm)
    m_yOffset   = 0.0;          // Y coord offset for conversion (in mm)
    m_version   = 0;            // the dxf version, not yet used
    m_defaultThickness = 0.2;   // default thickness (in mm)
    m_brdLayer = Dwgs_User;     // The default import layer
    m_importAsFPShapes = true;
    m_minX = m_minY = std::numeric_limits<double>::max();
    m_maxX = m_maxY = std::numeric_limits<double>::min();
    m_currentUnit = DXF_IMPORT_UNITS::DEFAULT;
    m_importCoordinatePrecision = 4;    // initial value per dxf spec
    m_importAnglePrecision = 0;         // initial value per dxf spec

    // placeholder layer so we can fallback to something later
    std::unique_ptr<DXF_IMPORT_LAYER> layer0 =
            std::make_unique<DXF_IMPORT_LAYER>( "", DXF_IMPORT_LINEWEIGHT_BY_LW_DEFAULT );
    m_layers.push_back( std::move( layer0 ) );

    m_currentBlock = nullptr;
}


DXF_IMPORT_PLUGIN::~DXF_IMPORT_PLUGIN()
{
}


bool DXF_IMPORT_PLUGIN::Load( const wxString& aFileName )
{
    return ImportDxfFile( aFileName );
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
    FILE* fp = wxFopen( aFile, "rt" );

    if( fp == nullptr )
        return false;

    // Note the dxf reader takes care of switching to "C" locale before reading the file
    // and will close the file after reading
    bool success = dxf_reader.in( fp, this );

    return success;
}


void DXF_IMPORT_PLUGIN::reportMsg( const wxString& aMessage )
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
    m_curr_entity.m_SplineControlPointList.emplace_back( aData.x , aData.y,
                                                                         aData.w );
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
    {
        lw = DXF_IMPORT_LINEWEIGHT_BY_LW_DEFAULT;
    }

    std::unique_ptr<DXF_IMPORT_LAYER> layer = std::make_unique<DXF_IMPORT_LAYER>( name, lw );

    m_layers.push_back( std::move( layer ) );
}


void DXF_IMPORT_PLUGIN::addLinetype( const DL_LinetypeData& data )
{
#if 0
    wxString name = FROM_UTF8( data.name.c_str() );
    wxString description = FROM_UTF8( data.description.c_str() );
#endif
}


double DXF_IMPORT_PLUGIN::lineWeightToWidth( int lw, DXF_IMPORT_LAYER* aLayer )
{
    if( lw == DXF_IMPORT_LINEWEIGHT_BY_LAYER && aLayer != nullptr )
    {
        lw = aLayer->m_lineWeight;
    }

    // All lineweights >= 0 are always in 100ths of mm
    double mm = m_defaultThickness;

    if( lw >= 0 )
    {
        mm = lw / 100.0;
    }

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
                [styleName]( const auto& it ) { return it->m_name == styleName; } );

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

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
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

    DXF_ARBITRARY_AXIS arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D vertexCoords      = ocsToWcs( arbAxis, VECTOR3D( vertex->x, vertex->y, vertex->z ) );

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
                insertLine( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart,
                            lineWidth );
            else
                insertArc( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart,
                           m_curr_entity.m_BulgeVertex, lineWidth );
        }
    }

    if( m_curr_entity.m_EntityType == DL_ENTITY_SPLINE )
    {
        insertSpline( lineWidth );
    }

    m_curr_entity.Clear();
}


void DXF_IMPORT_PLUGIN::addBlock( const DL_BlockData& aData )
{
    wxString name = wxString::FromUTF8( aData.name.c_str() );

    std::unique_ptr<DXF_IMPORT_BLOCK> block =
            std::make_unique<DXF_IMPORT_BLOCK>( name, aData.bpx, aData.bpy );

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

    DXF_ARBITRARY_AXIS arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D insertCoords      = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );

    VECTOR2D translation( mapX( insertCoords.x ), mapY( insertCoords.y ) );
    translation -= VECTOR2D( mapX( block->m_baseX ), mapY( block->m_baseY ) );


    for( auto& shape : block->m_buffer.GetShapes() )
    {
        std::unique_ptr<IMPORTED_SHAPE> newShape = shape->clone();

        newShape->Translate( translation  );
        newShape->Scale( aData.sx, aData.sy );

        m_internalImporter.AddShape( newShape );
    }
}


void DXF_IMPORT_PLUGIN::addCircle( const DL_CircleData& aData )
{
    DXF_ARBITRARY_AXIS arbAxis      = getArbitraryAxis( getExtrusion() );
    VECTOR3D           centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.cx, aData.cy, aData.cz ) );

    VECTOR2D          center( mapX( centerCoords.x ), mapY( centerCoords.y ) );
    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
    bufferToUse->AddCircle( center, mapDim( aData.radius ), lineWidth, false );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addArc( const DL_ArcData& aData )
{
    DXF_ARBITRARY_AXIS arbAxis      = getArbitraryAxis( getExtrusion() );
    VECTOR3D           centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.cx, aData.cy, aData.cz ) );

    // Init arc centre:
    VECTOR2D center( mapX( centerCoords.x ), mapY( centerCoords.y ) );

    // aData.anglex is in degrees.
    double  startangle = aData.angle1;
    double  endangle = aData.angle2;

    // Init arc start point
    VECTOR2D startPoint( aData.radius, 0.0 );
    startPoint = startPoint.Rotate( startangle * M_PI / 180.0 );
    VECTOR2D arcStart(
            mapX( startPoint.x + centerCoords.x ), mapY( startPoint.y + centerCoords.y ) );

    // calculate arc angle (arcs are CCW, and should be < 0 in Pcbnew)
    double angle = -( endangle - startangle );

    if( angle > 0.0 )
        angle -= 360.0;

    DXF_IMPORT_LAYER* layer     = getImportLayer( attributes.getLayer() );
    double            lineWidth = lineWeightToWidth( attributes.getWidth(), layer );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
    bufferToUse->AddArc( center, arcStart, angle, lineWidth );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addText( const DL_TextData& aData )
{
    DXF_ARBITRARY_AXIS arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D refPointCoords    = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );
    VECTOR3D secPointCoords    = ocsToWcs( arbAxis, VECTOR3D( std::isnan( aData.apx ) ? 0 : aData.apx,
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
    double textThickness = textHeight/8.0;          // Use a reasonable line thickness for this text

    VECTOR2D bottomLeft(0.0, 0.0);
    VECTOR2D bottomRight(0.0, 0.0);
    VECTOR2D topLeft(0.0, 0.0);
    VECTOR2D topRight(0.0, 0.0);

    EDA_TEXT_HJUSTIFY_T hJustify = GR_TEXT_HJUSTIFY_LEFT;
    EDA_TEXT_VJUSTIFY_T vJustify = GR_TEXT_VJUSTIFY_BOTTOM;

    switch( aData.vJustification )
    {
    case 0: //DRW_Text::VBaseLine:
    case 1: //DRW_Text::VBottom:
        vJustify = GR_TEXT_VJUSTIFY_BOTTOM;

        topLeft.y = textHeight;
        topRight.y = textHeight;
        break;

    case 2: //DRW_Text::VMiddle:
        vJustify = GR_TEXT_VJUSTIFY_CENTER;

        bottomRight.y = -textHeight / 2.0;
        bottomLeft.y = -textHeight / 2.0;
        topLeft.y = textHeight / 2.0;
        topRight.y = textHeight / 2.0;
        break;

    case 3: //DRW_Text::VTop:
        vJustify = GR_TEXT_VJUSTIFY_TOP;

        bottomLeft.y = -textHeight;
        bottomRight.y = -textHeight;
        break;
    }

    switch( aData.hJustification )
    {
    case 0: //DRW_Text::HLeft:
    case 3: //DRW_Text::HAligned:    // no equivalent options in text pcb.
    case 5: //DRW_Text::HFit:       // no equivalent options in text pcb.
        hJustify = GR_TEXT_HJUSTIFY_LEFT;

        bottomRight.x = textWidth;
        topRight.x = textWidth;
        break;

    case 1: //DRW_Text::HCenter:
    case 4: //DRW_Text::HMiddle:     // no equivalent options in text pcb.
        hJustify = GR_TEXT_HJUSTIFY_CENTER;

        bottomLeft.x = -textWidth / 2.0;
        topLeft.x = -textWidth / 2.0;
        bottomRight.x = textWidth / 2.0;
        topRight.x = textWidth / 2.0;
        break;

    case 2: //DRW_Text::HRight:
        hJustify = GR_TEXT_HJUSTIFY_RIGHT;

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

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
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


void DXF_IMPORT_PLUGIN::addMText( const DL_MTextData& aData )
{
    wxString    text = toNativeString( wxString::FromUTF8( aData.text.c_str() ) );
    wxString    attrib, tmp;

    DXF_IMPORT_STYLE* style      = getImportStyle( aData.style.c_str() );

    double textHeight = mapDim( aData.height );

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

    /* Some texts start by '\' and have formatting chars (font name, font option...)
     *  ending with ';'
     *  Here are some mtext formatting codes:
     *  Format code        Purpose
     * \0...\o            Turns overline on and off
     *  \L...\l            Turns underline on and off
     * \~                 Inserts a nonbreaking space
     \\                 Inserts a backslash
     \\\{...\}            Inserts an opening and closing brace
     \\ \File name;        Changes to the specified font file
     \\ \Hvalue;           Changes to the text height specified in drawing units
     \\ \Hvaluex;          Changes the text height to a multiple of the current text height
     \\ \S...^...;         Stacks the subsequent text at the \, #, or ^ symbol
     \\ \Tvalue;           Adjusts the space between characters, from.75 to 4 times
     \\ \Qangle;           Changes oblique angle
     \\ \Wvalue;           Changes width factor to produce wide text
     \\ \A                 Sets the alignment value; valid values: 0, 1, 2 (bottom, center, top)    while( text.StartsWith( wxT("\\") ) )
     */
    while( text.StartsWith( wxT( "\\" ) ) )
    {
        attrib << text.BeforeFirst( ';' );
        tmp     = text.AfterFirst( ';' );
        text    = tmp;
    }

    DXF_ARBITRARY_AXIS arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D textposCoords     = ocsToWcs( arbAxis, VECTOR3D( aData.ipx, aData.ipy, aData.ipz ) );

    VECTOR2D textpos( mapX( textposCoords.x ), mapY( textposCoords.y ) );

    // Initialize text justifications:
    EDA_TEXT_HJUSTIFY_T hJustify = GR_TEXT_HJUSTIFY_LEFT;
    EDA_TEXT_VJUSTIFY_T vJustify = GR_TEXT_VJUSTIFY_BOTTOM;

    if( aData.attachmentPoint <= 3 )
    {
        vJustify = GR_TEXT_VJUSTIFY_TOP;

        bottomLeft.y = -textHeight;
        bottomRight.y = -textHeight;
    }
    else if( aData.attachmentPoint <= 6 )
    {
        vJustify = GR_TEXT_VJUSTIFY_CENTER;

        bottomRight.y = -textHeight / 2.0;
        bottomLeft.y = -textHeight / 2.0;
        topLeft.y = textHeight / 2.0;
        topRight.y = textHeight / 2.0;
    }
    else
    {
        vJustify = GR_TEXT_VJUSTIFY_BOTTOM;

        topLeft.y = textHeight;
        topRight.y = textHeight;
    }

    if( aData.attachmentPoint % 3 == 1 )
    {
        hJustify = GR_TEXT_HJUSTIFY_LEFT;

        bottomRight.x = textWidth;
        topRight.x = textWidth;
    }
    else if( aData.attachmentPoint % 3 == 2 )
    {
        hJustify = GR_TEXT_HJUSTIFY_CENTER;

        bottomLeft.x = -textWidth / 2.0;
        topLeft.x = -textWidth / 2.0;
        bottomRight.x = textWidth / 2.0;
        topRight.x = textWidth / 2.0;
    }
    else
    {
        hJustify = GR_TEXT_HJUSTIFY_RIGHT;

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


    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
    bufferToUse->AddText( textpos, text, textHeight, charWidth,
                          textThickness, angle_degree, hJustify, vJustify );

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

}


double DXF_IMPORT_PLUGIN::getCurrentUnitScale()
{
    double scale = 1.0;
    switch( m_currentUnit )
    {
    case DXF_IMPORT_UNITS::INCHES:
        scale = 25.4;
        break;

    case DXF_IMPORT_UNITS::FEET:
        scale = 304.8;
        break;

    case DXF_IMPORT_UNITS::MILLIMETERS:
        scale = 1.0;
        break;

    case DXF_IMPORT_UNITS::CENTIMETERS:
        scale = 10.0;
        break;

    case DXF_IMPORT_UNITS::METERS:
        scale = 1000.0;
        break;

    case DXF_IMPORT_UNITS::MICROINCHES:
        scale = 2.54e-5;
        break;

    case DXF_IMPORT_UNITS::MILS:
        scale = 0.0254;
        break;

    case DXF_IMPORT_UNITS::YARDS:
        scale = 914.4;
        break;

    case DXF_IMPORT_UNITS::ANGSTROMS:
        scale = 1.0e-7;
        break;

    case DXF_IMPORT_UNITS::NANOMETERS:
        scale = 1.0e-6;
        break;

    case DXF_IMPORT_UNITS::MICRONS:
        scale = 1.0e-3;
        break;

    case DXF_IMPORT_UNITS::DECIMETERS:
        scale = 100.0;
        break;

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
        scale = 1.0;
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
        switch( value )
        {
        case 1:     // inches
            m_currentUnit = DXF_IMPORT_UNITS::INCHES;
            break;

        case 2:     // feet
            m_currentUnit = DXF_IMPORT_UNITS::FEET;
            break;

        case 4:     // mm
            m_currentUnit = DXF_IMPORT_UNITS::MILLIMETERS;
            break;

        case 5:     // centimeters
            m_currentUnit = DXF_IMPORT_UNITS::CENTIMETERS;
            break;

        case 6:     // meters
            m_currentUnit = DXF_IMPORT_UNITS::METERS;
            break;

        case 8:     // microinches
            m_currentUnit = DXF_IMPORT_UNITS::MICROINCHES;
            break;

        case 9:     // mils
            m_currentUnit = DXF_IMPORT_UNITS::MILS;
            break;

        case 10:    // yards
            m_currentUnit = DXF_IMPORT_UNITS::YARDS;
            break;

        case 11:    // Angstroms
            m_currentUnit = DXF_IMPORT_UNITS::ANGSTROMS;
            break;

        case 12:    // nanometers
            m_currentUnit = DXF_IMPORT_UNITS::NANOMETERS;
            break;

        case 13:    // micrometers
            m_currentUnit = DXF_IMPORT_UNITS::MICRONS;
            break;

        case 14:    // decimeters
            m_currentUnit = DXF_IMPORT_UNITS::DECIMETERS;
            break;

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
            m_currentUnit = DXF_IMPORT_UNITS::DEFAULT;
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
    wxString    res;

    // Ignore font tags:
    int         j = 0;

    for( unsigned i = 0; i < aData.length(); ++i )
    {
        if( aData[ i ] == 0x7B )                                     // is '{' ?
        {
            if( aData[ i + 1 ] == 0x5c && aData[ i + 2 ] == 0x66 )    // is "\f" ?
            {
                // found font tag, append parsed part
                res.append( aData.Mid( j, i - j ) );

                // skip to ';'
                for( unsigned k = i + 3; k < aData.length(); ++k )
                {
                    if( aData[ k ] == 0x3B )
                    {
                        i = j = ++k;
                        break;
                    }
                }

                // add to '}'
                for( unsigned k = i; k < aData.length(); ++k )
                {
                    if( aData[ k ] == 0x7D )
                    {
                        res.append( aData.Mid( i, k - i ) );
                        i = j = ++k;
                        break;
                    }
                }
            }
        }
    }

    res.append( aData.Mid( j ) );

#if 1
    wxRegEx regexp;
    // Line feed:
    regexp.Compile( wxT( "\\\\P" ) );
    regexp.Replace( &res, wxT( "\n" ) );

    // Space:
    regexp.Compile( wxT( "\\\\~" ) );
    regexp.Replace( &res, wxT( " " ) );

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

    std::unique_ptr<DXF_IMPORT_STYLE> style =
            std::make_unique<DXF_IMPORT_STYLE>( name, aData.fixedTextHeight, aData.widthFactor, aData.bold, aData.italic );

    m_styles.push_back( std::move( style ) );
}


void DXF_IMPORT_PLUGIN::addPoint( const DL_PointData& aData )
{
    DXF_ARBITRARY_AXIS arbAxis = getArbitraryAxis( getExtrusion() );
    VECTOR3D           centerCoords = ocsToWcs( arbAxis, VECTOR3D( aData.x, aData.y, aData.z ) );

    VECTOR2D          center( mapX( centerCoords.x ), mapY( centerCoords.y ) );

    // we emulate points with filled circles
    // set the linewidth to something that even small circles look good with
    // thickness is optional for dxf points
    // note: we had to modify the dxf library to grab the attribute for thickness
    double lineWidth = 0.0001;
    double thickness = mapDim( std::max( aData.thickness, 0.01 ) );

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
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

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
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
    double d = 0.5 * sqrt( (sp.x - ep.x) * (sp.x - ep.x) + (sp.y - ep.y) * (sp.y - ep.y) );
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
    double angle = RAD2DEG( ang );

    if( ang < 0.0 )
    {
        arc_start = VECTOR2D( SCALE_FACTOR( ep.x ), SCALE_FACTOR( -ep.y ) );
    }
    else
    {
        arc_start = VECTOR2D( SCALE_FACTOR( sp.x ), SCALE_FACTOR( -sp.y ) );
        angle = -angle;
    }

    GRAPHICS_IMPORTER_BUFFER* bufferToUse =
            ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
    bufferToUse->AddArc( center, arc_start, angle, aWidth );

    VECTOR2D radiusDelta( SCALE_FACTOR( radius ), SCALE_FACTOR( radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
    return;
}


#include "tinysplinecpp.h"

void DXF_IMPORT_PLUGIN::insertSpline( double aWidth )
{
    #if 0   // Debug only
    wxLogMessage("spl deg %d kn %d ctr %d fit %d",
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

    std::vector<double> coords;
    try
    {
	    tinyspline::BSpline dxfspline( m_curr_entity.m_SplineControlPointList.size(),
                                       /* coord dim */ 2, m_curr_entity.m_SplineDegree );

	    dxfspline.setCtrlp( ctrlp );
	    dxfspline.setKnots( m_curr_entity.m_SplineKnotsList );
	    tinyspline::BSpline beziers( dxfspline.toBeziers() );

        coords = beziers.ctrlp();
    }
    catch( const std::runtime_error& )  //tinyspline throws everything including data validation as runtime errors
    {
        // invalid spline definition, drop this block
        reportMsg( _( "Invalid spline definition encountered" ) );
        return;
    }

    if( coords.size() % 8 != 0 )
    {
        // somehow we generated a bad Bezier curve
        reportMsg( _( "Invalid Bezier curve created" ) );
        return;
    }

    // Each Bezier curve uses 4 vertices (a start point, 2 control points and a end point).
    // So we can have more than one Bezier curve ( there are one curve each four vertices)
    // However, one can have one Bezier curve with end point = ctrl point 2, having only 3
    // defined points in list.
    for( unsigned ii = 0; ii < coords.size(); ii += 8 )
    {
        VECTOR2D start( mapX( coords[ii] ), mapY( coords[ii+1] ) );
        VECTOR2D bezierControl1( mapX( coords[ii+2] ), mapY( coords[ii+3] ) );
        VECTOR2D bezierControl2( mapX( coords[ii+4] ), mapY( coords[ii+5] ) );
        VECTOR2D end;

        if( ii+7 < coords.size() )
            end = VECTOR2D( mapX( coords[ii+6] ), mapY( coords[ii+7] ) );
        else
            end = bezierControl2;

        GRAPHICS_IMPORTER_BUFFER* bufferToUse =
                ( m_currentBlock != nullptr ) ? &m_currentBlock->m_buffer : &m_internalImporter;
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


DXF_ARBITRARY_AXIS DXF_IMPORT_PLUGIN::getArbitraryAxis( DL_Extrusion* aData )
{
    VECTOR3D arbZ, arbX, arbY;

    double direction[3];
    aData->getDirection( direction );

    arbZ = VECTOR3D( direction[0], direction[1], direction[2] ).Normalize();

    if( ( abs( arbZ.x ) < ( 1.0 / 64.0 ) ) && ( abs( arbZ.y ) < ( 1.0 / 64.0 ) ) )
    {
        arbX = VECTOR3D( 0, 1, 0 ).Cross( arbZ ).Normalize();
    }
    else
    {
        arbX = VECTOR3D( 0, 0, 1 ).Cross( arbZ ).Normalize();
    }

    arbY = arbZ.Cross( arbX ).Normalize();

    return DXF_ARBITRARY_AXIS{ arbX, arbY, arbZ };
}


VECTOR3D DXF_IMPORT_PLUGIN::wcsToOcs( const DXF_ARBITRARY_AXIS& arbitraryAxis, VECTOR3D point )
{
    double x, y, z;
    x = point.x * arbitraryAxis.vecX.x + point.y * arbitraryAxis.vecX.y
        + point.z * arbitraryAxis.vecX.z;
    y = point.x * arbitraryAxis.vecY.x + point.y * arbitraryAxis.vecY.y
        + point.z * arbitraryAxis.vecY.z;
    z = point.x * arbitraryAxis.vecZ.x + point.y * arbitraryAxis.vecZ.y
        + point.z * arbitraryAxis.vecZ.z;

    return VECTOR3D( x, y, z );
}


VECTOR3D DXF_IMPORT_PLUGIN::ocsToWcs( const DXF_ARBITRARY_AXIS& arbitraryAxis, VECTOR3D point )
{
    VECTOR3D worldX = wcsToOcs( arbitraryAxis, VECTOR3D( 1, 0, 0 ) );
    VECTOR3D worldY = wcsToOcs( arbitraryAxis, VECTOR3D( 0, 1, 0 ) );
    VECTOR3D worldZ = wcsToOcs( arbitraryAxis, VECTOR3D( 0, 0, 1 ) );

    double x, y, z;
    x = point.x * worldX.x + point.y * worldX.y + point.z * worldX.z;
    y = point.x * worldY.x + point.y * worldY.y + point.z * worldY.z;
    z = point.x * worldZ.x + point.y * worldZ.y + point.z * worldZ.z;

    return VECTOR3D( x, y, z );
}
