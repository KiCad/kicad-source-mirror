/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file GERBER_plotter.cpp
 * @brief specialized plotter for GERBER files format
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <trigo.h>
#include <eda_base_frame.h>
#include <base_struct.h>
#include <common.h>
#include <plotter.h>
#include <macros.h>
#include <kicad_string.h>
#include <convert_basic_shapes_to_polygon.h>
#include <math/util.h>      // for KiROUND

#include <build_version.h>

#include <gbr_metadata.h>


GERBER_PLOTTER::GERBER_PLOTTER()
{
    workFile  = NULL;
    finalFile = NULL;
    m_currentApertureIdx = -1;
    m_apertureAttribute = 0;

    // number of digits after the point (number of digits of the mantissa
    // Be carefull: the Gerber coordinates are stored in an integer
    // so 6 digits (inches) or 5 digits (mm) is a good value
    // To avoid overflow, 7 digits (inches) or 6 digits is a max.
    // with lower values than 6 digits (inches) or 5 digits (mm),
    // Creating self-intersecting polygons from non-intersecting polygons
    // happen easily.
    m_gerberUnitInch = false;
    m_gerberUnitFmt = 6;
    m_useX2format = true;
    m_useNetAttributes = true;
}


void GERBER_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror )
{
    wxASSERT( aMirror == false );
    m_plotMirror = false;
    plotOffset = aOffset;
    wxASSERT( aScale == 1 );    // aScale parameter is not used in Gerber
    plotScale = 1;              // Plot scale is *always* 1.0

    m_IUsPerDecimil = aIusPerDecimil;
    // gives now a default value to iuPerDeviceUnit (because the units of the caller is now known)
    // which could be modified later by calling SetGerberCoordinatesFormat()
    iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    // We don't handle the filmbox, and it's more useful to keep the
    // origin at the origin
    paperSize.x = 0;
    paperSize.y = 0;
}


void GERBER_PLOTTER::SetGerberCoordinatesFormat( int aResolution, bool aUseInches )
{
    m_gerberUnitInch = aUseInches;
    m_gerberUnitFmt = aResolution;

    iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    if( ! m_gerberUnitInch )
        iuPerDeviceUnit *= 25.4;     // gerber output in mm
}


void GERBER_PLOTTER::emitDcode( const DPOINT& pt, int dcode )
{

    fprintf( outputFile, "X%dY%dD%02d*\n", KiROUND( pt.x ), KiROUND( pt.y ), dcode );
}

void GERBER_PLOTTER::ClearAllAttributes()
{
    // Remove all attributes from object attributes dictionary (TO. and TA commands)
    if( m_useX2format )
        fputs( "%TD*%\n", outputFile );
    else
        fputs( "G04 #@! TD*\n", outputFile );

    m_objectAttributesDictionnary.clear();
}


void GERBER_PLOTTER::clearNetAttribute()
{
    // disable a Gerber net attribute (exists only in X2 with net attributes mode).
    if( m_objectAttributesDictionnary.empty() )     // No net attribute or not X2 mode
        return;

    // Remove all net attributes from object attributes dictionary
    if( m_useX2format )
        fputs( "%TD*%\n", outputFile );
    else
        fputs( "G04 #@! TD*\n", outputFile );

    m_objectAttributesDictionnary.clear();
}


void GERBER_PLOTTER::StartBlock( void* aData )
{
    // Currently, it is the same as EndBlock(): clear all aperture net attributes
    EndBlock( aData );
}


void GERBER_PLOTTER::EndBlock( void* aData )
{
    // Remove all net attributes from object attributes dictionary
    clearNetAttribute();
}


void GERBER_PLOTTER::formatNetAttribute( GBR_NETLIST_METADATA* aData )
{
    // print a Gerber net attribute record.
    // it is added to the object attributes dictionary
    // On file, only modified or new attributes are printed.
    if( aData == NULL )
        return;

    if( !m_useNetAttributes )
        return;

    bool useX1StructuredComment = !m_useX2format;

    bool clearDict;
    std::string short_attribute_string;

    if( !FormatNetAttribute( short_attribute_string, m_objectAttributesDictionnary,
                        aData, clearDict, useX1StructuredComment ) )
        return;

    if( clearDict )
        clearNetAttribute();

    if( !short_attribute_string.empty() )
        fputs( short_attribute_string.c_str(), outputFile );

    if( m_useX2format && !aData->m_ExtraData.IsEmpty() )
    {
        std::string extra_data = TO_UTF8( aData->m_ExtraData );
        fputs( extra_data.c_str(), outputFile );
    }
}


bool GERBER_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );

    finalFile = outputFile;     // the actual gerber file will be created later

    // Create a temporary filename to store gerber file
    // note tmpfile() does not work under Vista and W7 in user mode
    m_workFilename = filename + wxT(".tmp");
    workFile   = wxFopen( m_workFilename, wxT( "wt" ));
    outputFile = workFile;
    wxASSERT( outputFile );

    if( outputFile == NULL )
        return false;

    for( unsigned ii = 0; ii < m_headerExtraLines.GetCount(); ii++ )
    {
        if( ! m_headerExtraLines[ii].IsEmpty() )
            fprintf( outputFile, "%s\n", TO_UTF8( m_headerExtraLines[ii] ) );
    }

    // Set coordinate format to 3.6 or 4.5 absolute, leading zero omitted
    // the number of digits for the integer part of coordinates is needed
    // in gerber format, but is not very important when omitting leading zeros
    // It is fixed here to 3 (inch) or 4 (mm), but is not actually used
    int leadingDigitCount = m_gerberUnitInch ? 3 : 4;

    fprintf( outputFile, "%%FSLAX%d%dY%d%d*%%\n",
             leadingDigitCount, m_gerberUnitFmt,
             leadingDigitCount, m_gerberUnitFmt );
    fprintf( outputFile,
             "G04 Gerber Fmt %d.%d, Leading zero omitted, Abs format (unit %s)*\n",
             leadingDigitCount, m_gerberUnitFmt,
             m_gerberUnitInch ? "inch" : "mm" );

    wxString Title = creator + wxT( " " ) + GetBuildVersion();
    // In gerber files, ASCII7 chars only are allowed.
    // So use a ISO date format (using a space as separator between date and time),
    // not a localized date format
    wxDateTime date = wxDateTime::Now();
    fprintf( outputFile, "G04 Created by KiCad (%s) date %s*\n",
             TO_UTF8( Title ), TO_UTF8( date.FormatISOCombined( ' ') ) );

    /* Mass parameter: unit = INCHES/MM */
    if( m_gerberUnitInch )
        fputs( "%MOIN*%\n", outputFile );
    else
        fputs( "%MOMM*%\n", outputFile );

    // Be sure the usual dark polarity is selected:
    fputs( "%LPD*%\n", outputFile );

    // Set initial interpolation mode: always G01 (linear):
    fputs( "G01*\n", outputFile );

    // Set aperture list starting point:
    fputs( "G04 APERTURE LIST*\n", outputFile );

    return true;
}


bool GERBER_PLOTTER::EndPlot()
{
    char     line[1024];
    wxString msg;

    wxASSERT( outputFile );

    /* Outfile is actually a temporary file i.e. workFile */
    fputs( "M02*\n", outputFile );
    fflush( outputFile );

    fclose( workFile );
    workFile   = wxFopen( m_workFilename, wxT( "rt" ));
    wxASSERT( workFile );
    outputFile = finalFile;

    // Placement of apertures in RS274X
    while( fgets( line, 1024, workFile ) )
    {
        fputs( line, outputFile );

        char* substr = strtok( line, "\n\r" );

        if( substr && strcmp( substr, "G04 APERTURE LIST*" ) == 0 )
        {
            writeApertureList();
            fputs( "G04 APERTURE END LIST*\n", outputFile );
        }
    }

    fclose( workFile );
    fclose( finalFile );
    ::wxRemoveFile( m_workFilename );
    outputFile = 0;

    return true;
}


void GERBER_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth = m_renderSettings->GetDefaultPenWidth();

    wxASSERT_MSG( aWidth >= 0, "Plotter called to set negative pen width" );

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );
    int aperture_attribute = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;

    selectAperture( wxSize( aWidth, aWidth ), APERTURE::AT_PLOTTING, aperture_attribute );
    currentPenWidth = aWidth;
}


int GERBER_PLOTTER::GetOrCreateAperture( const wxSize& aSize,
                        APERTURE::APERTURE_TYPE aType, int aApertureAttribute )
{
    int last_D_code = 9;

    // Search an existing aperture
    for( int idx = 0; idx < (int)m_apertures.size(); ++idx )
    {
        APERTURE* tool = &m_apertures[idx];
        last_D_code = tool->m_DCode;

        if( (tool->m_Type == aType) && (tool->m_Size == aSize) &&
            (tool->m_ApertureAttribute == aApertureAttribute) )
            return idx;
    }

    // Allocate a new aperture
    APERTURE new_tool;
    new_tool.m_Size  = aSize;
    new_tool.m_Type  = aType;
    new_tool.m_DCode = last_D_code + 1;
    new_tool.m_ApertureAttribute = aApertureAttribute;

    m_apertures.push_back( new_tool );

    return m_apertures.size() - 1;
}


void GERBER_PLOTTER::selectAperture( const wxSize&           aSize,
                                     APERTURE::APERTURE_TYPE aType,
                                     int aApertureAttribute )
{
    bool change = ( m_currentApertureIdx < 0 ) ||
                  ( m_apertures[m_currentApertureIdx].m_Type != aType ) ||
                  ( m_apertures[m_currentApertureIdx].m_Size != aSize );

    if( !change )
        change = m_apertures[m_currentApertureIdx].m_ApertureAttribute != aApertureAttribute;

    if( change )
    {
        // Pick an existing aperture or create a new one
        m_currentApertureIdx = GetOrCreateAperture( aSize, aType, aApertureAttribute );
        fprintf( outputFile, "D%d*\n", m_apertures[m_currentApertureIdx].m_DCode );
    }
}

void GERBER_PLOTTER::selectAperture( int aDiameter, double aPolygonRotation,
                     APERTURE::APERTURE_TYPE aType, int aApertureAttribute )
{
    // Pick an existing aperture or create a new one, matching the
    // aDiameter, aPolygonRotation, type and attributes for type =
    // AT_REGULAR_POLY3 to AT_REGULAR_POLY12

    wxASSERT( aType>= APERTURE::APERTURE_TYPE::AT_REGULAR_POLY3 &&
                      aType <= APERTURE::APERTURE_TYPE::AT_REGULAR_POLY12 );

    // To use selectAperture( size, ... ) calculate a equivalent aperture size:
    // for AT_REGULAR_POLYxx the parameter APERTURE::m_Size contains
    // aDiameter (in m_Size.x) and aPolygonRotation in 1/1000 degree (in m_Size.y)
    wxSize size( aDiameter, (int)( aPolygonRotation * 1000.0 ) );
    selectAperture( size, aType, aApertureAttribute );
}

void GERBER_PLOTTER::writeApertureList()
{
    wxASSERT( outputFile );
    char cbuf[1024];

    bool useX1StructuredComment = false;

    if( !m_useX2format )
        useX1StructuredComment = true;

    // Init
    for( APERTURE& tool : m_apertures )
    {
        // apertude sizes are in inch or mm, regardless the
        // coordinates format
        double fscale = 0.0001 * plotScale / m_IUsPerDecimil; // inches

        if(! m_gerberUnitInch )
            fscale *= 25.4;     // size in mm

        int attribute = tool.m_ApertureAttribute;

        if( attribute != m_apertureAttribute )
        {
            fputs( GBR_APERTURE_METADATA::FormatAttribute(
                    (GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB) attribute,
                            useX1StructuredComment ).c_str(), outputFile );
        }

        char* text = cbuf + sprintf( cbuf, "%%ADD%d", tool.m_DCode );

        /* Please note: the Gerber specs for mass parameters say that
           exponential syntax is *not* allowed and the decimal point should
           also be always inserted. So the %g format is ruled out, but %f is fine
           (the # modifier forces the decimal point). Sadly the %f formatter
           can't remove trailing zeros but thats not a problem, since nothing
           forbid it (the file is only slightly longer) */

        switch( tool.m_Type )
        {
        case APERTURE::AT_CIRCLE:
            sprintf( text, "C,%#f*%%\n", tool.GetDiameter() * fscale );
            break;

        case APERTURE::AT_RECT:
            sprintf( text, "R,%#fX%#f*%%\n", tool.m_Size.x * fscale,
                                             tool.m_Size.y * fscale );
            break;

        case APERTURE::AT_PLOTTING:
            sprintf( text, "C,%#f*%%\n", tool.m_Size.x * fscale );
            break;

        case APERTURE::AT_OVAL:
            sprintf( text, "O,%#fX%#f*%%\n", tool.m_Size.x * fscale,
                                             tool.m_Size.y * fscale );
            break;

        case APERTURE::AT_REGULAR_POLY:
        case APERTURE::AT_REGULAR_POLY3:
        case APERTURE::AT_REGULAR_POLY4:
        case APERTURE::AT_REGULAR_POLY5:
        case APERTURE::AT_REGULAR_POLY6:
        case APERTURE::AT_REGULAR_POLY7:
        case APERTURE::AT_REGULAR_POLY8:
        case APERTURE::AT_REGULAR_POLY9:
        case APERTURE::AT_REGULAR_POLY10:
        case APERTURE::AT_REGULAR_POLY11:
        case APERTURE::AT_REGULAR_POLY12:
            sprintf( text, "P,%#fX%dX%#f*%%\n", tool.GetDiameter() * fscale,
                     tool.GetVerticeCount(), tool.GetRotation() );
            break;
        }

        fputs( cbuf, outputFile );

        m_apertureAttribute = attribute;

        // Currently reset the aperture attribute. Perhaps a better optimization
        // is to store the last attribute
        if( attribute )
        {
            if( m_useX2format )
                fputs( "%TD*%\n", outputFile );
            else
                fputs( "G04 #@! TD*\n", outputFile );

            m_apertureAttribute = 0;
        }

    }
}


void GERBER_PLOTTER::PenTo( const wxPoint& aPos, char plume )
{
    wxASSERT( outputFile );
    DPOINT pos_dev = userToDeviceCoordinates( aPos );

    switch( plume )
    {
    case 'Z':
        break;

    case 'U':
        emitDcode( pos_dev, 2 );
        break;

    case 'D':
        emitDcode( pos_dev, 1 );
    }

    penState = plume;
}


void GERBER_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill, int width )
{
    std::vector< wxPoint > cornerList;

    // Build corners list
    cornerList.push_back( p1 );
    wxPoint corner(p1.x, p2.y);
    cornerList.push_back( corner );
    cornerList.push_back( p2 );
    corner.x = p2.x;
    corner.y = p1.y;
    cornerList.push_back( corner );
    cornerList.push_back( p1 );

    PlotPoly( cornerList, fill, width );
}


void GERBER_PLOTTER::Circle( const wxPoint& aCenter, int aDiameter, FILL_T aFill, int aWidth )
{
    Arc( aCenter, 0, 3600, aDiameter / 2, aFill, aWidth );
}


void GERBER_PLOTTER::Arc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                          int aRadius, FILL_T aFill, int aWidth )
{
    SetCurrentLineWidth( aWidth );

    // aFill is not used here.
    plotArc( aCenter, aStAngle, aEndAngle, aRadius, false );
}


void GERBER_PLOTTER::plotArc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                  int aRadius, bool aPlotInRegion )
{
    wxPoint start, end;
    start.x = aCenter.x + KiROUND( cosdecideg( aRadius, aStAngle ) );
    start.y = aCenter.y - KiROUND( sindecideg( aRadius, aStAngle ) );

    if( !aPlotInRegion )
        MoveTo( start );
    else
        LineTo( start );

    end.x = aCenter.x + KiROUND( cosdecideg( aRadius, aEndAngle ) );
    end.y = aCenter.y - KiROUND( sindecideg( aRadius, aEndAngle ) );
    DPOINT devEnd = userToDeviceCoordinates( end );
    DPOINT devCenter = userToDeviceCoordinates( aCenter ) - userToDeviceCoordinates( start );

    fprintf( outputFile, "G75*\n" );        // Multiquadrant (360 degrees) mode

    if( aStAngle < aEndAngle )
        fprintf( outputFile, "G03*\n" );    // Active circular interpolation, CCW
    else
        fprintf( outputFile, "G02*\n" );    // Active circular interpolation, CW

    fprintf( outputFile, "X%dY%dI%dJ%dD01*\n",
             KiROUND( devEnd.x ), KiROUND( devEnd.y ),
             KiROUND( devCenter.x ), KiROUND( devCenter.y ) );

    fprintf( outputFile, "G01*\n" ); // Back to linear interpol (perhaps useless here).
}


void GERBER_PLOTTER::PlotGerberRegion( const std::vector< wxPoint >& aCornerList,
                                 void * aData )
{
    if( aCornerList.size() <= 2 )
        return;

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

    if( gbr_metadata )
    {
        std::string attrib = gbr_metadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

        if( !attrib.empty() )
        {
            fputs( attrib.c_str(), outputFile );
            clearTA_AperFunction = true;
        }
    }

    PlotPoly( aCornerList, FILLED_SHAPE, 0 , gbr_metadata );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
        {
            fputs( "%TD.AperFunction*%\n", outputFile );
        }
        else
        {
            fputs( "G04 #@! TD.AperFunction*\n", outputFile );
        }
    }
}

void GERBER_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                               FILL_T aFill, int aWidth, void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Therefore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( aFill )
    {
        fputs( "G36*\n", outputFile );

        MoveTo( aCornerList[0] );
        fputs( "G01*\n", outputFile );      // Set linear interpolation.

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // If the polygon is not closed, close it:
        if( aCornerList[0] != aCornerList[aCornerList.size()-1] )
            FinishTo( aCornerList[0] );

        fputs( "G37*\n", outputFile );
    }

    if( aWidth > 0 )    // Draw the polyline/polygon outline
    {
        SetCurrentLineWidth( aWidth, gbr_metadata );

        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Ensure the thick outline is closed for filled polygons
        // (if not filled, could be only a polyline)
        if( aFill && ( aCornerList[aCornerList.size()-1] != aCornerList[0] ) )
            LineTo( aCornerList[0] );

        PenFinish();
    }
}


void GERBER_PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                            EDA_DRAW_MODE_T tracemode, void* aData )
{
    if( tracemode == FILLED )
    {
        GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
        SetCurrentLineWidth( width, gbr_metadata );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        MoveTo( start );
        FinishTo( end );
    }
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        segmentAsOval( start, end, width, tracemode );
    }
}

void GERBER_PLOTTER::ThickArc( const wxPoint& centre, double StAngle, double EndAngle,
                           int radius, int width, EDA_DRAW_MODE_T tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Arc( centre, StAngle, EndAngle, radius, NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        Arc( centre, StAngle, EndAngle,
             radius - ( width - currentPenWidth ) / 2,
             NO_FILL, DO_NOT_SET_LINE_WIDTH );
        Arc( centre, StAngle, EndAngle,
             radius + ( width - currentPenWidth ) / 2, NO_FILL,
             DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            EDA_DRAW_MODE_T tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Rect( p1, p2, NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        wxPoint offsetp1( p1.x - (width - currentPenWidth) / 2,
                          p1.y - (width - currentPenWidth) / 2 );
        wxPoint offsetp2( p2.x + (width - currentPenWidth) / 2,
              p2.y + (width - currentPenWidth) / 2 );
        Rect( offsetp1, offsetp2, NO_FILL, -1 );
        offsetp1.x += (width - currentPenWidth);
        offsetp1.y += (width - currentPenWidth);
        offsetp2.x -= (width - currentPenWidth);
        offsetp2.y -= (width - currentPenWidth);
        Rect( offsetp1, offsetp2, NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::ThickCircle( const wxPoint& pos, int diametre, int width,
                              EDA_DRAW_MODE_T tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Circle( pos, diametre, NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH, gbr_metadata );
        Circle( pos, diametre - (width - currentPenWidth),
                    NO_FILL, DO_NOT_SET_LINE_WIDTH );
        Circle( pos, diametre + (width - currentPenWidth),
                    NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre, EDA_DRAW_MODE_T trace_mode, void* aData )
{
    wxSize size( diametre, diametre );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( trace_mode == SKETCH )
    {
        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        Circle( pos, diametre - currentPenWidth, NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
    else
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );

        int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
        selectAperture( size, APERTURE::AT_CIRCLE, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
    }
}


void GERBER_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                   EDA_DRAW_MODE_T trace_mode, void* aData )
{
    wxASSERT( outputFile );
    wxSize size( aSize );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    // Flash a vertical or horizontal shape (this is a basic aperture).
    if( ( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
        && trace_mode == FILLED )
    {
        if( orient == 900 || orient == 2700 ) /* orientation turned 90 deg. */
            std::swap( size.x, size.y );

        DPOINT pos_dev = userToDeviceCoordinates( pos );
        int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
        selectAperture( size, APERTURE::AT_OVAL, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
    }
    else    // Plot pad as region.
            // Only regions and flashed items accept a object attribute TO.P for the pin name
    {
        if( trace_mode == FILLED )
        {
            // Draw the oval as round rect pad with a radius = 50% min size)
            // In gerber file, it will be drawn as a region with arcs, and can be
            // detected as pads (similar to a flashed pad)
            FlashPadRoundRect( pos, aSize, std::min( aSize.x, aSize.y ) /2,
                               orient, FILLED, aData );
        }
        else    // Non filled shape: plot outlines:
        {
            if( size.x > size.y )
            {
                std::swap( size.x, size.y );

                if( orient < 2700 )
                    orient += 900;
                else
                    orient -= 2700;
            }

            sketchOval( pos, size, orient, -1 );
        }
    }
}


void GERBER_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& aSize,
                                   double orient, EDA_DRAW_MODE_T trace_mode, void* aData )

{
    wxASSERT( outputFile );
    wxSize size( aSize );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    // Plot as an aperture flash
    switch( int( orient ) )
    {
    case 900:
    case 2700:        // rotation of 90 degrees or 270 swaps sizes
        std::swap( size.x, size.y );
        KI_FALLTHROUGH;

    case 0:
    case 1800:
        if( trace_mode == SKETCH )
        {
            if( gbr_metadata )
                formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

            Rect( wxPoint( pos.x - (size.x - currentPenWidth) / 2,
                           pos.y - (size.y - currentPenWidth) / 2 ),
                  wxPoint( pos.x + (size.x - currentPenWidth) / 2,
                           pos.y + (size.y - currentPenWidth) / 2 ),
                  NO_FILL, GetCurrentLineWidth() );
        }
        else
        {
            DPOINT pos_dev = userToDeviceCoordinates( pos );
            int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
            selectAperture( size, APERTURE::AT_RECT, aperture_attrib );

            if( gbr_metadata )
                formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

            emitDcode( pos_dev, 3 );
        }
        break;

    default: // plot pad shape as Gerber region
        {
        wxPoint coord[4];
        // coord[0] is assumed the lower left
        // coord[1] is assumed the upper left
        // coord[2] is assumed the upper right
        // coord[3] is assumed the lower right

        coord[0].x = -size.x/2;   // lower left
        coord[0].y = size.y/2;
        coord[1].x = -size.x/2;   // upper left
        coord[1].y = -size.y/2;
        coord[2].x = size.x/2;    // upper right
        coord[2].y = -size.y/2;
        coord[3].x = size.x/2;    // lower right
        coord[3].y = size.y/2;

        FlashPadTrapez( pos, coord, orient, trace_mode, aData );
        }
    break;
    }
}

void GERBER_PLOTTER::FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                     int aCornerRadius, double aOrient,
                                     EDA_DRAW_MODE_T aTraceMode, void* aData )

{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( aTraceMode != FILLED )
    {
        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient,
                                     aCornerRadius, 0.0, 0, GetPlotterArcHighDef() );

        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH, &gbr_metadata );
        outline.Inflate( -GetCurrentLineWidth()/2, 16 );

        std::vector< wxPoint > cornerList;
        // TransformRoundRectToPolygon creates only one convex polygon
        SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
        cornerList.reserve( poly.PointCount() + 1 );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        // Close polygon
        cornerList.push_back( cornerList[0] );

        // plot outlines
        PlotPoly( cornerList, NO_FILL, GetCurrentLineWidth(), gbr_metadata );
    }
    else
    {
        // A Pad RoundRect is plotted as a Gerber region.
        // Initialize region metadata:
        bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

        if( gbr_metadata )
        {
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );
            std::string attrib = gbr_metadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

            if( !attrib.empty() )
            {
                fputs( attrib.c_str(), outputFile );
                clearTA_AperFunction = true;
            }
        }

        // Plot the region using arcs in corners.
        plotRoundRectAsRegion( aPadPos, aSize, aCornerRadius, aOrient );

        // Clear the TA attribute, to avoid the next item to inherit it:
        if( clearTA_AperFunction )
        {
            if( m_useX2format )
            {
                fputs( "%TD.AperFunction*%\n", outputFile );
            }
            else
            {
                fputs( "G04 #@! TD.AperFunction*\n", outputFile );
            }
        }
    }
}


void GERBER_PLOTTER::plotRoundRectAsRegion( const wxPoint& aRectCenter, const wxSize& aSize,
                                            int aCornerRadius, double aOrient )
{
    // The region outline is generated by 4 sides and 4 90 deg arcs
    //  1 --- 2
    //  |  c  |
    //  4 --- 3

    // Note also in user coordinates the Y axis is from top to bottom
    // for historical reasons.

    // A helper structure to handle outlines coordinates (segments and arcs)
    // in user coordinates
    struct RR_EDGE
    {
        wxPoint m_start;
        wxPoint m_end;
        wxPoint m_center;
        // in decidegrees: angle start. angle end = m_arc_angle_start+arc_angle
        double  m_arc_angle_start;
    };

    const double arc_angle = -900.0;    // in decidegrees
    int hsizeX = aSize.x/2;
    int hsizeY = aSize.y/2;

    RR_EDGE curr_edge;
    std::vector<RR_EDGE> rr_outline;

    // Build outline coordinates, relative to rectangle center, rotation 0:

    // Top left corner 1 (and 4 to 1 left vertical side @ x=-hsizeX)
    curr_edge.m_start.x = -hsizeX;
    curr_edge.m_start.y = hsizeY - aCornerRadius;
    curr_edge.m_end.x = curr_edge.m_start.x;
    curr_edge.m_end.y = -hsizeY + aCornerRadius;
    curr_edge.m_center.x = -hsizeX + aCornerRadius;
    curr_edge.m_center.y = curr_edge.m_end.y;
    curr_edge.m_arc_angle_start = aOrient + 1800.0;     // En decidegree

    rr_outline.push_back( curr_edge );

    // Top right corner 2 (and 1 to 2 top horizontal side @ y=-hsizeY)
    curr_edge.m_start.x = -hsizeX + aCornerRadius;
    curr_edge.m_start.y = -hsizeY;
    curr_edge.m_end.x = hsizeX - aCornerRadius;
    curr_edge.m_end.y = curr_edge.m_start.y;
    curr_edge.m_center.x = curr_edge.m_end.x;
    curr_edge.m_center.y = -hsizeY + aCornerRadius;
    curr_edge.m_arc_angle_start = aOrient + 900.0;     // En decidegree

    rr_outline.push_back( curr_edge );

    // bottom right corner 3 (and 2 to 3 right vertical side @ x=hsizeX)
    curr_edge.m_start.x = hsizeX;
    curr_edge.m_start.y = -hsizeY + aCornerRadius;
    curr_edge.m_end.x =  curr_edge.m_start.x;
    curr_edge.m_end.y = hsizeY - aCornerRadius;
    curr_edge.m_center.x = hsizeX - aCornerRadius;
    curr_edge.m_center.y = curr_edge.m_end.y;
    curr_edge.m_arc_angle_start = aOrient + 0.0;     // En decidegree

    rr_outline.push_back( curr_edge );

    // bottom left corner 4 (and 3 to 4 bottom horizontal side @ y=hsizeY)
    curr_edge.m_start.x = hsizeX - aCornerRadius;
    curr_edge.m_start.y = hsizeY;
    curr_edge.m_end.x =  -hsizeX + aCornerRadius;
    curr_edge.m_end.y = curr_edge.m_start.y;
    curr_edge.m_center.x = curr_edge.m_end.x;
    curr_edge.m_center.y = hsizeY - aCornerRadius;
    curr_edge.m_arc_angle_start = aOrient - 900.0;     // En decidegree

    rr_outline.push_back( curr_edge );

    // Move relative coordinates to the actual location and rotation:
    wxPoint arc_last_center;
    int arc_last_angle = curr_edge.m_arc_angle_start+arc_angle;

    for( RR_EDGE& rr_edge: rr_outline )
    {
        RotatePoint( &rr_edge.m_start, aOrient );
        RotatePoint( &rr_edge.m_end, aOrient );
        RotatePoint( &rr_edge.m_center, aOrient );
        rr_edge.m_start += aRectCenter;
        rr_edge.m_end += aRectCenter;
        rr_edge.m_center += aRectCenter;
        arc_last_center = rr_edge.m_center;
    }

    // Ensure the region is a closed polygon, i.e. the end point of last segment
    // (end of arc) is the same as the first point. Rounding issues can create a
    // small difference, mainly for rotated pads.
    // calculate last point (end of last arc):
    wxPoint last_pt;
    last_pt.x = arc_last_center.x + KiROUND( cosdecideg( aCornerRadius, arc_last_angle ) );
    last_pt.y = arc_last_center.y - KiROUND( sindecideg( aCornerRadius, arc_last_angle ) );

    wxPoint first_pt = rr_outline[0].m_start;

#if 0    // For test only:
    if( last_pt != first_pt )
        wxLogMessage( "first pt %d %d last pt %d %d",
                      first_pt.x, first_pt.y, last_pt.x, last_pt.y );
#endif

    fputs( "G36*\n", outputFile );  // Start region
    fputs( "G01*\n", outputFile );  // Set linear interpolation.
    first_pt = last_pt;
    MoveTo( first_pt );             // Start point of region, must be same as end point

    for( RR_EDGE& rr_edge: rr_outline )
    {
        if( aCornerRadius )     // Guard: ensure we do not create arcs with radius = 0
        {
            // LineTo( rr_edge.m_end ); // made in plotArc()
            plotArc( rr_edge.m_center,
                     rr_edge.m_arc_angle_start, rr_edge.m_arc_angle_start+arc_angle,
                     aCornerRadius, true );
        }
        else
            LineTo( rr_edge.m_end );
    }

    fputs( "G37*\n", outputFile );      // Close region
}


void GERBER_PLOTTER::FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                     SHAPE_POLY_SET* aPolygons,
                                     EDA_DRAW_MODE_T aTraceMode, void* aData )

{
    // A Pad custom is plotted as polygon (a region in Gerber language).

    GBR_METADATA gbr_metadata;

    if( aData )
        gbr_metadata = *static_cast<GBR_METADATA*>( aData );

    SHAPE_POLY_SET polyshape = *aPolygons;

    if( aTraceMode != FILLED )
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH, &gbr_metadata );
        polyshape.Inflate( -GetCurrentLineWidth()/2, 16 );
    }

    std::vector< wxPoint > cornerList;

    for( int cnt = 0; cnt < polyshape.OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = polyshape.Outline( cnt );

        cornerList.clear();

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        // Close polygon
        cornerList.push_back( cornerList[0] );

        if( aTraceMode == SKETCH )
            PlotPoly( cornerList, NO_FILL, GetCurrentLineWidth(), &gbr_metadata );
        else
            PlotGerberRegion( cornerList, &gbr_metadata );
    }
}


void GERBER_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos,  const wxPoint* aCorners,
                                     double aPadOrient, EDA_DRAW_MODE_T aTrace_Mode, void* aData )

{
    // TODO: use Aperture macro and flash it

    // polygon corners list
    std::vector<wxPoint> cornerList = { aCorners[0], aCorners[1], aCorners[2], aCorners[3] };

    // Draw the polygon and fill the interior as required
    for( unsigned ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    // Close the polygon
    cornerList.push_back( cornerList[0] );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    GBR_METADATA metadata;

    if( gbr_metadata )
        metadata = *gbr_metadata;

    if( aTrace_Mode == SKETCH )
        PlotPoly( cornerList, NO_FILL, DO_NOT_SET_LINE_WIDTH, &metadata );
    else
        PlotGerberRegion( cornerList, &metadata );
}


void GERBER_PLOTTER::FlashRegularPolygon( const wxPoint& aShapePos,
                                          int aDiameter, int aCornerCount,
                                          double aOrient, EDA_DRAW_MODE_T aTraceMode,
                                          void* aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( aTraceMode == SKETCH )
    {
        // Build the polygon:
        std::vector< wxPoint > cornerList;

        double angle_delta = 3600.0 / aCornerCount; // in 0.1 degree

        for( int ii = 0; ii < aCornerCount; ii++ )
        {
            double rot = aOrient + (angle_delta*ii);
            wxPoint vertice( aDiameter/2, 0 );
            RotatePoint( &vertice, rot );
            vertice += aShapePos;
            cornerList.push_back( vertice );
        }

        cornerList.push_back( cornerList[0] );  // Close the shape

        PlotPoly( cornerList, NO_FILL, GetCurrentLineWidth(), gbr_metadata );
    }
    else
    {
        DPOINT pos_dev = userToDeviceCoordinates( aShapePos );

        int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;

        APERTURE::APERTURE_TYPE apert_type =
                (APERTURE::APERTURE_TYPE)(APERTURE::AT_REGULAR_POLY3 + aCornerCount - 3);
        selectAperture( aDiameter, aOrient, apert_type, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
    }
}


void GERBER_PLOTTER::Text( const wxPoint& aPos, const COLOR4D aColor,
                           const wxString& aText, double aOrient, const wxSize& aSize,
                           enum EDA_TEXT_HJUSTIFY_T aH_justify,
                           enum EDA_TEXT_VJUSTIFY_T aV_justify, int aWidth, bool aItalic,
                           bool aBold, bool aMultilineAllowed, void* aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                   aBold, aMultilineAllowed, aData );
}


void GERBER_PLOTTER::SetLayerPolarity( bool aPositive )
{
    if( aPositive )
        fprintf( outputFile, "%%LPD*%%\n" );
    else
        fprintf( outputFile, "%%LPC*%%\n" );
}
