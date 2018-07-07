/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

// The DXF reader lib (libdxfrw) comes from LibreCAD project, a 2D CAD program
// libdxfrw can be found on http://sourceforge.net/projects/libdxfrw/
// or (latest sources) on
// https://github.com/LibreCAD/LibreCAD/tree/master/libraries/libdxfrw/src
//
// There is no doc to use it, but have a look to
// https://github.com/LibreCAD/LibreCAD/blob/master/librecad/src/lib/filters/rs_filterdxf.cpp
// and https://github.com/LibreCAD/LibreCAD/blob/master/librecad/src/lib/filters/rs_filterdxf.h
// Each time a dxf entity is read, a "call back" fuction is called
// like void DXF2BRD_CONVERTER::addLine( const DRW_Line& data ) when a line is read.
// this function just add the BOARD entity from dxf parameters (start and end point ...)


#include "dxf2brd_items.h"
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

// minimum bulge value before resorting to a line segment;
// the value 0.0218 is equivalent to about 5 degrees arc,
#define MIN_BULGE 0.0218

DXF2BRD_CONVERTER::DXF2BRD_CONVERTER() : DL_CreationAdapter()
{
    m_xOffset   = 0.0;          // X coord offset for conversion (in mm)
    m_yOffset   = 0.0;          // Y coord offset for conversion (in mm)
    m_DXF2mm    = 1.0;          // The scale factor to convert DXF units to mm
    m_version   = 0;            // the dxf version, not yet used
    m_defaultThickness = 0.2;   // default thickness (in mm)
    m_brdLayer = Dwgs_User;     // The default import layer
    m_importAsfootprintGraphicItems = true;
}


DXF2BRD_CONVERTER::~DXF2BRD_CONVERTER()
{
}


// coordinate conversions from dxf to internal units
int DXF2BRD_CONVERTER::mapX( double aDxfCoordX )
{
    return Millimeter2iu( m_xOffset + ( aDxfCoordX * m_DXF2mm ) );
}


int DXF2BRD_CONVERTER::mapY( double aDxfCoordY )
{
    return Millimeter2iu( m_yOffset - ( aDxfCoordY * m_DXF2mm ) );
}


int DXF2BRD_CONVERTER::mapDim( double aDxfValue )
{
    return Millimeter2iu( aDxfValue * m_DXF2mm );
}


int DXF2BRD_CONVERTER::mapWidth( double aDxfWidth )
{
    // Always return the default line width
#if 0
    // mapWidth returns the aDxfValue if aDxfWidth > 0 m_defaultThickness
    if( aDxfWidth > 0.0 )
        return Millimeter2iu( aDxfWidth * m_DXF2mm );
#endif
    return  Millimeter2iu( m_defaultThickness );
}

bool DXF2BRD_CONVERTER::ImportDxfFile( const wxString& aFile )
{
    LOCALE_IO locale;

    DL_Dxf dxf_reader;
    std::string filename = TO_UTF8( aFile );
    bool success = true;

    if( !dxf_reader.in( filename, this ) )  // if file open failed
        success = false;

    return success;
}


void DXF2BRD_CONVERTER::reportMsg( const char* aMessage )
{
    // Add message to keep trace of not handled dxf entities
    m_messages += aMessage;
    m_messages += '\n';
}


void DXF2BRD_CONVERTER::addSpline( const DL_SplineData& aData )
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


void DXF2BRD_CONVERTER::addControlPoint( const DL_ControlPointData& aData )
{
    // Called for every spline control point, when reading a spline entity
    m_curr_entity.m_SplineControlPointList.push_back( SPLINE_CTRL_POINT( aData.x , aData.y, aData.w ) );
}

void DXF2BRD_CONVERTER::addFitPoint( const DL_FitPointData& aData )
{
    // Called for every spline fit point, when reading a spline entity
    // we store only the X,Y coord values in a wxRealPoint
    m_curr_entity.m_SplineFitPointList.push_back( wxRealPoint( aData.x, aData.y ) );
}


void DXF2BRD_CONVERTER::addKnot( const DL_KnotData& aData)
{
    // Called for every spline knot value, when reading a spline entity
    m_curr_entity.m_SplineKnotsList.push_back( aData.k );
}


void DXF2BRD_CONVERTER::addLayer( const DL_LayerData& aData )
{
    // Not yet useful in Pcbnew.
#if 0
    wxString name = wxString::FromUTF8( aData.name.c_str() );
    wxLogMessage( name );
#endif
}


void DXF2BRD_CONVERTER::addLine( const DL_LineData& aData )
{
    DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                        static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) : new DRAWSEGMENT;

    segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
    wxPoint start( mapX( aData.x1 ), mapY( aData.y1 ) );
    segm->SetStart( start );
    wxPoint end( mapX( aData.x2 ), mapY( aData.y2 ) );
    segm->SetEnd( end );
    segm->SetWidth( mapWidth( attributes.getWidth() ) );
    m_newItemsList.push_back( segm );
}


void DXF2BRD_CONVERTER::addPolyline(const DL_PolylineData& aData )
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


void DXF2BRD_CONVERTER::addVertex( const DL_VertexData& aData )
{
    if( m_curr_entity.m_EntityParseStatus == 0 )
        return;     // Error

    int lineWidth = mapWidth( attributes.getWidth() );

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


    wxRealPoint seg_end( m_xOffset + vertex->x * m_DXF2mm,
                         m_yOffset - vertex->y * m_DXF2mm );

    if( std::abs( m_curr_entity.m_BulgeVertex ) < MIN_BULGE )
        insertLine( m_curr_entity.m_LastCoordinate, seg_end, lineWidth );
    else
        insertArc( m_curr_entity.m_LastCoordinate, seg_end, m_curr_entity.m_BulgeVertex, lineWidth );

    m_curr_entity.m_LastCoordinate = seg_end;
    m_curr_entity.m_BulgeVertex = vertex->bulge;
}


void DXF2BRD_CONVERTER::endEntity()
{
    if( m_curr_entity.m_EntityType == DL_ENTITY_POLYLINE ||
        m_curr_entity.m_EntityType == DL_ENTITY_LWPOLYLINE )
    {
        // Polyline flags bit 0 indicates closed (1) or open (0) polyline
        if( m_curr_entity.m_EntityFlag & 1 )
        {
            int lineWidth = mapWidth( attributes.getWidth() );

            if( std::abs( m_curr_entity.m_BulgeVertex ) < MIN_BULGE )
                insertLine( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart, lineWidth );
            else
                insertArc( m_curr_entity.m_LastCoordinate, m_curr_entity.m_PolylineStart,
                           m_curr_entity.m_BulgeVertex, lineWidth );
        }
    }

    if( m_curr_entity.m_EntityType == DL_ENTITY_SPLINE )
    {
        int lineWidth = mapWidth( attributes.getWidth() );
        insertSpline( lineWidth );
    }

    m_curr_entity.Clear();
}


void DXF2BRD_CONVERTER::addCircle( const DL_CircleData& aData )
{
    DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                        static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) : new DRAWSEGMENT;

    segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
    segm->SetShape( S_CIRCLE );
    wxPoint center( mapX( aData.cx ), mapY( aData.cy ) );
    segm->SetCenter( center );
    wxPoint circle_start( mapX( aData.cx + aData.radius ), mapY( aData.cy ) );
    segm->SetArcStart( circle_start );
    segm->SetWidth( mapWidth( attributes.getWidth() ) );
    m_newItemsList.push_back( segm );
}


/*
 * Import Arc entities.
 */
void DXF2BRD_CONVERTER::addArc( const DL_ArcData& data )
{
    DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                        static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) : new DRAWSEGMENT;

    segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
    segm->SetShape( S_ARC );

    // Init arc centre:
    wxPoint center( mapX( data.cx ), mapY( data.cy ) );
    segm->SetCenter( center );

    // Init arc start point
    double  arcStartx   = data.radius;
    double  arcStarty   = 0;
    double  startangle = data.angle1;
    double  endangle = data.angle2;

    RotatePoint( &arcStartx, &arcStarty, -RAD2DECIDEG( startangle ) );
    wxPoint arcStart( mapX( arcStartx + data.cx ),
                      mapY( arcStarty + data.cy ) );
    segm->SetArcStart( arcStart );

    // calculate arc angle (arcs are CCW, and should be < 0 in Pcbnew)
    double angle = -RAD2DECIDEG( endangle - startangle );

    if( angle > 0.0 )
        angle -= 3600.0;

    segm->SetAngle( angle );

    segm->SetWidth( mapWidth( attributes.getWidth() ) );
    m_newItemsList.push_back( segm );
}


void DXF2BRD_CONVERTER::addText( const DL_TextData& aData )
{
    BOARD_ITEM* brdItem;
    EDA_TEXT* textItem;

    if( m_importAsfootprintGraphicItems )
    {
        TEXTE_MODULE* modText = new TEXTE_MODULE( NULL );
        brdItem = static_cast< BOARD_ITEM* >( modText );
        textItem = static_cast< EDA_TEXT* >( modText );
    }
    else
    {
        TEXTE_PCB* pcbText = new TEXTE_PCB( NULL );
        brdItem = static_cast< BOARD_ITEM* >( pcbText );
        textItem = static_cast< EDA_TEXT* >( pcbText );
    }

    brdItem->SetLayer( ToLAYER_ID( m_brdLayer ) );

    wxPoint refPoint( mapX( aData.ipx ), mapY( aData.ipy ) );
    wxPoint secPoint( mapX( aData.apx ), mapY( aData.apy ) );

    if( aData.vJustification != 0 || aData.hJustification != 0 || aData.hJustification == 4 )
    {
        if( aData.hJustification != 3 && aData.hJustification != 5 )
        {
            wxPoint tmp = secPoint;
            secPoint = refPoint;
            refPoint = tmp;
        }
    }

    switch( aData.vJustification )
    {
    case 0: //DRW_Text::VBaseLine:
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case 1: //DRW_Text::VBottom:
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        break;

    case 2: //DRW_Text::VMiddle:
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        break;

    case 3: //DRW_Text::VTop:
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        break;
    }

    switch( aData.hJustification )
    {
    case 0: //DRW_Text::HLeft:
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case 1: //DRW_Text::HCenter:
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case 2: //DRW_Text::HRight:
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case 3: //DRW_Text::HAligned:
        // no equivalent options in text pcb.
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case 4: //DRW_Text::HMiddle:
        // no equivalent options in text pcb.
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case 5: //DRW_Text::HFit:
        // no equivalent options in text pcb.
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
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

    wxString text = toNativeString( wxString::FromUTF8( aData.text.c_str() ) );

    textItem->SetTextPos( refPoint );
    textItem->SetTextAngle( aData.angle * 10 );

    // The 0.8 factor gives a better height/width ratio with our font
    textItem->SetTextWidth( mapDim( aData.height * 0.8 ) );
    textItem->SetTextHeight( mapDim( aData.height ) );
    textItem->SetThickness( mapWidth( aData.height * 0.15 ) );  // Gives a reasonable text thickness
    textItem->SetText( text );

    m_newItemsList.push_back( static_cast< BOARD_ITEM* >( brdItem ) );
}


void DXF2BRD_CONVERTER::addMText( const DL_MTextData& aData )
{
    wxString    text = toNativeString( wxString::FromUTF8( aData.text.c_str() ) );
    wxString    attrib, tmp;

    /* Some texts start by '\' and have formating chars (font name, font option...)
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
     \\ \Qangle;           Changes obliquing angle
     \\ \Wvalue;           Changes width factor to produce wide text
     \\ \A                 Sets the alignment value; valid values: 0, 1, 2 (bottom, center, top)    while( text.StartsWith( wxT("\\") ) )
     */
    while( text.StartsWith( wxT( "\\" ) ) )
    {
        attrib << text.BeforeFirst( ';' );
        tmp     = text.AfterFirst( ';' );
        text    = tmp;
    }

    BOARD_ITEM* brdItem;
    EDA_TEXT* textItem;

    if( m_importAsfootprintGraphicItems )
    {
        TEXTE_MODULE* modText = new TEXTE_MODULE( NULL );
        brdItem = static_cast< BOARD_ITEM* >( modText );
        textItem = static_cast< EDA_TEXT* >( modText );
    }
    else
    {
        TEXTE_PCB* pcbText = new TEXTE_PCB( NULL );
        brdItem = static_cast< BOARD_ITEM* >( pcbText );
        textItem = static_cast< EDA_TEXT* >( pcbText );
    }

    brdItem->SetLayer( ToLAYER_ID( m_brdLayer ) );
    wxPoint textpos( mapX( aData.ipx ), mapY( aData.ipy ) );

    textItem->SetTextPos( textpos );
    textItem->SetTextAngle( aData.angle * 10 );

    // The 0.8 factor gives a better height/width ratio with our font
    textItem->SetTextWidth( mapDim( aData.height * 0.8 ) );
    textItem->SetTextHeight( mapDim( aData.height ) );
    textItem->SetThickness( mapWidth( aData.height * 0.15 ) );  // Gives a reasonable text thickness
    textItem->SetText( text );

    // Initialize text justifications:
    if( aData.attachmentPoint <= 3 )
    {
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
    }
    else if( aData.attachmentPoint <= 6 )
    {
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
    }
    else
    {
        textItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
    }

    if( aData.attachmentPoint % 3 == 1 )
    {
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
    }
    else if( aData.attachmentPoint % 3 == 2 )
    {
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
    }
    else
    {
        textItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
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

    m_newItemsList.push_back( static_cast< BOARD_ITEM* >( brdItem ) );
}


void DXF2BRD_CONVERTER::setVariableInt( const std::string& key, int value, int code )
{
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


void DXF2BRD_CONVERTER::setVariableString( const std::string& key, const std::string& value,
        int code )
{
    // Called for every string variable in the DXF file (e.g. "$ACADVER").
}


wxString DXF2BRD_CONVERTER::toDxfString( const wxString& aStr )
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


wxString DXF2BRD_CONVERTER::toNativeString( const wxString& aData )
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


void DXF2BRD_CONVERTER::addTextStyle( const DL_StyleData& aData )
{
    // TODO
}


void DXF2BRD_CONVERTER::insertLine( const wxRealPoint& aSegStart,
                                    const wxRealPoint& aSegEnd, int aWidth )
{
    DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                        static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) : new DRAWSEGMENT;
    wxPoint segment_startpoint( Millimeter2iu( aSegStart.x ), Millimeter2iu( aSegStart.y ) );
    wxPoint segment_endpoint( Millimeter2iu( aSegEnd.x ), Millimeter2iu( aSegEnd.y ) );

    segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
    segm->SetStart( segment_startpoint );
    segm->SetEnd( segment_endpoint );
    segm->SetWidth( aWidth );

    m_newItemsList.push_back( segm );
}


void DXF2BRD_CONVERTER::insertArc( const wxRealPoint& aSegStart, const wxRealPoint& aSegEnd,
                                   double aBulge, int aWidth )
{
    DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                        static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) : new DRAWSEGMENT;

    wxPoint segment_startpoint( Millimeter2iu( aSegStart.x ), Millimeter2iu( aSegStart.y ) );
    wxPoint segment_endpoint( Millimeter2iu( aSegEnd.x ), Millimeter2iu( aSegEnd.y ) );

    // ensure aBulge represents an angle from +/- ( 0 .. approx 359.8 deg )
    if( aBulge < -2000.0 )
        aBulge = -2000.0;
    else if( aBulge > 2000.0 )
        aBulge = 2000.0;

    double ang = 4.0 * atan( aBulge );

    // reflect the Y values to put everything in a RHCS
    wxRealPoint sp( aSegStart.x, -aSegStart.y );
    wxRealPoint ep( aSegEnd.x, -aSegEnd.y );
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

    segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
    segm->SetShape( S_ARC );
    segm->SetCenter( wxPoint( Millimeter2iu( cx ), Millimeter2iu( -cy ) ) );

    if( ang < 0.0 )
    {
        segm->SetArcStart( wxPoint( Millimeter2iu( ep.x ), Millimeter2iu( -ep.y ) ) );
        segm->SetAngle( RAD2DECIDEG( ang ) );
    }
    else
    {
        segm->SetArcStart( wxPoint( Millimeter2iu( sp.x ), Millimeter2iu( -sp.y ) ) );
        segm->SetAngle( RAD2DECIDEG( -ang ) );
    }

    segm->SetWidth( aWidth );

    m_newItemsList.push_back( segm );
    return;
}


#include "tinyspline_lib/tinysplinecpp.h"

void DXF2BRD_CONVERTER::insertSpline( int aWidth )
{
    #if 0   // Debug only
    wxLogMessage("spl deg %d kn %d ctr %d fit %d",
         m_curr_entity.m_SplineDegree,
         m_curr_entity.m_SplineKnotsList.size(),
         m_curr_entity.m_SplineControlPointList.size(),
         m_curr_entity.m_SplineFitPointList.size() );
    #endif

    // Very basic conversion to segments
    unsigned imax = m_curr_entity.m_SplineControlPointList.size();

    if( imax < 2 )  // malformed spline
        return;

#if 0   // set to 1 to approximate the spline by segments between 2 control points
    wxPoint startpoint( mapX( m_curr_entity.m_SplineControlPointList[0].m_x ),
                        mapY( m_curr_entity.m_SplineControlPointList[0].m_y ) );

    for( unsigned int ii = 1; ii < imax; ++ii )
    {
        wxPoint endpoint( mapX( m_curr_entity.m_SplineControlPointList[ii].m_x ),
                          mapY( m_curr_entity.m_SplineControlPointList[ii].m_y ) );

        if( startpoint != endpoint )
        {
            DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                                    static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) :
                                    new DRAWSEGMENT;
            segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
            segm->SetStart( startpoint );
            segm->SetEnd( endpoint );
            segm->SetWidth( aWidth );
            m_newItemsList.push_back( segm );
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
        DRAWSEGMENT* segm = ( m_importAsfootprintGraphicItems ) ?
                                static_cast< DRAWSEGMENT* >( new EDGE_MODULE( NULL ) ) :
                                new DRAWSEGMENT;
        segm->SetLayer( ToLAYER_ID( m_brdLayer ) );
        segm->SetShape( S_CURVE );
        segm->SetStart( wxPoint( mapX( coords[ii] ), mapY( coords[ii+1] ) ) );
        segm->SetBezControl1( wxPoint( mapX( coords[ii+2] ), mapY( coords[ii+3] ) ) );
        segm->SetBezControl2( wxPoint( mapX( coords[ii+4] ), mapY( coords[ii+5] ) ) );
        segm->SetEnd( wxPoint( mapX( coords[ii+6] ), mapY( coords[ii+7] ) ) );
        segm->SetWidth( aWidth );
        segm->RebuildBezierToSegmentsPointsList( aWidth );
        m_newItemsList.push_back( segm );
    }
#endif
}

