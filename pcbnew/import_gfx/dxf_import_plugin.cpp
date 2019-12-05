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
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include "common.h"


/*
 * Important note: all DXF coordinates and sizes are converted to mm.
 * they will be converted to internal units later.
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
    m_DXF2mm    = 1.0;          // The scale factor to convert DXF units to mm
    m_version   = 0;            // the dxf version, not yet used
    m_inBlock   = false;        // Discard blocks
    m_defaultThickness = 0.2;   // default thickness (in mm)
    m_brdLayer = Dwgs_User;     // The default import layer
    m_importAsfootprintGraphicItems = true;
    m_minX = m_minY = std::numeric_limits<double>::max();
    m_maxX = m_maxY = std::numeric_limits<double>::min();
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
    return SCALE_FACTOR( m_xOffset + ( aDxfCoordX * m_DXF2mm ) );
}


double DXF_IMPORT_PLUGIN::mapY( double aDxfCoordY )
{
    return SCALE_FACTOR( m_yOffset - ( aDxfCoordY * m_DXF2mm ) );
}


double DXF_IMPORT_PLUGIN::mapDim( double aDxfValue )
{
    return SCALE_FACTOR( aDxfValue * m_DXF2mm );
}


double DXF_IMPORT_PLUGIN::mapWidth( double aDxfWidth )
{
    // Always return the default line width
#if 0
    // mapWidth returns the aDxfValue if aDxfWidth > 0 m_defaultThickness
    if( aDxfWidth > 0.0 )
        return SCALE_FACTOR( aDxfWidth * m_DXF2mm );
#endif
    return  SCALE_FACTOR( m_defaultThickness );
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


void DXF_IMPORT_PLUGIN::reportMsg( const char* aMessage )
{
    // Add message to keep trace of not handled dxf entities
    m_messages += aMessage;
    m_messages += '\n';
}


void DXF_IMPORT_PLUGIN::addSpline( const DL_SplineData& aData )
{
    if( m_inBlock )
        return;

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
    if( m_inBlock )
        return;

    // Called for every spline control point, when reading a spline entity
    m_curr_entity.m_SplineControlPointList.emplace_back( aData.x , aData.y,
                                                                         aData.w );
}


void DXF_IMPORT_PLUGIN::addFitPoint( const DL_FitPointData& aData )
{
    if( m_inBlock )
        return;

    // Called for every spline fit point, when reading a spline entity
    // we store only the X,Y coord values in a VECTOR2D
    m_curr_entity.m_SplineFitPointList.emplace_back( aData.x, aData.y );
}


void DXF_IMPORT_PLUGIN::addKnot( const DL_KnotData& aData)
{
    if( m_inBlock )
        return;

    // Called for every spline knot value, when reading a spline entity
    m_curr_entity.m_SplineKnotsList.push_back( aData.k );
}


void DXF_IMPORT_PLUGIN::addLayer( const DL_LayerData& aData )
{
    // Not yet useful in Pcbnew.
#if 0
    wxString name = wxString::FromUTF8( aData.name.c_str() );
    wxLogMessage( name );
#endif
}


void DXF_IMPORT_PLUGIN::addLine( const DL_LineData& aData )
{
    if( m_inBlock )
        return;

    VECTOR2D start( mapX( aData.x1 ), mapY( aData.y1 ) );
    VECTOR2D end( mapX( aData.x2 ), mapY( aData.y2 ) );
    double lineWidth = mapWidth( attributes.getWidth() );

    m_internalImporter.AddLine( start, end, lineWidth );

    updateImageLimits( start );
    updateImageLimits( end );
}


void DXF_IMPORT_PLUGIN::addPolyline(const DL_PolylineData& aData )
{
    if( m_inBlock )
        return;

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
    if( m_inBlock )
        return;

    if( m_curr_entity.m_EntityParseStatus == 0 )
        return;     // Error

    double lineWidth = mapWidth( attributes.getWidth() );

    const DL_VertexData* vertex = &aData;

    if( m_curr_entity.m_EntityParseStatus == 1 )    // This is the first vertex of an entity
    {
        m_curr_entity.m_LastCoordinate.x = m_xOffset + vertex->x * m_DXF2mm;
        m_curr_entity.m_LastCoordinate.y = m_yOffset - vertex->y * m_DXF2mm;
        m_curr_entity.m_PolylineStart = m_curr_entity.m_LastCoordinate;
        m_curr_entity.m_BulgeVertex = vertex->bulge;
        m_curr_entity.m_EntityParseStatus = 2;
        return;
    }

    VECTOR2D seg_end( m_xOffset + vertex->x * m_DXF2mm,
                         m_yOffset - vertex->y * m_DXF2mm );

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
    if( m_curr_entity.m_EntityType == DL_ENTITY_POLYLINE ||
        m_curr_entity.m_EntityType == DL_ENTITY_LWPOLYLINE )
    {
        // Polyline flags bit 0 indicates closed (1) or open (0) polyline
        if( m_curr_entity.m_EntityFlag & 1 )
        {
            double lineWidth = mapWidth( attributes.getWidth() );

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
        double lineWidth = mapWidth( attributes.getWidth() );
        insertSpline( lineWidth );
    }

    m_curr_entity.Clear();
}


void DXF_IMPORT_PLUGIN::addBlock( const DL_BlockData& aData )
{
    // The DXF blocks are not useful in our import, so we skip them with the exception
    // of the main block that is shown when editing the file
    if( aData.name.compare( "*Model_Space") )
        m_inBlock = true;
}


void DXF_IMPORT_PLUGIN::endBlock()
{
    m_inBlock = false;
}


void DXF_IMPORT_PLUGIN::addCircle( const DL_CircleData& aData )
{
    if( m_inBlock )
        return;

    VECTOR2D center( mapX( aData.cx ), mapY( aData.cy ) );
    double lineWidth = mapWidth( attributes.getWidth() );
    m_internalImporter.AddCircle( center, mapDim( aData.radius ), lineWidth );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addArc( const DL_ArcData& aData )
{
    if( m_inBlock )
        return;

    // Init arc centre:
    VECTOR2D center( mapX( aData.cx ), mapY( aData.cy ) );

    // aData.anglex is in degrees.
    double  startangle = aData.angle1;
    double  endangle = aData.angle2;

    // Init arc start point
    VECTOR2D startPoint( aData.radius, 0.0 );
    startPoint = startPoint.Rotate( startangle * M_PI / 180.0 );
    VECTOR2D arcStart( mapX( startPoint.x + aData.cx ), mapY( startPoint.y + aData.cy ) );

    // calculate arc angle (arcs are CCW, and should be < 0 in Pcbnew)
    double angle = -( endangle - startangle );

    if( angle > 0.0 )
        angle -= 360.0;

    double lineWidth = mapWidth( attributes.getWidth() );
    m_internalImporter.AddArc( center, arcStart, angle, lineWidth );

    VECTOR2D radiusDelta( mapDim( aData.radius ), mapDim( aData.radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
}


void DXF_IMPORT_PLUGIN::addText( const DL_TextData& aData )
{
    if( m_inBlock )
        return;

    VECTOR2D refPoint( mapX( aData.ipx ), mapY( aData.ipy ) );
    VECTOR2D secPoint( mapX( aData.apx ), mapY( aData.apy ) );

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

    double textHeight = mapDim( aData.height );
    // The 0.9 factor gives a better height/width ratio with our font
    double charWidth = textHeight * 0.9;
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

    m_internalImporter.AddText( refPoint, text, textHeight, charWidth, textThickness,
                                angle_degree, hJustify, vJustify );

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
    if( m_inBlock )
        return;

    wxString    text = toNativeString( wxString::FromUTF8( aData.text.c_str() ) );
    wxString    attrib, tmp;

    double textHeight = mapDim( aData.height );
    // The 0.9 factor gives a better height/width ratio with our font
    double charWidth = textHeight * 0.9;
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

    VECTOR2D textpos( mapX( aData.ipx ), mapY( aData.ipy ) );

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

    m_internalImporter.AddText( textpos, text, textHeight, charWidth,
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


void DXF_IMPORT_PLUGIN::setVariableInt( const std::string& key, int value, int code )
{
    if( m_inBlock )
        return;

    // Called for every int variable in the DXF file (e.g. "$INSUNITS").

    if( key == "$DWGCODEPAGE" )
    {
        m_codePage = value;
        return;
    }

    if( key == "$INSUNITS" )    // Drawing units
    {
        switch( value )
        {
        case 1:     // inches
            m_DXF2mm = 25.4;
            break;

        case 2:     // feet
            m_DXF2mm = 304.8;
            break;

        case 4:     // mm
            m_DXF2mm = 1.0;
            break;

        case 5:     // centimeters
            m_DXF2mm = 10.0;
            break;

        case 6:     // meters
            m_DXF2mm = 1000.0;
            break;

        case 8:     // microinches
            m_DXF2mm = 2.54e-5;
            break;

        case 9:     // mils
            m_DXF2mm = 0.0254;
            break;

        case 10:    // yards
            m_DXF2mm = 914.4;
            break;

        case 11:    // Angstroms
            m_DXF2mm = 1.0e-7;
            break;

        case 12:    // nanometers
            m_DXF2mm = 1.0e-6;
            break;

        case 13:    // micrometers
            m_DXF2mm = 1.0e-3;
            break;

        case 14:    // decimeters
            m_DXF2mm = 100.0;
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
            m_DXF2mm = 1.0;
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
#ifdef __WINDOWS_
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
    // TODO
}


void DXF_IMPORT_PLUGIN::insertLine( const VECTOR2D& aSegStart,
                                    const VECTOR2D& aSegEnd, int aWidth )
{
    VECTOR2D origin( SCALE_FACTOR( aSegStart.x ), SCALE_FACTOR( aSegStart.y ) );
    VECTOR2D end( SCALE_FACTOR( aSegEnd.x ), SCALE_FACTOR( aSegEnd.y ) );
    m_internalImporter.AddLine( origin, end, aWidth );

    updateImageLimits( origin );
    updateImageLimits( end );
}


void DXF_IMPORT_PLUGIN::insertArc( const VECTOR2D& aSegStart, const VECTOR2D& aSegEnd,
                                   double aBulge, int aWidth )
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

    m_internalImporter.AddArc( center, arc_start, angle, aWidth );

    VECTOR2D radiusDelta( SCALE_FACTOR( radius ), SCALE_FACTOR( radius ) );

    updateImageLimits( center + radiusDelta );
    updateImageLimits( center - radiusDelta );
    return;
}


#include "tinyspline_lib/tinysplinecpp.h"

void DXF_IMPORT_PLUGIN::insertSpline( int aWidth )
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
            m_internalImporter.AddLine( startpoint, endpoint );

            updateImageLimits( startpoint );
            updateImageLimits( endpoint );

            startpoint = endpoint;
        }
    }
#else   // Use bezier curves, supported by pcbnew, to approximate the spline
	tinyspline::BSpline dxfspline( m_curr_entity.m_SplineControlPointList.size(),
                                   /* coord dim */ 2, m_curr_entity.m_SplineDegree );
    std::vector<double> ctrlp;

    for( unsigned ii = 0; ii < imax; ++ii )
    {
        ctrlp.push_back( m_curr_entity.m_SplineControlPointList[ii].m_x );
        ctrlp.push_back( m_curr_entity.m_SplineControlPointList[ii].m_y );
    }

	dxfspline.setCtrlp( ctrlp );
	dxfspline.setKnots( m_curr_entity.m_SplineKnotsList );
	tinyspline::BSpline beziers( dxfspline.toBeziers() );

    std::vector<double> coords = beziers.ctrlp();

    // Each Bezier curve uses 4 vertices (a start point, 2 control points and a end point).
    // So we can have more than one Bezier curve ( there are one curve each four vertices)
    for( unsigned ii = 0; ii < coords.size(); ii += 8 )
    {
        VECTOR2D start( mapX( coords[ii] ), mapY( coords[ii+1] ) );
        VECTOR2D bezierControl1( mapX( coords[ii+2] ), mapY( coords[ii+3] ) );
        VECTOR2D bezierControl2( mapX( coords[ii+4] ), mapY( coords[ii+5] ) );
        VECTOR2D end( mapX( coords[ii+6] ), mapY( coords[ii+7] ) );
        m_internalImporter.AddSpline( start, bezierControl1, bezierControl2, end , aWidth );
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
