/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file GERBER_plotter.cpp
 * @brief specialized plotter for GERBER files format
 */

#include <eda_base_frame.h>
#include <fill_type.h>
#include <kicad_string.h>
#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <render_settings.h>
#include <trigo.h>
#include <wx/log.h>

#include <build_version.h>

#include "plotter_gerber.h"
#include "gbr_plotter_aperture_macros.h"

#include <gbr_metadata.h>


// if GBR_USE_MACROS is defined, pads having a shape that is not a Gerber primitive
// will use a macro when possible
// Old code will be removed only after many tests
//
// Note also: setting m_gerberDisableApertMacros to true disable all aperture macros
// in Gerber files
//
#define GBR_USE_MACROS_FOR_CHAMFERED_ROUND_RECT
#define GBR_USE_MACROS_FOR_CHAMFERED_RECT
#define GBR_USE_MACROS_FOR_ROUNDRECT
#define GBR_USE_MACROS_FOR_TRAPEZOID
#define GBR_USE_MACROS_FOR_ROTATED_OVAL
#define GBR_USE_MACROS_FOR_ROTATED_RECT
#define GBR_USE_MACROS_FOR_CUSTOM_PAD

// max count of corners to create a aperture macro for a custom shape.
// provided just in case a aperture macro type free polygon creates issues
// when the number of corners is too high.
// (1 corner = up to 24 chars)
// Gerber doc say max corners 5000. We use a slightly smaller value.
// if a custom shape needs more than GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT, it
// will be plot using a region.
#define GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT 4990
#define AM_FREEPOLY_BASENAME "FreePoly"

// A helper function to compare 2 polygons: polygons are similar if they havve the same
// number of vertices and each vertex coordinate are similar, i.e. if the difference
// between coordinates is small ( <= margin to accept rounding issues coming from polygon
// geometric transforms like rotation
static bool polyCompare( const std::vector<wxPoint>& aPolygon,
                         const std::vector<wxPoint>& aTestPolygon )
{
    // fast test: polygon sizes must be the same:
    if( aTestPolygon.size() != aPolygon.size() )
        return false;

    const int margin = 2;

    for( size_t jj = 0; jj < aPolygon.size(); jj++ )
    {
        if( std::abs( aPolygon[jj].x - aTestPolygon[jj].x ) > margin ||
            std::abs( aPolygon[jj].y - aTestPolygon[jj].y ) > margin )
            return false;
    }

    return true;
}


GERBER_PLOTTER::GERBER_PLOTTER()
{
    workFile  = nullptr;
    finalFile = nullptr;
    m_currentApertureIdx = -1;
    m_apertureAttribute = 0;

    // number of digits after the point (number of digits of the mantissa
    // Be careful: the Gerber coordinates are stored in an integer
    // so 6 digits (inches) or 5 digits (mm) is a good value
    // To avoid overflow, 7 digits (inches) or 6 digits is a max.
    // with lower values than 6 digits (inches) or 5 digits (mm),
    // Creating self-intersecting polygons from non-intersecting polygons
    // happen easily.
    m_gerberUnitInch = false;
    m_gerberUnitFmt = 6;
    m_useX2format = true;
    m_useNetAttributes = true;
    m_gerberDisableApertMacros = false;

    m_hasApertureRoundRect = false;     // true is at least one round rect aperture is in use
    m_hasApertureRotOval = false;       // true is at least one oval rotated aperture is in use
    m_hasApertureRotRect = false;       // true is at least one rect. rotated aperture is in use
    m_hasApertureOutline4P = false;       // true is at least one rotated rect or trapezoid pad
                                        // aperture is in use
    m_hasApertureChamferedRect = false; // true is at least one chamfered rect
                                        // (no rounded corner) is in use
}


void GERBER_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror )
{
    wxASSERT( aMirror == false );
    m_plotMirror = false;
    m_plotOffset = aOffset;
    wxASSERT( aScale == 1 );              // aScale parameter is not used in Gerber
    m_plotScale = 1;                      // Plot scale is *always* 1.0

    m_IUsPerDecimil = aIusPerDecimil;
    // gives now a default value to iuPerDeviceUnit (because the units of the caller is now known)
    // which could be modified later by calling SetGerberCoordinatesFormat()
    m_iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    // We don't handle the filmbox, and it's more useful to keep the
    // origin at the origin
    m_paperSize.x = 0;
    m_paperSize.y = 0;
}


void GERBER_PLOTTER::SetGerberCoordinatesFormat( int aResolution, bool aUseInches )
{
    m_gerberUnitInch = aUseInches;
    m_gerberUnitFmt = aResolution;

    m_iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    if( ! m_gerberUnitInch )
        m_iuPerDeviceUnit *= 25.4;     // gerber output in mm
}


void GERBER_PLOTTER::emitDcode( const DPOINT& pt, int dcode )
{

    fprintf( m_outputFile, "X%dY%dD%02d*\n", KiROUND( pt.x ), KiROUND( pt.y ), dcode );
}

void GERBER_PLOTTER::ClearAllAttributes()
{
    // Remove all attributes from object attributes dictionary (TO. and TA commands)
    if( m_useX2format )
        fputs( "%TD*%\n", m_outputFile );
    else
        fputs( "G04 #@! TD*\n", m_outputFile );

    m_objectAttributesDictionary.clear();
}


void GERBER_PLOTTER::clearNetAttribute()
{
    // disable a Gerber net attribute (exists only in X2 with net attributes mode).
    if( m_objectAttributesDictionary.empty() )     // No net attribute or not X2 mode
        return;

    // Remove all net attributes from object attributes dictionary
    if( m_useX2format )
        fputs( "%TD*%\n", m_outputFile );
    else
        fputs( "G04 #@! TD*\n", m_outputFile );

    m_objectAttributesDictionary.clear();
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
    if( aData == nullptr )
        return;

    if( !m_useNetAttributes )
        return;

    bool useX1StructuredComment = !m_useX2format;

    bool clearDict;
    std::string short_attribute_string;

    if( !FormatNetAttribute( short_attribute_string, m_objectAttributesDictionary,
                        aData, clearDict, useX1StructuredComment ) )
        return;

    if( clearDict )
        clearNetAttribute();

    if( !short_attribute_string.empty() )
        fputs( short_attribute_string.c_str(), m_outputFile );

    if( m_useX2format && !aData->m_ExtraData.IsEmpty() )
    {
        std::string extra_data = TO_UTF8( aData->m_ExtraData );
        fputs( extra_data.c_str(), m_outputFile );
    }
}


bool GERBER_PLOTTER::StartPlot()
{
    m_hasApertureRoundRect = false;     // true is at least one round rect aperture is in use
    m_hasApertureRotOval = false;       // true is at least one oval rotated aperture is in use
    m_hasApertureRotRect = false;       // true is at least one rect. rotated aperture is in use
    m_hasApertureOutline4P = false;     // true is at least one rotated rect/trapezoid aperture is in use
    m_hasApertureChamferedRect = false; // true is at least one chamfered rect is in use
    m_am_freepoly_list.ClearList();

    wxASSERT( m_outputFile );

    finalFile = m_outputFile;     // the actual gerber file will be created later

    // Create a temp file in system temp to avoid potential network share buffer issues for the final read and save
    m_workFilename = wxFileName::CreateTempFileName( "" );
    workFile   = wxFopen( m_workFilename, wxT( "wt" ));
    m_outputFile = workFile;
    wxASSERT( m_outputFile );

    if( m_outputFile == nullptr )
        return false;

    for( unsigned ii = 0; ii < m_headerExtraLines.GetCount(); ii++ )
    {
        if( ! m_headerExtraLines[ii].IsEmpty() )
            fprintf( m_outputFile, "%s\n", TO_UTF8( m_headerExtraLines[ii] ) );
    }

    // Set coordinate format to 3.6 or 4.5 absolute, leading zero omitted
    // the number of digits for the integer part of coordinates is needed
    // in gerber format, but is not very important when omitting leading zeros
    // It is fixed here to 3 (inch) or 4 (mm), but is not actually used
    int leadingDigitCount = m_gerberUnitInch ? 3 : 4;

    fprintf( m_outputFile, "%%FSLAX%d%dY%d%d*%%\n",
             leadingDigitCount, m_gerberUnitFmt,
             leadingDigitCount, m_gerberUnitFmt );
    fprintf( m_outputFile,
             "G04 Gerber Fmt %d.%d, Leading zero omitted, Abs format (unit %s)*\n",
             leadingDigitCount, m_gerberUnitFmt,
             m_gerberUnitInch ? "inch" : "mm" );

    wxString Title = m_creator + wxT( " " ) + GetBuildVersion();
    // In gerber files, ASCII7 chars only are allowed.
    // So use a ISO date format (using a space as separator between date and time),
    // not a localized date format
    wxDateTime date = wxDateTime::Now();
    fprintf( m_outputFile, "G04 Created by KiCad (%s) date %s*\n",
             TO_UTF8( Title ), TO_UTF8( date.FormatISOCombined( ' ') ) );

    /* Mass parameter: unit = INCHES/MM */
    if( m_gerberUnitInch )
        fputs( "%MOIN*%\n", m_outputFile );
    else
        fputs( "%MOMM*%\n", m_outputFile );

    // Be sure the usual dark polarity is selected:
    fputs( "%LPD*%\n", m_outputFile );

    // Set initial interpolation mode: always G01 (linear):
    fputs( "G01*\n", m_outputFile );

    // Add aperture list start point
    fputs( "G04 APERTURE LIST*\n", m_outputFile );

    // Give a minimal value to the default pen size, used to plot items in sketch mode
    if( m_renderSettings )
    {
        const int pen_min = 0.1 * m_IUsPerDecimil * 10000 / 25.4;   // for min width = 0.1 mm
        m_renderSettings->SetDefaultPenWidth( std::max( m_renderSettings->GetDefaultPenWidth(),
                                                        pen_min ) );
    }

    return true;
}


bool GERBER_PLOTTER::EndPlot()
{
    char     line[1024];
    wxString msg;

    wxASSERT( m_outputFile );

    /* Outfile is actually a temporary file i.e. workFile */
    fputs( "M02*\n", m_outputFile );
    fflush( m_outputFile );

    fclose( workFile );
    workFile   = wxFopen( m_workFilename, wxT( "rt" ));
    wxASSERT( workFile );
    m_outputFile = finalFile;

    // Placement of apertures in RS274X
    while( fgets( line, 1024, workFile ) )
    {
        fputs( line, m_outputFile );

        char* substr = strtok( line, "\n\r" );

        if( substr && strcmp( substr, "G04 APERTURE LIST*" ) == 0 )
        {
            // Add aperture list macro:
            if( m_hasApertureRoundRect | m_hasApertureRotOval ||
                m_hasApertureOutline4P || m_hasApertureRotRect ||
                m_hasApertureChamferedRect || m_am_freepoly_list.AmCount() )
            {
                fputs( "G04 Aperture macros list*\n", m_outputFile );

                if( m_hasApertureRoundRect )
                    fputs( APER_MACRO_ROUNDRECT_HEADER, m_outputFile );

                if( m_hasApertureRotOval )
                    fputs( APER_MACRO_SHAPE_OVAL_HEADER, m_outputFile );

                if( m_hasApertureRotRect )
                    fputs( APER_MACRO_ROT_RECT_HEADER, m_outputFile );

                if( m_hasApertureOutline4P )
                    fputs( APER_MACRO_OUTLINE4P_HEADER, m_outputFile );

                if( m_hasApertureChamferedRect )
                {
                    fputs( APER_MACRO_OUTLINE5P_HEADER, m_outputFile );
                    fputs( APER_MACRO_OUTLINE6P_HEADER, m_outputFile );
                    fputs( APER_MACRO_OUTLINE7P_HEADER, m_outputFile );
                    fputs( APER_MACRO_OUTLINE8P_HEADER, m_outputFile );
                }

                if( m_am_freepoly_list.AmCount() )
                {
                    // apertude sizes are in inch or mm, regardless the
                    // coordinates format
                    double fscale = 0.0001 * m_plotScale / m_IUsPerDecimil; // inches

                    if(! m_gerberUnitInch )
                        fscale *= 25.4;     // size in mm

                    m_am_freepoly_list.Format( m_outputFile, fscale );
                }

                fputs( "G04 Aperture macros list end*\n", m_outputFile );
            }

            writeApertureList();
            fputs( "G04 APERTURE END LIST*\n", m_outputFile );
        }
    }

    fclose( workFile );
    fclose( finalFile );
    ::wxRemoveFile( m_workFilename );
    m_outputFile = nullptr;

    return true;
}


void GERBER_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth =  m_renderSettings->GetDefaultPenWidth();

    wxASSERT_MSG( aWidth >= 0, "Plotter called to set negative pen width" );

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );
    int aperture_attribute = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;

    selectAperture( wxSize( aWidth, aWidth ), 0, 0.0, APERTURE::AT_PLOTTING, aperture_attribute );
    m_currentPenWidth = aWidth;
}


int GERBER_PLOTTER::GetOrCreateAperture( const wxSize& aSize, int aRadius, double aRotDegree,
                        APERTURE::APERTURE_TYPE aType, int aApertureAttribute )
{
    int last_D_code = 9;

    // Search an existing aperture
    for( int idx = 0; idx < (int)m_apertures.size(); ++idx )
    {
        APERTURE* tool = &m_apertures[idx];
        last_D_code = tool->m_DCode;

        if( (tool->m_Type == aType) && (tool->m_Size == aSize) &&
            (tool->m_Radius == aRadius) && (tool->m_Rotation == aRotDegree) &&
            (tool->m_ApertureAttribute == aApertureAttribute) )
            return idx;
    }

    // Allocate a new aperture
    APERTURE new_tool;
    new_tool.m_Size  = aSize;
    new_tool.m_Type  = aType;
    new_tool.m_Radius  = aRadius;
    new_tool.m_Rotation  = aRotDegree;
    new_tool.m_DCode = last_D_code + 1;
    new_tool.m_ApertureAttribute = aApertureAttribute;

    m_apertures.push_back( new_tool );

    return m_apertures.size() - 1;
}


int GERBER_PLOTTER::GetOrCreateAperture( const std::vector<wxPoint>& aCorners, double aRotDegree,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute )
{
    int last_D_code = 9;

    // For APERTURE::AM_FREE_POLYGON aperture macros, we need to create the macro
    // on the fly, because due to the fact the vertice count is not a constant we
    // cannot create a static definition.
    if( APERTURE::AM_FREE_POLYGON == aType )
    {
        int idx = m_am_freepoly_list.FindAm( aCorners );

        if( idx < 0 )
            m_am_freepoly_list.Append( aCorners );
    }

    // Search an existing aperture
    for( int idx = 0; idx < (int)m_apertures.size(); ++idx )
    {
        APERTURE* tool = &m_apertures[idx];

       last_D_code = tool->m_DCode;

        if( (tool->m_Type == aType) &&
            (tool->m_Corners.size() == aCorners.size() ) &&
            (tool->m_Rotation == aRotDegree) &&
            (tool->m_ApertureAttribute == aApertureAttribute) )
        {
            // A candidate is found. the corner lists must be similar
            bool is_same = polyCompare( tool->m_Corners, aCorners );

            if( is_same )
                return idx;
        }
    }

    // Allocate a new aperture
    APERTURE new_tool;

    new_tool.m_Corners  = aCorners;
    new_tool.m_Size     = wxSize( 0, 0 );   // Not used
    new_tool.m_Type     = aType;
    new_tool.m_Radius   = 0;             // Not used
    new_tool.m_Rotation = aRotDegree;
    new_tool.m_DCode    = last_D_code + 1;
    new_tool.m_ApertureAttribute = aApertureAttribute;

    m_apertures.push_back( new_tool );

    return m_apertures.size() - 1;
}


void GERBER_PLOTTER::selectAperture( const wxSize& aSize, int aRadius, double aRotDegree,
                                     APERTURE::APERTURE_TYPE aType,
                                     int aApertureAttribute )
{
    bool change = ( m_currentApertureIdx < 0 ) ||
                  ( m_apertures[m_currentApertureIdx].m_Type != aType ) ||
                  ( m_apertures[m_currentApertureIdx].m_Size != aSize ) ||
                  ( m_apertures[m_currentApertureIdx].m_Radius != aRadius ) ||
                  ( m_apertures[m_currentApertureIdx].m_Rotation != aRotDegree );

    if( !change )
        change = m_apertures[m_currentApertureIdx].m_ApertureAttribute != aApertureAttribute;

    if( change )
    {
        // Pick an existing aperture or create a new one
        m_currentApertureIdx = GetOrCreateAperture( aSize, aRadius, aRotDegree,
                                                    aType, aApertureAttribute );
        fprintf( m_outputFile, "D%d*\n", m_apertures[m_currentApertureIdx].m_DCode );
    }
}


void GERBER_PLOTTER::selectAperture( const std::vector<wxPoint>& aCorners, double aRotDegree,
                         APERTURE::APERTURE_TYPE aType, int aApertureAttribute )
{
    bool change = ( m_currentApertureIdx < 0 ) ||
                  ( m_apertures[m_currentApertureIdx].m_Type != aType ) ||
                  ( m_apertures[m_currentApertureIdx].m_Corners.size() != aCorners.size() ) ||
                  ( m_apertures[m_currentApertureIdx].m_Rotation != aRotDegree );

    if( !change )   // Compare corner lists
    {
        for( size_t ii = 0; ii < aCorners.size(); ii++ )
        {
            if( aCorners[ii] != m_apertures[m_currentApertureIdx].m_Corners[ii] )
            {
                change = true;
                break;
            }
        }
    }

    if( !change )
        change = m_apertures[m_currentApertureIdx].m_ApertureAttribute != aApertureAttribute;

    if( change )
    {
        // Pick an existing aperture or create a new one
        m_currentApertureIdx = GetOrCreateAperture( aCorners, aRotDegree,
                                                    aType, aApertureAttribute );
        fprintf( m_outputFile, "D%d*\n", m_apertures[m_currentApertureIdx].m_DCode );
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

    wxSize size( aDiameter, (int)( aPolygonRotation * 1000.0 ) );
    selectAperture( wxSize( 0, 0), aDiameter/2, aPolygonRotation, aType, aApertureAttribute );
}

void GERBER_PLOTTER::writeApertureList()
{
    wxASSERT( m_outputFile );
    char cbuf[1024];
    std::string buffer;

    bool useX1StructuredComment = false;

    if( !m_useX2format )
        useX1StructuredComment = true;

    // Init
    for( APERTURE& tool : m_apertures )
    {
        // apertude sizes are in inch or mm, regardless the
        // coordinates format
        double fscale = 0.0001 * m_plotScale / m_IUsPerDecimil; // inches

        if(! m_gerberUnitInch )
            fscale *= 25.4;     // size in mm

        int attribute = tool.m_ApertureAttribute;

        if( attribute != m_apertureAttribute )
        {
            fputs( GBR_APERTURE_METADATA::FormatAttribute(
                    (GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB) attribute,
                            useX1StructuredComment ).c_str(), m_outputFile );
        }

        sprintf( cbuf, "%%ADD%d", tool.m_DCode );
        buffer = cbuf;

        /* Please note: the Gerber specs for mass parameters say that
           exponential syntax is *not* allowed and the decimal point should
           also be always inserted. So the %g format is ruled out, but %f is fine
           (the # modifier forces the decimal point). Sadly the %f formatter
           can't remove trailing zeros but thats not a problem, since nothing
           forbid it (the file is only slightly longer) */

        switch( tool.m_Type )
        {
        case APERTURE::AT_CIRCLE:
            sprintf( cbuf, "C,%#f*%%\n", tool.GetDiameter() * fscale );
            break;

        case APERTURE::AT_RECT:
            sprintf( cbuf, "R,%#fX%#f*%%\n", tool.m_Size.x * fscale,
                                             tool.m_Size.y * fscale );
            break;

        case APERTURE::AT_PLOTTING:
            sprintf( cbuf, "C,%#f*%%\n", tool.m_Size.x * fscale );
            break;

        case APERTURE::AT_OVAL:
            sprintf( cbuf, "O,%#fX%#f*%%\n", tool.m_Size.x * fscale,
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
            sprintf( cbuf, "P,%#fX%dX%#f*%%\n", tool.GetDiameter() * fscale,
                     tool.GetRegPolyVerticeCount(), tool.GetRotation() );
            break;

        case APERTURE::AM_ROUND_RECT:       // Aperture macro for round rect pads
            {
            // The aperture macro needs coordinates of the centers of the 4 corners
            std::vector<VECTOR2I> corners;
            wxSize half_size( tool.m_Size.x/2-tool.m_Radius, tool.m_Size.y/2-tool.m_Radius );

            corners.emplace_back( -half_size.x, -half_size.y );
            corners.emplace_back( half_size.x, -half_size.y );
            corners.emplace_back( half_size.x, half_size.y );
            corners.emplace_back( -half_size.x, half_size.y );

            // Rotate the corner coordinates:
            for( int ii = 0; ii < 4; ii++ )
                RotatePoint( corners[ii], -tool.m_Rotation*10.0 );

            sprintf( cbuf, "%s,%#fX", APER_MACRO_ROUNDRECT_NAME,
                     tool.m_Radius * fscale );
            buffer += cbuf;

            // Add each corner
            for( int ii = 0; ii < 4; ii++ )
            {
                sprintf( cbuf, "%#fX%#fX",
                         corners[ii].x * fscale, corners[ii].y * fscale );
                buffer += cbuf;
            }

            sprintf( cbuf, "0*%%\n" );
            }
            break;

        case APERTURE::AM_ROT_RECT:         // Aperture macro for rotated rect pads
            sprintf( cbuf, "%s,%#fX%#fX%#f*%%\n", APER_MACRO_ROT_RECT_NAME,
                     tool.m_Size.x * fscale, tool.m_Size.y * fscale,
                     tool.m_Rotation );
            break;

        case APERTURE::APER_MACRO_OUTLINE4P:    // Aperture macro for trapezoid pads
        case APERTURE::APER_MACRO_OUTLINE5P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE6P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE7P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE8P:    // Aperture macro for chamfered rect pads
            switch( tool.m_Type )
            {
            case APERTURE::APER_MACRO_OUTLINE4P:
                sprintf( cbuf, "%s,", APER_MACRO_OUTLINE4P_NAME ); break;
            case APERTURE::APER_MACRO_OUTLINE5P:
                sprintf( cbuf, "%s,", APER_MACRO_OUTLINE5P_NAME ); break;
            case APERTURE::APER_MACRO_OUTLINE6P:
                sprintf( cbuf, "%s,", APER_MACRO_OUTLINE6P_NAME ); break;
            case APERTURE::APER_MACRO_OUTLINE7P:
                sprintf( cbuf, "%s,", APER_MACRO_OUTLINE7P_NAME ); break;
            case APERTURE::APER_MACRO_OUTLINE8P:
                sprintf( cbuf, "%s,", APER_MACRO_OUTLINE8P_NAME ); break;
            default:
                break;
            }

            buffer += cbuf;

            // Output all corners (should be 4 to 8 corners)
            // Remember: the Y coordinate must be negated, due to the fact in Pcbnew
            // the Y axis is from top to bottom
            for( size_t ii = 0; ii < tool.m_Corners.size(); ii++ )
            {
                sprintf( cbuf, "%#fX%#fX",
                         tool.m_Corners[ii].x * fscale, -tool.m_Corners[ii].y * fscale );
                buffer += cbuf;
            }

            // close outline and output rotation
            sprintf( cbuf, "%#f*%%\n", tool.m_Rotation );
            break;

        case APERTURE::AM_ROTATED_OVAL:         // Aperture macro for rotated oval pads
                                                // (not rotated is a primitive)
            // m_Size.x = full lenght; m_Size.y = width, and the macro aperure expects
            // the position of ends
            {
                // the seg_len is the distance between the 2 circle centers
                int seg_len = tool.m_Size.x - tool.m_Size.y;
                // Center of the circle on the segment start point:
                VECTOR2I start( seg_len/2, 0 );
                // Center of the circle on the segment end point:
                VECTOR2I end( - seg_len/2, 0 );

                RotatePoint( start, tool.m_Rotation*10.0 );
                RotatePoint( end, tool.m_Rotation*10.0 );

                sprintf( cbuf, "%s,%#fX%#fX%#fX%#fX%#fX0*%%\n", APER_MACRO_SHAPE_OVAL_NAME,
                         tool.m_Size.y * fscale,                // width
                         start.x * fscale, -start.y * fscale,   // X,Y corner start pos
                         end.x * fscale, -end.y * fscale );     // X,Y cornerend  pos
            }
            break;

            case APERTURE::AM_FREE_POLYGON:
            {
                // Find the aperture macro name in the list of aperture macro
                // created on the fly for this polygon:
                int idx = m_am_freepoly_list.FindAm( tool.m_Corners );

                // Write DCODE id ( "%ADDxx" is already in buffer) and rotation
                // the full line is something like :%ADD12FreePoly1,45.000000*%
                sprintf( cbuf, "%s%d,%#f*%%\n", AM_FREEPOLY_BASENAME, idx, tool.m_Rotation );
            }
            break;
        }

        buffer += cbuf;
        fputs( buffer.c_str(), m_outputFile );

        m_apertureAttribute = attribute;

        // Currently reset the aperture attribute. Perhaps a better optimization
        // is to store the last attribute
        if( attribute )
        {
            if( m_useX2format )
                fputs( "%TD*%\n", m_outputFile );
            else
                fputs( "G04 #@! TD*\n", m_outputFile );

            m_apertureAttribute = 0;
        }

    }
}


void GERBER_PLOTTER::PenTo( const wxPoint& aPos, char plume )
{
    wxASSERT( m_outputFile );
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

    m_penState = plume;
}


void GERBER_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill, int width )
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


void GERBER_PLOTTER::Circle( const wxPoint& aCenter, int aDiameter, FILL_TYPE aFill, int aWidth )
{
    Arc( aCenter, 0, 3600, aDiameter / 2, aFill, aWidth );
}



void GERBER_PLOTTER::Arc( const wxPoint& aCenter, double aStAngle, double aEndAngle,
                          int aRadius, FILL_TYPE aFill, int aWidth )
{
    SetCurrentLineWidth( aWidth );

    // aFill is not used here.
    plotArc( aCenter, aStAngle, aEndAngle, aRadius, false );
}

void GERBER_PLOTTER::Arc( const SHAPE_ARC& aArc )
{
    SetCurrentLineWidth( aArc.GetWidth() );

    // aFill is not used here.
    plotArc( aArc, false );
}


void GERBER_PLOTTER::plotArc( const SHAPE_ARC& aArc, bool aPlotInRegion )
{
    wxPoint start( aArc.GetP0() );
    wxPoint end( aArc.GetP1() );
    wxPoint center( aArc.GetCenter() );
    double start_angle = aArc.GetStartAngle();
    double end_angle = aArc.GetEndAngle();

    if( !aPlotInRegion )
        MoveTo( start);
    else
        LineTo( start );

    DPOINT devEnd = userToDeviceCoordinates( end );
    DPOINT devCenter = userToDeviceCoordinates( center ) - userToDeviceCoordinates( start );

    if( start_angle < end_angle )
        fprintf( m_outputFile, "G03*\n" );    // Active circular interpolation, CCW
    else
        fprintf( m_outputFile, "G02*\n" );    // Active circular interpolation, CW

    fprintf( m_outputFile, "X%dY%dI%dJ%dD01*\n",
             KiROUND( devEnd.x ), KiROUND( devEnd.y ),
             KiROUND( devCenter.x ), KiROUND( devCenter.y ) );

    fprintf( m_outputFile, "G01*\n" ); // Back to linear interpol (perhaps useless here).
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

    fprintf( m_outputFile, "G75*\n" );        // Multiquadrant (360 degrees) mode

    if( aStAngle < aEndAngle )
        fprintf( m_outputFile, "G03*\n" );    // Active circular interpolation, CCW
    else
        fprintf( m_outputFile, "G02*\n" );    // Active circular interpolation, CW

    fprintf( m_outputFile, "X%dY%dI%dJ%dD01*\n",
             KiROUND( devEnd.x ), KiROUND( devEnd.y ),
             KiROUND( devCenter.x ), KiROUND( devCenter.y ) );

    fprintf( m_outputFile, "G01*\n" ); // Back to linear interpol (perhaps useless here).
}


void GERBER_PLOTTER::PlotGerberRegion( const SHAPE_LINE_CHAIN& aPoly,
                                 void* aData )
{
    if( aPoly.PointCount() <= 2 )
        return;

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

    if( gbr_metadata )
    {
        std::string attrib = gbr_metadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

        if( !attrib.empty() )
        {
            fputs( attrib.c_str(), m_outputFile );
            clearTA_AperFunction = true;
        }
    }

    PlotPoly( aPoly, FILL_TYPE::FILLED_SHAPE, 0 , gbr_metadata );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
        {
            fputs( "%TD.AperFunction*%\n", m_outputFile );
        }
        else
        {
            fputs( "G04 #@! TD.AperFunction*\n", m_outputFile );
        }
    }
}


void GERBER_PLOTTER::PlotGerberRegion( const std::vector< wxPoint >& aCornerList,
                                 void* aData )
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
            fputs( attrib.c_str(), m_outputFile );
            clearTA_AperFunction = true;
        }
    }

    PlotPoly( aCornerList, FILL_TYPE::FILLED_SHAPE, 0, gbr_metadata );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
        {
            fputs( "%TD.AperFunction*%\n", m_outputFile );
        }
        else
        {
            fputs( "G04 #@! TD.AperFunction*\n", m_outputFile );
        }
    }
}

void GERBER_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aPoly,
                               FILL_TYPE aFill, int aWidth, void* aData )
{
    if( aPoly.CPoints().size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Therefore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( aFill != FILL_TYPE::NO_FILL )
    {
        fputs( "G36*\n", m_outputFile );

        MoveTo( wxPoint( aPoly.CPoint( 0 ) ) );

        fputs( "G01*\n", m_outputFile );      // Set linear interpolation.

        for( int ii = 1; ii < aPoly.PointCount(); ii++ )
        {
            int arcindex = aPoly.ArcIndex( ii );

            if( arcindex < 0 )
            {
                /// Plain point
                LineTo( wxPoint( aPoly.CPoint( ii ) ) );
            }
            else
            {
                const SHAPE_ARC& arc = aPoly.Arc( arcindex );

                plotArc( arc, ii > 0 );
            }
        }

        // If the polygon is not closed, close it:
        if( aPoly.CPoint( 0 ) != aPoly.CPoint( -1 ) )
            FinishTo( wxPoint( aPoly.CPoint( 0 ) ) );

        fputs( "G37*\n", m_outputFile );
    }

    if( aWidth > 0 )    // Draw the polyline/polygon outline
    {
        SetCurrentLineWidth( aWidth, gbr_metadata );

        MoveTo( wxPoint( aPoly.CPoint( 0 ) ) );

        for( int ii = 1; ii < aPoly.PointCount(); ii++ )
        {
            int arcindex = aPoly.ArcIndex( ii );

            if( arcindex < 0 )
            {
                /// Plain point
                LineTo( wxPoint( aPoly.CPoint( ii ) ) );
            }
            else
            {
                const SHAPE_ARC& arc = aPoly.Arc( arcindex );

                plotArc( arc, ii > 0 );
            }
        }

        // Ensure the thick outline is closed for filled polygons
        // (if not filled, could be only a polyline)
        if( aFill != FILL_TYPE::NO_FILL && ( aPoly.CPoint( 0 ) != aPoly.CPoint( -1 ) ) )
            LineTo( wxPoint( aPoly.CPoint( 0 ) ) );

        PenFinish();
    }
}

void GERBER_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                               FILL_TYPE aFill, int aWidth, void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Therefore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( aFill != FILL_TYPE::NO_FILL )
    {
        fputs( "G36*\n", m_outputFile );

        MoveTo( aCornerList[0] );
        fputs( "G01*\n", m_outputFile );      // Set linear interpolation.

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // If the polygon is not closed, close it:
        if( aCornerList[0] != aCornerList[aCornerList.size()-1] )
            FinishTo( aCornerList[0] );

        fputs( "G37*\n", m_outputFile );
    }

    if( aWidth > 0 )    // Draw the polyline/polygon outline
    {
        SetCurrentLineWidth( aWidth, gbr_metadata );

        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Ensure the thick outline is closed for filled polygons
        // (if not filled, could be only a polyline)
        if( aFill != FILL_TYPE::NO_FILL &&( aCornerList[aCornerList.size() - 1] != aCornerList[0] ) )
            LineTo( aCornerList[0] );

        PenFinish();
    }
}


void GERBER_PLOTTER::ThickSegment( const wxPoint& start, const wxPoint& end, int width,
                            OUTLINE_MODE tracemode, void* aData )
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
                           int radius, int width, OUTLINE_MODE tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Arc( centre, StAngle, EndAngle, radius, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        Arc( centre, StAngle, EndAngle,
             radius - ( width - m_currentPenWidth ) / 2, FILL_TYPE::NO_FILL,
             DO_NOT_SET_LINE_WIDTH );
        Arc( centre, StAngle, EndAngle, radius + ( width - m_currentPenWidth ) / 2, FILL_TYPE::NO_FILL,
             DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::ThickRect( const wxPoint& p1, const wxPoint& p2, int width,
                            OUTLINE_MODE tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Rect( p1, p2, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        wxPoint offsetp1( p1.x - (width - m_currentPenWidth) / 2,
                          p1.y - (width - m_currentPenWidth) / 2 );
        wxPoint offsetp2( p2.x + (width - m_currentPenWidth) / 2,
              p2.y + (width - m_currentPenWidth) / 2 );
        Rect( offsetp1, offsetp2, FILL_TYPE::NO_FILL, -1 );
        offsetp1.x += (width - m_currentPenWidth);
        offsetp1.y += (width - m_currentPenWidth);
        offsetp2.x -= (width - m_currentPenWidth);
        offsetp2.y -= (width - m_currentPenWidth);
        Rect( offsetp1, offsetp2, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::ThickCircle( const wxPoint& pos, int diametre, int width,
                              OUTLINE_MODE tracemode, void* aData )
{
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );
    SetCurrentLineWidth( width, gbr_metadata );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
        Circle( pos, diametre, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH, gbr_metadata );
        Circle( pos, diametre - (width - m_currentPenWidth),
                    FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
        Circle( pos, diametre + (width - m_currentPenWidth),
                    FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::FilledCircle( const wxPoint& pos, int diametre,
                              OUTLINE_MODE tracemode, void* aData )
{
    // A filled circle is a graphic item, not a pad.
    // So it is drawn, not flashed.
    GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( tracemode == FILLED )
    {
        // Draw a circle of diameter = diametre/2 with a line thickness = radius,
        // To create a filled circle
        SetCurrentLineWidth( diametre/2, gbr_metadata );
        Circle( pos, diametre/2, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH, gbr_metadata );
        Circle( pos, diametre, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
}


void GERBER_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre, OUTLINE_MODE trace_mode, void* aData )
{
    wxSize size( diametre, diametre );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( trace_mode == SKETCH )
    {
        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );

        Circle( pos, diametre - m_currentPenWidth, FILL_TYPE::NO_FILL, DO_NOT_SET_LINE_WIDTH );
    }
    else
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );

        int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
        selectAperture( size, 0, 0.0, APERTURE::AT_CIRCLE, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
    }
}


void GERBER_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                   OUTLINE_MODE trace_mode, void* aData )
{
    wxASSERT( m_outputFile );
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
        selectAperture( size, 0, 0.0, APERTURE::AT_OVAL, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
    }
    else    // Plot pad as region.
            // Only regions and flashed items accept a object attribute TO.P for the pin name
    {
        if( trace_mode == FILLED )
        {
        #ifdef GBR_USE_MACROS_FOR_ROTATED_OVAL
            if( !m_gerberDisableApertMacros )
        #endif
            {
                m_hasApertureRotOval = true;
                // We are using a aperture macro that expect size.y < size.x
                // i.e draw a horizontal line for rotation = 0.0
                // size.x = length, size.y = width
                if( size.x < size.y )
                {
                    std::swap( size.x, size.y );
                    orient += 900;

                    if( orient > 1800 )
                        orient -= 1800;
                }

                DPOINT pos_dev = userToDeviceCoordinates( pos );
                int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
                selectAperture( size, 0, orient/10.0, APERTURE::AM_ROTATED_OVAL, aperture_attrib );

                if( gbr_metadata )
                    formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

                emitDcode( pos_dev, 3 );
                return;
            }
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
                                   double orient, OUTLINE_MODE trace_mode, void* aData )

{
    wxASSERT( m_outputFile );
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

            SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
            Rect( wxPoint( pos.x - (size.x - GetCurrentLineWidth()) / 2,
                           pos.y - (size.y - GetCurrentLineWidth()) / 2 ),
                  wxPoint( pos.x + (size.x - GetCurrentLineWidth()) / 2,
                           pos.y + (size.y - GetCurrentLineWidth()) / 2 ),
                  FILL_TYPE::NO_FILL, GetCurrentLineWidth() );
        }
        else
        {
            DPOINT pos_dev = userToDeviceCoordinates( pos );
            int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
            selectAperture( size, 0, 0.0, APERTURE::AT_RECT, aperture_attrib );

            if( gbr_metadata )
                formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

            emitDcode( pos_dev, 3 );
        }
        break;

    default:
    #ifdef GBR_USE_MACROS_FOR_ROTATED_RECT
        if( trace_mode != SKETCH && !m_gerberDisableApertMacros )
        {
            m_hasApertureRotRect = true;
            DPOINT pos_dev = userToDeviceCoordinates( pos );
            int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
            selectAperture( size, 0, orient/10.0, APERTURE::AM_ROT_RECT, aperture_attrib );

            if( gbr_metadata )
                formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

            emitDcode( pos_dev, 3 );

            break;
        }
    #endif
        {
        // plot pad shape as Gerber region
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
                                     OUTLINE_MODE aTraceMode, void* aData )

{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( aTraceMode != FILLED )
    {
        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient,
                                     aCornerRadius, 0.0, 0, GetPlotterArcHighDef(), ERROR_INSIDE );

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
        PlotPoly( cornerList, FILL_TYPE::NO_FILL, GetCurrentLineWidth(), gbr_metadata );
    }
    else
    {
    #ifdef GBR_USE_MACROS_FOR_ROUNDRECT
        if( !m_gerberDisableApertMacros )
    #endif
        {
            m_hasApertureRoundRect = true;

            DPOINT pos_dev = userToDeviceCoordinates( aPadPos );
            int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
            selectAperture( aSize, aCornerRadius, aOrient/10.0,
                            APERTURE::AM_ROUND_RECT, aperture_attrib );

            if( gbr_metadata )
                formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

            emitDcode( pos_dev, 3 );
            return;
        }
        // A Pad RoundRect is plotted as a Gerber region.
        // Initialize region metadata:
        bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

        if( gbr_metadata )
        {
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );
            std::string attrib = gbr_metadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

            if( !attrib.empty() )
            {
                fputs( attrib.c_str(), m_outputFile );
                clearTA_AperFunction = true;
            }
        }

        // Plot the region using arcs in corners.
        plotRoundRectAsRegion( aPadPos, aSize, aCornerRadius, aOrient );

        // Clear the TA attribute, to avoid the next item to inherit it:
        if( clearTA_AperFunction )
        {
            if( m_useX2format )
                fputs( "%TD.AperFunction*%\n", m_outputFile );
            else
                fputs( "G04 #@! TD.AperFunction*\n", m_outputFile );
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

    fputs( "G36*\n", m_outputFile );  // Start region
    fputs( "G01*\n", m_outputFile );  // Set linear interpolation.
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

    fputs( "G37*\n", m_outputFile );      // Close region
}


void GERBER_PLOTTER::FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                     double aOrient, SHAPE_POLY_SET* aPolygons,
                                     OUTLINE_MODE aTraceMode, void* aData )

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
            PlotPoly( cornerList, FILL_TYPE::NO_FILL, GetCurrentLineWidth(), &gbr_metadata );
        else
        {
#ifdef GBR_USE_MACROS_FOR_CUSTOM_PAD
            if( m_gerberDisableApertMacros
                    || cornerList.size() > GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT )
                PlotGerberRegion( cornerList, &gbr_metadata );
            else
            {
               // An AM will be created. the shape must be in position 0,0 and orientation 0
                // to be able to reuse the same AM for pads having the same shape
                for( size_t ii = 0; ii < cornerList.size(); ii++ )
                {
                    cornerList[ii] -= aPadPos;
                    RotatePoint( &cornerList[ii], -aOrient );
                }

                DPOINT pos_dev = userToDeviceCoordinates( aPadPos );
                selectAperture( cornerList, aOrient/10.0,
                                APERTURE::AM_FREE_POLYGON, gbr_metadata.GetApertureAttrib() );
                formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

                emitDcode( pos_dev, 3 );
            }
#else
            PlotGerberRegion( cornerList, &gbr_metadata );
#endif
        }
    }
}


void GERBER_PLOTTER::FlashPadChamferRoundRect( const wxPoint& aShapePos, const wxSize& aPadSize,
                                   int aCornerRadius, double aChamferRatio,
                                   int aChamferPositions,
                                   double aPadOrient, OUTLINE_MODE aPlotMode, void* aData )

{
    GBR_METADATA gbr_metadata;

    if( aData )
        gbr_metadata = *static_cast<GBR_METADATA*>( aData );

    DPOINT pos_dev = userToDeviceCoordinates( aShapePos );

    SHAPE_POLY_SET outline;
    // polygon corners list
    std::vector<wxPoint> cornerList;

    bool hasRoundedCorner = aCornerRadius != 0 && aChamferPositions != 15;

#ifdef GBR_USE_MACROS_FOR_CHAMFERED_RECT
    // Sketch mode or round rect shape or Apert Macros disabled
    if( aPlotMode != FILLED || hasRoundedCorner || m_gerberDisableApertMacros )
#endif
    {
        TransformRoundChamferedRectToPolygon( outline, aShapePos, aPadSize, aPadOrient,
                                              aCornerRadius, aChamferRatio, aChamferPositions,
                                              GetPlotterArcHighDef(), ERROR_INSIDE );

        // Build the corner list
        const SHAPE_LINE_CHAIN& corners = outline.Outline(0);

        for( int ii = 0; ii < corners.PointCount(); ii++ )
            cornerList.emplace_back( corners.CPoint( ii ).x, corners.CPoint( ii ).y );

        // Close the polygon
        cornerList.push_back( cornerList[0] );

        if( aPlotMode == SKETCH )
            PlotPoly( cornerList, FILL_TYPE::NO_FILL, GetCurrentLineWidth(), &gbr_metadata );
        else
        {
#ifdef GBR_USE_MACROS_FOR_CHAMFERED_ROUND_RECT
            if( m_gerberDisableApertMacros )
                PlotGerberRegion( cornerList, &gbr_metadata );
            else
            {
               // An AM will be created. the shape must be in position 0,0 and orientation 0
                // to be able to reuse the same AM for pads having the same shape
                for( size_t ii = 0; ii < cornerList.size(); ii++ )
                {
                    cornerList[ii] -= aShapePos;
                    RotatePoint( &cornerList[ii], -aPadOrient );
                }

                selectAperture( cornerList, aPadOrient/10.0,
                                APERTURE::AM_FREE_POLYGON, gbr_metadata.GetApertureAttrib() );
                formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

                emitDcode( pos_dev, 3 );
            }
#else
            PlotGerberRegion( cornerList, &gbr_metadata );
#endif
        }

        return;
    }

    // Build the chamfered polygon (4 to 8 corners )
    TransformRoundChamferedRectToPolygon( outline, wxPoint( 0, 0 ), aPadSize, 0.0, 0,
                                          aChamferRatio, aChamferPositions,
                                          GetPlotterArcHighDef(), ERROR_INSIDE );

    // Build the corner list
    const SHAPE_LINE_CHAIN& corners = outline.Outline(0);

    // Generate the polygon (4 to 8 corners )
    for( int ii = 0; ii < corners.PointCount(); ii++ )
        cornerList.emplace_back( corners.CPoint( ii ).x, corners.CPoint( ii ).y );

    switch( cornerList.size() )
    {
    case 4:
        m_hasApertureOutline4P = true;
        selectAperture( cornerList, aPadOrient/10.0,
                        APERTURE::APER_MACRO_OUTLINE4P, gbr_metadata.GetApertureAttrib() );
        break;

    case 5:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient/10.0,
                        APERTURE::APER_MACRO_OUTLINE5P, gbr_metadata.GetApertureAttrib() );
        break;

    case 6:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient/10.0,
                        APERTURE::APER_MACRO_OUTLINE6P, gbr_metadata.GetApertureAttrib() );
        break;

    case 7:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient/10.0,
                        APERTURE::APER_MACRO_OUTLINE7P, gbr_metadata.GetApertureAttrib() );
        break;

    case 8:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient/10.0,
                        APERTURE::APER_MACRO_OUTLINE8P, gbr_metadata.GetApertureAttrib() );
        break;

    default:
        wxLogMessage( "FlashPadChamferRoundRect(): Unexpected number of corners (%d)",
                      (int)cornerList.size() );
        break;
    }

    formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

    emitDcode( pos_dev, 3 );
}


void GERBER_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos,  const wxPoint* aCorners,
                                     double aPadOrient, OUTLINE_MODE aTrace_Mode, void* aData )

{
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
    {
        PlotPoly( cornerList, FILL_TYPE::NO_FILL, GetCurrentLineWidth(), &metadata );
        return;
    }

    // Plot a filled polygon:
    #ifdef GBR_USE_MACROS_FOR_TRAPEZOID
    if( !m_gerberDisableApertMacros )
    #endif
    {
        m_hasApertureOutline4P = true;
        DPOINT pos_dev = userToDeviceCoordinates( aPadPos );
        // polygon corners list
        std::vector<wxPoint> corners = { aCorners[0], aCorners[1], aCorners[2], aCorners[3] };
        int aperture_attrib = gbr_metadata ? gbr_metadata->GetApertureAttrib() : 0;
        selectAperture( corners, aPadOrient/10.0, APERTURE::APER_MACRO_OUTLINE4P, aperture_attrib );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
        return;
    }

    PlotGerberRegion( cornerList, &metadata );
}


void GERBER_PLOTTER::FlashRegularPolygon( const wxPoint& aShapePos,
                                          int aDiameter, int aCornerCount,
                                          double aOrient, OUTLINE_MODE aTraceMode,
                                          void* aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    GBR_METADATA metadata;

    if( gbr_metadata )
        metadata = *gbr_metadata;

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

        PlotPoly( cornerList, FILL_TYPE::NO_FILL, GetCurrentLineWidth(), &gbr_metadata );
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
        fprintf( m_outputFile, "%%LPD*%%\n" );
    else
        fprintf( m_outputFile, "%%LPC*%%\n" );
}


bool APER_MACRO_FREEPOLY::IsSamePoly( const std::vector<wxPoint>& aPolygon ) const
{
    return polyCompare( m_Corners, aPolygon );
}


void APER_MACRO_FREEPOLY::Format( FILE * aOutput, double aIu2GbrMacroUnit )
{
   // Write aperture header
    fprintf( aOutput, "%%AM%s%d*\n", AM_FREEPOLY_BASENAME, m_Id );
    fprintf( aOutput, "4,1,%d,", (int)m_Corners.size() );

    // Insert a newline after curr_line_count_max coordinates.
    int curr_line_corner_count = 0;
    const int curr_line_count_max = 20;     // <= 0 to disable newlines

    for( size_t ii = 0; ii <= m_Corners.size(); ii++ )
    {
        int jj = ii;

        if( ii >= m_Corners.size() )
            jj = 0;

        // Note: parameter values are always mm or inches
        fprintf( aOutput, "%#f,%#f,",
                 m_Corners[jj].x * aIu2GbrMacroUnit, -m_Corners[jj].y * aIu2GbrMacroUnit );

        if( curr_line_count_max >= 0
                && ++curr_line_corner_count >= curr_line_count_max )
        {
            fprintf( aOutput, "\n" );
            curr_line_corner_count = 0;
        }
    }

    // output rotation parameter
    fputs( "$1*%\n", aOutput );
}


void APER_MACRO_FREEPOLY_LIST::Format( FILE * aOutput, double aIu2GbrMacroUnit )
{
    for( int idx = 0; idx < AmCount(); idx++ )
        m_AMList[idx].Format( aOutput, aIu2GbrMacroUnit );
}


void APER_MACRO_FREEPOLY_LIST::Append( const std::vector<wxPoint>& aPolygon )
{
    m_AMList.emplace_back( aPolygon, AmCount() );
}


int APER_MACRO_FREEPOLY_LIST::FindAm( const std::vector<wxPoint>& aPolygon ) const
{
    for( int idx = 0; idx < AmCount(); idx++ )
    {
        if( m_AMList[idx].IsSamePoly( aPolygon ) )
            return idx;
    }

    return -1;
}
